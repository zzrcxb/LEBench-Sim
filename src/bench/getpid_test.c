#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <sys/syscall.h>


bool getpid_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = init_diff_array(iter_cnt);
    if (!diffs) goto err;

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        syscall(SYS_getpid);
        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    free(diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
