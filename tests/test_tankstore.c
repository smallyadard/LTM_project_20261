#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "controller/tank_store.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static char path[128];

static void write_file(const char *content) {
    FILE *f = fopen(path, "w"); fputs(content, f); fclose(f);
}

static void add_dev(tank_t *t, const char *id, device_type_t type, int port, const char *token) {
    tank_device_t d; memset(&d, 0, sizeof d);
    snprintf(d.id, sizeof d.id, "%s", id); d.type = type; d.port = port;
    snprintf(d.ip, sizeof d.ip, "192.168.1.10"); snprintf(d.token, sizeof d.token, "%s", token);
    tank_add_device(t, &d);
}

static int t_round_trip(void) {
    tank_mgr_t a, b; tm_init(&a); int id1, id2;
    tm_add(&a, "Ho tom so 1", &id1); tm_add(&a, "Ho ca", &id2);
    add_dev(tm_find(&a, id1), "PUMP-AAAAAA", DEV_PUMP, 6001, "0123456789ABCDEF0123456789ABCDEF");
    add_dev(tm_find(&a, id1), "PH-BBBBBB", DEV_PH, 6002, "");                    /* chua ket noi */
    tm_delete(&a, id2);
    tm_add(&a, "Ho cuoi", NULL);                                                  /* id = 3 */
    CHECK(tm_save(&a, path) == TM_OK);

    tm_init(&b);
    CHECK(tm_load(&b, path) == TM_OK);
    CHECK(b.count == 2 && b.next_id == 4);
    CHECK(b.tanks[0].id == 1 && strcmp(b.tanks[0].name, "Ho tom so 1") == 0 && b.tanks[0].device_count == 2);
    CHECK(b.tanks[1].id == 3 && b.tanks[1].device_count == 0);
    tank_device_t *d = tank_find_device(&b.tanks[0], "PUMP-AAAAAA");
    CHECK(d && d->type == DEV_PUMP && d->port == 6001 && strcmp(d->ip, "192.168.1.10") == 0);
    CHECK(strcmp(d->token, "0123456789ABCDEF0123456789ABCDEF") == 0);
    CHECK(tank_find_device(&b.tanks[0], "PH-BBBBBB")->token[0] == '\0');
    int nid; tm_add(&b, "moi", &nid); CHECK(nid == 4);
    PASS("store: save then load restores tanks, devices, tokens, next id"); return 0;
}

static int t_missing_file(void) {
    tank_mgr_t m; tm_init(&m); tm_add(&m, "Giu nguyen", NULL);
    CHECK(tm_load(&m, "/nonexistent_dir/tanks.dat") == TM_ERR_IO);
    CHECK(m.count == 1);
    PASS("store: missing file -> TM_ERR_IO, data untouched"); return 0;
}

static int t_corrupt_files(void) {
    const char *bad[] = {
        "DEV|P-1|PUMP|1.2.3.4|6000|\n",                         /* DEV truoc TANK */
        "TANK|1|A\nTANK|1|B\n",                                 /* trung ID */
        "TANK|x|A\n",                                           /* ID khong phai so */
        "TANK|1|A\nDEV|P-1|TOASTER|1.2.3.4|6000|\n",            /* loai sai */
        "TANK|1|A\nDEV|P-1|PUMP|1.2.3.4|99999|\n",              /* cong sai */
        "TANK|1|A\nDEV|P-1|PUMP|1.2.3.4\n",                     /* thieu truong */
        "HELLO|world\n",                                        /* lenh la */
        "TANK|1|\n",                                            /* ten rong */
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        tank_mgr_t m; tm_init(&m); tm_add(&m, "Giu nguyen", NULL);
        write_file(bad[i]);
        CHECK(tm_load(&m, path) == TM_ERR_FORMAT);
        CHECK(m.count == 1 && strcmp(m.tanks[0].name, "Giu nguyen") == 0);   /* khong bi thay doi */
    }
    char longline[700]; memset(longline, 'x', sizeof longline - 1); longline[sizeof longline - 1] = '\0';
    tank_mgr_t m; tm_init(&m); write_file(longline);
    CHECK(tm_load(&m, path) == TM_ERR_FORMAT);
    PASS("store: 9 kinds of corrupt file rejected without touching data"); return 0;
}

static int t_empty_and_crlf(void) {
    tank_mgr_t m; tm_init(&m);
    write_file("");
    CHECK(tm_load(&m, path) == TM_OK && m.count == 0 && m.next_id == 1);
    write_file("TANK|2|Ho A\r\n\r\nDEV|S-1|SENSOR|10.0.0.5|7000|tok\r\n");
    CHECK(tm_load(&m, path) == TM_OK && m.count == 1 && m.tanks[0].device_count == 1 && m.next_id == 3);
    PASS("store: empty file, CRLF and blank lines"); return 0;
}

static int t_save_failure_keeps_old(void) {
    tank_mgr_t a; tm_init(&a); tm_add(&a, "Cu", NULL);
    CHECK(tm_save(&a, path) == TM_OK);
    tank_mgr_t b; tm_init(&b); tm_add(&b, "Moi", NULL);
    CHECK(tm_save(&b, "/nonexistent_dir/x.dat") == TM_ERR_IO);
    tank_mgr_t c; tm_init(&c);
    CHECK(tm_load(&c, path) == TM_OK && strcmp(c.tanks[0].name, "Cu") == 0);
    PASS("store: failed save does not damage existing file"); return 0;
}

int main(void) {
    snprintf(path, sizeof path, "/tmp/seafarm_tanks_%d.dat", (int)getpid());
    int (*tests[])(void) = { t_round_trip, t_missing_file, t_corrupt_files, t_empty_and_crlf, t_save_failure_keeps_old };
    total = (int)(sizeof tests / sizeof tests[0]);
    int rc = 0;
    for (int i = 0; i < total && !rc; i++) rc = tests[i]();
    remove(path);
    if (rc) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
