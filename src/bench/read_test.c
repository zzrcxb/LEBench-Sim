#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>


#define FILE_PATH "/tmp/LEBench_WR"
bool read_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    int fd = -1;
    char *buf = NULL;
    double *diffs = init_diff_array(iter_cnt);
    if (!diffs) goto err;

    size_t file_size;
    switch (config->i_size) {
        case TEST: file_size = 4096; break;
        case SMALL: file_size = 4096; break;
        case MEDIUM: file_size = 40960; break;
        case LARGE: file_size = 4096000; break;
        default: assert(false && "Invalid input size");
    }

    buf = calloc(file_size, sizeof(char));
    fd = create_and_fill(LE_FILE_PATH, file_size, 'a');
    if (fd < 0 || !buf) goto err;

    // warmup
    bool UNUSED _ret;
    for (int i = 0; i < 100; i++) {
        _ret = read(fd, buf, file_size);
    }

    // real measurement
    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        _ret = read(fd, buf, file_size);
        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    free(buf);
    free(diffs);
    close(fd);
    remove(FILE_PATH);
    return ret;

err:
    ret = true;
    goto cleanup;
}
