#include "hotspot3D_common.h"
#include "../timer/timer.h"
#include "debug_defines.h"
#include "../common/debug/manager.cl"

__kernel void hotspotOpt1(__global float* restrict p,
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

  int i = get_global_id(0);
  int j = get_global_id(1);
  int c = i + j * nx;
  int xy = nx * ny;

  int W = (i == 0)        ? c : c - 1;
  int E = (i == nx-1)     ? c : c + 1;
  int N = (j == 0)        ? c : c - nx;
  int S = (j == ny-1)     ? c : c + nx;

  float temp1, temp2, temp3;
  temp1 = temp2 = tIn[c];
  temp3 = tIn[c+xy];
  tOut[c] = cc * temp2 + cn * tIn[N] + cs * tIn[S] + ce * tIn[E]
    + cw * tIn[W] + ct * temp3 + cb * temp1 + sdc * p[c] + ct * AMB_TEMP;
  c += xy;
  W += xy;
  E += xy;
  N += xy;
  S += xy;

  for (int k = 1; k < nz-1; ++k) {
      // ii
      monitor_ii_3(buf, i, j, k, nx, ny);

      temp1 = temp2;
      temp2 = temp3;
      temp3 = tIn[c+xy];
      
      temp = cc * temp2 + cn * tIn[N] + cs * tIn[S] + ce * tIn[E]
        + cw * tIn[W] + ct * temp3 + cb * temp1 + sdc * p[c] + ct * AMB_TEMP;

      // ms: unsure of value for ftime_t, dummy value?
      monitor_ms_3(msbuf1, i, j, k, nx, ny, (ftime_t)temp1);

      tOut[c] = temp;
      
      monitor_ms_3(msbuf2, i, j, k, nx, ny, (ftime_t)temp2);

      // st
      monitor_st_3(stbuf, i, j, k, nx, ny, temp);

      c += xy;
      W += xy;
      E += xy;
      N += xy;
      S += xy;
  }
  temp1 = temp2;
  temp2 = temp3;
  tOut[c] = cc * temp2 + cn * tIn[N] + cs * tIn[S] + ce * tIn[E]
    + cw * tIn[W] + ct * temp3 + cb * temp1 + sdc * p[c] + ct * AMB_TEMP;
  
  // finish monitor stuff, above has another save in tOut?
  finish_monitor_ii(buf, 0);
  finish_monitor_ms_2(msbuf1, msbuf2, 0);
  finish_monitor_st(stbuf, 0);

  return;
}


