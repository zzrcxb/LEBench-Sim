#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>


#define FILE_PATH "/tmp/LEBench_WR"
void page_fault_test(BenchConfig *config, BenchResult *res) {
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

    char *buf = (char*)malloc(sizeof(char) * file_size);
    memset(buf, 'a', file_size);
    buf[file_size - 1] = '\0';

    remove(FILE_PATH);
    FILE *fp = fopen(FILE_PATH, "w");
    if (fp == NULL) {
        fprintf(stderr, ZERROR"Failed to create file at %s\n", FILE_PATH);
        res->errored = true;
        free(buf);
        free(diffs);
        return;
    }
    fprintf(fp, "%s", buf);
    fclose(fp);

    int fd = open(FILE_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, ZERROR"Failed to read file at %s\n", FILE_PATH);
        res->errored = true;
        free(buf);
        free(diffs);
        return;
    }

    // real measurement
    FILE *dev_null = fopen("/dev/null", "w");
    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        void *addr = (void*)syscall(SYS_mmap, NULL, file_size, PROT_READ,
                                    MAP_PRIVATE, fd, 0);
        start_timer(&tstart);

        char _a = *((char *)addr);

        stop_timer(&tend);
        fprintf(dev_null, "%c\n", _a);
        munmap(addr, file_size);
        get_duration(diffs[idx], &tstart, &tend);
        usleep(TEST_INTERVAL);
    }
    roi_end();

    collect_results(diffs, iter_cnt, config, res);

    close(fd);
    remove(FILE_PATH);
    free(buf);
    free(diffs);

    return;
}
