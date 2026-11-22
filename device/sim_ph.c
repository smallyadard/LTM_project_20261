#include "device/sim.h"

double sim_ph_dose(const ph_params_t *p, double current_ph, sim_state_t *s) {
    if (current_ph >= p->ph_min) return 0.0;
    s->lime_total_kg += p->lime_amount;
    s->lime_count++;
    return p->lime_amount;
}

double sim_ph_apply_lime(double ph, double kg) {
    double v = ph + LIME_PH_PER_KG * kg;
    return v > 14.0 ? 14.0 : v;
}
