# OTIMIZACOES — o que o runtime faz para ser rápido (e a prova de que não mente)

Medições nesta máquina (WSL Ubuntu, gcc 13.3, CPU 12 threads). Números absolutos variam
com carga; as **razões** são o que importa. Reproduza com os comandos indicados.

## 1. JIT: 400× em laço numérico

Função de um parâmetro inteiro com atribuição definida é compilada para C e carregada como
`.so`. Mesmo resultado, tempo diferente:

| Execução do laço de 200 000 iterações | Tempo |
|---|---:|
| Interpretador (AST) | 14,6 ms |
| JIT | **36 µs** |
| Razão | ~400× |

Reproduzir: `./pifari examples/runtime_fused_chain.pif` ou qualquer laço `range(n)` dentro de
função de 1 parâmetro.

## 2. Tiering: não pagar gcc por função que roda uma vez

`PIFARI_OPT=1` (padrão) começa interpretado e compila depois de **16 chamadas ou 50 ms**
acumulados (`PIFARI_JIT_THRESHOLD`, `PIFARI_JIT_BUDGET_MS`). `PIFARI_OPT=2` compila no
registro da função.

| Cenário | Antes (compila sempre) | Agora (tiering) |
|---|---:|---:|
| Função chamada 1 vez | ~850 ms (gcc) | **~25 ms** |
| Função chamada 200× | ~850 ms + rápido | ~850 ms + rápido (sobe após amadurecer) |

## 3. Cache de aquecimento: 78× entre a primeira e a segunda execução

Manifesto `~/.cache/pifari/warmup.txt` guarda, por função, chamadas e tempo acumulados.
Na execução seguinte a função nasce "madura" e entra no JIT na primeira chamada; o `.so` já
está em `~/.cache/pifari/jit/` (não em `/tmp`, que some no reboot do WSL).

Carga: 20 chamadas de laço com 200 000 iterações.

| Execução | Tempo |
|---|---:|
| Fria (sem cache) | 2 572 ms |
| Morna (manifesto + `.so`) | **33 ms** |
| Razão | ~78× |

Reproduzir:

```bash
rm -rf ~/.cache/pifari
time ./pifari programa_pesado.pif   # frio
time ./pifari programa_pesado.pif   # morno
```

Desligar: `PIFARI_WARMUP=off`.

## 4. Contrato: otimizar não muda resultado

- `PIFARI_OPT=0` desliga bytecode, JIT e kernel nativo (referência).
- `make test-equiv` compara stdout **byte a byte** e código de saída entre `OPT=0` e `OPT=2`.
  Resultado atual: **26 programas idênticos**, 8 pulados por serem não determinísticos por
  natureza (tempo, `thread_id`, print dentro de fluxo paralelo).
- `make test-golden` congela a saída de 23 programas e acusa qualquer mudança.

## 5. O que ainda não é otimizado

- Laço `for` puro ainda não vira kernel nativo; o IR cobre expressão unária e `(A, i)`.
- Sem vetorização SIMD explícita.
- Sem seleção adaptativa entre planos por amostra (o tiering cobre o caso comum).
- GPU: existe e funciona, mas perde da CPU por transporte (`docs/BENCHMARKS_GPU.md`).

## 6. Como inspecionar

```bash
PIFARI_OPT_DEBUG=1 ./pifari programa.pif   # escolhe plano e tier-up
PIFARI_OPT=0 ./pifari programa.pif         # referência sem otimização
PIFARI_JIT_BUDGET_MS=5 ./pifari programa.pif  # sobe para JIT mais cedo
```

## 7. Paridade de erros no JIT (corrigido)

O C gerado usava `+ - * / %` diretos: overflow **envolvia** e divisão por zero **derrubava o
processo**, enquanto o interpretador levantava erro. Corrigido com helpers que devolvem código
de erro (`pifari_jit_add/sub/mul/div/mod/neg`), assinatura
`long long pifari_jit(long long arg0, int *err)` e mensagem no ponto de chamada.

| Programa | `OPT=0` | `OPT=2` |
|---|---|---|
| `9e18 + 9e18` em laço | erro: overflow de inteiro | erro: overflow de inteiro |
| `total // 0` em laço | erro: divisão por zero | erro: divisão por zero |

A mensagem do JIT nomeia a função (`overflow de inteiro em 'cresce'`) em vez da linha do
operador; a categoria e o código de saída são os mesmos. É a diferença aceita no contrato:
o mesmo erro, no mesmo ponto lógico.
