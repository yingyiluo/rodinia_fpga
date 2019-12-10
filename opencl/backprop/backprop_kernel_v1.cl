#include "timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"

#define ABS(x)          (((x) > 0.0) ? (x) : (-(x)))
#define ETA             0.3  //eta value
#define MOMENTUM        0.3  //momentum value

inline float squash(float x)
{
  return (1.0 / (1.0 + exp(-x)));
}

__kernel void bpnn_layerforward(__global float* restrict l1,
                                __global float* restrict l2,
                                __global float* restrict conn,
                                         int             n1,
                                         int             n2)
{
	float sum;
	int j, k;

	// Set up thresholding unit
	l1[0] = 1.0;

	// For each unit in second layer
	for (j = 1; j <= n2; j++)
	{
		// Compute weighted sum of its inputs
		sum = 0.0;
		for (k = 0; k <= n1; k++)
		{
			sum += conn[k * (n2 + 1) + j] * l1[k];
		}
		l2[j] = squash(sum);
	}
}

__kernel void bpnn_output_error(__global float* restrict delta,
                                __global float* restrict target,
                                __global float* restrict output,
                                         int             nj,
                                __global float* restrict err)
{
	int j;
	float o, t, errsum;
	errsum = 0.0;
	for (j = 1; j <= nj; j++)
	{
		o = output[j];
		t = target[j];
		delta[j] = o * (1.0 - o) * (t - o);
		errsum += ABS(delta[j]);
	}
	err[0] = errsum;
}

__kernel void bpnn_hidden_error(__global float* restrict delta_h,   
                                         int             nh, 
                                __global float* restrict delta_o, 
                                         int             no, 
                                __global float* restrict who, 
                                __global float* restrict hidden, 
                                __global float* restrict err)
{
	int j, k;
	float h, sum, errsum;

	errsum = 0.0;
	for (j = 1; j <= nh; j++)
	{
		h = hidden[j];
		sum = 0.0;
		for (k = 1; k <= no; k++)
		{
			sum += delta_o[k] * who[j * (no + 1) + k];
		}
		delta_h[j] = h * (1.0 - h) * sum;
		errsum += ABS(delta_h[j]);
	}
	err[0] = errsum;
}

__kernel void bpnn_adjust_weights(__global float* restrict delta,
                                           int             ndelta,
                                  __global float* restrict ly,
                                           int             nly,
                                  __global float* restrict w,
                                  __global float* restrict oldw)
{
	float new_dw;
	int k, j;
	ly[0] = 1.0;
	__local stamp_t buf[SIZE_II];

	for (j = 1; j <= ndelta; j++)
	{
		// ii
		monitor_ii_1(buf, (j - 1));
		for (k = 0; k <= nly; k++)
		{
			// ii (2)
			// monitor_ii_2(buf, (j - 1), k, (nly + 1));

			new_dw = ((ETA * delta[j] * ly[k]) + (MOMENTUM * oldw[k * (ndelta + 1) + j]));
			w[k * (ndelta + 1) + j] += new_dw;
			oldw[k * (ndelta + 1) + j] = new_dw;
		}
	}
	finish_monitor_ii(buf, 0);
}
