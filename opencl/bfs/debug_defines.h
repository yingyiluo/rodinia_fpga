#ifndef DEBUG_DEFINES__H
#define DEBUG_DEFINES__H

#define NUM_II 1
#define NUM_MS 0
#define NUM_ST 0
#define NUM_CF 0

typedef ulong  TIME_TYPE;
typedef float data_t;
/*
#define SIZE_II 1024
#define MASK_II 0x3ff
#define SIZE_MS 1024
#define MASK_MS 0x3ff
#define SIZE_ST 1024
#define MASK_ST 0x3ff
#define SIZE_CF 1024
#define MASK_CF 0x3ff
*/

#define SIZE_II 2048
#define MASK_II 0x7ff
#define SIZE_MS 2048
#define MASK_MS 0x7ff
#define SIZE_ST 2048
#define MASK_ST 0x7ff
#define SIZE_CF 2048
#define MASK_CF 0x7ff


typedef enum {
    II,
    MS,
    ST,
    DF
} metric_t;

typedef TIME_TYPE ftime_t;

typedef struct __attribute__((packed)) __attribute__((aligned(8))) {
    ftime_t      time;
    int          index;
} stamp_t;

typedef struct __attribute__((packed)) __attribute__((aligned(8))) {
    ftime_t      time;
    data_t       data;
    int          index;   
} signal_t;

typedef struct __attribute__((packed)) __attribute__((aligned(8))) {
    ftime_t      time;
    int          depth;
//    int          index;
} channel_t;
#endif //DEBUG_DEFINES__H
