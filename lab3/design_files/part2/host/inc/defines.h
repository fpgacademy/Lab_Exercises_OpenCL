#ifndef __DEFINES_H_
#define __DEFINES_H_

#define BMP_HEADER_SIZE 54
#define ROWS 540
#define COLS 720
#define X_START -COLS/2
#define Y_START -ROWS/2
#define X_END COLS/2
#define Y_END ROWS/2
#define SUBSAMPLE_FACTOR 2
#define RHOS (900/SUBSAMPLE_FACTOR) // How many values of rho do we go through? Use 1 pixel resolution. Sqrt(ROWS^2 + COLS^2) = 450.
#define THETAS 180 // How many values of theta do we go through? Do 180/128 = 1.40625 degree resolution.
#define THETA_INCREMENT (180.0f/THETAS)

#endif
