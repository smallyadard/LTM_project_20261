#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/protocol.h"
#include "controller/scan.h"

int main(int argc, char **argv) {
    const char *addr = SCAN_BROADCAST_ADDR;
    int port = SCAN_PORT, wait_ms = 1500, opt;
    while ((opt = getopt(argc, argv, "a:p:w:")) != -1) {
        switch (opt) {
        case 'a': addr = optarg; break;
        case 'p': port = atoi(optarg); break;
        case 'w': wait_ms = atoi(optarg); break;
        default:
            fprintf(stderr, "Cach dung: %s [-a dia_chi_broadcast] [-p udp_port] [-w ms]\n", argv[0]);
            return 1;
        }
    }
    scan_result_t res[SCAN_MAX_RESULTS];
    int n = scan_devices(addr, port, wait_ms, res, SCAN_MAX_RESULTS);
    if (n < 0) { fprintf(stderr, "Loi khi quet\n"); return 1; }

    printf("Tim thay %d thiet bi\n", n);
    if (n > 0) printf("%-3s %-8s %-16s %s\n", "#", "LOAI", "ID", "DIA CHI");
    for (int i = 0; i < n; i++)
        printf("%-3d %-8s %-16s %s:%d\n", i + 1, device_type_to_str(res[i].type), res[i].id, res[i].ip, res[i].port);
    return 0;
}
