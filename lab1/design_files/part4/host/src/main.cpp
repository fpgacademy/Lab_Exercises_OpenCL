// Copyright (C) 2013-2018 Altera Corporation, San Jose, California, USA. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
// whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// This agreement shall be governed in all respects by the laws of the State of California and
// by the laws of the United States of America.

///////////////////////////////////////////////////////////////////////////////////
// This host program executes a vector addition kernel to perform:
//  C = A + B
// where A, B and C are vectors with N elements.
//
// Verification is performed against the same computation on the host CPU.
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

// OpenCL runtime configuration
cl_platform_id platform = NULL;
unsigned num_devices = 0;
cl_device_id device; // num_devices elements
cl_context context = NULL;
cl_command_queue queue; // num_devices elements
cl_program program = NULL;
cl_kernel kernel; // num_devices elements
cl_mem input_a_buf; // num_devices elements
cl_mem input_b_buf; // num_devices elements
cl_mem output_buf; // num_devices elements

// Problem data.
unsigned N = 1000000; // problem size
cl_float *input_a, *input_b, *output, *ref_output;

// Function prototypes
float rand_float();
bool init_opencl();
void init_problem();
void run();
void cleanup();

// Entry point.
int main(int argc, char **argv) {
	Options options(argc, argv);

	// Optional argument to specify the problem size.
	if(options.has("n")) {
		N = options.get<unsigned>("n");
	}

	// Initialize OpenCL.
	if(!init_opencl()) {
		return -1;
	}

	// Initialize the problem data.
	init_problem();

	// Run the kernel.
	run();

	// Free the resources allocated
	cleanup();

	return 0;
}

/////// HELPER FUNCTIONS ///////

// Randomly generate a floating-point number between -10 and 10.
float rand_float() {
	return float(rand()) / float(RAND_MAX) * 20.0f - 10.0f;
}

// Initializes the OpenCL objects.
bool init_opencl() {
	cl_int status;

	printf("Initializing OpenCL\n");

	if(!setCwdToExeDir()) {
		return false;
	}

	// Get the OpenCL platform.
	platform = findPlatform("Intel(R) FPGA SDK for OpenCL(TM)");
	if(platform == NULL) {
		printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
		return false;
	}
  
	// Get the first device. There should only be one device.
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
	checkError (status, "Error: could not query devices");

	printf("Platform: %s\n", getPlatformName(platform).c_str());
	printf("  %s\n", getDeviceName(device).c_str());

	// Create the context.
	context = clCreateContext(NULL, 1, &device, &oclContextCallback, NULL, &status);
	checkError(status, "Failed to create context");

	// Create the program.
	std::string binary_file = getBoardBinaryFile("vector_add", device);
	printf("Using AOCX: %s\n", binary_file.c_str());
	program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

	// Build the program that was just created.
	status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
	checkError(status, "Failed to build program");

	// Command queue.
	queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
	checkError(status, "Failed to create command queue");

	// Kernel.
	const char *kernel_name = "vector_add";
	kernel = clCreateKernel(program, kernel_name, &status);
	checkError(status, "Failed to create kernel");

	// Input buffers.
	input_a_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), NULL, &status);
	checkError(status, "Failed to create buffer for input A");

	input_b_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), NULL, &status);
	checkError(status, "Failed to create buffer for input B");

	// Output buffer.
	output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), NULL, &status);
	checkError(status, "Failed to create buffer for output");

  return true;
}

// Initialize the data for the problem. 
void init_problem() {

	input_a = (cl_float*)alignedMalloc(sizeof(float) * N);
	input_b = (cl_float*)alignedMalloc(sizeof(float) * N);
	ref_output = (cl_float*)alignedMalloc(sizeof(float) * N);
	output = (cl_float*)alignedMalloc(sizeof(float) * N);

	// Generate input vectors A and B and the reference output consisting
	// of a total of N elements.
	for(unsigned j = 0; j < N; ++j) {
		input_a[j] = rand_float();
		input_b[j] = rand_float();
		ref_output[j] = input_a[j] + input_b[j];
	}
}

void run() {
	cl_int status;

	const double start_time = getCurrentTimestamp();

	// Launch the problem for each device.
	cl_event kernel_event;

    // Transfer inputs to each device. Each of the host buffers supplied to
    // clEnqueueWriteBuffer here is already aligned to ensure that DMA is used
    // for the host-to-device transfer.
    cl_event write_event[2];
    status = clEnqueueWriteBuffer(queue, input_a_buf, CL_FALSE,
        0, N * sizeof(float), input_a, 0, NULL, &write_event[0]);
    checkError(status, "Failed to transfer input A");

    status = clEnqueueWriteBuffer(queue, input_b_buf, CL_FALSE,
        0, N * sizeof(float), input_b, 0, NULL, &write_event[1]);
    checkError(status, "Failed to transfer input B");

    // Set kernel arguments.
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_a_buf);
    checkError(status, "Failed to set argument 0");
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_b_buf);
    checkError(status, "Failed to set argument 1");
    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_buf);
    checkError(status, "Failed to set argument 2");
    status = clSetKernelArg(kernel, 3, sizeof(int), &N);
    checkError(status, "Failed to set argument 3");

    // Enqueue kernel.
    // Events (write_event) are used to ensure that the kernel is not launched
    // until the writes to the input buffers have completed.
    status = clEnqueueTask(queue, kernel, 2, write_event, &kernel_event);
    checkError(status, "Failed to launch kernel");

    // Read the result. This the final operation.
    status = clEnqueueReadBuffer(queue, output_buf, CL_FALSE,
        0, N * sizeof(float), output, 1, &kernel_event, NULL);

    // Release local events.
    clReleaseEvent(write_event[0]);
    clReleaseEvent(write_event[1]);

	// Wait for all enqueued operations to finish.
	clFinish(queue);

	const double end_time = getCurrentTimestamp();

	// Wall-clock time taken.
	printf("\nTime: %0.3f ms\n", (end_time - start_time) * 1e3);

	// Get kernel times using the OpenCL event profiling API.
	cl_ulong time_ns = getStartEndTime(kernel_event);
	printf("Kernel time: %0.3f ms\n", double(time_ns) * 1e-6);

	// Release all events.
	clReleaseEvent(kernel_event);

	// Verify results.
	bool pass = true;
	for(unsigned j = 0; j < N && pass; ++j) {
		if(fabsf(output[j] - ref_output[j]) > 1.0e-5f) {
			printf("Failed verification @ index %d\nOutput: %f\nReference: %f\n", j, output[j], ref_output[j]);
			pass = false;
		}
	}
	printf("\nVerification: %s\n", pass ? "PASS" : "FAIL");
}

// Free the resources allocated during initialization
void cleanup() {
	if(kernel) {
		clReleaseKernel(kernel);
	}
	if(queue) {
		clReleaseCommandQueue(queue);
	}
	if(input_a_buf) {
		clReleaseMemObject(input_a_buf);
	}
	if(input_b_buf) {
		clReleaseMemObject(input_b_buf);
	}
	if(output_buf) {
		clReleaseMemObject(output_buf);
	}
	if(program) {
		clReleaseProgram(program);
	}
	if(context) {
		clReleaseContext(context);
	}
}

