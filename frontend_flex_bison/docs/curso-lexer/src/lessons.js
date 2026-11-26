export const lessons = [
  { id: 'mapa', title: 'Comece pelo mapa', tag: 'Aquecimento', icon: '✳', time: '3 min', description: 'Veja a viagem completa: código escrito até tokens prontos para o Bison.', sections: [
    { title: 'Uma linha de montagem', text: 'O lexer lê caracteres e agrupa sequências com significado lexical. Ele não entende ainda que uma expressão está correta: entrega peças classificadas para o parser.', code: 'idade = 20 + anos', note: 'O lexer pode devolver IDENTIFIER, ASSIGN, INTEGER, PLUS, IDENTIFIER. A gramática decide depois se essa sequência forma uma atribuição válida.' },
    { title: 'Onde estamos no Pifari?', text: 'O arquivo frontend/lexer.l contém as regras Flex e ações C. O parser está em frontend/parser.y; os nomes dos tokens conectam os dois arquivos.', code: 'texto .pif → lexer.l → tokens → parser.y → AST', note: 'A análise léxica transforma a forma do texto. Ela não calcula 20 + anos nem executa o programa.' }
  ]},
  { id: 'regras', title: 'Como ler uma regra Flex', tag: 'Fundamentos', icon: '⌁', time: '6 min', description: 'Padrão à esquerda, ação à direita: reconhecer e decidir o próximo passo.', sections: [
    { title: 'A forma de uma regra', text: 'Na seção de regras do Flex, cada linha associa um padrão a uma ação. O padrão descreve caracteres; a ação é código C que pode retornar um token, ignorar o texto ou mudar de estado.', code: '<INITIAL>"if" { return IF; }', note: '<INITIAL> restringe o estado; "if" reconhece exatamente esses dois caracteres; return IF entrega o token ao Bison.' },
    { title: 'Regex que o arquivo usa', text: 'ID [A-Za-z_][A-Za-z0-9_]* aceita nomes que começam com letra ou sublinhado e continuam com letras, dígitos ou sublinhados. INT [0-9]+ pede ao menos um dígito.', code: 'ID    [A-Za-z_][A-Za-z0-9_]*\nINT   [0-9]+\nFLOATN ([0-9]+\\.[0-9]*|\\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+', note: 'Essas definições nomeadas aparecem antes do separador %% e podem ser reutilizadas por regras abaixo.' },
    { title: 'Ação também carrega valor', text: 'Para identificadores e literais, o lexer copia o lexema para yylval.str antes de devolver a categoria. Assim, o parser recebe tanto o tipo do token quanto o conteúdo associado.', code: '<INITIAL>{ID} { yylval.str=copy_text(yytext,yyleng); return IDENTIFIER; }', note: 'yytext aponta para o texto reconhecido; yyleng informa seu tamanho. A cópia permite que o valor continue válido depois que o scanner avançar.' }
  ]},
  { id: 'automatos', title: 'Regex vira autômato', tag: 'Visualizar', icon: '◎', time: '8 min', description: 'Monte uma máquina pequena e acompanhe a leitura símbolo por símbolo.', sections: [
    { title: 'Da expressão ao NFA', text: 'Uma expressão regular pode virar um autômato finito não determinístico (NFA): estados e transições representam os caminhos possíveis que casam com o padrão. Para o padrão a(b|c), o NFA aceita “ab” ou “ac”.', code: 'a(b|c)', note: 'A ramificação depois de “a” representa a escolha entre b e c. Em um NFA, mais de um caminho pode estar ativo ao mesmo tempo.' },
    { title: 'Do NFA ao DFA', text: 'O Flex converte os padrões para uma máquina determinística equivalente, usando construção por subconjuntos. Cada estado do DFA representa um conjunto de estados possíveis do NFA. Depois, o gerador organiza as transições para produzir o scanner.', code: 'NFA → construção por subconjuntos → DFA → código C + tabelas comprimidas', note: 'O scanner gerado costuma usar tabelas compactadas e código de controle; não espere encontrar uma matriz simples “estado × caractere” escrita de forma didática.' },
    { title: 'Percorra o autômato', text: 'Clique em cada etapa da palavra. Observe o estado atual, a entrada consumida e se a máquina chegou a um estado de aceitação.', code: 'Padrão de treino: a(b|c)    Exemplos aceitos: ab, ac', note: 'Este diagrama é uma visualização didática da regra de treino. Ele não substitui o scanner gerado para todas as regras do lexer Pifari.' }
  ]},
  { id: 'escolha', title: 'Quem ganha a disputa?', tag: 'Regra do Flex', icon: '⚖', time: '5 min', description: 'Entenda maior correspondência e desempate por ordem das regras.', sections: [
    { title: 'Maior trecho primeiro', text: 'Quando várias regras podem começar na posição atual, o Flex escolhe a que consome mais caracteres. Por isso, “elseif” como palavra inteira vence a regra genérica de identificador para esse lexema.', code: '"elseif" { return ELSEIF; }\n{ID}      { return IDENTIFIER; }', note: 'As regras de palavras reservadas aparecem antes do identificador. A ordem só desempata se duas regras reconhecerem um trecho do mesmo tamanho.' },
    { title: 'Empate: ganha a regra anterior', text: 'Se duas regras consomem o mesmo número de caracteres, vence a que aparece primeiro no arquivo. Essa ordem importa em operadores com prefixos, como “=” e “==”.', code: '"==" { return EQ; }\n"="  { return ASSIGN; }', note: 'Para a entrada “==”, a correspondência mais longa escolhe EQ. Para “=”, a regra ASSIGN reconhece o único caractere.' }
  ]},
  { id: 'estados', title: 'Estados e indentação', tag: 'Lexer Pifari', icon: '↳', time: '8 min', description: 'Explore BOL, INITIAL e COMMENT; veja como INDENT e DEDENT são sintetizados.', sections: [
    { title: 'Estados exclusivos', text: '%x BOL COMMENT declara estados exclusivos. BOL trata o começo de linha e a indentação; INITIAL lê tokens comuns; COMMENT consome o comentário multilinha.', code: '%x BOL COMMENT\n#define YY_USER_INIT BEGIN(BOL);', note: 'O lexer também conta brace_depth: dentro de chaves, a indentação de estilo Python não gera INDENT/DEDENT.' },
    { title: 'Indentação vira tokens', text: 'Em BOL, a ação mede espaços e tabulações. Se a coluna aumenta, empilha o nível e devolve INDENT. Se diminui, desempilha níveis e agenda DEDENTs.', code: 'if(col>indent_stack[indent_top]) {\n  indent_stack[++indent_top]=col;\n  BEGIN(INITIAL); return INDENT;\n}', note: 'DEDENT pode precisar ser devolvido em chamadas seguintes. pending_dedents guarda os tokens restantes sem consumir outro caractere.' },
    { title: 'Comentários como estado', text: 'Ao reconhecer @""", o scanner entra em COMMENT. Continua ignorando caracteres até encontrar """@, quando retorna a INITIAL. EOF nesse estado gera erro de comentário não terminado.', code: '<INITIAL>"@\\"\\"\\"" { BEGIN(COMMENT); }\n<COMMENT>"\\"\\"@" { BEGIN(INITIAL); }', note: 'Estados evitam que palavras e símbolos dentro do comentário sejam confundidos com código Pifari.' }
  ]},
  { id: 'codigo', title: 'Passeio pelo lexer.l', tag: 'Código real', icon: '{ }', time: '12 min', description: 'Clique numa linha para entender o papel dela, e conecte as regras aos conceitos.', sections: [
    { title: 'Cabeçalho e apoio em C', text: 'As primeiras linhas configuram o scanner, declaram estados e definem funções auxiliares. YY_USER_ACTION registra localização para cada lexema reconhecido.', code: '%option noyywrap yylineno nodefault noinput nounput\n%x BOL COMMENT\n#define YY_USER_ACTION do { ... } while(0);', note: 'yycolumn é mantida pelo próprio lexer; yylineno é atualizada pelo Flex. O detalhe de atualização depende da regra que consome a quebra de linha.' },
    { title: 'Palavras reservadas antes de ID', text: 'O bloco retorna tokens para palavras-chave e operadores. Depois, ID reconhece nomes que não casaram com palavra reservada. A ordem torna a intenção do arquivo fácil de revisar.', code: '<INITIAL>"if" { return IF; }\n...\n<INITIAL>{ID} { yylval.str=copy_text(yytext,yyleng); return IDENTIFIER; }', note: 'Palavra reservada não vira IDENTIFIER: a regra específica consome o mesmo texto e, no empate, aparece primeiro.' },
    { title: 'EOF fecha blocos abertos', text: 'No fim do arquivo, o lexer emite DEDENTs pendentes para fechar níveis de indentação antes de retornar 0, que sinaliza fim da entrada ao parser.', code: '<INITIAL,BOL><<EOF>> {\n  while(indent_top>0){indent_top--;pending_dedents++;}\n  if(pending_dedents){pending_dedents--;return DEDENT;}\n  return 0;\n}', note: 'EOF em COMMENT é tratado por outra regra e vira erro. São caminhos diferentes para fins de entrada diferentes.' }
  ]},
  { id: 'depois', title: 'Depois do arquivo .l', tag: 'Flex → C', icon: '⚙', time: '5 min', description: 'Gere o scanner e descubra onde procurar cada peça.', sections: [
    { title: 'Geração', text: 'O Makefile chama Flex para transformar frontend/lexer.l em lex.yy.c. O Bison gera parser.tab.c e parser.tab.h; o header fornece os nomes de tokens usados pelo lexer.', code: 'flex -o lex.yy.c frontend/lexer.l\nbison -d -o parser.tab.c frontend/parser.y', note: 'No projeto, use make para seguir as dependências oficiais. Evite editar lex.yy.c: ele é gerado e suas alterações serão substituídas.' },
    { title: 'O que procurar no resultado', text: 'O C gerado contém rotinas do scanner, estados e estruturas de transição. Flex pode comprimir tabelas para reduzir tamanho e acelerar acesso. Os nomes internos e o formato mudam com opções e versão.', code: 'lexer.l (fonte legível) → lex.yy.c (scanner gerado)\nparser.y → parser.tab.c + parser.tab.h', note: 'O parser.tab.h é a interface dos tokens. As tabelas do scanner descrevem transições do autômato; não são uma lista de tokens.' },
    { title: 'Ciclo seguro de mudança', text: 'Escolha uma regra pequena, escreva um exemplo de entrada, preveja lexema/token/valor, altere o fonte .l, reconstrua pelo Makefile e rode os testes do frontend. Compare o resultado real com sua previsão.', code: 'fonte .l → make → scanner gerado → exemplo/teste → inspeção do token', note: 'Mude uma ideia por vez. Se o parser reclamar, confira primeiro se o lexer devolveu o token que a gramática espera.' }
  ]}
];

export const lexerSource = `1  %option noyywrap yylineno nodefault noinput nounput
2  %x BOL COMMENT
3  %{
4  #include "frontend/ast.h"
5  #include "parser.tab.h"
6  #include <stdio.h>
7  #include <stdlib.h>
8  #include <string.h>
9  int yycolumn=1;
10 static int indent_stack[256]={0}, indent_top=0, pending_dedents=0, brace_depth=0;
11 static int rawlex(void);
12 #define YY_DECL static int rawlex(void)
13 #define YY_USER_INIT BEGIN(BOL);
14 #define YY_USER_ACTION do { yylloc.first_line=yylineno; yylloc.last_line=yylineno; yylloc.first_column=yycolumn; yylloc.last_column=yycolumn+yyleng-1; yycolumn+=yyleng; } while(0);
15 static char *copy_text(const char*s,size_t n){char*p=malloc(n+1);if(!p){perror("malloc");exit(2);}memcpy(p,s,n);p[n]=0;return p;}
16 static int indentation(const char*s,int n){int col=0,sp=0,tb=0;for(int i=0;i<n;i++){if(s[i]==' '){col++;sp++;}else if(s[i]=='\\t'){col=(col/8+1)*8;tb++;}}if(sp&&tb){fprintf(stderr,"Pifari:%d: indentação mistura espaços e tabulações\\n",yylineno);exit(1);}return col;}
17 int yylex(void){if(pending_dedents>0){pending_dedents--;return DEDENT;}return rawlex();}
18 %}
19 ID [A-Za-z_][A-Za-z0-9_]*
20 INT [0-9]+
21 FLOATN ([0-9]+\\.[0-9]*|\\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+
22 STR (\\"([^\\\\\\"\\n]|\\\\.)*\\"|'([^\\\\'\\n]|\\\\.)*')
23 %%
24 <BOL>[ \\t]+ { ... regras de INDENT/DEDENT ... }
25 <BOL>\\n { yycolumn=1;return NEWLINE; }
26 <INITIAL>\\n { yycolumn=1;BEGIN(BOL);return NEWLINE; }
27 <INITIAL>\"@\\\"\\\"\\\"\" { BEGIN(COMMENT); }
28 <COMMENT>\"\\\"\\\"@\" { BEGIN(INITIAL); }
29 <INITIAL>\"#include\" {return INCLUDE;}
30 <INITIAL>\"if\" {return IF;}
31 <INITIAL>\"else\" {return ELSE;}
32 <INITIAL>\":=\" {return CLASS_DEF;}
33 <INITIAL>\"=: \" {return TYPED_DECL;}
34 <INITIAL>\"==\" {return EQ;}
35 <INITIAL>\"=\" {return ASSIGN;}
36 <INITIAL>{FLOATN} {yylval.str=copy_text(yytext,yyleng);return FLOAT;}
37 <INITIAL>{INT} {yylval.str=copy_text(yytext,yyleng);return INTEGER;}
38 <INITIAL>{STR} {yylval.str=copy_text(yytext+1,yyleng-2);return STRING;}
39 <INITIAL>{ID} {yylval.str=copy_text(yytext,yyleng);return IDENTIFIER;}
40 <INITIAL>[ \\t\\r]+ {}
41 <INITIAL>. {fprintf(stderr,"Pifari:%d:%d: caractere inesperado '%s'\\n",yylineno,yycolumn-yyleng,yytext);exit(1);}
42 <INITIAL,BOL><<EOF>> { ... emite DEDENTs restantes e retorna 0 ... }
43 <COMMENT><<EOF>> {fprintf(stderr,"Pifari:%d: comentário multilinha não terminado\\n",yylineno);return 1;}
44 %%`;
