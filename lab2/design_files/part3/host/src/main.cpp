#define NOMINMAX // so that windows.h does not define min/max macros

#include <algorithm>
#include <iostream>
#include <time.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"
#include "defines.h"
#include "utils.h"

using namespace aocl_utils;

// OpenCL Global Variables.
cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_kernel kernel;
cl_program program;

struct pixel *h_input = NULL;
cl_uchar *h_output = NULL;
cl_mem c_in_buffer, c_out_buffer;

// Global variables.
short hysteresis_high_thresh;
short hysteresis_low_thresh;
int iterations;
std::string imageFilename;
std::string aocxFilename;
std::string deviceInfo;
unsigned char* bmp_header;
int cols, rows;

// Function prototypes.
void filter();
void initCL();
void cleanup();
void teardown(int exit_status = 1);
void print_usage();

int main(int argc, char **argv) {
	// Parsing command line arguments.
	Options options(argc, argv);

	// Relative path to image filename option.
	if(options.has("img")) {
		imageFilename = options.get<std::string>("img");
	} else {
		print_usage();
		return 0;
	}
	
	// Optional sobel threshold option.
	if(options.has("high_thresh")) {
		hysteresis_high_thresh = options.get<short>("high_thresh");  
	} else {
		hysteresis_high_thresh = 48;
	}
	
	// Optional hystersis threshold option.
	if(options.has("low_thresh")) {
		hysteresis_low_thresh = options.get<short>("low_thresh");  
	} else {
		hysteresis_low_thresh = 12;
	}
	
	// Relative path to aocx filename.
	if(options.has("aocx")) {
		aocxFilename = options.get<std::string>("aocx");  
	} else {
		aocxFilename = "edgedetect";
	}
	
	// Load the image.
	bmp_header = (unsigned char*) malloc(BMP_HEADER_SIZE*sizeof(unsigned char));
	if(!read_bmp(imageFilename.c_str(), bmp_header, &h_input)) {
		std::cerr << "Error: could not load " << argv[1] << std::endl;
		teardown(-1);
	}
	cols = *(int*)&bmp_header[18];
	rows = *(int*)&bmp_header[22];
	std::cout << "Input image dimensions: " << cols << "x" << rows << std::endl;
	
	// Make sure the image is 720 pixels wide
	if (cols != COLS){
		std::cerr << "Error: image should be " << COLS << " pixels wide, but is actually " << cols << std::endl;
		teardown(-1);
	}
	
	// Initializing OpenCL and the kernels.
	h_output = (cl_uchar*)alignedMalloc(sizeof(unsigned char) * rows * cols);
	initCL();
	
	iterations = rows * cols;

	
	// Start measuring edge detect runtime
	double start = get_wall_time();
	
	// Call the filter.
	filter();
	
	// Stop measuring the edge detect runtime.
	double end = get_wall_time();
	printf("TIME ELAPSED: %.2f ms\n", end - start);
  
	// Dump out the resulting edge detected image to disk.
	printf("Writing resulting edge detected image to edges.bmp\n");
	write_bmp("edges.bmp", bmp_header, (unsigned char*) h_output);
	
	// Teardown OpenCL.
	teardown(0);
}

void filter() {
	size_t size = 1;
	cl_int status;
	
	// Create kernel input and output buffers.
	c_in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(struct pixel) * rows * cols, NULL, &status);
	checkError(status, "Error: could not create device buffer");
	c_out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned char) * rows * cols, NULL, &status);
	checkError(status, "Error: could not create output buffer");
	
	// Copy data to kernel input buffer.
	status = clEnqueueWriteBuffer(queue, c_in_buffer, CL_TRUE, 0, sizeof(struct pixel) * rows * cols, h_input, 0, NULL, NULL);
	checkError(status, "Error: could not copy data into device");
	
	// Set the arguments for the edge detect kernel.
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&c_in_buffer);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&c_out_buffer);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernel, 2, sizeof(int), (void*)&iterations);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernel, 3, sizeof(short), (void*)&hysteresis_low_thresh);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernel, 4, sizeof(short), (void*)&hysteresis_high_thresh);
	checkError(status, "Error: could not set ");

	cl_event event;
	
	// Enqueue the kernel. //
	status = clEnqueueTask(queue, kernel, 0, NULL, &event);
	checkError(status, "Error: failed to launch sobel");
	
	// Wait for command queue to complete pending events.
	status = clFinish(queue);
	checkError(status, "Failed to finish");
	
	// Read output buffer from kernel.
	status = clEnqueueReadBuffer(queue, c_out_buffer, CL_TRUE, 0, sizeof(char) * rows * cols, h_output, 0, NULL, NULL);
	checkError(status, "Error: could not copy data from device");
}

void initCL() {
	cl_int status;

	// Start everything at NULL to help identify errors.
	kernel = NULL;
	queue = NULL;
	
	// Locate files via. relative paths.
	if(!setCwdToExeDir()) {
		teardown();
	}

	// Get the OpenCL platform.
	platform = findPlatform("Intel(R) FPGA");
	if (platform == NULL) {
		teardown();
	}

	// Get the first device.
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
	checkError (status, "Error: could not query devices");

	char info[256];
	clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(info), info, NULL);
	deviceInfo = info;

	// Create the context.
	context = clCreateContext(0, 1, &device, &oclContextCallback, NULL, &status);
	checkError(status, "Error: could not create OpenCL context");

	// Create the command queues for the kernels.
	queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);

	// Create the program.
	std::string binary_file = getBoardBinaryFile(aocxFilename.c_str(), device);
	std::cout << "Using AOCX: " << binary_file << "\n";
	program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

	// Build the program that was just created.
	status = clBuildProgram(program, 1, &device, "", NULL, NULL);
	checkError(status, "Error: could not build program");
	
	// Create the kernel - name passed in here must match kernel name in the original CL file.
	kernel = clCreateKernel(program, "edgedetect", &status);
	checkError(status, "Failed to create kernel");
}

void cleanup() {
	// Called from aocl_utils::check_error, so there's an error.
	teardown(-1);
}

void teardown(int exit_status) {
	if(kernel) 
		clReleaseKernel(kernel);
	if(queue) 
		clReleaseCommandQueue(queue);
	
	if(h_input) alignedFree(h_input);
	if(h_output) alignedFree(h_output);
	if(c_in_buffer) clReleaseMemObject(c_in_buffer);
	if(c_out_buffer) clReleaseMemObject(c_out_buffer);
	if(program) clReleaseProgram(program);
	if(context) clReleaseContext(context);
	
	exit(exit_status);
}

void print_usage() {
	printf("\nUsage:\n");
	printf("\tedgedetect --img=<img> [--low_thresh=<int>] [--high_thresh=<int>] [--aocx=<aocx file>]\n\n");
	printf("Options:\n\n");
	printf("--img=<img>\n");
	printf("\tThe relative path to the input image to be edge detected.\n\n");
	printf("[--low_thresh=<int>]\n");
	printf("\tSets the low threshold to be used in the hysteresis stage (default: 12).\n\n");
	printf("[--high_thresh=<int>]\n");
	printf("\tSets the high threshold to be used in the hysteresis stage (default: 48).\n\n");
	printf("[--aocx=<aocx file>]\n");
	printf("\tThe relative path to the aocx file without the .aocx suffix (default: edgedetect).\n\n");
}
