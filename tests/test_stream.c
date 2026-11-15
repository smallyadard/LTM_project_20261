#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common/linebuf.h"
#include "common/message.h"
#include "common/net.h"

static int passed = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static void pair(int sv[2]) { assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0); }

static int t_lb_two_lines_glued(void) {
    linebuf_t lb; char out[600]; lb_init(&lb);
    const char *d = "SCAN\nCONNECT|app1|pw\n";
    CHECK(lb_append(&lb, d, strlen(d)) == 0);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "SCAN") == 0);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "CONNECT|app1|pw") == 0);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 0);
    PASS("linebuf: two lines in one chunk"); return 0;
}

static int t_lb_split_line(void) {
    linebuf_t lb; char out[600]; lb_init(&lb);
    lb_append(&lb, "CONNE", 5);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 0);
    lb_append(&lb, "CT|a|b\nNEXT", 11);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "CONNECT|a|b") == 0);
    CHECK(lb.len == 4);   /* "NEXT" duoc giu lai */
    PASS("linebuf: line split across chunks, remainder kept"); return 0;
}

static int t_lb_crlf_and_empty(void) {
    linebuf_t lb; char out[600]; lb_init(&lb);
    lb_append(&lb, "A\r\n\nB\n", 6);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "A") == 0);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "") == 0);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "B") == 0);
    PASS("linebuf: CRLF and empty line"); return 0;
}

static int t_lb_too_long(void) {
    linebuf_t lb; char out[600], big[LINE_MAX_LEN + 50]; lb_init(&lb);
    memset(big, 'x', sizeof big);
    lb_append(&lb, big, sizeof big);                      /* chua co '\n' */
    CHECK(lb_extract_line(&lb, out, sizeof out) == -1);
    CHECK(lb.len == 0);
    lb_append(&lb, big, 520); lb_append(&lb, "\nOK\n", 4); /* co '\n' nhung qua dai */
    CHECK(lb_extract_line(&lb, out, sizeof out) == -1);
    CHECK(lb_extract_line(&lb, out, sizeof out) == 1 && strcmp(out, "OK") == 0);
    CHECK(lb_append(&lb, big, LB_CAP + 1) == -1);
    PASS("linebuf: too long line rejected, next line still works"); return 0;
}

static int t_send_all_large(void) {
    int sv[2]; pair(sv);
    char buf[60000], rx[60000]; memset(buf, 'a', sizeof buf);
    CHECK(send_all(sv[0], buf, sizeof buf) == 0);
    size_t got = 0;
    while (got < sizeof rx) {
        ssize_t n = recv_some(sv[1], rx + got, sizeof rx - got, 1000);
        CHECK(n > 0); got += (size_t)n;
    }
    CHECK(memcmp(buf, rx, sizeof buf) == 0);
    close(sv[0]); close(sv[1]);
    PASS("send_all: large buffer delivered completely"); return 0;
}

static int t_recv_timeout_and_close(void) {
    int sv[2]; pair(sv); char b[8];
    CHECK(recv_some(sv[1], b, sizeof b, 50) == NET_TIMEOUT);
    close(sv[0]);
    CHECK(recv_some(sv[1], b, sizeof b, 50) == 0);
    close(sv[1]);
    PASS("recv_some: timeout and peer closed"); return 0;
}

static int t_recv_line_glued_and_split(void) {
    int sv[2]; pair(sv); linebuf_t lb; char out[600]; lb_init(&lb);
    send_all(sv[0], "SCAN\nGET_INFO|tok\nSET_P", 23);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == 1 && strcmp(out, "SCAN") == 0);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == 1 && strcmp(out, "GET_INFO|tok") == 0);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 50) == NET_TIMEOUT);   /* dong dang do */
    send_all(sv[0], "UMP|tok|10|2\n", 13);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == 1 && strcmp(out, "SET_PUMP|tok|10|2") == 0);
    close(sv[0]); close(sv[1]);
    PASS("recv_line: glued lines and line split over two sends"); return 0;
}

static int t_recv_line_closed_midway(void) {
    int sv[2]; pair(sv); linebuf_t lb; char out[600]; lb_init(&lb);
    send_all(sv[0], "HALF", 4); close(sv[0]);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == NET_CLOSED);
    close(sv[1]);
    PASS("recv_line: connection closed mid-line"); return 0;
}

static int t_recv_line_too_long(void) {
    int sv[2]; pair(sv); linebuf_t lb; char out[600], big[700]; lb_init(&lb);
    memset(big, 'x', sizeof big);
    send_all(sv[0], big, sizeof big);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == NET_TOO_LONG);
    close(sv[0]); close(sv[1]);
    PASS("recv_line: overlong line"); return 0;
}

static int t_send_line(void) {
    int sv[2]; pair(sv); linebuf_t lb; char out[600]; lb_init(&lb);
    CHECK(send_line(sv[0], "OK|abc") == 0);
    CHECK(recv_line(sv[1], &lb, out, sizeof out, 500) == 1 && strcmp(out, "OK|abc") == 0);
    close(sv[0]); close(sv[1]);
    PASS("send_line + recv_line round trip"); return 0;
}

static int t_msg_parse(void) {
    message_t m;
    CHECK(msg_parse("CONNECT|app1|secret", &m) == 0);
    CHECK(strcmp(m.cmd, "CONNECT") == 0 && m.argc == 2);
    CHECK(strcmp(m.args[0], "app1") == 0 && strcmp(m.args[1], "secret") == 0);
    CHECK(msg_parse("SCAN", &m) == 0 && m.argc == 0);
    CHECK(msg_parse("X|a||c", &m) == 0 && m.argc == 3 && strcmp(m.args[1], "") == 0);
    CHECK(msg_parse("", &m) == -1);
    CHECK(msg_parse("|a", &m) == -1);
    PASS("message: parse valid and invalid"); return 0;
}

static int t_msg_limits(void) {
    message_t m; char line[1024], c[200];
    memset(c, 'y', 199); c[199] = '\0';
    snprintf(line, sizeof line, "CMD|%s", c);
    CHECK(msg_parse(line, &m) == -1);                       /* arg qua dai */
    strcpy(line, "C");
    for (int i = 0; i < 17; i++) strcat(line, "|a");
    CHECK(msg_parse(line, &m) == -1);                       /* 17 args */
    PASS("message: length and argument limits"); return 0;
}

static int t_msg_build(void) {
    char out[64]; const char *a[] = { "app1", "pw" };
    CHECK(msg_build(out, sizeof out, "CONNECT", 2, a) == 15);
    CHECK(strcmp(out, "CONNECT|app1|pw") == 0);
    CHECK(msg_build(out, sizeof out, "SCAN", 0, NULL) == 4 && strcmp(out, "SCAN") == 0);
    const char *bad[] = { "a|b" };
    CHECK(msg_build(out, sizeof out, "X", 1, bad) == -1);
    CHECK(msg_build(out, 8, "CONNECT", 2, a) == -1);        /* out qua nho */
    message_t m;                                            /* round trip */
    msg_build(out, sizeof out, "SET_PUMP", 2, (const char *[]){ "10", "2" });
    CHECK(msg_parse(out, &m) == 0 && m.argc == 2 && strcmp(m.args[1], "2") == 0);
    PASS("message: build, reject '|', small buffer, round trip"); return 0;
}

int main(void) {
    int (*tests[])(void) = {
        t_lb_two_lines_glued, t_lb_split_line, t_lb_crlf_and_empty, t_lb_too_long,
        t_send_all_large, t_recv_timeout_and_close, t_recv_line_glued_and_split,
        t_recv_line_closed_midway, t_recv_line_too_long, t_send_line,
        t_msg_parse, t_msg_limits, t_msg_build,
    };
    int n = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < n; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, n);
    return 0;
}
