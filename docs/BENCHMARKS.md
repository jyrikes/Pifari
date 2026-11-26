# Benchmarks Reais Antes e Depois — Pifari Runtime & MCP Multi-Agent Platform

**Data de Execução:** 2026-10-23  
**Ambiente:** WSL2 `Ubuntu-24.04` (Linux 6.6.87.2-microsoft-standard-WSL2 x86_64)  
**Compilador:** `gcc 13.3.0` (`-std=c11 -O3 -flto -march=native -pthread`)  
**Python de Referência:** `Python 3.12.3`

---

## 1. Resumo Comparativo (Baseline vs. Pós-Otimização)

| Carga / Benchmark | Métrica | Baseline (Antes) | Pós-Otimização (Depois) | Ganho Real | Causa Principal do Ganho |
|---|---|---:|---:|---:|---|
| **MLP XOR 2-4-1 (3000 épocas)** (`examples/mlp_xor.pif` em `make test`) | Tempo de treino (`train_ms`) | **121,77 ms** | **48,84 ms** | **2,49x mais rápido** (-59,9%) | `BC_EXP` na VM de bytecode + fast-path `BC_LOAD + BC_INDEX` sem refcount + `LocalFrame` na stack + `array_set` in-place |
| **MLP XOR Benchmark Script** (`python3 examples/benchmarks/benchmark_mlp_xor.py`, mediana de 5 runs) | Mediana Pifari (`train_ms`) | **685,44 ms** | **111,61 ms** | **6,14x mais rápido** (-83,7%) | Compilação de `sigmoid(z)` (`exp`) para bytecode VM e eliminação de alocações heap por chamada de função |
| **MLP XOR Fluxo** (`examples/mlp_xor_fluxo.pif`) | Tempo de treino (`treino_ms`) | **121,77 ms** | **49,11 ms** | **2,48x mais rápido** (-59,7%) | VM de funções com `BC_EXP` e indexação direta de `f64_array` |
| **Carga de Fluxo Puro — Paralelo** (`examples/carga_fluxo_puro.pif`, 500.000 itens) | `fluxo_paralelo_ms` | **3036,87 ms** | **2102,13 ms** | **1,44x mais rápido** (-30,8%) | Pré-cômputo único de `slot_names`/`annotate_slot_nodes` e `LocalFrame` na stack sem `calloc`/`free` nem `memset` de 2 KB por chamada |
| **Carga de Fluxo Puro — Cadeia Fundida (Com Compilação/Cache)** (`examples/carga_fluxo_puro.pif`, 500.000 itens, 3 etapas) | `cadeia_fundida_ms_inclui_compilacao` | **3611,61 ms** | **603,48 ms** | **5,98x mais rápido** (-83,3%) | Cache persistente de objetos `.so` por hash FNV-1a 64-bit em `/tmp/pifari_kernel_cache_<uid>` em `backend/optimization/native_kernel.c` |
| **Carga de Fluxo Puro — Cadeia Fundida Aquecida** (`examples/carga_fluxo_puro.pif`, 500.000 itens, 3 etapas) | `cadeia_fundida_ms_aquecido` | **42,99 ms** | **38,21 ms** | **1,13x mais rápido** (-11,1%) | Buffer `f64_array` direto sem cópias extras |

---

## 2. Otimizações Implementadas e Verificadas

1. **Eliminação de Data Race e Travessias AST por Chamada (`backend/runtime.c`):**
   - Antes: cada chamada de função em `call_user_function` invocava `count_nodes(body)`, `collect_slot_names(body)` e `annotate_slot_nodes(body)` mutando `n->local_slot` na AST compartilhada entre threads durante `>|`.
   - Depois: `prepare_function_slots(FunctionDef *f)` executa uma única vez no registro da função, pré-computando `slot_names` e anotando a AST antes de qualquer execução concorrente.
2. **Alocação em Stack de `LocalFrame` e Hash On-Demand (`backend/runtime.c`):**
   - Antes: toda chamada de função fazia `calloc` no heap para `LocalFrame` + `Binding[]` e zerava `256 * sizeof(Binding*)` (`2 KB`) para `build_slot_index`, mesmo quando a função rodava 100% em bytecode (`BC_LOAD_SLOT`).
   - Depois: `stack_frame`, `stack_slots[16]`, `stack_values[8]` e `local_bounds[32]` residem na stack C e `build_slot_index` só é executado quando a função não usa bytecode.
3. **Opcode `BC_EXP` e Fast-Path `BC_LOAD + BC_INDEX` (`frontend/ast.h`, `backend/runtime.c`):**
   - Funções que utilizam `exp(x)` (como `sigmoid(z)` em redes neurais e `etapa3` em fluxos numéricos) agora compilam integralmente para a VM de bytecode de funções.
   - Sequências `BC_LOAD` seguidas de `BC_INDEX` sobre `V_F64_ARRAY` leem `array->data[idx]` diretamente sem incrementar/decrementar `refs` atomicamente a cada acesso.
4. **Sintaxe de Atribuição Indexada Direta (`A[i] = val` e `A[i][j] = val`) (`frontend/parser.y`, `backend/runtime.c`):**
   - Unificada a regra de atribuição no parser mantendo exatamente `%expect 7` (zero novos conflitos shift/reduce), permitindo mutação in-place de `f64_array` e listas aninhadas.
5. **Cache Persistente de Kernels C Nativos (`backend/optimization/native_kernel.c`):**
   - Kernels gerados em C são indexados pelo hash FNV-1a 64-bit do código-fonte e armazenados em `/tmp/pifari_kernel_cache_<uid>` com permissões `0700` e verificação de proprietário (`st_uid == getuid()`).

---

## 3. Comandos para Reproduzir

```bash
# No WSL (Ubuntu-24.04) dentro de Pifari:
make clean && make -j2
./pifari examples/carga_fluxo_puro.pif
python3 examples/benchmarks/benchmark_mlp_xor.py
./pifari examples/mlp_xor.pif
./pifari examples/mlp_xor_fluxo.pif
```
