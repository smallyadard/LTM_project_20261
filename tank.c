#include "common/tank.h"
#include <stdio.h>
#include <string.h>

void tank_init(tank_t *t, int id, const char *name) {
    memset(t, 0, sizeof(*t));
    t->id = id;
    snprintf(t->name, sizeof(t->name), "%s", name ? name : "");
}

tank_device_t *tank_find_device(tank_t *t, const char *id) {
    if (!t || !id) return NULL;
    for (int i = 0; i < t->device_count; i++) {
        if (strcmp(t->devices[i].id, id) == 0) return &t->devices[i];
    }
    return NULL;
}

int tank_add_device(tank_t *t, const tank_device_t *d) {
    if (!t || !d || d->id[0] == '\0') return TANK_ERR_ARG;
    if (tank_find_device(t, d->id)) return TANK_ERR_DUP;
    if (t->device_count >= MAX_DEVICES_PER_TANK) return TANK_ERR_FULL;
    t->devices[t->device_count++] = *d;
    return TANK_OK;
}

int tank_remove_device(tank_t *t, const char *id) {
    if (!t || !id) return TANK_ERR_ARG;
    for (int i = 0; i < t->device_count; i++) {
        if (strcmp(t->devices[i].id, id) == 0) {
            memmove(&t->devices[i], &t->devices[i + 1],
                    (size_t)(t->device_count - i - 1) * sizeof(tank_device_t));
            t->device_count--;
            memset(&t->devices[t->device_count], 0, sizeof(tank_device_t));
            return TANK_OK;
        }
    }
    return TANK_ERR_NOENT;
}
