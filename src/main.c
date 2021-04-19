#define _GNU_SOURCE

#include <le_bench.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils.h>

typedef struct TestConfig {
    TestFunc_p tester;
    size_t iter;
    TestInputSize i_size;
    char *name;
} TestConfig;

const TestConfig testConfigs[] = {
    (TestConfig){context_switch_test, 200, LARGE, "context-switch"},  // 0
    (TestConfig){read_test, 500, SMALL, "small-read"},                // 1
    (TestConfig){read_test, 200, MEDIUM, "med-read"},                 // 2
    (TestConfig){read_test, 100, LARGE, "big-read"},                  // 3
    (TestConfig){write_test, 500, SMALL, "small-write"},              // 4
    (TestConfig){write_test, 200, MEDIUM, "med-write"},               // 5
    (TestConfig){write_test, 50, LARGE, "large-write"},               // 6
    (TestConfig){mmap_test, 500, MEDIUM, "mmap"},                     // 7
    (TestConfig){munmap_test, 500, MEDIUM, "munmap"},                 // 8
    (TestConfig){fork_test, 200, SMALL, "fork"},                      // 9
    (TestConfig){fork_test, 50, LARGE, "big-fork"},                  // 10
    (TestConfig){thread_create_test, 200, LARGE, "thrcreate"},        // 11
    (TestConfig){send_test, 500, SMALL, "small-send"},                // 12
    (TestConfig){send_test, 200, LARGE, "big-send"},                  // 13
    (TestConfig){recv_test, 500, SMALL, "small-recv"},                // 14
    (TestConfig){recv_test, 200, LARGE, "big-recv"},                  // 15
    (TestConfig){select_test, 500, SMALL, "small-select"},            // 16
    (TestConfig){select_test, 100, LARGE, "big-select"},              // 17
    (TestConfig){poll_test, 500, SMALL, "small-poll"},                // 18
    (TestConfig){poll_test, 100, LARGE, "big-poll"},                  // 19
    (TestConfig){epoll_test, 500, SMALL, "small-epoll"},              // 20
    (TestConfig){epoll_test, 100, LARGE, "big-epoll"},                // 21
    (TestConfig){page_fault_test, 200, SMALL, "small-pagefault"},     // 22
    (TestConfig){page_fault_test, 100, LARGE, "big-pagefault"},       // 23
};
const size_t NUM_TESTS = 24;

void print_results(BenchResult *p_res, char *name) {
    BenchResult *res = p_res;
    while (res) {
        if (res->errored) {
            fprintf(stderr, "Test failed\n");
            break;
        } else {
#ifndef DISABLE_TIMER
            printf(
                "%s: closest_k: %.2f %s; mean: %.2f %s; stddev: %.2f %s;"
                "max: %.2f %s; min: %.2f %s\n",
                name,
                res->k_closest, TIME_UNIT, res->mean, TIME_UNIT,
                res->stddev, TIME_UNIT, res->max, TIME_UNIT,
                res->min, TIME_UNIT);
#else
            printf("Test finished\n");
#endif
            res = res->child;
        }
    }

    // free up everything
    res = p_res->child;
    while (res) {
        BenchResult *child = res->child;
        free(res);
        res = child;
    }
}

inline bool set_affinity_priority(unsigned core, int prio) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    bool ret1 = sched_setaffinity(getpid(), sizeof(set), &set) != -1;
    bool ret2 = setpriority(PRIO_PROCESS, 0, prio) != -1;
    return ret1 && ret2;
}

int main(int argc, char **argv) {
    int idx = atoi(argv[1]);
    int scale = atoi(argv[2]);

    if (idx < 0 || idx >= NUM_TESTS) {
        fprintf(stderr, "Invalid test ID: %d; Test ID range: 0-%lu\n",
                idx, NUM_TESTS);
        return 1;
    }

    if (scale < 1) {
        fprintf(stderr, "Invalid scale %d\n", scale);
        return 2;
    }

    BenchConfig config;
    BenchResult res;
    res.child = NULL;

    TestConfig t_config = testConfigs[idx];

    fprintf(stderr, ZINFO "Running \"%s\"\n", t_config.name);

    config.iter = t_config.iter * scale;
    config.i_size = t_config.i_size;
    t_config.tester(&config, &res);

    print_results(&res, t_config.name);
}
