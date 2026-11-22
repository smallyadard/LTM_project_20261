#ifndef RANDUTIL_H
#define RANDUTIL_H
#include <stddef.h>

/* Ghi nhex ky tu hex ngau nhien + '\0' vao out (out phai >= nhex+1, nhex <= 64).
 * Doc tu /dev/urandom, du phong bang time/pid. 0: ok, -1: tham so sai. */
int rand_hex(char *out, size_t nhex);

#endif
