#ifndef PIFARI_GPU_CLIENT_H
#define PIFARI_GPU_CLIENT_H

#include <stddef.h>

/* Cliente do worker de GPU (tools/pifari_worker.py --stdio).
   O worker roda no lado Windows; aqui só trafega comando e resultado.
   Qualquer falha marca o cliente como indisponível e o chamador cai para CPU. */

int pifari_gpu_available(void);

/* y[i] = max(0, escala*x[i] + deslocamento). 0 = ok, -1 = falhou (use CPU). */
int pifari_gpu_affine_relu(const double *x, double *y, size_t n, double scale, double bias);

void pifari_gpu_shutdown(void);
const char *pifari_gpu_last_error(void);

#endif
