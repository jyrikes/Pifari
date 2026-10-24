CC := gcc
FLEX := flex
BISON := bison
CFLAGS := -std=c11 -O3 -flto -march=native -Wall -Wextra -Wpedantic -Ifrontend -Ibackend
.PHONY: all clean test test-core test-conc test-fuzz benchmark test-gpu
all: pifari
parser.tab.c parser.tab.h &: frontend/parser.y frontend/ast.h backend/runtime.h
	$(BISON) -Wall -d -o parser.tab.c frontend/parser.y
lex.yy.c: frontend/lexer.l parser.tab.h
	$(FLEX) -o lex.yy.c frontend/lexer.l
frontend/parser.o: parser.tab.c frontend/ast.h backend/runtime.h
	$(CC) $(CFLAGS) -pthread -c parser.tab.c -o $@
frontend/lexer.o: lex.yy.c frontend/ast.h
	$(CC) $(CFLAGS) -pthread -c lex.yy.c -o $@
backend/runtime.o: backend/runtime.c backend/runtime.h backend/operator_engine.h backend/ast_ir.h backend/numeric_buffer.h backend/native_kernel.h backend/agent_runtime.h backend/mcp_gateway.h backend/mcp_server.h frontend/ast.h
	$(CC) $(CFLAGS) -pthread -c backend/runtime.c -o $@
backend/mcp_gateway.o: backend/mcp_gateway.c backend/mcp_gateway.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@
backend/mcp_server.o: backend/mcp_server.c backend/mcp_server.h backend/mcp_gateway.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@
backend/agent_runtime.o: backend/agent_runtime.c backend/agent_runtime.h backend/mcp_gateway.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@
backend/optimizer.o: backend/optimizer.c backend/optimizer.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/ast_ir.o: backend/ast_ir.c backend/ast_ir.h backend/optimizer.h frontend/ast.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/numeric_buffer.o: backend/numeric_buffer.c backend/numeric_buffer.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/dirac.o: backend/dirac.c backend/dirac.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/native_kernel.o: backend/native_kernel.c backend/native_kernel.h backend/optimizer.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@
backend/operator_engine.o: backend/operator_engine.c backend/operator_engine.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@
backend/memory_plan.o: backend/memory_plan.c backend/memory_plan.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/reversible.o: backend/reversible.c backend/reversible.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/gpu_client.o: backend/gpu_client.c backend/gpu_client.h backend/devices.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/cache.o: backend/cache.c backend/cache.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/plan.o: backend/plan.c backend/plan.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/effects.o: backend/effects.c backend/effects.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/typecheck.o: backend/typecheck.c backend/typecheck.h backend/types.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/types.o: backend/types.c backend/types.h
	$(CC) $(CFLAGS) -c $< -o $@
backend/devices.o: backend/devices.c backend/devices.h
	$(CC) $(CFLAGS) -c $< -o $@
pifari: frontend/parser.o frontend/lexer.o backend/runtime.o backend/optimizer.o backend/ast_ir.o backend/numeric_buffer.o backend/operator_engine.o backend/memory_plan.o backend/reversible.o backend/dirac.o backend/native_kernel.o backend/mcp_gateway.o backend/mcp_server.o backend/agent_runtime.o backend/devices.o backend/types.o backend/typecheck.o backend/effects.o backend/plan.o backend/cache.o backend/gpu_client.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm -ldl
test_optimizer: tests/test_optimizer.c backend/optimizer.o backend/operator_engine.o backend/memory_plan.o backend/reversible.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm
test_ast_ir: tests/test_ast_ir.c backend/ast_ir.o backend/optimizer.o backend/numeric_buffer.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm
test_dirac: tests/test_dirac.c backend/dirac.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm
test_native_kernel: tests/test_native_kernel.c backend/native_kernel.o backend/optimizer.o backend/cache.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm -ldl
test_agent_runtime: tests/test_agent_runtime.c backend/agent_runtime.o backend/mcp_gateway.o backend/mcp_server.o
	$(CC) $(CFLAGS) -pthread -o $@ $^ -lm
test: pifari test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
	./test_optimizer
	./test_ast_ir
	./test_dirac
	./test_native_kernel
	./test_agent_runtime
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
	./pifari examples/orquestrador_reels_aistudio.pif
test-fuzz: pifari
	python3 tests/fuzz_parser.py
benchmark: pifari
	python3 examples/benchmark_cpu.py
	python3 examples/benchmark_matrizes.py
	./pifari examples/benchmark_fused_chain.pif
	python3 examples/benchmark_indexed_vs_python.py
	python3 examples/benchmark_mlp_xor.py
	./pifari examples/carga_fluxo_puro.pif
clean:
	rm -f pifari test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime parser.tab.c parser.tab.h lex.yy.c parser.output frontend/parser.o frontend/lexer.o backend/runtime.o backend/optimizer.o backend/ast_ir.o backend/numeric_buffer.o backend/operator_engine.o backend/memory_plan.o backend/reversible.o backend/dirac.o backend/native_kernel.o backend/mcp_gateway.o backend/mcp_server.o backend/agent_runtime.o backend/devices.o backend/types.o backend/typecheck.o backend/effects.o backend/plan.o backend/cache.o backend/gpu_client.o

test-gpu: pifari
	./pifari examples/gpu_dispositivos.pif

test-core: pifari test_optimizer test_ast_ir test_dirac test_native_kernel test_agent_runtime
	./test_optimizer
	./test_ast_ir
	./test_dirac
	./test_native_kernel
	./test_agent_runtime
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
	./pifari --check examples/benchmark_fused_chain.pif
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
	./pifari examples/efeitos.pif
	./pifari --check examples/conc_shared.pif

test-equiv: pifari
	sh tests/equiv.sh examples/gpu_dispositivos.pif examples/carga_fluxo_puro.pif examples/benchmark_fused_chain.pif examples/teste_cliente_servidor_mcp_pifari.pif examples/prototipo_multiagente_mcp.pif examples/mlp_xor_fluxo.pif examples/mlp_xor.pif examples/paralelismo_assincrono.pif examples/nova_sintaxe_fluxo.pif examples/funcoes.pif examples/conc_tarefas.pif examples/conc_processos.pif examples/conc_fluxos.pif examples/fluxo_sincronizacao.pif examples/fluxo_coleta.pif examples/fluxo_paralelo.pif examples/fluxo_sequencial.pif examples/runtime_fluxo.pif examples/runtime_slots_pool.pif examples/runtime_otimizacoes.pif examples/runtime_matmul.pif examples/runtime_matriz.pif examples/runtime_indexed_native.pif examples/typed_arrays.pif examples/runtime_fused_chain.pif examples/runtime_vm_index_compare.pif examples/runtime_vm.pif examples/runtime_operadores.pif examples/runtime_lista.pif examples/teste_range_preguicoso.pif examples/runtime_numeric_tools.pif examples/runtime_fusion.pif examples/runtime_map_fused.pif examples/efeitos.pif
