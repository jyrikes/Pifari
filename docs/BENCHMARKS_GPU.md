# BENCHMARKS_GPU — Iris Xe vs CPU no Pifari

Medido em 2026-10-24, nesta máquina (WSL Ubuntu + Windows 11), com o worker
`tools/pifari_worker.py --stdio` e o kernel `map_affine_relu` em `float`.

## Hardware

| Item | Valor |
|---|---|
| iGPU | Intel(R) Iris(R) Xe Graphics |
| OpenCL | 3.0 NEO, 80 compute units, workgroup máx 512 |
| Memória | 7102 MB (compartilhada com a CPU) |
| `cl_khr_fp64` | **ausente** → kernels em `float` |
| CPU | 12 threads visíveis no WSL |

## Operação medida: `map_affine_relu(lista, 2, -500)`

Tempo dentro do interpretador (`agora_ns()` em volta da chamada), mediana de uma
execução por tamanho. "GPU frio" inclui compilar o kernel; "GPU quente" reusa
contexto e kernel.

| N | GPU frio | GPU quente | CPU (12 threads) | GPU/CPU quente |
|---:|---:|---:|---:|---:|
| 200.000 | 1.281 ms | 122–135 ms | 15 ms | **~8× mais lento** |
| 1.000.000 | 1.645 ms | 442 ms | 62 ms | **~7× mais lento** |

## Correção do resultado

`maior_diferenca_absoluta = 0` em 200.000 elementos: os valores do teste
(`2i − 500`) são inteiros representáveis exatamente em `float32`, então não houve
divergência. Para dados fracionários a tolerância esperada é de alguns ULP de
`float32` (≈1e-7 relativo), nunca precisão de `double`.

## Interpretação honesta

O kernel em si é irrelevante neste tamanho: 1M de elementos é menos de 1 ms de
GPU. Todo o custo está no **transporte**:

1. serialização da lista para `double[]` (cópia em C);
2. base64 no lado WSL (~33% de inflação);
3. pipe do WSL para o processo Windows (interop);
4. `numpy` + `pyopencl` copiando para o device e de volta;
5. base64 e parse no caminho inverso.

Resultado: ~7–8× mais lento que a CPU em todos os tamanhos testados. O `auto`
**permanece em CPU**; a GPU só é usada com `usar("gpu:0")` explícito. Isso é
exatamente o portão de medição previsto em `docs/SPEC_CONCORRENCIA.md` §7.4.

## O que faria a GPU ganhar

Buffers residentes no device, com uma API de handles:

```
h = dispositivo_upload(lista)      # custo de transporte pago uma vez
r = dispositivo_executar(h, op)    # N chamadas sem transporte
lista2 = dispositivo_baixar(r)
```

Para uma cadeia de k operações sobre o mesmo N, o transporte deixa de ser k× e
passa a ser 1×. É a próxima etapa natural — mas exige um tipo novo (handle) no
runtime, então fica fora do escopo desta entrega.
