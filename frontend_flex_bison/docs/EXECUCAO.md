# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2600 linhas).

## Estado

- **Etapa atual:** F3 — planos de execução e IR estendido (F0, F1 e F2 concluídas)
- **Último teste verde:** `make test-core` exit 0 (37 programas) e `make check-core` exit 0
- **Plano:** F0–F5; semântica em `docs/SPEC_SEMANTICA.md` (§11 `--check`, §12 efeitos)

## Concluído

- **F0** — semântica declarada; JIT com atribuição definida (corrigiu resultado errado em
  `acc = acc + x`); overflow detectável nos 3 caminhos; `//` e `%` truncando; diretivas
  não implementadas falham; `dict` fora do léxico.
- **F1a/b/c** — `types.[ch]` com promoção `int→float`; `=:` verificado; parâmetros e retorno
  tipados (`function f(a: int) -> float`); `typecheck.[ch]` + `pifari --check` (nomes,
  duplicata, `break`, aridade, tipo desconhecido, diretivas) e `make check-core`.
- **F2** — `backend/effects.[ch]`: builtins classificados em puro/leitura/escrita/io;
  `FunctionDef.effects` calculado no registro com análise transitiva (chamada a função não
  analisada = io); builtin `efeitos("nome")`; `try_native_map_flow` agora exige
  `effects == EFFECT_PURE`. `examples/efeitos.pif` cobre os quatro rótulos.

## Próximos 3 passos (F3)

1. `backend/plan.[ch]`: enum `PLAN_WALK|BYTECODE|JIT|NATIVE|GPU`, guardas por tipo/pureza,
   escolha na entrada da função/laço e deopt para WALK quando a guarda falha.
2. IR estendido em `ast_ir.[ch]`: laço `for i in range(n)` puro, redução (soma) e `A[i]`
   usado em laço — hoje o IR cobre expressão unária e `(A,i)`.
3. `PIFARI_OPT=0|1|2` e `PIFARI_OPT_DEBUG=1`: comparar saídas em 0 e 2 sobre `test-core`
   (contrato do §8) e publicar tempos em `docs/OTIMIZACOES.md`.

## Observações

- `examples/teste_cliente_servidor_mcp_pifari.pif` é **flaky** (timeout do mock MCP);
  F5 deve dar retry/timeout maior.
- `--check` reprova 4 exemplos por motivo conhecido (fora do `test-core`): sintaxe inválida
  proposital (`condicao_implicita`, `runtime_basico`), `diretivas` e rascunho `fluxo`.
- Avisos de ligação tardia (`pesos` em `mlp_xor*.pif`) são esperados.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Efeito não altera execução: só autoriza/veta especialização.
- Função anotada não vai para o JIT (o C gerado não carrega tipos).
- `Param` = nó com `value`=nome + filho `Type`; `Function` tem 3 filhos quando há retorno.

## Arquivos tocados

- `backend/types.[ch]`, `backend/typecheck.[ch]`, `backend/effects.[ch]` (novos)
- `backend/runtime.c`, `frontend/parser.y`, `frontend/lexer.l`, `Makefile`
- `docs/SPEC_SEMANTICA.md`, `docs/STATUS_REAL.md`, `docs/EXECUCAO.md`
- `examples/tipos.pif`, `examples/efeitos.pif` (novos), `examples/test.pif`
