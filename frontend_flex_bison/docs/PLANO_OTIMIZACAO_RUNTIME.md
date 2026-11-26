# Plano de execução — otimização da VM e do runtime Pifari

## Objetivo
Reduzir o custo por operação e por iteração sem alterar a semântica observável. Cada fase deve manter `make test` verde e validar os resultados antes de aceitar uma otimização.

## Estado conhecido
- Runtime C com interpretador AST e VM de bytecode parcial para expressões numéricas.
- Bytecode armazenado no nó AST depois da primeira compilação.
- Pool paralelo persistente de até quatro threads por execução; ambiente base compartilhado para leitura, bindings rasos privados e slots locais isolados.
- Compilação usa `-O3 -flto`, pthreads e tabela hash para bindings.
- O benchmark atual faz uma amostra de Pifari e cinco amostras de Python; os tempos são indicativos, não uma comparação estatística perfeitamente equivalente.

## Fase 1 — Diagnóstico e segurança
- [x] Confirmar compilação em WSL.
- [x] Executar `make test`.
- [x] Executar `make benchmark` e validar resultados.
- [ ] Repetir benchmarks de Pifari e calcular medianas.
- [ ] Medir alocações, instruções e cache misses com ferramentas de profiling disponíveis.

## Fase 2 — Caminho quente da VM (implementada nesta execução)
- [x] Retirar o mutex de `bc_ensure` do caminho quente depois que o bytecode é publicado. A primeira compilação continua protegida por mutex; publicação/leitura usam acquire/release.
- [x] Usar pilha local de 32 slots em `eval_bytecode`; alocar no heap apenas para expressões maiores.
- [x] Manter fallback interpretado para nós que não podem ser compilados em bytecode.
- [x] Recompilar e executar testes e benchmark.

## Fase 3 — Loops, dados e alocações
- [ ] Medir `copy_value`, `find_binding`, chamadas de função e alocações dentro de loops.
- [ ] Avaliar slots locais para evitar busca por nome a cada acesso.
- [ ] Adicionar saltos e controle de fluxo ao bytecode, com testes de semântica.
- [ ] Implementar/otimizar indexação e atribuição por índice, inclusive listas aninhadas.
- [ ] Avaliar arrays numéricos contíguos tipados como tipo separado das listas heterogêneas.

## Fase 4 — Paralelismo
- [ ] Separar custo de criar threads, copiar bindings, distribuir tarefas e executar trabalho útil.
- [ ] Medir o limiar para executar `>| ` sequencialmente quando as tarefas forem pequenas.
- [ ] Avaliar pool persistente e distribuição em blocos, preservando ordem dos resultados e propagação de erros.
- [ ] Tornar o número de workers configurável depois das medições.

## Fase 5 — Avançado
- [ ] Folding de constantes com preservação de erros e semântica.
- [ ] Comparar bytecode de pilha e bytecode baseado em registradores.
- [ ] Considerar SIMD/BLAS para arrays homogêneos.
- [ ] Considerar JIT apenas depois de perfilar e estabilizar VM/runtime.

## Validação desta execução
- `make clean && make -j2`: compilação concluída sem erros.
- `make test`: suíte executada; registro do terminal foi consultado para confirmar o resultado.
- `make benchmark`: os resultados calculados por Pifari e Python foram iguais.
- Amostra após as alterações: Pifari sequencial 105 ms e paralelo 50 ms; Python sequencial mediana 64,22 ms, threads 84,58 ms, processos 44,43 ms. São amostras de uma execução; ruído de CPU/WSL e a diferença entre amostra única e mediana impedem atribuir o resultado apenas às mudanças feitas.

## Critérios de aceite
1. `make test` passa após cada fase.
2. Saídas e resultados numéricos permanecem corretos.
3. Benchmarks usam mesmo trabalho, aquecimento e várias amostras por modo.
4. Manter uma otimização quando o ganho for repetível ou quando reduzir memória/alocações sem regressão relevante.

## Atualização — segunda rodada

- [x] Contadores opcionais ativados com `PIFARI_PROFILE=1`: nós avaliados, buscas de bindings, cópias de valores e instruções de bytecode. Contadores incluem workers e são emitidos em stderr; desativados por padrão.
- [x] Caminho rápido para indexação de listas/matrizes aninhadas por nome: não clona a matriz inteira para cada leitura; copia apenas o valor selecionado.
- [x] Concatenação de listas com `+`, necessária para montar matrizes incrementalmente.
- [x] Benchmark de CPU com cinco execuções de cada runtime e medianas para Pifari sequencial/paralelo e Python sequencial/threads/processos.
- [x] Benchmark de matrizes com N=60, três execuções e checksum comparável.
- [x] `make benchmark` agora executa os dois benchmarks; testes de integração passaram.

### Medições observadas nesta rodada

CPU: Pifari sequencial mediana 111 ms, paralelo 50 ms; Python sequencial 38,67 ms, threads 62,28 ms e processos 24,68 ms. Os resultados numéricos foram iguais.

Matrizes N=60: Pifari mediana 127 ms; Python mediana 46,18 ms; checksum igual (-68440). Com profiling ligado, o runtime registrou 1.999.170 nós AST, 1.779.391 buscas de binding, 2.880.921 cópias de valores e 21.606 operações de bytecode. A execução instrumentada é mais lenta; use os contadores para identificar custos, não compare seu tempo com a execução normal.

Os benchmarks repetidos reduzem ruído, mas ainda são medições de uma máquina/ambiente. Pifari e Python têm custos semânticos diferentes (incluindo cópia de ambientes em chamadas de função), então os números são diagnóstico e não uma afirmação de desempenho universal.

## Atualização — terceira rodada

- [x] Folding de constantes numéricas em tempo de construção da AST para literais e subexpressões aritméticas seguras; divisão por zero e overflow não são dobrados, preservando o caminho normal de erro.
- [x] Adicionado `examples/runtime_otimizacoes.pif` à suíte: verifica precedência, folding, concatenação de listas e indexação aninhada/negativa.
- [x] Troca de tarefas no fluxo paralelo agora usa contador atômico, evitando mutex em cada item; o mutex continua reservado para registrar falhas.
- [x] Build limpo sem warnings e `make test` passou após as mudanças.

## Atualização — quarta rodada: slots, pool e memória

- [x] Frames de função com slots contíguos indexados por hash; coletam parâmetros, atribuições, declarações tipadas e variáveis de laço. Os slots são inicializados a partir do ambiente visível, preservando a semântica anterior de ambiente isolado.
- [x] Chamadas de função deixaram de clonar a lista inteira de bindings globais e de reconstruir a tabela hash global em cada chamada.
- [x] Pool persistente sob demanda com até quatro workers, reutilizado por múltiplos fluxos de paralelismo e encerrado no fim de runtime_execute.
- [x] Workers usam cópias rasas de nós de binding para o ambiente base; os valores são compartilhados apenas para leitura. A variável do fluxo usa slot local por worker e as atribuições de funções ficam em frames locais.
- [x] Para fontes que já são listas, o fluxo transfere o buffer da lista avaliada em vez de fazer uma cópia profunda extra antes de distribuir tarefas.
- [x] Adicionado examples/runtime_slots_pool.pif, que valida variáveis locais, preservação de globals, múltiplas chamadas paralelas e reutilização do pool.
- [x] Build sem warnings, suíte completa e benchmarks com validação dos resultados/checksum passaram após as alterações.

- Bytecode com saltos/loops e registradores: ainda não implementado; a VM continua cobrindo um subconjunto aritmético e usa fallback AST para o restante.
- Arrays homogêneos/SIMD e JIT: ainda não implementados. JIT depende de uma representação estável de bytecode/valores e de profiling que justifique o custo.

## Validação da quarta rodada

- make clean && make -j2: passou; após remover uma função auxiliar obsoleta, a compilação foi repetida para confirmar ausência de warnings.
- make test: passou incluindo o teste de slots e dois fluxos paralelos no mesmo programa.
- make benchmark: passou; CPU e checksum de matriz foram validados. Os tempos variam conforme carga do host/WSL e não demonstram, isoladamente, um ganho quantitativo.

## Validação final após a terceira rodada

- [x] Build limpo `make clean && make -j2`: passou sem warnings.
- [x] `make test`: passou, incluindo `runtime_otimizacoes.pif`.
- [x] `make benchmark`: passou e os resultados/checksums coincidiram.
- [x] A regra de benchmark agora mede cinco execuções de CPU e três de matrizes.

Última amostra de benchmark apresentou variação muito alta entre repetições (Pifari CPU sequencial de 176 a 505 ms; matriz de 310 a 705 ms), indicando carga/ruído significativo do host/WSL. Portanto, esses números não devem ser usados para alegar regressão ou ganho; repita com máquina ociosa e, idealmente, fixe CPU/afinidade antes de comparar mudanças.


## Rodada 5 — JIT nativo, superinstruções e matrizes

- [x] Contagem de referências atômica para listas com armazenamento único contíguo (cabeçalho + elementos); cópias de listas passam a compartilhar o buffer, mantendo ownership seguro e sem duplicação profunda.
- [x] Slots locais anotados nos nós AST; bytecode pode carregar slots diretamente sem recalcular hash para variáveis locais.
- [x] VM de funções para subconjunto de atribuições, aritmética, laços for sobre range e return, com saltos e limites de laço.
- [x] Superinstrução para acumulação total = total + x * y quando os operandos são slots locais.
- [x] JIT restrito a funções numéricas de um argumento e laços range(parâmetro), com GCC -O3 -march=native, dlopen e cache de artefatos pelo hash do código gerado em /tmp. Casos fora do subconjunto permanecem no interpretador.
- [x] JIT e VM recusam otimizar certos casos em que a variável do laço é observada/escrita fora do corpo, preservando o valor final da variável; teste ultimo_indice(5) retorna 4.
- [x] Primitiva nativa matmul(A, B) para matrizes retangulares numéricas, com verificação de dimensões e resultado validado contra o baseline Python.
- [x] Relógio monotônico em nanossegundos (agora_ns) para medir kernels abaixo de 1 ms.
- [x] Flags -O3 -flto -march=native; link com -ldl para o JIT.
- [x] Benchmarks reportam tanto o kernel medido quanto o tempo total do processo e mostram as medianas de múltiplas amostras.

### Resultado observado nesta máquina/WSL

- Última rodada com cache JIT aquecido: kernel da soma de quadrados, mediana 0,287 ms sequencial e 0,637 ms paralelo para quatro tarefas de 200.000 iterações; Python sequencial ficou em 51,55 ms e Python com pool de processos aquecido em 20,88 ms. Incluindo inicialização dos processos, Python com processos levou 33,91 ms; o processo Pifari completo aquecido levou 10,68 ms. Todos os resultados foram iguais.
- Multiplicação de matrizes N=60 via matmul nativo: mediana 0,360 ms; Python puro com três laços levou 13,18 ms. A versão Pifari com laços interpretados levou 110,61 ms; checksum -68440 foi igual nos três casos.
- Em uma medição anterior, o processo Pifari com cache JIT frio levou 745 ms devido à compilação dinâmica. Assim, o kernel otimizado supera os kernels Python medidos e o processo aquecido superou a criação fria de um pool Python nesta rodada; a primeira execução pode perder para Python por causa do custo de compilação. O benchmark explicita ambos.
- Esses ganhos se aplicam ao subconjunto compilado/JIT e à primitiva matmul; não significam que qualquer programa Pifari ou todos os laços interpretados sejam mais rápidos que Python.


## Rodada 6 — Motor de operadores modular

Implementado nesta rodada:

- `backend/optimizer.[ch]`: grafo numérico intermediário, dobra segura de constantes e execução por elemento sem criar um array intermediário por nó. Identidades de ponto flutuante potencialmente não equivalentes (NaN/zero com sinal) não são aplicadas automaticamente.
- `backend/operator_engine.[ch]`: kernel nativo fundido `map_affine_relu`, que calcula `max(0, escala*x + deslocamento)` em uma passagem. Para entradas grandes, pode distribuir blocos por threads; entradas pequenas ficam sequenciais para evitar overhead.
- `backend/memory_plan.[ch]`: alocador de slots temporários baseado em intervalos de vida, permitindo reutilizar armazenamento que não está mais vivo.
- `backend/reversible.[ch]`: primitiva didática XOR reversível, com teste de que aplicar a mesma máscara duas vezes restaura os bits.
- `tests/test_optimizer.c`: testes do grafo, kernel serial/paralelo, slots e reversibilidade. O exemplo `examples/runtime_fusion.pif` testa a integração do kernel com a linguagem.

Verificação: `make clean && make -j2 && make test` passou; `make benchmark` também concluiu e validou os checksums. Os benchmarks continuam dependentes da carga do ambiente. A operação integrada `map_affine_relu` ainda converte a lista genérica para buffers `double` e volta; o próximo passo é introduzir arrays numéricos tipados/contíguos para remover essas conversões e compilar grafos arbitrários em kernels JIT, em vez de apenas interpretar o grafo genérico. A sintaxe de Dirac, operações matriciais simbólicas gerais e uncomputation automático ainda não estão implementados. A primitiva reversível atual é uma demonstração técnica, não um modo reversível completo da linguagem.


## Rodada 7 — IR iterativa e operações numéricas nativas

- Refatorado `optimizer.c`: avaliação iterativa topológica, um único workspace temporário por worker, detecção de divisão por zero e API `pifari_graph_map_parallel` com partições disjuntas e fallback sequencial em falha ao criar threads.
- Adicionados builtins `sort`, `binary_search` e `solve_linear` no runtime. Ordenação não altera a lista original; busca binária requer lista previamente ordenada; solver usa eliminação de Gauss com pivoteamento parcial e rejeita singularidade/entrada não finita.
- Adicionado `examples/runtime_numeric_tools.pif` à suíte `make test`.
- Testes de regressão adicionais com 20 mil elementos para equivalência entre grafo sequencial/paralelo e teste de divisão por zero no grafo.

Complexidades: `sort` O(n log n); `binary_search` O(log n); `solve_linear` O(n^3) tempo e O(n^2) espaço; `matmul` O(r*k*c); map de grafo O(n*m), onde n é o número de elementos e m o número de nós do grafo, com O(m) workspace por worker.

Limite arquitetural ainda presente: a IR não é produzida automaticamente a partir da AST da linguagem; arrays numéricos continuam representados como listas genéricas no runtime; não há JIT para grafos arbitrários, nem operadores de Dirac/uncomputation automáticos. Esses itens exigem integração de compilador e uma representação tipada de arrays, e não devem ser considerados concluídos nesta rodada.


## Auditoria de complexidade — 2026-10-19

Escopo contado: 42 arquivos de código/exemplos em `frontend/`, `backend/`, `tests/` e `examples/`, totalizando aproximadamente 2.051 linhas de fonte, cabeçalhos, gramática, exemplos Pifari e benchmarks Python (não inclui os arquivos C gerados pelo Bison/Flex). O runtime C concentra cerca de 1.015 linhas e é o módulo mais complexo. `make clean && make -j2 && make test` passou após as mudanças; sem warnings do compilador C, apenas o aviso informativo do LTO sobre compilar os jobs LTRANS serialmente.

Benchmark desta auditoria (ambiente compartilhado, variância visível): JIT aquecido Pifari sequencial 0,280 ms; kernel paralelo Pifari 0,572 ms; processo Pifari completo 14,00 ms; Python sequencial 45,78 ms; Python pool de processos aquecido 27,88 ms; Python processo completo 75,00 ms (mediana com amostras muito variáveis). Matriz N=60: `matmul` nativo Pifari 0,359 ms, Python puro 12,56 ms, loops interpretados Pifari 141,847 ms. Esses números indicam que o kernel JIT/nativo pode ser muito rápido, mas não demonstram superioridade global do interpretador; loops matriciais Pifari ainda são o gargalo.

Classificação do projeto: protótipo de linguagem/compilador de complexidade intermediária, com runtime já considerável e várias otimizações experimentais. Não é ainda um compilador otimizador integrado de produção. Os principais débitos arquiteturais são: (1) IR do otimizador não é construída a partir da AST; (2) JIT só cobre subconjunto numérico restrito; (3) ausência de array numérico tipado/contíguo expõe custo de conversões e boxing; (4) falta de testes de fuzzing/sanitizers e benchmark estável controlado; (5) builtins de álgebra numérica não substituem uma biblioteca científica robusta. Operadores de Dirac e uncomputation automático continuam pesquisa/planejamento, não funcionalidade implementada.


## Rodada 8 — AST → IR, kernel nativo e base de arrays tipados

Implementações adicionadas:

- `backend/ast_ir.[ch]`: conversão automática de uma função aritmética pura de um parâmetro para o grafo IR; erro explícito para variável livre ou nó não suportado.
- `backend/native_kernel.[ch]`: codegen C restrito a opcodes conhecidos, compilação dinâmica com GCC e carregamento de função nativa. O kernel em lote é reutilizado e pode particionar grandes buffers em workers.
- Integração em `runtime.c`: as funções tentam construir IR ao serem registradas; `map_fused` usa kernel nativo sob demanda e o fluxo `>| item >| funcao(item)` tenta detectar automaticamente o caso de map numérico elegível. O caminho antigo continua como fallback quando a IR/JIT não é aplicável.
- `backend/numeric_buffer.[ch]`: buffer F64 contíguo com alinhamento de 64 bytes e verificações de tamanho; usado na fronteira do kernel fundido.
- `backend/optimization/optimizer.c`: planejamento linear de slots temporários por último uso integrado à avaliação serial/paralela, reduzindo workspace quando o grafo permite reutilização.
- `backend/optimization/operator_engine.c`: loop com contrato de independência por elemento para facilitar vetorização SIMD pelo GCC.
- `backend/dirac.[ch]`: API simbólica experimental para bra/ket, operador, soma, composição, adjunto e produto interno. Não integrada à gramática Pifari.
- `backend/reversible.[ch]`: sequência explícita de gates XOR, NOT e SWAP e uncomputation automática aplicando a sequência inversa ao contrário.
- Novos testes `test_ast_ir`, `test_native_kernel`, `test_dirac`; o `test_optimizer` verifica restauração de estado após uncomputation. `make test-fuzz` roda 120 entradas pseudoaleatórias determinísticas no parser/runtime.

### Fronteira de implementação

A conversão AST→IR e a compilação nativa estão implementadas para expressões aritméticas puras com um parâmetro; não caracterizam JIT geral. A fusão automática é aplicada ao padrão de map numérico no fluxo paralelo, não a todas as cadeias arbitrárias. Buffers F64 já são contíguos e alinhados, mas listas genéricas ainda são convertidas na fronteira e os resultados continuam sendo listas genéricas. A álgebra de Dirac e a reversibilidade são APIs experimentais em C, não recursos sintáticos/semânticos automáticos da linguagem. Fuzzing é smoke test; faltam ASan/UBSan na suíte inteira, injeção de falhas de alocação, stress concorrente e benchmark controlado.

## 2026-10-19 — ampliação da VM geral

- A VM de expressões e a VM de funções agora emitem/consomem bytecodes para comparações relacionais/equivalência e indexação de listas/ranges.
- A indexação aninhada permite executar expressões como matriz[i][j] pela VM quando os subnós são compatíveis; índices negativos e validação de limites foram mantidos.
- O exemplo examples/runtime_vm_index_compare.pif cobre matrizes 2D, índices negativos, comparações e um loop range com acumulação indexada.
- Validação desta etapa: make test e make test-fuzz passaram; fuzz smoke: 120 entradas sem crash/timeout.
- Não declarar ainda ganho percentual de desempenho: esta etapa ampliou cobertura do bytecode, mas não foi feita comparação controlada antes/depois.
- Próxima etapa do JIT nativo: ABI de arrays tipados, indexação com verificação de limites, laços com dependências analisadas e fusão de cadeias; isso exige um caminho explícito de fallback e validação semântica, não apenas gerar C a partir de qualquer AST.

## 2026-10-19 — composição e cache de cadeias nativas

Implementado `pifari_graph_compose` para compor grafos unários, builtin `map_fused_chain` para aplicar uma sequência de funções aritméticas puras num único kernel C nativo e cache do kernel por sequência de nomes durante a vida do processo. O caminho aceita lista numérica ou range e evita materializar listas intermediárias entre as etapas; ainda produz lista genérica ao final.

O exemplo de regressão está em `examples/runtime_fused_chain.pif`; o benchmark repetido e as limitações estão em `benchmarks/BENCHMARK_FUSAO_CADEIAS_2026-10-19.md`. Nesta máquina, com 20 mil itens, a mediana observada foi ~777 ms para duas etapas no fluxo, ~469 ms na primeira chamada fundida incluindo compilação e ~0,991 ms para a chamada aquecida com kernel em cache. Interpretar como resultado específico do workload, não como aceleração universal.

Próxima etapa: arrays tipados de primeira classe (shape/strides, ABI explícita e ownership) e IR com load/store indexado e limites verificados. O JIT ainda não compila loops arbitrários, condicionais, nem funções que acessam arrays por indexação; a fusão automática geral permanece pendente.


## Implementação — arrays tipados F64 (2026-10-19)

- [x] Array F64 como tipo de valor de primeira classe (`f64_array` / `array_f64`), buffer contíguo alinhado a 64 bytes e contagem de referências.
- [x] Construção a partir de tamanho (zero-inicializado) ou lista/range numérico; `len`, impressão e igualdade.
- [x] Indexação de leitura no bytecode para arrays tipados com acesso direto `data[index]`, índices negativos e checagem de limites.
- [x] Escrita por `array_set(array, indice, valor)` sem recriar o buffer; aliases observam as alterações.
- [x] Iteração `for` e fluxos paralelos sobre arrays tipados.
- [x] `map_fused_chain` aceita arrays F64 sem materializar uma lista genérica e devolve array F64, mantendo o kernel aritmético compilado nativamente.
- [x] Exemplo de integração adicionado a `examples/typed_arrays.pif` e ao alvo `make test`.

Limitações restantes: ainda não existe sintaxe `array[i] = valor`, arrays multidimensionais com shape/strides, slicing, tipos F32/I64, nem operação `LOAD_INDEX` incorporada ao IR JIT geral de funções. O opcode de bytecode de indexação faz acesso direto ao buffer contíguo; não confundir isso com JIT arbitrário de ASTs contendo indexação.


## Implementação — JIT de acesso indexado em arrays F64 (2026-10-19)

- [x] Novo lowering AST→IR para funções com dois parâmetros `(array, indice)` e expressão `array[indice]`.
- [x] O acesso indexado reconhecido é reduzido a `PEX_INPUT`; o kernel nativo emite leitura direta `input[i]` dentro do loop C compilado.
- [x] Novo built-in `map_indexed_native("funcao", array_f64[, workers])` com cache de IR/kernel por definição da função e saída F64 tipada.
- [x] Testes funcionais para indexação e expressões aritméticas, incluídos em `make test`.
- [x] Benchmark repetível comparando kernel nativo aquecido, loop interpretado Pifari e duas implementações Python: `examples/benchmarks/benchmark_indexed_vs_python.py`.

Limitação deliberada do subset inicial: somente o padrão `A[i]` onde `A` e `i` são os nomes dos dois parâmetros, mais operações aritméticas puras. Ainda não há `A[i+1]`, indexação múltipla/multidimensional, escrita no kernel ou JIT genérico de loops arbitrários.
