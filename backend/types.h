#ifndef PIFARI_TYPES_H
#define PIFARI_TYPES_H

/* Tipos declarados na linguagem (SPEC_SEMANTICA §2–§5).
   Espelham os tipos de valor do runtime, mas existem na AST e sobrevivem sem
   um Value — é isso que permite checar antes de executar (F1) e escolher plano
   de execução depois (F3). */

typedef enum {
    PT_ANY = 0,
    PT_NULL,
    PT_INT,
    PT_FLOAT,
    PT_BOOL,
    PT_STR,
    PT_LIST,
    PT_F64ARRAY,
    PT_RANGE,
    PT_TASK,
    PT_SHARED
} PifariType;

/* Nome canônico para mensagens ("int", "float", ...). */
const char *pifari_type_name(PifariType type);

/* Reconhece o nome escrito no programa. PT_ANY quando não é tipo conhecido. */
PifariType pifari_type_from_name(const char *name);

/* 1 quando o nome é um tipo da linguagem (usado no --check). */
int pifari_type_is_known(const char *name);

/* Compatibilidade de atribuição: 0 = incompatível, 1 = exato, 2 = promoção segura
   (int -> float). `annotated` é o tipo declarado, `actual` o valor atribuído. */
int pifari_type_assignable(PifariType annotated, PifariType actual);

#endif
