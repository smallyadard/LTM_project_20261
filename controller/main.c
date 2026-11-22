#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "controller/tank_mgr.h"

static tank_mgr_t mgr;

/* ===== [commit 10] BAT DAU khoi luu/doc file =====
 * O commit 9: thay ca khoi nay bang dong duy nhat:   static void save_or_warn(void) {}
 * va bo khoi [commit 10] DOC FILE trong main(). */
#include "controller/tank_store.h"
#define TANK_FILE "data/tanks.dat"

static void save_or_warn(void) {
    if (tm_save(&mgr, TANK_FILE) != TM_OK) printf("Canh bao: khong luu duoc %s\n", TANK_FILE);
}
/* ===== [commit 10] KET THUC khoi luu/doc file ===== */

/* Doc 1 dong tu ban phim (bo '\n'). 0: ok, -1: het du lieu (EOF). */
static int ask(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)size, stdin)) return -1;
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    else { int c; while ((c = getchar()) != '\n' && c != EOF) {} }   /* bo phan du neu nhap qua dai */
    return 0;
}

static int ask_int(const char *prompt, int *out) {
    char buf[32], *end;
    if (ask(prompt, buf, sizeof buf) < 0) return -1;
    long v = strtol(buf, &end, 10);
    if (buf[0] == '\0' || *end != '\0') { *out = -1; return 0; }
    *out = (int)v;
    return 0;
}

static const char *err_text(int rc) {
    switch (rc) {
    case TM_ERR_ARG:   return "Ten khong hop le (1-31 ky tu, khong chua '|').";
    case TM_ERR_DUP:   return "Ten ho da ton tai.";
    case TM_ERR_FULL:  return "Da dat so ho toi da.";
    case TM_ERR_NOENT: return "Khong tim thay ho nuoi.";
    default:           return "Loi khong xac dinh.";
    }
}

static void do_add(void) {
    char name[64]; int id;
    if (ask("Ten ho moi: ", name, sizeof name) < 0) return;
    int rc = tm_add(&mgr, name, &id);
    if (rc == TM_OK) { printf("Da them ho '%s' (ID %d).\n", name, id); save_or_warn(); }
    else printf("%s\n", err_text(rc));
}

static void do_rename(void) {
    int id; char name[64];
    if (ask_int("ID ho can doi ten: ", &id) < 0 || ask("Ten moi: ", name, sizeof name) < 0) return;
    int rc = tm_rename(&mgr, id, name);
    if (rc == TM_OK) { printf("Da doi ten.\n"); save_or_warn(); }
    else printf("%s\n", err_text(rc));
}

static void do_delete(void) {
    int id; char yn[8];
    if (ask_int("ID ho can xoa: ", &id) < 0) return;
    tank_t *t = tm_find(&mgr, id);
    if (!t) { printf("%s\n", err_text(TM_ERR_NOENT)); return; }
    printf("Xoa ho '%s' va %d thiet bi?\n", t->name, t->device_count);
    if (ask("Nhap 'y' de xac nhan: ", yn, sizeof yn) < 0 || strcmp(yn, "y") != 0) { printf("Da huy.\n"); return; }
    tm_delete(&mgr, id);
    printf("Da xoa.\n");
    save_or_warn();
}

static void do_devices(void) {
    int id;
    if (ask_int("ID ho: ", &id) < 0) return;
    tank_t *t = tm_find(&mgr, id);
    if (!t) { printf("%s\n", err_text(TM_ERR_NOENT)); return; }
    if (t->device_count == 0) { printf("Ho '%s' chua co thiet bi.\n", t->name); return; }
    printf("Ho '%s':\n%-16s %-8s %s\n", t->name, "ID", "LOAI", "DIA CHI");
    for (int i = 0; i < t->device_count; i++) {
        const tank_device_t *d = &t->devices[i];
        printf("%-16s %-8s %s:%d %s\n", d->id, device_type_to_str(d->type), d->ip, d->port,
               d->token[0] ? "(da ket noi)" : "");
    }
}

int main(void) {
    tm_init(&mgr);
    /* [commit 10] DOC FILE: bat dau */
    int rc = tm_load(&mgr, TANK_FILE);
    if (rc == TM_ERR_FORMAT) printf("Canh bao: file %s bi hong, bat dau danh sach rong.\n", TANK_FILE);
    /* [commit 10] DOC FILE: ket thuc */

    for (;;) {
        printf("\n=== QUAN LY HO NUOI ===\n"
               "1. Danh sach ho\n2. Them ho\n3. Doi ten ho\n4. Xoa ho\n5. Xem thiet bi trong ho\n0. Thoat\n");
        int choice;
        if (ask_int("Chon: ", &choice) < 0) break;                   /* EOF */
        switch (choice) {
        case 1: tm_print(&mgr, stdout); break;
        case 2: do_add(); break;
        case 3: do_rename(); break;
        case 4: do_delete(); break;
        case 5: do_devices(); break;
        case 0: return 0;
        default: printf("Lua chon khong hop le.\n");
        }
    }
    return 0;
}
