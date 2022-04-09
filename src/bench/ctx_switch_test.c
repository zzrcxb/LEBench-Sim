#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>


inline bool set_affinity_priority(uint core, int prio) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    bool ret1 = sched_setaffinity(getpid(), sizeof(set), &set) != -1;
    bool ret2 = setpriority(PRIO_PROCESS, 0, prio) != -1;
    return ret1 && ret2;
}

bool save_affinity_priority(bool restore) {
    static bool saved = false;
    static cpu_set_t cpuset;
    static int prio;

    if (!restore) {
        // save it
        bool ret = sched_getaffinity(getpid(), sizeof(cpuset), &cpuset) != -1;
        prio = getpriority(PRIO_PROCESS, 0);
        saved = (prio != -1) && ret;
        return saved;
    } else if (saved && restore) {
        // restore it
        bool ret1 = sched_setaffinity(getpid(), sizeof(cpuset), &cpuset) != -1;
        bool ret2 = setpriority(PRIO_PROCESS, 0, prio) != -1;
        saved = false;
        return ret1 && ret2;
    } else {
        // it should never happen
        assert(false);
    }
}

void context_switch_test(BenchConfig *config, BenchResult *res) {
    size_t iter_cnt = config->iter;
    TimeType tstart, tend;
    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    // setup pipes
    int fds1[2], fds2[2];
    int r1 = pipe(fds1);
    int r2 = pipe(fds2);
    if (r1 || r2) {
        fprintf(stderr, ZERROR"Failed to create pipes; r1: %d, r2: %d\n", r1, r2);
        res->errored = true;
        free(diffs);
        return;
    }

    // backup affinity
    if (!save_affinity_priority(false)) {
        fprintf(stderr, ZERROR"Failed to save affinity & priority;\n");
        res->errored = true;
        free(diffs);
        return;
    }

    size_t __attribute__((unused)) _unused;
    char reader, writer = 'a';
    int fork_id = fork();
    if (fork_id > 0) {
        r1 = close(fds1[0]);
        r2 = close(fds2[1]);
        if (r1 || r2) {
            fprintf(stderr, ZERROR"Failed to close pipes; r1: %d, r2: %d\n", r1, r2);
            res->errored = true;
            free(diffs);
            return;
        }

        if (!set_affinity_priority(0, -20)) {
            fprintf(stderr, ZERROR"Failed to set affinity & priority;\n");
            res->errored = true;
            free(diffs);
            return;
        }

        _unused = read(fds2[0], &reader, 1);
        roi_begin();
        for (size_t idx = 0; idx < config->iter; idx++) {
            start_timer(&tstart);

            _unused = write(fds1[1], &writer, 1);
            _unused = read(fds2[0], &reader, 1);

            stop_timer(&tend);
            get_duration(diffs[idx], &tstart, &tend);
        }
        roi_end();

        int status;
        wait(&status);

        close(fds1[1]);
        close(fds2[0]);
    } else if (fork_id == 0) {
        r1 = close(fds1[1]);
        r2 = close(fds2[0]);
        if (r1 || r2) {
            fprintf(stderr, ZERROR"Failed to close pipes; r1: %d, r2: %d\n", r1, r2);
            res->errored = true;
            return;
        }

        if (!set_affinity_priority(0, -20)) {
            fprintf(stderr, ZERROR"Failed to set affinity & priority;\n");
            res->errored = true;
            return;
        }

        _unused = write(fds2[1], &writer, 1);
        for (size_t idx = 0; idx < config->iter; idx++) {
            _unused = read(fds1[0], &reader, 1);
            _unused = write(fds2[1], &writer, 1);
        }

        usleep(TEST_INTERVAL);
        kill(getpid(), SIGINT);
        fprintf(stderr, ZERROR"Failed to kill child process;\n");
        return;
    } else {
        fprintf(stderr, ZERROR"Failed to fork;\n");
        res->errored = true;
        free(diffs);
        return;
    }

    if (!save_affinity_priority(true)) {
        fprintf(stderr, ZERROR"Failed to restore affinity & priority;\n");
        res->errored = true;
        free(diffs);
        return;
    }

    // process resultss
    collect_results(diffs, iter_cnt, config, res);

    // clean up
    free(diffs);
    return;
}
