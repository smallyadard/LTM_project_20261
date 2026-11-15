#include "common/types.h"
#include <string.h>

static const char *TYPE_NAMES[DEV_TYPE_COUNT] = {
    "SENSOR", "PUMP", "AERATOR", "PH", "FEEDER"
};

const char *device_type_to_str(device_type_t t) {
    if (t < 0 || t >= DEV_TYPE_COUNT) return "UNKNOWN";
    return TYPE_NAMES[t];
}

int device_type_from_str(const char *s, device_type_t *out) {
    if (!s) return -1;
    for (int i = 0; i < DEV_TYPE_COUNT; i++) {
        if (strcmp(s, TYPE_NAMES[i]) == 0) {
            *out = (device_type_t)i;
            return 0;
        }
    }
    return -1;
}
