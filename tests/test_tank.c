#include <stdio.h>
#include <string.h>
#include "common/tank.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static tank_device_t mk(const char *id, device_type_t type) {
    tank_device_t d; memset(&d, 0, sizeof d);
    snprintf(d.id, sizeof d.id, "%s", id);
    d.type = type; d.port = 5000; snprintf(d.ip, sizeof d.ip, "127.0.0.1");
    return d;
}

static int t_add_find(void) {
    tank_t t; tank_init(&t, 1, "Ho A");
    tank_device_t a = mk("PUMP-1", DEV_PUMP), b = mk("PH-1", DEV_PH);
    CHECK(strcmp(t.name, "Ho A") == 0 && t.device_count == 0);
    CHECK(tank_add_device(&t, &a) == TANK_OK && tank_add_device(&t, &b) == TANK_OK);
    CHECK(t.device_count == 2);
    CHECK(tank_find_device(&t, "PH-1") && tank_find_device(&t, "PH-1")->type == DEV_PH);
    CHECK(tank_find_device(&t, "NOPE") == NULL);
    PASS("tank: add and find"); return 0;
}

static int t_duplicate_and_invalid(void) {
    tank_t t; tank_init(&t, 1, "Ho A");
    tank_device_t a = mk("PUMP-1", DEV_PUMP), e = mk("", DEV_PUMP);
    tank_add_device(&t, &a);
    CHECK(tank_add_device(&t, &a) == TANK_ERR_DUP && t.device_count == 1);
    CHECK(tank_add_device(&t, &e) == TANK_ERR_ARG);
    CHECK(tank_add_device(&t, NULL) == TANK_ERR_ARG);
    PASS("tank: reject duplicate and invalid"); return 0;
}

static int t_full(void) {
    tank_t t; tank_init(&t, 1, "Ho A"); char id[DEVICE_ID_LEN];
    for (int i = 0; i < MAX_DEVICES_PER_TANK; i++) {
        snprintf(id, sizeof id, "D-%d", i);
        tank_device_t d = mk(id, DEV_SENSOR);
        CHECK(tank_add_device(&t, &d) == TANK_OK);
    }
    tank_device_t x = mk("EXTRA", DEV_SENSOR);
    CHECK(tank_add_device(&t, &x) == TANK_ERR_FULL);
    CHECK(t.device_count == MAX_DEVICES_PER_TANK);
    PASS("tank: full tank rejects new device"); return 0;
}

static int t_remove_keeps_order(void) {
    tank_t t; tank_init(&t, 1, "Ho A");
    tank_device_t a = mk("A", DEV_PUMP), b = mk("B", DEV_PH), c = mk("C", DEV_FEEDER);
    tank_add_device(&t, &a); tank_add_device(&t, &b); tank_add_device(&t, &c);
    CHECK(tank_remove_device(&t, "B") == TANK_OK && t.device_count == 2);
    CHECK(strcmp(t.devices[0].id, "A") == 0 && strcmp(t.devices[1].id, "C") == 0);
    CHECK(tank_find_device(&t, "B") == NULL);
    CHECK(tank_remove_device(&t, "B") == TANK_ERR_NOENT);
    CHECK(tank_remove_device(&t, "C") == TANK_OK && tank_remove_device(&t, "A") == TANK_OK);
    CHECK(t.device_count == 0);
    PASS("tank: remove keeps order, handles missing id"); return 0;
}

static int t_remove_then_add_when_full(void) {
    tank_t t; tank_init(&t, 1, "Ho A"); char id[DEVICE_ID_LEN];
    for (int i = 0; i < MAX_DEVICES_PER_TANK; i++) {
        snprintf(id, sizeof id, "D-%d", i);
        tank_device_t d = mk(id, DEV_SENSOR); tank_add_device(&t, &d);
    }
    CHECK(tank_remove_device(&t, "D-0") == TANK_OK);
    tank_device_t n = mk("NEW", DEV_PUMP);
    CHECK(tank_add_device(&t, &n) == TANK_OK && t.device_count == MAX_DEVICES_PER_TANK);
    PASS("tank: slot reusable after remove"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_add_find, t_duplicate_and_invalid, t_full,
                             t_remove_keeps_order, t_remove_then_add_when_full };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
