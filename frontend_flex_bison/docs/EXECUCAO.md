# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2650 linhas).

## Estado

- **Etapa atual:** F3b — seleção adaptativa + IR estendido (F0–F2 e F3a concluídas)
- **Último teste verde:** `test-core`, `check-core` e `test-equiv` (26 iguais, 8 pulados)
- **Plano:** F0–F5; semântica em `docs/SPEC_SEMANTICA.md` (§8 contrato, §11 check, §12 efeitos)

## Concluído

- **F0** — semântica declarada; JIT com atribuição definida; overflow detectável; `//` e `%`
  truncando; diretivas falham; `dict` fora do léxico.
- **F1a/b/c** — `types.[ch]`; `=:` verificado com promoção `int→float`; parâmetros e retorno
  tipados; `typecheck.[ch]` + `pifari --check` + `make check-core`.
- **F2** — `effects.[ch]`: puro/leitura/escrita/io, análise transitiva, builtin `efeitos()`,
  gate `EFFECT_PURE` no kernel nativo.
- **F3a** — `backend/plan.[ch]`: `PIFARI_OPT=0|1|2`, `PIFARI_OPT_DEBUG=1` com log de plano;
  `bc_ensure`, `bc_ensure_function`, `jit_compile_function` e `try_native_map_flow` gateados;
  `tests/equiv.sh` + `make test-equiv` comparando byte a byte `OPT=0` vs `OPT=2`; ressalva
  documentada (§8.1) sobre efeitos dentro de `>|` serem não determinísticos e (§8.2) sobre
  nunca incluir o orquestrador de Reels em teste.

## Próximos 3 passos (F3b)

1. `ast_ir.[ch]`: IR para laço `for i in range(n)` puro, redução de soma e `A[i]` em laço
   (hoje só expressão unária e `(A,i)`).
2. Seleção adaptativa: escolher plano por função/laço com base em guardas de tipo + amostras
   medidas (EWMA), trocando só na entrada; deopt para WALK quando a guarda falhar.
3. `docs/OTIMIZACOES.md` com antes/depois por exemplo, sempre validado por `make test-equiv`.

## Observações

- **Acidente registrado:** uma execução de `sh tests/equiv.sh examples/*.pif` incluiu
  `orquestrador_reels_aistudio.pif`, que chamou a API do Gemini e consumiu quota. O script
  agora documenta explicitamente para nunca incluir esse arquivo; os alvos do Makefile já
  o excluem.
- `teste_cliente_servidor_mcp_pifari.pif` é flaky (timeout do mock MCP).
- JIT frio custa ~850 ms de gcc; quente roda 20 chamadas de 200k iterações em 14 ms.
  Isso é o argumento da F4 (cache persistente).
- 8 programas do `test-core` são não determinísticos por natureza (tempo, `thread_id`,
  print dentro de fluxo paralelo) e são pulados no `test-equiv`.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Efeito não altera execução: só autoriza/veta especialização.
- `OPT=0` desliga bytecode, JIT e kernel nativo: é a referência de comparação.
- Contrato de equivalência vale para programa determinístico; `>|` com efeito não é.

## Arquivos tocados

- `backend/types.[ch]`, `backend/typecheck.[ch]`, `backend/effects.[ch]`, `backend/plan.[ch]`
- `backend/runtime.c`, `frontend/parser.y`, `frontend/lexer.l`, `Makefile`
- `tests/equiv.sh`, `docs/SPEC_SEMANTICA.md`, `docs/STATUS_REAL.md`, `docs/EXECUCAO.md`
- `examples/tipos.pif`, `examples/efeitos.pif`
