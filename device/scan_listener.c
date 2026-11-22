#include "device/scan_listener.h"
#include "common/linebuf.h"
#include "common/message.h"
#include "common/protocol.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int scan_listener_open(int udp_port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof on);
#endif
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons((unsigned short)udp_port);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int scan_listener_handle(int fd, const device_base_t *base, int tcp_port) {
    char buf[LINE_MAX_LEN + 1];
    struct sockaddr_in from;
    socklen_t flen = sizeof from;

    ssize_t n = recvfrom(fd, buf, sizeof buf - 1, 0, (struct sockaddr *)&from, &flen);
    if (n < 0) return -1;
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';

    message_t m;
    if (msg_parse(buf, &m) < 0 || strcmp(m.cmd, CMD_SCAN) != 0) return 0;

    char port[8];
    snprintf(port, sizeof port, "%d", tcp_port);
    const char *args[3] = { device_type_to_str(base->type), base->id, port };
    char reply[LINE_MAX_LEN];
    int len = msg_build(reply, sizeof reply, CMD_DEVICE, 3, args);
    if (len < 0) return -1;
    if (sendto(fd, reply, (size_t)len, 0, (struct sockaddr *)&from, flen) < 0) return -1;
    return 1;
}
