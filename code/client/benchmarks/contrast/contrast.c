#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MAX_IMAGE_SIZE 512*512

#define width 16
#define height 16
#define lut_size 256

#include "taintgrind.h"

void contrast_filter(unsigned char* contrast, unsigned char* contrast_lut) {

    float contrast_f = (float)contrast[0] + ((float)contrast[1]/100);
    float lut[256]={.0};

    for (int i = 0; i < 256; i++) {

        float value = (((float)i / 255.0) - 0.5) * contrast_f + 0.5;
        lut[i] = value * 255;
    }

    //form the output
    for(int i = 0; i < 256; i++) {

        if(lut[i] < 0) {
            contrast_lut[i*4] = 1;
            lut[i] = -lut[i];
            // cout  << lut[i] << " " << (int)contrast_lut[i*4] << " ";
        }
        else {
            contrast_lut[i*4] = 0;
        }
        contrast_lut[i*4+1] = (int)lut[i] / 256;
        contrast_lut[i*4+2] = (int)lut[i] % 256;
        contrast_lut[i*4+3] = (int)(lut[i]*100) % 100;
        // cout  << (int)contrast_lut[i*4+1] << " " << (int)contrast_lut[i*4+2] << " " << (int)contrast_lut[i*4+3] << endl;
    }

}

void contrast_apply(unsigned char* input_lut, unsigned char* input_data, unsigned char* output_image, int image_size) {
//lut: 256
//image: 256
//output: 256
    float lut[256]={.0};
    for(int i = 0; i < 256; i++) {
        lut[i] += input_lut[i*4+1]*256;
        lut[i] += input_lut[i*4+2];
        lut[i] += input_lut[i*4+3]/100;
        if(input_lut[i*4] == 1) {
            lut[i] = -lut[i];
        }
    }

    for (int i = 0; i < image_size; i++) {
        float tmp = lut[input_data[i]];
        if(tmp < 0) {
            output_image[i*4] = 1;
        }
        else {
            output_image[i*4] = 0;
        }
        output_image[i*4+1] = (int)tmp/256;
        output_image[i*4+2] = (int)tmp%256;
        output_image[i*4+3] = (int)(tmp*100)%100;
    }

}

int main() {
    int image_size = width*height;
    unsigned char image_data[width*height]= {0};
    unsigned char output[width*height] = {0};
    
    for(int i = 0; i < image_size; i++) {
        image_data[i] = i%256;
    }

    unsigned char contrast_f[2] = {1, 80};
    unsigned char contrast_lut[256] = {0};
    
    TNT_TAINT(image_data, sizeof(image_data));
    
    unsigned char input[width*height+256] = {0};

    contrast_filter(contrast_f, contrast_lut);

    for(int i = 0; i < 256; i++) {
        input[i] = contrast_lut[i];
    }
    for(int i = 0; i < image_size; i++) {
        input[i+256] = image_data[i];
    }
     
    contrast_apply(input, output, image_size);

    return 0;   
}


