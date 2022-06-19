#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

bool select_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;

    size_t fd_count;
    switch (config->i_size) {
        case TEST: fd_count = 1; break;
        case SMALL: fd_count = 10; break;
        case MEDIUM: fd_count = 100; break;
        case LARGE: fd_count = 1000; break;
        default:
            assert(false && "Invalid input size");
    }
    int *fds = malloc(fd_count * sizeof(int)), max_fd;
    if (!fds) return true;
    memset(fds, -1, fd_count * sizeof(int));

    double *diffs = init_diff_array(iter_cnt);
    if (!diffs) goto err;

    // setup fd
    fd_set monitored;
    FD_ZERO(&monitored);
    for (size_t idx = 0; idx < fd_count; idx++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) {
            FD_SET(fd, &monitored);
            fds[idx] = fd;
            max_fd = fd > max_fd ? fd : max_fd;
        } else {
            fprintf(stderr, ZERROR "Failed to get a valid fd\n");
            goto err;
        }
    }

    struct timeval tv = {0, 0};
    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);
#ifdef AARCH64
        int _ret = pselect6(max_fd + 1, &monitored, NULL, NULL, &tv, NULL);
#else
        int _ret = select(max_fd + 1, &monitored, NULL, NULL, &tv);
#endif
        stop_timer(&tend);
        if (_ret == -1) {
            fprintf(stderr, "Failed to select!\n");
            goto err;
        }
        get_duration(diffs[idy], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    for (size_t idx = 0; idx < fd_count; idx++) {
        int UNUSED _ret = close(fds[idx]);
    }
    free(fds);
    free(diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
