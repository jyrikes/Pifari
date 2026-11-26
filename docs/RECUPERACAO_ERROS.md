# Recuperação de erros no Pifari

Hotfix de 26/11/2026, baseado na release acadêmica `v1.0.0-rc.1`.

O objetivo é produzir vários diagnósticos úteis numa análise, sem executar um
programa parcialmente corrigido. A recuperação altera apenas a estrutura
temporária usada pelo analisador; não reescreve o arquivo do usuário.

## Técnicas e alcance

| Técnica | Implementação | Comportamento |
|---|---|---|
| Recuperação léxica por descarte | `frontend/lexer.l` | Consome um caractere inesperado, registra o erro e continua. Texto não terminado é descartado até o fim da linha. |
| Modo pânico por linha | `lines: lines error NEWLINE` | Descarta tokens da instrução inválida até a quebra de linha, preservando instruções já analisadas. |
| Sincronização por bloco | Alternativas de `suite` com `error DEDENT` e `error RBRACE` | Permite encerrar um bloco danificado e retomar a análise no bloco externo. |
| Recuperação local, em nível de frase | Alternativas de `primary` com `error` | Usa `)` e `]` como pontos de retomada em agrupamentos, chamadas e listas; produz um nó `Error`. A quebra de linha é uma saída alternativa quando falta o fechamento. |
| Produção de erro explícita | `arguments_opt: arguments COMMA` | Reconhece a vírgula final sem expressão em chamadas/listas e emite um diagnóstico específico. Continua com os argumentos já reconhecidos, mas rejeita o programa. |
| Continuação semântica | `backend/typecheck.c`, modo `--check` | Comportamento preexistente, agora também coberto pela suíte do hotfix: relata nomes não declarados em instruções distintas. |

Não há correção global por custo mínimo nem busca de todas as possíveis edições.
Também não há inserção geral de tokens, inferência da intenção do autor ou
execução de uma AST recuperada. Esses algoritmos excedem o escopo do hotfix.

## Fluxo de análise e contrato de segurança

1. Flex reconhece tokens, posições e indentação. Erros léxicos alimentam o
   contador compartilhado de `frontend/diagnostics.h`.
2. Bison registra erros sintáticos e tenta uma das regras de sincronização.
   `yyerrok` encerra a recuperação depois do delimitador consumido, permitindo
   reportar também o erro seguinte. O lookahead não é apagado com `yyclearin`.
3. `%define parse.lac full` melhora a consistência dos tokens esperados nas
   mensagens detalhadas. Os sete conflitos shift/reduce preexistentes permanecem
   declarados por `%expect 7`; LAC não elimina esses conflitos.
4. Uma análise recuperada pode retornar zero em `yyparse()`. Por isso, `main`
   exige também `pifari_frontend_errors == 0` antes de verificar tipos ou executar.
5. O processo libera a AST, fecha o arquivo e libera o scanner. A ordem importa:
   `yylex_destroy()` reinicializa variáveis do Flex, inclusive `yyin`.

O limite compartilhado é de **50 diagnósticos léxicos/sintáticos** por execução.
Ao atingi-lo, o scanner encerra a leitura e é emitida uma mensagem de limite.
Esse teto não foi estendido ao verificador semântico preexistente.

| Código de saída | Significado |
|---|---|
| `0` | Análise/execução concluída sem erro no modo selecionado. |
| `1` | Falha léxica/sintática, inclusive após recuperação; ou falha de execução. |
| `2` | Uso/abertura de arquivo inválidos, erros semânticos em `--check` ou determinados erros de recursos. |

Erros sintáticos impedem a análise semântica daquela AST; não se mistura uma
árvore incompleta com diagnósticos de nomes/tipos que poderiam ser enganosos.
O hotfix não altera a política dos erros de execução, como divisão por zero.

## Posições, indentação e fim de arquivo

Os diagnósticos seguem `Pifari:linha:coluna: erro fase: mensagem`. Linhas e colunas
começam em 1; colunas contam bytes do arquivo, não largura visual de Unicode.
Tabs avançam para múltiplos de oito na comparação da indentação, mas contam um
byte na posição do token.

O estado `BOL` devolve o primeiro caractere ao scanner com `yyless(0)`; a coluna
agora é restaurada antes dessa releitura. Uma quebra LF/CRLF é localizada na
linha que termina, e só depois incrementa o contador de linha. O EOF recebe uma
posição explícita, inclusive quando não há quebra final.

Linhas vazias e comentários não alteram a pilha de indentação. Mistura de tabs e
espaços gera diagnóstico, mas a largura calculada permite continuar. Um dedent
inconsistente é aproximado ao nível inferior existente apenas para análise.
O excesso de profundidade da pilha é fatal para o scanner e não escreve fora do
array. Comentário multilinha não fechado é reportado na posição de abertura;
depois o estado do scanner muda, evitando repetir o mesmo erro no EOF.

## Propriedade de memória

`%destructor { free($$); } <str>` libera textos de tokens descartados;
`%destructor { node_free($$); } <node>` libera subárvores abandonadas por Bison.
Nas reduções normais, as ações continuam transferindo a propriedade para o pai.
O símbolo inicial transfere a AST para `pifari_root` e fica com valor nulo: assim,
seu destrutor não libera a árvore que pertence a `main`.

A produção de atribuição inválida mantém a liberação explícita antes de
`YYERROR`, pois Bison não destrói automaticamente os símbolos do lado direito
da regra que invoca esse macro. Nós `Error` nunca chegam ao runtime.

## Exemplo reproduzível

Arquivo `examples/erros_recuperacao.pif`:

```pifari
print("esta mensagem não será executada")
x =
print(1 + )
valores = [1,]
~
```

```sh
./pifari examples/erros_recuperacao.pif
./pifari --check examples/erros_recuperacao.pif
```

Ambos terminam com código `1`, sem saída em stdout, e reportam erros nas linhas
2, 3, 4 e 5. O texto genérico de Bison pode variar conforme sua versão.

## Verificação

```sh
make
make test-errors
python3 tests/test_academic.py
make test-fuzz
python3 tests/sanitize_frontend.py
make test-core check-core test-conc test-equiv test-golden
```

`test-errors` também faz parte de `test-core`. A suíte inclui múltiplos erros,
linhas posteriores, delimitadores, blocos aninhados, literais malformados,
indentação, CRLF, EOF, limite de diagnósticos e bloqueio de execução parcial.
Os casos válidos verificam saída, comentários e preservação dos blocos.

`sanitize_frontend.py` compila um executável temporário com AddressSanitizer,
UndefinedBehaviorSanitizer e detecção de vazamentos. Instrumenta lexer, parser,
verificador semântico e tipos; substitui o runtime por uma função que aborta se
for chamada. Portanto, esse teste verifica a memória do frontend e a barreira
de execução, não certifica a memória do backend completo. Requer GCC com os
sanitizers disponíveis em Linux/WSL. Os arquivos temporários são removidos.

## Limites

Recuperação é uma heurística. Erros aninhados ou delimitadores incompatíveis
podem descartar mais de uma construção e provocar diagnósticos em cascata.
Não se garante um diagnóstico por erro humano, nem a descoberta de todos os
erros de um arquivo. Listas/chamadas multilinha não foram introduzidas neste
hotfix. Virgula final continua inválida: a produção extra apenas explica o erro.

## Referências

- [GNU Bison — Error Recovery](https://www.gnu.org/software/bison/manual/html_node/Error-Recovery.html)
- [GNU Bison — Freeing Discarded Symbols](https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html)
- [GNU Bison — LAC](https://www.gnu.org/software/bison/manual/html_node/LAC.html)

Uso de IA nesta alteração: ChatGPT auxiliou na implementação, nos testes e na
documentação. A equipe deve revisar os resultados e complementar a declaração
de ferramentas do projeto.
