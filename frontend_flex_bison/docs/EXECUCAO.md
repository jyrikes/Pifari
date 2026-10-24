# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E6 — memória compartilhada (E0–E5 concluídas)
- **Último teste verde:** `FULL_SUITE_OK` (29 `.pif` + 5 binários C) após a gramática de tarefas
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Hardware real (medido, 2026-10-24)

- iGPU **Intel(R) Iris(R) Xe Graphics**, OpenCL 3.0 NEO, 7102 MB, 80 CU, **sem `cl_khr_fp64`**.
- GPU **perde da CPU** (~7–8×) por causa do transporte WSL↔Windows: `docs/BENCHMARKS_GPU.md`.
  `auto` = CPU; `gpu:0` só explícito; resultado correto (diff 0 nos dados inteiros testados).

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` (C1–C12) e §231–238 do `.tex`.
- **E1** — iterador isolado; `>|` com expressão arbitrária; falha por índice; aninhamento sem
  deadlock; `error_message` por thread.
- **E2** — `modo_processos(n)`/`modo_threads()`; fork + contador `mmap(MAP_SHARED)`; resultado
  por pipe; `poll()` + timeout; guarda de fork por `/proc/self/status`.
- **E3** — `backend/devices.[ch]`, `tools/pifari_worker.py`, `dispositivos()`/`usar(id)`,
  `make test-gpu`.
- **E4** — `backend/gpu_client.[ch]`: worker persistente `--stdio`, `map_affine_relu` na GPU
  com fallback para CPU, benchmark publicado.
- **E5** — `spawn`/`await` (expressões prefixas) e `cancelar(id)`; tarefa com ambiente clonado,
  registro de 64 slots, status atômico; cancelamento cooperativo verificado nos laços da VM de
  função (sem ele o cancelamento só era visto no fim). Medido: tarefa que levaria ~2 s termina
  em **69 ms** após `cancelar`. Falha propaga: `tarefa falhou: divisão por zero`.

## Próximos 3 passos (E6)

1. `compartilhado(v)`, `ler(c)`, `escrever(c,v)`, `incrementar(c,d)`: célula com `pthread_mutex_t`
   guardando um `Value` (o valor é `static` no registro? não: alocar por handle).
2. Verificação de escrita externa na entrada de `>|` e de `spawn`: varrer a AST do alvo e falhar
   quando `Assign`/`IndexAssign` resolver para binding fora do frame do fluxo.
3. Teste C8: contador com 4×1000 incrementos = 4000; teste C9: escrita externa dá erro.

## Decisões tomadas (não reabrir)

- `spawn`/`await` são **expressões prefixas**, não instruções: permite `t = spawn f()`.
- O cancelamento precisa de checagem nos três caminhos de execução: `exec_block`, ramos `For` de
  `exec_node` e o laço da VM de função (`while(pc<...&&!PIFARI_TASK_CANCELLED())`).
- `|<` agrega e liga ao nome à direita; forma expressiva adiada (16 conflitos shift/reduce).
- GPU: `float` obrigatório; `auto` = CPU por medição; handles/resident buffers ficam pós-E7.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1–E5), `backend/devices.[ch]`, `backend/gpu_client.[ch]`,
  `tools/pifari_worker.py`, `frontend/parser.y`, `frontend/lexer.l`
- `examples/conc_fluxos.pif`, `conc_processos.pif`, `conc_tarefas.pif`,
  `gpu_dispositivos.pif`, `gpu_affine.pif`
- `Makefile` (`test-gpu`), `docs/SPEC_CONCORRENCIA.md`, `docs/BENCHMARKS_GPU.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
