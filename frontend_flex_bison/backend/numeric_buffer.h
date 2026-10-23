#ifndef PIFARI_NUMERIC_BUFFER_H
#define PIFARI_NUMERIC_BUFFER_H

#include <stddef.h>

/* Buffer contiguo tipado: double IEEE-754, alinhamento de 64 bytes. */
typedef struct {
    double *data;
    size_t length;
} PifariF64Buffer;

int pifari_f64_buffer_init(PifariF64Buffer *buffer, size_t length);
void pifari_f64_buffer_destroy(PifariF64Buffer *buffer);

#endif
