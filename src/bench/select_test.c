#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

void select_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    size_t fd_count;
    switch (config->i_size) {
        case TEST: fd_count = 1; break;
        case SMALL: fd_count = 10; break;
        case MEDIUM: fd_count = 100; break;
        case LARGE: fd_count = 1000; break;
        default:
            assert(false && "Invalid input size");
    }

    int fds[fd_count], max_fd;
    fd_set monitored;
    FD_ZERO(&monitored);

    // setup fd
    for (size_t idx = 0; idx < fd_count; idx++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) {
            FD_SET(fd, &monitored);
            fds[idx] = fd;
            max_fd = fd > max_fd ? fd : max_fd;
        } else {
            fprintf(stderr, ZERROR"Failed to get a valid fd\n");
            res->errored = true;
            return;
        }
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // measure time
    bool err = false;
    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);

        err |= syscall(SYS_select, max_fd + 1, &monitored,
                       NULL, NULL, &tv) != fd_count;

        stop_timer(&tend);
        get_duration(diffs[idy], &tstart, &tend);
        usleep(TEST_INTERVAL);
    }
    roi_end();

    if (err) {
        fprintf(stderr, ZERROR"Failed to execute select\n");
        res->errored = true;
        free(diffs);
        return;
    }

    for (size_t idx = 0; idx < fd_count; idx++) {
        int UNUSED _ret = close(fds[idx]);
    }

    // data processing
    collect_results(diffs, iter_cnt, config, res);

    // clean up
    free(diffs);
    return;
}
