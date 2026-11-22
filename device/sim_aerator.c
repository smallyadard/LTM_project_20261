#include "device/sim.h"

int sim_aerator_scheduled(const aerator_params_t *a, int minute) {
    for (int i = 0; i < a->range_count && i < MAX_SCHEDULES; i++) {
        int start = a->ranges[i].start_min, end = a->ranges[i].end_min;
        if (start == end) continue;                       /* khoang rong */
        if (start < end) {
            if (minute >= start && minute < end) return 1;
        } else {                                          /* qua nua dem */
            if (minute >= start || minute < end) return 1;
        }
    }
    return 0;
}
