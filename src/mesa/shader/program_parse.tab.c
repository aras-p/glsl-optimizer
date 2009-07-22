
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.4.1"

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



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "program_parse.y"

/*
 * Copyright © 2009 Intel Corporation
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

#include "main/mtypes.h"
#include "prog_parameter.h"
#include "prog_parameter_layout.h"
#include "prog_statevars.h"
#include "prog_instruction.h"

#include "symbol_table.h"
#include "program_parser.h"

extern void *yy_scan_string(char *);
extern void yy_delete_buffer(void *);

static struct asm_symbol *declare_variable(struct asm_parser_state *state,
    char *name, enum asm_type t, struct YYLTYPE *locp);

static int initialize_symbol_from_state(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_param(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_const(struct gl_program *prog,
    struct asm_symbol *param_var, const struct asm_vector *vec);

static int yyparse(struct asm_parser_state *state);

static void yyerror(struct YYLTYPE *locp, struct asm_parser_state *state,
    const char *s);

static int validate_inputs(struct YYLTYPE *locp,
    struct asm_parser_state *state);

static void init_dst_reg(struct prog_dst_register *r);

static void init_src_reg(struct asm_src_register *r);

static struct asm_instruction *asm_instruction_ctor(gl_inst_opcode op,
    const struct prog_dst_register *dst, const struct asm_src_register *src0,
    const struct asm_src_register *src1, const struct asm_src_register *src2);

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define YYLLOC_DEFAULT(Current, Rhs, N)					\
   do {									\
      if (YYID(N)) {							\
	 (Current).first_line = YYRHSLOC(Rhs, 1).first_line;		\
	 (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
	 (Current).position = YYRHSLOC(Rhs, 1).position;		\
	 (Current).last_line = YYRHSLOC(Rhs, N).last_line;		\
	 (Current).last_column = YYRHSLOC(Rhs, N).last_column;		\
      } else {								\
	 (Current).first_line = YYRHSLOC(Rhs, 0).last_line;		\
	 (Current).last_line = (Current).first_line;			\
	 (Current).first_column = YYRHSLOC(Rhs, 0).last_column;		\
	 (Current).last_column = (Current).first_column;		\
	 (Current).position = YYRHSLOC(Rhs, 0).position			\
	    + (Current).first_column;					\
      }									\
   } while(YYID(0))

#define YYLEX_PARAM state->scanner


/* Line 189 of yacc.c  */
#line 167 "program_parse.tab.c"

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
     ARBvp_10 = 258,
     ARBfp_10 = 259,
     ADDRESS = 260,
     ALIAS = 261,
     ATTRIB = 262,
     OPTION = 263,
     OUTPUT = 264,
     PARAM = 265,
     TEMP = 266,
     END = 267,
     BIN_OP = 268,
     BINSC_OP = 269,
     SAMPLE_OP = 270,
     SCALAR_OP = 271,
     TRI_OP = 272,
     VECTOR_OP = 273,
     ARL = 274,
     KIL = 275,
     SWZ = 276,
     INTEGER = 277,
     REAL = 278,
     AMBIENT = 279,
     ATTENUATION = 280,
     BACK = 281,
     CLIP = 282,
     COLOR = 283,
     DEPTH = 284,
     DIFFUSE = 285,
     DIRECTION = 286,
     EMISSION = 287,
     ENV = 288,
     EYE = 289,
     FOG = 290,
     FOGCOORD = 291,
     FRAGMENT = 292,
     FRONT = 293,
     HALF = 294,
     INVERSE = 295,
     INVTRANS = 296,
     LIGHT = 297,
     LIGHTMODEL = 298,
     LIGHTPROD = 299,
     LOCAL = 300,
     MATERIAL = 301,
     MAT_PROGRAM = 302,
     MATRIX = 303,
     MATRIXINDEX = 304,
     MODELVIEW = 305,
     MVP = 306,
     NORMAL = 307,
     OBJECT = 308,
     PALETTE = 309,
     PARAMS = 310,
     PLANE = 311,
     POINT = 312,
     POINTSIZE = 313,
     POSITION = 314,
     PRIMARY = 315,
     PROGRAM = 316,
     PROJECTION = 317,
     RANGE = 318,
     RESULT = 319,
     ROW = 320,
     SCENECOLOR = 321,
     SECONDARY = 322,
     SHININESS = 323,
     SIZE = 324,
     SPECULAR = 325,
     SPOT = 326,
     STATE = 327,
     TEXCOORD = 328,
     TEXENV = 329,
     TEXGEN = 330,
     TEXGEN_Q = 331,
     TEXGEN_R = 332,
     TEXGEN_S = 333,
     TEXGEN_T = 334,
     TEXTURE = 335,
     TRANSPOSE = 336,
     TEXTURE_UNIT = 337,
     TEX_1D = 338,
     TEX_2D = 339,
     TEX_3D = 340,
     TEX_CUBE = 341,
     TEX_RECT = 342,
     VERTEX = 343,
     VTXATTRIB = 344,
     WEIGHT = 345,
     IDENTIFIER = 346,
     MASK4 = 347,
     MASK3 = 348,
     MASK2 = 349,
     MASK1 = 350,
     SWIZZLE = 351,
     DOT_DOT = 352,
     DOT = 353
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 100 "program_parse.y"

   struct asm_instruction *inst;
   struct asm_symbol *sym;
   struct asm_symbol temp_sym;
   struct asm_swizzle_mask swiz_mask;
   struct asm_src_register src_reg;
   struct prog_dst_register dst_reg;
   struct prog_instruction temp_inst;
   char *string;
   unsigned result;
   unsigned attrib;
   int integer;
   float real;
   unsigned state[5];
   int negate;
   struct asm_vector vector;
   gl_inst_opcode opcode;



/* Line 214 of yacc.c  */
#line 322 "program_parse.tab.c"
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
#line 233 "program_parse.y"

extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param,
    void *yyscanner);


/* Line 264 of yacc.c  */
#line 353 "program_parse.tab.c"

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
# if YYENABLE_NLS
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
#define YYLAST   335

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  108
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  133
/* YYNRULES -- Number of rules.  */
#define YYNRULES  255
/* YYNRULES -- Number of states.  */
#define YYNSTATES  424

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   353

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   103,   100,   104,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    99,
       2,   105,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   101,     2,   102,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   106,     2,   107,     2,     2,     2,     2,
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
      95,    96,    97,    98
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     8,    10,    12,    15,    16,    20,    23,
      24,    27,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    57,    62,    67,    74,    81,    90,
      99,   102,   105,   107,   109,   111,   113,   115,   122,   126,
     130,   133,   136,   144,   147,   149,   151,   153,   155,   160,
     162,   164,   166,   168,   170,   172,   174,   178,   179,   182,
     185,   187,   189,   191,   193,   195,   197,   199,   201,   203,
     204,   206,   208,   210,   212,   213,   215,   217,   219,   221,
     223,   225,   230,   233,   236,   238,   241,   243,   246,   248,
     251,   256,   261,   263,   264,   268,   270,   272,   275,   277,
     280,   282,   284,   288,   295,   296,   298,   301,   306,   308,
     312,   314,   316,   318,   320,   322,   324,   326,   328,   330,
     332,   335,   338,   341,   344,   347,   350,   353,   356,   359,
     362,   365,   369,   371,   373,   375,   381,   383,   385,   387,
     390,   392,   394,   397,   399,   402,   409,   411,   415,   417,
     419,   421,   423,   425,   430,   432,   434,   436,   438,   440,
     442,   445,   447,   449,   455,   457,   460,   462,   464,   470,
     473,   474,   481,   485,   486,   488,   490,   492,   494,   496,
     499,   501,   503,   506,   511,   516,   517,   519,   521,   523,
     525,   527,   529,   531,   533,   539,   541,   545,   551,   557,
     559,   563,   569,   571,   573,   575,   577,   579,   581,   583,
     585,   587,   591,   597,   605,   615,   618,   621,   623,   625,
     626,   627,   631,   632,   636,   640,   642,   647,   650,   653,
     656,   659,   663,   666,   670,   671,   673,   675,   676,   678,
     680,   681,   683,   685,   686,   688,   690,   691,   695,   696,
     700,   701,   705,   707,   709,   711
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     109,     0,    -1,   110,   111,   113,    12,    -1,     3,    -1,
       4,    -1,   111,   112,    -1,    -1,     8,    91,    99,    -1,
     113,   114,    -1,    -1,   115,    99,    -1,   151,    99,    -1,
     116,    -1,   117,    -1,   118,    -1,   119,    -1,   120,    -1,
     121,    -1,   122,    -1,   123,    -1,   128,    -1,   124,    -1,
     125,    -1,    19,   132,   100,   129,    -1,    18,   131,   100,
     130,    -1,    16,   131,   100,   129,    -1,    14,   131,   100,
     129,   100,   129,    -1,    13,   131,   100,   130,   100,   130,
      -1,    17,   131,   100,   130,   100,   130,   100,   130,    -1,
      15,   131,   100,   130,   100,   126,   100,   127,    -1,    20,
     130,    -1,    82,   235,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    87,    -1,    21,   131,   100,   136,   100,
     133,    -1,   221,   136,   148,    -1,   221,   136,   149,    -1,
     137,   150,    -1,   145,   147,    -1,   134,   100,   134,   100,
     134,   100,   134,    -1,   221,   135,    -1,    22,    -1,    91,
      -1,    91,    -1,   153,    -1,   138,   101,   139,   102,    -1,
     167,    -1,   228,    -1,    91,    -1,    91,    -1,   140,    -1,
     141,    -1,    22,    -1,   145,   146,   142,    -1,    -1,   103,
     143,    -1,   104,   144,    -1,    22,    -1,    22,    -1,    91,
      -1,    95,    -1,    95,    -1,    95,    -1,    95,    -1,    92,
      -1,    96,    -1,    -1,    92,    -1,    93,    -1,    94,    -1,
      95,    -1,    -1,   152,    -1,   159,    -1,   222,    -1,   224,
      -1,   227,    -1,   240,    -1,     7,    91,   105,   153,    -1,
      88,   154,    -1,    37,   158,    -1,    59,    -1,    90,   156,
      -1,    52,    -1,    28,   233,    -1,    36,    -1,    73,   234,
      -1,    49,   101,   157,   102,    -1,    89,   101,   155,   102,
      -1,    22,    -1,    -1,   101,   157,   102,    -1,    22,    -1,
      59,    -1,    28,   233,    -1,    36,    -1,    73,   234,    -1,
     160,    -1,   161,    -1,    10,    91,   163,    -1,    10,    91,
     101,   162,   102,   164,    -1,    -1,    22,    -1,   105,   166,
      -1,   105,   106,   165,   107,    -1,   168,    -1,   165,   100,
     168,    -1,   170,    -1,   205,    -1,   215,    -1,   170,    -1,
     205,    -1,   216,    -1,   169,    -1,   206,    -1,   215,    -1,
     170,    -1,    72,   194,    -1,    72,   171,    -1,    72,   173,
      -1,    72,   176,    -1,    72,   178,    -1,    72,   184,    -1,
      72,   180,    -1,    72,   187,    -1,    72,   189,    -1,    72,
     191,    -1,    72,   193,    -1,    46,   232,   172,    -1,   182,
      -1,    32,    -1,    68,    -1,    42,   101,   183,   102,   174,
      -1,   182,    -1,    59,    -1,    25,    -1,    71,   175,    -1,
      39,    -1,    31,    -1,    43,   177,    -1,    24,    -1,   232,
      66,    -1,    44,   101,   183,   102,   232,   179,    -1,   182,
      -1,    74,   236,   181,    -1,    28,    -1,    24,    -1,    30,
      -1,    70,    -1,    22,    -1,    75,   234,   185,   186,    -1,
      34,    -1,    53,    -1,    78,    -1,    79,    -1,    77,    -1,
      76,    -1,    35,   188,    -1,    28,    -1,    55,    -1,    27,
     101,   190,   102,    56,    -1,    22,    -1,    57,   192,    -1,
      69,    -1,    25,    -1,   196,    65,   101,   199,   102,    -1,
     196,   195,    -1,    -1,    65,   101,   199,    97,   199,   102,
      -1,    48,   200,   197,    -1,    -1,   198,    -1,    40,    -1,
      81,    -1,    41,    -1,    22,    -1,    50,   201,    -1,    62,
      -1,    51,    -1,    80,   234,    -1,    54,   101,   203,   102,
      -1,    47,   101,   204,   102,    -1,    -1,   202,    -1,    22,
      -1,    22,    -1,    22,    -1,   209,    -1,   212,    -1,   207,
      -1,   210,    -1,    61,    33,   101,   208,   102,    -1,   213,
      -1,   213,    97,   213,    -1,    61,    33,   101,   213,   102,
      -1,    61,    45,   101,   211,   102,    -1,   214,    -1,   214,
      97,   214,    -1,    61,    45,   101,   214,   102,    -1,    22,
      -1,    22,    -1,   217,    -1,   219,    -1,   218,    -1,   219,
      -1,   220,    -1,    23,    -1,    22,    -1,   106,   220,   107,
      -1,   106,   220,   100,   220,   107,    -1,   106,   220,   100,
     220,   100,   220,   107,    -1,   106,   220,   100,   220,   100,
     220,   100,   220,   107,    -1,   221,    23,    -1,   221,    22,
      -1,   103,    -1,   104,    -1,    -1,    -1,    11,   223,   226,
      -1,    -1,     5,   225,   226,    -1,   226,   100,    91,    -1,
      91,    -1,     9,    91,   105,   228,    -1,    64,    59,    -1,
      64,    36,    -1,    64,   229,    -1,    64,    58,    -1,    64,
      73,   234,    -1,    64,    29,    -1,    28,   230,   231,    -1,
      -1,    38,    -1,    26,    -1,    -1,    60,    -1,    67,    -1,
      -1,    38,    -1,    26,    -1,    -1,    60,    -1,    67,    -1,
      -1,   101,   237,   102,    -1,    -1,   101,   238,   102,    -1,
      -1,   101,   239,   102,    -1,    22,    -1,    22,    -1,    22,
      -1,     6,    91,   105,    91,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   240,   240,   243,   251,   260,   261,   264,   282,   283,
     286,   301,   304,   305,   308,   309,   310,   311,   312,   313,
     314,   317,   318,   321,   327,   334,   341,   349,   356,   364,
     375,   381,   387,   388,   389,   390,   391,   394,   406,   419,
     432,   454,   463,   472,   479,   488,   516,   558,   569,   590,
     600,   606,   630,   647,   647,   649,   656,   668,   669,   670,
     673,   685,   697,   715,   726,   738,   740,   741,   742,   743,
     746,   746,   746,   746,   747,   750,   751,   752,   753,   754,
     755,   758,   776,   780,   786,   790,   794,   798,   802,   806,
     810,   814,   820,   831,   831,   832,   834,   838,   842,   846,
     852,   852,   854,   870,   893,   896,   907,   913,   919,   920,
     927,   933,   939,   947,   953,   959,   967,   973,   979,   987,
     988,   991,   992,   993,   994,   995,   996,   997,   998,   999,
    1000,  1003,  1012,  1016,  1020,  1026,  1035,  1039,  1043,  1047,
    1051,  1057,  1063,  1070,  1075,  1083,  1093,  1095,  1103,  1109,
    1113,  1117,  1123,  1134,  1143,  1147,  1152,  1156,  1160,  1164,
    1170,  1177,  1181,  1187,  1195,  1206,  1213,  1217,  1223,  1233,
    1244,  1248,  1266,  1275,  1278,  1284,  1288,  1292,  1298,  1309,
    1314,  1319,  1324,  1329,  1333,  1341,  1344,  1349,  1362,  1370,
    1383,  1383,  1385,  1385,  1387,  1397,  1402,  1409,  1419,  1428,
    1433,  1440,  1450,  1460,  1472,  1472,  1473,  1473,  1475,  1482,
    1487,  1494,  1499,  1505,  1513,  1524,  1528,  1534,  1535,  1536,
    1539,  1539,  1542,  1542,  1545,  1551,  1559,  1572,  1581,  1590,
    1594,  1603,  1612,  1623,  1630,  1635,  1644,  1656,  1659,  1668,
    1679,  1680,  1681,  1684,  1685,  1686,  1689,  1690,  1693,  1694,
    1697,  1698,  1701,  1712,  1723,  1734
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ARBvp_10", "ARBfp_10", "ADDRESS",
  "ALIAS", "ATTRIB", "OPTION", "OUTPUT", "PARAM", "TEMP", "END", "BIN_OP",
  "BINSC_OP", "SAMPLE_OP", "SCALAR_OP", "TRI_OP", "VECTOR_OP", "ARL",
  "KIL", "SWZ", "INTEGER", "REAL", "AMBIENT", "ATTENUATION", "BACK",
  "CLIP", "COLOR", "DEPTH", "DIFFUSE", "DIRECTION", "EMISSION", "ENV",
  "EYE", "FOG", "FOGCOORD", "FRAGMENT", "FRONT", "HALF", "INVERSE",
  "INVTRANS", "LIGHT", "LIGHTMODEL", "LIGHTPROD", "LOCAL", "MATERIAL",
  "MAT_PROGRAM", "MATRIX", "MATRIXINDEX", "MODELVIEW", "MVP", "NORMAL",
  "OBJECT", "PALETTE", "PARAMS", "PLANE", "POINT", "POINTSIZE", "POSITION",
  "PRIMARY", "PROGRAM", "PROJECTION", "RANGE", "RESULT", "ROW",
  "SCENECOLOR", "SECONDARY", "SHININESS", "SIZE", "SPECULAR", "SPOT",
  "STATE", "TEXCOORD", "TEXENV", "TEXGEN", "TEXGEN_Q", "TEXGEN_R",
  "TEXGEN_S", "TEXGEN_T", "TEXTURE", "TRANSPOSE", "TEXTURE_UNIT", "TEX_1D",
  "TEX_2D", "TEX_3D", "TEX_CUBE", "TEX_RECT", "VERTEX", "VTXATTRIB",
  "WEIGHT", "IDENTIFIER", "MASK4", "MASK3", "MASK2", "MASK1", "SWIZZLE",
  "DOT_DOT", "DOT", "';'", "','", "'['", "']'", "'+'", "'-'", "'='", "'{'",
  "'}'", "$accept", "program", "language", "optionSequence", "option",
  "statementSequence", "statement", "instruction", "ALU_instruction",
  "TexInstruction", "ARL_instruction", "VECTORop_instruction",
  "SCALARop_instruction", "BINSCop_instruction", "BINop_instruction",
  "TRIop_instruction", "SAMPLE_instruction", "KIL_instruction",
  "texImageUnit", "texTarget", "SWZ_instruction", "scalarSrcReg",
  "swizzleSrcReg", "maskedDstReg", "maskedAddrReg", "extendedSwizzle",
  "extSwizComp", "extSwizSel", "srcReg", "dstReg", "progParamArray",
  "progParamArrayMem", "progParamArrayAbs", "progParamArrayRel",
  "addrRegRelOffset", "addrRegPosOffset", "addrRegNegOffset", "addrReg",
  "addrComponent", "addrWriteMask", "scalarSuffix", "swizzleSuffix",
  "optionalMask", "namingStatement", "ATTRIB_statement", "attribBinding",
  "vtxAttribItem", "vtxAttribNum", "vtxOptWeightNum", "vtxWeightNum",
  "fragAttribItem", "PARAM_statement", "PARAM_singleStmt",
  "PARAM_multipleStmt", "optArraySize", "paramSingleInit",
  "paramMultipleInit", "paramMultInitList", "paramSingleItemDecl",
  "paramSingleItemUse", "paramMultipleItem", "stateMultipleItem",
  "stateSingleItem", "stateMaterialItem", "stateMatProperty",
  "stateLightItem", "stateLightProperty", "stateSpotProperty",
  "stateLightModelItem", "stateLModProperty", "stateLightProdItem",
  "stateLProdProperty", "stateTexEnvItem", "stateTexEnvProperty",
  "ambDiffSpecProperty", "stateLightNumber", "stateTexGenItem",
  "stateTexGenType", "stateTexGenCoord", "stateFogItem",
  "stateFogProperty", "stateClipPlaneItem", "stateClipPlaneNum",
  "statePointItem", "statePointProperty", "stateMatrixRow",
  "stateMatrixRows", "optMatrixRows", "stateMatrixItem",
  "stateOptMatModifier", "stateMatModifier", "stateMatrixRowNum",
  "stateMatrixName", "stateOptModMatNum", "stateModMatNum",
  "statePaletteMatNum", "stateProgramMatNum", "programSingleItem",
  "programMultipleItem", "progEnvParams", "progEnvParamNums",
  "progEnvParam", "progLocalParams", "progLocalParamNums",
  "progLocalParam", "progEnvParamNum", "progLocalParamNum",
  "paramConstDecl", "paramConstUse", "paramConstScalarDecl",
  "paramConstScalarUse", "paramConstVector", "signedFloatConstant",
  "optionalSign", "TEMP_statement", "@1", "ADDRESS_statement", "@2",
  "varNameList", "OUTPUT_statement", "resultBinding", "resultColBinding",
  "optResultFaceType", "optResultColorType", "optFaceType", "optColorType",
  "optTexCoordUnitNum", "optTexImageUnitNum", "optLegacyTexUnitNum",
  "texCoordUnitNum", "texImageUnitNum", "legacyTexUnitNum",
  "ALIAS_statement", 0
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
     345,   346,   347,   348,   349,   350,   351,   352,   353,    59,
      44,    91,    93,    43,    45,    61,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   108,   109,   110,   110,   111,   111,   112,   113,   113,
     114,   114,   115,   115,   116,   116,   116,   116,   116,   116,
     116,   117,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   127,   127,   127,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   135,   136,   136,   136,   136,
     137,   137,   138,   139,   139,   140,   141,   142,   142,   142,
     143,   144,   145,   146,   147,   148,   149,   149,   149,   149,
     150,   150,   150,   150,   150,   151,   151,   151,   151,   151,
     151,   152,   153,   153,   154,   154,   154,   154,   154,   154,
     154,   154,   155,   156,   156,   157,   158,   158,   158,   158,
     159,   159,   160,   161,   162,   162,   163,   164,   165,   165,
     166,   166,   166,   167,   167,   167,   168,   168,   168,   169,
     169,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   171,   172,   172,   172,   173,   174,   174,   174,   174,
     174,   175,   176,   177,   177,   178,   179,   180,   181,   182,
     182,   182,   183,   184,   185,   185,   186,   186,   186,   186,
     187,   188,   188,   189,   190,   191,   192,   192,   193,   194,
     195,   195,   196,   197,   197,   198,   198,   198,   199,   200,
     200,   200,   200,   200,   200,   201,   201,   202,   203,   204,
     205,   205,   206,   206,   207,   208,   208,   209,   210,   211,
     211,   212,   213,   214,   215,   215,   216,   216,   217,   218,
     218,   219,   219,   219,   219,   220,   220,   221,   221,   221,
     223,   222,   225,   224,   226,   226,   227,   228,   228,   228,
     228,   228,   228,   229,   230,   230,   230,   231,   231,   231,
     232,   232,   232,   233,   233,   233,   234,   234,   235,   235,
     236,   236,   237,   238,   239,   240
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     1,     1,     2,     0,     3,     2,     0,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     4,     6,     6,     8,     8,
       2,     2,     1,     1,     1,     1,     1,     6,     3,     3,
       2,     2,     7,     2,     1,     1,     1,     1,     4,     1,
       1,     1,     1,     1,     1,     1,     3,     0,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1,     1,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     4,     2,     2,     1,     2,     1,     2,     1,     2,
       4,     4,     1,     0,     3,     1,     1,     2,     1,     2,
       1,     1,     3,     6,     0,     1,     2,     4,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     1,     1,     1,     5,     1,     1,     1,     2,
       1,     1,     2,     1,     2,     6,     1,     3,     1,     1,
       1,     1,     1,     4,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     5,     1,     2,     1,     1,     5,     2,
       0,     6,     3,     0,     1,     1,     1,     1,     1,     2,
       1,     1,     2,     4,     4,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     5,     1,     3,     5,     5,     1,
       3,     5,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     5,     7,     9,     2,     2,     1,     1,     0,
       0,     3,     0,     3,     3,     1,     4,     2,     2,     2,
       2,     3,     2,     3,     0,     1,     1,     0,     1,     1,
       0,     1,     1,     0,     1,     1,     0,     3,     0,     3,
       0,     3,     1,     1,     1,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     3,     4,     0,     6,     1,     9,     0,     5,     0,
       0,   222,     0,     0,     0,     0,   220,     2,     0,     0,
       0,     0,     0,     0,     0,   219,     0,     8,     0,    12,
      13,    14,    15,    16,    17,    18,    19,    21,    22,    20,
       0,    75,    76,   100,   101,    77,    78,    79,    80,     7,
       0,     0,     0,     0,     0,     0,     0,    51,     0,    74,
      50,     0,     0,     0,     0,     0,    62,     0,     0,   217,
     218,    30,     0,     0,    10,    11,   225,   223,     0,     0,
       0,   104,   219,   102,   221,   234,   232,   228,   230,   227,
     246,   229,   219,    70,    71,    72,    73,    40,   219,   219,
     219,   219,   219,   219,    64,    41,   210,   209,     0,     0,
       0,     0,    46,   219,    69,     0,    47,    49,   113,   114,
     190,   191,   115,   206,   207,     0,     0,   255,    81,   226,
     105,     0,   106,   110,   111,   112,   204,   205,   208,     0,
     236,   235,   237,     0,   231,     0,     0,     0,     0,    25,
       0,    24,    23,   243,    98,    96,   246,    83,     0,     0,
       0,     0,     0,   240,     0,   240,     0,     0,   250,   246,
     121,   122,   123,   124,   126,   125,   127,   128,   129,   130,
       0,   243,    88,     0,    86,    84,   246,     0,    93,    82,
       0,    67,    66,    68,    39,     0,     0,   224,     0,   216,
     215,   238,   239,   233,   252,     0,   219,   219,     0,     0,
     219,   244,   245,    97,    99,     0,     0,     0,   161,   162,
     160,     0,   143,   242,   241,   142,     0,     0,     0,     0,
     185,   181,     0,   180,   246,   173,   167,   166,   165,     0,
       0,     0,     0,    87,     0,    89,     0,     0,    85,   219,
     211,    55,     0,    53,    54,     0,   219,     0,   103,   247,
      27,    26,    65,    38,   248,     0,     0,   202,     0,   203,
       0,   164,     0,   152,     0,   144,     0,   149,   150,   133,
     134,   151,   131,   132,     0,   187,   179,   186,     0,   182,
     175,   177,   176,   172,   174,   254,     0,   148,   147,   154,
     155,     0,     0,    95,     0,    92,     0,     0,     0,    48,
      63,    57,    37,     0,     0,   219,     0,    31,     0,   219,
     197,   201,     0,     0,   240,   189,     0,   188,     0,   251,
     159,   158,   156,   157,   153,   178,     0,    90,    91,    94,
     219,   212,     0,     0,    56,   219,    44,    45,    43,     0,
       0,     0,   108,   116,   119,   117,   192,   193,   118,   253,
       0,    32,    33,    34,    35,    36,    29,    28,   163,   138,
     140,   137,     0,   135,   136,     0,   184,   183,   168,     0,
      60,    58,    61,    59,     0,     0,     0,   120,   170,   219,
     107,   249,   141,   139,   145,   146,   219,   213,   219,     0,
       0,     0,   169,   109,     0,     0,     0,   195,     0,   199,
       0,   214,   219,   194,     0,   198,     0,     0,    42,   196,
     200,     0,     0,   171
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     6,     8,     9,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,   265,   366,
      39,   146,    71,    58,    67,   312,   313,   348,   114,    59,
     115,   252,   253,   254,   344,   381,   383,    68,   311,   105,
     263,   194,    97,    40,    41,   116,   189,   306,   248,   304,
     157,    42,    43,    44,   131,    83,   258,   351,   132,   117,
     352,   353,   118,   170,   282,   171,   373,   393,   172,   225,
     173,   394,   174,   298,   283,   274,   175,   301,   334,   176,
     220,   177,   272,   178,   238,   179,   387,   402,   180,   293,
     294,   336,   235,   286,   287,   328,   326,   119,   355,   356,
     406,   120,   357,   408,   121,   268,   270,   358,   122,   136,
     123,   124,   138,    72,    45,    55,    46,    50,    77,    47,
      60,    91,   142,   203,   226,   213,   144,   317,   240,   205,
     360,   296,    48
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -371
static const yytype_int16 yypact[] =
{
     159,  -371,  -371,    34,  -371,  -371,    66,    11,  -371,   140,
     -22,  -371,    25,    37,    51,    53,  -371,  -371,   -28,   -28,
     -28,   -28,   -28,   -28,    74,   102,   -28,  -371,    80,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
      82,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
     120,    -6,   107,   108,    91,   120,    61,  -371,    93,   105,
    -371,   114,   115,   116,   117,   118,  -371,   119,    87,  -371,
    -371,  -371,   -15,   121,  -371,  -371,  -371,   122,   129,   -17,
     160,   201,    -3,  -371,   122,    21,  -371,  -371,  -371,  -371,
     124,  -371,   102,  -371,  -371,  -371,  -371,  -371,   102,   102,
     102,   102,   102,   102,  -371,  -371,  -371,  -371,    68,    72,
       8,   -11,   126,   102,    99,   127,  -371,  -371,  -371,  -371,
    -371,  -371,  -371,  -371,  -371,   -15,   135,  -371,  -371,  -371,
    -371,   130,  -371,  -371,  -371,  -371,  -371,  -371,  -371,   185,
    -371,  -371,    48,   207,  -371,   136,   137,   -15,   138,  -371,
     139,  -371,  -371,    64,  -371,  -371,   124,  -371,   141,   142,
     143,     9,   144,    88,   145,    83,    86,    24,   146,   124,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
     175,    64,  -371,   147,  -371,  -371,   124,   148,   150,  -371,
      73,  -371,  -371,  -371,  -371,   -10,   152,  -371,   151,  -371,
    -371,  -371,  -371,  -371,  -371,   153,   102,   102,   155,   171,
     102,  -371,  -371,  -371,  -371,   219,   232,   235,  -371,  -371,
    -371,   237,  -371,  -371,  -371,  -371,   194,   237,     0,   161,
     239,  -371,   163,  -371,   124,   -14,  -371,  -371,  -371,   243,
     238,    58,   166,  -371,   246,  -371,   247,   246,  -371,   102,
    -371,  -371,   168,  -371,  -371,   176,   102,   167,  -371,  -371,
    -371,  -371,  -371,  -371,   173,   172,   177,  -371,   174,  -371,
     178,  -371,   179,  -371,   180,  -371,   181,  -371,  -371,  -371,
    -371,  -371,  -371,  -371,   253,  -371,  -371,  -371,   256,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,   182,  -371,  -371,  -371,
    -371,   125,   257,  -371,   183,  -371,   186,   187,    76,  -371,
    -371,   106,  -371,   190,    -7,    26,   265,  -371,   103,   102,
    -371,  -371,   236,    36,    83,  -371,   189,  -371,   191,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,   192,  -371,  -371,  -371,
     102,  -371,   273,   274,  -371,   102,  -371,  -371,  -371,    90,
       8,    77,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
     195,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
    -371,  -371,   267,  -371,  -371,    15,  -371,  -371,  -371,    78,
    -371,  -371,  -371,  -371,   199,   200,   202,  -371,   240,    26,
    -371,  -371,  -371,  -371,  -371,  -371,   102,  -371,   102,   219,
     232,   203,  -371,  -371,   193,   206,   208,   205,   209,   215,
     257,  -371,   102,  -371,   219,  -371,   232,    41,  -371,  -371,
    -371,   257,   211,  -371
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
    -371,   -94,   -88,   149,  -371,  -371,  -326,  -371,   -92,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,   123,  -371,  -371,
    -371,  -371,  -371,  -371,  -371,   241,  -371,  -371,  -371,    70,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
     -74,  -371,   -81,  -371,  -371,  -371,  -371,  -371,  -371,  -371,
    -371,  -371,  -371,  -371,  -295,    92,  -371,  -371,  -371,  -371,
    -371,  -371,  -371,  -371,  -371,  -371,  -371,  -371,   -29,  -371,
    -371,  -368,  -371,  -371,  -371,  -371,  -371,   242,  -371,  -371,
    -371,  -371,  -371,  -371,  -371,  -370,  -306,   244,  -371,  -371,
    -371,   -80,  -110,   -82,  -371,  -371,  -371,  -371,   268,  -371,
     245,  -371,  -371,  -371,  -160,   154,  -146,  -371,  -371,  -371,
    -371,  -371,  -371
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -53
static const yytype_int16 yytable[] =
{
     139,   133,   137,   190,   145,   228,   149,   106,   107,   152,
     214,   148,   251,   150,   151,   346,   147,   181,   147,   384,
     108,   147,   108,   241,   277,   182,   290,   291,   374,   407,
     278,   139,   279,   196,     5,   160,    56,   218,   183,   277,
     245,   184,   417,   161,   419,   278,   109,   140,   185,   236,
     162,   163,   164,   422,   165,   208,   166,   110,   109,   141,
     277,   369,   186,    57,   219,   167,   278,   292,   280,   110,
     281,   111,   405,   111,     7,   370,   112,    49,   187,   188,
     395,    66,   168,   169,   347,   281,   418,   349,   289,    85,
      86,   113,   299,   237,   409,   371,   153,    87,   350,    78,
      69,    70,    10,   113,   154,   158,   281,   372,   201,   223,
     420,   300,   222,   261,   223,   202,    51,   159,   260,    88,
      89,   224,   266,   385,   211,   147,   224,   155,    52,    69,
      70,   212,   113,   229,    90,   386,   230,   231,   421,   308,
     232,   156,    53,   378,    54,    11,    12,    13,   233,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,     1,     2,   375,    66,   234,   139,    61,    62,
      63,    64,    65,   249,   314,    73,   340,   389,   396,    74,
     250,    75,   104,   341,   390,   397,   361,   362,   363,   364,
     365,   191,    81,    92,   192,   193,    82,    93,    94,    95,
      96,   330,   331,   332,   333,    69,    70,   199,   200,   342,
     343,    76,    79,    80,    98,    99,   100,   101,   102,   103,
     127,   125,   126,   130,    56,   143,   197,   -52,   195,   204,
     379,   367,   198,   139,   354,   137,   206,   207,   209,   210,
     242,   267,   215,   216,   217,   221,   227,   239,   244,   246,
     262,   247,   256,   264,   269,   259,   257,   271,   139,   273,
     275,   285,   284,   314,   288,   295,   297,   302,   303,   305,
     309,   310,   318,   315,   316,   325,   320,   319,   327,   335,
     321,   322,   323,   324,   329,   337,   404,   359,   338,   339,
     345,   376,   368,   377,   378,   380,   382,   391,   392,   398,
     411,   399,   414,   400,   410,   401,   412,   139,   354,   137,
     413,   415,   416,   423,   139,   403,   314,   307,   255,   276,
     128,   388,     0,    84,   134,   129,   135,     0,     0,     0,
     314,     0,     0,     0,     0,   243
};

static const yytype_int16 yycheck[] =
{
      82,    82,    82,   113,    92,   165,   100,    22,    23,   103,
     156,    99,    22,   101,   102,    22,    98,    28,   100,   345,
      37,   103,    37,   169,    24,    36,    40,    41,   323,   399,
      30,   113,    32,   125,     0,    27,    64,    28,    49,    24,
     186,    52,   410,    35,   414,    30,    61,    26,    59,    25,
      42,    43,    44,   421,    46,   147,    48,    72,    61,    38,
      24,    25,    73,    91,    55,    57,    30,    81,    68,    72,
      70,    88,   398,    88,     8,    39,    91,    99,    89,    90,
     375,    91,    74,    75,    91,    70,   412,    61,   234,    28,
      29,   106,    34,    69,   400,    59,    28,    36,    72,   105,
     103,   104,    91,   106,    36,    33,    70,    71,    60,    26,
     416,    53,    24,   207,    26,    67,    91,    45,   206,    58,
      59,    38,   210,    33,    60,   207,    38,    59,    91,   103,
     104,    67,   106,    47,    73,    45,    50,    51,    97,   249,
      54,    73,    91,   102,    91,     5,     6,     7,    62,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,     3,     4,   324,    91,    80,   249,    19,    20,
      21,    22,    23,   100,   256,    26,   100,   100,   100,    99,
     107,    99,    95,   107,   107,   107,    83,    84,    85,    86,
      87,    92,   101,   100,    95,    96,   105,    92,    93,    94,
      95,    76,    77,    78,    79,   103,   104,    22,    23,   103,
     104,    91,   105,   105,   100,   100,   100,   100,   100,   100,
      91,   100,   100,    22,    64,   101,    91,   101,   101,    22,
     340,   319,   102,   315,   315,   315,   100,   100,   100,   100,
      65,    22,   101,   101,   101,   101,   101,   101,   101,   101,
      95,   101,   100,    82,    22,   102,   105,    22,   340,    22,
      66,    22,   101,   345,   101,    22,    28,   101,    22,    22,
     102,    95,   100,   106,   101,    22,   102,   100,    22,    22,
     102,   102,   102,   102,   102,   102,   396,    22,   102,   102,
     100,   102,    56,   102,   102,    22,    22,   102,    31,   100,
     107,   101,    97,   101,   101,    65,   100,   389,   389,   389,
     102,   102,    97,   102,   396,   389,   398,   247,   195,   227,
      79,   350,    -1,    55,    82,    80,    82,    -1,    -1,    -1,
     412,    -1,    -1,    -1,    -1,   181
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,   109,   110,     0,   111,     8,   112,   113,
      91,     5,     6,     7,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   128,
     151,   152,   159,   160,   161,   222,   224,   227,   240,    99,
     225,    91,    91,    91,    91,   223,    64,    91,   131,   137,
     228,   131,   131,   131,   131,   131,    91,   132,   145,   103,
     104,   130,   221,   131,    99,    99,    91,   226,   105,   105,
     105,   101,   105,   163,   226,    28,    29,    36,    58,    59,
      73,   229,   100,    92,    93,    94,    95,   150,   100,   100,
     100,   100,   100,   100,    95,   147,    22,    23,    37,    61,
      72,    88,    91,   106,   136,   138,   153,   167,   170,   205,
     209,   212,   216,   218,   219,   100,   100,    91,   153,   228,
      22,   162,   166,   170,   205,   215,   217,   219,   220,   221,
      26,    38,   230,   101,   234,   130,   129,   221,   130,   129,
     130,   130,   129,    28,    36,    59,    73,   158,    33,    45,
      27,    35,    42,    43,    44,    46,    48,    57,    74,    75,
     171,   173,   176,   178,   180,   184,   187,   189,   191,   193,
     196,    28,    36,    49,    52,    59,    73,    89,    90,   154,
     220,    92,    95,    96,   149,   101,   136,    91,   102,    22,
      23,    60,    67,   231,    22,   237,   100,   100,   136,   100,
     100,    60,    67,   233,   234,   101,   101,   101,    28,    55,
     188,   101,    24,    26,    38,   177,   232,   101,   232,    47,
      50,    51,    54,    62,    80,   200,    25,    69,   192,   101,
     236,   234,    65,   233,   101,   234,   101,   101,   156,   100,
     107,    22,   139,   140,   141,   145,   100,   105,   164,   102,
     130,   129,    95,   148,    82,   126,   130,    22,   213,    22,
     214,    22,   190,    22,   183,    66,   183,    24,    30,    32,
      68,    70,   172,   182,   101,    22,   201,   202,   101,   234,
      40,    41,    81,   197,   198,    22,   239,    28,   181,    34,
      53,   185,   101,    22,   157,    22,   155,   157,   220,   102,
      95,   146,   133,   134,   221,   106,   101,   235,   100,   100,
     102,   102,   102,   102,   102,    22,   204,    22,   203,   102,
      76,    77,    78,    79,   186,    22,   199,   102,   102,   102,
     100,   107,   103,   104,   142,   100,    22,    91,   135,    61,
      72,   165,   168,   169,   170,   206,   207,   210,   215,    22,
     238,    83,    84,    85,    86,    87,   127,   130,    56,    25,
      39,    59,    71,   174,   182,   232,   102,   102,   102,   220,
      22,   143,    22,   144,   134,    33,    45,   194,   196,   100,
     107,   102,    31,   175,   179,   182,   100,   107,   100,   101,
     101,    65,   195,   168,   220,   134,   208,   213,   211,   214,
     101,   107,   100,   102,    97,   102,    97,   199,   134,   213,
     214,    97,   199,   102
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
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

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
# if YYLTYPE_IS_TRIVIAL
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct asm_parser_state *state)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct asm_parser_state *state;
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct asm_parser_state *state)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, state)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    struct asm_parser_state *state;
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
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, struct asm_parser_state *state)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, state)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    struct asm_parser_state *state;
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct asm_parser_state *state)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, state)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    struct asm_parser_state *state;
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
int yyparse (struct asm_parser_state *state);
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
yyparse (struct asm_parser_state *state)
#else
int
yyparse (state)
    struct asm_parser_state *state;
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
    YYLTYPE yyerror_range[2];

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

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

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
        case 3:

/* Line 1455 of yacc.c  */
#line 244 "program_parse.y"
    {
	   if (state->prog->Target != GL_VERTEX_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid fragment program header");

	   }
	   state->mode = ARB_vertex;
	;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 252 "program_parse.y"
    {
	   if (state->prog->Target != GL_FRAGMENT_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex program header");
	   }
	   state->mode = ARB_fragment;
	;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 265 "program_parse.y"
    {
	   int valid = 0;

	   if (state->mode == ARB_vertex) {
	      valid = _mesa_ARBvp_parse_option(state, (yyvsp[(2) - (3)].string));
	   } else if (state->mode == ARB_fragment) {
	      valid = _mesa_ARBfp_parse_option(state, (yyvsp[(2) - (3)].string));
	   }


	   if (!valid) {
	      yyerror(& (yylsp[(2) - (3)]), state, "invalid option string");
	      YYERROR;
	   }
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 287 "program_parse.y"
    {
	   if ((yyvsp[(1) - (2)].inst) != NULL) {
	      if (state->inst_tail == NULL) {
		 state->inst_head = (yyvsp[(1) - (2)].inst);
	      } else {
		 state->inst_tail->next = (yyvsp[(1) - (2)].inst);
	      }

	      state->inst_tail = (yyvsp[(1) - (2)].inst);
	      (yyvsp[(1) - (2)].inst)->next = NULL;

	      state->prog->NumInstructions++;
	   }
	;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 322 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_ARL, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 328 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 335 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 342 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 350 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 358 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (8)].temp_inst).Opcode, & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), & (yyvsp[(6) - (8)].src_reg), & (yyvsp[(8) - (8)].src_reg));
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (8)].temp_inst).SaturateMode;
	;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 365 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (8)].temp_inst).Opcode, & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), NULL, NULL);
	   if ((yyval.inst) != NULL) {
	      (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (8)].temp_inst).SaturateMode;
	      (yyval.inst)->Base.TexSrcUnit = (yyvsp[(6) - (8)].integer);
	      (yyval.inst)->Base.TexSrcTarget = (yyvsp[(8) - (8)].integer);
	   }
	;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 376 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_KIL, NULL, & (yyvsp[(2) - (2)].src_reg), NULL, NULL);
	;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 382 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 387 "program_parse.y"
    { (yyval.integer) = TEXTURE_1D_INDEX; ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 388 "program_parse.y"
    { (yyval.integer) = TEXTURE_2D_INDEX; ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 389 "program_parse.y"
    { (yyval.integer) = TEXTURE_3D_INDEX; ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 390 "program_parse.y"
    { (yyval.integer) = TEXTURE_CUBE_INDEX; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 391 "program_parse.y"
    { (yyval.integer) = TEXTURE_RECT_INDEX; ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 395 "program_parse.y"
    {
	   /* FIXME: Is this correct?  Should the extenedSwizzle be applied
	    * FIXME: to the existing swizzle?
	    */
	   (yyvsp[(4) - (6)].src_reg).Base.Swizzle = (yyvsp[(6) - (6)].swiz_mask).swizzle;

	   (yyval.inst) = asm_instruction_ctor(OPCODE_SWZ, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 407 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 420 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 433 "program_parse.y"
    {
	   (yyval.dst_reg) = (yyvsp[(1) - (2)].dst_reg);
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (2)].swiz_mask).mask;

	   if ((yyval.dst_reg).File == PROGRAM_OUTPUT) {
	      /* Technically speaking, this should check that it is in
	       * vertex program mode.  However, PositionInvariant can never be
	       * set in fragment program mode, so it is somewhat irrelevant.
	       */
	      if (state->option.PositionInvariant
	       && ((yyval.dst_reg).Index == VERT_RESULT_HPOS)) {
		 yyerror(& (yylsp[(1) - (2)]), state, "position-invariant programs cannot "
			 "write position");
		 YYERROR;
	      }

	      state->prog->OutputsWritten |= (1U << (yyval.dst_reg).Index);
	   }
	;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 455 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_ADDRESS;
	   (yyval.dst_reg).Index = 0;
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (2)].swiz_mask).mask;
	;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 464 "program_parse.y"
    {
	   (yyval.swiz_mask).swizzle = MAKE_SWIZZLE4((yyvsp[(1) - (7)].swiz_mask).swizzle, (yyvsp[(3) - (7)].swiz_mask).swizzle,
				      (yyvsp[(5) - (7)].swiz_mask).swizzle, (yyvsp[(7) - (7)].swiz_mask).swizzle);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (7)].swiz_mask).mask) | ((yyvsp[(3) - (7)].swiz_mask).mask << 1) | ((yyvsp[(5) - (7)].swiz_mask).mask << 2)
	      | ((yyvsp[(7) - (7)].swiz_mask).mask << 3);
	;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 473 "program_parse.y"
    {
	   (yyval.swiz_mask).swizzle = (yyvsp[(2) - (2)].integer);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (2)].negate)) ? 1 : 0;
	;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 480 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) != 0) && ((yyvsp[(1) - (1)].integer) != 1)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   (yyval.integer) = ((yyvsp[(1) - (1)].integer) == 0) ? SWIZZLE_ZERO : SWIZZLE_ONE;
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 489 "program_parse.y"
    {
	   if (strlen((yyvsp[(1) - (1)].string)) > 1) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   switch ((yyvsp[(1) - (1)].string)[0]) {
	   case 'x':
	      (yyval.integer) = SWIZZLE_X;
	      break;
	   case 'y':
	      (yyval.integer) = SWIZZLE_Y;
	      break;
	   case 'z':
	      (yyval.integer) = SWIZZLE_Z;
	      break;
	   case 'w':
	      (yyval.integer) = SWIZZLE_W;
	      break;
	   default:
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	      break;
	   }
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 517 "program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) && (s->type != at_temp)
		      && (s->type != at_attrib)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type == at_param) && s->param_is_array) {
	      yyerror(& (yylsp[(1) - (1)]), state, "non-array access to array PARAM");
	      YYERROR;
	   }

	   init_src_reg(& (yyval.src_reg));
	   switch (s->type) {
	   case at_temp:
	      (yyval.src_reg).Base.File = PROGRAM_TEMPORARY;
	      (yyval.src_reg).Base.Index = s->temp_binding;
	      break;
	   case at_param:
	      (yyval.src_reg).Base.File = s->param_binding_type;
	      (yyval.src_reg).Base.Index = s->param_binding_begin;
	      break;
	   case at_attrib:
	     (yyval.src_reg).Base.File = PROGRAM_INPUT;
	     (yyval.src_reg).Base.Index = s->attrib_binding;
	     state->prog->InputsRead |= (1U << (yyval.src_reg).Base.Index);

	     if (!validate_inputs(& (yylsp[(1) - (1)]), state)) {
		YYERROR;
	     }
	     break;

	   default:
	      YYERROR;
	      break;
	   }
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 559 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = PROGRAM_INPUT;
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].attrib);
	   state->prog->InputsRead |= (1U << (yyval.src_reg).Base.Index);

	   if (!validate_inputs(& (yylsp[(1) - (1)]), state)) {
	      YYERROR;
	   }
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 570 "program_parse.y"
    {
	   if (! (yyvsp[(3) - (4)].src_reg).Base.RelAddr
	       && ((yyvsp[(3) - (4)].src_reg).Base.Index >= (yyvsp[(1) - (4)].sym)->param_binding_length)) {
	      yyerror(& (yylsp[(3) - (4)]), state, "out of bounds array access");
	      YYERROR;
	   }

	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = (yyvsp[(1) - (4)].sym)->param_binding_type;

	   if ((yyvsp[(3) - (4)].src_reg).Base.RelAddr) {
	      (yyvsp[(1) - (4)].sym)->param_accessed_indirectly = 1;

	      (yyval.src_reg).Base.RelAddr = 1;
	      (yyval.src_reg).Base.Index = (yyvsp[(3) - (4)].src_reg).Base.Index;
	      (yyval.src_reg).Symbol = (yyvsp[(1) - (4)].sym);
	   } else {
	      (yyval.src_reg).Base.Index = (yyvsp[(1) - (4)].sym)->param_binding_begin + (yyvsp[(3) - (4)].src_reg).Base.Index;
	   }
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 591 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = ((yyvsp[(1) - (1)].temp_sym).name != NULL) 
	      ? (yyvsp[(1) - (1)].temp_sym).param_binding_type
	      : PROGRAM_CONSTANT;
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].temp_sym).param_binding_begin;
	;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 601 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_OUTPUT;
	   (yyval.dst_reg).Index = (yyvsp[(1) - (1)].result);
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 607 "program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_output) && (s->type != at_temp)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   }

	   init_dst_reg(& (yyval.dst_reg));
	   if (s->type == at_temp) {
	      (yyval.dst_reg).File = PROGRAM_TEMPORARY;
	      (yyval.dst_reg).Index = s->temp_binding;
	   } else {
	      (yyval.dst_reg).File = s->param_binding_type;
	      (yyval.dst_reg).Index = s->param_binding_begin;
	   }
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 631 "program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid operand variable");
	      YYERROR;
	   } else if ((s->type != at_param) || !s->param_is_array) {
	      yyerror(& (yylsp[(1) - (1)]), state, "array access to non-PARAM variable");
	      YYERROR;
	   } else {
	      (yyval.sym) = s;
	   }
	;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 650 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 657 "program_parse.y"
    {
	   /* FINISHME: Add support for multiple address registers.
	    */
	   /* FINISHME: Add support for 4-component address registers.
	    */
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.RelAddr = 1;
	   (yyval.src_reg).Base.Index = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 668 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 669 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 670 "program_parse.y"
    { (yyval.integer) = -(yyvsp[(2) - (2)].integer); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 674 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 0) || ((yyvsp[(1) - (1)].integer) > 63)) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "relative address offset too large (positive)");
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 686 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 0) || ((yyvsp[(1) - (1)].integer) > 64)) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "relative address offset too large (negative)");
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 698 "program_parse.y"
    {
	   struct asm_symbol *const s = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(1) - (1)].string));

	   if (s == NULL) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid array member");
	      YYERROR;
	   } else if (s->type != at_address) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "invalid variable for indexed array access");
	      YYERROR;
	   } else {
	      (yyval.sym) = s;
	   }
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 716 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid address component selector");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 727 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state,
		      "address register write mask must be \".x\"");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 743 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 747 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 759 "program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(2) - (4)].string), at_attrib, & (yylsp[(2) - (4)]));

	   if (s == NULL) {
	      YYERROR;
	   } else {
	      s->attrib_binding = (yyvsp[(4) - (4)].attrib);
	      state->InputsBound |= (1U << s->attrib_binding);

	      if (!validate_inputs(& (yylsp[(4) - (4)]), state)) {
		 YYERROR;
	      }
	   }
	;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 777 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 781 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 787 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_POS;
	;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 791 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_WEIGHT;
	;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 795 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_NORMAL;
	;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 799 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_COLOR0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 803 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_FOG;
	;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 807 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 811 "program_parse.y"
    {
	   YYERROR;
	;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 815 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_GENERIC0 + (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 821 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->limits->MaxAttribs) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex attribute reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 835 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_WPOS;
	;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 839 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_COL0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 843 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_FOGC;
	;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 847 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 855 "program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(2) - (3)].string), at_param, & (yylsp[(2) - (3)]));

	   if (s == NULL) {
	      YYERROR;
	   } else {
	      s->param_binding_type = (yyvsp[(3) - (3)].temp_sym).param_binding_type;
	      s->param_binding_begin = (yyvsp[(3) - (3)].temp_sym).param_binding_begin;
	      s->param_binding_length = (yyvsp[(3) - (3)].temp_sym).param_binding_length;
	      s->param_is_array = 0;
	   }
	;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 871 "program_parse.y"
    {
	   if (((yyvsp[(4) - (6)].integer) != 0) && ((yyvsp[(4) - (6)].integer) != (yyvsp[(6) - (6)].temp_sym).param_binding_length)) {
	      yyerror(& (yylsp[(4) - (6)]), state, 
		      "parameter array size and number of bindings must match");
	      YYERROR;
	   } else {
	      struct asm_symbol *const s =
		 declare_variable(state, (yyvsp[(2) - (6)].string), (yyvsp[(6) - (6)].temp_sym).type, & (yylsp[(2) - (6)]));

	      if (s == NULL) {
		 YYERROR;
	      } else {
		 s->param_binding_type = (yyvsp[(6) - (6)].temp_sym).param_binding_type;
		 s->param_binding_begin = (yyvsp[(6) - (6)].temp_sym).param_binding_begin;
		 s->param_binding_length = (yyvsp[(6) - (6)].temp_sym).param_binding_length;
		 s->param_is_array = 1;
	      }
	   }
	;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 893 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 897 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 1) || ((yyvsp[(1) - (1)].integer) >= state->limits->MaxParameters)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid parameter array size");
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 908 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(2) - (2)].temp_sym);
	;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 914 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(3) - (4)].temp_sym);
	;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 921 "program_parse.y"
    {
	   (yyvsp[(1) - (3)].temp_sym).param_binding_length += (yyvsp[(3) - (3)].temp_sym).param_binding_length;
	   (yyval.temp_sym) = (yyvsp[(1) - (3)].temp_sym);
	;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 928 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 934 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 940 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 948 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 954 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 960 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 968 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 974 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 980 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 987 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(1) - (1)].state), sizeof((yyval.state))); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 988 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 991 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 992 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 993 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 994 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 995 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 996 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 997 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 998 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 999 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1000 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1004 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_MATERIAL;
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1013 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1017 "program_parse.y"
    {
	   (yyval.integer) = STATE_EMISSION;
	;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1021 "program_parse.y"
    {
	   (yyval.integer) = STATE_SHININESS;
	;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1027 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHT;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (5)].integer);
	;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1036 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1040 "program_parse.y"
    {
	   (yyval.integer) = STATE_POSITION;
	;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1044 "program_parse.y"
    {
	   (yyval.integer) = STATE_ATTENUATION;
	;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1048 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1052 "program_parse.y"
    {
	   (yyval.integer) = STATE_HALF_VECTOR;
	;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1058 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPOT_DIRECTION;
	;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1064 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (2)].state)[1];
	;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1071 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_AMBIENT;
	;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1076 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_SCENECOLOR;
	   (yyval.state)[1] = (yyvsp[(1) - (2)].integer);
	;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1084 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTPROD;
	   (yyval.state)[1] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(6) - (6)].integer);
	;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1096 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(3) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1104 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXENV_COLOR;
	;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1110 "program_parse.y"
    {
	   (yyval.integer) = STATE_AMBIENT;
	;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1114 "program_parse.y"
    {
	   (yyval.integer) = STATE_DIFFUSE;
	;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1118 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPECULAR;
	;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1124 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxLights) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid light selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1135 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_TEXGEN;
	   (yyval.state)[1] = (yyvsp[(2) - (4)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (4)].integer) + (yyvsp[(4) - (4)].integer);
	;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1144 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S;
	;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1148 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_OBJECT_S;
	;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1153 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1157 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1161 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1165 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1171 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1178 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_COLOR;
	;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1182 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_PARAMS;
	;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1188 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_CLIPPLANE;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1196 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxClipPlanes) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid clip plane selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1207 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1214 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_SIZE;
	;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1218 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_ATTENUATION;
	;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1224 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (5)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (5)].state)[1];
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[4] = (yyvsp[(1) - (5)].state)[2];
	;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1234 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (2)].state)[1];
	   (yyval.state)[2] = (yyvsp[(2) - (2)].state)[2];
	   (yyval.state)[3] = (yyvsp[(2) - (2)].state)[3];
	   (yyval.state)[4] = (yyvsp[(1) - (2)].state)[2];
	;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1244 "program_parse.y"
    {
	   (yyval.state)[2] = 0;
	   (yyval.state)[3] = 3;
	;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1249 "program_parse.y"
    {
	   /* It seems logical that the matrix row range specifier would have
	    * to specify a range or more than one row (i.e., $5 > $3).
	    * However, the ARB_vertex_program spec says "a program will fail
	    * to load if <a> is greater than <b>."  This means that $3 == $5
	    * is valid.
	    */
	   if ((yyvsp[(3) - (6)].integer) > (yyvsp[(5) - (6)].integer)) {
	      yyerror(& (yylsp[(3) - (6)]), state, "invalid matrix row range");
	      YYERROR;
	   }

	   (yyval.state)[2] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(5) - (6)].integer);
	;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1267 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (3)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (3)].state)[1];
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1275 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1279 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1285 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVERSE;
	;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1289 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_TRANSPOSE;
	;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1293 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVTRANS;
	;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1299 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) > 3) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid matrix row reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1310 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MODELVIEW_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1315 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROJECTION_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1320 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MVP_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1325 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_TEXTURE_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1330 "program_parse.y"
    {
	   YYERROR;
	;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1334 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROGRAM_MATRIX;
	   (yyval.state)[1] = (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1341 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1345 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1350 "program_parse.y"
    {
	   /* Since GL_ARB_vertex_blend isn't supported, only modelview matrix
	    * zero is valid.
	    */
	   if ((yyvsp[(1) - (1)].integer) != 0) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid modelview matrix index");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1363 "program_parse.y"
    {
	   /* Since GL_ARB_matrix_palette isn't supported, just let any value
	    * through here.  The error will be generated later.
	    */
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1371 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxProgramMatrices) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program matrix selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1388 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1398 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1403 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1410 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1420 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1429 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1434 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1441 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1451 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->limits->MaxEnvParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid environment parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1461 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->limits->MaxLocalParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid local parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 1476 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1483 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1488 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (float) (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1495 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(2) - (3)].real);
	;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1500 "program_parse.y"
    {
	   (yyval.vector).count = 2;
	   (yyval.vector).data[0] = (yyvsp[(2) - (5)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (5)].real);
	;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1507 "program_parse.y"
    {
	   (yyval.vector).count = 3;
	   (yyval.vector).data[0] = (yyvsp[(2) - (7)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (7)].real);
	   (yyval.vector).data[1] = (yyvsp[(6) - (7)].real);
	;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1515 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (9)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (9)].real);
	   (yyval.vector).data[1] = (yyvsp[(6) - (9)].real);
	   (yyval.vector).data[1] = (yyvsp[(8) - (9)].real);
	;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1525 "program_parse.y"
    {
	   (yyval.real) = ((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].real) : (yyvsp[(2) - (2)].real);
	;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1529 "program_parse.y"
    {
	   (yyval.real) = (float)(((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].integer) : (yyvsp[(2) - (2)].integer));
	;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1534 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1535 "program_parse.y"
    { (yyval.negate) = TRUE;  ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1536 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1539 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1542 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1546 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(3) - (3)].string), (yyvsp[(0) - (3)].integer), & (yylsp[(3) - (3)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1552 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(1) - (1)].string), (yyvsp[(0) - (1)].integer), & (yylsp[(1) - (1)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1560 "program_parse.y"
    {
	   struct asm_symbol *const s =
	      declare_variable(state, (yyvsp[(2) - (4)].string), at_output, & (yylsp[(2) - (4)]));

	   if (s == NULL) {
	      YYERROR;
	   } else {
	      s->output_binding = (yyvsp[(4) - (4)].result);
	   }
	;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1573 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_HPOS;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1582 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_FOGC;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1591 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (2)].result);
	;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1595 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_PSIZ;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1604 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_TEX0 + (yyvsp[(3) - (3)].integer);
	   } else {
	      yyerror(& (yylsp[(2) - (3)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1613 "program_parse.y"
    {
	   if (state->mode == ARB_fragment) {
	      (yyval.result) = FRAG_RESULT_DEPTH;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1624 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (3)].integer) + (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1630 "program_parse.y"
    {
	   (yyval.integer) = (state->mode == ARB_vertex)
	      ? VERT_RESULT_COL0
	      : FRAG_RESULT_COLOR;
	;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1636 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_COL0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1645 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_BFC0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1656 "program_parse.y"
    {
	   (yyval.integer) = 0; 
	;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1660 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1669 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 1;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1679 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1680 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1681 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1684 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1685 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1686 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1689 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1690 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1693 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1694 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1697 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1698 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1702 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxTextureCoordUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture coordinate unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1713 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxTextureImageUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture image unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1724 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) >= state->MaxTextureUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1735 "program_parse.y"
    {
	   struct asm_symbol *exist = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(2) - (4)].string));
	   struct asm_symbol *target = (struct asm_symbol *)
	      _mesa_symbol_table_find_symbol(state->st, 0, (yyvsp[(4) - (4)].string));


	   if (exist != NULL) {
	      yyerror(& (yylsp[(2) - (4)]), state, "redeclared identifier");
	      YYERROR;
	   } else if (target == NULL) {
	      yyerror(& (yylsp[(4) - (4)]), state,
		      "undefined variable binding in ALIAS statement");
	      YYERROR;
	   } else {
	      _mesa_symbol_table_add_symbol(state->st, 0, (yyvsp[(2) - (4)].string), target);
	   }
	;}
    break;



/* Line 1455 of yacc.c  */
#line 4320 "program_parse.tab.c"
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

  yyerror_range[0] = yylloc;

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

  yyerror_range[0] = yylsp[1-yylen];
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

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, state);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
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



/* Line 1675 of yacc.c  */
#line 1755 "program_parse.y"


struct asm_instruction *
asm_instruction_ctor(gl_inst_opcode op,
		     const struct prog_dst_register *dst,
		     const struct asm_src_register *src0,
		     const struct asm_src_register *src1,
		     const struct asm_src_register *src2)
{
   struct asm_instruction *inst = calloc(1, sizeof(struct asm_instruction));

   if (inst) {
      _mesa_init_instructions(inst, 1);
      inst->Base.Opcode = op;
      inst->Base.DstReg = *dst;

      inst->Base.SrcReg[0] = src0->Base;
      inst->SrcReg[0] = *src0;

      if (src1 != NULL) {
	 inst->Base.SrcReg[1] = src1->Base;
	 inst->SrcReg[1] = *src1;
      } else {
	 init_src_reg(& inst->SrcReg[1]);
      }

      if (src2 != NULL) {
	 inst->Base.SrcReg[2] = src2->Base;
	 inst->SrcReg[2] = *src2;
      } else {
	 init_src_reg(& inst->SrcReg[2]);
      }
   }

   return inst;
}


void
init_dst_reg(struct prog_dst_register *r)
{
   memset(r, 0, sizeof(*r));
   r->File = PROGRAM_UNDEFINED;
   r->WriteMask = WRITEMASK_XYZW;
   r->CondMask = COND_TR;
   r->CondSwizzle = SWIZZLE_NOOP;
}


void
init_src_reg(struct asm_src_register *r)
{
   memset(r, 0, sizeof(*r));
   r->Base.File = PROGRAM_UNDEFINED;
   r->Base.Swizzle = SWIZZLE_NOOP;
   r->Symbol = NULL;
}


/**
 * Validate the set of inputs used by a program
 *
 * Validates that legal sets of inputs are used by the program.  In this case
 * "used" included both reading the input or binding the input to a name using
 * the \c ATTRIB command.
 *
 * \return
 * \c TRUE if the combination of inputs used is valid, \c FALSE otherwise.
 */
int
validate_inputs(struct YYLTYPE *locp, struct asm_parser_state *state)
{
   const int inputs = state->prog->InputsRead | state->InputsBound;

   if (((inputs & 0x0ffff) & (inputs >> 16)) != 0) {
      yyerror(locp, state, "illegal use of generic attribute and name attribute");
      return 0;
   }

   return 1;
}


struct asm_symbol *
declare_variable(struct asm_parser_state *state, char *name, enum asm_type t,
		 struct YYLTYPE *locp)
{
   struct asm_symbol *s = NULL;
   struct asm_symbol *exist = (struct asm_symbol *)
      _mesa_symbol_table_find_symbol(state->st, 0, name);


   if (exist != NULL) {
      yyerror(locp, state, "redeclared identifier");
   } else {
      s = calloc(1, sizeof(struct asm_symbol));
      s->name = name;
      s->type = t;

      switch (t) {
      case at_temp:
	 if (state->prog->NumTemporaries >= state->limits->MaxTemps) {
	    yyerror(locp, state, "too many temporaries declared");
	    free(s);
	    return NULL;
	 }

	 s->temp_binding = state->prog->NumTemporaries;
	 state->prog->NumTemporaries++;
	 break;

      case at_address:
	 if (state->prog->NumAddressRegs >= state->limits->MaxAddressRegs) {
	    yyerror(locp, state, "too many address registers declared");
	    free(s);
	    return NULL;
	 }

	 /* FINISHME: Add support for multiple address registers.
	  */
	 state->prog->NumAddressRegs++;
	 break;

      default:
	 break;
      }

      _mesa_symbol_table_add_symbol(state->st, 0, s->name, s);
   }

   return s;
}


int add_state_reference(struct gl_program_parameter_list *param_list,
			const gl_state_index tokens[STATE_LENGTH])
{
   const GLuint size = 4; /* XXX fix */
   char *name;
   GLint index;

   name = _mesa_program_state_string(tokens);
   index = _mesa_add_parameter(param_list, PROGRAM_STATE_VAR, name,
                               size, GL_NONE,
                               NULL, (gl_state_index *) tokens, 0x0);
   param_list->StateFlags |= _mesa_program_state_flags(tokens);

   /* free name string here since we duplicated it in add_parameter() */
   _mesa_free(name);

   return index;
}


int
initialize_symbol_from_state(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* If we are adding a STATE_MATRIX that has multiple rows, we need to
    * unroll it and call add_state_reference() for each row
    */
   if ((state_tokens[0] == STATE_MODELVIEW_MATRIX ||
	state_tokens[0] == STATE_PROJECTION_MATRIX ||
	state_tokens[0] == STATE_MVP_MATRIX ||
	state_tokens[0] == STATE_TEXTURE_MATRIX ||
	state_tokens[0] == STATE_PROGRAM_MATRIX)
       && (state_tokens[2] != state_tokens[3])) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U)
	    param_var->param_binding_begin = idx;
	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U)
	 param_var->param_binding_begin = idx;
      param_var->param_binding_length++;
   }

   return idx;
}


int
initialize_symbol_from_param(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const gl_state_index tokens[STATE_LENGTH])
{
   int idx = -1;
   gl_state_index state_tokens[STATE_LENGTH];


   memcpy(state_tokens, tokens, sizeof(state_tokens));

   assert((state_tokens[0] == STATE_VERTEX_PROGRAM)
	  || (state_tokens[0] == STATE_FRAGMENT_PROGRAM));
   assert((state_tokens[1] == STATE_ENV)
	  || (state_tokens[1] == STATE_LOCAL));

   param_var->type = at_param;
   param_var->param_binding_type = (state_tokens[1] == STATE_ENV)
     ? PROGRAM_ENV_PARAM : PROGRAM_LOCAL_PARAM;

   /* If we are adding a STATE_ENV or STATE_LOCAL that has multiple elements,
    * we need to unroll it and call add_state_reference() for each row
    */
   if (state_tokens[2] != state_tokens[3]) {
      int row;
      const int first_row = state_tokens[2];
      const int last_row = state_tokens[3];

      for (row = first_row; row <= last_row; row++) {
	 state_tokens[2] = state_tokens[3] = row;

	 idx = add_state_reference(prog->Parameters, state_tokens);
	 if (param_var->param_binding_begin == ~0U)
	    param_var->param_binding_begin = idx;
	 param_var->param_binding_length++;
      }
   }
   else {
      idx = add_state_reference(prog->Parameters, state_tokens);
      if (param_var->param_binding_begin == ~0U)
	 param_var->param_binding_begin = idx;
      param_var->param_binding_length++;
   }

   return idx;
}


int
initialize_symbol_from_const(struct gl_program *prog,
			     struct asm_symbol *param_var, 
			     const struct asm_vector *vec)
{
   const int idx = _mesa_add_parameter(prog->Parameters, PROGRAM_CONSTANT,
				       NULL, vec->count, GL_NONE, vec->data,
				       NULL, 0x0);

   param_var->type = at_param;
   param_var->param_binding_type = PROGRAM_CONSTANT;

   if (param_var->param_binding_begin == ~0U)
      param_var->param_binding_begin = idx;
   param_var->param_binding_length++;

   return idx;
}


char *
make_error_string(const char *fmt, ...)
{
   int length;
   char *str;
   va_list args;

   va_start(args, fmt);

   /* Call vsnprintf once to determine how large the final string is.  Call it
    * again to do the actual formatting.  from the vsnprintf manual page:
    *
    *    Upon successful return, these functions return the number of
    *    characters printed  (not including the trailing '\0' used to end
    *    output to strings).
    */
   length = 1 + vsnprintf(NULL, 0, fmt, args);

   str = _mesa_malloc(length);
   if (str) {
      vsnprintf(str, length, fmt, args);
   }

   va_end(args);

   return str;
}


void
yyerror(YYLTYPE *locp, struct asm_parser_state *state, const char *s)
{
   char *err_str;


   err_str = make_error_string("glProgramStringARB(%s)\n", s);
   if (err_str) {
      _mesa_error(state->ctx, GL_INVALID_OPERATION, err_str);
      _mesa_free(err_str);
   }

   err_str = make_error_string("line %u, char %u: error: %s\n",
			       locp->first_line, locp->first_column, s);
   _mesa_set_program_error(state->ctx, locp->position, err_str);

   if (err_str) {
      _mesa_free(err_str);
   }
}


GLboolean
_mesa_parse_arb_program(GLcontext *ctx, GLenum target, const GLubyte *str,
			GLsizei len, struct asm_parser_state *state)
{
   struct gl_program_constants limits;
   struct asm_instruction *inst;
   unsigned i;
   GLubyte *strz;

   state->ctx = ctx;
   state->prog->Target = target;
   state->prog->Parameters = _mesa_new_parameter_list();

   /* Make a copy of the program string and force it to be NUL-terminated.
    */
   strz = (GLubyte *) _mesa_malloc(len + 1);
   if (strz == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return GL_FALSE;
   }
   _mesa_memcpy (strz, str, len);
   strz[len] = '\0';

   state->prog->String = strz;

   state->st = _mesa_symbol_table_ctor();

   /* All of these limits should come from ctx.
    */
   limits.MaxInstructions = 128;
   limits.MaxAluInstructions = 128;
   limits.MaxTexInstructions = 128;
   limits.MaxTexIndirections = 128; 
   limits.MaxAttribs = 16;
   limits.MaxTemps = 128;
   limits.MaxAddressRegs = 1;
   limits.MaxParameters = 128;
   limits.MaxLocalParams = 256;
   limits.MaxEnvParams = 128;
   limits.MaxNativeInstructions = 128;
   limits.MaxNativeAluInstructions = 128;
   limits.MaxNativeTexInstructions = 128;
   limits.MaxNativeTexIndirections = 128;
   limits.MaxNativeAttribs = 16;
   limits.MaxNativeTemps = 128;
   limits.MaxNativeAddressRegs = 1;
   limits.MaxNativeParameters = 128;
   limits.MaxUniformComponents = 0;

   state->limits = & limits;

   state->MaxTextureImageUnits = 16;
   state->MaxTextureCoordUnits = 8;
   state->MaxTextureUnits = 8;
   state->MaxClipPlanes = 6;
   state->MaxLights = 8;
   state->MaxProgramMatrices = 8;

   state->state_param_enum = (target == GL_VERTEX_PROGRAM_ARB)
      ? STATE_VERTEX_PROGRAM : STATE_FRAGMENT_PROGRAM;

   _mesa_set_program_error(ctx, -1, NULL);

   _mesa_program_lexer_ctor(& state->scanner, state, (const char *) str, len);
   yyparse(state);
   _mesa_program_lexer_dtor(state->scanner);


   if (ctx->Program.ErrorPos != -1) {
      return GL_FALSE;
   }

   if (! _mesa_layout_parameters(state)) {
      struct YYLTYPE loc;

      loc.first_line = 0;
      loc.first_column = 0;
      loc.position = len;

      yyerror(& loc, state, "invalid PARAM usage");
      return GL_FALSE;
   }


   
   /* Add one instruction to store the "END" instruction.
    */
   state->prog->Instructions =
      _mesa_alloc_instructions(state->prog->NumInstructions + 1);
   inst = state->inst_head;
   for (i = 0; i < state->prog->NumInstructions; i++) {
      struct asm_instruction *const temp = inst->next;

      state->prog->Instructions[i] = inst->Base;
      _mesa_free(inst);

      inst = temp;
   }

   /* Finally, tag on an OPCODE_END instruction */
   {
      const GLuint numInst = state->prog->NumInstructions;
      _mesa_init_instructions(state->prog->Instructions + numInst, 1);
      state->prog->Instructions[numInst].Opcode = OPCODE_END;
   }
   state->prog->NumInstructions++;

   state->prog->NumParameters = state->prog->Parameters->NumParameters;

   /*
    * Initialize native counts to logical counts.  The device driver may
    * change them if program is translated into a hardware program.
    */
   state->prog->NumNativeInstructions = state->prog->NumInstructions;
   state->prog->NumNativeTemporaries = state->prog->NumTemporaries;
   state->prog->NumNativeParameters = state->prog->NumParameters;
   state->prog->NumNativeAttributes = state->prog->NumAttributes;
   state->prog->NumNativeAddressRegs = state->prog->NumAddressRegs;

   return GL_TRUE;
}

