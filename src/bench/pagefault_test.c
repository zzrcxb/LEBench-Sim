#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>


bool page_fault_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    int fd = -1;
    size_t iter_cnt = config->iter;
    double *diffs = init_diff_array(iter_cnt);

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

    char _a = 0;
    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        void *addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        start_timer(&tstart);
        _a += *((char *)addr);
        stop_timer(&tend);
        munmap(addr, file_size);
        get_duration(diffs[idx], &tstart, &tend);
    }
    roi_end();
    collect_results(diffs, iter_cnt, config, res);

    FILE *dev_null = fopen("/dev/null", "w");
    if (dev_null) {
        fprintf(dev_null, "%c\n", _a); // prevent compiler opt.
        fclose(dev_null);
    }

cleanup:
    close(fd);
    free(diffs);
    remove(LE_FILE_PATH);
    return ret;

err:
    ret = true;
    goto cleanup;
}
