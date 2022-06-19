#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

bool epoll_test(BenchConfig *config, BenchResult *res) {
    bool ret = false; // has error?
    TimeType tstart, tend;
    int epfd = -1;
    struct epoll_event *events = NULL;
    double *diffs = NULL;
    size_t iter_cnt = config->iter;

    size_t fd_count;
    switch (config->i_size) {
        case TEST: fd_count = 1; break;
        case SMALL: fd_count = 10; break;
        case MEDIUM: fd_count = 100; break;
        case LARGE: fd_count = 1000; break;
        default: assert(false && "Invalid input size");
    }

    int *fds = malloc(fd_count * sizeof(int));
    if (!fds) return true;
    memset(fds, -1, fd_count * sizeof(int));

    epfd = epoll_create(fd_count);
    if (epfd < 0) {
        fprintf(stderr, ZERROR "Failed to create epoll fd!\n");
        goto err;
    }

    events = (struct epoll_event *)malloc(fd_count * sizeof(struct epoll_event));
    diffs = init_diff_array(iter_cnt);
    if (!diffs || !events) goto err;

    // setup
    for (int i = 0; i < fd_count; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            fprintf(stderr, ZERROR"Socket %d: failed to get a valid fd\n", i);
            goto err;
        }
        fds[i] = fd;

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) != 0) {
            fprintf(stderr, ZERROR"Socket %d: Failed to run epoll_ctl\n", i);
            goto err;
        }
    }

    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);
        int _ret = epoll_wait(epfd, events, fd_count, 0);
        stop_timer(&tend);

        if (_ret != fd_count) {
            fprintf(stderr, "Failed to run epoll\n");
            goto err;
        }
        get_duration(diffs[idy], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    for (size_t idx = 0; idx < fd_count; idx++) {
        close(fds[idx]);
    }
    close(epfd);
    free(fds);
    free(diffs);
    free(events);
    return ret;

err:
    ret = true;
    goto cleanup;
}
