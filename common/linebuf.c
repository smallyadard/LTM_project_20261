#include "common/linebuf.h"
#include <string.h>

void lb_init(linebuf_t *lb) {
    lb->len = 0;
}

int lb_append(linebuf_t *lb, const char *data, size_t n) {
    if (n > LB_CAP - lb->len) return -1;
    memcpy(lb->data + lb->len, data, n);
    lb->len += n;
    return 0;
}

int lb_extract_line(linebuf_t *lb, char *out, size_t out_size) {
    char *nl = memchr(lb->data, '\n', lb->len);
    if (!nl) {
        if (lb->len >= LINE_MAX_LEN) {   /* qua dai ma van chua co '\n' */
            lb->len = 0;
            return -1;
        }
        return 0;
    }
    size_t line_len = (size_t)(nl - lb->data);
    size_t consumed = line_len + 1;
    if (line_len > 0 && lb->data[line_len - 1] == '\r') line_len--;

    int ret = 1;
    if (line_len > LINE_MAX_LEN || line_len + 1 > out_size) {
        ret = -1;
    } else {
        memcpy(out, lb->data, line_len);
        out[line_len] = '\0';
    }
    memmove(lb->data, lb->data + consumed, lb->len - consumed);
    lb->len -= consumed;
    return ret;
}
