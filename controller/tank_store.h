#ifndef TANK_STORE_H
#define TANK_STORE_H
#include "controller/tank_mgr.h"

/* Dinh dang file (moi dong dung quy tac CMD|arg|... nhu giao thuc):
 *   TANK|id|ten
 *   DEV|id|LOAI|ip|port|token       (thuoc ho TANK ngay phia tren)
 */

/* Ghi ra file tam roi doi ten (khong lam hong file cu neu loi giua chung).
 * TM_OK hoac TM_ERR_IO. */
int tm_save(const tank_mgr_t *m, const char *path);

/* Doc file. Chi thay doi *m khi thanh cong.
 * TM_OK, TM_ERR_IO (khong mo duoc, vd chua co file), TM_ERR_FORMAT (noi dung sai). */
int tm_load(tank_mgr_t *m, const char *path);

#endif
