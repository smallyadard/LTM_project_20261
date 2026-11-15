#include "common/net.h"
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int send_all(int fd, const char *buf, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        ssize_t r = send(fd, buf + sent, n - sent, MSG_NOSIGNAL);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)r;
    }
    return 0;
}

ssize_t recv_some(int fd, char *buf, size_t n, int timeout_ms) {
    struct pollfd p = { .fd = fd, .events = POLLIN };
    for (;;) {
        int pr = poll(&p, 1, timeout_ms);
        if (pr < 0) {
            if (errno == EINTR) continue;
            return NET_ERR;
        }
        if (pr == 0) return NET_TIMEOUT;
        break;
    }
    for (;;) {
        ssize_t r = recv(fd, buf, n, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return NET_ERR;
        }
        return r;   /* 0 = dong ket noi */
    }
}

int send_line(int fd, const char *line) {
    size_t n = strlen(line);
    if (n > LINE_MAX_LEN) return -1;
    if (send_all(fd, line, n) < 0) return -1;
    return send_all(fd, "\n", 1);
}

int recv_line(int fd, linebuf_t *lb, char *out, size_t out_size, int timeout_ms) {
    char tmp[256];
    for (;;) {
        int r = lb_extract_line(lb, out, out_size);
        if (r == 1)  return 1;
        if (r == -1) return NET_TOO_LONG;

        ssize_t n = recv_some(fd, tmp, sizeof(tmp), timeout_ms);
        if (n <= 0) return (int)n;   /* 0 dong, -1 loi, -2 timeout */
        if (lb_append(lb, tmp, (size_t)n) < 0) return NET_TOO_LONG;
    }
}
