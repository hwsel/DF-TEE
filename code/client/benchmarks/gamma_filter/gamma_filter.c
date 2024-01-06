#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define width 16
#define height 16
#define lut_size 256

#include "taintgrind.h"

void gamma_filter( unsigned char* gamma, unsigned char* gamma_lut) {
    float gamma_f = (float)gamma[0] + ((float)gamma[1]/100);
    float inv_gamma = 1.0 / gamma_f;
    
    for (int i = 0; i < 256; i++) {
        gamma_lut[i] = (unsigned char)(pow((float)i / 255.0, inv_gamma) * 255.0 + 0.5);
    }
} // end of function


void gamma_apply(unsigned char* input, unsigned char* output, int size){
//lut: 256
//image: 256
//output: 256
    unsigned char lut[256] = {0};
    for(int i = 0; i < 256; i++) {
        lut[i] = input[i];
    }
    for (int i = 0; i < size; i++) {
        output[i] = lut[input[i+256]];
    }
} //end of function

int main() {

    int image_size = width*height;
    unsigned char image_data[width*height]= {0};
    unsigned char output[width*height] = {0};
    
    for(int i = 0; i < image_size; i++) {
        image_data[i] = i%256;
    }

    unsigned char gamma_f[2] = {1, 80};
    unsigned char gamma_lut[256] = {0};
    
    TNT_TAINT(image_data, sizeof(image_data));

    gamma_filter(gamma_f, gamma_lut);

    unsigned char input[width*height+256] = {0};
    for(int i = 0; i < 256; i++) {
        input[i] = gamma_lut[i];
    }
    for(int i = 0; i < image_size; i++) {
        input[i+256] = image_data[i];
    }
     
    gamma_apply(input, output, image_size);

    return 0;
}