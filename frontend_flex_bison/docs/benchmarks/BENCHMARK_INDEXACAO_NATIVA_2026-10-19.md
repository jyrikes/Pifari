# Benchmark: JIT de indexação F64 vs Python

Data: 2026-10-19. Vetor de 50.000 elementos. Todas as implementações calculam `out[i] = data[i] * 2 + 3`, com dados preparados fora da região cronometrada. O tempo Pifari aquecido inclui alocação do array de saída e execução do kernel; exclui apenas a compilação inicial. Python usa listas e `float` built-in, sem NumPy.

## Resultado medido

| Implementação | Mediana |
|---|---:|
| Pifari primeira chamada (compilação + execução) | 635,390 ms |
| Pifari JIT indexado (kernel aquecido) | 0,361 ms |
| Pifari interpretado (índice + `array_set`) | 36,034 ms |
| Python `for` com lista pré-alocada | 7,162 ms |
| Python list comprehension | 6,806 ms |

- JIT aquecido vs Pifari interpretado: **99,80×**.
- JIT aquecido vs Python `for`: **19,83×**.
- JIT aquecido vs Python list comprehension: **18,85×**.

A primeira chamada é muito mais lenta porque compila o kernel C em tempo de execução; ela não deve ser confundida com o custo por chamada depois do cache aquecer. Os números são específicos deste host, carga e tamanho de vetor. O script `examples/benchmarks/benchmark_indexed_vs_python.py` repete a medição (3 processos Pifari, 7 rodadas por implementação Python), valida os valores inicial/final e calcula as medianas.

## O que está sendo medido

A função Pifari recebe `(A, i)` e retorna `A[i] * 2 + 3`. O lowering AST→IR reconhece precisamente `A[i]`, reduz o carregamento a `PEX_INPUT`, e o gerador de C emite uma leitura direta de `input[i]` no laço nativo. `map_indexed_native` cacheia o IR e o kernel nativo na definição da função durante a execução do programa.

Limitações: apenas `A[i]` com o nome exato dos parâmetros e expressões aritméticas puras (`+`, `-`, `*`, `/`, unário `-`). Não suporta offsets (`A[i+1]`), múltiplos arrays, store nativo ou arrays multidimensionais nesse JIT especializado.
