#include "reversible.h"

#include <stdint.h>
#include <stdlib.h>

void pifari_xor_transform(uint64_t *values, size_t count, uint64_t mask) {
    if (!values)
        return;
    for (size_t i = 0; i < count; ++i)
        values[i] ^= mask;
}

void pifari_reversible_init(PifariReversibleProgram *program) {
    if (program)
        *program = (PifariReversibleProgram){0};
}

void pifari_reversible_destroy(PifariReversibleProgram *program) {
    if (!program)
        return;
    free(program->gates);
    *program = (PifariReversibleProgram){0};
}

static int append_gate(PifariReversibleProgram *program, PifariGate gate) {
    if (!program || program->count == SIZE_MAX)
        return -1;
    if (program->count == program->capacity) {
        size_t capacity = program->capacity ? program->capacity * 2 : 8;
        if (capacity < program->capacity || capacity > SIZE_MAX / sizeof(*program->gates))
            return -1;
        void *memory = realloc(program->gates, capacity * sizeof(*program->gates));
        if (!memory)
            return -1;
        program->gates = memory;
        program->capacity = capacity;
    }
    program->gates[program->count++] = gate;
    return 0;
}

int pifari_reversible_add_xor(PifariReversibleProgram *program, size_t target, uint64_t mask) {
    return append_gate(program, (PifariGate){PIFARI_GATE_XOR_MASK, target, 0, mask});
}

int pifari_reversible_add_not(PifariReversibleProgram *program, size_t target) {
    return append_gate(program, (PifariGate){PIFARI_GATE_NOT, target, 0, 0});
}

int pifari_reversible_add_swap(PifariReversibleProgram *program, size_t left, size_t right) {
    return append_gate(program, (PifariGate){PIFARI_GATE_SWAP, left, right, 0});
}

static int apply_gate(PifariGate gate, uint64_t *state, size_t count) {
    if (gate.a >= count)
        return -1;
    switch (gate.kind) {
        case PIFARI_GATE_XOR_MASK:
            state[gate.a] ^= gate.mask;
            return 0;
        case PIFARI_GATE_NOT:
            state[gate.a] = ~state[gate.a];
            return 0;
        case PIFARI_GATE_SWAP:
            if (gate.b >= count)
                return -1;
            if (gate.a != gate.b) {
                uint64_t temp = state[gate.a];
                state[gate.a] = state[gate.b];
                state[gate.b] = temp;
            }
            return 0;
    }
    return -1;
}

static int validate_program(const PifariReversibleProgram *program, size_t count) {
    if (!program || (count && !program->gates))
        return -1;
    for (size_t i = 0; i < program->count; ++i) {
        const PifariGate gate = program->gates[i];
        if (gate.a >= count)
            return -1;
        if (gate.kind == PIFARI_GATE_SWAP && gate.b >= count)
            return -1;
        if (gate.kind < PIFARI_GATE_XOR_MASK || gate.kind > PIFARI_GATE_SWAP)
            return -1;
    }
    return 0;
}

int pifari_reversible_apply(const PifariReversibleProgram *program, uint64_t *state, size_t count) {
    if ((count && !state) || validate_program(program, count) != 0)
        return -1;
    for (size_t i = 0; i < program->count; ++i)
        (void)apply_gate(program->gates[i], state, count);
    return 0;
}

int pifari_reversible_uncompute(const PifariReversibleProgram *program, uint64_t *state,
                                size_t count) {
    if ((count && !state) || validate_program(program, count) != 0)
        return -1;
    for (size_t i = program->count; i > 0; --i)
        (void)apply_gate(program->gates[i - 1], state, count);
    return 0;
}
