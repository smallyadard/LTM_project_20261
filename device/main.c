#include <stdio.h>
#include "common/protocol.h"
#include "device/config.h"
#include "device/scan_listener.h"
#include "device/server.h"

static device_config_t cfg;
static server_t srv;

static void on_scan_readable(int fd, void *user) {
    (void)user;
    scan_listener_handle(fd, &cfg.dev.base, srv.port);
}

/* Tuan 3 se them CONNECT, token va cac lenh dieu khien o day. */
static int on_line(server_t *s, int idx, const char *line, void *user) {
    (void)line; (void)user;
    server_send(s, idx, CMD_ERR "|" ERR_UNKNOWN_CMD);
    return 0;
}

int main(int argc, char **argv) {
    if (config_parse(argc, argv, &cfg) < 0) {
        config_usage(argv[0]);
        return 1;
    }
    if (server_open(&srv, cfg.tcp_port) < 0) {
        perror("server_open");
        return 1;
    }
    int udp = scan_listener_open(cfg.udp_port);
    if (udp < 0) {
        perror("scan_listener_open");
        return 1;
    }
    server_set_aux(&srv, udp, on_scan_readable, NULL);

    printf("[%s] %s | TCP %d | scan UDP %d\n", cfg.dev.base.id,
           device_type_to_str(cfg.dev.base.type), srv.port, cfg.udp_port);
    fflush(stdout);

    for (;;) {
        if (server_run_once(&srv, 1000, on_line, NULL) < 0) {
            perror("poll");
            return 1;
        }
    }
}
