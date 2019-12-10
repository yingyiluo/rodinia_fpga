#pragma OPENCL EXTENSION cl_khr_byte_addressable_store: enable

#include "timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"

//Structure to hold a node information
typedef struct{
	int starting;
	int no_of_edges;
} Node;

//--7 parameters
__kernel void BFS_1(__global const Node* restrict g_graph_nodes,
		    __global const int*  restrict g_graph_edges, 
		    __global char*       restrict g_graph_mask, 
		    __global char*       restrict g_updating_graph_mask, 
		    __global char*       restrict g_graph_visited, 
		    __global int*        restrict g_cost, 
		             const int            no_of_nodes)
{
	#pragma unroll 7
	__local stamp_t buf[SIZE_II];
	for (int tid=0; tid<no_of_nodes; tid++)
	{
		monitor_ii_1(buf, tid);
		if(g_graph_mask[tid])
		{
			g_graph_mask[tid]=false;
			for(int i=g_graph_nodes[tid].starting; i<(g_graph_nodes[tid].no_of_edges + g_graph_nodes[tid].starting); i++)
			{
				// ii
				// monitor_ii_2(buf, tid, (i - temp), temp2);

				int id = g_graph_edges[i];
				if(!g_graph_visited[id])
				{
					g_cost[id]=g_cost[tid]+1;
					g_updating_graph_mask[id]=true;
				}
			}
		}
	}
	finish_monitor_ii(buf, 0);
}

//--5 parameters
__kernel void BFS_2(__global char*     restrict g_graph_mask, 
		    __global char*     restrict g_updating_graph_mask, 
		    __global char*     restrict g_graph_visited, 
		    __global char*     restrict g_over,
		             const int          no_of_nodes)
{
	#pragma unroll 64
	__local stamp_t buf[SIZE_II];
	for (int tid=0; tid<no_of_nodes; tid++)
	{
		// ii
		monitor_ii_1(buf, tid);

		if(g_updating_graph_mask[tid])
		{
			g_graph_mask[tid]=true;
			g_graph_visited[tid]=true;
			*g_over=true;
			g_updating_graph_mask[tid]=false;
		}
	}
	finish_monitor_ii(buf, 1);
}
