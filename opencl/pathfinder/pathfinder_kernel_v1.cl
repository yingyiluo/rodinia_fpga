#include "timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"

#define MIN(a, b) ((a)<=(b) ? (a) : (b))

__kernel void dynproc_kernel (__global int* restrict wall,
                              __global int* restrict src,
                              __global int* restrict dst,
                                       int  cols,
                                       int  t)
{
	__local stamp_t buf[SIZE_II];
	for(int n = 0; n < cols; n++)
	{
		monitor_ii_1(buf, n);
		int min = src[n];
		if (n > 0)
		{
			min = MIN(min, src[n - 1]);
		}
		if (n < cols - 1)
		{
			min = MIN(min, src[n + 1]);
		}
		dst[n] = wall[(t + 1) * cols + n] + min;
	}
	finish_monitor_ii(buf, 0);
}
