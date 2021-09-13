# Graph files for the contest
The following repository consists the necessary input test files for the FPGA track contest. There are three sets of test cases. Each test case folder consists of three files with graph represented in CSR format. 
* data-csr-indicesweights.mtx (indices file) : Here, the column indices of the adjacency matrix of input graph are listed with the weights. Since the input graphs are unweighted(same cost to each connectivity), the weights are given as 1.000000 along with the respective column index. The first line of the indices file indicates the total number of non zeroes in the adjacency matrix(number of edges/connections in the graph). 
* data-csr-offset.mtx (row offset file) : Here, the file consists of the pointer value, pointing at the first non-zero(edge) of each vertex(node/row). Subtracting the consecutive pointer values results in the number of edges of each vertex(node/row). The first line of the offset file indicates the number of rows and columns of the adjacency matrix.
* data-golden.sssp.mtx (golden file) : This file is to check the correctness of the final output. The file consists of vertex_ID, distance and predecessor. For a given input vertex(source node) the vertex_id lists all the output vertices(destination nodes) and the corresponding distances to it from the source node. It also has predecessor as the third value in each line for a vertex which indicates the prior node of the destination node in the shortest path of source and destination. The contestant must compare all the values in the file and print TEST PASSED only if the output matches. The first line of the file consists the heading Vertex_ID,Distance,Pred. Note that the vertex_ID (destination node index) starts from 1 in golden file instead of 0. 
# Dataset information
* Small : <br />
 Number of nodes = 93  <br />
 Number of edges = 785 <br />
 Source node = 92 (node index starting from 0)<br />
* Medium :<br />
 Number of nodes = 4720 <br /> 
 Number of edges = 27444 <br />
 Source node = 7 (node index starting from 0) <br />
 Link : https://sparse.tamu.edu/AG-Monien/3elt
* Large :
 Number of nodes = 77360     
 Number of edges =  905468   
 Source node =  2481 (node index starting from 0)   
 Link : https://sparse.tamu.edu/SNAP/soc-Slashdot0811
