# DF-TEE: Trusted Execution Environment for Disaggregated Multi-FPGA Cloud Systems
THis is apreliminary version of the source code for DF-TEE based on our AsianHost paper: "DF-TEE: Trusted Execution Environment for Disaggregated Multi-FPGA Cloud Systems". 

# System Overview
The system is divided into five components:
- **Client** supports SGX and wishes to offload computation to FPGA.
- **Trusted edge server** slices the client source code into untrusted and trusted functions, and compiles the code into FPGA bitstreams.
- **FPGA manager** communicates with the FPGAs to execute the computation.
- **FPGAs** are the hardwares to run the compiled workload.

# Execute Instructions
## Client
- Install SGX and setup the SGX environment
- Modify `settings` with your own information
- Execute:
```
./bootstrap
./configure
make
./run-client
```
## Secure monitor
- Execute:
```
./bootstrap
./configure
make
./run-server
```

## Trusted edge server
- Install ![TaintGrind(v.3.12.0)](https://github.com/wmkhoo/taintgrind) 
- Download the code for TZSlicer and add it to the folder 'code/trusted_edge_server'
- Execute:
```
gcc -I ./valgrind/valgrind-3.20.0/taintgrind/ -I ./valgrind/valgrind-3.20.0/include/ -g $c_source_file -o $compiled_c_object -lm
./valgrind/valgrind-3.20.0/build/bin/valgrind --tool=taintgrind $compiled_c_object 2>$log_file
python Main.py gamma_filter 0 0 0 0
python slicer.py
```
To compile the sliced code into bitstreams, execute the 'run.sh' script in the folder 'code/trusted_edge_server/benchmark'.

## FPGAs and the FPGA manager
To run the compiled workload, execute the files separately:
```
FPGA_manager/FPGA_manager.ipynb
FPGA/FPGA_1.ipynb
FPGA/FPGA_2.ipynb
```