# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2750 linhas).

## Estado

- **Etapa atual:** F5 — verificação final e benchmarks (F0–F4 concluídas)
- **Último teste verde:** `test-core` exit 0, `check-core` exit 0, `test-equiv` EQUIV_OK
- **Plano:** F0–F5; semântica em `docs/SPEC_SEMANTICA.md` (§13 cache, §11 check, §12 efeitos)

## Concluído

- **F0** — semântica declarada; JIT com atribuição definida; overflow detectável; `//` e `%`
  truncando; diretivas falham; `dict` fora do léxico.
- **F1a/b/c** — `types.[ch]`; `=:` verificado; parâmetros e retorno tipados;
  `typecheck.[ch]` + `pifari --check` + `make check-core`.
- **F2** — `effects.[ch]` com puro/leitura/escrita/io, análise transitiva, `efeitos()`,
  gate `EFFECT_PURE` no kernel nativo.
- **F3a** — `plan.[ch]`: `PIFARI_OPT=0|1|2`, `PIFARI_OPT_DEBUG=1`, gates de bytecode/JIT/nativo,
  `tests/equiv.sh` + `make test-equiv` (26 iguais, 8 pulados, EQUIV_OK).
- **F3b** — tiering do JIT: `OPT=1` começa interpretado e compila após **16 chamadas ou 50 ms**
  acumulados (`PIFARI_JIT_THRESHOLD`, `PIFARI_JIT_BUDGET_MS`); `OPT=2` compila no registro.
  Chamada única não paga gcc (~25 ms em vez de ~850 ms).
- **F4** — `backend/cache.[ch]`: cache persistente em `~/.cache/pifari/{jit,kernels}` (0700),
  com queda para `/tmp`; `warmup.txt` com `fn <nome> <fingerprint> <chamadas> <tempo_ns>`;
  seed na inicialização faz a função entrar no JIT na primeira chamada; gravação atômica;
  `PIFARI_WARMUP=off`. Medido: frio 2 572 ms → morno **33 ms** (78×).

## Próximos 3 passos (F5)

1. `tests/golden/`: saída canônica dos programas determinísticos do `test-core`; alvo
   `make test-golden` regenerando quando combinado (`--atualizar`).
2. `docs/OTIMIZACOES.md`: tabela antes/depois por exemplo (JIT frio vs morno, tiering,
   `OPT=0` vs `OPT=2`), sempre validada por `test-equiv`.
3. Fechar: rodar `test-core`, `check-core`, `test-equiv`, `test-gpu` e `test-golden`;
   atualizar `STATUS_REAL.md` e o README com a seção de otimização.

## Observações

- `/tmp` do WSL é limpo entre sessões — mais um motivo para o cache viver em `~/.cache`.
- **Nunca** rodar `examples/orquestrador_reels_aistudio.pif` (chama Gemini e gasta quota).
- `teste_cliente_servidor_mcp_pifari.pif` é flaky (timeout do mock MCP).
- 8 programas do `test-core` são não determinísticos por natureza e são pulados no equiv.
- Falta o IR estendido para laços (F3b original): hoje o kernel nativo cobre expressão
  unária e `(A,i)`; laço `for` puro ainda não vira kernel. Registrado como próximo passo
  opcional pós-F5.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Efeito não altera execução: só autoriza/veta especialização.
- `OPT=0` desliga bytecode, JIT e kernel nativo: é a referência.
- Contrato de equivalência vale para programa determinístico.
- Manifesto de aquecimento nunca muda resultado, só quando o JIT entra.

## Arquivos tocados

- `backend/`: types, typecheck, effects, plan, cache (novos) + runtime, native_kernel
- `frontend/parser.y`, `frontend/lexer.l`, `Makefile`, `tests/equiv.sh`
- `docs/`: SPEC_SEMANTICA (novo), STATUS_REAL, EXECUCAO
- `examples/`: tipos.pif, efeitos.pif (novos)
