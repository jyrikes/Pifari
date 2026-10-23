/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "frontend/parser.y"

#include <stdio.h>
#include <stdlib.h>
#include "frontend/ast.h"
#include "backend/runtime.h"
int yylex(void); void yyerror(const char *s);
extern int yylineno, yycolumn; extern FILE *yyin;
Node *pifari_root=NULL;
static Node *as_block(Node *items,int l,int c){Node*b=node_new("Block",NULL,l,c);if(items){for(size_t i=0;i<items->count;i++)node_add(b,items->children[i]);free(items->children);free(items->kind);free(items->value);free(items);}return b;}
static char *join_path(char*a,char*b){size_t n=strlen(a)+strlen(b)+2;char*p=malloc(n);if(!p)exit(2);snprintf(p,n,"%s.%s",a,b);free(a);free(b);return p;}

#line 83 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENTIFIER = 3,                 /* IDENTIFIER  */
  YYSYMBOL_STRING = 4,                     /* STRING  */
  YYSYMBOL_INTEGER = 5,                    /* INTEGER  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_TYPE = 7,                       /* TYPE  */
  YYSYMBOL_TRUE = 8,                       /* TRUE  */
  YYSYMBOL_FALSE = 9,                      /* FALSE  */
  YYSYMBOL_NULLVAL = 10,                   /* NULLVAL  */
  YYSYMBOL_IF = 11,                        /* IF  */
  YYSYMBOL_ELSE = 12,                      /* ELSE  */
  YYSYMBOL_ELSEIF = 13,                    /* ELSEIF  */
  YYSYMBOL_FOR = 14,                       /* FOR  */
  YYSYMBOL_IN = 15,                        /* IN  */
  YYSYMBOL_RETURN = 16,                    /* RETURN  */
  YYSYMBOL_BREAK = 17,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 18,                  /* CONTINUE  */
  YYSYMBOL_IMPORT = 19,                    /* IMPORT  */
  YYSYMBOL_LINK = 20,                      /* LINK  */
  YYSYMBOL_TO = 21,                        /* TO  */
  YYSYMBOL_INCLUDE = 22,                   /* INCLUDE  */
  YYSYMBOL_FUNCTION = 23,                  /* FUNCTION  */
  YYSYMBOL_CLASS_DEF = 24,                 /* CLASS_DEF  */
  YYSYMBOL_TYPED_DECL = 25,                /* TYPED_DECL  */
  YYSYMBOL_ASSIGN = 26,                    /* ASSIGN  */
  YYSYMBOL_INDENT = 27,                    /* INDENT  */
  YYSYMBOL_DEDENT = 28,                    /* DEDENT  */
  YYSYMBOL_NEWLINE = 29,                   /* NEWLINE  */
  YYSYMBOL_LPAREN = 30,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 31,                    /* RPAREN  */
  YYSYMBOL_LBRACE = 32,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 33,                    /* RBRACE  */
  YYSYMBOL_LBRACKET = 34,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 35,                  /* RBRACKET  */
  YYSYMBOL_COMMA = 36,                     /* COMMA  */
  YYSYMBOL_DOT = 37,                       /* DOT  */
  YYSYMBOL_COLON = 38,                     /* COLON  */
  YYSYMBOL_PLUS = 39,                      /* PLUS  */
  YYSYMBOL_MINUS = 40,                     /* MINUS  */
  YYSYMBOL_STAR = 41,                      /* STAR  */
  YYSYMBOL_SLASH = 42,                     /* SLASH  */
  YYSYMBOL_IDIV = 43,                      /* IDIV  */
  YYSYMBOL_PERCENT = 44,                   /* PERCENT  */
  YYSYMBOL_POWER = 45,                     /* POWER  */
  YYSYMBOL_EQ = 46,                        /* EQ  */
  YYSYMBOL_NE = 47,                        /* NE  */
  YYSYMBOL_LT = 48,                        /* LT  */
  YYSYMBOL_LE = 49,                        /* LE  */
  YYSYMBOL_GT = 50,                        /* GT  */
  YYSYMBOL_GE = 51,                        /* GE  */
  YYSYMBOL_AND = 52,                       /* AND  */
  YYSYMBOL_OR = 53,                        /* OR  */
  YYSYMBOL_NOT = 54,                       /* NOT  */
  YYSYMBOL_PIPE = 55,                      /* PIPE  */
  YYSYMBOL_APPLY = 56,                     /* APPLY  */
  YYSYMBOL_FANOUT = 57,                    /* FANOUT  */
  YYSYMBOL_JOIN = 58,                      /* JOIN  */
  YYSYMBOL_ARROW = 59,                     /* ARROW  */
  YYSYMBOL_STMT_END = 60,                  /* STMT_END  */
  YYSYMBOL_UMINUS = 61,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 62,                  /* $accept  */
  YYSYMBOL_program = 63,                   /* program  */
  YYSYMBOL_lines = 64,                     /* lines  */
  YYSYMBOL_statement = 65,                 /* statement  */
  YYSYMBOL_simple_statement = 66,          /* simple_statement  */
  YYSYMBOL_include_path = 67,              /* include_path  */
  YYSYMBOL_optional_expr = 68,             /* optional_expr  */
  YYSYMBOL_compound_statement = 69,        /* compound_statement  */
  YYSYMBOL_suite = 70,                     /* suite  */
  YYSYMBOL_expr = 71,                      /* expr  */
  YYSYMBOL_primary = 72,                   /* primary  */
  YYSYMBOL_parameters_opt = 73,            /* parameters_opt  */
  YYSYMBOL_parameters = 74,                /* parameters  */
  YYSYMBOL_arguments_opt = 75,             /* arguments_opt  */
  YYSYMBOL_arguments = 76,                 /* arguments  */
  YYSYMBOL_type_expr = 77                  /* type_expr  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   528

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  81
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  153

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   316


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    39,    39,    40,    41,    42,    44,    44,    46,    47,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    80,    80,    81,    81,    83,    84,    85,
      86,    87,    88,    89,    91,    92,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   134,   134,   135,   135,   136,   136,   137,   137,
     138,   138
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  static const char *const yy_sname[] =
  {
  "end of file", "error", "invalid token", "IDENTIFIER", "STRING",
  "INTEGER", "FLOAT", "TYPE", "TRUE", "FALSE", "NULLVAL", "IF", "ELSE",
  "ELSEIF", "FOR", "IN", "RETURN", "BREAK", "CONTINUE", "IMPORT", "LINK",
  "TO", "INCLUDE", "FUNCTION", "CLASS_DEF", "TYPED_DECL", "ASSIGN",
  "INDENT", "DEDENT", "NEWLINE", "LPAREN", "RPAREN", "LBRACE", "RBRACE",
  "LBRACKET", "RBRACKET", "COMMA", "DOT", "COLON", "PLUS", "MINUS", "STAR",
  "SLASH", "IDIV", "PERCENT", "POWER", "EQ", "NE", "LT", "LE", "GT", "GE",
  "AND", "OR", "NOT", "PIPE", "APPLY", "FANOUT", "JOIN", "ARROW",
  "STMT_END", "UMINUS", "$accept", "program", "lines", "statement",
  "simple_statement", "include_path", "optional_expr",
  "compound_statement", "suite", "expr", "primary", "parameters_opt",
  "parameters", "arguments_opt", "arguments", "type_expr", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

#define YYPACT_NINF (-81)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -81,    11,   193,   -81,   -22,   -81,   -81,   -81,   -10,   -81,
     -81,   -81,   246,    26,   231,   -81,   -81,    23,    17,    19,
      63,    65,    67,   -81,   246,   246,   246,   246,   -81,   -81,
     -81,   262,   -81,    34,   246,    41,   246,    43,   292,    59,
     -81,   370,   -81,   -81,    36,   -81,    79,    54,   -23,    34,
     320,   370,    51,    52,   -81,   -81,   246,   246,    84,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   -81,   -81,
      60,    58,    89,    62,    68,   -81,    30,   246,   -81,   -81,
     -81,   -33,    89,   -81,   -81,   -81,   -81,   246,   370,   345,
     -81,    35,    35,   -27,   -27,   -27,   -27,   -27,   445,   445,
     470,   470,   470,   470,   420,   395,    -9,    -9,    -9,    -9,
     246,   -81,   -81,    66,    64,   -81,   -81,   115,   -23,   246,
     292,    91,   -81,    70,   370,   -81,    71,   370,   -23,    95,
     154,   -81,   -81,   292,   -81,   -81,   -23,   -81,   -81,   -81,
     -81,   -81,   -81
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     0,     2,     1,    60,    61,    62,    63,    64,    65,
      66,    67,     0,     0,     0,    14,    15,     0,     0,     0,
       0,     0,     0,     4,     0,    76,     0,     0,     5,     6,
       7,    22,    36,     0,    76,     0,    76,    60,     0,     0,
      12,    13,    16,    17,     0,    21,     0,     0,     0,     0,
       0,    78,     0,    77,    56,    57,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    81,    80,
      10,     0,    72,     0,     0,     3,    30,     0,    19,    18,
      23,     0,    72,    29,     8,    68,    69,     0,     9,     0,
      58,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      25,    70,    74,     0,    73,    71,     3,     0,     0,     0,
       0,     0,    20,     0,    79,    59,     0,    26,     0,     0,
       0,    35,    31,     0,    33,    24,     0,    11,    28,    75,
      34,    32,    27
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -81,   -81,   -80,   -81,   -81,   -81,   -81,   -81,   -47,   -12,
     -81,    12,   -81,   -15,   -81,    56
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     2,    28,    29,    91,   136,    30,    86,    31,
      32,   123,   124,    52,    53,    80
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      38,    93,    41,    33,   131,   127,    84,    57,    34,    85,
      58,     3,    50,    51,    54,    55,    35,   132,    65,    81,
      36,    83,    51,    45,    51,    57,    42,    43,    58,    39,
      59,    60,    61,    62,    63,    64,    65,    78,    44,    88,
      89,    79,   128,   129,    98,    99,   140,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,    47,    46,    48,    57,
      49,    82,    58,    34,    87,   130,    61,    62,    63,    64,
      65,   142,    90,   144,    92,   134,    96,   100,    97,   121,
     120,   148,   122,   125,   145,   126,   151,   138,   149,   152,
     139,   146,   147,     0,   133,    94,     0,     0,   137,     0,
       0,     0,     0,     0,     0,     0,     0,   143,     4,     5,
       6,     7,     8,     9,    10,    11,    12,     0,     0,    13,
       0,    14,    15,    16,    17,    18,     0,    19,    20,    21,
      22,     0,     0,     0,    23,    24,     0,     0,   141,    25,
       0,     0,     0,     0,     0,    26,     0,     4,     5,     6,
       7,     8,     9,    10,    11,    12,     0,     0,    13,    27,
      14,    15,    16,    17,    18,     0,    19,    20,    21,    22,
       0,     0,   150,    23,    24,     0,     0,     0,    25,     0,
       0,     0,     0,     0,    26,     0,     4,     5,     6,     7,
       8,     9,    10,    11,    12,     0,     0,    13,    27,    14,
      15,    16,    17,    18,     0,    19,    20,    21,    22,     0,
       0,     0,    23,    24,     0,     0,     0,    25,     0,     0,
       0,     0,     0,    26,    37,     5,     6,     7,     8,     9,
      10,    11,     0,     0,     0,     0,     0,    27,     0,    37,
       5,     6,     7,     8,     9,    10,    11,     0,     0,     0,
      40,    24,     0,     0,     0,    25,     0,     0,     0,     0,
       0,    26,     0,     0,     0,     0,    24,     0,     0,     0,
      25,     0,     0,     0,     0,    27,    26,     0,    56,     0,
       0,     0,     0,     0,     0,     0,    57,     0,     0,    58,
      27,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,     0,    74,    75,    76,
      77,    84,     0,     0,    85,     0,    57,     0,     0,    58,
       0,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,     0,    74,    75,    76,
      77,    95,     0,     0,    57,     0,     0,    58,     0,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,     0,    74,    75,    76,    77,    57,
     135,     0,    58,     0,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,     0,
      74,    75,    76,    77,    57,     0,     0,    58,     0,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,     0,    74,    75,    76,    77,    57,
       0,     0,    58,     0,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,     0,     0,
      74,    75,    76,    77,    57,     0,     0,    58,     0,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,     0,     0,     0,    74,    75,    76,    77,    57,
       0,     0,    58,     0,    59,    60,    61,    62,    63,    64,
      65,     0,     0,    68,    69,    70,    71,     0,     0,     0,
      74,    75,    76,    77,    57,     0,     0,    58,     0,    59,
      60,    61,    62,    63,    64,    65,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    74,    75,    76,    77
};

static const yytype_int16 yycheck[] =
{
      12,    48,    14,    25,    37,    85,    29,    34,    30,    32,
      37,     0,    24,    25,    26,    27,    38,    50,    45,    34,
      30,    36,    34,     4,    36,    34,     3,     4,    37,     3,
      39,    40,    41,    42,    43,    44,    45,     3,    21,     3,
       4,     7,    12,    13,    56,    57,   126,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,     3,    48,     3,    34,
       3,    30,    37,    30,    15,    87,    41,    42,    43,    44,
      45,   128,     3,   130,    30,    97,    35,     3,    36,    31,
      30,   138,     3,    31,     3,    27,   143,    31,     3,   146,
      36,    31,    31,    -1,    92,    49,    -1,    -1,   120,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    -1,    14,
      -1,    16,    17,    18,    19,    20,    -1,    22,    23,    24,
      25,    -1,    -1,    -1,    29,    30,    -1,    -1,    33,    34,
      -1,    -1,    -1,    -1,    -1,    40,    -1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    14,    54,
      16,    17,    18,    19,    20,    -1,    22,    23,    24,    25,
      -1,    -1,    28,    29,    30,    -1,    -1,    -1,    34,    -1,
      -1,    -1,    -1,    -1,    40,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    -1,    14,    54,    16,
      17,    18,    19,    20,    -1,    22,    23,    24,    25,    -1,
      -1,    -1,    29,    30,    -1,    -1,    -1,    34,    -1,    -1,
      -1,    -1,    -1,    40,     3,     4,     5,     6,     7,     8,
       9,    10,    -1,    -1,    -1,    -1,    -1,    54,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    -1,    -1,    -1,
      29,    30,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    40,    -1,    -1,    -1,    -1,    30,    -1,    -1,    -1,
      34,    -1,    -1,    -1,    -1,    54,    40,    -1,    26,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,    37,
      54,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    56,    57,
      58,    29,    -1,    -1,    32,    -1,    34,    -1,    -1,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    55,    56,    57,
      58,    31,    -1,    -1,    34,    -1,    -1,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    -1,    55,    56,    57,    58,    34,
      35,    -1,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    56,    57,    58,    34,    -1,    -1,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    -1,    55,    56,    57,    58,    34,
      -1,    -1,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      55,    56,    57,    58,    34,    -1,    -1,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    -1,    -1,    -1,    55,    56,    57,    58,    34,
      -1,    -1,    37,    -1,    39,    40,    41,    42,    43,    44,
      45,    -1,    -1,    48,    49,    50,    51,    -1,    -1,    -1,
      55,    56,    57,    58,    34,    -1,    -1,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    55,    56,    57,    58
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    63,    64,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    14,    16,    17,    18,    19,    20,    22,
      23,    24,    25,    29,    30,    34,    40,    54,    65,    66,
      69,    71,    72,    25,    30,    38,    30,     3,    71,     3,
      29,    71,     3,     4,    21,     4,    48,     3,     3,     3,
      71,    71,    75,    76,    71,    71,    26,    34,    37,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    55,    56,    57,    58,     3,     7,
      77,    75,    30,    75,    29,    32,    70,    15,     3,     4,
       3,    67,    30,    70,    77,    31,    35,    36,    71,    71,
       3,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      30,    31,     3,    73,    74,    31,    27,    64,    12,    13,
      71,    37,    50,    73,    71,    35,    68,    71,    31,    36,
      64,    33,    70,    71,    70,     3,    31,    31,    70,     3,
      28,    70,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    62,    63,    64,    64,    64,    65,    65,    66,    66,
      66,    66,    66,    66,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    67,    67,    68,    68,    69,    69,    69,
      69,    69,    69,    69,    70,    70,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    73,    73,    74,    74,    75,    75,    76,    76,
      77,    77
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     2,     2,     1,     1,     3,     3,
       3,     6,     2,     2,     1,     1,     2,     2,     3,     3,
       4,     2,     1,     1,     3,     0,     1,     6,     6,     3,
       3,     5,     6,     5,     4,     3,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     3,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       4,     4,     0,     1,     1,     3,     0,     1,     1,     3,
       1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif



static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yystrlen (yysymbol_name (yyarg[yyi]));
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp = yystpcpy (yyp, yysymbol_name (yyarg[yyi++]));
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: lines  */
#line 39 "frontend/parser.y"
               {pifari_root=node_new("Program",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column);if((yyvsp[0].node)){for(size_t i=0;i<(yyvsp[0].node)->count;i++)node_add(pifari_root,(yyvsp[0].node)->children[i]);free((yyvsp[0].node)->children);free((yyvsp[0].node)->kind);free((yyvsp[0].node)->value);free((yyvsp[0].node));}}
#line 1659 "parser.tab.c"
    break;

  case 3: /* lines: %empty  */
#line 40 "frontend/parser.y"
              {(yyval.node)=node_new("Statements",NULL,(yyloc).first_line,(yyloc).first_column);}
#line 1665 "parser.tab.c"
    break;

  case 4: /* lines: lines NEWLINE  */
#line 41 "frontend/parser.y"
                 {(yyval.node)=(yyvsp[-1].node);}
#line 1671 "parser.tab.c"
    break;

  case 5: /* lines: lines statement  */
#line 42 "frontend/parser.y"
                   {(yyval.node)=node_add((yyvsp[-1].node),(yyvsp[0].node));}
#line 1677 "parser.tab.c"
    break;

  case 6: /* statement: simple_statement  */
#line 44 "frontend/parser.y"
                            {(yyval.node)=(yyvsp[0].node);}
#line 1683 "parser.tab.c"
    break;

  case 7: /* statement: compound_statement  */
#line 44 "frontend/parser.y"
                                                          {(yyval.node)=(yyvsp[0].node);}
#line 1689 "parser.tab.c"
    break;

  case 8: /* simple_statement: TYPED_DECL IDENTIFIER type_expr  */
#line 46 "frontend/parser.y"
                                 {(yyval.node)=node_pair("Attribute",(yyvsp[-1].str),(yyvsp[0].node),NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);free((yyvsp[-1].str));}
#line 1695 "parser.tab.c"
    break;

  case 9: /* simple_statement: expr ASSIGN expr  */
#line 47 "frontend/parser.y"
                    {
   if((yyvsp[-2].node)&&!strcmp((yyvsp[-2].node)->kind,"Name")){
     (yyval.node)=node_pair("Assign",(yyvsp[-2].node)->value,(yyvsp[0].node),NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);
     node_free((yyvsp[-2].node));
   } else if((yyvsp[-2].node)&&!strcmp((yyvsp[-2].node)->kind,"Index")&&(yyvsp[-2].node)->count==2&&(yyvsp[-2].node)->children[0]&&!strcmp((yyvsp[-2].node)->children[0]->kind,"Name")){
     (yyval.node)=node_pair("IndexAssign",(yyvsp[-2].node)->children[0]->value,(yyvsp[-2].node)->children[1],(yyvsp[0].node),(yylsp[-2]).first_line,(yylsp[-2]).first_column);
     (yyvsp[-2].node)->children[1]=NULL;
     node_free((yyvsp[-2].node));
   } else if((yyvsp[-2].node)&&!strcmp((yyvsp[-2].node)->kind,"Index")&&(yyvsp[-2].node)->count==2){
     Node *n=node_new("IndexAssignNested",NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);
     node_add(n,(yyvsp[-2].node)->children[0]);node_add(n,(yyvsp[-2].node)->children[1]);node_add(n,(yyvsp[0].node));
     (yyvsp[-2].node)->children[0]=NULL;(yyvsp[-2].node)->children[1]=NULL;
     node_free((yyvsp[-2].node));
     (yyval.node)=n;
   } else {
     yyerror("lado esquerdo de '=' deve ser identificador ou elemento indexado");
     node_free((yyvsp[-2].node));node_free((yyvsp[0].node));YYERROR;
   }
 }
#line 1719 "parser.tab.c"
    break;

  case 10: /* simple_statement: IDENTIFIER TYPED_DECL type_expr  */
#line 66 "frontend/parser.y"
                                   {(yyval.node)=node_pair("TypedDecl",(yyvsp[-2].str),(yyvsp[0].node),NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);free((yyvsp[-2].str));}
#line 1725 "parser.tab.c"
    break;

  case 11: /* simple_statement: IDENTIFIER TYPED_DECL type_expr LPAREN optional_expr RPAREN  */
#line 67 "frontend/parser.y"
                                                               {(yyval.node)=node_pair("TypedDecl",(yyvsp[-5].str),(yyvsp[-3].node),(yyvsp[-1].node),(yylsp[-5]).first_line,(yylsp[-5]).first_column);free((yyvsp[-5].str));}
#line 1731 "parser.tab.c"
    break;

  case 12: /* simple_statement: RETURN NEWLINE  */
#line 68 "frontend/parser.y"
                  {(yyval.node)=node_new("Return",NULL,(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1737 "parser.tab.c"
    break;

  case 13: /* simple_statement: RETURN expr  */
#line 69 "frontend/parser.y"
                              {(yyval.node)=node_pair("Return",NULL,(yyvsp[0].node),NULL,(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1743 "parser.tab.c"
    break;

  case 14: /* simple_statement: BREAK  */
#line 70 "frontend/parser.y"
         {(yyval.node)=node_new("Break",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 1749 "parser.tab.c"
    break;

  case 15: /* simple_statement: CONTINUE  */
#line 71 "frontend/parser.y"
            {(yyval.node)=node_new("Continue",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 1755 "parser.tab.c"
    break;

  case 16: /* simple_statement: IMPORT IDENTIFIER  */
#line 72 "frontend/parser.y"
                     {(yyval.node)=node_new("Import",(yyvsp[0].str),(yylsp[-1]).first_line,(yylsp[-1]).first_column);free((yyvsp[0].str));}
#line 1761 "parser.tab.c"
    break;

  case 17: /* simple_statement: IMPORT STRING  */
#line 73 "frontend/parser.y"
                 {(yyval.node)=node_new("Import",(yyvsp[0].str),(yylsp[-1]).first_line,(yylsp[-1]).first_column);free((yyvsp[0].str));}
#line 1767 "parser.tab.c"
    break;

  case 18: /* simple_statement: LINK TO STRING  */
#line 74 "frontend/parser.y"
                  {(yyval.node)=node_new("LinkTo",(yyvsp[0].str),(yylsp[-2]).first_line,(yylsp[-2]).first_column);free((yyvsp[0].str));}
#line 1773 "parser.tab.c"
    break;

  case 19: /* simple_statement: LINK TO IDENTIFIER  */
#line 75 "frontend/parser.y"
                      {(yyval.node)=node_new("LinkTo",(yyvsp[0].str),(yylsp[-2]).first_line,(yylsp[-2]).first_column);free((yyvsp[0].str));}
#line 1779 "parser.tab.c"
    break;

  case 20: /* simple_statement: INCLUDE LT include_path GT  */
#line 76 "frontend/parser.y"
                              {(yyval.node)=node_new("Include",(yyvsp[-1].str),(yylsp[-3]).first_line,(yylsp[-3]).first_column);free((yyvsp[-1].str));}
#line 1785 "parser.tab.c"
    break;

  case 21: /* simple_statement: INCLUDE STRING  */
#line 77 "frontend/parser.y"
                  {(yyval.node)=node_new("Include",(yyvsp[0].str),(yylsp[-1]).first_line,(yylsp[-1]).first_column);free((yyvsp[0].str));}
#line 1791 "parser.tab.c"
    break;

  case 22: /* simple_statement: expr  */
#line 78 "frontend/parser.y"
                       {(yyval.node)=node_pair("ExpressionStatement",NULL,(yyvsp[0].node),NULL,(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 1797 "parser.tab.c"
    break;

  case 23: /* include_path: IDENTIFIER  */
#line 80 "frontend/parser.y"
                         {(yyval.str)=(yyvsp[0].str);}
#line 1803 "parser.tab.c"
    break;

  case 24: /* include_path: include_path DOT IDENTIFIER  */
#line 80 "frontend/parser.y"
                                                                {(yyval.str)=join_path((yyvsp[-2].str),(yyvsp[0].str));}
#line 1809 "parser.tab.c"
    break;

  case 25: /* optional_expr: %empty  */
#line 81 "frontend/parser.y"
                      {(yyval.node)=NULL;}
#line 1815 "parser.tab.c"
    break;

  case 26: /* optional_expr: expr  */
#line 81 "frontend/parser.y"
                                        {(yyval.node)=(yyvsp[0].node);}
#line 1821 "parser.tab.c"
    break;

  case 27: /* compound_statement: FUNCTION IDENTIFIER LPAREN parameters_opt RPAREN suite  */
#line 83 "frontend/parser.y"
                                                        {(yyval.node)=node_pair("Function",(yyvsp[-4].str),(yyvsp[-2].node),(yyvsp[0].node),(yylsp[-5]).first_line,(yylsp[-5]).first_column);free((yyvsp[-4].str));}
#line 1827 "parser.tab.c"
    break;

  case 28: /* compound_statement: IDENTIFIER COLON LPAREN parameters_opt RPAREN suite  */
#line 84 "frontend/parser.y"
                                                       {(yyval.node)=node_pair("FunctionExpr",(yyvsp[-5].str),(yyvsp[-2].node),(yyvsp[0].node),(yylsp[-5]).first_line,(yylsp[-5]).first_column);free((yyvsp[-5].str));}
#line 1833 "parser.tab.c"
    break;

  case 29: /* compound_statement: CLASS_DEF IDENTIFIER suite  */
#line 85 "frontend/parser.y"
                              {(yyval.node)=node_pair("Class",(yyvsp[-1].str),(yyvsp[0].node),NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);free((yyvsp[-1].str));}
#line 1839 "parser.tab.c"
    break;

  case 30: /* compound_statement: IF expr suite  */
#line 86 "frontend/parser.y"
                 {(yyval.node)=node_pair("If",NULL,(yyvsp[-1].node),(yyvsp[0].node),(yylsp[-2]).first_line,(yylsp[-2]).first_column);}
#line 1845 "parser.tab.c"
    break;

  case 31: /* compound_statement: IF expr suite ELSE suite  */
#line 87 "frontend/parser.y"
                            {(yyval.node)=node_pair("IfElse",NULL,node_pair("Condition",NULL,(yyvsp[-3].node),(yyvsp[-2].node),(yylsp[-4]).first_line,(yylsp[-4]).first_column),(yyvsp[0].node),(yylsp[-4]).first_line,(yylsp[-4]).first_column);}
#line 1851 "parser.tab.c"
    break;

  case 32: /* compound_statement: IF expr suite ELSEIF expr suite  */
#line 88 "frontend/parser.y"
                                   {(yyval.node)=node_pair("IfElseIf",NULL,node_pair("Condition",NULL,(yyvsp[-4].node),(yyvsp[-3].node),(yylsp[-5]).first_line,(yylsp[-5]).first_column),node_pair("ElseIf",NULL,(yyvsp[-1].node),(yyvsp[0].node),(yylsp[-5]).first_line,(yylsp[-5]).first_column),(yylsp[-5]).first_line,(yylsp[-5]).first_column);}
#line 1857 "parser.tab.c"
    break;

  case 33: /* compound_statement: FOR IDENTIFIER IN expr suite  */
#line 89 "frontend/parser.y"
                                {(yyval.node)=node_pair("For",(yyvsp[-3].str),(yyvsp[-1].node),(yyvsp[0].node),(yylsp[-4]).first_line,(yylsp[-4]).first_column);free((yyvsp[-3].str));}
#line 1863 "parser.tab.c"
    break;

  case 34: /* suite: NEWLINE INDENT lines DEDENT  */
#line 91 "frontend/parser.y"
                                   {(yyval.node)=as_block((yyvsp[-1].node),(yylsp[-3]).first_line,(yylsp[-3]).first_column);}
#line 1869 "parser.tab.c"
    break;

  case 35: /* suite: LBRACE lines RBRACE  */
#line 92 "frontend/parser.y"
                       {(yyval.node)=as_block((yyvsp[-1].node),(yylsp[-2]).first_line,(yylsp[-2]).first_column);}
#line 1875 "parser.tab.c"
    break;

  case 36: /* expr: primary  */
#line 95 "frontend/parser.y"
         {(yyval.node)=(yyvsp[0].node);}
#line 1881 "parser.tab.c"
    break;

  case 37: /* expr: expr PLUS expr  */
#line 96 "frontend/parser.y"
                  {(yyval.node)=node_pair("Binary","+", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1887 "parser.tab.c"
    break;

  case 38: /* expr: expr MINUS expr  */
#line 97 "frontend/parser.y"
                   {(yyval.node)=node_pair("Binary","-", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1893 "parser.tab.c"
    break;

  case 39: /* expr: expr STAR expr  */
#line 98 "frontend/parser.y"
                  {(yyval.node)=node_pair("Binary","*", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1899 "parser.tab.c"
    break;

  case 40: /* expr: expr SLASH expr  */
#line 99 "frontend/parser.y"
                   {(yyval.node)=node_pair("Binary","/", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1905 "parser.tab.c"
    break;

  case 41: /* expr: expr IDIV expr  */
#line 100 "frontend/parser.y"
                  {(yyval.node)=node_pair("Binary","//", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1911 "parser.tab.c"
    break;

  case 42: /* expr: expr PERCENT expr  */
#line 101 "frontend/parser.y"
                     {(yyval.node)=node_pair("Binary","%", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1917 "parser.tab.c"
    break;

  case 43: /* expr: expr POWER expr  */
#line 102 "frontend/parser.y"
                   {(yyval.node)=node_pair("Binary","**", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1923 "parser.tab.c"
    break;

  case 44: /* expr: expr EQ expr  */
#line 103 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary","==", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1929 "parser.tab.c"
    break;

  case 45: /* expr: expr NE expr  */
#line 104 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary","!=", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1935 "parser.tab.c"
    break;

  case 46: /* expr: expr LT expr  */
#line 105 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary","<", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1941 "parser.tab.c"
    break;

  case 47: /* expr: expr LE expr  */
#line 106 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary","<=", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1947 "parser.tab.c"
    break;

  case 48: /* expr: expr GT expr  */
#line 107 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary",">", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1953 "parser.tab.c"
    break;

  case 49: /* expr: expr GE expr  */
#line 108 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary",">=", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1959 "parser.tab.c"
    break;

  case 50: /* expr: expr AND expr  */
#line 109 "frontend/parser.y"
                 {(yyval.node)=node_pair("Binary","and", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1965 "parser.tab.c"
    break;

  case 51: /* expr: expr OR expr  */
#line 110 "frontend/parser.y"
                {(yyval.node)=node_pair("Binary","or", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1971 "parser.tab.c"
    break;

  case 52: /* expr: expr PIPE expr  */
#line 111 "frontend/parser.y"
                  {(yyval.node)=node_pair("Flow","|>", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1977 "parser.tab.c"
    break;

  case 53: /* expr: expr APPLY expr  */
#line 112 "frontend/parser.y"
                   {(yyval.node)=node_pair("Flow","<|", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1983 "parser.tab.c"
    break;

  case 54: /* expr: expr FANOUT expr  */
#line 113 "frontend/parser.y"
                    {(yyval.node)=node_pair("Flow",">|", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1989 "parser.tab.c"
    break;

  case 55: /* expr: expr JOIN expr  */
#line 114 "frontend/parser.y"
                  {(yyval.node)=node_pair("Flow","|<", (yyvsp[-2].node),(yyvsp[0].node),(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 1995 "parser.tab.c"
    break;

  case 56: /* expr: MINUS expr  */
#line 115 "frontend/parser.y"
                           {(yyval.node)=node_pair("Unary","-",(yyvsp[0].node),NULL,(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 2001 "parser.tab.c"
    break;

  case 57: /* expr: NOT expr  */
#line 116 "frontend/parser.y"
            {(yyval.node)=node_pair("Unary","not",(yyvsp[0].node),NULL,(yylsp[-1]).first_line,(yylsp[-1]).first_column);}
#line 2007 "parser.tab.c"
    break;

  case 58: /* expr: expr DOT IDENTIFIER  */
#line 117 "frontend/parser.y"
                       {(yyval.node)=node_pair("Member",(yyvsp[0].str),(yyvsp[-2].node),NULL,(yylsp[-1]).first_line,(yylsp[-1]).first_column);free((yyvsp[0].str));}
#line 2013 "parser.tab.c"
    break;

  case 59: /* expr: expr LBRACKET expr RBRACKET  */
#line 118 "frontend/parser.y"
                               {(yyval.node)=node_pair("Index",NULL,(yyvsp[-3].node),(yyvsp[-1].node),(yylsp[-2]).first_line,(yylsp[-2]).first_column);}
#line 2019 "parser.tab.c"
    break;

  case 60: /* primary: IDENTIFIER  */
#line 121 "frontend/parser.y"
            {(yyval.node)=node_new("Name",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2025 "parser.tab.c"
    break;

  case 61: /* primary: STRING  */
#line 122 "frontend/parser.y"
          {(yyval.node)=node_new("String",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2031 "parser.tab.c"
    break;

  case 62: /* primary: INTEGER  */
#line 123 "frontend/parser.y"
           {(yyval.node)=node_new("Integer",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2037 "parser.tab.c"
    break;

  case 63: /* primary: FLOAT  */
#line 124 "frontend/parser.y"
         {(yyval.node)=node_new("Float",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2043 "parser.tab.c"
    break;

  case 64: /* primary: TYPE  */
#line 125 "frontend/parser.y"
        {(yyval.node)=node_new("Name",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2049 "parser.tab.c"
    break;

  case 65: /* primary: TRUE  */
#line 126 "frontend/parser.y"
        {(yyval.node)=node_new("Boolean","true",(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 2055 "parser.tab.c"
    break;

  case 66: /* primary: FALSE  */
#line 127 "frontend/parser.y"
         {(yyval.node)=node_new("Boolean","false",(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 2061 "parser.tab.c"
    break;

  case 67: /* primary: NULLVAL  */
#line 128 "frontend/parser.y"
           {(yyval.node)=node_new("Null",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column);}
#line 2067 "parser.tab.c"
    break;

  case 68: /* primary: LPAREN expr RPAREN  */
#line 129 "frontend/parser.y"
                      {(yyval.node)=(yyvsp[-1].node);}
#line 2073 "parser.tab.c"
    break;

  case 69: /* primary: LBRACKET arguments_opt RBRACKET  */
#line 130 "frontend/parser.y"
                                   {(yyval.node)=node_pair("List",NULL,(yyvsp[-1].node),NULL,(yylsp[-2]).first_line,(yylsp[-2]).first_column);}
#line 2079 "parser.tab.c"
    break;

  case 70: /* primary: IDENTIFIER LPAREN arguments_opt RPAREN  */
#line 131 "frontend/parser.y"
                                          {(yyval.node)=node_pair("Call",(yyvsp[-3].str),(yyvsp[-1].node),NULL,(yylsp[-3]).first_line,(yylsp[-3]).first_column);free((yyvsp[-3].str));}
#line 2085 "parser.tab.c"
    break;

  case 71: /* primary: TYPE LPAREN arguments_opt RPAREN  */
#line 132 "frontend/parser.y"
                                    {(yyval.node)=node_pair("Call",(yyvsp[-3].str),(yyvsp[-1].node),NULL,(yylsp[-3]).first_line,(yylsp[-3]).first_column);free((yyvsp[-3].str));}
#line 2091 "parser.tab.c"
    break;

  case 72: /* parameters_opt: %empty  */
#line 134 "frontend/parser.y"
                       {(yyval.node)=node_new("Parameters",NULL,(yyloc).first_line,(yyloc).first_column);}
#line 2097 "parser.tab.c"
    break;

  case 73: /* parameters_opt: parameters  */
#line 134 "frontend/parser.y"
                                                                                                    {(yyval.node)=(yyvsp[0].node);}
#line 2103 "parser.tab.c"
    break;

  case 74: /* parameters: IDENTIFIER  */
#line 135 "frontend/parser.y"
                       {(yyval.node)=node_add(node_new("Parameters",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column),node_new("Name",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column));free((yyvsp[0].str));}
#line 2109 "parser.tab.c"
    break;

  case 75: /* parameters: parameters COMMA IDENTIFIER  */
#line 135 "frontend/parser.y"
                                                                                                                                                                                          {(yyval.node)=node_add((yyvsp[-2].node),node_new("Name",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column));free((yyvsp[0].str));}
#line 2115 "parser.tab.c"
    break;

  case 76: /* arguments_opt: %empty  */
#line 136 "frontend/parser.y"
                      {(yyval.node)=node_new("Arguments",NULL,(yyloc).first_line,(yyloc).first_column);}
#line 2121 "parser.tab.c"
    break;

  case 77: /* arguments_opt: arguments  */
#line 136 "frontend/parser.y"
                                                                                                 {(yyval.node)=(yyvsp[0].node);}
#line 2127 "parser.tab.c"
    break;

  case 78: /* arguments: expr  */
#line 137 "frontend/parser.y"
                {(yyval.node)=node_add(node_new("Arguments",NULL,(yylsp[0]).first_line,(yylsp[0]).first_column),(yyvsp[0].node));}
#line 2133 "parser.tab.c"
    break;

  case 79: /* arguments: arguments COMMA expr  */
#line 137 "frontend/parser.y"
                                                                                                                   {(yyval.node)=node_add((yyvsp[-2].node),(yyvsp[0].node));}
#line 2139 "parser.tab.c"
    break;

  case 80: /* type_expr: TYPE  */
#line 138 "frontend/parser.y"
                {(yyval.node)=node_new("Type",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2145 "parser.tab.c"
    break;

  case 81: /* type_expr: IDENTIFIER  */
#line 138 "frontend/parser.y"
                                                                                              {(yyval.node)=node_new("Type",(yyvsp[0].str),(yylsp[0]).first_line,(yylsp[0]).first_column);free((yyvsp[0].str));}
#line 2151 "parser.tab.c"
    break;


#line 2155 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 139 "frontend/parser.y"

void yyerror(const char*s){fprintf(stderr,"Pifari:%d:%d: erro sintático: %s\n",yylloc.first_line,yylloc.first_column,s);}
int main(int argc,char**argv){if(argc!=2){fprintf(stderr,"Uso: %s arquivo.pif\n",argv[0]);return 2;}yyin=fopen(argv[1],"r");if(!yyin){perror(argv[1]);return 2;}int parsed=yyparse();int status=1;if(parsed==0&&pifari_root)status=runtime_execute(pifari_root);node_free(pifari_root);fclose(yyin);return status;}
