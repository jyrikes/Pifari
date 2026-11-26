#ifndef PIFARI_REDUCE_H
#define PIFARI_REDUCE_H

#include "../../frontend/ast.h"

/* Kernel nativo de redução (etapa F3).
   Reconhece funções cujo corpo tem exatamente a forma

       acc = <inteiro>
       for i in range(<parametro>)
         acc = acc <op> <expressao inteira em i>
       return acc

   e compila isso para C com aritmética inteira checada. O contrato de
   equivalência (docs/SPEC_SEMANTICA.md §8) vale: só o tempo muda. Overflow
   continua sendo erro de execução e `range` mantém o limite de 1000000.
   Função fora dessa forma (float, escrita fora do laço, chamadas, índice)
   simplesmente não é elegível e segue pelo interpretador. */

typedef struct PifariReduceKernel PifariReduceKernel;

/* 0 = kernel pronto; != 0 = função não elegível ou compilação falhou.
   Nunca falha o programa: quem chama segue para o interpretador. */
int pifari_reduce_create(const Node *function, PifariReduceKernel **kernel_out);

void pifari_reduce_destroy(PifariReduceKernel *kernel);

/* Códigos de retorno: 0 ok, 1 overflow, 3 argumentos inválidos,
   4 limite de `range` violado (0..1000000). */
int pifari_reduce_run(const PifariReduceKernel *kernel, long long argument, long long *result);

#endif
