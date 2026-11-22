#ifndef SIM_H
#define SIM_H
#include "common/types.h"

/* Trang thai chay cua mo phong (khong thuoc thong so cai dat cua nguoi dung). */
typedef struct {
    unsigned seed;                 /* [commit 6] bo sinh so ngau nhien cua cam bien */

    double   pump_run_h;           /* [commit 7] so gio bom ke tu luc bat */
    double   pump_volume_m3;       /* [commit 7] tong the tich da bom */

    double   lime_total_kg;        /* [commit 8] tong luong voi da phun */
    int      lime_count;           /* [commit 8] so lan phun */
    int      feeder_started;       /* [commit 8] 0 cho toi lan kiem tra dau tien */
    int      feeder_last_min;      /* [commit 8] phut cua lan kiem tra truoc */
    double   feed_total_kg;        /* [commit 8] tong thuc an da cho */
    int      feed_count;           /* [commit 8] so lan cho an */
} sim_state_t;

/* [commit 6] ---- Cam bien ---- */
void sim_init(sim_state_t *s, unsigned seed);
/* Moi lan goi: muoi, oxi, pH troi nhe ngau nhien va luon nam trong khoang hop ly. */
void sim_sensor_step(sensor_data_t *d, sim_state_t *s);

/* [commit 7] ---- Bom nuoc ---- */
/* Bat bom: state = ON, dat lai bo dem gio. (the tich tong van duoc giu) */
void sim_pump_start(device_state_t *st, sim_state_t *s);
/* Troi dt_hours gio. Chi bom khi dang ON, toi da T gio roi tu tat.
 * Tra ve 1 neu bom vua ket thuc trong lan nay, nguoc lai 0. */
int  sim_pump_tick(const pump_params_t *p, device_state_t *st, sim_state_t *s, double dt_hours);

/* [commit 7] ---- Quat suc khi ---- */
/* 1 neu phut trong ngay (0..1439) nam trong 1 khoang gio da dat [start, end).
 * Ho tro khoang qua nua dem (start > end, vd 22:00 -> 05:00). */
int  sim_aerator_scheduled(const aerator_params_t *a, int minute);

/* [commit 8] ---- Can bang pH ---- */
#define LIME_PH_PER_KG 0.2
/* Neu pH < pHmin thi phun Wca kg voi: tra ve luong phun (0 neu khong phun). */
double sim_ph_dose(const ph_params_t *p, double current_ph, sim_state_t *s);
/* pH sau khi them kg voi (toi da 14). */
double sim_ph_apply_lime(double ph, double kg);

/* [commit 8] ---- May cho an ---- */
/* So thoi diem cho an roi vao khoang (prev_min, now_min]. Ho tro qua nua dem (now < prev). */
int    sim_feeder_due(const feeder_params_t *f, int prev_min, int now_min);
/* Tinh luong thuc an can cho tu lan kiem tra truoc den now_min va cap nhat tong.
 * Lan goi dau tien chi ghi nho moc thoi gian (tra ve 0). */
double sim_feeder_tick(const feeder_params_t *f, sim_state_t *s, int now_min);

#endif
