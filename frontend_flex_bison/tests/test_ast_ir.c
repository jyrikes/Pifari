#include "../backend/optimization/ast_ir.h"
#include "../backend/numeric_buffer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    Node *x = node_new("Name", "x", 1, 1);
    Node *two = node_new("Integer", "2", 1, 5);
    Node *mul = node_pair("Binary", "*", x, two, 1, 3);
    Node *one = node_new("Integer", "1", 1, 9);
    Node *expr = node_pair("Binary", "+", mul, one, 1, 7);
    PifariExprGraph graph;
    char error[160];
    assert(pifari_ast_expr_to_ir(expr, "x", &graph, error, sizeof(error)) == 0);
    double result = 0.0;
    assert(pifari_graph_eval(&graph, 4.0, &result) == 0);
    assert(result == 9.0);
    assert(pifari_graph_workspace_slots(&graph) < graph.count);
    assert(pifari_graph_map(&graph, (double[]){1, 2, 3}, (double[3]){0}, 0) == 0);
    pifari_graph_destroy(&graph);
    node_free(expr);

    Node *unknown = node_new("Name", "other", 1, 1);
    assert(pifari_ast_expr_to_ir(unknown, "x", &graph, error, sizeof(error)) != 0);
    assert(strstr(error, "variavel livre") != NULL);
    node_free(unknown);

    PifariF64Buffer buffer = {0};
    assert(pifari_f64_buffer_init(&buffer, 8) == 0);
    assert(buffer.length == 8 && buffer.data != NULL);
    assert(((uintptr_t)buffer.data % 64u) == 0);
    pifari_f64_buffer_destroy(&buffer);
    assert(buffer.data == NULL && buffer.length == 0);
    assert(pifari_f64_buffer_init(&buffer, SIZE_MAX) != 0);

    puts("AST -> IR / buffer F64: OK");
    return 0;
}
