/*  Copyright 2011, 2012 The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// local:
#include "OpenCL_RD.hpp"
#include "OpenCL_utils.hpp"
#include "utils.hpp"

// STL:
#include <vector>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <cassert>
using namespace std;

// VTK:
#include <vtkImageData.h>

OpenCL_RD::OpenCL_RD()
{
    this->iPlatform = 0;
    this->iDevice = 0;
    this->need_reload_context = true;
    this->kernel_function_name = "rd_compute";

    // initialise the opencl things to null in case we fail to create them
    this->device_id = NULL;
    this->context = NULL;
    this->command_queue = NULL;
    this->kernel = NULL;

    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");
        
    this->iCurrentBuffer = 0;
}

OpenCL_RD::~OpenCL_RD()
{
    clReleaseContext(this->context);
    clReleaseCommandQueue(this->command_queue);
    clReleaseKernel(this->kernel);
    for(int io=0;io<2;io++)
        for(int i=0;i<(int)this->buffers[io].size();i++)
            clReleaseMemObject(this->buffers[io][i]);
}

void OpenCL_RD::SetPlatform(int i)
{
    if(i != this->iPlatform)
        this->need_reload_context = true;
    this->iPlatform = i;
}

void OpenCL_RD::SetDevice(int i)
{
    if(i != this->iDevice)
        this->need_reload_context = true;
    this->iDevice = i;
}

int OpenCL_RD::GetPlatform() const
{
    return this->iPlatform;
}

int OpenCL_RD::GetDevice() const
{
    return this->iDevice;
}

void OpenCL_RD::ReloadContextIfNeeded()
{
    if(!this->need_reload_context) return;

    cl_int ret;

    // retrieve our chosen platform
    cl_platform_id platform_id;
    {
        const int MAX_PLATFORMS = 10;
        cl_platform_id platforms_available[MAX_PLATFORMS];
        cl_uint num_platforms;
        ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
        if(ret != CL_SUCCESS || num_platforms==0)
        {
            throw runtime_error("No OpenCL platforms available");
            // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
        }
        if(this->iPlatform >= (int)num_platforms)
            throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few platforms available");
        platform_id = platforms_available[this->iPlatform];
    }

    // retrieve our chosen device
    {
        const int MAX_DEVICES = 10;
        cl_device_id devices_available[MAX_DEVICES];
        cl_uint num_devices;
        ret = clGetDeviceIDs(platform_id,CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
        throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to retrieve device IDs: ");
        if(this->iDevice >= (int)num_devices)
            throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few devices available");
        this->device_id = devices_available[this->iDevice];
    }

    // create the context
    this->context = clCreateContext(NULL,1,&this->device_id,NULL,NULL,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to create context: ");

    // create the command queue
    this->command_queue = clCreateCommandQueue(this->context,this->device_id,0,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to create command queue: ");

    this->need_reload_context = false;
}

void OpenCL_RD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_formula) return;

    cl_int ret;

    // create the program
    this->kernel_source = this->AssembleKernelSourceFromFormula(this->formula);
    const char *source = this->kernel_source.c_str();
    size_t source_size = this->kernel_source.length();
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : Failed to create program with source: ");

    // make a set of options to pass to the compiler
    ostringstream options;
    //options << "-cl-denorms-are-zero -cl-fast-relaxed-math";

    // build the program
    ret = clBuildProgram(program,1,&this->device_id,options.str().c_str(),NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCL_RD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_RD::ReloadKernelIfNeeded : build failed (kernel saved as kernel.txt):\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }

    // create the kernel
    this->kernel = clCreateKernel(program,this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : kernel creation failed: ");

    this->global_range[0] = max(1,this->GetX() / this->GetBlockSizeX());
    this->global_range[1] = max(1,this->GetY() / this->GetBlockSizeY());
    this->global_range[2] = max(1,this->GetZ() / this->GetBlockSizeZ());
    // (we let the local work group size be automatically decided, seems to be faster and more flexible that way)

    this->need_reload_formula = false;
}

void OpenCL_RD::CreateOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();
    const int NC = this->GetNumberOfChemicals();

    cl_int ret;

    for(int io=0;io<2;io++) // we create two buffers for each chemical, and switch between them
    {
        this->buffers[io].resize(NC);
        for(int ic=0;ic<NC;ic++)
        {
            this->buffers[io][ic] = clCreateBuffer(this->context, CL_MEM_READ_WRITE, MEM_SIZE, NULL, &ret);
            throwOnError(ret,"OpenCL_RD::CreateBuffers : buffer creation failed: ");
        }
    }
}

void OpenCL_RD::WriteToOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();

    this->iCurrentBuffer = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::WriteToBuffers : buffer writing failed: ");
    }
}

void OpenCL_RD::TestFormula(std::string formula)
{
    this->need_reload_context = true;
    this->ReloadContextIfNeeded();

    string kernel_source = this->AssembleKernelSourceFromFormula(formula);

    cl_int ret;

    // create the program
    const char *source = kernel_source.c_str();
    size_t source_size = kernel_source.length();
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_RD::TestProgram : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(program,1,&this->device_id,NULL,NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCL_RD::TestProgram : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_RD::TestProgram : build failed: (kernel saved as kernel.txt)\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }
}

void OpenCL_RD::CopyFromImage(vtkImageData* im)
{
    ImageRD::CopyFromImage(im);
    this->WriteToOpenCLBuffers();
}

void OpenCL_RD::SetParameterValue(int iParam,float val)
{
    ImageRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

void OpenCL_RD::SetParameterName(int iParam,const string& s)
{
    ImageRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

void OpenCL_RD::AddParameter(const std::string& name,float val)
{
    ImageRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

void OpenCL_RD::DeleteParameter(int iParam)
{
    ImageRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

void OpenCL_RD::DeleteAllParameters()
{
    ImageRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

void OpenCL_RD::GenerateInitialPattern()
{
	ImageRD::GenerateInitialPattern();
    this->WriteToOpenCLBuffers();
}

void OpenCL_RD::BlankImage()
{
	ImageRD::BlankImage();
    this->WriteToOpenCLBuffers();
}

void OpenCL_RD::AllocateImages(int x,int y,int z,int nc)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCL_RD::Allocate : for wrap-around in OpenCL we require all the dimensions to be powers of 2");
    ImageRD::AllocateImages(x,y,z,nc);
    this->need_reload_formula = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL_RD::InternalUpdate(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();

    cl_int ret;
    int iBuffer;
    const int NC = this->GetNumberOfChemicals();

    for(int it=0;it<n_steps;it++)
    {
        for(int io=0;io<2;io++) // first input buffers (io=0) then output buffers (io=1)
        {
            iBuffer = (this->iCurrentBuffer+io)%2;
            for(int ic=0;ic<NC;ic++)
            {
                // a_in, b_in, ... a_out, b_out ...
                ret = clSetKernelArg(this->kernel, io*NC+ic, sizeof(cl_mem), (void *)&this->buffers[iBuffer][ic]);
                throwOnError(ret,"OpenCL_RD::Update : clSetKernelArg failed: ");
            }
        }
        ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, NULL, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::Update : clEnqueueNDRangeKernel failed: ");
        this->iCurrentBuffer = 1 - this->iCurrentBuffer;
    }

    // read from opencl buffers into our image
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::Update : buffer reading failed: ");
    }
}