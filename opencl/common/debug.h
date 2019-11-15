#ifndef DEBUG__H
#define DEBUG__H

#include <stdio.h>
#include <CL/opencl.h>
#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "debug_defines.h"

/* Intiailize the timer infrastructure for debug and launch auto kernels if using emulator */
void init_debug(const cl_context         context
               ,const cl_program         program
               ,const cl_device_id       device
               ,      cl_kernel**        kernel
               ,      cl_command_queue** queue);

void read_ii_ms(const cl_context         context
               ,const cl_program         program
               ,const cl_kernel*         kernel
               ,const cl_command_queue*  queue
               ,const metric_t           metric
               ,const cl_int             buffer_id
               ,stamp_t**                iibuf);


void read_ii_ms_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,const metric_t           metric
                           ,stamp_t**                iibuf); 

void print_ii_ms(const metric_t metric
                ,stamp_t* buf); 

void read_st(const cl_context         context
               ,const cl_program         program
               ,const cl_kernel*         kernel
               ,const cl_command_queue*  queue
               ,const cl_int             buffer_id
               ,signal_t**               stbuf);

void read_st_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,signal_t**               stbuf); 

void print_st(signal_t* stbuf); 

void read_cf(const cl_context         context
               ,const cl_program         program
               ,const cl_kernel*         kernel
               ,const cl_command_queue*  queue
               ,const cl_int             buffer_id
               ,channel_t**              cfbuf);

void read_cf_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,channel_t**              cfbuf); 

void print_cf(channel_t* cfbuf); 

#endif //DEBUG__H
