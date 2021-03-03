#include <le_bench.h>
#include <utils.h>


const TestFunc_p TESTERS[] = {
    getpid_test,
    context_switch_test,
    fork_test,
    thread_create_test,
};


void print_results(BenchResult *p_res) {
    BenchResult *res = p_res;
    while (res) {
        if (res->errored) {
            fprintf(stderr, "Test failed\n");
            return;
        } else {
#ifndef DISABLE_TIMER
            printf("closest_k: %.2f %s; mean: %.2f %s; stddiv: %.2f %s;"
                   "max: %.2f %s; min: %.2f %s\n",
                    res->k_closest, TIME_UNIT, res->mean, TIME_UNIT,
                    res->stddiv, TIME_UNIT, res->max, TIME_UNIT,
                    res->min, TIME_UNIT);
#else
            printf("Test finished\n");
#endif
            res = res->child;
        }
    }

    res = p_res->child;
    while (res) {
        BenchResult *tmp = res->child;
        free(res);
        res = tmp;
    }
}


int main(int argc, char **argv) {
    int idx = argv[1][0] - '0';
    int iter = atoi(argv[2]);

    BenchConfig config;
    BenchResult res;
    config.iter = iter;
    config.i_size = SMALL;

    TESTERS[idx](&config, &res);
    print_results(&res);
}
