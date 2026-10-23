/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_H_INCLUDED
# define YY_YY_PARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    IDENTIFIER = 258,              /* IDENTIFIER  */
    STRING = 259,                  /* STRING  */
    INTEGER = 260,                 /* INTEGER  */
    FLOAT = 261,                   /* FLOAT  */
    TYPE = 262,                    /* TYPE  */
    TRUE = 263,                    /* TRUE  */
    FALSE = 264,                   /* FALSE  */
    NULLVAL = 265,                 /* NULLVAL  */
    IF = 266,                      /* IF  */
    ELSE = 267,                    /* ELSE  */
    ELSEIF = 268,                  /* ELSEIF  */
    FOR = 269,                     /* FOR  */
    IN = 270,                      /* IN  */
    RETURN = 271,                  /* RETURN  */
    BREAK = 272,                   /* BREAK  */
    CONTINUE = 273,                /* CONTINUE  */
    IMPORT = 274,                  /* IMPORT  */
    LINK = 275,                    /* LINK  */
    TO = 276,                      /* TO  */
    INCLUDE = 277,                 /* INCLUDE  */
    FUNCTION = 278,                /* FUNCTION  */
    CLASS_DEF = 279,               /* CLASS_DEF  */
    TYPED_DECL = 280,              /* TYPED_DECL  */
    ASSIGN = 281,                  /* ASSIGN  */
    INDENT = 282,                  /* INDENT  */
    DEDENT = 283,                  /* DEDENT  */
    NEWLINE = 284,                 /* NEWLINE  */
    LPAREN = 285,                  /* LPAREN  */
    RPAREN = 286,                  /* RPAREN  */
    LBRACE = 287,                  /* LBRACE  */
    RBRACE = 288,                  /* RBRACE  */
    LBRACKET = 289,                /* LBRACKET  */
    RBRACKET = 290,                /* RBRACKET  */
    COMMA = 291,                   /* COMMA  */
    DOT = 292,                     /* DOT  */
    COLON = 293,                   /* COLON  */
    PLUS = 294,                    /* PLUS  */
    MINUS = 295,                   /* MINUS  */
    STAR = 296,                    /* STAR  */
    SLASH = 297,                   /* SLASH  */
    IDIV = 298,                    /* IDIV  */
    PERCENT = 299,                 /* PERCENT  */
    POWER = 300,                   /* POWER  */
    EQ = 301,                      /* EQ  */
    NE = 302,                      /* NE  */
    LT = 303,                      /* LT  */
    LE = 304,                      /* LE  */
    GT = 305,                      /* GT  */
    GE = 306,                      /* GE  */
    AND = 307,                     /* AND  */
    OR = 308,                      /* OR  */
    NOT = 309,                     /* NOT  */
    PIPE = 310,                    /* PIPE  */
    APPLY = 311,                   /* APPLY  */
    FANOUT = 312,                  /* FANOUT  */
    JOIN = 313,                    /* JOIN  */
    ARROW = 314,                   /* ARROW  */
    STMT_END = 315,                /* STMT_END  */
    UMINUS = 316                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 15 "frontend/parser.y"
 char *str; Node *node; 

#line 128 "parser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_H_INCLUDED  */
