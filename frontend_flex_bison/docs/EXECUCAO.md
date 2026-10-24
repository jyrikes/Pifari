# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E2 — multiprocessos (E0 e E1 concluídas)
- **Último teste verde:** 27/27 `.pif` + 5 binários C + `examples/conc_fluxos.pif`, nesta máquina
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` com as regras normativas dos quatro operadores, escopo,
  falha, tarefas, memória compartilhada e dispositivos; 12 casos de conformidade (C1–C12).
- **E0** — `Definicao_Pifari_v3.tex` §231–238 corrigido e compilado (23 páginas, sem erro).
- **E1** — iterador isolado em `|>` (frame próprio) e `>|` com `eval` de expressão arbitrária.
- **E1** — falha por item: `ParallelWork.errors[]` + `record_parallel_error`; a barreira reporta
  `fluxo paralelo falhou no índice N: <mensagem>` e os demais itens completam.
- **E1** — reentrância: `_Thread_local in_worker` + `run_work_serial`; `>|` aninhado não trava mais.
- **E1** — `fail()` guarda a mensagem em `_Thread_local error_message[256]` (base para `await` em E5).
- **E1** — anotação de slots não muta AST compartilhada a partir de trabalhador.
- **E1** — `examples/conc_fluxos.pif` (paralelo == sequencial, aninhamento) e entrada no `make test`.

## Próximos 3 passos (E2)

1. `backend/serialize.[ch]`: `Value` ↔ binário, com limite de profundidade/bytes e recusa de handles.
2. `backend/process_pool.[ch]`: `fork()` de N filhos, pipe por filho, `[índice][tamanho][payload]`,
   coleta fora de ordem preservando índice, `poll()` + timeout + `SIGKILL`.
3. Builtins `modo_processos(n)` / `modo_threads()` e teste de equivalência byte a byte com o modo thread.

## Decisões tomadas (não reabrir)

- `|<` agrega e liga ao nome à direita; forma puramente expressiva **adiada** (a produção extra
  levava a gramática de 7 a 16 conflitos shift/reduce). Ver §2.1 do SPEC.
- Modo de processo e dispositivo são decisões de runtime, sem sintaxe nova (`modo_processos`, `usar`).
- GPU local exige registrar o ICD da Intel (admin, uma vez). Sem isso, o plano segue pelo worker
  remoto e a GPU local fica indisponível — não é bloqueio do restante.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1)
- `frontend/parser.y` (revertido ao estado de 7 conflitos)
- `examples/conc_fluxos.pif` (novo), `Makefile`
- `docs/SPEC_CONCORRENCIA.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
