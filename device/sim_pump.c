#include "device/sim.h"

void sim_pump_start(device_state_t *st, sim_state_t *s) {
    *st = STATE_ON;
    s->pump_run_h = 0.0;
}

int sim_pump_tick(const pump_params_t *p, device_state_t *st, sim_state_t *s, double dt_hours) {
    if (*st != STATE_ON || dt_hours <= 0.0) return 0;

    double remaining = p->duration - s->pump_run_h;
    double used = dt_hours < remaining ? dt_hours : remaining;
    if (used < 0.0) used = 0.0;

    s->pump_run_h += used;
    s->pump_volume_m3 += p->flow * used;

    if (s->pump_run_h >= p->duration - 1e-9) {   /* du T gio: tu tat */
        *st = STATE_OFF;
        return 1;
    }
    return 0;
}
