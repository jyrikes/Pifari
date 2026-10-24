#define _GNU_SOURCE
#include "native_kernel.h"
#include "cache.h"

#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int (*NativeMapFn)(const double *, double *, size_t);

struct PifariNativeKernel {
    void *handle;
    NativeMapFn map;
};

typedef struct {
    const PifariNativeKernel *kernel;
    const double *input;
    double *output;
    size_t begin;
    size_t count;
    int status;
} NativeTask;

static int append_source(char *source, size_t capacity, size_t *used,
                         const char *format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(source + *used, capacity - *used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= capacity - *used) return -1;
    *used += (size_t)written;
    return 0;
}

int pifari_native_kernel_create(const PifariExprGraph *graph,
                                PifariNativeKernel **kernel_out) {
    if (!graph || !kernel_out || !graph->nodes || !graph->count ||
        graph->root < 0 || (size_t)graph->root >= graph->count ||
        graph->count > (SIZE_MAX - 4096) / 128) return -1;
    *kernel_out = NULL;

    size_t capacity = graph->count * 128 + 4096;
    char *source = calloc(capacity, 1);
    if (!source) return -1;
    size_t used = 0;
    int failed = append_source(source, capacity, &used,
        "#include <stddef.h>\n"
        "int pifari_native_map(const double * restrict input, "
        "double * restrict output, size_t count) {\n"
        "  if (count && (!input || !output)) return -1;\n"
        "  for (size_t i=0; i<count; ++i) {\n"
        "    double x=input[i];\n");

    for (size_t i = 0; i < graph->count && !failed; ++i) {
        const PifariExprNode *node = &graph->nodes[i];
        switch (node->op) {
            case PEX_INPUT:
                failed = append_source(source, capacity, &used,
                                       "    double v%zu=x;\n", i);
                break;
            case PEX_CONST:
                if (!isfinite(node->constant)) { failed = -1; break; }
                failed = append_source(source, capacity, &used,
                                       "    double v%zu=%.17g;\n",
                                       i, node->constant);
                break;
            case PEX_ALIAS:
                if (node->left < 0 || (size_t)node->left >= i) { failed = -1; break; }
                failed = append_source(source, capacity, &used,
                                       "    double v%zu=v%d;\n", i, node->left);
                break;
            case PEX_NEG:
            case PEX_MAX0:
                if (node->left < 0 || (size_t)node->left >= i) { failed = -1; break; }
                failed = append_source(source, capacity, &used,
                    node->op == PEX_NEG ? "    double v%zu=-v%d;\n" :
                                          "    double v%zu=v%d>0.0?v%d:0.0;\n",
                    i, node->left, node->left);
                break;
            case PEX_ADD:
            case PEX_SUB:
            case PEX_MUL:
            case PEX_DIV: {
                if (node->left < 0 || node->right < 0 ||
                    (size_t)node->left >= i || (size_t)node->right >= i) {
                    failed = -1; break;
                }
                if (node->op == PEX_DIV)
                    failed = append_source(source, capacity, &used,
                        "    if (v%d==0.0) return -2;\n", node->right);
                const char *op = node->op == PEX_ADD ? "+" :
                                 node->op == PEX_SUB ? "-" :
                                 node->op == PEX_MUL ? "*" : "/";
                if (!failed)
                    failed = append_source(source, capacity, &used,
                        "    double v%zu=v%d %s v%d;\n",
                        i, node->left, op, node->right);
                break;
            }
            default:
                failed = -1;
                break;
        }
    }

    if (!failed)
        failed = append_source(source, capacity, &used,
            "    output[i]=v%d;\n  }\n  return 0;\n}\n", graph->root);
    if (failed) { free(source); return -1; }

    char cache_dir[256], cache_path[320];
    int cache_ok = (pifari_cache_subdir("kernels", cache_dir, sizeof(cache_dir)) == 0);
    if (!cache_ok) {
        snprintf(cache_dir, sizeof(cache_dir), "/tmp/pifari_kernel_cache_%lu",
                 (unsigned long)getuid());
        cache_ok = (mkdir(cache_dir, 0700) == 0 || errno == EEXIST);
    }
    if (cache_ok) {
        struct stat st;
        if (stat(cache_dir, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid())
            cache_ok = 0;
        else
            (void)chmod(cache_dir, 0700);
    }
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < used; ++i) {
        hash ^= (unsigned char)source[i];
        hash *= UINT64_C(1099511628211);
    }
    snprintf(cache_path, sizeof(cache_path), "%s/%016llx.so",
             cache_dir, (unsigned long long)hash);

    void *handle = NULL;
    if (cache_ok && getenv("PIFARI_DISABLE_KERNEL_CACHE") == NULL &&
        access(cache_path, R_OK) == 0) {
        handle = dlopen(cache_path, RTLD_NOW | RTLD_LOCAL);
    }

    if (!handle) {
        char directory[] = "/tmp/pifari_native_XXXXXX";
        if (!mkdtemp(directory)) { free(source); return -1; }
        char source_path[256], library_path[256], command[768];
        snprintf(source_path, sizeof(source_path), "%s/kernel.c", directory);
        snprintf(library_path, sizeof(library_path), "%s/kernel.so", directory);

        FILE *file = fopen(source_path, "wb");
        if (!file) { free(source); rmdir(directory); return -1; }
        size_t written = fwrite(source, 1, used, file);
        int close_status = fclose(file);
        if (written != used || close_status != 0) {
            free(source);
            unlink(source_path); rmdir(directory); return -1;
        }

        int command_length = snprintf(command, sizeof(command),
            "gcc -std=c11 -O3 -march=native -fPIC -shared -o %s %s </dev/null >/dev/null 2>/dev/null",
            library_path, source_path);
        int sys_rc = (command_length < 0 || (size_t)command_length >= sizeof(command))
            ? -1 : system(command);
        unlink(source_path);
        if (sys_rc != 0) {
            free(source);
            unlink(library_path); rmdir(directory); return -1;
        }

        if (cache_ok && rename(library_path, cache_path) == 0) {
            rmdir(directory);
            handle = dlopen(cache_path, RTLD_NOW | RTLD_LOCAL);
        } else {
            handle = dlopen(library_path, RTLD_NOW | RTLD_LOCAL);
            unlink(library_path);
            rmdir(directory);
        }
    }
    free(source);
    if (!handle) return -1;

    void *symbol = dlsym(handle, "pifari_native_map");
    NativeMapFn map = NULL;
    if (!symbol || sizeof(map) != sizeof(symbol)) {
        dlclose(handle);
        return -1;
    }
    memcpy(&map, &symbol, sizeof(map));

    PifariNativeKernel *kernel = calloc(1, sizeof(*kernel));
    if (!kernel) { dlclose(handle); return -1; }
    kernel->handle = handle;
    kernel->map = map;
    *kernel_out = kernel;
    return 0;
}

void pifari_native_kernel_destroy(PifariNativeKernel *kernel) {
    if (!kernel) return;
    if (kernel->handle) dlclose(kernel->handle);
    free(kernel);
}

static void *run_native_task(void *opaque) {
    NativeTask *task = opaque;
    task->status = task->kernel->map(task->input + task->begin,
                                     task->output + task->begin,
                                     task->count);
    return NULL;
}

int pifari_native_kernel_map(const PifariNativeKernel *kernel,
                             const double *input, double *output,
                             size_t count, unsigned workers) {
    if (!kernel || !kernel->map || (count && (!input || !output))) return -1;
    if (!count) return 0;
    if (workers < 2 || count < 16384) return kernel->map(input, output, count);
    if (workers > 32) workers = 32;
    if ((size_t)workers > count) workers = (unsigned)count;

    pthread_t threads[32];
    NativeTask tasks[32];
    unsigned created = 0;
    size_t base = count / workers, extra = count % workers, position = 0;
    for (unsigned i = 0; i < workers; ++i) {
        size_t length = base + (i < extra);
        tasks[i] = (NativeTask){kernel, input, output, position, length, 0};
        position += length;
        if (pthread_create(&threads[i], NULL, run_native_task, &tasks[i]) != 0) break;
        ++created;
    }
    if (created != workers) {
        for (unsigned i = 0; i < created; ++i) pthread_join(threads[i], NULL);
        return kernel->map(input, output, count);
    }
    int status = 0;
    for (unsigned i = 0; i < workers; ++i) {
        pthread_join(threads[i], NULL);
        if (tasks[i].status && !status) status = tasks[i].status;
    }
    return status;
}
