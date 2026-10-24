# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2600 linhas).

## Estado

- **Etapa atual:** F2 — efeitos e pureza (F0 e F1 concluídas)
- **Último teste verde:** `make test-core` exit 0 e `make check-core` exit 0 (36 programas)
- **Plano:** F0–F5; semântica em `docs/SPEC_SEMANTICA.md` (§11 cobre o `--check`)

## Concluído

- **F0** — `SPEC_SEMANTICA.md`; JIT com atribuição definida (corrigiu `acc = acc + x` que
  devolvia 5 em vez de 105); overflow detectável nos 3 caminhos; `//` e `%` truncando para
  zero; diretivas não implementadas falham; `dict` fora do léxico.
- **F1a** — `backend/types.[ch]`; `=:` verificado em execução com promoção `int -> float`;
  tipo desconhecido falha; `Binding.kind` pronto para a F3.
- **F1b** — `param: IDENTIFIER COLON type_expr` e `function f(...) -> tipo`; checagem de
  argumento e de retorno nomeando parâmetro/função; JIT desligado para função anotada.
- **F1c** — `backend/typecheck.[ch]` + `pifari --check`: nomes globais não declarados,
  `=:` duplicado, `break`/`continue`, aridade de chamada, tipo desconhecido, retorno literal
  incompatível, diretivas e `class`. Dentro de função, nome desconhecido é **aviso**
  (ligação tardia de global) para não gerar falso positivo. `make check-core` cobre os 36
  programas do `test-core`.

## Próximos 3 passos (F2)

1. Tabela de efeitos dos builtins (puro / leitura / escrita / I/O) em `backend/effects.[ch]`.
2. Análise por função: pura quando só lê parâmetros/locais e chama apenas puros; gravar
   `fn->effects` no `FunctionDef`.
3. Usar a pureza: (a) permitir kernel nativo/paralelização automática só para puras;
   (b) recusar especialização quando houver `compartilhado`/`print`; (c) expor em
   `--check` como aviso quando um fluxo paralelo chama função impura.

## Observações

- `examples/teste_cliente_servidor_mcp_pifari.pif` é **flaky** (timeout do mock MCP);
  passou 3/3 na repetição. F5 deve dar retry/timeout maior.
- `--check` ainda reprova 4 exemplos por motivo conhecido: `condicao_implicita.pif` e
  `runtime_basico.pif` (sintaxe inválida de propósito), `diretivas.pif` (demonstra diretivas)
  e `fluxo.pif` (rascunho antigo). Nenhum está no `test-core`.
- Avisos de ligação tardia (`pesos` em `mlp_xor*.pif`) são esperados e não bloqueiam.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Em função, nome desconhecido é aviso, não erro (Pifari resolve global na chamada).
- `Param` é nó com `value` = nome e filho `Type`; `Function` com 3 filhos quando há retorno.

## Arquivos tocados

- `backend/types.[ch]`, `backend/typecheck.[ch]` (novos), `backend/runtime.c`
- `frontend/parser.y`, `frontend/lexer.l`, `Makefile` (`check-core`)
- `docs/SPEC_SEMANTICA.md`, `docs/STATUS_REAL.md`, `docs/EXECUCAO.md`
- `examples/tipos.pif` (novo), `examples/test.pif` (correção de `float("1.75")`)
