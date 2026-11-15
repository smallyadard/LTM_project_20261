#include "common/message.h"
#include <string.h>

static int has_bad_char(const char *s) {
    return strpbrk(s, "|\r\n") != NULL;
}

int msg_parse(const char *line, message_t *m) {
    memset(m, 0, sizeof(*m));
    const char *p = line;
    int field = 0;   /* 0 = cmd, 1.. = args */

    for (;;) {
        const char *end = strchr(p, '|');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        if (field == 0) {
            if (len == 0 || len >= MSG_CMD_LEN) return -1;
            memcpy(m->cmd, p, len);
            m->cmd[len] = '\0';
        } else {
            if (field > MSG_MAX_ARGS || len >= MSG_ARG_LEN) return -1;
            memcpy(m->args[field - 1], p, len);
            m->args[field - 1][len] = '\0';
            m->argc = field;
        }
        if (!end) break;
        p = end + 1;
        field++;
    }
    return 0;
}

int msg_build(char *out, size_t out_size, const char *cmd, int argc, const char *const args[]) {
    if (!cmd || cmd[0] == '\0' || has_bad_char(cmd)) return -1;
    if (argc < 0 || argc > MSG_MAX_ARGS) return -1;

    size_t pos = 0;
    size_t cl = strlen(cmd);
    if (cl >= MSG_CMD_LEN || cl + 1 > out_size) return -1;
    memcpy(out, cmd, cl);
    pos = cl;

    for (int i = 0; i < argc; i++) {
        if (!args[i] || has_bad_char(args[i])) return -1;
        size_t al = strlen(args[i]);
        if (al >= MSG_ARG_LEN || pos + 1 + al + 1 > out_size) return -1;
        out[pos++] = '|';
        memcpy(out + pos, args[i], al);
        pos += al;
    }
    out[pos] = '\0';
    return (int)pos;
}
