#ifndef NET_H
#define NET_H
#include <stddef.h>
#include <sys/types.h>
#include "common/linebuf.h"

#define NET_CLOSED    0
#define NET_ERR      (-1)
#define NET_TIMEOUT  (-2)
#define NET_TOO_LONG (-3)

/* Gui du n byte (lap den khi het). 0: ok, -1: loi. */
int send_all(int fd, const char *buf, size_t n);

/* Doc toi da n byte, cho toi da timeout_ms (am = cho vo han).
 * >0: so byte, 0: doi phuong dong ket noi, -1: loi, -2: timeout. */
ssize_t recv_some(int fd, char *buf, size_t n, int timeout_ms);

/* Gui line + '\n'. 0: ok, -1: loi. */
int send_line(int fd, const char *line);

/* Doc tron 1 dong vao out (da bo '\n'), dung lb de giu phan du giua cac lan goi.
 * 1: co dong, 0: dong ket noi, -1: loi, -2: timeout, -3: dong qua dai. */
int recv_line(int fd, linebuf_t *lb, char *out, size_t out_size, int timeout_ms);

#endif
