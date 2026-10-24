# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E4 — execução OpenCL na iGPU (E0–E3 concluídas)
- **Último teste verde:** suíte completa (29 `.pif` + 5 binários C) antes da E3; depois da E3,
  subconjunto de fluxo + `make test-gpu` verdes
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Hardware real (medido, 2026-10-24)

- iGPU **Intel(R) Iris(R) Xe Graphics**, OpenCL 3.0 NEO, **7102 MB** compartilhados,
  workgroup máx **512**, **80 compute units**, **sem `cl_khr_fp64`** (kernels em `float`).
- A iGPU só é alcançável do **lado Windows**; o WSL não tem `/dev/dri`.
- `pyopencl` instalado no Python do Windows (`C:\Users\Yngrid Kalinne\miniconda3\python.exe`)
  e já enxerga o device — **não foi preciso registrar ICD com admin**.
- `python.exe` funciona a partir do WSL (interop).

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` (12 casos C1–C12) e §231–238 do `.tex` corrigido.
- **E1** — iterador isolado; `>|` com expressão arbitrária; falha por índice; fluxo aninhado
  sem deadlock; `error_message` por thread; slots anotados fora do caminho de trabalhador.
- **E2** — `modo_processos(n)`/`modo_threads()`; fork + contador em `mmap(MAP_SHARED)`;
  resultado serializado por pipe; `poll()` + timeout 60 s; guarda de fork por `/proc/self/status`.
- **E3** — `backend/devices.[ch]` (registro, `cpu:0` sempre, probe preguiçoso do worker),
  `tools/pifari_worker.py` (`--list`/`--stdio`), builtins `dispositivos()` e `usar(id)`,
  `make test-gpu`, `examples/gpu_dispositivos.pif`. Descoberta verificada: 2 dispositivos.

## Próximos 3 passos (E4)

1. `backend/ocl_emit.[ch]`: gerar OpenCL C do `PifariExprGraph` já existente (mesmo subconjunto
   do `native_kernel`: `+ - * / % **`, `A[i]`, relu/max/exp), em `float` (sem fp64 no device).
2. `tools/pifari_worker.py --stdio`: modo persistente; comando `MAP` com buffer binário;
   resposta `OK` + bytes. Reaproveitar o framing binário já usado no modo processos.
3. `>|` com `usar("gpu:0")` (ou `PIFARI_DEVICE=gpu:0`): enviar ao worker só quando o alvo for
   função pura suportada; comparar com CPU (tolerância 1e-5, pois é `float`); medir e publicar
   limiar em `docs/BENCHMARKS_GPU.md`.

## Decisões tomadas (não reabrir)

- `|<` agrega e liga ao nome à direita; forma expressiva adiada (16 conflitos shift/reduce).
- Modo de processo e dispositivo são decisões de runtime, sem sintaxe nova.
- `fork` só em processo single-thread (guarda por `/proc/self/status`).
- GPU sem `fp64`: kernels em `float` e testes com tolerância; não prometer exatidão de `double`.
- Custo do probe: ~3–4 s por execução (spawn do Python + init do OpenCL). O worker persistente
  da E4 elimina isso quando o programa usa GPU de verdade.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1, E2, E3), `backend/devices.[ch]`, `tools/pifari_worker.py`
- `frontend/parser.y` (revertido ao estado de 7 conflitos)
- `examples/conc_fluxos.pif`, `examples/conc_processos.pif`, `examples/gpu_dispositivos.pif`
- `Makefile` (`test-gpu`), `docs/SPEC_CONCORRENCIA.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
