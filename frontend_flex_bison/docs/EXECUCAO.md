# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E3 — descoberta de dispositivos + worker (E0, E1 e E2 concluídas)
- **Último teste verde:** 29/29 `.pif` + 5 binários C, nesta máquina (`FULL_SUITE_OK`)
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` (regras normativas, 12 casos C1–C12) e §231–238 do
  `Definicao_Pifari_v3.tex` corrigido e compilado (23 páginas).
- **E1** — iterador isolado em `|>`; `>|` aceita expressão arbitrária; falha por índice
  (`ParallelWork.errors[]`); `run_work_serial` elimina o deadlock do fluxo aninhado;
  `error_message` por thread; anotação de slots fora do caminho de trabalhador.
- **E2** — `modo_processos(n)` / `modo_threads()`; distribuição em processos com contador
  atômico em `mmap(MAP_SHARED)`, filhos `fork()` lendo os itens por copy-on-write e devolvendo
  só o resultado serializado por pipe; `poll()` + timeout de 60 s + `SIGKILL`; índice e mensagem
  de erro por item; guarda de `fork` baseada em `/proc/self/status` (Threads > 1 → recusa).
- **E2** — `examples/conc_processos.pif`: mesmo fluxo em processos e threads dá o mesmo
  resultado; nenhum processo órfão após a execução (verificado com `pgrep`).

## Próximos 3 passos (E3)

1. `backend/devices.[ch]`: registro de dispositivos (`cpu:0` sempre; `gpu:0` quando houver),
   campos id/backend/nome/memória/ops, e seleção por `PIFARI_DEVICE`/`PIFARI_GPU_THRESHOLD`.
2. `tools/pifari_worker.py` (lado Windows): `--stdio` e `--tcp host:port`; handshake `HELLO`
   devolvendo dispositivos; usa `pyopencl` (pip, disponível) para OpenCL na Iris Xe.
3. Builtins `dispositivos()` e `usar(id)`; `usar("gpu:0")` só passa a valer quando o worker
   responder — sem worker, degrada para CPU com aviso.

## Bootstrap pendente (uma vez, com admin)

O ICD da Intel existe no DriverStore (`Intel_OpenCL_ICD64.dll`) mas não está registrado em
`HKLM\SOFTWARE\Khronos\OpenCL\Vendors` — hoje o loader reporta zero plataformas. Registrar
antes da E4 (comando exato no plano). Sem isso, E3 funciona com `cpu:0` e worker remoto.

## Decisões tomadas (não reabrir)

- `|<` agrega e liga ao nome à direita; forma puramente expressiva **adiada** (levava a
  gramática de 7 para 16 conflitos shift/reduce). Ver §2.1 do SPEC.
- Modo de processo e dispositivo são decisões de runtime, sem sintaxe nova.
- `fork` só é permitido em processo single-thread; a guarda verifica `/proc/self/status`.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados

- `backend/runtime.c` (E1 e E2: serialização de `Value`, pool de processos, builtins de modo)
- `frontend/parser.y` (revertido ao estado de 7 conflitos)
- `examples/conc_fluxos.pif`, `examples/conc_processos.pif`, `Makefile`
- `docs/SPEC_CONCORRENCIA.md`, `docs/EXECUCAO.md`
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
