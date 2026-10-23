%{
#include <stdio.h>
#include <stdlib.h>
#include "frontend/ast.h"
#include "backend/runtime.h"
int yylex(void); void yyerror(const char *s);
extern int yylineno, yycolumn; extern FILE *yyin;
Node *pifari_root=NULL;
static Node *as_block(Node *items,int l,int c){Node*b=node_new("Block",NULL,l,c);if(items){for(size_t i=0;i<items->count;i++)node_add(b,items->children[i]);free(items->children);free(items->kind);free(items->value);free(items);}return b;}
static char *join_path(char*a,char*b){size_t n=strlen(a)+strlen(b)+2;char*p=malloc(n);if(!p)exit(2);snprintf(p,n,"%s.%s",a,b);free(a);free(b);return p;}
%}
%define parse.error detailed
%expect 7 /* conflitos shift/reduce conhecidos; gramática ainda em refinamento */
%locations
%union { char *str; Node *node; }
%token <str> IDENTIFIER STRING INTEGER FLOAT TYPE
%token TRUE FALSE NULLVAL
%token IF ELSE ELSEIF FOR IN RETURN BREAK CONTINUE IMPORT LINK TO INCLUDE FUNCTION
%token CLASS_DEF TYPED_DECL ASSIGN
%token INDENT DEDENT NEWLINE
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET COMMA DOT COLON
%token PLUS MINUS STAR SLASH IDIV PERCENT POWER EQ NE LT LE GT GE AND OR NOT
%token PIPE APPLY FANOUT JOIN ARROW
%type <node> program lines statement simple_statement compound_statement suite expr primary arguments_opt arguments optional_expr type_expr parameters_opt parameters
%type <str> include_path
%precedence STMT_END
%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PIPE APPLY FANOUT JOIN
%left PLUS MINUS
%left STAR SLASH IDIV PERCENT
%right POWER
%precedence DOT LBRACKET
%precedence NOT UMINUS
%start program
%%
program: lines {pifari_root=node_new("Program",NULL,@1.first_line,@1.first_column);if($1){for(size_t i=0;i<$1->count;i++)node_add(pifari_root,$1->children[i]);free($1->children);free($1->kind);free($1->value);free($1);}};
lines: %empty {$$=node_new("Statements",NULL,@$.first_line,@$.first_column);}
 | lines NEWLINE {$$=$1;}
 | lines statement {$$=node_add($1,$2);}
;
statement: simple_statement {$$=$1;} | compound_statement {$$=$1;};
simple_statement:
 TYPED_DECL IDENTIFIER type_expr {$$=node_pair("Attribute",$2,$3,NULL,@1.first_line,@1.first_column);free($2);}
 | IDENTIFIER ASSIGN expr {$$=node_pair("Assign",$1,$3,NULL,@1.first_line,@1.first_column);free($1);}
 | IDENTIFIER TYPED_DECL type_expr {$$=node_pair("TypedDecl",$1,$3,NULL,@1.first_line,@1.first_column);free($1);}
 | IDENTIFIER TYPED_DECL type_expr LPAREN optional_expr RPAREN {$$=node_pair("TypedDecl",$1,$3,$5,@1.first_line,@1.first_column);free($1);}
 | RETURN NEWLINE {$$=node_new("Return",NULL,@1.first_line,@1.first_column);}
 | RETURN expr %prec STMT_END {$$=node_pair("Return",NULL,$2,NULL,@1.first_line,@1.first_column);}
 | BREAK {$$=node_new("Break",NULL,@1.first_line,@1.first_column);}
 | CONTINUE {$$=node_new("Continue",NULL,@1.first_line,@1.first_column);}
 | IMPORT IDENTIFIER {$$=node_new("Import",$2,@1.first_line,@1.first_column);free($2);}
 | IMPORT STRING {$$=node_new("Import",$2,@1.first_line,@1.first_column);free($2);}
 | LINK TO STRING {$$=node_new("LinkTo",$3,@1.first_line,@1.first_column);free($3);}
 | LINK TO IDENTIFIER {$$=node_new("LinkTo",$3,@1.first_line,@1.first_column);free($3);}
 | INCLUDE LT include_path GT {$$=node_new("Include",$3,@1.first_line,@1.first_column);free($3);}
 | INCLUDE STRING {$$=node_new("Include",$2,@1.first_line,@1.first_column);free($2);}
 | expr %prec STMT_END {$$=node_pair("ExpressionStatement",NULL,$1,NULL,@1.first_line,@1.first_column);}
;
include_path: IDENTIFIER {$$=$1;} | include_path DOT IDENTIFIER {$$=join_path($1,$3);};
optional_expr: %empty {$$=NULL;} | expr {$$=$1;};
compound_statement:
 FUNCTION IDENTIFIER LPAREN parameters_opt RPAREN suite {$$=node_pair("Function",$2,$4,$6,@1.first_line,@1.first_column);free($2);}
 | IDENTIFIER COLON LPAREN parameters_opt RPAREN suite {$$=node_pair("FunctionExpr",$1,$4,$6,@1.first_line,@1.first_column);free($1);}
 | CLASS_DEF IDENTIFIER suite {$$=node_pair("Class",$2,$3,NULL,@1.first_line,@1.first_column);free($2);}
 | IF expr suite {$$=node_pair("If",NULL,$2,$3,@1.first_line,@1.first_column);}
 | IF expr suite ELSE suite {$$=node_pair("IfElse",NULL,node_pair("Condition",NULL,$2,$3,@1.first_line,@1.first_column),$5,@1.first_line,@1.first_column);}
 | IF expr suite ELSEIF expr suite {$$=node_pair("IfElseIf",NULL,node_pair("Condition",NULL,$2,$3,@1.first_line,@1.first_column),node_pair("ElseIf",NULL,$5,$6,@1.first_line,@1.first_column),@1.first_line,@1.first_column);}
 | FOR IDENTIFIER IN expr suite {$$=node_pair("For",$2,$4,$5,@1.first_line,@1.first_column);free($2);}
;
suite: NEWLINE INDENT lines DEDENT {$$=as_block($3,@1.first_line,@1.first_column);}
 | LBRACE lines RBRACE {$$=as_block($2,@1.first_line,@1.first_column);}
;
expr:
 primary {$$=$1;}
 | expr PLUS expr {$$=node_pair("Binary","+", $1,$3,@2.first_line,@2.first_column);}
 | expr MINUS expr {$$=node_pair("Binary","-", $1,$3,@2.first_line,@2.first_column);}
 | expr STAR expr {$$=node_pair("Binary","*", $1,$3,@2.first_line,@2.first_column);}
 | expr SLASH expr {$$=node_pair("Binary","/", $1,$3,@2.first_line,@2.first_column);}
 | expr IDIV expr {$$=node_pair("Binary","//", $1,$3,@2.first_line,@2.first_column);}
 | expr PERCENT expr {$$=node_pair("Binary","%", $1,$3,@2.first_line,@2.first_column);}
 | expr POWER expr {$$=node_pair("Binary","**", $1,$3,@2.first_line,@2.first_column);}
 | expr EQ expr {$$=node_pair("Binary","==", $1,$3,@2.first_line,@2.first_column);}
 | expr NE expr {$$=node_pair("Binary","!=", $1,$3,@2.first_line,@2.first_column);}
 | expr LT expr {$$=node_pair("Binary","<", $1,$3,@2.first_line,@2.first_column);}
 | expr LE expr {$$=node_pair("Binary","<=", $1,$3,@2.first_line,@2.first_column);}
 | expr GT expr {$$=node_pair("Binary",">", $1,$3,@2.first_line,@2.first_column);}
 | expr GE expr {$$=node_pair("Binary",">=", $1,$3,@2.first_line,@2.first_column);}
 | expr AND expr {$$=node_pair("Binary","and", $1,$3,@2.first_line,@2.first_column);}
 | expr OR expr {$$=node_pair("Binary","or", $1,$3,@2.first_line,@2.first_column);}
 | expr PIPE expr {$$=node_pair("Flow","|>", $1,$3,@2.first_line,@2.first_column);}
 | expr APPLY expr {$$=node_pair("Flow","<|", $1,$3,@2.first_line,@2.first_column);}
 | expr FANOUT expr {$$=node_pair("Flow",">|", $1,$3,@2.first_line,@2.first_column);}
 | expr JOIN expr {$$=node_pair("Flow","|<", $1,$3,@2.first_line,@2.first_column);}
 | MINUS expr %prec UMINUS {$$=node_pair("Unary","-",$2,NULL,@1.first_line,@1.first_column);}
 | NOT expr {$$=node_pair("Unary","not",$2,NULL,@1.first_line,@1.first_column);}
 | expr DOT IDENTIFIER {$$=node_pair("Member",$3,$1,NULL,@2.first_line,@2.first_column);free($3);}
 | expr LBRACKET expr RBRACKET {$$=node_pair("Index",NULL,$1,$3,@2.first_line,@2.first_column);}
;
primary:
 IDENTIFIER {$$=node_new("Name",$1,@1.first_line,@1.first_column);free($1);}
 | STRING {$$=node_new("String",$1,@1.first_line,@1.first_column);free($1);}
 | INTEGER {$$=node_new("Integer",$1,@1.first_line,@1.first_column);free($1);}
 | FLOAT {$$=node_new("Float",$1,@1.first_line,@1.first_column);free($1);}
 | TYPE {$$=node_new("Name",$1,@1.first_line,@1.first_column);free($1);}
 | TRUE {$$=node_new("Boolean","true",@1.first_line,@1.first_column);}
 | FALSE {$$=node_new("Boolean","false",@1.first_line,@1.first_column);}
 | NULLVAL {$$=node_new("Null",NULL,@1.first_line,@1.first_column);}
 | LPAREN expr RPAREN {$$=$2;}
 | LBRACKET arguments_opt RBRACKET {$$=node_pair("List",NULL,$2,NULL,@1.first_line,@1.first_column);}
 | IDENTIFIER LPAREN arguments_opt RPAREN {$$=node_pair("Call",$1,$3,NULL,@1.first_line,@1.first_column);free($1);}
 | TYPE LPAREN arguments_opt RPAREN {$$=node_pair("Call",$1,$3,NULL,@1.first_line,@1.first_column);free($1);}
;
parameters_opt: %empty {$$=node_new("Parameters",NULL,@$.first_line,@$.first_column);} | parameters {$$=$1;};
parameters: IDENTIFIER {$$=node_add(node_new("Parameters",NULL,@1.first_line,@1.first_column),node_new("Name",$1,@1.first_line,@1.first_column));free($1);} | parameters COMMA IDENTIFIER {$$=node_add($1,node_new("Name",$3,@3.first_line,@3.first_column));free($3);};
arguments_opt: %empty {$$=node_new("Arguments",NULL,@$.first_line,@$.first_column);} | arguments {$$=$1;};
arguments: expr {$$=node_add(node_new("Arguments",NULL,@1.first_line,@1.first_column),$1);} | arguments COMMA expr {$$=node_add($1,$3);};
type_expr: TYPE {$$=node_new("Type",$1,@1.first_line,@1.first_column);free($1);} | IDENTIFIER {$$=node_new("Type",$1,@1.first_line,@1.first_column);free($1);};
%%
void yyerror(const char*s){fprintf(stderr,"Pifari:%d:%d: erro sintático: %s\n",yylloc.first_line,yylloc.first_column,s);}
int main(int argc,char**argv){if(argc!=2){fprintf(stderr,"Uso: %s arquivo.pif\n",argv[0]);return 2;}yyin=fopen(argv[1],"r");if(!yyin){perror(argv[1]);return 2;}int parsed=yyparse();int status=1;if(parsed==0&&pifari_root)status=runtime_execute(pifari_root);node_free(pifari_root);fclose(yyin);return status;}
