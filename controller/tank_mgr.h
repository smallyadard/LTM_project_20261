#ifndef TANK_MGR_H
#define TANK_MGR_H
#include <stdio.h>
#include "common/tank.h"

#define MAX_TANKS 16

#define TM_OK          0
#define TM_ERR_FULL   (-1)
#define TM_ERR_ARG    (-2)
#define TM_ERR_DUP    (-3)
#define TM_ERR_NOENT  (-4)
#define TM_ERR_IO     (-5)
#define TM_ERR_FORMAT (-6)

typedef struct {
    tank_t tanks[MAX_TANKS];
    int    count;
    int    next_id;     /* ID ke tiep se cap, khong bao gio tai su dung ID da xoa */
} tank_mgr_t;

void tm_init(tank_mgr_t *m);

/* Ten hop le: 1..31 ky tu, khong chua '|' hoac xuong dong. */
int  tm_valid_name(const char *name);

/* Them ho moi. TM_OK (ghi ID vao *out_id neu khac NULL), TM_ERR_ARG (ten sai),
 * TM_ERR_DUP (trung ten), TM_ERR_FULL. */
int  tm_add(tank_mgr_t *m, const char *name, int *out_id);

/* Doi ten. TM_OK, TM_ERR_NOENT, TM_ERR_ARG, TM_ERR_DUP (trung ten ho khac). */
int  tm_rename(tank_mgr_t *m, int id, const char *new_name);

/* Xoa ho (kem danh sach thiet bi cua ho), giu thu tu cac ho con lai. TM_OK / TM_ERR_NOENT */
int  tm_delete(tank_mgr_t *m, int id);

tank_t *tm_find(tank_mgr_t *m, int id);

/* In danh sach ho: "ID  TEN  (so thiet bi)" */
void tm_print(const tank_mgr_t *m, FILE *out);

#endif
