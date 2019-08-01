/*
 * =====================================================================================
 *
 *       Filename:  lud.cu
 *
 *    Description:  The main wrapper for the suite
 *
 *        Version:  1.0
 *        Created:  10/22/2009 08:40:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Liang Wang (lw2aw), lw2aw@virginia.edu
 *        Company:  CS@UVa
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
/*
#if defined(AOCL_BOARD_a10pl4_dd4gb_gx115) || defined(AOCL_BOARD_p385a_sch_ax115)
	#include "../../../common/power_fpga.h"
#endif
*/
#if defined(AOCL_BOARD_p385a_sch_ax115)
	#include "../../../common/pwr_temp_fpga.h"
#endif

#include "common.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
using std::string;

#include "../../common/opencl_util.h"
#include "../../../common/timer.h"

static cl_context       context;
static cl_command_queue cmd_queue;
static cl_command_queue cmd_queue2;
static cl_device_type   device_type;
static cl_device_id   * device_list;
static cl_int           num_devices;

static int initialize()
{
	size_t size;
	cl_int err;
	// create OpenCL context
#if 0
	cl_platform_id platform_id;
	if (clGetPlatformIDs(1, &platform_id, NULL) != CL_SUCCESS) { printf("ERROR: clGetPlatformIDs(1,*,0) failed\n"); return -1; }
	cl_context_properties ctxprop[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id, 0};
	device_type = use_gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU;
#else
	cl_platform_id *platforms = NULL;
	cl_uint num_platforms = 0;
	cl_context_properties ctxprop[3];
	display_device_info(&platforms, &num_platforms);
	select_device_type(platforms, &num_platforms, &device_type);
	validate_selection(platforms, &num_platforms, ctxprop, &device_type);
#endif

	context = clCreateContextFromType( ctxprop, device_type, NULL, NULL, &err );
	if ( err != CL_SUCCESS ) {
		display_error_message(err, stderr);
		return -1;
	}

	// get the list of GPUs
	CL_SAFE_CALL(clGetContextInfo( context, CL_CONTEXT_DEVICES, 0, NULL, &size ));
	num_devices = (int) (size / sizeof(cl_device_id));
	//printf("num_devices = %d\n", num_devices);

	if( num_devices < 1 ) { printf("ERROR: clGetContextInfo() failed\n"); return -1; }
	device_list = new cl_device_id[num_devices];
	if( !device_list ) { printf("ERROR: new cl_device_id[] failed\n"); return -1; }
	CL_SAFE_CALL(clGetContextInfo( context, CL_CONTEXT_DEVICES, size, device_list, NULL ));

	// create command queue for the first device
	cmd_queue = clCreateCommandQueue( context, device_list[0], 0, NULL );
	cmd_queue2 = clCreateCommandQueue( context, device_list[0], 0, NULL );
	if( !cmd_queue ) { printf("ERROR: clCreateCommandQueue() failed\n"); return -1; }
	if( !cmd_queue2 ) { printf("ERROR: clCreateCommandQueue() failed\n"); return -1; }
	return 0;
}

static int shutdown()
{
	// release resources
	if( cmd_queue ) clReleaseCommandQueue( cmd_queue );
	if( cmd_queue2 ) clReleaseCommandQueue( cmd_queue2 );
	if( context ) clReleaseContext( context );
	if( device_list ) delete device_list;

	// reset all variables
	cmd_queue = 0;
	cmd_queue2 = 0;
	context = 0;
	device_list = 0;
	num_devices = 0;
	device_type = 0;

	return 0;
}

static int do_verify = 0;
void lud_cuda(float *d_m, int matrix_dim);

static struct option long_options[] = {
      /* name, has_arg, flag, val */
      {"input", 1, NULL, 'i'},
      {"size", 1, NULL, 's'},
      {"verify", 0, NULL, 'v'},
      {0,0,0,0}
};

int main ( int argc, char *argv[] )
{
	int matrix_dim = 32; // default matrix dimension
	int opt, option_index=0;
	func_ret_t ret;
	const char *input_file = NULL;
	float *m, *mm;
	int version;
	TimeStamp start, end;
	double totalTime;

#ifdef PROFILE
	TimeStamp start1, end1;
	double diatotal = 0, peritotal = 0, intertotal = 0;
#endif
	size_t globalwork2, globalwork3;

#if defined(AOCL_BOARD_a10pl4_dd4gb_gx115) || defined(AOCL_BOARD_p385a_sch_ax115)
	// power measurement parameters, only for Bittware and Nallatech's Arria 10 boards
	int flag = 0;
	double power = 0;
	double energy = 0;
#endif

	cl_kernel diagonal = NULL, perimeter = NULL, internal = NULL;	// for NDRange kernels

	// get kernel version from commandline
	init_fpga(&argc, &argv, &version);

	// Does Windows have getopt_long? This is just simple argument
	// handling, so if it's not available on Windows, just not use
	// the function.
	while ((opt = getopt_long(argc, argv, "::vs:i:b:", long_options, &option_index)) != -1 ) {
		switch(opt){
			case 'i':
				input_file = optarg;
				break;
			case 'v':
				do_verify = 1;
				break;
			case 's':
				matrix_dim = atoi(optarg);
				printf("Generate input matrix internally, size =%d\n", matrix_dim);
				break;
			case '?':
				fprintf(stderr, "invalid option\n");
				break;
			case ':':
				fprintf(stderr, "missing argument\n");
				break;
			default:
				fprintf(stderr, "Usage: %s [-v] [-s matrix_size|-i input_file]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if ( (optind < argc) || (optind == 1)) {
		fprintf(stderr, "Usage: %s [-v] [-s matrix_size|-i input_file]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (input_file) {
		printf("Reading matrix from file %s\n", input_file);
		ret = create_matrix_from_file(&m, input_file, &matrix_dim);
		if (ret != RET_SUCCESS) {
			m = NULL;
			fprintf(stderr, "error create matrix from file %s\n", input_file);
			exit(EXIT_FAILURE);
		}
	}

	else if (matrix_dim) {
		printf("Creating matrix internally size=%d\n", matrix_dim);
		ret = create_matrix(&m, matrix_dim);
		if (ret != RET_SUCCESS) {
			m = NULL;
			fprintf(stderr, "error create matrix internally size=%d\n", matrix_dim);
			exit(EXIT_FAILURE);
		}
	}

	else {
		printf("No input file specified!\n");
		exit(EXIT_FAILURE);
	}

	printf("WG size of kernel = %d X %d\n", BSIZE, BSIZE);

	if (do_verify){
		//printf("Before LUD\n");
		// print_matrix(m, matrix_dim);
		matrix_duplicate(m, &mm, matrix_dim);
	}
	
	//int sourcesize = 1024*1024;
	//char * source = (char *)calloc(sourcesize, sizeof(char)); 
	//if(!source) { printf("ERROR: calloc(%d) failed\n", sourcesize); return -1; }
	
	size_t sourcesize;
	char *kernel_file_path = getVersionedKernelName("./ocl/lud_kernel", version);
	char *source = read_kernel(kernel_file_path, &sourcesize);
	free(kernel_file_path);

	// OpenCL initialization
	if (initialize()) return -1;
	
	// compile kernel
	cl_int err = 0;
	
#if defined(USE_JIT)
	const char * slist[2] = { source, 0 };
	cl_program prog = clCreateProgramWithSource(context, 1, slist, NULL, &err);
#else
	cl_program prog = clCreateProgramWithBinary(context, 1, device_list, &sourcesize, (const unsigned char**)&source, NULL, &err);
#endif

	if(err != CL_SUCCESS) {
		display_error_message(err, stderr);
		return -1;
	}
#if defined(USE_JIT)
	char clOptions[110];
	sprintf(clOptions, "-I./ocl -DBSIZE=%d", BSIZE);
	clBuildProgram_SAFE(prog, num_devices, device_list, clOptions, NULL, NULL);
#endif // USE_JIT        

	cl_mem d_m;
	d_m = clCreateBuffer(context, CL_MEM_READ_WRITE, matrix_dim * matrix_dim * sizeof(float), NULL, &err );
	if(err != CL_SUCCESS) { printf("ERROR: clCreateBuffer d_m (size:%d) => %d\n", matrix_dim * matrix_dim, err); return -1;} 

	CL_SAFE_CALL(clEnqueueWriteBuffer(cmd_queue, d_m, 1, 0, matrix_dim * matrix_dim * sizeof(float), m, 0, 0, 0));

	// create kernels
	diagonal = clCreateKernel(prog, "lud_diagonal", &err);
	if(err != CL_SUCCESS) { printf("ERROR: clCreateKernel(diagonal) 0 => %d\n", err); return -1; }
	perimeter = clCreateKernel(prog, "lud_perimeter", &err);
	if(err != CL_SUCCESS) { printf("ERROR: clCreateKernel(perimeter) 0 => %d\n", err); return -1; }
	internal = clCreateKernel(prog, "lud_internal", &err);  
	if(err != CL_SUCCESS) { printf("ERROR: clCreateKernel(internal) 0 => %d\n", err); return -1; }
	clReleaseProgram(prog); 

	// set fixed kernel arguments
	CL_SAFE_CALL( clSetKernelArg(diagonal, 0, sizeof(void *), (void*) &d_m       ) );
	CL_SAFE_CALL( clSetKernelArg(diagonal, 1, sizeof(cl_int), (void*) &matrix_dim) );
	
	CL_SAFE_CALL( clSetKernelArg(perimeter, 0, sizeof(void *), (void*) &d_m       ) );
	CL_SAFE_CALL( clSetKernelArg(perimeter, 1, sizeof(cl_int), (void*) &matrix_dim) );
	
	CL_SAFE_CALL( clSetKernelArg(internal, 0, sizeof(void *), (void*) &d_m       ) );
	CL_SAFE_CALL( clSetKernelArg(internal, 1, sizeof(cl_int), (void*) &matrix_dim) );
	
	// fixed work sizes
	size_t global_work1[3] = {(size_t)BSIZE, 1, 1};
	size_t local_work1[3]  = {(size_t)BSIZE, 1, 1};

	size_t local_work2[3]  = {(size_t)BSIZE * 2, 1, 1};

	size_t local_work3[3]  = {(size_t)BSIZE, (size_t)BSIZE, 1};

#if defined(AOCL_BOARD_a10pl4_dd4gb_gx115) || defined(AOCL_BOARD_p385a_sch_ax115)
	#pragma omp parallel num_threads(2) shared(flag)
	{
		if (omp_get_thread_num() == 0)
		{
			#ifdef AOCL_BOARD_a10pl4_dd4gb_gx115
				power = GetPowerFPGA(&flag);
			#else
				//power = GetPowerFPGA(&flag, device_list);
				monitor_and_finish(&flag, stdout);
			#endif
		}
		else
		{
			#pragma omp barrier
#endif
			// beginning of timing point
			GetTime(start);

			for (int i = 0; i < matrix_dim - BSIZE; i += BSIZE)
			{
				if (version == 4)
				{
					int offset = i * matrix_dim + i;
					CL_SAFE_CALL( clSetKernelArg(diagonal , 2, sizeof(cl_int), (void*) &offset) );
					CL_SAFE_CALL( clSetKernelArg(perimeter, 2, sizeof(cl_int), (void*) &offset) );
					CL_SAFE_CALL( clSetKernelArg(internal , 2, sizeof(cl_int), (void*) &offset) );
				}
				else
				{
					CL_SAFE_CALL( clSetKernelArg(diagonal , 2, sizeof(cl_int), (void*) &i) );
					CL_SAFE_CALL( clSetKernelArg(perimeter, 2, sizeof(cl_int), (void*) &i) );
					CL_SAFE_CALL( clSetKernelArg(internal , 2, sizeof(cl_int), (void*) &i) );
				}
				
				globalwork2 = BSIZE * 2 * (((matrix_dim-i)/BSIZE) - 1);
				globalwork3 = BSIZE * (((matrix_dim-i)/BSIZE) - 1);
				
				size_t global_work2[3] = {globalwork2, 1, 1};
				size_t global_work3[3] = {globalwork3, globalwork3, 1};

				if (is_ndrange_kernel(version))
				{
					#ifdef PROFILE
					GetTime(start1);
					#endif
					
					CL_SAFE_CALL( clEnqueueNDRangeKernel(cmd_queue, diagonal , 2, NULL, global_work1, local_work1, 0, 0, 0) );
					clFinish(cmd_queue);
					
					#ifdef PROFILE
					GetTime(end1);
					diatotal += TimeDiff(start1, end1);
					printf("%d: diameter: %f\n", i, TimeDiff(start1, end1));
					#endif

					#ifdef PROFILE
					GetTime(start1);
					#endif
					
					CL_SAFE_CALL( clEnqueueNDRangeKernel(cmd_queue, perimeter, 2, NULL, global_work2, local_work2, 0, 0, 0) );
					clFinish(cmd_queue);
					
					#ifdef PROFILE
					GetTime(end1);
					peritotal += TimeDiff(start1, end1);
					printf("%d: perimete: %f\n", i, TimeDiff(start1, end1));
					#endif

					#ifdef PROFILE
					GetTime(start1);
					#endif

					CL_SAFE_CALL( clEnqueueNDRangeKernel(cmd_queue, internal , 2, NULL, global_work3, local_work3, 0, 0, 0) );
					clFinish(cmd_queue);

					#ifdef PROFILE
					GetTime(end1);
					intertotal += TimeDiff(start1, end1);
					printf("%d: internal: %f\n\n", i, TimeDiff(start1, end1));
					#endif
				}
				else
				{
					#ifdef PROFILE
					GetTime(start1);
					#endif
					
					CL_SAFE_CALL( clEnqueueTask(cmd_queue, diagonal , 0, NULL, NULL) );
					clFinish(cmd_queue);
					
					#ifdef PROFILE
					GetTime(end1);
					diatotal += TimeDiff(start1, end1);
					printf("%d: diameter: %f\n", i, TimeDiff(start1, end1));
					#endif

					#ifdef PROFILE
					GetTime(start1);
					#endif

					if (version == 1)
					{
						CL_SAFE_CALL( clEnqueueTask(cmd_queue, perimeter, 0, NULL, NULL) );
					}
					else
					{
						int chunk_num = ((matrix_dim - i) / BSIZE) - 1;

						for (int chunk_idx = 0; chunk_idx < chunk_num; chunk_idx++)
						{
							int offset = i + BSIZE * (chunk_idx + 1);

							CL_SAFE_CALL( clSetKernelArg(perimeter, 3, sizeof(cl_int), (void*) &offset) );

							CL_SAFE_CALL( clEnqueueTask(cmd_queue, perimeter, 0, NULL, NULL) );
						}
					}
					clFinish(cmd_queue);
					
					#ifdef PROFILE
					GetTime(end1);
					peritotal += TimeDiff(start1, end1);
					printf("%d: perimete: %f\n", i, TimeDiff(start1, end1));
					#endif

					#ifdef PROFILE
					GetTime(start1);
					#endif

					if (version == 1)
					{
						CL_SAFE_CALL( clEnqueueTask(cmd_queue, internal, 0, NULL, NULL) );
					}
					else
					{
						int chunk_num = ((matrix_dim - i) / BSIZE) - 1;

						for  (int chunk_idx = 0; chunk_idx < chunk_num * chunk_num; chunk_idx++)
						{
							int i_global = i + BSIZE * (1 + chunk_idx / chunk_num);
							int j_global = i + BSIZE * (1 + chunk_idx % chunk_num);

							CL_SAFE_CALL( clSetKernelArg(internal, 3, sizeof(cl_int), (void*) &i_global) );
							CL_SAFE_CALL( clSetKernelArg(internal, 4, sizeof(cl_int), (void*) &j_global) );

							CL_SAFE_CALL( clEnqueueTask(cmd_queue, internal, 0, NULL, NULL) );
						}
					}
					clFinish(cmd_queue);

					#ifdef PROFILE
					GetTime(end1);
					intertotal += TimeDiff(start1, end1);
					printf("%d: internal: %f\n\n", i, TimeDiff(start1, end1));
					#endif
				}
			}

			#ifdef PROFILE
			GetTime(start1);
			#endif

			int offset = (version == 4) ? (matrix_dim - BSIZE) * matrix_dim + (matrix_dim - BSIZE) : matrix_dim - BSIZE;
			CL_SAFE_CALL( clSetKernelArg(diagonal, 2, sizeof(cl_int), (void*) &offset) );

			if (is_ndrange_kernel(version))
			{
				CL_SAFE_CALL( clEnqueueNDRangeKernel(cmd_queue, diagonal, 2, NULL, global_work1, local_work1, 0, 0, 0) );
			}
			else
			{
				CL_SAFE_CALL( clEnqueueTask(cmd_queue, diagonal, 0, NULL, NULL) );
			}

			clFinish(cmd_queue);

			#ifdef PROFILE
			GetTime(end1);
			diatotal += TimeDiff(start1, end1);
			printf("%d: diameter: %f\n", matrix_dim, TimeDiff(start1, end1));
			#endif

			// end of timing point
			GetTime(end);
#if defined(AOCL_BOARD_a10pl4_dd4gb_gx115) || defined(AOCL_BOARD_p385a_sch_ax115)
			flag = 1;
		}
	}
#endif

	CL_SAFE_CALL( clEnqueueReadBuffer(cmd_queue, d_m, 1, 0, matrix_dim * matrix_dim * sizeof(float), m, 0, 0, 0) );
	clFinish(cmd_queue);
	clReleaseMemObject(d_m);

	#ifdef PROFILE
	printf("\ntotal: diameter: %0.3lf\n", diatotal);
	printf("total: perimete: %0.3lf\n", peritotal);
	printf("total: internal: %0.3lf\n\n", intertotal);
	#endif

	totalTime = TimeDiff(start, end);
	printf("Computation done in %0.3lf ms.\n", totalTime);
/*
#if defined(AOCL_BOARD_a10pl4_dd4gb_gx115) || defined(AOCL_BOARD_p385a_sch_ax115)
	energy = GetEnergyFPGA(power, totalTime);
	if (power != -1) // -1 --> failed to read energy values
	{
		printf("Total energy used is %0.3lf jouls.\n", energy);
		printf("Average power consumption is %0.3lf watts.\n", power);
	}
#endif
*/
	if (do_verify){
		//printf("After LUD\n");
		// print_matrix(m, matrix_dim);
		printf("Verifying output: ");
		if (lud_verify(mm, m, matrix_dim) == RET_SUCCESS)
		{
			printf("verification succeeded!\n");
		}
		else
		{
			printf("verification failed!\n");
		}
		free(mm);
	}

	free(m);
	free(source);
	
	if(shutdown()) return -1;
	
}				

/* ----------  end of function main  ---------- */


