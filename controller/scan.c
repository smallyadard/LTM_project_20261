#include "controller/scan.h"
#include "common/message.h"
#include "common/protocol.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define SCAN_BUF 512

static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* Kiem tra va chuyen "DEVICE|type|id|port" thanh ket qua. 0: hop le, -1: bo qua. */
static int parse_reply(const char *buf, scan_result_t *r) {
    message_t m;
    if (msg_parse(buf, &m) < 0 || strcmp(m.cmd, CMD_DEVICE) != 0 || m.argc != 3) return -1;
    if (device_type_from_str(m.args[0], &r->type) < 0) return -1;
    if (m.args[1][0] == '\0' || strlen(m.args[1]) >= DEVICE_ID_LEN) return -1;
    char *end;
    long port = strtol(m.args[2], &end, 10);
    if (*m.args[2] == '\0' || *end != '\0' || port < 1 || port > 65535) return -1;
    memcpy(r->id, m.args[1], strlen(m.args[1]) + 1);
    r->port = (int)port;
    return 0;
}

int scan_devices(const char *dest_ip, int udp_port, int timeout_ms, scan_result_t *out, int max) {
    if (!dest_ip || !out || max <= 0 || timeout_ms < 0) return -1;

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof dest);
    dest.sin_family = AF_INET;
    dest.sin_port = htons((unsigned short)udp_port);
    if (inet_pton(AF_INET, dest_ip, &dest.sin_addr) != 1) return -1;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof on);

    long start = now_ms();
    long half = start + timeout_ms / 2, deadline = start + timeout_ms;
    int sent_second = 0, count = 0;
    const char *req = CMD_SCAN;

    if (sendto(fd, req, strlen(req), 0, (struct sockaddr *)&dest, sizeof dest) < 0) {
        close(fd);
        return -1;
    }

    for (;;) {
        long now = now_ms();
        if (now >= deadline) break;
        if (!sent_second && now >= half) {
            sendto(fd, req, strlen(req), 0, (struct sockaddr *)&dest, sizeof dest);
            sent_second = 1;
        }
        long wait = (sent_second ? deadline : half) - now;   /* cho toi moc ke tiep */
        if (wait < 0) wait = 0;

        struct pollfd p = { .fd = fd, .events = POLLIN };
        int pr = poll(&p, 1, (int)wait);
        if (pr <= 0) continue;   /* het gio nho (se gui lan 2 / thoat) hoac EINTR */

        char buf[SCAN_BUF];
        struct sockaddr_in from;
        socklen_t flen = sizeof from;
        ssize_t n = recvfrom(fd, buf, sizeof buf - 1, 0, (struct sockaddr *)&from, &flen);
        if (n <= 0) continue;
        buf[n] = '\0';
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';

        scan_result_t r;
        if (parse_reply(buf, &r) < 0) continue;
        inet_ntop(AF_INET, &from.sin_addr, r.ip, sizeof r.ip);

        int dup = 0;
        for (int i = 0; i < count; i++) if (strcmp(out[i].id, r.id) == 0) { dup = 1; break; }
        if (!dup && count < max) out[count++] = r;
    }
    close(fd);
    return count;
}
