#include "controller/tank_store.h"
#include "common/linebuf.h"
#include "common/message.h"
#include <stdlib.h>
#include <string.h>

static int put(FILE *f, const char *cmd, int argc, const char *const args[]) {
    char line[LINE_MAX_LEN + 1];
    if (msg_build(line, sizeof line, cmd, argc, args) < 0) return -1;
    return fprintf(f, "%s\n", line) < 0 ? -1 : 0;
}

int tm_save(const tank_mgr_t *m, const char *path) {
    char tmp_path[512];
    if (snprintf(tmp_path, sizeof tmp_path, "%s.tmp", path) >= (int)sizeof tmp_path) return TM_ERR_IO;
    FILE *f = fopen(tmp_path, "w");
    if (!f) return TM_ERR_IO;

    int ok = 1;
    for (int i = 0; i < m->count && ok; i++) {
        const tank_t *t = &m->tanks[i];
        char id[16];
        snprintf(id, sizeof id, "%d", t->id);
        const char *ta[2] = { id, t->name };
        ok = put(f, "TANK", 2, ta) == 0;

        for (int j = 0; j < t->device_count && ok; j++) {
            const tank_device_t *d = &t->devices[j];
            char port[16];
            snprintf(port, sizeof port, "%d", d->port);
            const char *da[5] = { d->id, device_type_to_str(d->type), d->ip, port, d->token };
            ok = put(f, "DEV", 5, da) == 0;
        }
    }
    if (fflush(f) != 0 || ferror(f)) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok || rename(tmp_path, path) != 0) {
        remove(tmp_path);
        return TM_ERR_IO;
    }
    return TM_OK;
}

static int to_int(const char *s, long lo, long hi, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || v < lo || v > hi) return -1;
    *out = (int)v;
    return 0;
}

int tm_load(tank_mgr_t *m, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return TM_ERR_IO;

    tank_mgr_t tmp;
    tm_init(&tmp);
    tank_t *cur = NULL;
    char line[LINE_MAX_LEN + 2];
    int rc = TM_OK;

    while (rc == TM_OK && fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        if (n > 0 && line[n - 1] == '\n') line[--n] = '\0';
        else if (!feof(f)) { rc = TM_ERR_FORMAT; break; }          /* dong qua dai */
        if (n > 0 && line[n - 1] == '\r') line[--n] = '\0';
        if (n == 0) continue;

        message_t msg;
        if (msg_parse(line, &msg) < 0) { rc = TM_ERR_FORMAT; break; }

        if (strcmp(msg.cmd, "TANK") == 0 && msg.argc == 2) {
            int id;
            if (to_int(msg.args[0], 1, 1000000, &id) < 0 || !tm_valid_name(msg.args[1])
                || tmp.count >= MAX_TANKS || tm_find(&tmp, id)) { rc = TM_ERR_FORMAT; break; }
            cur = &tmp.tanks[tmp.count++];
            tank_init(cur, id, msg.args[1]);
            if (id >= tmp.next_id) tmp.next_id = id + 1;
        } else if (strcmp(msg.cmd, "DEV") == 0 && msg.argc == 5 && cur) {
            tank_device_t d;
            memset(&d, 0, sizeof d);
            if (strlen(msg.args[0]) >= DEVICE_ID_LEN || strlen(msg.args[2]) >= IP_LEN
                || strlen(msg.args[4]) >= TOKEN_LEN
                || device_type_from_str(msg.args[1], &d.type) < 0
                || to_int(msg.args[3], 1, 65535, &d.port) < 0) { rc = TM_ERR_FORMAT; break; }
            memcpy(d.id, msg.args[0], strlen(msg.args[0]));
            memcpy(d.ip, msg.args[2], strlen(msg.args[2]));
            memcpy(d.token, msg.args[4], strlen(msg.args[4]));
            if (tank_add_device(cur, &d) != TANK_OK) { rc = TM_ERR_FORMAT; break; }
        } else {
            rc = TM_ERR_FORMAT;
        }
    }
    fclose(f);
    if (rc != TM_OK) return rc;
    *m = tmp;
    return TM_OK;
}
