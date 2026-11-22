#ifndef TYPES_H
#define TYPES_H

/* ---- Gioi han co dinh (khop voi dac ta giao thuc) ---- */
#define DEVICE_ID_LEN   16   /* vd "PUMP-0001" + '\0' */
#define PASSWORD_LEN    32
#define TOKEN_LEN       33   /* 32 ky tu hex + '\0' */
#define MAX_SCHEDULES   8    /* so khoang gio cua quat / so thoi diem cho an */

/* ---- Loai thiet bi (chuoi tren day khop ten trong giao thuc) ---- */
typedef enum {
    DEV_SENSOR  = 0,   /* "SENSOR"  */
    DEV_PUMP    = 1,   /* "PUMP"    */
    DEV_AERATOR = 2,   /* "AERATOR" */
    DEV_PH      = 3,   /* "PH"      */
    DEV_FEEDER  = 4,   /* "FEEDER"  */
    DEV_TYPE_COUNT
} device_type_t;

typedef enum {
    STATE_OFF = 0,
    STATE_ON  = 1
} device_state_t;

/* Thong tin chung cua moi thiet bi */
typedef struct {
    char           id[DEVICE_ID_LEN];
    device_type_t  type;
    device_state_t state;
    char           password[PASSWORD_LEN];
} device_base_t;

/* Thoi gian trong ngay luu dang so phut ke tu 0h (0..1439) */
#define MINUTES_PER_DAY 1440

/* Cam bien moi truong: gia tri do duoc */
typedef struct {
    double salinity;   /* nong do muoi (ppt) */
    double oxygen;     /* nong do oxi hoa tan (mg/L) */
    double ph;         /* do pH */
} sensor_data_t;

/* Bo phan bom nuoc */
typedef struct {
    double flow;       /* V: luu luong (m3/gio) */
    double duration;   /* T: thoi gian bom (gio) */
} pump_params_t;

/* Khoang gio trong ngay [start, end), don vi phut ke tu 0h */
typedef struct {
    int start_min;
    int end_min;
} time_range_t;

/* Quat suc khi be mat */
typedef struct {
    int          rpm;                       /* C: toc do quay (vong/phut) */
    time_range_t ranges[MAX_SCHEDULES];     /* cac khoang gio hoat dong */
    int          range_count;
} aerator_params_t;

/* May cho an tu dong */
typedef struct {
    double amount;                          /* Wf: luong thuc an moi lan (kg) */
    int    times[MAX_SCHEDULES];            /* cac thoi diem cho an (phut ke tu 0h) */
    int    time_count;
} feeder_params_t;

/* Bo phan can bang pH bang phun voi bot */
typedef struct {
    double lime_amount;   /* Wca: luong voi moi lan phun (kg) */
    double ph_min;        /* pHmin: phun voi khi pH < pHmin */
} ph_params_t;

/* Thong so rieng theo loai thiet bi */
typedef union {
    sensor_data_t    sensor;
    pump_params_t    pump;
    aerator_params_t aerator;
    ph_params_t      ph;
    feeder_params_t  feeder;
} device_params_t;

/* Thiet bi day du (dung o chuong trinh mo phong) */
typedef struct {
    device_base_t   base;
    device_params_t params;
} device_t;

/* Chuyen loai thiet bi <-> chuoi giao thuc. from_str tra -1 neu khong hop le. */
const char *device_type_to_str(device_type_t t);
int         device_type_from_str(const char *s, device_type_t *out);

#endif
