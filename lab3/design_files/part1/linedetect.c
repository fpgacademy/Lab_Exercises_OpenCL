#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define HYSTERESIS_HIGH_THRESH 45
#define HYSTERESIS_LOW_THRESH 15
	
// The dimensions of the image
int width, height;

double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec*1000 + (double)time.tv_usec /1000;
}

struct pixel {
	unsigned char b;
	unsigned char g;
	unsigned char r;
};

// Read BMP file and extract the pixel values (store in data) and header (store in header)
// data is data[0] = BLUE, data[1] = GREEN, data[2] = RED, etc...
int read_bmp(char* filename, unsigned char** header, struct pixel** data) {
	struct pixel * data_temp;
	unsigned char * header_temp;
	FILE* file = fopen(filename, "rb");

	if (!file) return -1;
   
	// read the 54-byte header
	header_temp = (unsigned char *) malloc(54*sizeof(unsigned char));
	if (fread(header_temp, sizeof(unsigned char), 54, file) != 54){
		printf("Error reading BMP header\n");
		return -1;
	}

	// get height and width of image
	width = *(int*)&header_temp[18];
	height = *(int*)&header_temp[22];

	// Read in the image
	int size = width * height;
	data_temp = (struct pixel *) malloc(size*sizeof(struct pixel)); 
	if (fread(data_temp, sizeof(struct pixel), size, file) != size){
		printf("Error reading BMP image\n");
		return -1;
	}
	fclose(file);

	*header = header_temp;
	*data = data_temp;

	return 0;
}

void write_bmp(char* filename, unsigned char* header, struct pixel* data) {
	FILE* file = fopen(filename, "wb");

	// write the 54-byte header
	fwrite(header, sizeof(unsigned char), 54, file); 

	int size = width * height;
	fwrite(data, sizeof(struct pixel), size, file); 
	fclose(file);
}

// Write the grayscale image to disk.
void write_grayscale_bmp(char* filename, unsigned char* header, unsigned char* data) {
	FILE* file = fopen(filename, "wb");

	int size = width * height;
	struct pixel * data_temp = (struct pixel *) malloc(size*sizeof(struct pixel)); 

	// write the 54-byte header
	fwrite(header, sizeof(unsigned char), 54, file); 
	int y, x;
   
	// the r field of the pixel has the grayscale value. copy to g and b.
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			(*(data_temp + y*width + x)).b = (*(data + y*width + x));
			(*(data_temp + y*width + x)).g = (*(data + y*width + x));
			(*(data_temp + y*width + x)).r = (*(data + y*width + x));
		}
	}
   
	size = width * height;
	fwrite(data_temp, sizeof(struct pixel), size, file); 

	free(data_temp);
	fclose(file);
}

// Determine the grayscale 8 bit value by averaging the r, g, and b channel values.
void convert_to_grayscale(struct pixel * data, unsigned char ** converted_data) {
	int i;
	*converted_data = (unsigned char*) malloc(width * height*sizeof(unsigned char));

	for (i = 0; i < width*height; i++) {
		(*converted_data)[i] = (data[i].r + data[i].g + data[i].b)/3;
	}
}

// Gaussian blur. 
void gaussian_blur(unsigned char ** data) {
	unsigned int gaussian_filter[5][5] = {
		{ 2, 4, 5, 4, 2 },
		{ 4, 9,12, 9, 4 },
		{ 5,12,15,12, 5 },
		{ 4, 9,12, 9, 4 },
		{ 2, 4, 5, 4, 2 }
	};
	int x, y, i, j;
	unsigned int numerator_r, denominator;
   
	unsigned char * temp_data = (unsigned char*) malloc(width * height*sizeof(unsigned char)); 
   
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			numerator_r = 0;
			denominator = 0;
			for (j = -2; j <= 2; j++) {
				for (i = -2; i <= 2; i++) {
					if ( (x+i) >= 0 && (x+i) < width && (y+j) >= 0 && (y+j) < height) {
						numerator_r += (*(*data + (y+j)*width + (x+i)))*gaussian_filter[i+2][j+2];
						denominator += gaussian_filter[i+2][j+2];
					}
				}
			}
			(*(temp_data + y*width + x)) = numerator_r/denominator;
		}
	}
	free(*data);
	*data = temp_data;
}

void sobel_filter(unsigned char ** data) {
	
	// Your implementation here
	
}

void non_maximum_suppressor(unsigned char ** data) {
	
	// Your implementation here
	
}

// Only keep pixels that are next to at least one strong pixel.
void hysteresis_filter(unsigned char ** data) {
	
	// Your implementation here
	
}

// Hough transform constants for a 720x540 image
#define ROWS 540
#define COLS 720
// The x and y ranges for the Hough transform sweep
#define X_START -COLS/2
#define Y_START -ROWS/2
#define X_END COLS/2
#define Y_END 0 // Only sweep the bottom half of the image to ignore the horizon
// Accumulator dimensions
#define RHO_RESOLUTION 2
#define RHOS (900/RHO_RESOLUTION) // How many values of rho do we go through? Sqrt(ROWS^2 + COLS^2) = 900.
#define THETAS 180 // How many values of theta do we go through? 

// sin and cos lookup values for thetas 0 to 179
float sinvals[THETAS] = {0.0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765817, 0.10452846326765346, 0.12186934340514748, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791169081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735581699916, 0.29237170472273677, 0.3090169943749474, 0.32556815445715664, 0.3420201433256687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642332049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614753256582, 0.6293203910498374, 0.6427876096865393, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589973, 0.7071067811865475, 0.7193398003386511, 0.7313537016191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090169943749475, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8571673007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299167, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755993167, 0.9510565162951535, 0.9563047559630354, 0.9612616959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.981627183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415703, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1.0, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461516413221, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630355, 0.9510565162951536, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462991669, 0.8910065241883679, 0.8829475928589271, 0.8746197071393959, 0.8660254037844387, 0.8571673007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090169943749475, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537016191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6293203910498377, 0.6156614753256584, 0.6018150231520482, 0.5877852522924732, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642332049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927416, 0.37460659341591224, 0.3583679495453002, 0.3420201433256689, 0.32556815445715703, 0.3090169943749475, 0.29237170472273705, 0.27563735581699966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791169081775931, 0.19080899537654497, 0.17364817766693028, 0.15643446504023098, 0.13917310096006574, 0.12186934340514755, 0.10452846326765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344};
float cosvals[THETAS] = {1.0, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546151641322, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630354, 0.9510565162951535, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299167, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8571673007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550416, 0.8191520442889918, 0.8090169943749475, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537016191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6293203910498375, 0.6156614753256583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642332049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201433256688, 0.32556815445715676, 0.30901699437494745, 0.29237170472273677, 0.27563735581699916, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791169081775945, 0.19080899537654492, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846326765346, 0.08715574274765814, 0.06975647374412546, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123233995736766e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765824, -0.10452846326765333, -0.12186934340514737, -0.13917310096006535, -0.15643446504023104, -0.1736481776669303, -0.1908089953765448, -0.20791169081775912, -0.2249510543438648, -0.24192189559966779, -0.25881904510252085, -0.27563735581699905, -0.29237170472273666, -0.30901699437494734, -0.3255681544571564, -0.3420201433256687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614753256583, -0.6293203910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537016191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090169943749473, -0.8191520442889916, -0.8290375725550416, -0.8386705679454242, -0.848048096156426, -0.8571673007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299167, -0.9063077870366499, -0.9135454576426008, -0.9205048534524402, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755993167, -0.9510565162951535, -0.9563047559630354, -0.9612616959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.981627183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415703, -0.992546151641322, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913};

// data from BMP is stored with the bottom-left pixel first (origin is at bottom left)
void hough_transform(unsigned char * data, unsigned short ** accum) {
	
	if (height != ROWS || width != COLS){
		printf("ERROR: Invalid image dimensions for the hough transform\n");
		return;
	}
	
	unsigned short * temp_accum = (unsigned short*) malloc(RHOS * THETAS * sizeof(unsigned short)); 
	
	// Your implementation here
	
	*accum = temp_accum;
}

// A line (rho,theta)
struct line {
	short rho;
	unsigned short theta;
};

// Find num_lines lines with the highest accumulated values (local maxima)
void extract_lines(unsigned short * accum, struct line ** lines, int num_lines) {
	
	struct line * temp_lines = (struct line *) malloc(num_lines * sizeof(struct line));

	// Your implementation here
	
	*lines = temp_lines;
}

// Functions for finding coordinates x,y coordinates from a rho,theta line. 
// These x,y coordinates correspond to the coordinate system of the original 720x540 image,
// which as its origin at the bottom left of the image.
int find_x(int y_bottomleft_origin, struct line l) {
	// Using the coordinate system with origin at center of image, rho = xcostheta + ysintheta
	int y_center_origin = y_bottomleft_origin - height/2;
	int x_center_origin = (l.rho*RHO_RESOLUTION - y_center_origin*sinvals[l.theta])/cosvals[l.theta];
	int x_bottomleft_origin = x_center_origin + width/2;
	return x_bottomleft_origin >= 0 && x_bottomleft_origin < width ? x_bottomleft_origin : -1;
}
int find_y(int x_bottomleft_origin, struct line l) {
	// Using the coordinate system with origin at center of image, rho = xcostheta + ysintheta
	int x_center_origin = x_bottomleft_origin - width/2;
	int y_center_origin = (l.rho*RHO_RESOLUTION - x_center_origin*cosvals[l.theta])/sinvals[l.theta];
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

int main(int argc, char *argv[]) {
	struct pixel * rgb_data;
	unsigned char * data;
	unsigned char * header;
	unsigned short * accum = NULL;
	struct line * lines;
	int debug = 0;
	double start, end, func_start, hough_runtime, edge_runtime;
	char outputfile [256];
	int num_lines;

	// Check inputs
	if (argc < 4) {
		printf("Usage: edgedetect <BMP filename> <debug> <num lines to find>\n\ndebug: true false\n");
		return 0;
	}

	// Determine debug mode
	if (argv[2][0] == 't') debug = 1;

	// Parse number of lines to find
	num_lines = atoi(argv[3]);

	// Open input image file (24-bit bitmap image)
	if (read_bmp(argv[1], &header, &rgb_data) < 0) {
		printf("Failed to read BMP\n");
		return 0;
	}

	/********************************************
	*          IMAGE PROCESSING STAGES          *
	********************************************/

	// Start measuring time
	start = get_wall_time();
	
	/// Grayscale conversion
	convert_to_grayscale(rgb_data, &data);

	/// Gaussian filter
	gaussian_blur(&data);

	/// Sobel operator
	sobel_filter(&data);

	/// Non-maximum suppression
	non_maximum_suppressor(&data);

	/// Hysteresis
	hysteresis_filter(&data);
	if (debug) {
		snprintf(outputfile, 256, "%s_part1_edges.bmp",argv[1]);
		printf("Writing edge-detected image to %s\n",outputfile);
		write_grayscale_bmp(outputfile, header, data);
	}
	
	edge_runtime = get_wall_time() - start;
	
	/// Hough
	func_start = get_wall_time();
	hough_transform(data, &accum); 
	hough_runtime = get_wall_time() - func_start;
	
	/// Find the strongest num_lines lines
	extract_lines(accum, &lines, num_lines);
	
	// Stop measuring time.
	end = get_wall_time();
	
	printf("EDGE DETECTION TIME ELAPSED: %.2f ms\n", edge_runtime);
	printf("HOUGH TRANSFORM TIME ELAPSED: %.2f ms\n", hough_runtime);
	printf("TOTAL TIME ELAPSED: %.2f ms\n", end - start);
	
	/// Overlay the detected lines onto the original image
	overlay_lines(rgb_data, lines, num_lines);
	
	// Write out the original image with detected lines overlaid
	snprintf(outputfile, 256, "%s_part1_lines.bmp",argv[1]);
	printf("Writing line-detected image to %s\n",outputfile);
	write_bmp(outputfile, header, rgb_data);

	return 0;
}


