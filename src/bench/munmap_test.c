#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

bool munmap_test(BenchConfig *config, BenchResult *res) {
    bool ret = false; // has error?
    TimeType tstart, tend;
    int fd = -1;
    size_t iter_cnt = config->iter;
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
    fd = create_and_fill(LE_FILE_PATH, file_size, 'a');
    if (fd < 0) goto err;

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        void *addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            fprintf(stderr, ZERROR"Failed to mmap\n");
            goto err;
        }

        start_timer(&tstart);
        munmap(addr, file_size);
        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    close(fd);
    remove(LE_FILE_PATH);
    free(diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
