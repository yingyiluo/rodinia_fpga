#pragma OPENCL EXTENSION cl_altera_channels : enable channel

#if NUM_II > 0
channel stamp_t ii_chan[NUM_II] __attribute__((depth(SIZE_II)));
#endif

#if NUM_MS > 0
channel stamp_t ms_chan[NUM_MS] __attribute__((depth(SIZE_MS)));
#endif

#if NUM_ST > 0
channel signal_t st_chan[NUM_ST] __attribute__((depth(SIZE_ST)));
#endif

#if NUM_CF > 0
channel int       write_chan[NUM_CF] __attribute__((depth(4)));
channel int       read_chan[NUM_CF]  __attribute__((depth(4)));
channel channel_t cf_chan[NUM_CF]    __attribute__((depth(SIZE_CF)));
#endif

#if NUM_II > 0
inline void monitor_ii_1(__local stamp_t* buf, int i) {
  int n = i & MASK_II;
  buf[n].time = get_time((ftime_t)i);
  buf[n].index = i;
}

inline void monitor_ii_2(__local stamp_t* buf, int i, int j, int sj) {
  int idx = i*sj + j;
  int n = idx & MASK_II;
  buf[n].time = get_time((ftime_t)j);
  buf[n].index = idx;
}

inline void monitor_ii_3(__local stamp_t* buf, int i, int j, int k, int sj, int sk) {
  int idx = i*sj*sk + j*sk + k;
  int n = idx & MASK_II;
  buf[n].time = get_time((ftime_t)k);
  buf[n].index = idx;
}

inline void finish_monitor_ii(__local stamp_t* buf, int id) {
  for(int i = 0; i < SIZE_II; i++) {
    (void) write_channel_nb_altera(ii_chan[id], buf[i]);  
  }
}

inline void monitor_ii_ndr_1(int idx, int id) {
  stamp_t tmp;
  tmp.time = get_time((ftime_t)idx);
  tmp.index = idx;
  (void) write_channel_nb_altera(ii_chan[id], tmp);
}

#endif

#if NUM_MS > 0
// use two bufs to record start and end time
inline void monitor_ms_1(__local stamp_t* buf, int i, ftime_t dummy) {
  int n = i & MASK_MS;
  buf[n].time = get_time((ftime_t)dummy);
  buf[n].index = i;
  mem_fence(CLK_GLOBAL_MEM_FENCE);
}

inline ftime_t monitor_ms_prep_1(ftime_t dummy) {
  ftime_t t = get_time((ftime_t)dummy);
  mem_fence(CLK_GLOBAL_MEM_FENCE);
  return t;
}

inline void monitor_ms_1_one(__local stamp_t* buf, int i, ftime_t s, ftime_t dummy) {
  ftime_t time = get_time((ftime_t)dummy) - s;
  int n = i & MASK_MS;
  buf[n].time = time;
  buf[n].index = i;
  mem_fence(CLK_GLOBAL_MEM_FENCE);
}

inline void monitor_ms_2(__local stamp_t* buf, int i, int j, int sj, ftime_t dummy) {
  int idx = i*sj + j;
  int n = idx & MASK_MS;
  buf[n].time = get_time((ftime_t)dummy);
  buf[n].index = idx;
  mem_fence(CLK_GLOBAL_MEM_FENCE);
}

inline void monitor_ms_3(__local stamp_t* buf, int i, int j, int k, int sj, int sk, ftime_t dummy) {
  int idx = i*sj*sk + j*sk + k;
  int n = idx & MASK_MS;
  buf[n].time = get_time((ftime_t)dummy);
  buf[n].index = idx;
  mem_fence(CLK_GLOBAL_MEM_FENCE);
}

inline void finish_monitor_ms(__local stamp_t* buf, int id) {
  for(int i = 0; i < SIZE_MS; i++) {
    (void) write_channel_nb_altera(ms_chan[id], buf[i]);  
  }
}

inline void finish_monitor_ms_2(__local stamp_t* buf1, __local stamp_t* buf2, int id) {
  for(int i = 0; i < SIZE_MS; i++) {
    stamp_t tmp;
    tmp.time = buf2[i].time - buf1[i].time;
    tmp.index = buf1[i].index;
    (void) write_channel_nb_altera(ms_chan[id], tmp);  
  }
}
#endif

#if NUM_ST > 0
inline void monitor_st_1(__local signal_t* buf, int i, data_t value) {
  int n = i & MASK_ST;
  buf[n].time = get_time((ftime_t)value);
  buf[n].data = value;
  buf[n].index = i;
}

inline void monitor_st_2(__local signal_t* buf, int i, int j, int sj, data_t value) {
  int idx = i*sj + j;
  int n = idx & MASK_ST;
  buf[n].time = get_time((ftime_t)value);
  buf[n].data = value;
  buf[n].index = idx;
}

inline void monitor_st_3(__local signal_t* buf, int i, int j, int k, int sj, int sk, data_t value) {
  int idx = i*sj*sk + j*sk + k;
  int n = idx & MASK_ST;
  buf[n].time = get_time((ftime_t)value);
  buf[n].data = value;
  buf[n].index = idx;
}

inline void finish_monitor_st(__local signal_t* buf, int id) {
  for(int i = 0; i < SIZE_ST; i++) {
    (void) write_channel_nb_altera(st_chan[id], buf[i]);  
  }
}
#endif

#if NUM_CF > 0
inline void monitor_cf_write(int id, int depth) {
  mem_fence(CLK_CHANNEL_MEM_FENCE);
  (void) write_channel_nb_altera(write_chan[id], depth);
}

inline void monitor_cf_read(int id, int depth) {
  mem_fence(CLK_CHANNEL_MEM_FENCE);
  (void) write_channel_nb_altera(read_chan[id], depth);
}
#endif

#if (NUM_II + NUM_MS) > 0
__attribute__((max_global_work_dim(0)))
__kernel void collect_ii_ms(metric_t m,
                           int id,
                           __global stamp_t* restrict output) {
  int SIZE = m == II ? SIZE_II : SIZE_MS;
  for(int i = 0; i < SIZE; i++) {
    stamp_t tmp;
    #if NUM_II > 0
    if(m == II) {
      #pragma unroll
      for(int idx = 0; idx < NUM_II; idx++) {
        if(idx == id)
          tmp = read_channel_altera(ii_chan[idx]);
      }
    }
    #endif

    #if NUM_MS > 0
    if(m == MS) {
      #pragma unroll
      for(int idx = 0; idx < NUM_MS; idx++) {
        if(idx == id)
          tmp = read_channel_altera(ms_chan[idx]);
      }
    }
    #endif
    output[i] = tmp;
  }
}
#endif

#if NUM_ST > 0
__attribute__((max_global_work_dim(0)))
__kernel void collect_st(int id,
                         __global signal_t* restrict output) {
  for(int i = 0; i < SIZE_ST; i++) {
    signal_t tmp;
    #pragma unroll
    for(int idx = 0; idx < NUM_ST; idx++) {
      if(idx == id)
        tmp = read_channel_altera(st_chan[idx]);
    }
    output[i] = tmp;
  }
}
#endif


#if NUM_CF > 0
#ifdef EMULATOR

void watch_chan_emulate(uchar buffer_id) {
    #include "watch_chan_body.cl"
}

#if NUM_CF > 0 
    void kernel watch_chan_0() { watch_chan_emulate(0); }
#endif
#if NUM_CF > 1 
    void kernel watch_chan_1() { watch_chan_emulate(1); }
#endif
#if NUM_CF > 2 
    void kernel watch_chan_2() { watch_chan_emulate(2); }
#endif
#if NUM_CF > 3 
    void kernel watch_chan_3() { watch_chan_emulate(3); }
#endif

#else 

__attribute__((max_global_work_dim(0)))
__attribute__((autorun))
__attribute__((num_compute_units(NUM_CF,1)))
__kernel void watch_chan() { 
    uint buffer_id = get_compute_id(0);
    #include "watch_chan_body.cl" 
}

#endif //EMULATOR

__attribute__((max_global_work_dim(0)))
__kernel void collect_cf(int id,
                         __global channel_t* restrict output) {
  for(int i = 0; i < SIZE_CF; i++) {
    channel_t tmp;
    #pragma unroll
    for(int idx = 0; idx < NUM_CF; idx++) {
      if(idx == id)
        tmp = read_channel_altera(cf_chan[idx]);
    }
    output[i] = tmp;
  }
}
#endif
