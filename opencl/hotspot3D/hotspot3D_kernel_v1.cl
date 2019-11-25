#include "hotspot3D_common.h"
#include "timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"


__kernel void hotspotOpt1(__global float* restrict pIn,
                          __global float* restrict tIn,
                          __global float* restrict tOut,
                                   float           sdc,
                                   int             nx,
                                   int             ny,
                                   int             nz,
                                   float           ce,
                                   float           cw, 
                                   float           cn,
                                   float           cs,
                                   float           ct,
                                   float           cb, 
                                   float           cc)
{
	__local stamp_t buf[SIZE_II];
	__local stamp_t msbuf1[SIZE_MS];
	__local stamp_t msbuf2[SIZE_MS];
	__local signal_t stbuf[SIZE_ST];

	for(int z = 0; z < nz; z++)
	{
		for(int y = 0; y < ny; y++)
		{
			for(int x = 0; x < nx; x++)
			{
				// ii
				monitor_ii_3(buf, z, y, x, ny, nx);

				int c = x + y * nx + z * nx * ny;

				int w = (x == 0) ? c : c - 1;
				int e = (x == nx - 1) ? c : c + 1;
				int n = (y == 0) ? c : c - nx;
				int s = (y == ny - 1) ? c : c + nx;
				int b = (z == 0) ? c : c - nx * ny;
				int t = (z == nz - 1) ? c : c + nx * ny;

                                float temp = tIn[c]*cc + tIn[n]*cn + tIn[s]*cs + tIn[e]*ce + tIn[w]*cw + tIn[t]*ct + tIn[b]*cb + sdc * pIn[c] + ct*AMB_TEMP;
				
				// ms
				monitor_ms_3(msbuf1, z, y, x, ny, nx, (ftime_t)c);

				tOut[c] = temp;
				
				monitor_ms_3(msbuf2, z, y, x, ny, nx, (ftime_t)c);		
	
				// st
				monitor_st_3(stbuf, z, y, x, ny, nx, temp);

			}
		}
	}
	finish_monitor_ii(buf, 0);
	finish_monitor_ms_2(msbuf1, msbuf2, 0);
	finish_monitor_st(stbuf, 0);
}
