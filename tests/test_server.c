#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "common/net.h"
#include "common/protocol.h"
#include "device/server.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static int echo_cb(server_t *s, int idx, const char *line, void *u) {
    (void)u;
    char out[600];
    snprintf(out, sizeof out, "ECHO|%s", line);
    server_send(s, idx, out);
    return 0;
}

static int dial(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) { close(fd); return -1; }
    return fd;
}

/* Chay server trong tien trinh cha cho den khi cac con xong. */
static int pump_until_children_done(server_t *s, int nchild, int *bad) {
    int done = 0; time_t t0 = time(NULL);
    while (done < nchild && time(NULL) - t0 < 8) {
        server_run_once(s, 30, echo_cb, NULL);
        int st; pid_t p;
        while ((p = waitpid(-1, &st, WNOHANG)) > 0) {
            done++;
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) (*bad)++;
        }
    }
    return done == nchild ? 0 : -1;
}

static void client_glued_split(int port, int k) {
    int fd = dial(port); if (fd < 0) _exit(2);
    char a[64], b[64], out[128]; linebuf_t lb; lb_init(&lb);
    snprintf(a, sizeof a, "HELLO|%d\nSECO", k);
    snprintf(b, sizeof b, "ND|%d\n", k);
    send_all(fd, a, strlen(a));
    { struct timespec ts = { 0, 100000000L }; nanosleep(&ts, NULL); }
    send_all(fd, b, strlen(b));
    snprintf(a, sizeof a, "ECHO|HELLO|%d", k);
    snprintf(b, sizeof b, "ECHO|SECOND|%d", k);
    if (recv_line(fd, &lb, out, sizeof out, 3000) != 1 || strcmp(out, a) != 0) _exit(3);
    if (recv_line(fd, &lb, out, sizeof out, 3000) != 1 || strcmp(out, b) != 0) _exit(4);
    close(fd); _exit(0);
}

static int t_multi_clients(void) {
    server_t s; CHECK(server_open(&s, 0) == 0 && s.port > 0);
    for (int k = 0; k < 3; k++) if (fork() == 0) client_glued_split(s.port, k);
    int bad = 0;
    CHECK(pump_until_children_done(&s, 3, &bad) == 0 && bad == 0);
    for (int i = 0; i < 10; i++) server_run_once(&s, 20, echo_cb, NULL);   /* nhan EOF */
    CHECK(server_conn_count(&s) == 0);
    server_close(&s);
    PASS("server: 3 concurrent clients, glued + split lines, slots freed on close"); return 0;
}

static int t_half_line_does_not_block(void) {
    server_t s; CHECK(server_open(&s, 0) == 0);
    int slow = dial(s.port), fast = dial(s.port);
    CHECK(slow >= 0 && fast >= 0);
    send_all(slow, "NEVER_FINISHED", 14);          /* khong co '\n' */
    send_line(fast, "PING");
    linebuf_t lb; lb_init(&lb); char out[64]; int got = 0;
    for (int i = 0; i < 20 && !got; i++) {
        server_run_once(&s, 20, echo_cb, NULL);
        if (recv_line(fast, &lb, out, sizeof out, 0) == 1) got = 1;
    }
    CHECK(got && strcmp(out, "ECHO|PING") == 0);
    close(slow); close(fast); server_close(&s);
    PASS("server: half-sent line from one client does not block others"); return 0;
}

static int t_line_too_long(void) {
    server_t s; CHECK(server_open(&s, 0) == 0);
    int c = dial(s.port); CHECK(c >= 0);
    char big[700]; memset(big, 'x', sizeof big);
    send_all(c, big, sizeof big);
    for (int i = 0; i < 10; i++) server_run_once(&s, 20, echo_cb, NULL);
    linebuf_t lb; lb_init(&lb); char out[64];
    CHECK(recv_line(c, &lb, out, sizeof out, 500) == 1 && strcmp(out, "ERR|LINE_TOO_LONG") == 0);
    int r = recv_line(c, &lb, out, sizeof out, 500);   /* dong hoac RST do con du lieu chua doc */
    CHECK(r == NET_CLOSED || r == NET_ERR);
    CHECK(server_conn_count(&s) == 0);
    close(c); server_close(&s);
    PASS("server: overlong line gets ERR|LINE_TOO_LONG and connection closed"); return 0;
}

static int t_busy(void) {
    server_t s; CHECK(server_open(&s, 0) == 0);
    int fds[SERVER_MAX_CONNS + 1];
    for (int i = 0; i <= SERVER_MAX_CONNS; i++) {
        fds[i] = dial(s.port); CHECK(fds[i] >= 0);
        server_run_once(&s, 20, echo_cb, NULL);
    }
    CHECK(server_conn_count(&s) == SERVER_MAX_CONNS);
    linebuf_t lb; lb_init(&lb); char out[64];
    CHECK(recv_line(fds[SERVER_MAX_CONNS], &lb, out, sizeof out, 500) == 1 && strcmp(out, "ERR|BUSY") == 0);
    for (int i = 0; i <= SERVER_MAX_CONNS; i++) close(fds[i]);
    server_close(&s);
    PASS("server: connection beyond limit gets ERR|BUSY"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_multi_clients, t_half_line_does_not_block, t_line_too_long, t_busy };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
