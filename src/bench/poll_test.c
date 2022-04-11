#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

void poll_test(BenchConfig *config, BenchResult *res) {
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

    int fds[fd_count];
    struct pollfd pfds[fd_count];
    memset(pfds, 0, sizeof(pfds));

    // setup
    for (int i = 0; i < fd_count; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            fprintf(stderr, ZERROR"Failed to get a valid fd\n");
            res->errored = true;
            free(diffs);
            return;
        }

        pfds[i].fd = fd;
        pfds[i].events = POLLIN;
        fds[i] = fd;
    }

    // measure time
    bool err = false;
    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);

#ifdef AARCH64
        err |= syscall(SYS_ppoll, pfds, fd_count, NULL, NULL) != fd_count;
#else
        err |= syscall(SYS_poll, pfds, fd_count, 0) != fd_count;
#endif

        stop_timer(&tend);
        get_duration(diffs[idy], &tstart, &tend);
        usleep(TEST_INTERVAL);
    }
    roi_end();

    if (err) {
        fprintf(stderr, ZERROR"Failed to execute poll\n");
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
