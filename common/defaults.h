#ifndef DEFAULTS_H
#define DEFAULTS_H
#include "common/types.h"

#define DEFAULT_PASSWORD "123456"

void sensor_init_default(sensor_data_t *s);
void pump_init_default(pump_params_t *p);
void aerator_init_default(aerator_params_t *a);
void ph_init_default(ph_params_t *p);
void feeder_init_default(feeder_params_t *f);

/* Khoi tao thiet bi theo loai: dat id, mat khau (NULL = mac dinh), trang thai OFF,
 * thong so mac dinh. 0: ok, -1: tham so sai. */
int device_init(device_t *d, device_type_t type, const char *id, const char *password);

#endif
