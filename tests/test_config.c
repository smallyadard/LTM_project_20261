#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "common/protocol.h"
#include "common/randutil.h"
#include "device/config.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static int t_rand_hex(void) {
    char a[33], b[33];
    CHECK(rand_hex(a, 32) == 0 && strlen(a) == 32);
    for (int i = 0; i < 32; i++) CHECK(isxdigit((unsigned char)a[i]));
    CHECK(rand_hex(b, 32) == 0 && strcmp(a, b) != 0);
    CHECK(rand_hex(a, 0) == -1 && rand_hex(a, 65) == -1 && rand_hex(NULL, 4) == -1);
    CHECK(rand_hex(a, 5) == 0 && strlen(a) == 5);   /* so le */
    PASS("randutil: hex format, different each call, bad args"); return 0;
}

static int t_parse_ok(void) {
    device_config_t c; char *argv[] = { "dev", "-t", "PUMP", "-i", "P-1", "-p", "6001", "-u", "9100", "-w", "pw" };
    CHECK(config_parse(11, argv, &c) == 0);
    CHECK(c.dev.base.type == DEV_PUMP && strcmp(c.dev.base.id, "P-1") == 0);
    CHECK(c.tcp_port == 6001 && c.udp_port == 9100 && strcmp(c.dev.base.password, "pw") == 0);
    CHECK(c.dev.params.pump.flow > 0);
    PASS("config: full argument list"); return 0;
}

static int t_parse_defaults_and_unique_id(void) {
    device_config_t c1, c2; char *argv[] = { "dev", "-t", "AERATOR" };
    CHECK(config_parse(3, argv, &c1) == 0 && config_parse(3, argv, &c2) == 0);
    CHECK(c1.tcp_port == 0 && c1.udp_port == SCAN_PORT);
    CHECK(strcmp(c1.dev.base.password, DEFAULT_PASSWORD) == 0);
    CHECK(strncmp(c1.dev.base.id, "AERATOR-", 8) == 0 && strlen(c1.dev.base.id) == 14);
    CHECK(strcmp(c1.dev.base.id, c2.dev.base.id) != 0);
    PASS("config: defaults and auto unique ID (longest type fits)"); return 0;
}

static int t_parse_invalid(void) {
    device_config_t c;
    char *a1[] = { "dev" };
    char *a2[] = { "dev", "-t", "TOASTER" };
    char *a3[] = { "dev", "-t", "PUMP", "-p", "70000" };
    char *a4[] = { "dev", "-t", "PUMP", "-p", "abc" };
    char *a5[] = { "dev", "-t", "PUMP", "-i", "this-id-is-way-too-long" };
    char *a6[] = { "dev", "-t", "PUMP", "-x" };
    char *a7[] = { "dev", "-t", "PUMP", "-u", "0" };
    CHECK(config_parse(1, a1, &c) == -1);
    CHECK(config_parse(3, a2, &c) == -1);
    CHECK(config_parse(5, a3, &c) == -1);
    CHECK(config_parse(5, a4, &c) == -1);
    CHECK(config_parse(5, a5, &c) == -1);
    CHECK(config_parse(4, a6, &c) == -1);
    CHECK(config_parse(5, a7, &c) == -1);
    PASS("config: missing/invalid type, bad ports, long id, unknown option"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_rand_hex, t_parse_ok, t_parse_defaults_and_unique_id, t_parse_invalid };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
