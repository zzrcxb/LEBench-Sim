#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

bool poll_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    double *diffs = NULL;
    struct pollfd *pfds = NULL;

    size_t fd_count;
    switch (config->i_size) {
        case TEST: fd_count = 1; break;
        case SMALL: fd_count = 10; break;
        case MEDIUM: fd_count = 100; break;
        case LARGE: fd_count = 1000; break;
        default:
            assert(false && "Invalid input size");
    }

    int *fds = malloc(fd_count * sizeof(int));
    if (!fds) return true;
    memset(fds, -1, fd_count * sizeof(int));

    pfds = calloc(fd_count, sizeof(struct pollfd));
    diffs = init_diff_array(iter_cnt);
    if (!pfds || !diffs) goto err;

    // setup
    for (int i = 0; i < fd_count; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            fprintf(stderr, ZERROR"Socket %d: failed to get a valid fd\n", i);
            goto err;
        }
        pfds[i].fd = fd;
        pfds[i].events = POLLIN;
        fds[i] = fd;
    }

    // measure time
    roi_begin();
    for (size_t idy = 0; idy < iter_cnt; idy++) {
        start_timer(&tstart);
#ifdef AARCH64
        size_t _ret = ppoll(pfds, fd_count, NULL, NULL);
#else
        size_t _ret = poll(pfds, fd_count, 0);
#endif
        stop_timer(&tend);

        if (_ret != fd_count) {
            fprintf(stderr, "Failed to run poll\n");
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
    free(pfds);
    return ret;

err:
    ret = true;
    goto cleanup;
}
