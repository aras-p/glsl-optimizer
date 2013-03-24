/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.5"

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

/* Line 268 of yacc.c  */
#line 1 "src/glsl/glsl_parser.yy"

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
#include "main/context.h"

#if defined(_MSC_VER)
#	pragma warning(disable: 4065) // warning C4065: switch statement contains 'default' but no 'case' labels
#	pragma warning(disable: 4244) // warning C4244: '=' : conversion from 'double' to 'float', possible loss of data
#endif // defined(_MSC_VER)

#define YYLEX_PARAM state->scanner

#undef yyerror

static void yyerror(YYLTYPE *loc, _mesa_glsl_parse_state *st, const char *msg)
{
   _mesa_glsl_error(loc, st, "%s", msg);
}


/* Line 268 of yacc.c  */
#line 128 "src/glsl/glsl_parser.cpp"

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
     SAMPLER2DRECT = 328,
     ISAMPLER2DRECT = 329,
     USAMPLER2DRECT = 330,
     SAMPLER2DRECTSHADOW = 331,
     SAMPLERBUFFER = 332,
     ISAMPLERBUFFER = 333,
     USAMPLERBUFFER = 334,
     SAMPLEREXTERNALOES = 335,
     STRUCT = 336,
     VOID_TOK = 337,
     WHILE = 338,
     IDENTIFIER = 339,
     TYPE_IDENTIFIER = 340,
     NEW_IDENTIFIER = 341,
     FLOATCONSTANT = 342,
     INTCONSTANT = 343,
     UINTCONSTANT = 344,
     BOOLCONSTANT = 345,
     FIELD_SELECTION = 346,
     LEFT_OP = 347,
     RIGHT_OP = 348,
     INC_OP = 349,
     DEC_OP = 350,
     LE_OP = 351,
     GE_OP = 352,
     EQ_OP = 353,
     NE_OP = 354,
     AND_OP = 355,
     OR_OP = 356,
     XOR_OP = 357,
     MUL_ASSIGN = 358,
     DIV_ASSIGN = 359,
     ADD_ASSIGN = 360,
     MOD_ASSIGN = 361,
     LEFT_ASSIGN = 362,
     RIGHT_ASSIGN = 363,
     AND_ASSIGN = 364,
     XOR_ASSIGN = 365,
     OR_ASSIGN = 366,
     SUB_ASSIGN = 367,
     INVARIANT = 368,
     LOWP = 369,
     MEDIUMP = 370,
     HIGHP = 371,
     SUPERP = 372,
     PRECISION = 373,
     VERSION_TOK = 374,
     EXTENSION = 375,
     LINE = 376,
     COLON = 377,
     EOL = 378,
     INTERFACE = 379,
     OUTPUT = 380,
     PRAGMA_DEBUG_ON = 381,
     PRAGMA_DEBUG_OFF = 382,
     PRAGMA_OPTIMIZE_ON = 383,
     PRAGMA_OPTIMIZE_OFF = 384,
     PRAGMA_INVARIANT_ALL = 385,
     LAYOUT_TOK = 386,
     ASM = 387,
     CLASS = 388,
     UNION = 389,
     ENUM = 390,
     TYPEDEF = 391,
     TEMPLATE = 392,
     THIS = 393,
     PACKED_TOK = 394,
     GOTO = 395,
     INLINE_TOK = 396,
     NOINLINE = 397,
     VOLATILE = 398,
     PUBLIC_TOK = 399,
     STATIC = 400,
     EXTERN = 401,
     EXTERNAL = 402,
     LONG_TOK = 403,
     SHORT_TOK = 404,
     DOUBLE_TOK = 405,
     HALF = 406,
     FIXED_TOK = 407,
     UNSIGNED = 408,
     INPUT_TOK = 409,
     OUPTUT = 410,
     HVEC2 = 411,
     HVEC3 = 412,
     HVEC4 = 413,
     DVEC2 = 414,
     DVEC3 = 415,
     DVEC4 = 416,
     FVEC2 = 417,
     FVEC3 = 418,
     FVEC4 = 419,
     SAMPLER3DRECT = 420,
     SIZEOF = 421,
     CAST = 422,
     NAMESPACE = 423,
     USING = 424,
     ERROR_TOK = 425,
     COMMON = 426,
     PARTITION = 427,
     ACTIVE = 428,
     FILTER = 429,
     IMAGE1D = 430,
     IMAGE2D = 431,
     IMAGE3D = 432,
     IMAGECUBE = 433,
     IMAGE1DARRAY = 434,
     IMAGE2DARRAY = 435,
     IIMAGE1D = 436,
     IIMAGE2D = 437,
     IIMAGE3D = 438,
     IIMAGECUBE = 439,
     IIMAGE1DARRAY = 440,
     IIMAGE2DARRAY = 441,
     UIMAGE1D = 442,
     UIMAGE2D = 443,
     UIMAGE3D = 444,
     UIMAGECUBE = 445,
     UIMAGE1DARRAY = 446,
     UIMAGE2DARRAY = 447,
     IMAGE1DSHADOW = 448,
     IMAGE2DSHADOW = 449,
     IMAGEBUFFER = 450,
     IIMAGEBUFFER = 451,
     UIMAGEBUFFER = 452,
     IMAGE1DARRAYSHADOW = 453,
     IMAGE2DARRAYSHADOW = 454,
     ROW_MAJOR = 455
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 64 "src/glsl/glsl_parser.yy"

   int n;
   float real;
   const char *identifier;

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
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;



/* Line 293 of yacc.c  */
#line 401 "src/glsl/glsl_parser.cpp"
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


/* Line 343 of yacc.c  */
#line 426 "src/glsl/glsl_parser.cpp"

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
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
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3101

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  225
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  103
/* YYNRULES -- Number of rules.  */
#define YYNRULES  318
/* YYNRULES -- Number of states.  */
#define YYNSTATES  483

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   455

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   209,     2,     2,     2,   213,   216,     2,
     201,   202,   211,   207,   206,   208,   205,   212,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   220,   222,
     214,   221,   215,   219,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   203,     2,   204,   217,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   223,   218,   224,   210,     2,     2,     2,
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
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    17,    20,    23,
      26,    29,    30,    33,    35,    37,    39,    45,    47,    50,
      52,    54,    56,    58,    60,    62,    64,    68,    70,    75,
      77,    81,    84,    87,    89,    91,    93,    97,   100,   103,
     106,   108,   111,   115,   118,   120,   122,   124,   127,   130,
     133,   135,   138,   142,   145,   147,   150,   153,   156,   158,
     160,   162,   164,   166,   170,   174,   178,   180,   184,   188,
     190,   194,   198,   200,   204,   208,   212,   216,   218,   222,
     226,   228,   232,   234,   238,   240,   244,   246,   250,   252,
     256,   258,   262,   264,   270,   272,   276,   278,   280,   282,
     284,   286,   288,   290,   292,   294,   296,   298,   300,   304,
     306,   309,   312,   317,   319,   322,   324,   326,   329,   333,
     337,   340,   346,   350,   353,   357,   360,   361,   363,   365,
     367,   369,   371,   375,   381,   388,   396,   405,   411,   413,
     416,   421,   427,   434,   442,   447,   450,   452,   455,   460,
     462,   466,   468,   472,   474,   476,   478,   480,   482,   484,
     486,   488,   490,   493,   495,   498,   501,   505,   507,   509,
     511,   513,   516,   518,   520,   523,   526,   528,   530,   533,
     535,   539,   544,   546,   548,   550,   552,   554,   556,   558,
     560,   562,   564,   566,   568,   570,   572,   574,   576,   578,
     580,   582,   584,   586,   588,   590,   592,   594,   596,   598,
     600,   602,   604,   606,   608,   610,   612,   614,   616,   618,
     620,   622,   624,   626,   628,   630,   632,   634,   636,   638,
     640,   642,   644,   646,   648,   650,   652,   654,   656,   658,
     660,   662,   664,   666,   668,   670,   676,   681,   683,   686,
     690,   692,   696,   698,   703,   705,   707,   709,   711,   713,
     715,   717,   719,   721,   723,   726,   727,   732,   734,   736,
     739,   743,   745,   748,   750,   753,   759,   763,   765,   767,
     772,   778,   781,   785,   789,   792,   794,   797,   800,   803,
     805,   808,   814,   822,   829,   831,   833,   835,   836,   839,
     843,   846,   849,   852,   856,   859,   861,   863,   865,   867,
     870,   877,   885,   887,   890,   891,   893,   899,   904
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     226,     0,    -1,    -1,   228,   230,   227,   233,    -1,    -1,
     119,    88,   123,    -1,   126,   123,    -1,   127,   123,    -1,
     128,   123,    -1,   129,   123,    -1,   130,   123,    -1,    -1,
     230,   232,    -1,    84,    -1,    85,    -1,    86,    -1,   120,
     231,   122,   231,   123,    -1,   321,    -1,   233,   321,    -1,
      84,    -1,    86,    -1,   234,    -1,    88,    -1,    89,    -1,
      87,    -1,    90,    -1,   201,   265,   202,    -1,   235,    -1,
     236,   203,   237,   204,    -1,   238,    -1,   236,   205,   231,
      -1,   236,    94,    -1,   236,    95,    -1,   265,    -1,   239,
      -1,   240,    -1,   236,   205,   245,    -1,   242,   202,    -1,
     241,   202,    -1,   243,    82,    -1,   243,    -1,   243,   263,
      -1,   242,   206,   263,    -1,   244,   201,    -1,   287,    -1,
     234,    -1,    91,    -1,   247,   202,    -1,   246,   202,    -1,
     248,    82,    -1,   248,    -1,   248,   263,    -1,   247,   206,
     263,    -1,   234,   201,    -1,   236,    -1,    94,   249,    -1,
      95,   249,    -1,   250,   249,    -1,   207,    -1,   208,    -1,
     209,    -1,   210,    -1,   249,    -1,   251,   211,   249,    -1,
     251,   212,   249,    -1,   251,   213,   249,    -1,   251,    -1,
     252,   207,   251,    -1,   252,   208,   251,    -1,   252,    -1,
     253,    92,   252,    -1,   253,    93,   252,    -1,   253,    -1,
     254,   214,   253,    -1,   254,   215,   253,    -1,   254,    96,
     253,    -1,   254,    97,   253,    -1,   254,    -1,   255,    98,
     254,    -1,   255,    99,   254,    -1,   255,    -1,   256,   216,
     255,    -1,   256,    -1,   257,   217,   256,    -1,   257,    -1,
     258,   218,   257,    -1,   258,    -1,   259,   100,   258,    -1,
     259,    -1,   260,   102,   259,    -1,   260,    -1,   261,   101,
     260,    -1,   261,    -1,   261,   219,   265,   220,   263,    -1,
     262,    -1,   249,   264,   263,    -1,   221,    -1,   103,    -1,
     104,    -1,   106,    -1,   105,    -1,   112,    -1,   107,    -1,
     108,    -1,   109,    -1,   110,    -1,   111,    -1,   263,    -1,
     265,   206,   263,    -1,   262,    -1,   268,   222,    -1,   276,
     222,    -1,   118,   291,   288,   222,    -1,   323,    -1,   269,
     202,    -1,   271,    -1,   270,    -1,   271,   273,    -1,   270,
     206,   273,    -1,   278,   234,   201,    -1,   287,   231,    -1,
     287,   231,   203,   266,   204,    -1,   284,   274,   272,    -1,
     274,   272,    -1,   284,   274,   275,    -1,   274,   275,    -1,
      -1,    33,    -1,    34,    -1,    35,    -1,   287,    -1,   277,
      -1,   276,   206,   231,    -1,   276,   206,   231,   203,   204,
      -1,   276,   206,   231,   203,   266,   204,    -1,   276,   206,
     231,   203,   204,   221,   297,    -1,   276,   206,   231,   203,
     266,   204,   221,   297,    -1,   276,   206,   231,   221,   297,
      -1,   278,    -1,   278,   231,    -1,   278,   231,   203,   204,
      -1,   278,   231,   203,   266,   204,    -1,   278,   231,   203,
     204,   221,   297,    -1,   278,   231,   203,   266,   204,   221,
     297,    -1,   278,   231,   221,   297,    -1,   113,   234,    -1,
     287,    -1,   285,   287,    -1,   131,   201,   280,   202,    -1,
     281,    -1,   280,   206,   281,    -1,   231,    -1,   231,   221,
      88,    -1,   282,    -1,   200,    -1,   139,    -1,    40,    -1,
      39,    -1,    38,    -1,     4,    -1,   286,    -1,   279,    -1,
     279,   286,    -1,   283,    -1,   283,   286,    -1,   113,   286,
      -1,   113,   283,   286,    -1,   113,    -1,     4,    -1,     3,
      -1,    37,    -1,    32,    37,    -1,    33,    -1,    34,    -1,
      32,    33,    -1,    32,    34,    -1,    36,    -1,   288,    -1,
     291,   288,    -1,   289,    -1,   289,   203,   204,    -1,   289,
     203,   266,   204,    -1,   290,    -1,   292,    -1,    85,    -1,
      82,    -1,     6,    -1,     7,    -1,     8,    -1,     5,    -1,
      29,    -1,    30,    -1,    31,    -1,    20,    -1,    21,    -1,
      22,    -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,
      27,    -1,    28,    -1,    41,    -1,    42,    -1,    43,    -1,
      44,    -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,
      49,    -1,    50,    -1,    51,    -1,    73,    -1,    52,    -1,
      53,    -1,    80,    -1,    54,    -1,    55,    -1,    76,    -1,
      56,    -1,    57,    -1,    58,    -1,    59,    -1,    60,    -1,
      77,    -1,    61,    -1,    62,    -1,    74,    -1,    63,    -1,
      64,    -1,    65,    -1,    66,    -1,    78,    -1,    67,    -1,
      68,    -1,    75,    -1,    69,    -1,    70,    -1,    71,    -1,
      72,    -1,    79,    -1,   116,    -1,   115,    -1,   114,    -1,
      81,   231,   223,   293,   224,    -1,    81,   223,   293,   224,
      -1,   294,    -1,   293,   294,    -1,   287,   295,   222,    -1,
     296,    -1,   295,   206,   296,    -1,   231,    -1,   231,   203,
     266,   204,    -1,   263,    -1,   267,    -1,   301,    -1,   300,
      -1,   298,    -1,   306,    -1,   307,    -1,   310,    -1,   316,
      -1,   320,    -1,   223,   224,    -1,    -1,   223,   302,   305,
     224,    -1,   304,    -1,   300,    -1,   223,   224,    -1,   223,
     305,   224,    -1,   299,    -1,   305,   299,    -1,   222,    -1,
     265,   222,    -1,    14,   201,   265,   202,   308,    -1,   299,
      12,   299,    -1,   299,    -1,   265,    -1,   278,   231,   221,
     297,    -1,    17,   201,   265,   202,   311,    -1,   223,   224,
      -1,   223,   315,   224,    -1,    18,   265,   220,    -1,    19,
     220,    -1,   312,    -1,   313,   312,    -1,   313,   299,    -1,
     314,   299,    -1,   314,    -1,   315,   314,    -1,    83,   201,
     309,   202,   303,    -1,    11,   299,    83,   201,   265,   202,
     222,    -1,    13,   201,   317,   319,   202,   303,    -1,   306,
      -1,   298,    -1,   309,    -1,    -1,   318,   222,    -1,   318,
     222,   265,    -1,    10,   222,    -1,     9,   222,    -1,    16,
     222,    -1,    16,   265,   222,    -1,    15,   222,    -1,   322,
      -1,   267,    -1,   229,    -1,   327,    -1,   268,   304,    -1,
      36,    86,   223,   324,   224,   222,    -1,   279,    36,    86,
     223,   324,   224,   222,    -1,   326,    -1,   326,   324,    -1,
      -1,    36,    -1,   279,   325,   287,   295,   222,    -1,   325,
     287,   295,   222,    -1,   279,    36,   222,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   248,   248,   247,   259,   261,   314,   315,   316,   317,
     318,   330,   332,   336,   337,   338,   342,   351,   359,   370,
     371,   375,   382,   389,   396,   403,   410,   417,   418,   424,
     428,   435,   441,   450,   454,   458,   459,   468,   469,   473,
     474,   478,   484,   496,   500,   506,   513,   523,   524,   528,
     529,   533,   539,   551,   562,   563,   569,   575,   585,   586,
     587,   588,   592,   593,   599,   605,   614,   615,   621,   630,
     631,   637,   646,   647,   653,   659,   665,   674,   675,   681,
     690,   691,   700,   701,   710,   711,   720,   721,   730,   731,
     740,   741,   750,   751,   760,   761,   770,   771,   772,   773,
     774,   775,   776,   777,   778,   779,   780,   784,   788,   804,
     808,   813,   817,   823,   830,   834,   835,   839,   844,   852,
     866,   876,   891,   898,   903,   914,   927,   930,   935,   940,
     949,   953,   954,   964,   974,   984,   994,  1004,  1018,  1025,
    1034,  1043,  1052,  1061,  1070,  1079,  1093,  1100,  1111,  1118,
    1119,  1129,  1197,  1243,  1265,  1270,  1278,  1283,  1288,  1296,
    1304,  1305,  1306,  1311,  1312,  1317,  1322,  1328,  1336,  1341,
    1346,  1351,  1357,  1362,  1367,  1372,  1377,  1385,  1389,  1397,
    1398,  1404,  1413,  1419,  1425,  1434,  1435,  1436,  1437,  1438,
    1439,  1440,  1441,  1442,  1443,  1444,  1445,  1446,  1447,  1448,
    1449,  1450,  1451,  1452,  1453,  1454,  1455,  1456,  1457,  1458,
    1459,  1460,  1461,  1462,  1463,  1464,  1465,  1466,  1467,  1468,
    1469,  1470,  1471,  1472,  1473,  1474,  1475,  1476,  1477,  1478,
    1479,  1480,  1481,  1482,  1483,  1484,  1485,  1486,  1487,  1488,
    1489,  1490,  1494,  1504,  1514,  1527,  1534,  1543,  1548,  1556,
    1571,  1576,  1584,  1591,  1600,  1604,  1610,  1611,  1615,  1616,
    1617,  1618,  1619,  1620,  1624,  1631,  1630,  1644,  1645,  1649,
    1655,  1664,  1674,  1686,  1692,  1701,  1710,  1715,  1723,  1727,
    1745,  1753,  1758,  1766,  1771,  1779,  1787,  1795,  1803,  1811,
    1819,  1827,  1834,  1841,  1851,  1852,  1856,  1858,  1864,  1869,
    1878,  1884,  1890,  1896,  1902,  1911,  1912,  1913,  1914,  1918,
    1932,  1948,  1971,  1976,  1984,  1986,  1990,  2005,  2022
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
  "USAMPLER2DARRAY", "SAMPLER2DRECT", "ISAMPLER2DRECT", "USAMPLER2DRECT",
  "SAMPLER2DRECTSHADOW", "SAMPLERBUFFER", "ISAMPLERBUFFER",
  "USAMPLERBUFFER", "SAMPLEREXTERNALOES", "STRUCT", "VOID_TOK", "WHILE",
  "IDENTIFIER", "TYPE_IDENTIFIER", "NEW_IDENTIFIER", "FLOATCONSTANT",
  "INTCONSTANT", "UINTCONSTANT", "BOOLCONSTANT", "FIELD_SELECTION",
  "LEFT_OP", "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP",
  "NE_OP", "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN",
  "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN",
  "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT", "LOWP", "MEDIUMP",
  "HIGHP", "SUPERP", "PRECISION", "VERSION_TOK", "EXTENSION", "LINE",
  "COLON", "EOL", "INTERFACE", "OUTPUT", "PRAGMA_DEBUG_ON",
  "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON", "PRAGMA_OPTIMIZE_OFF",
  "PRAGMA_INVARIANT_ALL", "LAYOUT_TOK", "ASM", "CLASS", "UNION", "ENUM",
  "TYPEDEF", "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK",
  "NOINLINE", "VOLATILE", "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL",
  "LONG_TOK", "SHORT_TOK", "DOUBLE_TOK", "HALF", "FIXED_TOK", "UNSIGNED",
  "INPUT_TOK", "OUPTUT", "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3",
  "DVEC4", "FVEC2", "FVEC3", "FVEC4", "SAMPLER3DRECT", "SIZEOF", "CAST",
  "NAMESPACE", "USING", "ERROR_TOK", "COMMON", "PARTITION", "ACTIVE",
  "FILTER", "IMAGE1D", "IMAGE2D", "IMAGE3D", "IMAGECUBE", "IMAGE1DARRAY",
  "IMAGE2DARRAY", "IIMAGE1D", "IIMAGE2D", "IIMAGE3D", "IIMAGECUBE",
  "IIMAGE1DARRAY", "IIMAGE2DARRAY", "UIMAGE1D", "UIMAGE2D", "UIMAGE3D",
  "UIMAGECUBE", "UIMAGE1DARRAY", "UIMAGE2DARRAY", "IMAGE1DSHADOW",
  "IMAGE2DSHADOW", "IMAGEBUFFER", "IIMAGEBUFFER", "UIMAGEBUFFER",
  "IMAGE1DARRAYSHADOW", "IMAGE2DARRAYSHADOW", "ROW_MAJOR", "'('", "')'",
  "'['", "']'", "'.'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'",
  "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'",
  "'{'", "'}'", "$accept", "translation_unit", "$@1", "version_statement",
  "pragma_statement", "extension_statement_list", "any_identifier",
  "extension_statement", "external_declaration_list",
  "variable_identifier", "primary_expression", "postfix_expression",
  "integer_expression", "function_call", "function_call_or_method",
  "function_call_generic", "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "method_call_generic",
  "method_call_header_no_parameters", "method_call_header_with_parameters",
  "method_call_header", "unary_expression", "unary_operator",
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
  "uniform_block_layout_qualifier", "interpolation_qualifier",
  "parameter_type_qualifier", "type_qualifier", "storage_qualifier",
  "type_specifier", "type_specifier_no_prec", "type_specifier_nonarray",
  "basic_type_specifier_nonarray", "precision_qualifier",
  "struct_specifier", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "$@2", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "switch_body", "case_label", "case_label_list", "case_statement",
  "case_statement_list", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "external_declaration", "function_definition", "uniform_block",
  "member_list", "uniformopt", "member_declaration", "layout_defaults", 0
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
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,    40,    41,    91,    93,    46,    44,    43,    45,    33,
     126,    42,    47,    37,    60,    62,    38,    94,   124,    63,
      58,    61,    59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   225,   227,   226,   228,   228,   229,   229,   229,   229,
     229,   230,   230,   231,   231,   231,   232,   233,   233,   234,
     234,   235,   235,   235,   235,   235,   235,   236,   236,   236,
     236,   236,   236,   237,   238,   239,   239,   240,   240,   241,
     241,   242,   242,   243,   244,   244,   244,   245,   245,   246,
     246,   247,   247,   248,   249,   249,   249,   249,   250,   250,
     250,   250,   251,   251,   251,   251,   252,   252,   252,   253,
     253,   253,   254,   254,   254,   254,   254,   255,   255,   255,
     256,   256,   257,   257,   258,   258,   259,   259,   260,   260,
     261,   261,   262,   262,   263,   263,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   265,   265,   266,
     267,   267,   267,   267,   268,   269,   269,   270,   270,   271,
     272,   272,   273,   273,   273,   273,   274,   274,   274,   274,
     275,   276,   276,   276,   276,   276,   276,   276,   277,   277,
     277,   277,   277,   277,   277,   277,   278,   278,   279,   280,
     280,   281,   281,   281,   282,   282,   283,   283,   283,   284,
     285,   285,   285,   285,   285,   285,   285,   285,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   287,   287,   288,
     288,   288,   289,   289,   289,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   290,   290,   290,   290,   290,   290,   290,   290,
     290,   290,   291,   291,   291,   292,   292,   293,   293,   294,
     295,   295,   296,   296,   297,   298,   299,   299,   300,   300,
     300,   300,   300,   300,   301,   302,   301,   303,   303,   304,
     304,   305,   305,   306,   306,   307,   308,   308,   309,   309,
     310,   311,   311,   312,   312,   313,   313,   314,   314,   315,
     315,   316,   316,   316,   317,   317,   318,   318,   319,   319,
     320,   320,   320,   320,   320,   321,   321,   321,   321,   322,
     323,   323,   324,   324,   325,   325,   326,   326,   327
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     2,     2,     2,     2,
       2,     0,     2,     1,     1,     1,     5,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     4,     1,
       3,     2,     2,     1,     1,     1,     3,     2,     2,     2,
       1,     2,     3,     2,     1,     1,     1,     2,     2,     2,
       1,     2,     3,     2,     1,     2,     2,     2,     1,     1,
       1,     1,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     5,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       2,     2,     4,     1,     2,     1,     1,     2,     3,     3,
       2,     5,     3,     2,     3,     2,     0,     1,     1,     1,
       1,     1,     3,     5,     6,     7,     8,     5,     1,     2,
       4,     5,     6,     7,     4,     2,     1,     2,     4,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     3,     1,     1,     1,
       1,     2,     1,     1,     2,     2,     1,     1,     2,     1,
       3,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     5,     4,     1,     2,     3,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     0,     4,     1,     1,     2,
       3,     1,     2,     1,     2,     5,     3,     1,     1,     4,
       5,     2,     3,     3,     2,     1,     2,     2,     2,     1,
       2,     5,     7,     6,     1,     1,     1,     0,     2,     3,
       2,     2,     2,     3,     2,     1,     1,     1,     1,     2,
       6,     7,     1,     2,     0,     1,     5,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    11,     0,     1,     2,     5,     0,     0,
      12,    13,    14,    15,     0,   169,   168,   189,   186,   187,
     188,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     190,   191,   192,     0,   172,   173,   176,   170,   158,   157,
     156,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   214,   215,   217,   218,   220,   221,   222,   223,
     224,   226,   227,   229,   230,   231,   232,   234,   235,   237,
     238,   239,   240,   213,   228,   236,   219,   225,   233,   241,
     216,     0,   185,   184,   167,   244,   243,   242,     0,     0,
       0,     0,     0,     0,     0,   307,     3,   306,     0,     0,
     116,   126,     0,   131,   138,   161,   163,     0,   160,   146,
     177,   179,   182,     0,   183,    17,   305,   113,   308,     0,
     174,   175,   171,     0,     0,     0,   176,    19,    20,   145,
       0,   165,     0,     6,     7,     8,     9,    10,     0,    18,
     110,     0,   309,   114,   126,   159,   127,   128,   129,   117,
       0,   126,     0,   111,    13,    15,   139,     0,   176,   162,
     164,   147,     0,   178,     0,   314,     0,     0,   247,     0,
     166,     0,   155,   154,   151,     0,   149,   153,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    24,    22,    23,
      25,    46,     0,     0,     0,    58,    59,    60,    61,   273,
     265,   269,    21,    27,    54,    29,    34,    35,     0,     0,
      40,     0,    62,     0,    66,    69,    72,    77,    80,    82,
      84,    86,    88,    90,    92,    94,   107,     0,   255,     0,
     161,   146,   258,   271,   257,   256,     0,   259,   260,   261,
     262,   263,   118,   123,   125,   130,     0,   132,     0,     0,
     119,     0,   318,   180,    62,   109,     0,    44,    16,   315,
     314,     0,     0,   314,   252,     0,   250,   246,   248,     0,
     112,     0,   148,     0,   301,   300,     0,     0,     0,   304,
     302,     0,     0,     0,    55,    56,     0,   264,     0,    31,
      32,     0,     0,    38,    37,     0,   185,    41,    43,    97,
      98,   100,    99,   102,   103,   104,   105,   106,   101,    96,
       0,    57,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   274,   176,   270,   272,   120,   122,   124,
       0,     0,   140,     0,   254,   144,   314,   181,     0,     0,
       0,   313,     0,     0,   249,   245,   152,   150,     0,   295,
     294,   297,     0,   303,     0,   167,   278,     0,   161,     0,
      26,     0,     0,    33,    30,     0,    36,     0,     0,    50,
      42,    95,    63,    64,    65,    67,    68,    70,    71,    75,
      76,    73,    74,    78,    79,    81,    83,    85,    87,    89,
      91,     0,   108,     0,   133,     0,   137,     0,   141,     0,
       0,   310,     0,     0,   251,     0,   296,     0,     0,     0,
       0,     0,     0,   266,    28,    53,    48,    47,     0,   185,
      51,     0,     0,     0,   134,   142,     0,     0,     0,   317,
     253,     0,   298,     0,   277,   275,     0,   280,     0,   268,
     291,   267,    52,    93,   121,   135,     0,   143,   311,   316,
       0,   299,   293,     0,     0,     0,   281,   285,     0,   289,
       0,   279,   136,   292,   276,     0,   284,   287,   286,   288,
     282,   290,   283
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     9,     3,    95,     6,   264,    10,    96,   202,
     203,   204,   372,   205,   206,   207,   208,   209,   210,   211,
     376,   377,   378,   379,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   310,
     227,   256,   228,   229,    99,   100,   101,   243,   149,   150,
     244,   102,   103,   104,   230,   175,   176,   177,   106,   151,
     107,   108,   257,   110,   111,   112,   113,   114,   167,   168,
     265,   266,   345,   232,   233,   234,   235,   288,   450,   451,
     236,   237,   238,   445,   369,   239,   447,   467,   468,   469,
     470,   240,   361,   417,   418,   241,   115,   116,   117,   261,
     262,   263,   118
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -373
static const yytype_int16 yypact[] =
{
     -75,   -37,    53,  -373,   -50,  -373,   -19,  -373,   160,  2970,
    -373,  -373,  -373,  -373,   -36,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,   130,  -373,  -373,    28,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,   -58,  -373,  -373,    43,  -373,  -373,  -373,   147,     0,
      17,    20,    26,    46,   -26,  -373,  2970,  -373,  -166,   -15,
      -8,     2,  -128,  -373,   196,   173,   198,  1003,  -373,  -373,
    -373,     5,  -373,  1225,  -373,  -373,  -373,  -373,  -373,   160,
    -373,  -373,  -373,   -20,  1003,    10,  -373,  -373,  -373,  -373,
     198,  -373,  1225,  -373,  -373,  -373,  -373,  -373,   -54,  -373,
    -373,   460,  -373,  -373,   100,  -373,  -373,  -373,  -373,  -373,
    1003,   181,   160,  -373,    19,    35,  -162,    38,   -78,  -373,
    -373,  -373,  2180,  -373,    95,    -3,   160,   559,  -373,  1003,
    -373,    42,  -373,  -373,    21,  -157,  -373,  -373,    47,    49,
    1347,    40,    64,    50,  1875,    75,    90,  -373,  -373,  -373,
    -373,  -373,  2568,  2568,  2568,  -373,  -373,  -373,  -373,  -373,
      60,  -373,    96,  -373,   -52,  -373,  -373,  -373,    48,  -114,
    2759,    98,   -42,  2568,    83,   -53,   101,   -76,   113,    84,
      85,    86,   201,   204,   -89,  -373,  -373,  -122,  -373,    81,
     222,   106,  -373,  -373,  -373,  -373,   682,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,   160,  1003,  -149,  2280,  2568,
    -373,    87,  -373,  -373,  -373,  -373,   104,  -373,  -373,  -373,
     273,    91,  1003,   -25,   108,  -113,  -373,  -373,  -373,   781,
    -373,   226,  -373,   -54,  -373,  -373,   252,  1776,  2568,  -373,
    -373,  -111,  2568,  2083,  -373,  -373,   -45,  -373,  1347,  -373,
    -373,  2568,   196,  -373,  -373,  2568,   134,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    2568,  -373,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,
    2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,  2568,
    2568,  2568,  2568,  -373,   251,  -373,  -373,   135,  -373,  -373,
    2471,  2568,   129,   149,  -373,  -373,    -3,  -373,  1003,   132,
     160,  -373,  2568,   160,  -373,  -373,  -373,  -373,   150,  -373,
    -373,  2083,   -34,  -373,   -24,   309,   151,   160,   198,   156,
    -373,   904,   155,   151,  -373,   162,  -373,   159,   -22,  2856,
    -373,  -373,  -373,  -373,  -373,    83,    83,   -53,   -53,   101,
     101,   101,   101,   -76,   -76,   113,    84,    85,    86,   201,
     204,  -107,  -373,  2568,   143,   161,  -373,  2568,   145,   144,
     160,  -373,   -86,   163,  -373,  2568,  -373,   148,   167,  1347,
     152,   153,  1568,  -373,  -373,  -373,  -373,  -373,  2568,   169,
    -373,  2568,   168,  2568,   157,  -373,  2568,   154,   -80,  -373,
    -373,   -21,  2568,  1568,   361,  -373,    -5,  -373,  2568,  -373,
    -373,  -373,  -373,  -373,  -373,  -373,  2568,  -373,  -373,  -373,
     158,   151,  -373,  1347,  2568,   164,  -373,  -373,  1126,  1347,
      -2,  -373,  -373,  -373,  -373,  -101,  -373,  -373,  -373,  -373,
    -373,  1347,  -373
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -373,  -373,  -373,  -373,  -373,  -373,    -7,  -373,  -373,   -79,
    -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,  -373,
    -373,  -373,  -373,  -373,     4,  -373,   -92,   -70,  -130,   -95,
      52,    55,    56,    51,    57,    58,  -373,  -152,  -158,  -373,
    -175,  -230,     6,    29,  -373,  -373,  -373,   136,   241,   236,
     146,  -373,  -373,  -243,    -6,  -373,   116,  -373,   -77,  -373,
    -373,   -82,    -9,   -74,  -373,  -373,   302,  -373,   224,  -145,
    -321,    41,  -286,   114,  -176,  -372,  -373,  -373,   -48,   298,
     109,   121,  -373,  -373,    39,  -373,  -373,   -69,  -373,   -68,
    -373,  -373,  -373,  -373,  -373,  -373,   305,  -373,  -373,  -229,
     165,  -373,  -373
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -313
static const yytype_int16 yytable[] =
{
     109,    14,   131,   105,   276,   129,   145,   130,   251,   281,
     255,   259,   330,   464,   465,    97,   464,   465,   343,   286,
     319,   320,   268,   159,   160,   157,    11,    12,    13,   412,
      11,    12,    13,   259,   351,   146,   147,   148,    98,   163,
     367,   248,   289,   290,     1,   272,    15,    16,   170,   273,
     449,     4,   297,     5,   340,   406,   140,   141,   171,   249,
     336,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   449,   341,     7,   125,    33,    34,    35,   152,   126,
      37,    38,    39,    40,   332,   172,   119,   109,   294,   438,
     105,   344,   295,   353,   153,   332,   255,   156,   161,   332,
     333,     8,    97,   362,   145,   332,    94,   364,   366,   354,
     405,   363,   164,   431,   123,   166,   373,   409,   367,   482,
     353,   435,   413,   133,   268,    98,   353,   127,    94,   128,
     331,   174,   231,   146,   147,   148,   439,   380,   321,   322,
     134,   245,   459,   135,   252,   247,   173,   455,   159,   136,
     457,   291,   381,   292,   315,   316,   401,   370,   166,   260,
     166,   332,   471,   120,   121,   124,   254,   122,   419,   137,
     472,   231,   332,   432,   402,   138,    15,    16,   420,   309,
     427,   460,   332,   344,   428,   332,   366,   143,   255,   389,
     390,   391,   392,   317,   318,   336,   284,   285,   144,  -312,
     255,    15,    16,   165,  -115,    33,    34,    35,   162,   158,
      37,   323,   324,   375,   146,   147,   148,   311,   258,   466,
     -19,   430,   480,   385,   386,    15,    16,   231,   393,   394,
      33,    34,    35,   169,   126,    37,   -20,   245,   337,   250,
     441,   277,   271,   444,    11,    12,    13,   387,   388,   344,
     293,   255,   254,   350,    33,    34,    35,   260,   334,    37,
     166,    85,    86,    87,   270,   278,   174,   461,   231,   274,
     452,   275,   279,   453,   231,   344,   282,   368,   344,   231,
     154,    12,   155,   131,   287,   374,   159,   474,   130,   475,
     344,   283,   477,   479,   312,   313,   314,   -45,   344,   298,
     325,   328,   326,   140,   327,   479,   329,   -44,   347,   259,
     346,   352,    15,    16,   356,   349,   382,   383,   384,   254,
     254,   254,   254,   254,   254,   254,   254,   254,   254,   254,
     254,   254,   254,   254,   254,   358,   -39,   251,   403,   410,
     260,    33,    34,    35,   254,   126,    37,    38,    39,    40,
     407,   415,   231,   408,   411,   368,   254,   332,   422,   424,
     421,   426,   231,   425,   433,   434,   436,   440,   437,   443,
     442,   -49,   454,   463,   448,   446,   458,   395,   456,   398,
     473,   396,   338,   397,   476,   242,   399,   246,   400,   357,
     132,   359,   339,   269,   414,   462,   142,   371,   360,   478,
     416,   139,   481,     0,     0,     0,     0,   254,     0,     0,
     231,     0,     0,   231,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   348,     0,     0,     0,     0,
       0,     0,     0,     0,   231,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   231,     0,     0,     0,     0,   231,
     231,     0,     0,    15,    16,    17,    18,    19,    20,   178,
     179,   180,   231,   181,   182,   183,   184,   185,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,     0,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,   186,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,     0,     0,     0,    17,    18,    19,    20,     0,     0,
       0,     0,     0,    84,    85,    86,    87,     0,    88,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    94,     0,     0,     0,     0,     0,     0,     0,     0,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,     0,     0,    83,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   194,     0,     0,     0,     0,     0,   195,   196,   197,
     198,     0,     0,    85,    86,    87,     0,     0,     0,     0,
       0,     0,   199,   200,   201,    15,    16,    17,    18,    19,
      20,   178,   179,   180,     0,   181,   182,   183,   184,   185,
       0,     0,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,     0,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,   186,   127,    83,   128,   187,
     188,   189,   190,   191,     0,     0,   192,   193,     0,     0,
       0,     0,     0,   267,     0,     0,    17,    18,    19,    20,
       0,     0,     0,     0,     0,    84,    85,    86,    87,     0,
      88,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    94,     0,     0,     0,     0,     0,     0,
       0,     0,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,     0,     0,    83,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   194,     0,     0,     0,     0,     0,   195,
     196,   197,   198,     0,     0,    85,    86,    87,     0,     0,
       0,     0,     0,     0,   199,   200,   335,    15,    16,    17,
      18,    19,    20,   178,   179,   180,     0,   181,   182,   183,
     184,   185,     0,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     0,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   186,   127,    83,
     128,   187,   188,   189,   190,   191,     0,     0,   192,   193,
       0,     0,     0,     0,     0,   355,     0,     0,    17,    18,
      19,    20,     0,     0,     0,     0,     0,    84,    85,    86,
      87,     0,    88,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    94,     0,     0,     0,     0,
       0,     0,     0,     0,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,     0,     0,    83,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   194,     0,     0,     0,     0,
       0,   195,   196,   197,   198,     0,     0,    85,    86,    87,
       0,     0,     0,     0,     0,     0,   199,   200,   423,    15,
      16,    17,    18,    19,    20,   178,   179,   180,     0,   181,
     182,   183,   184,   185,   464,   465,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,     0,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,   186,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,    19,    20,     0,     0,     0,     0,     0,    84,
      85,    86,    87,     0,    88,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    94,     0,     0,
       0,     0,     0,     0,     0,     0,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,     0,     0,
      83,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   199,   200,
      15,    16,    17,    18,    19,    20,   178,   179,   180,     0,
     181,   182,   183,   184,   185,     0,     0,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,     0,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
     186,   127,    83,   128,   187,   188,   189,   190,   191,     0,
       0,   192,   193,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      84,    85,    86,    87,     0,    88,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    94,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   194,     0,
       0,     0,     0,     0,   195,   196,   197,   198,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   199,
     200,    15,    16,    17,    18,    19,    20,   178,   179,   180,
       0,   181,   182,   183,   184,   185,     0,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,     0,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,   186,   127,    83,   128,   187,   188,   189,   190,   191,
       0,     0,   192,   193,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    84,    85,    86,    87,     0,    88,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    94,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   194,
       0,     0,     0,     0,     0,   195,   196,   197,   198,    15,
      16,    17,    18,    19,    20,     0,     0,     0,     0,     0,
     199,   141,     0,     0,     0,     0,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,     0,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,     0,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
      17,    18,    19,    20,     0,     0,     0,     0,     0,    84,
      85,    86,    87,     0,    88,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    94,     0,     0,
       0,     0,     0,     0,     0,     0,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,     0,   127,
      83,   128,   187,   188,   189,   190,   191,     0,     0,   192,
     193,     0,     0,     0,     0,     0,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,    85,
      86,    87,     0,     0,     0,     0,     0,     0,   199,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   194,     0,     0,     0,
       0,     0,   195,   196,   197,   198,    15,    16,    17,    18,
      19,    20,     0,     0,     0,     0,     0,   280,     0,     0,
       0,     0,     0,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,     0,   126,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,     0,   127,    83,   128,
     187,   188,   189,   190,   191,     0,     0,   192,   193,     0,
       0,     0,     0,     0,     0,    17,    18,    19,    20,     0,
       0,     0,     0,     0,     0,     0,   365,    85,    86,    87,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,     0,     0,    94,     0,     0,     0,     0,     0,
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,     0,     0,     0,   194,    17,    18,    19,    20,     0,
     195,   196,   197,   198,    85,    86,    87,     0,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,   127,    83,   128,   187,   188,   189,
     190,   191,     0,     0,   192,   193,     0,     0,     0,     0,
       0,   194,     0,     0,   253,     0,     0,   195,   196,   197,
     198,     0,     0,     0,    85,    86,    87,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    17,    18,    19,    20,
       0,   194,     0,     0,   342,     0,     0,   195,   196,   197,
     198,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,     0,   127,    83,   128,   187,   188,
     189,   190,   191,     0,     0,   192,   193,     0,     0,     0,
       0,     0,     0,    17,    18,    19,    20,     0,     0,     0,
       0,     0,     0,     0,     0,    85,    86,    87,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,     0,   127,    83,   128,   187,   188,   189,   190,   191,
       0,     0,   192,   193,     0,     0,     0,     0,     0,     0,
       0,     0,   194,     0,     0,   404,     0,     0,   195,   196,
     197,   198,    85,    86,    87,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    17,    18,    19,    20,     0,   194,
       0,     0,     0,     0,     0,   195,   196,   197,   198,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,   296,     0,   127,    83,   128,   187,   188,   189,   190,
     191,     0,     0,   192,   193,     0,     0,     0,     0,     0,
       0,    17,    18,    19,    20,     0,     0,     0,     0,     0,
       0,     0,     0,    85,    86,    87,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,   429,     0,
     127,    83,   128,   187,   188,   189,   190,   191,     0,     0,
     192,   193,     0,     0,     0,     0,     0,     0,     0,     0,
     194,     0,     0,     0,     0,     0,   195,   196,   197,   198,
      85,    86,    87,    15,    16,    17,    18,    19,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,     0,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,     0,     0,    83,     0,   194,     0,     0,
       0,     0,     0,   195,   196,   197,   198,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    84,    85,    86,    87,     0,    88,     0,
       0,     0,     0,     0,     0,     0,    89,    90,    91,    92,
      93,    94
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-373))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       9,     8,    84,     9,   180,    84,     4,    84,    86,   184,
     162,    36,   101,    18,    19,     9,    18,    19,   248,   194,
      96,    97,   167,   105,   106,   104,    84,    85,    86,   350,
      84,    85,    86,    36,   263,    33,    34,    35,     9,   113,
     283,   203,    94,    95,   119,   202,     3,     4,   130,   206,
     422,    88,   210,     0,   203,   341,   222,   223,   132,   221,
     236,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   443,   221,   123,    81,    32,    33,    34,   206,    36,
      37,    38,    39,    40,   206,   139,   122,    96,   202,   410,
      96,   249,   206,   206,   222,   206,   248,   104,   107,   206,
     222,   120,    96,   278,     4,   206,   131,   282,   283,   222,
     340,   222,   119,   220,    86,   124,   291,   346,   361,   220,
     206,   407,   352,   123,   269,    96,   206,    84,   131,    86,
     219,   138,   141,    33,    34,    35,   222,   295,   214,   215,
     123,   150,   222,   123,   222,   152,   200,   433,   230,   123,
     436,   203,   310,   205,   207,   208,   331,   202,   167,   165,
     169,   206,   448,    33,    34,   223,   162,    37,   202,   123,
     456,   180,   206,   403,   332,   201,     3,     4,   202,   221,
     202,   202,   206,   341,   206,   206,   361,   202,   340,   319,
     320,   321,   322,    92,    93,   371,   192,   193,   206,   224,
     352,     3,     4,   223,   202,    32,    33,    34,   203,    36,
      37,    98,    99,   292,    33,    34,    35,   213,   123,   224,
     201,   379,   224,   315,   316,     3,     4,   236,   323,   324,
      32,    33,    34,   223,    36,    37,   201,   246,   245,   201,
     415,   201,   221,   419,    84,    85,    86,   317,   318,   407,
     202,   403,   248,   262,    32,    33,    34,   263,    36,    37,
     269,   114,   115,   116,   222,   201,   273,   442,   277,   222,
     428,   222,   222,   431,   283,   433,   201,   283,   436,   288,
      84,    85,    86,   365,   224,   292,   368,   463,   365,   464,
     448,   201,   468,   469,   211,   212,   213,   201,   456,   201,
     216,   100,   217,   222,   218,   481,   102,   201,   204,    36,
     223,   203,     3,     4,    88,   224,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,    83,   202,    86,   203,   348,
     346,    32,    33,    34,   340,    36,    37,    38,    39,    40,
     221,   201,   361,   204,   222,   361,   352,   206,   202,   204,
     367,   202,   371,   201,   221,   204,   221,   204,   224,   202,
     222,   202,   204,    12,   221,   223,   222,   325,   221,   328,
     222,   326,   246,   327,   220,   144,   329,   151,   330,   273,
      88,   277,   246,   169,   353,   443,    98,   288,   277,   468,
     361,    96,   470,    -1,    -1,    -1,    -1,   403,    -1,    -1,
     419,    -1,    -1,   422,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   260,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   443,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   463,    -1,    -1,    -1,    -1,   468,
     469,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   481,    13,    14,    15,    16,    17,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    -1,   113,   114,   115,   116,    -1,   118,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,   131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,
     210,    -1,    -1,   114,   115,   116,    -1,    -1,    -1,    -1,
      -1,    -1,   222,   223,   224,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    13,    14,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    -1,    -1,    94,    95,    -1,    -1,
      -1,    -1,    -1,   224,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,   113,   114,   115,   116,    -1,
     118,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   131,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    -1,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   201,    -1,    -1,    -1,    -1,    -1,   207,
     208,   209,   210,    -1,    -1,   114,   115,   116,    -1,    -1,
      -1,    -1,    -1,    -1,   222,   223,   224,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    14,    15,
      16,    17,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    -1,    94,    95,
      -1,    -1,    -1,    -1,    -1,   224,    -1,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   113,   114,   115,
     116,    -1,   118,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,   131,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    -1,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,    -1,
      -1,   207,   208,   209,   210,    -1,    -1,   114,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,   222,   223,   224,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,   113,
     114,   115,   116,    -1,   118,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   222,   223,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    14,    15,    16,    17,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    -1,
      -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,   114,   115,   116,    -1,   118,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,
      -1,    -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   222,
     223,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    13,    14,    15,    16,    17,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,   114,   115,   116,    -1,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   131,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   201,
      -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
     222,   223,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,   113,
     114,   115,   116,    -1,   118,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    -1,    84,
      85,    86,    87,    88,    89,    90,    91,    -1,    -1,    94,
      95,    -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,   114,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,   222,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   201,    -1,    -1,    -1,
      -1,    -1,   207,   208,   209,   210,     3,     4,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   222,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    -1,    84,    85,    86,
      87,    88,    89,    90,    91,    -1,    -1,    94,    95,    -1,
      -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,   114,   115,   116,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,   131,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   201,     5,     6,     7,     8,    -1,
     207,   208,   209,   210,   114,   115,   116,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    84,    85,    86,    87,    88,    89,
      90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,
      -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,   209,
     210,    -1,    -1,    -1,   114,   115,   116,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,   209,
     210,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    -1,    84,    85,    86,    87,    88,
      89,    90,    91,    -1,    -1,    94,    95,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   114,   115,   116,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    -1,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   201,    -1,    -1,   204,    -1,    -1,   207,   208,
     209,   210,   114,   115,   116,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,   201,
      -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    -1,    84,    85,    86,    87,    88,    89,    90,
      91,    -1,    -1,    94,    95,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   114,   115,   116,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    -1,
      84,    85,    86,    87,    88,    89,    90,    91,    -1,    -1,
      94,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     201,    -1,    -1,    -1,    -1,    -1,   207,   208,   209,   210,
     114,   115,   116,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    -1,    -1,    85,    -1,   201,    -1,    -1,
      -1,    -1,    -1,   207,   208,   209,   210,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   113,   114,   115,   116,    -1,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   126,   127,   128,   129,
     130,   131
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   119,   226,   228,    88,     0,   230,   123,   120,   227,
     232,    84,    85,    86,   231,     3,     4,     5,     6,     7,
       8,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    85,   113,   114,   115,   116,   118,   126,
     127,   128,   129,   130,   131,   229,   233,   267,   268,   269,
     270,   271,   276,   277,   278,   279,   283,   285,   286,   287,
     288,   289,   290,   291,   292,   321,   322,   323,   327,   122,
      33,    34,    37,    86,   223,   231,    36,    84,    86,   234,
     283,   286,   291,   123,   123,   123,   123,   123,   201,   321,
     222,   223,   304,   202,   206,     4,    33,    34,    35,   273,
     274,   284,   206,   222,    84,    86,   231,   234,    36,   286,
     286,   287,   203,   288,   231,   223,   287,   293,   294,   223,
     286,   288,   139,   200,   231,   280,   281,   282,     9,    10,
      11,    13,    14,    15,    16,    17,    83,    87,    88,    89,
      90,    91,    94,    95,   201,   207,   208,   209,   210,   222,
     223,   224,   234,   235,   236,   238,   239,   240,   241,   242,
     243,   244,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   265,   267,   268,
     279,   287,   298,   299,   300,   301,   305,   306,   307,   310,
     316,   320,   273,   272,   275,   287,   274,   231,   203,   221,
     201,    86,   222,   204,   249,   262,   266,   287,   123,    36,
     279,   324,   325,   326,   231,   295,   296,   224,   294,   293,
     222,   221,   202,   206,   222,   222,   299,   201,   201,   222,
     222,   265,   201,   201,   249,   249,   265,   224,   302,    94,
      95,   203,   205,   202,   202,   206,    82,   263,   201,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   221,
     264,   249,   211,   212,   213,   207,   208,    92,    93,    96,
      97,   214,   215,    98,    99,   216,   217,   218,   100,   102,
     101,   219,   206,   222,    36,   224,   299,   231,   272,   275,
     203,   221,   204,   266,   263,   297,   223,   204,   325,   224,
     287,   324,   203,   206,   222,   224,    88,   281,    83,   298,
     306,   317,   265,   222,   265,   113,   265,   278,   279,   309,
     202,   305,   237,   265,   231,   234,   245,   246,   247,   248,
     263,   263,   249,   249,   249,   251,   251,   252,   252,   253,
     253,   253,   253,   254,   254,   255,   256,   257,   258,   259,
     260,   265,   263,   203,   204,   266,   297,   221,   204,   324,
     287,   222,   295,   266,   296,   201,   309,   318,   319,   202,
     202,   231,   202,   224,   204,   201,   202,   202,   206,    82,
     263,   220,   266,   221,   204,   297,   221,   224,   295,   222,
     204,   265,   222,   202,   299,   308,   223,   311,   221,   300,
     303,   304,   263,   263,   204,   297,   221,   297,   222,   222,
     202,   265,   303,    12,    18,    19,   224,   312,   313,   314,
     315,   297,   297,   222,   299,   265,   220,   299,   312,   299,
     224,   314,   220
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
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
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
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
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
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
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
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


/*----------.
| yyparse.  |
`----------*/

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

/* Line 1590 of yacc.c  */
#line 53 "src/glsl/glsl_parser.yy"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
}

/* Line 1590 of yacc.c  */
#line 2630 "src/glsl/glsl_parser.cpp"
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
  if (yypact_value_is_default (yyn))
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

/* Line 1806 of yacc.c  */
#line 248 "src/glsl/glsl_parser.yy"
    {
	   _mesa_glsl_initialize_types(state);
	}
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 252 "src/glsl/glsl_parser.yy"
    {
	   delete state->symbols;
	   state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
	   _mesa_glsl_initialize_types(state);
	}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 262 "src/glsl/glsl_parser.yy"
    {
	   bool supported = false;

	   switch ((yyvsp[(2) - (3)].n)) {
	   case 100:
	      state->es_shader = true;
	      supported = state->ctx->API == API_OPENGLES2 ||
		          state->ctx->Extensions.ARB_ES2_compatibility;
	      break;
	   case 110:
	   case 120:
	      /* FINISHME: Once the OpenGL 3.0 'forward compatible' context or
	       * the OpenGL 3.2 Core context is supported, this logic will need
	       * change.  Older versions of GLSL are no longer supported
	       * outside the compatibility contexts of 3.x.
	       */
	   case 130:
	   case 140:
	   case 150:
	   case 330:
	   case 400:
	   case 410:
	   case 420:
	      supported = _mesa_is_desktop_gl(state->ctx) &&
			  ((unsigned) (yyvsp[(2) - (3)].n)) <= state->ctx->Const.GLSLVersion;
	      break;
	   default:
	      supported = false;
	      break;
	   }

	   state->language_version = (yyvsp[(2) - (3)].n);
	   state->version_string =
	      ralloc_asprintf(state, "GLSL%s %d.%02d",
			      state->es_shader ? " ES" : "",
			      state->language_version / 100,
			      state->language_version % 100);

	   if (!supported) {
	      _mesa_glsl_error(& (yylsp[(2) - (3)]), state, "%s is not supported. "
			       "Supported versions are: %s\n",
			       state->version_string,
			       state->supported_version_string);
	   }

	   if (state->language_version >= 140) {
	      state->ARB_uniform_buffer_object_enable = true;
	   }
	}
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 319 "src/glsl/glsl_parser.yy"
    {
	   if (state->language_version == 110) {
	      _mesa_glsl_warning(& (yylsp[(1) - (2)]), state,
				 "pragma `invariant(all)' not supported in %s",
				 state->version_string);
	   } else {
	      state->all_invariant = true;
	   }
	}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 343 "src/glsl/glsl_parser.yy"
    {
	   if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
	      YYERROR;
	   }
	}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 352 "src/glsl/glsl_parser.yy"
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[(1) - (1)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
	}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 360 "src/glsl/glsl_parser.yy"
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[(2) - (2)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
	}
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 376 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
	}
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 383 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
	}
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 390 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
	}
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 397 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
	}
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 404 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
	}
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 411 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(2) - (3)].expression);
	}
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 419 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 425 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	}
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 429 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
	}
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 436 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 442 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 460 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 479 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	}
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 485 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	}
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 501 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
	   (yyval.expression)->set_location(yylloc);
   	}
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 507 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 514 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 534 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	}
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 540 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	}
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 552 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (2)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	}
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 564 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 570 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 576 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 585 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_plus; }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 586 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_neg; }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 587 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_logic_not; }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 588 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_bit_not; }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 594 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 600 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 606 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 616 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 622 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 632 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 638 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 648 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 654 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 660 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 666 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 676 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 682 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 692 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 702 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 712 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 722 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 732 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 742 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 752 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 762 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	}
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 770 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_assign; }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 771 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mul_assign; }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 772 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_div_assign; }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 773 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mod_assign; }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 774 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_add_assign; }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 775 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_sub_assign; }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 776 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_ls_assign; }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 777 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_rs_assign; }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 778 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_and_assign; }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 779 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_xor_assign; }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 780 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_or_assign; }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 785 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	}
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 789 "src/glsl/glsl_parser.yy"
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
	}
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 809 "src/glsl/glsl_parser.yy"
    {
	   state->symbols->pop_scope();
	   (yyval.node) = (yyvsp[(1) - (2)].function);
	}
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 814 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
	}
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 818 "src/glsl/glsl_parser.yy"
    {
	   (yyvsp[(3) - (4)].type_specifier)->precision = (yyvsp[(2) - (4)].n);
	   (yyvsp[(3) - (4)].type_specifier)->is_precision_statement = true;
	   (yyval.node) = (yyvsp[(3) - (4)].type_specifier);
	}
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 824 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (yyvsp[(1) - (1)].node);
	}
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 840 "src/glsl/glsl_parser.yy"
    {
	   (yyval.function) = (yyvsp[(1) - (2)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
	}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 845 "src/glsl/glsl_parser.yy"
    {
	   (yyval.function) = (yyvsp[(1) - (3)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
	}
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 853 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.function) = new(ctx) ast_function();
	   (yyval.function)->set_location(yylloc);
	   (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
	   (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);

	   state->symbols->add_function(new(state) ir_function((yyvsp[(2) - (3)].identifier)));
	   state->symbols->push_scope();
	}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 867 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->set_location(yylloc);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (2)].type_specifier);
	   (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (2)].identifier);
	}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 877 "src/glsl/glsl_parser.yy"
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
	}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 892 "src/glsl/glsl_parser.yy"
    {
	   (yyvsp[(1) - (3)].type_qualifier).flags.i |= (yyvsp[(2) - (3)].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = (yyvsp[(3) - (3)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier);
	}
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 899 "src/glsl/glsl_parser.yy"
    {
	   (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 904 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyvsp[(1) - (3)].type_qualifier).flags.i |= (yyvsp[(2) - (3)].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(3) - (3)].type_specifier);
	}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 915 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
	}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 927 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 931 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	}
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 936 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 941 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	   (yyval.type_qualifier).flags.q.out = 1;
	}
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 955 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), false, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (3)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 965 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), true, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 975 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), true, (yyvsp[(5) - (6)].expression), NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (6)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 985 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (7)].identifier), true, NULL, (yyvsp[(7) - (7)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (7)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (7)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 995 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (8)].identifier), true, (yyvsp[(5) - (8)].expression), (yyvsp[(8) - (8)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (8)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (8)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 1005 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), false, NULL, (yyvsp[(5) - (5)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 1019 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   /* Empty declaration list is valid. */
	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (1)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	}
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 1026 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 1035 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), true, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 1044 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), true, (yyvsp[(4) - (5)].expression), NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 1053 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (6)].identifier), true, NULL, (yyvsp[(6) - (6)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (6)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 1062 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (7)].identifier), true, (yyvsp[(4) - (7)].expression), (yyvsp[(7) - (7)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (7)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 1071 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 1080 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->invariant = true;

	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	}
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 1094 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
	}
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 1101 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
	}
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 1112 "src/glsl/glsl_parser.yy"
    {
	  (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
	}
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 1120 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (3)].type_qualifier);
	   if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(3) - (3)]), state, (yyvsp[(3) - (3)].type_qualifier))) {
	      YYERROR;
	   }
	}
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 1130 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   /* Layout qualifiers for ARB_fragment_coord_conventions. */
	   if (!(yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_enable) {
	      if (strcmp((yyvsp[(1) - (1)].identifier), "origin_upper_left") == 0) {
		 (yyval.type_qualifier).flags.q.origin_upper_left = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "pixel_center_integer") == 0) {
		 (yyval.type_qualifier).flags.q.pixel_center_integer = 1;
	      }

	      if ((yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_warn) {
		 _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
				    "GL_ARB_fragment_coord_conventions layout "
				    "identifier `%s' used\n", (yyvsp[(1) - (1)].identifier));
	      }
	   }

	   /* Layout qualifiers for AMD/ARB_conservative_depth. */
	   if (!(yyval.type_qualifier).flags.i &&
	       (state->AMD_conservative_depth_enable ||
	        state->ARB_conservative_depth_enable)) {
	      if (strcmp((yyvsp[(1) - (1)].identifier), "depth_any") == 0) {
	         (yyval.type_qualifier).flags.q.depth_any = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "depth_greater") == 0) {
	         (yyval.type_qualifier).flags.q.depth_greater = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "depth_less") == 0) {
	         (yyval.type_qualifier).flags.q.depth_less = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "depth_unchanged") == 0) {
	         (yyval.type_qualifier).flags.q.depth_unchanged = 1;
	      }
	
	      if ((yyval.type_qualifier).flags.i && state->AMD_conservative_depth_warn) {
	         _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
	                            "GL_AMD_conservative_depth "
	                            "layout qualifier `%s' is used\n", (yyvsp[(1) - (1)].identifier));
	      }
	      if ((yyval.type_qualifier).flags.i && state->ARB_conservative_depth_warn) {
	         _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
	                            "GL_ARB_conservative_depth "
	                            "layout qualifier `%s' is used\n", (yyvsp[(1) - (1)].identifier));
	      }
	   }

	   /* See also uniform_block_layout_qualifier. */
	   if (!(yyval.type_qualifier).flags.i && state->ARB_uniform_buffer_object_enable) {
	      if (strcmp((yyvsp[(1) - (1)].identifier), "std140") == 0) {
	         (yyval.type_qualifier).flags.q.std140 = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "shared") == 0) {
	         (yyval.type_qualifier).flags.q.shared = 1;
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "column_major") == 0) {
	         (yyval.type_qualifier).flags.q.column_major = 1;
	      }

	      if ((yyval.type_qualifier).flags.i && state->ARB_uniform_buffer_object_warn) {
	         _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
	                            "#version 140 / GL_ARB_uniform_buffer_object "
	                            "layout qualifier `%s' is used\n", (yyvsp[(1) - (1)].identifier));
	      }
	   }

	   if (!(yyval.type_qualifier).flags.i) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[(1) - (1)].identifier));
	      YYERROR;
	   }
	}
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 1198 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

	   if (state->ARB_explicit_attrib_location_enable) {
	      /* FINISHME: Handle 'index' once GL_ARB_blend_func_exteneded and
	       * FINISHME: GLSL 1.30 (or later) are supported.
	       */
	      if (strcmp("location", (yyvsp[(1) - (3)].identifier)) == 0) {
		 (yyval.type_qualifier).flags.q.explicit_location = 1;

		 if ((yyvsp[(3) - (3)].n) >= 0) {
		    (yyval.type_qualifier).location = (yyvsp[(3) - (3)].n);
		 } else {
		    _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
				     "invalid location %d specified\n", (yyvsp[(3) - (3)].n));
		    YYERROR;
		 }
	      }

	      if (strcmp("index", (yyvsp[(1) - (3)].identifier)) == 0) {
		 (yyval.type_qualifier).flags.q.explicit_index = 1;

		 if ((yyvsp[(3) - (3)].n) >= 0) {
		    (yyval.type_qualifier).index = (yyvsp[(3) - (3)].n);
		 } else {
		    _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
		                     "invalid index %d specified\n", (yyvsp[(3) - (3)].n));
                    YYERROR;
                 }
              }
	   }

	   /* If the identifier didn't match any known layout identifiers,
	    * emit an error.
	    */
	   if (!(yyval.type_qualifier).flags.i) {
	      _mesa_glsl_error(& (yylsp[(1) - (3)]), state, "unrecognized layout identifier "
			       "`%s'\n", (yyvsp[(1) - (3)].identifier));
	      YYERROR;
	   } else if (state->ARB_explicit_attrib_location_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (3)]), state,
				 "GL_ARB_explicit_attrib_location layout "
				 "identifier `%s' used\n", (yyvsp[(1) - (3)].identifier));
	   }
	}
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 1244 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (1)].type_qualifier);
	   /* Layout qualifiers for ARB_uniform_buffer_object. */
	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "layout qualifier `%s' is used\n", (yyvsp[(1) - (1)].type_qualifier));
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "layout qualifier `%s' is used\n", (yyvsp[(1) - (1)].type_qualifier));
	   }
	}
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 1266 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.row_major = 1;
	}
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 1271 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.packed = 1;
	}
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 1279 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.smooth = 1;
	}
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 1284 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.flat = 1;
	}
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 1289 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.noperspective = 1;
	}
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 1297 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	}
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 1307 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	}
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 1313 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	}
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 1318 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 1323 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (3)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(3) - (3)].type_qualifier).flags.i;
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 1329 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.invariant = 1;
	}
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 1337 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	}
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 1342 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.attribute = 1;
	}
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 1347 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.varying = 1;
	}
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 1352 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1;
	   (yyval.type_qualifier).flags.q.varying = 1;
	}
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 1358 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	}
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 1363 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	}
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 1368 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.in = 1;
	}
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 1373 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.out = 1;
	}
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 1378 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.uniform = 1;
	}
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 1386 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (1)].type_specifier);
	}
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 1390 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(2) - (2)].type_specifier);
	   (yyval.type_specifier)->precision = (yyvsp[(1) - (2)].n);
	}
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 1399 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = NULL;
	}
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 1405 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (4)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = (yyvsp[(3) - (4)].expression);
	}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 1414 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 1420 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 1426 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	}
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 1434 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "void"; }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 1435 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "float"; }
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 1436 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "int"; }
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 1437 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uint"; }
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 1438 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bool"; }
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 1439 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec2"; }
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 1440 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec3"; }
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 1441 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec4"; }
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 1442 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec2"; }
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 1443 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec3"; }
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 1444 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec4"; }
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 1445 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec2"; }
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 1446 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec3"; }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 1447 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec4"; }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 1448 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec2"; }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 1449 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec3"; }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 1450 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec4"; }
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 1451 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2"; }
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 1452 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x3"; }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 1453 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x4"; }
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 1454 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x2"; }
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 1455 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3"; }
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 1456 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x4"; }
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 1457 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x2"; }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 1458 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x3"; }
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 1459 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4"; }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 1460 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1D"; }
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 1461 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2D"; }
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 1462 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRect"; }
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 1463 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler3D"; }
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 1464 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCube"; }
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 1465 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerExternalOES"; }
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 1466 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DShadow"; }
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 1467 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DShadow"; }
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 1468 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRectShadow"; }
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 1469 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeShadow"; }
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 1470 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArray"; }
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 1471 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArray"; }
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 1472 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArrayShadow"; }
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 1473 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArrayShadow"; }
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 1474 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerBuffer"; }
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 1475 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1D"; }
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 1476 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2D"; }
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 1477 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DRect"; }
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 1478 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler3D"; }
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 1479 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerCube"; }
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 1480 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1DArray"; }
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 1481 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DArray"; }
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 1482 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerBuffer"; }
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 1483 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1D"; }
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 1484 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2D"; }
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 1485 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DRect"; }
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 1486 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler3D"; }
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 1487 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerCube"; }
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 1488 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1DArray"; }
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 1489 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DArray"; }
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 1490 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerBuffer"; }
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 1494 "src/glsl/glsl_parser.yy"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
				         "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_high;
		  }
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 1504 "src/glsl/glsl_parser.yy"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_medium;
		  }
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 1514 "src/glsl/glsl_parser.yy"
    {
		     if (!state->es_shader && state->language_version < 130)
			_mesa_glsl_error(& (yylsp[(1) - (1)]), state,
					 "precision qualifier forbidden "
					 "in %s (1.30 or later "
					 "required)\n",
					 state->version_string);

		     (yyval.n) = ast_precision_low;
		  }
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 1528 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	   state->symbols->add_type((yyvsp[(2) - (5)].identifier), glsl_type::void_type);
	}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 1535 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[(3) - (4)].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	}
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 1544 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	}
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 1549 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
	   (yyval.declarator_list)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
	}
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 1557 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->specifier = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(2) - (3)].declaration)->link);
	}
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 1572 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
	   (yyvsp[(1) - (1)].declaration)->link.self_link();
	}
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 1577 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
	   (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
	}
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 1585 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), false, NULL, NULL);
	   (yyval.declaration)->set_location(yylloc);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(1) - (1)].identifier), ir_var_auto, glsl_precision_undefined));
	}
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 1592 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (4)].identifier), true, (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.declaration)->set_location(yylloc);
	}
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 1610 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); }
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 1625 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	}
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 1631 "src/glsl/glsl_parser.yy"
    {
	   state->symbols->push_scope();
	}
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 1635 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(3) - (4)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	   state->symbols->pop_scope();
	}
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 1644 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); }
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 1650 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	}
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 1656 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	}
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 1665 "src/glsl/glsl_parser.yy"
    {
	   if ((yyvsp[(1) - (1)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement\n");
	      assert((yyvsp[(1) - (1)].node) != NULL);
	   }

	   (yyval.node) = (yyvsp[(1) - (1)].node);
	   (yyval.node)->link.self_link();
	}
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 1675 "src/glsl/glsl_parser.yy"
    {
	   if ((yyvsp[(2) - (2)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement\n");
	      assert((yyvsp[(2) - (2)].node) != NULL);
	   }
	   (yyval.node) = (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
	}
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 1687 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement(NULL);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 1693 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 1702 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = new(state) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].selection_rest_statement).then_statement,
						   (yyvsp[(5) - (5)].selection_rest_statement).else_statement);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 1711 "src/glsl/glsl_parser.yy"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (3)].node);
	   (yyval.selection_rest_statement).else_statement = (yyvsp[(3) - (3)].node);
	}
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 1716 "src/glsl/glsl_parser.yy"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (1)].node);
	   (yyval.selection_rest_statement).else_statement = NULL;
	}
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 1724 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
	}
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 1728 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));
	   ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   decl->set_location(yylloc);
	   declarator->set_location(yylloc);

	   declarator->declarations.push_tail(&decl->link);
	   (yyval.node) = declarator;
	}
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 1746 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = new(state) ast_switch_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].switch_body));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 1754 "src/glsl/glsl_parser.yy"
    {
	   (yyval.switch_body) = new(state) ast_switch_body(NULL);
	   (yyval.switch_body)->set_location(yylloc);
	}
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 1759 "src/glsl/glsl_parser.yy"
    {
	   (yyval.switch_body) = new(state) ast_switch_body((yyvsp[(2) - (3)].case_statement_list));
	   (yyval.switch_body)->set_location(yylloc);
	}
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 1767 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label) = new(state) ast_case_label((yyvsp[(2) - (3)].expression));
	   (yyval.case_label)->set_location(yylloc);
	}
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 1772 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label) = new(state) ast_case_label(NULL);
	   (yyval.case_label)->set_location(yylloc);
	}
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 1780 "src/glsl/glsl_parser.yy"
    {
	   ast_case_label_list *labels = new(state) ast_case_label_list();

	   labels->labels.push_tail(& (yyvsp[(1) - (1)].case_label)->link);
	   (yyval.case_label_list) = labels;
	   (yyval.case_label_list)->set_location(yylloc);
	}
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 1788 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label_list) = (yyvsp[(1) - (2)].case_label_list);
	   (yyval.case_label_list)->labels.push_tail(& (yyvsp[(2) - (2)].case_label)->link);
	}
    break;

  case 287:

/* Line 1806 of yacc.c  */
#line 1796 "src/glsl/glsl_parser.yy"
    {
	   ast_case_statement *stmts = new(state) ast_case_statement((yyvsp[(1) - (2)].case_label_list));
	   stmts->set_location(yylloc);

	   stmts->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
	   (yyval.case_statement) = stmts;
	}
    break;

  case 288:

/* Line 1806 of yacc.c  */
#line 1804 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_statement) = (yyvsp[(1) - (2)].case_statement);
	   (yyval.case_statement)->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
	}
    break;

  case 289:

/* Line 1806 of yacc.c  */
#line 1812 "src/glsl/glsl_parser.yy"
    {
	   ast_case_statement_list *cases= new(state) ast_case_statement_list();
	   cases->set_location(yylloc);

	   cases->cases.push_tail(& (yyvsp[(1) - (1)].case_statement)->link);
	   (yyval.case_statement_list) = cases;
	}
    break;

  case 290:

/* Line 1806 of yacc.c  */
#line 1820 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_statement_list) = (yyvsp[(1) - (2)].case_statement_list);
	   (yyval.case_statement_list)->cases.push_tail(& (yyvsp[(2) - (2)].case_statement)->link);
	}
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 1828 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 1835 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 1842 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 1858 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = NULL;
	}
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 1865 "src/glsl/glsl_parser.yy"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
	   (yyval.for_rest_statement).rest = NULL;
	}
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 1870 "src/glsl/glsl_parser.yy"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
	   (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
	}
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 1879 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 1885 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 302:

/* Line 1806 of yacc.c  */
#line 1891 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 1897 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 304:

/* Line 1806 of yacc.c  */
#line 1903 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   (yyval.node)->set_location(yylloc);
	}
    break;

  case 305:

/* Line 1806 of yacc.c  */
#line 1911 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); }
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 1912 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); }
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 1913 "src/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; }
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 1914 "src/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; }
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 1919 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.function_definition) = new(ctx) ast_function_definition();
	   (yyval.function_definition)->set_location(yylloc);
	   (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
	   (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);

	   state->symbols->pop_scope();
	}
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 1933 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_uniform_block(*state->default_uniform_qualifier,
					   (yyvsp[(2) - (6)].identifier), (yyvsp[(4) - (6)].declarator_list));

	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[(1) - (6)]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "required for defining uniform blocks\n");
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (6)]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "required for defining uniform blocks\n");
	   }
	}
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 1949 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;

	   ast_type_qualifier qual = *state->default_uniform_qualifier;
	   if (!qual.merge_qualifier(& (yylsp[(1) - (7)]), state, (yyvsp[(1) - (7)].type_qualifier))) {
	      YYERROR;
	   }
	   (yyval.node) = new(ctx) ast_uniform_block(qual, (yyvsp[(3) - (7)].identifier), (yyvsp[(5) - (7)].declarator_list));

	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "required for defining uniform blocks\n");
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (7)]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "required for defining uniform blocks\n");
	   }
	}
    break;

  case 312:

/* Line 1806 of yacc.c  */
#line 1972 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	}
    break;

  case 313:

/* Line 1806 of yacc.c  */
#line 1977 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
	   (yyvsp[(2) - (2)].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
	}
    break;

  case 316:

/* Line 1806 of yacc.c  */
#line 1991 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->qualifier = (yyvsp[(1) - (5)].type_qualifier);
	   type->qualifier.flags.q.uniform = true;
	   type->specifier = (yyvsp[(3) - (5)].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->ubo_qualifiers_valid = true;

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(4) - (5)].declaration)->link);
	}
    break;

  case 317:

/* Line 1806 of yacc.c  */
#line 2006 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_fully_specified_type *type = new(ctx) ast_fully_specified_type();
	   type->set_location(yylloc);

	   type->qualifier.flags.q.uniform = true;
	   type->specifier = (yyvsp[(2) - (4)].type_specifier);
	   (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->ubo_qualifiers_valid = true;

	   (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(3) - (4)].declaration)->link);
	}
    break;

  case 318:

/* Line 1806 of yacc.c  */
#line 2023 "src/glsl/glsl_parser.yy"
    {
	   if (!state->default_uniform_qualifier->merge_qualifier(& (yylsp[(1) - (3)]), state,
								  (yyvsp[(1) - (3)].type_qualifier))) {
	      YYERROR;
	   }
	}
    break;



/* Line 1806 of yacc.c  */
#line 5556 "src/glsl/glsl_parser.cpp"
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, state, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, state, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
      if (!yypact_value_is_default (yyn))
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, state);
    }
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



