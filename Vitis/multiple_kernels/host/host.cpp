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

#include <iostream>
#include <cstring>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define DATA_SIZE 4096

int main(int argc, char** argv) {

    // Read settings
    std::string binaryFile = argv[1]; 
    int device_index = 0;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

    // Vector Addition Kernel
    std::cout << "\nStarting the vadd kernel...\n";
    auto krnl_vadd = xrt::kernel(device, uuid, "krnl_vadd");

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bo0 = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(0));
    auto bo1 = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(1));
    auto bo_out = xrt::bo(device, vector_size_bytes, krnl_vadd.group_id(2));

    // Map the contents of the buffer object into host memory
    auto bo0_map = bo0.map<int*>();
    auto bo1_map = bo1.map<int*>();
    auto bo_out_map = bo_out.map<int*>();

    // Create the test data
    int buf_ref[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo0_map[i] = i;
        bo1_map[i] = i;
        buf_ref[i] = bo0_map[i] + bo1_map[i];
    }

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    bo0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo1.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Execution of the vadd kernel\n";
    auto run = krnl_vadd(bo0, bo1, bo_out, DATA_SIZE);
    run.wait();

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Validate our results
    if (std::memcmp(bo_out_map, buf_ref, DATA_SIZE))
        throw std::runtime_error("Value read back does not match reference");

    // Vector Multiplication Kernel
    std::cout << "\nStarting the vmult kernel...\n";
    auto krnl_vmult = xrt::kernel(device, uuid, "krnl_vmult");

    std::cout << "Allocate Buffer in Global Memory\n";
    auto bo2 = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(0));
    auto bo3 = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(1));
    auto bo_output = xrt::bo(device, vector_size_bytes, krnl_vmult.group_id(2));

    // Map the contents of the buffer object into host memory
    auto bo2_map = bo2.map<int*>();
    auto bo3_map = bo3.map<int*>();
    auto bo_output_map = bo_output.map<int*>();

    // Create the test data
    int buf_ref1[DATA_SIZE];
    for (int i = 0; i < DATA_SIZE; ++i) {
        bo2_map[i] = i;
        bo3_map[i] = i;
        buf_ref1[i] = bo2_map[i] * bo3_map[i];
    }

    // Synchronize buffer content with device side
    std::cout << "synchronize input buffer data to device global memory\n";

    bo2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo3.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Execution of the vmult kernel\n";
    auto run1 = krnl_vmult(bo2, bo3, bo_output, DATA_SIZE);
    run1.wait();

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    bo_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Validate our results
    if (std::memcmp(bo_output_map, buf_ref1, DATA_SIZE))
        throw std::runtime_error("Value read back does not match reference");
    
    std::cout << "TEST PASSED\n";
    return 0;
}
