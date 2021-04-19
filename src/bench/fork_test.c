#define _GNU_SOURCE

#include <utils.h>
#include <le_bench.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/resource.h>


void fork_test(BenchConfig *config, BenchResult *res) {
    size_t iter_cnt = config->iter, page_cnt = 0;
    TimeType tstart, t_parent_end;

    switch (config->i_size) {
        case TEST:   iter_cnt /= 50; break;
        case SMALL:  break;
        case MEDIUM: iter_cnt /= 5; page_cnt = 6000; break;
        case LARGE:  iter_cnt /= 10; page_cnt = 12000; break;
        default: assert(false);
    }

    double *child_diffs = (double*)malloc(iter_cnt * sizeof(double));
    double *parent_diffs = (double*)malloc(iter_cnt * sizeof(double));

    TimeType *t_child_end = mmap(NULL, sizeof(TimeType), PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // setup pages
    void **pages = NULL;
    if (page_cnt > 0) {
        pages = malloc(page_cnt * sizeof(void*));
        for (size_t i = 0; i < page_cnt; i++) {
            pages[i] = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        }
    }

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        int pid = fork();
        if (pid == 0) {
            // child process
            stop_timer(t_child_end);
            kill(getpid(), SIGINT);
            fprintf(stderr, ZERROR"Failed to kill child process;\n");
            return;
        } else if (pid > 0) {
            stop_timer(&t_parent_end);
            int status;
            wait(&status);
        } else {
            fprintf(stderr, ZERROR"Failed to fork;\n");
            res->errored = true;
            return;
        }
        get_duration(parent_diffs[idx], &tstart, &t_parent_end);
        get_duration(child_diffs[idx], &tstart, t_child_end);
    }
    roi_end();

    // process results
    collect_results(parent_diffs, iter_cnt, config, res);
    res->child = (BenchResult*)malloc(sizeof(BenchResult));
    collect_results(child_diffs, iter_cnt, config, res->child);

    // clean up
    if (page_cnt > 0) {
        assert(pages != NULL);
        for (size_t i = 0; i < page_cnt; i++) {
            munmap(pages[i], getpagesize());
        }
    }

    munmap(t_child_end, sizeof(TimeType));
    free(child_diffs);
    free(parent_diffs);
    return;
}
