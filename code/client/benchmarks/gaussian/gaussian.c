#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define M_PI 3.1415926536

#define width 16
#define height 16

#define MAX_IMAGE_SIZE 512*512
#define MAX_WIDTH 512
#define MAX_HEIGHT 512

#include "taintgrind.h"

void filter(unsigned char* out,
          float sigma) {

	float r;
	float s = 2.0 * sigma * sigma;

    // sum is for normalization
    float sum = 0.0;
	float GKernel[5][5];

    // generating 5x5 kernel
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            r = sqrt(x * x + y * y);
            GKernel[x + 2][y + 2] = (exp(-(r * r) / s)) / (M_PI * s);
            sum += GKernel[x + 2][y + 2];
        }
    }

    // normalising the Kernel
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j) {

            GKernel[i][j] /= sum;
			// out[i*5+j] = GKernel[i][j]*255;
            // printf("%f \t", GKernel[i][j]);
		}

    //convert float to unsigned char - 4 bytes
    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 5; j++) {
            int tmp = GKernel[i][j]*10000;
            // cout << tmp <<" ";
            out[(i*5+j)*2] = (tmp & 0x00FF);
            out[(i*5+j)*2+1] = (tmp & 0xFF00) >> 8;
        }
    }
}

void gaussian_blur(unsigned char* image_input, unsigned char* image_output, uint64_t width, unit64_t height) {
//GKernel: 25
//image_input: 256
//image_output: 256
    float GKernel[5][5];

    uint64_t output_height = height - 5 + 1;
    uint64_t output_width = width - 5 + 1;

    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 5; j++) {

            int index = (i*5+j)*2;
            int tmp = image_input[0].range(8*index+7, 8*index) + (image_input[0].range((index+1)*8+7, (index+1)*8) << 8);
            GKernel[i][j] = (float)tmp/10000;
            // cout << tmp << " " << GKernel[i][j] << endl;
        }
        // cout << endl;
    }

     for(int i = 0 ; i < output_height; i++) {
        for(int j = 0; j < output_width; j++) {
            float sum = 0.0;
            for(int m = i; m <i+5; m++) {
                for(int n = j; n <j+5; n++) {
                    sum += GKernel[m-i][n-j] * image_input[m*width+n];
                }
            }
            image_output[i*output_width+j] = round(sum);
            // cout << sum << " " << (int)image_output[i*output_width+j]<< endl;
        }
    }     

}

int main() {
    int image_size = width*height;
    unsigned char image_data[width*height]= {0};
    unsigned char output[width*height] = {0};
    unsigned char gaussian_kernel[5*5*2] = {0};
    
    for(int i = 0; i < image_size; i++) {
        image_data[i] = i%256;
    }

    unsigned char gaussian_f[2] = {1, 80};
    unsigned char gaussian_lut[5*5*2] = {0};
    
    TNT_TAINT(image_data, sizeof(image_data));
    
    unsigned char input[width*height+5*5*2] = {0};

    filter(gaussian_kernel, gaussian_f);

    for(int i = 0; i < 5*5*2; i++) {
        input[i] = gaussian_lut[i];
    }
    for(int i = 0; i < image_size; i++) {
        input[i+5*5*3] = image_data[i];
    }
     
    gaussian_blur(input, output, image_size);

    return 0;

}