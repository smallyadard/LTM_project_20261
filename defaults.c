#include "common/defaults.h"
#include <stdio.h>
#include <string.h>

void sensor_init_default(sensor_data_t *s) {
    s->salinity = 30.0;
    s->oxygen   = 6.0;
    s->ph       = 7.5;
}

void pump_init_default(pump_params_t *p) {
    p->flow     = 10.0;
    p->duration = 1.0;
}

void aerator_init_default(aerator_params_t *a) {
    memset(a, 0, sizeof(*a));
    a->rpm = 60;
}

void ph_init_default(ph_params_t *p) {
    p->lime_amount = 1.0;
    p->ph_min      = 6.5;
}

void feeder_init_default(feeder_params_t *f) {
    memset(f, 0, sizeof(*f));
    f->amount = 0.5;
}

int device_init(device_t *d, device_type_t type, const char *id, const char *password) {
    if (!d || !id || id[0] == '\0' || type < 0 || type >= DEV_TYPE_COUNT) return -1;
    if (strlen(id) >= DEVICE_ID_LEN) return -1;

    const char *pw = password ? password : DEFAULT_PASSWORD;
    if (strlen(pw) >= PASSWORD_LEN) return -1;

    memset(d, 0, sizeof(*d));
    snprintf(d->base.id, sizeof(d->base.id), "%s", id);
    snprintf(d->base.password, sizeof(d->base.password), "%s", pw);
    d->base.type  = type;
    d->base.state = STATE_OFF;

    switch (type) {
    case DEV_SENSOR:  sensor_init_default(&d->params.sensor);   break;
    case DEV_PUMP:    pump_init_default(&d->params.pump);       break;
    case DEV_AERATOR: aerator_init_default(&d->params.aerator); break;
    case DEV_PH:      ph_init_default(&d->params.ph);           break;
    case DEV_FEEDER:  feeder_init_default(&d->params.feeder);   break;
    default: return -1;
    }
    return 0;
}
