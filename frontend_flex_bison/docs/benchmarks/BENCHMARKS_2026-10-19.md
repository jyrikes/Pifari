# Benchmarks do Pifari — 2026-10-19

## Ambiente

- CPU: Intel Core i5-1235U de 12ª geração, 12 CPUs lógicas (6 núcleos físicos / 2 threads por núcleo).
- Sistema de compilação: GCC 13.3.0, Ubuntu 24.04 no WSL.
- Python: 3.12.3.
- Flags principais do runtime: `-O3 -flto -march=native`; os kernels nativos são compilados com `-O3 -march=native -fPIC -shared`.
- Método: mediana de 5 amostras no benchmark de CPU e 3 no benchmark de matrizes. Os scripts verificam igualdade dos resultados/checksums. Máquina de desenvolvimento, sem isolamento de CPU; os tempos variam com carga do sistema.

## CPU — soma de quadrados em quatro tarefas de 200.000 itens

| Caminho | Mediana | Amostras (ms) |
|---|---:|---|
| Pifari sequencial (kernel medido internamente) | 0,659 ms | 1,395; 0,550; 0,577; 0,859; 0,659 |
| Pifari paralelo (kernel + pool) | 1,556 ms | 3,526; 2,101; 1,556; 1,474; 1,366 |
| Pifari, processo completo | 39,22 ms | 52,34; 36,06; 39,22; 38,42; 41,51 |
| Python sequencial | 105,21 ms | 107,26; 97,67; 104,22; 127,40; 105,21 |
| Python ThreadPoolExecutor, 4 threads | 192,75 ms | 186,29; 168,76; 192,75; 214,10; 195,70 |
| Python ProcessPoolExecutor, pool aquecido | 120,28 ms | 55,37; 125,22; 133,76; 120,28; 96,50 |
| Python ProcessPoolExecutor, criação do pool + trabalho | 108,43 ms | 199,81; 138,40; 95,51; 108,43; 108,24 |

Todos os caminhos produziram quatro vezes o resultado `2666646666700000`.

### Razões calculadas a partir das medianas

- Python sequencial / Pifari sequencial: aproximadamente **159,7×** para esse kernel e essa configuração.
- Python ThreadPoolExecutor / Pifari paralelo: aproximadamente **123,9×** para esse caso específico.
- O caminho paralelo do Pifari ficou mais lento que o sequencial neste tamanho de tarefa (1,556 ms contra 0,659 ms). Isso evidencia o custo de agendamento/coordenação; paralelismo não garante ganho em toda carga.
- O processo completo do Pifari levou 39,22 ms de mediana. Não comparar esse número diretamente com o tempo interno do kernel: o processo completo inclui inicialização e preparação do runtime.

Os fatores de aceleração são específicos desse benchmark, não uma promessa geral de desempenho. As amostras do ProcessPoolExecutor variaram bastante; a medição precisa ser repetida em máquina ociosa para uma comparação formal.

## Multiplicação de matrizes 60 × 60

| Caminho | Mediana | Amostras (ms) |
|---|---:|---|
| Pifari com loops interpretados | 266,241 ms | 222,634; 296,457; 266,241 |
| Pifari `matmul` nativo | 1,092 ms | 0,679; 1,389; 1,092 |
| Python com loops puros | 36,26 ms | 36,265; 43,696; 26,995 |

O checksum de todas as implementações foi `-68440`.

- Pifari interpretado / Pifari `matmul` nativo: aproximadamente **243,8×**.
- Python puro / Pifari `matmul` nativo: aproximadamente **33,2×**.
- Python puro foi aproximadamente **7,3× mais rápido** que os loops interpretados do Pifari nesse teste. Portanto, a aceleração vem da primitiva matricial nativa; não é evidência de que os loops interpretados sejam mais rápidos que Python.

## Validação da rodada

Executado em 2026-10-19:

- `make clean && make -j2`: concluído; sem warnings do código-fonte, apenas mensagem informativa do LTO sobre jobs LTRANS serializados.
- `make test`: passou, incluindo testes de otimizador, IR, Dirac, kernel nativo serial/paralelo e exemplos do runtime.
- `make test-fuzz`: 120 entradas pseudoaleatórias, sem crashes ou timeouts.
- `make benchmark`: os dois benchmarks terminaram e validaram resultados/checksum.

## Como repetir

Na raiz `frontend_flex_bison`:

```bash
make clean && make -j2
make test
make test-fuzz
make benchmark
```

Para reduzir variação: feche tarefas pesadas, execute mais de uma rodada e compare medianas. O benchmark de CPU separa o tempo interno medido pelo programa do custo de iniciar o processo; o benchmark de matrizes valida o checksum antes de aceitar os resultados.

## Interpretação arquitetural

O pipeline AST → IR → kernel nativo está disponível para funções aritméticas puras de um parâmetro, e o fluxo paralelo reconhece um padrão específico de map numérico. O `matmul` nativo é uma primitiva especializada. O JIT ainda não compila de forma geral indexação, loops aninhados ou todos os tipos; listas genéricas também ainda são convertidas na fronteira do kernel. Essas limitações devem ser resolvidas antes de caracterizar o runtime como compilador otimizante geral.
