# Curso Interativo do Lexer Pifari — Design

## Intenção

Criar um site local separado do guia geral para ensinar, com calma e prática, como o analisador léxico real da Pifari transforma caracteres em tokens. O objetivo é que a pessoa consiga explicar e alterar `frontend/lexer.l` com segurança.

## Objetivos

- Explicar conceitos antes de apresentar cada trecho real de `frontend/lexer.l`.
- Acompanhar o código em pequenos blocos, incluindo estados do Flex, expressões regulares, ações C, posições, indentação, comentários, erros e EOF.
- Ensinar o caminho de uma regra do Flex até NFA, DFA, escolha de correspondência mais longa/desempate por ordem, e scanner gerado com tabelas compactadas.
- Distinguir as tabelas internas do Flex das tabelas de tokens e do parser Bison.
- Incluir exercícios em que a pessoa digita padrões e ações, prevê tokens ou completa trechos; dar feedback local e mostrar o raciocínio.
- Apresentar a compilação do `.l` em C (`lex.yy.c`), a inspeção do scanner gerado e como verificar o comportamento com exemplos e testes.
- Funcionar localmente sem API, dependências externas ou conexão à internet.

## Fora de escopo

- Alterar a implementação do lexer ou o comportamento do compilador.
- Transformar o curso em editor/compilador Flex completo no navegador.
- Cobrir parser, AST e runtime além do necessário para mostrar o contrato dos tokens.

## Abordagens consideradas

1. Acrescentar a aula ao guia didático geral: simples, mas mistura o curso prático com a visão ampla do projeto; não atende ao pedido de experiência separada.
2. Site independente para o curso do lexer: mantém o curso focado, permite ritmo e exercícios próprios e pode abrir diretamente no navegador. Escolhido por corresponder ao pedido. A implementação escolhida depois é React + Vite em um pacote próprio dentro da pasta do curso.
3. Aplicativo com backend e execução de Flex no servidor: poderia compilar respostas livres, mas adiciona dependências e operação desnecessárias para ensinar o lexer atual.

## Experiência proposta

Uma página de curso com trilha em passos curtos: (1) caractere, lexema e token; (2) regex e regras; (3) autômatos; (4) seleção de regra no Flex; (5) ações C e estados deste lexer; (6) geração/estrutura do scanner; (7) depuração e mudanças seguras. Cada etapa combina explicação simples, animação/diagrama SVG ou tabelas, trecho real com linhas e notas vinculadas, e uma atividade de escrita ou previsão. O progresso fica no navegador.

Os exercícios rodam em JavaScript local com casos didáticos definidos pelo curso. Eles não afirmam compilar código Flex arbitrário. Para trechos reais, a pessoa recebe comandos para gerar `lex.yy.c` e inspecionar símbolos/tabelas, além de um exemplo resolvido que explica o output gerado.

## Restrições e decisões

- Site independente dentro de `docs/curso-lexer/`, com React e Vite em package isolado; sem CDN, serviços externos ou alterações às dependências do projeto C.
- Fonte de verdade: `frontend/lexer.l`, `frontend/parser.y`, `frontend/ast.h`, `Makefile`, exemplos e testes existentes. Trechos apresentados devem ser conferidos contra os arquivos do projeto.
- Abrir por servidor HTTP local com Vite, seguindo a abordagem do guia existente para evitar limitações de `file://`; não expor serviço à rede.
- O material explica que o Flex constrói autômatos para reconhecer padrões e normalmente emite código C baseado em tabelas comprimidas; não promete que o `lex.yy.c` exponha uma tabela simples e legível de estados/transições.
- O projeto já tem muitas alterações locais. As mudanças do curso ficam restritas ao novo diretório e, se necessário, a uma instrução mínima de abertura sem reformatar ou limpar outros arquivos.

## Critério de sucesso

Uma pessoa iniciante consegue seguir uma entrada caractere a caractere, prever o token emitido, explicar ao menos uma regra completa do lexer, reconhecer por que Flex escolhe uma regra, e fazer alterações/exercícios sem confundir lexema, token, parser e tabelas do scanner.
