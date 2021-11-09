#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "utils.h"
#include "defines.h"
#include "AOCLUtils/aocl_utils.h"

// Read BMP file and extract the pixel values (store in data) and header (store in header)
// data is data[0] = BLUE, data[1] = GREEN, data[2] = RED, etc...
bool read_bmp(const char* filename, unsigned char* header, struct pixel** data) {
   // struct pixel * data_temp;
   // unsigned char * header_temp;
   FILE* file = fopen(filename, "rb");
   
   if (!file) return false;
   
   // read the 54-byte header
   if (fread(header, sizeof(unsigned char), BMP_HEADER_SIZE, file) < BMP_HEADER_SIZE) return false; 

   // get height and width of image
   int width = *(int*)&header[18];
   int height = *(int*)&header[22];

   *data = (struct pixel*)aocl_utils::alignedMalloc(sizeof(struct pixel) * width * height);
   // Read in the image
   int size = width * height;
   if (fread(*data, sizeof(struct pixel), size, file) < size) return false; 
   fclose(file);
   
   return true;
}

void write_bmp(char* filename, unsigned char* header, struct pixel* data) {
	FILE* file = fopen(filename, "wb");

	// write the 54-byte header
	fwrite(header, sizeof(unsigned char), 54, file); 

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
	int size = width * height;
	fwrite(data, sizeof(struct pixel), size, file); 
	fclose(file);
}

double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec*1000 + (double)time.tv_usec /1000;
}
