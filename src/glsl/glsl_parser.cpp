/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         _mesa_glsl_parse
#define yylex           _mesa_glsl_lex
#define yyerror         _mesa_glsl_error
#define yylval          _mesa_glsl_lval
#define yychar          _mesa_glsl_char
#define yydebug         _mesa_glsl_debug
#define yynerrs         _mesa_glsl_nerrs
#define yylloc          _mesa_glsl_lloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "glsl_parser.ypp"

/*
 * Copyright © 2008, 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
    
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"

#define YYLEX_PARAM state->scanner



/* Line 189 of yacc.c  */
#line 117 "glsl_parser.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ATTRIBUTE = 258,
     CONST_TOK = 259,
     BOOL_TOK = 260,
     FLOAT_TOK = 261,
     INT_TOK = 262,
     UINT_TOK = 263,
     BREAK = 264,
     CONTINUE = 265,
     DO = 266,
     ELSE = 267,
     FOR = 268,
     IF = 269,
     DISCARD = 270,
     RETURN = 271,
     SWITCH = 272,
     CASE = 273,
     DEFAULT = 274,
     BVEC2 = 275,
     BVEC3 = 276,
     BVEC4 = 277,
     IVEC2 = 278,
     IVEC3 = 279,
     IVEC4 = 280,
     UVEC2 = 281,
     UVEC3 = 282,
     UVEC4 = 283,
     VEC2 = 284,
     VEC3 = 285,
     VEC4 = 286,
     CENTROID = 287,
     IN_TOK = 288,
     OUT_TOK = 289,
     INOUT_TOK = 290,
     UNIFORM = 291,
     VARYING = 292,
     NOPERSPECTIVE = 293,
     FLAT = 294,
     SMOOTH = 295,
     MAT2X2 = 296,
     MAT2X3 = 297,
     MAT2X4 = 298,
     MAT3X2 = 299,
     MAT3X3 = 300,
     MAT3X4 = 301,
     MAT4X2 = 302,
     MAT4X3 = 303,
     MAT4X4 = 304,
     SAMPLER1D = 305,
     SAMPLER2D = 306,
     SAMPLER3D = 307,
     SAMPLERCUBE = 308,
     SAMPLER1DSHADOW = 309,
     SAMPLER2DSHADOW = 310,
     SAMPLERCUBESHADOW = 311,
     SAMPLER1DARRAY = 312,
     SAMPLER2DARRAY = 313,
     SAMPLER1DARRAYSHADOW = 314,
     SAMPLER2DARRAYSHADOW = 315,
     ISAMPLER1D = 316,
     ISAMPLER2D = 317,
     ISAMPLER3D = 318,
     ISAMPLERCUBE = 319,
     ISAMPLER1DARRAY = 320,
     ISAMPLER2DARRAY = 321,
     USAMPLER1D = 322,
     USAMPLER2D = 323,
     USAMPLER3D = 324,
     USAMPLERCUBE = 325,
     USAMPLER1DARRAY = 326,
     USAMPLER2DARRAY = 327,
     STRUCT = 328,
     VOID_TOK = 329,
     WHILE = 330,
     IDENTIFIER = 331,
     FLOATCONSTANT = 332,
     INTCONSTANT = 333,
     UINTCONSTANT = 334,
     BOOLCONSTANT = 335,
     FIELD_SELECTION = 336,
     LEFT_OP = 337,
     RIGHT_OP = 338,
     INC_OP = 339,
     DEC_OP = 340,
     LE_OP = 341,
     GE_OP = 342,
     EQ_OP = 343,
     NE_OP = 344,
     AND_OP = 345,
     OR_OP = 346,
     XOR_OP = 347,
     MUL_ASSIGN = 348,
     DIV_ASSIGN = 349,
     ADD_ASSIGN = 350,
     MOD_ASSIGN = 351,
     LEFT_ASSIGN = 352,
     RIGHT_ASSIGN = 353,
     AND_ASSIGN = 354,
     XOR_ASSIGN = 355,
     OR_ASSIGN = 356,
     SUB_ASSIGN = 357,
     INVARIANT = 358,
     LOWP = 359,
     MEDIUMP = 360,
     HIGHP = 361,
     SUPERP = 362,
     PRECISION = 363,
     VERSION = 364,
     EXTENSION = 365,
     LINE = 366,
     COLON = 367,
     EOL = 368,
     INTERFACE = 369,
     OUTPUT = 370,
     PRAGMA_DEBUG_ON = 371,
     PRAGMA_DEBUG_OFF = 372,
     PRAGMA_OPTIMIZE_ON = 373,
     PRAGMA_OPTIMIZE_OFF = 374,
     LAYOUT_TOK = 375,
     ASM = 376,
     CLASS = 377,
     UNION = 378,
     ENUM = 379,
     TYPEDEF = 380,
     TEMPLATE = 381,
     THIS = 382,
     PACKED_TOK = 383,
     GOTO = 384,
     INLINE_TOK = 385,
     NOINLINE = 386,
     VOLATILE = 387,
     PUBLIC_TOK = 388,
     STATIC = 389,
     EXTERN = 390,
     EXTERNAL = 391,
     LONG_TOK = 392,
     SHORT_TOK = 393,
     DOUBLE_TOK = 394,
     HALF = 395,
     FIXED_TOK = 396,
     UNSIGNED = 397,
     INPUT_TOK = 398,
     OUPTUT = 399,
     HVEC2 = 400,
     HVEC3 = 401,
     HVEC4 = 402,
     DVEC2 = 403,
     DVEC3 = 404,
     DVEC4 = 405,
     FVEC2 = 406,
     FVEC3 = 407,
     FVEC4 = 408,
     SAMPLER2DRECT = 409,
     SAMPLER3DRECT = 410,
     SAMPLER2DRECTSHADOW = 411,
     SIZEOF = 412,
     CAST = 413,
     NAMESPACE = 414,
     USING = 415,
     ERROR_TOK = 416,
     COMMON = 417,
     PARTITION = 418,
     ACTIVE = 419,
     SAMPLERBUFFER = 420,
     FILTER = 421,
     IMAGE1D = 422,
     IMAGE2D = 423,
     IMAGE3D = 424,
     IMAGECUBE = 425,
     IMAGE1DARRAY = 426,
     IMAGE2DARRAY = 427,
     IIMAGE1D = 428,
     IIMAGE2D = 429,
     IIMAGE3D = 430,
     IIMAGECUBE = 431,
     IIMAGE1DARRAY = 432,
     IIMAGE2DARRAY = 433,
     UIMAGE1D = 434,
     UIMAGE2D = 435,
     UIMAGE3D = 436,
     UIMAGECUBE = 437,
     UIMAGE1DARRAY = 438,
     UIMAGE2DARRAY = 439,
     IMAGE1DSHADOW = 440,
     IMAGE2DSHADOW = 441,
     IMAGEBUFFER = 442,
     IIMAGEBUFFER = 443,
     UIMAGEBUFFER = 444,
     IMAGE1DARRAYSHADOW = 445,
     IMAGE2DARRAYSHADOW = 446,
     ROW_MAJOR = 447
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 52 "glsl_parser.ypp"

   int n;
   float real;
   char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;



/* Line 214 of yacc.c  */
#line 377 "glsl_parser.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 402 "glsl_parser.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3718

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  217
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  87
/* YYNRULES -- Number of rules.  */
#define YYNRULES  278
/* YYNRULES -- Number of states.  */
#define YYNSTATES  413

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   447

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   201,     2,     2,     2,   205,   208,     2,
     193,   194,   203,   199,   198,   200,   197,   204,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   212,   214,
     206,   213,   207,   211,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   195,     2,   196,   209,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   215,   210,   216,   202,     2,     2,     2,
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    17,    20,    23,
      26,    27,    30,    36,    38,    41,    43,    45,    47,    49,
      51,    53,    57,    59,    64,    66,    70,    73,    76,    78,
      80,    82,    86,    89,    92,    95,    97,   100,   104,   107,
     109,   111,   113,   115,   118,   121,   124,   126,   128,   130,
     132,   134,   138,   142,   146,   148,   152,   156,   158,   162,
     166,   168,   172,   176,   180,   184,   186,   190,   194,   196,
     200,   202,   206,   208,   212,   214,   218,   220,   224,   226,
     230,   232,   238,   240,   244,   246,   248,   250,   252,   254,
     256,   258,   260,   262,   264,   266,   268,   272,   274,   277,
     280,   285,   288,   290,   292,   295,   299,   303,   306,   312,
     316,   319,   323,   326,   327,   329,   331,   333,   335,   337,
     341,   347,   354,   362,   371,   377,   379,   382,   387,   393,
     400,   408,   413,   416,   418,   421,   426,   428,   432,   434,
     438,   440,   442,   444,   446,   448,   450,   453,   455,   458,
     461,   465,   467,   469,   471,   473,   476,   478,   480,   483,
     486,   488,   490,   493,   495,   499,   504,   506,   508,   510,
     512,   514,   516,   518,   520,   522,   524,   526,   528,   530,
     532,   534,   536,   538,   540,   542,   544,   546,   548,   550,
     552,   554,   556,   558,   560,   562,   564,   566,   568,   570,
     572,   574,   576,   578,   580,   582,   584,   586,   588,   590,
     592,   594,   596,   598,   600,   602,   604,   606,   608,   610,
     612,   614,   616,   618,   624,   629,   631,   634,   638,   640,
     644,   646,   651,   653,   655,   657,   659,   661,   663,   665,
     667,   669,   671,   673,   676,   680,   682,   684,   687,   691,
     693,   696,   698,   701,   707,   711,   713,   715,   720,   726,
     730,   733,   739,   747,   754,   756,   758,   760,   761,   764,
     768,   771,   774,   777,   781,   784,   786,   788,   790
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     218,     0,    -1,    -1,   220,   222,   219,   224,    -1,    -1,
     109,    78,   113,    -1,   116,   113,    -1,   117,   113,    -1,
     118,   113,    -1,   119,   113,    -1,    -1,   222,   223,    -1,
     110,    76,   112,    76,   113,    -1,   302,    -1,   224,   302,
      -1,    76,    -1,   225,    -1,    78,    -1,    79,    -1,    77,
      -1,    80,    -1,   193,   252,   194,    -1,   226,    -1,   227,
     195,   228,   196,    -1,   229,    -1,   227,   197,    76,    -1,
     227,    84,    -1,   227,    85,    -1,   252,    -1,   230,    -1,
     231,    -1,   227,   197,   231,    -1,   233,   194,    -1,   232,
     194,    -1,   234,    74,    -1,   234,    -1,   234,   250,    -1,
     233,   198,   250,    -1,   235,   193,    -1,   273,    -1,    76,
      -1,    81,    -1,   227,    -1,    84,   236,    -1,    85,   236,
      -1,   237,   236,    -1,   199,    -1,   200,    -1,   201,    -1,
     202,    -1,   236,    -1,   238,   203,   236,    -1,   238,   204,
     236,    -1,   238,   205,   236,    -1,   238,    -1,   239,   199,
     238,    -1,   239,   200,   238,    -1,   239,    -1,   240,    82,
     239,    -1,   240,    83,   239,    -1,   240,    -1,   241,   206,
     240,    -1,   241,   207,   240,    -1,   241,    86,   240,    -1,
     241,    87,   240,    -1,   241,    -1,   242,    88,   241,    -1,
     242,    89,   241,    -1,   242,    -1,   243,   208,   242,    -1,
     243,    -1,   244,   209,   243,    -1,   244,    -1,   245,   210,
     244,    -1,   245,    -1,   246,    90,   245,    -1,   246,    -1,
     247,    92,   246,    -1,   247,    -1,   248,    91,   247,    -1,
     248,    -1,   248,   211,   252,   212,   250,    -1,   249,    -1,
     236,   251,   250,    -1,   213,    -1,    93,    -1,    94,    -1,
      96,    -1,    95,    -1,   102,    -1,    97,    -1,    98,    -1,
      99,    -1,   100,    -1,   101,    -1,   250,    -1,   252,   198,
     250,    -1,   249,    -1,   255,   214,    -1,   263,   214,    -1,
     108,   277,   274,   214,    -1,   256,   194,    -1,   258,    -1,
     257,    -1,   258,   260,    -1,   257,   198,   260,    -1,   265,
      76,   193,    -1,   273,    76,    -1,   273,    76,   195,   253,
     196,    -1,   270,   261,   259,    -1,   261,   259,    -1,   270,
     261,   262,    -1,   261,   262,    -1,    -1,    33,    -1,    34,
      -1,    35,    -1,   273,    -1,   264,    -1,   263,   198,    76,
      -1,   263,   198,    76,   195,   196,    -1,   263,   198,    76,
     195,   253,   196,    -1,   263,   198,    76,   195,   196,   213,
     283,    -1,   263,   198,    76,   195,   253,   196,   213,   283,
      -1,   263,   198,    76,   213,   283,    -1,   265,    -1,   265,
      76,    -1,   265,    76,   195,   196,    -1,   265,    76,   195,
     253,   196,    -1,   265,    76,   195,   196,   213,   283,    -1,
     265,    76,   195,   253,   196,   213,   283,    -1,   265,    76,
     213,   283,    -1,   103,    76,    -1,   273,    -1,   271,   273,
      -1,   120,   193,   267,   194,    -1,   268,    -1,   267,   198,
     268,    -1,    76,    -1,    76,   213,    78,    -1,    40,    -1,
      39,    -1,    38,    -1,     4,    -1,   272,    -1,   266,    -1,
     266,   272,    -1,   269,    -1,   269,   272,    -1,   103,   272,
      -1,   103,   269,   272,    -1,   103,    -1,     4,    -1,     3,
      -1,    37,    -1,    32,    37,    -1,    33,    -1,    34,    -1,
      32,    33,    -1,    32,    34,    -1,    36,    -1,   274,    -1,
     277,   274,    -1,   275,    -1,   275,   195,   196,    -1,   275,
     195,   253,   196,    -1,   276,    -1,   278,    -1,    76,    -1,
      74,    -1,     6,    -1,     7,    -1,     8,    -1,     5,    -1,
      29,    -1,    30,    -1,    31,    -1,    20,    -1,    21,    -1,
      22,    -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,
      27,    -1,    28,    -1,    41,    -1,    42,    -1,    43,    -1,
      44,    -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,
      49,    -1,    50,    -1,    51,    -1,   154,    -1,    52,    -1,
      53,    -1,    54,    -1,    55,    -1,   156,    -1,    56,    -1,
      57,    -1,    58,    -1,    59,    -1,    60,    -1,    61,    -1,
      62,    -1,    63,    -1,    64,    -1,    65,    -1,    66,    -1,
      67,    -1,    68,    -1,    69,    -1,    70,    -1,    71,    -1,
      72,    -1,   106,    -1,   105,    -1,   104,    -1,    73,    76,
     215,   279,   216,    -1,    73,   215,   279,   216,    -1,   280,
      -1,   279,   280,    -1,   273,   281,   214,    -1,   282,    -1,
     281,   198,   282,    -1,    76,    -1,    76,   195,   253,   196,
      -1,   250,    -1,   254,    -1,   287,    -1,   286,    -1,   284,
      -1,   291,    -1,   292,    -1,   295,    -1,   296,    -1,   297,
      -1,   301,    -1,   215,   216,    -1,   215,   290,   216,    -1,
     289,    -1,   286,    -1,   215,   216,    -1,   215,   290,   216,
      -1,   285,    -1,   290,   285,    -1,   214,    -1,   252,   214,
      -1,    14,   193,   252,   194,   293,    -1,   285,    12,   285,
      -1,   285,    -1,   252,    -1,   265,    76,   213,   283,    -1,
      17,   193,   252,   194,   287,    -1,    18,   252,   212,    -1,
      19,   212,    -1,    75,   193,   294,   194,   288,    -1,    11,
     285,    75,   193,   252,   194,   214,    -1,    13,   193,   298,
     300,   194,   288,    -1,   291,    -1,   284,    -1,   294,    -1,
      -1,   299,   214,    -1,   299,   214,   252,    -1,    10,   214,
      -1,     9,   214,    -1,    16,   214,    -1,    16,   252,   214,
      -1,    15,   214,    -1,   303,    -1,   254,    -1,   221,    -1,
     255,   289,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   212,   212,   211,   218,   220,   240,   241,   242,   243,
     246,   248,   252,   261,   269,   280,   284,   291,   298,   305,
     312,   319,   326,   327,   333,   337,   344,   350,   359,   363,
     367,   368,   377,   378,   382,   383,   387,   393,   405,   409,
     415,   422,   433,   434,   440,   446,   456,   457,   458,   459,
     463,   464,   470,   476,   485,   486,   492,   501,   502,   508,
     517,   518,   524,   530,   536,   545,   546,   552,   561,   562,
     571,   572,   581,   582,   591,   592,   601,   602,   611,   612,
     621,   622,   631,   632,   641,   642,   643,   644,   645,   646,
     647,   648,   649,   650,   651,   655,   659,   675,   679,   683,
     687,   701,   705,   706,   710,   715,   723,   734,   744,   759,
     766,   771,   782,   795,   798,   803,   808,   817,   821,   822,
     831,   840,   849,   858,   867,   880,   891,   900,   909,   918,
     927,   936,   945,   959,   966,   977,   984,   985,  1004,  1033,
    1074,  1079,  1084,  1092,  1100,  1101,  1102,  1107,  1108,  1113,
    1118,  1124,  1132,  1137,  1142,  1147,  1153,  1158,  1163,  1168,
    1173,  1181,  1182,  1190,  1191,  1197,  1206,  1212,  1218,  1227,
    1228,  1229,  1230,  1231,  1232,  1233,  1234,  1235,  1236,  1237,
    1238,  1239,  1240,  1241,  1242,  1243,  1244,  1245,  1246,  1247,
    1248,  1249,  1250,  1251,  1252,  1253,  1254,  1255,  1256,  1257,
    1258,  1259,  1260,  1261,  1262,  1263,  1264,  1265,  1266,  1267,
    1268,  1269,  1270,  1271,  1272,  1273,  1274,  1275,  1276,  1277,
    1281,  1292,  1303,  1317,  1323,  1332,  1337,  1345,  1360,  1365,
    1373,  1379,  1388,  1392,  1398,  1399,  1403,  1404,  1405,  1406,
    1407,  1408,  1409,  1413,  1419,  1428,  1429,  1433,  1439,  1448,
    1458,  1470,  1476,  1485,  1494,  1499,  1507,  1511,  1525,  1529,
    1530,  1534,  1541,  1548,  1558,  1559,  1563,  1565,  1571,  1576,
    1585,  1591,  1597,  1603,  1609,  1618,  1619,  1620,  1624
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ATTRIBUTE", "CONST_TOK", "BOOL_TOK",
  "FLOAT_TOK", "INT_TOK", "UINT_TOK", "BREAK", "CONTINUE", "DO", "ELSE",
  "FOR", "IF", "DISCARD", "RETURN", "SWITCH", "CASE", "DEFAULT", "BVEC2",
  "BVEC3", "BVEC4", "IVEC2", "IVEC3", "IVEC4", "UVEC2", "UVEC3", "UVEC4",
  "VEC2", "VEC3", "VEC4", "CENTROID", "IN_TOK", "OUT_TOK", "INOUT_TOK",
  "UNIFORM", "VARYING", "NOPERSPECTIVE", "FLAT", "SMOOTH", "MAT2X2",
  "MAT2X3", "MAT2X4", "MAT3X2", "MAT3X3", "MAT3X4", "MAT4X2", "MAT4X3",
  "MAT4X4", "SAMPLER1D", "SAMPLER2D", "SAMPLER3D", "SAMPLERCUBE",
  "SAMPLER1DSHADOW", "SAMPLER2DSHADOW", "SAMPLERCUBESHADOW",
  "SAMPLER1DARRAY", "SAMPLER2DARRAY", "SAMPLER1DARRAYSHADOW",
  "SAMPLER2DARRAYSHADOW", "ISAMPLER1D", "ISAMPLER2D", "ISAMPLER3D",
  "ISAMPLERCUBE", "ISAMPLER1DARRAY", "ISAMPLER2DARRAY", "USAMPLER1D",
  "USAMPLER2D", "USAMPLER3D", "USAMPLERCUBE", "USAMPLER1DARRAY",
  "USAMPLER2DARRAY", "STRUCT", "VOID_TOK", "WHILE", "IDENTIFIER",
  "FLOATCONSTANT", "INTCONSTANT", "UINTCONSTANT", "BOOLCONSTANT",
  "FIELD_SELECTION", "LEFT_OP", "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP",
  "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN",
  "DIV_ASSIGN", "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN",
  "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT",
  "LOWP", "MEDIUMP", "HIGHP", "SUPERP", "PRECISION", "VERSION",
  "EXTENSION", "LINE", "COLON", "EOL", "INTERFACE", "OUTPUT",
  "PRAGMA_DEBUG_ON", "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON",
  "PRAGMA_OPTIMIZE_OFF", "LAYOUT_TOK", "ASM", "CLASS", "UNION", "ENUM",
  "TYPEDEF", "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK",
  "NOINLINE", "VOLATILE", "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL",
  "LONG_TOK", "SHORT_TOK", "DOUBLE_TOK", "HALF", "FIXED_TOK", "UNSIGNED",
  "INPUT_TOK", "OUPTUT", "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3",
  "DVEC4", "FVEC2", "FVEC3", "FVEC4", "SAMPLER2DRECT", "SAMPLER3DRECT",
  "SAMPLER2DRECTSHADOW", "SIZEOF", "CAST", "NAMESPACE", "USING",
  "ERROR_TOK", "COMMON", "PARTITION", "ACTIVE", "SAMPLERBUFFER", "FILTER",
  "IMAGE1D", "IMAGE2D", "IMAGE3D", "IMAGECUBE", "IMAGE1DARRAY",
  "IMAGE2DARRAY", "IIMAGE1D", "IIMAGE2D", "IIMAGE3D", "IIMAGECUBE",
  "IIMAGE1DARRAY", "IIMAGE2DARRAY", "UIMAGE1D", "UIMAGE2D", "UIMAGE3D",
  "UIMAGECUBE", "UIMAGE1DARRAY", "UIMAGE2DARRAY", "IMAGE1DSHADOW",
  "IMAGE2DSHADOW", "IMAGEBUFFER", "IIMAGEBUFFER", "UIMAGEBUFFER",
  "IMAGE1DARRAYSHADOW", "IMAGE2DARRAYSHADOW", "ROW_MAJOR", "'('", "')'",
  "'['", "']'", "'.'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'",
  "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'",
  "'{'", "'}'", "$accept", "translation_unit", "$@1", "version_statement",
  "pragma_statement", "extension_statement_list", "extension_statement",
  "external_declaration_list", "variable_identifier", "primary_expression",
  "postfix_expression", "integer_expression", "function_call",
  "function_call_or_method", "function_call_generic",
  "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "unary_expression", "unary_operator",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_type_specifier",
  "init_declarator_list", "single_declaration", "fully_specified_type",
  "layout_qualifier", "layout_qualifier_id_list", "layout_qualifier_id",
  "interpolation_qualifier", "parameter_type_qualifier", "type_qualifier",
  "storage_qualifier", "type_specifier", "type_specifier_no_prec",
  "type_specifier_nonarray", "basic_type_specifier_nonarray",
  "precision_qualifier", "struct_specifier", "struct_declaration_list",
  "struct_declaration", "struct_declarator_list", "struct_declarator",
  "initializer", "declaration_statement", "statement", "simple_statement",
  "compound_statement", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "case_label", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "external_declaration", "function_definition", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,    40,    41,    91,    93,    46,    44,    43,
      45,    33,   126,    42,    47,    37,    60,    62,    38,    94,
     124,    63,    58,    61,    59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   217,   219,   218,   220,   220,   221,   221,   221,   221,
     222,   222,   223,   224,   224,   225,   226,   226,   226,   226,
     226,   226,   227,   227,   227,   227,   227,   227,   228,   229,
     230,   230,   231,   231,   232,   232,   233,   233,   234,   235,
     235,   235,   236,   236,   236,   236,   237,   237,   237,   237,
     238,   238,   238,   238,   239,   239,   239,   240,   240,   240,
     241,   241,   241,   241,   241,   242,   242,   242,   243,   243,
     244,   244,   245,   245,   246,   246,   247,   247,   248,   248,
     249,   249,   250,   250,   251,   251,   251,   251,   251,   251,
     251,   251,   251,   251,   251,   252,   252,   253,   254,   254,
     254,   255,   256,   256,   257,   257,   258,   259,   259,   260,
     260,   260,   260,   261,   261,   261,   261,   262,   263,   263,
     263,   263,   263,   263,   263,   264,   264,   264,   264,   264,
     264,   264,   264,   265,   265,   266,   267,   267,   268,   268,
     269,   269,   269,   270,   271,   271,   271,   271,   271,   271,
     271,   271,   272,   272,   272,   272,   272,   272,   272,   272,
     272,   273,   273,   274,   274,   274,   275,   275,   275,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     276,   276,   276,   276,   276,   276,   276,   276,   276,   276,
     277,   277,   277,   278,   278,   279,   279,   280,   281,   281,
     282,   282,   283,   284,   285,   285,   286,   286,   286,   286,
     286,   286,   286,   287,   287,   288,   288,   289,   289,   290,
     290,   291,   291,   292,   293,   293,   294,   294,   295,   296,
     296,   297,   297,   297,   298,   298,   299,   299,   300,   300,
     301,   301,   301,   301,   301,   302,   302,   302,   303
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     2,     2,     2,     2,
       0,     2,     5,     1,     2,     1,     1,     1,     1,     1,
       1,     3,     1,     4,     1,     3,     2,     2,     1,     1,
       1,     3,     2,     2,     2,     1,     2,     3,     2,     1,
       1,     1,     1,     2,     2,     2,     1,     1,     1,     1,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     5,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     2,     2,
       4,     2,     1,     1,     2,     3,     3,     2,     5,     3,
       2,     3,     2,     0,     1,     1,     1,     1,     1,     3,
       5,     6,     7,     8,     5,     1,     2,     4,     5,     6,
       7,     4,     2,     1,     2,     4,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     2,     1,     2,     2,
       3,     1,     1,     1,     1,     2,     1,     1,     2,     2,
       1,     1,     2,     1,     3,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     5,     4,     1,     2,     3,     1,     3,
       1,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     1,     1,     2,     3,     1,
       2,     1,     2,     5,     3,     1,     1,     4,     5,     3,
       2,     5,     7,     6,     1,     1,     1,     0,     2,     3,
       2,     2,     2,     3,     2,     1,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    10,     0,     1,     2,     5,     0,     0,
      11,     0,   153,   152,   173,   170,   171,   172,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   174,   175,   176,
       0,   156,   157,   160,   154,   142,   141,   140,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   198,
     199,   200,   201,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
       0,   169,   168,   151,   222,   221,   220,     0,     0,     0,
       0,     0,     0,   197,   202,   277,     3,   276,     0,     0,
     103,   113,     0,   118,   125,   145,   147,     0,   144,   133,
     161,   163,   166,     0,   167,    13,   275,     0,   158,   159,
     155,     0,     0,   132,     0,   149,     0,     6,     7,     8,
       9,     0,    14,    98,     0,   278,   101,   113,   143,   114,
     115,   116,   104,     0,   113,     0,    99,   126,   146,   148,
     134,     0,   162,     0,     0,     0,     0,   225,   150,     0,
     138,     0,   136,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    15,    19,    17,    18,    20,    41,
       0,     0,     0,    46,    47,    48,    49,   251,     0,   247,
      16,    22,    42,    24,    29,    30,     0,     0,    35,     0,
      50,     0,    54,    57,    60,    65,    68,    70,    72,    74,
      76,    78,    80,    82,    95,     0,   233,     0,   133,   236,
     249,   235,   234,     0,   237,   238,   239,   240,   241,   242,
     105,   110,   112,   117,     0,   119,   106,     0,     0,   164,
      50,    97,     0,    39,    12,     0,   230,     0,   228,   224,
     226,   100,     0,   135,     0,   271,   270,     0,     0,     0,
     274,   272,     0,     0,     0,   260,     0,    43,    44,     0,
     243,     0,    26,    27,     0,     0,    33,    32,     0,   169,
      36,    38,    85,    86,    88,    87,    90,    91,    92,    93,
      94,    89,    84,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   252,   248,   250,   107,
     109,   111,     0,     0,   127,     0,   232,   131,   165,   223,
       0,     0,   227,   139,   137,     0,   265,   264,   267,     0,
     273,     0,   259,   151,   256,     0,     0,    21,   244,     0,
      28,    25,    31,    37,    83,    51,    52,    53,    55,    56,
      58,    59,    63,    64,    61,    62,    66,    67,    69,    71,
      73,    75,    77,    79,     0,    96,     0,   120,     0,   124,
       0,   128,     0,   229,     0,   266,     0,     0,     0,     0,
       0,     0,    23,     0,     0,     0,   121,   129,     0,   231,
       0,   268,     0,   255,   253,   258,     0,   246,   261,   245,
      81,   108,   122,     0,   130,     0,   269,   263,     0,   257,
     123,   262,   254
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     9,     3,    85,     6,    10,    86,   180,   181,
     182,   339,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   283,   205,   232,   206,   207,    89,
      90,    91,   221,   132,   133,   222,    92,    93,    94,    95,
     151,   152,    96,   134,    97,    98,   233,   100,   101,   102,
     103,   104,   146,   147,   237,   238,   317,   209,   210,   211,
     212,   398,   399,   213,   214,   215,   394,   336,   216,   217,
     218,   328,   376,   377,   219,   105,   106
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -350
static const yytype_int16 yypact[] =
{
     -78,   -56,    54,  -350,   -52,  -350,   -37,  -350,     8,  3302,
    -350,   -26,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
      88,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
     -66,  -350,  -350,    42,  -350,  -350,  -350,    70,    -4,    10,
      29,    34,   -79,  -350,  -350,  -350,  3302,  -350,   -19,   -24,
     -69,     5,  -154,  -350,   102,    16,    16,  3490,  -350,  -350,
    -350,    18,  -350,  3562,  -350,  -350,  -350,   108,  -350,  -350,
    -350,    -8,  3490,  -350,    16,  -350,  3562,  -350,  -350,  -350,
    -350,   138,  -350,  -350,   387,  -350,  -350,    24,  -350,  -350,
    -350,  -350,  -350,  3490,   147,   141,  -350,  -166,  -350,  -350,
    -350,  2387,  -350,   106,  3490,   144,  1772,  -350,  -350,     7,
      11,   -87,  -350,    14,    15,  1243,    30,    32,    20,  2004,
      37,  2936,    25,    39,   -65,  -350,  -350,  -350,  -350,  -350,
    2936,  2936,  2936,  -350,  -350,  -350,  -350,  -350,   601,  -350,
    -350,  -350,   -59,  -350,  -350,  -350,    28,   -82,  3119,    43,
     -30,  2936,   -11,    -2,   118,   -74,   114,    35,    31,    36,
     148,   152,   -77,  -350,  -350,  -115,  -350,    40,    52,  -350,
    -350,  -350,  -350,   815,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,   172,  3490,  -180,  -350,  2570,  2936,  -350,
    -350,  -350,    53,  -350,  -350,  1888,    55,  -113,  -350,  -350,
    -350,  -350,   173,  -350,   138,  -350,  -350,   178,  1656,  2936,
    -350,  -350,  -108,  2936,  -161,  -350,  2204,  -350,  -350,   -68,
    -350,  1029,  -350,  -350,  2936,  3418,  -350,  -350,  2936,    61,
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  2936,  -350,  2936,  2936,  2936,  2936,  2936,
    2936,  2936,  2936,  2936,  2936,  2936,  2936,  2936,  2936,  2936,
    2936,  2936,  2936,  2936,  2936,  2936,  -350,  -350,  -350,    63,
    -350,  -350,  2753,  2936,    46,    60,  -350,  -350,  -350,  -350,
    2936,   144,  -350,  -350,  -350,    67,  -350,  -350,  2204,   -55,
    -350,   -54,  -350,   238,    65,   188,    71,  -350,  -350,    72,
      65,    73,  -350,  -350,  -350,  -350,  -350,  -350,   -11,   -11,
      -2,    -2,   118,   118,   118,   118,   -74,   -74,   114,    35,
      31,    36,   148,   152,  -157,  -350,  2936,    56,    83,  -350,
    2936,    68,    84,  -350,  2936,  -350,    69,    90,  1243,    74,
      77,  1456,  -350,  2936,    86,  2936,    79,  -350,  2936,  -350,
     -53,  2936,  1456,   255,  -350,  -350,  2936,  -350,  -350,  -350,
    -350,  -350,  -350,  2936,  -350,    80,    65,  -350,  1243,  -350,
    -350,  -350,  -350
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,  -350,
    -350,  -350,  -350,  -350,    22,  -350,  -350,  -350,  -350,  -135,
    -350,   -83,   -81,  -104,   -85,   -13,    -6,    -5,    -3,    -1,
      -7,  -350,  -133,   -97,  -350,  -156,  -193,     9,    12,  -350,
    -350,  -350,    76,   170,   168,    81,  -350,  -350,  -239,  -350,
    -350,    59,   -71,  -350,  -350,   -72,    -9,     1,  -350,  -350,
     227,  -350,   163,  -139,  -350,   -12,  -283,    62,  -151,  -349,
     -67,   -84,   223,   135,    66,  -350,  -350,   -10,  -350,  -350,
    -350,  -350,  -350,  -350,  -350,   229,  -350
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -169
static const yytype_int16 yytable[] =
{
      99,   115,   114,   252,   247,   254,   230,   240,   231,   128,
     111,  -168,   292,   293,   303,   312,   259,   335,    87,    12,
      13,    88,     4,   138,   139,   262,   263,   226,   128,   227,
     369,     1,   397,   313,   315,   257,   258,   305,   129,   130,
     131,   305,   148,   397,   135,    12,    13,   228,    30,    31,
      32,   332,    33,    34,     5,   383,   284,   129,   130,   131,
     136,     7,   308,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,     8,    30,    31,    32,    99,    33,    34,
      35,    36,    37,   305,    11,   321,   107,   387,   140,   335,
     305,   270,   230,   329,   231,    87,   240,   331,    88,   306,
     334,   322,   402,   145,   142,   404,   330,   243,   340,   117,
     308,   244,   267,   409,   121,   208,   268,   149,   113,   368,
     410,   108,   109,   118,   223,   110,   337,   372,   -40,   127,
     305,   316,   294,   295,   304,   145,   264,   145,   265,   378,
     379,   405,   119,   305,   305,   305,   208,   120,   364,   112,
     345,   346,   347,   230,   230,   230,   230,   230,   230,   230,
     230,   230,   230,   230,   230,   230,   230,   230,   230,   208,
     126,   343,   334,   384,    74,    75,    76,   230,   137,   231,
     129,   130,   131,   282,   143,   230,   344,   231,   352,   353,
     354,   355,   285,   286,   287,   123,   124,   288,   289,  -102,
     290,   291,   296,   297,   208,   348,   349,   144,   365,   350,
     351,   356,   357,   141,   150,   223,   316,   225,   390,   234,
     236,   241,   266,   248,   242,   249,   145,   393,   245,   246,
     253,   230,   256,   231,   250,   406,   271,   255,   301,   208,
     299,    12,    13,   298,   302,   -39,   300,   208,   309,   318,
     320,   323,   208,   325,   123,   -34,   371,   412,   366,   370,
     374,   115,   114,   305,   380,   381,   -40,   408,   382,   385,
      30,    31,    32,   316,    33,    34,    35,    36,    37,   386,
     389,   388,   401,   391,   392,   358,   400,   342,   316,   178,
     396,   316,   403,   359,   411,   360,   363,   220,   361,   316,
     310,   362,   224,   324,   116,   311,   316,   235,   407,   373,
     326,   125,   395,   261,   327,   122,     0,     0,   375,   208,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   208,
       0,     0,   208,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   208,     0,     0,     0,     0,     0,     0,
      12,    13,    14,    15,    16,    17,   153,   154,   155,   208,
     156,   157,   158,   159,   160,   161,   162,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,     0,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,   163,   164,   165,   166,   167,   168,   169,     0,
       0,   170,   171,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      73,    74,    75,    76,     0,    77,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    82,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     172,     0,     0,     0,     0,     0,   173,   174,   175,   176,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   177,   178,   179,    12,    13,    14,    15,    16,    17,
     153,   154,   155,     0,   156,   157,   158,   159,   160,   161,
     162,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,     0,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,   163,   164,   165,   166,
     167,   168,   169,     0,     0,   170,   171,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,    74,    75,    76,     0,    77,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    82,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    83,     0,    84,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   172,     0,     0,     0,     0,     0,
     173,   174,   175,   176,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   177,   178,   260,    12,    13,
      14,    15,    16,    17,   153,   154,   155,     0,   156,   157,
     158,   159,   160,   161,   162,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
       0,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
     163,   164,   165,   166,   167,   168,   169,     0,     0,   170,
     171,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    73,    74,
      75,    76,     0,    77,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    82,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    83,
       0,    84,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   172,     0,
       0,     0,     0,     0,   173,   174,   175,   176,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   177,
     178,   307,    12,    13,    14,    15,    16,    17,   153,   154,
     155,     0,   156,   157,   158,   159,   160,   161,   162,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,     0,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,   163,   164,   165,   166,   167,   168,
     169,     0,     0,   170,   171,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    73,    74,    75,    76,     0,    77,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    82,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,    84,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   172,     0,     0,     0,     0,     0,   173,   174,
     175,   176,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   177,   178,   338,    12,    13,    14,    15,
      16,    17,   153,   154,   155,     0,   156,   157,   158,   159,
     160,   161,   162,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     0,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,   163,   164,
     165,   166,   167,   168,   169,     0,     0,   170,   171,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    73,    74,    75,    76,
       0,    77,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    82,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,    84,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   172,     0,     0,     0,
       0,     0,   173,   174,   175,   176,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   177,   178,    12,
      13,    14,    15,    16,    17,   153,   154,   155,     0,   156,
     157,   158,   159,   160,   161,   162,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,     0,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,   163,   164,   165,   166,   167,   168,   169,     0,     0,
     170,   171,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    73,
      74,    75,    76,     0,    77,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    82,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      83,     0,    84,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   172,
       0,     0,     0,     0,     0,   173,   174,   175,   176,    12,
      13,    14,    15,    16,    17,     0,     0,     0,     0,     0,
     177,   124,     0,     0,     0,     0,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,     0,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,     0,   164,   165,   166,   167,   168,   169,     0,     0,
     170,   171,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    73,
      74,    75,    76,     0,    77,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    82,    14,    15,    16,
      17,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,     0,     0,     0,     0,     0,     0,
      83,     0,    84,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,     0,    72,   172,
       0,     0,     0,     0,     0,   173,   174,   175,   176,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     177,     0,     0,     0,     0,     0,    74,    75,    76,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    14,    15,    16,    17,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,     0,     0,     0,     0,     0,    83,     0,    84,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     0,    72,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   239,     0,
       0,     0,    74,    75,    76,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
      15,    16,    17,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,     0,     0,     0,     0,
       0,     0,    83,     0,    84,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,     0,
     164,   165,   166,   167,   168,   169,     0,     0,   170,   171,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   319,     0,     0,     0,    74,    75,
      76,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    83,     0,
      84,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   172,     0,     0,
       0,     0,     0,   173,   174,   175,   176,    12,    13,    14,
      15,    16,    17,     0,     0,     0,     0,     0,   251,     0,
       0,     0,     0,     0,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,     0,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,     0,
     164,   165,   166,   167,   168,   169,     0,     0,   170,   171,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   333,    74,    75,
      76,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    82,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    83,     0,
      84,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,    15,    16,    17,     0,   172,     0,     0,
       0,     0,     0,   173,   174,   175,   176,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,     0,   164,   165,   166,   167,   168,   169,     0,
       0,   170,   171,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    74,    75,    76,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    83,     0,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,    15,    16,    17,     0,
     172,     0,     0,   229,     0,     0,   173,   174,   175,   176,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,     0,   164,   165,   166,   167,
     168,   169,     0,     0,   170,   171,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    74,    75,    76,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    83,     0,    84,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    14,    15,
      16,    17,     0,   172,     0,     0,   314,     0,     0,   173,
     174,   175,   176,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,     0,   164,
     165,   166,   167,   168,   169,     0,     0,   170,   171,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    74,    75,    76,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    83,     0,    84,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    14,    15,    16,    17,     0,   172,     0,     0,   367,
       0,     0,   173,   174,   175,   176,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,     0,   164,   165,   166,   167,   168,   169,     0,     0,
     170,   171,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      74,    75,    76,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      83,     0,    84,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    14,    15,    16,    17,     0,   172,
       0,     0,     0,     0,     0,   173,   174,   175,   176,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,   269,     0,   164,   165,   166,   167,   168,
     169,     0,     0,   170,   171,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    74,    75,    76,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    83,     0,    84,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,    13,    14,    15,    16,
      17,     0,   172,     0,     0,     0,     0,     0,   173,   174,
     175,   176,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,     0,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,     0,    72,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    73,    74,    75,    76,     0,
      77,     0,     0,     0,     0,     0,     0,     0,    78,    79,
      80,    81,    82,    14,    15,    16,    17,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,     0,     0,     0,     0,     0,    83,     0,    84,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     0,   341,    14,    15,    16,    17,   169,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    74,    75,    76,     0,     0,     0,     0,     0,
       0,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,     0,    72,    14,    15,    16,
      17,     0,    83,     0,    84,     0,     0,     0,     0,     0,
       0,     0,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    74,    75,    76,     0,     0,     0,
       0,     0,     0,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,     0,    72,     0,
       0,     0,     0,     0,    83,     0,    84,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    83,     0,    84
};

static const yytype_int16 yycheck[] =
{
       9,    73,    73,   159,   155,   161,   141,   146,   141,     4,
      76,    76,    86,    87,    91,   195,   172,   256,     9,     3,
       4,     9,    78,    95,    96,    84,    85,   193,     4,   195,
     313,   109,   381,   213,   227,   170,   171,   198,    33,    34,
      35,   198,   114,   392,   198,     3,     4,   213,    32,    33,
      34,   212,    36,    37,     0,   212,   191,    33,    34,    35,
     214,   113,   213,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   110,    32,    33,    34,    86,    36,    37,
      38,    39,    40,   198,    76,   198,   112,   370,    97,   328,
     198,   188,   227,   249,   227,    86,   235,   253,    86,   214,
     256,   214,   385,   112,   103,   388,   214,   194,   264,   113,
     261,   198,   194,   396,   193,   124,   198,   116,    76,   312,
     403,    33,    34,   113,   133,    37,   194,   320,   193,   198,
     198,   228,   206,   207,   211,   144,   195,   146,   197,   194,
     194,   194,   113,   198,   198,   198,   155,   113,   304,   215,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   178,
     194,   268,   328,   366,   104,   105,   106,   312,    76,   312,
      33,    34,    35,   213,    76,   320,   283,   320,   292,   293,
     294,   295,   203,   204,   205,   214,   215,   199,   200,   194,
      82,    83,    88,    89,   213,   288,   289,   215,   305,   290,
     291,   296,   297,   195,    76,   224,   313,    76,   374,   113,
      76,   214,   194,   193,   213,   193,   235,   378,   214,   214,
     193,   366,   193,   366,   214,   391,   193,   212,    90,   248,
     209,     3,     4,   208,    92,   193,   210,   256,    76,   196,
     195,    78,   261,    75,   214,   194,   196,   408,   195,   213,
     193,   333,   333,   198,    76,   194,   193,    12,   196,   213,
      32,    33,    34,   370,    36,    37,    38,    39,    40,   196,
     196,   213,   196,   214,   194,   298,   383,   265,   385,   215,
     213,   388,   213,   299,   214,   300,   303,   127,   301,   396,
     224,   302,   134,   244,    77,   224,   403,   144,   392,   321,
     248,    88,   379,   178,   248,    86,    -1,    -1,   328,   328,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   378,
      -1,    -1,   381,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   392,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,   408,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     103,   104,   105,   106,    -1,   108,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   154,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,    -1,    -1,    -1,    -1,   199,   200,   201,   202,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   214,   215,   216,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   103,   104,   105,   106,    -1,   108,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   120,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   154,    -1,   156,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,    -1,    -1,
     199,   200,   201,   202,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   214,   215,   216,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,
     105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,
      -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,
      -1,    -1,    -1,    -1,   199,   200,   201,   202,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,
     215,   216,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   103,   104,   105,   106,    -1,   108,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   154,    -1,   156,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   193,    -1,    -1,    -1,    -1,    -1,   199,   200,
     201,   202,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   214,   215,   216,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,
      -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   120,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,   156,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,    -1,
      -1,    -1,   199,   200,   201,   202,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   214,   215,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,
     104,   105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     154,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
      -1,    -1,    -1,    -1,    -1,   199,   200,   201,   202,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
     214,   215,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    -1,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,
     104,   105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   120,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
     154,    -1,   156,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    -1,    76,   193,
      -1,    -1,    -1,    -1,    -1,   199,   200,   201,   202,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     214,    -1,    -1,    -1,    -1,    -1,   104,   105,   106,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,   156,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    -1,    76,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   216,    -1,
      -1,    -1,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,   154,    -1,   156,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    -1,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   216,    -1,    -1,    -1,   104,   105,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,
     156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,    -1,    -1,
      -1,    -1,    -1,   199,   200,   201,   202,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,   214,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    -1,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   120,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,
     156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     5,     6,     7,     8,    -1,   193,    -1,    -1,
      -1,    -1,    -1,   199,   200,   201,   202,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    -1,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   154,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,
     193,    -1,    -1,   196,    -1,    -1,   199,   200,   201,   202,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    -1,    76,    77,    78,    79,
      80,    81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   154,    -1,   156,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,     6,
       7,     8,    -1,   193,    -1,    -1,   196,    -1,    -1,   199,
     200,   201,   202,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    -1,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,   105,   106,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,   156,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,   193,    -1,    -1,   196,
      -1,    -1,   199,   200,   201,   202,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    -1,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     154,    -1,   156,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,   193,
      -1,    -1,    -1,    -1,    -1,   199,   200,   201,   202,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    -1,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   104,   105,   106,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   154,    -1,   156,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,    -1,   193,    -1,    -1,    -1,    -1,    -1,   199,   200,
     201,   202,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,    -1,
     108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   116,   117,
     118,   119,   120,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,   156,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    -1,    76,     5,     6,     7,     8,    81,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    -1,    76,     5,     6,     7,
       8,    -1,   154,    -1,   156,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    -1,    76,    -1,
      -1,    -1,    -1,    -1,   154,    -1,   156,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,   156
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   109,   218,   220,    78,     0,   222,   113,   110,   219,
     223,    76,     3,     4,     5,     6,     7,     8,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    76,   103,   104,   105,   106,   108,   116,   117,
     118,   119,   120,   154,   156,   221,   224,   254,   255,   256,
     257,   258,   263,   264,   265,   266,   269,   271,   272,   273,
     274,   275,   276,   277,   278,   302,   303,   112,    33,    34,
      37,    76,   215,    76,   269,   272,   277,   113,   113,   113,
     113,   193,   302,   214,   215,   289,   194,   198,     4,    33,
      34,    35,   260,   261,   270,   198,   214,    76,   272,   272,
     273,   195,   274,    76,   215,   273,   279,   280,   272,   274,
      76,   267,   268,     9,    10,    11,    13,    14,    15,    16,
      17,    18,    19,    75,    76,    77,    78,    79,    80,    81,
      84,    85,   193,   199,   200,   201,   202,   214,   215,   216,
     225,   226,   227,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   252,   254,   255,   273,   284,
     285,   286,   287,   290,   291,   292,   295,   296,   297,   301,
     260,   259,   262,   273,   261,    76,   193,   195,   213,   196,
     236,   249,   253,   273,   113,   279,    76,   281,   282,   216,
     280,   214,   213,   194,   198,   214,   214,   285,   193,   193,
     214,   214,   252,   193,   252,   212,   193,   236,   236,   252,
     216,   290,    84,    85,   195,   197,   194,   194,   198,    74,
     250,   193,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   213,   251,   236,   203,   204,   205,   199,   200,
      82,    83,    86,    87,   206,   207,    88,    89,   208,   209,
     210,    90,    92,    91,   211,   198,   214,   216,   285,    76,
     259,   262,   195,   213,   196,   253,   250,   283,   196,   216,
     195,   198,   214,    78,   268,    75,   284,   291,   298,   252,
     214,   252,   212,   103,   252,   265,   294,   194,   216,   228,
     252,    76,   231,   250,   250,   236,   236,   236,   238,   238,
     239,   239,   240,   240,   240,   240,   241,   241,   242,   243,
     244,   245,   246,   247,   252,   250,   195,   196,   253,   283,
     213,   196,   253,   282,   193,   294,   299,   300,   194,   194,
      76,   194,   196,   212,   253,   213,   196,   283,   213,   196,
     252,   214,   194,   285,   293,   287,   213,   286,   288,   289,
     250,   196,   283,   213,   283,   194,   252,   288,    12,   283,
     283,   214,   285
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, state, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, state); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct _mesa_glsl_parse_state *state;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (state);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct _mesa_glsl_parse_state *state;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, struct _mesa_glsl_parse_state *state)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, state)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    struct _mesa_glsl_parse_state *state;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , state);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, state); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct _mesa_glsl_parse_state *state)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, state)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    struct _mesa_glsl_parse_state *state;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (state);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (struct _mesa_glsl_parse_state *state);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (struct _mesa_glsl_parse_state *state)
#else
int
yyparse (state)
    struct _mesa_glsl_parse_state *state;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

/* User initialization code.  */

/* Line 1251 of yacc.c  */
#line 41 "glsl_parser.ypp"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
}

/* Line 1251 of yacc.c  */
#line 2635 "glsl_parser.cpp"
  yylsp[0] = yylloc;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1464 of yacc.c  */
#line 212 "glsl_parser.ypp"
    {
	   _mesa_glsl_initialize_types(state);
	;}
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 221 "glsl_parser.ypp"
    {
	   switch ((yyvsp[(2) - (3)].n)) {
	   case 100:
	      state->es_shader = true;
	   case 110:
	   case 120:
	   case 130:
	      /* FINISHME: Check against implementation support versions. */
	      state->language_version = (yyvsp[(2) - (3)].n);
	      break;
	   default:
	      _mesa_glsl_error(& (yylsp[(2) - (3)]), state, "Shading language version"
			       "%u is not supported\n", (yyvsp[(2) - (3)].n));
	      break;
	   }
	;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 253 "glsl_parser.ypp"
    {
	   if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
	      YYERROR;
	   }
	;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 262 "glsl_parser.ypp"
    {
	   /* FINISHME: The NULL test is only required because 'precision'
	    * FINISHME: statements are not yet supported.
	    */
	   if ((yyvsp[(1) - (1)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
	;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 270 "glsl_parser.ypp"
    {
	   /* FINISHME: The NULL test is only required because 'precision'
	    * FINISHME: statements are not yet supported.
	    */
	   if ((yyvsp[(2) - (2)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 285 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
	;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 292 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 299 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 306 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 313 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 320 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(2) - (3)].expression);
	;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 328 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 334 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 338 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
	;}
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 345 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 351 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 369 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 388 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 394 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 410 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 416 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 423 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 435 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 441 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 447 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 456 "glsl_parser.ypp"
    { (yyval.n) = ast_plus; ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 457 "glsl_parser.ypp"
    { (yyval.n) = ast_neg; ;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 458 "glsl_parser.ypp"
    { (yyval.n) = ast_logic_not; ;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 459 "glsl_parser.ypp"
    { (yyval.n) = ast_bit_not; ;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 465 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 471 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 477 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 487 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 493 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 503 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 509 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 519 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 525 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 531 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 537 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 547 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 553 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 563 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 573 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 583 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 593 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 603 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 613 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 623 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 633 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 641 "glsl_parser.ypp"
    { (yyval.n) = ast_assign; ;}
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 642 "glsl_parser.ypp"
    { (yyval.n) = ast_mul_assign; ;}
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 643 "glsl_parser.ypp"
    { (yyval.n) = ast_div_assign; ;}
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 644 "glsl_parser.ypp"
    { (yyval.n) = ast_mod_assign; ;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 645 "glsl_parser.ypp"
    { (yyval.n) = ast_add_assign; ;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 646 "glsl_parser.ypp"
    { (yyval.n) = ast_sub_assign; ;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 647 "glsl_parser.ypp"
    { (yyval.n) = ast_ls_assign; ;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 648 "glsl_parser.ypp"
    { (yyval.n) = ast_rs_assign; ;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 649 "glsl_parser.ypp"
    { (yyval.n) = ast_and_assign; ;}
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 650 "glsl_parser.ypp"
    { (yyval.n) = ast_xor_assign; ;}
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 651 "glsl_parser.ypp"
    { (yyval.n) = ast_or_assign; ;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 656 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 660 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   if ((yyvsp[(1) - (3)].expression)->oper != ast_sequence) {
	      (yyval.expression) = new(ctx) ast_expression(ast_sequence, NULL, NULL, NULL);
	      (yyval.expression)->set_location(yylloc);
	      (yyval.expression)->expressions.push_tail(& (yyvsp[(1) - (3)].expression)->link);
	   } else {
	      (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   }

	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	;}
    break;

  case 98:

/* Line 1464 of yacc.c  */
#line 680 "glsl_parser.ypp"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].function);
	;}
    break;

  case 99:

/* Line 1464 of yacc.c  */
#line 684 "glsl_parser.ypp"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
	;}
    break;

  case 100:

/* Line 1464 of yacc.c  */
#line 688 "glsl_parser.ypp"
    {
	   if (((yyvsp[(3) - (4)].type_specifier)->type_specifier != ast_float)
	       && ((yyvsp[(3) - (4)].type_specifier)->type_specifier != ast_int)) {
	      _mesa_glsl_error(& (yylsp[(3) - (4)]), state, "global precision qualifier can "
			       "only be applied to `int' or `float'\n");
	      YYERROR;
	   }

	   (yyval.node) = NULL; /* FINISHME */
	;}
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 711 "glsl_parser.ypp"
    {
	   (yyval.function) = (yyvsp[(1) - (2)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
	;}
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 716 "glsl_parser.ypp"
    {
	   (yyval.function) = (yyvsp[(1) - (3)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
	;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 724 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.function) = new(ctx) ast_function();
	   (yyval.function)->set_location(yylloc);
	   (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
	   (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);
	;}
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 735 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->set_location(yylloc);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (2)].type_specifier);
	   (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (2)].identifier);
	;}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 745 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->set_location(yylloc);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (5)].type_specifier);
	   (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (5)].identifier);
	   (yyval.parameter_declarator)->is_array = true;
	   (yyval.parameter_declarator)->array_size = (yyvsp[(4) - (5)].expression);
	;}
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 760 "glsl_parser.ypp"
    {
	   (yyvsp[(1) - (3)].type_qualifier).flags.i |= (yyvsp[(2) - (3)].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = (yyvsp[(3) - (3)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier);
	;}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 767 "glsl_parser.ypp"
    {
	   (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 772 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyvsp[(1) - (3)].type_qualifier).flags.i |= (yyvsp[(2) - (3)].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(3) - (3)].type_specifier);
	;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 783 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 795 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	;}
    break;

  case 114:

/* Line 1464 of yacc.c  */
#line 799 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 804 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 809 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 823 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), false, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 832 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), true, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 841 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), true, (yyvsp[(5) - (6)].expression), NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 122:

/* Line 1464 of yacc.c  */
#line 850 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (7)].identifier), true, NULL, (yyvsp[(7) - (7)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (7)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 859 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (8)].identifier), true, (yyvsp[(5) - (8)].expression), (yyvsp[(8) - (8)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (8)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 868 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), false, NULL, (yyvsp[(5) - (5)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 881 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   if ((yyvsp[(1) - (1)].fully_specified_type)->specifier->type_specifier != ast_struct) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "empty declaration list\n");
	      YYERROR;
	   } else {
	      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (1)].fully_specified_type));
	      (yyval.declarator_list)->set_location(yylloc);
	   }
	;}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 892 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 901 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), true, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 910 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), true, (yyvsp[(4) - (5)].expression), NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 919 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (6)].identifier), true, NULL, (yyvsp[(6) - (6)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (6)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 928 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (7)].identifier), true, (yyvsp[(4) - (7)].expression), (yyvsp[(7) - (7)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (7)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 937 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 946 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->invariant = true;

	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 960 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
	;}
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 967 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 978 "glsl_parser.ypp"
    {
	  (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
	;}
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 986 "glsl_parser.ypp"
    {
	   if (((yyvsp[(1) - (3)].type_qualifier).flags.i & (yyvsp[(3) - (3)].type_qualifier).flags.i) != 0) {
	      _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
			       "duplicate layout qualifiers used\n");
	      YYERROR;
	   }

	   (yyval.type_qualifier).flags.i = (yyvsp[(1) - (3)].type_qualifier).flags.i | (yyvsp[(3) - (3)].type_qualifier).flags.i;

	   if ((yyvsp[(1) - (3)].type_qualifier).flags.q.explicit_location)
	      (yyval.type_qualifier).location = (yyvsp[(1) - (3)].type_qualifier).location;

	   if ((yyvsp[(3) - (3)].type_qualifier).flags.q.explicit_location)
	      (yyval.type_qualifier).location = (yyvsp[(3) - (3)].type_qualifier).location;
	;}
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 1005 "glsl_parser.ypp"
    {
	   bool got_one = false;

	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   if (state->ARB_fragment_coord_conventions_enable) {
	      if (strcmp((yyvsp[(1) - (1)].identifier), "origin_upper_left") == 0) {
		 got_one = true;
		 (yyval.type_qualifier).flags.q.origin_upper_left = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "pixel_center_integer") == 0) {
		 got_one = true;
		 (yyval.type_qualifier).flags.q.pixel_center_integer = 1;
	      }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if (!got_one) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[(1) - (1)].identifier));
	      YYERROR;
	   } else if (state->ARB_fragment_coord_conventions_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
				 "GL_ARB_fragment_coord_conventions layout "
				 "identifier `%s' used\n", (yyvsp[(1) - (1)].identifier));
	   }
	;}
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 1034 "glsl_parser.ypp"
    {
	   bool got_one = false;

	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   if (state->ARB_explicit_attrib_location_enable) {
	      /* FINISHME: Handle 'index' once GL_ARB_blend_func_exteneded and
	       * FINISHME: GLSL 1.30 (or later) are supported.
	       */
	      if (strcmp("location", (yyvsp[(1) - (3)].identifier)) == 0) {
		 got_one = true;

		 (yyval.type_qualifier).flags.q.explicit_location = 1;

		 if ((yyvsp[(3) - (3)].n) >= 0) {
		    (yyval.type_qualifier).location = (yyvsp[(3) - (3)].n);
		 } else {
		    _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
				     "invalid location %d specified\n", (yyvsp[(3) - (3)].n));
		    YYERROR;
		 }
	      }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if (!got_one) {
	      _mesa_glsl_error(& (yylsp[(1) - (3)]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[(1) - (3)].identifier));
	      YYERROR;
	   } else if (state->ARB_explicit_attrib_location_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (3)]), state,
				 "GL_ARB_explicit_attrib_location layout "
				 "identifier `%s' used\n", (yyvsp[(1) - (3)].identifier));
	   }
	;}
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 1075 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.smooth = 1;
	;}
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 1080 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.flat = 1;
	;}
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 1085 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.noperspective = 1;
	;}
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 1093 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	;}
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 1103 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	;}
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 1109 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	;}
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 1114 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 1119 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (3)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(3) - (3)].type_qualifier).flags.i;
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 1125 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 1133 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	;}
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 1138 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.attribute = 1;
	;}
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 1143 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.varying = 1;
	;}
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 1148 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1;
	   (yyval.type_qualifier).flags.q.varying = 1;
	;}
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 1154 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 1159 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 1164 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 1169 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 1174 "glsl_parser.ypp"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.uniform = 1;
	;}
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 1183 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(2) - (2)].type_specifier);
	   (yyval.type_specifier)->precision = (yyvsp[(1) - (2)].n);
	;}
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 1192 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = NULL;
	;}
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 1198 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (4)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = (yyvsp[(3) - (4)].expression);
	;}
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 1207 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].n));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 1213 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 1219 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 1227 "glsl_parser.ypp"
    { (yyval.n) = ast_void; ;}
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 1228 "glsl_parser.ypp"
    { (yyval.n) = ast_float; ;}
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 1229 "glsl_parser.ypp"
    { (yyval.n) = ast_int; ;}
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 1230 "glsl_parser.ypp"
    { (yyval.n) = ast_uint; ;}
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 1231 "glsl_parser.ypp"
    { (yyval.n) = ast_bool; ;}
    break;

  case 174:

/* Line 1464 of yacc.c  */
#line 1232 "glsl_parser.ypp"
    { (yyval.n) = ast_vec2; ;}
    break;

  case 175:

/* Line 1464 of yacc.c  */
#line 1233 "glsl_parser.ypp"
    { (yyval.n) = ast_vec3; ;}
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 1234 "glsl_parser.ypp"
    { (yyval.n) = ast_vec4; ;}
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 1235 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec2; ;}
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 1236 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec3; ;}
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 1237 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec4; ;}
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 1238 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec2; ;}
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 1239 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec3; ;}
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 1240 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec4; ;}
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 1241 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec2; ;}
    break;

  case 184:

/* Line 1464 of yacc.c  */
#line 1242 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec3; ;}
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 1243 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec4; ;}
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 1244 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2; ;}
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 1245 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2x3; ;}
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 1246 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2x4; ;}
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 1247 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3x2; ;}
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 1248 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3; ;}
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 1249 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3x4; ;}
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 1250 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4x2; ;}
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 1251 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4x3; ;}
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 1252 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4; ;}
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 1253 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1d; ;}
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 1254 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2d; ;}
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 1255 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2drect; ;}
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 1256 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler3d; ;}
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 1257 "glsl_parser.ypp"
    { (yyval.n) = ast_samplercube; ;}
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 1258 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1dshadow; ;}
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 1259 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2dshadow; ;}
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 1260 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2drectshadow; ;}
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 1261 "glsl_parser.ypp"
    { (yyval.n) = ast_samplercubeshadow; ;}
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 1262 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1darray; ;}
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 1263 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2darray; ;}
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 1264 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1darrayshadow; ;}
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 1265 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2darrayshadow; ;}
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 1266 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler1d; ;}
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 1267 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler2d; ;}
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 1268 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler3d; ;}
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 1269 "glsl_parser.ypp"
    { (yyval.n) = ast_isamplercube; ;}
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 1270 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler1darray; ;}
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 1271 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler2darray; ;}
    break;

  case 214:

/* Line 1464 of yacc.c  */
#line 1272 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler1d; ;}
    break;

  case 215:

/* Line 1464 of yacc.c  */
#line 1273 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler2d; ;}
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 1274 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler3d; ;}
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 1275 "glsl_parser.ypp"
    { (yyval.n) = ast_usamplercube; ;}
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 1276 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler1darray; ;}
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 1277 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler2darray; ;}
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 1281 "glsl_parser.ypp"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
				         "precision qualifier forbidden "
					 "in GLSL %d.%d (1.30 or later "
					 "required)\n",
					 state->language_version / 100,
					 state->language_version % 100);

		     (yyval.n) = ast_precision_high;
		  ;}
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 1292 "glsl_parser.ypp"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					 "precision qualifier forbidden "
					 "in GLSL %d.%d (1.30 or later "
					 "required)\n",
					 state->language_version / 100,
					 state->language_version % 100);

		     (yyval.n) = ast_precision_medium;
		  ;}
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 1303 "glsl_parser.ypp"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					 "precision qualifier forbidden "
					 "in GLSL %d.%d (1.30 or later "
					 "required)\n",
					 state->language_version / 100,
					 state->language_version % 100);

		     (yyval.n) = ast_precision_low;
		  ;}
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 1318 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].node));
	   (yyval.struct_specifier)->set_location(yylloc);
	;}
    break;

  case 224:

/* Line 1464 of yacc.c  */
#line 1324 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[(3) - (4)].node));
	   (yyval.struct_specifier)->set_location(yylloc);
	;}
    break;

  case 225:

/* Line 1464 of yacc.c  */
#line 1333 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	;}
    break;

  case 226:

/* Line 1464 of yacc.c  */
#line 1338 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
	;}
    break;

  case 227:

/* Line 1464 of yacc.c  */
#line 1346 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->specifier = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(2) - (3)].declaration)->link);
	;}
    break;

  case 228:

/* Line 1464 of yacc.c  */
#line 1361 "glsl_parser.ypp"
    {
	   (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
	   (yyvsp[(1) - (1)].declaration)->link.self_link();
	;}
    break;

  case 229:

/* Line 1464 of yacc.c  */
#line 1366 "glsl_parser.ypp"
    {
	   (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
	   (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
	;}
    break;

  case 230:

/* Line 1464 of yacc.c  */
#line 1374 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), false, NULL, NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 231:

/* Line 1464 of yacc.c  */
#line 1380 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (4)].identifier), true, (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 1398 "glsl_parser.ypp"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 1406 "glsl_parser.ypp"
    { (yyval.node) = NULL; ;}
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 1407 "glsl_parser.ypp"
    { (yyval.node) = NULL; ;}
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 1414 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 1420 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 1428 "glsl_parser.ypp"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 1434 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 1440 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 1449 "glsl_parser.ypp"
    {
	   if ((yyvsp[(1) - (1)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement\n");
	      assert((yyvsp[(1) - (1)].node) != NULL);
	   }

	   (yyval.node) = (yyvsp[(1) - (1)].node);
	   (yyval.node)->link.self_link();
	;}
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 1459 "glsl_parser.ypp"
    {
	   if ((yyvsp[(2) - (2)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement\n");
	      assert((yyvsp[(2) - (2)].node) != NULL);
	   }
	   (yyval.node) = (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 1471 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement(NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 1477 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 1486 "glsl_parser.ypp"
    {
	   (yyval.node) = new(state) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].selection_rest_statement).then_statement,
						   (yyvsp[(5) - (5)].selection_rest_statement).else_statement);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 1495 "glsl_parser.ypp"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (3)].node);
	   (yyval.selection_rest_statement).else_statement = (yyvsp[(3) - (3)].node);
	;}
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 1500 "glsl_parser.ypp"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (1)].node);
	   (yyval.selection_rest_statement).else_statement = NULL;
	;}
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 1508 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 1512 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));
	   ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   decl->set_location(yylloc);
	   declarator->set_location(yylloc);

	   declarator->declarations.push_tail(&decl->link);
	   (yyval.node) = declarator;
	;}
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 1535 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 262:

/* Line 1464 of yacc.c  */
#line 1542 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 1549 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 1565 "glsl_parser.ypp"
    {
	   (yyval.node) = NULL;
	;}
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 1572 "glsl_parser.ypp"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
	   (yyval.for_rest_statement).rest = NULL;
	;}
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 1577 "glsl_parser.ypp"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
	   (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
	;}
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 1586 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 1592 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 1598 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 1604 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 274:

/* Line 1464 of yacc.c  */
#line 1610 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 275:

/* Line 1464 of yacc.c  */
#line 1618 "glsl_parser.ypp"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); ;}
    break;

  case 276:

/* Line 1464 of yacc.c  */
#line 1619 "glsl_parser.ypp"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 277:

/* Line 1464 of yacc.c  */
#line 1620 "glsl_parser.ypp"
    { (yyval.node) = NULL; ;}
    break;

  case 278:

/* Line 1464 of yacc.c  */
#line 1625 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.function_definition) = new(ctx) ast_function_definition();
	   (yyval.function_definition)->set_location(yylloc);
	   (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
	   (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);
	;}
    break;



/* Line 1464 of yacc.c  */
#line 5090 "glsl_parser.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, state, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, state, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, state, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
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
		      yytoken, &yylval, &yylloc, state);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp, yylsp, state);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, state, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, state);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, state);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



