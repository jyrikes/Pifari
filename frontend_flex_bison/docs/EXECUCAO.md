# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2750 linhas).

## Estado

- **Etapa atual:** F3 fechada — kernel de redução para laço puro (F0–F5 concluídas)
- **Testes verdes:** `test-core`, `check-core`, `test-equiv` (EQUIV_OK, 27 iguais / 9 pulados),
  `test-golden` (GOLDEN_OK, 24 iguais / 10 pulados) e `test-gpu`
- **Semântica:** `docs/SPEC_SEMANTICA.md` (§8 contrato e §8.3 redução, §11 check, §12 efeitos, §13 cache)
- **Números:** `docs/OTIMIZACOES.md` (§8 redução nativa)

## Concluído

- **F0** — semântica declarada; JIT com atribuição definida; overflow detectável; `//` e `%`
  truncando; diretivas falham; `dict` fora do léxico.
- **F1a/b/c** — tipos (`types.[ch]`), `=:` verificado, parâmetros/retorno tipados,
  `typecheck.[ch]`, `pifari --check`, `make check-core`.
- **F2** — `effects.[ch]`: puro/leitura/escrita/io transitivo, `efeitos()`, gate de kernel.
- **F3a** — `plan.[ch]`: `PIFARI_OPT`, `PIFARI_OPT_DEBUG`, gates, `tests/equiv.sh`.
- **F3b** — tiering do JIT (16 chamadas ou 50 ms; `PIFARI_JIT_THRESHOLD`,
  `PIFARI_JIT_BUDGET_MS`); `OPT=2` compila no registro.
- **F3c** — JIT com aritmética checada: helpers com `__builtin_*_overflow`, divisão por zero e
  `LLONG_MIN/-1`; paridade de erro com `OPT=0` verificada. `tests/equiv-skip.txt` lista os
  programas com efeito dentro de `>|` (ordem de print depende do escalonamento).
- **F3d** — `reduce.[ch]`: kernel nativo de **redução** para o corpo
  `acc = <inteiro>; for i in range(param); acc = acc <op> expr(i); return acc`.
  C gerado em inteiro de 64 bits com `__builtin_*_overflow`, cache por hash em
  `~/.cache/pifari/kernels/`, plano `NATIVE` tentado antes do JIT (tier-up em `OPT=1`,
  registro em `OPT=2`, desligado em `OPT=0`). Overflow e limite de `range` com paridade de
  erro. Laço de 1 000 000: 132–151 ms (`OPT=0`) → 14–18 ms (`OPT=2`).
- **F4** — `cache.[ch]` + `warmup.txt`: cache persistente em `~/.cache/pifari`, seed por
  função, gravação atômica, `PIFARI_WARMUP=off`. Frio 2 572 ms → morno 33 ms.
- **F5** — `tests/golden.sh` + `make test-golden` (24 programas congelados, 10 pulados por
  não determinismo), `docs/OTIMIZACOES.md`, README e STATUS_REAL atualizados.

## Pendente (fora do escopo do plano)

- Laço sobre `f64_array` (em vez de `range`) como redução nativa: hoje o map indexado
  (`(A, i)`) cobre o caso de mapeamento, mas não há acumulação sobre array.
- Buffer residente na GPU com handles (1 transporte para k operações).
- Vetorização SIMD manual e seleção adaptativa por amostra.

## Observações

- `/tmp` do WSL é limpo entre sessões: o cache vive em `~/.cache/pifari`.
- **Nunca** rodar `examples/orquestrador_reels_aistudio.pif` (Gemini, consome quota).
- `teste_cliente_servidor_mcp_pifari.pif` é flaky (mock MCP).
- `runtime_map_fused.pif` às vezes falha em criar o kernel nativo e cai para o caminho
  genérico; por isso não foi congelado no golden.
- A redução nativa exige um parâmetro **sem anotação** e **sem anotação de retorno**:
  anotação promoveria int→float no interpretador e o kernel devolveria int.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Efeito não altera execução: só autoriza/veta especialização.
- `OPT=0` = referência (sem bytecode, JIT ou kernel).
- Contrato de equivalência vale para programa determinístico.
- Manifesto de aquecimento nunca muda resultado, só quando o JIT entra.
- Redução nativa só para aritmética inteira; float continua no interpretador.

## Arquivos tocados

- `backend/`: types, typecheck, effects, plan, cache, reduce (novos) + runtime, native_kernel
- `frontend/parser.y`, `frontend/lexer.l`, `Makefile`
- `tests/`: equiv.sh, golden.sh, golden/ (24 pares .txt/.exit)
- `docs/`: SPEC_SEMANTICA, OTIMIZACOES (novos), STATUS_REAL, EXECUCAO
- `examples/`: tipos.pif, efeitos.pif, reducao_nativa.pif, reducao_erro.pif (novos)
