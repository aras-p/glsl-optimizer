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
     PRAGMA = 367,
     COLON = 368,
     EOL = 369,
     INTERFACE = 370,
     OUTPUT = 371,
     LAYOUT_TOK = 372,
     ASM = 373,
     CLASS = 374,
     UNION = 375,
     ENUM = 376,
     TYPEDEF = 377,
     TEMPLATE = 378,
     THIS = 379,
     PACKED_TOK = 380,
     GOTO = 381,
     INLINE_TOK = 382,
     NOINLINE = 383,
     VOLATILE = 384,
     PUBLIC_TOK = 385,
     STATIC = 386,
     EXTERN = 387,
     EXTERNAL = 388,
     LONG_TOK = 389,
     SHORT_TOK = 390,
     DOUBLE_TOK = 391,
     HALF = 392,
     FIXED_TOK = 393,
     UNSIGNED = 394,
     INPUT_TOK = 395,
     OUPTUT = 396,
     HVEC2 = 397,
     HVEC3 = 398,
     HVEC4 = 399,
     DVEC2 = 400,
     DVEC3 = 401,
     DVEC4 = 402,
     FVEC2 = 403,
     FVEC3 = 404,
     FVEC4 = 405,
     SAMPLER2DRECT = 406,
     SAMPLER3DRECT = 407,
     SAMPLER2DRECTSHADOW = 408,
     SIZEOF = 409,
     CAST = 410,
     NAMESPACE = 411,
     USING = 412,
     ERROR_TOK = 413,
     COMMON = 414,
     PARTITION = 415,
     ACTIVE = 416,
     SAMPLERBUFFER = 417,
     FILTER = 418,
     IMAGE1D = 419,
     IMAGE2D = 420,
     IMAGE3D = 421,
     IMAGECUBE = 422,
     IMAGE1DARRAY = 423,
     IMAGE2DARRAY = 424,
     IIMAGE1D = 425,
     IIMAGE2D = 426,
     IIMAGE3D = 427,
     IIMAGECUBE = 428,
     IIMAGE1DARRAY = 429,
     IIMAGE2DARRAY = 430,
     UIMAGE1D = 431,
     UIMAGE2D = 432,
     UIMAGE3D = 433,
     UIMAGECUBE = 434,
     UIMAGE1DARRAY = 435,
     UIMAGE2DARRAY = 436,
     IMAGE1DSHADOW = 437,
     IMAGE2DSHADOW = 438,
     IMAGEBUFFER = 439,
     IIMAGEBUFFER = 440,
     UIMAGEBUFFER = 441,
     ROW_MAJOR = 442
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

   union {
      struct ast_type_qualifier q;
      unsigned i;
   } type_qualifier;

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



/* Line 214 of yacc.c  */
#line 370 "glsl_parser.cpp"
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
#line 395 "glsl_parser.cpp"

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
#define YYLAST   4096

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  212
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  89
/* YYNRULES -- Number of rules.  */
#define YYNRULES  273
/* YYNRULES -- Number of states.  */
#define YYNSTATES  410

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   442

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   196,     2,     2,     2,   200,   203,     2,
     188,   189,   198,   194,   193,   195,   192,   199,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   207,   209,
     201,   208,   202,   206,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   190,     2,   191,   204,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   210,   205,   211,   197,     2,     2,     2,
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
     185,   186,   187
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    15,    18,    24,
      26,    29,    31,    33,    35,    37,    39,    41,    45,    47,
      52,    54,    58,    61,    64,    66,    68,    70,    74,    77,
      80,    83,    85,    88,    92,    95,    97,    99,   101,   103,
     106,   109,   112,   114,   116,   118,   120,   122,   126,   130,
     134,   136,   140,   144,   146,   150,   154,   156,   160,   164,
     168,   172,   174,   178,   182,   184,   188,   190,   194,   196,
     200,   202,   206,   208,   212,   214,   218,   220,   226,   228,
     232,   234,   236,   238,   240,   242,   244,   246,   248,   250,
     252,   254,   256,   260,   262,   265,   268,   273,   276,   278,
     280,   283,   287,   291,   294,   300,   304,   307,   311,   314,
     315,   317,   319,   321,   323,   325,   329,   335,   342,   350,
     359,   365,   367,   370,   375,   381,   388,   396,   401,   404,
     406,   409,   410,   412,   417,   419,   423,   425,   427,   429,
     431,   433,   435,   438,   441,   443,   445,   448,   451,   454,
     456,   459,   462,   464,   466,   469,   471,   475,   480,   482,
     484,   486,   488,   490,   492,   494,   496,   498,   500,   502,
     504,   506,   508,   510,   512,   514,   516,   518,   520,   522,
     524,   526,   528,   530,   532,   534,   536,   538,   540,   542,
     544,   546,   548,   550,   552,   554,   556,   558,   560,   562,
     564,   566,   568,   570,   572,   574,   576,   578,   580,   582,
     584,   586,   588,   590,   592,   594,   600,   605,   607,   610,
     614,   616,   620,   622,   627,   629,   631,   633,   635,   637,
     639,   641,   643,   645,   647,   649,   651,   653,   655,   658,
     662,   664,   666,   669,   673,   675,   678,   680,   683,   691,
     697,   703,   711,   713,   718,   724,   728,   731,   737,   745,
     752,   754,   756,   758,   759,   762,   766,   769,   772,   775,
     779,   782,   784,   786
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     213,     0,    -1,    -1,   215,   216,   214,   218,    -1,    -1,
     109,    78,   114,    -1,    -1,   216,   217,    -1,   110,    76,
     113,    76,   114,    -1,   299,    -1,   218,   299,    -1,    76,
      -1,   219,    -1,    78,    -1,    79,    -1,    77,    -1,    80,
      -1,   188,   246,   189,    -1,   220,    -1,   221,   190,   222,
     191,    -1,   223,    -1,   221,   192,    76,    -1,   221,    84,
      -1,   221,    85,    -1,   246,    -1,   224,    -1,   225,    -1,
     221,   192,   225,    -1,   227,   189,    -1,   226,   189,    -1,
     228,    74,    -1,   228,    -1,   228,   244,    -1,   227,   193,
     244,    -1,   229,   188,    -1,   268,    -1,    76,    -1,    81,
      -1,   221,    -1,    84,   230,    -1,    85,   230,    -1,   231,
     230,    -1,   194,    -1,   195,    -1,   196,    -1,   197,    -1,
     230,    -1,   232,   198,   230,    -1,   232,   199,   230,    -1,
     232,   200,   230,    -1,   232,    -1,   233,   194,   232,    -1,
     233,   195,   232,    -1,   233,    -1,   234,    82,   233,    -1,
     234,    83,   233,    -1,   234,    -1,   235,   201,   234,    -1,
     235,   202,   234,    -1,   235,    86,   234,    -1,   235,    87,
     234,    -1,   235,    -1,   236,    88,   235,    -1,   236,    89,
     235,    -1,   236,    -1,   237,   203,   236,    -1,   237,    -1,
     238,   204,   237,    -1,   238,    -1,   239,   205,   238,    -1,
     239,    -1,   240,    90,   239,    -1,   240,    -1,   241,    92,
     240,    -1,   241,    -1,   242,    91,   241,    -1,   242,    -1,
     242,   206,   246,   207,   244,    -1,   243,    -1,   230,   245,
     244,    -1,   208,    -1,    93,    -1,    94,    -1,    96,    -1,
      95,    -1,   102,    -1,    97,    -1,    98,    -1,    99,    -1,
     100,    -1,   101,    -1,   244,    -1,   246,   193,   244,    -1,
     243,    -1,   249,   209,    -1,   257,   209,    -1,   108,   272,
     269,   209,    -1,   250,   189,    -1,   252,    -1,   251,    -1,
     252,   254,    -1,   251,   193,   254,    -1,   259,    76,   188,
      -1,   268,    76,    -1,   268,    76,   190,   247,   191,    -1,
     265,   255,   253,    -1,   255,   253,    -1,   265,   255,   256,
      -1,   255,   256,    -1,    -1,    33,    -1,    34,    -1,    35,
      -1,   268,    -1,   258,    -1,   257,   193,    76,    -1,   257,
     193,    76,   190,   191,    -1,   257,   193,    76,   190,   247,
     191,    -1,   257,   193,    76,   190,   191,   208,   278,    -1,
     257,   193,    76,   190,   247,   191,   208,   278,    -1,   257,
     193,    76,   208,   278,    -1,   259,    -1,   259,    76,    -1,
     259,    76,   190,   191,    -1,   259,    76,   190,   247,   191,
      -1,   259,    76,   190,   191,   208,   278,    -1,   259,    76,
     190,   247,   191,   208,   278,    -1,   259,    76,   208,   278,
      -1,   103,    76,    -1,   268,    -1,   266,   268,    -1,    -1,
     261,    -1,   117,   188,   262,   189,    -1,   263,    -1,   262,
     193,   263,    -1,    76,    -1,    40,    -1,    39,    -1,    38,
      -1,     4,    -1,   267,    -1,   264,   266,    -1,   103,   266,
      -1,     4,    -1,     3,    -1,   260,    37,    -1,    32,    37,
      -1,   260,    33,    -1,    34,    -1,    32,    33,    -1,    32,
      34,    -1,    36,    -1,   269,    -1,   272,   269,    -1,   270,
      -1,   270,   190,   191,    -1,   270,   190,   247,   191,    -1,
     271,    -1,   273,    -1,    76,    -1,    74,    -1,     6,    -1,
       7,    -1,     8,    -1,     5,    -1,    29,    -1,    30,    -1,
      31,    -1,    20,    -1,    21,    -1,    22,    -1,    23,    -1,
      24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,    -1,
      41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,    -1,
      46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,    -1,
      51,    -1,   151,    -1,    52,    -1,    53,    -1,    54,    -1,
      55,    -1,   153,    -1,    56,    -1,    57,    -1,    58,    -1,
      59,    -1,    60,    -1,    61,    -1,    62,    -1,    63,    -1,
      64,    -1,    65,    -1,    66,    -1,    67,    -1,    68,    -1,
      69,    -1,    70,    -1,    71,    -1,    72,    -1,   106,    -1,
     105,    -1,   104,    -1,    73,    76,   210,   274,   211,    -1,
      73,   210,   274,   211,    -1,   275,    -1,   274,   275,    -1,
     268,   276,   209,    -1,   277,    -1,   276,   193,   277,    -1,
      76,    -1,    76,   190,   247,   191,    -1,   244,    -1,   248,
      -1,   281,    -1,   282,    -1,   284,    -1,   283,    -1,   290,
      -1,   279,    -1,   288,    -1,   289,    -1,   292,    -1,   293,
      -1,   294,    -1,   298,    -1,   210,   211,    -1,   210,   287,
     211,    -1,   286,    -1,   283,    -1,   210,   211,    -1,   210,
     287,   211,    -1,   280,    -1,   287,   280,    -1,   209,    -1,
     246,   209,    -1,    14,   188,   246,   189,   281,    12,   281,
      -1,    14,   188,   246,   189,   281,    -1,    14,   188,   246,
     189,   282,    -1,    14,   188,   246,   189,   281,    12,   282,
      -1,   246,    -1,   259,    76,   208,   278,    -1,    17,   188,
     246,   189,   284,    -1,    18,   246,   207,    -1,    19,   207,
      -1,    75,   188,   291,   189,   285,    -1,    11,   280,    75,
     188,   246,   189,   209,    -1,    13,   188,   295,   297,   189,
     285,    -1,   288,    -1,   279,    -1,   291,    -1,    -1,   296,
     209,    -1,   296,   209,   246,    -1,    10,   209,    -1,     9,
     209,    -1,    16,   209,    -1,    16,   246,   209,    -1,    15,
     209,    -1,   300,    -1,   248,    -1,   249,   286,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   209,   209,   208,   217,   221,   239,   241,   245,   254,
     262,   273,   277,   284,   291,   298,   305,   312,   319,   320,
     326,   330,   337,   343,   352,   356,   360,   361,   370,   371,
     375,   376,   380,   386,   398,   402,   408,   415,   426,   427,
     433,   439,   449,   450,   451,   452,   456,   457,   463,   469,
     478,   479,   485,   494,   495,   501,   510,   511,   517,   523,
     529,   538,   539,   545,   554,   555,   564,   565,   574,   575,
     584,   585,   594,   595,   604,   605,   614,   615,   624,   625,
     634,   635,   636,   637,   638,   639,   640,   641,   642,   643,
     644,   648,   652,   668,   672,   676,   680,   694,   698,   699,
     703,   708,   716,   727,   737,   752,   759,   764,   775,   787,
     788,   789,   790,   794,   798,   799,   808,   817,   826,   835,
     844,   857,   868,   877,   886,   895,   904,   913,   922,   936,
     943,   954,   955,   959,   966,   967,   974,  1008,  1009,  1010,
    1014,  1018,  1019,  1023,  1031,  1032,  1033,  1034,  1035,  1036,
    1037,  1038,  1039,  1043,  1044,  1052,  1053,  1059,  1068,  1074,
    1080,  1089,  1090,  1091,  1092,  1093,  1094,  1095,  1096,  1097,
    1098,  1099,  1100,  1101,  1102,  1103,  1104,  1105,  1106,  1107,
    1108,  1109,  1110,  1111,  1112,  1113,  1114,  1115,  1116,  1117,
    1118,  1119,  1120,  1121,  1122,  1123,  1124,  1125,  1126,  1127,
    1128,  1129,  1130,  1131,  1132,  1133,  1134,  1135,  1136,  1137,
    1138,  1139,  1143,  1154,  1165,  1179,  1185,  1194,  1199,  1207,
    1222,  1227,  1235,  1241,  1250,  1254,  1260,  1261,  1265,  1266,
    1270,  1274,  1275,  1276,  1277,  1278,  1279,  1280,  1284,  1290,
    1299,  1300,  1304,  1310,  1319,  1329,  1341,  1347,  1356,  1365,
    1371,  1377,  1386,  1390,  1404,  1408,  1409,  1413,  1420,  1427,
    1437,  1438,  1442,  1444,  1450,  1455,  1464,  1470,  1476,  1482,
    1488,  1497,  1498,  1502
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
  "EXTENSION", "LINE", "PRAGMA", "COLON", "EOL", "INTERFACE", "OUTPUT",
  "LAYOUT_TOK", "ASM", "CLASS", "UNION", "ENUM", "TYPEDEF", "TEMPLATE",
  "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK", "NOINLINE", "VOLATILE",
  "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL", "LONG_TOK", "SHORT_TOK",
  "DOUBLE_TOK", "HALF", "FIXED_TOK", "UNSIGNED", "INPUT_TOK", "OUPTUT",
  "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3", "DVEC4", "FVEC2", "FVEC3",
  "FVEC4", "SAMPLER2DRECT", "SAMPLER3DRECT", "SAMPLER2DRECTSHADOW",
  "SIZEOF", "CAST", "NAMESPACE", "USING", "ERROR_TOK", "COMMON",
  "PARTITION", "ACTIVE", "SAMPLERBUFFER", "FILTER", "IMAGE1D", "IMAGE2D",
  "IMAGE3D", "IMAGECUBE", "IMAGE1DARRAY", "IMAGE2DARRAY", "IIMAGE1D",
  "IIMAGE2D", "IIMAGE3D", "IIMAGECUBE", "IIMAGE1DARRAY", "IIMAGE2DARRAY",
  "UIMAGE1D", "UIMAGE2D", "UIMAGE3D", "UIMAGECUBE", "UIMAGE1DARRAY",
  "UIMAGE2DARRAY", "IMAGE1DSHADOW", "IMAGE2DSHADOW", "IMAGEBUFFER",
  "IIMAGEBUFFER", "UIMAGEBUFFER", "ROW_MAJOR", "'('", "')'", "'['", "']'",
  "'.'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'", "'%'", "'<'",
  "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'", "'{'", "'}'",
  "$accept", "translation_unit", "$@1", "version_statement",
  "extension_statement_list", "extension_statement",
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
  "opt_layout_qualifier", "layout_qualifier", "layout_qualifier_id_list",
  "layout_qualifier_id", "interpolation_qualifier",
  "parameter_type_qualifier", "type_qualifier", "storage_qualifier",
  "type_specifier", "type_specifier_no_prec", "type_specifier_nonarray",
  "basic_type_specifier_nonarray", "precision_qualifier",
  "struct_specifier", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "statement_matched",
  "statement_unmatched", "simple_statement", "compound_statement",
  "statement_no_new_scope", "compound_statement_no_new_scope",
  "statement_list", "expression_statement", "selection_statement_matched",
  "selection_statement_unmatched", "condition", "switch_statement",
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
     435,   436,   437,   438,   439,   440,   441,   442,    40,    41,
      91,    93,    46,    44,    43,    45,    33,   126,    42,    47,
      37,    60,    62,    38,    94,   124,    63,    58,    61,    59,
     123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   212,   214,   213,   215,   215,   216,   216,   217,   218,
     218,   219,   220,   220,   220,   220,   220,   220,   221,   221,
     221,   221,   221,   221,   222,   223,   224,   224,   225,   225,
     226,   226,   227,   227,   228,   229,   229,   229,   230,   230,
     230,   230,   231,   231,   231,   231,   232,   232,   232,   232,
     233,   233,   233,   234,   234,   234,   235,   235,   235,   235,
     235,   236,   236,   236,   237,   237,   238,   238,   239,   239,
     240,   240,   241,   241,   242,   242,   243,   243,   244,   244,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   246,   246,   247,   248,   248,   248,   249,   250,   250,
     251,   251,   252,   253,   253,   254,   254,   254,   254,   255,
     255,   255,   255,   256,   257,   257,   257,   257,   257,   257,
     257,   258,   258,   258,   258,   258,   258,   258,   258,   259,
     259,   260,   260,   261,   262,   262,   263,   264,   264,   264,
     265,   266,   266,   266,   267,   267,   267,   267,   267,   267,
     267,   267,   267,   268,   268,   269,   269,   269,   270,   270,
     270,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   272,   272,   272,   273,   273,   274,   274,   275,
     276,   276,   277,   277,   278,   279,   280,   280,   281,   281,
     282,   283,   283,   283,   283,   283,   283,   283,   284,   284,
     285,   285,   286,   286,   287,   287,   288,   288,   289,   290,
     290,   290,   291,   291,   292,   293,   293,   294,   294,   294,
     295,   295,   296,   296,   297,   297,   298,   298,   298,   298,
     298,   299,   299,   300
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     0,     2,     5,     1,
       2,     1,     1,     1,     1,     1,     1,     3,     1,     4,
       1,     3,     2,     2,     1,     1,     1,     3,     2,     2,
       2,     1,     2,     3,     2,     1,     1,     1,     1,     2,
       2,     2,     1,     1,     1,     1,     1,     3,     3,     3,
       1,     3,     3,     1,     3,     3,     1,     3,     3,     3,
       3,     1,     3,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     5,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     2,     2,     4,     2,     1,     1,
       2,     3,     3,     2,     5,     3,     2,     3,     2,     0,
       1,     1,     1,     1,     1,     3,     5,     6,     7,     8,
       5,     1,     2,     4,     5,     6,     7,     4,     2,     1,
       2,     0,     1,     4,     1,     3,     1,     1,     1,     1,
       1,     1,     2,     2,     1,     1,     2,     2,     2,     1,
       2,     2,     1,     1,     2,     1,     3,     4,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     4,     1,     2,     3,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     3,
       1,     1,     2,     3,     1,     2,     1,     2,     7,     5,
       5,     7,     1,     4,     5,     3,     2,     5,     7,     6,
       1,     1,     1,     0,     2,     3,     2,     2,     2,     3,
       2,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,     6,     0,     1,     2,     5,     0,   131,
       7,     0,   145,   144,   165,   162,   163,   164,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   166,   167,   168,
       0,   149,   152,   139,   138,   137,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   190,   191,   192,
     193,   195,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,     0,   161,
     160,   131,   214,   213,   212,     0,     0,   189,   194,   131,
     272,     0,     0,    99,   109,     0,   114,   121,     0,   132,
     131,     0,   141,   129,   153,   155,   158,     0,   159,     9,
     271,     0,   150,   151,   147,     0,     0,   128,   131,   143,
       0,     0,    10,    94,   131,   273,    97,   109,   140,   110,
     111,   112,   100,     0,   109,     0,    95,   122,   148,   146,
     142,   130,     0,   154,     0,     0,     0,     0,   217,     0,
     136,     0,   134,     0,     0,   131,     0,     0,     0,     0,
       0,     0,     0,     0,    11,    15,    13,    14,    16,    37,
       0,     0,     0,    42,    43,    44,    45,   246,   131,   242,
      12,    18,    38,    20,    25,    26,     0,     0,    31,     0,
      46,     0,    50,    53,    56,    61,    64,    66,    68,    70,
      72,    74,    76,    78,    91,     0,   225,     0,   129,   231,
     244,   226,   227,   229,   228,   131,   232,   233,   230,   234,
     235,   236,   237,   101,   106,   108,   113,     0,   115,   102,
       0,     0,   156,    46,    93,     0,    35,     8,     0,   222,
       0,   220,   216,   218,    96,   133,     0,   267,   266,     0,
     131,     0,   270,   268,     0,     0,     0,   256,   131,    39,
      40,     0,   238,   131,    22,    23,     0,     0,    29,    28,
       0,   161,    32,    34,    81,    82,    84,    83,    86,    87,
      88,    89,    90,    85,    80,     0,    41,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   247,   243,
     245,   103,   105,   107,     0,     0,   123,     0,   224,   127,
     157,   215,     0,     0,   219,   135,     0,   261,   260,   131,
       0,   269,     0,   255,   252,     0,     0,    17,   239,     0,
      24,    21,    27,    33,    79,    47,    48,    49,    51,    52,
      54,    55,    59,    60,    57,    58,    62,    63,    65,    67,
      69,    71,    73,    75,     0,    92,     0,   116,     0,   120,
       0,   124,     0,   221,     0,   262,     0,     0,   131,     0,
       0,   131,    19,     0,     0,     0,   117,   125,     0,   223,
       0,   264,   131,   249,   250,   254,     0,     0,   241,   257,
     240,    77,   104,   118,     0,   126,     0,   265,   259,   131,
     253,     0,   119,   258,   248,   251,     0,   131,     0,   131
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     9,     3,     6,    10,    79,   170,   171,   172,
     329,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   275,   195,   225,   196,   197,    82,    83,
      84,   214,   122,   123,   215,    85,    86,    87,    88,    89,
     141,   142,    90,   124,    91,    92,   226,    94,    95,    96,
      97,    98,   137,   138,   230,   231,   309,   199,   200,   201,
     202,   203,   204,   389,   390,   205,   206,   207,   208,   326,
     209,   210,   211,   319,   366,   367,   212,    99,   100
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -345
static const yytype_int16 yypact[] =
{
     -30,    29,   120,  -345,    15,  -345,    22,  -345,    59,  3758,
    -345,    25,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
      79,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,   -71,  -345,
    -345,   130,  -345,  -345,  -345,   -79,   -42,  -345,  -345,  3642,
    -345,    -5,   -38,   -32,     4,  -181,  -345,    87,    62,  -345,
      27,  3871,  -345,  -345,  -345,   -25,  -345,  3943,  -345,  -345,
    -345,    91,  -345,  -345,  -345,   -37,  3871,  -345,    27,  -345,
    3943,    95,  -345,  -345,   398,  -345,  -345,    19,  -345,  -345,
    -345,  -345,  -345,  3871,     0,   119,  -345,  -128,  -345,  -345,
    -345,  -345,  2752,  -345,    86,  3871,   131,  2153,  -345,    11,
    -345,   -87,  -345,    21,    23,  1234,    40,    50,    36,  2379,
      63,  3286,    43,    64,   -73,  -345,  -345,  -345,  -345,  -345,
    3286,  3286,  3286,  -345,  -345,  -345,  -345,  -345,   607,  -345,
    -345,  -345,   -67,  -345,  -345,  -345,    78,   -62,  3464,    80,
     -53,  3286,    -1,    20,   140,   -80,   136,    66,    67,    65,
     182,   181,   -82,  -345,  -345,  -173,  -345,   103,   125,  -345,
    -345,  -345,  -345,  -345,  -345,   816,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  -345,   198,  3871,  -140,  -345,
    2930,  3286,  -345,  -345,  -345,    84,  -345,  -345,  2266,   124,
    -137,  -345,  -345,  -345,  -345,  -345,    95,  -345,  -345,   240,
    1845,  3286,  -345,  -345,  -118,  3286,  -120,  -345,  2574,  -345,
    -345,   -48,  -345,  1025,  -345,  -345,  3286,   235,  -345,  -345,
    3286,   128,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,  -345,  -345,  3286,  -345,  3286,  3286,  3286,
    3286,  3286,  3286,  3286,  3286,  3286,  3286,  3286,  3286,  3286,
    3286,  3286,  3286,  3286,  3286,  3286,  3286,  3286,  -345,  -345,
    -345,   129,  -345,  -345,  3108,  3286,   110,   132,  -345,  -345,
    -345,  -345,  3286,   131,  -345,  -345,   133,  -345,  -345,  2040,
     -46,  -345,   -36,  -345,   127,   246,   135,  -345,  -345,   134,
     127,   138,  -345,  -345,  -345,  -345,  -345,  -345,    -1,    -1,
      20,    20,   140,   140,   140,   140,   -80,   -80,   136,    66,
      67,    65,   182,   181,  -117,  -345,  3286,   121,   137,  -345,
    3286,   122,   141,  -345,  3286,  -345,   118,   142,  1234,   123,
     126,  1442,  -345,  3286,   144,  3286,   139,  -345,  3286,  -345,
     -35,  3286,  1442,   324,  -345,  -345,  3286,   149,  -345,  -345,
    -345,  -345,  -345,  -345,  3286,  -345,   143,   127,  -345,  1234,
    -345,  3286,  -345,  -345,  -345,  -345,   -33,  1650,   326,  1650
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,  -345,
    -345,  -345,  -345,    85,  -345,  -345,  -345,  -345,  -103,  -345,
     -54,   -47,   -74,   -40,    53,    54,    52,    55,    56,    51,
    -345,  -110,  -157,  -345,  -147,  -219,     5,     7,  -345,  -345,
    -345,   146,   232,   227,   147,  -345,  -345,  -238,  -345,  -345,
    -345,   117,  -345,  -345,   -39,  -345,    -9,   -14,  -345,  -345,
     279,  -345,   220,  -124,  -345,    44,  -286,   116,  -134,  -257,
    -344,  -294,   -11,   -22,   280,   197,   145,  -345,  -345,    47,
    -345,  -345,  -345,  -345,  -345,  -345,  -345,   288,  -345
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -264
static const yytype_int16 yytable[] =
{
      93,   307,   244,  -160,   246,   105,   284,   285,   118,   295,
     325,   239,   125,   233,    80,   251,    81,   254,   255,   359,
     297,   262,   224,   118,   384,    72,    73,    74,   126,   223,
      12,    13,   109,   119,   120,   121,   298,   119,   120,   121,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     304,   130,   119,   120,   121,   405,   313,   249,   250,    30,
     219,    31,   220,    32,   308,    33,    34,    35,   305,   109,
      93,   300,   314,   297,   377,   297,   297,   388,   276,     1,
     221,   325,   131,   133,    80,   358,    81,   323,   388,   393,
     373,   321,   395,   362,   320,   128,   139,   136,   322,   129,
     400,   324,   235,   333,   233,   198,   236,     4,   402,   330,
     224,   383,   102,   103,   216,   -36,   104,   223,   334,   300,
       5,   286,   287,   256,   296,   257,   136,   259,   136,     7,
     108,   260,     8,    12,    13,    11,   198,   374,   101,   106,
     355,   327,   404,   368,    76,   297,   111,   297,   308,   354,
     408,   116,   404,   369,   396,   274,   407,   297,   297,   198,
     297,   117,    30,   127,    31,   132,    32,   134,    33,    34,
      35,   140,   324,   135,   335,   336,   337,   223,   223,   223,
     223,   223,   223,   223,   223,   223,   223,   223,   223,   223,
     223,   223,   223,   -98,   224,   218,   198,   277,   278,   279,
     227,   223,   224,   308,   113,   114,   107,   229,   216,   223,
     342,   343,   344,   345,   280,   281,   391,   380,   308,   136,
     234,   308,   282,   283,   288,   289,   338,   339,   240,   308,
     237,   198,   238,   108,   397,   340,   341,   308,   241,   198,
      14,    15,    16,    17,   198,   242,   224,    76,   346,   347,
     247,   245,   248,   223,   406,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,   258,   263,   290,
     292,   291,   293,   294,   301,   310,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
     198,   331,   113,   -35,   312,   316,   159,   -30,   360,   356,
     297,   364,   370,   361,   371,   372,   -36,   381,   376,   375,
     378,   382,   379,   168,   386,   392,   399,   401,   409,    72,
      73,    74,   332,   348,   350,   349,   353,   394,   351,   213,
     352,   217,   403,   315,   110,   228,   317,   363,   385,   198,
     398,   115,   198,   302,   303,   253,   365,   112,     0,     0,
       0,     0,     0,   198,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   318,    77,     0,    78,     0,
     198,     0,     0,     0,     0,     0,     0,     0,   198,     0,
     198,    12,    13,    14,    15,    16,    17,   143,   144,   145,
       0,   146,   147,   148,   149,   150,   151,   152,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,     0,    31,     0,    32,     0,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,   153,   154,   155,   156,   157,   158,   159,
       0,     0,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    71,    72,    73,    74,     0,    75,     0,     0,     0,
       0,     0,     0,     0,     0,    76,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
       0,    78,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   162,     0,     0,     0,
       0,     0,   163,   164,   165,   166,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   167,   168,   169,
      12,    13,    14,    15,    16,    17,   143,   144,   145,     0,
     146,   147,   148,   149,   150,   151,   152,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       0,    31,     0,    32,     0,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,   153,   154,   155,   156,   157,   158,   159,     0,
       0,   160,   161,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      71,    72,    73,    74,     0,    75,     0,     0,     0,     0,
       0,     0,     0,     0,    76,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    77,     0,
      78,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   162,     0,     0,     0,     0,
       0,   163,   164,   165,   166,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   167,   168,   252,    12,
      13,    14,    15,    16,    17,   143,   144,   145,     0,   146,
     147,   148,   149,   150,   151,   152,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,     0,
      31,     0,    32,     0,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,   153,   154,   155,   156,   157,   158,   159,     0,     0,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    71,
      72,    73,    74,     0,    75,     0,     0,     0,     0,     0,
       0,     0,     0,    76,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    77,     0,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   162,     0,     0,     0,     0,     0,
     163,   164,   165,   166,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   167,   168,   299,    12,    13,
      14,    15,    16,    17,   143,   144,   145,     0,   146,   147,
     148,   149,   150,   151,   152,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,     0,    31,
       0,    32,     0,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
     153,   154,   155,   156,   157,   158,   159,     0,     0,   160,
     161,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    71,    72,
      73,    74,     0,    75,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    77,     0,    78,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,     0,     0,     0,     0,     0,   163,
     164,   165,   166,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   167,   168,   328,    12,    13,    14,
      15,    16,    17,   143,   144,   145,     0,   146,   147,   148,
     149,   150,   151,   152,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,     0,    31,     0,
      32,     0,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,   153,
     154,   155,   156,   157,   158,   159,     0,     0,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    71,    72,    73,
      74,     0,    75,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,     0,    78,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   162,     0,     0,     0,     0,     0,   163,   164,
     165,   166,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   167,   168,    12,    13,    14,    15,    16,
      17,   143,   144,   145,     0,   146,   387,   148,   149,   150,
     151,   152,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,     0,    31,     0,    32,     0,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,   153,   154,   155,
     156,   157,   158,   159,     0,     0,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    71,    72,    73,    74,     0,
      75,     0,     0,     0,     0,     0,     0,     0,     0,    76,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,     0,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     162,     0,     0,     0,     0,     0,   163,   164,   165,   166,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   167,   114,    12,    13,    14,    15,    16,    17,   143,
     144,   145,     0,   146,   387,   148,   149,   150,   151,   152,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,     0,    31,     0,    32,     0,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,   153,   154,   155,   156,   157,
     158,   159,     0,     0,   160,   161,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    71,    72,    73,    74,     0,    75,     0,
       0,     0,     0,     0,     0,     0,     0,    76,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    77,     0,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   162,     0,
       0,     0,     0,     0,   163,   164,   165,   166,    12,    13,
      14,    15,    16,    17,     0,     0,     0,     0,     0,   167,
     168,     0,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,     0,    31,
       0,    32,     0,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
       0,   154,   155,   156,   157,   158,   159,     0,     0,   160,
     161,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    71,    72,
      73,    74,     0,    75,     0,     0,     0,     0,     0,     0,
       0,     0,    76,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    77,     0,    78,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   162,     0,     0,     0,     0,     0,   163,
     164,   165,   166,    12,    13,    14,    15,    16,    17,     0,
       0,     0,     0,     0,   167,     0,     0,     0,     0,     0,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,     0,    31,     0,    32,     0,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,     0,   154,   155,   156,   157,
     158,   159,     0,     0,   160,   161,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   108,    72,    73,    74,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    76,    14,    15,
      16,    17,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,     0,     0,     0,     0,     0,
       0,    77,     0,    78,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,   162,    70,
       0,     0,     0,     0,   163,   164,   165,   166,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -263,
       0,     0,     0,     0,     0,     0,     0,    72,    73,    74,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    14,    15,    16,    17,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,     0,     0,
       0,     0,     0,     0,    77,     0,    78,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,     0,    70,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   232,     0,     0,     0,     0,     0,
      72,    73,    74,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    14,    15,    16,    17,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,     0,     0,     0,     0,     0,     0,    77,     0,    78,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,     0,   154,   155,   156,   157,   158,
     159,     0,     0,   160,   161,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   311,     0,     0,
       0,     0,     0,    72,    73,    74,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      77,     0,    78,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   162,     0,     0,
       0,     0,     0,   163,   164,   165,   166,    12,    13,    14,
      15,    16,    17,     0,     0,     0,     0,     0,   243,     0,
       0,     0,     0,     0,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,     0,    31,     0,
      32,     0,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,     0,
     154,   155,   156,   157,   158,   159,     0,     0,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,    72,    73,
      74,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,     0,    78,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    14,    15,    16,
      17,     0,   162,     0,     0,     0,     0,     0,   163,   164,
     165,   166,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,     0,   154,   155,
     156,   157,   158,   159,     0,     0,   160,   161,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    72,    73,    74,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    77,     0,    78,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,    15,    16,    17,     0,
     162,     0,     0,   222,     0,     0,   163,   164,   165,   166,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,     0,   154,   155,   156,   157,
     158,   159,     0,     0,   160,   161,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    72,    73,    74,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    77,     0,    78,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    14,    15,    16,    17,     0,   162,     0,
       0,   306,     0,     0,   163,   164,   165,   166,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,     0,   154,   155,   156,   157,   158,   159,
       0,     0,   160,   161,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    72,    73,    74,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    77,
       0,    78,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    14,    15,    16,    17,     0,   162,     0,     0,   357,
       0,     0,   163,   164,   165,   166,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,     0,   154,   155,   156,   157,   158,   159,     0,     0,
     160,   161,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      72,    73,    74,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    77,     0,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    14,
      15,    16,    17,     0,   162,     0,     0,     0,     0,     0,
     163,   164,   165,   166,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,   261,     0,
     154,   155,   156,   157,   158,   159,     0,     0,   160,   161,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    72,    73,
      74,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    77,     0,    78,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    -3,     0,     0,    12,    13,    14,    15,    16,
      17,     0,   162,     0,     0,     0,     0,     0,   163,   164,
     165,   166,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,     0,    31,     0,    32,     0,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,     0,    70,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    71,    72,    73,    74,     0,
      75,     0,     0,     0,     0,     0,     0,     0,     0,    76,
       0,    12,    13,    14,    15,    16,    17,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,     0,    31,    77,    32,    78,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,     0,    70,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    71,    72,    73,    74,     0,    75,     0,     0,     0,
       0,     0,     0,     0,     0,    76,    14,    15,    16,    17,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,     0,     0,     0,     0,     0,     0,    77,
       0,    78,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,     0,    70,    14,    15,
      16,    17,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    72,    73,    74,     0,     0,
       0,     0,     0,     0,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,     0,    70,
       0,     0,    77,     0,    78,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    77,     0,    78
};

static const yytype_int16 yycheck[] =
{
       9,   220,   149,    76,   151,    76,    86,    87,     4,    91,
     248,   145,   193,   137,     9,   162,     9,    84,    85,   305,
     193,   178,   132,     4,   368,   104,   105,   106,   209,   132,
       3,     4,    71,    33,    34,    35,   209,    33,    34,    35,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     190,    90,    33,    34,    35,   399,   193,   160,   161,    32,
     188,    34,   190,    36,   221,    38,    39,    40,   208,   108,
      79,   205,   209,   193,   360,   193,   193,   371,   181,   109,
     208,   319,    91,    97,    79,   304,    79,   207,   382,   375,
     207,   209,   378,   312,   241,    33,   110,   106,   245,    37,
     386,   248,   189,   260,   228,   114,   193,    78,   394,   256,
     220,   368,    33,    34,   123,   188,    37,   220,   275,   253,
       0,   201,   202,   190,   206,   192,   135,   189,   137,   114,
     103,   193,   110,     3,     4,    76,   145,   356,   113,   210,
     297,   189,   399,   189,   117,   193,   188,   193,   305,   296,
     407,   189,   409,   189,   189,   208,   189,   193,   193,   168,
     193,   193,    32,    76,    34,   190,    36,    76,    38,    39,
      40,    76,   319,   210,   277,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   189,   304,    76,   205,   198,   199,   200,
     114,   304,   312,   360,   209,   210,    76,    76,   217,   312,
     284,   285,   286,   287,   194,   195,   373,   364,   375,   228,
     209,   378,    82,    83,    88,    89,   280,   281,   188,   386,
     209,   240,   209,   103,   381,   282,   283,   394,   188,   248,
       5,     6,     7,     8,   253,   209,   356,   117,   288,   289,
     207,   188,   188,   356,   401,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   189,   188,   203,
     205,   204,    90,    92,    76,   191,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
     319,    76,   209,   188,   190,    75,    81,   189,   208,   190,
     193,   188,    76,   191,   189,   191,   188,   209,   191,   208,
     208,   189,   191,   210,   208,   191,    12,   188,    12,   104,
     105,   106,   257,   290,   292,   291,   295,   208,   293,   117,
     294,   124,   209,   236,    75,   135,   240,   313,   369,   368,
     382,    81,   371,   217,   217,   168,   319,    79,    -1,    -1,
      -1,    -1,    -1,   382,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   240,   151,    -1,   153,    -1,
     399,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,    -1,
     409,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    34,    -1,    36,    -1,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   103,   104,   105,   106,    -1,   108,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
      -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,    -1,    -1,
      -1,    -1,   194,   195,   196,   197,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,   210,   211,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    34,    -1,    36,    -1,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     103,   104,   105,   106,    -1,   108,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   117,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,
     153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   188,    -1,    -1,    -1,    -1,
      -1,   194,   195,   196,   197,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   209,   210,   211,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      34,    -1,    36,    -1,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,
     104,   105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   188,    -1,    -1,    -1,    -1,    -1,
     194,   195,   196,   197,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   209,   210,   211,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      -1,    36,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,
     105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   188,    -1,    -1,    -1,    -1,    -1,   194,
     195,   196,   197,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   209,   210,   211,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    34,    -1,
      36,    -1,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,
     106,    -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,    -1,   153,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   188,    -1,    -1,    -1,    -1,    -1,   194,   195,
     196,   197,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   209,   210,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    34,    -1,    36,    -1,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,    -1,
     108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,    -1,   153,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     188,    -1,    -1,    -1,    -1,    -1,   194,   195,   196,   197,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   209,   210,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    34,    -1,    36,    -1,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   103,   104,   105,   106,    -1,   108,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,    -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,
      -1,    -1,    -1,    -1,   194,   195,   196,   197,     3,     4,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,   209,
     210,    -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      -1,    36,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      -1,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,
     105,   106,    -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   188,    -1,    -1,    -1,    -1,    -1,   194,
     195,   196,   197,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    -1,    34,    -1,    36,    -1,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    -1,    76,    77,    78,    79,
      80,    81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   103,   104,   105,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,
      -1,   151,    -1,   153,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,   188,    76,
      -1,    -1,    -1,    -1,   194,   195,   196,   197,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,   105,   106,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,   151,    -1,   153,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   211,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    -1,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   211,    -1,    -1,
      -1,    -1,    -1,   104,   105,   106,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     151,    -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   188,    -1,    -1,
      -1,    -1,    -1,   194,   195,   196,   197,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,   209,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    -1,    34,    -1,
      36,    -1,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    -1,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   117,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,    -1,   153,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,
       8,    -1,   188,    -1,    -1,    -1,    -1,    -1,   194,   195,
     196,   197,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    -1,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   104,   105,   106,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   151,    -1,   153,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,
     188,    -1,    -1,   191,    -1,    -1,   194,   195,   196,   197,
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
      -1,   151,    -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,   188,    -1,
      -1,   191,    -1,    -1,   194,   195,   196,   197,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    -1,    76,    77,    78,    79,    80,    81,
      -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   104,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,
      -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,   188,    -1,    -1,   191,
      -1,    -1,   194,   195,   196,   197,    20,    21,    22,    23,
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
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,   153,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    -1,   188,    -1,    -1,    -1,    -1,    -1,
     194,   195,   196,   197,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    -1,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,   105,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   151,    -1,   153,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     0,    -1,    -1,     3,     4,     5,     6,     7,
       8,    -1,   188,    -1,    -1,    -1,    -1,    -1,   194,   195,
     196,   197,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    34,    -1,    36,    -1,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,    -1,
     108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   117,
      -1,     3,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    34,   151,    36,   153,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    -1,    76,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   103,   104,   105,   106,    -1,   108,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   117,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,   151,
      -1,   153,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    -1,    76,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,   104,   105,   106,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    -1,    76,
      -1,    -1,   151,    -1,   153,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   151,    -1,   153
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   109,   213,   215,    78,     0,   216,   114,   110,   214,
     217,    76,     3,     4,     5,     6,     7,     8,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    34,    36,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      76,   103,   104,   105,   106,   108,   117,   151,   153,   218,
     248,   249,   250,   251,   252,   257,   258,   259,   260,   261,
     264,   266,   267,   268,   269,   270,   271,   272,   273,   299,
     300,   113,    33,    34,    37,    76,   210,    76,   103,   266,
     272,   188,   299,   209,   210,   286,   189,   193,     4,    33,
      34,    35,   254,   255,   265,   193,   209,    76,    33,    37,
     266,   268,   190,   269,    76,   210,   268,   274,   275,   269,
      76,   262,   263,     9,    10,    11,    13,    14,    15,    16,
      17,    18,    19,    75,    76,    77,    78,    79,    80,    81,
      84,    85,   188,   194,   195,   196,   197,   209,   210,   211,
     219,   220,   221,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   246,   248,   249,   268,   279,
     280,   281,   282,   283,   284,   287,   288,   289,   290,   292,
     293,   294,   298,   254,   253,   256,   268,   255,    76,   188,
     190,   208,   191,   230,   243,   247,   268,   114,   274,    76,
     276,   277,   211,   275,   209,   189,   193,   209,   209,   280,
     188,   188,   209,   209,   246,   188,   246,   207,   188,   230,
     230,   246,   211,   287,    84,    85,   190,   192,   189,   189,
     193,    74,   244,   188,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   208,   245,   230,   198,   199,   200,
     194,   195,    82,    83,    86,    87,   201,   202,    88,    89,
     203,   204,   205,    90,    92,    91,   206,   193,   209,   211,
     280,    76,   253,   256,   190,   208,   191,   247,   244,   278,
     191,   211,   190,   193,   209,   263,    75,   279,   288,   295,
     246,   209,   246,   207,   246,   259,   291,   189,   211,   222,
     246,    76,   225,   244,   244,   230,   230,   230,   232,   232,
     233,   233,   234,   234,   234,   234,   235,   235,   236,   237,
     238,   239,   240,   241,   246,   244,   190,   191,   247,   278,
     208,   191,   247,   277,   188,   291,   296,   297,   189,   189,
      76,   189,   191,   207,   247,   208,   191,   278,   208,   191,
     246,   209,   189,   281,   282,   284,   208,    14,   283,   285,
     286,   244,   191,   278,   208,   278,   189,   246,   285,    12,
     278,   188,   278,   209,   281,   282,   246,   189,   281,    12
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
#line 2699 "glsl_parser.cpp"
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
#line 209 "glsl_parser.ypp"
    {
	   _mesa_glsl_initialize_types(state);
	;}
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 217 "glsl_parser.ypp"
    {
	   state->language_version = 110;
	   state->symbols->language_version = 110;
	;}
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 222 "glsl_parser.ypp"
    {
	   switch ((yyvsp[(2) - (3)].n)) {
	   case 110:
	   case 120:
	   case 130:
	      /* FINISHME: Check against implementation support versions. */
	      state->language_version = (yyvsp[(2) - (3)].n);
	      state->symbols->language_version = (yyvsp[(2) - (3)].n);
	      break;
	   default:
	      _mesa_glsl_error(& (yylsp[(2) - (3)]), state, "Shading language version"
			       "%u is not supported\n", (yyvsp[(2) - (3)].n));
	      break;
	   }
	;}
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 246 "glsl_parser.ypp"
    {
	   if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
	      YYERROR;
	   }
	;}
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 255 "glsl_parser.ypp"
    {
	   /* FINISHME: The NULL test is only required because 'precision'
	    * FINISHME: statements are not yet supported.
	    */
	   if ((yyvsp[(1) - (1)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
	;}
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 263 "glsl_parser.ypp"
    {
	   /* FINISHME: The NULL test is only required because 'precision'
	    * FINISHME: statements are not yet supported.
	    */
	   if ((yyvsp[(2) - (2)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 278 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
	;}
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 285 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 292 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 299 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 306 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 313 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(2) - (3)].expression);
	;}
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 321 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 327 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 331 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
	;}
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 338 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 344 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 362 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 381 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	;}
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 387 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	;}
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 403 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 409 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 416 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 428 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 434 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 440 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 449 "glsl_parser.ypp"
    { (yyval.n) = ast_plus; ;}
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 450 "glsl_parser.ypp"
    { (yyval.n) = ast_neg; ;}
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 451 "glsl_parser.ypp"
    { (yyval.n) = ast_logic_not; ;}
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 452 "glsl_parser.ypp"
    { (yyval.n) = ast_bit_not; ;}
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 458 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 464 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 470 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 480 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 486 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 496 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 502 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 512 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 518 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 524 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 530 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 540 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 546 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 556 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 566 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 576 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 586 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 596 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 606 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 616 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 626 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 634 "glsl_parser.ypp"
    { (yyval.n) = ast_assign; ;}
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 635 "glsl_parser.ypp"
    { (yyval.n) = ast_mul_assign; ;}
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 636 "glsl_parser.ypp"
    { (yyval.n) = ast_div_assign; ;}
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 637 "glsl_parser.ypp"
    { (yyval.n) = ast_mod_assign; ;}
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 638 "glsl_parser.ypp"
    { (yyval.n) = ast_add_assign; ;}
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 639 "glsl_parser.ypp"
    { (yyval.n) = ast_sub_assign; ;}
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 640 "glsl_parser.ypp"
    { (yyval.n) = ast_ls_assign; ;}
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 641 "glsl_parser.ypp"
    { (yyval.n) = ast_rs_assign; ;}
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 642 "glsl_parser.ypp"
    { (yyval.n) = ast_and_assign; ;}
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 643 "glsl_parser.ypp"
    { (yyval.n) = ast_xor_assign; ;}
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 644 "glsl_parser.ypp"
    { (yyval.n) = ast_or_assign; ;}
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 649 "glsl_parser.ypp"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 653 "glsl_parser.ypp"
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

  case 94:

/* Line 1464 of yacc.c  */
#line 673 "glsl_parser.ypp"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].function);
	;}
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 677 "glsl_parser.ypp"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
	;}
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 681 "glsl_parser.ypp"
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

  case 100:

/* Line 1464 of yacc.c  */
#line 704 "glsl_parser.ypp"
    {
	   (yyval.function) = (yyvsp[(1) - (2)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
	;}
    break;

  case 101:

/* Line 1464 of yacc.c  */
#line 709 "glsl_parser.ypp"
    {
	   (yyval.function) = (yyvsp[(1) - (3)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
	;}
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 717 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.function) = new(ctx) ast_function();
	   (yyval.function)->set_location(yylloc);
	   (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
	   (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);
	;}
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 728 "glsl_parser.ypp"
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

  case 104:

/* Line 1464 of yacc.c  */
#line 738 "glsl_parser.ypp"
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

  case 105:

/* Line 1464 of yacc.c  */
#line 753 "glsl_parser.ypp"
    {
	   (yyvsp[(1) - (3)].type_qualifier).i |= (yyvsp[(2) - (3)].type_qualifier).i;

	   (yyval.parameter_declarator) = (yyvsp[(3) - (3)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier).q;
	;}
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 760 "glsl_parser.ypp"
    {
	   (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier).q;
	;}
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 765 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyvsp[(1) - (3)].type_qualifier).i |= (yyvsp[(2) - (3)].type_qualifier).i;

	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier).q;
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(3) - (3)].type_specifier);
	;}
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 776 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier).q;
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 787 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; ;}
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 788 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.in = 1; ;}
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 789 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.out = 1; ;}
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 790 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.in = 1; (yyval.type_qualifier).q.out = 1; ;}
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 800 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), false, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 809 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), true, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 818 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), true, (yyvsp[(5) - (6)].expression), NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 827 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (7)].identifier), true, NULL, (yyvsp[(7) - (7)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (7)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 836 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (8)].identifier), true, (yyvsp[(5) - (8)].expression), (yyvsp[(8) - (8)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (8)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 845 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), false, NULL, (yyvsp[(5) - (5)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 858 "glsl_parser.ypp"
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

  case 122:

/* Line 1464 of yacc.c  */
#line 869 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 878 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), true, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 887 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), true, (yyvsp[(4) - (5)].expression), NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 896 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (6)].identifier), true, NULL, (yyvsp[(6) - (6)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (6)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 905 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (7)].identifier), true, (yyvsp[(4) - (7)].expression), (yyvsp[(7) - (7)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (7)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 914 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 923 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->invariant = true;

	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 937 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
	;}
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 944 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier).q;
	   (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 954 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; ;}
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 960 "glsl_parser.ypp"
    {
	  (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
	;}
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 968 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier).i = (yyvsp[(1) - (3)].type_qualifier).i | (yyvsp[(3) - (3)].type_qualifier).i;
	;}
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 975 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier).i = 0;

	   if (state->ARB_fragment_coord_conventions_enable) {
	      bool got_one = false;

	      if (strcmp((yyvsp[(1) - (1)].identifier), "origin_upper_left") == 0) {
		 got_one = true;
		 (yyval.type_qualifier).q.origin_upper_left = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "pixel_center_integer") == 0) {
		 got_one = true;
		 (yyval.type_qualifier).q.pixel_center_integer = 1;
	      }

	      if (state->ARB_fragment_coord_conventions_warn && got_one) {
		 _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
				    "GL_ARB_fragment_coord_conventions layout "
				    "identifier `%s' used\n", (yyvsp[(1) - (1)].identifier));
	      }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if ((yyval.type_qualifier).i == 0) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[(1) - (1)].identifier));
	      YYERROR;
	   }
	;}
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 1008 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.smooth = 1; ;}
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 1009 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.flat = 1; ;}
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 1010 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.noperspective = 1; ;}
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 1014 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.constant = 1; ;}
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 1020 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier).i = (yyvsp[(1) - (2)].type_qualifier).i | (yyvsp[(2) - (2)].type_qualifier).i;
	;}
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 1024 "glsl_parser.ypp"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
	   (yyval.type_qualifier).q.invariant = 1;
	;}
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 1031 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.constant = 1; ;}
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 1032 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.attribute = 1; ;}
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 1033 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = (yyvsp[(1) - (2)].type_qualifier).i; (yyval.type_qualifier).q.varying = 1; ;}
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 1034 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.centroid = 1; (yyval.type_qualifier).q.varying = 1; ;}
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 1035 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.in = 1; ;}
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 1036 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.out = 1; ;}
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 1037 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.centroid = 1; (yyval.type_qualifier).q.in = 1; ;}
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 1038 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.centroid = 1; (yyval.type_qualifier).q.out = 1; ;}
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 1039 "glsl_parser.ypp"
    { (yyval.type_qualifier).i = 0; (yyval.type_qualifier).q.uniform = 1; ;}
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 1045 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(2) - (2)].type_specifier);
	   (yyval.type_specifier)->precision = (yyvsp[(1) - (2)].n);
	;}
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 1054 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = NULL;
	;}
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 1060 "glsl_parser.ypp"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (4)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = (yyvsp[(3) - (4)].expression);
	;}
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 1069 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].n));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 1075 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 1081 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 1089 "glsl_parser.ypp"
    { (yyval.n) = ast_void; ;}
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 1090 "glsl_parser.ypp"
    { (yyval.n) = ast_float; ;}
    break;

  case 163:

/* Line 1464 of yacc.c  */
#line 1091 "glsl_parser.ypp"
    { (yyval.n) = ast_int; ;}
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 1092 "glsl_parser.ypp"
    { (yyval.n) = ast_uint; ;}
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 1093 "glsl_parser.ypp"
    { (yyval.n) = ast_bool; ;}
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 1094 "glsl_parser.ypp"
    { (yyval.n) = ast_vec2; ;}
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 1095 "glsl_parser.ypp"
    { (yyval.n) = ast_vec3; ;}
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 1096 "glsl_parser.ypp"
    { (yyval.n) = ast_vec4; ;}
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 1097 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec2; ;}
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 1098 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec3; ;}
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 1099 "glsl_parser.ypp"
    { (yyval.n) = ast_bvec4; ;}
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 1100 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec2; ;}
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 1101 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec3; ;}
    break;

  case 174:

/* Line 1464 of yacc.c  */
#line 1102 "glsl_parser.ypp"
    { (yyval.n) = ast_ivec4; ;}
    break;

  case 175:

/* Line 1464 of yacc.c  */
#line 1103 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec2; ;}
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 1104 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec3; ;}
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 1105 "glsl_parser.ypp"
    { (yyval.n) = ast_uvec4; ;}
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 1106 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2; ;}
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 1107 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2x3; ;}
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 1108 "glsl_parser.ypp"
    { (yyval.n) = ast_mat2x4; ;}
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 1109 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3x2; ;}
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 1110 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3; ;}
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 1111 "glsl_parser.ypp"
    { (yyval.n) = ast_mat3x4; ;}
    break;

  case 184:

/* Line 1464 of yacc.c  */
#line 1112 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4x2; ;}
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 1113 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4x3; ;}
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 1114 "glsl_parser.ypp"
    { (yyval.n) = ast_mat4; ;}
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 1115 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1d; ;}
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 1116 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2d; ;}
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 1117 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2drect; ;}
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 1118 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler3d; ;}
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 1119 "glsl_parser.ypp"
    { (yyval.n) = ast_samplercube; ;}
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 1120 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1dshadow; ;}
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 1121 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2dshadow; ;}
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 1122 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2drectshadow; ;}
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 1123 "glsl_parser.ypp"
    { (yyval.n) = ast_samplercubeshadow; ;}
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 1124 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1darray; ;}
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 1125 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2darray; ;}
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 1126 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler1darrayshadow; ;}
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 1127 "glsl_parser.ypp"
    { (yyval.n) = ast_sampler2darrayshadow; ;}
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 1128 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler1d; ;}
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 1129 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler2d; ;}
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 1130 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler3d; ;}
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 1131 "glsl_parser.ypp"
    { (yyval.n) = ast_isamplercube; ;}
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 1132 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler1darray; ;}
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 1133 "glsl_parser.ypp"
    { (yyval.n) = ast_isampler2darray; ;}
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 1134 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler1d; ;}
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 1135 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler2d; ;}
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 1136 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler3d; ;}
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 1137 "glsl_parser.ypp"
    { (yyval.n) = ast_usamplercube; ;}
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 1138 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler1darray; ;}
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 1139 "glsl_parser.ypp"
    { (yyval.n) = ast_usampler2darray; ;}
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 1143 "glsl_parser.ypp"
    {
			   if (state->language_version < 130)
			      _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					       "precision qualifier forbidden "
					       "in GLSL %d.%d (1.30 or later "
					       "required)\n",
					       state->language_version / 100,
					       state->language_version % 100);

			   (yyval.n) = ast_precision_high;
			;}
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 1154 "glsl_parser.ypp"
    {
			   if (state->language_version < 130)
			      _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					       "precision qualifier forbidden "
					       "in GLSL %d.%d (1.30 or later "
					       "required)\n",
					       state->language_version / 100,
					       state->language_version % 100);

			   (yyval.n) = ast_precision_medium;
			;}
    break;

  case 214:

/* Line 1464 of yacc.c  */
#line 1165 "glsl_parser.ypp"
    {
			   if (state->language_version < 130)
			      _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					       "precision qualifier forbidden "
					       "in GLSL %d.%d (1.30 or later "
					       "required)\n",
					       state->language_version / 100,
					       state->language_version % 100);

			   (yyval.n) = ast_precision_low;
			;}
    break;

  case 215:

/* Line 1464 of yacc.c  */
#line 1180 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].node));
	   (yyval.struct_specifier)->set_location(yylloc);
	;}
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 1186 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[(3) - (4)].node));
	   (yyval.struct_specifier)->set_location(yylloc);
	;}
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 1195 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	;}
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 1200 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
	;}
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 1208 "glsl_parser.ypp"
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

  case 220:

/* Line 1464 of yacc.c  */
#line 1223 "glsl_parser.ypp"
    {
	   (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
	   (yyvsp[(1) - (1)].declaration)->link.self_link();
	;}
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 1228 "glsl_parser.ypp"
    {
	   (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
	   (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
	;}
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 1236 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), false, NULL, NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 1242 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (4)].identifier), true, (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 228:

/* Line 1464 of yacc.c  */
#line 1265 "glsl_parser.ypp"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 1277 "glsl_parser.ypp"
    { (yyval.node) = NULL; ;}
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 1278 "glsl_parser.ypp"
    { (yyval.node) = NULL; ;}
    break;

  case 238:

/* Line 1464 of yacc.c  */
#line 1285 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 1291 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 1299 "glsl_parser.ypp"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 1305 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 1311 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 1320 "glsl_parser.ypp"
    {
	   if ((yyvsp[(1) - (1)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement\n");
	      assert((yyvsp[(1) - (1)].node) != NULL);
	   }

	   (yyval.node) = (yyvsp[(1) - (1)].node);
	   (yyval.node)->link.self_link();
	;}
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 1330 "glsl_parser.ypp"
    {
	   if ((yyvsp[(2) - (2)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement\n");
	      assert((yyvsp[(2) - (2)].node) != NULL);
	   }
	   (yyval.node) = (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 1342 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement(NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 1348 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 1357 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_selection_statement((yyvsp[(3) - (7)].expression), (yyvsp[(5) - (7)].node), (yyvsp[(7) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 1366 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].node), NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 1372 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].node), NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 1378 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_selection_statement((yyvsp[(3) - (7)].expression), (yyvsp[(5) - (7)].node), (yyvsp[(7) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 1387 "glsl_parser.ypp"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 1391 "glsl_parser.ypp"
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

  case 257:

/* Line 1464 of yacc.c  */
#line 1414 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 1421 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 1428 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 1444 "glsl_parser.ypp"
    {
	   (yyval.node) = NULL;
	;}
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 1451 "glsl_parser.ypp"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
	   (yyval.for_rest_statement).rest = NULL;
	;}
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 1456 "glsl_parser.ypp"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
	   (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
	;}
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 1465 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 1471 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 1477 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 1483 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 1489 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 1497 "glsl_parser.ypp"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); ;}
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 1498 "glsl_parser.ypp"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 1503 "glsl_parser.ypp"
    {
	   void *ctx = state;
	   (yyval.function_definition) = new(ctx) ast_function_definition();
	   (yyval.function_definition)->set_location(yylloc);
	   (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
	   (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);
	;}
    break;



/* Line 1464 of yacc.c  */
#line 5039 "glsl_parser.cpp"
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



