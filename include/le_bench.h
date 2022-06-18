#ifndef LE_BENCH_H
#define LE_BENCH_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#define ZINFO  "[\033[32m INFO\033[0m] "
#define ZWARN  "[\033[33m WARN\033[0m] "
#define ZERROR "[\033[31mERROR\033[0m] "

#define LE_FILE_PATH "/tmp/LEBench_TMP"
#define LE_SOCK_PATH "/tmp/LEBench_SOCK"

#ifndef DISABLE_TIMER
#define TEST_INTERVAL 1  // micro-second
#else
#define TEST_INTERVAL 0  // micro-second
#endif

typedef enum {
    TEST,
    SMALL,
    MEDIUM,
    LARGE
} TestInputSize;

typedef enum {
    SIMSMALL,
    SIMLARGE,
    NATIVE
} SuiteInputSize;

typedef struct BenchConfig {
    size_t iter;
    TestInputSize i_size;
} BenchConfig;

typedef struct BenchResult {
    BenchConfig *config;
    double mean, stddev, max, min, k_closest;
    bool errored;

    // do forget to free the child
    struct BenchResult *child;
} BenchResult;

typedef bool(*TestFunc_p)(BenchConfig*, BenchResult*);

bool getpid_test(BenchConfig*, BenchResult*);

bool context_switch_test(BenchConfig*, BenchResult*);

bool fork_test(BenchConfig*, BenchResult*);

bool thread_create_test(BenchConfig*, BenchResult*);

bool mmap_test(BenchConfig*, BenchResult*);

bool munmap_test(BenchConfig*, BenchResult*);

bool page_fault_test(BenchConfig*, BenchResult*);

bool read_test(BenchConfig*, BenchResult*);

bool write_test(BenchConfig*, BenchResult*);

bool poll_test(BenchConfig*, BenchResult*);

bool epoll_test(BenchConfig*, BenchResult*);

bool select_test(BenchConfig*, BenchResult*);

bool send_test(BenchConfig*, BenchResult*);

bool recv_test(BenchConfig*, BenchResult*);

#endif
