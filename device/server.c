#include "device/server.h"
#include "common/net.h"
#include "common/protocol.h"
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int server_open(server_t *s, int port) {
    memset(s, 0, sizeof(*s));
    s->listen_fd = -1;
    s->aux_fd = -1;
    for (int i = 0; i < SERVER_MAX_CONNS; i++) s->conns[i].fd = -1;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons((unsigned short)port);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0 || listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    socklen_t len = sizeof a;
    if (getsockname(fd, (struct sockaddr *)&a, &len) < 0) {
        close(fd);
        return -1;
    }
    s->listen_fd = fd;
    s->port = ntohs(a.sin_port);
    return 0;
}

void server_set_aux(server_t *s, int fd, server_aux_cb cb, void *user) {
    s->aux_fd = fd;
    s->aux_cb = cb;
    s->aux_user = user;
}

int server_send(server_t *s, int idx, const char *line) {
    if (idx < 0 || idx >= SERVER_MAX_CONNS || s->conns[idx].fd < 0) return -1;
    return send_line(s->conns[idx].fd, line);
}

void server_close_conn(server_t *s, int idx) {
    if (idx < 0 || idx >= SERVER_MAX_CONNS || s->conns[idx].fd < 0) return;
    close(s->conns[idx].fd);
    s->conns[idx].fd = -1;
    lb_init(&s->conns[idx].lb);
}

int server_conn_count(const server_t *s) {
    int n = 0;
    for (int i = 0; i < SERVER_MAX_CONNS; i++) if (s->conns[i].fd >= 0) n++;
    return n;
}

void server_close(server_t *s) {
    for (int i = 0; i < SERVER_MAX_CONNS; i++) server_close_conn(s, i);
    if (s->listen_fd >= 0) close(s->listen_fd);
    s->listen_fd = -1;
}

static void accept_new(server_t *s) {
    struct sockaddr_in a;
    socklen_t len = sizeof a;
    int fd = accept(s->listen_fd, (struct sockaddr *)&a, &len);
    if (fd < 0) return;

    for (int i = 0; i < SERVER_MAX_CONNS; i++) {
        if (s->conns[i].fd < 0) {
            s->conns[i].fd = fd;
            lb_init(&s->conns[i].lb);
            inet_ntop(AF_INET, &a.sin_addr, s->conns[i].ip, sizeof s->conns[i].ip);
            return;
        }
    }
    send_line(fd, CMD_ERR "|" ERR_BUSY);   /* het cho: bao ban roi dong */
    close(fd);
}

static void handle_conn(server_t *s, int idx, server_line_cb cb, void *user) {
    conn_t *c = &s->conns[idx];
    char tmp[512];

    /* Khong dung recv_line o day: no se CHAN toi khi du 1 dong,
     * 1 client gui nua dong roi im lang se treo ca server. */
    ssize_t n = recv_some(c->fd, tmp, sizeof tmp, 0);
    if (n == NET_TIMEOUT) return;
    if (n <= 0) { server_close_conn(s, idx); return; }

    if (lb_append(&c->lb, tmp, (size_t)n) < 0) {
        server_send(s, idx, CMD_ERR "|" ERR_LINE_TOO_LONG);
        server_close_conn(s, idx);
        return;
    }

    char line[LINE_MAX_LEN + 1];
    for (;;) {
        int r = lb_extract_line(&c->lb, line, sizeof line);
        if (r == 0) break;
        if (r < 0) {
            server_send(s, idx, CMD_ERR "|" ERR_LINE_TOO_LONG);
            server_close_conn(s, idx);
            return;
        }
        if (cb(s, idx, line, user) != 0) {
            server_close_conn(s, idx);
            return;
        }
    }
}

int server_run_once(server_t *s, int timeout_ms, server_line_cb cb, void *user) {
    struct pollfd pf[2 + SERVER_MAX_CONNS];
    int map[2 + SERVER_MAX_CONNS];   /* -1 = listen, -2 = aux, >=0 = chi so ket noi */
    int n = 0;

    pf[n].fd = s->listen_fd; pf[n].events = POLLIN; map[n++] = -1;
    if (s->aux_fd >= 0) { pf[n].fd = s->aux_fd; pf[n].events = POLLIN; map[n++] = -2; }
    for (int i = 0; i < SERVER_MAX_CONNS; i++) {
        if (s->conns[i].fd >= 0) { pf[n].fd = s->conns[i].fd; pf[n].events = POLLIN; map[n++] = i; }
    }

    int r = poll(pf, (nfds_t)n, timeout_ms);
    if (r < 0) return (errno == EINTR) ? 0 : -1;
    if (r == 0) return 0;

    for (int k = 0; k < n; k++) {
        if (!pf[k].revents) continue;
        if (map[k] == -1)      accept_new(s);
        else if (map[k] == -2) { if (s->aux_cb) s->aux_cb(s->aux_fd, s->aux_user); }
        else                   handle_conn(s, map[k], cb, user);
    }
    return 0;
}
