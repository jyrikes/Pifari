#ifndef PIFARI_EFFECTS_H
#define PIFARI_EFFECTS_H

/* Classificação de efeitos (docs/SPEC_SEMANTICA.md, spec v3 §384).
   Quanto maior o valor, mais forte o efeito:
   PURE  - só lê argumentos e calcula
   READ  - lê estado do ambiente (relógio, dispositivos, célula compartilhada)
   WRITE - altera estado (array_set, célula compartilhada, configuração de execução)
   IO    - fala com o mundo (print, MCP, tarefas, disco)
   A otimização só pode especializar funções PURE (e, com cuidado, READ). */

typedef enum {
 EFFECT_PURE = 0,
 EFFECT_READ = 1,
 EFFECT_WRITE = 2,
 EFFECT_IO = 4
} PifariEffect;

/* Efeito de um builtin pelo nome. Nome desconhecido devolve EFFECT_IO
   (conservador: não provamos pureza de algo que não conhecemos). */
PifariEffect pifari_builtin_effect(const char *name);

const char *pifari_effect_name(PifariEffect effect);

#endif
