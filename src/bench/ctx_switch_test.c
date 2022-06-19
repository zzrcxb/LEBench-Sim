#define _GNU_SOURCE

#include <le_bench.h>
#include <errno.h>
#include <utils.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>


bool set_affinity_priority(uint core, int prio) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    bool ret1 = sched_setaffinity(getpid(), sizeof(set), &set) != 0;
    bool ret2 = setpriority(PRIO_PROCESS, 0, prio) != 0;
    return ret1 || ret2;
}

bool save_affinity_priority(bool restore) {
    static bool saved = false;
    static cpu_set_t cpuset;
    static int prio;

    bool ret;
    if (!restore) {
        // save it
        errno = 0;
        prio = getpriority(PRIO_PROCESS, 0);
        ret = errno != 0;

        ret |= sched_getaffinity(getpid(), sizeof(cpuset), &cpuset) != 0;
        saved = !ret;
        return ret;
    } else if (saved && restore) {
        // restore it
        ret = setpriority(PRIO_PROCESS, 0, prio) != 0;
        ret |= sched_setaffinity(getpid(), sizeof(cpuset), &cpuset) != 0;
        return ret;
    } else {
        // it should never happen
        assert(false);
    }
}

bool context_switch_test(BenchConfig *config, BenchResult *res) {
    bool ret = false; // has error?
    size_t iter_cnt = config->iter;
    TimeType tstart, tend;
    int fds1[2] = {-1, -1}, fds2[2] = {-1, -1};

    // backup affinity
    if (save_affinity_priority(false)) {
        fprintf(stderr, ZERROR"Failed to save affinity & priority;\n");
        return true;
    }

    double *diffs = init_diff_array(iter_cnt);
    if (!diffs) goto err;

    // setup pipes
    if (pipe(fds1) || pipe(fds2)) {
        fprintf(stderr, ZERROR"Failed to create pipes\n");
        goto err;
    }

    UNUSED size_t _unused;
    char reader, writer = 'a';
    int pid = fork();
    if (pid > 0) {
        if (set_affinity_priority(0, -20)) {
            fprintf(stderr, ZERROR"Failed to set affinity & priority;\n");
            goto err;
        }

        _unused = read(fds2[0], &reader, 1);
        roi_begin();
        for (size_t idx = 0; idx < iter_cnt; idx++) {
            start_timer(&tstart);
            _unused = write(fds1[1], &writer, 1);
            _unused = read(fds2[0], &reader, 1);
            stop_timer(&tend);
            get_duration(diffs[idx], &tstart, &tend);
        }
        roi_end();
        collect_results(diffs, iter_cnt, config, res);

        int status;
        wait(&status); // wait for the child
    } else if (pid == 0) {
        if (set_affinity_priority(0, -20)) {
            fprintf(stderr, ZERROR"Failed to set affinity & priority;\n");
            goto err;
        }

        _unused = write(fds2[1], &writer, 1);
        for (size_t idx = 0; idx < iter_cnt; idx++) {
            _unused = read(fds1[0], &reader, 1);
            _unused = write(fds2[1], &writer, 1);
        }

        kill(getpid(), SIGINT);
        fprintf(stderr, ZERROR"Failed to kill child process;\n");
        goto err;
    } else {
        fprintf(stderr, ZERROR"Failed to fork;\n");
        goto err;
    }

cleanup:
    if (save_affinity_priority(true)) {
        fprintf(stderr, ZERROR"Failed to restore affinity & priority;\n");
    }
    close(fds1[0]); close(fds1[1]);
    close(fds2[0]); close(fds2[1]);
    free(diffs);
    return ret;

err:
    ret = true;
    goto cleanup;
}
