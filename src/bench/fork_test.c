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


bool fork_test(BenchConfig *config, BenchResult *res) {
    bool ret = false; // has error
    size_t iter_cnt = config->iter, page_cnt = 0;
    TimeType tstart, t_parent_end;
    double *child_diffs = NULL, *parent_diffs = NULL;
    TimeType *t_child_end = NULL;

    switch (config->i_size) {
        case TEST: break;
        case SMALL: break;
        case MEDIUM: page_cnt = 3000; break;
        case LARGE: page_cnt = 6000; break;
        default: assert(false);
    }

    // setup pages
    void **pages = NULL;
    if (page_cnt > 0) {
        pages = calloc(page_cnt, sizeof(void *));
        if (!pages) goto err;

        for (size_t i = 0; i < page_cnt; i++) {
            void *p = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (p == MAP_FAILED) goto err;
            pages[i] = p;
        }
    }

    child_diffs = init_diff_array(iter_cnt);
    parent_diffs = init_diff_array(iter_cnt);
    t_child_end = mmap(NULL, sizeof(TimeType), PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!child_diffs || !parent_diffs || t_child_end == MAP_FAILED) goto err;

    roi_begin();
    for (size_t idx = 0; idx < iter_cnt; idx++) {
        start_timer(&tstart);
        int pid = fork();
        if (pid == 0) {
            // child process
            stop_timer(t_child_end);
            kill(getpid(), SIGINT);
            fprintf(stderr, ZERROR "Failed to kill child process;\n");
            goto err;
        } else if (pid > 0) {
            stop_timer(&t_parent_end);
            int status;
            wait(&status);
        } else {
            fprintf(stderr, ZERROR "Failed to fork;\n");
            goto err;
        }
        get_duration(parent_diffs[idx], &tstart, &t_parent_end);
        get_duration(child_diffs[idx], &tstart, t_child_end);
    }
    roi_end();

    // process results
    collect_results(parent_diffs, iter_cnt, config, res);
    res->child = (BenchResult*)malloc(sizeof(BenchResult));
    collect_results(child_diffs, iter_cnt, config, res->child);

cleanup:
    if (page_cnt > 0 && pages) {
        for (size_t i = 0; i < page_cnt; i++) {
            munmap(pages[i], getpagesize());
        }
    }
    munmap(t_child_end, sizeof(TimeType));
    free(child_diffs);
    free(parent_diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
