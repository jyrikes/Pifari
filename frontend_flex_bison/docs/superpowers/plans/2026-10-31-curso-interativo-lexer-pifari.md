# Curso Interativo do Lexer Pifari Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Criar um curso local separado, visual e prático que ensine a pessoa a entender e modificar o lexer Flex real da Pifari sem alterar o compilador.

**Architecture:** Site independente em `docs/curso-lexer/`, implementado em React + Vite com package isolado. As aulas e exercícios ficam em módulos separados da interface; exercícios locais usam exemplos limitados e explicam o comportamento, enquanto comandos opcionais demonstram a geração real do scanner pelo Flex.

**Tech Stack:** React, Vite, JavaScript, CSS e SVG inline; sem CDN ou dependências do app Pifari.

**Spec:** `docs/superpowers/specs/2026-10-30-curso-interativo-lexer-pifari-design.md`

## Global Constraints

- Não alterar `frontend/lexer.l`, `frontend/parser.y`, `frontend/ast.h`, backend, Makefile, binários ou exemplos do compilador.
- Criar todo o produto em `docs/curso-lexer/`; mudanças fora dali só se forem estritamente necessárias para uma instrução de abertura.
- Não usar APIs, CDN, fontes remotas ou downloads em tempo de execução; manter dependências React/Vite no package isolado do curso.
- Manter exercícios autocontidos no navegador e deixar claro quando uma simulação não compila regras Flex arbitrárias.
- Explicar separadamente NFA/DFA, política de correspondência do Flex, código C gerado e tabelas comprimidas; não confundir isso com tabela de tokens ou parser.
- Preservar todas as alterações Git preexistentes e não limpar artefatos nem executar comandos que sobrescrevam arquivos gerados.
- Texto em português claro, conciso e completo: cada conceito vem com exemplo e consequência prática; evitar tanto prolixidade quanto definições vazias.

## Review Focus

- Regras Flex sobrepostas: demonstrar maior correspondência e desempate por ordem, sem ensinar prioridade simples incorreta.
- Estados exclusivos, EOF, `yyless`, `yylineno` e `YY_USER_ACTION`: explicar o fluxo real e apontar os limites do exemplo didático.
- Tabelas comprimidas geradas: evitar apresentá-las como matriz universal simples ou como tabela de tokens.
- Edição em exercício: aceitar entradas vazias, espaços e caracteres especiais sem quebrar a página ou interpretar texto do aluno como HTML.
- Compatibilidade com arquivo local: instrução de abertura deve usar HTTP local seguro e explicar a dependência de Flex/WSL quando aplicável.

---

### Task 1: Base do site independente

**Files:**
- Create: `docs/curso-lexer/index.html`
- Create: `docs/curso-lexer/styles.css`
- Create: `docs/curso-lexer/app.js`
- Create: `docs/curso-lexer/README.md`

**Interfaces:**
- Produces: navegação de curso por etapas, estado de progresso local, layout responsivo, área de conteúdo acessível e carregamento sem dependências remotas.

- [ ] Criar esqueleto semântico, navegação por capítulos, indicador de progresso persistido em `localStorage`, modo responsivo e estados vazios/erro.
- [ ] Garantir que conteúdo digitado pelo aluno seja renderizado como texto, nunca como HTML executável.
- [ ] Documentar abertura local sem modificar configuração do compilador.

### Task 2: Trilha didática baseada no lexer real

**Files:**
- Create: `docs/curso-lexer/lessons.js`
- Reference: `frontend/lexer.l`
- Reference: `frontend/parser.y`
- Reference: `frontend/ast.h`
- Reference: `Makefile`

**Interfaces:**
- Consumes: modelo de capítulos da interface definido na Task 1.
- Produces: capítulos com `id`, `title`, `goal`, `explanation`, `source`, `walkthrough`, `example` e `exercise`.

- [ ] Escrever sequência: caracteres/lexemas/tokens; sintaxe de regras Flex; regex; estados `BOL`, `INITIAL`, `COMMENT`; indentação; palavras reservadas e identificadores; operadores; números e strings; posições/erros; EOF; integração com Bison.
- [ ] Para cada bloco relevante de `lexer.l`, explicar padrão, ação, retorno/token, estado e implicação de manutenção.
- [ ] Incluir exemplos concretos seguindo uma entrada Pifari desde caracteres até sequência de tokens.
- [ ] Explicar o contrato entre token retornado pelo lexer, `parser.y` e valores semânticos em `yylval`.

### Task 3: Autômatos, Flex e scanner gerado

**Files:**
- Create: `docs/curso-lexer/automata.js`
- Modify: `docs/curso-lexer/lessons.js`
- Reference: `Makefile`

**Interfaces:**
- Produces: diagramas SVG interativos com estados/nós nomeados, transições e descrição textual equivalente para acessibilidade.

- [ ] Mostrar regex simples → NFA → subconjuntos/DFA → scanner emitido; usar um exemplo pequeno com transições rastreáveis.
- [ ] Demonstrar cursor de entrada, estado atual, transição, aceitação e lexema reconhecido passo a passo.
- [ ] Explicar maior correspondência e regra mais antiga em empate com dois exemplos.
- [ ] Mostrar como Flex representa o scanner gerado em C e por que usa tabelas de transição comprimidas; distinguir `lex.yy.c`, `parser.tab.h` e tabelas internas.
- [ ] Incluir comandos seguros para regenerar e inspecionar o scanner, sem apagar nem sobrescrever fontes mantidas manualmente.

### Task 4: Exercícios progressivos de escrita

**Files:**
- Create: `docs/curso-lexer/exercises.js`
- Modify: `docs/curso-lexer/app.js`
- Modify: `docs/curso-lexer/lessons.js`

**Interfaces:**
- Produces: exercícios locais tipados: completar padrão/ação, classificar token, prever prioridade e interpretar o próximo passo do scanner.

- [ ] Criar atividades curtas em ordem crescente, exigindo que a pessoa escreva ou complete regras e preveja tokens.
- [ ] Dar feedback específico: esperado, resposta recebida e motivo; permitir tentar novamente sem apagar o progresso.
- [ ] Usar exemplos delimitados e validação local; declarar que o editor é didático e não executa Flex arbitrário.
- [ ] Adicionar mini desafio final: explicar e propor uma alteração localizada no lexer, mostrando perguntas de revisão antes de aplicar ao projeto.

### Task 5: Conferência final do material

**Files:**
- Review: todos os arquivos em `docs/curso-lexer/`
- Reference: fontes reais e Makefile

- [ ] Conferir cada trecho de código e número de linha com a versão atual de `frontend/lexer.l`.
- [ ] Conferir exemplos dos tokens contra nomes declarados em `frontend/parser.y` e `frontend/ast.h`.
- [ ] Abrir o site pelo servidor local e percorrer capítulos e exercícios em viewport desktop e estreita.
- [ ] Confirmar que não há chamadas de rede externa e que a aula permanece utilizável por teclado e leitor de tela.
- [ ] Relatar as verificações feitas sem alterar arquivos de produção do compilador.
