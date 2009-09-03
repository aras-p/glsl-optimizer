
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
#include "main/imports.h"
#include "program.h"
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

static int add_state_reference(struct gl_program_parameter_list *param_list,
    const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_state(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_param(struct gl_program *prog,
    struct asm_symbol *param_var, const gl_state_index tokens[STATE_LENGTH]);

static int initialize_symbol_from_const(struct gl_program *prog,
    struct asm_symbol *param_var, const struct asm_vector *vec);

static int yyparse(struct asm_parser_state *state);

static char *make_error_string(const char *fmt, ...);

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
#line 174 "program_parse.tab.c"

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
     TXD_OP = 277,
     INTEGER = 278,
     REAL = 279,
     AMBIENT = 280,
     ATTENUATION = 281,
     BACK = 282,
     CLIP = 283,
     COLOR = 284,
     DEPTH = 285,
     DIFFUSE = 286,
     DIRECTION = 287,
     EMISSION = 288,
     ENV = 289,
     EYE = 290,
     FOG = 291,
     FOGCOORD = 292,
     FRAGMENT = 293,
     FRONT = 294,
     HALF = 295,
     INVERSE = 296,
     INVTRANS = 297,
     LIGHT = 298,
     LIGHTMODEL = 299,
     LIGHTPROD = 300,
     LOCAL = 301,
     MATERIAL = 302,
     MAT_PROGRAM = 303,
     MATRIX = 304,
     MATRIXINDEX = 305,
     MODELVIEW = 306,
     MVP = 307,
     NORMAL = 308,
     OBJECT = 309,
     PALETTE = 310,
     PARAMS = 311,
     PLANE = 312,
     POINT = 313,
     POINTSIZE = 314,
     POSITION = 315,
     PRIMARY = 316,
     PROGRAM = 317,
     PROJECTION = 318,
     RANGE = 319,
     RESULT = 320,
     ROW = 321,
     SCENECOLOR = 322,
     SECONDARY = 323,
     SHININESS = 324,
     SIZE = 325,
     SPECULAR = 326,
     SPOT = 327,
     STATE = 328,
     TEXCOORD = 329,
     TEXENV = 330,
     TEXGEN = 331,
     TEXGEN_Q = 332,
     TEXGEN_R = 333,
     TEXGEN_S = 334,
     TEXGEN_T = 335,
     TEXTURE = 336,
     TRANSPOSE = 337,
     TEXTURE_UNIT = 338,
     TEX_1D = 339,
     TEX_2D = 340,
     TEX_3D = 341,
     TEX_CUBE = 342,
     TEX_RECT = 343,
     TEX_SHADOW1D = 344,
     TEX_SHADOW2D = 345,
     TEX_SHADOWRECT = 346,
     TEX_ARRAY1D = 347,
     TEX_ARRAY2D = 348,
     TEX_ARRAYSHADOW1D = 349,
     TEX_ARRAYSHADOW2D = 350,
     VERTEX = 351,
     VTXATTRIB = 352,
     WEIGHT = 353,
     IDENTIFIER = 354,
     MASK4 = 355,
     MASK3 = 356,
     MASK2 = 357,
     MASK1 = 358,
     SWIZZLE = 359,
     DOT_DOT = 360,
     DOT = 361
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 107 "program_parse.y"

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

   struct {
      unsigned swz;
      unsigned rgba_valid:1;
      unsigned xyzw_valid:1;
      unsigned negate:1;
   } ext_swizzle;



/* Line 214 of yacc.c  */
#line 344 "program_parse.tab.c"
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
#line 249 "program_parse.y"

extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param,
    void *yyscanner);


/* Line 264 of yacc.c  */
#line 375 "program_parse.tab.c"

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
#define YYLAST   375

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  117
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  136
/* YYNRULES -- Number of rules.  */
#define YYNRULES  270
/* YYNRULES -- Number of states.  */
#define YYNSTATES  456

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   361

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   112,   108,   113,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   107,
       2,   114,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   110,     2,   111,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   115,   109,   116,     2,     2,     2,     2,
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
     105,   106
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     8,    10,    12,    15,    16,    20,    23,
      24,    27,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    54,    59,    64,    69,    76,    83,
      92,   101,   104,   117,   120,   122,   124,   126,   128,   130,
     132,   134,   136,   138,   140,   142,   144,   151,   154,   159,
     162,   164,   168,   174,   177,   180,   188,   191,   193,   195,
     197,   199,   204,   206,   208,   210,   212,   214,   216,   218,
     222,   223,   226,   229,   231,   233,   235,   237,   239,   241,
     243,   245,   247,   248,   250,   252,   254,   256,   257,   259,
     261,   263,   265,   267,   269,   274,   277,   280,   282,   285,
     287,   290,   292,   295,   300,   305,   307,   308,   312,   314,
     316,   319,   321,   324,   326,   328,   332,   339,   340,   342,
     345,   350,   352,   356,   358,   360,   362,   364,   366,   368,
     370,   372,   374,   376,   379,   382,   385,   388,   391,   394,
     397,   400,   403,   406,   409,   412,   416,   418,   420,   422,
     428,   430,   432,   434,   437,   439,   441,   444,   446,   449,
     456,   458,   462,   464,   466,   468,   470,   472,   477,   479,
     481,   483,   485,   487,   489,   492,   494,   496,   502,   504,
     507,   509,   511,   517,   520,   521,   528,   532,   533,   535,
     537,   539,   541,   543,   546,   548,   550,   553,   558,   563,
     564,   566,   568,   570,   572,   575,   577,   579,   581,   583,
     589,   591,   595,   601,   607,   609,   613,   619,   621,   623,
     625,   627,   629,   631,   633,   635,   637,   641,   647,   655,
     665,   668,   671,   673,   675,   676,   677,   681,   682,   686,
     690,   692,   697,   700,   703,   706,   709,   713,   716,   720,
     721,   723,   725,   726,   728,   730,   731,   733,   735,   736,
     738,   740,   741,   745,   746,   750,   751,   755,   757,   759,
     761
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     118,     0,    -1,   119,   120,   122,    12,    -1,     3,    -1,
       4,    -1,   120,   121,    -1,    -1,     8,    99,   107,    -1,
     122,   123,    -1,    -1,   124,   107,    -1,   162,   107,    -1,
     125,    -1,   126,    -1,   127,    -1,   128,    -1,   129,    -1,
     130,    -1,   131,    -1,   132,    -1,   138,    -1,   133,    -1,
     134,    -1,   135,    -1,    19,   143,   108,   139,    -1,    18,
     142,   108,   141,    -1,    16,   142,   108,   139,    -1,    14,
     142,   108,   139,   108,   139,    -1,    13,   142,   108,   141,
     108,   141,    -1,    17,   142,   108,   141,   108,   141,   108,
     141,    -1,    15,   142,   108,   141,   108,   136,   108,   137,
      -1,    20,   141,    -1,    22,   142,   108,   141,   108,   141,
     108,   141,   108,   136,   108,   137,    -1,    83,   247,    -1,
      84,    -1,    85,    -1,    86,    -1,    87,    -1,    88,    -1,
      89,    -1,    90,    -1,    91,    -1,    92,    -1,    93,    -1,
      94,    -1,    95,    -1,    21,   142,   108,   147,   108,   144,
      -1,   233,   140,    -1,   233,   109,   140,   109,    -1,   147,
     159,    -1,   230,    -1,   233,   147,   160,    -1,   233,   109,
     147,   160,   109,    -1,   148,   161,    -1,   156,   158,    -1,
     145,   108,   145,   108,   145,   108,   145,    -1,   233,   146,
      -1,    23,    -1,    99,    -1,    99,    -1,   164,    -1,   149,
     110,   150,   111,    -1,   178,    -1,   240,    -1,    99,    -1,
      99,    -1,   151,    -1,   152,    -1,    23,    -1,   156,   157,
     153,    -1,    -1,   112,   154,    -1,   113,   155,    -1,    23,
      -1,    23,    -1,    99,    -1,   103,    -1,   103,    -1,   103,
      -1,   103,    -1,   100,    -1,   104,    -1,    -1,   100,    -1,
     101,    -1,   102,    -1,   103,    -1,    -1,   163,    -1,   170,
      -1,   234,    -1,   236,    -1,   239,    -1,   252,    -1,     7,
      99,   114,   164,    -1,    96,   165,    -1,    38,   169,    -1,
      60,    -1,    98,   167,    -1,    53,    -1,    29,   245,    -1,
      37,    -1,    74,   246,    -1,    50,   110,   168,   111,    -1,
      97,   110,   166,   111,    -1,    23,    -1,    -1,   110,   168,
     111,    -1,    23,    -1,    60,    -1,    29,   245,    -1,    37,
      -1,    74,   246,    -1,   171,    -1,   172,    -1,    10,    99,
     174,    -1,    10,    99,   110,   173,   111,   175,    -1,    -1,
      23,    -1,   114,   177,    -1,   114,   115,   176,   116,    -1,
     179,    -1,   176,   108,   179,    -1,   181,    -1,   217,    -1,
     227,    -1,   181,    -1,   217,    -1,   228,    -1,   180,    -1,
     218,    -1,   227,    -1,   181,    -1,    73,   205,    -1,    73,
     182,    -1,    73,   184,    -1,    73,   187,    -1,    73,   189,
      -1,    73,   195,    -1,    73,   191,    -1,    73,   198,    -1,
      73,   200,    -1,    73,   202,    -1,    73,   204,    -1,    73,
     216,    -1,    47,   244,   183,    -1,   193,    -1,    33,    -1,
      69,    -1,    43,   110,   194,   111,   185,    -1,   193,    -1,
      60,    -1,    26,    -1,    72,   186,    -1,    40,    -1,    32,
      -1,    44,   188,    -1,    25,    -1,   244,    67,    -1,    45,
     110,   194,   111,   244,   190,    -1,   193,    -1,    75,   248,
     192,    -1,    29,    -1,    25,    -1,    31,    -1,    71,    -1,
      23,    -1,    76,   246,   196,   197,    -1,    35,    -1,    54,
      -1,    79,    -1,    80,    -1,    78,    -1,    77,    -1,    36,
     199,    -1,    29,    -1,    56,    -1,    28,   110,   201,   111,
      57,    -1,    23,    -1,    58,   203,    -1,    70,    -1,    26,
      -1,   207,    66,   110,   210,   111,    -1,   207,   206,    -1,
      -1,    66,   110,   210,   105,   210,   111,    -1,    49,   211,
     208,    -1,    -1,   209,    -1,    41,    -1,    82,    -1,    42,
      -1,    23,    -1,    51,   212,    -1,    63,    -1,    52,    -1,
      81,   246,    -1,    55,   110,   214,   111,    -1,    48,   110,
     215,   111,    -1,    -1,   213,    -1,    23,    -1,    23,    -1,
      23,    -1,    30,    64,    -1,   221,    -1,   224,    -1,   219,
      -1,   222,    -1,    62,    34,   110,   220,   111,    -1,   225,
      -1,   225,   105,   225,    -1,    62,    34,   110,   225,   111,
      -1,    62,    46,   110,   223,   111,    -1,   226,    -1,   226,
     105,   226,    -1,    62,    46,   110,   226,   111,    -1,    23,
      -1,    23,    -1,   229,    -1,   231,    -1,   230,    -1,   231,
      -1,   232,    -1,    24,    -1,    23,    -1,   115,   232,   116,
      -1,   115,   232,   108,   232,   116,    -1,   115,   232,   108,
     232,   108,   232,   116,    -1,   115,   232,   108,   232,   108,
     232,   108,   232,   116,    -1,   233,    24,    -1,   233,    23,
      -1,   112,    -1,   113,    -1,    -1,    -1,    11,   235,   238,
      -1,    -1,     5,   237,   238,    -1,   238,   108,    99,    -1,
      99,    -1,     9,    99,   114,   240,    -1,    65,    60,    -1,
      65,    37,    -1,    65,   241,    -1,    65,    59,    -1,    65,
      74,   246,    -1,    65,    30,    -1,    29,   242,   243,    -1,
      -1,    39,    -1,    27,    -1,    -1,    61,    -1,    68,    -1,
      -1,    39,    -1,    27,    -1,    -1,    61,    -1,    68,    -1,
      -1,   110,   249,   111,    -1,    -1,   110,   250,   111,    -1,
      -1,   110,   251,   111,    -1,    23,    -1,    23,    -1,    23,
      -1,     6,    99,   114,    99,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   256,   256,   259,   267,   279,   280,   283,   305,   306,
     309,   324,   327,   332,   339,   340,   341,   342,   343,   344,
     345,   348,   349,   350,   353,   359,   366,   373,   381,   388,
     396,   441,   448,   493,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   513,   526,   534,   551,
     558,   577,   588,   608,   630,   639,   672,   679,   694,   744,
     786,   797,   818,   828,   834,   865,   882,   882,   884,   891,
     903,   904,   905,   908,   920,   932,   950,   961,   973,   975,
     976,   977,   978,   981,   981,   981,   981,   982,   985,   986,
     987,   988,   989,   990,   993,  1011,  1015,  1021,  1025,  1029,
    1033,  1042,  1051,  1055,  1060,  1066,  1077,  1077,  1078,  1080,
    1084,  1088,  1092,  1098,  1098,  1100,  1116,  1139,  1142,  1153,
    1159,  1165,  1166,  1173,  1179,  1185,  1193,  1199,  1205,  1213,
    1219,  1225,  1233,  1234,  1237,  1238,  1239,  1240,  1241,  1242,
    1243,  1244,  1245,  1246,  1247,  1250,  1259,  1263,  1267,  1273,
    1282,  1286,  1290,  1299,  1303,  1309,  1315,  1322,  1327,  1335,
    1345,  1347,  1355,  1361,  1365,  1369,  1375,  1386,  1395,  1399,
    1404,  1408,  1412,  1416,  1422,  1429,  1433,  1439,  1447,  1458,
    1465,  1469,  1475,  1485,  1496,  1500,  1518,  1527,  1530,  1536,
    1540,  1544,  1550,  1561,  1566,  1571,  1576,  1581,  1586,  1594,
    1597,  1602,  1615,  1623,  1634,  1642,  1642,  1644,  1644,  1646,
    1656,  1661,  1668,  1678,  1687,  1692,  1699,  1709,  1719,  1731,
    1731,  1732,  1732,  1734,  1744,  1752,  1762,  1770,  1778,  1787,
    1798,  1802,  1808,  1809,  1810,  1813,  1813,  1816,  1816,  1819,
    1825,  1833,  1846,  1855,  1864,  1868,  1877,  1886,  1897,  1904,
    1909,  1918,  1930,  1933,  1942,  1953,  1954,  1955,  1958,  1959,
    1960,  1963,  1964,  1967,  1968,  1971,  1972,  1975,  1986,  1997,
    2008
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
  "KIL", "SWZ", "TXD_OP", "INTEGER", "REAL", "AMBIENT", "ATTENUATION",
  "BACK", "CLIP", "COLOR", "DEPTH", "DIFFUSE", "DIRECTION", "EMISSION",
  "ENV", "EYE", "FOG", "FOGCOORD", "FRAGMENT", "FRONT", "HALF", "INVERSE",
  "INVTRANS", "LIGHT", "LIGHTMODEL", "LIGHTPROD", "LOCAL", "MATERIAL",
  "MAT_PROGRAM", "MATRIX", "MATRIXINDEX", "MODELVIEW", "MVP", "NORMAL",
  "OBJECT", "PALETTE", "PARAMS", "PLANE", "POINT", "POINTSIZE", "POSITION",
  "PRIMARY", "PROGRAM", "PROJECTION", "RANGE", "RESULT", "ROW",
  "SCENECOLOR", "SECONDARY", "SHININESS", "SIZE", "SPECULAR", "SPOT",
  "STATE", "TEXCOORD", "TEXENV", "TEXGEN", "TEXGEN_Q", "TEXGEN_R",
  "TEXGEN_S", "TEXGEN_T", "TEXTURE", "TRANSPOSE", "TEXTURE_UNIT", "TEX_1D",
  "TEX_2D", "TEX_3D", "TEX_CUBE", "TEX_RECT", "TEX_SHADOW1D",
  "TEX_SHADOW2D", "TEX_SHADOWRECT", "TEX_ARRAY1D", "TEX_ARRAY2D",
  "TEX_ARRAYSHADOW1D", "TEX_ARRAYSHADOW2D", "VERTEX", "VTXATTRIB",
  "WEIGHT", "IDENTIFIER", "MASK4", "MASK3", "MASK2", "MASK1", "SWIZZLE",
  "DOT_DOT", "DOT", "';'", "','", "'|'", "'['", "']'", "'+'", "'-'", "'='",
  "'{'", "'}'", "$accept", "program", "language", "optionSequence",
  "option", "statementSequence", "statement", "instruction",
  "ALU_instruction", "TexInstruction", "ARL_instruction",
  "VECTORop_instruction", "SCALARop_instruction", "BINSCop_instruction",
  "BINop_instruction", "TRIop_instruction", "SAMPLE_instruction",
  "KIL_instruction", "TXD_instruction", "texImageUnit", "texTarget",
  "SWZ_instruction", "scalarSrcReg", "scalarUse", "swizzleSrcReg",
  "maskedDstReg", "maskedAddrReg", "extendedSwizzle", "extSwizComp",
  "extSwizSel", "srcReg", "dstReg", "progParamArray", "progParamArrayMem",
  "progParamArrayAbs", "progParamArrayRel", "addrRegRelOffset",
  "addrRegPosOffset", "addrRegNegOffset", "addrReg", "addrComponent",
  "addrWriteMask", "scalarSuffix", "swizzleSuffix", "optionalMask",
  "namingStatement", "ATTRIB_statement", "attribBinding", "vtxAttribItem",
  "vtxAttribNum", "vtxOptWeightNum", "vtxWeightNum", "fragAttribItem",
  "PARAM_statement", "PARAM_singleStmt", "PARAM_multipleStmt",
  "optArraySize", "paramSingleInit", "paramMultipleInit",
  "paramMultInitList", "paramSingleItemDecl", "paramSingleItemUse",
  "paramMultipleItem", "stateMultipleItem", "stateSingleItem",
  "stateMaterialItem", "stateMatProperty", "stateLightItem",
  "stateLightProperty", "stateSpotProperty", "stateLightModelItem",
  "stateLModProperty", "stateLightProdItem", "stateLProdProperty",
  "stateTexEnvItem", "stateTexEnvProperty", "ambDiffSpecProperty",
  "stateLightNumber", "stateTexGenItem", "stateTexGenType",
  "stateTexGenCoord", "stateFogItem", "stateFogProperty",
  "stateClipPlaneItem", "stateClipPlaneNum", "statePointItem",
  "statePointProperty", "stateMatrixRow", "stateMatrixRows",
  "optMatrixRows", "stateMatrixItem", "stateOptMatModifier",
  "stateMatModifier", "stateMatrixRowNum", "stateMatrixName",
  "stateOptModMatNum", "stateModMatNum", "statePaletteMatNum",
  "stateProgramMatNum", "stateDepthItem", "programSingleItem",
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
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,    59,    44,   124,
      91,    93,    43,    45,    61,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   117,   118,   119,   119,   120,   120,   121,   122,   122,
     123,   123,   124,   124,   125,   125,   125,   125,   125,   125,
     125,   126,   126,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   137,   137,   137,   137,   137,
     137,   137,   137,   137,   137,   137,   138,   139,   139,   140,
     140,   141,   141,   142,   143,   144,   145,   146,   146,   147,
     147,   147,   147,   148,   148,   149,   150,   150,   151,   152,
     153,   153,   153,   154,   155,   156,   157,   158,   159,   160,
     160,   160,   160,   161,   161,   161,   161,   161,   162,   162,
     162,   162,   162,   162,   163,   164,   164,   165,   165,   165,
     165,   165,   165,   165,   165,   166,   167,   167,   168,   169,
     169,   169,   169,   170,   170,   171,   172,   173,   173,   174,
     175,   176,   176,   177,   177,   177,   178,   178,   178,   179,
     179,   179,   180,   180,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   182,   183,   183,   183,   184,
     185,   185,   185,   185,   185,   186,   187,   188,   188,   189,
     190,   191,   192,   193,   193,   193,   194,   195,   196,   196,
     197,   197,   197,   197,   198,   199,   199,   200,   201,   202,
     203,   203,   204,   205,   206,   206,   207,   208,   208,   209,
     209,   209,   210,   211,   211,   211,   211,   211,   211,   212,
     212,   213,   214,   215,   216,   217,   217,   218,   218,   219,
     220,   220,   221,   222,   223,   223,   224,   225,   226,   227,
     227,   228,   228,   229,   230,   230,   231,   231,   231,   231,
     232,   232,   233,   233,   233,   235,   234,   237,   236,   238,
     238,   239,   240,   240,   240,   240,   240,   240,   241,   242,
     242,   242,   243,   243,   243,   244,   244,   244,   245,   245,
     245,   246,   246,   247,   247,   248,   248,   249,   250,   251,
     252
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     1,     1,     2,     0,     3,     2,     0,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     4,     4,     6,     6,     8,
       8,     2,    12,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     6,     2,     4,     2,
       1,     3,     5,     2,     2,     7,     2,     1,     1,     1,
       1,     4,     1,     1,     1,     1,     1,     1,     1,     3,
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     4,     2,     2,     1,     2,     1,
       2,     1,     2,     4,     4,     1,     0,     3,     1,     1,
       2,     1,     2,     1,     1,     3,     6,     0,     1,     2,
       4,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     1,     1,     1,     5,
       1,     1,     1,     2,     1,     1,     2,     1,     2,     6,
       1,     3,     1,     1,     1,     1,     1,     4,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     5,     1,     2,
       1,     1,     5,     2,     0,     6,     3,     0,     1,     1,
       1,     1,     1,     2,     1,     1,     2,     4,     4,     0,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     5,
       1,     3,     5,     5,     1,     3,     5,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     5,     7,     9,
       2,     2,     1,     1,     0,     0,     3,     0,     3,     3,
       1,     4,     2,     2,     2,     2,     3,     2,     3,     0,
       1,     1,     0,     1,     1,     0,     1,     1,     0,     1,
       1,     0,     3,     0,     3,     0,     3,     1,     1,     1,
       4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     3,     4,     0,     6,     1,     9,     0,     5,     0,
       0,   237,     0,     0,     0,     0,   235,     2,     0,     0,
       0,     0,     0,     0,     0,   234,     0,     0,     8,     0,
      12,    13,    14,    15,    16,    17,    18,    19,    21,    22,
      23,    20,     0,    88,    89,   113,   114,    90,    91,    92,
      93,     7,     0,     0,     0,     0,     0,     0,     0,    64,
       0,    87,    63,     0,     0,     0,     0,     0,    75,     0,
       0,   232,   233,    31,     0,     0,     0,    10,    11,   240,
     238,     0,     0,     0,   117,   234,   115,   236,   249,   247,
     243,   245,   242,   261,   244,   234,    83,    84,    85,    86,
      53,   234,   234,   234,   234,   234,   234,    77,    54,   225,
     224,     0,     0,     0,     0,    59,     0,   234,    82,     0,
      60,    62,   126,   127,   205,   206,   128,   221,   222,     0,
     234,     0,   270,    94,   241,   118,     0,   119,   123,   124,
     125,   219,   220,   223,     0,   251,   250,   252,     0,   246,
       0,     0,     0,     0,    26,     0,    25,    24,   258,   111,
     109,   261,    96,     0,     0,     0,     0,     0,     0,   255,
       0,   255,     0,     0,   265,   261,   134,   135,   136,   137,
     139,   138,   140,   141,   142,   143,     0,   144,   258,   101,
       0,    99,    97,   261,     0,   106,    95,    82,     0,    80,
      79,    81,    51,     0,     0,     0,   239,     0,   231,   230,
     253,   254,   248,   267,     0,   234,   234,     0,    47,     0,
      50,     0,   234,   259,   260,   110,   112,     0,     0,     0,
     204,   175,   176,   174,     0,   157,   257,   256,   156,     0,
       0,     0,     0,   199,   195,     0,   194,   261,   187,   181,
     180,   179,     0,     0,     0,     0,   100,     0,   102,     0,
       0,    98,     0,   234,   226,    68,     0,    66,    67,     0,
     234,   234,     0,   116,   262,    28,    27,     0,    78,    49,
     263,     0,     0,   217,     0,   218,     0,   178,     0,   166,
       0,   158,     0,   163,   164,   147,   148,   165,   145,   146,
       0,   201,   193,   200,     0,   196,   189,   191,   190,   186,
     188,   269,     0,   162,   161,   168,   169,     0,     0,   108,
       0,   105,     0,     0,    52,     0,    61,    76,    70,    46,
       0,     0,     0,   234,    48,     0,    33,     0,   234,   212,
     216,     0,     0,   255,   203,     0,   202,     0,   266,   173,
     172,   170,   171,   167,   192,     0,   103,   104,   107,   234,
     227,     0,     0,    69,   234,    57,    58,    56,   234,     0,
       0,     0,   121,   129,   132,   130,   207,   208,   131,   268,
       0,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    30,    29,   177,   152,   154,   151,     0,
     149,   150,     0,   198,   197,   182,     0,    73,    71,    74,
      72,     0,     0,     0,     0,   133,   184,   234,   120,   264,
     155,   153,   159,   160,   234,   228,   234,     0,     0,     0,
       0,   183,   122,     0,     0,     0,     0,   210,     0,   214,
       0,   229,   234,     0,   209,     0,   213,     0,     0,    55,
      32,   211,   215,     0,     0,   185
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     6,     8,     9,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,   281,
     393,    41,   151,   218,    73,    60,    69,   329,   330,   367,
     219,    61,   119,   266,   267,   268,   363,   408,   410,    70,
     328,   108,   279,   202,   100,    42,    43,   120,   196,   322,
     261,   320,   162,    44,    45,    46,   136,    86,   273,   371,
     137,   121,   372,   373,   122,   176,   298,   177,   400,   421,
     178,   238,   179,   422,   180,   314,   299,   290,   181,   317,
     353,   182,   233,   183,   288,   184,   251,   185,   415,   431,
     186,   309,   310,   355,   248,   302,   303,   347,   345,   187,
     123,   375,   376,   436,   124,   377,   438,   125,   284,   286,
     378,   126,   141,   127,   128,   143,    74,    47,    57,    48,
      52,    80,    49,    62,    94,   147,   212,   239,   225,   149,
     336,   253,   214,   380,   312,    50
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -403
static const yytype_int16 yypact[] =
{
      62,  -403,  -403,    41,  -403,  -403,    81,   -36,  -403,   198,
      -8,  -403,    -5,    18,    20,    35,  -403,  -403,   -23,   -23,
     -23,   -23,   -23,   -23,    45,   131,   -23,   -23,  -403,    58,
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,    67,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,    63,    86,    92,   128,    84,    63,    99,  -403,
      69,   133,  -403,    87,    89,   129,   145,   146,  -403,   147,
     154,  -403,  -403,  -403,   -13,   150,   151,  -403,  -403,  -403,
     152,   162,   -14,   197,   240,   -34,  -403,   152,    98,  -403,
    -403,  -403,  -403,   155,  -403,   131,  -403,  -403,  -403,  -403,
    -403,   131,   131,   131,   131,   131,   131,  -403,  -403,  -403,
    -403,    72,   105,    77,    17,   156,    12,   131,    68,   157,
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,    12,
     131,   165,  -403,  -403,  -403,  -403,   158,  -403,  -403,  -403,
    -403,  -403,  -403,  -403,   223,  -403,  -403,   -16,   245,  -403,
     163,   164,    -9,   167,  -403,   168,  -403,  -403,    55,  -403,
    -403,   155,  -403,   170,   171,   172,   206,     2,   173,    34,
     174,   127,   112,     1,   175,   155,  -403,  -403,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,  -403,   207,  -403,    55,  -403,
     177,  -403,  -403,   155,   178,   179,  -403,    68,    22,  -403,
    -403,  -403,  -403,    -6,   169,   182,  -403,   180,  -403,  -403,
    -403,  -403,  -403,  -403,   181,   131,   131,    12,  -403,   188,
     190,   195,   131,  -403,  -403,  -403,  -403,   272,   273,   274,
    -403,  -403,  -403,  -403,   275,  -403,  -403,  -403,  -403,   232,
     275,    79,   191,   277,  -403,   192,  -403,   155,    15,  -403,
    -403,  -403,   280,   276,     8,   194,  -403,   283,  -403,   284,
     283,  -403,   199,   131,  -403,  -403,   200,  -403,  -403,   209,
     131,   131,   201,  -403,  -403,  -403,  -403,   204,  -403,  -403,
     205,   210,   211,  -403,   203,  -403,   212,  -403,   213,  -403,
     214,  -403,   215,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
     286,  -403,  -403,  -403,   294,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,   216,  -403,  -403,  -403,  -403,   161,   297,  -403,
     217,  -403,   218,   219,  -403,    76,  -403,  -403,   139,  -403,
     227,    -4,   228,    30,  -403,   298,  -403,   137,   131,  -403,
    -403,   265,   130,   127,  -403,   220,  -403,   226,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,   229,  -403,  -403,  -403,   131,
    -403,   315,   319,  -403,   131,  -403,  -403,  -403,   131,   123,
      77,    80,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
     233,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,   311,
    -403,  -403,     9,  -403,  -403,  -403,    83,  -403,  -403,  -403,
    -403,   237,   238,   239,   241,  -403,   281,    30,  -403,  -403,
    -403,  -403,  -403,  -403,   131,  -403,   131,   195,   272,   273,
     242,  -403,  -403,   234,   246,   247,   248,   243,   249,   251,
     297,  -403,   131,   137,  -403,   272,  -403,   273,    36,  -403,
    -403,  -403,  -403,   297,   250,  -403
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,   -74,
     -81,  -403,   -98,   141,   -82,   160,  -403,  -403,  -358,  -403,
     -41,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,   166,
    -403,  -403,  -403,   176,  -403,  -403,  -403,   282,  -403,  -403,
    -403,   103,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,   -52,  -403,   -84,  -403,  -403,  -403,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,  -403,  -333,   126,  -403,  -403,
    -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -403,
      -3,  -403,  -403,  -402,  -403,  -403,  -403,  -403,  -403,  -403,
     285,  -403,  -403,  -403,  -403,  -403,  -403,  -403,  -398,  -392,
     287,  -403,  -403,  -145,   -83,  -114,   -85,  -403,  -403,  -403,
    -403,   314,  -403,   291,  -403,  -403,  -403,  -167,   187,  -149,
    -403,  -403,  -403,  -403,  -403,  -403
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -222
static const yytype_int16 yytable[] =
{
     144,   138,   142,   198,   241,   154,   411,   220,   157,   401,
     109,   110,   226,   150,   109,   110,   152,   265,   152,   365,
     153,   152,   155,   156,   111,   111,   254,   249,   112,   111,
     437,   231,   144,   118,   293,   109,   110,   439,   448,   113,
     294,     5,    58,   315,   258,   210,   188,   451,   205,   112,
     111,   454,   211,   112,   189,   452,   306,   307,   232,   235,
     113,   236,   316,    10,   113,     1,     2,   190,   434,   423,
     191,   250,   220,   237,   112,   197,    59,   192,    71,    72,
     297,   117,   114,   114,   449,   113,   115,   114,   204,     7,
     115,   193,   369,    68,    53,   366,   116,   308,   305,    51,
     217,   158,   117,   370,   293,   165,   117,   166,   114,   159,
     294,   115,   295,   167,   194,   195,   223,    54,   276,    55,
     168,   169,   170,   224,   171,   145,   172,   117,    88,    89,
     263,   152,   160,   275,    56,   173,    90,   146,   264,   163,
     282,   453,    71,    72,    68,   117,   161,   405,   296,   325,
     297,   164,   174,   175,   236,   293,   396,   413,    91,    92,
     242,   294,    79,   243,   244,    77,   237,   245,   199,   414,
     397,   200,   201,    93,    78,   246,   402,    95,   144,    63,
      64,    65,    66,    67,   359,   331,    75,    76,   417,   332,
     398,   424,   360,   247,    84,   101,   418,   102,    85,   425,
      81,   297,   399,    11,    12,    13,    82,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,    96,    97,    98,    99,   103,   349,   350,
     351,   352,    83,    71,    72,   406,   208,   209,   144,   374,
     142,   361,   362,   104,   105,   106,   394,   107,   129,   130,
     131,   132,    58,   135,   206,   148,   -65,   203,   213,   207,
     230,   215,   216,   255,   144,   221,   222,   270,   280,   331,
     227,   228,   229,   234,   240,   252,   412,   257,   259,   260,
     271,   278,   274,  -221,   272,   283,   285,   287,   289,   291,
     301,   300,   304,   311,   318,   313,   319,   321,   324,   344,
     433,   326,   327,   334,   339,   335,   333,   346,   337,   338,
     354,   379,   395,   340,   341,   342,   343,   348,   356,   357,
     358,   403,   144,   374,   142,   364,   368,   404,   407,   144,
     405,   331,   409,   420,   419,   426,   427,   430,   445,   428,
     441,   429,   440,   435,   442,   443,   447,   331,   277,   444,
     446,   455,   450,   323,   133,   432,   292,   416,     0,   269,
     139,    87,   140,   262,   134,   256
};

static const yytype_int16 yycheck[] =
{
      85,    85,    85,   117,   171,   103,   364,   152,   106,   342,
      23,    24,   161,    95,    23,    24,   101,    23,   103,    23,
     102,   106,   104,   105,    38,    38,   175,    26,    62,    38,
     428,    29,   117,    74,    25,    23,    24,   429,   440,    73,
      31,     0,    65,    35,   193,    61,    29,   445,   130,    62,
      38,   453,    68,    62,    37,   447,    41,    42,    56,    25,
      73,    27,    54,    99,    73,     3,     4,    50,   426,   402,
      53,    70,   217,    39,    62,   116,    99,    60,   112,   113,
      71,   115,    96,    96,   442,    73,    99,    96,   129,     8,
      99,    74,    62,    99,    99,    99,   109,    82,   247,   107,
     109,    29,   115,    73,    25,    28,   115,    30,    96,    37,
      31,    99,    33,    36,    97,    98,    61,    99,   216,    99,
      43,    44,    45,    68,    47,    27,    49,   115,    29,    30,
     108,   216,    60,   215,    99,    58,    37,    39,   116,    34,
     222,   105,   112,   113,    99,   115,    74,   111,    69,   263,
      71,    46,    75,    76,    27,    25,    26,    34,    59,    60,
      48,    31,    99,    51,    52,   107,    39,    55,   100,    46,
      40,   103,   104,    74,   107,    63,   343,   108,   263,    19,
      20,    21,    22,    23,   108,   270,    26,    27,   108,   271,
      60,   108,   116,    81,   110,   108,   116,   108,   114,   116,
     114,    71,    72,     5,     6,     7,   114,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,   100,   101,   102,   103,   108,    77,    78,
      79,    80,   114,   112,   113,   359,    23,    24,   333,   333,
     333,   112,   113,   108,   108,   108,   338,   103,   108,   108,
     108,    99,    65,    23,    99,   110,   110,   110,    23,   111,
      64,   108,   108,    66,   359,   108,   108,   108,    83,   364,
     110,   110,   110,   110,   110,   110,   368,   110,   110,   110,
     108,   103,   111,   103,   114,    23,    23,    23,    23,    67,
      23,   110,   110,    23,   110,    29,    23,    23,   109,    23,
     424,   111,   103,   109,   111,   110,   115,    23,   108,   108,
      23,    23,    57,   111,   111,   111,   111,   111,   111,   111,
     111,   111,   417,   417,   417,   108,   108,   111,    23,   424,
     111,   426,    23,    32,   111,   108,   108,    66,   105,   110,
     116,   110,   110,   427,   108,   108,   105,   442,   217,   111,
     111,   111,   443,   260,    82,   417,   240,   370,    -1,   203,
      85,    57,    85,   197,    83,   188
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,   118,   119,     0,   120,     8,   121,   122,
      99,     5,     6,     7,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   138,   162,   163,   170,   171,   172,   234,   236,   239,
     252,   107,   237,    99,    99,    99,    99,   235,    65,    99,
     142,   148,   240,   142,   142,   142,   142,   142,    99,   143,
     156,   112,   113,   141,   233,   142,   142,   107,   107,    99,
     238,   114,   114,   114,   110,   114,   174,   238,    29,    30,
      37,    59,    60,    74,   241,   108,   100,   101,   102,   103,
     161,   108,   108,   108,   108,   108,   108,   103,   158,    23,
      24,    38,    62,    73,    96,    99,   109,   115,   147,   149,
     164,   178,   181,   217,   221,   224,   228,   230,   231,   108,
     108,   108,    99,   164,   240,    23,   173,   177,   181,   217,
     227,   229,   231,   232,   233,    27,    39,   242,   110,   246,
     141,   139,   233,   141,   139,   141,   141,   139,    29,    37,
      60,    74,   169,    34,    46,    28,    30,    36,    43,    44,
      45,    47,    49,    58,    75,    76,   182,   184,   187,   189,
     191,   195,   198,   200,   202,   204,   207,   216,    29,    37,
      50,    53,    60,    74,    97,    98,   165,   147,   232,   100,
     103,   104,   160,   110,   147,   141,    99,   111,    23,    24,
      61,    68,   243,    23,   249,   108,   108,   109,   140,   147,
     230,   108,   108,    61,    68,   245,   246,   110,   110,   110,
      64,    29,    56,   199,   110,    25,    27,    39,   188,   244,
     110,   244,    48,    51,    52,    55,    63,    81,   211,    26,
      70,   203,   110,   248,   246,    66,   245,   110,   246,   110,
     110,   167,   160,   108,   116,    23,   150,   151,   152,   156,
     108,   108,   114,   175,   111,   141,   139,   140,   103,   159,
      83,   136,   141,    23,   225,    23,   226,    23,   201,    23,
     194,    67,   194,    25,    31,    33,    69,    71,   183,   193,
     110,    23,   212,   213,   110,   246,    41,    42,    82,   208,
     209,    23,   251,    29,   192,    35,    54,   196,   110,    23,
     168,    23,   166,   168,   109,   232,   111,   103,   157,   144,
     145,   233,   141,   115,   109,   110,   247,   108,   108,   111,
     111,   111,   111,   111,    23,   215,    23,   214,   111,    77,
      78,    79,    80,   197,    23,   210,   111,   111,   111,   108,
     116,   112,   113,   153,   108,    23,    99,   146,   108,    62,
      73,   176,   179,   180,   181,   218,   219,   222,   227,    23,
     250,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,   137,   141,    57,    26,    40,    60,    72,
     185,   193,   244,   111,   111,   111,   232,    23,   154,    23,
     155,   145,   141,    34,    46,   205,   207,   108,   116,   111,
      32,   186,   190,   193,   108,   116,   108,   108,   110,   110,
      66,   206,   179,   232,   145,   136,   220,   225,   223,   226,
     110,   116,   108,   108,   111,   105,   111,   105,   210,   145,
     137,   225,   226,   105,   210,   111
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
#line 260 "program_parse.y"
    {
	   if (state->prog->Target != GL_VERTEX_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid fragment program header");

	   }
	   state->mode = ARB_vertex;
	;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 268 "program_parse.y"
    {
	   if (state->prog->Target != GL_FRAGMENT_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex program header");
	   }
	   state->mode = ARB_fragment;

	   state->option.TexRect =
	      (state->ctx->Extensions.NV_texture_rectangle != GL_FALSE);
	;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 284 "program_parse.y"
    {
	   int valid = 0;

	   if (state->mode == ARB_vertex) {
	      valid = _mesa_ARBvp_parse_option(state, (yyvsp[(2) - (3)].string));
	   } else if (state->mode == ARB_fragment) {
	      valid = _mesa_ARBfp_parse_option(state, (yyvsp[(2) - (3)].string));
	   }


	   if (!valid) {
	      const char *const err_str = (state->mode == ARB_vertex)
		 ? "invalid ARB vertex program option"
		 : "invalid ARB fragment program option";

	      yyerror(& (yylsp[(2) - (3)]), state, err_str);
	      YYERROR;
	   }
	;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 310 "program_parse.y"
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

  case 12:

/* Line 1455 of yacc.c  */
#line 328 "program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumAluInstructions++;
	;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 333 "program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumTexInstructions++;
	;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 354 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_ARL, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 360 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 367 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 374 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 382 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 390 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (8)].temp_inst).Opcode, & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), & (yyvsp[(6) - (8)].src_reg), & (yyvsp[(8) - (8)].src_reg));
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (8)].temp_inst).SaturateMode;
	;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 397 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (8)].temp_inst).Opcode, & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), NULL, NULL);
	   if ((yyval.inst) != NULL) {
	      const GLbitfield tex_mask = (1U << (yyvsp[(6) - (8)].integer));
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (8)].temp_inst).SaturateMode;
	      (yyval.inst)->Base.TexSrcUnit = (yyvsp[(6) - (8)].integer);

	      if ((yyvsp[(8) - (8)].integer) < 0) {
		 shadow_tex = tex_mask;

		 (yyval.inst)->Base.TexSrcTarget = -(yyvsp[(8) - (8)].integer);
		 (yyval.inst)->Base.TexShadow = 1;
	      } else {
		 (yyval.inst)->Base.TexSrcTarget = (yyvsp[(8) - (8)].integer);
	      }

	      target_mask = (1U << (yyval.inst)->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] != 0)
		  && ((state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& (yylsp[(8) - (8)]), state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[(yyvsp[(6) - (8)].integer)] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 442 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_KIL, NULL, & (yyvsp[(2) - (2)].src_reg), NULL, NULL);
	   state->fragment.UsesKill = 1;
	;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 449 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (12)].temp_inst).Opcode, & (yyvsp[(2) - (12)].dst_reg), & (yyvsp[(4) - (12)].src_reg), & (yyvsp[(6) - (12)].src_reg), & (yyvsp[(8) - (12)].src_reg));
	   if ((yyval.inst) != NULL) {
	      const GLbitfield tex_mask = (1U << (yyvsp[(10) - (12)].integer));
	      GLbitfield shadow_tex = 0;
	      GLbitfield target_mask = 0;


	      (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (12)].temp_inst).SaturateMode;
	      (yyval.inst)->Base.TexSrcUnit = (yyvsp[(10) - (12)].integer);

	      if ((yyvsp[(12) - (12)].integer) < 0) {
		 shadow_tex = tex_mask;

		 (yyval.inst)->Base.TexSrcTarget = -(yyvsp[(12) - (12)].integer);
		 (yyval.inst)->Base.TexShadow = 1;
	      } else {
		 (yyval.inst)->Base.TexSrcTarget = (yyvsp[(12) - (12)].integer);
	      }

	      target_mask = (1U << (yyval.inst)->Base.TexSrcTarget);

	      /* If this texture unit was previously accessed and that access
	       * had a different texture target, generate an error.
	       *
	       * If this texture unit was previously accessed and that access
	       * had a different shadow mode, generate an error.
	       */
	      if ((state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] != 0)
		  && ((state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] != target_mask)
		      || ((state->prog->ShadowSamplers & tex_mask)
			  != shadow_tex))) {
		 yyerror(& (yylsp[(12) - (12)]), state,
			 "multiple targets used on one texture image unit");
		 YYERROR;
	      }


	      state->prog->TexturesUsed[(yyvsp[(10) - (12)].integer)] |= target_mask;
	      state->prog->ShadowSamplers |= shadow_tex;
	   }
	;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 494 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 499 "program_parse.y"
    { (yyval.integer) = TEXTURE_1D_INDEX; ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 500 "program_parse.y"
    { (yyval.integer) = TEXTURE_2D_INDEX; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 501 "program_parse.y"
    { (yyval.integer) = TEXTURE_3D_INDEX; ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 502 "program_parse.y"
    { (yyval.integer) = TEXTURE_CUBE_INDEX; ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 503 "program_parse.y"
    { (yyval.integer) = TEXTURE_RECT_INDEX; ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 504 "program_parse.y"
    { (yyval.integer) = -TEXTURE_1D_INDEX; ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 505 "program_parse.y"
    { (yyval.integer) = -TEXTURE_2D_INDEX; ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 506 "program_parse.y"
    { (yyval.integer) = -TEXTURE_RECT_INDEX; ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 507 "program_parse.y"
    { (yyval.integer) = TEXTURE_1D_ARRAY_INDEX; ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 508 "program_parse.y"
    { (yyval.integer) = TEXTURE_2D_ARRAY_INDEX; ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 509 "program_parse.y"
    { (yyval.integer) = -TEXTURE_1D_ARRAY_INDEX; ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 510 "program_parse.y"
    { (yyval.integer) = -TEXTURE_2D_ARRAY_INDEX; ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 514 "program_parse.y"
    {
	   /* FIXME: Is this correct?  Should the extenedSwizzle be applied
	    * FIXME: to the existing swizzle?
	    */
	   (yyvsp[(4) - (6)].src_reg).Base.Swizzle = (yyvsp[(6) - (6)].swiz_mask).swizzle;
	   (yyvsp[(4) - (6)].src_reg).Base.Negate = (yyvsp[(6) - (6)].swiz_mask).mask;

	   (yyval.inst) = asm_instruction_ctor(OPCODE_SWZ, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 527 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (2)].src_reg);

	   if ((yyvsp[(1) - (2)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 535 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(3) - (4)].src_reg);

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(2) - (4)]), state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ((yyvsp[(1) - (4)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Abs = 1;
	;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 552 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(1) - (2)].src_reg);

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(2) - (2)].swiz_mask).swizzle);
	;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 559 "program_parse.y"
    {
	   struct asm_symbol temp_sym;

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(1) - (1)]), state, "expected scalar suffix");
	      YYERROR;
	   }

	   memset(& temp_sym, 0, sizeof(temp_sym));
	   temp_sym.param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & temp_sym, & (yyvsp[(1) - (1)].vector));

	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = PROGRAM_CONSTANT;
	   (yyval.src_reg).Base.Index = temp_sym.param_binding_begin;
	;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 578 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 589 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(3) - (5)].src_reg);

	   if (!state->option.NV_fragment) {
	      yyerror(& (yylsp[(2) - (5)]), state, "unexpected character '|'");
	      YYERROR;
	   }

	   if ((yyvsp[(1) - (5)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Abs = 1;
	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(4) - (5)].swiz_mask).swizzle);
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 609 "program_parse.y"
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

  case 54:

/* Line 1455 of yacc.c  */
#line 631 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_ADDRESS;
	   (yyval.dst_reg).Index = 0;
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (2)].swiz_mask).mask;
	;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 640 "program_parse.y"
    {
	   const unsigned xyzw_valid =
	      ((yyvsp[(1) - (7)].ext_swizzle).xyzw_valid << 0)
	      | ((yyvsp[(3) - (7)].ext_swizzle).xyzw_valid << 1)
	      | ((yyvsp[(5) - (7)].ext_swizzle).xyzw_valid << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).xyzw_valid << 3);
	   const unsigned rgba_valid =
	      ((yyvsp[(1) - (7)].ext_swizzle).rgba_valid << 0)
	      | ((yyvsp[(3) - (7)].ext_swizzle).rgba_valid << 1)
	      | ((yyvsp[(5) - (7)].ext_swizzle).rgba_valid << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).rgba_valid << 3);

	   /* All of the swizzle components have to be valid in either RGBA
	    * or XYZW.  Note that 0 and 1 are valid in both, so both masks
	    * can have some bits set.
	    *
	    * We somewhat deviate from the spec here.  It would be really hard
	    * to figure out which component is the error, and there probably
	    * isn't a lot of benefit.
	    */
	   if ((rgba_valid != 0x0f) && (xyzw_valid != 0x0f)) {
	      yyerror(& (yylsp[(1) - (7)]), state, "cannot combine RGBA and XYZW swizzle "
		      "components");
	      YYERROR;
	   }

	   (yyval.swiz_mask).swizzle = MAKE_SWIZZLE4((yyvsp[(1) - (7)].ext_swizzle).swz, (yyvsp[(3) - (7)].ext_swizzle).swz, (yyvsp[(5) - (7)].ext_swizzle).swz, (yyvsp[(7) - (7)].ext_swizzle).swz);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (7)].ext_swizzle).negate) | ((yyvsp[(3) - (7)].ext_swizzle).negate << 1) | ((yyvsp[(5) - (7)].ext_swizzle).negate << 2)
	      | ((yyvsp[(7) - (7)].ext_swizzle).negate << 3);
	;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 673 "program_parse.y"
    {
	   (yyval.ext_swizzle) = (yyvsp[(2) - (2)].ext_swizzle);
	   (yyval.ext_swizzle).negate = ((yyvsp[(1) - (2)].negate)) ? 1 : 0;
	;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 680 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) != 0) && ((yyvsp[(1) - (1)].integer) != 1)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   (yyval.ext_swizzle).swz = ((yyvsp[(1) - (1)].integer) == 0) ? SWIZZLE_ZERO : SWIZZLE_ONE;

	   /* 0 and 1 are valid for both RGBA swizzle names and XYZW
	    * swizzle names.
	    */
	   (yyval.ext_swizzle).xyzw_valid = 1;
	   (yyval.ext_swizzle).rgba_valid = 1;
	;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 695 "program_parse.y"
    {
	   if (strlen((yyvsp[(1) - (1)].string)) > 1) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   switch ((yyvsp[(1) - (1)].string)[0]) {
	   case 'x':
	      (yyval.ext_swizzle).swz = SWIZZLE_X;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'y':
	      (yyval.ext_swizzle).swz = SWIZZLE_Y;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'z':
	      (yyval.ext_swizzle).swz = SWIZZLE_Z;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;
	   case 'w':
	      (yyval.ext_swizzle).swz = SWIZZLE_W;
	      (yyval.ext_swizzle).xyzw_valid = 1;
	      break;

	   case 'r':
	      (yyval.ext_swizzle).swz = SWIZZLE_X;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'g':
	      (yyval.ext_swizzle).swz = SWIZZLE_Y;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'b':
	      (yyval.ext_swizzle).swz = SWIZZLE_Z;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;
	   case 'a':
	      (yyval.ext_swizzle).swz = SWIZZLE_W;
	      (yyval.ext_swizzle).rgba_valid = 1;
	      break;

	   default:
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	      break;
	   }
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 745 "program_parse.y"
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

  case 60:

/* Line 1455 of yacc.c  */
#line 787 "program_parse.y"
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

  case 61:

/* Line 1455 of yacc.c  */
#line 798 "program_parse.y"
    {
	   if (! (yyvsp[(3) - (4)].src_reg).Base.RelAddr
	       && ((unsigned) (yyvsp[(3) - (4)].src_reg).Base.Index >= (yyvsp[(1) - (4)].sym)->param_binding_length)) {
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

  case 62:

/* Line 1455 of yacc.c  */
#line 819 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = ((yyvsp[(1) - (1)].temp_sym).name != NULL) 
	      ? (yyvsp[(1) - (1)].temp_sym).param_binding_type
	      : PROGRAM_CONSTANT;
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].temp_sym).param_binding_begin;
	;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 829 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_OUTPUT;
	   (yyval.dst_reg).Index = (yyvsp[(1) - (1)].result);
	;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 835 "program_parse.y"
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
	   switch (s->type) {
	   case at_temp:
	      (yyval.dst_reg).File = PROGRAM_TEMPORARY;
	      (yyval.dst_reg).Index = s->temp_binding;
	      break;
	   case at_output:
	      (yyval.dst_reg).File = PROGRAM_OUTPUT;
	      (yyval.dst_reg).Index = s->output_binding;
	      break;
	   default:
	      (yyval.dst_reg).File = s->param_binding_type;
	      (yyval.dst_reg).Index = s->param_binding_begin;
	      break;
	   }
	;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 866 "program_parse.y"
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

  case 68:

/* Line 1455 of yacc.c  */
#line 885 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 892 "program_parse.y"
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

  case 70:

/* Line 1455 of yacc.c  */
#line 903 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 904 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 905 "program_parse.y"
    { (yyval.integer) = -(yyvsp[(2) - (2)].integer); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 909 "program_parse.y"
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

  case 74:

/* Line 1455 of yacc.c  */
#line 921 "program_parse.y"
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

  case 75:

/* Line 1455 of yacc.c  */
#line 933 "program_parse.y"
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

  case 76:

/* Line 1455 of yacc.c  */
#line 951 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid address component selector");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 962 "program_parse.y"
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

  case 82:

/* Line 1455 of yacc.c  */
#line 978 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 982 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 994 "program_parse.y"
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

  case 95:

/* Line 1455 of yacc.c  */
#line 1012 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 1016 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 1022 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_POS;
	;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 1026 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_WEIGHT;
	;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 1030 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_NORMAL;
	;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 1034 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_secondary_color) {
	      yyerror(& (yylsp[(2) - (2)]), state, "GL_EXT_secondary_color not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_COLOR0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 1043 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_fog_coord) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_EXT_fog_coord not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_FOG;
	;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 1052 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 1056 "program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 1061 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_GENERIC0 + (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 1067 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxAttribs) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex attribute reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 1081 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_WPOS;
	;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 1085 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_COL0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 1089 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_FOGC;
	;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 1093 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1101 "program_parse.y"
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

  case 116:

/* Line 1455 of yacc.c  */
#line 1117 "program_parse.y"
    {
	   if (((yyvsp[(4) - (6)].integer) != 0) && ((unsigned) (yyvsp[(4) - (6)].integer) != (yyvsp[(6) - (6)].temp_sym).param_binding_length)) {
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

  case 117:

/* Line 1455 of yacc.c  */
#line 1139 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1143 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 1) || ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxParameters)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid parameter array size");
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1154 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(2) - (2)].temp_sym);
	;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1160 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(3) - (4)].temp_sym);
	;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1167 "program_parse.y"
    {
	   (yyvsp[(1) - (3)].temp_sym).param_binding_length += (yyvsp[(3) - (3)].temp_sym).param_binding_length;
	   (yyval.temp_sym) = (yyvsp[(1) - (3)].temp_sym);
	;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1174 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1180 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1186 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1194 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1200 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1206 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1214 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1220 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1226 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1233 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(1) - (1)].state), sizeof((yyval.state))); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1234 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1237 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1238 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1239 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1240 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1241 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1242 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1243 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1244 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1245 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1246 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1247 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1251 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_MATERIAL;
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1260 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1264 "program_parse.y"
    {
	   (yyval.integer) = STATE_EMISSION;
	;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1268 "program_parse.y"
    {
	   (yyval.integer) = STATE_SHININESS;
	;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 1274 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHT;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (5)].integer);
	;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1283 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1287 "program_parse.y"
    {
	   (yyval.integer) = STATE_POSITION;
	;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1291 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_point_parameters) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_ARB_point_parameters not supported");
	      YYERROR;
	   }

	   (yyval.integer) = STATE_ATTENUATION;
	;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1300 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1304 "program_parse.y"
    {
	   (yyval.integer) = STATE_HALF_VECTOR;
	;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1310 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPOT_DIRECTION;
	;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1316 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (2)].state)[1];
	;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1323 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_AMBIENT;
	;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1328 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_SCENECOLOR;
	   (yyval.state)[1] = (yyvsp[(1) - (2)].integer);
	;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1336 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTPROD;
	   (yyval.state)[1] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(6) - (6)].integer);
	;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1348 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(3) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1356 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXENV_COLOR;
	;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1362 "program_parse.y"
    {
	   (yyval.integer) = STATE_AMBIENT;
	;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1366 "program_parse.y"
    {
	   (yyval.integer) = STATE_DIFFUSE;
	;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1370 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPECULAR;
	;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1376 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxLights) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid light selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1387 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_TEXGEN;
	   (yyval.state)[1] = (yyvsp[(2) - (4)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (4)].integer) + (yyvsp[(4) - (4)].integer);
	;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1396 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S;
	;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1400 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_OBJECT_S;
	;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1405 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1409 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1413 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1417 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1423 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 1430 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_COLOR;
	;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1434 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_PARAMS;
	;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1440 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_CLIPPLANE;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1448 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxClipPlanes) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid clip plane selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1459 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1466 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_SIZE;
	;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1470 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_ATTENUATION;
	;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1476 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (5)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (5)].state)[1];
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[4] = (yyvsp[(1) - (5)].state)[2];
	;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1486 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (2)].state)[1];
	   (yyval.state)[2] = (yyvsp[(2) - (2)].state)[2];
	   (yyval.state)[3] = (yyvsp[(2) - (2)].state)[3];
	   (yyval.state)[4] = (yyvsp[(1) - (2)].state)[2];
	;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1496 "program_parse.y"
    {
	   (yyval.state)[2] = 0;
	   (yyval.state)[3] = 3;
	;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1501 "program_parse.y"
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

  case 186:

/* Line 1455 of yacc.c  */
#line 1519 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (3)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (3)].state)[1];
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1527 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1531 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1537 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVERSE;
	;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1541 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_TRANSPOSE;
	;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 1545 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVTRANS;
	;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1551 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) > 3) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid matrix row reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 1562 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MODELVIEW_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 1567 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROJECTION_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 1572 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MVP_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 1577 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_TEXTURE_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1582 "program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1587 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROGRAM_MATRIX;
	   (yyval.state)[1] = (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1594 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1598 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1603 "program_parse.y"
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

  case 202:

/* Line 1455 of yacc.c  */
#line 1616 "program_parse.y"
    {
	   /* Since GL_ARB_matrix_palette isn't supported, just let any value
	    * through here.  The error will be generated later.
	    */
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1624 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxProgramMatrices) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program matrix selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1635 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_DEPTH_RANGE;
	;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 1647 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 1657 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1662 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1669 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1679 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1688 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1693 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1700 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1710 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxEnvParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid environment parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1720 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxLocalParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid local parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1735 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[1] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[2] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[3] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 1745 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[1] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[2] = (yyvsp[(1) - (1)].real);
	   (yyval.vector).data[3] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1753 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[1] = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[2] = (float) (yyvsp[(1) - (1)].integer);
	   (yyval.vector).data[3] = (float) (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 1763 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (3)].real);
	   (yyval.vector).data[1] = 0.0f;
	   (yyval.vector).data[2] = 0.0f;
	   (yyval.vector).data[3] = 1.0f;
	;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1771 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (5)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (5)].real);
	   (yyval.vector).data[2] = 0.0f;
	   (yyval.vector).data[3] = 1.0f;
	;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1780 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (7)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (7)].real);
	   (yyval.vector).data[2] = (yyvsp[(6) - (7)].real);
	   (yyval.vector).data[3] = 1.0f;
	;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1789 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (9)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (9)].real);
	   (yyval.vector).data[2] = (yyvsp[(6) - (9)].real);
	   (yyval.vector).data[3] = (yyvsp[(8) - (9)].real);
	;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 1799 "program_parse.y"
    {
	   (yyval.real) = ((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].real) : (yyvsp[(2) - (2)].real);
	;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1803 "program_parse.y"
    {
	   (yyval.real) = (float)(((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].integer) : (yyvsp[(2) - (2)].integer));
	;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1808 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1809 "program_parse.y"
    { (yyval.negate) = TRUE;  ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1810 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1813 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1816 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1820 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(3) - (3)].string), (yyvsp[(0) - (3)].integer), & (yylsp[(3) - (3)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1826 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(1) - (1)].string), (yyvsp[(0) - (1)].integer), & (yylsp[(1) - (1)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1834 "program_parse.y"
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

  case 242:

/* Line 1455 of yacc.c  */
#line 1847 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_HPOS;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1856 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_FOGC;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1865 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (2)].result);
	;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1869 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_PSIZ;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1878 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_TEX0 + (yyvsp[(3) - (3)].integer);
	   } else {
	      yyerror(& (yylsp[(2) - (3)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1887 "program_parse.y"
    {
	   if (state->mode == ARB_fragment) {
	      (yyval.result) = FRAG_RESULT_DEPTH;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1898 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (3)].integer) + (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1904 "program_parse.y"
    {
	   (yyval.integer) = (state->mode == ARB_vertex)
	      ? VERT_RESULT_COL0
	      : FRAG_RESULT_COLOR;
	;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1910 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_COL0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1919 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_BFC0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1930 "program_parse.y"
    {
	   (yyval.integer) = 0; 
	;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1934 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1943 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 1;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1953 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1954 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1955 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1958 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 1959 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 1960 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 1963 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 1964 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 1967 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 1968 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 1971 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 1972 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 1976 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureCoordUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture coordinate unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 1987 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureImageUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture image unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 1998 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 2009 "program_parse.y"
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
#line 4718 "program_parse.tab.c"
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
#line 2029 "program_parse.y"


struct asm_instruction *
asm_instruction_ctor(gl_inst_opcode op,
		     const struct prog_dst_register *dst,
		     const struct asm_src_register *src0,
		     const struct asm_src_register *src1,
		     const struct asm_src_register *src2)
{
   struct asm_instruction *inst = calloc(1, sizeof(struct asm_instruction));

   if (inst) {
      _mesa_init_instructions(& inst->Base, 1);
      inst->Base.Opcode = op;

      /* In the core ARB extensions only the KIL instruction doesn't have a
       * destination register.
       */
      if (dst == NULL) {
	 init_dst_reg(& inst->Base.DstReg);
      } else {
	 inst->Base.DstReg = *dst;
      }

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
      s->next = state->sym;
      state->sym = s;
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
   struct asm_instruction *inst;
   unsigned i;
   GLubyte *strz;
   GLboolean result = GL_FALSE;
   void *temp;
   struct asm_symbol *sym;

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

   state->limits = (target == GL_VERTEX_PROGRAM_ARB)
      ? & ctx->Const.VertexProgram
      : & ctx->Const.FragmentProgram;

   state->MaxTextureImageUnits = ctx->Const.MaxTextureImageUnits;
   state->MaxTextureCoordUnits = ctx->Const.MaxTextureCoordUnits;
   state->MaxTextureUnits = ctx->Const.MaxTextureUnits;
   state->MaxClipPlanes = ctx->Const.MaxClipPlanes;
   state->MaxLights = ctx->Const.MaxLights;
   state->MaxProgramMatrices = ctx->Const.MaxProgramMatrices;

   state->state_param_enum = (target == GL_VERTEX_PROGRAM_ARB)
      ? STATE_VERTEX_PROGRAM : STATE_FRAGMENT_PROGRAM;

   _mesa_set_program_error(ctx, -1, NULL);

   _mesa_program_lexer_ctor(& state->scanner, state, (const char *) str, len);
   yyparse(state);
   _mesa_program_lexer_dtor(state->scanner);


   if (ctx->Program.ErrorPos != -1) {
      goto error;
   }

   if (! _mesa_layout_parameters(state)) {
      struct YYLTYPE loc;

      loc.first_line = 0;
      loc.first_column = 0;
      loc.position = len;

      yyerror(& loc, state, "invalid PARAM usage");
      goto error;
   }


   
   /* Add one instruction to store the "END" instruction.
    */
   state->prog->Instructions =
      _mesa_alloc_instructions(state->prog->NumInstructions + 1);
   inst = state->inst_head;
   for (i = 0; i < state->prog->NumInstructions; i++) {
      struct asm_instruction *const temp = inst->next;

      state->prog->Instructions[i] = inst->Base;
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
   state->prog->NumAttributes = _mesa_bitcount(state->prog->InputsRead);

   /*
    * Initialize native counts to logical counts.  The device driver may
    * change them if program is translated into a hardware program.
    */
   state->prog->NumNativeInstructions = state->prog->NumInstructions;
   state->prog->NumNativeTemporaries = state->prog->NumTemporaries;
   state->prog->NumNativeParameters = state->prog->NumParameters;
   state->prog->NumNativeAttributes = state->prog->NumAttributes;
   state->prog->NumNativeAddressRegs = state->prog->NumAddressRegs;

   result = GL_TRUE;

error:
   for (inst = state->inst_head; inst != NULL; inst = temp) {
      temp = inst->next;
      _mesa_free(inst);
   }

   state->inst_head = NULL;
   state->inst_tail = NULL;

   for (sym = state->sym; sym != NULL; sym = temp) {
      temp = sym->next;

      _mesa_free((void *) sym->name);
      _mesa_free(sym);
   }
   state->sym = NULL;

   _mesa_symbol_table_dtor(state->st);
   state->st = NULL;

   return result;
}

