#include "device/config.h"
#include "common/protocol.h"
#include "common/randutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int config_make_id(device_type_t type, char *out, size_t out_size) {
    char hex[7];
    if (type < 0 || type >= DEV_TYPE_COUNT) return -1;
    if (rand_hex(hex, 6) < 0) return -1;
    int n = snprintf(out, out_size, "%s-%s", device_type_to_str(type), hex);
    if (n < 0 || (size_t)n >= out_size) return -1;
    return 0;
}

static int parse_int(const char *s, long lo, long hi, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || v < lo || v > hi) return -1;
    *out = (int)v;
    return 0;
}

int config_parse(int argc, char **argv, device_config_t *cfg) {
    const char *type_s = NULL, *id = NULL, *pw = NULL;
    memset(cfg, 0, sizeof(*cfg));
    cfg->tcp_port = 0;
    cfg->udp_port = SCAN_PORT;

    optind = 1;
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, "t:i:p:u:w:")) != -1) {
        switch (opt) {
        case 't': type_s = optarg; break;
        case 'i': id = optarg; break;
        case 'w': pw = optarg; break;
        case 'p': if (parse_int(optarg, 0, 65535, &cfg->tcp_port) < 0) return -1; break;
        case 'u': if (parse_int(optarg, 1, 65535, &cfg->udp_port) < 0) return -1; break;
        default:  return -1;
        }
    }

    device_type_t type;
    if (!type_s || device_type_from_str(type_s, &type) < 0) return -1;

    char gen[DEVICE_ID_LEN];
    if (!id) {
        if (config_make_id(type, gen, sizeof gen) < 0) return -1;
        id = gen;
    }
    return device_init(&cfg->dev, type, id, pw);
}

void config_usage(const char *prog) {
    fprintf(stderr,
        "Cach dung: %s -t LOAI [-i ID] [-p tcp_port] [-u udp_port] [-w mat_khau]\n"
        "  LOAI: SENSOR | PUMP | AERATOR | PH | FEEDER\n"
        "  -i  ID thiet bi (mac dinh: tu sinh ngau nhien)\n"
        "  -p  cong TCP (mac dinh 0 = tu chon)\n"
        "  -u  cong UDP nhan lenh quet (mac dinh %d)\n"
        "  -w  mat khau (mac dinh %s)\n", prog, SCAN_PORT, DEFAULT_PASSWORD);
}
