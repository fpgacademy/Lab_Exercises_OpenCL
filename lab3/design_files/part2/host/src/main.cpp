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
cl_kernel kernels[2];
cl_program program;

enum {EDGE_KERNEL, HOUGH_KERNEL};

struct pixel *image_input = NULL;
cl_ushort *accum = NULL;
cl_mem edge_in_buffer, edge_out_buffer, hough_accum_out_buffer;

// Global variables.
short hysteresis_high_thresh;
short hysteresis_low_thresh;
int iterations;
std::string imageFilename;
std::string aocxFilename;
std::string deviceInfo;
unsigned char* bmp_header;
int cols, rows;
int width, height;
int num_lines;
char outputfile [256];

// A line (rho,theta) that goes from x1,y1 to x2,y2
struct line {
	short rho;
	unsigned short theta;
};

struct line * lines;

float sinvals[THETAS] = {0.0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765817, 0.10452846326765346, 0.12186934340514748, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791169081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735581699916, 0.29237170472273677, 0.3090169943749474, 0.32556815445715664, 0.3420201433256687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642332049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614753256582, 0.6293203910498374, 0.6427876096865393, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589973, 0.7071067811865475, 0.7193398003386511, 0.7313537016191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090169943749475, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8571673007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299167, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755993167, 0.9510565162951535, 0.9563047559630354, 0.9612616959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.981627183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415703, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1.0, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461516413221, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630355, 0.9510565162951536, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462991669, 0.8910065241883679, 0.8829475928589271, 0.8746197071393959, 0.8660254037844387, 0.8571673007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090169943749475, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537016191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6293203910498377, 0.6156614753256584, 0.6018150231520482, 0.5877852522924732, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642332049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927416, 0.37460659341591224, 0.3583679495453002, 0.3420201433256689, 0.32556815445715703, 0.3090169943749475, 0.29237170472273705, 0.27563735581699966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791169081775931, 0.19080899537654497, 0.17364817766693028, 0.15643446504023098, 0.13917310096006574, 0.12186934340514755, 0.10452846326765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344};
float cosvals[THETAS] = {1.0, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546151641322, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630354, 0.9510565162951535, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299167, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8571673007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550416, 0.8191520442889918, 0.8090169943749475, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537016191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6293203910498375, 0.6156614753256583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642332049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201433256688, 0.32556815445715676, 0.30901699437494745, 0.29237170472273677, 0.27563735581699916, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791169081775945, 0.19080899537654492, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846326765346, 0.08715574274765814, 0.06975647374412546, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123233995736766e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765824, -0.10452846326765333, -0.12186934340514737, -0.13917310096006535, -0.15643446504023104, -0.1736481776669303, -0.1908089953765448, -0.20791169081775912, -0.2249510543438648, -0.24192189559966779, -0.25881904510252085, -0.27563735581699905, -0.29237170472273666, -0.30901699437494734, -0.3255681544571564, -0.3420201433256687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614753256583, -0.6293203910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537016191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090169943749473, -0.8191520442889916, -0.8290375725550416, -0.8386705679454242, -0.848048096156426, -0.8571673007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299167, -0.9063077870366499, -0.9135454576426008, -0.9205048534524402, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755993167, -0.9510565162951535, -0.9563047559630354, -0.9612616959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.981627183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415703, -0.992546151641322, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913};

// Function prototypes.
void hough_flow();
void initCL();
void cleanup();
void teardown(int exit_status = 1);
void print_usage();

void extract_lines(unsigned short * accum, struct line ** lines, int num_lines) {
	
	struct line * temp_lines = (struct line *) malloc(num_lines * sizeof(struct line));

	// Your implementation here
	
	*lines = temp_lines;
}

// Functions for finding coordinate y = f(x), x = f(y). These coordinates correspond to the original 720x540 image.
int find_x(int y_bottomleft_origin, struct line l) {
	// Using the coordinate system with origin at center of image, rho = xcostheta + ysintheta
	int y_center_origin = y_bottomleft_origin - height/2;
	int x_center_origin = (l.rho*SUBSAMPLE_FACTOR - y_center_origin*sinvals[l.theta])/cosvals[l.theta];
	int x_bottomleft_origin = x_center_origin + width/2;
	return x_bottomleft_origin >= 0 && x_bottomleft_origin < width ? x_bottomleft_origin : -1;
}
int find_y(int x_bottomleft_origin, struct line l) {
	// Using the coordinate system with origin at center of image, rho = xcostheta + ysintheta
	int x_center_origin = x_bottomleft_origin - width/2;
	int y_center_origin = (l.rho*SUBSAMPLE_FACTOR - x_center_origin*cosvals[l.theta])/sinvals[l.theta];
	int y_bottomleft_origin = y_center_origin + height/2;
	return y_bottomleft_origin >= 0 && y_bottomleft_origin < height ? y_bottomleft_origin : -1;
}

// Overlay the lines onto the original image.
void overlay_lines(struct pixel * data, struct line * lines, int num_lines) {
	
	int i, y, x;
	
	// Overlay the lines
	for (i = 0; i < num_lines; i++){
		// Determine the points that exist on this line.
		// If the line is more vertical than horizontal, sweep across the y
		if (lines[i].theta < THETAS/4 || lines[i].theta >= 3*THETAS/4){
			for (y = 0; y < height; y++){
				x = find_x(y, lines[i]);
				if (x >= 0){
					data[y*width + x].r = 255;
					data[y*width + x].g = 0;
					data[y*width + x].b = 0;
				}
			}
		// If the line is more horizontal than vertical, sweep across the x
		} else if (lines[i].theta >= THETAS/4 && lines[i].theta < 3*THETAS/4){
			for (x = 0; x < width; x++){
				y = find_y(x, lines[i]);
				if (y >= 0){
					data[y*width + x].r = 255;
					data[y*width + x].g = 0;
					data[y*width + x].b = 0;
				}
			}
		}
	}
}

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
		hysteresis_high_thresh = 45;
	}
	
	// Optional hystersis threshold option.
	if(options.has("low_thresh")) {
		hysteresis_low_thresh = options.get<short>("low_thresh");  
	} else {
		hysteresis_low_thresh = 15;
	}
	
	// Relative path to aocx filename.
	if(options.has("aocx")) {
		aocxFilename = options.get<std::string>("aocx");  
	} else {
		aocxFilename = "hough";
	}

	if(options.has("num_lines")) {
		num_lines = options.get<int>("num_lines");  
	} else {
		num_lines = 5;
	}
	
	
	// Load the image.
	bmp_header = (unsigned char*) malloc(BMP_HEADER_SIZE*sizeof(unsigned char));
	if(!read_bmp(imageFilename.c_str(), bmp_header, &image_input)) {
		std::cerr << "Error: could not load " << argv[1] << std::endl;
		teardown(-1);
	}
	cols = *(int*)&bmp_header[18];
	rows = *(int*)&bmp_header[22];
	width = *(int*)&bmp_header[18];
	height = *(int*)&bmp_header[22];
	std::cout << "Input image dimensions: " << cols << "x" << rows << std::endl;
	
	// Make sure the image is 720 pixels wide
	if (cols != COLS){
		std::cerr << "Error: image should be " << COLS << " pixels wide, but is actually " << cols << std::endl;
		teardown(-1);
	}
	
	// Initializing OpenCL and the kernels.
	accum = (cl_ushort*)alignedMalloc(sizeof(unsigned short) * RHOS * THETAS);
	initCL();
	
	iterations = rows * cols;

	
	// Start measuring hough_flow time
	double start = get_wall_time();
	
	// Call the hough_flow.
	hough_flow();
	
	// Stop measuring the hough_flow time.
	double end = get_wall_time();
	printf("TIME ELAPSED: %.2f ms\n", end - start);
  
	/// Find the strongest num_lines lines
	extract_lines(accum, &lines, num_lines);
	
	/// Overlay the detected lines onto the original image
	overlay_lines(image_input, lines, num_lines);
	
	// Write out the original image with detected lines overlaid
	snprintf(outputfile, 256, "%s_part2_lines.bmp",imageFilename.c_str());
	printf("Writing line-detected image to %s\n",outputfile);
	write_bmp(outputfile, bmp_header, image_input);
	
	// Teardown OpenCL.
	teardown(0);
}

// The flow that includes edge detect + hough transform
void hough_flow() {
	size_t size = 1;
	cl_int status;
	
	/// SETUP EDGE KERNEL ARGUMENTS AND BUFFERS
	edge_in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(struct pixel) * rows * cols, NULL, &status);
	checkError(status, "Error: could not create edge_in_buffer");
	edge_out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned char) * rows * cols, NULL, &status);
	checkError(status, "Error: could not create edge_out_buffer");
	
	// Copy data to kernel input buffer.
	status = clEnqueueWriteBuffer(queue, edge_in_buffer, CL_TRUE, 0, sizeof(struct pixel) * rows * cols, image_input, 0, NULL, NULL);
	checkError(status, "Error: could not copy data into device");
	
	// Set the arguments for the edge detect kernel.
	status = clSetKernelArg(kernels[EDGE_KERNEL], 0, sizeof(cl_mem), (void*)&edge_in_buffer);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernels[EDGE_KERNEL], 1, sizeof(cl_mem), (void*)&edge_out_buffer);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernels[EDGE_KERNEL], 2, sizeof(int), (void*)&iterations); 
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernels[EDGE_KERNEL], 3, sizeof(short), (void*)&hysteresis_low_thresh);
	checkError(status, "Error: could not set ");
	status = clSetKernelArg(kernels[EDGE_KERNEL], 4, sizeof(short), (void*)&hysteresis_high_thresh);
	checkError(status, "Error: could not set ");

	/// SETUP HOUGH KERNEL ARGUMENTS AND BUFFERS
	// Your implementation here
	
	cl_event event;
	
	// Enqueue the edge kernel
	status = clEnqueueTask(queue, kernels[EDGE_KERNEL], 0, NULL, &event);
	checkError(status, "Error: failed to enqueue edgedetect");
	
	// Enqueue the hough kernel
	// Your implementation here

	// Wait for command queue to complete pending events.
	status = clFinish(queue);
	checkError(status, "Failed to finish");
	
	// Read output buffer from Hough kernel to accum
	// Your implementation here
}

void initCL() {
	cl_int status;

	// Start everything at NULL to help identify errors.
	kernels[EDGE_KERNEL] = NULL;
	kernels[HOUGH_KERNEL] = NULL;
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
	kernels[EDGE_KERNEL] = clCreateKernel(program, "edgedetect", &status);
	kernels[HOUGH_KERNEL] = clCreateKernel(program, "houghline", &status);
	checkError(status, "Failed to create kernel");
}

void cleanup() {
	// Called from aocl_utils::check_error, so there's an error.
	teardown(-1);
}

void teardown(int exit_status) {
	if(kernels[EDGE_KERNEL]) 
		clReleaseKernel(kernels[EDGE_KERNEL]);
	if(kernels[HOUGH_KERNEL]) 
		clReleaseKernel(kernels[HOUGH_KERNEL]);
	if(queue) 
		clReleaseCommandQueue(queue);
	
	if(image_input) alignedFree(image_input);
	if(accum) alignedFree(accum);
	if(edge_in_buffer) clReleaseMemObject(edge_in_buffer);
	if(edge_out_buffer) clReleaseMemObject(edge_out_buffer);
	if(program) clReleaseProgram(program);
	if(context) clReleaseContext(context);
	
	exit(exit_status);
}

void print_usage() {
	printf("\nUsage:\n");
	printf("\tlinedetect --img=<img> [--low_thresh=<int>] [--high_thresh=<int>] [--aocx=<aocx file>]\n\n");
	printf("Options:\n\n");
	printf("--img=<img>\n");
	printf("\tThe relative path to the input image to be line detected.\n\n");
	printf("[--low_thresh=<int>]\n");
	printf("\tSets the low threshold to be used in the hysteresis stage (default: 15).\n\n");
	printf("[--high_thresh=<int>]\n");
	printf("\tSets the high threshold to be used in the hysteresis stage (default: 45).\n\n");
	printf("[--aocx=<aocx file>]\n");
	printf("\tThe relative path to the aocx file without the .aocx suffix (default: edgedetect).\n\n");
	printf("[--num_lines=<int>]\n");
	printf("\tNumber of lines to extract from the accumulator (default: 5).\n\n");
}
