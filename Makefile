CC := gcc
FLEX := flex
BISON := bison
BUILD_DIR := build
GEN_DIR := $(BUILD_DIR)/generated
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
PARSER_C := $(GEN_DIR)/parser.tab.c
PARSER_H := $(GEN_DIR)/parser.tab.h
LEX_C := $(GEN_DIR)/lex.yy.c
CFLAGS := -std=c11 -O3 -flto -march=native -Wall -Wextra -Wpedantic -I. -Ifrontend -Ibackend -Ibackend/optimization -Ibackend/integrations -I$(GEN_DIR)

BACKEND_MODULES := \
    runtime integrations/mcp_gateway integrations/mcp_server integrations/agent_runtime \
    optimization/optimizer optimization/ast_ir numeric_buffer dirac \
    optimization/native_kernel optimization/operator_engine optimization/memory_plan reversible \
    integrations/gpu_client optimization/cache optimization/reduce optimization/plan \
    effects typecheck types integrations/devices
FRONTEND_OBJECTS := $(OBJ_DIR)/frontend/parser.o $(OBJ_DIR)/frontend/lexer.o
BACKEND_OBJECTS := $(addprefix $(OBJ_DIR)/backend/,$(addsuffix .o,$(BACKEND_MODULES)))
DEPFILES := $(FRONTEND_OBJECTS:.o=.d) $(BACKEND_OBJECTS:.o=.d)

.PHONY: all clean test test-core test-conc test-fuzz benchmark test-gpu test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
all: pifari

$(GEN_DIR) $(OBJ_DIR)/frontend $(OBJ_DIR)/backend $(BIN_DIR):
	mkdir -p $@

$(PARSER_C) $(PARSER_H) &: frontend/parser.y frontend/ast.h backend/runtime.h | $(GEN_DIR)
	$(BISON) -Wall -d -o $(PARSER_C) frontend/parser.y

$(LEX_C): frontend/lexer.l $(PARSER_H) | $(GEN_DIR)
	$(FLEX) -o $(LEX_C) frontend/lexer.l

$(OBJ_DIR)/frontend/parser.o: $(PARSER_C) $(PARSER_H) frontend/ast.h backend/runtime.h | $(OBJ_DIR)/frontend
	$(CC) $(CFLAGS) -pthread -MMD -MP -MF $(@:.o=.d) -MT $@ -c $(PARSER_C) -o $@

$(OBJ_DIR)/frontend/lexer.o: $(LEX_C) $(PARSER_H) frontend/ast.h | $(OBJ_DIR)/frontend
	$(CC) $(CFLAGS) -pthread -MMD -MP -MF $(@:.o=.d) -MT $@ -c $(LEX_C) -o $@

$(OBJ_DIR)/backend/%.o: backend/%.c | $(OBJ_DIR)/backend
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -pthread -MMD -MP -MF $(@:.o=.d) -MT $@ -c $< -o $@

-include $(DEPFILES)

pifari: $(FRONTEND_OBJECTS) $(BACKEND_OBJECTS)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm -ldl

test_optimizer: $(BIN_DIR)/test_optimizer
test_ast_ir: $(BIN_DIR)/test_ast_ir
test_dirac: $(BIN_DIR)/test_dirac
test_native_kernel: $(BIN_DIR)/test_native_kernel
test_agent_runtime: $(BIN_DIR)/test_agent_runtime

$(BIN_DIR)/test_optimizer: tests/test_optimizer.c $(OBJ_DIR)/backend/optimization/optimizer.o $(OBJ_DIR)/backend/optimization/operator_engine.o $(OBJ_DIR)/backend/optimization/memory_plan.o $(OBJ_DIR)/backend/reversible.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm

$(BIN_DIR)/test_ast_ir: tests/test_ast_ir.c $(OBJ_DIR)/backend/optimization/ast_ir.o $(OBJ_DIR)/backend/optimization/optimizer.o $(OBJ_DIR)/backend/numeric_buffer.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm

$(BIN_DIR)/test_dirac: tests/test_dirac.c $(OBJ_DIR)/backend/dirac.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm

$(BIN_DIR)/test_native_kernel: tests/test_native_kernel.c $(OBJ_DIR)/backend/optimization/native_kernel.o $(OBJ_DIR)/backend/optimization/optimizer.o $(OBJ_DIR)/backend/optimization/cache.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm -ldl

$(BIN_DIR)/test_agent_runtime: tests/test_agent_runtime.c $(OBJ_DIR)/backend/integrations/agent_runtime.o $(OBJ_DIR)/backend/integrations/mcp_gateway.o $(OBJ_DIR)/backend/integrations/mcp_server.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm
test: pifari test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
	./$(BIN_DIR)/test_optimizer
	./$(BIN_DIR)/test_ast_ir
	./$(BIN_DIR)/test_dirac
	./$(BIN_DIR)/test_native_kernel
	./$(BIN_DIR)/test_agent_runtime
	./pifari examples/runtime_map_fused.pif
	./pifari examples/runtime_fusion.pif
	./pifari examples/runtime_numeric_tools.pif
	./pifari examples/teste_range_preguicoso.pif
	./pifari examples/runtime_lista.pif
	./pifari examples/runtime_operadores.pif
	./pifari examples/runtime_vm.pif
	./pifari examples/runtime_vm_index_compare.pif
	./pifari examples/runtime_fused_chain.pif
	./pifari examples/typed_arrays.pif
	./pifari examples/runtime_indexed_native.pif
	./pifari examples/runtime_matriz.pif
	./pifari examples/runtime_matmul.pif
	./pifari examples/runtime_otimizacoes.pif
	./pifari examples/runtime_slots_pool.pif
	./pifari examples/runtime_fluxo.pif
	./pifari examples/fluxo_sequencial.pif
	./pifari examples/fluxo_paralelo.pif
	./pifari examples/fluxo_coleta.pif
	./pifari examples/fluxo_sincronizacao.pif
	./pifari examples/conc_fluxos.pif
	./pifari examples/conc_processos.pif
	./pifari examples/conc_tarefas.pif
	./pifari examples/funcoes.pif
	./pifari examples/nova_sintaxe_fluxo.pif
	./pifari examples/paralelismo_assincrono.pif
	./pifari examples/mlp_xor.pif
	./pifari examples/mlp_xor_fluxo.pif
	./pifari examples/prototipo_multiagente_mcp.pif
	./pifari examples/teste_cliente_servidor_mcp_pifari.pif
	./pifari examples/reducao_nativa.pif
	./pifari examples/orquestrador_reels_aistudio.pif
test-fuzz: pifari
	python3 tests/fuzz_parser.py
benchmark: pifari
	python3 examples/benchmarks/benchmark_cpu.py
	python3 examples/benchmarks/benchmark_matrizes.py
	./pifari examples/benchmarks/benchmark_fused_chain.pif
	python3 examples/benchmarks/benchmark_indexed_vs_python.py
	python3 examples/benchmarks/benchmark_mlp_xor.py
	./pifari examples/carga_fluxo_puro.pif
clean:
	rm -rf $(BUILD_DIR)
	rm -f pifari parser.tab.c parser.tab.h lex.yy.c parser.output
	rm -f frontend/*.o backend/*.o
	rm -f test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
test-gpu: pifari
	./pifari examples/gpu_dispositivos.pif

test-core: test-errors pifari test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
	./$(BIN_DIR)/test_optimizer
	./$(BIN_DIR)/test_ast_ir
	./$(BIN_DIR)/test_dirac
	./$(BIN_DIR)/test_native_kernel
	./$(BIN_DIR)/test_agent_runtime
	./pifari examples/runtime_map_fused.pif
	./pifari examples/runtime_fusion.pif
	./pifari examples/runtime_numeric_tools.pif
	./pifari examples/teste_range_preguicoso.pif
	./pifari examples/runtime_lista.pif
	./pifari examples/runtime_operadores.pif
	./pifari examples/runtime_vm.pif
	./pifari examples/runtime_vm_index_compare.pif
	./pifari examples/runtime_fused_chain.pif
	./pifari examples/typed_arrays.pif
	./pifari examples/runtime_indexed_native.pif
	./pifari examples/runtime_matriz.pif
	./pifari examples/runtime_matmul.pif
	./pifari examples/runtime_otimizacoes.pif
	./pifari examples/runtime_slots_pool.pif
	./pifari examples/runtime_fluxo.pif
	./pifari examples/fluxo_sequencial.pif
	./pifari examples/fluxo_paralelo.pif
	./pifari examples/fluxo_coleta.pif
	./pifari examples/fluxo_sincronizacao.pif
	./pifari examples/conc_fluxos.pif
	./pifari examples/conc_processos.pif
	./pifari examples/conc_tarefas.pif
	./pifari examples/funcoes.pif
	./pifari examples/nova_sintaxe_fluxo.pif
	./pifari examples/paralelismo_assincrono.pif
	./pifari examples/mlp_xor.pif
	./pifari examples/mlp_xor_fluxo.pif
	./pifari examples/prototipo_multiagente_mcp.pif
	./pifari examples/teste_cliente_servidor_mcp_pifari.pif
	./pifari examples/reducao_nativa.pif

test-conc: pifari
	./pifari examples/conc_fluxos.pif
	./pifari examples/conc_processos.pif
	./pifari examples/conc_tarefas.pif
	./pifari examples/conc_shared.pif
	./pifari examples/tipos.pif
	./pifari examples/efeitos.pif

check-core: pifari
	./pifari --check examples/gpu_dispositivos.pif
	./pifari --check examples/carga_fluxo_puro.pif
	./pifari --check examples/benchmarks/benchmark_fused_chain.pif
	./pifari --check examples/orquestrador_reels_aistudio.pif
	./pifari --check examples/teste_cliente_servidor_mcp_pifari.pif
	./pifari --check examples/prototipo_multiagente_mcp.pif
	./pifari --check examples/mlp_xor_fluxo.pif
	./pifari --check examples/mlp_xor.pif
	./pifari --check examples/paralelismo_assincrono.pif
	./pifari --check examples/nova_sintaxe_fluxo.pif
	./pifari --check examples/funcoes.pif
	./pifari --check examples/conc_tarefas.pif
	./pifari --check examples/conc_processos.pif
	./pifari --check examples/conc_fluxos.pif
	./pifari --check examples/fluxo_sincronizacao.pif
	./pifari --check examples/fluxo_coleta.pif
	./pifari --check examples/fluxo_paralelo.pif
	./pifari --check examples/fluxo_sequencial.pif
	./pifari --check examples/runtime_fluxo.pif
	./pifari --check examples/runtime_slots_pool.pif
	./pifari --check examples/runtime_otimizacoes.pif
	./pifari --check examples/runtime_matmul.pif
	./pifari --check examples/runtime_matriz.pif
	./pifari --check examples/runtime_indexed_native.pif
	./pifari --check examples/typed_arrays.pif
	./pifari --check examples/runtime_fused_chain.pif
	./pifari --check examples/runtime_vm_index_compare.pif
	./pifari --check examples/runtime_vm.pif
	./pifari --check examples/runtime_operadores.pif
	./pifari --check examples/runtime_lista.pif
	./pifari --check examples/teste_range_preguicoso.pif
	./pifari --check examples/runtime_numeric_tools.pif
	./pifari --check examples/runtime_fusion.pif
	./pifari --check examples/runtime_map_fused.pif
	./pifari --check examples/tipos.pif
	./pifari --check examples/reducao_nativa.pif
	./pifari --check examples/reducao_erro.pif
	./pifari examples/efeitos.pif
	./pifari --check examples/conc_shared.pif

test-equiv: pifari
	sh tests/equiv.sh examples/gpu_dispositivos.pif examples/carga_fluxo_puro.pif examples/benchmarks/benchmark_fused_chain.pif examples/teste_cliente_servidor_mcp_pifari.pif examples/prototipo_multiagente_mcp.pif examples/mlp_xor_fluxo.pif examples/mlp_xor.pif examples/paralelismo_assincrono.pif examples/nova_sintaxe_fluxo.pif examples/funcoes.pif examples/conc_tarefas.pif examples/conc_processos.pif examples/conc_fluxos.pif examples/fluxo_sincronizacao.pif examples/fluxo_coleta.pif examples/fluxo_paralelo.pif examples/fluxo_sequencial.pif examples/runtime_fluxo.pif examples/runtime_slots_pool.pif examples/runtime_otimizacoes.pif examples/runtime_matmul.pif examples/runtime_matriz.pif examples/runtime_indexed_native.pif examples/typed_arrays.pif examples/runtime_fused_chain.pif examples/runtime_vm_index_compare.pif examples/runtime_vm.pif examples/runtime_operadores.pif examples/runtime_lista.pif examples/teste_range_preguicoso.pif examples/runtime_numeric_tools.pif examples/runtime_fusion.pif examples/runtime_map_fused.pif examples/efeitos.pif examples/reducao_nativa.pif examples/reducao_erro.pif

test-golden: pifari
	sh tests/golden.sh examples/gpu_dispositivos.pif examples/carga_fluxo_puro.pif examples/benchmarks/benchmark_fused_chain.pif examples/teste_cliente_servidor_mcp_pifari.pif examples/prototipo_multiagente_mcp.pif examples/mlp_xor_fluxo.pif examples/mlp_xor.pif examples/paralelismo_assincrono.pif examples/nova_sintaxe_fluxo.pif examples/funcoes.pif examples/conc_tarefas.pif examples/conc_processos.pif examples/conc_fluxos.pif examples/fluxo_sincronizacao.pif examples/fluxo_coleta.pif examples/fluxo_paralelo.pif examples/fluxo_sequencial.pif examples/runtime_fluxo.pif examples/runtime_slots_pool.pif examples/runtime_otimizacoes.pif examples/runtime_matmul.pif examples/runtime_matriz.pif examples/runtime_indexed_native.pif examples/typed_arrays.pif examples/runtime_fused_chain.pif examples/runtime_vm_index_compare.pif examples/runtime_vm.pif examples/runtime_operadores.pif examples/runtime_lista.pif examples/teste_range_preguicoso.pif examples/runtime_numeric_tools.pif examples/runtime_fusion.pif examples/runtime_map_fused.pif examples/reducao_nativa.pif

.PHONY: test-errors
test-errors: pifari
	python3 tests/test_error_recovery.py
