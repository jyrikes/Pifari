#ifndef PIFARI_PLAN_H
#define PIFARI_PLAN_H

/* Planos de execução (etapa F3). O plano escolhido nunca pode mudar o resultado
   observável (docs/SPEC_SEMANTICA.md §8): só tempo. */

typedef enum {
    PLAN_WALK = 0, /* AST interpretada: referência de comportamento */
    PLAN_BYTECODE = 1,
    PLAN_JIT = 2,
    PLAN_NATIVE = 3, /* kernel C fundido para função pura */
    PLAN_GPU = 4
} PifariPlan;

const char *pifari_plan_name(PifariPlan plan);

/* 0 = desligado (referência), 1 = conservador (padrão), 2 = agressivo.
   Lido de PIFARI_OPT na primeira chamada. */
int pifari_opt_level(void);
int pifari_opt_debug(void);

/* Log de decisão em stderr quando PIFARI_OPT_DEBUG=1. */
void pifari_plan_log(const char *what, const char *name, PifariPlan plan, const char *reason);

#endif
