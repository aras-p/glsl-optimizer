
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
     TEX_SHADOW1D = 343,
     TEX_SHADOW2D = 344,
     TEX_SHADOWRECT = 345,
     VERTEX = 346,
     VTXATTRIB = 347,
     WEIGHT = 348,
     IDENTIFIER = 349,
     MASK4 = 350,
     MASK3 = 351,
     MASK2 = 352,
     MASK1 = 353,
     SWIZZLE = 354,
     DOT_DOT = 355,
     DOT = 356
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



/* Line 214 of yacc.c  */
#line 332 "program_parse.tab.c"
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
#line 241 "program_parse.y"

extern int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc_param,
    void *yyscanner);


/* Line 264 of yacc.c  */
#line 363 "program_parse.tab.c"

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
#define YYLAST   334

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  111
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  133
/* YYNRULES -- Number of rules.  */
#define YYNRULES  258
/* YYNRULES -- Number of states.  */
#define YYNSTATES  427

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   356

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   106,   103,   107,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   102,
       2,   108,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   104,     2,   105,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   109,     2,   110,     2,     2,     2,     2,
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
      95,    96,    97,    98,    99,   100,   101
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     8,    10,    12,    15,    16,    20,    23,
      24,    27,    30,    32,    34,    36,    38,    40,    42,    44,
      46,    48,    50,    52,    57,    62,    67,    74,    81,    90,
      99,   102,   105,   107,   109,   111,   113,   115,   117,   119,
     121,   128,   132,   136,   139,   142,   150,   153,   155,   157,
     159,   161,   166,   168,   170,   172,   174,   176,   178,   180,
     184,   185,   188,   191,   193,   195,   197,   199,   201,   203,
     205,   207,   209,   210,   212,   214,   216,   218,   219,   221,
     223,   225,   227,   229,   231,   236,   239,   242,   244,   247,
     249,   252,   254,   257,   262,   267,   269,   270,   274,   276,
     278,   281,   283,   286,   288,   290,   294,   301,   302,   304,
     307,   312,   314,   318,   320,   322,   324,   326,   328,   330,
     332,   334,   336,   338,   341,   344,   347,   350,   353,   356,
     359,   362,   365,   368,   371,   375,   377,   379,   381,   387,
     389,   391,   393,   396,   398,   400,   403,   405,   408,   415,
     417,   421,   423,   425,   427,   429,   431,   436,   438,   440,
     442,   444,   446,   448,   451,   453,   455,   461,   463,   466,
     468,   470,   476,   479,   480,   487,   491,   492,   494,   496,
     498,   500,   502,   505,   507,   509,   512,   517,   522,   523,
     525,   527,   529,   531,   533,   535,   537,   539,   545,   547,
     551,   557,   563,   565,   569,   575,   577,   579,   581,   583,
     585,   587,   589,   591,   593,   597,   603,   611,   621,   624,
     627,   629,   631,   632,   633,   637,   638,   642,   646,   648,
     653,   656,   659,   662,   665,   669,   672,   676,   677,   679,
     681,   682,   684,   686,   687,   689,   691,   692,   694,   696,
     697,   701,   702,   706,   707,   711,   713,   715,   717
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     112,     0,    -1,   113,   114,   116,    12,    -1,     3,    -1,
       4,    -1,   114,   115,    -1,    -1,     8,    94,   102,    -1,
     116,   117,    -1,    -1,   118,   102,    -1,   154,   102,    -1,
     119,    -1,   120,    -1,   121,    -1,   122,    -1,   123,    -1,
     124,    -1,   125,    -1,   126,    -1,   131,    -1,   127,    -1,
     128,    -1,    19,   135,   103,   132,    -1,    18,   134,   103,
     133,    -1,    16,   134,   103,   132,    -1,    14,   134,   103,
     132,   103,   132,    -1,    13,   134,   103,   133,   103,   133,
      -1,    17,   134,   103,   133,   103,   133,   103,   133,    -1,
      15,   134,   103,   133,   103,   129,   103,   130,    -1,    20,
     133,    -1,    82,   238,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,    90,
      -1,    21,   134,   103,   139,   103,   136,    -1,   224,   139,
     151,    -1,   224,   139,   152,    -1,   140,   153,    -1,   148,
     150,    -1,   137,   103,   137,   103,   137,   103,   137,    -1,
     224,   138,    -1,    22,    -1,    94,    -1,    94,    -1,   156,
      -1,   141,   104,   142,   105,    -1,   170,    -1,   231,    -1,
      94,    -1,    94,    -1,   143,    -1,   144,    -1,    22,    -1,
     148,   149,   145,    -1,    -1,   106,   146,    -1,   107,   147,
      -1,    22,    -1,    22,    -1,    94,    -1,    98,    -1,    98,
      -1,    98,    -1,    98,    -1,    95,    -1,    99,    -1,    -1,
      95,    -1,    96,    -1,    97,    -1,    98,    -1,    -1,   155,
      -1,   162,    -1,   225,    -1,   227,    -1,   230,    -1,   243,
      -1,     7,    94,   108,   156,    -1,    91,   157,    -1,    37,
     161,    -1,    59,    -1,    93,   159,    -1,    52,    -1,    28,
     236,    -1,    36,    -1,    73,   237,    -1,    49,   104,   160,
     105,    -1,    92,   104,   158,   105,    -1,    22,    -1,    -1,
     104,   160,   105,    -1,    22,    -1,    59,    -1,    28,   236,
      -1,    36,    -1,    73,   237,    -1,   163,    -1,   164,    -1,
      10,    94,   166,    -1,    10,    94,   104,   165,   105,   167,
      -1,    -1,    22,    -1,   108,   169,    -1,   108,   109,   168,
     110,    -1,   171,    -1,   168,   103,   171,    -1,   173,    -1,
     208,    -1,   218,    -1,   173,    -1,   208,    -1,   219,    -1,
     172,    -1,   209,    -1,   218,    -1,   173,    -1,    72,   197,
      -1,    72,   174,    -1,    72,   176,    -1,    72,   179,    -1,
      72,   181,    -1,    72,   187,    -1,    72,   183,    -1,    72,
     190,    -1,    72,   192,    -1,    72,   194,    -1,    72,   196,
      -1,    46,   235,   175,    -1,   185,    -1,    32,    -1,    68,
      -1,    42,   104,   186,   105,   177,    -1,   185,    -1,    59,
      -1,    25,    -1,    71,   178,    -1,    39,    -1,    31,    -1,
      43,   180,    -1,    24,    -1,   235,    66,    -1,    44,   104,
     186,   105,   235,   182,    -1,   185,    -1,    74,   239,   184,
      -1,    28,    -1,    24,    -1,    30,    -1,    70,    -1,    22,
      -1,    75,   237,   188,   189,    -1,    34,    -1,    53,    -1,
      78,    -1,    79,    -1,    77,    -1,    76,    -1,    35,   191,
      -1,    28,    -1,    55,    -1,    27,   104,   193,   105,    56,
      -1,    22,    -1,    57,   195,    -1,    69,    -1,    25,    -1,
     199,    65,   104,   202,   105,    -1,   199,   198,    -1,    -1,
      65,   104,   202,   100,   202,   105,    -1,    48,   203,   200,
      -1,    -1,   201,    -1,    40,    -1,    81,    -1,    41,    -1,
      22,    -1,    50,   204,    -1,    62,    -1,    51,    -1,    80,
     237,    -1,    54,   104,   206,   105,    -1,    47,   104,   207,
     105,    -1,    -1,   205,    -1,    22,    -1,    22,    -1,    22,
      -1,   212,    -1,   215,    -1,   210,    -1,   213,    -1,    61,
      33,   104,   211,   105,    -1,   216,    -1,   216,   100,   216,
      -1,    61,    33,   104,   216,   105,    -1,    61,    45,   104,
     214,   105,    -1,   217,    -1,   217,   100,   217,    -1,    61,
      45,   104,   217,   105,    -1,    22,    -1,    22,    -1,   220,
      -1,   222,    -1,   221,    -1,   222,    -1,   223,    -1,    23,
      -1,    22,    -1,   109,   223,   110,    -1,   109,   223,   103,
     223,   110,    -1,   109,   223,   103,   223,   103,   223,   110,
      -1,   109,   223,   103,   223,   103,   223,   103,   223,   110,
      -1,   224,    23,    -1,   224,    22,    -1,   106,    -1,   107,
      -1,    -1,    -1,    11,   226,   229,    -1,    -1,     5,   228,
     229,    -1,   229,   103,    94,    -1,    94,    -1,     9,    94,
     108,   231,    -1,    64,    59,    -1,    64,    36,    -1,    64,
     232,    -1,    64,    58,    -1,    64,    73,   237,    -1,    64,
      29,    -1,    28,   233,   234,    -1,    -1,    38,    -1,    26,
      -1,    -1,    60,    -1,    67,    -1,    -1,    38,    -1,    26,
      -1,    -1,    60,    -1,    67,    -1,    -1,   104,   240,   105,
      -1,    -1,   104,   241,   105,    -1,    -1,   104,   242,   105,
      -1,    22,    -1,    22,    -1,    22,    -1,     6,    94,   108,
      94,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   248,   248,   251,   259,   271,   272,   275,   297,   298,
     301,   316,   319,   324,   331,   332,   333,   334,   335,   336,
     337,   340,   341,   344,   350,   357,   364,   372,   379,   387,
     432,   439,   445,   446,   447,   448,   449,   450,   451,   452,
     455,   467,   480,   493,   515,   524,   533,   540,   549,   577,
     619,   630,   651,   661,   667,   691,   708,   708,   710,   717,
     729,   730,   731,   734,   746,   758,   776,   787,   799,   801,
     802,   803,   804,   807,   807,   807,   807,   808,   811,   812,
     813,   814,   815,   816,   819,   837,   841,   847,   851,   855,
     859,   868,   877,   881,   886,   892,   903,   903,   904,   906,
     910,   914,   918,   924,   924,   926,   942,   965,   968,   979,
     985,   991,   992,   999,  1005,  1011,  1019,  1025,  1031,  1039,
    1045,  1051,  1059,  1060,  1063,  1064,  1065,  1066,  1067,  1068,
    1069,  1070,  1071,  1072,  1075,  1084,  1088,  1092,  1098,  1107,
    1111,  1115,  1124,  1128,  1134,  1140,  1147,  1152,  1160,  1170,
    1172,  1180,  1186,  1190,  1194,  1200,  1211,  1220,  1224,  1229,
    1233,  1237,  1241,  1247,  1254,  1258,  1264,  1272,  1283,  1290,
    1294,  1300,  1310,  1321,  1325,  1343,  1352,  1355,  1361,  1365,
    1369,  1375,  1386,  1391,  1396,  1401,  1406,  1411,  1419,  1422,
    1427,  1440,  1448,  1461,  1461,  1463,  1463,  1465,  1475,  1480,
    1487,  1497,  1506,  1511,  1518,  1528,  1538,  1550,  1550,  1551,
    1551,  1553,  1560,  1565,  1572,  1580,  1588,  1597,  1608,  1612,
    1618,  1619,  1620,  1623,  1623,  1626,  1626,  1629,  1635,  1643,
    1656,  1665,  1674,  1678,  1687,  1696,  1707,  1714,  1719,  1728,
    1740,  1743,  1752,  1763,  1764,  1765,  1768,  1769,  1770,  1773,
    1774,  1777,  1778,  1781,  1782,  1785,  1796,  1807,  1818
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
  "TEX_2D", "TEX_3D", "TEX_CUBE", "TEX_RECT", "TEX_SHADOW1D",
  "TEX_SHADOW2D", "TEX_SHADOWRECT", "VERTEX", "VTXATTRIB", "WEIGHT",
  "IDENTIFIER", "MASK4", "MASK3", "MASK2", "MASK1", "SWIZZLE", "DOT_DOT",
  "DOT", "';'", "','", "'['", "']'", "'+'", "'-'", "'='", "'{'", "'}'",
  "$accept", "program", "language", "optionSequence", "option",
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
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,    59,    44,    91,    93,    43,    45,    61,   123,
     125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   111,   112,   113,   113,   114,   114,   115,   116,   116,
     117,   117,   118,   118,   119,   119,   119,   119,   119,   119,
     119,   120,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   130,   130,   130,   130,   130,   130,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   138,   139,
     139,   139,   139,   140,   140,   141,   142,   142,   143,   144,
     145,   145,   145,   146,   147,   148,   149,   150,   151,   152,
     152,   152,   152,   153,   153,   153,   153,   153,   154,   154,
     154,   154,   154,   154,   155,   156,   156,   157,   157,   157,
     157,   157,   157,   157,   157,   158,   159,   159,   160,   161,
     161,   161,   161,   162,   162,   163,   164,   165,   165,   166,
     167,   168,   168,   169,   169,   169,   170,   170,   170,   171,
     171,   171,   172,   172,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   174,   175,   175,   175,   176,   177,
     177,   177,   177,   177,   178,   179,   180,   180,   181,   182,
     183,   184,   185,   185,   185,   186,   187,   188,   188,   189,
     189,   189,   189,   190,   191,   191,   192,   193,   194,   195,
     195,   196,   197,   198,   198,   199,   200,   200,   201,   201,
     201,   202,   203,   203,   203,   203,   203,   203,   204,   204,
     205,   206,   207,   208,   208,   209,   209,   210,   211,   211,
     212,   213,   214,   214,   215,   216,   217,   218,   218,   219,
     219,   220,   221,   221,   222,   222,   222,   222,   223,   223,
     224,   224,   224,   226,   225,   228,   227,   229,   229,   230,
     231,   231,   231,   231,   231,   231,   232,   233,   233,   233,
     234,   234,   234,   235,   235,   235,   236,   236,   236,   237,
     237,   238,   238,   239,   239,   240,   241,   242,   243
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     1,     1,     2,     0,     3,     2,     0,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     4,     6,     6,     8,     8,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       6,     3,     3,     2,     2,     7,     2,     1,     1,     1,
       1,     4,     1,     1,     1,     1,     1,     1,     1,     3,
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     4,     2,     2,     1,     2,     1,
       2,     1,     2,     4,     4,     1,     0,     3,     1,     1,
       2,     1,     2,     1,     1,     3,     6,     0,     1,     2,
       4,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     3,     1,     1,     1,     5,     1,
       1,     1,     2,     1,     1,     2,     1,     2,     6,     1,
       3,     1,     1,     1,     1,     1,     4,     1,     1,     1,
       1,     1,     1,     2,     1,     1,     5,     1,     2,     1,
       1,     5,     2,     0,     6,     3,     0,     1,     1,     1,
       1,     1,     2,     1,     1,     2,     4,     4,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     1,     3,
       5,     5,     1,     3,     5,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     5,     7,     9,     2,     2,
       1,     1,     0,     0,     3,     0,     3,     3,     1,     4,
       2,     2,     2,     2,     3,     2,     3,     0,     1,     1,
       0,     1,     1,     0,     1,     1,     0,     1,     1,     0,
       3,     0,     3,     0,     3,     1,     1,     1,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     3,     4,     0,     6,     1,     9,     0,     5,     0,
       0,   225,     0,     0,     0,     0,   223,     2,     0,     0,
       0,     0,     0,     0,     0,   222,     0,     8,     0,    12,
      13,    14,    15,    16,    17,    18,    19,    21,    22,    20,
       0,    78,    79,   103,   104,    80,    81,    82,    83,     7,
       0,     0,     0,     0,     0,     0,     0,    54,     0,    77,
      53,     0,     0,     0,     0,     0,    65,     0,     0,   220,
     221,    30,     0,     0,    10,    11,   228,   226,     0,     0,
       0,   107,   222,   105,   224,   237,   235,   231,   233,   230,
     249,   232,   222,    73,    74,    75,    76,    43,   222,   222,
     222,   222,   222,   222,    67,    44,   213,   212,     0,     0,
       0,     0,    49,   222,    72,     0,    50,    52,   116,   117,
     193,   194,   118,   209,   210,     0,     0,   258,    84,   229,
     108,     0,   109,   113,   114,   115,   207,   208,   211,     0,
     239,   238,   240,     0,   234,     0,     0,     0,     0,    25,
       0,    24,    23,   246,   101,    99,   249,    86,     0,     0,
       0,     0,     0,   243,     0,   243,     0,     0,   253,   249,
     124,   125,   126,   127,   129,   128,   130,   131,   132,   133,
       0,   246,    91,     0,    89,    87,   249,     0,    96,    85,
       0,    70,    69,    71,    42,     0,     0,   227,     0,   219,
     218,   241,   242,   236,   255,     0,   222,   222,     0,     0,
     222,   247,   248,   100,   102,     0,     0,     0,   164,   165,
     163,     0,   146,   245,   244,   145,     0,     0,     0,     0,
     188,   184,     0,   183,   249,   176,   170,   169,   168,     0,
       0,     0,     0,    90,     0,    92,     0,     0,    88,   222,
     214,    58,     0,    56,    57,     0,   222,     0,   106,   250,
      27,    26,    68,    41,   251,     0,     0,   205,     0,   206,
       0,   167,     0,   155,     0,   147,     0,   152,   153,   136,
     137,   154,   134,   135,     0,   190,   182,   189,     0,   185,
     178,   180,   179,   175,   177,   257,     0,   151,   150,   157,
     158,     0,     0,    98,     0,    95,     0,     0,     0,    51,
      66,    60,    40,     0,     0,   222,     0,    31,     0,   222,
     200,   204,     0,     0,   243,   192,     0,   191,     0,   254,
     162,   161,   159,   160,   156,   181,     0,    93,    94,    97,
     222,   215,     0,     0,    59,   222,    47,    48,    46,     0,
       0,     0,   111,   119,   122,   120,   195,   196,   121,   256,
       0,    32,    33,    34,    35,    36,    37,    38,    39,    29,
      28,   166,   141,   143,   140,     0,   138,   139,     0,   187,
     186,   171,     0,    63,    61,    64,    62,     0,     0,     0,
     123,   173,   222,   110,   252,   144,   142,   148,   149,   222,
     216,   222,     0,     0,     0,   172,   112,     0,     0,     0,
     198,     0,   202,     0,   217,   222,   197,     0,   201,     0,
       0,    45,   199,   203,     0,     0,   174
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     6,     8,     9,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,   265,   369,
      39,   146,    71,    58,    67,   312,   313,   348,   114,    59,
     115,   252,   253,   254,   344,   384,   386,    68,   311,   105,
     263,   194,    97,    40,    41,   116,   189,   306,   248,   304,
     157,    42,    43,    44,   131,    83,   258,   351,   132,   117,
     352,   353,   118,   170,   282,   171,   376,   396,   172,   225,
     173,   397,   174,   298,   283,   274,   175,   301,   334,   176,
     220,   177,   272,   178,   238,   179,   390,   405,   180,   293,
     294,   336,   235,   286,   287,   328,   326,   119,   355,   356,
     409,   120,   357,   411,   121,   268,   270,   358,   122,   136,
     123,   124,   138,    72,    45,    55,    46,    50,    77,    47,
      60,    91,   142,   203,   226,   213,   144,   317,   240,   205,
     360,   296,    48
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -374
static const yytype_int16 yypact[] =
{
     154,  -374,  -374,    33,  -374,  -374,    35,   -31,  -374,   166,
      27,  -374,    55,    66,    72,    96,  -374,  -374,   -34,   -34,
     -34,   -34,   -34,   -34,   115,    87,   -34,  -374,   108,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
     109,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
     118,   105,   106,   107,    -5,   118,    92,  -374,   113,   102,
    -374,   114,   116,   117,   119,   120,  -374,   121,    97,  -374,
    -374,  -374,   -15,   122,  -374,  -374,  -374,   123,   124,   -17,
     157,   205,   -16,  -374,   123,    13,  -374,  -374,  -374,  -374,
     128,  -374,    87,  -374,  -374,  -374,  -374,  -374,    87,    87,
      87,    87,    87,    87,  -374,  -374,  -374,  -374,    19,    40,
      23,   -11,   132,    87,    93,   134,  -374,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,   -15,   135,  -374,  -374,  -374,
    -374,   136,  -374,  -374,  -374,  -374,  -374,  -374,  -374,   183,
    -374,  -374,    57,   206,  -374,   137,   139,   -15,   140,  -374,
     141,  -374,  -374,    80,  -374,  -374,   128,  -374,   142,   143,
     144,     6,   145,    85,   146,    26,    65,    -1,   147,   128,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
     172,    80,  -374,   148,  -374,  -374,   128,   149,   150,  -374,
      58,  -374,  -374,  -374,  -374,   -10,   152,  -374,   131,  -374,
    -374,  -374,  -374,  -374,  -374,   151,    87,    87,   159,   163,
      87,  -374,  -374,  -374,  -374,   237,   238,   239,  -374,  -374,
    -374,   240,  -374,  -374,  -374,  -374,   198,   240,    76,   161,
     244,  -374,   164,  -374,   128,    -4,  -374,  -374,  -374,   245,
     241,     1,   167,  -374,   248,  -374,   250,   248,  -374,    87,
    -374,  -374,   168,  -374,  -374,   176,    87,   169,  -374,  -374,
    -374,  -374,  -374,  -374,   171,   173,   174,  -374,   175,  -374,
     177,  -374,   178,  -374,   179,  -374,   180,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,   257,  -374,  -374,  -374,   259,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,   181,  -374,  -374,  -374,
    -374,   125,   265,  -374,   185,  -374,   186,   187,    59,  -374,
    -374,   101,  -374,   190,    -7,    -2,   266,  -374,    48,    87,
    -374,  -374,   242,    71,    26,  -374,   189,  -374,   191,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,   192,  -374,  -374,  -374,
      87,  -374,   273,   277,  -374,    87,  -374,  -374,  -374,    81,
      23,    60,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
     195,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,   270,  -374,  -374,     2,  -374,
    -374,  -374,    86,  -374,  -374,  -374,  -374,   199,   200,   201,
    -374,   243,    -2,  -374,  -374,  -374,  -374,  -374,  -374,    87,
    -374,    87,   237,   238,   202,  -374,  -374,   193,   204,   208,
     209,   210,   214,   265,  -374,    87,  -374,   237,  -374,   238,
      43,  -374,  -374,  -374,   265,   211,  -374
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
    -374,   -94,   -88,   133,  -374,  -374,  -326,  -374,   -98,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,   126,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,   246,  -374,  -374,  -374,    73,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
     -74,  -374,   -81,  -374,  -374,  -374,  -374,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,  -295,    95,  -374,  -374,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,  -374,  -374,  -374,   -27,  -374,
    -374,  -371,  -374,  -374,  -374,  -374,  -374,   247,  -374,  -374,
    -374,  -374,  -374,  -374,  -374,  -373,  -317,   249,  -374,  -374,
    -374,   -80,  -110,   -82,  -374,  -374,  -374,  -374,   269,  -374,
     252,  -374,  -374,  -374,  -160,   153,  -146,  -374,  -374,  -374,
    -374,  -374,  -374
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -56
static const yytype_int16 yytable[] =
{
     139,   133,   137,   190,   145,   228,   149,   106,   107,   152,
     214,   148,   251,   150,   151,   346,   147,   181,   147,   387,
     108,   147,   108,   241,   236,   182,   277,   196,   377,   410,
      56,   139,   278,     5,   218,   299,   290,   291,   183,   140,
     245,   184,   420,     7,   422,   109,   109,   153,   185,   208,
     160,   141,   223,   425,   300,   154,   110,   110,   161,   349,
      57,   219,   186,    10,   224,   162,   163,   164,   237,   165,
     350,   166,   281,   158,   111,   408,   111,   292,   155,   112,
     167,   187,   188,   398,    66,   159,   412,   347,   289,   421,
      69,    70,   156,   113,   113,   277,   372,   168,   169,    81,
     277,   278,   423,    82,    69,    70,   278,   113,   279,   222,
     373,   223,   229,   261,   388,   230,   231,   201,   260,   232,
      85,    86,   266,   224,   202,   147,   389,   233,    87,    49,
     374,   361,   362,   363,   364,   365,   366,   367,   368,   308,
     211,   281,   375,   424,   280,   234,   281,   212,   381,    51,
      88,    89,    61,    62,    63,    64,    65,     1,     2,    73,
      52,   249,   340,   392,   378,    90,    53,   139,   250,   341,
     393,    11,    12,    13,   314,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,   191,   399,
      54,   192,   193,    69,    70,   104,   400,    93,    94,    95,
      96,   330,   331,   332,   333,   199,   200,   342,   343,    66,
      74,    75,    76,    78,    79,    80,    92,    98,   127,    99,
     100,    56,   101,   102,   103,   125,   126,   130,   204,   197,
     382,   370,   143,   139,   354,   137,   -55,   242,   195,   257,
     206,   198,   207,   209,   210,   264,   215,   216,   217,   221,
     227,   239,   244,   246,   247,   256,   259,   262,   139,   267,
     269,   271,   273,   314,   275,   284,   285,   295,   288,   297,
     303,   302,   305,   309,   310,   316,   318,   319,   315,   325,
     320,   327,   321,   322,   323,   324,   329,   335,   359,   407,
     337,   338,   339,   345,   379,   383,   380,   381,   371,   385,
     394,   395,   401,   414,   402,   403,   413,   415,   404,   417,
     139,   354,   137,   416,   419,   418,   426,   139,   406,   314,
     307,   255,   276,   391,    84,   128,     0,     0,     0,   134,
       0,   135,   129,   314,   243
};

static const yytype_int16 yycheck[] =
{
      82,    82,    82,   113,    92,   165,   100,    22,    23,   103,
     156,    99,    22,   101,   102,    22,    98,    28,   100,   345,
      37,   103,    37,   169,    25,    36,    24,   125,   323,   402,
      64,   113,    30,     0,    28,    34,    40,    41,    49,    26,
     186,    52,   413,     8,   417,    61,    61,    28,    59,   147,
      27,    38,    26,   424,    53,    36,    72,    72,    35,    61,
      94,    55,    73,    94,    38,    42,    43,    44,    69,    46,
      72,    48,    70,    33,    91,   401,    91,    81,    59,    94,
      57,    92,    93,   378,    94,    45,   403,    94,   234,   415,
     106,   107,    73,   109,   109,    24,    25,    74,    75,   104,
      24,    30,   419,   108,   106,   107,    30,   109,    32,    24,
      39,    26,    47,   207,    33,    50,    51,    60,   206,    54,
      28,    29,   210,    38,    67,   207,    45,    62,    36,   102,
      59,    83,    84,    85,    86,    87,    88,    89,    90,   249,
      60,    70,    71,   100,    68,    80,    70,    67,   105,    94,
      58,    59,    19,    20,    21,    22,    23,     3,     4,    26,
      94,   103,   103,   103,   324,    73,    94,   249,   110,   110,
     110,     5,     6,     7,   256,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    95,   103,
      94,    98,    99,   106,   107,    98,   110,    95,    96,    97,
      98,    76,    77,    78,    79,    22,    23,   106,   107,    94,
     102,   102,    94,   108,   108,   108,   103,   103,    94,   103,
     103,    64,   103,   103,   103,   103,   103,    22,    22,    94,
     340,   319,   104,   315,   315,   315,   104,    65,   104,   108,
     103,   105,   103,   103,   103,    82,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   103,   105,    98,   340,    22,
      22,    22,    22,   345,    66,   104,    22,    22,   104,    28,
      22,   104,    22,   105,    98,   104,   103,   103,   109,    22,
     105,    22,   105,   105,   105,   105,   105,    22,    22,   399,
     105,   105,   105,   103,   105,    22,   105,   105,    56,    22,
     105,    31,   103,   110,   104,   104,   104,   103,    65,   100,
     392,   392,   392,   105,   100,   105,   105,   399,   392,   401,
     247,   195,   227,   350,    55,    79,    -1,    -1,    -1,    82,
      -1,    82,    80,   415,   181
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,   112,   113,     0,   114,     8,   115,   116,
      94,     5,     6,     7,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   131,
     154,   155,   162,   163,   164,   225,   227,   230,   243,   102,
     228,    94,    94,    94,    94,   226,    64,    94,   134,   140,
     231,   134,   134,   134,   134,   134,    94,   135,   148,   106,
     107,   133,   224,   134,   102,   102,    94,   229,   108,   108,
     108,   104,   108,   166,   229,    28,    29,    36,    58,    59,
      73,   232,   103,    95,    96,    97,    98,   153,   103,   103,
     103,   103,   103,   103,    98,   150,    22,    23,    37,    61,
      72,    91,    94,   109,   139,   141,   156,   170,   173,   208,
     212,   215,   219,   221,   222,   103,   103,    94,   156,   231,
      22,   165,   169,   173,   208,   218,   220,   222,   223,   224,
      26,    38,   233,   104,   237,   133,   132,   224,   133,   132,
     133,   133,   132,    28,    36,    59,    73,   161,    33,    45,
      27,    35,    42,    43,    44,    46,    48,    57,    74,    75,
     174,   176,   179,   181,   183,   187,   190,   192,   194,   196,
     199,    28,    36,    49,    52,    59,    73,    92,    93,   157,
     223,    95,    98,    99,   152,   104,   139,    94,   105,    22,
      23,    60,    67,   234,    22,   240,   103,   103,   139,   103,
     103,    60,    67,   236,   237,   104,   104,   104,    28,    55,
     191,   104,    24,    26,    38,   180,   235,   104,   235,    47,
      50,    51,    54,    62,    80,   203,    25,    69,   195,   104,
     239,   237,    65,   236,   104,   237,   104,   104,   159,   103,
     110,    22,   142,   143,   144,   148,   103,   108,   167,   105,
     133,   132,    98,   151,    82,   129,   133,    22,   216,    22,
     217,    22,   193,    22,   186,    66,   186,    24,    30,    32,
      68,    70,   175,   185,   104,    22,   204,   205,   104,   237,
      40,    41,    81,   200,   201,    22,   242,    28,   184,    34,
      53,   188,   104,    22,   160,    22,   158,   160,   223,   105,
      98,   149,   136,   137,   224,   109,   104,   238,   103,   103,
     105,   105,   105,   105,   105,    22,   207,    22,   206,   105,
      76,    77,    78,    79,   189,    22,   202,   105,   105,   105,
     103,   110,   106,   107,   145,   103,    22,    94,   138,    61,
      72,   168,   171,   172,   173,   209,   210,   213,   218,    22,
     241,    83,    84,    85,    86,    87,    88,    89,    90,   130,
     133,    56,    25,    39,    59,    71,   177,   185,   235,   105,
     105,   105,   223,    22,   146,    22,   147,   137,    33,    45,
     197,   199,   103,   110,   105,    31,   178,   182,   185,   103,
     110,   103,   104,   104,    65,   198,   171,   223,   137,   211,
     216,   214,   217,   104,   110,   103,   105,   100,   105,   100,
     202,   137,   216,   217,   100,   202,   105
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
#line 252 "program_parse.y"
    {
	   if (state->prog->Target != GL_VERTEX_PROGRAM_ARB) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid fragment program header");

	   }
	   state->mode = ARB_vertex;
	;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 260 "program_parse.y"
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
#line 276 "program_parse.y"
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
#line 302 "program_parse.y"
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
#line 320 "program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumAluInstructions++;
	;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 325 "program_parse.y"
    {
	   (yyval.inst) = (yyvsp[(1) - (1)].inst);
	   state->prog->NumTexInstructions++;
	;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 345 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_ARL, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 351 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 358 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (4)].temp_inst).Opcode, & (yyvsp[(2) - (4)].dst_reg), & (yyvsp[(4) - (4)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (4)].temp_inst).SaturateMode;
	;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 365 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 373 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (6)].temp_inst).Opcode, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), & (yyvsp[(6) - (6)].src_reg), NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 381 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor((yyvsp[(1) - (8)].temp_inst).Opcode, & (yyvsp[(2) - (8)].dst_reg), & (yyvsp[(4) - (8)].src_reg), & (yyvsp[(6) - (8)].src_reg), & (yyvsp[(8) - (8)].src_reg));
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (8)].temp_inst).SaturateMode;
	;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 388 "program_parse.y"
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

  case 30:

/* Line 1455 of yacc.c  */
#line 433 "program_parse.y"
    {
	   (yyval.inst) = asm_instruction_ctor(OPCODE_KIL, NULL, & (yyvsp[(2) - (2)].src_reg), NULL, NULL);
	   state->fragment.UsesKill = 1;
	;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 440 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 445 "program_parse.y"
    { (yyval.integer) = TEXTURE_1D_INDEX; ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 446 "program_parse.y"
    { (yyval.integer) = TEXTURE_2D_INDEX; ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 447 "program_parse.y"
    { (yyval.integer) = TEXTURE_3D_INDEX; ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 448 "program_parse.y"
    { (yyval.integer) = TEXTURE_CUBE_INDEX; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 449 "program_parse.y"
    { (yyval.integer) = TEXTURE_RECT_INDEX; ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 450 "program_parse.y"
    { (yyval.integer) = -TEXTURE_1D_INDEX; ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 451 "program_parse.y"
    { (yyval.integer) = -TEXTURE_2D_INDEX; ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 452 "program_parse.y"
    { (yyval.integer) = -TEXTURE_RECT_INDEX; ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 456 "program_parse.y"
    {
	   /* FIXME: Is this correct?  Should the extenedSwizzle be applied
	    * FIXME: to the existing swizzle?
	    */
	   (yyvsp[(4) - (6)].src_reg).Base.Swizzle = (yyvsp[(6) - (6)].swiz_mask).swizzle;

	   (yyval.inst) = asm_instruction_ctor(OPCODE_SWZ, & (yyvsp[(2) - (6)].dst_reg), & (yyvsp[(4) - (6)].src_reg), NULL, NULL);
	   (yyval.inst)->Base.SaturateMode = (yyvsp[(1) - (6)].temp_inst).SaturateMode;
	;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 468 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 481 "program_parse.y"
    {
	   (yyval.src_reg) = (yyvsp[(2) - (3)].src_reg);

	   if ((yyvsp[(1) - (3)].negate)) {
	      (yyval.src_reg).Base.Negate = ~(yyval.src_reg).Base.Negate;
	   }

	   (yyval.src_reg).Base.Swizzle = _mesa_combine_swizzles((yyval.src_reg).Base.Swizzle,
						    (yyvsp[(3) - (3)].swiz_mask).swizzle);
	;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 494 "program_parse.y"
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

  case 44:

/* Line 1455 of yacc.c  */
#line 516 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_ADDRESS;
	   (yyval.dst_reg).Index = 0;
	   (yyval.dst_reg).WriteMask = (yyvsp[(2) - (2)].swiz_mask).mask;
	;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 525 "program_parse.y"
    {
	   (yyval.swiz_mask).swizzle = MAKE_SWIZZLE4((yyvsp[(1) - (7)].swiz_mask).swizzle, (yyvsp[(3) - (7)].swiz_mask).swizzle,
				      (yyvsp[(5) - (7)].swiz_mask).swizzle, (yyvsp[(7) - (7)].swiz_mask).swizzle);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (7)].swiz_mask).mask) | ((yyvsp[(3) - (7)].swiz_mask).mask << 1) | ((yyvsp[(5) - (7)].swiz_mask).mask << 2)
	      | ((yyvsp[(7) - (7)].swiz_mask).mask << 3);
	;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 534 "program_parse.y"
    {
	   (yyval.swiz_mask).swizzle = (yyvsp[(2) - (2)].integer);
	   (yyval.swiz_mask).mask = ((yyvsp[(1) - (2)].negate)) ? 1 : 0;
	;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 541 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) != 0) && ((yyvsp[(1) - (1)].integer) != 1)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid extended swizzle selector");
	      YYERROR;
	   }

	   (yyval.integer) = ((yyvsp[(1) - (1)].integer) == 0) ? SWIZZLE_ZERO : SWIZZLE_ONE;
	;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 550 "program_parse.y"
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

  case 49:

/* Line 1455 of yacc.c  */
#line 578 "program_parse.y"
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

  case 50:

/* Line 1455 of yacc.c  */
#line 620 "program_parse.y"
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

  case 51:

/* Line 1455 of yacc.c  */
#line 631 "program_parse.y"
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

  case 52:

/* Line 1455 of yacc.c  */
#line 652 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.File = ((yyvsp[(1) - (1)].temp_sym).name != NULL) 
	      ? (yyvsp[(1) - (1)].temp_sym).param_binding_type
	      : PROGRAM_CONSTANT;
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].temp_sym).param_binding_begin;
	;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 662 "program_parse.y"
    {
	   init_dst_reg(& (yyval.dst_reg));
	   (yyval.dst_reg).File = PROGRAM_OUTPUT;
	   (yyval.dst_reg).Index = (yyvsp[(1) - (1)].result);
	;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 668 "program_parse.y"
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

  case 55:

/* Line 1455 of yacc.c  */
#line 692 "program_parse.y"
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

  case 58:

/* Line 1455 of yacc.c  */
#line 711 "program_parse.y"
    {
	   init_src_reg(& (yyval.src_reg));
	   (yyval.src_reg).Base.Index = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 718 "program_parse.y"
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

  case 60:

/* Line 1455 of yacc.c  */
#line 729 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 730 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (2)].integer); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 731 "program_parse.y"
    { (yyval.integer) = -(yyvsp[(2) - (2)].integer); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 735 "program_parse.y"
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

  case 64:

/* Line 1455 of yacc.c  */
#line 747 "program_parse.y"
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

  case 65:

/* Line 1455 of yacc.c  */
#line 759 "program_parse.y"
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

  case 66:

/* Line 1455 of yacc.c  */
#line 777 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].swiz_mask).mask != WRITEMASK_X) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid address component selector");
	      YYERROR;
	   } else {
	      (yyval.swiz_mask) = (yyvsp[(1) - (1)].swiz_mask);
	   }
	;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 788 "program_parse.y"
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

  case 72:

/* Line 1455 of yacc.c  */
#line 804 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 808 "program_parse.y"
    { (yyval.swiz_mask).swizzle = SWIZZLE_NOOP; (yyval.swiz_mask).mask = WRITEMASK_XYZW; ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 820 "program_parse.y"
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

  case 85:

/* Line 1455 of yacc.c  */
#line 838 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 842 "program_parse.y"
    {
	   (yyval.attrib) = (yyvsp[(2) - (2)].attrib);
	;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 848 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_POS;
	;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 852 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_WEIGHT;
	;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 856 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_NORMAL;
	;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 860 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_secondary_color) {
	      yyerror(& (yylsp[(2) - (2)]), state, "GL_EXT_secondary_color not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_COLOR0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 869 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_fog_coord) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_EXT_fog_coord not supported");
	      YYERROR;
	   }

	   (yyval.attrib) = VERT_ATTRIB_FOG;
	;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 878 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 882 "program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 887 "program_parse.y"
    {
	   (yyval.attrib) = VERT_ATTRIB_GENERIC0 + (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 893 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxAttribs) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid vertex attribute reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 907 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_WPOS;
	;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 911 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_COL0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 915 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_FOGC;
	;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 919 "program_parse.y"
    {
	   (yyval.attrib) = FRAG_ATTRIB_TEX0 + (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 927 "program_parse.y"
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

  case 106:

/* Line 1455 of yacc.c  */
#line 943 "program_parse.y"
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

  case 107:

/* Line 1455 of yacc.c  */
#line 965 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 969 "program_parse.y"
    {
	   if (((yyvsp[(1) - (1)].integer) < 1) || ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxParameters)) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid parameter array size");
	      YYERROR;
	   } else {
	      (yyval.integer) = (yyvsp[(1) - (1)].integer);
	   }
	;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 980 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(2) - (2)].temp_sym);
	;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 986 "program_parse.y"
    {
	   (yyval.temp_sym) = (yyvsp[(3) - (4)].temp_sym);
	;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 993 "program_parse.y"
    {
	   (yyvsp[(1) - (3)].temp_sym).param_binding_length += (yyvsp[(3) - (3)].temp_sym).param_binding_length;
	   (yyval.temp_sym) = (yyvsp[(1) - (3)].temp_sym);
	;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 1000 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 1006 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 1012 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 1020 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 1026 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 1032 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 1040 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_state(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 1046 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_param(state->prog, & (yyval.temp_sym), (yyvsp[(1) - (1)].state));
	;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 1052 "program_parse.y"
    {
	   memset(& (yyval.temp_sym), 0, sizeof((yyval.temp_sym)));
	   (yyval.temp_sym).param_binding_begin = ~0;
	   initialize_symbol_from_const(state->prog, & (yyval.temp_sym), & (yyvsp[(1) - (1)].vector));
	;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 1059 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(1) - (1)].state), sizeof((yyval.state))); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 1060 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 1063 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 1064 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 1065 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 1066 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 1067 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 1068 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 1069 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 1070 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 1071 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 1072 "program_parse.y"
    { memcpy((yyval.state), (yyvsp[(2) - (2)].state), sizeof((yyval.state))); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 1076 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_MATERIAL;
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 1085 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 1089 "program_parse.y"
    {
	   (yyval.integer) = STATE_EMISSION;
	;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 1093 "program_parse.y"
    {
	   (yyval.integer) = STATE_SHININESS;
	;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 1099 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHT;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (5)].integer);
	;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 1108 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 1112 "program_parse.y"
    {
	   (yyval.integer) = STATE_POSITION;
	;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 1116 "program_parse.y"
    {
	   if (!state->ctx->Extensions.EXT_point_parameters) {
	      yyerror(& (yylsp[(1) - (1)]), state, "GL_ARB_point_parameters not supported");
	      YYERROR;
	   }

	   (yyval.integer) = STATE_ATTENUATION;
	;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 1125 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 1129 "program_parse.y"
    {
	   (yyval.integer) = STATE_HALF_VECTOR;
	;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 1135 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPOT_DIRECTION;
	;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 1141 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (2)].state)[1];
	;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 1148 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_AMBIENT;
	;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 1153 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTMODEL_SCENECOLOR;
	   (yyval.state)[1] = (yyvsp[(1) - (2)].integer);
	;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 1161 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_LIGHTPROD;
	   (yyval.state)[1] = (yyvsp[(3) - (6)].integer);
	   (yyval.state)[2] = (yyvsp[(5) - (6)].integer);
	   (yyval.state)[3] = (yyvsp[(6) - (6)].integer);
	;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 1173 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(3) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(2) - (3)].integer);
	;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 1181 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXENV_COLOR;
	;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 1187 "program_parse.y"
    {
	   (yyval.integer) = STATE_AMBIENT;
	;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 1191 "program_parse.y"
    {
	   (yyval.integer) = STATE_DIFFUSE;
	;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 1195 "program_parse.y"
    {
	   (yyval.integer) = STATE_SPECULAR;
	;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 1201 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxLights) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid light selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 1212 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_TEXGEN;
	   (yyval.state)[1] = (yyvsp[(2) - (4)].integer);
	   (yyval.state)[2] = (yyvsp[(3) - (4)].integer) + (yyvsp[(4) - (4)].integer);
	;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 1221 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S;
	;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 1225 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_OBJECT_S;
	;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 1230 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_S - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 1234 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_T - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 1238 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_R - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 1242 "program_parse.y"
    {
	   (yyval.integer) = STATE_TEXGEN_EYE_Q - STATE_TEXGEN_EYE_S;
	;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 1248 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 1255 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_COLOR;
	;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 1259 "program_parse.y"
    {
	   (yyval.integer) = STATE_FOG_PARAMS;
	;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 1265 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = STATE_CLIPPLANE;
	   (yyval.state)[1] = (yyvsp[(3) - (5)].integer);
	;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 1273 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxClipPlanes) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid clip plane selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 1284 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 1291 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_SIZE;
	;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 1295 "program_parse.y"
    {
	   (yyval.integer) = STATE_POINT_ATTENUATION;
	;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 1301 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (5)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (5)].state)[1];
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[4] = (yyvsp[(1) - (5)].state)[2];
	;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 1311 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (2)].state)[0];
	   (yyval.state)[1] = (yyvsp[(1) - (2)].state)[1];
	   (yyval.state)[2] = (yyvsp[(2) - (2)].state)[2];
	   (yyval.state)[3] = (yyvsp[(2) - (2)].state)[3];
	   (yyval.state)[4] = (yyvsp[(1) - (2)].state)[2];
	;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 1321 "program_parse.y"
    {
	   (yyval.state)[2] = 0;
	   (yyval.state)[3] = 3;
	;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 1326 "program_parse.y"
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

  case 175:

/* Line 1455 of yacc.c  */
#line 1344 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(2) - (3)].state)[0];
	   (yyval.state)[1] = (yyvsp[(2) - (3)].state)[1];
	   (yyval.state)[2] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 1352 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 1356 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 1362 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVERSE;
	;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 1366 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_TRANSPOSE;
	;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 1370 "program_parse.y"
    {
	   (yyval.integer) = STATE_MATRIX_INVTRANS;
	;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 1376 "program_parse.y"
    {
	   if ((yyvsp[(1) - (1)].integer) > 3) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid matrix row reference");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 1387 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MODELVIEW_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 1392 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROJECTION_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 1397 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_MVP_MATRIX;
	   (yyval.state)[1] = 0;
	;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 1402 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_TEXTURE_MATRIX;
	   (yyval.state)[1] = (yyvsp[(2) - (2)].integer);
	;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 1407 "program_parse.y"
    {
	   yyerror(& (yylsp[(1) - (4)]), state, "GL_ARB_matrix_palette not supported");
	   YYERROR;
	;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 1412 "program_parse.y"
    {
	   (yyval.state)[0] = STATE_PROGRAM_MATRIX;
	   (yyval.state)[1] = (yyvsp[(3) - (4)].integer);
	;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 1419 "program_parse.y"
    {
	   (yyval.integer) = 0;
	;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 1423 "program_parse.y"
    {
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 1428 "program_parse.y"
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

  case 191:

/* Line 1455 of yacc.c  */
#line 1441 "program_parse.y"
    {
	   /* Since GL_ARB_matrix_palette isn't supported, just let any value
	    * through here.  The error will be generated later.
	    */
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 1449 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxProgramMatrices) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program matrix selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 1466 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 1476 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 1481 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 1488 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_ENV;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 1498 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].state)[0];
	   (yyval.state)[3] = (yyvsp[(4) - (5)].state)[1];
	;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 1507 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (1)].integer);
	   (yyval.state)[1] = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 1512 "program_parse.y"
    {
	   (yyval.state)[0] = (yyvsp[(1) - (3)].integer);
	   (yyval.state)[1] = (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 1519 "program_parse.y"
    {
	   memset((yyval.state), 0, sizeof((yyval.state)));
	   (yyval.state)[0] = state->state_param_enum;
	   (yyval.state)[1] = STATE_LOCAL;
	   (yyval.state)[2] = (yyvsp[(4) - (5)].integer);
	   (yyval.state)[3] = (yyvsp[(4) - (5)].integer);
	;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 1529 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxEnvParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid environment parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 1539 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->limits->MaxLocalParams) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid local parameter reference");
	      YYERROR;
	   }
	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 1554 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 1561 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 1566 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (float) (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 1573 "program_parse.y"
    {
	   (yyval.vector).count = 1;
	   (yyval.vector).data[0] = (yyvsp[(2) - (3)].real);
	   (yyval.vector).data[1] = 0.0f;
	   (yyval.vector).data[2] = 0.0f;
	   (yyval.vector).data[3] = 0.0f;
	;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 1581 "program_parse.y"
    {
	   (yyval.vector).count = 2;
	   (yyval.vector).data[0] = (yyvsp[(2) - (5)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (5)].real);
	   (yyval.vector).data[2] = 0.0f;
	   (yyval.vector).data[3] = 0.0f;
	;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 1590 "program_parse.y"
    {
	   (yyval.vector).count = 3;
	   (yyval.vector).data[0] = (yyvsp[(2) - (7)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (7)].real);
	   (yyval.vector).data[2] = (yyvsp[(6) - (7)].real);
	   (yyval.vector).data[3] = 0.0f;
	;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 1599 "program_parse.y"
    {
	   (yyval.vector).count = 4;
	   (yyval.vector).data[0] = (yyvsp[(2) - (9)].real);
	   (yyval.vector).data[1] = (yyvsp[(4) - (9)].real);
	   (yyval.vector).data[2] = (yyvsp[(6) - (9)].real);
	   (yyval.vector).data[3] = (yyvsp[(8) - (9)].real);
	;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 1609 "program_parse.y"
    {
	   (yyval.real) = ((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].real) : (yyvsp[(2) - (2)].real);
	;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 1613 "program_parse.y"
    {
	   (yyval.real) = (float)(((yyvsp[(1) - (2)].negate)) ? -(yyvsp[(2) - (2)].integer) : (yyvsp[(2) - (2)].integer));
	;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 1618 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 1619 "program_parse.y"
    { (yyval.negate) = TRUE;  ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 1620 "program_parse.y"
    { (yyval.negate) = FALSE; ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 1623 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 1626 "program_parse.y"
    { (yyval.integer) = (yyvsp[(1) - (1)].integer); ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 1630 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(3) - (3)].string), (yyvsp[(0) - (3)].integer), & (yylsp[(3) - (3)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 1636 "program_parse.y"
    {
	   if (!declare_variable(state, (yyvsp[(1) - (1)].string), (yyvsp[(0) - (1)].integer), & (yylsp[(1) - (1)]))) {
	      YYERROR;
	   }
	;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 1644 "program_parse.y"
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

  case 230:

/* Line 1455 of yacc.c  */
#line 1657 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_HPOS;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 1666 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_FOGC;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 1675 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (2)].result);
	;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 1679 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_PSIZ;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 1688 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.result) = VERT_RESULT_TEX0 + (yyvsp[(3) - (3)].integer);
	   } else {
	      yyerror(& (yylsp[(2) - (3)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 1697 "program_parse.y"
    {
	   if (state->mode == ARB_fragment) {
	      (yyval.result) = FRAG_RESULT_DEPTH;
	   } else {
	      yyerror(& (yylsp[(2) - (2)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 1708 "program_parse.y"
    {
	   (yyval.result) = (yyvsp[(2) - (3)].integer) + (yyvsp[(3) - (3)].integer);
	;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 1714 "program_parse.y"
    {
	   (yyval.integer) = (state->mode == ARB_vertex)
	      ? VERT_RESULT_COL0
	      : FRAG_RESULT_COLOR;
	;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 1720 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_COL0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 1729 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = VERT_RESULT_BFC0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 1740 "program_parse.y"
    {
	   (yyval.integer) = 0; 
	;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 1744 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 0;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 1753 "program_parse.y"
    {
	   if (state->mode == ARB_vertex) {
	      (yyval.integer) = 1;
	   } else {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid program result name");
	      YYERROR;
	   }
	;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 1763 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 1764 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 1765 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 1768 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 1769 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 1770 "program_parse.y"
    { (yyval.integer) = 1; ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 1773 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 1774 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 1777 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 1778 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 1781 "program_parse.y"
    { (yyval.integer) = 0; ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 1782 "program_parse.y"
    { (yyval.integer) = (yyvsp[(2) - (3)].integer); ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 1786 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureCoordUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture coordinate unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 1797 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureImageUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture image unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 1808 "program_parse.y"
    {
	   if ((unsigned) (yyvsp[(1) - (1)].integer) >= state->MaxTextureUnits) {
	      yyerror(& (yylsp[(1) - (1)]), state, "invalid texture unit selector");
	      YYERROR;
	   }

	   (yyval.integer) = (yyvsp[(1) - (1)].integer);
	;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 1819 "program_parse.y"
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
#line 4439 "program_parse.tab.c"
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
#line 1839 "program_parse.y"


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
   struct gl_program_constants limits;
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

