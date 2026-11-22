#include <stdio.h>
#include <string.h>
#include "common/defaults.h"
#include "common/types.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static int t_type_strings(void) {
    device_type_t t;
    CHECK(strcmp(device_type_to_str(DEV_PUMP), "PUMP") == 0);
    CHECK(strcmp(device_type_to_str(DEV_TYPE_COUNT), "UNKNOWN") == 0);
    CHECK(device_type_from_str("FEEDER", &t) == 0 && t == DEV_FEEDER);
    CHECK(device_type_from_str("pump", &t) == -1);
    CHECK(device_type_from_str(NULL, &t) == -1);
    for (int i = 0; i < DEV_TYPE_COUNT; i++) {          /* round trip */
        CHECK(device_type_from_str(device_type_to_str((device_type_t)i), &t) == 0 && (int)t == i);
    }
    PASS("types: to_str / from_str round trip"); return 0;
}

static int t_defaults(void) {
    sensor_data_t s; pump_params_t p; aerator_params_t a; ph_params_t h; feeder_params_t f;
    sensor_init_default(&s); pump_init_default(&p); aerator_init_default(&a);
    ph_init_default(&h); feeder_init_default(&f);
    CHECK(s.ph > 0 && s.ph < 14 && s.salinity > 0 && s.oxygen > 0);
    CHECK(p.flow > 0 && p.duration > 0);
    CHECK(a.rpm > 0 && a.range_count == 0);
    CHECK(h.ph_min == 6.5 && h.lime_amount == 1.0);
    CHECK(f.amount > 0 && f.time_count == 0);
    PASS("defaults: all 5 types valid"); return 0;
}

static int t_device_init(void) {
    device_t d;
    for (int i = 0; i < DEV_TYPE_COUNT; i++) {
        CHECK(device_init(&d, (device_type_t)i, "DEV-0001", NULL) == 0);
        CHECK(d.base.type == (device_type_t)i && d.base.state == STATE_OFF);
        CHECK(strcmp(d.base.id, "DEV-0001") == 0 && strcmp(d.base.password, DEFAULT_PASSWORD) == 0);
    }
    CHECK(device_init(&d, DEV_PH, "PH-1", "abc") == 0 && strcmp(d.base.password, "abc") == 0);
    CHECK(d.params.ph.ph_min == 6.5);
    CHECK(device_init(&d, DEV_PUMP, "", NULL) == -1);
    CHECK(device_init(&d, DEV_PUMP, "this-id-is-way-too-long", NULL) == -1);
    CHECK(device_init(&d, DEV_TYPE_COUNT, "X", NULL) == -1);
    CHECK(device_init(NULL, DEV_PUMP, "X", NULL) == -1);
    PASS("device_init: valid and invalid input"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_type_strings, t_defaults, t_device_init };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
