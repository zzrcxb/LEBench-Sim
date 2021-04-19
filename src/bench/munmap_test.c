#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#define FILE_PATH "/tmp/LEBench_WR"
void munmap_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;

    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    size_t file_size;
    switch (config->i_size) {
        case TEST: file_size = 4096; break;
        case SMALL: file_size = 4096; break;
        case MEDIUM: file_size = 40960; break;
        case LARGE: file_size = 4096000; break;
        default: assert(false && "Invalid input size");
    }

    remove(FILE_PATH);

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        int fd = open(FILE_PATH, O_RDWR | O_CREAT, 0664);
        if (fd < 0) {
            fprintf(stderr, ZERROR"Failed to read file at %s\n", FILE_PATH);
            res->errored = true;
            free(diffs);
            return;
        }

        void *addr = (void *)syscall(SYS_mmap, NULL, file_size, PROT_WRITE,
                                     MAP_PRIVATE, fd, 0);
        for (size_t i; i < file_size; i++) {
            ((char*)addr)[i] = 'a';
        }

        start_timer(&tstart);
        syscall(SYS_munmap, addr, file_size);
        stop_timer(&tend);
        get_duration(diffs[idx], &tstart, &tend);
        usleep(TEST_INTERVAL);
        close(fd);
    }
    roi_end();

    collect_results(diffs, iter_cnt, config, res);
    remove(FILE_PATH);
    free(diffs);

    return;
}
