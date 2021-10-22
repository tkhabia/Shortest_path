#include <stdio.h>
#include <string.h>
#include "hls_stream.h"
#include <ap_int.h> 
typedef ap_uint<512> uint512;


#define max_nodes 905469/16
#define q_size 64

void Uint512ToStream(int &st ,int &en  , uint512 &data ,hsl::stream< ap_uint<32> , q_size> &qu ){
    #pragma hls pipeline 
    for(int i = st; i < en ; i++){
        #pragma hls tripcount min=1 max= 16
        #pragma hls unroll factor=8 
        qu.write(temp1(i*32+31 ,i*32));
    }
}

void load_Queue(uint512 *queue , unsigned int &st , unsigned int &end , hsl::stream< ap_uint<32> , q_size> &qu ){
    int s1 = st/16  , en1 = 16, en= end/16;
    if ( s1 * 16 + 16 > end) 
        en1 = end%16; 
#pragma HLS DATAFLOW
    uint512 temp1 = queue[s1%max_nodes]  ; 
    Uint512ToStream(st%16 , en1 , temp1 ,qu) ; 
    Ib_load:for (int i = s1+1 ; i<en ; i++ ){
        
        temp1 = queue[i%max_nodes];
        Uint512ToStream(0 , 16 ,temp1, qu ) ; 
    }
    temp1 = queue[en%max_nodes] ; 
    Uint512ToStream(0 , end%16  , temp1 , qu); 
}

void writequeue(unsigned int &parent , ap_uint<32> *offset, uint512 *column, uint512 *queue, ap_uint<32> *pred, ap_uint<32> *dist , unsigned int &end ){
    unsigned int p_dist ,unstart, unend ; 
    p_dist = dist[parent] ; 
    unstart = offset [parent];
    unend = offset[parent+1];
    uint512 coltemp = column[unstart/16] ; 
    uint512 qutemp= queue[(end%max_nodes)/ 16] ; 
    for (int i = unstart  ; i< unend ; i++){
        #pragma hls pipeline 
        if (i%16 ==  0){
            coltemp = column[i/16]; 
        }
        uint loc = coltemp((i%16)*32 + 31 , (i%16)*32) ;
        if (dist[loc] != -1){
            dist[loc] = p_dist +1 ;
            qutemp(31 + (end%max_nodes)%16 ,(end%max_nodes)%16)  = loc;
            end++ ; 
            pred[loc] = parent ;
        }
        if (end%16 == 0 ){
            queue[end/16 -1] = qutemp;
        }
    }
}

extern "C"
{
    void shortestpath(ap_uint<32> *offset, uint512 *column, uint512 *queue, ap_uint<32> *pred, ap_uint<32> *dist, int source)
    {
#pragma HLS INTERFACE m_axi port = offset max_read_burst_length = 32 bundle = gmem0
#pragma HLS INTERFACE m_axi port = column max_read_burst_length = 32 bundle = gmem1
#pragma HLS INTERFACE m_axi port = queue max_write_burst_length = 32 bundle = gmem2
#pragma HLS INTERFACE m_axi port = pred offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = dist offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = offset bundle = control
#pragma HLS INTERFACE s_axilite port = column bundle = control
#pragma HLS INTERFACE s_axilite port = source bundle = control
#pragma HLS INTERFACE s_axilite port = queue bundle = control
#pragma HLS INTERFACE s_axilite port = pred bundle = control
#pragma HLS INTERFACE s_axilite port = dist bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
        // bool visited[max_nodes] = {0};
        unsigned int st = 0, end = 0 , parent = 0, p_dist = 0 ,unstart, unend ;
        hsl::stream< ap_uint<32> , q_size> qu;
        writequeue(source , offset ,column , queue , pred, dist, end )  ; 
    TopFor:
        for (; st < end;)
        {
#pragma HLS DATAFLOW
            uint lend= end - st > 64? st + 64 : end;  
            load_Queue(queue ,st  , lend, qu );

        WritingData:
            for (int i = st;i < lend; i++){
                #pragma HLS PIPELINE 
                // reading data 
                parent = qu.read(); 
                writequeue(parent, offset ,column , queue , pred, dist, end)  ; 
            }  
        }
    }
}