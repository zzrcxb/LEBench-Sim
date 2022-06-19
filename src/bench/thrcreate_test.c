#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


static void *t_func(void *arg) {
    TimeType *cend = (TimeType*)arg;
    stop_timer(cend);
    pthread_exit(NULL);
}

bool thread_create_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, t_parent_end, t_child_end;
    size_t iter_cnt = config->iter;

    double *parent_diffs = init_diff_array(iter_cnt);
    double *child_diffs = init_diff_array(iter_cnt);
    if (!parent_diffs || !child_diffs) goto err;

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
    if (!res->child) {
        fprintf(stderr, ZERROR "Failed to allocate for child results\n");
        goto err;
    }
    collect_results(child_diffs, iter_cnt, config, res->child);

cleanup:
    free(parent_diffs);
    free(child_diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
