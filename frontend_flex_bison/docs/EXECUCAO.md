# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2750 linhas).

## Estado

- **Etapa atual:** F3c — IR estendido para laço puro (único item pendente; F0–F5 feitas)
- **Testes verdes:** `test-core`, `check-core`, `test-equiv` (EQUIV_OK), `test-golden`
  (GOLDEN_OK) e `test-gpu`
- **Semântica:** `docs/SPEC_SEMANTICA.md` (§8 contrato, §11 check, §12 efeitos, §13 cache)
- **Números:** `docs/OTIMIZACOES.md`

## Concluído

- **F0** — semântica declarada; JIT com atribuição definida; overflow detectável; `//` e `%`
  truncando; diretivas falham; `dict` fora do léxico.
- **F1a/b/c** — tipos (`types.[ch]`), `=:` verificado, parâmetros/retorno tipados,
  `typecheck.[ch]`, `pifari --check`, `make check-core`.
- **F2** — `effects.[ch]`: puro/leitura/escrita/io transitivo, `efeitos()`, gate de kernel.
- **F3a** — `plan.[ch]`: `PIFARI_OPT`, `PIFARI_OPT_DEBUG`, gates, `tests/equiv.sh`.
- **F3b** — tiering do JIT (16 chamadas ou 50 ms; `PIFARI_JIT_THRESHOLD`,
  `PIFARI_JIT_BUDGET_MS`); `OPT=2` compila no registro.
- **F4** — `cache.[ch]` + `warmup.txt`: cache persistente em `~/.cache/pifari`, seed por
  função, gravação atômica, `PIFARI_WARMUP=off`. Frio 2 572 ms → morno 33 ms.
- **F5** — `tests/golden.sh` + `make test-golden` (23 programas congelados, 10 pulados por
  não determinismo), `docs/OTIMIZACOES.md`, README e STATUS_REAL atualizados.

## Pendente (próximo passo único)

**IR estendido para laço puro** (`backend/ast_ir.[ch]`):
1. Reconhecer corpo de função cujo único efeito é um `for i in range(n)` com acumulação
   (`total = total + f(i)`) ou mapeamento em `f64_array`.
2. Emitir kernel C de **redução** (além do `map` atual) e cachear em `~/.cache/pifari/kernels`.
3. Só aceitar função `EFFECT_PURE`, sem escrita fora do laço; comparar com `eval` no
   `test-equiv` e medir em `docs/OTIMIZACOES.md`.

Enquanto isso não entra, o JIT já cobre o laço (400×) — o kernel nativo reduziria mais,
mas o ganho incremental é menor que o risco; por isso é o único item deixado em aberto.

## Observações

- `/tmp` do WSL é limpo entre sessões: o cache vive em `~/.cache/pifari`.
- **Nunca** rodar `examples/orquestrador_reels_aistudio.pif` (Gemini, consome quota).
- `teste_cliente_servidor_mcp_pifari.pif` é flaky (mock MCP).
- `runtime_map_fused.pif` às vezes falha em criar o kernel nativo e cai para o caminho
  genérico; por isso não foi congelado no golden.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Efeito não altera execução: só autoriza/veta especialização.
- `OPT=0` = referência (sem bytecode, JIT ou kernel).
- Contrato de equivalência vale para programa determinístico.
- Manifesto de aquecimento nunca muda resultado, só quando o JIT entra.

## Arquivos tocados

- `backend/`: types, typecheck, effects, plan, cache (novos) + runtime, native_kernel
- `frontend/parser.y`, `frontend/lexer.l`, `Makefile`
- `tests/`: equiv.sh, golden.sh, golden/ (23 pares .txt/.exit)
- `docs/`: SPEC_SEMANTICA, OTIMIZACOES (novos), STATUS_REAL, EXECUCAO
- `examples/`: tipos.pif, efeitos.pif (novos)
