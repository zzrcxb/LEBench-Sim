#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <le_bench.h>

#ifdef GEM5_HOOK
#include <m5ops.h>
#endif

#define UNUSED __attribute__((unused))

// timing related
#ifndef DISABLE_TIMER
#ifdef USE_RDTSCP
typedef uint64_t TimeType;
#define TIME_UNIT "cyc."
#else // USE_RDTSCP
typedef struct timespec TimeType;
#define TIME_UNIT "ns"
#endif // USE_RDTSCP

#else // DISABLE_TIMER
typedef uint64_t TimeType;
#define TIME_UNIT "cyc."
#endif // DISABLE_TIMER

void start_timer(TimeType *ts);
void stop_timer(TimeType *ts);
double get_duration(TimeType *start, TimeType *end);

uint64_t rdtscp_begin();
uint64_t rdtscp_end();

double get_timespec_diff_sec(struct timespec *tstart, struct timespec *tend);
double get_timespec_diff_nsec(struct timespec *tstart, struct timespec *tend);

// roi related
inline void roi_begin() {
    fprintf(stderr, "=== ROI Begin ===\n");
#ifdef GEM5_HOOK
    m5_work_begin(0, 0);
    m5_reset_stats(0,0);
#endif
}

inline void roi_end() {
#ifdef GEM5_HOOK
    m5_work_end(0, 0);
    m5_dump_stats(0,0);
#endif
    fprintf(stderr, "=== ROI End ===\n");
}

// data processing related
void aggregate(double *data, size_t size, double *mean, double *stddiv,
               double *max, double *min);

double closest_k(double *data, size_t size, unsigned int k);

void collect_results(double *data, size_t size, BenchConfig* config,
                     BenchResult *res);

#endif