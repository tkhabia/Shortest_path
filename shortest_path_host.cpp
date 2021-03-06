#include <iostream>
#include <memory>
#include <string>
#include <vector>
//#include "xilinx_ocl.hpp"
#include <cstring>
#include <fstream>
#include <sys/time.h>
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"
#include <limits>
#include <sstream>

#include <algorithm>
// Xilinx OpenCL and XRT includes



int main(int argc, char *argv[])
{
    if (argc < 6) {
    	std::cout << "Insufficient argument provided " << std::endl;
    	return -1;
    }
	std::string xclbin_path(argv[1]);
    std::string offsetfile(argv[2]);
    std::string columnfile(argv[3]);
    std::string goldenfile(argv[4]);
    std::string source_st(argv[5]);

    std::cout << xclbin_path << "\n" << offsetfile << "\n" << columnfile<< "\n" <<goldenfile << "\n" <<source_st<< "\n";
    int numVertices , source = std::stoi(source_st);
    int numEdges;
    char line[1024] = {0};
    int index = 0, err = 0;

    std::fstream offsetfstream(offsetfile.c_str(), std::ios::in);
    if (!offsetfstream)
    {
        std::cout << "Error : " << offsetfile << " file doesn't exist !" << std::endl;
        exit(1);
    }
    offsetfstream.getline(line, sizeof(line));
    std::stringstream numOdata(line);
    numOdata >> numVertices;
    numOdata >> numVertices;
    std::vector<uint ,aligned_allocator<uint>> offset32(numVertices + 1);
    while (offsetfstream.getline(line, sizeof(line)))
    {
        std::stringstream data(line);
        data >> offset32[index];
        index++;
    }
    std::fstream columnfstream(columnfile.c_str(), std::ios::in);
    if (!columnfstream)
    {
        std::cout << "Error : " << columnfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    index = 0;
    columnfstream.getline(line, sizeof(line));
    std::stringstream numCdata(line);
    numCdata >> numEdges;

    std::vector<uint ,aligned_allocator<uint>> column32(numEdges);
    std::vector<float  , aligned_allocator<float>> weight32(numEdges);
    while (columnfstream.getline(line, sizeof(line)))
    {
        std::stringstream data(line);
        data >> column32[index];
        data >> weight32[index];
        index++;
    }
    uint max = 0;
    uint id = 0;
    for (int i = 0; i < numVertices; i++)
    {
        if (offset32[i + 1] - offset32[i] > max)
        {
            max = offset32[i + 1] - offset32[i];
            id = i;
        }
    }
    std::cout << "id: " << id << " max out: " << max << std::endl;
    std::vector<uint ,aligned_allocator<uint> >pred(((numVertices + 1023) / 1024) * 1024);
    std::vector<uint ,aligned_allocator<uint> >dist(((numVertices + 1023) / 1024) * 1024);
    for (int i = 0; i < numVertices; i++)
    {
        dist[i] = -1;
    }

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());
    auto fileBuf = xcl::read_binary_file(xclbin_path);
    cl::Program::Binaries xclBins {{fileBuf.data(), fileBuf.size()}};

    devices.resize(1);
    OCL_CHECK(err , cl::Program program(context, {device}, xclBins, NULL, &err));
    cl::Kernel shortestPath = cl::Kernel(program, "shortestpath", &err);
    std::cout << "kernel has been created" << std::endl;
    
    OCL_CHECK(err,cl::Buffer offset32_buff =cl::Buffer(context,
                                            CL_MEM_EXT_PTR_XILINX|CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            (numVertices + 1)*sizeof(uint), offset32.data(), &err));

    OCL_CHECK(err,cl::Buffer column32_buff = cl::Buffer(context,
                                            CL_MEM_EXT_PTR_XILINX|CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            numEdges * sizeof(uint), column32.data(), &err));

    OCL_CHECK(err,cl::Buffer queue_buff =cl::Buffer (context,
                                            CL_MEM_EXT_PTR_XILINX|CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            905469 * sizeof(uint), NULL, &err));

    OCL_CHECK(err,cl::Buffer pred_buff =cl::Buffer (context,
                                            CL_MEM_EXT_PTR_XILINX|CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            ((numVertices + 1023) / 1024) * 1024 * sizeof(uint), pred.data(), &err));

    OCL_CHECK(err, cl::Buffer dist_buff = cl::Buffer (context,
                                            CL_MEM_EXT_PTR_XILINX|CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            ((numVertices + 1023) / 1024) * 1024 * sizeof(uint), dist.data() , &err));
    q.enqueueMigrateMemObjects({offset32_buff, column32_buff,queue_buff,pred_buff ,dist_buff }, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED,
    nullptr, nullptr);
    q.finish();

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);
    ob_in.push_back(offset32_buff); 
    ob_in.push_back(column32_buff); 
    ob_in.push_back(dist_buff); 
    
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
    ob_out.push_back(dist_buff); 
    ob_out.push_back(pred_buff); 
    int j =0  ; 
    shortestPath.setArg(j++ , offset32_buff) ; 
    shortestPath.setArg(j++ , column32_buff) ; 
    shortestPath.setArg(j++ , queue_buff) ; 
    shortestPath.setArg(j++ , pred_buff) ; 
    shortestPath.setArg(j++ , dist_buff) ;
    shortestPath.setArg(j++ , source) ;
    q.enqueueTask(shortestPath, &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();
    unsigned long time1, time2, total_time;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Write DDR Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    total_time = time2 - time1;
    
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Kernel Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    total_time += time2 - time1;
    
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Read DDR Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    total_time += time2 - time1;
    std::cout << "Total Execution time " << total_time / 1000000.0 << "ms" << std::endl;
    std::fstream goldenfstream(goldenfile.c_str(), std::ios::in);
    if (!goldenfstream) {
        std::cout << "Err : " << goldenfile << " file doesn't exist !" << std::endl;
        exit(1);
    }
    goldenfstream.getline(line, sizeof(line));

    index = 0;
    while(goldenfstream.getline(line , sizeof(line))){
        std::string str(line);
        std::replace(str.begin(), str.end(), ',', ' ');
        std::stringstream data(str.c_str());
        uint vertex;
        uint distance;
        uint pred_golden;
        data >> vertex;
        data >> distance;
        data >> pred_golden;
        if (dist[vertex - 1] != distance ) {
            std::cout << "Distance incorrect\n";
            std::cout << "TEST FAILED\n" ; 
            return -1 ;
        }
        if (pred_golden - 1 != pred[vertex - 1]) {
            std::cout << "predicessor incorrect\n";
            std::cout << "TEST FAILED\n" ; 
            return -1 ;
        }

    }
    std::cout << "TEST PASSED\n" ; 
    return 0 ; 
}
