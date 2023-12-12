/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <ap_int.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>
#include <sys/time.h>
#include <new>
#include <algorithm>
#include <cstdlib>
#include <openssl/evp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <xcl2.hpp>
#include "xf_utils_sw/logger.hpp"
#include "xf_security/msgpack.hpp"

using namespace std;

#define KEY_SIZE 16
#define IV_SIZE 16
#define IMAGE_WIDTH 256
#define IMAGE_HEIGHT 256
#define LUT_SIZE 256

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

unsigned char* read_file(std::string file_name) {
    int i = 0;
    unsigned char *output = (unsigned char*)malloc(sizeof(unsigned char)*IMAGE_WIDTH*IMAGE_HEIGHT);
    std::fstream f(file_name, std::ios_base::in);
    int tmp = 0;
    while(f >> tmp) {
        // print("%d ", tmp);
        output[i] = tmp;
        i++;
    }
    return output;
}

// unsigned char key[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
//                         0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

// // initialization vector for test, other IVs are fine.
// unsigned char ivec[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
//                         0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};

int main(int argc, char* argv[]) {
    // cmd parser
    ArgParser parser(argc, (const char**)argv);
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    uint64_t in_pack_size = IMAGE_HEIGHT*IMAGE_WIDTH + LUT_SIZE;
    uint64_t out_pack_size = IMAGE_HEIGHT*IMAGE_WIDTH;
    uint64_t pure_msg_size = IMAGE_HEIGHT*IMAGE_WIDTH;
    uint64_t lut_size = LUT_SIZE;
    unsigned char* inputData = aligned_alloc<unsigned char>(in_pack_size);
    unsigned char* outputData = aligned_alloc<unsigned char>(out_pack_size);

    unsigned char* lut = read_file("../lut.txt");
    // uint64_t input_row_num = ((long int)pure_msg_size+15) / 16 + 4;
    // uint64_t len_in_bit = pure_msg_size*8;
    // for(int i =256; i < 256+64; i++) {
    //     inputData[i] = 0;
    // }
    // inputData[0+256] = 1; //msg_num
    // // source_in1[8] = ((long int)image_size+15) / 16 + 4; //row_num   
    // memcpy(&inputData[8+256], &input_row_num, sizeof(uint64_t)); 
    // memcpy(&inputData[16+256], &len_in_bit, sizeof(uint64_t)); 
    // memcpy(&inputData[32+256], ivec, sizeof(unsigned char)*IV_SIZE);
    // memcpy(&inputData[48+256], key, sizeof(unsigned char)*KEY_SIZE);
    memcpy(&inputData[0], lut, lut_size);
    for(int i = lut_size; i < in_pack_size; i++) {
        inputData[i] = i % 256;
    }
    // for(int i = 0; i < in_pack_size; i++) {
    //     printf("%d ", inputData[i] );
    // }
    printf("\n\n");
    // CL setup
    xf::common::utils_sw::Logger logger;
    cl_int err = CL_SUCCESS;

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    cl::Context context(device, NULL, NULL, NULL, &err);
    logger.logCreateContext(err);

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    logger.logCreateCommandQueue(err);

    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);

    cl::Program program(context, devices, xclBins, NULL, &err);
    logger.logCreateProgram(err);

    cl::Kernel kernel(program, "gamma_apply", &err);
    logger.logCreateKernel(err);

    cl_mem_ext_ptr_t inMemExt = {0, inputData, kernel()};
    cl_mem_ext_ptr_t outMemExt = {1, outputData, kernel()};

    timespec tv_1;
    clock_gettime(CLOCK_MONOTONIC,&tv_1);

    cl::Buffer in_buff = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    (size_t)(in_pack_size), &inMemExt);
    cl::Buffer out_buff = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     (size_t)(out_pack_size), &outMemExt);

    // CL buffers
    kernel.setArg(0, in_buff);
    kernel.setArg(1, out_buff);
    kernel.setArg(2, pure_msg_size);   

    std::vector<cl::Memory> initBuffs;

    initBuffs.resize(0);
    initBuffs.push_back(in_buff);
    initBuffs.push_back(out_buff);

    q.enqueueMigrateMemObjects(initBuffs, 0, nullptr, nullptr);
    q.finish();


    // H2D, Kernel Execute, D2H
    std::vector<cl::Memory> inBuffs, outBuffs;

    inBuffs.resize(0);
    inBuffs.push_back(in_buff);
    outBuffs.resize(0);
    outBuffs.push_back(out_buff);

    std::vector<cl::Event> h2d_evts, d2h_evts, krn_evts;
    h2d_evts.resize(1);
    d2h_evts.resize(1);
    krn_evts.resize(1);

    q.enqueueMigrateMemObjects(inBuffs, 0, nullptr, &h2d_evts[0]);
    q.enqueueTask(kernel, &h2d_evts, &krn_evts[0]);
    q.enqueueMigrateMemObjects(outBuffs, CL_MIGRATE_MEM_OBJECT_HOST, &krn_evts, &d2h_evts[0]);

    q.finish();
    timespec tv_2;
    clock_gettime(CLOCK_MONOTONIC,&tv_2);
    printf("The transmission time is: %ld\n", (tv_2.tv_sec*1000*1000 + tv_2.tv_nsec/1000 - (tv_1.tv_sec*1000*1000 + tv_1.tv_nsec/1000)));
    // for(int i =0; i<out_pack_size; i++) {
    //     if(i%64 == 0) {
    //         printf("\n%d: ", i/64);
    //     }
    //     std::cout<<(int)outputData[i]<<" ";
    // }

    // Performance profiling
    // unsigned long time1, time2;

    // h2d_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    // h2d_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    // std::cout << "Transfer package of " << in_pack_size / 1024.0 / 1024.0 << " MB to device took "
    //           << (time2 - time1) / 1000.0
    //           << "us, bandwidth = " << in_pack_size / 1024.0 / 1024.0 / ((time2 - time1) / 1000000000.0) << "MB/s"
    //           << std::endl;

    // krn_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    // krn_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    // std::cout << "Packages contains additional info, pure message size = " << pure_msg_size / 1024.0 / 1024.0 << "MB\n";
    // std::cout << "Kernel process message of " << pure_msg_size / 1024.0 / 1024.0 << " MB took "
    //           << (time2 - time1) / 1000.0
    //           << "us, performance = " << pure_msg_size / 1024.0 / 1024.0 / ((time2 - time1) / 1000000000.0) << "MB/s"
    //           << std::endl;

    // d2h_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    // d2h_evts[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    // std::cout << "Transfer package of " << out_pack_size / 1024.0 / 1024.0 << " MB to host took "
    //           << (time2 - time1) / 1000.0
    //           << "us, bandwidth = " << out_pack_size / 1024.0 / 1024.0 / ((time2 - time1) / 1000000000.0) << "MB/s"
    //           << std::endl;

    // release resource
    free(inputData);
    free(outputData);

}