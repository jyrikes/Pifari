#define _POSIX_C_SOURCE 200112L
#include "numeric_buffer.h"

#include <stdint.h>
#include <stdlib.h>

int pifari_f64_buffer_init(PifariF64Buffer *buffer, size_t length) {
    if (!buffer || length > SIZE_MAX / sizeof(double))
        return -1;
    buffer->data = NULL;
    buffer->length = 0;
    if (length == 0)
        return 0;

    void *memory = NULL;
    if (posix_memalign(&memory, 64, length * sizeof(double)) != 0)
        return -1;
    buffer->data = memory;
    buffer->length = length;
    return 0;
}

void pifari_f64_buffer_destroy(PifariF64Buffer *buffer) {
    if (!buffer)
        return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
}
