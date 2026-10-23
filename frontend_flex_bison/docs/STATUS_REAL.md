# Diagnóstico e Estado Real — Compilador e Runtime Pifari (`./pifari`)

**Data da auditoria:** 2026-10-23  
**Diretório auditado:** `C:\Users\Yngrid Kalinne\OneDrive\Desktop\Pifari_Definicao\frontend_flex_bison`  
**Ambiente verificado:** WSL Ubuntu 24.04 (Linux x86_64), GCC `13.3.0`, Flex `2.6.4`, Bison `3.8.2`, Python `3.12.3` (WSL) e Python `3.13` (Windows).

---

## 1. Inventário de Módulos Existentes

| Arquivo | Linhas | Estado Real | Responsabilidade |
| :--- | ---: | :--- | :--- |
| `frontend/ast.h` | 91 | Funcional | Nós da AST (`Node`), opcodes da VM (`ByteOp`), constant folding seguro (`pifari_try_fold`). |
| `frontend/lexer.l` | 108 | Funcional | Lexer Flex com pilha de indentação (`INDENT`/`DEDENT`), suporte híbrido a `{}` e tokens de tipos (`Task`, `Agent`, `int`, etc.). |
| `frontend/parser.y` | 124 | Funcional (`%expect 7`) | Gramática Bison, operadores aritméticos/lógicos/fluxo (`\|>`, `<\|`, `>\|`, `\|<`), funções e entrypoint `main`. |
| `backend/runtime.h` | 6 | Funcional | Contrato `int runtime_execute(const Node *program)`. |
| `backend/runtime.c` | 1.312 | Funcional (Monolítico) | Interpretador AST, VM de bytecode (expressões e funções), JIT C de 1 argumento inteiro, pool de 4 threads POSIX (`pthreads`), builtins e arrays tipados `V_F64_ARRAY`. |
| `backend/ast_ir.[ch]` | 245 | Funcional | Lowering de funções aritméticas unárias e indexadas `(A, i)` para grafo IR (`PifariExprGraph`). |
| `backend/optimizer.[ch]` | 349 | Funcional | Constant folding no DAG IR, composição `pifari_graph_compose`, planejamento de workspace e avaliação paralela. |
| `backend/native_kernel.[ch]` | 224 | Funcional (Sem cache em disco) | Emissão de C restrito, compilação `gcc -O3 -march=native -shared` + `dlopen` e execução paralela. |
| `backend/numeric_buffer.[ch]` | 42 | Funcional | Buffer `double` contíguo alinhado em 64 bytes (`posix_memalign`). |
| `backend/operator_engine.[ch]` | 60 | Funcional | Kernel nativo vetorizado/multi-thread `map_affine_relu`. |
| `backend/memory_plan.[ch]` | 19 | Parcial / Isolado | Alocador linear-scan testado isoladamente em `test_optimizer`, mas não chamado diretamente por `optimizer.c` (que possui seu próprio `build_workspace_plan`). |
| `backend/dirac.[ch]` | 201 | Isolado (Apenas API C) | Álgebra simbólica de bras/kets/operadores; testada em `test_dirac`, mas sem builtins em `runtime.c` nem sintaxe no parser. |
| `backend/reversible.[ch]` | 140 | Isolado (Apenas API C) | Gates reversíveis `XOR`, `NOT`, `SWAP` e `uncompute`; testado em `test_optimizer`, sem ligação com `runtime.c`. |
| `../lexer.py`, `../parser.py` | 420 | Referência (Sem runtime) | Frontend protótipo anterior em Python na raiz de `Pifari_Definicao` (`test_frontend.py` passa 7/7 testes). |

---

## 2. Baseline dos Testes Atuais

Comandos executados antes de qualquer alteração no código:

1. **Testes Unitários C e Integração `.pif` (`make test` no WSL):**
   - `./test_optimizer`: **PASSOU**
   - `./test_ast_ir`: **PASSOU**
   - `./test_dirac`: **PASSOU**
   - `./test_native_kernel`: **PASSOU**
   - 25 programas `.pif` da suíte `make test`: **PASSARAM (25/25)**
2. **Smoke Fuzzing do Parser/Runtime (`make test-fuzz` no WSL):**
   - `python3 tests/fuzz_parser.py`: **PASSOU** (`120 inputs, no crashes/timeouts`)
3. **Frontend de Referência Python (`python -m unittest -v test_frontend.py`):**
   - **PASSOU** (`Ran 7 tests in 0.002s - OK`)

### Exemplos `.pif` Fora do `make test` e Falhas Preexistentes Documentadas

Dos 37 arquivos `.pif` em `examples/`, 25 estão em `make test`, 5 estão em `make benchmark` e 7 são rascunhos/exemplos avulsos. Destes, **4 arquivos falham no baseline preexistente**:
- `examples/condicao_implicita.pif` e `examples/runtime_basico.pif`: falham com erro sintático porque usam condicional implícita sem `if` (`idade >= 18` seguido de bloco indentado), recurso ainda não suportado em `parser.y`.
- `examples/diretivas.pif`: falha propositalmente em `runtime.c:1295` ao encontrar `:= Pessoa` (`classes são reconhecidas pelo parser, mas objetos e métodos ainda não são executáveis`).
- `examples/fluxo.pif`: rascunho antigo com identificadores não declarados e sintaxe antiga de `>|` sem variável de iteração.

---

## 3. Baseline de Desempenho Medido (2026-10-23)

| Carga / Benchmark | Métrica Medida no Baseline | Observação Técnica |
| :--- | :--- | :--- |
| `examples/carga_fluxo_puro.pif` (`N = 500.000`, 3 etapas) | `fluxo_paralelo_ms`: **6.535,83 ms**<br>`cadeia_fundida_ms_inclui_compilacao`: **2.426,60 ms**<br>`cadeia_fundida_ms_aquecido`: **120,97 ms** | `native_kernel.c` recompila o `.so` sem cache em disco em toda execução; `try_native_map_flow` converte `V_LIST` ↔ `double*` em cada etapa e rejeita `V_RANGE` e `V_F64_ARRAY`. |
| `examples/benchmark_mlp_xor.py` (MLP 2–4–1, 3.000 épocas SGD escalar) | Mediana Pifari: **685,44 ms**<br>Mediana Python: **56,58 ms**<br>Razão Python/Pifari: **12,11×** mais lento que Python | O treino roda em laços interpretados; cada chamada a `sigmoid(x)` (36.000 chamadas) percorre e re-anota a AST 3 vezes (`count_nodes`, `collect_slot_names`, `annotate_slot_nodes`), aloca `LocalFrame` no heap e não compila `exp()` em bytecode (`BC_EXP` ausente). |

---

## 4. Gargalos, Bugs Latentes e Débitos Técnicos Identificados

1. **Data Race e Overhead O(N_AST) em `call_user_function` (`backend/runtime.c:950-970`):**
   - Toda chamada de função de usuário executa `count_nodes`, `collect_slot_names` e `annotate_slot_nodes((Node*)slot_body, ...)`, mutando `n->local_slot` e `n->target_slot` diretamente nos nós da AST compartilhada entre threads do `worker_pool`, além de alocar `LocalFrame` e `frame->slots` no heap a cada chamada.
2. **Ausência de `BC_EXP` e de `IndexAssign` (`A[i] = val`) na Gramática e na VM (`frontend/parser.y`, `backend/runtime.c`):**
   - `runtime.c:1264-1272` possui código para `IndexAssign`, mas `parser.y` não tem a produção `IDENTIFIER LBRACKET expr RBRACKET ASSIGN expr`, obrigando programas como `mlp_xor.pif` a usar llamadas de função `array_set(pesos, i, val)`.
   - `bc_compile_node` não compila chamadas ao builtin matemático `exp(x)`, impedindo que `sigmoid(x)` use a VM de bytecode.
3. **Inconsistência Semântica em `BC_IDIV` (`backend/runtime.c:289` vs `892`):**
   - `eval_bytecode` trunca divisão inteira em direção a zero (`a.as.i / b.as.i`), enquanto `execute_function_bytecode` usa `floor(x / y)` em `double`.
4. **Ausência de Cache em Disco em `backend/native_kernel.c` (`L118-143`):**
   - Enquanto `jit_compile_function` em `runtime.c` faz cache em `/tmp/pifari_jit_cache_<uid>`, `native_kernel.c` recompila o código C via `gcc` em todo processo e apaga o `.so` logo após `dlopen`.
5. **Restrição Artificial de `V_F64_ARRAY` e `V_RANGE` nos Fluxos `>|` e `|>` (`backend/runtime.c:1084, 1135, 1245`):**
   - `try_native_map_flow` só aceita `V_LIST` (rejeitando `V_RANGE` e `V_F64_ARRAY`), e `eval_parallel_flow` / `Flow("|>")` rejeitam `V_F64_ARRAY` na validação de tipo.
6. **Inexistência do Runtime de Agentes e MCP Gateway:**
   - Embora `lexer.l:68` reconheça os tipos `Task` e `Agent` e `Definicao_Pifari_v3.tex` preveja agentes e tarefas concorrentes, não há nenhuma implementação de `Task Engine`, `Agent Manager`, `MCP Gateway`, `Context Store`, `Policy Engine`, `Telemetry Engine` ou `Artifact Manager`.

---

## 5. Componentes Reutilizáveis e Interfaces de Integração

- **Parser/Lexer (`frontend/`):** Já reconhece `Task` e `Agent` como `TYPE` e transforma `Task(...)` / `Agent(...)` em nós `Call` sem precisar de novas palavras-chave que quebrem a gramática.
- **Pool de Threads e Primitivas POSIX (`backend/runtime.c`):** Possui relógio monotônico (`monotonic_ns`, `monotonic_ms`), hash FNV-1a de 64 bits e `worker_pool` pthreads que podem coexistir com o `Agent Runtime`.
- **Ponto de Acoplamento Limpo:** `backend/runtime.c` já é grande (1.312 linhas). Os 7 módulos de orquestração multiagente e o `MCP Gateway` devem residir em unidades de compilação próprias (`backend/mcp_gateway.[ch]`, `backend/agent_runtime.[ch]`) com uma ponte enxuta em `runtime.c` para despachar builtins de agentes/MCP sem inflar o núcleo numérico.
