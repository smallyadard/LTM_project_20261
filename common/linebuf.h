#ifndef LINEBUF_H
#define LINEBUF_H
#include <stddef.h>

#define LINE_MAX_LEN 512   /* do dai toi da 1 dong (khong tinh '\n') */
#define LB_CAP       1024

typedef struct {
    char   data[LB_CAP];
    size_t len;
} linebuf_t;

void lb_init(linebuf_t *lb);

/* Them n byte vao buffer. 0: ok, -1: khong du cho. */
int lb_append(linebuf_t *lb, const char *data, size_t n);

/* Lay 1 dong day du (bo '\n' va '\r') vao out, giu phan du.
 * Tra ve: 1 co dong, 0 chua du dong, -1 dong qua dai (da bo dong do). */
int lb_extract_line(linebuf_t *lb, char *out, size_t out_size);

#endif
