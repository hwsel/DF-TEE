from pycparser import CParser, parse_file, c_generator
from pycparser.c_ast import NodeVisitor, Constant
import re
import sys
import os

#step 1: split the input arguments
file1 = open('./benchmark/gamma_filter.c', 'r')

c_file = file1.readlines()
data_flow_type = ['unsigned char\* ', 'int\* ', 'float\* ', 'double\* ']
variable_type = ['unsigned char ', 'int ', 'float ', 'double ']
data_flow_name = {}
data_variable_name = {}
function_dict = {}

pragma_m_axi = "#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = 16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0_0 port = "
pragma_s_axilite = "#pragma HLS INTERFACE s_axilite bundle = control port = "
opencl_title = "extern \"C\" {\n"
opencl_title_end = "\n}"
library = "#include <hls_stream.h>\n#include <hls_math.h>\n#include <ap_int.h>\n#include <stdlib.h>\n#include <stdint.h>\n"

index = 0
flag = False

for line in c_file:
    index = index + 1
    match_obj = re.search(r'void', line)
    if match_obj:
        # print(line)
        flag = True
        function_name_match = r'(?<=void ).[a-zA-Z0-9_-]*'
        function_name = re.findall(function_name_match, line)
        function_dict[function_name[0]] = [line]
        # hls_file_name = r'./benchmark/' + function_name[0] + '.c'
        # file_output = open(hls_file_name, 'r')
        # file_output.write(line)
        for element in data_flow_type:
            match = r'(?<=' + element + ').[a-zA-Z0-9_-]*'
            flow_name = re.findall(match, line)
            # print(flow_name)
            if bool(flow_name) == True:
                data_flow_name[function_name[0]] = flow_name
        for element in variable_type:
            match = '(?<=' + element + ').[a-zA-Z0-9_-]*'
            variable_name = re.findall(match, line)
            if bool(variable_name) == True:
                data_variable_name[function_name[0]] = variable_name
        print(data_flow_name)        
        print(data_variable_name)
        
        if function_name[0] in data_flow_name.keys():
            for element in data_flow_name[function_name[0]]:
                pragma = pragma_m_axi + element + '\n'
                function_dict[function_name[0]].append(pragma)
                pragma = pragma_s_axilite + element + '\n'
                function_dict[function_name[0]].append(pragma)
        if function_name[0] in data_variable_name.keys():
            for element in data_variable_name[function_name[0]]:
                pragma = pragma_s_axilite + element + '\n'
                function_dict[function_name[0]].append(pragma)
        pragma = pragma_s_axilite + 'return' + '\n'
        function_dict[function_name[0]].append(pragma)
    match_obj = re.search(r'int main\(', line)
    if match_obj:
        break
    if bool(function_dict) == True and flag == False:
        last_key = list(function_dict)[-1]
        function_dict[last_key].append(line)
    flag = False
    
# generate the code for normal kernel
#pcie connected FPGA
encrypted_function = ['gamma_apply', 'main']
encryption_library = "#include \"aes128ctr.h\"\n#define MAX_IMAGE_SIZE 512*512\n"

#decryption_pre + (lut_size/64) + decryption_pre_2
decryption_pre = "ap_uint<512> image_input_tmp[MAX_IMAGE_SIZE/64];\n\
    for(int i = 0; i < size/64 + 1; i++) {\n\
        image_input_tmp[i] = image_input[i+"   #4];}\n"
decryption_pre_2 = "];\n    }\n"

#other_pre_1 + (lut_size/64) + other_pre_2 + (lut) + other_pre_3
other_pre_1 = "    for(int i = 0; i < "
other_pre_2 = "; i++) { \n\
        for(int j = 0; j < 64; j++) {\n"                       
other_pre_3 = "[i*64+j] = image_input[i].range(j*8+7, j*8);\n\
    }\n}\n"

decryption_function = "\
    unsigned char image[MAX_IMAGE_SIZE] = {0};\n\
    unsigned char output[MAX_IMAGE_SIZE];\n\
    ap_uint<128> ivstr;\n\
    ap_uint<128> keystr;\n\
    ap_uint<64> msg_num = image_input_tmp[0].range(63, 0);\n\
    ap_uint<64> row_num = image_input_tmp[0].range(127, 64);\n\
    ap_uint<64> msg_size = image_input_tmp[0].range(191, 128);\n\
    ivstr = image_input_tmp[0].range(383, 256);\n\
    keystr = image_input_tmp[0].range(511, 384);\n\
    dec_wrapper(image_input_tmp, image, msg_num, row_num, ivstr, keystr, msg_size);\n"                  
encryption_function = "    enc_wrapper(output, image_output, msg_num, row_num, ivstr, keystr, output_size); \n"

image_match = '(?<=image\: ).[a-zA-Z0-9_-]*'
lut_match = '(?<=lut\: ).[a-zA-Z0-9_-]*'
output_match = '(?<=output\: ).[a-zA-Z0-9_-]*'

encryption_krnl_declaration = 'void encryption_krnl(ap_uint<512>* image_input, ap_uint<512>* image_output, int size) {\n'

encryption_krnl_pragma = "#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
num_write_outstanding = 16 num_read_outstanding = 16 \
max_write_burst_length = 64 max_read_burst_length = 64 \
bundle = gmem0_0 port = image_input\n\n\
#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
num_write_outstanding = 16 num_read_outstanding = 16 \
max_write_burst_length = 64 max_read_burst_length = 64 \
bundle = gmem0_1 port = image_output\n\n\
#pragma HLS INTERFACE s_axilite port = image_input bundle = control\n\
#pragma HLS INTERFACE s_axilite port = image_output bundle = control\n\
#pragma HLS INTERFACE s_axilite port = size bundle = control\n\
#pragma HLS INTERFACE s_axilite port A= return bundle = control\n\n"

variable_size = {}
flag_lut = False
flag_image = False
flag_declaration = False

# print(function_dict)
for function_name in function_dict:
    print(function_name)
    if encrypted_function.count(function_name) == 0:
        #unencrypted function
        file_output_name = r'./benchmark/kernel_1/plain_krnl.cpp'
        print(file_output_name)
        file_output = open(file_output_name, 'w')
        file_output.write(library)
        file_output.write(opencl_title)
        for line in function_dict[function_name]:
            # print(line)
            file_output.write(line)
        file_output.write(opencl_title_end)
        file_output.close()
    else:
        file_output_name = r'./benchmark/kernel_2/encryption_krnl.cpp'
        file_output = open(file_output_name, 'w')
        file_output.write(library)
        file_output.write(encryption_library)
        file_output.write(opencl_title)
        for line in function_dict[function_name]:
            if bool(re.findall('(?<=void ).[a-zA-Z0-9_-]*', line)) == True:
                #rewrite the function declaration
                file_output.write(encryption_krnl_declaration)
                file_output.write(encryption_krnl_pragma)
            if bool(re.findall(lut_match, line)) == True:
                lut = re.findall(lut_match, line)
                # print(lut[0])
                variable_size['lut'] = lut[0]
                flag_lut = True
            if bool(re.findall(image_match, line)) == True:
                image_size = re.findall(image_match, line)
                # print(image_size[0])
                variable_size['image'] = image_size[0]
            if bool(re.findall(output_match, line)) == True:
                output_size = re.findall(output_match, line)
                # print(output_size[0])
                variable_size['output'] = output_size[0]
                flag_image = True
            if flag_lut == True and flag_image == True:
                flag_lut = False
                flag_image = False
                flag_declaration = True
                declaration_line = '\n  unsigned char ' + 'input' + '[' + variable_size['lut'] + '+' + variable_size['image'] + '] = {0};\n'
                # print(declaration_line)
                file_output.write(declaration_line)
                first_line = other_pre_1 + str(int(variable_size['lut'])/64 + int(variable_size['image'])/64) + other_pre_2
                print(first_line)
                file_output.write(first_line)
                second_line = '        input' +  other_pre_3
                file_output.write(second_line)
                input_size = int(variable_size['image'])/64
                encryption_prep = decryption_pre + str(input_size) + decryption_pre_2
                # print(encryption_prep)
                file_output.write(encryption_prep)
                file_output.write(decryption_function)
            if bool(re.findall('end of function', line)) == True:
                output_size_line = '\n    ap_uint<64> output_size = ' + str(int(variable_size['output'])/64) + ';\n'
                file_output.write(output_size_line)
                file_output.write(encryption_function)
            if flag_declaration == True:
                file_output.write(line)
        file_output.write(opencl_title_end)
        file_output.close()

