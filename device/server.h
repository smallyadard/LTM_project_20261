#ifndef DEVICE_SERVER_H
#define DEVICE_SERVER_H
#include <arpa/inet.h>
#include "common/linebuf.h"

#define SERVER_MAX_CONNS 32

typedef struct {
    int       fd;                       /* -1 = o trong */
    char      ip[INET_ADDRSTRLEN];
    linebuf_t lb;                       /* du lieu doc do dang cua rieng ket noi nay */
} conn_t;

typedef struct server_s server_t;

/* Goi cho moi dong day du nhan duoc tu ket noi idx. Tra ve 0: giu ket noi, khac 0: dong ket noi. */
typedef int  (*server_line_cb)(server_t *s, int idx, const char *line, void *user);
/* Goi khi fd phu (vd socket UDP quet) co du lieu. */
typedef void (*server_aux_cb)(int fd, void *user);

struct server_s {
    int           listen_fd;
    int           port;                 /* cong TCP thuc te dang lang nghe */
    conn_t        conns[SERVER_MAX_CONNS];
    int           aux_fd;
    server_aux_cb aux_cb;
    void         *aux_user;
};

/* Mo cong lang nghe TCP (port 0 = he dieu hanh chon, doc lai o s->port). 0/-1 */
int  server_open(server_t *s, int port);

/* Dang ky them 1 fd vao cung vong poll (fd < 0 de bo). */
void server_set_aux(server_t *s, int fd, server_aux_cb cb, void *user);

/* Cho toi da timeout_ms, xu ly moi su kien san sang (accept / doc / aux). 0: ok, -1: loi poll. */
int  server_run_once(server_t *s, int timeout_ms, server_line_cb cb, void *user);

int  server_send(server_t *s, int idx, const char *line);   /* gui line + '\n' */
void server_close_conn(server_t *s, int idx);
int  server_conn_count(const server_t *s);
void server_close(server_t *s);

#endif
