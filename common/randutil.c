#include "common/randutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int rand_hex(char *out, size_t nhex) {
    if (!out || nhex == 0 || nhex > 64) return -1;
    unsigned char buf[32];
    size_t nbytes = (nhex + 1) / 2;
    int ok = 0;

    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        ok = (fread(buf, 1, nbytes, f) == nbytes);
        fclose(f);
    }
    if (!ok) {   /* du phong: kem an toan hon, chi dung khi khong co urandom */
        static unsigned counter = 0;
        unsigned seed = (unsigned)time(NULL) ^ ((unsigned)getpid() << 8) ^ (++counter * 2654435761u);
        for (size_t i = 0; i < nbytes; i++) buf[i] = (unsigned char)(rand_r(&seed) >> 3);
    }
    static const char HEX[] = "0123456789ABCDEF";
    for (size_t i = 0; i < nhex; i++) {
        unsigned char b = buf[i / 2];
        out[i] = HEX[(i % 2 == 0) ? (b >> 4) : (b & 0x0F)];
    }
    out[nhex] = '\0';
    return 0;
}
