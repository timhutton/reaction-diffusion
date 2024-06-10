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
#include "OpenCL_utils.hpp"
using namespace OpenCL_utils;

// STL:
#include <sstream>
#include <stdexcept>

// SSE:
#if (defined(_WIN32) || defined(_WIN64))
  #include <intrin.h>
#endif

using namespace std;

// ---------------------------------------------------------------------------------------------------------

bool OpenCL_utils::IsOpenCLAvailable()
{
    if(LinkOpenCL()!= CL_SUCCESS)
        return false; // no OpenCL SDK or driver installed

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    if(ret != CL_SUCCESS || num_platforms==0)
        return false; // OpenCL is installed but no platforms available (in a virtualized OS?)

    // look for OpenCL devices on any platform
    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    for(unsigned int iPlatform=0;iPlatform<num_platforms;iPlatform++)
    {
        clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,
            devices_available,&num_devices);
        if(num_devices>0)
            return true;
    }
    return false; // platforms present but no OpenCL devices available
}

// ---------------------------------------------------------------------------------------------------------

void OpenCL_utils::throwOnError(cl_int ret,const char* message)
{
    if(ret == CL_SUCCESS) return;

    ostringstream oss;
    oss << message << GetDescriptionOfOpenCLError(ret);
    throw runtime_error(oss.str().c_str());
}

// ---------------------------------------------------------------------------------------------------------

std::string OpenCL_utils::GetOpenCLDiagnostics() // should never throw
{
    ostringstream report;

    cl_int ret = LinkOpenCL();
    if(ret != CL_SUCCESS)
    {
        report << "Failed to load dynamic library for OpenCL: " << GetDescriptionOfOpenCLError(ret);
        return report.str();
    }

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    if(ret != CL_SUCCESS)
    {
        report << "clGetPlatformIDs failed: " << GetDescriptionOfOpenCLError(ret);
        return report.str();
    }
    if(num_platforms==0)
    {
        report << "No OpenCL platforms available";
        // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
    }
    else
    {
        report << "Found " << num_platforms << " platform(s):\n";

        for(unsigned int iPlatform=0;iPlatform<num_platforms;iPlatform++)
        {
            report << "Platform " << iPlatform+1 << ":\n";
            for(cl_platform_info i=CL_PLATFORM_PROFILE;i<=CL_PLATFORM_EXTENSIONS;i++)
            {
                size_t info_length;
                ret = clGetPlatformInfo(platforms_available[iPlatform], i, 0, NULL, &info_length);
                if (ret != CL_SUCCESS)
                {
                    report << "clGetPlatformInfo failed: " << GetDescriptionOfOpenCLError(ret);
                }
                else {
                    char* info = new char[info_length];
                    ret = clGetPlatformInfo(platforms_available[iPlatform], i, info_length, info, &info_length);
                    if (ret != CL_SUCCESS)
                    {
                        report << "clGetPlatformInfo failed: " << GetDescriptionOfOpenCLError(ret);
                    }
                    report << GetPlatformInfoIdAsString(i) << " : " << info << "\n";
                    delete[] info;
                }
            }

            const size_t MAX_DEVICES = 10;
            cl_device_id devices_available[MAX_DEVICES];
            cl_uint num_devices;
            ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
            if(ret != CL_SUCCESS)
            {
                report << "clGetDeviceIDs failed: " << GetDescriptionOfOpenCLError(ret);
                return report.str();
            }
            report << "\nFound " << num_devices << " device(s) on this platform.\n";

            const int N_CHAR_RETURNING_IDS = 6;
            cl_device_info charReturningIds[N_CHAR_RETURNING_IDS] = {CL_DEVICE_EXTENSIONS,CL_DEVICE_NAME,CL_DEVICE_PROFILE,CL_DEVICE_VENDOR,
                CL_DEVICE_VERSION,CL_DRIVER_VERSION};
            const int N_UINT_RETURNING_IDS = 13;
            cl_device_info uintReturningIds[N_UINT_RETURNING_IDS] = {CL_DEVICE_ADDRESS_BITS,CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,CL_DEVICE_MAX_CLOCK_FREQUENCY,
                CL_DEVICE_MAX_COMPUTE_UNITS,CL_DEVICE_MAX_CONSTANT_ARGS,CL_DEVICE_MAX_READ_IMAGE_ARGS,CL_DEVICE_MAX_SAMPLERS,CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                CL_DEVICE_MAX_WRITE_IMAGE_ARGS,CL_DEVICE_MEM_BASE_ADDR_ALIGN,CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                CL_DEVICE_VENDOR_ID};
            const int N_BOOL_RETURNING_IDS = 5;
            cl_device_info boolReturningIds[N_BOOL_RETURNING_IDS] = {CL_DEVICE_AVAILABLE,CL_DEVICE_COMPILER_AVAILABLE,CL_DEVICE_ENDIAN_LITTLE,CL_DEVICE_ERROR_CORRECTION_SUPPORT,
                CL_DEVICE_IMAGE_SUPPORT};
            const int N_ULONG_RETURNING_IDS = 5;
            cl_device_info ulongReturningIds[N_ULONG_RETURNING_IDS] = {CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,CL_DEVICE_GLOBAL_MEM_SIZE,CL_DEVICE_LOCAL_MEM_SIZE,CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
                CL_DEVICE_MAX_MEM_ALLOC_SIZE};
            const int N_SIZET_RETURNING_IDS = 8;
            cl_device_info sizetReturningIds[N_SIZET_RETURNING_IDS] = {CL_DEVICE_IMAGE2D_MAX_HEIGHT,CL_DEVICE_IMAGE2D_MAX_WIDTH,CL_DEVICE_IMAGE3D_MAX_DEPTH,CL_DEVICE_IMAGE3D_MAX_HEIGHT,
                CL_DEVICE_IMAGE3D_MAX_WIDTH,CL_DEVICE_MAX_PARAMETER_SIZE,CL_DEVICE_MAX_WORK_GROUP_SIZE,CL_DEVICE_PROFILING_TIMER_RESOLUTION};
            const int N_FPCONFIG_RETURNING_IDS = 2;
            cl_device_info fpConfigReturningIds[N_FPCONFIG_RETURNING_IDS] = {CL_DEVICE_SINGLE_FP_CONFIG,CL_DEVICE_DOUBLE_FP_CONFIG};

            cl_uint uint_value;
            cl_bool bool_value;
            cl_ulong ulong_value;
            size_t sizet_value;
            cl_device_fp_config fp_config_value;
            for(unsigned int iDevice=0;iDevice<num_devices;iDevice++)
            {
                report << "Device " << iDevice+1 << ":\n";
                for(int i=0;i<N_CHAR_RETURNING_IDS;i++)
                {
                    size_t info_length;
                    ret = clGetDeviceInfo(devices_available[iDevice],charReturningIds[i],0,NULL,&info_length);
                    if (ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        char* info = new char[info_length];
                        ret = clGetDeviceInfo(devices_available[iDevice], charReturningIds[i], info_length, info, &info_length);
                        if (ret != CL_SUCCESS) {
                            report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                        }
                        else {
                            report << GetDeviceInfoIdAsString(charReturningIds[i]) << " : " << info << "\n";
                        }
                        delete[] info;
                    }
                }
                for(int i=0;i<N_UINT_RETURNING_IDS;i++)
                {
                    ret = clGetDeviceInfo(devices_available[iDevice],uintReturningIds[i],sizeof(uint_value),&uint_value,NULL);
                    if(ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        report << GetDeviceInfoIdAsString(uintReturningIds[i]) << " : " << uint_value << "\n";
                    }
                }
                for(int i=0;i<N_BOOL_RETURNING_IDS;i++)
                {
                    ret = clGetDeviceInfo(devices_available[iDevice],boolReturningIds[i],sizeof(bool_value),&bool_value,NULL);
                    if(ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        report << GetDeviceInfoIdAsString(boolReturningIds[i]) << " : " << (bool_value?"yes":"no") << "\n";
                    }
                }
                for(int i=0;i<N_ULONG_RETURNING_IDS;i++)
                {
                    ret = clGetDeviceInfo(devices_available[iDevice],ulongReturningIds[i],sizeof(ulong_value),&ulong_value,NULL);
                    if(ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        report << GetDeviceInfoIdAsString(ulongReturningIds[i]) << " : " << ulong_value << "\n";
                    }
                }
                for(int i=0;i<N_SIZET_RETURNING_IDS;i++)
                {
                    ret = clGetDeviceInfo(devices_available[iDevice],sizetReturningIds[i],sizeof(sizet_value),&sizet_value,NULL);
                    if(ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        report << GetDeviceInfoIdAsString(sizetReturningIds[i]) << " : " << sizet_value << "\n";
                    }
                }
                for(int i=0;i<N_FPCONFIG_RETURNING_IDS;i++)
                {
                    ret = clGetDeviceInfo(devices_available[iDevice],fpConfigReturningIds[i],sizeof(fp_config_value),&fp_config_value,NULL);
                    if(ret != CL_SUCCESS) {
                        report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                    }
                    else {
                        report << GetDeviceInfoIdAsString(fpConfigReturningIds[i]) << " :";
                        if(fp_config_value&CL_FP_DENORM) report << " CL_FP_DENORM";
                        if(fp_config_value&CL_FP_INF_NAN) report << " CL_FP_INF_NAN";
                        if(fp_config_value&CL_FP_ROUND_TO_NEAREST) report << " CL_FP_ROUND_TO_NEAREST";
                        if(fp_config_value&CL_FP_ROUND_TO_ZERO) report << " CL_FP_ROUND_TO_ZERO";
                        if(fp_config_value&CL_FP_ROUND_TO_INF ) report << " CL_FP_ROUND_TO_INF";
                        if(fp_config_value&CL_FP_FMA) report << " CL_FP_FMA";
                        report << "\n";
                    }
                }
                // CL_DEVICE_MAX_WORK_ITEM_SIZES:
                size_t dim[3];
                ret = clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(dim),dim,NULL);
                if(ret != CL_SUCCESS) {
                    report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                }
                else {
                    report << GetDeviceInfoIdAsString(CL_DEVICE_MAX_WORK_ITEM_SIZES) << " : " << dim[0] << ", " << dim[1] << ", " << dim[2] << "\n";
                }
                // CL_DEVICE_TYPE:
                cl_device_type device_type;
                ret = clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_TYPE,sizeof(device_type),&device_type,NULL);
                if(ret != CL_SUCCESS) {
                    report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                }
                else {
                    ostringstream oss;
                    if(device_type&CL_DEVICE_TYPE_CPU) oss << "CPU";
                    if(device_type&CL_DEVICE_TYPE_GPU) { if(!oss.str().empty()) { oss << " & "; } oss << "GPU"; }
                    if(device_type&CL_DEVICE_TYPE_ACCELERATOR) { if(!oss.str().empty()) { oss << " & "; } oss << "ACCELERATOR"; }
                    if(device_type&CL_DEVICE_TYPE_DEFAULT) { if(!oss.str().empty()) { oss << " & "; } oss << "DEFAULT"; }
                    report << GetDeviceInfoIdAsString(CL_DEVICE_TYPE) << " : " << oss.str() << "\n";
                }
                // CL_DEVICE_LOCAL_MEM_TYPE:
                cl_device_local_mem_type local_mem_type;
                ret = clGetDeviceInfo(devices_available[iDevice], CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &local_mem_type, NULL);
                if (ret != CL_SUCCESS) {
                    report << "clGetDeviceInfo failed: " << GetDescriptionOfOpenCLError(ret) << "\n";
                }
                else {
                    ostringstream oss;
                    if (local_mem_type & CL_LOCAL) { oss << "LOCAL"; }
                    if (local_mem_type & CL_GLOBAL) { oss << "GLOBAL"; }
                    report << GetDeviceInfoIdAsString(CL_DEVICE_LOCAL_MEM_TYPE) << " : " << oss.str() << "\n";
                }
            }
            report << "\n";
        }
    }

    // bonus feature: report CPU capabilities
    // TODO: use a cross-platform replacement for __cpuid()
    #if (defined(_WIN32) || defined(_WIN64))
    {
        // http://stackoverflow.com/questions/6121792/is-this-code-valid-to-check-for-sse3
        int x64     = false;
        int MMX     = false;
        int SSE     = false;
        int SSE2    = false;
        int SSE3    = false;
        int SSSE3   = false;
        int SSE41   = false;
        int SSE42   = false;
        int SSE4a   = false;
        int AVX     = false;
        int XOP     = false;
        int FMA3    = false;
        int FMA4    = false;

        int info[4];
        __cpuid(info, 0);
        int nIds = info[0];

        __cpuid(info, 0x80000000);
        int nExIds = info[0];

        //  Detect Instruction Set
        if (nIds >= 1){
            __cpuid(info,0x00000001);
            MMX   = (info[3] & ((int)1 << 23)) != 0;
            SSE   = (info[3] & ((int)1 << 25)) != 0;
            SSE2  = (info[3] & ((int)1 << 26)) != 0;
            SSE3  = (info[2] & ((int)1 <<  0)) != 0;

            SSSE3 = (info[2] & ((int)1 <<  9)) != 0;
            SSE41 = (info[2] & ((int)1 << 19)) != 0;
            SSE42 = (info[2] & ((int)1 << 20)) != 0;

            AVX   = (info[2] & ((int)1 << 28)) != 0;
            FMA3  = (info[2] & ((int)1 << 12)) != 0;
        }

        if (nExIds >= 0x80000001){
            __cpuid(info,0x80000001);
            x64   = (info[3] & ((int)1 << 29)) != 0;
            SSE4a = (info[2] & ((int)1 <<  6)) != 0;
            FMA4  = (info[2] & ((int)1 << 16)) != 0;
            XOP   = (info[2] & ((int)1 << 11)) != 0;
        }

        report << "----------- CPU information: ------------\n";
        report << "x64: " << x64 << "\n";
        report << "MMX: " << MMX << "\n";
        report << "SSE: " << SSE << "\n";
        report << "SSE2: " << SSE2 << "\n";
        report << "SSE3: " << SSE3 << "\n";
        report << "SSSE3: " << SSSE3 << "\n";
        report << "SSE41: " << SSE41 << "\n";
        report << "SSE42: " << SSE42 << "\n";
        report << "SSE4a: " << SSE4a << "\n";
        report << "AVX: " << AVX << "\n";
        report << "XOP: " << XOP << "\n";
        report << "FMA3: " << FMA3 << "\n";
        report << "FMA4: " << FMA4 << "\n";
    }
    #endif

    return report.str();
}

// ---------------------------------------------------------------------------------------------------------

int OpenCL_utils::GetNumberOfPlatforms()
{
    if(LinkOpenCL() != CL_SUCCESS)
        return 0;

    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_utils::GetNumberOfPlatforms : clGetPlatformIDs failed: ");
    return num_platforms;
}

// ---------------------------------------------------------------------------------------------------------

int OpenCL_utils::GetNumberOfDevices(int iPlatform)
{
    if(LinkOpenCL() != CL_SUCCESS)
        return 0;

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_utils::GetNumberOfDevices : clGetPlatformIDs failed: ");

    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
    throwOnError(ret,"OpenCL_utils::GetNumberOfDevices : clGetDeviceIDs failed: ");

    return num_devices;
}

// ---------------------------------------------------------------------------------------------------------

string OpenCL_utils::GetPlatformDescription(int iPlatform)
{
    LinkOpenCL();

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_utils::GetPlatformDescription : clGetPlatformIDs failed: ");

    ostringstream oss;
    size_t info_length;
    ret = clGetPlatformInfo(platforms_available[iPlatform], CL_PLATFORM_NAME, 0, NULL, &info_length);
    throwOnError(ret,"OpenCL_utils::GetPlatformDescription : clGetPlatformInfo failed: ");
    char* info = new char[info_length];
    ret = clGetPlatformInfo(platforms_available[iPlatform], CL_PLATFORM_NAME, info_length, info, &info_length);
    throwOnError(ret, "OpenCL_utils::GetPlatformDescription : clGetPlatformInfo failed: ");
    string platform_name(info, info + info_length);
    delete[] info;
    platform_name = platform_name.substr(platform_name.find_first_not_of(" \n\r\t"));
    oss << platform_name;
    return oss.str();
}

// ---------------------------------------------------------------------------------------------------------

string OpenCL_utils::GetDeviceDescription(int iPlatform,int iDevice)
{
    LinkOpenCL();

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_utils::GetDeviceDescription : clGetPlatformIDs failed: ");

    ostringstream oss;
    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,
        MAX_DEVICES,devices_available,&num_devices);
    throwOnError(ret,"OpenCL_utils::GetDeviceDescription : clGetDeviceIDs failed: ");
    size_t info_length;
    ret = clGetDeviceInfo(devices_available[iDevice], CL_DEVICE_NAME, 0, NULL, &info_length);
    throwOnError(ret, "OpenCL_utils::GetDeviceDescription : clGetDeviceInfo failed: ");
    char* info = new char[info_length];
    ret = clGetDeviceInfo(devices_available[iDevice], CL_DEVICE_NAME, info_length, info, &info_length);
    throwOnError(ret, "OpenCL_utils::GetDeviceDescription : clGetDeviceInfo failed: ");
    string device_name(info, info + info_length);
    delete[] info;
    device_name = device_name.substr(device_name.find_first_not_of(" \n\r\t"));
    oss << device_name;
    return oss.str();
}

// ---------------------------------------------------------------------------------------------------------

bool OpenCL_utils::CanUseDoubles(int iPlatform,int iDevice)
{
    LinkOpenCL();

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_utils::CanUseDoubles : clGetPlatformIDs failed: ");

    ostringstream oss;
    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,
        MAX_DEVICES,devices_available,&num_devices);
    throwOnError(ret,"OpenCL_utils::CanUseDoubles : clGetDeviceIDs failed: ");

    cl_device_fp_config double_fp_config;
    ret = clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_DOUBLE_FP_CONFIG,
        sizeof(double_fp_config),&double_fp_config,NULL);
    throwOnError(ret,"OpenCL_utils::CanUseDoubles : clGetDeviceInfo failed: ");

    return double_fp_config > 0;
}

// ---------------------------------------------------------------------------------------------------------

cl_int OpenCL_utils::LinkOpenCL()
{
#ifdef __APPLE__
    return CL_SUCCESS;
#else
    return clLibLoad();
#endif
}

// ---------------------------------------------------------------------------------------------------------

// http://www.khronos.org/message_boards/viewtopic.php?f=37&t=2107
const char* OpenCL_utils::GetDescriptionOfOpenCLError(cl_int err)
{
    switch (err) {
        case CL_SUCCESS:                            return "Success!";
        case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                   return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
        case CL_MAP_FAILURE:                        return "Map failure";
        case CL_INVALID_VALUE:                      return "Invalid value";
        case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
        case CL_INVALID_PLATFORM:                   return "Invalid platform";
        case CL_INVALID_DEVICE:                     return "Invalid device";
        case CL_INVALID_CONTEXT:                    return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
        case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
        case CL_INVALID_SAMPLER:                    return "Invalid sampler";
        case CL_INVALID_BINARY:                     return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
        case CL_INVALID_PROGRAM:                    return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
        case CL_INVALID_KERNEL:                     return "Invalid kernel";
        case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
        case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
        case CL_INVALID_EVENT:                      return "Invalid event";
        case CL_INVALID_OPERATION:                  return "Invalid operation";
        case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
        default: return "Unknown";
    }
}

// ---------------------------------------------------------------------------------------------------------

const char* OpenCL_utils::GetPlatformInfoIdAsString(cl_platform_info i)
{
    switch(i)
    {
        case CL_PLATFORM_PROFILE:    return "CL_PLATFORM_PROFILE";
        case CL_PLATFORM_VERSION:    return "CL_PLATFORM_VERSION";
        case CL_PLATFORM_NAME:       return "CL_PLATFORM_NAME";
        case CL_PLATFORM_VENDOR:     return "CL_PLATFORM_VENDOR";
        case CL_PLATFORM_EXTENSIONS: return "CL_PLATFORM_EXTENSIONS";
        default: return "Unknown";
    }
}

// ---------------------------------------------------------------------------------------------------------

const char* OpenCL_utils::GetDeviceInfoIdAsString(cl_device_info i)
{
    switch(i)
    {
        case CL_DEVICE_TYPE:                           return "CL_DEVICE_TYPE";
        case CL_DEVICE_VENDOR_ID:                      return "CL_DEVICE_VENDOR_ID";
        case CL_DEVICE_MAX_COMPUTE_UNITS:              return "CL_DEVICE_MAX_COMPUTE_UNITS";
        case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:       return "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS";
        case CL_DEVICE_MAX_WORK_GROUP_SIZE:            return "CL_DEVICE_MAX_WORK_GROUP_SIZE";
        case CL_DEVICE_MAX_WORK_ITEM_SIZES:            return "CL_DEVICE_MAX_WORK_ITEM_SIZES";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:    return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:   return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:     return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:    return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:   return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE";
        case CL_DEVICE_MAX_CLOCK_FREQUENCY:            return "CL_DEVICE_MAX_CLOCK_FREQUENCY";
        case CL_DEVICE_ADDRESS_BITS:                   return "CL_DEVICE_ADDRESS_BITS";
        case CL_DEVICE_MAX_READ_IMAGE_ARGS:            return "CL_DEVICE_MAX_READ_IMAGE_ARGS";
        case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:           return "CL_DEVICE_MAX_WRITE_IMAGE_ARGS";
        case CL_DEVICE_MAX_MEM_ALLOC_SIZE:             return "CL_DEVICE_MAX_MEM_ALLOC_SIZE";
        case CL_DEVICE_IMAGE2D_MAX_WIDTH:              return "CL_DEVICE_IMAGE2D_MAX_WIDTH";
        case CL_DEVICE_IMAGE2D_MAX_HEIGHT:             return "CL_DEVICE_IMAGE2D_MAX_HEIGHT";
        case CL_DEVICE_IMAGE3D_MAX_WIDTH:              return "CL_DEVICE_IMAGE3D_MAX_WIDTH";
        case CL_DEVICE_IMAGE3D_MAX_HEIGHT:             return "CL_DEVICE_IMAGE3D_MAX_HEIGHT";
        case CL_DEVICE_IMAGE3D_MAX_DEPTH:              return "CL_DEVICE_IMAGE3D_MAX_DEPTH";
        case CL_DEVICE_IMAGE_SUPPORT:                  return "CL_DEVICE_IMAGE_SUPPORT";
        case CL_DEVICE_MAX_PARAMETER_SIZE:             return "CL_DEVICE_MAX_PARAMETER_SIZE";
        case CL_DEVICE_MAX_SAMPLERS:                   return "CL_DEVICE_MAX_SAMPLERS";
        case CL_DEVICE_MEM_BASE_ADDR_ALIGN:            return "CL_DEVICE_MEM_BASE_ADDR_ALIGN";
        case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:       return "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE";
        case CL_DEVICE_DOUBLE_FP_CONFIG:               return "CL_DEVICE_DOUBLE_FP_CONFIG";
        case CL_DEVICE_SINGLE_FP_CONFIG:               return "CL_DEVICE_SINGLE_FP_CONFIG";
        case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:          return "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE";
        case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:      return "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE";
        case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:          return "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE";
        case CL_DEVICE_GLOBAL_MEM_SIZE:                return "CL_DEVICE_GLOBAL_MEM_SIZE";
        case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:       return "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE";
        case CL_DEVICE_MAX_CONSTANT_ARGS:              return "CL_DEVICE_MAX_CONSTANT_ARGS";
        case CL_DEVICE_LOCAL_MEM_TYPE:                 return "CL_DEVICE_LOCAL_MEM_TYPE";
        case CL_DEVICE_LOCAL_MEM_SIZE:                 return "CL_DEVICE_LOCAL_MEM_SIZE";
        case CL_DEVICE_ERROR_CORRECTION_SUPPORT:       return "CL_DEVICE_ERROR_CORRECTION_SUPPORT";
        case CL_DEVICE_PROFILING_TIMER_RESOLUTION:     return "CL_DEVICE_PROFILING_TIMER_RESOLUTION";
        case CL_DEVICE_ENDIAN_LITTLE:                  return "CL_DEVICE_ENDIAN_LITTLE";
        case CL_DEVICE_AVAILABLE:                      return "CL_DEVICE_AVAILABLE";
        case CL_DEVICE_COMPILER_AVAILABLE:             return "CL_DEVICE_COMPILER_AVAILABLE";
        case CL_DEVICE_EXECUTION_CAPABILITIES:         return "CL_DEVICE_EXECUTION_CAPABILITIES";
        case CL_DEVICE_QUEUE_PROPERTIES:               return "CL_DEVICE_QUEUE_PROPERTIES";
        case CL_DEVICE_NAME:                           return "CL_DEVICE_NAME";
        case CL_DEVICE_VENDOR:                         return "CL_DEVICE_VENDOR";
        case CL_DRIVER_VERSION:                        return "CL_DRIVER_VERSION";
        case CL_DEVICE_PROFILE:                        return "CL_DEVICE_PROFILE";
        case CL_DEVICE_VERSION:                        return "CL_DEVICE_VERSION";
        case CL_DEVICE_EXTENSIONS:                     return "CL_DEVICE_EXTENSIONS";
        case CL_DEVICE_PLATFORM:                       return "CL_DEVICE_PLATFORM";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:    return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF";
        case CL_DEVICE_HOST_UNIFIED_MEMORY:            return "CL_DEVICE_HOST_UNIFIED_MEMORY";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:       return "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:      return "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:        return "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:       return "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:      return "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:     return "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:       return "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF";
        case CL_DEVICE_OPENCL_C_VERSION:               return "CL_DEVICE_OPENCL_C_VERSION";
        default: return "Unknown";
    }
}

// ---------------------------------------------------------------------------------------------------------

const char* OpenCL_utils::GetOpenCLInstallationHints()
{
    return
        "This requires OpenCL, which has not been detected on your system.\n\n"
        "OpenCL allows Ready to take advantage of the many-core architectures on\n"
        "graphics cards and modern CPUs. OpenCL also allows rules to be written in\n"
        "a text format and compiled on the fly. It is available on every operating\n"
        "system, so please install it to get the most out of Ready. (One exception:\n"
        "If your OS is running in a virtual machine then it may not be possible to\n"
        "get OpenCL working.)\n\n"
        "Links:\n"
        "https://streamcomputing.eu/blog/2015-03-16/how-to-install-opencl-on-windows/\n"
        "https://wiki.tiker.net/OpenCLHowTo (for Linux)\n\n"
        "Without OpenCL you can load the files in the 'CPU-only' folder. Or use\n"
        "File > New Pattern or File > Import Mesh to make new examples.";
}
// -------------------------------------------------------------------------------------------------------------

