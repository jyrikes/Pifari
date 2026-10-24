# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (~2500 linhas).

## Estado

- **Etapa atual:** F1c — `typecheck` estático + `--check` (F0, F1a e F1b concluídas)
- **Último teste verde:** `make test-core` exit 0 (31 `.pif` + 5 binários C) após F1b
- **Plano:** F0–F5; semântica em `docs/SPEC_SEMANTICA.md`

## Concluído

- **F0** — `SPEC_SEMANTICA.md`; JIT com atribuição definida (corrigiu `acc = acc + x` que
  devolvia 5 em vez de 105); overflow detectável nos 3 caminhos; `//` e `%` truncando para
  zero; diretivas não implementadas falham; `dict` fora do léxico.
- **F1a** — `backend/types.[ch]`: `PifariType`, `pifari_type_from_name`, `pifari_type_assignable`
  (promoção `int -> float`); `Binding.kind`; `=:` verificado em execução; tipo desconhecido
  falha (`tipo desconhecido: 'dict'`); `examples/tipos.pif`.
- **F1b** — gramática de parâmetro tipado (`a: int`) e retorno (`-> tipo`) usando `ARROW`;
  checagem de argumento e de retorno com promoção e erro nomeando parâmetro/função.
  JIT desligado para funções anotadas (o C gerado não carrega tipos ainda).

## Próximos 3 passos (F1c)

1. `backend/typecheck.[ch]`: passada sobre a AST antes de executar — nomes não declarados,
   declaração duplicada, `break`/`continue` fora de laço, `await` de não-tarefa, aridade
   quando a função tem parâmetros tipados.
2. `PIFARI_CHECK=1` / `--check`: só valida, imprime diagnósticos e sai com 2 em erro.
   Ligar em `make check-core` sobre os exemplos (todos devem passar).
3. `PIFARI_STRICT=1`: condição exige `bool` (hoje aceita truthiness); documentar no §5 do
   `SPEC_SEMANTICA.md` como opção.

## Observações

- `examples/teste_cliente_servidor_mcp_pifari.pif` é **flaky** (timeout do mock MCP em
  `mcp_discover_tools`); passou 3/3 na repetição. Não é regressão. F5 deve dar retry ou
  timeout maior nesse teste.
- JIT só compila função sem anotação, com 1 parâmetro, sem leitura de nome não atribuído.
- Binários `.o`, `pifari`, `parser.tab.c`, `lex.yy.c` continuam como artefatos não commitados.

## Decisões (não reabrir)

- Overflow = erro; `->` = tipo de retorno; lista = valor; classes/closures/módulos/GC fora.
- Tipos de função só com `function`; a forma `nome : (params) { }` continua sem anotação.
- `Param` é um nó com `value` = nome e filho `Type`, para não quebrar código que lê
  `params->children[i]->value`.

## Arquivos tocados

- `backend/types.[ch]` (novo), `backend/runtime.c`, `frontend/parser.y`, `frontend/lexer.l`
- `docs/SPEC_SEMANTICA.md`, `docs/STATUS_REAL.md`, `docs/EXECUCAO.md`
- `examples/tipos.pif` (novo)
