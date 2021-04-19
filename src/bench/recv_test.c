#define _GNU_SOURCE

#include <le_bench.h>
#include <utils.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/syscall.h>


#define SOCK_PATH "/tmp/LE_socket"
void recv_test(BenchConfig *config, BenchResult *res) {
    TimeType tstart, tend;
    size_t iter_cnt = config->iter;

    double *diffs = (double*)malloc(iter_cnt * sizeof(double));

    size_t msg_size;
    switch (config->i_size) {
        case TEST: msg_size = 1; break;
        case SMALL: msg_size = 1; break;
        case MEDIUM: msg_size = 4800; break;
        case LARGE: msg_size = 96000; break;
        default: assert(false && "Invalid input size");
    }

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

    // setup socket address
    remove(SOCK_PATH);
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCK_PATH, sizeof(server_addr.sun_path) - 1);

    // fork
    int UNUSED _ret;
    char reader, writer = 'b';

    int fork_id = fork();
    if (fork_id > 0) {
        close(fds1[0]);
        close(fds2[1]);
        int fd_server = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_server < 0) {
            fprintf(stderr, ZERROR"Failed to open server\n");
            res->errored = true;
            free(diffs);
            return;
        }

        if (bind(fd_server, (struct sockaddr *) &server_addr,
                 sizeof(struct sockaddr_un)) == -1) {
            fprintf(stderr, ZERROR"Failed to bind\n");
            res->errored = true;
            free(diffs);
            return;
        }

        if (listen(fd_server, 10) == -1) {
            fprintf(stderr, ZERROR"Failed to listen\n");
            res->errored = true;
            free(diffs);
            return;
        }

        _ret = write(fds1[1], &writer, 1);

        int fd_connect = accept(fd_server, (struct sockaddr *)0, (socklen_t *)0);
        char *buf = (char *)malloc(sizeof(char) * msg_size);
        bool err = recv(fd_connect, buf, msg_size, MSG_DONTWAIT) == -1;
        roi_begin();
        for (size_t idx = 0; idx < iter_cnt; idx++) {
            start_timer(&tstart);

            err |= syscall(SYS_recvfrom, fd_connect, buf, msg_size,
                           MSG_DONTWAIT, NULL, NULL) == -1;

            stop_timer(&tend);
            get_duration(diffs[idx], &tstart, &tend);
            usleep(TEST_INTERVAL);
        }
        roi_end();

        _ret = read(fds2[0], &reader, 1);

        remove(SOCK_PATH);
        close(fd_server);
        close(fd_connect);
        close(fds1[1]);
        close(fds2[0]);
        free(buf);

        int status;
        wait(&status);
    } else {
        close(fds1[1]);
        close(fds2[0]);
        _ret = read(fds1[0], &reader, 1);

        int fd_client = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_client < 0) {
            fprintf(stderr, ZERROR"Failed to open client\n");
            _ret = write(fds2[1], &writer, 1);
            res->errored = true;
            free(diffs);
            return;
        }

        if (connect(fd_client, (struct sockaddr *) &server_addr,
                    sizeof(struct sockaddr_un)) == -1) {
            fprintf(stderr, ZERROR"Failed to connect\n");
            _ret = write(fds2[1], &writer, 1);
            res->errored = true;
            free(diffs);
            return;
        }

        char *buf = (char *)malloc(sizeof(char) * msg_size);
        for (int i = 0; i < msg_size; i++) {
            buf[i] = 'a';
        }

        bool err = send(fd_client, buf, msg_size, MSG_DONTWAIT) == -1;
        for (size_t idx = 0; idx < iter_cnt; idx++) {
            err |= syscall(SYS_sendto, fd_client, buf, msg_size,
                           MSG_DONTWAIT, NULL, 0) == -1;
        }

        _ret = write(fds2[1], &writer, 1);

        close(fd_client);
        close(fds1[0]);
        close(fds2[1]);
        free(buf);

        kill(getpid(), SIGINT);
        fprintf(stderr, ZERROR"Failed to kill child process;\n");
        return;
    }

    collect_results(diffs, iter_cnt, config, res);

    free(diffs);
    return;
}
