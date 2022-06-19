#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>


bool write_test(BenchConfig *config, BenchResult *res) {
    bool ret = false; // has error?
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = NULL;
    char *buf = NULL;
    int fd = -1;

    size_t file_size;
    switch (config->i_size) {
        case TEST: file_size = 4096; break;
        case SMALL: file_size = 4096; break;
        case MEDIUM: file_size = 40960; break;
        case LARGE: file_size = 40960 * 4; break;
        default: assert(false && "Invalid input size");
    }
    fd = create_and_fill(LE_FILE_PATH, file_size, 'a');
    if (fd < 0) goto err;

    buf = malloc(file_size);
    diffs = init_diff_array(iter_cnt);
    if (!diffs || !buf) goto err;
    memset(buf, 'a', file_size);

    // warmup
    bool UNUSED _ret;
    for (int i = 0; i < 100; i++) {
        _ret = write(fd, buf, file_size);
    }

    // real measurement
    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        _ret = write(fd, buf, file_size);
        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    close(fd);
    remove(LE_FILE_PATH);
    free(diffs);
    free(buf);
    return ret;

err:
    ret = true;
    goto cleanup;
}
