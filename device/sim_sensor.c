#include "device/sim.h"
#include <stdlib.h>
#include <string.h>

void sim_init(sim_state_t *s, unsigned seed) {
    memset(s, 0, sizeof(*s));
    s->seed = seed;
}

/* Ngau nhien trong [-1, 1] */
static double rnd_signed(unsigned *seed) {
    return ((double)rand_r(seed) / (double)RAND_MAX) * 2.0 - 1.0;
}

static double walk(double v, double step, double lo, double hi, unsigned *seed) {
    v += rnd_signed(seed) * step;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

void sim_sensor_step(sensor_data_t *d, sim_state_t *s) {
    d->salinity = walk(d->salinity, 0.20, 0.0, 45.0, &s->seed);   /* ppt  */
    d->oxygen   = walk(d->oxygen,   0.10, 0.0, 14.0, &s->seed);   /* mg/L */
    d->ph       = walk(d->ph,       0.05, 4.0, 10.0, &s->seed);
}
