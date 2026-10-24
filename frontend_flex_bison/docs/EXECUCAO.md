# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E5 — tarefas (`spawn`/`await`/`cancelar`) (E0–E4 concluídas)
- **Último teste verde:** suíte completa (`FULL_SUITE_OK`: 29 `.pif` + 5 binários C)
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Hardware real (medido, 2026-10-24)

- iGPU **Intel(R) Iris(R) Xe Graphics**, OpenCL 3.0 NEO, **7102 MB**, workgroup 512,
  **80 CU**, **sem `cl_khr_fp64`** → kernels em `float`.
- iGPU só do lado Windows; `python.exe` e `pyopencl` funcionam a partir do WSL (interop).
- **A GPU perde da CPU**: ~7–8× mais lenta em 200k e 1M elementos, porque o transporte
  WSL↔Windows domina. Números em `docs/BENCHMARKS_GPU.md`. `auto` = CPU; `gpu:0` é explícito.

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` (12 casos C1–C12) e §231–238 do `.tex` corrigido.
- **E1** — iterador isolado; `>|` com expressão arbitrária; falha por índice; fluxo aninhado
  sem deadlock; `error_message` por thread; slots anotados fora do caminho de trabalhador.
- **E2** — `modo_processos(n)`/`modo_threads()`; fork + contador em `mmap(MAP_SHARED)`;
  resultado serializado por pipe; `poll()` + timeout 60 s; guarda de fork por `/proc/self/status`.
- **E3** — `backend/devices.[ch]`, `tools/pifari_worker.py`, builtins `dispositivos()`/`usar(id)`,
  `make test-gpu`. Descoberta: `cpu:0` + `gpu:0` (Iris Xe).
- **E4** — `backend/gpu_client.[ch]`: worker persistente `--stdio`, base64 sobre pipe,
  `map_affine_relu` na GPU com fallback para CPU; resultado idêntico ao da CPU (diff 0 nos
  dados inteiros testados); benchmark publicado em `docs/BENCHMARKS_GPU.md`.

## Próximos 3 passos (E5)

1. Lexer: tokens `spawn` e `await`; parser: `simple_statement: SPAWN expr` e `expr: AWAIT expr`
   (prefixo). Conferir que `%expect 7` continua válido.
2. `backend/task.[ch]`: `PifariTask { pthread_t, _Atomic status/cancel, Value result, char *error }`,
   env clonado como no worker, registro global para encerramento ordenado.
3. `await` faz join e propaga a falha original; `cancelar(t)` marca flag e a tarefa termina
   `Cancelled` no próximo ponto seguro; teste de cancelamento < 500 ms.

## Decisões tomadas (não reabrir)

- `|<` agrega e liga ao nome à direita; forma expressiva adiada (16 conflitos shift/reduce).
- Modo de processo e dispositivo são decisões de runtime, sem sintaxe nova.
- `fork` só em processo single-thread (guarda por `/proc/self/status`).
- GPU: `float` obrigatório (sem fp64 no device); `auto` = CPU por medição, GPU explícita.
- Para a GPU ganhar seria preciso buffer residente + handles (1 transporte para k operações);
  registrado como próximo passo pós-E7, não entra agora.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1–E4), `backend/devices.[ch]`, `backend/gpu_client.[ch]`,
  `tools/pifari_worker.py`
- `frontend/parser.y` (revertido ao estado de 7 conflitos)
- `examples/conc_fluxos.pif`, `conc_processos.pif`, `gpu_dispositivos.pif`, `gpu_affine.pif`
- `Makefile` (`test-gpu`), `docs/SPEC_CONCORRENCIA.md`, `docs/BENCHMARKS_GPU.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
