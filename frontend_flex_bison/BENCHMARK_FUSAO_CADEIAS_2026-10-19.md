# Fusão nativa de cadeias — benchmark inicial (2026-10-19)

## O que foi implementado

- `pifari_graph_compose` compõe dois grafos IR unários, preservando dependências e otimizando o resultado.
- O builtin `map_fused_chain(["dobro", "soma_tres"], dados[, workers])` aceita uma lista de nomes de funções aritméticas puras suportadas pelo IR e aplica a composição como um kernel C nativo único.
- A cadeia usa buffers F64 alinhados e evita materializar uma lista intermediária entre funções. O resultado final ainda é uma lista genérica de Values.
- A entrada aceita lista numérica ou `range(N)`. Para range, os valores são preenchidos diretamente no buffer F64.
- O kernel compilado é armazenado em cache por sequência de nomes durante a vida do processo. As funções precisam permanecer inalteradas durante essa execução.
- Exemplo funcional: `examples/runtime_fused_chain.pif`. Benchmark: `examples/benchmark_fused_chain.pif`.

## Benchmark

Máquina: mesma máquina de desenvolvimento documentada em `BENCHMARKS_2026-10-19.md` (Core i5-1235U, WSL Ubuntu 24.04, GCC 13.3.0). 20.000 itens; cadeia `dobro(x) = x*2` seguida de `soma_tres(x) = x+3`. Três execuções de processo, relógio monotônico via `agora_ns()`. Os valores medidos são desta carga e deste host, não uma garantia geral.

| Caminho | Amostras | Mediana aproximada |
|---|---:|---:|
| Duas etapas no fluxo, incluindo listas intermediárias | 686,63; 777,47; 1107,65 ms | 777,47 ms |
| Cadeia fundida, primeira chamada (compilação + execução) | 359,40; 468,71; 506,26 ms | 468,71 ms |
| Cadeia fundida, kernel aquecido e em cache | 0,994; 0,991; 0,789 ms | 0,991 ms |

A cadeia fundida retornou 20.000 itens em todas as execuções; o primeiro e último valores foram `3` e `40001`. A diferença entre a primeira chamada e a chamada aquecida é dominada pelo custo de gerar/compilar o kernel C. A comparação sequencial/fundida é específica a esse runtime e a essa carga; repetir a chamada fundida reutiliza o kernel compilado e evita as listas intermediárias.

## Limites importantes

- Esta é uma cadeia de funções **aritméticas puras de um argumento** suportadas pelo IR, não fusão automática de qualquer função Pifari.
- Funções com efeitos colaterais, chamadas arbitrárias, variáveis livres, condicionais, loops e indexação de matrizes não são aceitas pelo IR nativo atual.
- O runtime ainda converte listas genéricas para F64 na fronteira e converte o resultado final para lista genérica. `range(N)` evita a conversão da entrada ao materializar o range diretamente no buffer.
- O cache vive até o encerramento do processo; alterações dinâmicas de definições de função durante a execução não são suportadas.
- Próximo passo para arrays tipados completos: um tipo de array de primeira classe com shape/strides e ABI explícita, seguido de IR para load/store indexado com verificação de limites. Isso ainda não está implementado.
