#ifndef SCAN_LISTENER_H
#define SCAN_LISTENER_H
#include "common/types.h"

/* Mo socket UDP lang nghe lenh quet tren udp_port. Cho phep nhieu thiet bi cung may
 * dung chung 1 cong (SO_REUSEADDR/SO_REUSEPORT) de demo. Tra ve fd, hoac -1. */
int scan_listener_open(int udp_port);

/* Doc 1 datagram. Neu la "SCAN" thi tra loi nguoi gui "DEVICE|type|id|tcp_port".
 * 1: da tra loi, 0: bo qua (khong phai SCAN), -1: loi. */
int scan_listener_handle(int fd, const device_base_t *base, int tcp_port);

#endif
