#ifndef PIFARI_TYPECHECK_H
#define PIFARI_TYPECHECK_H

#include "../frontend/ast.h"

/* Passada estática sobre a AST (docs/SPEC_SEMANTICA.md §6, etapa F1c).
   Não executa nada. Devolve o número de erros encontrados; imprime cada um em
   stderr no formato Pifari:<linha>:<coluna>: erro semântico: <mensagem>. */
int pifari_typecheck_program(const Node *program);

#endif
