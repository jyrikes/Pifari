#ifndef PIFARI_REVERSIBLE_H
#define PIFARI_REVERSIBLE_H

#include <stddef.h>
#include <stdint.h>

/* Transformacao elementar reversivel sobre palavras de 64 bits. */
typedef enum {
    PIFARI_GATE_XOR_MASK,
    PIFARI_GATE_NOT,
    PIFARI_GATE_SWAP
} PifariGateKind;

typedef struct {
    PifariGateKind kind;
    size_t a;
    size_t b;
    uint64_t mask;
} PifariGate;

typedef struct {
    PifariGate *gates;
    size_t count;
    size_t capacity;
} PifariReversibleProgram;

void pifari_reversible_init(PifariReversibleProgram *program);
void pifari_reversible_destroy(PifariReversibleProgram *program);
int pifari_reversible_add_xor(PifariReversibleProgram *program,
                              size_t target, uint64_t mask);
int pifari_reversible_add_not(PifariReversibleProgram *program, size_t target);
int pifari_reversible_add_swap(PifariReversibleProgram *program,
                               size_t left, size_t right);
int pifari_reversible_apply(const PifariReversibleProgram *program,
                            uint64_t *state, size_t count);
int pifari_reversible_uncompute(const PifariReversibleProgram *program,
                                uint64_t *state, size_t count);

/* Compatibilidade com a transformacao XOR anterior. */
void pifari_xor_transform(uint64_t *values, size_t count, uint64_t mask);

#endif
