#ifndef CONTROLLER_SCAN_H
#define CONTROLLER_SCAN_H
#include "common/tank.h"   /* IP_LEN */
#include "common/types.h"

#define SCAN_BROADCAST_ADDR "255.255.255.255"
#define SCAN_MAX_RESULTS    64

typedef struct {
    char          id[DEVICE_ID_LEN];
    device_type_t type;
    char          ip[IP_LEN];     /* lay tu dia chi nguoi gui goi tra loi */
    int           port;           /* cong TCP de ket noi dieu khien */
} scan_result_t;

/* Gui SCAN den dest_ip:udp_port (thuong la dia chi broadcast), thu cac phan hoi trong timeout_ms.
 * SCAN duoc gui 2 lan (dau va giua thoi gian cho) de giam mat goi UDP; trung ID chi tinh 1 lan.
 * Tra ve so thiet bi tim duoc (0..max), -1 neu loi. */
int scan_devices(const char *dest_ip, int udp_port, int timeout_ms, scan_result_t *out, int max);

#endif
