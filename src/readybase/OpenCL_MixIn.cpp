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

// local:
#include "OpenCL_MixIn.hpp"
#include "OpenCL_utils.hpp"
using namespace OpenCL_utils;

// STL:
#include <stdexcept>
#include <fstream>
#include <sstream>

using namespace std;

// ---------------------------------------------------------------------------

OpenCL_MixIn::OpenCL_MixIn(int opencl_platform, int opencl_device)
    : context(NULL)
    , device_id(NULL)
    , program(NULL)
    , kernel(NULL)
    , kernel_function_name("rd_compute")
    , global_range{ 1, 1, 1 }
    , local_work_size{ 1, 1, 1 }
    , command_queue(NULL)
    , need_reload_context(true)
    , need_write_to_opencl_buffers(true)
    , iCurrentBuffer(0)
    , iPlatform(opencl_platform)
    , iDevice(opencl_device)
{
    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");
}

// ---------------------------------------------------------------------------

OpenCL_MixIn::~OpenCL_MixIn()
{
    clFlush(this->command_queue);
    clFinish(this->command_queue);
    clReleaseKernel(this->kernel);
    clReleaseProgram(this->program);
    for(int i=0;i<2;i++)
        for(vector<cl_mem>::const_iterator it = this->buffers[i].begin();it!=this->buffers[i].end();it++)
            clReleaseMemObject(*it);
    clReleaseCommandQueue(this->command_queue);
    clReleaseContext(this->context);
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::SetPlatform(int i)
{
    if(i != this->iPlatform)
        this->need_reload_context = true;
    this->iPlatform = i;
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::SetDevice(int i)
{
    if(i != this->iDevice)
        this->need_reload_context = true;
    this->iDevice = i;
}

// ---------------------------------------------------------------------------

int OpenCL_MixIn::GetPlatform() const
{
    return this->iPlatform;
}

// ---------------------------------------------------------------------------

int OpenCL_MixIn::GetDevice() const
{
    return this->iDevice;
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::ReloadContextIfNeeded()
{
    if(!this->need_reload_context) return;

    cl_int ret;

    // retrieve our chosen platform
    cl_platform_id platform_id;
    {
        cl_uint num_platforms = 0;
        ret = clGetPlatformIDs( 0, 0, &num_platforms );
        if(ret != CL_SUCCESS || num_platforms==0)
        {
            throw runtime_error("No OpenCL platforms available");
            // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
        }
        if(this->iPlatform >= (int)num_platforms)
            throw runtime_error("OpenCL_MixIn::ReloadContextIfNeeded : too few platforms available");
        vector<cl_platform_id> platforms_available( num_platforms );
        ret = clGetPlatformIDs( num_platforms, platforms_available.data(), 0 );
        if(ret != CL_SUCCESS)
        {
            throw runtime_error("Failed to retrieve OpenCL platforms");
            // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
        }
        platform_id = platforms_available[this->iPlatform];
    }

    // retrieve our chosen device
    {
        cl_uint num_devices = 0;
        ret = clGetDeviceIDs(platform_id,CL_DEVICE_TYPE_ALL,0,0,&num_devices);
        throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to retrieve number of device IDs: ");
        if(this->iDevice >= (int)num_devices)
            throw runtime_error("OpenCL_MixIn::ReloadContextIfNeeded : too few devices available");

        vector<cl_device_id> devices_available(num_devices);
        ret = clGetDeviceIDs(platform_id,CL_DEVICE_TYPE_ALL,num_devices,devices_available.data(),0);
        throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to retrieve device IDs: ");
        this->device_id = devices_available[this->iDevice];
    }

    // create the context
    clReleaseContext(this->context);
    this->context = clCreateContext(NULL,1,&this->device_id,NULL,NULL,&ret);
    throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to create context: ");

    // create the command queue
    clReleaseCommandQueue(this->command_queue);
    this->command_queue = clCreateCommandQueue(this->context,this->device_id,0,&ret);
    throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to create command queue: ");

    this->need_reload_context = false;
}

// -------------------------------------------------------------------------

void OpenCL_MixIn::TestKernel(std::string kernel_source)
{
    this->need_reload_context = true;
    this->ReloadContextIfNeeded();

    cl_int ret;

    // create the program
    const char *source = kernel_source.c_str();
    size_t source_size = kernel_source.length();
    cl_program temp_program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_MixIn::TestKernel : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(temp_program,1,&this->device_id,"-cl-denorms-are-zero",NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        size_t build_log_length = 0;
        cl_int ret2 = clGetProgramBuildInfo(temp_program,this->device_id,CL_PROGRAM_BUILD_LOG,0,0,&build_log_length);
        throwOnError(ret2,"OpenCL_MixIn::TestKernel : retrieving length of program build log failed: ");
        vector<char> build_log(build_log_length);
        cl_int ret3 = clGetProgramBuildInfo(temp_program,this->device_id,CL_PROGRAM_BUILD_LOG,build_log_length,build_log.data(),0);
        throwOnError(ret3,"OpenCL_MixIn::TestKernel : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_MixIn::TestKernel : build failed (kernel saved as kernel.txt):\n\n" << string( build_log.begin(), build_log.end() );
        throwOnError(ret,oss.str().c_str());
    }
    clReleaseProgram(temp_program);
}

// -----------------------------------------------------------------------

void OpenCL_MixIn::ReleaseOpenCLBuffers()
{
    for(int i=0;i<2;i++)
        for(vector<cl_mem>::const_iterator it = this->buffers[i].begin();it!=this->buffers[i].end();it++)
            clReleaseMemObject(*it);
}

// -----------------------------------------------------------------------
