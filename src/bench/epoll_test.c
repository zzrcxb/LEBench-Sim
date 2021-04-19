#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

void epoll_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    size_t fd_count;
    switch (config->i_size) {
        case TEST:
            fd_count = 1;
            break;
        case SMALL:
            fd_count = 10;
            break;
        case MEDIUM:
            fd_count = 100;
            break;
        case LARGE:
            fd_count = 1000;
            break;
        default:
            assert(false && "Invalid input size");
    }

    int fds[fd_count];
    int epfd = epoll_create(fd_count);

    // setup
    for (int i = 0; i < fd_count; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            fprintf(stderr, ZERROR"Failed to get a valid fd\n");
            res->errored = true;
            free(diffs);
            return;
        }

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = fd;

        int _ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
        if (_ret == -1) {
            fprintf(stderr, ZERROR"Failed to execute epoll_ctl\n");
            res->errored = true;
            free(diffs);
            return;
        }

        fds[i] = fd;
    }

    // measure time
    bool err = false;
    struct epoll_event *events =
        (struct epoll_event *)malloc(fd_count * sizeof(struct epoll_event));
    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);

        err |= epoll_wait(epfd, events, fd_count, 0) != fd_count;

        stop_timer(&tend);
        get_duration(diffs[idy], &tstart, &tend);
        usleep(TEST_INTERVAL);
    }
    roi_end();

    if (err) {
        fprintf(stderr, ZERROR"Failed to execute epoll\n");
        res->errored = true;
        free(events);
        free(diffs);
        return;
    }

    close(epfd);
    for (size_t idx = 0; idx < fd_count; idx++) {
        int UNUSED _ret = close(fds[idx]);
    }

    // data processing
    collect_results(diffs, iter_cnt, config, res);

    // clean up
    free(events);
    free(diffs);
    return;
}
