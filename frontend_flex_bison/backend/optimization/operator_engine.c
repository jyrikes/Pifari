#define _POSIX_C_SOURCE 200809L
#include "operator_engine.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef struct {
    const double *in;
    double *out;
    size_t begin, end;
    double scale, bias;
} KernelTask;
static void *run_kernel(void *opaque) {
    KernelTask *t = (KernelTask *)opaque;
    const double *restrict input = t->in;
    double *restrict output = t->out;
    const double scale = t->scale, bias = t->bias;
/* O contrato do kernel garante intervalos de saída disjuntos e sem alias
   com a entrada; ivdep permite ao GCC vetorização SIMD quando disponível. */
#pragma GCC ivdep
    for (size_t i = t->begin; i < t->end; i++) {
        double value = scale * input[i] + bias;
        output[i] = value > 0.0 ? value : 0.0;
    }
    return NULL;
}
int pifari_kernel_affine_relu(const double *input, double *output, size_t count, double scale,
                              double bias, unsigned workers) {
    if ((count && (!input || !output)) || !isfinite(scale) || !isfinite(bias))
        return -1;
    if (!count)
        return 0;
    /* Para vetores pequenos, criar threads custa mais do que o kernel. */
    if (workers < 2 || count < 16384) {
        KernelTask t = {input, output, 0, count, scale, bias};
        run_kernel(&t);
        return 0;
    }
    if (workers > 32)
        workers = 32;
    if (workers > count)
        workers = (unsigned)count;
    pthread_t *threads = calloc(workers, sizeof(*threads));
    KernelTask *tasks = calloc(workers, sizeof(*tasks));
    if (!threads || !tasks) {
        free(threads);
        free(tasks);
        return -1;
    }
    unsigned created = 0;
    for (unsigned w = 0; w < workers; w++) {
        size_t begin = (count * w) / workers, end = (count * (w + 1)) / workers;
        tasks[w] = (KernelTask){input, output, begin, end, scale, bias};
        if (pthread_create(&threads[w], NULL, run_kernel, &tasks[w]) != 0)
            break;
        created++;
    }
    if (created != workers) {
        /* Nunca deixa uma região sem calcular em caso de falha na criação. */
        for (unsigned w = 0; w < created; w++)
            pthread_join(threads[w], NULL);
        KernelTask rest = {input, output, 0, count, scale, bias};
        run_kernel(&rest);
        free(threads);
        free(tasks);
        return 0;
    }
    for (unsigned w = 0; w < workers; w++)
        pthread_join(threads[w], NULL);
    free(threads);
    free(tasks);
    return 0;
}
