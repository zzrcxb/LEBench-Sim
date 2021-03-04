#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <pthread.h>
#include <time.h>


static void *t_func(void *arg) {
    TimeType *cend = (TimeType*)arg;
    stop_timer(cend);
    pthread_exit(NULL);
}

void thread_create_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, t_parent_end, t_child_end;
    size_t iter_cnt = config->iter;

    double *parent_diffs = (double*)malloc(iter_cnt * sizeof(double));
    double *child_diffs = (double*)malloc(iter_cnt * sizeof(double));

    pthread_t new_thrd;
    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        int UNUSED err = pthread_create(&new_thrd, NULL, t_func, &t_child_end);
        stop_timer(&t_parent_end);

        pthread_join(new_thrd, NULL);

        get_duration(parent_diffs[idx], &tstart, &t_parent_end);
        get_duration(child_diffs[idx], &tstart, &t_child_end);
    }
    roi_end();

    // collect results
    collect_results(parent_diffs, iter_cnt, config, res);
    res->child = (BenchResult*)malloc(sizeof(BenchResult));
    collect_results(child_diffs, iter_cnt, config, res->child);

    // clean up
    free(parent_diffs);
    free(child_diffs);
    return;
}
