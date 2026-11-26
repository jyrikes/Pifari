#ifndef PIFARI_AST_IR_H
#define PIFARI_AST_IR_H

#include "../../frontend/ast.h"
#include "optimizer.h"

/* Compile uma expressão aritmética pura da AST para o IR de grafo.
 * A única variável livre permitida é input_name, representada por PEX_INPUT.
 * Em falha, retorna -1 e escreve uma mensagem curta em error. */
int pifari_ast_expr_to_ir(const Node *expr, const char *input_name, PifariExprGraph *graph,
                          char *error, size_t error_capacity);

/* Aceita o corpo simples de uma função: return expr ou expr como resultado
 * implícito. O parâmetro único é usado como entrada do grafo. */
int pifari_ast_function_to_ir(const Node *function, PifariExprGraph *graph, char *error,
                              size_t error_capacity);

/* Compila função de dois parâmetros (array, índice) cuja expressão pode conter
 * array[index]. Esse acesso é reduzido a PEX_INPUT e vira input[i] no kernel C. */
int pifari_ast_indexed_function_to_ir(const Node *function, PifariExprGraph *graph, char *error,
                                      size_t error_capacity);

#endif
