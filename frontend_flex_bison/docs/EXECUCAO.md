# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E7 — polimento e regressão final (E0–E6 concluídas)
- **Último teste verde:** `FULL_SUITE_OK` com 30 `.pif` + 5 binários C (inclui `conc_shared.pif`)
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo)

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` (C1–C12) e §231–238 do `Definicao_Pifari_v3.tex`.
- **E1** — iterador isolado; `>|` com expressão arbitrária; falha por índice; sem deadlock no
  aninhamento; `error_message` por thread.
- **E2** — `modo_processos(n)`/`modo_threads()`; fork + contador `mmap(MAP_SHARED)`; resultado
  por pipe; `poll()` + timeout; guarda de fork por `/proc/self/status`.
- **E3** — `backend/devices.[ch]`, `tools/pifari_worker.py`, `dispositivos()`/`usar(id)`, `test-gpu`.
- **E4** — `backend/gpu_client.[ch]`; `map_affine_relu` na Iris Xe com fallback; benchmark em
  `docs/BENCHMARKS_GPU.md` (GPU perde ~7–8× por transporte; `auto` = CPU).
- **E5** — `spawn`/`await` prefixos + `cancelar(id)`; cancelamento cooperativo nos três caminhos
  (exec_block, For de exec_node, VM de função). Tarefa de ~2 s termina em 69 ms após cancelar.
- **E6** — `compartilhado`/`ler`/`escrever`/`incrementar` com mutex por célula; 4 tarefas × 1000
  incrementos = **4000** (sem atualização perdida). Verificado também: escrita dentro de função
  não altera o escopo externo (`acc` continua 100), então o erro de "escrita externa" do §3.3 é
  inalcançável por construção hoje — o comportamento seguro é o padrão.

## Próximos passos (E7)

1. `Makefile`: `test-core` (C + todos os `.pif` menos o orquestrador de Reels), `test-conc`
   (conc_*.pif), manter `test-gpu`.
2. Rodar `make test-core`, `make test-conc`, `make test-gpu` e registrar os resultados.
3. Atualizar `README.md` (tabela de estado) e fechar o checkpoint.

## Observação registrada (não introduzida por este trabalho)

Dentro de função, um nome atribuído que coincide com variável externa é tratado como slot
local inicializado em `Null` (que vira 0 na aritmética) em vez de herdar o valor externo.
É semântica de função pré-existente; fica anotada para revisão futura, fora do escopo da
concorrência.

## Decisões (não reabrir)

- `spawn`/`await` são expressões prefixas (`t = spawn f()`).
- Cancelamento exige checagem nos três caminhos de execução — os três estão cobertos.
- `|<` agrega e liga ao nome à direita; forma expressiva adiada (16 conflitos shift/reduce).
- GPU em `float`; `auto` = CPU por medição; handles/resident buffers ficam pós-E7.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1–E6), `backend/devices.[ch]`, `backend/gpu_client.[ch]`,
  `tools/pifari_worker.py`, `frontend/parser.y`, `frontend/lexer.l`
- `examples/conc_fluxos.pif`, `conc_processos.pif`, `conc_tarefas.pif`, `conc_shared.pif`,
  `gpu_dispositivos.pif`, `gpu_affine.pif`
- `Makefile`, `docs/SPEC_CONCORRENCIA.md`, `docs/BENCHMARKS_GPU.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex`
