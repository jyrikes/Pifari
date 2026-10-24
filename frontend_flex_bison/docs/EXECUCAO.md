# EXECUCAO — checkpoint do plano de concorrência

Atualizado ao fim de cada sessão. Ao retomar: ler **só este arquivo** + `git log --oneline -5`
e `git status --short`. Não reler `runtime.c` inteiro.

## Estado

- **Etapa atual:** E1 — núcleo de threads correto (E0 concluída)
- **Último teste verde:** baseline pré-existente (25/25 `.pif` + 4 binários C) — não re-rodado nesta sessão
- **Plano completo:** `docs/SPEC_CONCORRENCIA.md` (normativo) + plano aprovado na sessão do DSH

## Concluído

- **E0** — `docs/SPEC_CONCORRENCIA.md` com as regras normativas dos quatro operadores, escopo,
  falha, tarefas, memória compartilhada e dispositivos; 12 casos de conformidade (C1–C12).
- **E0** — `Definicao_Pifari_v3.tex` §231–238 corrigido: `|<` agora é barreira que agrega
  (com a regra de compatibilidade da forma de ligação) e o parágrafo passa a proibir
  propagação de escrita para o escopo do chamador.

## Próximos 3 passos (E1)

1. Pré-anotar slots de todas as `FunctionDef` numa passada single-thread no início do programa;
   remover a anotação preguiçosa de `call_user_function` (`runtime.c:886–905`, `992–996`).
2. Trocar `int errors` de `ParallelWork` por `char **errors` indexado, para falha por item
   (`runtime.c:1313–1355`, `1468–1499`).
3. `_Thread_local int in_worker` + `pool_run` serial quando chamado de dentro de worker
   (corrige o deadlock do `>|` aninhado, verificado por teste).

## Decisões tomadas (não reabrir)

- `|<` = barreira + agregação; forma de ligação preservada quando o RHS é `Name` não ligado.
- Modo de processo e dispositivo são decisões de runtime, sem sintaxe nova (`modo_processos`,
  `usar`), para não inflar a gramática.
- GPU local exige registrar o ICD da Intel (admin, uma vez). Sem isso, o plano segue pelo
  worker remoto e a GPU local fica indisponível — não é bloqueio do restante.
- `make test` puro não é usado em iteração (chama o orquestrador de Reels e gasta quota Gemini).

## Arquivos tocados até agora

- `docs/SPEC_CONCORRENCIA.md` (novo)
- `docs/EXECUCAO.md` (novo)
- `../Definicao_Pifari_v3.tex` (fora do repo, §231–238)
