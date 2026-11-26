#ifndef PIFARI_NATIVE_KERNEL_H
#define PIFARI_NATIVE_KERNEL_H

#include "optimizer.h"
#include <stddef.h>

typedef struct PifariNativeKernel PifariNativeKernel;

/* Gera C restrito a opcodes conhecidos, compila em shared object e carrega. */
int pifari_native_kernel_create(const PifariExprGraph *graph, PifariNativeKernel **kernel_out);
void pifari_native_kernel_destroy(PifariNativeKernel *kernel);
int pifari_native_kernel_map(const PifariNativeKernel *kernel, const double *input, double *output,
                             size_t count, unsigned workers);

#endif
