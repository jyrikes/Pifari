#include "../backend/native_kernel.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    PifariExprGraph graph;
    pifari_graph_init(&graph);
    int input = pifari_graph_add(&graph, PEX_INPUT, -1, -1, 0.0);
    int two = pifari_graph_add(&graph, PEX_CONST, -1, -1, 2.0);
    int mul = pifari_graph_add(&graph, PEX_MUL, input, two, 0.0);
    int one = pifari_graph_add(&graph, PEX_CONST, -1, -1, 1.0);
    graph.root = pifari_graph_add(&graph, PEX_ADD, mul, one, 0.0);

    PifariNativeKernel *kernel = NULL;
    assert(pifari_native_kernel_create(&graph, &kernel) == 0);
    double input_values[] = {1, 2, 3, 4};
    double output_values[4] = {0};
    assert(pifari_native_kernel_map(kernel, input_values, output_values, 4, 1) == 0);
    for (size_t i = 0; i < 4; ++i) assert(output_values[i] == input_values[i] * 2 + 1);
    size_t length = 20000;
    double *large_input = malloc(length * sizeof(*large_input));
    double *large_output = malloc(length * sizeof(*large_output));
    assert(large_input && large_output);
    for (size_t i = 0; i < length; ++i) large_input[i] = (double)i - 10000.0;
    assert(pifari_native_kernel_map(kernel, large_input, large_output, length, 4) == 0);
    for (size_t i = 0; i < length; ++i)
        assert(large_output[i] == large_input[i] * 2.0 + 1.0);
    free(large_input);
    free(large_output);
    pifari_native_kernel_destroy(kernel);
    pifari_graph_destroy(&graph);

    pifari_graph_init(&graph);
    input = pifari_graph_add(&graph, PEX_INPUT, -1, -1, 0.0);
    int zero = pifari_graph_add(&graph, PEX_CONST, -1, -1, 0.0);
    graph.root = pifari_graph_add(&graph, PEX_DIV, input, zero, 0.0);
    assert(pifari_native_kernel_create(&graph, &kernel) == 0);
    assert(pifari_native_kernel_map(kernel, input_values, output_values, 4, 1) == -2);
    pifari_native_kernel_destroy(kernel);
    pifari_graph_destroy(&graph);
    puts("native fused kernel serial/parallel/error: OK");
    return 0;
}
