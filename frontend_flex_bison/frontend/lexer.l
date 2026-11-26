%option noyywrap yylineno nodefault noinput nounput
%x BOL COMMENT
%{
#include "frontend/ast.h"
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int yycolumn = 1;
static int indent_stack[256] = {0}, indent_top = 0, pending_dedents = 0, brace_depth = 0;
static int rawlex(void);
#define YY_DECL static int rawlex(void)
#define YY_USER_INIT BEGIN(BOL);
#define YY_USER_ACTION                                                                             \
    do {                                                                                           \
        yylloc.first_line = yylineno;                                                              \
        yylloc.last_line = yylineno;                                                               \
        yylloc.first_column = yycolumn;                                                            \
        yylloc.last_column = yycolumn + yyleng - 1;                                                \
        yycolumn += yyleng;                                                                        \
    } while (0);
static char *copy_text(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (!p) {
        perror("malloc");
        exit(2);
    }
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}
static int indentation(const char *s, int n) {
    int col = 0, sp = 0, tb = 0;
    for (int i = 0; i < n; i++) {
        if (s[i] == ' ') {
            col++;
            sp++;
        } else if (s[i] == '\t') {
            col = (col / 8 + 1) * 8;
            tb++;
        }
    }
    if (sp && tb) {
        fprintf(stderr, "Pifari:%d: indentação mistura espaços e tabulações\n", yylineno);
        exit(1);
    }
    return col;
}
int yylex(void) {
    if (pending_dedents > 0) {
        pending_dedents--;
        return DEDENT;
    }
    return rawlex();
}
%}
ID [A-Za-z_][A-Za-z0-9_]*
INT [0-9]+
FLOATN ([0-9]+\.[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+
STR (\"([^\\\"\n]|\\.)*\"|'([^\\'\n]|\\.)*')
%%
<BOL>[ \t]+ {
 if(brace_depth>0){BEGIN(INITIAL);}
 else {
 int col=indentation(yytext,yyleng);
 if(col>indent_stack[indent_top]) {
   if(indent_top>=255){fprintf(stderr,"Pifari:%d: blocos aninhados demais\n",yylineno);exit(1);}
   indent_stack[++indent_top]=col; BEGIN(INITIAL); return INDENT;
 }
 if(col<indent_stack[indent_top]) {
   while(indent_top>0&&col<indent_stack[indent_top]){indent_top--;pending_dedents++;}
   if(col!=indent_stack[indent_top]){fprintf(stderr,"Pifari:%d: nível de indentação inconsistente\n",yylineno);exit(1);}
   BEGIN(INITIAL);
   if(pending_dedents){pending_dedents--;return DEDENT;}
 }
 BEGIN(INITIAL);
 }
}
<BOL>\n {yycolumn=1;return NEWLINE;}
<BOL>. { if(brace_depth>0){BEGIN(INITIAL);yyless(0);} else {if(indent_stack[indent_top]>0){while(indent_top>0){indent_top--;pending_dedents++;} BEGIN(INITIAL); yyless(0); if(pending_dedents){pending_dedents--;return DEDENT;} } BEGIN(INITIAL); yyless(0);} }
<INITIAL>\n {yycolumn=1;BEGIN(BOL);return NEWLINE;}
<INITIAL>"@\"\"\"" {BEGIN(COMMENT);}
<COMMENT>"\"\"\"@" {BEGIN(INITIAL);}
<COMMENT>\n {yycolumn=1;}
<COMMENT>. {}
<INITIAL>"@"[^\n]* {}
<INITIAL>"#include" {return INCLUDE;}
<INITIAL>"Link" {return LINK;}
<INITIAL>"to" {return TO;}
<INITIAL>"if" {return IF;}
<INITIAL>"else" {return ELSE;}
<INITIAL>"elseif" {return ELSEIF;}
<INITIAL>"function"|"func" {return FUNCTION;}
<INITIAL>"for" {return FOR;}
<INITIAL>"in" {return IN;}
<INITIAL>"spawn" {return SPAWN;}
<INITIAL>"await" {return AWAIT;}
<INITIAL>"return" {return RETURN;}
<INITIAL>"break" {return BREAK;}
<INITIAL>"continue" {return CONTINUE;}
<INITIAL>"import" {return IMPORT;}
<INITIAL>":=" {return CLASS_DEF;}
<INITIAL>"=:" {return TYPED_DECL;}
<INITIAL>"=" {return ASSIGN;}
<INITIAL>"true" {return TRUE;}
<INITIAL>"false" {return FALSE;}
<INITIAL>"null"|"None" {return NULLVAL;}
<INITIAL>"int"|"char"|"short"|"long"|"signed"|"unsigned"|"float"|"double"|"bool"|"void"|"txt"|"str"|"list"|"array"|"Task"|"Agent" {yylval.str=copy_text(yytext,yyleng);return TYPE;}
<INITIAL>"==" {return EQ;}
<INITIAL>"!=" {return NE;}
<INITIAL>"<=" {return LE;}
<INITIAL>">=" {return GE;}
<INITIAL>"&&"|"and" {return AND;}
<INITIAL>"||"|"or" {return OR;}
<INITIAL>"not"|"!" {return NOT;}
<INITIAL>"//" {return IDIV;}
<INITIAL>"**" {return POWER;}
<INITIAL>"|<" {return JOIN;}
<INITIAL>"<|" {return APPLY;}
<INITIAL>">|" {return FANOUT;}
<INITIAL>"|>" {return PIPE;}
<INITIAL>"->" {return ARROW;}
<INITIAL>"+" {return PLUS;}
<INITIAL>"-" {return MINUS;}
<INITIAL>"*" {return STAR;}
<INITIAL>"/" {return SLASH;}
<INITIAL>"%" {return PERCENT;}
<INITIAL>"<" {return LT;}
<INITIAL>">" {return GT;}
<INITIAL>"(" {return LPAREN;}
<INITIAL>")" {return RPAREN;}
<INITIAL>"{" {brace_depth++;return LBRACE;}
<INITIAL>"}" {if(brace_depth>0)brace_depth--;return RBRACE;}
<INITIAL>"[" {return LBRACKET;}
<INITIAL>"]" {return RBRACKET;}
<INITIAL>"," {return COMMA;}
<INITIAL>"." {return DOT;}
<INITIAL>":" {return COLON;}
<INITIAL>{FLOATN} {yylval.str=copy_text(yytext,yyleng);return FLOAT;}
<INITIAL>{INT} {yylval.str=copy_text(yytext,yyleng);return INTEGER;}
<INITIAL>{STR} {yylval.str=copy_text(yytext+1,yyleng-2);return STRING;}
<INITIAL>{ID} {yylval.str=copy_text(yytext,yyleng);return IDENTIFIER;}
<INITIAL>[ \t\r]+ {}
<INITIAL>. {fprintf(stderr,"Pifari:%d:%d: caractere inesperado '%s'\n",yylineno,yycolumn-yyleng,yytext);exit(1);}
<INITIAL,BOL><<EOF>> {while(indent_top>0){indent_top--;pending_dedents++;}if(pending_dedents){pending_dedents--;return DEDENT;}return 0;}
<COMMENT><<EOF>> {fprintf(stderr,"Pifari:%d: comentário multilinha não terminado\n",yylineno);return 1;}
%%
