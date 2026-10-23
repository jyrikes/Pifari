#ifndef PIFARI_OPERATOR_ENGINE_H
#define PIFARI_OPERATOR_ENGINE_H
#include <stddef.h>
/* Kernel numérico fundido: y[i] = max(0, scale*x[i] + bias), opcionalmente paralelo. */
int pifari_kernel_affine_relu(const double *input, double *output, size_t count,
                              double scale, double bias, unsigned workers);
#endif
