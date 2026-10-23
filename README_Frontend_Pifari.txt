FRONT-END DE REFERENCIA DA PIFARI

Arquivos:
- lexer.py: análise léxica, tokens com linha e coluna, indentação, comentários, literais e operadores.
- parser.py: parser recursivo descendente e construção da AST.
- test_frontend.py: testes automatizados.

Executar testes no PowerShell:
python -m unittest -v test_frontend.py

Subconjunto atual:
- literais inteiros, reais, booleanos, nulos e strings com aspas simples ou duplas;
- comentários de linha iniciados por @ e multilinha iniciados por @""" e encerrados por """@;
- operadores aritméticos, lógicos, comparativos e operadores de fluxo |<, <|, >| e |>;
- atribuição dinâmica =, declaração tipada =: e declaração de classe :=;
- blocos por indentação e blocos delimitados por chaves;
- if/elseif/else, for, return, break e continue;
- condição concisa com expressão booleana seguida de bloco e else;
- laço iterável compacto: numeros |> numero |> print(numero), convertido em nó ForEachPipe;
- import, #include <stdio.h> e Link to "biblioteca";
- atributos simples de classe com =: nome tipo e cabeçalhos de métodos.

Limitações conhecidas:
- É somente lexer e parser: não executa programas nem implementa runtime.
- O token #include é analisado, mas nenhum pré-processador nem linker é executado.
- O parser de membros de classe é inicial e não cobre ainda construtores, herança, visibilidade e assinaturas completas.
- A forma txt(conteúdo sem aspas) ainda é reconhecida como chamada normal, não como literal textual especial.
- Agentes, tarefas, await, spawn, mensagens e semântica de paralelismo ainda precisam de produções e validação próprias.
- O modelo de tipos e as regras semânticas precisam de uma etapa posterior de análise semântica.

O parser rejeita erros sintáticos com localização quando possível e constrói uma AST para etapas futuras. Nenhuma funcionalidade não implementada deve ser tratada como pronta.
