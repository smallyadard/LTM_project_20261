#ifndef MESSAGE_H
#define MESSAGE_H
#include <stddef.h>

#define MSG_MAX_ARGS 16
#define MSG_CMD_LEN  32
#define MSG_ARG_LEN  128

typedef struct {
    char cmd[MSG_CMD_LEN];
    char args[MSG_MAX_ARGS][MSG_ARG_LEN];
    int  argc;
} message_t;

/* Tach "CMD|a1|a2" (khong co '\n'). 0: ok, -1: sai dinh dang/qua dai/qua nhieu tham so. */
int msg_parse(const char *line, message_t *m);

/* Tao "CMD|a1|a2" vao out. Tra ve do dai chuoi, -1 neu loi
 * (co '|' hoac '\n' trong cmd/args, cmd rong, out qua nho, qua nhieu tham so). */
int msg_build(char *out, size_t out_size, const char *cmd, int argc, const char *const args[]);

#endif
