/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse _mesa_glsl_parse
#define yylex   _mesa_glsl_lex
#define yyerror _mesa_glsl_error
#define yylval  _mesa_glsl_lval
#define yychar  _mesa_glsl_char
#define yydebug _mesa_glsl_debug
#define yynerrs _mesa_glsl_nerrs
#define yylloc _mesa_glsl_lloc

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ATTRIBUTE = 258,
     CONST_TOK = 259,
     BASIC_TYPE_TOK = 260,
     BREAK = 261,
     BUFFER = 262,
     CONTINUE = 263,
     DO = 264,
     ELSE = 265,
     FOR = 266,
     IF = 267,
     DEMOTE = 268,
     DISCARD = 269,
     RETURN = 270,
     SWITCH = 271,
     CASE = 272,
     DEFAULT = 273,
     CENTROID = 274,
     IN_TOK = 275,
     OUT_TOK = 276,
     INOUT_TOK = 277,
     UNIFORM = 278,
     VARYING = 279,
     SAMPLE = 280,
     NOPERSPECTIVE = 281,
     FLAT = 282,
     SMOOTH = 283,
     IMAGE1DSHADOW = 284,
     IMAGE2DSHADOW = 285,
     IMAGE1DARRAYSHADOW = 286,
     IMAGE2DARRAYSHADOW = 287,
     COHERENT = 288,
     VOLATILE = 289,
     RESTRICT = 290,
     READONLY = 291,
     WRITEONLY = 292,
     SHARED = 293,
     STRUCT = 294,
     VOID_TOK = 295,
     WHILE = 296,
     IDENTIFIER = 297,
     TYPE_IDENTIFIER = 298,
     NEW_IDENTIFIER = 299,
     FLOATCONSTANT = 300,
     DOUBLECONSTANT = 301,
     INTCONSTANT = 302,
     UINTCONSTANT = 303,
     BOOLCONSTANT = 304,
     INT64CONSTANT = 305,
     UINT64CONSTANT = 306,
     FIELD_SELECTION = 307,
     LEFT_OP = 308,
     RIGHT_OP = 309,
     INC_OP = 310,
     DEC_OP = 311,
     LE_OP = 312,
     GE_OP = 313,
     EQ_OP = 314,
     NE_OP = 315,
     AND_OP = 316,
     OR_OP = 317,
     XOR_OP = 318,
     MUL_ASSIGN = 319,
     DIV_ASSIGN = 320,
     ADD_ASSIGN = 321,
     MOD_ASSIGN = 322,
     LEFT_ASSIGN = 323,
     RIGHT_ASSIGN = 324,
     AND_ASSIGN = 325,
     XOR_ASSIGN = 326,
     OR_ASSIGN = 327,
     SUB_ASSIGN = 328,
     INVARIANT = 329,
     PRECISE = 330,
     LOWP = 331,
     MEDIUMP = 332,
     HIGHP = 333,
     SUPERP = 334,
     PRECISION = 335,
     VERSION_TOK = 336,
     EXTENSION = 337,
     LINE = 338,
     COLON = 339,
     EOL = 340,
     INTERFACE = 341,
     OUTPUT = 342,
     PRAGMA_DEBUG_ON = 343,
     PRAGMA_DEBUG_OFF = 344,
     PRAGMA_OPTIMIZE_ON = 345,
     PRAGMA_OPTIMIZE_OFF = 346,
     PRAGMA_WARNING_ON = 347,
     PRAGMA_WARNING_OFF = 348,
     PRAGMA_INVARIANT_ALL = 349,
     LAYOUT_TOK = 350,
     DOT_TOK = 351,
     ASM = 352,
     CLASS = 353,
     UNION = 354,
     ENUM = 355,
     TYPEDEF = 356,
     TEMPLATE = 357,
     THIS = 358,
     PACKED_TOK = 359,
     GOTO = 360,
     INLINE_TOK = 361,
     NOINLINE = 362,
     PUBLIC_TOK = 363,
     STATIC = 364,
     EXTERN = 365,
     EXTERNAL = 366,
     LONG_TOK = 367,
     SHORT_TOK = 368,
     HALF = 369,
     FIXED_TOK = 370,
     UNSIGNED = 371,
     INPUT_TOK = 372,
     HVEC2 = 373,
     HVEC3 = 374,
     HVEC4 = 375,
     FVEC2 = 376,
     FVEC3 = 377,
     FVEC4 = 378,
     SAMPLER3DRECT = 379,
     SIZEOF = 380,
     CAST = 381,
     NAMESPACE = 382,
     USING = 383,
     RESOURCE = 384,
     PATCH = 385,
     SUBROUTINE = 386,
     ERROR_TOK = 387,
     COMMON = 388,
     PARTITION = 389,
     ACTIVE = 390,
     FILTER = 391,
     ROW_MAJOR = 392,
     THEN = 393
   };
#endif
/* Tokens.  */
#define ATTRIBUTE 258
#define CONST_TOK 259
#define BASIC_TYPE_TOK 260
#define BREAK 261
#define BUFFER 262
#define CONTINUE 263
#define DO 264
#define ELSE 265
#define FOR 266
#define IF 267
#define DEMOTE 268
#define DISCARD 269
#define RETURN 270
#define SWITCH 271
#define CASE 272
#define DEFAULT 273
#define CENTROID 274
#define IN_TOK 275
#define OUT_TOK 276
#define INOUT_TOK 277
#define UNIFORM 278
#define VARYING 279
#define SAMPLE 280
#define NOPERSPECTIVE 281
#define FLAT 282
#define SMOOTH 283
#define IMAGE1DSHADOW 284
#define IMAGE2DSHADOW 285
#define IMAGE1DARRAYSHADOW 286
#define IMAGE2DARRAYSHADOW 287
#define COHERENT 288
#define VOLATILE 289
#define RESTRICT 290
#define READONLY 291
#define WRITEONLY 292
#define SHARED 293
#define STRUCT 294
#define VOID_TOK 295
#define WHILE 296
#define IDENTIFIER 297
#define TYPE_IDENTIFIER 298
#define NEW_IDENTIFIER 299
#define FLOATCONSTANT 300
#define DOUBLECONSTANT 301
#define INTCONSTANT 302
#define UINTCONSTANT 303
#define BOOLCONSTANT 304
#define INT64CONSTANT 305
#define UINT64CONSTANT 306
#define FIELD_SELECTION 307
#define LEFT_OP 308
#define RIGHT_OP 309
#define INC_OP 310
#define DEC_OP 311
#define LE_OP 312
#define GE_OP 313
#define EQ_OP 314
#define NE_OP 315
#define AND_OP 316
#define OR_OP 317
#define XOR_OP 318
#define MUL_ASSIGN 319
#define DIV_ASSIGN 320
#define ADD_ASSIGN 321
#define MOD_ASSIGN 322
#define LEFT_ASSIGN 323
#define RIGHT_ASSIGN 324
#define AND_ASSIGN 325
#define XOR_ASSIGN 326
#define OR_ASSIGN 327
#define SUB_ASSIGN 328
#define INVARIANT 329
#define PRECISE 330
#define LOWP 331
#define MEDIUMP 332
#define HIGHP 333
#define SUPERP 334
#define PRECISION 335
#define VERSION_TOK 336
#define EXTENSION 337
#define LINE 338
#define COLON 339
#define EOL 340
#define INTERFACE 341
#define OUTPUT 342
#define PRAGMA_DEBUG_ON 343
#define PRAGMA_DEBUG_OFF 344
#define PRAGMA_OPTIMIZE_ON 345
#define PRAGMA_OPTIMIZE_OFF 346
#define PRAGMA_WARNING_ON 347
#define PRAGMA_WARNING_OFF 348
#define PRAGMA_INVARIANT_ALL 349
#define LAYOUT_TOK 350
#define DOT_TOK 351
#define ASM 352
#define CLASS 353
#define UNION 354
#define ENUM 355
#define TYPEDEF 356
#define TEMPLATE 357
#define THIS 358
#define PACKED_TOK 359
#define GOTO 360
#define INLINE_TOK 361
#define NOINLINE 362
#define PUBLIC_TOK 363
#define STATIC 364
#define EXTERN 365
#define EXTERNAL 366
#define LONG_TOK 367
#define SHORT_TOK 368
#define HALF 369
#define FIXED_TOK 370
#define UNSIGNED 371
#define INPUT_TOK 372
#define HVEC2 373
#define HVEC3 374
#define HVEC4 375
#define FVEC2 376
#define FVEC3 377
#define FVEC4 378
#define SAMPLER3DRECT 379
#define SIZEOF 380
#define CAST 381
#define NAMESPACE 382
#define USING 383
#define RESOURCE 384
#define PATCH 385
#define SUBROUTINE 386
#define ERROR_TOK 387
#define COMMON 388
#define PARTITION 389
#define ACTIVE 390
#define FILTER 391
#define ROW_MAJOR 392
#define THEN 393




/* Copy the first part of user declarations.  */
#line 1 "src/compiler/glsl/glsl_parser.yy"

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
/* glsl-optimizer change: Image layout qualifiers need only pipe_format enum values. Include
 * p_format.h directly instead of the format conversion/codec API. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <assert.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "compiler/glsl_types.h"
#include "main/context.h"
#include "util/u_string.h"
#include "pipe/p_format.h"

#ifdef _MSC_VER
#pragma warning( disable : 4065 ) // switch statement contains 'default' but no 'case' labels
#endif

#undef yyerror

static void yyerror(YYLTYPE *loc, _mesa_glsl_parse_state *st, const char *msg)
{
   _mesa_glsl_error(loc, st, "%s", msg);
}

static int
_mesa_glsl_lex(YYSTYPE *val, YYLTYPE *loc, _mesa_glsl_parse_state *state)
{
   return _mesa_glsl_lexer_lex(val, loc, state->scanner);
}

static bool match_layout_qualifier(const char *s1, const char *s2,
                                   _mesa_glsl_parse_state *state)
{
   /* From the GLSL 1.50 spec, section 4.3.8 (Layout Qualifiers):
    *
    *     "The tokens in any layout-qualifier-id-list ... are not case
    *     sensitive, unless explicitly noted otherwise."
    *
    * The text "unless explicitly noted otherwise" appears to be
    * vacuous--no desktop GLSL spec (up through GLSL 4.40) notes
    * otherwise.
    *
    * However, the GLSL ES 3.00 spec says, in section 4.3.8 (Layout
    * Qualifiers):
    *
    *     "As for other identifiers, they are case sensitive."
    *
    * So we need to do a case-sensitive or a case-insensitive match,
    * depending on whether we are compiling for GLSL ES.
    */
   if (state->es_shader)
      return strcmp(s1, s2);
   else
      return strcasecmp(s1, s2);
}


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

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 105 "src/compiler/glsl/glsl_parser.yy"
{
   int n;
   int64_t n64;
   float real;
   double dreal;
   const char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_array_specifier *array_specifier;
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
   ast_interface_block *interface_block;
   ast_subroutine_list *subroutine_list;
   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;

   const glsl_type *type;
}
/* Line 193 of yacc.c.  */
#line 507 "src/compiler/glsl/glsl_parser.cpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
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


/* Line 216 of yacc.c.  */
#line 532 "src/compiler/glsl/glsl_parser.cpp"

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
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
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
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
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
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2514

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  162
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  111
/* YYNRULES -- Number of rules.  */
#define YYNRULES  312
/* YYNRULES -- Number of states.  */
#define YYNSTATES  475

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   393

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   146,     2,     2,     2,   150,   153,     2,
     139,   140,   148,   144,   143,   145,     2,   149,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   157,   159,
     151,   158,   152,   156,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   141,     2,   142,   154,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   160,   155,   161,   147,     2,     2,     2,
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
     135,   136,   137,   138
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    19,    22,    25,
      28,    31,    34,    37,    40,    41,    44,    46,    48,    50,
      56,    58,    61,    64,    66,    68,    70,    72,    74,    76,
      78,    80,    82,    84,    88,    90,    95,    97,   101,   104,
     107,   109,   111,   113,   116,   119,   122,   124,   127,   131,
     134,   136,   138,   140,   143,   146,   149,   151,   153,   155,
     157,   159,   163,   167,   171,   173,   177,   181,   183,   187,
     191,   193,   197,   201,   205,   209,   211,   215,   219,   221,
     225,   227,   231,   233,   237,   239,   243,   245,   249,   251,
     255,   257,   263,   265,   269,   271,   273,   275,   277,   279,
     281,   283,   285,   287,   289,   291,   293,   297,   299,   302,
     305,   310,   312,   315,   317,   319,   322,   326,   330,   333,
     337,   341,   344,   347,   348,   351,   354,   357,   360,   363,
     365,   367,   369,   371,   373,   377,   382,   389,   395,   397,
     400,   404,   410,   415,   418,   421,   423,   426,   431,   433,
     437,   439,   443,   445,   447,   449,   451,   453,   458,   460,
     464,   466,   468,   470,   472,   474,   476,   478,   480,   482,
     484,   486,   488,   491,   494,   497,   500,   503,   506,   509,
     512,   515,   517,   519,   521,   523,   525,   527,   529,   531,
     533,   535,   537,   539,   541,   543,   545,   547,   549,   552,
     556,   560,   565,   567,   570,   572,   574,   576,   578,   580,
     583,   585,   587,   589,   595,   600,   602,   605,   609,   611,
     615,   617,   620,   622,   626,   631,   633,   637,   639,   641,
     643,   645,   647,   649,   651,   653,   655,   657,   660,   661,
     666,   668,   670,   673,   677,   679,   682,   685,   687,   690,
     696,   700,   702,   704,   709,   715,   718,   722,   726,   729,
     731,   734,   737,   740,   742,   745,   751,   759,   766,   768,
     770,   772,   773,   776,   780,   783,   786,   789,   793,   796,
     799,   801,   803,   805,   807,   809,   812,   814,   817,   820,
     828,   830,   832,   834,   836,   839,   840,   842,   845,   847,
     850,   854,   857,   861,   864,   868,   871,   875,   878,   882,
     884,   886,   888
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     163,     0,    -1,    -1,   165,   167,   164,   170,    -1,    -1,
      81,    47,    85,    -1,    81,    47,   168,    85,    -1,    88,
      85,    -1,    89,    85,    -1,    90,    85,    -1,    91,    85,
      -1,    94,    85,    -1,    92,    85,    -1,    93,    85,    -1,
      -1,   167,   169,    -1,    42,    -1,    43,    -1,    44,    -1,
      82,   168,    84,   168,    85,    -1,   260,    -1,   170,   260,
      -1,   170,   169,    -1,    42,    -1,    44,    -1,   171,    -1,
      47,    -1,    48,    -1,    50,    -1,    51,    -1,    45,    -1,
      46,    -1,    49,    -1,   139,   198,   140,    -1,   172,    -1,
     173,   141,   174,   142,    -1,   175,    -1,   173,    96,    52,
      -1,   173,    55,    -1,   173,    56,    -1,   198,    -1,   176,
      -1,   177,    -1,   179,   140,    -1,   178,   140,    -1,   180,
      40,    -1,   180,    -1,   180,   196,    -1,   179,   143,   196,
      -1,   181,   139,    -1,   225,    -1,   173,    -1,   173,    -1,
      55,   182,    -1,    56,   182,    -1,   183,   182,    -1,   144,
      -1,   145,    -1,   146,    -1,   147,    -1,   182,    -1,   184,
     148,   182,    -1,   184,   149,   182,    -1,   184,   150,   182,
      -1,   184,    -1,   185,   144,   184,    -1,   185,   145,   184,
      -1,   185,    -1,   186,    53,   185,    -1,   186,    54,   185,
      -1,   186,    -1,   187,   151,   186,    -1,   187,   152,   186,
      -1,   187,    57,   186,    -1,   187,    58,   186,    -1,   187,
      -1,   188,    59,   187,    -1,   188,    60,   187,    -1,   188,
      -1,   189,   153,   188,    -1,   189,    -1,   190,   154,   189,
      -1,   190,    -1,   191,   155,   190,    -1,   191,    -1,   192,
      61,   191,    -1,   192,    -1,   193,    63,   192,    -1,   193,
      -1,   194,    62,   193,    -1,   194,    -1,   194,   156,   198,
     157,   196,    -1,   195,    -1,   182,   197,   196,    -1,   158,
      -1,    64,    -1,    65,    -1,    67,    -1,    66,    -1,    73,
      -1,    68,    -1,    69,    -1,    70,    -1,    71,    -1,    72,
      -1,   196,    -1,   198,   143,   196,    -1,   195,    -1,   201,
     159,    -1,   210,   159,    -1,    80,   228,   225,   159,    -1,
     262,    -1,   202,   140,    -1,   204,    -1,   203,    -1,   204,
     206,    -1,   203,   143,   206,    -1,   212,   171,   139,    -1,
     225,   168,    -1,   213,   225,   168,    -1,   225,   168,   224,
      -1,   207,   205,    -1,   207,   209,    -1,    -1,     4,   207,
      -1,    75,   207,    -1,   208,   207,    -1,   228,   207,    -1,
     223,   207,    -1,    20,    -1,    21,    -1,    22,    -1,   225,
      -1,   211,    -1,   210,   143,   168,    -1,   210,   143,   168,
     224,    -1,   210,   143,   168,   224,   158,   234,    -1,   210,
     143,   168,   158,   234,    -1,   212,    -1,   212,   168,    -1,
     212,   168,   224,    -1,   212,   168,   224,   158,   234,    -1,
     212,   168,   158,   234,    -1,    74,   171,    -1,    75,   171,
      -1,   225,    -1,   220,   225,    -1,    95,   139,   214,   140,
      -1,   215,    -1,   214,   143,   215,    -1,   168,    -1,   168,
     158,   199,    -1,   216,    -1,   137,    -1,   104,    -1,    38,
      -1,   131,    -1,   131,   139,   218,   140,    -1,   168,    -1,
     218,   143,   168,    -1,    28,    -1,    27,    -1,    26,    -1,
      74,    -1,    75,    -1,   221,    -1,   222,    -1,   219,    -1,
     213,    -1,   223,    -1,   217,    -1,   228,    -1,    75,   220,
      -1,    74,   220,    -1,   219,   220,    -1,   213,   220,    -1,
     217,   220,    -1,   221,   220,    -1,   222,   220,    -1,   228,
     220,    -1,   223,   220,    -1,    19,    -1,    25,    -1,   130,
      -1,     4,    -1,     3,    -1,    24,    -1,    20,    -1,    21,
      -1,    22,    -1,    23,    -1,     7,    -1,    38,    -1,    33,
      -1,    34,    -1,    35,    -1,    36,    -1,    37,    -1,   141,
     142,    -1,   141,   199,   142,    -1,   224,   141,   142,    -1,
     224,   141,   199,   142,    -1,   226,    -1,   226,   224,    -1,
     227,    -1,   229,    -1,    43,    -1,    40,    -1,     5,    -1,
     116,     5,    -1,    78,    -1,    77,    -1,    76,    -1,    39,
     168,   160,   230,   161,    -1,    39,   160,   230,   161,    -1,
     231,    -1,   230,   231,    -1,   212,   232,   159,    -1,   233,
      -1,   232,   143,   233,    -1,   168,    -1,   168,   224,    -1,
     196,    -1,   160,   235,   161,    -1,   160,   235,   143,   161,
      -1,   234,    -1,   235,   143,   234,    -1,   200,    -1,   239,
      -1,   238,    -1,   236,    -1,   244,    -1,   245,    -1,   248,
      -1,   254,    -1,   258,    -1,   259,    -1,   160,   161,    -1,
      -1,   160,   240,   243,   161,    -1,   242,    -1,   238,    -1,
     160,   161,    -1,   160,   243,   161,    -1,   237,    -1,   243,
     237,    -1,   243,   169,    -1,   159,    -1,   198,   159,    -1,
      12,   139,   198,   140,   246,    -1,   237,    10,   237,    -1,
     237,    -1,   198,    -1,   212,   168,   158,   234,    -1,    16,
     139,   198,   140,   249,    -1,   160,   161,    -1,   160,   253,
     161,    -1,    17,   198,   157,    -1,    18,   157,    -1,   250,
      -1,   251,   250,    -1,   251,   237,    -1,   252,   237,    -1,
     252,    -1,   253,   252,    -1,    41,   139,   247,   140,   241,
      -1,     9,   237,    41,   139,   198,   140,   159,    -1,    11,
     139,   255,   257,   140,   241,    -1,   244,    -1,   236,    -1,
     247,    -1,    -1,   256,   159,    -1,   256,   159,   198,    -1,
       8,   159,    -1,     6,   159,    -1,    15,   159,    -1,    15,
     198,   159,    -1,    14,   159,    -1,    13,   159,    -1,   261,
      -1,   200,    -1,   166,    -1,   272,    -1,   159,    -1,   201,
     242,    -1,   263,    -1,   213,   262,    -1,   223,   262,    -1,
     264,    44,   160,   266,   161,   265,   159,    -1,    20,    -1,
      21,    -1,    23,    -1,     7,    -1,   221,   264,    -1,    -1,
      44,    -1,    44,   224,    -1,   267,    -1,   267,   266,    -1,
     212,   232,   159,    -1,   213,   268,    -1,   213,    23,   159,
      -1,   213,   269,    -1,   213,     7,   159,    -1,   213,   270,
      -1,   213,    20,   159,    -1,   213,   271,    -1,   213,    21,
     159,    -1,   268,    -1,   269,    -1,   270,    -1,   271,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   299,   299,   298,   323,   325,   332,   342,   343,   344,
     345,   346,   370,   375,   382,   384,   388,   389,   390,   394,
     403,   411,   419,   430,   431,   435,   442,   449,   456,   463,
     470,   477,   484,   491,   498,   499,   505,   509,   516,   522,
     531,   535,   539,   543,   544,   548,   549,   553,   559,   571,
     575,   581,   595,   596,   602,   608,   618,   619,   620,   621,
     625,   626,   632,   638,   647,   648,   654,   663,   664,   670,
     679,   680,   686,   692,   698,   707,   708,   714,   723,   724,
     733,   734,   743,   744,   753,   754,   763,   764,   773,   774,
     783,   784,   793,   794,   803,   804,   805,   806,   807,   808,
     809,   810,   811,   812,   813,   817,   821,   837,   841,   846,
     850,   855,   872,   876,   877,   881,   886,   894,   912,   923,
     928,   943,   951,   968,   971,   979,   987,   999,  1011,  1018,
    1023,  1028,  1037,  1041,  1042,  1052,  1062,  1072,  1086,  1093,
    1104,  1115,  1126,  1137,  1149,  1164,  1171,  1189,  1196,  1197,
    1207,  1730,  1895,  1921,  1926,  1931,  1939,  1944,  1953,  1962,
    1974,  1979,  1984,  1993,  1998,  2003,  2004,  2005,  2006,  2007,
    2008,  2009,  2027,  2035,  2060,  2084,  2098,  2103,  2119,  2144,
    2156,  2164,  2169,  2174,  2181,  2186,  2191,  2196,  2201,  2226,
    2238,  2243,  2248,  2256,  2261,  2266,  2272,  2277,  2285,  2293,
    2299,  2309,  2320,  2321,  2329,  2335,  2341,  2350,  2351,  2352,
    2364,  2369,  2374,  2383,  2390,  2407,  2412,  2420,  2458,  2463,
    2471,  2477,  2486,  2487,  2491,  2498,  2505,  2512,  2518,  2519,
    2523,  2524,  2525,  2526,  2527,  2528,  2529,  2533,  2540,  2539,
    2553,  2554,  2558,  2564,  2573,  2583,  2592,  2604,  2610,  2619,
    2628,  2633,  2641,  2645,  2663,  2671,  2676,  2684,  2689,  2697,
    2705,  2713,  2721,  2729,  2737,  2745,  2752,  2759,  2769,  2770,
    2774,  2776,  2782,  2787,  2796,  2802,  2808,  2814,  2820,  2829,
    2838,  2839,  2840,  2841,  2842,  2846,  2860,  2864,  2877,  2895,
    2914,  2919,  2924,  2929,  2934,  2949,  2952,  2957,  2965,  2970,
    2978,  3002,  3009,  3013,  3020,  3024,  3034,  3043,  3053,  3062,
    3074,  3096,  3106
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ATTRIBUTE", "CONST_TOK",
  "BASIC_TYPE_TOK", "BREAK", "BUFFER", "CONTINUE", "DO", "ELSE", "FOR",
  "IF", "DEMOTE", "DISCARD", "RETURN", "SWITCH", "CASE", "DEFAULT",
  "CENTROID", "IN_TOK", "OUT_TOK", "INOUT_TOK", "UNIFORM", "VARYING",
  "SAMPLE", "NOPERSPECTIVE", "FLAT", "SMOOTH", "IMAGE1DSHADOW",
  "IMAGE2DSHADOW", "IMAGE1DARRAYSHADOW", "IMAGE2DARRAYSHADOW", "COHERENT",
  "VOLATILE", "RESTRICT", "READONLY", "WRITEONLY", "SHARED", "STRUCT",
  "VOID_TOK", "WHILE", "IDENTIFIER", "TYPE_IDENTIFIER", "NEW_IDENTIFIER",
  "FLOATCONSTANT", "DOUBLECONSTANT", "INTCONSTANT", "UINTCONSTANT",
  "BOOLCONSTANT", "INT64CONSTANT", "UINT64CONSTANT", "FIELD_SELECTION",
  "LEFT_OP", "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP",
  "NE_OP", "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN",
  "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN",
  "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT", "PRECISE", "LOWP",
  "MEDIUMP", "HIGHP", "SUPERP", "PRECISION", "VERSION_TOK", "EXTENSION",
  "LINE", "COLON", "EOL", "INTERFACE", "OUTPUT", "PRAGMA_DEBUG_ON",
  "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON", "PRAGMA_OPTIMIZE_OFF",
  "PRAGMA_WARNING_ON", "PRAGMA_WARNING_OFF", "PRAGMA_INVARIANT_ALL",
  "LAYOUT_TOK", "DOT_TOK", "ASM", "CLASS", "UNION", "ENUM", "TYPEDEF",
  "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK", "NOINLINE",
  "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL", "LONG_TOK", "SHORT_TOK",
  "HALF", "FIXED_TOK", "UNSIGNED", "INPUT_TOK", "HVEC2", "HVEC3", "HVEC4",
  "FVEC2", "FVEC3", "FVEC4", "SAMPLER3DRECT", "SIZEOF", "CAST",
  "NAMESPACE", "USING", "RESOURCE", "PATCH", "SUBROUTINE", "ERROR_TOK",
  "COMMON", "PARTITION", "ACTIVE", "FILTER", "ROW_MAJOR", "THEN", "'('",
  "')'", "'['", "']'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'",
  "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'",
  "'{'", "'}'", "$accept", "translation_unit", "@1", "version_statement",
  "pragma_statement", "extension_statement_list", "any_identifier",
  "extension_statement", "external_declaration_list",
  "variable_identifier", "primary_expression", "postfix_expression",
  "integer_expression", "function_call", "function_call_or_method",
  "function_call_generic", "function_call_header_no_parameters",
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
  "parameter_qualifier", "parameter_direction_qualifier",
  "parameter_type_specifier", "init_declarator_list", "single_declaration",
  "fully_specified_type", "layout_qualifier", "layout_qualifier_id_list",
  "layout_qualifier_id", "interface_block_layout_qualifier",
  "subroutine_qualifier", "subroutine_type_list",
  "interpolation_qualifier", "type_qualifier",
  "auxiliary_storage_qualifier", "storage_qualifier", "memory_qualifier",
  "array_specifier", "type_specifier", "type_specifier_nonarray",
  "basic_type_specifier_nonarray", "precision_qualifier",
  "struct_specifier", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "initializer_list", "declaration_statement", "statement",
  "simple_statement", "compound_statement", "@2", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "switch_body", "case_label", "case_label_list", "case_statement",
  "case_statement_list", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "demote_statement", "external_declaration", "function_definition",
  "interface_block", "basic_interface_block", "interface_qualifier",
  "instance_name_opt", "member_list", "member_declaration",
  "layout_uniform_defaults", "layout_buffer_defaults",
  "layout_in_defaults", "layout_out_defaults", "layout_defaults", 0
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
     385,   386,   387,   388,   389,   390,   391,   392,   393,    40,
      41,    91,    93,    44,    43,    45,    33,   126,    42,    47,
      37,    60,    62,    38,    94,   124,    63,    58,    61,    59,
     123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   162,   164,   163,   165,   165,   165,   166,   166,   166,
     166,   166,   166,   166,   167,   167,   168,   168,   168,   169,
     170,   170,   170,   171,   171,   172,   172,   172,   172,   172,
     172,   172,   172,   172,   173,   173,   173,   173,   173,   173,
     174,   175,   176,   177,   177,   178,   178,   179,   179,   180,
     181,   181,   182,   182,   182,   182,   183,   183,   183,   183,
     184,   184,   184,   184,   185,   185,   185,   186,   186,   186,
     187,   187,   187,   187,   187,   188,   188,   188,   189,   189,
     190,   190,   191,   191,   192,   192,   193,   193,   194,   194,
     195,   195,   196,   196,   197,   197,   197,   197,   197,   197,
     197,   197,   197,   197,   197,   198,   198,   199,   200,   200,
     200,   200,   201,   202,   202,   203,   203,   204,   205,   205,
     205,   206,   206,   207,   207,   207,   207,   207,   207,   208,
     208,   208,   209,   210,   210,   210,   210,   210,   211,   211,
     211,   211,   211,   211,   211,   212,   212,   213,   214,   214,
     215,   215,   215,   216,   216,   216,   217,   217,   218,   218,
     219,   219,   219,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   221,   221,   221,   222,   222,   222,   222,   222,   222,
     222,   222,   222,   223,   223,   223,   223,   223,   224,   224,
     224,   224,   225,   225,   226,   226,   226,   227,   227,   227,
     228,   228,   228,   229,   229,   230,   230,   231,   232,   232,
     233,   233,   234,   234,   234,   235,   235,   236,   237,   237,
     238,   238,   238,   238,   238,   238,   238,   239,   240,   239,
     241,   241,   242,   242,   243,   243,   243,   244,   244,   245,
     246,   246,   247,   247,   248,   249,   249,   250,   250,   251,
     251,   252,   252,   253,   253,   254,   254,   254,   255,   255,
     256,   256,   257,   257,   258,   258,   258,   258,   258,   259,
     260,   260,   260,   260,   260,   261,   262,   262,   262,   263,
     264,   264,   264,   264,   264,   265,   265,   265,   266,   266,
     267,   268,   268,   269,   269,   270,   270,   271,   271,   272,
     272,   272,   272
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     4,     2,     2,     2,
       2,     2,     2,     2,     0,     2,     1,     1,     1,     5,
       1,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     4,     1,     3,     2,     2,
       1,     1,     1,     2,     2,     2,     1,     2,     3,     2,
       1,     1,     1,     2,     2,     2,     1,     1,     1,     1,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     5,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     2,     2,
       4,     1,     2,     1,     1,     2,     3,     3,     2,     3,
       3,     2,     2,     0,     2,     2,     2,     2,     2,     1,
       1,     1,     1,     1,     3,     4,     6,     5,     1,     2,
       3,     5,     4,     2,     2,     1,     2,     4,     1,     3,
       1,     3,     1,     1,     1,     1,     1,     4,     1,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     3,
       3,     4,     1,     2,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     5,     4,     1,     2,     3,     1,     3,
       1,     2,     1,     3,     4,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     4,
       1,     1,     2,     3,     1,     2,     2,     1,     2,     5,
       3,     1,     1,     4,     5,     2,     3,     3,     2,     1,
       2,     2,     2,     1,     2,     5,     7,     6,     1,     1,
       1,     0,     2,     3,     2,     2,     2,     3,     2,     2,
       1,     1,     1,     1,     1,     2,     1,     2,     2,     7,
       1,     1,     1,     1,     2,     0,     1,     2,     1,     2,
       3,     2,     3,     2,     3,     2,     3,     2,     3,     1,
       1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    14,     0,     1,     2,    16,    17,    18,
       5,     0,     0,     0,    15,     6,     0,   185,   184,   208,
     191,   181,   187,   188,   189,   190,   186,   182,   162,   161,
     160,   193,   194,   195,   196,   197,   192,     0,   207,   206,
     163,   164,   212,   211,   210,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   183,   156,   284,   282,     3,
     281,     0,     0,   114,   123,     0,   133,   138,   168,   170,
     167,     0,   165,   166,   169,   145,   202,   204,   171,   205,
      20,   280,   111,   286,     0,   309,   310,   311,   312,   283,
       0,     0,     0,   191,   187,   188,   190,    23,    24,   163,
     164,   143,   168,   173,   165,   169,   144,   172,     0,     7,
       8,     9,    10,    12,    13,    11,     0,   209,     0,    22,
      21,   108,     0,   285,   112,   123,   123,   129,   130,   131,
     123,   115,     0,   123,   123,   123,     0,   109,    16,    18,
     139,     0,   191,   187,   188,   190,   175,   287,   301,   303,
     305,   307,   176,   174,   146,   177,   294,   178,   168,   180,
     288,     0,   203,   179,     0,     0,     0,     0,   215,     0,
       0,   155,   154,   153,   150,     0,   148,   152,   158,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      30,    31,    26,    27,    32,    28,    29,     0,     0,     0,
      56,    57,    58,    59,   247,   238,   242,    25,    34,    52,
      36,    41,    42,     0,     0,    46,     0,    60,     0,    64,
      67,    70,    75,    78,    80,    82,    84,    86,    88,    90,
      92,   105,     0,   227,     0,   145,   230,   244,   229,   228,
       0,   231,   232,   233,   234,   235,   236,   116,   124,   125,
     121,   122,     0,   132,   126,   128,   127,   134,     0,   140,
     117,   304,   306,   308,   302,   198,    60,   107,     0,    50,
       0,     0,    19,   220,     0,   218,   214,   216,     0,   110,
       0,   147,     0,   157,     0,   275,   274,     0,     0,     0,
     279,   278,   276,     0,     0,     0,    53,    54,     0,   237,
       0,    38,    39,     0,     0,    44,    43,     0,   207,    47,
      49,    95,    96,    98,    97,   100,   101,   102,   103,   104,
      99,    94,     0,    55,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   248,   243,   246,   245,     0,
     118,     0,   135,     0,   222,   142,     0,   199,   200,     0,
       0,     0,   298,   221,     0,   217,   213,   151,   149,   159,
       0,   269,   268,   271,     0,   277,     0,   252,     0,     0,
      33,     0,    37,     0,    40,    48,    93,    61,    62,    63,
      65,    66,    68,    69,    73,    74,    71,    72,    76,    77,
      79,    81,    83,    85,    87,    89,     0,   106,   119,   120,
     137,     0,   225,     0,   141,   201,     0,   295,   299,   219,
       0,   270,     0,     0,     0,     0,     0,     0,   239,    35,
       0,   136,     0,   223,   300,   296,     0,     0,   272,     0,
     251,   249,     0,   254,     0,   241,   265,   240,    91,   224,
     226,   297,   289,     0,   273,   267,     0,     0,     0,   255,
     259,     0,   263,     0,   253,   266,   250,     0,   258,   261,
     260,   262,   256,   264,   257
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    13,     3,    58,     6,   273,   347,    59,   207,
     208,   209,   383,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   322,   232,   268,   233,   234,
      62,    63,    64,   250,   131,   132,   133,   251,    65,    66,
      67,   102,   175,   176,   177,    69,   179,    70,    71,    72,
      73,   105,   162,   269,    76,    77,    78,    79,   167,   168,
     274,   275,   355,   413,   236,   237,   238,   239,   300,   446,
     447,   240,   241,   242,   441,   379,   243,   443,   460,   461,
     462,   463,   244,   373,   422,   423,   245,   246,    80,    81,
      82,    83,    84,   436,   361,   362,    85,    86,    87,    88,
      89
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -292
static const yytype_int16 yypact[] =
{
      21,    64,   115,  -292,     5,  -292,    58,  -292,  -292,  -292,
    -292,    50,   154,  1766,  -292,  -292,    61,  -292,  -292,  -292,
     119,  -292,   130,   136,  -292,   168,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,   -23,  -292,  -292,
    2188,  2188,  -292,  -292,  -292,   167,   132,   144,   147,   153,
     170,   171,   175,   124,   261,  -292,   134,  -292,  -292,  1667,
    -292,  -122,   141,   131,   173,  -120,  -292,   210,  2254,  2317,
    2317,    31,  2383,  2317,  2383,  -292,   135,  -292,  2317,  -292,
    -292,  -292,  -292,  -292,   241,  -292,  -292,  -292,  -292,  -292,
     154,  2125,   126,  -292,  -292,  -292,  -292,  -292,  -292,  2317,
    2317,  -292,  2317,  -292,  2317,  2317,  -292,  -292,    31,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,   180,  -292,   154,  -292,
    -292,  -292,   815,  -292,  -292,   547,   547,  -292,  -292,  -292,
     547,  -292,     2,   547,   547,   547,   154,  -292,   149,   151,
     -59,   155,   -32,   -31,   -20,   -17,  -292,  -292,  -292,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  2383,  -292,
    -292,  1859,   152,  -292,   139,   212,   154,   944,  -292,  2125,
     145,  -292,  -292,  -292,   148,   -33,  -292,  -292,  -292,    22,
     146,   156,  1294,   163,   172,   160,   162,  1772,   177,   186,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  1995,  1995,  1995,
    -292,  -292,  -292,  -292,  -292,   165,  -292,  -292,  -292,   123,
    -292,  -292,  -292,   188,    32,  2027,   190,   273,  1995,   120,
      13,   137,    15,   143,   159,   179,   181,   246,   247,   -56,
    -292,  -292,   -67,  -292,   189,   195,  -292,  -292,  -292,  -292,
     497,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,    31,   154,  -292,  -292,  -292,   -57,  1506,   -55,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,   205,  -292,
    1971,  2125,  -292,   135,   -63,  -292,  -292,  -292,  1007,  -292,
    1995,  -292,   180,  -292,   154,  -292,  -292,   309,  1581,  1995,
    -292,  -292,  -292,   -54,  1995,  1917,  -292,  -292,    44,  -292,
    1294,  -292,  -292,   299,  1995,  -292,  -292,  1995,   213,  -292,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
    -292,  -292,  1995,  -292,  1995,  1995,  1995,  1995,  1995,  1995,
    1995,  1995,  1995,  1995,  1995,  1995,  1995,  1995,  1995,  1995,
    1995,  1995,  1995,  1995,  1995,  -292,  -292,  -292,  -292,   154,
     135,  1506,   -50,  1506,  -292,  -292,  1506,  -292,  -292,   214,
     154,   191,  2125,   152,   154,  -292,  -292,  -292,  -292,  -292,
     220,  -292,  -292,  1917,    46,  -292,    71,   218,   154,   224,
    -292,   656,  -292,   223,   218,  -292,  -292,  -292,  -292,  -292,
     120,   120,    13,    13,   137,   137,   137,   137,    15,    15,
     143,   159,   179,   181,   246,   247,    25,  -292,  -292,   152,
    -292,  1506,  -292,  -109,  -292,  -292,   -45,   323,  -292,  -292,
    1995,  -292,   215,   233,  1294,   216,   219,  1452,  -292,  -292,
    1995,  -292,   950,  -292,  -292,   135,   221,    73,  1995,  1452,
     368,  -292,    -8,  -292,  1506,  -292,  -292,  -292,  -292,  -292,
    -292,   152,  -292,   222,   218,  -292,  1294,  1995,   226,  -292,
    -292,  1136,  1294,    -1,  -292,  -292,  -292,    28,  -292,  -292,
    -292,  -292,  -292,  1294,  -292
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -292,  -292,  -292,  -292,  -292,  -292,    14,     9,  -292,    53,
    -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,  -292,
     157,  -292,  -107,  -104,   -97,   -89,    42,    55,    45,    48,
      56,    52,  -292,  -136,  -152,  -292,  -143,  -237,    -5,    -2,
    -292,  -292,  -292,  -292,   271,   236,  -292,  -292,  -292,  -292,
     -90,     1,  -292,   116,  -292,  -292,  -292,  -292,   317,   -38,
    -292,    -9,  -135,   -13,  -292,  -292,   197,  -292,   230,  -137,
      40,    37,  -268,  -292,   114,  -153,  -291,  -292,  -292,   -36,
     343,   105,   118,  -292,  -292,    34,  -292,  -292,   -53,  -292,
     -51,  -292,  -292,  -292,  -292,  -292,  -292,  -292,   350,  -292,
     -46,  -292,   338,  -292,    51,  -292,   352,   355,   356,   361,
    -292
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -294
static const yytype_int16 yytable[] =
{
      75,   166,   104,   104,    74,   259,   342,    19,    60,   457,
     458,    61,  -293,  -290,    68,    14,   457,   458,    11,     7,
       8,     9,   147,   136,  -291,   267,    16,  -292,   160,   287,
     277,   104,   104,   359,   432,   104,    19,   121,   122,   137,
     104,    37,    38,   367,   293,    39,    75,     7,     8,     9,
      74,    92,   433,   104,    60,   134,   298,    61,   154,    74,
      68,   104,   104,   309,   104,    74,   104,   104,   119,    68,
      37,    38,   331,   332,    39,   158,   344,   166,    75,   166,
     364,   140,   161,   410,   161,   412,   270,   348,   414,   344,
      10,   270,   345,   101,   106,   170,   365,    53,   364,   258,
     343,   351,     1,   356,   165,   375,   354,   281,   411,   235,
     282,     4,   147,    74,   434,     5,   134,   134,    54,   253,
     141,   134,   352,   158,   134,   134,   134,   261,   262,   104,
     174,   104,   178,   252,   267,    15,   445,    91,   363,   263,
      12,   277,   264,   431,   267,    90,   374,    54,   445,    74,
     257,   376,   377,   459,    75,   385,    75,   327,   328,   158,
     472,   384,   283,  -293,   450,   284,   333,   334,   344,   235,
     386,   344,   306,    74,  -290,   307,   464,   126,   301,   302,
    -291,   360,   430,   158,   380,   474,   424,   344,   166,   344,
     329,   330,   407,   127,   128,   129,     7,     8,     9,   354,
     406,   354,   335,   336,   354,   378,    31,    32,    33,    34,
      35,   425,  -292,   453,   344,   409,   344,   109,   171,   303,
     390,   391,     7,     8,     9,   392,   393,   235,   348,   110,
     377,    74,   111,   104,   394,   395,   396,   397,   112,   349,
     104,   158,   108,    42,    43,    44,   398,   399,   130,    42,
      43,    44,   138,     8,   139,   113,   114,   104,    75,   354,
     115,   135,   -51,   116,   304,    75,   117,   350,   324,   325,
     326,   440,   360,   118,   125,   235,   161,   437,   448,    74,
     354,   124,   235,   378,   172,   164,   169,   235,   -23,   158,
     -24,    74,   354,   270,   260,   454,   174,   272,   369,   271,
     451,   158,   288,   466,   279,   285,   280,   340,   469,   471,
     341,   289,   337,  -113,   467,   286,   294,   173,   266,   290,
     471,   291,   135,   135,   104,   295,   299,   135,   305,   310,
     135,   135,   135,   338,   -50,   104,   339,   311,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   357,   121,    75,
     370,   382,   417,   -45,   296,   297,   415,   103,   107,   420,
     235,   344,   248,   408,   427,   429,   249,   435,   235,   254,
     255,   256,    74,   439,   438,   323,   442,   444,   456,   400,
     452,   465,   158,   468,   402,   146,   152,   153,   403,   155,
     157,   159,   426,   401,   405,   163,   247,   404,   368,   278,
     416,   419,   371,   455,   123,   381,   372,   421,   470,   120,
     156,   235,   473,   418,   235,    74,   103,   107,    74,   146,
     148,   155,   159,   149,   150,   158,   235,   266,   158,   151,
      74,   321,     0,     0,     0,     0,     0,   266,     0,     0,
     158,     0,     0,   235,     0,     0,     0,    74,   235,   235,
       0,     0,    74,    74,     0,     0,     0,   158,     0,     0,
     235,     0,   158,   158,    74,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   158,   146,     0,     0,     0,     0,
       0,   387,   388,   389,   266,   266,   266,   266,   266,   266,
     266,   266,   266,   266,   266,   266,   266,   266,   266,   266,
      17,    18,    19,   180,    20,   181,   182,     0,   183,   184,
     185,   186,   187,   188,     0,     0,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,     0,     0,     0,     0,
      31,    32,    33,    34,    35,    36,    37,    38,   189,    97,
      39,    98,   190,   191,   192,   193,   194,   195,   196,     0,
       0,   126,   197,   198,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   127,   128,   129,
       0,    40,    41,    42,    43,    44,     0,    45,     0,    12,
      31,    32,    33,    34,    35,     0,     0,     0,     0,     0,
       0,     0,    53,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    54,     0,     0,     0,     0,     0,     0,
       0,     0,   130,    42,    43,    44,     0,    55,    56,     0,
       0,     0,     0,     0,     0,     0,   199,     0,     0,     0,
       0,   200,   201,   202,   203,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   204,   205,   346,    17,
      18,    19,   180,    20,   181,   182,     0,   183,   184,   185,
     186,   187,   188,     0,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,     0,     0,     0,     0,    31,
      32,    33,    34,    35,    36,    37,    38,   189,    97,    39,
      98,   190,   191,   192,   193,   194,   195,   196,     0,     0,
       0,   197,   198,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,    41,    42,    43,    44,     0,    45,     0,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    53,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,    56,     0,     0,
       0,     0,     0,     0,     0,   199,     0,     0,     0,     0,
     200,   201,   202,   203,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   204,   205,   428,    17,    18,
      19,   180,    20,   181,   182,     0,   183,   184,   185,   186,
     187,   188,     0,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,     0,     0,     0,     0,    31,    32,
      33,    34,    35,    36,    37,    38,   189,    97,    39,    98,
     190,   191,   192,   193,   194,   195,   196,     0,     0,     0,
     197,   198,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    40,
      41,    42,    43,    44,     0,    45,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      53,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    55,    56,    17,    18,    19,
       0,    93,     0,     0,   199,    19,     0,     0,     0,   200,
     201,   202,   203,    21,    94,    95,    24,    96,    26,    27,
      28,    29,    30,     0,   204,   205,   206,    31,    32,    33,
      34,    35,    36,    37,    38,     0,     0,    39,     0,    37,
      38,     0,    97,    39,    98,   190,   191,   192,   193,   194,
     195,   196,     0,     0,     0,   197,   198,     0,     0,     0,
      17,    18,    19,     0,    93,     0,     0,     0,    99,   100,
      42,    43,    44,     0,     0,     0,    21,    94,    95,    24,
      96,    26,    27,    28,    29,    30,     0,     0,     0,    53,
      31,    32,    33,    34,    35,    36,    37,    38,     0,     0,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      54,     0,     0,     0,     0,     0,    54,     0,     0,     0,
       0,     0,     0,     0,    55,    56,     0,     0,     0,     0,
       0,    99,   100,    42,    43,    44,     0,     0,     0,   199,
       0,     0,     0,     0,   200,   201,   202,   203,     0,     0,
       0,     0,    53,     0,     0,   276,     0,     0,     0,     0,
     353,   449,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    54,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    55,    56,    17,
      18,    19,   180,    20,   181,   182,     0,   183,   184,   185,
     186,   187,   188,   457,   458,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,     0,     0,     0,   366,    31,
      32,    33,    34,    35,    36,    37,    38,   189,    97,    39,
      98,   190,   191,   192,   193,   194,   195,   196,     0,     0,
       0,   197,   198,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,    41,    42,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    53,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    54,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,    56,     0,     0,
       0,     0,     0,     0,     0,   199,     0,     0,     0,     0,
     200,   201,   202,   203,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   204,   205,    17,    18,    19,
     180,    20,   181,   182,     0,   183,   184,   185,   186,   187,
     188,     0,     0,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,     0,     0,     0,     0,    31,    32,    33,
      34,    35,    36,    37,    38,   189,    97,    39,    98,   190,
     191,   192,   193,   194,   195,   196,     0,     0,     0,   197,
     198,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    40,    41,
      42,    43,    44,     0,    45,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    53,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      54,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,    56,     0,     0,     0,     0,
       0,     0,     0,   199,     0,     0,     0,     0,   200,   201,
     202,   203,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   204,   205,    17,    18,    19,   180,    20,
     181,   182,     0,   183,   184,   185,   186,   187,   188,     0,
       0,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,     0,     0,     0,     0,    31,    32,    33,    34,    35,
      36,    37,    38,   189,    97,    39,    98,   190,   191,   192,
     193,   194,   195,   196,     0,     0,     0,   197,   198,     0,
       0,    19,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    40,    41,    42,    43,
      44,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    37,    38,    53,    97,    39,
      98,   190,   191,   192,   193,   194,   195,   196,     0,     0,
       0,   197,   198,     0,     0,     0,     0,     0,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    55,    56,    17,    18,    19,     0,    20,     0,
       0,   199,     0,     0,     0,     0,   200,   201,   202,   203,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
       0,   204,   122,     0,    31,    32,    33,    34,    35,    36,
      37,    38,    54,    97,    39,    98,   190,   191,   192,   193,
     194,   195,   196,     0,     0,     0,   197,   198,     0,     0,
       0,     0,     0,     0,     0,   199,     0,     0,     0,     0,
     200,   201,   202,   203,     0,    40,    41,    42,    43,    44,
       0,    45,     0,     0,     0,     0,   353,     0,     0,     0,
      17,    18,    19,     0,    20,     0,    53,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,     0,    54,     0,     0,
      31,    32,    33,    34,    35,    36,    37,    38,     0,     0,
      39,    55,    56,     0,     0,     0,     0,     0,     0,     0,
     199,     0,     0,     0,     0,   200,   201,   202,   203,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     204,    40,    41,    42,    43,    44,     0,    45,     0,    12,
       0,     0,     0,     0,     0,    46,    47,    48,    49,    50,
      51,    52,    53,     0,     0,     0,     0,     0,     0,    17,
      18,    19,     0,    20,     0,     0,     0,    19,     0,     0,
       0,     0,     0,    54,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,     0,     0,    55,    56,    31,
      32,    33,    34,    35,    36,    37,    38,     0,     0,    39,
       0,    37,    38,     0,    97,    39,    98,   190,   191,   192,
     193,   194,   195,   196,     0,     0,    57,   197,   198,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      40,    41,    42,    43,    44,     0,    45,     0,     0,     0,
       0,     0,     0,     0,    46,    47,    48,    49,    50,    51,
      52,    53,     0,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    54,     0,     0,     0,     0,     0,    54,     0,
       0,     0,     0,     0,     0,     0,    55,    56,    37,    38,
       0,    97,    39,    98,   190,   191,   192,   193,   194,   195,
     196,   199,     0,     0,   197,   198,   200,   201,   202,   203,
      17,    18,    19,     0,    93,    57,     0,     0,     0,     0,
       0,   292,     0,     0,     0,     0,    21,    94,    95,    24,
      96,    26,    27,    28,    29,    30,     0,     0,     0,     0,
      31,    32,    33,    34,    35,    36,    37,    38,     0,    97,
      39,    98,   190,   191,   192,   193,   194,   195,   196,     0,
       0,     0,   197,   198,     0,    54,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    99,   100,    42,    43,    44,     0,     0,   199,     0,
      19,   265,     0,   200,   201,   202,   203,     0,     0,     0,
      37,    38,    53,    97,    39,    98,   190,   191,   192,   193,
     194,   195,   196,     0,     0,     0,   197,   198,     0,     0,
       0,     0,    19,    54,    37,    38,     0,    97,    39,    98,
     190,   191,   192,   193,   194,   195,   196,    55,    56,     0,
     197,   198,     0,     0,     0,     0,   199,     0,     0,     0,
       0,   200,   201,   202,   203,     0,    37,   308,     0,    97,
      39,    98,   190,   191,   192,   193,   194,   195,   196,     0,
       0,     0,   197,   198,     0,     0,     0,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     199,    54,     0,   358,     0,   200,   201,   202,   203,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    17,    18,
      19,     0,    93,     0,   199,     0,     0,     0,     0,   200,
     201,   202,   203,    54,    21,    94,    95,    24,    96,    26,
      27,    28,    29,    30,     0,     0,     0,     0,    31,    32,
      33,    34,    35,    36,    37,    38,   199,     0,    39,     0,
       0,   200,   201,   202,   203,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    17,    18,     0,     0,    93,     0,     0,     0,    99,
     100,    42,    43,    44,     0,     0,     0,    21,    94,    95,
      24,    96,    26,    27,    28,    29,    30,     0,     0,     0,
      53,    31,    32,    33,    34,    35,    36,     0,     0,     0,
      97,     0,    98,     0,     0,     0,     0,     0,     0,     0,
       0,    54,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    55,    56,    17,    18,     0,
       0,   142,    99,   100,    42,    43,    44,     0,     0,     0,
       0,     0,     0,    21,   143,   144,    24,   145,    26,    27,
      28,    29,    30,    53,     0,     0,     0,    31,    32,    33,
      34,    35,    36,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    55,    56,
      17,    18,     0,     0,    93,     0,     0,     0,    99,   100,
      42,    43,    44,     0,     0,     0,    21,    94,    95,    24,
      96,    26,    27,    28,    29,    30,     0,     0,     0,    53,
      31,    32,    33,    34,    35,    36,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    55,    56,    17,    18,     0,     0,
      20,    99,   100,    42,    43,    44,     0,     0,     0,     0,
       0,     0,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    53,     0,     0,     0,    31,    32,    33,    34,
      35,    36,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    55,    56,     0,
       0,     0,     0,     0,     0,     0,     0,    99,   100,    42,
      43,    44,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    53,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    55,    56
};

static const yytype_int16 yycheck[] =
{
      13,    91,    40,    41,    13,   140,    62,     5,    13,    17,
      18,    13,    44,    44,    13,     6,    17,    18,     4,    42,
      43,    44,    68,   143,    44,   161,    12,    44,    74,   182,
     167,    69,    70,   270,   143,    73,     5,   159,   160,   159,
      78,    39,    40,   280,   187,    43,    59,    42,    43,    44,
      59,    37,   161,    91,    59,    64,   199,    59,    71,    68,
      59,    99,   100,   215,   102,    74,   104,   105,    59,    68,
      39,    40,    57,    58,    43,    74,   143,   167,    91,   169,
     143,    67,   141,   351,   141,   353,   141,   240,   356,   143,
      85,   141,   159,    40,    41,   108,   159,    95,   143,   158,
     156,   158,    81,   158,    90,   159,   258,   140,   158,   122,
     143,    47,   158,   122,   159,     0,   125,   126,   116,   132,
      67,   130,   257,   122,   133,   134,   135,   159,   159,   167,
     116,   169,   118,   132,   270,    85,   427,   160,   273,   159,
      82,   278,   159,   411,   280,    84,   289,   116,   439,   158,
     136,   294,   295,   161,   167,   307,   169,   144,   145,   158,
     161,   304,   140,    44,   432,   143,   151,   152,   143,   182,
     322,   143,   140,   182,    44,   143,   444,     4,    55,    56,
      44,   271,   157,   182,   140,   157,   140,   143,   278,   143,
      53,    54,   344,    20,    21,    22,    42,    43,    44,   351,
     343,   353,    59,    60,   356,   295,    33,    34,    35,    36,
      37,   140,    44,   140,   143,   350,   143,    85,    38,    96,
     327,   328,    42,    43,    44,   329,   330,   240,   381,    85,
     373,   240,    85,   271,   331,   332,   333,   334,    85,   252,
     278,   240,    45,    76,    77,    78,   335,   336,    75,    76,
      77,    78,    42,    43,    44,    85,    85,   295,   271,   411,
      85,    64,   139,   139,   141,   278,     5,   253,   148,   149,
     150,   424,   362,   139,   143,   288,   141,   420,   430,   288,
     432,   140,   295,   373,   104,    44,   160,   300,   139,   288,
     139,   300,   444,   141,   139,   438,   282,    85,   284,   160,
     435,   300,   139,   456,   159,   159,   158,    61,   461,   462,
      63,   139,   153,   140,   457,   159,   139,   137,   161,   159,
     473,   159,   125,   126,   362,   139,   161,   130,   140,   139,
     133,   134,   135,   154,   139,   373,   155,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,   142,   159,   362,
      41,    52,   161,   140,   197,   198,   142,    40,    41,   139,
     373,   143,   126,   349,   140,   142,   130,    44,   381,   133,
     134,   135,   381,   140,   159,   218,   160,   158,    10,   337,
     159,   159,   381,   157,   339,    68,    69,    70,   340,    72,
      73,    74,   378,   338,   342,    78,   125,   341,   282,   169,
     360,   364,   288,   439,    61,   300,   288,   373,   461,    59,
      72,   424,   463,   362,   427,   424,    99,   100,   427,   102,
      68,   104,   105,    68,    68,   424,   439,   270,   427,    68,
     439,   158,    -1,    -1,    -1,    -1,    -1,   280,    -1,    -1,
     439,    -1,    -1,   456,    -1,    -1,    -1,   456,   461,   462,
      -1,    -1,   461,   462,    -1,    -1,    -1,   456,    -1,    -1,
     473,    -1,   461,   462,   473,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   473,   158,    -1,    -1,    -1,    -1,
      -1,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    12,
      13,    14,    15,    16,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    -1,
      -1,     4,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,
      -1,    74,    75,    76,    77,    78,    -1,    80,    -1,    82,
      33,    34,    35,    36,    37,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    75,    76,    77,    78,    -1,   130,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,   147,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   159,   160,   161,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    12,    13,
      14,    15,    16,    -1,    -1,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,    -1,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    -1,    -1,
      -1,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    -1,    80,    -1,    82,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   159,   160,   161,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    12,    13,    14,
      15,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    -1,    -1,    -1,
      55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    74,
      75,    76,    77,    78,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   130,   131,     3,     4,     5,
      -1,     7,    -1,    -1,   139,     5,    -1,    -1,    -1,   144,
     145,   146,   147,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    -1,   159,   160,   161,    33,    34,    35,
      36,    37,    38,    39,    40,    -1,    -1,    43,    -1,    39,
      40,    -1,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    -1,    -1,    -1,    55,    56,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,    -1,    -1,    -1,    74,    75,
      76,    77,    78,    -1,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    95,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,    -1,    -1,    -1,    -1,    -1,   116,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   130,   131,    -1,    -1,    -1,    -1,
      -1,    74,    75,    76,    77,    78,    -1,    -1,    -1,   139,
      -1,    -1,    -1,    -1,   144,   145,   146,   147,    -1,    -1,
      -1,    -1,    95,    -1,    -1,   161,    -1,    -1,    -1,    -1,
     160,   161,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,   131,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,    -1,   161,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    -1,    -1,
      -1,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    95,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   159,   160,     3,     4,     5,
       6,     7,     8,     9,    -1,    11,    12,    13,    14,    15,
      16,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    -1,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    -1,    -1,    -1,    55,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    74,    75,
      76,    77,    78,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   130,   131,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,   144,   145,
     146,   147,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   159,   160,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    12,    13,    14,    15,    16,    -1,
      -1,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    -1,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    -1,    -1,    -1,    55,    56,    -1,
      -1,     5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    74,    75,    76,    77,
      78,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    39,    40,    95,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    -1,    -1,
      -1,    55,    56,    -1,    -1,    -1,    -1,    -1,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   130,   131,     3,     4,     5,    -1,     7,    -1,
      -1,   139,    -1,    -1,    -1,    -1,   144,   145,   146,   147,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      -1,   159,   160,    -1,    33,    34,    35,    36,    37,    38,
      39,    40,   116,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    -1,    55,    56,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,    -1,
     144,   145,   146,   147,    -1,    74,    75,    76,    77,    78,
      -1,    80,    -1,    -1,    -1,    -1,   160,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,    -1,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,   116,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    -1,
      43,   130,   131,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,    -1,    -1,    -1,    -1,   144,   145,   146,   147,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     159,    74,    75,    76,    77,    78,    -1,    80,    -1,    82,
      -1,    -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,
      93,    94,    95,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,    -1,    -1,    -1,     5,    -1,    -1,
      -1,    -1,    -1,   116,    -1,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    -1,   130,   131,    33,
      34,    35,    36,    37,    38,    39,    40,    -1,    -1,    43,
      -1,    39,    40,    -1,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    -1,    -1,   159,    55,    56,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      74,    75,    76,    77,    78,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    88,    89,    90,    91,    92,    93,
      94,    95,    -1,    -1,     5,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    39,    40,
      -1,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,   139,    -1,    -1,    55,    56,   144,   145,   146,   147,
       3,     4,     5,    -1,     7,   159,    -1,    -1,    -1,    -1,
      -1,   159,    -1,    -1,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    -1,
      -1,    -1,    55,    56,    -1,   116,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    74,    75,    76,    77,    78,    -1,    -1,   139,    -1,
       5,   142,    -1,   144,   145,   146,   147,    -1,    -1,    -1,
      39,    40,    95,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    -1,    -1,    -1,    55,    56,    -1,    -1,
      -1,    -1,     5,   116,    39,    40,    -1,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,   130,   131,    -1,
      55,    56,    -1,    -1,    -1,    -1,   139,    -1,    -1,    -1,
      -1,   144,   145,   146,   147,    -1,    39,    40,    -1,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    -1,
      -1,    -1,    55,    56,    -1,    -1,    -1,   116,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,   116,    -1,   142,    -1,   144,   145,   146,   147,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,    -1,     7,    -1,   139,    -1,    -1,    -1,    -1,   144,
     145,   146,   147,   116,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,    33,    34,
      35,    36,    37,    38,    39,    40,   139,    -1,    43,    -1,
      -1,   144,   145,   146,   147,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,    -1,    -1,     7,    -1,    -1,    -1,    74,
      75,    76,    77,    78,    -1,    -1,    -1,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    -1,    -1,    -1,
      95,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      42,    -1,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   130,   131,     3,     4,    -1,
      -1,     7,    74,    75,    76,    77,    78,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    95,    -1,    -1,    -1,    33,    34,    35,
      36,    37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,   131,
       3,     4,    -1,    -1,     7,    -1,    -1,    -1,    74,    75,
      76,    77,    78,    -1,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    -1,    -1,    95,
      33,    34,    35,    36,    37,    38,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   130,   131,     3,     4,    -1,    -1,
       7,    74,    75,    76,    77,    78,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    95,    -1,    -1,    -1,    33,    34,    35,    36,
      37,    38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    74,    75,    76,
      77,    78,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   130,   131
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    81,   163,   165,    47,     0,   167,    42,    43,    44,
      85,   168,    82,   164,   169,    85,   168,     3,     4,     5,
       7,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    33,    34,    35,    36,    37,    38,    39,    40,    43,
      74,    75,    76,    77,    78,    80,    88,    89,    90,    91,
      92,    93,    94,    95,   116,   130,   131,   159,   166,   170,
     200,   201,   202,   203,   204,   210,   211,   212,   213,   217,
     219,   220,   221,   222,   223,   225,   226,   227,   228,   229,
     260,   261,   262,   263,   264,   268,   269,   270,   271,   272,
      84,   160,   168,     7,    20,    21,    23,    42,    44,    74,
      75,   171,   213,   220,   221,   223,   171,   220,   228,    85,
      85,    85,    85,    85,    85,    85,   139,     5,   139,   169,
     260,   159,   160,   242,   140,   143,     4,    20,    21,    22,
      75,   206,   207,   208,   223,   228,   143,   159,    42,    44,
     168,   171,     7,    20,    21,    23,   220,   262,   268,   269,
     270,   271,   220,   220,   225,   220,   264,   220,   213,   220,
     262,   141,   224,   220,    44,   168,   212,   230,   231,   160,
     225,    38,   104,   137,   168,   214,   215,   216,   168,   218,
       6,     8,     9,    11,    12,    13,    14,    15,    16,    41,
      45,    46,    47,    48,    49,    50,    51,    55,    56,   139,
     144,   145,   146,   147,   159,   160,   161,   171,   172,   173,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   198,   200,   201,   225,   236,   237,   238,   239,
     243,   244,   245,   248,   254,   258,   259,   206,   207,   207,
     205,   209,   213,   225,   207,   207,   207,   168,   158,   224,
     139,   159,   159,   159,   159,   142,   182,   195,   199,   225,
     141,   160,    85,   168,   232,   233,   161,   231,   230,   159,
     158,   140,   143,   140,   143,   159,   159,   237,   139,   139,
     159,   159,   159,   198,   139,   139,   182,   182,   198,   161,
     240,    55,    56,    96,   141,   140,   140,   143,    40,   196,
     139,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,   158,   197,   182,   148,   149,   150,   144,   145,    53,
      54,    57,    58,   151,   152,    59,    60,   153,   154,   155,
      61,    63,    62,   156,   143,   159,   161,   169,   237,   225,
     168,   158,   224,   160,   196,   234,   158,   142,   142,   199,
     212,   266,   267,   224,   143,   159,   161,   199,   215,   168,
      41,   236,   244,   255,   198,   159,   198,   198,   212,   247,
     140,   243,    52,   174,   198,   196,   196,   182,   182,   182,
     184,   184,   185,   185,   186,   186,   186,   186,   187,   187,
     188,   189,   190,   191,   192,   193,   198,   196,   168,   224,
     234,   158,   234,   235,   234,   142,   232,   161,   266,   233,
     139,   247,   256,   257,   140,   140,   168,   140,   161,   142,
     157,   234,   143,   161,   159,    44,   265,   198,   159,   140,
     237,   246,   160,   249,   158,   238,   241,   242,   196,   161,
     234,   224,   159,   140,   198,   241,    10,    17,    18,   161,
     250,   251,   252,   253,   234,   159,   237,   198,   157,   237,
     250,   237,   161,   252,   157
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
# define YYLEX yylex (&yylval, &yylloc, state)
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
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
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
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , state);
      fprintf (stderr, "\n");
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
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

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
  yylloc.first_column = yylloc.last_column = 0;
#endif


  /* User initialization code.  */
#line 93 "src/compiler/glsl/glsl_parser.yy"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
   yylloc.path = NULL;
}
/* Line 1078 of yacc.c.  */
#line 2542 "src/compiler/glsl/glsl_parser.cpp"
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
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
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
#line 299 "src/compiler/glsl/glsl_parser.yy"
    {
      _mesa_glsl_initialize_types(state);
   ;}
    break;

  case 3:
#line 303 "src/compiler/glsl/glsl_parser.yy"
    {
      delete state->symbols;
      state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
      if (state->es_shader) {
         if (state->stage == MESA_SHADER_FRAGMENT) {
            state->symbols->add_default_precision_qualifier("int", ast_precision_medium);
         } else {
            state->symbols->add_default_precision_qualifier("float", ast_precision_high);
            state->symbols->add_default_precision_qualifier("int", ast_precision_high);
         }
         /* glsl-optimizer change: lowp and mediump have the same output policy. */
         state->symbols->add_default_precision_qualifier("sampler2D", ast_precision_medium);
         state->symbols->add_default_precision_qualifier("samplerExternalOES", ast_precision_medium);
         state->symbols->add_default_precision_qualifier("samplerCube", ast_precision_medium);
         state->symbols->add_default_precision_qualifier("atomic_uint", ast_precision_high);
      }
      _mesa_glsl_initialize_types(state);
   ;}
    break;

  case 5:
#line 326 "src/compiler/glsl/glsl_parser.yy"
    {
      state->process_version_directive(&(yylsp[(2) - (3)]), (yyvsp[(2) - (3)].n), NULL);
      if (state->error) {
         YYERROR;
      }
   ;}
    break;

  case 6:
#line 333 "src/compiler/glsl/glsl_parser.yy"
    {
      state->process_version_directive(&(yylsp[(2) - (4)]), (yyvsp[(2) - (4)].n), (yyvsp[(3) - (4)].identifier));
      if (state->error) {
         YYERROR;
      }
   ;}
    break;

  case 7:
#line 342 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 8:
#line 343 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 9:
#line 344 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 10:
#line 345 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 11:
#line 347 "src/compiler/glsl/glsl_parser.yy"
    {
      /* Pragma invariant(all) cannot be used in a fragment shader.
       *
       * Page 27 of the GLSL 1.20 spec, Page 53 of the GLSL ES 3.00 spec:
       *
       *     "It is an error to use this pragma in a fragment shader."
       */
      if (state->is_version(120, 300) &&
          state->stage == MESA_SHADER_FRAGMENT) {
         _mesa_glsl_error(& (yylsp[(1) - (2)]), state,
                          "pragma `invariant(all)' cannot be used "
                          "in a fragment shader.");
      } else if (!state->is_version(120, 100)) {
         _mesa_glsl_warning(& (yylsp[(1) - (2)]), state,
                            "pragma `invariant(all)' not supported in %s "
                            "(GLSL ES 1.00 or GLSL 1.20 required)",
                            state->get_version_string());
      } else {
         state->all_invariant = true;
      }

      (yyval.node) = NULL;
   ;}
    break;

  case 12:
#line 371 "src/compiler/glsl/glsl_parser.yy"
    {
      void *mem_ctx = state->linalloc;
      (yyval.node) = new(mem_ctx) ast_warnings_toggle(true);
   ;}
    break;

  case 13:
#line 376 "src/compiler/glsl/glsl_parser.yy"
    {
      void *mem_ctx = state->linalloc;
      (yyval.node) = new(mem_ctx) ast_warnings_toggle(false);
   ;}
    break;

  case 19:
#line 395 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 20:
#line 404 "src/compiler/glsl/glsl_parser.yy"
    {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[(1) - (1)].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
   ;}
    break;

  case 21:
#line 412 "src/compiler/glsl/glsl_parser.yy"
    {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[(2) - (2)].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 22:
#line 419 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!state->allow_extension_directive_midshader) {
         _mesa_glsl_error(& (yylsp[(2) - (2)]), state,
                          "#extension directive is not allowed "
                          "in the middle of a shader");
         YYERROR;
      }
   ;}
    break;

  case 25:
#line 436 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
   ;}
    break;

  case 26:
#line 443 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 27:
#line 450 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 28:
#line 457 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_int64_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.int64_constant = (yyvsp[(1) - (1)].n64);
   ;}
    break;

  case 29:
#line 464 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_uint64_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.uint64_constant = (yyvsp[(1) - (1)].n64);
   ;}
    break;

  case 30:
#line 471 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
   ;}
    break;

  case 31:
#line 478 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_double_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.double_constant = (yyvsp[(1) - (1)].dreal);
   ;}
    break;

  case 32:
#line 485 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 33:
#line 492 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (3)].expression);
   ;}
    break;

  case 35:
#line 500 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (4)]), (yylsp[(4) - (4)]));
   ;}
    break;

  case 36:
#line 506 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 37:
#line 510 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
   ;}
    break;

  case 38:
#line 517 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 39:
#line 523 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 47:
#line 554 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (2)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
   ;}
    break;

  case 48:
#line 560 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (3)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (3)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 50:
#line 576 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      ;}
    break;

  case 51:
#line 582 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].expression));
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      ;}
    break;

  case 53:
#line 597 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 54:
#line 603 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 55:
#line 609 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 56:
#line 618 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_plus; ;}
    break;

  case 57:
#line 619 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_neg; ;}
    break;

  case 58:
#line 620 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_logic_not; ;}
    break;

  case 59:
#line 621 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_bit_not; ;}
    break;

  case 61:
#line 627 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 62:
#line 633 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 63:
#line 639 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 65:
#line 649 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 66:
#line 655 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 68:
#line 665 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 69:
#line 671 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 71:
#line 681 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 72:
#line 687 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 73:
#line 693 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 74:
#line 699 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 76:
#line 709 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 77:
#line 715 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 79:
#line 725 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 81:
#line 735 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 83:
#line 745 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 85:
#line 755 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 87:
#line 765 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 89:
#line 775 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 91:
#line 785 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 93:
#line 795 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 94:
#line 803 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_assign; ;}
    break;

  case 95:
#line 804 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mul_assign; ;}
    break;

  case 96:
#line 805 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_div_assign; ;}
    break;

  case 97:
#line 806 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mod_assign; ;}
    break;

  case 98:
#line 807 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_add_assign; ;}
    break;

  case 99:
#line 808 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_sub_assign; ;}
    break;

  case 100:
#line 809 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_ls_assign; ;}
    break;

  case 101:
#line 810 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_rs_assign; ;}
    break;

  case 102:
#line 811 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_and_assign; ;}
    break;

  case 103:
#line 812 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_xor_assign; ;}
    break;

  case 104:
#line 813 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.n) = ast_or_assign; ;}
    break;

  case 105:
#line 818 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 106:
#line 822 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      if ((yyvsp[(1) - (3)].expression)->oper != ast_sequence) {
         (yyval.expression) = new(ctx) ast_expression(ast_sequence, NULL, NULL, NULL);
         (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
         (yyval.expression)->expressions.push_tail(& (yyvsp[(1) - (3)].expression)->link);
      } else {
         (yyval.expression) = (yyvsp[(1) - (3)].expression);
      }

      (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 108:
#line 842 "src/compiler/glsl/glsl_parser.yy"
    {
      state->symbols->pop_scope();
      (yyval.node) = (yyvsp[(1) - (2)].function);
   ;}
    break;

  case 109:
#line 847 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
   ;}
    break;

  case 110:
#line 851 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyvsp[(3) - (4)].type_specifier)->default_precision = (yyvsp[(2) - (4)].n);
      (yyval.node) = (yyvsp[(3) - (4)].type_specifier);
   ;}
    break;

  case 111:
#line 856 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_interface_block *block = (ast_interface_block *) (yyvsp[(1) - (1)].node);
      if (block->layout.has_layout() || block->layout.has_memory()) {
         if (!block->default_layout.merge_qualifier(& (yylsp[(1) - (1)]), state, block->layout, false)) {
            YYERROR;
         }
      }
      block->layout = block->default_layout;
      if (!block->layout.push_to_global(& (yylsp[(1) - (1)]), state)) {
         YYERROR;
      }
      (yyval.node) = (yyvsp[(1) - (1)].node);
   ;}
    break;

  case 115:
#line 882 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.function) = (yyvsp[(1) - (2)].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
   ;}
    break;

  case 116:
#line 887 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.function) = (yyvsp[(1) - (3)].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
   ;}
    break;

  case 117:
#line 895 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.function) = new(ctx) ast_function();
      (yyval.function)->set_location((yylsp[(2) - (3)]));
      (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
      (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);

      if ((yyvsp[(1) - (3)].fully_specified_type)->qualifier.is_subroutine_decl()) {
         /* add type for IDENTIFIER search */
         state->symbols->add_type((yyvsp[(2) - (3)].identifier), glsl_type::get_subroutine_instance((yyvsp[(2) - (3)].identifier)));
      } else
         state->symbols->add_function(new(state) ir_function((yyvsp[(2) - (3)].identifier)));
      state->symbols->push_scope();
   ;}
    break;

  case 118:
#line 913 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[(1) - (2)]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (2)].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (2)].identifier);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (2)].identifier), ir_var_auto));
   ;}
    break;

  case 119:
#line 924 "src/compiler/glsl/glsl_parser.yy"
    {
      _mesa_glsl_error(&(yylsp[(1) - (3)]), state, "is is not allowed on function parameter");
      YYERROR;
   ;}
    break;

  case 120:
#line 929 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[(1) - (3)]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (3)].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (3)].identifier);
      (yyval.parameter_declarator)->array_specifier = (yyvsp[(3) - (3)].array_specifier);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (3)].identifier), ir_var_auto));
   ;}
    break;

  case 121:
#line 944 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.parameter_declarator)->type->qualifier.push_to_global(& (yylsp[(1) - (2)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 122:
#line 952 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location((yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.parameter_declarator)->type->qualifier.push_to_global(& (yylsp[(1) - (2)]), state)) {
         YYERROR;
      }
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
   ;}
    break;

  case 123:
#line 968 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
   ;}
    break;

  case 124:
#line 972 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate const qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.constant = 1;
   ;}
    break;

  case 125:
#line 980 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precise qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 126:
#line 988 "src/compiler/glsl/glsl_parser.yy"
    {
      if (((yyvsp[(1) - (2)].type_qualifier).flags.q.in || (yyvsp[(1) - (2)].type_qualifier).flags.q.out) && ((yyvsp[(2) - (2)].type_qualifier).flags.q.in || (yyvsp[(2) - (2)].type_qualifier).flags.q.out))
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate in/out/inout qualifier");

      if (!state->has_420pack_or_es31() && (yyvsp[(2) - (2)].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "in/out/inout must come after const "
                                      "or precise");

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 127:
#line 1000 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precision qualifier");

      if (!state->has_420pack_or_es31() &&
          (yyvsp[(2) - (2)].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[(1) - (2)].n);
   ;}
    break;

  case 128:
#line 1012 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 129:
#line 1019 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 130:
#line 1024 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 131:
#line 1029 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 134:
#line 1043 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), NULL, NULL);
      decl->set_location((yylsp[(3) - (3)]));

      (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (3)].identifier), ir_var_auto));
   ;}
    break;

  case 135:
#line 1053 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (4)].identifier), (yyvsp[(4) - (4)].array_specifier), NULL);
      decl->set_location_range((yylsp[(3) - (4)]), (yylsp[(4) - (4)]));

      (yyval.declarator_list) = (yyvsp[(1) - (4)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (4)].identifier), ir_var_auto));
   ;}
    break;

  case 136:
#line 1063 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), (yyvsp[(4) - (6)].array_specifier), (yyvsp[(6) - (6)].expression));
      decl->set_location_range((yylsp[(3) - (6)]), (yylsp[(4) - (6)]));

      (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (6)].identifier), ir_var_auto));
   ;}
    break;

  case 137:
#line 1073 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), NULL, (yyvsp[(5) - (5)].expression));
      decl->set_location((yylsp[(3) - (5)]));

      (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto));
   ;}
    break;

  case 138:
#line 1087 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      /* Empty declaration list is valid. */
      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (1)].fully_specified_type));
      (yyval.declarator_list)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 139:
#line 1094 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (2)].identifier), ir_var_auto));
   ;}
    break;

  case 140:
#line 1105 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (3)].identifier), (yyvsp[(3) - (3)].array_specifier), NULL);
      decl->set_location_range((yylsp[(2) - (3)]), (yylsp[(3) - (3)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (3)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (3)].identifier), ir_var_auto));
   ;}
    break;

  case 141:
#line 1116 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), (yyvsp[(3) - (5)].array_specifier), (yyvsp[(5) - (5)].expression));
      decl->set_location_range((yylsp[(2) - (5)]), (yylsp[(3) - (5)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (5)]), (yylsp[(3) - (5)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (5)].identifier), ir_var_auto));
   ;}
    break;

  case 142:
#line 1127 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), NULL, (yyvsp[(4) - (4)].expression));
      decl->set_location((yylsp[(2) - (4)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (4)]), (yylsp[(2) - (4)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(2) - (4)].identifier), ir_var_auto));
   ;}
    break;

  case 143:
#line 1138 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->invariant = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 144:
#line 1150 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->precise = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 145:
#line 1165 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location((yylsp[(1) - (1)]));
      (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
   ;}
    break;

  case 146:
#line 1172 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.fully_specified_type)->qualifier.push_to_global(& (yylsp[(1) - (2)]), state)) {
         YYERROR;
      }
      (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
      if ((yyval.fully_specified_type)->specifier->structure != NULL &&
          (yyval.fully_specified_type)->specifier->structure->is_declaration) {
            (yyval.fully_specified_type)->specifier->structure->layout = &(yyval.fully_specified_type)->qualifier;
      }
   ;}
    break;

  case 147:
#line 1190 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
   ;}
    break;

  case 149:
#line 1198 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (3)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(3) - (3)]), state, (yyvsp[(3) - (3)].type_qualifier), true)) {
         YYERROR;
      }
   ;}
    break;

  case 150:
#line 1208 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

      /* Layout qualifiers for ARB_fragment_coord_conventions. */
      if (!(yyval.type_qualifier).flags.i && (state->ARB_fragment_coord_conventions_enable ||
                          state->is_version(150, 0))) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "origin_upper_left", state) == 0) {
            (yyval.type_qualifier).flags.q.origin_upper_left = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "pixel_center_integer",
                                           state) == 0) {
            (yyval.type_qualifier).flags.q.pixel_center_integer = 1;
         }

         if ((yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                               "GL_ARB_fragment_coord_conventions layout "
                               "identifier `%s' used", (yyvsp[(1) - (1)].identifier));
         }
      }

      /* Layout qualifiers for AMD/ARB_conservative_depth. */
      if (!(yyval.type_qualifier).flags.i &&
          (state->AMD_conservative_depth_enable ||
           state->ARB_conservative_depth_enable ||
           state->is_version(420, 0))) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_any", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_any;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_greater", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_greater;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_less", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_less;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_unchanged",
                                           state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_unchanged;
         }

         if ((yyval.type_qualifier).flags.i && state->AMD_conservative_depth_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                               "GL_AMD_conservative_depth "
                               "layout qualifier `%s' is used", (yyvsp[(1) - (1)].identifier));
         }
         if ((yyval.type_qualifier).flags.i && state->ARB_conservative_depth_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                               "GL_ARB_conservative_depth "
                               "layout qualifier `%s' is used", (yyvsp[(1) - (1)].identifier));
         }
      }

      /* See also interface_block_layout_qualifier. */
      if (!(yyval.type_qualifier).flags.i && state->has_uniform_buffer_objects()) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "std140", state) == 0) {
            (yyval.type_qualifier).flags.q.std140 = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "shared", state) == 0) {
            (yyval.type_qualifier).flags.q.shared = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "std430", state) == 0) {
            (yyval.type_qualifier).flags.q.std430 = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "column_major", state) == 0) {
            (yyval.type_qualifier).flags.q.column_major = 1;
         /* "row_major" is a reserved word in GLSL 1.30+. Its token is parsed
          * below in the interface_block_layout_qualifier rule.
          *
          * It is not a reserved word in GLSL ES 3.00, so it's handled here as
          * an identifier.
          *
          * Also, this takes care of alternate capitalizations of
          * "row_major" (which is necessary because layout qualifiers
          * are case-insensitive in desktop GLSL).
          */
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "row_major", state) == 0) {
            (yyval.type_qualifier).flags.q.row_major = 1;
         /* "packed" is a reserved word in GLSL, and its token is
          * parsed below in the interface_block_layout_qualifier rule.
          * However, we must take care of alternate capitalizations of
          * "packed", because layout qualifiers are case-insensitive
          * in desktop GLSL.
          */
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "packed", state) == 0) {
           (yyval.type_qualifier).flags.q.packed = 1;
         }

         if ((yyval.type_qualifier).flags.i && state->ARB_uniform_buffer_object_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                               "#version 140 / GL_ARB_uniform_buffer_object "
                               "layout qualifier `%s' is used", (yyvsp[(1) - (1)].identifier));
         }
      }

      /* Layout qualifiers for GLSL 1.50 geometry shaders. */
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            GLenum e;
         } map[] = {
                 { "points", GL_POINTS },
                 { "lines", GL_LINES },
                 { "lines_adjacency", GL_LINES_ADJACENCY },
                 { "line_strip", GL_LINE_STRIP },
                 { "triangles", GL_TRIANGLES },
                 { "triangles_adjacency", GL_TRIANGLES_ADJACENCY },
                 { "triangle_strip", GL_TRIANGLE_STRIP },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.prim_type = 1;
               (yyval.type_qualifier).prim_type = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_geometry_shader() &&
             !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "#version 150 layout "
                             "qualifier `%s' used", (yyvsp[(1) - (1)].identifier));
         }
      }

      /* Layout qualifiers for ARB_shader_image_load_store. */
      if (state->has_shader_image_load_store()) {
         if (!(yyval.type_qualifier).flags.i) {
            static const struct {
               const char *name;
               enum pipe_format format;
               glsl_base_type base_type;
               /** Minimum desktop GLSL version required for the image
                * format.  Use 130 if already present in the original
                * ARB extension.
                */
               unsigned required_glsl;
               /** Minimum GLSL ES version required for the image format. */
               unsigned required_essl;
               /* NV_image_formats */
               bool nv_image_formats;
               bool ext_qualifiers;
            } map[] = {
               { "rgba32f", PIPE_FORMAT_R32G32B32A32_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false },
               { "rgba16f", PIPE_FORMAT_R16G16B16A16_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false },
               { "rg32f", PIPE_FORMAT_R32G32_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rg16f", PIPE_FORMAT_R16G16_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r11f_g11f_b10f", PIPE_FORMAT_R11G11B10_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r32f", PIPE_FORMAT_R32_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false },
               { "r16f", PIPE_FORMAT_R16_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rgba32ui", PIPE_FORMAT_R32G32B32A32_UINT, GLSL_TYPE_UINT, 130, 310, false, false },
               { "rgba16ui", PIPE_FORMAT_R16G16B16A16_UINT, GLSL_TYPE_UINT, 130, 310, false, false },
               { "rgb10_a2ui", PIPE_FORMAT_R10G10B10A2_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "rgba8ui", PIPE_FORMAT_R8G8B8A8_UINT, GLSL_TYPE_UINT, 130, 310, false, false },
               { "rg32ui", PIPE_FORMAT_R32G32_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "rg16ui", PIPE_FORMAT_R16G16_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "rg8ui", PIPE_FORMAT_R8G8_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "r32ui", PIPE_FORMAT_R32_UINT, GLSL_TYPE_UINT, 130, 310, false, false },
               { "r16ui", PIPE_FORMAT_R16_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "r8ui", PIPE_FORMAT_R8_UINT, GLSL_TYPE_UINT, 130, 0, true, false },
               { "rgba32i", PIPE_FORMAT_R32G32B32A32_SINT, GLSL_TYPE_INT, 130, 310, false, false },
               { "rgba16i", PIPE_FORMAT_R16G16B16A16_SINT, GLSL_TYPE_INT, 130, 310, false, false },
               { "rgba8i", PIPE_FORMAT_R8G8B8A8_SINT, GLSL_TYPE_INT, 130, 310, false, false },
               { "rg32i", PIPE_FORMAT_R32G32_SINT, GLSL_TYPE_INT, 130, 0, true, false },
               { "rg16i", PIPE_FORMAT_R16G16_SINT, GLSL_TYPE_INT, 130, 0, true, false },
               { "rg8i", PIPE_FORMAT_R8G8_SINT, GLSL_TYPE_INT, 130, 0, true, false },
               { "r32i", PIPE_FORMAT_R32_SINT, GLSL_TYPE_INT, 130, 310, false, false },
               { "r16i", PIPE_FORMAT_R16_SINT, GLSL_TYPE_INT, 130, 0, true, false },
               { "r8i", PIPE_FORMAT_R8_SINT, GLSL_TYPE_INT, 130, 0, true, false },
               { "rgba16", PIPE_FORMAT_R16G16B16A16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rgb10_a2", PIPE_FORMAT_R10G10B10A2_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rgba8", PIPE_FORMAT_R8G8B8A8_UNORM, GLSL_TYPE_FLOAT, 130, 310, false, false },
               { "rg16", PIPE_FORMAT_R16G16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rg8", PIPE_FORMAT_R8G8_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r16", PIPE_FORMAT_R16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r8", PIPE_FORMAT_R8_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rgba16_snorm", PIPE_FORMAT_R16G16B16A16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rgba8_snorm", PIPE_FORMAT_R8G8B8A8_SNORM, GLSL_TYPE_FLOAT, 130, 310, false, false },
               { "rg16_snorm", PIPE_FORMAT_R16G16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "rg8_snorm", PIPE_FORMAT_R8G8_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r16_snorm", PIPE_FORMAT_R16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },
               { "r8_snorm", PIPE_FORMAT_R8_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false },

               /* From GL_EXT_shader_image_load_store: */
               /* base_type is incorrect but it'll be patched later when we know
                * the variable type. See ast_to_hir.cpp */
               { "size1x8", PIPE_FORMAT_R8_SINT, GLSL_TYPE_VOID, 130, 0, false, true },
               { "size1x16", PIPE_FORMAT_R16_SINT, GLSL_TYPE_VOID, 130, 0, false, true },
               { "size1x32", PIPE_FORMAT_R32_SINT, GLSL_TYPE_VOID, 130, 0, false, true },
               { "size2x32", PIPE_FORMAT_R32G32_SINT, GLSL_TYPE_VOID, 130, 0, false, true },
               { "size4x32", PIPE_FORMAT_R32G32B32A32_SINT, GLSL_TYPE_VOID, 130, 0, false, true },
            };

            for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
               if ((state->is_version(map[i].required_glsl,
                                      map[i].required_essl) ||
                    (state->NV_image_formats_enable &&
                     map[i].nv_image_formats)) &&
                   match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].name, state) == 0) {
                  /* Skip ARB_shader_image_load_store qualifiers if not enabled */
                  if (!map[i].ext_qualifiers && !(state->ARB_shader_image_load_store_enable ||
                                                  state->is_version(420, 310))) {
                     continue;
                  }
                  /* Skip EXT_shader_image_load_store qualifiers if not enabled */
                  if (map[i].ext_qualifiers && !state->EXT_shader_image_load_store_enable) {
                     continue;
                  }
                  (yyval.type_qualifier).flags.q.explicit_image_format = 1;
                  (yyval.type_qualifier).image_format = map[i].format;
                  (yyval.type_qualifier).image_base_type = map[i].base_type;
                  break;
               }
            }
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "early_fragment_tests", state) == 0) {
            /* From section 4.4.1.3 of the GLSL 4.50 specification
             * (Fragment Shader Inputs):
             *
             *  "Fragment shaders also allow the following layout
             *   qualifier on in only (not with variable declarations)
             *     layout-qualifier-id
             *        early_fragment_tests
             *   [...]"
             */
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "early_fragment_tests layout qualifier only "
                                "valid in fragment shaders");
            }

            (yyval.type_qualifier).flags.q.early_fragment_tests = 1;
         }

         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "inner_coverage", state) == 0) {
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "inner_coverage layout qualifier only "
                                "valid in fragment shaders");
            }

	    if (state->INTEL_conservative_rasterization_enable) {
	       (yyval.type_qualifier).flags.q.inner_coverage = 1;
	    } else {
	       _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "inner_coverage layout qualifier present, "
                                "but the INTEL_conservative_rasterization extension "
                                "is not enabled.");
            }
         }

         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "post_depth_coverage", state) == 0) {
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "post_depth_coverage layout qualifier only "
                                "valid in fragment shaders");
            }

            if (state->ARB_post_depth_coverage_enable ||
		state->INTEL_conservative_rasterization_enable) {
               (yyval.type_qualifier).flags.q.post_depth_coverage = 1;
            } else {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "post_depth_coverage layout qualifier present, "
                                "but the GL_ARB_post_depth_coverage extension "
                                "is not enabled.");
            }
         }

         if ((yyval.type_qualifier).flags.q.post_depth_coverage && (yyval.type_qualifier).flags.q.inner_coverage) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "post_depth_coverage & inner_coverage layout qualifiers "
                             "are mutually exclusive");
         }
      }

      const bool pixel_interlock_ordered = match_layout_qualifier((yyvsp[(1) - (1)].identifier),
         "pixel_interlock_ordered", state) == 0;
      const bool pixel_interlock_unordered = match_layout_qualifier((yyvsp[(1) - (1)].identifier),
         "pixel_interlock_unordered", state) == 0;
      const bool sample_interlock_ordered = match_layout_qualifier((yyvsp[(1) - (1)].identifier),
         "sample_interlock_ordered", state) == 0;
      const bool sample_interlock_unordered = match_layout_qualifier((yyvsp[(1) - (1)].identifier),
         "sample_interlock_unordered", state) == 0;

      if (pixel_interlock_ordered + pixel_interlock_unordered +
          sample_interlock_ordered + sample_interlock_unordered > 0 &&
          state->stage != MESA_SHADER_FRAGMENT) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "interlock layout qualifiers: "
                          "pixel_interlock_ordered, pixel_interlock_unordered, "
                          "sample_interlock_ordered and sample_interlock_unordered, "
                          "only valid in fragment shader input layout declaration.");
      } else if (pixel_interlock_ordered + pixel_interlock_unordered +
                 sample_interlock_ordered + sample_interlock_unordered > 0 &&
                 !state->ARB_fragment_shader_interlock_enable &&
                 !state->NV_fragment_shader_interlock_enable) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                          "interlock layout qualifier present, but the "
                          "GL_ARB_fragment_shader_interlock or "
                          "GL_NV_fragment_shader_interlock extension is not "
                          "enabled.");
      } else {
         (yyval.type_qualifier).flags.q.pixel_interlock_ordered = pixel_interlock_ordered;
         (yyval.type_qualifier).flags.q.pixel_interlock_unordered = pixel_interlock_unordered;
         (yyval.type_qualifier).flags.q.sample_interlock_ordered = sample_interlock_ordered;
         (yyval.type_qualifier).flags.q.sample_interlock_unordered = sample_interlock_unordered;
      }

      /* Layout qualifiers for tessellation evaluation shaders. */
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            GLenum e;
         } map[] = {
                 /* triangles already parsed by gs-specific code */
                 { "quads", GL_QUADS },
                 { "isolines", GL_ISOLINES },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.prim_type = 1;
               (yyval.type_qualifier).prim_type = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "primitive mode qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[(1) - (1)].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            enum gl_tess_spacing e;
         } map[] = {
                 { "equal_spacing", TESS_SPACING_EQUAL },
                 { "fractional_odd_spacing", TESS_SPACING_FRACTIONAL_ODD },
                 { "fractional_even_spacing", TESS_SPACING_FRACTIONAL_EVEN },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.vertex_spacing = 1;
               (yyval.type_qualifier).vertex_spacing = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "vertex spacing qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[(1) - (1)].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "cw", state) == 0) {
            (yyval.type_qualifier).flags.q.ordering = 1;
            (yyval.type_qualifier).ordering = GL_CW;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "ccw", state) == 0) {
            (yyval.type_qualifier).flags.q.ordering = 1;
            (yyval.type_qualifier).ordering = GL_CCW;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "ordering qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[(1) - (1)].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "point_mode", state) == 0) {
            (yyval.type_qualifier).flags.q.point_mode = 1;
            (yyval.type_qualifier).point_mode = true;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "qualifier `point_mode' requires "
                             "GLSL 4.00 or ARB_tessellation_shader");
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            uint32_t mask;
         } map[] = {
                 { "blend_support_multiply",       BITFIELD_BIT(BLEND_MULTIPLY) },
                 { "blend_support_screen",         BITFIELD_BIT(BLEND_SCREEN) },
                 { "blend_support_overlay",        BITFIELD_BIT(BLEND_OVERLAY) },
                 { "blend_support_darken",         BITFIELD_BIT(BLEND_DARKEN) },
                 { "blend_support_lighten",        BITFIELD_BIT(BLEND_LIGHTEN) },
                 { "blend_support_colordodge",     BITFIELD_BIT(BLEND_COLORDODGE) },
                 { "blend_support_colorburn",      BITFIELD_BIT(BLEND_COLORBURN) },
                 { "blend_support_hardlight",      BITFIELD_BIT(BLEND_HARDLIGHT) },
                 { "blend_support_softlight",      BITFIELD_BIT(BLEND_SOFTLIGHT) },
                 { "blend_support_difference",     BITFIELD_BIT(BLEND_DIFFERENCE) },
                 { "blend_support_exclusion",      BITFIELD_BIT(BLEND_EXCLUSION) },
                 { "blend_support_hsl_hue",        BITFIELD_BIT(BLEND_HSL_HUE) },
                 { "blend_support_hsl_saturation", BITFIELD_BIT(BLEND_HSL_SATURATION) },
                 { "blend_support_hsl_color",      BITFIELD_BIT(BLEND_HSL_COLOR) },
                 { "blend_support_hsl_luminosity", BITFIELD_BIT(BLEND_HSL_LUMINOSITY) },
                 { "blend_support_all_equations",  (1u << (BLEND_HSL_LUMINOSITY + 1)) - 2 },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.blend_support = 1;
               state->fs_blend_support |= map[i].mask;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i &&
             !state->KHR_blend_equation_advanced_enable &&
             !state->is_version(0, 320)) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "advanced blending layout qualifiers require "
                             "ESSL 3.20 or KHR_blend_equation_advanced");
         }

         if ((yyval.type_qualifier).flags.i && state->stage != MESA_SHADER_FRAGMENT) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "advanced blending layout qualifiers only "
                             "valid in fragment shaders");
         }
      }

      /* Layout qualifiers for ARB_compute_variable_group_size. */
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "local_size_variable", state) == 0) {
            (yyval.type_qualifier).flags.q.local_size_variable = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->ARB_compute_variable_group_size_enable) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "qualifier `local_size_variable` requires "
                             "ARB_compute_variable_group_size");
         }
      }

      /* Layout qualifiers for ARB_bindless_texture. */
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "bindless_sampler", state) == 0)
            (yyval.type_qualifier).flags.q.bindless_sampler = 1;
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "bound_sampler", state) == 0)
            (yyval.type_qualifier).flags.q.bound_sampler = 1;

         if (state->has_shader_image_load_store()) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "bindless_image", state) == 0)
               (yyval.type_qualifier).flags.q.bindless_image = 1;
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "bound_image", state) == 0)
               (yyval.type_qualifier).flags.q.bound_image = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_bindless()) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "qualifier `%s` requires "
                             "ARB_bindless_texture", (yyvsp[(1) - (1)].identifier));
         }
      }

      if (!(yyval.type_qualifier).flags.i &&
          state->EXT_shader_framebuffer_fetch_non_coherent_enable) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "noncoherent", state) == 0)
            (yyval.type_qualifier).flags.q.non_coherent = 1;
      }

      // Layout qualifiers for NV_compute_shader_derivatives.
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "derivative_group_quadsNV", state) == 0) {
            (yyval.type_qualifier).flags.q.derivative_group = 1;
            (yyval.type_qualifier).derivative_group = DERIVATIVE_GROUP_QUADS;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "derivative_group_linearNV", state) == 0) {
            (yyval.type_qualifier).flags.q.derivative_group = 1;
            (yyval.type_qualifier).derivative_group = DERIVATIVE_GROUP_LINEAR;
         }

         if ((yyval.type_qualifier).flags.i) {
            if (!state->has_compute_shader()) {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "qualifier `%s' requires "
                                "a compute shader", (yyvsp[(1) - (1)].identifier));
            }

            if (!state->NV_compute_shader_derivatives_enable) {
               _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                                "qualifier `%s' requires "
                                "NV_compute_shader_derivatives", (yyvsp[(1) - (1)].identifier));
            }

            if (state->NV_compute_shader_derivatives_warn) {
               _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                                  "NV_compute_shader_derivatives layout "
                                  "qualifier `%s' used", (yyvsp[(1) - (1)].identifier));
            }
         }
      }

      /* Layout qualifier for NV_viewport_array2. */
      if (!(yyval.type_qualifier).flags.i && state->stage != MESA_SHADER_FRAGMENT) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "viewport_relative", state) == 0) {
            (yyval.type_qualifier).flags.q.viewport_relative = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->NV_viewport_array2_enable) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                             "qualifier `%s' requires "
                             "GL_NV_viewport_array2", (yyvsp[(1) - (1)].identifier));
         }

         if ((yyval.type_qualifier).flags.i && state->NV_viewport_array2_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                               "GL_NV_viewport_array2 layout "
                               "identifier `%s' used", (yyvsp[(1) - (1)].identifier));
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "unrecognized layout identifier "
                          "`%s'", (yyvsp[(1) - (1)].identifier));
         YYERROR;
      }
   ;}
    break;

  case 151:
#line 1731 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      void *ctx = state->linalloc;

      if ((yyvsp[(3) - (3)].expression)->oper != ast_int_constant &&
          (yyvsp[(3) - (3)].expression)->oper != ast_uint_constant &&
          !state->has_enhanced_layouts()) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "compile-time constant expressions require "
                          "GLSL 4.40 or ARB_enhanced_layouts");
      }

      if (match_layout_qualifier("align", (yyvsp[(1) - (3)].identifier), state) == 0) {
         if (!state->has_enhanced_layouts()) {
            _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                             "align qualifier requires "
                             "GLSL 4.40 or ARB_enhanced_layouts");
         } else {
            (yyval.type_qualifier).flags.q.explicit_align = 1;
            (yyval.type_qualifier).align = (yyvsp[(3) - (3)].expression);
         }
      }

      if (match_layout_qualifier("location", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_location = 1;

         if ((yyval.type_qualifier).flags.q.attribute == 1 &&
             state->ARB_explicit_attrib_location_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (3)]), state,
                               "GL_ARB_explicit_attrib_location layout "
                               "identifier `%s' used", (yyvsp[(1) - (3)].identifier));
         }
         (yyval.type_qualifier).location = (yyvsp[(3) - (3)].expression);
      }

      if (match_layout_qualifier("component", (yyvsp[(1) - (3)].identifier), state) == 0) {
         if (!state->has_enhanced_layouts()) {
            _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                             "component qualifier requires "
                             "GLSL 4.40 or ARB_enhanced_layouts");
         } else {
            (yyval.type_qualifier).flags.q.explicit_component = 1;
            (yyval.type_qualifier).component = (yyvsp[(3) - (3)].expression);
         }
      }

      if (match_layout_qualifier("index", (yyvsp[(1) - (3)].identifier), state) == 0) {
         if (state->es_shader && !state->EXT_blend_func_extended_enable) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state, "index layout qualifier requires EXT_blend_func_extended");
            YYERROR;
         }

         (yyval.type_qualifier).flags.q.explicit_index = 1;
         (yyval.type_qualifier).index = (yyvsp[(3) - (3)].expression);
      }

      if ((state->has_420pack_or_es31() ||
           state->has_atomic_counters() ||
           state->has_shader_storage_buffer_objects()) &&
          match_layout_qualifier("binding", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_binding = 1;
         (yyval.type_qualifier).binding = (yyvsp[(3) - (3)].expression);
      }

      if ((state->has_atomic_counters() ||
           state->has_enhanced_layouts()) &&
          match_layout_qualifier("offset", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_offset = 1;
         (yyval.type_qualifier).offset = (yyvsp[(3) - (3)].expression);
      }

      if (match_layout_qualifier("max_vertices", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.max_vertices = 1;
         (yyval.type_qualifier).max_vertices = new(ctx) ast_layout_expression((yylsp[(1) - (3)]), (yyvsp[(3) - (3)].expression));
         if (!state->has_geometry_shader()) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                             "#version 150 max_vertices qualifier "
                             "specified", (yyvsp[(3) - (3)].expression));
         }
      }

      if (state->stage == MESA_SHADER_GEOMETRY) {
         if (match_layout_qualifier("stream", (yyvsp[(1) - (3)].identifier), state) == 0 &&
             state->check_explicit_attrib_stream_allowed(& (yylsp[(3) - (3)]))) {
            (yyval.type_qualifier).flags.q.stream = 1;
            (yyval.type_qualifier).flags.q.explicit_stream = 1;
            (yyval.type_qualifier).stream = (yyvsp[(3) - (3)].expression);
         }
      }

      if (state->has_enhanced_layouts()) {
         if (match_layout_qualifier("xfb_buffer", (yyvsp[(1) - (3)].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.xfb_buffer = 1;
            (yyval.type_qualifier).flags.q.explicit_xfb_buffer = 1;
            (yyval.type_qualifier).xfb_buffer = (yyvsp[(3) - (3)].expression);
         }

         if (match_layout_qualifier("xfb_offset", (yyvsp[(1) - (3)].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.explicit_xfb_offset = 1;
            (yyval.type_qualifier).offset = (yyvsp[(3) - (3)].expression);
         }

         if (match_layout_qualifier("xfb_stride", (yyvsp[(1) - (3)].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.xfb_stride = 1;
            (yyval.type_qualifier).flags.q.explicit_xfb_stride = 1;
            (yyval.type_qualifier).xfb_stride = (yyvsp[(3) - (3)].expression);
         }
      }

      static const char * const local_size_qualifiers[3] = {
         "local_size_x",
         "local_size_y",
         "local_size_z",
      };
      for (int i = 0; i < 3; i++) {
         if (match_layout_qualifier(local_size_qualifiers[i], (yyvsp[(1) - (3)].identifier),
                                    state) == 0) {
            if (!state->has_compute_shader()) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "%s qualifier requires GLSL 4.30 or "
                                "GLSL ES 3.10 or ARB_compute_shader",
                                local_size_qualifiers[i]);
               YYERROR;
            } else {
               (yyval.type_qualifier).flags.q.local_size |= (1 << i);
               (yyval.type_qualifier).local_size[i] = new(ctx) ast_layout_expression((yylsp[(1) - (3)]), (yyvsp[(3) - (3)].expression));
            }
            break;
         }
      }

      if (match_layout_qualifier("invocations", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.invocations = 1;
         (yyval.type_qualifier).invocations = new(ctx) ast_layout_expression((yylsp[(1) - (3)]), (yyvsp[(3) - (3)].expression));
         if (!state->is_version(400, 320) &&
             !state->ARB_gpu_shader5_enable &&
             !state->OES_geometry_shader_enable &&
             !state->EXT_geometry_shader_enable) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                             "GL_ARB_gpu_shader5 invocations "
                             "qualifier specified", (yyvsp[(3) - (3)].expression));
         }
      }

      /* Layout qualifiers for tessellation control shaders. */
      if (match_layout_qualifier("vertices", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.vertices = 1;
         (yyval.type_qualifier).vertices = new(ctx) ast_layout_expression((yylsp[(1) - (3)]), (yyvsp[(3) - (3)].expression));
         if (!state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                             "vertices qualifier requires GLSL 4.00 or "
                             "ARB_tessellation_shader");
         }
      }

      /* If the identifier didn't match any known layout identifiers,
       * emit an error.
       */
      if (!(yyval.type_qualifier).flags.i) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state, "unrecognized layout identifier "
                          "`%s'", (yyvsp[(1) - (3)].identifier));
         YYERROR;
      }
   ;}
    break;

  case 152:
#line 1896 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (1)].type_qualifier);
      /* Layout qualifiers for ARB_uniform_buffer_object. */
      if ((yyval.type_qualifier).flags.q.uniform && !state->has_uniform_buffer_objects()) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                          "#version 140 / GL_ARB_uniform_buffer_object "
                          "layout qualifier `%s' is used", (yyvsp[(1) - (1)].type_qualifier));
      } else if ((yyval.type_qualifier).flags.q.uniform && state->ARB_uniform_buffer_object_warn) {
         _mesa_glsl_warning(& (yylsp[(1) - (1)]), state,
                            "#version 140 / GL_ARB_uniform_buffer_object "
                            "layout qualifier `%s' is used", (yyvsp[(1) - (1)].type_qualifier));
      }
   ;}
    break;

  case 153:
#line 1922 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.row_major = 1;
   ;}
    break;

  case 154:
#line 1927 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.packed = 1;
   ;}
    break;

  case 155:
#line 1932 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.shared = 1;
   ;}
    break;

  case 156:
#line 1940 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.subroutine = 1;
   ;}
    break;

  case 157:
#line 1945 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.subroutine = 1;
      (yyval.type_qualifier).subroutine_list = (yyvsp[(3) - (4)].subroutine_list);
   ;}
    break;

  case 158:
#line 1954 "src/compiler/glsl/glsl_parser.yy"
    {
        void *ctx = state->linalloc;
        ast_declaration *decl = new(ctx)  ast_declaration((yyvsp[(1) - (1)].identifier), NULL, NULL);
        decl->set_location((yylsp[(1) - (1)]));

        (yyval.subroutine_list) = new(ctx) ast_subroutine_list();
        (yyval.subroutine_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 159:
#line 1963 "src/compiler/glsl/glsl_parser.yy"
    {
        void *ctx = state->linalloc;
        ast_declaration *decl = new(ctx)  ast_declaration((yyvsp[(3) - (3)].identifier), NULL, NULL);
        decl->set_location((yylsp[(3) - (3)]));

        (yyval.subroutine_list) = (yyvsp[(1) - (3)].subroutine_list);
        (yyval.subroutine_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 160:
#line 1975 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.smooth = 1;
   ;}
    break;

  case 161:
#line 1980 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.flat = 1;
   ;}
    break;

  case 162:
#line 1985 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.noperspective = 1;
   ;}
    break;

  case 163:
#line 1994 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.invariant = 1;
   ;}
    break;

  case 164:
#line 1999 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 171:
#line 2010 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(&(yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).precision = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 172:
#line 2028 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate \"precise\" qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 173:
#line 2036 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.invariant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate \"invariant\" qualifier");

      if (!state->has_420pack_or_es31() && (yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state,
                          "\"invariant\" must come after \"precise\"");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.invariant = 1;

      /* GLSL ES 3.00 spec, section 4.6.1 "The Invariant Qualifier":
       *
       * "Only variables output from a shader can be candidates for invariance.
       * This includes user-defined output variables and the built-in output
       * variables. As only outputs can be declared as invariant, an invariant
       * output from one shader stage will still match an input of a subsequent
       * stage without the input being declared as invariant."
       *
       * On the desktop side, this text first appears in GLSL 4.30.
       */
      if (state->is_version(430, 300) && (yyval.type_qualifier).flags.q.in)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "invariant qualifiers cannot be used with shader inputs");
   ;}
    break;

  case 174:
#line 2061 "src/compiler/glsl/glsl_parser.yy"
    {
      /* Section 4.3 of the GLSL 1.40 specification states:
       * "...qualified with one of these interpolation qualifiers"
       *
       * GLSL 1.30 claims to allow "one or more", but insists that:
       * "These interpolation qualifiers may only precede the qualifiers in,
       *  centroid in, out, or centroid out in a declaration."
       *
       * ...which means that e.g. smooth can't precede smooth, so there can be
       * only one after all, and the 1.40 text is a clarification, not a change.
       */
      if ((yyvsp[(2) - (2)].type_qualifier).has_interpolation())
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate interpolation qualifier");

      if (!state->has_420pack_or_es31() &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant)) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "interpolation qualifiers must come "
                          "after \"precise\" or \"invariant\"");
      }

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 175:
#line 2085 "src/compiler/glsl/glsl_parser.yy"
    {
      /* In the absence of ARB_shading_language_420pack, layout qualifiers may
       * appear no later than auxiliary storage qualifiers. There is no
       * particularly clear spec language mandating this, but in all examples
       * the layout qualifier precedes the storage qualifier.
       *
       * We allow combinations of layout with interpolation, invariant or
       * precise qualifiers since these are useful in ARB_separate_shader_objects.
       * There is no clear spec guidance on this either.
       */
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false, (yyvsp[(2) - (2)].type_qualifier).has_layout());
   ;}
    break;

  case 176:
#line 2099 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 177:
#line 2104 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).has_auxiliary_storage()) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state,
                          "duplicate auxiliary storage qualifier (centroid or sample)");
      }

      if ((!state->has_420pack_or_es31() && !state->EXT_gpu_shader4_enable) &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant ||
           (yyvsp[(2) - (2)].type_qualifier).has_interpolation() || (yyvsp[(2) - (2)].type_qualifier).has_layout())) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "auxiliary storage qualifiers must come "
                          "just before storage qualifiers");
      }
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 178:
#line 2120 "src/compiler/glsl/glsl_parser.yy"
    {
      /* Section 4.3 of the GLSL 1.20 specification states:
       * "Variable declarations may have a storage qualifier specified..."
       *  1.30 clarifies this to "may have one storage qualifier".
       *
       * GL_EXT_gpu_shader4 allows "varying out" in fragment shaders.
       */
      if ((yyvsp[(2) - (2)].type_qualifier).has_storage() &&
          (!state->EXT_gpu_shader4_enable ||
           state->stage != MESA_SHADER_FRAGMENT ||
           !(yyvsp[(1) - (2)].type_qualifier).flags.q.varying || !(yyvsp[(2) - (2)].type_qualifier).flags.q.out))
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate storage qualifier");

      if (!state->has_420pack_or_es31() &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant || (yyvsp[(2) - (2)].type_qualifier).has_interpolation() ||
           (yyvsp[(2) - (2)].type_qualifier).has_layout() || (yyvsp[(2) - (2)].type_qualifier).has_auxiliary_storage())) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "storage qualifiers must come after "
                          "precise, invariant, interpolation, layout and auxiliary "
                          "storage qualifiers");
      }

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 179:
#line 2145 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precision qualifier");

      if (!(state->has_420pack_or_es31()) &&
          (yyvsp[(2) - (2)].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[(1) - (2)].n);
   ;}
    break;

  case 180:
#line 2157 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false);
   ;}
    break;

  case 181:
#line 2165 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.centroid = 1;
   ;}
    break;

  case 182:
#line 2170 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.sample = 1;
   ;}
    break;

  case 183:
#line 2175 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.patch = 1;
   ;}
    break;

  case 184:
#line 2182 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.constant = 1;
   ;}
    break;

  case 185:
#line 2187 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.attribute = 1;
   ;}
    break;

  case 186:
#line 2192 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.varying = 1;
   ;}
    break;

  case 187:
#line 2197 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 188:
#line 2202 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;

      if (state->stage == MESA_SHADER_GEOMETRY &&
          state->has_explicit_attrib_stream()) {
         /* Section 4.3.8.2 (Output Layout Qualifiers) of the GLSL 4.00
          * spec says:
          *
          *     "If the block or variable is declared with the stream
          *     identifier, it is associated with the specified stream;
          *     otherwise, it is associated with the current default stream."
          */
          (yyval.type_qualifier).flags.q.stream = 1;
          (yyval.type_qualifier).flags.q.explicit_stream = 0;
          (yyval.type_qualifier).stream = state->out_qualifier->stream;
      }

      if (state->has_enhanced_layouts()) {
          (yyval.type_qualifier).flags.q.xfb_buffer = 1;
          (yyval.type_qualifier).flags.q.explicit_xfb_buffer = 0;
          (yyval.type_qualifier).xfb_buffer = state->out_qualifier->xfb_buffer;
      }
   ;}
    break;

  case 189:
#line 2227 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
      (yyval.type_qualifier).flags.q.out = 1;

      if (!state->has_framebuffer_fetch() ||
          !state->is_version(130, 300) ||
          state->stage != MESA_SHADER_FRAGMENT)
         _mesa_glsl_error(&(yylsp[(1) - (1)]), state, "A single interface variable cannot be "
                          "declared as both input and output");
   ;}
    break;

  case 190:
#line 2239 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.uniform = 1;
   ;}
    break;

  case 191:
#line 2244 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.buffer = 1;
   ;}
    break;

  case 192:
#line 2249 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.shared_storage = 1;
   ;}
    break;

  case 193:
#line 2257 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.coherent = 1;
   ;}
    break;

  case 194:
#line 2262 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q._volatile = 1;
   ;}
    break;

  case 195:
#line 2267 "src/compiler/glsl/glsl_parser.yy"
    {
      STATIC_ASSERT(sizeof((yyval.type_qualifier).flags.q) <= sizeof((yyval.type_qualifier).flags.i));
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.restrict_flag = 1;
   ;}
    break;

  case 196:
#line 2273 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.read_only = 1;
   ;}
    break;

  case 197:
#line 2278 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.write_only = 1;
   ;}
    break;

  case 198:
#line 2286 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[(1) - (2)]), new(ctx) ast_expression(
                                                  ast_unsized_array_dim, NULL,
                                                  NULL, NULL));
      (yyval.array_specifier)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 199:
#line 2294 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[(1) - (3)]), (yyvsp[(2) - (3)].expression));
      (yyval.array_specifier)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 200:
#line 2300 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.array_specifier) = (yyvsp[(1) - (3)].array_specifier);

      if (state->check_arrays_of_arrays_allowed(& (yylsp[(1) - (3)]))) {
         (yyval.array_specifier)->add_dimension(new(ctx) ast_expression(ast_unsized_array_dim, NULL,
                                                   NULL, NULL));
      }
   ;}
    break;

  case 201:
#line 2310 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.array_specifier) = (yyvsp[(1) - (4)].array_specifier);

      if (state->check_arrays_of_arrays_allowed(& (yylsp[(1) - (4)]))) {
         (yyval.array_specifier)->add_dimension((yyvsp[(3) - (4)].expression));
      }
   ;}
    break;

  case 203:
#line 2322 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_specifier) = (yyvsp[(1) - (2)].type_specifier);
      (yyval.type_specifier)->array_specifier = (yyvsp[(2) - (2)].array_specifier);
   ;}
    break;

  case 204:
#line 2330 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].type));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 205:
#line 2336 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 206:
#line 2342 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 207:
#line 2350 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.type) = glsl_type::void_type; ;}
    break;

  case 208:
#line 2351 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.type) = (yyvsp[(1) - (1)].type); ;}
    break;

  case 209:
#line 2353 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type) == glsl_type::int_type) {
         (yyval.type) = glsl_type::uint_type;
      } else {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state,
                          "\"unsigned\" is only allowed before \"int\"");
      }
   ;}
    break;

  case 210:
#line 2365 "src/compiler/glsl/glsl_parser.yy"
    {
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_high;
   ;}
    break;

  case 211:
#line 2370 "src/compiler/glsl/glsl_parser.yy"
    {
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_medium;
   ;}
    break;

  case 212:
#line 2375 "src/compiler/glsl/glsl_parser.yy"
    {
      /* glsl-optimizer change: normalize before storing any precision metadata. */
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_medium;
   ;}
    break;

  case 213:
#line 2384 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].declarator_list));
      (yyval.struct_specifier)->set_location_range((yylsp[(2) - (5)]), (yylsp[(5) - (5)]));
      state->symbols->add_type((yyvsp[(2) - (5)].identifier), glsl_type::void_type);
   ;}
    break;

  case 214:
#line 2391 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;

      /* All anonymous structs have the same name. This simplifies matching of
       * globals whose type is an unnamed struct.
       *
       * It also avoids a memory leak when the same shader is compiled over and
       * over again.
       */
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier("#anon_struct", (yyvsp[(3) - (4)].declarator_list));

      (yyval.struct_specifier)->set_location_range((yylsp[(2) - (4)]), (yylsp[(4) - (4)]));
   ;}
    break;

  case 215:
#line 2408 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
      (yyvsp[(1) - (1)].declarator_list)->link.self_link();
   ;}
    break;

  case 216:
#line 2413 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
      (yyval.declarator_list)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
   ;}
    break;

  case 217:
#line 2421 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_fully_specified_type *const type = (yyvsp[(1) - (3)].fully_specified_type);
      type->set_location((yylsp[(1) - (3)]));

      if (state->has_bindless()) {
         ast_type_qualifier input_layout_mask;

         /* Allow to declare qualifiers for images. */
         input_layout_mask.flags.i = 0;
         input_layout_mask.flags.q.coherent = 1;
         input_layout_mask.flags.q._volatile = 1;
         input_layout_mask.flags.q.restrict_flag = 1;
         input_layout_mask.flags.q.read_only = 1;
         input_layout_mask.flags.q.write_only = 1;
         input_layout_mask.flags.q.explicit_image_format = 1;

         if ((type->qualifier.flags.i & ~input_layout_mask.flags.i) != 0) {
            _mesa_glsl_error(&(yylsp[(1) - (3)]), state,
                             "only precision and image qualifiers may be "
                             "applied to structure members");
         }
      } else {
         if (type->qualifier.flags.i != 0)
            _mesa_glsl_error(&(yylsp[(1) - (3)]), state,
                             "only precision qualifiers may be applied to "
                             "structure members");
      }

      (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
      (yyval.declarator_list)->set_location((yylsp[(2) - (3)]));

      (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(2) - (3)].declaration)->link);
   ;}
    break;

  case 218:
#line 2459 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
      (yyvsp[(1) - (1)].declaration)->link.self_link();
   ;}
    break;

  case 219:
#line 2464 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
      (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
   ;}
    break;

  case 220:
#line 2472 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), NULL, NULL);
      (yyval.declaration)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 221:
#line 2478 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].array_specifier), NULL);
      (yyval.declaration)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 223:
#line 2488 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (3)].expression);
   ;}
    break;

  case 224:
#line 2492 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (4)].expression);
   ;}
    break;

  case 225:
#line 2499 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_aggregate_initializer();
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(1) - (1)].expression)->link);
   ;}
    break;

  case 226:
#line 2506 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyvsp[(1) - (3)].expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 228:
#line 2518 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 237:
#line 2534 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 238:
#line 2540 "src/compiler/glsl/glsl_parser.yy"
    {
      state->symbols->push_scope();
   ;}
    break;

  case 239:
#line 2544 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(3) - (4)].node));
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (4)]), (yylsp[(4) - (4)]));
      state->symbols->pop_scope();
   ;}
    break;

  case 240:
#line 2553 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 242:
#line 2559 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 243:
#line 2565 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 244:
#line 2574 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(1) - (1)].node) == NULL) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement");
         assert((yyvsp[(1) - (1)].node) != NULL);
      }

      (yyval.node) = (yyvsp[(1) - (1)].node);
      (yyval.node)->link.self_link();
   ;}
    break;

  case 245:
#line 2584 "src/compiler/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].node) == NULL) {
         _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement");
         assert((yyvsp[(2) - (2)].node) != NULL);
      }
      (yyval.node) = (yyvsp[(1) - (2)].node);
      (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 246:
#line 2593 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!state->allow_extension_directive_midshader) {
         _mesa_glsl_error(& (yylsp[(1) - (2)]), state,
                          "#extension directive is not allowed "
                          "in the middle of a shader");
         YYERROR;
      }
   ;}
    break;

  case 247:
#line 2605 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_expression_statement(NULL);
      (yyval.node)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 248:
#line 2611 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 249:
#line 2620 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = new(state->linalloc) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].selection_rest_statement).then_statement,
                                                        (yyvsp[(5) - (5)].selection_rest_statement).else_statement);
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 250:
#line 2629 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (3)].node);
      (yyval.selection_rest_statement).else_statement = (yyvsp[(3) - (3)].node);
   ;}
    break;

  case 251:
#line 2634 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (1)].node);
      (yyval.selection_rest_statement).else_statement = NULL;
   ;}
    break;

  case 252:
#line 2642 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 253:
#line 2646 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), NULL, (yyvsp[(4) - (4)].expression));
      ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
      decl->set_location_range((yylsp[(2) - (4)]), (yylsp[(4) - (4)]));
      declarator->set_location((yylsp[(1) - (4)]));

      declarator->declarations.push_tail(&decl->link);
      (yyval.node) = declarator;
   ;}
    break;

  case 254:
#line 2664 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = new(state->linalloc) ast_switch_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].switch_body));
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 255:
#line 2672 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.switch_body) = new(state->linalloc) ast_switch_body(NULL);
      (yyval.switch_body)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 256:
#line 2677 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.switch_body) = new(state->linalloc) ast_switch_body((yyvsp[(2) - (3)].case_statement_list));
      (yyval.switch_body)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 257:
#line 2685 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.case_label) = new(state->linalloc) ast_case_label((yyvsp[(2) - (3)].expression));
      (yyval.case_label)->set_location((yylsp[(2) - (3)]));
   ;}
    break;

  case 258:
#line 2690 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.case_label) = new(state->linalloc) ast_case_label(NULL);
      (yyval.case_label)->set_location((yylsp[(2) - (2)]));
   ;}
    break;

  case 259:
#line 2698 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_case_label_list *labels = new(state->linalloc) ast_case_label_list();

      labels->labels.push_tail(& (yyvsp[(1) - (1)].case_label)->link);
      (yyval.case_label_list) = labels;
      (yyval.case_label_list)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 260:
#line 2706 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.case_label_list) = (yyvsp[(1) - (2)].case_label_list);
      (yyval.case_label_list)->labels.push_tail(& (yyvsp[(2) - (2)].case_label)->link);
   ;}
    break;

  case 261:
#line 2714 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_case_statement *stmts = new(state->linalloc) ast_case_statement((yyvsp[(1) - (2)].case_label_list));
      stmts->set_location((yylsp[(2) - (2)]));

      stmts->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
      (yyval.case_statement) = stmts;
   ;}
    break;

  case 262:
#line 2722 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.case_statement) = (yyvsp[(1) - (2)].case_statement);
      (yyval.case_statement)->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 263:
#line 2730 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_case_statement_list *cases= new(state->linalloc) ast_case_statement_list();
      cases->set_location((yylsp[(1) - (1)]));

      cases->cases.push_tail(& (yyvsp[(1) - (1)].case_statement)->link);
      (yyval.case_statement_list) = cases;
   ;}
    break;

  case 264:
#line 2738 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.case_statement_list) = (yyvsp[(1) - (2)].case_statement_list);
      (yyval.case_statement_list)->cases.push_tail(& (yyvsp[(2) - (2)].case_statement)->link);
   ;}
    break;

  case 265:
#line 2746 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
                                            NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(4) - (5)]));
   ;}
    break;

  case 266:
#line 2753 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
                                            NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (7)]), (yylsp[(6) - (7)]));
   ;}
    break;

  case 267:
#line 2760 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
                                            (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (6)]), (yylsp[(6) - (6)]));
   ;}
    break;

  case 271:
#line 2776 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
   ;}
    break;

  case 272:
#line 2783 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
      (yyval.for_rest_statement).rest = NULL;
   ;}
    break;

  case 273:
#line 2788 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
      (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
   ;}
    break;

  case 274:
#line 2797 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 275:
#line 2803 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 276:
#line 2809 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 277:
#line 2815 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
      (yyval.node)->set_location_range((yylsp[(1) - (3)]), (yylsp[(2) - (3)]));
   ;}
    break;

  case 278:
#line 2821 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 279:
#line 2830 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_demote_statement();
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 280:
#line 2838 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); ;}
    break;

  case 281:
#line 2839 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 282:
#line 2840 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 283:
#line 2841 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 284:
#line 2842 "src/compiler/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 285:
#line 2847 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      (yyval.function_definition) = new(ctx) ast_function_definition();
      (yyval.function_definition)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
      (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);

      state->symbols->pop_scope();
   ;}
    break;

  case 286:
#line 2861 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = (yyvsp[(1) - (1)].interface_block);
   ;}
    break;

  case 287:
#line 2865 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_interface_block *block = (ast_interface_block *) (yyvsp[(2) - (2)].node);

      if (!(yyvsp[(1) - (2)].type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, block->layout, false,
                              block->layout.has_layout())) {
         YYERROR;
      }

      block->layout = (yyvsp[(1) - (2)].type_qualifier);

      (yyval.node) = block;
   ;}
    break;

  case 288:
#line 2878 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_interface_block *block = (ast_interface_block *)(yyvsp[(2) - (2)].node);

      if (!block->default_layout.flags.q.buffer) {
            _mesa_glsl_error(& (yylsp[(1) - (2)]), state,
                             "memory qualifiers can only be used in the "
                             "declaration of shader storage blocks");
      }
      if (!(yyvsp[(1) - (2)].type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, block->layout, false)) {
         YYERROR;
      }
      block->layout = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.node) = block;
   ;}
    break;

  case 289:
#line 2896 "src/compiler/glsl/glsl_parser.yy"
    {
      ast_interface_block *const block = (yyvsp[(6) - (7)].interface_block);

      if ((yyvsp[(1) - (7)].type_qualifier).flags.q.uniform) {
         block->default_layout = *state->default_uniform_qualifier;
      } else if ((yyvsp[(1) - (7)].type_qualifier).flags.q.buffer) {
         block->default_layout = *state->default_shader_storage_qualifier;
      }
      block->block_name = (yyvsp[(2) - (7)].identifier);
      block->declarations.push_degenerate_list_at_head(& (yyvsp[(4) - (7)].declarator_list)->link);

      _mesa_ast_process_interface_block(& (yylsp[(1) - (7)]), state, block, (yyvsp[(1) - (7)].type_qualifier));

      (yyval.interface_block) = block;
   ;}
    break;

  case 290:
#line 2915 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 291:
#line 2920 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 292:
#line 2925 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.uniform = 1;
   ;}
    break;

  case 293:
#line 2930 "src/compiler/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.buffer = 1;
   ;}
    break;

  case 294:
#line 2935 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!(yyvsp[(1) - (2)].type_qualifier).flags.q.patch) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "invalid interface qualifier");
      }
      if ((yyvsp[(2) - (2)].type_qualifier).has_auxiliary_storage()) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate patch qualifier");
      }
      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.patch = 1;
   ;}
    break;

  case 295:
#line 2949 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block(NULL, NULL);
   ;}
    break;

  case 296:
#line 2953 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block((yyvsp[(1) - (1)].identifier), NULL);
      (yyval.interface_block)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 297:
#line 2958 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block((yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].array_specifier));
      (yyval.interface_block)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 298:
#line 2966 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
      (yyvsp[(1) - (1)].declarator_list)->link.self_link();
   ;}
    break;

  case 299:
#line 2971 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
      (yyvsp[(2) - (2)].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
   ;}
    break;

  case 300:
#line 2979 "src/compiler/glsl/glsl_parser.yy"
    {
      void *ctx = state->linalloc;
      ast_fully_specified_type *type = (yyvsp[(1) - (3)].fully_specified_type);
      type->set_location((yylsp[(1) - (3)]));

      if (type->qualifier.flags.q.attribute) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "keyword 'attribute' cannot be used with "
                          "interface block member");
      } else if (type->qualifier.flags.q.varying) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "keyword 'varying' cannot be used with "
                          "interface block member");
      }

      (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
      (yyval.declarator_list)->set_location((yylsp[(2) - (3)]));

      (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(2) - (3)].declaration)->link);
   ;}
    break;

  case 301:
#line 3003 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false, true)) {
         YYERROR;
      }
   ;}
    break;

  case 303:
#line 3014 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false, true)) {
         YYERROR;
      }
   ;}
    break;

  case 305:
#line 3025 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false, true)) {
         YYERROR;
      }
      if (!(yyval.type_qualifier).validate_in_qualifier(& (yylsp[(1) - (2)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 306:
#line 3035 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!(yyvsp[(1) - (3)].type_qualifier).validate_in_qualifier(& (yylsp[(1) - (3)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 307:
#line 3044 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier), false, true)) {
         YYERROR;
      }
      if (!(yyval.type_qualifier).validate_out_qualifier(& (yylsp[(1) - (2)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 308:
#line 3054 "src/compiler/glsl/glsl_parser.yy"
    {
      if (!(yyvsp[(1) - (3)].type_qualifier).validate_out_qualifier(& (yylsp[(1) - (3)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 309:
#line 3063 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
      if (!state->default_uniform_qualifier->
             merge_qualifier(& (yylsp[(1) - (1)]), state, (yyvsp[(1) - (1)].type_qualifier), false)) {
         YYERROR;
      }
      if (!state->default_uniform_qualifier->
             push_to_global(& (yylsp[(1) - (1)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 310:
#line 3075 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
      if (!state->default_shader_storage_qualifier->
             merge_qualifier(& (yylsp[(1) - (1)]), state, (yyvsp[(1) - (1)].type_qualifier), false)) {
         YYERROR;
      }
      if (!state->default_shader_storage_qualifier->
             push_to_global(& (yylsp[(1) - (1)]), state)) {
         YYERROR;
      }

      /* From the GLSL 4.50 spec, section 4.4.5:
       *
       *     "It is a compile-time error to specify the binding identifier for
       *     the global scope or for block member declarations."
       */
      if (state->default_shader_storage_qualifier->flags.q.explicit_binding) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state,
                          "binding qualifier cannot be set for default layout");
      }
   ;}
    break;

  case 311:
#line 3097 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
      if (!(yyvsp[(1) - (1)].type_qualifier).merge_into_in_qualifier(& (yylsp[(1) - (1)]), state, (yyval.node))) {
         YYERROR;
      }
      if (!state->in_qualifier->push_to_global(& (yylsp[(1) - (1)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 312:
#line 3107 "src/compiler/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
      if (!(yyvsp[(1) - (1)].type_qualifier).merge_into_out_qualifier(& (yylsp[(1) - (1)]), state, (yyval.node))) {
         YYERROR;
      }
      if (!state->out_qualifier->push_to_global(& (yylsp[(1) - (1)]), state)) {
         YYERROR;
      }
   ;}
    break;


/* Line 1267 of yacc.c.  */
#line 5904 "src/compiler/glsl/glsl_parser.cpp"
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
      /* If just tried and failed to reuse look-ahead token after an
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

  /* Else will try to reuse look-ahead token after shifting the error
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

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, state, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
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



