#include "device/sim.h"

int sim_feeder_due(const feeder_params_t *f, int prev_min, int now_min) {
    int n = 0;
    for (int i = 0; i < f->time_count && i < MAX_SCHEDULES; i++) {
        int t = f->times[i];
        if (prev_min <= now_min) {
            if (t > prev_min && t <= now_min) n++;                  /* (prev, now] */
        } else {
            if (t > prev_min || t <= now_min) n++;                  /* qua nua dem */
        }
    }
    return n;
}

double sim_feeder_tick(const feeder_params_t *f, sim_state_t *s, int now_min) {
    if (!s->feeder_started) {              /* lan dau: chi ghi nho moc */
        s->feeder_started = 1;
        s->feeder_last_min = now_min;
        return 0.0;
    }
    int n = sim_feeder_due(f, s->feeder_last_min, now_min);
    s->feeder_last_min = now_min;
    if (n == 0) return 0.0;

    double kg = f->amount * n;
    s->feed_total_kg += kg;
    s->feed_count += n;
    return kg;
}
