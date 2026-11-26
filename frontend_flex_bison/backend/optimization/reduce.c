#define _GNU_SOURCE
#include "reduce.h"
#include "cache.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Mesmo limite do builtin range() no runtime. */
#define PIFARI_REDUCE_MAX_RANGE 1000000LL
#define PIFARI_REDUCE_MAX_SOURCE (64u * 1024u)

typedef int (*ReduceFn)(const long long *, long long, long long *);

struct PifariReduceKernel {
    void *handle;
    ReduceFn run;
};

typedef struct {
    char *accumulator;
    char *loop_variable;
    char accumulator_op; /* '+', '-' ou '*' */
    long long initial;
    const Node *step; /* expressão do passo, ainda na AST */
} ReduceSpec;

static void set_error(char *dst, size_t cap, const char *message) {
    if (dst && cap)
        (void)snprintf(dst, cap, "%s", message);
}

static int is_name(const Node *node, const char *value) {
    return node && node->kind && !strcmp(node->kind, "Name") && node->value &&
           !strcmp(node->value, value);
}

/* Literal inteiro depois do dobramento de constantes. Float não entra: a
   redução nativa só cobre aritmética inteira para manter paridade exata. */
static int int_literal(const Node *node, long long *out) {
    if (!node || !out)
        return 0;
    if (node->constant_folded) {
        if (node->constant_is_float)
            return 0;
        *out = node->constant_integer;
        return 1;
    }
    if (!node->kind)
        return 0;
    if (!strcmp(node->kind, "Integer") && node->numeric_cached) {
        *out = node->integer_cache;
        return 1;
    }
    if (!strcmp(node->kind, "Unary") && node->count == 1 && node->value &&
        !strcmp(node->value, "-")) {
        long long inside = 0;
        if (!int_literal(node->children[0], &inside) || inside == LLONG_MIN)
            return 0;
        *out = -inside;
        return 1;
    }
    return 0;
}

/* Expressão do passo: apenas i, inteiros e + - * unários/binários. */
static int step_expr_ok(const Node *expr, const char *loop_variable) {
    if (!expr)
        return 0;
    if (expr->constant_folded)
        return !expr->constant_is_float;
    if (!expr->kind)
        return 0;
    if (!strcmp(expr->kind, "Integer"))
        return expr->numeric_cached != 0;
    if (!strcmp(expr->kind, "Name"))
        return is_name(expr, loop_variable);
    if (!strcmp(expr->kind, "Unary") && expr->count == 1 && expr->value &&
        !strcmp(expr->value, "-"))
        return step_expr_ok(expr->children[0], loop_variable);
    if (!strcmp(expr->kind, "Binary") && expr->count == 2 && expr->value) {
        if (strcmp(expr->value, "+") && strcmp(expr->value, "-") && strcmp(expr->value, "*"))
            return 0;
        return step_expr_ok(expr->children[0], loop_variable) &&
               step_expr_ok(expr->children[1], loop_variable);
    }
    return 0;
}

static int spec_from_function(const Node *function, ReduceSpec *spec, char *error,
                              size_t capacity) {
    if (!function || !function->kind ||
        (strcmp(function->kind, "Function") && strcmp(function->kind, "FunctionExpr"))) {
        set_error(error, capacity, "nao e uma definicao de funcao");
        return -1;
    }
    if (function->count < 2 || !function->children[0] || !function->children[1]) {
        set_error(error, capacity, "funcao sem parametros ou sem corpo");
        return -1;
    }
    /* Anotação de retorno (`-> float`) promoveria o inteiro no interpretador;
       o kernel devolve inteiro. Fora do formato para não divergir. */
    if (function->count > 2) {
        set_error(error, capacity, "reducao nao cobre anotacao de retorno");
        return -1;
    }
    const Node *params = function->children[0];
    const Node *body = function->children[1];
    if (params->count != 1 || !params->children[0] || !params->children[0]->kind ||
        strcmp(params->children[0]->kind, "Name") || !params->children[0]->value) {
        set_error(error, capacity, "reducao exige exatamente um parametro sem anotacao");
        return -1;
    }
    const char *parameter = params->children[0]->value;

    if (!body->kind || strcmp(body->kind, "Block") || body->count != 3) {
        set_error(error, capacity, "corpo deve ser: acc = inteiro; for; return");
        return -1;
    }
    const Node *init = body->children[0];
    const Node *loop = body->children[1];
    const Node *ret = body->children[2];

    if (!init->kind || strcmp(init->kind, "Assign") || !init->value || init->count != 1 ||
        !int_literal(init->children[0], &spec->initial)) {
        set_error(error, capacity, "primeira instrucao deve ser acc = <inteiro>");
        return -1;
    }
    const char *accumulator = init->value;

    if (!loop->kind || strcmp(loop->kind, "For") || !loop->value || loop->count < 2) {
        set_error(error, capacity, "segunda instrucao deve ser um for");
        return -1;
    }
    const Node *iterable = loop->children[0];
    const Node *loop_body = loop->children[1];
    if (!iterable || !iterable->kind || strcmp(iterable->kind, "Call") || !iterable->value ||
        strcmp(iterable->value, "range") || iterable->count != 1) {
        set_error(error, capacity, "for deve iterar sobre range(<parametro>)");
        return -1;
    }
    const Node *range_args = iterable->children[0];
    if (!range_args || range_args->count != 1 || !is_name(range_args->children[0], parameter)) {
        set_error(error, capacity, "range deve receber o parametro da funcao");
        return -1;
    }
    if (!loop_body || !loop_body->kind || strcmp(loop_body->kind, "Block") ||
        loop_body->count != 1) {
        set_error(error, capacity, "corpo do for deve ter uma unica instrucao");
        return -1;
    }
    const Node *step_statement = loop_body->children[0];
    if (!step_statement->kind || strcmp(step_statement->kind, "Assign") || !step_statement->value ||
        strcmp(step_statement->value, accumulator) || step_statement->count != 1) {
        set_error(error, capacity, "for deve acumular em acc");
        return -1;
    }
    const Node *step_expression = step_statement->children[0];
    if (!step_expression || !step_expression->kind || strcmp(step_expression->kind, "Binary") ||
        step_expression->count != 2 || !step_expression->value ||
        (strcmp(step_expression->value, "+") && strcmp(step_expression->value, "-") &&
         strcmp(step_expression->value, "*")) ||
        !is_name(step_expression->children[0], accumulator) ||
        !step_expr_ok(step_expression->children[1], loop->value)) {
        set_error(error, capacity, "passo deve ser acc = acc <op> expressao(i)");
        return -1;
    }

    if (!ret->kind || strcmp(ret->kind, "Return") || ret->count != 1 ||
        !is_name(ret->children[0], accumulator)) {
        set_error(error, capacity, "ultima instrucao deve ser return acc");
        return -1;
    }
    if (!strcmp(accumulator, parameter) || !strcmp(accumulator, loop->value) ||
        !strcmp(loop->value, parameter)) {
        set_error(error, capacity, "acumulador, iterador e parametro precisam ter nomes distintos");
        return -1;
    }

    spec->accumulator = pifari_strdup(accumulator);
    spec->loop_variable = pifari_strdup(loop->value);
    spec->accumulator_op = step_expression->value[0];
    spec->step = step_expression->children[1];
    if (!spec->accumulator || !spec->loop_variable) {
        set_error(error, capacity, "memoria insuficiente para descrever a reducao");
        return -1;
    }
    return 0;
}

typedef struct {
    char *data;
    size_t length, capacity;
    int failed;
} SrcBuf;

static void src_append(SrcBuf *buffer, const char *format, ...) {
    if (!buffer || buffer->failed)
        return;
    va_list args;
    va_start(args, format);
    int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if (needed < 0 || buffer->length + (size_t)needed + 1 > PIFARI_REDUCE_MAX_SOURCE) {
        buffer->failed = 1;
        return;
    }
    size_t wanted = buffer->length + (size_t)needed + 1;
    if (wanted > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 512;
        while (capacity < wanted)
            capacity *= 2;
        char *grown = realloc(buffer->data, capacity);
        if (!grown) {
            buffer->failed = 1;
            return;
        }
        buffer->data = grown;
        buffer->capacity = capacity;
    }
    va_start(args, format);
    vsnprintf(buffer->data + buffer->length, buffer->capacity - buffer->length, format, args);
    va_end(args);
    buffer->length += (size_t)needed;
}

static void emit_int(SrcBuf *source, long long value) {
    if (value == LLONG_MIN)
        src_append(source, "(-9223372036854775807LL - 1LL)");
    else
        src_append(source, "%lldLL", value);
}

static void emit_step(SrcBuf *source, const Node *expr, const char *loop_variable) {
    long long constant = 0;
    if (int_literal(expr, &constant)) {
        emit_int(source, constant);
        return;
    }
    if (!expr || !expr->kind) {
        source->failed = 1;
        return;
    }
    if (is_name(expr, loop_variable)) {
        src_append(source, "i");
        return;
    }
    if (!strcmp(expr->kind, "Unary") && expr->count == 1 && expr->value &&
        !strcmp(expr->value, "-")) {
        src_append(source, "pr_neg(");
        emit_step(source, expr->children[0], loop_variable);
        src_append(source, ",&err)");
        return;
    }
    if (!strcmp(expr->kind, "Binary") && expr->count == 2 && expr->value) {
        const char *helper = !strcmp(expr->value, "+")   ? "pr_add"
                             : !strcmp(expr->value, "-") ? "pr_sub"
                             : !strcmp(expr->value, "*") ? "pr_mul"
                                                         : NULL;
        if (!helper) {
            source->failed = 1;
            return;
        }
        src_append(source, "%s(", helper);
        emit_step(source, expr->children[0], loop_variable);
        src_append(source, ",");
        emit_step(source, expr->children[1], loop_variable);
        src_append(source, ",&err)");
        return;
    }
    source->failed = 1;
}

static int build_source(const ReduceSpec *spec, SrcBuf *source) {
    src_append(source,
               "#include <limits.h>\n"
               "static long long pr_add(long long a, long long b, int *e) { long long r; if "
               "(__builtin_add_overflow(a, b, &r)) { *e = 1; return 0; } return r; }\n"
               "static long long pr_sub(long long a, long long b, int *e) { long long r; if "
               "(__builtin_sub_overflow(a, b, &r)) { *e = 1; return 0; } return r; }\n"
               "static long long pr_mul(long long a, long long b, int *e) { long long r; if "
               "(__builtin_mul_overflow(a, b, &r)) { *e = 1; return 0; } return r; }\n"
               "static long long pr_neg(long long a, int *e) { if (a == LLONG_MIN) { *e = 1; "
               "return 0; } return -a; }\n"
               "int pifari_native_reduce(const long long *a, long long na, long long *out) {\n"
               "  if (!a || !out || na < 1) return 3;\n"
               "  long long n = a[0];\n"
               "  if (n < 0 || n > %lld) return 4;\n"
               "  long long acc = ",
               PIFARI_REDUCE_MAX_RANGE);
    emit_int(source, spec->initial);
    src_append(source,
               ";\n  int err = 0;\n  for (long long i = 0; i < n; ++i) {\n    long long step = ");
    emit_step(source, spec->step, spec->loop_variable);
    src_append(source,
               ";\n    if (err) return err;\n"
               "    acc = pr_%s(acc, step, &err);\n"
               "    if (err) return err;\n"
               "  }\n  *out = acc;\n  return 0;\n}\n",
               spec->accumulator_op == '+'   ? "add"
               : spec->accumulator_op == '-' ? "sub"
                                             : "mul");
    return source->failed ? -1 : 0;
}

/* Compila e carrega com o mesmo cache dos kernels fundidos (F4). */
static void *load_cached_kernel(const char *source, size_t length) {
    char directory[256], path[320];
    int cache_ok = (pifari_cache_subdir("kernels", directory, sizeof(directory)) == 0);
    if (!cache_ok) {
        snprintf(directory, sizeof(directory), "/tmp/pifari_reduce_cache_%lu",
                 (unsigned long)getuid());
        cache_ok = (mkdir(directory, 0700) == 0 || errno == EEXIST);
    }
    if (cache_ok) {
        struct stat st;
        if (stat(directory, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid())
            cache_ok = 0;
        else
            (void)chmod(directory, 0700);
    }
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < length; ++i) {
        hash ^= (unsigned char)source[i];
        hash *= UINT64_C(1099511628211);
    }
    snprintf(path, sizeof(path), "%s/%016llx.so", directory, (unsigned long long)hash);

    if (cache_ok && getenv("PIFARI_DISABLE_KERNEL_CACHE") == NULL && access(path, R_OK) == 0) {
        void *cached = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (cached)
            return cached;
    }

    char temporary[] = "/tmp/pifari_reduce_XXXXXX";
    if (!mkdtemp(temporary))
        return NULL;
    char source_path[256], library_path[256], command[768];
    snprintf(source_path, sizeof(source_path), "%s/kernel.c", temporary);
    snprintf(library_path, sizeof(library_path), "%s/kernel.so", temporary);

    FILE *file = fopen(source_path, "wb");
    if (!file) {
        rmdir(temporary);
        return NULL;
    }
    size_t written = fwrite(source, 1, length, file);
    int close_status = fclose(file);
    if (written != length || close_status != 0) {
        unlink(source_path);
        rmdir(temporary);
        return NULL;
    }
    int command_length = snprintf(
        command, sizeof(command),
        "gcc -std=c11 -O3 -march=native -fPIC -shared -o %s %s </dev/null >/dev/null 2>/dev/null",
        library_path, source_path);
    int status =
        (command_length < 0 || (size_t)command_length >= sizeof(command)) ? -1 : system(command);
    unlink(source_path);
    if (status != 0) {
        unlink(library_path);
        rmdir(temporary);
        return NULL;
    }

    void *handle = NULL;
    if (cache_ok && rename(library_path, path) == 0) {
        rmdir(temporary);
        handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    } else {
        handle = dlopen(library_path, RTLD_NOW | RTLD_LOCAL);
        unlink(library_path);
        rmdir(temporary);
    }
    return handle;
}

int pifari_reduce_create(const Node *function, PifariReduceKernel **kernel_out) {
    if (!kernel_out)
        return -1;
    *kernel_out = NULL;
    ReduceSpec spec = {0};
    char error[160];
    if (spec_from_function(function, &spec, error, sizeof(error)) != 0) {
        free(spec.accumulator);
        free(spec.loop_variable);
        return -1;
    }

    SrcBuf source = {0};
    int built = build_source(&spec, &source);
    free(spec.accumulator);
    free(spec.loop_variable);
    if (built != 0 || !source.data) {
        free(source.data);
        return -1;
    }

    void *handle = load_cached_kernel(source.data, source.length);
    free(source.data);
    if (!handle)
        return -1;

    void *symbol = dlsym(handle, "pifari_native_reduce");
    ReduceFn run = NULL;
    if (!symbol || sizeof(run) != sizeof(symbol)) {
        dlclose(handle);
        return -1;
    }
    memcpy(&run, &symbol, sizeof(run));

    PifariReduceKernel *kernel = calloc(1, sizeof(*kernel));
    if (!kernel) {
        dlclose(handle);
        return -1;
    }
    kernel->handle = handle;
    kernel->run = run;
    *kernel_out = kernel;
    return 0;
}

void pifari_reduce_destroy(PifariReduceKernel *kernel) {
    if (!kernel)
        return;
    if (kernel->handle)
        dlclose(kernel->handle);
    free(kernel);
}

int pifari_reduce_run(const PifariReduceKernel *kernel, long long argument, long long *result) {
    if (!kernel || !kernel->run || !result)
        return 3;
    return kernel->run(&argument, 1, result);
}
