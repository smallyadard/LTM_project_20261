#include <stdio.h>
#include <string.h>
#include "controller/tank_mgr.h"

static int passed = 0, total = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)
#define PASS(name) do { printf("  ok  %s\n", name); passed++; } while (0)

static int t_add_list(void) {
    tank_mgr_t m; tm_init(&m); int a, b;
    CHECK(tm_add(&m, "Ho tom 1", &a) == TM_OK && tm_add(&m, "Ho ca", &b) == TM_OK);
    CHECK(a == 1 && b == 2 && m.count == 2);
    CHECK(strcmp(tm_find(&m, 2)->name, "Ho ca") == 0 && tm_find(&m, 99) == NULL);
    CHECK(tm_add(&m, "Ho moi", NULL) == TM_OK);
    PASS("tank_mgr: add, ids increase, find"); return 0;
}

static int t_invalid_and_duplicate(void) {
    tank_mgr_t m; tm_init(&m);
    char longname[64]; memset(longname, 'a', 40); longname[40] = '\0';
    CHECK(tm_add(&m, "", NULL) == TM_ERR_ARG && tm_add(&m, NULL, NULL) == TM_ERR_ARG);
    CHECK(tm_add(&m, "a|b", NULL) == TM_ERR_ARG && tm_add(&m, "a\nb", NULL) == TM_ERR_ARG);
    CHECK(tm_add(&m, longname, NULL) == TM_ERR_ARG);
    CHECK(tm_add(&m, "X", NULL) == TM_OK && tm_add(&m, "X", NULL) == TM_ERR_DUP);
    CHECK(m.count == 1);
    PASS("tank_mgr: reject bad/duplicate names"); return 0;
}

static int t_full(void) {
    tank_mgr_t m; tm_init(&m); char n[16];
    for (int i = 0; i < MAX_TANKS; i++) { snprintf(n, sizeof n, "H%d", i); CHECK(tm_add(&m, n, NULL) == TM_OK); }
    CHECK(tm_add(&m, "extra", NULL) == TM_ERR_FULL);
    PASS("tank_mgr: full list rejects new tank"); return 0;
}

static int t_rename(void) {
    tank_mgr_t m; tm_init(&m); int a, b;
    tm_add(&m, "A", &a); tm_add(&m, "B", &b);
    CHECK(tm_rename(&m, a, "A2") == TM_OK && strcmp(tm_find(&m, a)->name, "A2") == 0);
    CHECK(tm_rename(&m, a, "A2") == TM_OK);                 /* giu nguyen ten cua chinh no */
    CHECK(tm_rename(&m, a, "B") == TM_ERR_DUP);
    CHECK(tm_rename(&m, a, "x|y") == TM_ERR_ARG);
    CHECK(tm_rename(&m, 77, "Z") == TM_ERR_NOENT);
    CHECK(strcmp(tm_find(&m, a)->name, "A2") == 0);          /* khong bi doi khi loi */
    PASS("tank_mgr: rename rules"); return 0;
}

static int t_delete(void) {
    tank_mgr_t m; tm_init(&m); int a, b, c;
    tm_add(&m, "A", &a); tm_add(&m, "B", &b); tm_add(&m, "C", &c);
    tank_device_t d; memset(&d, 0, sizeof d); snprintf(d.id, sizeof d.id, "P-1"); d.type = DEV_PUMP;
    tank_add_device(tm_find(&m, b), &d);
    CHECK(tm_delete(&m, b) == TM_OK && m.count == 2);
    CHECK(m.tanks[0].id == a && m.tanks[1].id == c && tm_find(&m, b) == NULL);
    CHECK(tm_delete(&m, b) == TM_ERR_NOENT);
    int n; tm_add(&m, "D", &n);
    CHECK(n == c + 1);                                       /* ID khong tai su dung */
    PASS("tank_mgr: delete keeps order, ids never reused"); return 0;
}

static int t_print(void) {
    tank_mgr_t m; tm_init(&m); char buf[256]; FILE *f;
    f = fmemopen(buf, sizeof buf, "w"); tm_print(&m, f); fclose(f);
    CHECK(strstr(buf, "chua co") != NULL);
    tm_add(&m, "Ho 1", NULL);
    f = fmemopen(buf, sizeof buf, "w"); tm_print(&m, f); fclose(f);
    CHECK(strstr(buf, "Ho 1") != NULL);
    PASS("tank_mgr: print empty and non-empty"); return 0;
}

int main(void) {
    int (*tests[])(void) = { t_add_list, t_invalid_and_duplicate, t_full, t_rename, t_delete, t_print };
    total = (int)(sizeof tests / sizeof tests[0]);
    for (int i = 0; i < total; i++) if (tests[i]()) return 1;
    printf("%d/%d tests passed\n", passed, total);
    return 0;
}
