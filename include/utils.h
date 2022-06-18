#pragma once

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
#define ALWAYS_INLINE __attribute__((always_inline))

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
    // just put something here to pass compilation
    typedef uint64_t TimeType;
    #define TIME_UNIT "n.d."
#endif // DISABLE_TIMER

void _start_timer(TimeType *ts);
void _stop_timer(TimeType *ts);
double _get_duration(TimeType *start, TimeType *end);

#ifndef DISABLE_TIMER
    #define start_timer(TS) (_start_timer(TS))
    #define stop_timer(TS) (_stop_timer(TS))
    #define get_duration(DEST, START, END) (DEST = _get_duration(START, END))
#else
    #define start_timer(TS) {}
    #define stop_timer(TS) {}
    #define get_duration(DEST, START, END) {}
#endif

ALWAYS_INLINE inline uint64_t _rdtsc_google_begin(void) {
#ifdef AARCH64
    // https://lore.kernel.org/patchwork/patch/1305380/
    // SPDX-License-Identifier: GPL-2.0
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    uint64_t t;
    __asm__ __volatile__("lfence\n\t"
                         "rdtsc\n\t"
                         "shl $32, %%rdx\n\t"
                         "or %%rdx, %0\n\t"
                         "lfence"
                         : "=a"(t)
                         :
                         // "memory" avoids reordering. rdx = TSC >> 32.
                         // "cc" = flags modified by SHL.
                         : "rdx", "memory", "cc");
    return t;
#endif
}

ALWAYS_INLINE inline uint64_t _rdtscp_google_end(void) {
#ifdef AARCH64
    // https://lore.kernel.org/patchwork/patch/1305380/
    // SPDX-License-Identifier: GPL-2.0
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    uint64_t t;
    __asm__ __volatile__("rdtscp\n\t"
                         "shl $32, %%rdx\n\t"
                         "or %%rdx, %0\n\t"
                         "lfence"
                         : "=a"(t)
                         :
                         // "memory" avoids reordering.
                         // rcx = TSC_AUX. rdx = TSC >> 32.
                         // "cc" = flags modified by SHL.
                         : "rcx", "rdx", "memory", "cc");
    return t;
#endif
}

double get_timespec_diff_sec(struct timespec *tstart, struct timespec *tend);
double get_timespec_diff_nsec(struct timespec *tstart, struct timespec *tend);

// Region-Of-Interests (ROI) related
ALWAYS_INLINE inline void roi_begin() {
#ifdef GEM5_HOOK
    fprintf(stderr, "=== ROI Begin ===\n");
    m5_work_begin(0, 0);
    m5_reset_stats(0, 0);
#endif
}

ALWAYS_INLINE inline void roi_end() {
#ifdef GEM5_HOOK
    m5_work_end(0, 0);
    m5_dump_stats(0, 0);
    fprintf(stderr, "=== ROI End ===\n");
#endif
}

// data processing related
void aggregate(double *data, size_t size, double *mean, double *stddev,
               double *max, double *min);

double closest_k(double *data, size_t size, unsigned int k);

void collect_results(double *data, size_t size, BenchConfig* config,
                     BenchResult *res);
