#include <math.h>
#include <stdio.h>
#include <string.h>
#include "common/defaults.h"
#include "device/sim.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)
#define NEAR(a, b) (fabs((a) - (b)) < 1e-6)

static int t_sensor_bounds_and_variation(void) {
    sensor_data_t d; sim_state_t s; sensor_init_default(&d); sim_init(&s, 42);
    double min_ph = 99, max_ph = -99;
    for (int i = 0; i < 20000; i++) {
        sim_sensor_step(&d, &s);
        CHECK(d.salinity >= 0 && d.salinity <= 45 && d.oxygen >= 0 && d.oxygen <= 14);
        CHECK(d.ph >= 4 && d.ph <= 10);
        if (d.ph < min_ph) min_ph = d.ph;
        if (d.ph > max_ph) max_ph = d.ph;
    }
    CHECK(max_ph - min_ph > 0.5);   /* thuc su dao dong */
    PASS("sensor: values stay in range and vary"); return 0;
}

static int t_sensor_seed(void) {
    sensor_data_t a, b, c; sim_state_t sa, sb, sc;
    sensor_init_default(&a); sensor_init_default(&b); sensor_init_default(&c);
    sim_init(&sa, 7); sim_init(&sb, 7); sim_init(&sc, 8);
    for (int i = 0; i < 50; i++) { sim_sensor_step(&a, &sa); sim_sensor_step(&b, &sb); sim_sensor_step(&c, &sc); }
    CHECK(a.ph == b.ph && a.oxygen == b.oxygen);      /* cung seed -> cung day */
    CHECK(a.ph != c.ph || a.oxygen != c.oxygen);      /* khac seed -> khac */
    PASS("sensor: deterministic per seed"); return 0;
}

static int t_pump_runs_T_hours(void) {
    pump_params_t p = { .flow = 10.0, .duration = 2.0 };
    device_state_t st = STATE_OFF; sim_state_t s; sim_init(&s, 1);
    CHECK(sim_pump_tick(&p, &st, &s, 1.0) == 0 && s.pump_volume_m3 == 0);   /* OFF: khong bom */
    sim_pump_start(&st, &s);
    CHECK(st == STATE_ON);
    CHECK(sim_pump_tick(&p, &st, &s, 0.5) == 0 && NEAR(s.pump_volume_m3, 5.0));
    CHECK(sim_pump_tick(&p, &st, &s, 0.5) == 0 && sim_pump_tick(&p, &st, &s, 0.5) == 0);
    CHECK(st == STATE_ON && NEAR(s.pump_volume_m3, 15.0));
    CHECK(sim_pump_tick(&p, &st, &s, 0.5) == 1);                            /* du 2h */
    CHECK(st == STATE_OFF && NEAR(s.pump_volume_m3, 20.0));
    CHECK(sim_pump_tick(&p, &st, &s, 1.0) == 0 && NEAR(s.pump_volume_m3, 20.0));
    PASS("pump: pumps V*T then turns itself off"); return 0;
}

static int t_pump_overshoot_and_restart(void) {
    pump_params_t p = { .flow = 4.0, .duration = 1.0 };
    device_state_t st; sim_state_t s; sim_init(&s, 1);
    sim_pump_start(&st, &s);
    CHECK(sim_pump_tick(&p, &st, &s, 5.0) == 1);          /* buoc thoi gian lon */
    CHECK(NEAR(s.pump_volume_m3, 4.0));                   /* khong bom qua T */
    sim_pump_start(&st, &s);                              /* bat lai: dem gio moi, tong van giu */
    CHECK(sim_pump_tick(&p, &st, &s, 1.0) == 1 && NEAR(s.pump_volume_m3, 8.0));
    PASS("pump: large time step clamped, restart keeps total volume"); return 0;
}

static int t_aerator_ranges(void) {
    aerator_params_t a; aerator_init_default(&a);
    CHECK(!sim_aerator_scheduled(&a, 100));                          /* chua co lich */
    a.ranges[0].start_min = 360;  a.ranges[0].end_min = 480;         /* 06:00-08:00 */
    a.ranges[1].start_min = 1320; a.ranges[1].end_min = 300;         /* 22:00-05:00 */
    a.ranges[2].start_min = 600;  a.ranges[2].end_min = 600;         /* rong */
    a.range_count = 3;
    CHECK(!sim_aerator_scheduled(&a, 359) && sim_aerator_scheduled(&a, 360));
    CHECK(sim_aerator_scheduled(&a, 479) && !sim_aerator_scheduled(&a, 480));
    CHECK(sim_aerator_scheduled(&a, 1400) && sim_aerator_scheduled(&a, 0) && sim_aerator_scheduled(&a, 299));
    CHECK(!sim_aerator_scheduled(&a, 300) && !sim_aerator_scheduled(&a, 1319));
    CHECK(!sim_aerator_scheduled(&a, 600));
    PASS("aerator: ranges, midnight wrap, empty range"); return 0;
}

static int t_ph_dose(void) {
    ph_params_t p; ph_init_default(&p); sim_state_t s; sim_init(&s, 1);
    CHECK(NEAR(sim_ph_dose(&p, 7.2, &s), 0.0) && NEAR(sim_ph_dose(&p, 6.5, &s), 0.0));   /* khong < pHmin */
    CHECK(NEAR(sim_ph_dose(&p, 6.4, &s), 1.0));
    p.lime_amount = 2.5;
    CHECK(NEAR(sim_ph_dose(&p, 5.0, &s), 2.5));
    CHECK(s.lime_count == 2 && NEAR(s.lime_total_kg, 3.5));
    CHECK(NEAR(sim_ph_apply_lime(6.4, 2.0), 6.8) && NEAR(sim_ph_apply_lime(13.9, 5.0), 14.0));
    PASS("pH: doses Wca only when pH < pHmin, lime raises pH"); return 0;
}

static int t_feeder_due(void) {
    feeder_params_t f; feeder_init_default(&f);
    f.times[0] = 480; f.times[1] = 1080; f.time_count = 2;           /* 08:00, 18:00 */
    CHECK(sim_feeder_due(&f, 479, 480) == 1);
    CHECK(sim_feeder_due(&f, 480, 481) == 0);                         /* khong cho lap lai */
    CHECK(sim_feeder_due(&f, 500, 500) == 0);
    CHECK(sim_feeder_due(&f, 0, 1439) == 2);
    CHECK(sim_feeder_due(&f, 1400, 500) == 1);                        /* 23:20 -> 08:20: gom 08:00 */
    CHECK(sim_feeder_due(&f, 1100, 400) == 0);
    f.time_count = 0;
    CHECK(sim_feeder_due(&f, 0, 1439) == 0);
    PASS("feeder: due times, no repeats, midnight wrap"); return 0;
}

static int t_feeder_tick(void) {
    feeder_params_t f; feeder_init_default(&f);
    f.amount = 0.5; f.times[0] = 480; f.times[1] = 1080; f.time_count = 2;
    sim_state_t s; sim_init(&s, 1);
    CHECK(NEAR(sim_feeder_tick(&f, &s, 479), 0.0));                  /* lan dau chi ghi moc */
    CHECK(NEAR(sim_feeder_tick(&f, &s, 480), 0.5) && s.feed_count == 1);
    CHECK(NEAR(sim_feeder_tick(&f, &s, 481), 0.0));
    CHECK(NEAR(sim_feeder_tick(&f, &s, 1100), 0.5));
    CHECK(s.feed_count == 2 && NEAR(s.feed_total_kg, 1.0));
    CHECK(NEAR(sim_feeder_tick(&f, &s, 1439), 0.0));
    CHECK(NEAR(sim_feeder_tick(&f, &s, 600), 0.5));                  /* qua nua dem den 10:00: chi 08:00 roi vao */
    CHECK(s.feed_count == 3 && NEAR(s.feed_total_kg, 1.5));
    PASS("feeder: tick accumulates Wf per due time"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_sensor_bounds_and_variation, t_sensor_seed, t_pump_runs_T_hours,
        t_pump_overshoot_and_restart, t_aerator_ranges, t_ph_dose, t_feeder_due, t_feeder_tick };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
