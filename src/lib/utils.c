#define _GNU_SOURCE

#include <utils.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef DISABLE_TIMER
    #ifdef USE_RDTSCP
        // measure execution time with rdtscp
        // "https://www.intel.com/content/dam/www/public/us/en/documents/
        // white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf"
        void _start_timer(TimeType *ts) {
            *ts = rdtscp_begin();
        }

        void _stop_timer(TimeType *ts) {
            *ts = rdtscp_end();
        }

        double _get_duration(TimeType *start, TimeType *end) {
            return *end - *start; // in CPU cycles
        }
    #else
        // measure execution time with Linux gettime
        void _start_timer(TimeType *ts) {
            clock_gettime(CLOCK_MONOTONIC, ts);
        }

        void _stop_timer(TimeType *ts) {
            clock_gettime(CLOCK_MONOTONIC, ts);
        }

        double _get_duration(TimeType *start, TimeType *end) {
            return get_timespec_diff_nsec(start, end); // in nano seconds
        }
    #endif // USE_RDTSCP
#else // DISABLE_TIMER
    // disable time measurement; it is useful if you are measuring the
    // CPI within ROIs in a simulator
    void _start_timer(TimeType *ts) { assert(false); }
    void _stop_timer(TimeType *ts) { assert(false); }
    double _get_duration(TimeType *start, TimeType *end) { assert(false); }
#endif // DISABLE_TIMER

uint64_t rdtscp_begin() {
    uint32_t lo, hi;
    asm volatile ("cpuid\n\t"
                  "rdtsc\n\t"
                  "mov %%edx, %0\n\t"
                  "mov %%eax, %1\n\t"
                  : "=r" (hi), "=r" (lo)
                  :: "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)hi << 32) | lo;
}

uint64_t rdtscp_end() {
    uint32_t lo, hi;
    asm volatile ("rdtscp\n\t"
                  "mov %%edx, %0\n\t"
                  "mov %%eax, %1\n\t"
                  "cpuid\n\t"
                  : "=r" (hi), "=r" (lo)
                  :: "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)hi << 32) | lo;
}

double get_timespec_diff_sec(struct timespec *tstart, struct timespec *tend) {
    if (tstart && tend) {
        return (double)(tend->tv_sec - tstart->tv_sec) +
               (double)(tend->tv_nsec - tstart->tv_nsec) * 1e-9;
    } else {
        return 0.0;
    }
}

double get_timespec_diff_nsec(struct timespec *tstart, struct timespec *tend) {
    if (tstart && tend) {
        return (double)(tend->tv_sec - tstart->tv_sec) * 1e9 +
               (double)(tend->tv_nsec - tstart->tv_nsec);
    } else {
        return 0.0;
    }
}

// data processing related
#define MAX_ITER 5
#define TOLERANCE 3
void aggregate(double *data, size_t size, double *mean, double *stddiv,
               double *max, double *min) {
    double avg = 0.0, div = 0.0;
    double _avg = 0.0, _stddiv = 0.0;

    bool changed = true;
    size_t iter = 0, removed = 0;
    while (iter < MAX_ITER && changed) {
        *max = LLONG_MIN;
        *min = LLONG_MAX;
        changed = iter == 0;
        removed = 0;
        avg = 0.0;
        div = 0.0;

        for (size_t idx = 0; idx < size; idx++) {
            if (iter == 0 || fabs(data[idx] - _avg) < TOLERANCE * _stddiv) {
                avg += data[idx] / (double)size;
                *max = data[idx] > *max ? data[idx] : *max;
                *min = data[idx] < *min ? data[idx] : *min;
            } else {
                changed = true;
                removed += 1;
            }
        }

        // account for the removed
        avg *= (size / (size - removed));
        size -= removed;

        for (size_t idx = 0; idx < size; idx++) {
            if (iter == 0 || fabs(data[idx] - _avg) < 3 * _stddiv) {
                div += ((data[idx] - avg) / size) * (data[idx] - avg);
            }
        }

        _avg = avg;
        _stddiv = sqrt(div);

        if (_avg < 1e-3 || _stddiv / _avg < 1e-3) {
            break;
        }

        iter++;
    }

    if (size != 0) {
        *mean = avg;
        if (size == 1) {
            *stddiv = 0;
        } else {
            *stddiv = sqrt(div);
        }
    } else {
        *mean = NAN;
        *stddiv = NAN;
        *max = NAN;
        *min = NAN;
    }
}

static int double_cmp(const void *a, const void *b) {
    if (*(double*)a < *(double*)b) return -1;
    else if (*(double*)a > *(double*)b) return 1;
    else return 0;
}

#define IMPRECISION 0.05
double closest_k(double *data, size_t size, unsigned int k) {
    if (size == 0) return NAN;
    if (size == 1) return data[0];

    qsort(data, size, sizeof(double), double_cmp);
    size_t cont_cnt = 0, idx;
    for (idx = 1; idx < size; idx++) {
        if ((fabs(data[idx] - data[idx - 1]) / data[idx]) < IMPRECISION) {
            cont_cnt += 1;
            if (cont_cnt == k) {
                return data[idx - cont_cnt];
            }
        } else {
            cont_cnt = 0;
        }
    }

    if (cont_cnt != k) {
        fprintf(stderr, ZWARN"Only found %lu closest\n", cont_cnt);
    }
    return data[idx - cont_cnt];
}

#define CLOSEST_K 5
void collect_results(double *data, size_t size, BenchConfig* config,
                     BenchResult *res) {
    res->config = config;
    res->child = NULL;
    res->errored = false;
#ifndef DISABLE_TIMER
    aggregate(data, size, &res->mean, &res->stddiv, &res->max, &res->min);
    res->k_closest = closest_k(data, size, CLOSEST_K);
#endif
}
