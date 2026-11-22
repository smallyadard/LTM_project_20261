#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include "common/defaults.h"
#include "common/protocol.h"
#include "controller/scan.h"
#include "device/scan_listener.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

#define NDEV 5
static pid_t kids[NDEV];
static int nkids = 0;
static int udp_port;

/* Mo phong 1 thiet bi: tien trinh con chi tra loi lenh quet. */
static void device_process(int fd, device_type_t type, const char *id, int tcp_port) {
    device_t d; device_init(&d, type, id, NULL);
    struct pollfd p = { .fd = fd, .events = POLLIN };
    for (;;) if (poll(&p, 1, -1) > 0) scan_listener_handle(fd, &d.base, tcp_port);
}

static int start_devices(int n) {
    for (int i = 0; i < n; i++) {
        int fd = scan_listener_open(udp_port);     /* mo TRUOC khi fork de khong bi race */
        if (fd < 0) return -1;
        pid_t pid = fork();
        if (pid == 0) {
            char id[DEVICE_ID_LEN]; snprintf(id, sizeof id, "DEV-%d", i);
            device_process(fd, (device_type_t)(i % DEV_TYPE_COUNT), id, 6000 + i);
            _exit(0);
        }
        close(fd);
        kids[nkids++] = pid;
    }
    return 0;
}

static void stop_devices(void) {
    for (int i = 0; i < nkids; i++) { kill(kids[i], SIGTERM); waitpid(kids[i], NULL, 0); }
    nkids = 0;
}

/* Thu dia chi broadcast noi bo truoc, roi toi broadcast chung. */
static int scan_any(scan_result_t *res, int max, int wait_ms) {
    static const char *addrs[] = { "127.255.255.255", SCAN_BROADCAST_ADDR };
    int n = 0;
    for (int i = 0; i < 2 && n == 0; i++) n = scan_devices(addrs[i], udp_port, wait_ms, res, max);
    return n;
}

static int t_scan_multiple(void) {
    CHECK(start_devices(NDEV) == 0);
    scan_result_t res[SCAN_MAX_RESULTS];
    int n = scan_any(res, SCAN_MAX_RESULTS, 800);
    CHECK(n == NDEV);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) CHECK(strcmp(res[i].id, res[j].id) != 0);   /* khong trung */
        int k = res[i].id[4] - '0';
        CHECK(res[i].port == 6000 + k && (int)res[i].type == k % DEV_TYPE_COUNT);
        CHECK(res[i].ip[0] != '\0');
    }
    stop_devices();
    PASS("scan: finds all 5 devices with correct id/type/port, no duplicates"); return 0;
}

static int t_scan_limit_and_empty(void) {
    CHECK(start_devices(NDEV) == 0);
    scan_result_t res[3];
    CHECK(scan_any(res, 3, 600) == 3);                       /* gioi han max */
    stop_devices();
    CHECK(scan_devices("127.255.255.255", udp_port, 250, res, 3) == 0);   /* khong ai tra loi */
    PASS("scan: respects max results, returns 0 when nobody answers"); return 0;
}

static int t_scan_ignores_garbage(void) {
    CHECK(start_devices(2) == 0);
    int s = socket(AF_INET, SOCK_DGRAM, 0); int on = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof on);
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons((unsigned short)udp_port);
    inet_pton(AF_INET, "127.255.255.255", &a.sin_addr);
    sendto(s, "HELLO|x", 7, 0, (struct sockaddr *)&a, sizeof a);   /* khong phai SCAN */
    sendto(s, "\xff\xfe|||", 5, 0, (struct sockaddr *)&a, sizeof a);
    close(s);
    scan_result_t res[8];
    CHECK(scan_any(res, 8, 600) == 2);
    stop_devices();
    PASS("scan: garbage datagrams ignored, devices still answer"); return 0;
}

static int t_bad_args(void) {
    scan_result_t res[2];
    CHECK(scan_devices("not-an-ip", udp_port, 100, res, 2) == -1);
    CHECK(scan_devices("127.0.0.1", udp_port, 100, res, 0) == -1);
    CHECK(scan_devices(NULL, udp_port, 100, res, 2) == -1);
    PASS("scan: invalid arguments rejected"); return 0;
}

int main(void) {
    udp_port = 20000 + (int)(getpid() % 5000);
    int (*tests[])(void) = { t_scan_multiple, t_scan_limit_and_empty, t_scan_ignores_garbage, t_bad_args };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) { stop_devices(); return 1; }
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
