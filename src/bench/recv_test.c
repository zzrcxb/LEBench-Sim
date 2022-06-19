#define _GNU_SOURCE

#include <errno.h>
#include <le_bench.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utils.h>


bool recv_test(BenchConfig *config, BenchResult *res) {
    bool ret = false;
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;
    int fds1[2] = {-1, -1}, fds2[2] = {-1, -1};
    int fd_server = -1, fd_connect = -1, fd_client = -1;
    double *diffs = NULL;
    char *buf = NULL;

    diffs = init_diff_array(iter_cnt);
    if (!diffs) goto err;

    size_t msg_size;
    switch (config->i_size) {
        case TEST: msg_size = 1; break;
        case SMALL: msg_size = 1; break;
        case MEDIUM: msg_size = 4800; break;
        case LARGE: msg_size = 96000; break;
        default: assert(false && "Invalid input size");
    }
    buf = calloc(msg_size, sizeof(char));
    if (!buf) goto err;

    // setup pipes
    if (pipe(fds1) || pipe(fds2)) {
        fprintf(stderr, ZERROR "Failed to create pipes!\n");
        goto err;
    }

    // setup socket address
    remove(LE_SOCK_PATH);
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, LE_SOCK_PATH,
            sizeof(server_addr.sun_path) - 1);

    // fork
    int UNUSED _unused;
    char reader, writer = 'a';
    int pid = fork();
    if (pid > 0) {
        bool errored = true;
        int fd_server = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_server < 0) {
            fprintf(stderr, ZERROR "Failed to open server\n");
            goto server_err;
        }

        if (bind(fd_server, (struct sockaddr *)&server_addr,
                 sizeof(struct sockaddr_un)) == -1) {
            fprintf(stderr, ZERROR "Failed to bind\n");
            goto server_err;
        }

        if (listen(fd_server, 10) == -1) {
            fprintf(stderr, ZERROR "Failed to listen\n");
            goto server_err;
        }

        _unused = write(fds1[1], &writer, 1);
        int fd_connect =
            accept(fd_server, (struct sockaddr *)0, (socklen_t *)0);
        if (fd_connect < -1) {
            fprintf(stderr, ZERROR "Failed to accept\n");
            goto server_err;
        }

        roi_begin();
        for (size_t idx = 0; idx < iter_cnt; idx++) {
            _unused = read(fds2[0], &reader, 1);
            start_timer(&tstart);
            int64_t _size = recv(fd_connect, buf, msg_size, MSG_DONTWAIT);
            stop_timer(&tend);
            if (_size != msg_size) {
                fprintf(stderr, ZERROR "Failed to receive!\n");
                goto server_err;
            }
            get_duration(diffs[idx], &tstart, &tend);
            _unused = write(fds1[1], &writer, 1);
        }
        roi_end();
        errored = false;
        int status;
        wait(&status);
    server_err:
        if (errored) {
            kill(pid, SIGINT);
            goto err;
        }
    } else if (pid == 0) {
        _unused = read(fds1[0], &reader, 1);
        int fd_client = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_client < 0) {
            fprintf(stderr, ZERROR "Failed to open client\n");
            goto client_err;
        }

        if (connect(fd_client, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr_un)) == -1) {
            fprintf(stderr, ZERROR "Failed to connect\n");
            goto client_err;
        }

        memset(buf, 'a', msg_size - 1);
        for (size_t idx = 0; idx < iter_cnt;) {
            int64_t _size = send(fd_client, buf, msg_size, MSG_DONTWAIT);
            if (_size != msg_size && errno == EAGAIN) {
                continue;
            }
            _unused = write(fds2[1], &writer, 1);
            _unused = read(fds1[0], &reader, 1);
            idx++;
        }
    client_err:
        kill(getpid(), SIGINT);
        fprintf(stderr, ZERROR "Failed to kill child process;\n");
        return ret;
    } else {
        fprintf(stderr, ZERROR "Failed to fork!\n");
        goto err;
    }
    collect_results(diffs, iter_cnt, config, res);

cleanup:
    close(fds1[0]); close(fds1[1]);
    close(fds2[0]); close(fds2[1]);
    close(fd_client); close(fd_connect); close(fd_server);
    free(diffs); free(buf);
    remove(LE_SOCK_PATH);
    return ret;

err:
    ret = true;
    goto cleanup;
}
