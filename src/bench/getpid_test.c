#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <sys/syscall.h>


void getpid_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);

        syscall(SYS_getpid);

        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();

    // process resultss
    collect_results(diffs, iter_cnt, config, res);

    // clean up
    free(diffs);
    return;
}
