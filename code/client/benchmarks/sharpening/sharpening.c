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

void filter(unsigned char* out, unsigned char* sigma_u) {
    int size = 5;
    int i, j, k = 0;
    float sum = 0.0;
    float sigma = sigma_u[0] + (float)sigma_u[1]/100;
    float filter[25] = {0};

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            int x = i - size / 2;
            int y = j - size / 2;
            float val = -(1.0 / (M_PI * sigma * sigma * sigma * sigma)) *
                (1.0 - (x * x + y * y) / (2.0 * sigma * sigma)) *
                exp(-(x * x + y * y) / (2.0 * sigma * sigma));
            filter[k++] = val;
            sum += val;
        }
    }

    for (i = 0; i < size * size; i++) {
        filter[i] /= sum;
    }

    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 5; j++) {
            int tmp = filter[i*5+j]*10000;
            if(tmp < 0) {
                out[(i*5+j)*3] = 1;
                tmp = -tmp;
            }
            else {
                out[(i*5+j)*3] = 0;
            }
            // cout << tmp <<" ";
            out[(i*5+j)*3+1] = (tmp & 0x00FF);
            out[(i*5+j)*3+2] = (tmp & 0xFF00) >> 8;
            //test code
            // float tmp_f = 0;
            // tmp_f = (float)(out[(i*5+j)*3+1] + (out[(i*5+j)*3+2] << 8))/10000;
            // cout << tmp_f <<" ";
        }
    }
}

void sharpening_apply(unsigned char* input, unsigned char* output, int image_width, int image_height) 
{

    float GKernel[5][5] = {0};
    int output_height = image_height -5 + 1;
    int output_width = image_width - 5 + 1;

    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 5; j++) {

            float tmp = input[(i*5+j)*3+1] + (input[(i*5+j)*3+2] << 8);
            GKernel[i][j] = (float)tmp/10000; 
            if(input[(i*5+j)*3] == 1) {
                GKernel[i][j] = -GKernel[i][j];
            } 
            // cout << GKernel[i][j] << " ";
        }
        // cout << endl;
    }

   
    for(int i = 0 ; i < output_height; i++) {
        for(int j = 0; j < output_width; j++) {
            float sum = 0.0;
            for(int m = i; m <i+5; m++) {
                for(int n = j; n <j+5; n++) {
                    sum += GKernel[m-i][n-j] * input[m*output_width+n + 5*5*3];
                }
            }
			//three bytes to record the 
			int tmp = round(sum);
			if(tmp < 0) {
				output[(i*output_width+j)*3] = 1;
				tmp = -tmp;
			}
			else {
				output[(i*output_width+j)*3] = 0;
			}
			output[(i*output_width+j)*3+1] = tmp/256;
			output[(i*output_width+j)*3+2] = tmp%256;
            // cout << sum << " " << (int)output[(i*output_width+j)*3] << " " << (int)output[(i*output_width+j)*3+1] << " " << (int)output[(i*output_width+j)*3+2] << endl;
            // cout << hex << (int)output[(i*output_width+j)*3] << " " << hex << (int)output[(i*output_width+j)*3+1] << " " << hex << (int)output[(i*output_width+j)*3+2] << endl;
        }
    }

}

int main() {
    int image_size = width*height;
    unsigned char image_data[width*height]= {0};
    unsigned char output[width*height] = {0};
    unsigned char kernel[5*5*3] = {0};
    
    for(int i = 0; i < image_size; i++) {
        image_data[i] = i%256;
    }

    unsigned char f[2] = {1, 80};
    unsigned char lut[5*5*3] = {0};
    
    TNT_TAINT(image_data, sizeof(image_data));
    
    unsigned char input[width*height+5*5*3] = {0};

    filter(kernel, f);

    for(int i = 0; i < 5*5*3; i++) {
        input[i] = lut[i];
    }
    for(int i = 0; i < image_size; i++) {
        input[i+5*5*3] = image_data[i];
    }
     
    sharpening_apply(input, output, image_size);

    return 0;

}
