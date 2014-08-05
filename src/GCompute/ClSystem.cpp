// Copyright (c) 2013-2014 Matthew Paul Reid

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#define CL_CHECK(_expr)                                                         \
   do {                                                                         \
     cl_int _err = _expr;                                                       \
     if (_err == CL_SUCCESS)                                                    \
       break;                                                                   \
     fprintf(stderr, "OpenCL Error: '%s' returned %d!\n", #_expr, (int)_err);   \
     abort();                                                                   \
   } while (0)

#include "ClIncludes.h"
#include "ClSystem.h"
#include "ClError.h"
#include <GCommon/Logger.h>

#include <boost/filesystem.hpp>
#include <stdexcept>
#include <iostream>
#include <fstream>

#include "Context.h"

using namespace GCommon;

namespace GCompute {

ClSystem::ClSystem()
{
	int deviceType = CL_DEVICE_TYPE_GPU;
	cl::Platform platform = getPlatform(deviceType);

    defaultLogger()->logLine("Creating context");
    m_context = createContext(deviceType, platform);
    if (!m_context)
	{
        throw std::runtime_error("Cound not create context");
    }

	platform.getDevices(deviceType, &m_devices);

	m_queue.reset(new cl::CommandQueue(*m_context, _getDevice(), 0));


	cl_device_id devices[100];
	cl_uint devices_n = 0;
	// CL_CHECK(clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, 100, devices, &devices_n));
	
	cl_platform_id platforms[100];
	cl_uint platforms_n = 0;
	CL_CHECK(clGetPlatformIDs(100, platforms, &platforms_n));

	printf("=== %d OpenCL platform(s) found: ===\n", platforms_n);
	for (int i=0; i<platforms_n; i++)
	{
		char buffer[10240];
		printf("  -- %d --\n", i);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, 10240, buffer, NULL));
		printf("  PROFILE = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, 10240, buffer, NULL));
		printf("  VERSION = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 10240, buffer, NULL));
		printf("  NAME = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL));
		printf("  VENDOR = %s\n", buffer);
		CL_CHECK(clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL));
		printf("  EXTENSIONS = %s\n", buffer);
	}

	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 100, devices, &devices_n);

	printf("=== %d OpenCL device(s) found on platform:\n", platforms_n);
	for (int i=0; i<devices_n; i++)
	{
		char buffer[10240];
		cl_uint buf_uint;
		cl_ulong buf_ulong;
		printf("  -- %d --\n", i);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL));
		printf("  DEVICE_NAME = %s\n", buffer);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL));
		printf("  DEVICE_VENDOR = %s\n", buffer);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL));
		printf("  DEVICE_VERSION = %s\n", buffer);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL));
		printf("  DRIVER_VERSION = %s\n", buffer);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL));
		printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL));
		printf("  DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
		CL_CHECK(clGetDeviceInfo(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL));
		printf("  DEVICE_GLOBAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
	}

}

ClSystem::~ClSystem()
{
}

void ClSystem::loadProgram(cl::Program& program, const std::string &filename) const
{
	cl_int err;

    defaultLogger()->logLine("Loading and compiling CL source");
	std::ifstream file(filename);
	if (!file.is_open())
	{
         throw std::runtime_error("Could not load CL source code file: " + filename);
    }

	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    cl::Program::Sources sources(1, std::make_pair(str.c_str(), str.size()));

    program = cl::Program(*m_context, sources, &err);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Program::Program() failed. Reason: " + getOpenClErrorString(err));
    }

	std::string includePath = boost::filesystem::path(filename).parent_path().string();

	err = program.build(m_devices, std::string("-I " + includePath).c_str());
    if (err != CL_SUCCESS) {

        if(err == CL_BUILD_PROGRAM_FAILURE)
        {
            cl::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_devices[0]);

            defaultLogger()->logLine(" \n\t\t\tBUILD LOG");
            defaultLogger()->logLine(" ************************************************");
			defaultLogger()->logLine(str.c_str());
            defaultLogger()->logLine(" ************************************************");
        }

        throw std::runtime_error("Program::build() failed. Reason: " + getOpenClErrorString(err));
    }

	defaultLogger()->logLine("Compilation successful");
}

cl::Context& ClSystem::_getContext() const
{
	return *m_context;
}

const cl::Device& ClSystem::_getDevice() const
{
	return m_devices[0];
}

cl::Device& ClSystem::_getDevice()
{
	return m_devices[0];
}

void ClSystem::writeToDevice(const cl::Buffer& buffer, const void* data, int sizeBytes)
{
	cl::Event evt;
	checkError(m_queue->enqueueWriteBuffer(buffer, CL_TRUE, 0, sizeBytes, data, NULL, &evt));
	checkError(m_queue->flush());
	waitForComplete(evt);
}

void ClSystem::readFromDevice(void* data, const cl::Buffer& buffer, int sizeBytes)
{
	cl::Event evt;
	checkError(m_queue->enqueueReadBuffer(buffer, CL_TRUE, 0, sizeBytes, data, NULL, &evt));
	checkError(m_queue->flush());
	waitForComplete(evt);
}

void ClSystem::runKernel(cl::Kernel& kernel, const cl::NDRange& globalThreads, const cl::NDRange& localThreads) const
{
	cl::Event evt;
	checkError(m_queue->enqueueNDRangeKernel(kernel, cl::NullRange, globalThreads, localThreads, NULL, &evt));
	checkError(m_queue->flush());
	waitForComplete(evt);
}

void ClSystem::createKernel(cl::Kernel& kernel, const cl::Program& program, const std::string& name)
{
	cl_int err;
	kernel = cl::Kernel(program, name.c_str(),  &err);
	checkError(err, "Could not create kernel: " + name);
}
static void createKernel(cl::Kernel& kernel, const cl::Program& program, const std::string& name)
{
	cl_int err;
	kernel = cl::Kernel(program, name.c_str(),  &err);
	checkError(err, "Could not create kernel: " + name);
}
int ClSystem::getMaxWorkGroupSize() const
{
	size_t maxWorkGroupSize = 0;
	cl_int status = _getDevice().getInfo<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE, &maxWorkGroupSize);
	checkError(status, "clGetDeviceIDs(CL_DEVICE_MAX_WORK_GROUP_SIZE) failed");

	return (int)maxWorkGroupSize;
}

void checkError(cl_int status, const std::string& contextMessage)
{
	// Status < 0 in OpenCL means error occured
	if (status < 0)
	{
		if (contextMessage.empty())
		{
			throw std::runtime_error("OpenCL error: " + getOpenClErrorString(status));
		}
		else
		{
			throw std::runtime_error(contextMessage + " failed. Reason: " + getOpenClErrorString(status));
		}
	}
}

void waitForComplete(const cl::Event& evt)
{
    evt.wait();
	cl_int eventStatus;
	checkError(evt.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &eventStatus));
	checkError(eventStatus);
}

} // namespace GCompute
