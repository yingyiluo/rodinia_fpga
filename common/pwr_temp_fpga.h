#include <sys/types.h>
#include <time.h>
#include <CL/opencl.h>
#include <aocl_mmd.h>
#include <unistd.h>
#include <omp.h>

extern int aclnalla_pcie0_handle;
const char *boardname = "aclnalla_pcie0";

// High-resolution timer.
static inline double getCurrentTimestamp() {
#ifdef _WIN32 // Windows
  // Use the high-resolution performance counter.

  static LARGE_INTEGER ticks_per_second = {};
  if(ticks_per_second.QuadPart == 0) {
    // First call - get the frequency.
    QueryPerformanceFrequency(&ticks_per_second);
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  double seconds = double(counter.QuadPart) / double(ticks_per_second.QuadPart);
  return seconds;
#else         // Linux
  timespec a;
  clock_gettime(CLOCK_MONOTONIC, &a);
  return (double(a.tv_nsec) * 1.0e-9) + double(a.tv_sec);
#endif
}

/**
  * The function displays the temperature and power values of the FPGA board 
  * using two API functions. The two function definitions can be found in the
  * Intel FPGA SDK for OpenCL Custom Platform Toolkit User Guide.
  */
static inline int print_monitor(FILE *f) {
	float temp, power;
	size_t retsize;
	double ts = getCurrentTimestamp();
	aocl_mmd_get_info(aclnalla_pcie0_handle, AOCL_MMD_TEMPERATURE, sizeof(float), (void *)&temp, &retsize);
	aocl_mmd_card_info(boardname, AOCL_MMD_POWER, sizeof(float), (void *)&power, &retsize);
	return fprintf(f, "[%f] temp=%f, power=%f\n", ts, temp, power);
}

/**
  * The function displays the temperature and power values of the FPGA board every 
  * one second until the flag is set to 1.
  */
static inline cl_int monitor_and_finish(int *flag, FILE *f) {
	struct timespec tick;
	int sleep_ret = 0;
	clock_gettime(CLOCK_REALTIME, &tick);
	#pragma omp barrier
	while (*flag == 0) {
		if (sleep_ret == 0) {
			print_monitor(f);
			tick.tv_sec++;
		}
		sleep_ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &tick, NULL);
	}
	print_monitor(f);
	return CL_SUCCESS;
}
