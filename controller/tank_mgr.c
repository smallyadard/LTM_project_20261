#include "controller/tank_mgr.h"
#include <string.h>

void tm_init(tank_mgr_t *m) {
    memset(m, 0, sizeof(*m));
    m->next_id = 1;
}

int tm_valid_name(const char *name) {
    if (!name) return 0;
    size_t n = strlen(name);
    if (n == 0 || n >= TANK_NAME_LEN) return 0;
    return strpbrk(name, "|\r\n") == NULL;
}

static int name_taken(const tank_mgr_t *m, const char *name, int except_id) {
    for (int i = 0; i < m->count; i++) {
        if (m->tanks[i].id != except_id && strcmp(m->tanks[i].name, name) == 0) return 1;
    }
    return 0;
}

tank_t *tm_find(tank_mgr_t *m, int id) {
    for (int i = 0; i < m->count; i++) if (m->tanks[i].id == id) return &m->tanks[i];
    return NULL;
}

int tm_add(tank_mgr_t *m, const char *name, int *out_id) {
    if (!tm_valid_name(name)) return TM_ERR_ARG;
    if (name_taken(m, name, -1)) return TM_ERR_DUP;
    if (m->count >= MAX_TANKS) return TM_ERR_FULL;

    int id = m->next_id++;
    tank_init(&m->tanks[m->count++], id, name);
    if (out_id) *out_id = id;
    return TM_OK;
}

int tm_rename(tank_mgr_t *m, int id, const char *new_name) {
    tank_t *t = tm_find(m, id);
    if (!t) return TM_ERR_NOENT;
    if (!tm_valid_name(new_name)) return TM_ERR_ARG;
    if (name_taken(m, new_name, id)) return TM_ERR_DUP;
    memset(t->name, 0, sizeof t->name);
    memcpy(t->name, new_name, strlen(new_name));
    return TM_OK;
}

int tm_delete(tank_mgr_t *m, int id) {
    for (int i = 0; i < m->count; i++) {
        if (m->tanks[i].id == id) {
            memmove(&m->tanks[i], &m->tanks[i + 1], (size_t)(m->count - i - 1) * sizeof(tank_t));
            m->count--;
            memset(&m->tanks[m->count], 0, sizeof(tank_t));
            return TM_OK;
        }
    }
    return TM_ERR_NOENT;
}

void tm_print(const tank_mgr_t *m, FILE *out) {
    if (m->count == 0) {
        fprintf(out, "(chua co ho nuoi nao)\n");
        return;
    }
    fprintf(out, "%-4s %-32s %s\n", "ID", "TEN", "THIET BI");
    for (int i = 0; i < m->count; i++)
        fprintf(out, "%-4d %-32s %d\n", m->tanks[i].id, m->tanks[i].name, m->tanks[i].device_count);
}
