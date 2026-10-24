# EXECUCAO — checkpoint do plano de tipos e otimização

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro (2421 linhas).

## Estado

- **Etapa atual:** F1 — tipos (F0 concluída)
- **Último teste verde:** `FULL_SUITE_OK` (30 `.pif` + 5 binários C) após as mudanças de F0
- **Plano completo:** F0–F5 aprovado; semântica em `docs/SPEC_SEMANTICA.md`

## Concluído — F0 (semântica e coerência)

- `docs/SPEC_SEMANTICA.md`: ordem de avaliação, int64 com overflow = erro, float IEEE-754
  sem reassociação, listas por valor, taxonomia de erros, contrato do otimizador (§8).
- **JIT corrigido**: o C gerado inicializava slots em 0 e mudava o resultado
  (`acc = acc + x` dava 5 em vez de 105). Agora `jit_definitely_assigned` só compila funções
  com atribuição definida. Função puramente local continua 400× mais rápida (36 µs vs 14,6 ms).
- **Overflow detectável** nos três caminhos (interpretador, bytecode, VM de função).
- **`//` e `%` consistentes** (truncamento para zero): `-7 // 2 == -3`, `-7 % 2 == -1`.
- **Diretivas** `import`/`#include`/`Link to` falham com erro semântico; `dict` saiu do léxico.
- Matriz de construtores atualizada em `docs/STATUS_REAL.md`.

## Próximos 3 passos (F1)

1. `backend/types.h`: `PifariType` + `PifariTypeDesc`; `Node.type_ann`, `Binding.kind`.
2. Gramática: parâmetro tipado (`a: int`), retorno (`function f(...) -> tipo`, usando o
   `ARROW` que já é lexado), tipo de coleção (`list(int)`); manter `%expect 7`.
3. `backend/typecheck.[ch]`: passada estática (nomes, duplicatas, aridade/tipos de chamada,
   `break`/`continue`, `await`, retorno) + verificação dinâmica em atribuição tipada;
   `PIFARI_CHECK=1`/`--check` sai com 2 em erro.

## Decisões (não reabrir)

- Overflow = erro (spec §180). F3 pode remover a checagem em laço com prova de intervalo.
- `->` é tipo de retorno (token já existia reservado).
- Lista tem semântica de valor; não mexer (30 exemplos dependem).
- Classes, closures, módulos e GC fora de escopo.
- O JIT só compila com atribuição definida até receber os slots iniciais por parâmetro.

## Arquivos tocados

- `backend/runtime.c` (F0), `frontend/lexer.l`
- `docs/SPEC_SEMANTICA.md` (novo), `docs/STATUS_REAL.md`, `docs/EXECUCAO.md`
- `backend/devices.[ch]`, `backend/gpu_client.[ch]`, `tools/pifari_worker.py` (ciclo anterior)
- `docs/SPEC_CONCORRENCIA.md`, `docs/BENCHMARKS_GPU.md` (ciclo anterior)
