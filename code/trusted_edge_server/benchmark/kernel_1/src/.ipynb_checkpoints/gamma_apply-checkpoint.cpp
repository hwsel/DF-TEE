
// Includes
#include <hls_stream.h>
#include <hls_math.h>
#include <ap_int.h>
#include <stdlib.h>
#include <stdint.h>


#define MAX_IMAGE_SIZE 512*512

void from_axi(ap_uint<512>* in, hls::stream<ap_uint<128>>& rawStrm, hls::stream<bool>& endRawStrm, int size) {
    ap_uint<128> tmp_msg = 0;
    for(int i = 0; i < size/16; i++) {
        int inner_idx = i%4;
        tmp_msg = in[i/4].range((inner_idx+1)*128-1, inner_idx*128);
        rawStrm.write(tmp_msg);
        endRawStrm.write(0);
    }
    endRawStrm.write(1);
}

void from_strm(unsigned char* data_in, hls::stream<ap_uint<128>>& rawStrm, hls::stream<bool>& endRawStrm, int size) {
    ap_uint<128> tmp = 0;
    ap_uint<64> index_i = 0; 
    while(!endRawStrm.read()) {
#pragma HLS PIPELINE II = 1  
        tmp = rawStrm.read();
        for(int j = 0; j < 16; j++) {
            data_in[index_i*16+j] = tmp.range((j+1)*8-1, j*8);
        }
        index_i++;
    }   
}

void to_strm(unsigned char* data_out, hls::stream<ap_uint<128>>& rawStrm, hls::stream<bool>& endRawStrm, int size) {
    ap_uint<128> tmp_msg = 0;
    for(int i = 0; i < size/16; i++) {
#pragma HLS pipeline II = 1
        for(int j = 0; j < 16; j++) {
#pragma HLS unroll     
            tmp_msg.range((j+1)*8-1, j*8) = data_out[j+i*16];   
        }
        rawStrm.write(tmp_msg);
        endRawStrm.write(0);
    }
    endRawStrm.write(1);
}

void to_axi(ap_uint<512>* out, hls::stream<ap_uint<128>>& rawStrm, hls::stream<bool>& endRawStrm, int size) {
    ap_uint<128> tmp = 0;
    ap_uint<64> index_i = 0;
    while(!endRawStrm.read()) {
#pragma HLS PIPELINE II = 1  
        tmp = rawStrm.read();
        out[index_i/4].range(((index_i)%4)*128+127, ((index_i%4)*128)) = tmp;
        index_i++;
    }

}

void read_data_from_axi(ap_uint<512>* in, unsigned char* data_in, int size) {
#pragma HLS dataflow
    hls::stream<ap_uint<128>> rawStrm;
#pragma HLS stream variable = rawStrm depth = 128
#pragma HLS resource variable = rawStrm core = FIFO_LUTRAM
    hls::stream<bool> endRawStrm;
#pragma HLS stream variable = endRawStrm depth = 64
#pragma HLS resource variable = endRawStrm core = FIFO_LUTRAM  
    from_axi(in, rawStrm, endRawStrm, size);
    from_strm(data_in, rawStrm, endRawStrm, size);
}

void write_data_to_axi(unsigned char* data_out, ap_uint<512>* out, int size) {
#pragma HLS dataflow
    hls::stream<ap_uint<128>> rawStrm;
#pragma HLS stream variable = rawStrm depth = 128
#pragma HLS resource variable = rawStrm core = FIFO_LUTRAM
    hls::stream<bool> endRawStrm;
#pragma HLS stream variable = endRawStrm depth = 64
#pragma HLS resource variable = endRawStrm core = FIFO_LUTRAM      
    to_strm(data_out, rawStrm, endRawStrm, size);
    to_axi(out, rawStrm, endRawStrm, size);
}

extern "C" {
void gamma_apply(ap_uint<512>* image_data, ap_uint<512>* output_image, uint64_t size) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_0 port = image_data

#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_1 port = output_image
    
#pragma HLS INTERFACE s_axilite port = image_data bundle = control
#pragma HLS INTERFACE s_axilite port = output_image bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    unsigned char lut[256];

//     hls::stream<ap_uint<128> > rawStrm;
// #pragma HLS stream variable = rawStrm depth = 128
// #pragma HLS resource variable = rawStrm core = FIFO_LUTRAM

//     hls::stream<bool> endTextStrm;
// #pragma HLS stream variable = endTextStrm depth = 64
// #pragma HLS resource variable = endTextStrm core = FIFO_LUTRAM

//     hls::stream<ap_uint<128> > decStrm;
// #pragma HLS stream variable = decStrm depth = 128
// #pragma HLS resource variable = decStrm core = FIFO_LUTRAM

//     hls::stream<bool> endDecStrm;
// #pragma HLS stream variable = endDecStrm depth = 128
// #pragma HLS resource variable = endDecStrm core = FIFO_LUTRAM


//     scanRaw(image_data, (size+256)/16, rawStrm, endTextStrm);
// 	printf("finish scan\n");
// 	msgDecode(rawStrm, endTextStrm, 1, image);

// 	for(int i = 0; i <256; i++) {
// 		lut[i] = image[i];
// 		// printf("%d ", lut[i]);
// 	}
// 	for(int i = 0; i < size; i++) {
// 		output[i] = lut[image[i+256]];
// 		// printf("%d ", output[i]);
// 	}

// 	for(int i = 0; i < size/64; i++) {
// #pragma HLS pipeline II = 1
// 		for(int j = 0; j < 64; j++) {
// #pragma HLS unroll
// 			output_image[i].range(j*8+7, j*8) = output[i*64+j];
// 		}
// 	}

// 	msgEncode(decStrm, endDecStrm, 1, output, size);

//     ap_uint<128> tmp = 0;
//     ap_uint<64> index_i = 0;
//     ap_uint<64> index_j = 0;
//     // bool tmp_bool = 0;
//     while(!endDecStrm.read()) {
// #pragma HLS PIPELINE II = 1
//         tmp = decStrm.read();
//         // tmp_arr[index_i] = tmp;
//         // cout << tmp_arr[index_i] << endl;
//         output_image[index_i/4].range(((index_i)%4)*128+127, ((index_i%4)*128)) = tmp;
//         index_i++;
//         // tmp_bool = endEncStrm.read();
//     }


    in_lut_loop: for(int i = 0; i < 4; i++) {
#pragma HLS pipeline II = 1
        for(int j = 0; j < 64; j++) {
#pragma HLS unroll
            lut[i*64+j] = image_data[i].range(j*8+7, j*8);
        }
    }

    unsigned char image[MAX_IMAGE_SIZE];
    unsigned char output[MAX_IMAGE_SIZE];
    ap_uint<512> image_input_tmp[MAX_IMAGE_SIZE/64];

    for(int i = 0; i < size/64; i++) {
        image_input_tmp[i] = image_data[i+4];
        // cout << image_input_tmp[i];
    }
    read_data_from_axi(image_input_tmp, image, size);
// 	in_image_loop: for(int i = 0; i < size/64; i++) {
// // #pragma HLS unroll factor = 4
//         for(int j = 0; j < 64; j++) {
//             image[i*64+j] = image_data[i+4].range(j*8+7, j*8);
//             // cout << (int)lut[i*64+j] << " ";
//         }		
// 	}

    // Generate the gamma LUTDDD
    execute_loop: for (int i = 0; i < size; i++) {
        output[i] = lut[image[i]];
    }

// 	out_image_loop: for(int i = 0; i < (size+63)/64; i++) {
// #pragma HLS pipeline II = 1
//         for(int j = 0; j < 64 && i*64+j < size; j++) {
// #pragma HLS unroll
//             output_image[i].range(j*8+7, j*8) = image[i*64+j];
//         }
// 		// cout << output_image[i] << endl;			
// 	}
    write_data_to_axi(output, output_image, size);

    }
}
