#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H
#include <stddef.h>
#include "common/defaults.h"

typedef struct {
    device_t dev;       /* id, loai, mat khau, thong so mac dinh */
    int      tcp_port;  /* 0 = he dieu hanh tu chon, cong thuc te duoc bao trong ban tin quet */
    int      udp_port;  /* cong lang nghe lenh quet */
} device_config_t;

/* Tao ID duy nhat dang "<LOAI>-<6 hex ngau nhien>" (toi da 14 ky tu). 0/-1 */
int config_make_id(device_type_t type, char *out, size_t out_size);

/* Doc tham so dong lenh:  -t LOAI (bat buoc) [-i ID] [-p tcp_port] [-u udp_port] [-w mat_khau]
 * 0: ok, -1: sai tham so. */
int config_parse(int argc, char **argv, device_config_t *cfg);

void config_usage(const char *prog);

#endif
