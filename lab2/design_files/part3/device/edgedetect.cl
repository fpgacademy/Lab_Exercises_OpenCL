#define COLS 720
#define SR_LENGTH_5x5 (4 * COLS + 5)

// SR_INIT_CYCLES is how many elements must be shifted into the SR before starting the 5x5/3x3 box operation.
// The operation can start when the top left corner of the image is in the frame. 
#define SR_INIT_CYCLES_5x5 (2*COLS + 3)

// INITIALIZATION_CYCLES are used to initialize the shift registers. This is the number of
// cycles before outputting the first edge-detected pixel to image_out.
// Once they are initialized (at count == 0) we start outputting pixels to image_out.
// The -1 for the fact that pixels are loaded at the start of the cycle (INITIALIZATION_CYCLES + 1
// pixels will have been shifted in by cycle 0, when output starts).
#define INITIALIZATION_CYCLES (SR_INIT_CYCLES_5x5 - 1)

struct pixel {
   unsigned char b;
   unsigned char g;
   unsigned char r;
};

// canny edge detection kernel
__kernel
void edgedetect(global const struct pixel * restrict image_in, global unsigned char * restrict image_out,
				const int iterations, const short low_threshold, const short high_threshold)
{
    // Filter coefficients
    unsigned char gaussian[5][5] = {{2,4,5,4,2},{4,9,12,9,4},{5,12,15,12,5},{4,9,12,9,4},{2,4,5,4,2}};

	// Pixel buffer of 4 rows and 5 extra pixels for doing 5x5 box operations
	unsigned char rows_post_grayscale[SR_LENGTH_5x5];
	
    int count = -INITIALIZATION_CYCLES; 
	int nth_pixel = 0;
	
    while (count != iterations) {
		
        // Shift registers
        #pragma unroll
        for (int i = SR_LENGTH_5x5 - 1; i > 0; --i) {
			rows_post_grayscale[i] = rows_post_grayscale[i - 1];
		}
		
		/// Load pixel + Grayscale Conversion
		unsigned char pixel_grayscale_8bit = 0;
		if (count < (iterations - INITIALIZATION_CYCLES)){
			struct pixel pixel_color_24bit = image_in[nth_pixel++];
			pixel_grayscale_8bit = ((unsigned short)pixel_color_24bit.r + (unsigned short)pixel_color_24bit.g + (unsigned short)pixel_color_24bit.b)/3;
		}
        rows_post_grayscale[0] = pixel_grayscale_8bit;

		// As a placeholder, do a null operation (simply pass on the grayscale pixel 
		// at the center of the frame to image_out). You should replace the code
		// to carry out the necessary box operations before outputting to image_out.
		if (count >= 0) {
			image_out[count] = rows_post_grayscale[2 * COLS + 2];
		}
		
        count++;
    }
}
