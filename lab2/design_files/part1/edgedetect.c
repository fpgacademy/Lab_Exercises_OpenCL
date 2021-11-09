#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define high_threshold 48
#define low_threshold 12
	
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
   header_temp = malloc(54*sizeof(unsigned char));
   if (fread(header_temp, sizeof(unsigned char), 54, file) != 54){
		printf("Error reading BMP header\n");
		return -1;
   }   

   // get height and width of image
   width = *(int*)&header_temp[18];
   height = *(int*)&header_temp[22];

   // Read in the image
   int size = width * height;
   data_temp = malloc(size*sizeof(struct pixel)); 
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
   struct pixel * data_temp = malloc(size*sizeof(struct pixel)); 
   
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
   
   unsigned char * temp_data = malloc(width * height*sizeof(unsigned char)); 
   
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

int main(int argc, char *argv[]) {
	struct pixel * rgb_data;
	unsigned char * data;
	unsigned char * header;
	int debug = 0;
	double start, end;

	// Check inputs
	if (argc < 3) {
		printf("Usage: edgedetect <BMP filename> <debug>\n\ndebug: true false\n");
		return 0;
	}

	// Determine debug mode
	if (argv[2][0] == 't') debug = 1;

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
	if (debug) write_grayscale_bmp("stage0_grayscale.bmp", header, data);

	/// Gaussian filter
	gaussian_blur(&data);
	if (debug) write_grayscale_bmp("stage1_gaussian.bmp", header, data);

	/// Sobel operator
	sobel_filter(&data);
	if (debug) write_grayscale_bmp("stage2_sobel.bmp", header, data);

	/// Non-maximum suppression
	non_maximum_suppressor(&data);
	if (debug) write_grayscale_bmp("stage3_nonmax_suppression.bmp", header, data);

	/// Hysteresis
	hysteresis_filter(&data);
	if (debug) write_grayscale_bmp("stage4_hysteresis.bmp", header, data);

	// Stop measuring time.
	end = get_wall_time();

	printf("TIME ELAPSED: %.2f ms\n", end - start);

	write_grayscale_bmp("edges.bmp", header, data);

	return 0;
}


