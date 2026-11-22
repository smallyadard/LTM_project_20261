#ifndef TANK_H
#define TANK_H
#include "common/types.h"

#define MAX_DEVICES_PER_TANK 16
#define TANK_NAME_LEN        32
#define IP_LEN               16   /* "255.255.255.255" + '\0' */

/* Thiet bi ma controller biet ve (sau khi quet / ket noi) */
typedef struct {
    char          id[DEVICE_ID_LEN];
    device_type_t type;
    char          ip[IP_LEN];
    int           port;                 /* cong TCP cua thiet bi */
    char          token[TOKEN_LEN];     /* rong neu chua ket noi */
} tank_device_t;

typedef struct {
    int           id;
    char          name[TANK_NAME_LEN];
    tank_device_t devices[MAX_DEVICES_PER_TANK];
    int           device_count;
} tank_t;

#define TANK_OK         0
#define TANK_ERR_FULL  (-1)
#define TANK_ERR_DUP   (-2)
#define TANK_ERR_ARG   (-3)
#define TANK_ERR_NOENT (-4)

void tank_init(tank_t *t, int id, const char *name);

/* Them thiet bi: TANK_OK, TANK_ERR_FULL (ho day), TANK_ERR_DUP (trung id), TANK_ERR_ARG */
int tank_add_device(tank_t *t, const tank_device_t *d);

/* Xoa theo id, giu nguyen thu tu cac thiet bi con lai. TANK_OK hoac TANK_ERR_NOENT */
int tank_remove_device(tank_t *t, const char *id);

/* Tim theo id. NULL neu khong co. */
tank_device_t *tank_find_device(tank_t *t, const char *id);

#endif
