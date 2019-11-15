#ifndef DEBUG_CPP
#define DEBUG_CPP

#include "debug.h"
#include "CL/opencl.h"

void init_debug(const cl_context         context
               ,const cl_program         program
               ,const cl_device_id       device
               ,      cl_kernel**        kernel
               ,      cl_command_queue** queue) {

    int    no_of_kernels;
    cl_int status;

#ifdef EMULATOR 
    char debug_kernel_names[3 + NUM_CF][30];

    sprintf(debug_kernel_names[0],"collect_ii_ms");
    sprintf(debug_kernel_names[1],"collect_st");
    sprintf(debug_kernel_names[2],"collect_cf");
    for(int i = 0; i < NUM_CF; i++) 
        sprintf(debug_kernel_names[i+3], "watch_chan_%0d", i); 

    no_of_kernels = 3 + NUM_CF;
#else
    const char* debug_kernel_names[] = {"collect_ii_ms", "collect_st", "collect_cf"};
    no_of_kernels = 3;
#endif

    *kernel =  (cl_kernel *)         malloc(sizeof(cl_kernel)        * no_of_kernels);
    *queue  =  (cl_command_queue *)  malloc(sizeof(cl_command_queue) * no_of_kernels);

#if (NUM_II + NUM_MS) > 0
    (*queue)[0] = clCreateCommandQueue(context,device,CL_QUEUE_PROFILING_ENABLE,&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create queue in debug %0d\n", 0);
    }
    (*kernel)[0] = clCreateKernel(program,debug_kernel_names[0],&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create kernel in debug\n");
    }
    printf("kernel created\n");
#endif 

#if NUM_ST > 0
    (*queue)[1] = clCreateCommandQueue(context,device,CL_QUEUE_PROFILING_ENABLE,&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create queue in debug %1d\n", 1);
    }
    (*kernel)[1] = clCreateKernel(program,debug_kernel_names[1],&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create kernel in debug %1d\n", debug_kernel_names[1]);
    }
#endif

#if NUM_CF > 0
    (*queue)[2] = clCreateCommandQueue(context,device,CL_QUEUE_PROFILING_ENABLE,&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create queue in debug %1d\n", 2);
    }
    (*kernel)[2] = clCreateKernel(program,debug_kernel_names[2],&status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create kernel in debug %1d\n", debug_kernel_names[2]);
    }
#endif

#ifdef EMULATOR 
    for(int i = 3; i < 3 + NUM_CF; i++) { 
        (*queue)[i] = clCreateCommandQueue(context,device,CL_QUEUE_PROFILING_ENABLE,&status);
        if(status != CL_SUCCESS) 
            printf("-ERROR- Could not create queue in debug %0d\n", i);
        (*kernel)[i] = clCreateKernel(program,debug_kernel_names[i],&status);
        if(status != CL_SUCCESS) 
            printf("-ERROR- Could not create kernel in debug %s\n", debug_kernel_names[i]);
        status = clEnqueueTask((*queue)[i],(*kernel)[i],0,NULL,NULL);
        if(status != CL_SUCCESS)
            printf("-ERROR- Could not enqueue kernels in debug %s\n", debug_kernel_names[i]);
        else 
            printf("-INFO- : Launched Kernel %s\n", debug_kernel_names[i]);
    }
#endif 
}

void read_ii_ms(const cl_context         context
             ,const cl_program         program
             ,const cl_kernel*         kernel
             ,const cl_command_queue*  queue
             ,const metric_t           metric
             ,const cl_int             buffer_id
             ,stamp_t**                buf) {

    cl_mem read_buffer;
    cl_int status;
    
    const cl_int mem_size = metric == II ? SIZE_II : SIZE_MS;

    posix_memalign ((void **) buf, 64, sizeof(stamp_t) * mem_size);
    
    char*   kernel_name;
    size_t  kernel_name_size;
    cl_uint kernel_arg_size;
    char    message[200];
    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_FUNCTION_NAME ,0 , NULL, &kernel_name_size);

    sprintf(message,"Could not get info on kernel in %s", "collect_data");
    if(status != CL_SUCCESS) printf("%s %d",message,status);   
    kernel_name = (char *) malloc(sizeof(char) * kernel_name_size);

    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_FUNCTION_NAME 
                             ,sizeof(char)*kernel_name_size
                             ,kernel_name
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_NUM_ARGS 
                             ,sizeof(cl_uint)
                             ,&kernel_arg_size
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    printf("Kernel %s has %0d args\n", kernel_name, kernel_arg_size);

    read_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(stamp_t) * mem_size, NULL, &status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create read buffer in rest_debug\n");
    }
    
    cl_int m = metric;
    clSetKernelArg(kernel[0], 0, sizeof(cl_int), &m);
    clSetKernelArg(kernel[0], 1, sizeof(cl_int), &buffer_id);
    status = clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &read_buffer);

    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not set argument[1] for  Kernel %s %d\n","read_stamp", status);

    //cl_int cmd_in = D_READ;
    //status = clSetKernelArg(kernel[0],0,sizeof(cl_int) ,&cmd_in);

    //if(status != CL_SUCCESS) { 
    //    printf("-ERROR- Could not set argument[0] for  Kernel %s %d\n","read_stamp", status);
    //}

    status = clEnqueueTask(queue[0], kernel[0], 0, NULL, NULL);
    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not Enqueue Kernel %s\n","read_stamp");
    clFinish(queue[0]);

    status = clEnqueueReadBuffer(queue[0],
                                 read_buffer,
                                 CL_TRUE,
                                 0,
			         sizeof(stamp_t) * mem_size,
                                 *buf,
                                 0,
                                 NULL,
                                 NULL);
    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not read buffer from Kernel\n");
    clFinish(queue[0]);
}

void read_ii_ms_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,const metric_t           metric
                           ,stamp_t**                buf) {
    int NUM = metric == II ? NUM_II : NUM_MS;
    int SIZE = metric == II ? SIZE_II : SIZE_MS;
    const cl_int mem_size = NUM * SIZE;
    posix_memalign ((void**) buf, 64, sizeof(stamp_t) * mem_size);
    for(int trace_buffer = 0; trace_buffer < NUM; trace_buffer++) { 
        stamp_t* ii_buffer;
        read_ii_ms(context, program, kernel, queue, metric, trace_buffer, &ii_buffer);
        for(int sample_id = 0; sample_id < SIZE; sample_id++) { 
            (*buf)[trace_buffer * SIZE_II + sample_id] = ii_buffer[sample_id];
        }
        free(ii_buffer);
    }
}

void print_ii_ms(const metric_t metric
                ,stamp_t*       buf) { 
    int NUM = metric == II ? NUM_II : NUM_MS;
    int SIZE = metric == II ? SIZE_II : SIZE_MS;
    for(int i = 0; i < SIZE; i++) { 
        printf("TIME %5d",i);
        for(int j = 0; j < NUM; j++) { 
            printf(": %d : %10ld", buf[j * SIZE + i].index, buf[j * SIZE + i].time);
            if(metric == II && i > 0)
                printf(": %10ld", buf[j * SIZE + i].time - buf[j * SIZE + i - 1].time);
        }
        //if(i)printf("%4d ",(int) (time_stamp[1+DEBUG_SAMPLE_DEPTH+i] - time_stamp[i]));
        printf("\n");
    }
}

void read_ms(const cl_context         context
             ,const cl_program         program
             ,const cl_kernel*         kernel
             ,const cl_command_queue*  queue
             ,const cl_int             buffer_id
             ,stamp_t**                iibuf) {

    cl_mem read_buffer;
    cl_int status;

    const cl_int mem_size = SIZE_II;

    posix_memalign ((void **) iibuf, 64, sizeof(stamp_t) * mem_size);
    
    char*   kernel_name;
    size_t  kernel_name_size;
    cl_uint kernel_arg_size;
    char    message[200];
    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_FUNCTION_NAME ,0 , NULL, &kernel_name_size);

    sprintf(message,"Could not get info on kernel in %s", "collect_data");
    if(status != CL_SUCCESS) printf("%s %d",message,status);   
    kernel_name = (char *) malloc(sizeof(char) * kernel_name_size);

    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_FUNCTION_NAME 
                             ,sizeof(char)*kernel_name_size
                             ,kernel_name
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    status = clGetKernelInfo(kernel[0]
                             ,CL_KERNEL_NUM_ARGS 
                             ,sizeof(cl_uint)
                             ,&kernel_arg_size
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    printf("Kernel %s has %0d args\n", kernel_name, kernel_arg_size);

    read_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(stamp_t) * mem_size, NULL, &status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create read buffer in rest_debug\n");
    }
    
    cl_int metric = MS;
    clSetKernelArg(kernel[0], 0, sizeof(cl_int), &metric);
    clSetKernelArg(kernel[0], 1, sizeof(cl_int), &buffer_id);
    status = clSetKernelArg(kernel[0], 2, sizeof(cl_mem), &read_buffer);

    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not set argument[1] for  Kernel %s %d\n","read_stamp", status);

    //cl_int cmd_in = D_READ;
    //status = clSetKernelArg(kernel[0],0,sizeof(cl_int) ,&cmd_in);

    status = clEnqueueTask(queue[0], kernel[0], 0, NULL, NULL);
    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not Enqueue Kernel %s\n","read_stamp");
    clFinish(queue[0]);

    status = clEnqueueReadBuffer(queue[0],
                                 read_buffer,
                                 CL_TRUE,
                                 0,
			         sizeof(stamp_t) * mem_size,
                                 *iibuf,
                                 0,
                                 NULL,
                                 NULL);
    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not read buffer from Kernel\n");
    clFinish(queue[0]);
}

void read_ms_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,stamp_t**                iibuf) {
    const cl_int mem_size = NUM_MS * SIZE_MS;
    posix_memalign ((void**) iibuf, 64, sizeof(stamp_t) * mem_size);
    for(int trace_buffer = 0; trace_buffer < NUM_MS; trace_buffer++) { 
        stamp_t* ii_buffer;
        read_ms(context, program, kernel, queue, trace_buffer, &ii_buffer);
        for(int sample_id = 0; sample_id < SIZE_II; sample_id++) { 
            (*iibuf)[trace_buffer * SIZE_MS + sample_id] = ii_buffer[sample_id];
        }
        free(ii_buffer);
    }
}

void print_ms(stamp_t* msbuf) { 
    for(int i = 0; i < SIZE_MS; i++) { 
        printf("TIME %5d",i);
        for(int j = 0; j < NUM_MS; j++) { 
            printf(": %d : %10ld", msbuf[j * SIZE_MS + i].index, msbuf[j * SIZE_MS + i].time);
        }
        //if(i)printf("%4d ",(int) (time_stamp[1+DEBUG_SAMPLE_DEPTH+i] - time_stamp[i]));
        printf("\n");
    }
}

void read_st(const cl_context         context
             ,const cl_program         program
             ,const cl_kernel*         kernel
             ,const cl_command_queue*  queue
             ,const cl_int             buffer_id
             ,signal_t**               stbuf) {

    cl_mem read_buffer;
    cl_int status;

    const cl_int mem_size = SIZE_ST;

    posix_memalign ((void **) stbuf, 64, sizeof(signal_t) * mem_size);
    
    char*   kernel_name;
    size_t  kernel_name_size;
    cl_uint kernel_arg_size;
    char    message[200];
    status = clGetKernelInfo(kernel[1]
			     ,CL_KERNEL_FUNCTION_NAME ,0 , NULL, &kernel_name_size);
    sprintf(message,"Could not get info on kernel in %s", "collect_data");
    if(status != CL_SUCCESS) printf("%s %d",message,status);   
    kernel_name = (char *) malloc(sizeof(char) * kernel_name_size);

    status = clGetKernelInfo(kernel[1]
                             ,CL_KERNEL_FUNCTION_NAME 
                             ,sizeof(char)*kernel_name_size
                             ,kernel_name
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    status = clGetKernelInfo(kernel[1]
                             ,CL_KERNEL_NUM_ARGS 
                             ,sizeof(cl_uint)
                             ,&kernel_arg_size
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    read_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(signal_t) * mem_size, NULL, &status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create read buffer in rest_debug\n");
    }
    
    status = clSetKernelArg(kernel[1], 0, sizeof(cl_int), &buffer_id);
    status = clSetKernelArg(kernel[1], 1, sizeof(cl_mem), &read_buffer);

    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not set argument[1] for  Kernel %s %d\n","read_stamp", status);
    }

    status = clEnqueueTask(queue[1], kernel[1], 0, NULL, NULL);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not Enqueue Kernel %s\n","read_stamp");
    }
    clFinish(queue[1]);

    status = clEnqueueReadBuffer(queue[1],
                                 read_buffer,
                                 CL_TRUE,
                                 0,
			         sizeof(signal_t) * mem_size,
                                 *stbuf,
                                 0,
                                 NULL,
                                 NULL);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not read buffer from Kernel\n");
    }
    clFinish(queue[1]);
}

void read_st_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,signal_t**                stbuf) {
    
    const cl_int mem_size = NUM_ST * SIZE_ST;
    posix_memalign ((void**) stbuf, 64, sizeof(signal_t) * mem_size);
    for(int trace_buffer = 0; trace_buffer < NUM_ST; trace_buffer++) { 
        signal_t* st_buffer;
        read_st(context, program, kernel, queue, trace_buffer, &st_buffer);
        for(int sample_id = 0; sample_id < SIZE_II; sample_id++) { 
            (*stbuf)[trace_buffer * SIZE_ST + sample_id] = st_buffer[sample_id];
        }
        free(st_buffer);
    }
}

void print_st(signal_t* stbuf) { 
    for(int i = 0; i < SIZE_ST; i++) { 
        printf("TIME %5d",i);
        for(int j = 0; j < NUM_ST; j++) { 
            printf(": %d : %10ld : %f", stbuf[j * SIZE_ST + i].index, stbuf[j * SIZE_ST + i].time, stbuf[j * SIZE_ST + i].data);
        }
        printf("\n");
    }
}


void read_cf(const cl_context         context
             ,const cl_program         program
             ,const cl_kernel*         kernel
             ,const cl_command_queue*  queue
             ,const cl_int             buffer_id
             ,channel_t**              cfbuf) {

    cl_mem read_buffer;
    cl_int status;

    const cl_int mem_size = SIZE_CF;

    posix_memalign ((void **) cfbuf, 64, sizeof(channel_t) * mem_size);
    
    char*   kernel_name;
    size_t  kernel_name_size;
    cl_uint kernel_arg_size;
    char    message[200];
    status = clGetKernelInfo(kernel[2]
			     ,CL_KERNEL_FUNCTION_NAME ,0 , NULL, &kernel_name_size);
    sprintf(message,"Could not get info on kernel in %s", "collect_cf");
    if(status != CL_SUCCESS) printf("%s %d",message,status);   
    kernel_name = (char *) malloc(sizeof(char) * kernel_name_size);

    status = clGetKernelInfo(kernel[2]
                             ,CL_KERNEL_FUNCTION_NAME 
                             ,sizeof(char)*kernel_name_size
                             ,kernel_name
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    status = clGetKernelInfo(kernel[2]
                             ,CL_KERNEL_NUM_ARGS 
                             ,sizeof(cl_uint)
                             ,&kernel_arg_size
                             ,NULL);
    sprintf(message,"Could not get info on kernel in %s", kernel_name);
    if(status != CL_SUCCESS) printf("%s %d",message,status);   

    read_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(channel_t) * mem_size, NULL, &status);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not create read buffer in rest_debug\n");
    }
    
    status = clSetKernelArg(kernel[2], 0, sizeof(cl_int), &buffer_id);
    status = clSetKernelArg(kernel[2], 1, sizeof(cl_mem), &read_buffer);

    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not set argument[1] for  Kernel %s %d\n","read_stamp", status);
    }

    status = clEnqueueTask(queue[2], kernel[2], 0, NULL, NULL);
    if(status != CL_SUCCESS) { 
        printf("-ERROR- Could not Enqueue Kernel %s\n","read_stamp");
    }
    clFinish(queue[2]);

    status = clEnqueueReadBuffer(queue[2],
                                 read_buffer,
                                 CL_TRUE,
                                 0,
			         sizeof(channel_t) * mem_size,
                                 *cfbuf,
                                 0,
                                 NULL,
                                 NULL);
    if(status != CL_SUCCESS) 
        printf("-ERROR- Could not read buffer from Kernel\n");
    clFinish(queue[2]);
}

void read_cf_all_buffers(const cl_context         context
                           ,const cl_program         program
                           ,const cl_kernel*         kernel
                           ,const cl_command_queue*  queue
                           ,channel_t**              cfbuf) {
    
    const cl_int mem_size = NUM_CF * SIZE_CF;
    posix_memalign ((void**) cfbuf, 64, sizeof(channel_t) * mem_size);
    for(int trace_buffer = 0; trace_buffer < NUM_CF; trace_buffer++) { 
        channel_t* cf_buffer;
        read_cf(context, program, kernel, queue, trace_buffer, &cf_buffer);
        for(int sample_id = 0; sample_id < SIZE_CF; sample_id++) { 
            (*cfbuf)[trace_buffer * SIZE_CF + sample_id] = cf_buffer[sample_id];
        }
        free(cf_buffer);
    }
}

void print_cf(channel_t* cfbuf) { 
    for(int i = 0; i < SIZE_CF; i++) { 
        printf("TIME %5d",i);
        for(int j = 0; j < NUM_CF; j++) { 
            printf(": %10ld : %d", cfbuf[j * SIZE_ST + i].time, cfbuf[j * SIZE_ST + i].depth);
        }
        printf("\n");
    }
}

#endif //DEBUG_CPP
