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

typedef enum {
    TEST,
    SMALL,
    MEDIUM,
    LARGE
} InputSize;


typedef struct BenchConfig {
    size_t iter;
    InputSize i_size;
} BenchConfig;

typedef struct BenchResult {
    BenchConfig *config;
    double mean, stddiv, max, min, k_closest;
    bool errored;

    // do forget to free the child
    struct BenchResult *child;
} BenchResult;

typedef void(*TestFunc_p)(BenchConfig*, BenchResult*);

void getpid_test(BenchConfig*, BenchResult*);

void context_switch_test(BenchConfig*, BenchResult*);

void fork_test(BenchConfig*, BenchResult*);

void thread_create_test(BenchConfig*, BenchResult*);

void mmap_test(BenchConfig*, BenchResult*);

void page_fault_test(BenchConfig*, BenchResult*);

void read_test(BenchConfig*, BenchResult*);

void write_test(BenchConfig*, BenchResult*);

void poll_test(BenchConfig*, BenchResult*);

void epoll_test(BenchConfig*, BenchResult*);

void select_test(BenchConfig*, BenchResult*);

void send_recv_test(BenchConfig*, BenchResult*);

#endif