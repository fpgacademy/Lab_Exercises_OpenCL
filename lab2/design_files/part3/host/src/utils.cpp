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

void write_bmp(const char* filename, unsigned char* header, unsigned char* data) {
    FILE* file = fopen(filename, "wb");
	
	// Write the 54-byte header.
    fwrite(header, sizeof(unsigned char), BMP_HEADER_SIZE, file); 

    // Get height and width of image.
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int size = width * height;
    
    // Write data to a non-padded buffer.
    unsigned char *data_temp = (unsigned char*) malloc(size*3*sizeof(unsigned char));
    unsigned char *data_temp_ptr = data_temp;
    unsigned char *data_ptr = data;
    for(int i = 0; i < size; i++){
        data_temp_ptr[i*3+2] = data_ptr[i]; //r
        data_temp_ptr[i*3+1] = data_ptr[i]; //g
        data_temp_ptr[i*3+0] = data_ptr[i]; //b
    }
    
    fwrite(data_temp, sizeof(unsigned char), size*3, file); // write the rest of the data at once
    fclose(file);
    free(data_temp);
}

void write_grayscale_bmp(const char* filename, unsigned char* header, unsigned char* data) {
	// Get height and width of image.
    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int size = width * height;
	
	// Copy red (R) channel to the blue (B) and green (G) channels.
	for(int i = 0; i < size; i++){
		data[i*4+1] = data[i*4+0];	// Copy R to G.
		data[i*4+2] = data[i*4+0];	// Copy R to B.
		data[i*4+3] = 0;			// Don't care.
    }
	
	// Perform the write.
	write_bmp(filename, header, data);
}

double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec*1000 + (double)time.tv_usec /1000;
}
