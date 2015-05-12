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
#define yyparse glcpp_parser_parse
#define yylex   glcpp_parser_lex
#define yyerror glcpp_parser_error
#define yylval  glcpp_parser_lval
#define yychar  glcpp_parser_char
#define yydebug glcpp_parser_debug
#define yynerrs glcpp_parser_nerrs
#define yylloc glcpp_parser_lloc

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DEFINED = 258,
     ELIF_EXPANDED = 259,
     HASH_TOKEN = 260,
     DEFINE_TOKEN = 261,
     FUNC_IDENTIFIER = 262,
     OBJ_IDENTIFIER = 263,
     ELIF = 264,
     ELSE = 265,
     ENDIF = 266,
     ERROR_TOKEN = 267,
     IF = 268,
     IFDEF = 269,
     IFNDEF = 270,
     LINE = 271,
     PRAGMA = 272,
     UNDEF = 273,
     VERSION_TOKEN = 274,
     GARBAGE = 275,
     IDENTIFIER = 276,
     IF_EXPANDED = 277,
     INTEGER = 278,
     INTEGER_STRING = 279,
     LINE_EXPANDED = 280,
     NEWLINE = 281,
     OTHER = 282,
     PLACEHOLDER = 283,
     SPACE = 284,
     PLUS_PLUS = 285,
     MINUS_MINUS = 286,
     PASTE = 287,
     OR = 288,
     AND = 289,
     NOT_EQUAL = 290,
     EQUAL = 291,
     GREATER_OR_EQUAL = 292,
     LESS_OR_EQUAL = 293,
     RIGHT_SHIFT = 294,
     LEFT_SHIFT = 295,
     UNARY = 296
   };
#endif
/* Tokens.  */
#define DEFINED 258
#define ELIF_EXPANDED 259
#define HASH_TOKEN 260
#define DEFINE_TOKEN 261
#define FUNC_IDENTIFIER 262
#define OBJ_IDENTIFIER 263
#define ELIF 264
#define ELSE 265
#define ENDIF 266
#define ERROR_TOKEN 267
#define IF 268
#define IFDEF 269
#define IFNDEF 270
#define LINE 271
#define PRAGMA 272
#define UNDEF 273
#define VERSION_TOKEN 274
#define GARBAGE 275
#define IDENTIFIER 276
#define IF_EXPANDED 277
#define INTEGER 278
#define INTEGER_STRING 279
#define LINE_EXPANDED 280
#define NEWLINE 281
#define OTHER 282
#define PLACEHOLDER 283
#define SPACE 284
#define PLUS_PLUS 285
#define MINUS_MINUS 286
#define PASTE 287
#define OR 288
#define AND 289
#define NOT_EQUAL 290
#define EQUAL 291
#define GREATER_OR_EQUAL 292
#define LESS_OR_EQUAL 293
#define RIGHT_SHIFT 294
#define LEFT_SHIFT 295
#define UNARY 296




/* Copy the first part of user declarations.  */
#line 1 "src/glsl/glcpp/glcpp-parse.y"

/*
 * Copyright © 2010 Intel Corporation
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
#include <inttypes.h>

#include "glcpp.h"
#include "main/core.h" /* for struct gl_extensions */
#include "main/mtypes.h" /* for gl_api enum */

static void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error);

static void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *macro,
		      token_list_t *replacements);

static void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

static string_list_t *
_string_list_create (void *ctx);

static void
_string_list_append_item (string_list_t *list, const char *str);

static int
_string_list_contains (string_list_t *list, const char *member, int *index);

static const char *
_string_list_has_duplicate (string_list_t *list);

static int
_string_list_length (string_list_t *list);

static int
_string_list_equal (string_list_t *a, string_list_t *b);

static argument_list_t *
_argument_list_create (void *ctx);

static void
_argument_list_append (argument_list_t *list, token_list_t *argument);

static int
_argument_list_length (argument_list_t *list);

static token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

/* Note: This function ralloc_steal()s the str pointer. */
static token_t *
_token_create_str (void *ctx, int type, char *str);

static token_t *
_token_create_ival (void *ctx, int type, int ival);

static token_list_t *
_token_list_create (void *ctx);

static void
_token_list_append (token_list_t *list, token_t *token);

static void
_token_list_append_list (token_list_t *list, token_list_t *tail);

static int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b);

static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker);

static void
_parser_active_list_pop (glcpp_parser_t *parser);

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier);

typedef enum {
	EXPANSION_MODE_IGNORE_DEFINED,
	EXPANSION_MODE_EVALUATE_DEFINED
} expansion_mode_t;

/* Expand list, and begin lexing from the result (after first
 * prefixing a token of type 'head_token_type').
 */
static void
_glcpp_parser_expand_and_lex_from (glcpp_parser_t *parser,
				   int head_token_type,
				   token_list_t *list,
				   expansion_mode_t mode);

/* Perform macro expansion in-place on the given list. */
static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list,
				 expansion_mode_t mode);

static void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list);

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition);

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition);

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc);

static void
_glcpp_parser_handle_version_declaration(glcpp_parser_t *parser, intmax_t version,
                                         const char *ident, bool explicitly_set);

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser);

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list);

static void
add_builtin_define(glcpp_parser_t *parser, const char *name, int value);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
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
typedef int YYSTYPE;
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
#line 368 "src/glsl/glcpp/glcpp-parse.c"

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   652

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  64
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  25
/* YYNRULES -- Number of rules.  */
#define YYNRULES  118
/* YYNRULES -- Number of states.  */
#define YYNSTATES  185

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   296

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    54,     2,     2,     2,    50,    37,     2,
      52,    53,    48,    46,    56,    47,    61,    49,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    62,
      40,    63,    41,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    57,     2,    58,    36,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    59,    35,    60,    55,     2,     2,     2,
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
      38,    39,    42,    43,    44,    45,    51
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    12,    14,    16,    20,
      24,    28,    33,    37,    43,    50,    52,    54,    55,    61,
      62,    67,    68,    74,    75,    81,    85,    86,    93,    94,
     101,   106,   110,   111,   116,   117,   122,   127,   133,   136,
     140,   144,   148,   153,   155,   157,   159,   161,   165,   169,
     173,   177,   181,   185,   189,   193,   197,   201,   205,   209,
     213,   217,   221,   225,   229,   233,   236,   239,   242,   245,
     249,   251,   255,   257,   260,   261,   263,   264,   266,   268,
     271,   273,   275,   277,   279,   281,   283,   285,   287,   289,
     291,   293,   295,   297,   299,   301,   303,   305,   307,   309,
     311,   313,   315,   317,   319,   321,   323,   325,   327,   329,
     331,   333,   335,   337,   339,   341,   343,   345,   347
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      65,     0,    -1,    -1,    65,    66,    -1,    69,    -1,    29,
      69,    -1,    83,    -1,    67,    -1,    22,    81,    26,    -1,
       4,    81,    26,    -1,    25,    80,    26,    -1,    25,    80,
      80,    26,    -1,     8,    84,    26,    -1,     7,    52,    53,
      84,    26,    -1,     7,    52,    82,    53,    84,    26,    -1,
      71,    -1,    79,    -1,    -1,     5,    16,    70,    86,    26,
      -1,    -1,     5,     6,    72,    68,    -1,    -1,     5,    18,
      73,    21,    26,    -1,    -1,     5,    13,    74,    86,    26,
      -1,     5,    13,    26,    -1,    -1,     5,    14,    75,    21,
      85,    26,    -1,    -1,     5,    15,    76,    21,    85,    26,
      -1,     5,     9,    86,    26,    -1,     5,     9,    26,    -1,
      -1,     5,    10,    77,    26,    -1,    -1,     5,    11,    78,
      26,    -1,     5,    19,    80,    26,    -1,     5,    19,    80,
      21,    26,    -1,     5,    26,    -1,     5,    17,    26,    -1,
       5,    12,    26,    -1,     5,     6,    26,    -1,     5,    20,
      86,    26,    -1,    24,    -1,    23,    -1,    80,    -1,    21,
      -1,    81,    33,    81,    -1,    81,    34,    81,    -1,    81,
      35,    81,    -1,    81,    36,    81,    -1,    81,    37,    81,
      -1,    81,    38,    81,    -1,    81,    39,    81,    -1,    81,
      42,    81,    -1,    81,    43,    81,    -1,    81,    41,    81,
      -1,    81,    40,    81,    -1,    81,    44,    81,    -1,    81,
      45,    81,    -1,    81,    47,    81,    -1,    81,    46,    81,
      -1,    81,    50,    81,    -1,    81,    49,    81,    -1,    81,
      48,    81,    -1,    54,    81,    -1,    55,    81,    -1,    47,
      81,    -1,    46,    81,    -1,    52,    81,    53,    -1,    21,
      -1,    82,    56,    21,    -1,    26,    -1,    86,    26,    -1,
      -1,    86,    -1,    -1,    86,    -1,    87,    -1,    86,    87,
      -1,    21,    -1,    24,    -1,    88,    -1,     3,    -1,    27,
      -1,    29,    -1,    57,    -1,    58,    -1,    52,    -1,    53,
      -1,    59,    -1,    60,    -1,    61,    -1,    37,    -1,    48,
      -1,    46,    -1,    47,    -1,    55,    -1,    54,    -1,    49,
      -1,    50,    -1,    45,    -1,    44,    -1,    40,    -1,    41,
      -1,    43,    -1,    42,    -1,    39,    -1,    38,    -1,    36,
      -1,    35,    -1,    34,    -1,    33,    -1,    62,    -1,    56,
      -1,    63,    -1,    32,    -1,    30,    -1,    31,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   205,   205,   207,   211,   212,   213,   218,   222,   227,
     232,   240,   253,   256,   259,   265,   268,   269,   269,   284,
     284,   287,   287,   304,   304,   327,   337,   337,   344,   344,
     351,   377,   397,   397,   410,   410,   413,   419,   425,   428,
     434,   437,   440,   446,   455,   460,   464,   471,   482,   493,
     500,   507,   514,   521,   528,   535,   542,   549,   556,   563,
     570,   577,   584,   596,   608,   615,   619,   623,   627,   631,
     637,   642,   650,   651,   655,   656,   659,   661,   667,   672,
     679,   683,   687,   691,   695,   699,   706,   707,   708,   709,
     710,   711,   712,   713,   714,   715,   716,   717,   718,   719,
     720,   721,   722,   723,   724,   725,   726,   727,   728,   729,
     730,   731,   732,   733,   734,   735,   736,   737,   738
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DEFINED", "ELIF_EXPANDED", "HASH_TOKEN",
  "DEFINE_TOKEN", "FUNC_IDENTIFIER", "OBJ_IDENTIFIER", "ELIF", "ELSE",
  "ENDIF", "ERROR_TOKEN", "IF", "IFDEF", "IFNDEF", "LINE", "PRAGMA",
  "UNDEF", "VERSION_TOKEN", "GARBAGE", "IDENTIFIER", "IF_EXPANDED",
  "INTEGER", "INTEGER_STRING", "LINE_EXPANDED", "NEWLINE", "OTHER",
  "PLACEHOLDER", "SPACE", "PLUS_PLUS", "MINUS_MINUS", "PASTE", "OR", "AND",
  "'|'", "'^'", "'&'", "NOT_EQUAL", "EQUAL", "'<'", "'>'",
  "GREATER_OR_EQUAL", "LESS_OR_EQUAL", "RIGHT_SHIFT", "LEFT_SHIFT", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY", "'('", "')'", "'!'", "'~'", "','",
  "'['", "']'", "'{'", "'}'", "'.'", "';'", "'='", "$accept", "input",
  "line", "expanded_line", "define", "control_line", "@1",
  "control_line_success", "@2", "@3", "@4", "@5", "@6", "@7", "@8",
  "control_line_error", "integer_constant", "expression",
  "identifier_list", "text_line", "replacement_list", "junk", "pp_tokens",
  "preprocessing_token", "operator", 0
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
     285,   286,   287,   288,   289,   124,    94,    38,   290,   291,
      60,    62,   292,   293,   294,   295,    43,    45,    42,    47,
      37,   296,    40,    41,    33,   126,    44,    91,    93,   123,
     125,    46,    59,    61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    64,    65,    65,    66,    66,    66,    66,    67,    67,
      67,    67,    68,    68,    68,    69,    69,    70,    69,    72,
      71,    73,    71,    74,    71,    71,    75,    71,    76,    71,
      71,    71,    77,    71,    78,    71,    71,    71,    71,    71,
      79,    79,    79,    80,    80,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      82,    82,    83,    83,    84,    84,    85,    85,    86,    86,
      87,    87,    87,    87,    87,    87,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    88,    88,    88,    88,    88,    88,    88
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     2,     1,     1,     3,     3,
       3,     4,     3,     5,     6,     1,     1,     0,     5,     0,
       4,     0,     5,     0,     5,     3,     0,     6,     0,     6,
       4,     3,     0,     4,     0,     4,     4,     5,     2,     3,
       3,     3,     4,     1,     1,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     2,     2,     3,
       1,     3,     1,     2,     0,     1,     0,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    83,     0,     0,    80,     0,    81,     0,
      72,    84,    85,   117,   118,   116,   112,   111,   110,   109,
      93,   108,   107,   103,   104,   106,   105,   102,   101,    95,
      96,    94,    99,   100,    88,    89,    98,    97,   114,    86,
      87,    90,    91,    92,   113,   115,     3,     7,     4,    15,
      16,     6,     0,    78,    82,    46,    44,    43,     0,     0,
       0,     0,     0,    45,     0,    19,     0,    32,    34,     0,
      23,    26,    28,    17,     0,    21,     0,     0,    38,     0,
       0,     5,    73,    85,    79,    68,    67,     0,    65,    66,
       9,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
       0,    31,     0,     0,     0,    40,    25,     0,     0,     0,
       0,    39,     0,     0,     0,     8,    10,     0,    69,    47,
      48,    49,    50,    51,    52,    53,    57,    56,    54,    55,
      58,    59,    61,    60,    64,    63,    62,     0,    74,    20,
      30,    33,    35,     0,    76,    76,     0,     0,     0,    36,
      42,    11,     0,     0,    75,    24,     0,    77,     0,    18,
      22,    37,    70,    74,     0,    12,    27,    29,     0,    74,
       0,    13,     0,    71,    14
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    46,    47,   149,    48,   120,    49,   110,   122,
     117,   118,   119,   113,   114,    50,    63,    64,   174,    51,
     163,   166,   164,    53,    54
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -142
static const yytype_int16 yypact[] =
{
    -142,   134,  -142,  -142,   -18,   578,  -142,   -18,  -142,   -15,
    -142,  -142,    26,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
    -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
    -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
    -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,  -142,
    -142,  -142,   182,  -142,  -142,  -142,  -142,  -142,   -18,   -18,
     -18,   -18,   -18,  -142,   508,     7,   230,  -142,  -142,     9,
      25,  -142,  -142,  -142,    35,  -142,   -15,   470,  -142,   533,
      81,  -142,  -142,  -142,  -142,  -142,  -142,   -23,  -142,  -142,
    -142,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,
     -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,   -18,  -142,
      65,  -142,   278,    38,    58,  -142,  -142,   470,    90,    91,
     470,  -142,    92,    37,   326,  -142,  -142,    89,  -142,   571,
     587,   602,    83,   102,     0,     0,    33,    33,    33,    33,
      20,    20,    60,    60,  -142,  -142,  -142,    66,   470,  -142,
    -142,  -142,  -142,   374,   470,   470,   422,   109,   110,  -142,
    -142,  -142,   -14,   131,   470,  -142,   136,   470,   172,  -142,
    -142,  -142,  -142,   470,     4,  -142,  -142,  -142,   173,   470,
     179,  -142,   175,  -142,  -142
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -142,  -142,  -142,  -142,  -142,    47,  -142,  -142,  -142,  -142,
    -142,  -142,  -142,  -142,  -142,  -142,    -5,    -6,  -142,  -142,
    -141,    49,    -1,   -50,  -142
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      52,    79,    84,    55,    80,    56,    57,   172,    56,    57,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,    58,    59,
     128,     5,   178,   109,    60,   115,    61,    62,   182,   173,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   116,    85,    86,    87,    88,    89,   179,   158,    81,
     180,   121,    84,   159,   151,   112,   104,   105,   106,   107,
     108,   123,   147,   148,    84,   127,   124,   102,   103,   104,
     105,   106,   107,   108,   152,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,    84,    56,    57,    84,   126,   106,   107,
     108,   154,   155,   157,    84,   161,   153,    84,   162,   156,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,     2,   170,   171,     3,     4,     5,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   167,   167,     6,     7,   175,     8,     9,
      10,    11,   176,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     3,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,   177,   181,
     183,   184,     0,     6,   168,     0,     8,     0,    82,    11,
       0,    83,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     3,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,     0,     0,     0,     0,
       0,     6,     0,     0,     8,     0,   111,    11,     0,    83,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     3,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,     0,     0,     0,     0,     0,     6,
       0,     0,     8,     0,   150,    11,     0,    83,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,     3,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,     0,     0,     0,     0,     0,     6,     0,     0,
       8,     0,   160,    11,     0,    83,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,     3,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
       0,     0,     0,     0,     0,     6,     0,     0,     8,     0,
     165,    11,     0,    83,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,     3,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,     0,     0,
       0,     0,     0,     6,     0,     0,     8,     0,   169,    11,
       0,    83,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     3,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,     0,     0,     0,     0,
       0,     6,     0,     0,     8,     0,     0,    11,     0,    83,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     0,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    90,     0,     0,     0,     0,     0,
       0,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   125,
       0,     0,     0,     0,     0,     0,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,    65,     0,     0,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,     0,
       0,     0,     0,     0,    78,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108
};

static const yytype_int16 yycheck[] =
{
       1,     7,    52,    21,     9,    23,    24,    21,    23,    24,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    46,    47,
      53,     5,   173,    26,    52,    26,    54,    55,   179,    53,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    26,    58,    59,    60,    61,    62,    53,    21,    12,
      56,    26,   112,    26,    26,    66,    46,    47,    48,    49,
      50,    76,     7,     8,   124,    80,    77,    44,    45,    46,
      47,    48,    49,    50,    26,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   153,    23,    24,   156,    26,    48,    49,
      50,    21,    21,    21,   164,    26,   117,   167,    52,   120,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,     0,    26,    26,     3,     4,     5,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,   154,   155,    21,    22,    26,    24,    25,
      26,    27,    26,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,     3,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    26,    26,
      21,    26,    -1,    21,   155,    -1,    24,    -1,    26,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,     3,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    -1,    -1,    -1,    -1,
      -1,    21,    -1,    -1,    24,    -1,    26,    27,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,     3,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    -1,    -1,    -1,    -1,    -1,    21,
      -1,    -1,    24,    -1,    26,    27,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,     3,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,
      24,    -1,    26,    27,    -1,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,     3,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      -1,    -1,    -1,    -1,    -1,    21,    -1,    -1,    24,    -1,
      26,    27,    -1,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,     3,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    -1,    -1,
      -1,    -1,    -1,    21,    -1,    -1,    24,    -1,    26,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,     3,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    -1,    -1,    -1,    -1,
      -1,    21,    -1,    -1,    24,    -1,    -1,    27,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    -1,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    26,
      -1,    -1,    -1,    -1,    -1,    -1,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,     6,    -1,    -1,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    26,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    65,     0,     3,     4,     5,    21,    22,    24,    25,
      26,    27,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    66,    67,    69,    71,
      79,    83,    86,    87,    88,    21,    23,    24,    46,    47,
      52,    54,    55,    80,    81,     6,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    26,    81,
      80,    69,    26,    29,    87,    81,    81,    81,    81,    81,
      26,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    26,
      72,    26,    86,    77,    78,    26,    26,    74,    75,    76,
      70,    26,    73,    80,    86,    26,    26,    80,    53,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    81,    81,     7,     8,    68,
      26,    26,    26,    86,    21,    21,    86,    21,    21,    26,
      26,    26,    52,    84,    86,    26,    85,    86,    85,    26,
      26,    26,    21,    53,    82,    26,    26,    26,    84,    53,
      56,    26,    84,    21,    26
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
      yyerror (&yylloc, parser, YY_("syntax error: cannot back up")); \
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
# define YYLEX yylex (&yylval, &yylloc, parser)
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
		  Type, Value, Location, parser); \
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, glcpp_parser_t *parser)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, parser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    glcpp_parser_t *parser;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (parser);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, glcpp_parser_t *parser)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, parser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    glcpp_parser_t *parser;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, parser);
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
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, glcpp_parser_t *parser)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, parser)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    glcpp_parser_t *parser;
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
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , parser);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, parser); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, glcpp_parser_t *parser)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, parser)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    glcpp_parser_t *parser;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (parser);

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
int yyparse (glcpp_parser_t *parser);
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
yyparse (glcpp_parser_t *parser)
#else
int
yyparse (parser)
    glcpp_parser_t *parser;
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
#line 165 "src/glsl/glcpp/glcpp-parse.y"
{
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.last_line = 1;
	yylloc.last_column = 1;
	yylloc.source = 0;
}
/* Line 1078 of yacc.c.  */
#line 1693 "src/glsl/glcpp/glcpp-parse.c"
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
        case 6:
#line 213 "src/glsl/glcpp/glcpp-parse.y"
    {
		_glcpp_parser_print_expanded_token_list (parser, (yyvsp[(1) - (1)].token_list));
		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "\n");
		ralloc_free ((yyvsp[(1) - (1)].token_list));
	;}
    break;

  case 8:
#line 222 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (parser->is_gles && (yyvsp[(2) - (3)].expression_value).undefined_macro)
			glcpp_error(& (yylsp[(1) - (3)]), parser, "undefined macro %s in expression (illegal in GLES)", (yyvsp[(2) - (3)].expression_value).undefined_macro);
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[(1) - (3)]), (yyvsp[(2) - (3)].expression_value).value);
	;}
    break;

  case 9:
#line 227 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (parser->is_gles && (yyvsp[(2) - (3)].expression_value).undefined_macro)
			glcpp_error(& (yylsp[(1) - (3)]), parser, "undefined macro %s in expression (illegal in GLES)", (yyvsp[(2) - (3)].expression_value).undefined_macro);
		_glcpp_parser_skip_stack_change_if (parser, & (yylsp[(1) - (3)]), "elif", (yyvsp[(2) - (3)].expression_value).value);
	;}
    break;

  case 10:
#line 232 "src/glsl/glcpp/glcpp-parse.y"
    {
		parser->has_new_line_number = 1;
		parser->new_line_number = (yyvsp[(2) - (3)].ival);
		ralloc_asprintf_rewrite_tail (&parser->output,
					      &parser->output_length,
					      "#line %" PRIiMAX "\n",
					      (yyvsp[(2) - (3)].ival));
	;}
    break;

  case 11:
#line 240 "src/glsl/glcpp/glcpp-parse.y"
    {
		parser->has_new_line_number = 1;
		parser->new_line_number = (yyvsp[(2) - (4)].ival);
		parser->has_new_source_number = 1;
		parser->new_source_number = (yyvsp[(3) - (4)].ival);
		ralloc_asprintf_rewrite_tail (&parser->output,
					      &parser->output_length,
					      "#line %" PRIiMAX " %" PRIiMAX "\n",
					      (yyvsp[(2) - (4)].ival), (yyvsp[(3) - (4)].ival));
	;}
    break;

  case 12:
#line 253 "src/glsl/glcpp/glcpp-parse.y"
    {
		_define_object_macro (parser, & (yylsp[(1) - (3)]), (yyvsp[(1) - (3)].str), (yyvsp[(2) - (3)].token_list));
	;}
    break;

  case 13:
#line 256 "src/glsl/glcpp/glcpp-parse.y"
    {
		_define_function_macro (parser, & (yylsp[(1) - (5)]), (yyvsp[(1) - (5)].str), NULL, (yyvsp[(4) - (5)].token_list));
	;}
    break;

  case 14:
#line 259 "src/glsl/glcpp/glcpp-parse.y"
    {
		_define_function_macro (parser, & (yylsp[(1) - (6)]), (yyvsp[(1) - (6)].str), (yyvsp[(3) - (6)].string_list), (yyvsp[(5) - (6)].token_list));
	;}
    break;

  case 15:
#line 265 "src/glsl/glcpp/glcpp-parse.y"
    {
		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "\n");
	;}
    break;

  case 17:
#line 269 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 18:
#line 271 "src/glsl/glcpp/glcpp-parse.y"
    {

		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   LINE_EXPANDED, (yyvsp[(4) - (5)].token_list),
							   EXPANSION_MODE_IGNORE_DEFINED);
		}
	;}
    break;

  case 19:
#line 284 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 21:
#line 287 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 22:
#line 289 "src/glsl/glcpp/glcpp-parse.y"
    {
		macro_t *macro;
		if (strcmp("__LINE__", (yyvsp[(4) - (5)].str)) == 0
		    || strcmp("__FILE__", (yyvsp[(4) - (5)].str)) == 0
		    || strcmp("__VERSION__", (yyvsp[(4) - (5)].str)) == 0)
			glcpp_error(& (yylsp[(1) - (5)]), parser, "Built-in (pre-defined)"
				    " macro names can not be undefined.");

		macro = hash_table_find (parser->defines, (yyvsp[(4) - (5)].str));
		if (macro) {
			hash_table_remove (parser->defines, (yyvsp[(4) - (5)].str));
			ralloc_free (macro);
		}
		ralloc_free ((yyvsp[(4) - (5)].str));
	;}
    break;

  case 23:
#line 304 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 24:
#line 306 "src/glsl/glcpp/glcpp-parse.y"
    {
		/* Be careful to only evaluate the 'if' expression if
		 * we are not skipping. When we are skipping, we
		 * simply push a new 0-valued 'if' onto the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   IF_EXPANDED, (yyvsp[(4) - (5)].token_list),
							   EXPANSION_MODE_EVALUATE_DEFINED);
		}	
		else
		{
			_glcpp_parser_skip_stack_push_if (parser, & (yylsp[(1) - (5)]), 0);
			parser->skip_stack->type = SKIP_TO_ENDIF;
		}
	;}
    break;

  case 25:
#line 327 "src/glsl/glcpp/glcpp-parse.y"
    {
		/* #if without an expression is only an error if we
		 *  are not skipping */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			glcpp_error(& (yylsp[(1) - (3)]), parser, "#if with no expression");
		}	
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[(1) - (3)]), 0);
	;}
    break;

  case 26:
#line 337 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 27:
#line 339 "src/glsl/glcpp/glcpp-parse.y"
    {
		macro_t *macro = hash_table_find (parser->defines, (yyvsp[(4) - (6)].str));
		ralloc_free ((yyvsp[(4) - (6)].str));
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[(1) - (6)]), macro != NULL);
	;}
    break;

  case 28:
#line 344 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 29:
#line 346 "src/glsl/glcpp/glcpp-parse.y"
    {
		macro_t *macro = hash_table_find (parser->defines, (yyvsp[(4) - (6)].str));
		ralloc_free ((yyvsp[(4) - (6)].str));
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[(3) - (6)]), macro == NULL);
	;}
    break;

  case 30:
#line 351 "src/glsl/glcpp/glcpp-parse.y"
    {
		/* Be careful to only evaluate the 'elif' expression
		 * if we are not skipping. When we are skipping, we
		 * simply change to a 0-valued 'elif' on the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   ELIF_EXPANDED, (yyvsp[(3) - (4)].token_list),
							   EXPANSION_MODE_EVALUATE_DEFINED);
		}
		else if (parser->skip_stack &&
		    parser->skip_stack->has_else)
		{
			glcpp_error(& (yylsp[(1) - (4)]), parser, "#elif after #else");
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & (yylsp[(1) - (4)]),
							    "elif", 0);
		}
	;}
    break;

  case 31:
#line 377 "src/glsl/glcpp/glcpp-parse.y"
    {
		/* #elif without an expression is an error unless we
		 * are skipping. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			glcpp_error(& (yylsp[(1) - (3)]), parser, "#elif with no expression");
		}
		else if (parser->skip_stack &&
		    parser->skip_stack->has_else)
		{
			glcpp_error(& (yylsp[(1) - (3)]), parser, "#elif after #else");
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & (yylsp[(1) - (3)]),
							    "elif", 0);
			glcpp_warning(& (yylsp[(1) - (3)]), parser, "ignoring illegal #elif without expression");
		}
	;}
    break;

  case 32:
#line 397 "src/glsl/glcpp/glcpp-parse.y"
    { parser->lexing_directive = 1; ;}
    break;

  case 33:
#line 397 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (parser->skip_stack &&
		    parser->skip_stack->has_else)
		{
			glcpp_error(& (yylsp[(1) - (4)]), parser, "multiple #else");
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & (yylsp[(1) - (4)]), "else", 1);
			if (parser->skip_stack)
				parser->skip_stack->has_else = true;
		}
	;}
    break;

  case 34:
#line 410 "src/glsl/glcpp/glcpp-parse.y"
    {
		_glcpp_parser_skip_stack_pop (parser, & (yylsp[(1) - (2)]));
	;}
    break;

  case 36:
#line 413 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (parser->version_resolved) {
			glcpp_error(& (yylsp[(1) - (4)]), parser, "#version must appear on the first line");
		}
		_glcpp_parser_handle_version_declaration(parser, (yyvsp[(3) - (4)].ival), NULL, true);
	;}
    break;

  case 37:
#line 419 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (parser->version_resolved) {
			glcpp_error(& (yylsp[(1) - (5)]), parser, "#version must appear on the first line");
		}
		_glcpp_parser_handle_version_declaration(parser, (yyvsp[(3) - (5)].ival), (yyvsp[(4) - (5)].str), true);
	;}
    break;

  case 38:
#line 425 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_parser_resolve_implicit_version(parser);
	;}
    break;

  case 39:
#line 428 "src/glsl/glcpp/glcpp-parse.y"
    {
		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "#%s", (yyvsp[(2) - (3)].str));
	;}
    break;

  case 40:
#line 434 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_error(& (yylsp[(1) - (3)]), parser, "#%s", (yyvsp[(2) - (3)].str));
	;}
    break;

  case 41:
#line 437 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_error (& (yylsp[(1) - (3)]), parser, "#define without macro name");
	;}
    break;

  case 42:
#line 440 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_error (& (yylsp[(1) - (4)]), parser, "Illegal non-directive after #");
	;}
    break;

  case 43:
#line 446 "src/glsl/glcpp/glcpp-parse.y"
    {
		if (strlen ((yyvsp[(1) - (1)].str)) >= 3 && strncmp ((yyvsp[(1) - (1)].str), "0x", 2) == 0) {
			(yyval.ival) = (int)strtoll ((yyvsp[(1) - (1)].str) + 2, NULL, 16);
		} else if ((yyvsp[(1) - (1)].str)[0] == '0') {
			(yyval.ival) = (int)strtoll ((yyvsp[(1) - (1)].str), NULL, 8);
		} else {
			(yyval.ival) = (int)strtoll ((yyvsp[(1) - (1)].str), NULL, 10);
		}
	;}
    break;

  case 44:
#line 455 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.ival) = (yyvsp[(1) - (1)].ival);
	;}
    break;

  case 45:
#line 460 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (1)].ival);
		(yyval.expression_value).undefined_macro = NULL;
	;}
    break;

  case 46:
#line 464 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = 0;
		if (parser->is_gles)
			(yyval.expression_value).undefined_macro = ralloc_strdup (parser, (yyvsp[(1) - (1)].str));
		else
			(yyval.expression_value).undefined_macro = NULL;
	;}
    break;

  case 47:
#line 471 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value || (yyvsp[(3) - (3)].expression_value).value;

		/* Short-circuit: Only flag undefined from right side
		 * if left side evaluates to false.
		 */
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else if (! (yyvsp[(1) - (3)].expression_value).value)
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 48:
#line 482 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value && (yyvsp[(3) - (3)].expression_value).value;

		/* Short-circuit: Only flag undefined from right-side
		 * if left side evaluates to true.
		 */
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else if ((yyvsp[(1) - (3)].expression_value).value)
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 49:
#line 493 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value | (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 50:
#line 500 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value ^ (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 51:
#line 507 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value & (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 52:
#line 514 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value != (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 53:
#line 521 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value == (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 54:
#line 528 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value >= (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 55:
#line 535 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value <= (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 56:
#line 542 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value > (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 57:
#line 549 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value < (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 58:
#line 556 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value >> (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 59:
#line 563 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value << (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 60:
#line 570 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value - (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 61:
#line 577 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value + (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 62:
#line 584 "src/glsl/glcpp/glcpp-parse.y"
    {
		if ((yyvsp[(3) - (3)].expression_value).value == 0) {
			yyerror (& (yylsp[(1) - (3)]), parser,
				 "zero modulus in preprocessor directive");
		} else {
			(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value % (yyvsp[(3) - (3)].expression_value).value;
		}
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 63:
#line 596 "src/glsl/glcpp/glcpp-parse.y"
    {
		if ((yyvsp[(3) - (3)].expression_value).value == 0) {
			yyerror (& (yylsp[(1) - (3)]), parser,
				 "division by 0 in preprocessor directive");
		} else {
			(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value / (yyvsp[(3) - (3)].expression_value).value;
		}
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 64:
#line 608 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = (yyvsp[(1) - (3)].expression_value).value * (yyvsp[(3) - (3)].expression_value).value;
		if ((yyvsp[(1) - (3)].expression_value).undefined_macro)
			(yyval.expression_value).undefined_macro = (yyvsp[(1) - (3)].expression_value).undefined_macro;
                else
			(yyval.expression_value).undefined_macro = (yyvsp[(3) - (3)].expression_value).undefined_macro;
	;}
    break;

  case 65:
#line 615 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = ! (yyvsp[(2) - (2)].expression_value).value;
		(yyval.expression_value).undefined_macro = (yyvsp[(2) - (2)].expression_value).undefined_macro;
	;}
    break;

  case 66:
#line 619 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = ~ (yyvsp[(2) - (2)].expression_value).value;
		(yyval.expression_value).undefined_macro = (yyvsp[(2) - (2)].expression_value).undefined_macro;
	;}
    break;

  case 67:
#line 623 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = - (yyvsp[(2) - (2)].expression_value).value;
		(yyval.expression_value).undefined_macro = (yyvsp[(2) - (2)].expression_value).undefined_macro;
	;}
    break;

  case 68:
#line 627 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value).value = + (yyvsp[(2) - (2)].expression_value).value;
		(yyval.expression_value).undefined_macro = (yyvsp[(2) - (2)].expression_value).undefined_macro;
	;}
    break;

  case 69:
#line 631 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.expression_value) = (yyvsp[(2) - (3)].expression_value);
	;}
    break;

  case 70:
#line 637 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.string_list) = _string_list_create (parser);
		_string_list_append_item ((yyval.string_list), (yyvsp[(1) - (1)].str));
		ralloc_steal ((yyval.string_list), (yyvsp[(1) - (1)].str));
	;}
    break;

  case 71:
#line 642 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.string_list) = (yyvsp[(1) - (3)].string_list);	
		_string_list_append_item ((yyval.string_list), (yyvsp[(3) - (3)].str));
		ralloc_steal ((yyval.string_list), (yyvsp[(3) - (3)].str));
	;}
    break;

  case 72:
#line 650 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.token_list) = NULL; ;}
    break;

  case 74:
#line 655 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.token_list) = NULL; ;}
    break;

  case 77:
#line 661 "src/glsl/glcpp/glcpp-parse.y"
    {
		glcpp_error(&(yylsp[(1) - (1)]), parser, "extra tokens at end of directive");
	;}
    break;

  case 78:
#line 667 "src/glsl/glcpp/glcpp-parse.y"
    {
		parser->space_tokens = 1;
		(yyval.token_list) = _token_list_create (parser);
		_token_list_append ((yyval.token_list), (yyvsp[(1) - (1)].token));
	;}
    break;

  case 79:
#line 672 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token_list) = (yyvsp[(1) - (2)].token_list);
		_token_list_append ((yyval.token_list), (yyvsp[(2) - (2)].token));
	;}
    break;

  case 80:
#line 679 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_str (parser, IDENTIFIER, (yyvsp[(1) - (1)].str));
		(yyval.token)->location = yylloc;
	;}
    break;

  case 81:
#line 683 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_str (parser, INTEGER_STRING, (yyvsp[(1) - (1)].str));
		(yyval.token)->location = yylloc;
	;}
    break;

  case 82:
#line 687 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_ival (parser, (yyvsp[(1) - (1)].ival), (yyvsp[(1) - (1)].ival));
		(yyval.token)->location = yylloc;
	;}
    break;

  case 83:
#line 691 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_ival (parser, DEFINED, DEFINED);
		(yyval.token)->location = yylloc;
	;}
    break;

  case 84:
#line 695 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_str (parser, OTHER, (yyvsp[(1) - (1)].str));
		(yyval.token)->location = yylloc;
	;}
    break;

  case 85:
#line 699 "src/glsl/glcpp/glcpp-parse.y"
    {
		(yyval.token) = _token_create_ival (parser, SPACE, SPACE);
		(yyval.token)->location = yylloc;
	;}
    break;

  case 86:
#line 706 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '['; ;}
    break;

  case 87:
#line 707 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = ']'; ;}
    break;

  case 88:
#line 708 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '('; ;}
    break;

  case 89:
#line 709 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = ')'; ;}
    break;

  case 90:
#line 710 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '{'; ;}
    break;

  case 91:
#line 711 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '}'; ;}
    break;

  case 92:
#line 712 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '.'; ;}
    break;

  case 93:
#line 713 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '&'; ;}
    break;

  case 94:
#line 714 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '*'; ;}
    break;

  case 95:
#line 715 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '+'; ;}
    break;

  case 96:
#line 716 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '-'; ;}
    break;

  case 97:
#line 717 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '~'; ;}
    break;

  case 98:
#line 718 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '!'; ;}
    break;

  case 99:
#line 719 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '/'; ;}
    break;

  case 100:
#line 720 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '%'; ;}
    break;

  case 101:
#line 721 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = LEFT_SHIFT; ;}
    break;

  case 102:
#line 722 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = RIGHT_SHIFT; ;}
    break;

  case 103:
#line 723 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '<'; ;}
    break;

  case 104:
#line 724 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '>'; ;}
    break;

  case 105:
#line 725 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = LESS_OR_EQUAL; ;}
    break;

  case 106:
#line 726 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = GREATER_OR_EQUAL; ;}
    break;

  case 107:
#line 727 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = EQUAL; ;}
    break;

  case 108:
#line 728 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = NOT_EQUAL; ;}
    break;

  case 109:
#line 729 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '^'; ;}
    break;

  case 110:
#line 730 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '|'; ;}
    break;

  case 111:
#line 731 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = AND; ;}
    break;

  case 112:
#line 732 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = OR; ;}
    break;

  case 113:
#line 733 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = ';'; ;}
    break;

  case 114:
#line 734 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = ','; ;}
    break;

  case 115:
#line 735 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = '='; ;}
    break;

  case 116:
#line 736 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = PASTE; ;}
    break;

  case 117:
#line 737 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = PLUS_PLUS; ;}
    break;

  case 118:
#line 738 "src/glsl/glcpp/glcpp-parse.y"
    { (yyval.ival) = MINUS_MINUS; ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2789 "src/glsl/glcpp/glcpp-parse.c"
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
      yyerror (&yylloc, parser, YY_("syntax error"));
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
	    yyerror (&yylloc, parser, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, parser, YY_("syntax error"));
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
		      yytoken, &yylval, &yylloc, parser);
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
		  yystos[yystate], yyvsp, yylsp, parser);
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
  yyerror (&yylloc, parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, parser);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, parser);
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


#line 741 "src/glsl/glcpp/glcpp-parse.y"


string_list_t *
_string_list_create (void *ctx)
{
	string_list_t *list;

	list = ralloc (ctx, string_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_string_list_append_item (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = ralloc (list, string_node_t);
	node->str = ralloc_strdup (node, str);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_string_list_contains (string_list_t *list, const char *member, int *index)
{
	string_node_t *node;
	int i;

	if (list == NULL)
		return 0;

	for (i = 0, node = list->head; node; i++, node = node->next) {
		if (strcmp (node->str, member) == 0) {
			if (index)
				*index = i;
			return 1;
		}
	}

	return 0;
}

/* Return duplicate string in list (if any), NULL otherwise. */
const char *
_string_list_has_duplicate (string_list_t *list)
{
	string_node_t *node, *dup;

	if (list == NULL)
		return NULL;

	for (node = list->head; node; node = node->next) {
		for (dup = node->next; dup; dup = dup->next) {
			if (strcmp (node->str, dup->str) == 0)
				return node->str;
		}
	}

	return NULL;
}

int
_string_list_length (string_list_t *list)
{
	int length = 0;
	string_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

int
_string_list_equal (string_list_t *a, string_list_t *b)
{
	string_node_t *node_a, *node_b;

	if (a == NULL && b == NULL)
		return 1;

	if (a == NULL || b == NULL)
		return 0;

	for (node_a = a->head, node_b = b->head;
	     node_a && node_b;
	     node_a = node_a->next, node_b = node_b->next)
	{
		if (strcmp (node_a->str, node_b->str))
			return 0;
	}

	/* Catch the case of lists being different lengths, (which
	 * would cause the loop above to terminate after the shorter
	 * list). */
	return node_a == node_b;
}

argument_list_t *
_argument_list_create (void *ctx)
{
	argument_list_t *list;

	list = ralloc (ctx, argument_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_argument_list_append (argument_list_t *list, token_list_t *argument)
{
	argument_node_t *node;

	node = ralloc (list, argument_node_t);
	node->argument = argument;

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_argument_list_length (argument_list_t *list)
{
	int length = 0;
	argument_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

token_list_t *
_argument_list_member_at (argument_list_t *list, int index)
{
	argument_node_t *node;
	int i;

	if (list == NULL)
		return NULL;

	node = list->head;
	for (i = 0; i < index; i++) {
		node = node->next;
		if (node == NULL)
			break;
	}

	if (node)
		return node->argument;

	return NULL;
}

/* Note: This function ralloc_steal()s the str pointer. */
token_t *
_token_create_str (void *ctx, int type, char *str)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.str = str;

	ralloc_steal (token, str);

	return token;
}

token_t *
_token_create_ival (void *ctx, int type, int ival)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.ival = ival;

	return token;
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = ralloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;
	list->non_space_tail = NULL;

	return list;
}

void
_token_list_append (token_list_t *list, token_t *token)
{
	token_node_t *node;

	node = ralloc (list, token_node_t);
	node->token = token;
	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
	if (token->type != SPACE)
		list->non_space_tail = node;
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (tail == NULL || tail->head == NULL)
		return;

	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
	list->non_space_tail = tail->non_space_tail;
}

static token_list_t *
_token_list_copy (void *ctx, token_list_t *other)
{
	token_list_t *copy;
	token_node_t *node;

	if (other == NULL)
		return NULL;

	copy = _token_list_create (ctx);
	for (node = other->head; node; node = node->next) {
		token_t *new_token = ralloc (copy, token_t);
		*new_token = *node->token;
		_token_list_append (copy, new_token);
	}

	return copy;
}

static void
_token_list_trim_trailing_space (token_list_t *list)
{
	token_node_t *tail, *next;

	if (list->non_space_tail) {
		tail = list->non_space_tail->next;
		list->non_space_tail->next = NULL;
		list->tail = list->non_space_tail;

		while (tail) {
			next = tail->next;
			ralloc_free (tail);
			tail = next;
		}
	}
}

static int
_token_list_is_empty_ignoring_space (token_list_t *l)
{
	token_node_t *n;

	if (l == NULL)
		return 1;

	n = l->head;
	while (n != NULL && n->token->type == SPACE)
		n = n->next;

	return n == NULL;
}

int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b)
{
	token_node_t *node_a, *node_b;

	if (a == NULL || b == NULL) {
		int a_empty = _token_list_is_empty_ignoring_space(a);
		int b_empty = _token_list_is_empty_ignoring_space(b);
		return a_empty == b_empty;
	}

	node_a = a->head;
	node_b = b->head;

	while (1)
	{
		if (node_a == NULL && node_b == NULL)
			break;

		if (node_a == NULL || node_b == NULL)
			return 0;
		/* Make sure whitespace appears in the same places in both.
		 * It need not be exactly the same amount of whitespace,
		 * though.
		 */
		if (node_a->token->type == SPACE
		    && node_b->token->type == SPACE) {
			while (node_a->token->type == SPACE)
				node_a = node_a->next;
			while (node_b->token->type == SPACE)
				node_b = node_b->next;
			continue;
		}

		if (node_a->token->type != node_b->token->type)
			return 0;

		switch (node_a->token->type) {
		case INTEGER:
			if (node_a->token->value.ival != 
			    node_b->token->value.ival)
			{
				return 0;
			}
			break;
		case IDENTIFIER:
		case INTEGER_STRING:
		case OTHER:
			if (strcmp (node_a->token->value.str,
				    node_b->token->value.str))
			{
				return 0;
			}
			break;
		}

		node_a = node_a->next;
		node_b = node_b->next;
	}

	return 1;
}

static void
_token_print (char **out, size_t *len, token_t *token)
{
	if (token->type < 256) {
		ralloc_asprintf_rewrite_tail (out, len, "%c", token->type);
		return;
	}

	switch (token->type) {
	case INTEGER:
		ralloc_asprintf_rewrite_tail (out, len, "%" PRIiMAX, token->value.ival);
		break;
	case IDENTIFIER:
	case INTEGER_STRING:
	case OTHER:
		ralloc_asprintf_rewrite_tail (out, len, "%s", token->value.str);
		break;
	case SPACE:
		ralloc_asprintf_rewrite_tail (out, len, " ");
		break;
	case LEFT_SHIFT:
		ralloc_asprintf_rewrite_tail (out, len, "<<");
		break;
	case RIGHT_SHIFT:
		ralloc_asprintf_rewrite_tail (out, len, ">>");
		break;
	case LESS_OR_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "<=");
		break;
	case GREATER_OR_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, ">=");
		break;
	case EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "==");
		break;
	case NOT_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "!=");
		break;
	case AND:
		ralloc_asprintf_rewrite_tail (out, len, "&&");
		break;
	case OR:
		ralloc_asprintf_rewrite_tail (out, len, "||");
		break;
	case PASTE:
		ralloc_asprintf_rewrite_tail (out, len, "##");
		break;
        case PLUS_PLUS:
		ralloc_asprintf_rewrite_tail (out, len, "++");
		break;
        case MINUS_MINUS:
		ralloc_asprintf_rewrite_tail (out, len, "--");
		break;
	case DEFINED:
		ralloc_asprintf_rewrite_tail (out, len, "defined");
		break;
	case PLACEHOLDER:
		/* Nothing to print. */
		break;
	default:
		assert(!"Error: Don't know how to print token.");

		break;
	}
}

/* Return a new token (ralloc()ed off of 'token') formed by pasting
 * 'token' and 'other'. Note that this function may return 'token' or
 * 'other' directly rather than allocating anything new.
 *
 * Caution: Only very cursory error-checking is performed to see if
 * the final result is a valid single token. */
static token_t *
_token_paste (glcpp_parser_t *parser, token_t *token, token_t *other)
{
	token_t *combined = NULL;

	/* Pasting a placeholder onto anything makes no change. */
	if (other->type == PLACEHOLDER)
		return token;

	/* When 'token' is a placeholder, just return 'other'. */
	if (token->type == PLACEHOLDER)
		return other;

	/* A very few single-character punctuators can be combined
	 * with another to form a multi-character punctuator. */
	switch (token->type) {
	case '<':
		if (other->type == '<')
			combined = _token_create_ival (token, LEFT_SHIFT, LEFT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, LESS_OR_EQUAL, LESS_OR_EQUAL);
		break;
	case '>':
		if (other->type == '>')
			combined = _token_create_ival (token, RIGHT_SHIFT, RIGHT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, GREATER_OR_EQUAL, GREATER_OR_EQUAL);
		break;
	case '=':
		if (other->type == '=')
			combined = _token_create_ival (token, EQUAL, EQUAL);
		break;
	case '!':
		if (other->type == '=')
			combined = _token_create_ival (token, NOT_EQUAL, NOT_EQUAL);
		break;
	case '&':
		if (other->type == '&')
			combined = _token_create_ival (token, AND, AND);
		break;
	case '|':
		if (other->type == '|')
			combined = _token_create_ival (token, OR, OR);
		break;
	}

	if (combined != NULL) {
		/* Inherit the location from the first token */
		combined->location = token->location;
		return combined;
	}

	/* Two string-valued (or integer) tokens can usually just be
	 * mashed together. (We also handle a string followed by an
	 * integer here as well.)
	 *
	 * There are some exceptions here. Notably, if the first token
	 * is an integer (or a string representing an integer), then
	 * the second token must also be an integer or must be a
	 * string representing an integer that begins with a digit.
	 */
	if ((token->type == IDENTIFIER || token->type == OTHER || token->type == INTEGER_STRING || token->type == INTEGER) &&
	    (other->type == IDENTIFIER || other->type == OTHER || other->type == INTEGER_STRING || other->type == INTEGER))
	{
		char *str;
		int combined_type;

		/* Check that pasting onto an integer doesn't create a
		 * non-integer, (that is, only digits can be
		 * pasted. */
		if (token->type == INTEGER_STRING || token->type == INTEGER)
		{
			switch (other->type) {
			case INTEGER_STRING:
				if (other->value.str[0] < '0' ||
				    other->value.str[0] > '9')
					goto FAIL;
				break;
			case INTEGER:
				if (other->value.ival < 0)
					goto FAIL;
				break;
			default:
				goto FAIL;
			}
		}

		if (token->type == INTEGER)
			str = ralloc_asprintf (token, "%" PRIiMAX,
					       token->value.ival);
		else
			str = ralloc_strdup (token, token->value.str);
					       

		if (other->type == INTEGER)
			ralloc_asprintf_append (&str, "%" PRIiMAX,
						other->value.ival);
		else
			ralloc_strcat (&str, other->value.str);

		/* New token is same type as original token, unless we
		 * started with an integer, in which case we will be
		 * creating an integer-string. */
		combined_type = token->type;
		if (combined_type == INTEGER)
			combined_type = INTEGER_STRING;

		combined = _token_create_str (token, combined_type, str);
		combined->location = token->location;
		return combined;
	}

    FAIL:
	glcpp_error (&token->location, parser, "");
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "Pasting \"");
	_token_print (&parser->info_log, &parser->info_log_length, token);
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "\" and \"");
	_token_print (&parser->info_log, &parser->info_log_length, other);
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "\" does not give a valid preprocessing token.\n");

	return token;
}

static void
_token_list_print (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next)
		_token_print (&parser->output, &parser->output_length, node->token);
}

void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error)
{
	glcpp_error(locp, parser, "%s", error);
}

static void add_builtin_define(glcpp_parser_t *parser,
			       const char *name, int value)
{
   token_t *tok;
   token_list_t *list;

   tok = _token_create_ival (parser, INTEGER, value);

   list = _token_list_create(parser);
   _token_list_append(list, tok);
   _define_object_macro(parser, NULL, name, list);
}

glcpp_parser_t *
glcpp_parser_create (const struct gl_extensions *extensions, gl_api api)
{
	glcpp_parser_t *parser;

	parser = ralloc (NULL, glcpp_parser_t);

	glcpp_lex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->active = NULL;
	parser->lexing_directive = 0;
	parser->space_tokens = 1;
	parser->last_token_was_newline = 0;
	parser->last_token_was_space = 0;
	parser->first_non_space_token_this_line = 1;
	parser->newline_as_space = 0;
	parser->in_control_line = 0;
	parser->paren_count = 0;
        parser->commented_newlines = 0;

	parser->skip_stack = NULL;
	parser->skipping = 0;

	parser->lex_from_list = NULL;
	parser->lex_from_node = NULL;

	parser->output = ralloc_strdup(parser, "");
	parser->output_length = 0;
	parser->info_log = ralloc_strdup(parser, "");
	parser->info_log_length = 0;
	parser->error = 0;

        parser->extensions = extensions;
        parser->api = api;
        parser->version_resolved = false;

	parser->has_new_line_number = 0;
	parser->new_line_number = 1;
	parser->has_new_source_number = 0;
	parser->new_source_number = 0;

	return parser;
}

void
glcpp_parser_destroy (glcpp_parser_t *parser)
{
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	ralloc_free (parser);
}

typedef enum function_status
{
	FUNCTION_STATUS_SUCCESS,
	FUNCTION_NOT_A_FUNCTION,
	FUNCTION_UNBALANCED_PARENTHESES
} function_status_t;

/* Find a set of function-like macro arguments by looking for a
 * balanced set of parentheses.
 *
 * When called, 'node' should be the opening-parenthesis token, (or
 * perhaps preceeding SPACE tokens). Upon successful return *last will
 * be the last consumed node, (corresponding to the closing right
 * parenthesis).
 *
 * Return values:
 *
 *   FUNCTION_STATUS_SUCCESS:
 *
 *	Successfully parsed a set of function arguments.	
 *
 *   FUNCTION_NOT_A_FUNCTION:
 *
 *	Macro name not followed by a '('. This is not an error, but
 *	simply that the macro name should be treated as a non-macro.
 *
 *   FUNCTION_UNBALANCED_PARENTHESES
 *
 *	Macro name is not followed by a balanced set of parentheses.
 */
static function_status_t
_arguments_parse (argument_list_t *arguments,
		  token_node_t *node,
		  token_node_t **last)
{
	token_list_t *argument;
	int paren_count;

	node = node->next;

	/* Ignore whitespace before first parenthesis. */
	while (node && node->token->type == SPACE)
		node = node->next;

	if (node == NULL || node->token->type != '(')
		return FUNCTION_NOT_A_FUNCTION;

	node = node->next;

	argument = _token_list_create (arguments);
	_argument_list_append (arguments, argument);

	for (paren_count = 1; node; node = node->next) {
		if (node->token->type == '(')
		{
			paren_count++;
		}
		else if (node->token->type == ')')
		{
			paren_count--;
			if (paren_count == 0)
				break;
		}

		if (node->token->type == ',' &&
			 paren_count == 1)
		{
			_token_list_trim_trailing_space (argument);
			argument = _token_list_create (arguments);
			_argument_list_append (arguments, argument);
		}
		else {
			if (argument->head == NULL) {
				/* Don't treat initial whitespace as
				 * part of the argument. */
				if (node->token->type == SPACE)
					continue;
			}
			_token_list_append (argument, node->token);
		}
	}

	if (paren_count)
		return FUNCTION_UNBALANCED_PARENTHESES;

	*last = node;

	return FUNCTION_STATUS_SUCCESS;
}

static token_list_t *
_token_list_create_with_one_ival (void *ctx, int type, int ival)
{
	token_list_t *list;
	token_t *node;

	list = _token_list_create (ctx);
	node = _token_create_ival (list, type, ival);
	_token_list_append (list, node);

	return list;
}

static token_list_t *
_token_list_create_with_one_space (void *ctx)
{
	return _token_list_create_with_one_ival (ctx, SPACE, SPACE);
}

static token_list_t *
_token_list_create_with_one_integer (void *ctx, int ival)
{
	return _token_list_create_with_one_ival (ctx, INTEGER, ival);
}

/* Evaluate a DEFINED token node (based on subsequent tokens in the list).
 *
 * Note: This function must only be called when "node" is a DEFINED token,
 * (and will abort with an assertion failure otherwise).
 *
 * If "node" is followed, (ignoring any SPACE tokens), by an IDENTIFIER token
 * (optionally preceded and followed by '(' and ')' tokens) then the following
 * occurs:
 *
 *	If the identifier is a defined macro, this function returns 1.
 *
 *	If the identifier is not a defined macro, this function returns 0.
 *
 *	In either case, *last will be updated to the last node in the list
 *	consumed by the evaluation, (either the token of the identifier or the
 *	token of the closing parenthesis).
 *
 * In all other cases, (such as "node is the final node of the list", or
 * "missing closing parenthesis", etc.), this function generates a
 * preprocessor error, returns -1 and *last will not be set.
 */
static int
_glcpp_parser_evaluate_defined (glcpp_parser_t *parser,
				token_node_t *node,
				token_node_t **last)
{
	token_node_t *argument, *defined = node;

	assert (node->token->type == DEFINED);

	node = node->next;

	/* Ignore whitespace after DEFINED token. */
	while (node && node->token->type == SPACE)
		node = node->next;

	if (node == NULL)
		goto FAIL;

	if (node->token->type == IDENTIFIER || node->token->type == OTHER) {
		argument = node;
	} else if (node->token->type == '(') {
		node = node->next;

		/* Ignore whitespace after '(' token. */
		while (node && node->token->type == SPACE)
			node = node->next;

		if (node == NULL || (node->token->type != IDENTIFIER &&
				     node->token->type != OTHER))
		{
			goto FAIL;
		}

		argument = node;

		node = node->next;

		/* Ignore whitespace after identifier, before ')' token. */
		while (node && node->token->type == SPACE)
			node = node->next;

		if (node == NULL || node->token->type != ')')
			goto FAIL;
	} else {
		goto FAIL;
	}

	*last = node;

	return hash_table_find (parser->defines,
				argument->token->value.str) ? 1 : 0;

FAIL:
	glcpp_error (&defined->token->location, parser,
		     "\"defined\" not followed by an identifier");
	return -1;
}

/* Evaluate all DEFINED nodes in a given list, modifying the list in place.
 */
static void
_glcpp_parser_evaluate_defined_in_list (glcpp_parser_t *parser,
					token_list_t *list)
{
	token_node_t *node, *node_prev, *replacement, *last = NULL;
	int value;

	if (list == NULL)
		return;

	node_prev = NULL;
	node = list->head;

	while (node) {

		if (node->token->type != DEFINED)
			goto NEXT;

		value = _glcpp_parser_evaluate_defined (parser, node, &last);
		if (value == -1)
			goto NEXT;

		replacement = ralloc (list, token_node_t);
		replacement->token = _token_create_ival (list, INTEGER, value);

		/* Splice replacement node into list, replacing from "node"
		 * through "last". */
		if (node_prev)
			node_prev->next = replacement;
		else
			list->head = replacement;
		replacement->next = last->next;
		if (last == list->tail)
			list->tail = replacement;

		node = replacement;

	NEXT:
		node_prev = node;
		node = node->next;
	}
}

/* Perform macro expansion on 'list', placing the resulting tokens
 * into a new list which is initialized with a first token of type
 * 'head_token_type'. Then begin lexing from the resulting list,
 * (return to the current lexing source when this list is exhausted).
 *
 * See the documentation of _glcpp_parser_expand_token_list for a description
 * of the "mode" parameter.
 */
static void
_glcpp_parser_expand_and_lex_from (glcpp_parser_t *parser,
				   int head_token_type,
				   token_list_t *list,
				   expansion_mode_t mode)
{
	token_list_t *expanded;
	token_t *token;

	expanded = _token_list_create (parser);
	token = _token_create_ival (parser, head_token_type, head_token_type);
	_token_list_append (expanded, token);
	_glcpp_parser_expand_token_list (parser, list, mode);
	_token_list_append_list (expanded, list);
	glcpp_parser_lex_from (parser, expanded);
}

static void
_glcpp_parser_apply_pastes (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	node = list->head;
	while (node)
	{
		token_node_t *next_non_space;

		/* Look ahead for a PASTE token, skipping space. */
		next_non_space = node->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL)
			break;

		if (next_non_space->token->type != PASTE) {
			node = next_non_space;
			continue;
		}

		/* Now find the next non-space token after the PASTE. */
		next_non_space = next_non_space->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL) {
			yyerror (&node->token->location, parser, "'##' cannot appear at either end of a macro expansion\n");
			return;
		}

		node->token = _token_paste (parser, node->token, next_non_space->token);
		node->next = next_non_space->next;
		if (next_non_space == list->tail)
			list->tail = node;
	}

	list->non_space_tail = list->tail;
}

/* This is a helper function that's essentially part of the
 * implementation of _glcpp_parser_expand_node. It shouldn't be called
 * except for by that function.
 *
 * Returns NULL if node is a simple token with no expansion, (that is,
 * although 'node' corresponds to an identifier defined as a
 * function-like macro, it is not followed with a parenthesized
 * argument list).
 *
 * Compute the complete expansion of node (which is a function-like
 * macro) and subsequent nodes which are arguments.
 *
 * Returns the token list that results from the expansion and sets
 * *last to the last node in the list that was consumed by the
 * expansion. Specifically, *last will be set as follows: as the
 * token of the closing right parenthesis.
 *
 * See the documentation of _glcpp_parser_expand_token_list for a description
 * of the "mode" parameter.
 */
static token_list_t *
_glcpp_parser_expand_function (glcpp_parser_t *parser,
			       token_node_t *node,
			       token_node_t **last,
			       expansion_mode_t mode)
{
	macro_t *macro;
	const char *identifier;
	argument_list_t *arguments;
	function_status_t status;
	token_list_t *substituted;
	int parameter_index;

	identifier = node->token->value.str;

	macro = hash_table_find (parser->defines, identifier);

	assert (macro->is_function);

	arguments = _argument_list_create (parser);
	status = _arguments_parse (arguments, node, last);

	switch (status) {
	case FUNCTION_STATUS_SUCCESS:
		break;
	case FUNCTION_NOT_A_FUNCTION:
		return NULL;
	case FUNCTION_UNBALANCED_PARENTHESES:
		glcpp_error (&node->token->location, parser, "Macro %s call has unbalanced parentheses\n", identifier);
		return NULL;
	}

	/* Replace a macro defined as empty with a SPACE token. */
	if (macro->replacements == NULL) {
		ralloc_free (arguments);
		return _token_list_create_with_one_space (parser);
	}

	if (! ((_argument_list_length (arguments) == 
		_string_list_length (macro->parameters)) ||
	       (_string_list_length (macro->parameters) == 0 &&
		_argument_list_length (arguments) == 1 &&
		arguments->head->argument->head == NULL)))
	{
		glcpp_error (&node->token->location, parser,
			      "Error: macro %s invoked with %d arguments (expected %d)\n",
			      identifier,
			      _argument_list_length (arguments),
			      _string_list_length (macro->parameters));
		return NULL;
	}

	/* Perform argument substitution on the replacement list. */
	substituted = _token_list_create (arguments);

	for (node = macro->replacements->head; node; node = node->next)
	{
		if (node->token->type == IDENTIFIER &&
		    _string_list_contains (macro->parameters,
					   node->token->value.str,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			/* Before substituting, we expand the argument
			 * tokens, or append a placeholder token for
			 * an empty argument. */
			if (argument->head) {
				token_list_t *expanded_argument;
				expanded_argument = _token_list_copy (parser,
								      argument);
				_glcpp_parser_expand_token_list (parser,
								 expanded_argument,
								 mode);
				_token_list_append_list (substituted,
							 expanded_argument);
			} else {
				token_t *new_token;

				new_token = _token_create_ival (substituted,
								PLACEHOLDER,
								PLACEHOLDER);
				_token_list_append (substituted, new_token);
			}
		} else {
			_token_list_append (substituted, node->token);
		}
	}

	/* After argument substitution, and before further expansion
	 * below, implement token pasting. */

	_token_list_trim_trailing_space (substituted);

	_glcpp_parser_apply_pastes (parser, substituted);

	return substituted;
}

/* Compute the complete expansion of node, (and subsequent nodes after
 * 'node' in the case that 'node' is a function-like macro and
 * subsequent nodes are arguments).
 *
 * Returns NULL if node is a simple token with no expansion.
 *
 * Otherwise, returns the token list that results from the expansion
 * and sets *last to the last node in the list that was consumed by
 * the expansion. Specifically, *last will be set as follows:
 *
 *	As 'node' in the case of object-like macro expansion.
 *
 *	As the token of the closing right parenthesis in the case of
 *	function-like macro expansion.
 *
 * See the documentation of _glcpp_parser_expand_token_list for a description
 * of the "mode" parameter.
 */
static token_list_t *
_glcpp_parser_expand_node (glcpp_parser_t *parser,
			   token_node_t *node,
			   token_node_t **last,
			   expansion_mode_t mode)
{
	token_t *token = node->token;
	const char *identifier;
	macro_t *macro;

	/* We only expand identifiers */
	if (token->type != IDENTIFIER) {
		return NULL;
	}

	*last = node;
	identifier = token->value.str;

	/* Special handling for __LINE__ and __FILE__, (not through
	 * the hash table). */
	if (strcmp(identifier, "__LINE__") == 0)
		return _token_list_create_with_one_integer (parser, node->token->location.first_line);

	if (strcmp(identifier, "__FILE__") == 0)
		return _token_list_create_with_one_integer (parser, node->token->location.source);

	/* Look up this identifier in the hash table. */
	macro = hash_table_find (parser->defines, identifier);

	/* Not a macro, so no expansion needed. */
	if (macro == NULL)
		return NULL;

	/* Finally, don't expand this macro if we're already actively
	 * expanding it, (to avoid infinite recursion). */
	if (_parser_active_list_contains (parser, identifier)) {
		/* We change the token type here from IDENTIFIER to
		 * OTHER to prevent any future expansion of this
		 * unexpanded token. */
		char *str;
		token_list_t *expansion;
		token_t *final;

		str = ralloc_strdup (parser, token->value.str);
		final = _token_create_str (parser, OTHER, str);
		expansion = _token_list_create (parser);
		_token_list_append (expansion, final);
		return expansion;
	}

	if (! macro->is_function)
	{
		token_list_t *replacement;

		/* Replace a macro defined as empty with a SPACE token. */
		if (macro->replacements == NULL)
			return _token_list_create_with_one_space (parser);

		replacement = _token_list_copy (parser, macro->replacements);
		_glcpp_parser_apply_pastes (parser, replacement);
		return replacement;
	}

	return _glcpp_parser_expand_function (parser, node, last, mode);
}

/* Push a new identifier onto the parser's active list.
 *
 * Here, 'marker' is the token node that appears in the list after the
 * expansion of 'identifier'. That is, when the list iterator begins
 * examining 'marker', then it is time to pop this node from the
 * active stack.
 */
static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker)
{
	active_list_t *node;

	node = ralloc (parser->active, active_list_t);
	node->identifier = ralloc_strdup (node, identifier);
	node->marker = marker;
	node->next = parser->active;

	parser->active = node;
}

static void
_parser_active_list_pop (glcpp_parser_t *parser)
{
	active_list_t *node = parser->active;

	if (node == NULL) {
		parser->active = NULL;
		return;
	}

	node = parser->active->next;
	ralloc_free (parser->active);

	parser->active = node;
}

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier)
{
	active_list_t *node;

	if (parser->active == NULL)
		return 0;

	for (node = parser->active; node; node = node->next)
		if (strcmp (node->identifier, identifier) == 0)
			return 1;

	return 0;
}

/* Walk over the token list replacing nodes with their expansion.
 * Whenever nodes are expanded the walking will walk over the new
 * nodes, continuing to expand as necessary. The results are placed in
 * 'list' itself.
 *
 * The "mode" argument controls the handling of any DEFINED tokens that
 * result from expansion as follows:
 *
 *	EXPANSION_MODE_IGNORE_DEFINED: Any resulting DEFINED tokens will be
 *		left in the final list, unevaluated. This is the correct mode
 *		for expanding any list in any context other than a
 *		preprocessor conditional, (#if or #elif).
 *
 *	EXPANSION_MODE_EVALUATE_DEFINED: Any resulting DEFINED tokens will be
 *		evaluated to 0 or 1 tokens depending on whether the following
 *		token is the name of a defined macro. If the DEFINED token is
 *		not followed by an (optionally parenthesized) identifier, then
 *		an error will be generated. This the correct mode for
 *		expanding any list in the context of a preprocessor
 *		conditional, (#if or #elif).
 */
static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list,
				 expansion_mode_t mode)
{
	token_node_t *node_prev;
	token_node_t *node, *last = NULL;
	token_list_t *expansion;
	active_list_t *active_initial = parser->active;

	if (list == NULL)
		return;

	_token_list_trim_trailing_space (list);

	node_prev = NULL;
	node = list->head;

	if (mode == EXPANSION_MODE_EVALUATE_DEFINED)
		_glcpp_parser_evaluate_defined_in_list (parser, list);

	while (node) {

		while (parser->active && parser->active->marker == node)
			_parser_active_list_pop (parser);

		expansion = _glcpp_parser_expand_node (parser, node, &last, mode);
		if (expansion) {
			token_node_t *n;

			if (mode == EXPANSION_MODE_EVALUATE_DEFINED) {
				_glcpp_parser_evaluate_defined_in_list (parser,
									expansion);
			}

			for (n = node; n != last->next; n = n->next)
				while (parser->active &&
				       parser->active->marker == n)
				{
					_parser_active_list_pop (parser);
				}

			_parser_active_list_push (parser,
						  node->token->value.str,
						  last->next);
			
			/* Splice expansion into list, supporting a
			 * simple deletion if the expansion is
			 * empty. */
			if (expansion->head) {
				if (node_prev)
					node_prev->next = expansion->head;
				else
					list->head = expansion->head;
				expansion->tail->next = last->next;
				if (last == list->tail)
					list->tail = expansion->tail;
			} else {
				if (node_prev)
					node_prev->next = last->next;
				else
					list->head = last->next;
				if (last == list->tail)
					list->tail = NULL;
			}
		} else {
			node_prev = node;
		}
		node = node_prev ? node_prev->next : list->head;
	}

	/* Remove any lingering effects of this invocation on the
	 * active list. That is, pop until the list looks like it did
	 * at the beginning of this function. */
	while (parser->active && parser->active != active_initial)
		_parser_active_list_pop (parser);

	list->non_space_tail = list->tail;
}

void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list)
{
	if (list == NULL)
		return;

	_glcpp_parser_expand_token_list (parser, list, EXPANSION_MODE_IGNORE_DEFINED);

	_token_list_trim_trailing_space (list);

	_token_list_print (parser, list);
}

static void
_check_for_reserved_macro_name (glcpp_parser_t *parser, YYLTYPE *loc,
				const char *identifier)
{
	/* Section 3.3 (Preprocessor) of the GLSL 1.30 spec (and later) and
	 * the GLSL ES spec (all versions) say:
	 *
	 *     "All macro names containing two consecutive underscores ( __ )
	 *     are reserved for future use as predefined macro names. All
	 *     macro names prefixed with "GL_" ("GL" followed by a single
	 *     underscore) are also reserved."
	 *
	 * The intention is that names containing __ are reserved for internal
	 * use by the implementation, and names prefixed with GL_ are reserved
	 * for use by Khronos.  Since every extension adds a name prefixed
	 * with GL_ (i.e., the name of the extension), that should be an
	 * error.  Names simply containing __ are dangerous to use, but should
	 * be allowed.
	 *
	 * A future version of the GLSL specification will clarify this.
	 */
	if (strstr(identifier, "__")) {
		glcpp_warning(loc, parser,
			      "Macro names containing \"__\" are reserved "
			      "for use by the implementation.\n");
	}
	if (strncmp(identifier, "GL_", 3) == 0) {
		glcpp_error (loc, parser, "Macro names starting with \"GL_\" are reserved.\n");
	}
}

static int
_macro_equal (macro_t *a, macro_t *b)
{
	if (a->is_function != b->is_function)
		return 0;

	if (a->is_function) {
		if (! _string_list_equal (a->parameters, b->parameters))
			return 0;
	}

	return _token_list_equal_ignoring_space (a->replacements,
						 b->replacements);
}

void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *identifier,
		      token_list_t *replacements)
{
	macro_t *macro, *previous;

	/* We define pre-defined macros before we've started parsing the
         * actual file. So if there's no location defined yet, that's what
         * were doing and we don't want to generate an error for using the
         * reserved names. */
	if (loc != NULL)
		_check_for_reserved_macro_name(parser, loc, identifier);

	macro = ralloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	ralloc_steal (macro, replacements);

	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *identifier,
			string_list_t *parameters,
			token_list_t *replacements)
{
	macro_t *macro, *previous;
	const char *dup;

	_check_for_reserved_macro_name(parser, loc, identifier);

        /* Check for any duplicate parameter names. */
	if ((dup = _string_list_has_duplicate (parameters)) != NULL) {
		glcpp_error (loc, parser, "Duplicate macro parameter \"%s\"",
			     dup);
	}

	macro = ralloc (parser, macro_t);
	ralloc_steal (macro, parameters);
	ralloc_steal (macro, replacements);

	macro->is_function = 1;
	macro->parameters = parameters;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser)
{
	token_node_t *node;
	int ret;

	if (parser->lex_from_list == NULL) {
		ret = glcpp_lex (yylval, yylloc, parser->scanner);

		/* XXX: This ugly block of code exists for the sole
		 * purpose of converting a NEWLINE token into a SPACE
		 * token, but only in the case where we have seen a
		 * function-like macro name, but have not yet seen its
		 * closing parenthesis.
		 *
		 * There's perhaps a more compact way to do this with
		 * mid-rule actions in the grammar.
		 *
		 * I'm definitely not pleased with the complexity of
		 * this code here.
		 */
		if (parser->newline_as_space)
		{
			if (ret == '(') {
				parser->paren_count++;
			} else if (ret == ')') {
				parser->paren_count--;
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			} else if (ret == NEWLINE) {
				ret = SPACE;
			} else if (ret != SPACE) {
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			}
		}
		else if (parser->in_control_line)
		{
			if (ret == NEWLINE)
				parser->in_control_line = 0;
		}
		else if (ret == DEFINE_TOKEN ||
			 ret == UNDEF || ret == IF ||
			 ret == IFDEF || ret == IFNDEF ||
			 ret == ELIF || ret == ELSE ||
			 ret == ENDIF || ret == HASH_TOKEN)
		{
			parser->in_control_line = 1;
		}
		else if (ret == IDENTIFIER)
		{
			macro_t *macro;
			macro = hash_table_find (parser->defines,
						 yylval->str);
			if (macro && macro->is_function) {
				parser->newline_as_space = 1;
				parser->paren_count = 0;
			}
		}

		return ret;
	}

	node = parser->lex_from_node;

	if (node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
		return NEWLINE;
	}

	*yylval = node->token->value;
	ret = node->token->type;

	parser->lex_from_node = node->next;

	return ret;
}

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	assert (parser->lex_from_list == NULL);

	/* Copy list, eliminating any space tokens. */
	parser->lex_from_list = _token_list_create (parser);

	for (node = list->head; node; node = node->next) {
		if (node->token->type == SPACE)
			continue;
		_token_list_append (parser->lex_from_list, node->token);
	}

	ralloc_free (list);

	parser->lex_from_node = parser->lex_from_list->head;

	/* It's possible the list consisted of nothing but whitespace. */
	if (parser->lex_from_node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
	}
}

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition)
{
	skip_type_t current = SKIP_NO_SKIP;
	skip_node_t *node;

	if (parser->skip_stack)
		current = parser->skip_stack->type;

	node = ralloc (parser, skip_node_t);
	node->loc = *loc;

	if (current == SKIP_NO_SKIP) {
		if (condition)
			node->type = SKIP_NO_SKIP;
		else
			node->type = SKIP_TO_ELSE;
	} else {
		node->type = SKIP_TO_ENDIF;
	}

	node->has_else = false;
	node->next = parser->skip_stack;
	parser->skip_stack = node;
}

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition)
{
	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "#%s without #if\n", type);
		return;
	}

	if (parser->skip_stack->type == SKIP_TO_ELSE) {
		if (condition)
			parser->skip_stack->type = SKIP_NO_SKIP;
	} else {
		parser->skip_stack->type = SKIP_TO_ENDIF;
	}
}

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc)
{
	skip_node_t *node;

	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "#endif without #if\n");
		return;
	}

	node = parser->skip_stack;
	parser->skip_stack = node->next;
	ralloc_free (node);
}

static void
_glcpp_parser_handle_version_declaration(glcpp_parser_t *parser, intmax_t version,
                                         const char *es_identifier,
                                         bool explicitly_set)
{
	const struct gl_extensions *extensions = parser->extensions;

	if (parser->version_resolved)
		return;

	parser->version_resolved = true;

	add_builtin_define (parser, "__VERSION__", version);

	parser->is_gles = (version == 100) ||
			   (es_identifier &&
			    (strcmp(es_identifier, "es") == 0));

	/* Add pre-defined macros. */
	if (parser->is_gles) {
	   add_builtin_define(parser, "GL_ES", 1);
           add_builtin_define(parser, "GL_EXT_separate_shader_objects", 1);

	   if (extensions != NULL) {
	      if (extensions->OES_EGL_image_external)
	         add_builtin_define(parser, "GL_OES_EGL_image_external", 1);
              if (extensions->OES_standard_derivatives)
                 add_builtin_define(parser, "GL_OES_standard_derivatives", 1);
	   }
	} else {
	   add_builtin_define(parser, "GL_ARB_draw_buffers", 1);
           add_builtin_define(parser, "GL_ARB_separate_shader_objects", 1);
	   add_builtin_define(parser, "GL_ARB_texture_rectangle", 1);
           add_builtin_define(parser, "GL_AMD_shader_trinary_minmax", 1);


	   if (extensions != NULL) {
	      if (extensions->EXT_texture_array)
	         add_builtin_define(parser, "GL_EXT_texture_array", 1);

	      if (extensions->ARB_arrays_of_arrays)
	          add_builtin_define(parser, "GL_ARB_arrays_of_arrays", 1);

	      if (extensions->ARB_fragment_coord_conventions)
	         add_builtin_define(parser, "GL_ARB_fragment_coord_conventions",
				    1);

              if (extensions->ARB_fragment_layer_viewport)
                 add_builtin_define(parser, "GL_ARB_fragment_layer_viewport", 1);

	      if (extensions->ARB_explicit_attrib_location)
	         add_builtin_define(parser, "GL_ARB_explicit_attrib_location", 1);

	      if (extensions->ARB_explicit_uniform_location)
	         add_builtin_define(parser, "GL_ARB_explicit_uniform_location", 1);

	      if (extensions->ARB_shader_texture_lod)
	         add_builtin_define(parser, "GL_ARB_shader_texture_lod", 1);

	      if (extensions->ARB_draw_instanced)
	         add_builtin_define(parser, "GL_ARB_draw_instanced", 1);

	      if (extensions->EXT_draw_instanced)
	         add_builtin_define(parser, "GL_EXT_draw_instanced", 1);

	      if (extensions->ARB_conservative_depth) {
	         add_builtin_define(parser, "GL_AMD_conservative_depth", 1);
	         add_builtin_define(parser, "GL_ARB_conservative_depth", 1);
	      }

	      if (extensions->ARB_shader_bit_encoding)
	         add_builtin_define(parser, "GL_ARB_shader_bit_encoding", 1);

	      if (extensions->ARB_uniform_buffer_object)
	         add_builtin_define(parser, "GL_ARB_uniform_buffer_object", 1);

	      if (extensions->ARB_texture_cube_map_array)
	         add_builtin_define(parser, "GL_ARB_texture_cube_map_array", 1);

	      if (extensions->ARB_shading_language_packing)
	         add_builtin_define(parser, "GL_ARB_shading_language_packing", 1);

	      if (extensions->ARB_texture_multisample)
	         add_builtin_define(parser, "GL_ARB_texture_multisample", 1);

	      if (extensions->ARB_texture_query_levels)
	         add_builtin_define(parser, "GL_ARB_texture_query_levels", 1);

	      if (extensions->ARB_texture_query_lod)
	         add_builtin_define(parser, "GL_ARB_texture_query_lod", 1);

	      if (extensions->ARB_gpu_shader5)
	         add_builtin_define(parser, "GL_ARB_gpu_shader5", 1);

	      if (extensions->AMD_vertex_shader_layer)
	         add_builtin_define(parser, "GL_AMD_vertex_shader_layer", 1);

	      if (extensions->AMD_vertex_shader_viewport_index)
	         add_builtin_define(parser, "GL_AMD_vertex_shader_viewport_index", 1);

	      if (extensions->ARB_shading_language_420pack)
	         add_builtin_define(parser, "GL_ARB_shading_language_420pack", 1);

	      if (extensions->ARB_sample_shading)
	         add_builtin_define(parser, "GL_ARB_sample_shading", 1);

	      if (extensions->ARB_texture_gather)
	         add_builtin_define(parser, "GL_ARB_texture_gather", 1);

	      if (extensions->ARB_shader_atomic_counters)
	         add_builtin_define(parser, "GL_ARB_shader_atomic_counters", 1);

	      if (extensions->ARB_viewport_array)
	         add_builtin_define(parser, "GL_ARB_viewport_array", 1);

              if (extensions->ARB_compute_shader)
                 add_builtin_define(parser, "GL_ARB_compute_shader", 1);

	      if (extensions->ARB_shader_image_load_store)
	         add_builtin_define(parser, "GL_ARB_shader_image_load_store", 1);

              if (extensions->ARB_derivative_control)
                 add_builtin_define(parser, "GL_ARB_derivative_control", 1);
	   }
	}

	if (extensions != NULL) {
	   if (extensions->EXT_shader_integer_mix)
	      add_builtin_define(parser, "GL_EXT_shader_integer_mix", 1);
	}

	if (version >= 150)
		add_builtin_define(parser, "GL_core_profile", 1);

	/* Currently, all ES2/ES3 implementations support highp in the
	 * fragment shader, so we always define this macro in ES2/ES3.
	 * If we ever get a driver that doesn't support highp, we'll
	 * need to add a flag to the gl_context and check that here.
	 */
	if (version >= 130 || parser->is_gles)
		add_builtin_define (parser, "GL_FRAGMENT_PRECISION_HIGH", 1);

	if (explicitly_set) {
	   ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length,
					 "#version %" PRIiMAX "%s%s", version,
					 es_identifier ? " " : "",
					 es_identifier ? es_identifier : "");
	}
}

/* GLSL version if no version is explicitly specified. */
#define IMPLICIT_GLSL_VERSION 110
#define IMPLICIT_GLSL_ES_VERSION 100

/* GLSL ES version if no version is explicitly specified. */
#define IMPLICIT_GLSL_ES_VERSION 100

void
glcpp_parser_resolve_implicit_version(glcpp_parser_t *parser)
{
	int language_version = parser->api == API_OPENGLES2 ?
			       IMPLICIT_GLSL_ES_VERSION :
			       IMPLICIT_GLSL_VERSION;

	_glcpp_parser_handle_version_declaration(parser, language_version,
						 NULL, false);
}

