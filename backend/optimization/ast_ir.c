#include "ast_ir.h"

#include <stdio.h>
#include <string.h>

static void set_error(char *dst, size_t cap, const char *message) {
    if (dst && cap)
        (void)snprintf(dst, cap, "%s", message);
}

static int add_node(PifariExprGraph *graph, PifariExprOp op, int left, int right, double constant) {
    return pifari_graph_add(graph, op, left, right, constant);
}

static int compile_expr(const Node *expr, const char *input_name, PifariExprGraph *graph,
                        char *error, size_t cap) {
    if (!expr) {
        set_error(error, cap, "expressao ausente");
        return -1;
    }

    if (expr->constant_folded) {
        double value =
            expr->constant_is_float ? expr->constant_float : (double)expr->constant_integer;
        return add_node(graph, PEX_CONST, -1, -1, value);
    }

    if (!strcmp(expr->kind, "Integer") || !strcmp(expr->kind, "Float")) {
        if (!expr->value) {
            set_error(error, cap, "literal numerico sem texto");
            return -1;
        }
        char *end = NULL;
        double value = !strcmp(expr->kind, "Integer") ? (double)strtoll(expr->value, &end, 10)
                                                      : strtod(expr->value, &end);
        if (!end || end == expr->value || *end != '\0') {
            set_error(error, cap, "literal numerico invalido");
            return -1;
        }
        return add_node(graph, PEX_CONST, -1, -1, value);
    }

    if (!strcmp(expr->kind, "Name")) {
        if (input_name && expr->value && !strcmp(expr->value, input_name))
            return add_node(graph, PEX_INPUT, -1, -1, 0.0);
        set_error(error, cap, "a expressao contem variavel livre nao suportada");
        return -1;
    }

    if (!strcmp(expr->kind, "Unary") && expr->count == 1 && expr->value &&
        !strcmp(expr->value, "-")) {
        int operand = compile_expr(expr->children[0], input_name, graph, error, cap);
        return operand < 0 ? -1 : add_node(graph, PEX_NEG, operand, -1, 0.0);
    }

    if (!strcmp(expr->kind, "Binary") && expr->count == 2 && expr->value) {
        int left = compile_expr(expr->children[0], input_name, graph, error, cap);
        if (left < 0)
            return -1;
        int right = compile_expr(expr->children[1], input_name, graph, error, cap);
        if (right < 0)
            return -1;

        PifariExprOp op;
        if (!strcmp(expr->value, "+"))
            op = PEX_ADD;
        else if (!strcmp(expr->value, "-"))
            op = PEX_SUB;
        else if (!strcmp(expr->value, "*"))
            op = PEX_MUL;
        else if (!strcmp(expr->value, "/"))
            op = PEX_DIV;
        else {
            set_error(error, cap, "operador ainda nao suportado no IR numerico");
            return -1;
        }
        return add_node(graph, op, left, right, 0.0);
    }

    set_error(error, cap, "expressao nao e aritmetica pura suportada pelo IR");
    return -1;
}

int pifari_ast_expr_to_ir(const Node *expr, const char *input_name, PifariExprGraph *graph,
                          char *error, size_t error_capacity) {
    if (!graph) {
        set_error(error, error_capacity, "grafo de destino ausente");
        return -1;
    }
    pifari_graph_init(graph);
    if (error && error_capacity)
        error[0] = '\0';

    int root = compile_expr(expr, input_name, graph, error, error_capacity);
    if (root < 0) {
        pifari_graph_destroy(graph);
        return -1;
    }
    graph->root = root;
    if (pifari_graph_optimize(graph) != 0) {
        pifari_graph_destroy(graph);
        set_error(error, error_capacity, "falha ao otimizar o grafo");
        return -1;
    }
    return 0;
}

int pifari_ast_function_to_ir(const Node *function, PifariExprGraph *graph, char *error,
                              size_t error_capacity) {
    if (!function ||
        (strcmp(function->kind, "Function") && strcmp(function->kind, "FunctionExpr")) ||
        function->count < 2 || !function->children[0] || function->children[0]->count != 1) {
        set_error(error, error_capacity, "funcao deve ter exatamente um parametro");
        return -1;
    }

    const Node *parameter = function->children[0]->children[0];
    const Node *body = function->children[1];
    const Node *expr = NULL;

    if (body && !strcmp(body->kind, "Block")) {
        if (body->count == 1 && !strcmp(body->children[0]->kind, "Return") &&
            body->children[0]->count == 1) {
            expr = body->children[0]->children[0];
        } else if (body->count == 1 && !strcmp(body->children[0]->kind, "ExpressionStatement") &&
                   body->children[0]->count == 1) {
            expr = body->children[0]->children[0];
        }
    } else if (body && !strcmp(body->kind, "Return") && body->count == 1) {
        expr = body->children[0];
    } else if (body && !strcmp(body->kind, "ExpressionStatement") && body->count == 1) {
        expr = body->children[0];
    }

    if (!expr) {
        set_error(error, error_capacity,
                  "corpo deve conter apenas um return ou resultado aritmetico");
        return -1;
    }
    return pifari_ast_expr_to_ir(expr, parameter->value, graph, error, error_capacity);
}

static int compile_indexed_expr(const Node *expr, const char *array_name, const char *index_name,
                                PifariExprGraph *graph, char *error, size_t cap) {
    if (!expr) {
        set_error(error, cap, "expressao indexada ausente");
        return -1;
    }
    if (expr->constant_folded) {
        double value =
            expr->constant_is_float ? expr->constant_float : (double)expr->constant_integer;
        return add_node(graph, PEX_CONST, -1, -1, value);
    }
    if (!strcmp(expr->kind, "Integer") || !strcmp(expr->kind, "Float")) {
        if (!expr->value) {
            set_error(error, cap, "literal numerico sem texto");
            return -1;
        }
        char *end = NULL;
        double value = !strcmp(expr->kind, "Integer") ? (double)strtoll(expr->value, &end, 10)
                                                      : strtod(expr->value, &end);
        if (!end || end == expr->value || *end != '\0') {
            set_error(error, cap, "literal numerico invalido");
            return -1;
        }
        return add_node(graph, PEX_CONST, -1, -1, value);
    }
    if (!strcmp(expr->kind, "Index") && expr->count == 2 && expr->children[0] &&
        expr->children[1] && !strcmp(expr->children[0]->kind, "Name") &&
        !strcmp(expr->children[1]->kind, "Name") && expr->children[0]->value &&
        expr->children[1]->value && !strcmp(expr->children[0]->value, array_name) &&
        !strcmp(expr->children[1]->value, index_name)) {
        return add_node(graph, PEX_INPUT, -1, -1, 0.0);
    }
    if (!strcmp(expr->kind, "Unary") && expr->count == 1 && expr->value &&
        !strcmp(expr->value, "-")) {
        int operand =
            compile_indexed_expr(expr->children[0], array_name, index_name, graph, error, cap);
        return operand < 0 ? -1 : add_node(graph, PEX_NEG, operand, -1, 0.0);
    }
    if (!strcmp(expr->kind, "Binary") && expr->count == 2 && expr->value) {
        int left =
            compile_indexed_expr(expr->children[0], array_name, index_name, graph, error, cap);
        if (left < 0)
            return -1;
        int right =
            compile_indexed_expr(expr->children[1], array_name, index_name, graph, error, cap);
        if (right < 0)
            return -1;
        PifariExprOp op;
        if (!strcmp(expr->value, "+"))
            op = PEX_ADD;
        else if (!strcmp(expr->value, "-"))
            op = PEX_SUB;
        else if (!strcmp(expr->value, "*"))
            op = PEX_MUL;
        else if (!strcmp(expr->value, "/"))
            op = PEX_DIV;
        else {
            set_error(error, cap, "operador nao suportado na funcao indexada");
            return -1;
        }
        return add_node(graph, op, left, right, 0.0);
    }
    set_error(error, cap, "suportado: aritmetica pura e acesso array[indice]");
    return -1;
}

int pifari_ast_indexed_function_to_ir(const Node *function, PifariExprGraph *graph, char *error,
                                      size_t error_capacity) {
    if (!function ||
        (strcmp(function->kind, "Function") && strcmp(function->kind, "FunctionExpr")) ||
        function->count < 2 || !function->children[0] || function->children[0]->count != 2) {
        set_error(error, error_capacity, "funcao indexada deve receber (array, indice)");
        return -1;
    }
    const Node *params = function->children[0];
    const Node *array_param = params->children[0], *index_param = params->children[1];
    if (!array_param || !array_param->value || !index_param || !index_param->value) {
        set_error(error, error_capacity, "parametros da funcao indexada invalidos");
        return -1;
    }
    const Node *body = function->children[1], *expr = NULL;
    if (body && !strcmp(body->kind, "Block") && body->count == 1 &&
        !strcmp(body->children[0]->kind, "Return") && body->children[0]->count == 1)
        expr = body->children[0]->children[0];
    else if (body && !strcmp(body->kind, "Return") && body->count == 1)
        expr = body->children[0];
    if (!expr) {
        set_error(error, error_capacity, "corpo indexado deve conter apenas return expr");
        return -1;
    }
    if (!graph) {
        set_error(error, error_capacity, "grafo de destino ausente");
        return -1;
    }
    pifari_graph_init(graph);
    if (error && error_capacity)
        error[0] = '\0';
    int root = compile_indexed_expr(expr, array_param->value, index_param->value, graph, error,
                                    error_capacity);
    if (root < 0) {
        pifari_graph_destroy(graph);
        return -1;
    }
    graph->root = root;
    if (pifari_graph_optimize(graph) != 0) {
        pifari_graph_destroy(graph);
        set_error(error, error_capacity, "falha ao otimizar o grafo indexado");
        return -1;
    }
    return 0;
}
