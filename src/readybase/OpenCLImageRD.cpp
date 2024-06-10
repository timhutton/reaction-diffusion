/*  Copyright 2011-2024 The Ready Bunch

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

#include "OpenCLImageRD.hpp"

// local:
#include "OpenCL_utils.hpp"
#include "utils.hpp"
using namespace OpenCL_utils;

// STL:
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <vector>

// VTK:
#include <vtkImageData.h>
#include <vtkMath.h>

using namespace std;

// ----------------------------------------------------------------------------------------------------------------

OpenCLImageRD::OpenCLImageRD(int opencl_platform,int opencl_device,int data_type)
    : ImageRD(data_type)
    , OpenCL_MixIn(opencl_platform,opencl_device)
{
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::BuildProgram()
{
    // create the program
    this->kernel_source = this->AssembleKernelSourceFromFormula(this->formula);
    const char* source = this->kernel_source.c_str();
    size_t source_size = this->kernel_source.length();
    clReleaseProgram(this->program);
    cl_int ret;
    this->program = clCreateProgramWithSource(this->context, 1, &source, &source_size, &ret);
    throwOnError(ret, "OpenCLImageRD::ReloadKernelIfNeeded : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(this->program, 1, &this->device_id, "-cl-denorms-are-zero", NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        size_t build_log_length = 0;
        cl_int ret2 = clGetProgramBuildInfo(this->program, this->device_id, CL_PROGRAM_BUILD_LOG, 0, 0, &build_log_length);
        throwOnError(ret2, "OpenCLImageRD::ReloadKernelIfNeeded : retrieving length of program build log failed: ");
        vector<char> build_log(build_log_length);
        cl_int ret3 = clGetProgramBuildInfo(this->program, this->device_id, CL_PROGRAM_BUILD_LOG, build_log_length, build_log.data(), 0);
        throwOnError(ret3, "OpenCLImageRD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCLImageRD::ReloadKernelIfNeeded : build failed (kernel saved as kernel.txt):\n\n" << string(build_log.begin(), build_log.end());
        throwOnError(ret, oss.str().c_str());
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_formula) return;

    this->global_range[0] = max(1, vtkMath::Round(this->GetX()) / this->GetBlockSizeX());
    this->global_range[1] = max(1, vtkMath::Round(this->GetY()) / this->GetBlockSizeY());
    this->global_range[2] = max(1, vtkMath::Round(this->GetZ()) / this->GetBlockSizeZ());

    if (this->use_local_memory)
    {
        cl_ulong local_memory_size;
        clGetDeviceInfo(this->device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_memory_size), &local_memory_size, NULL);
        cl_ulong max_work_group_size;
        clGetDeviceInfo(this->device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);

        int n = 1;
        while (n <= 1024)
        {
            this->local_work_size[0] = min(this->global_range[0], (size_t)4 * n / this->GetBlockSizeX());
            this->local_work_size[1] = min(this->global_range[1], (size_t)4 * n / this->GetBlockSizeY());
            this->local_work_size[2] = min(this->global_range[2], (size_t)4 * n / this->GetBlockSizeZ());
            try
            {
                // ensure that we don't hit CL_DEVICE_MAX_WORK_GROUP_SIZE
                size_t work_group_size = this->local_work_size[0] * this->local_work_size[1] * this->local_work_size[2];
                if (work_group_size >= max_work_group_size)  // if allow to be equal, can get errors later
                {
                    break;
                }
                // ensure that we don't hit CL_DEVICE_LOCAL_MEM_SIZE
                int extra = 2;
                size_t expected_mem = 4 * sizeof(float) * (this->local_work_size[0] + extra) * (this->local_work_size[1] + extra) * (this->local_work_size[2] + extra);
                // TODO: allow for number of chemicals etc, as we allocate in the kernel
                if (expected_mem > local_memory_size)
                {
                    break;
                }
                BuildProgram();
            }
            catch (...)
            {
                break;
            }
            n *= 2;
        }
        n /= 2; // return to last known good
        this->local_work_size[0] = min(this->global_range[0], (size_t)4 * n / this->GetBlockSizeX());
        this->local_work_size[1] = min(this->global_range[1], (size_t)4 * n / this->GetBlockSizeY());
        this->local_work_size[2] = min(this->global_range[2], (size_t)4 * n / this->GetBlockSizeZ());
    }

    BuildProgram();

    // create the kernel
    clReleaseKernel(this->kernel);
    cl_int ret;
    this->kernel = clCreateKernel(this->program, this->kernel_function_name.c_str(), &ret);
    throwOnError(ret,"OpenCLImageRD::ReloadKernelIfNeeded : kernel creation failed: ");

    this->need_reload_formula = false;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::CreateOpenCLBuffers()
{
    this->ReloadContextIfNeeded();

    const size_t MEM_SIZE = this->data_type_size * this->GetX() * this->GetY() * this->GetZ();
    const int NC = this->GetNumberOfChemicals();

    this->ReleaseOpenCLBuffers();

    cl_int ret;

    for(int io=0;io<2;io++) // we create two buffers for each chemical, and switch between them
    {
        this->buffers[io].resize(NC);
        for(int ic=0;ic<NC;ic++)
        {
            this->buffers[io][ic] = clCreateBuffer(this->context, CL_MEM_READ_WRITE, MEM_SIZE, NULL, &ret);
            throwOnError(ret,"OpenCLImageRD::CreateOpenCLBuffers : buffer creation failed: ");
        }
    }

    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::WriteToOpenCLBuffersIfNeeded()
{
    if(!this->need_write_to_opencl_buffers) return;

    const size_t MEM_SIZE = this->data_type_size * this->GetX() * this->GetY() * this->GetZ();

    this->iCurrentBuffer = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        void* data = this->images[ic]->GetScalarPointer();
        cl_int ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLImageRD::WriteToOpenCLBuffers : buffer writing failed: ");
    }

    this->need_write_to_opencl_buffers = false;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::CopyFromImage(vtkImageData* im)
{
    ImageRD::CopyFromImage(im);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::SetFrom2DImage(int iChemical, vtkImageData *im)
{
    ImageRD::SetFrom2DImage(iChemical, im);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::GenerateInitialPattern()
{
    ImageRD::GenerateInitialPattern();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::BlankImage(float value)
{
    ImageRD::BlankImage(value);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::AllocateImages(int x,int y,int z,int nc,int data_type)
{
    ImageRD::AllocateImages(x,y,z,nc,data_type);
    this->need_reload_formula = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::SetNumberOfChemicals(int n, bool reallocate_storage)
{
    ImageRD::SetNumberOfChemicals(n, reallocate_storage);
    this->need_reload_formula = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::InternalUpdate(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->WriteToOpenCLBuffersIfNeeded();

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
                throwOnError(ret,"OpenCLImageRD::InternalUpdate : clSetKernelArg failed: ");
            }
        }
        ret = clEnqueueNDRangeKernel(this->command_queue, this->kernel, 3, // dimensions
            NULL, this->global_range, this->use_local_memory ? this->local_work_size : NULL,
            0, NULL, NULL);
        if (ret != CL_SUCCESS)
        {
            ostringstream oss;
            oss << "OpenCLImageRD::InternalUpdate : clEnqueueNDRangeKernel failed.\n";
            oss << "Local work size: " << this->local_work_size[0] << " x " << this->local_work_size[1] << " x " << this->local_work_size[2] << "\n";
            throwOnError(ret, oss.str().c_str());
        }
        this->iCurrentBuffer = 1 - this->iCurrentBuffer;
    }

    this->ReadFromOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::ReadFromOpenCLBuffers()
{
    // read from opencl buffers into our image
    const size_t MEM_SIZE = this->data_type_size * this->GetX() * this->GetY() * this->GetZ();
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        void* data = this->images[ic]->GetScalarPointer();
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLImageRD::ReadFromOpenCLBuffers : buffer reading failed: ");
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::TestFormula(std::string program_string)
{
    this->TestKernel(this->AssembleKernelSourceFromFormula(program_string));
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    ImageRD::SetValue(x,y,z,val,render_settings);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    ImageRD::SetValuesInRadius(x,y,z,r,val,render_settings);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::Undo()
{
    ImageRD::Undo();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::Redo()
{
    ImageRD::Redo();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------
