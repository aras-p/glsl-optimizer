/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ATTRIBUTE = 258,
     CONST_QUAL = 259,
     BOOL_TYPE = 260,
     FLOAT_TYPE = 261,
     INT_TYPE = 262,
     BREAK = 263,
     CONTINUE = 264,
     DO = 265,
     ELSE = 266,
     FOR = 267,
     IF = 268,
     DISCARD = 269,
     RETURN = 270,
     BVEC2 = 271,
     BVEC3 = 272,
     BVEC4 = 273,
     IVEC2 = 274,
     IVEC3 = 275,
     IVEC4 = 276,
     VEC2 = 277,
     VEC3 = 278,
     VEC4 = 279,
     MATRIX2 = 280,
     MATRIX3 = 281,
     MATRIX4 = 282,
     IN_QUAL = 283,
     OUT_QUAL = 284,
     INOUT_QUAL = 285,
     UNIFORM = 286,
     VARYING = 287,
     STRUCT = 288,
     VOID_TYPE = 289,
     WHILE = 290,
     SAMPLER1D = 291,
     SAMPLER2D = 292,
     SAMPLER3D = 293,
     SAMPLERCUBE = 294,
     SAMPLER1DSHADOW = 295,
     SAMPLER2DSHADOW = 296,
     IDENTIFIER = 297,
     TYPE_NAME = 298,
     FLOATCONSTANT = 299,
     INTCONSTANT = 300,
     BOOLCONSTANT = 301,
     FIELD_SELECTION = 302,
     LEFT_OP = 303,
     RIGHT_OP = 304,
     INC_OP = 305,
     DEC_OP = 306,
     LE_OP = 307,
     GE_OP = 308,
     EQ_OP = 309,
     NE_OP = 310,
     AND_OP = 311,
     OR_OP = 312,
     XOR_OP = 313,
     MUL_ASSIGN = 314,
     DIV_ASSIGN = 315,
     ADD_ASSIGN = 316,
     MOD_ASSIGN = 317,
     LEFT_ASSIGN = 318,
     RIGHT_ASSIGN = 319,
     AND_ASSIGN = 320,
     XOR_ASSIGN = 321,
     OR_ASSIGN = 322,
     SUB_ASSIGN = 323,
     LEFT_PAREN = 324,
     RIGHT_PAREN = 325,
     LEFT_BRACKET = 326,
     RIGHT_BRACKET = 327,
     LEFT_BRACE = 328,
     RIGHT_BRACE = 329,
     DOT = 330,
     COMMA = 331,
     COLON = 332,
     EQUAL = 333,
     SEMICOLON = 334,
     BANG = 335,
     DASH = 336,
     TILDE = 337,
     PLUS = 338,
     STAR = 339,
     SLASH = 340,
     PERCENT = 341,
     LEFT_ANGLE = 342,
     RIGHT_ANGLE = 343,
     VERTICAL_BAR = 344,
     CARET = 345,
     AMPERSAND = 346,
     QUESTION = 347
   };
#endif
#define ATTRIBUTE 258
#define CONST_QUAL 259
#define BOOL_TYPE 260
#define FLOAT_TYPE 261
#define INT_TYPE 262
#define BREAK 263
#define CONTINUE 264
#define DO 265
#define ELSE 266
#define FOR 267
#define IF 268
#define DISCARD 269
#define RETURN 270
#define BVEC2 271
#define BVEC3 272
#define BVEC4 273
#define IVEC2 274
#define IVEC3 275
#define IVEC4 276
#define VEC2 277
#define VEC3 278
#define VEC4 279
#define MATRIX2 280
#define MATRIX3 281
#define MATRIX4 282
#define IN_QUAL 283
#define OUT_QUAL 284
#define INOUT_QUAL 285
#define UNIFORM 286
#define VARYING 287
#define STRUCT 288
#define VOID_TYPE 289
#define WHILE 290
#define SAMPLER1D 291
#define SAMPLER2D 292
#define SAMPLER3D 293
#define SAMPLERCUBE 294
#define SAMPLER1DSHADOW 295
#define SAMPLER2DSHADOW 296
#define IDENTIFIER 297
#define TYPE_NAME 298
#define FLOATCONSTANT 299
#define INTCONSTANT 300
#define BOOLCONSTANT 301
#define FIELD_SELECTION 302
#define LEFT_OP 303
#define RIGHT_OP 304
#define INC_OP 305
#define DEC_OP 306
#define LE_OP 307
#define GE_OP 308
#define EQ_OP 309
#define NE_OP 310
#define AND_OP 311
#define OR_OP 312
#define XOR_OP 313
#define MUL_ASSIGN 314
#define DIV_ASSIGN 315
#define ADD_ASSIGN 316
#define MOD_ASSIGN 317
#define LEFT_ASSIGN 318
#define RIGHT_ASSIGN 319
#define AND_ASSIGN 320
#define XOR_ASSIGN 321
#define OR_ASSIGN 322
#define SUB_ASSIGN 323
#define LEFT_PAREN 324
#define RIGHT_PAREN 325
#define LEFT_BRACKET 326
#define RIGHT_BRACKET 327
#define LEFT_BRACE 328
#define RIGHT_BRACE 329
#define DOT 330
#define COMMA 331
#define COLON 332
#define EQUAL 333
#define SEMICOLON 334
#define BANG 335
#define DASH 336
#define TILDE 337
#define PLUS 338
#define STAR 339
#define SLASH 340
#define PERCENT 341
#define LEFT_ANGLE 342
#define RIGHT_ANGLE 343
#define VERTICAL_BAR 344
#define CARET 345
#define AMPERSAND 346
#define QUESTION 347




/* Copy the first part of user declarations.  */
#line 39 "glslang.y"


/* Based on:
ANSI C Yacc grammar

In 1985, Jeff Lee published his Yacc grammar (which is accompanied by a 
matching Lex specification) for the April 30, 1985 draft version of the 
ANSI C standard.  Tom Stockfisch reposted it to net.sources in 1987; that
original, as mentioned in the answer to question 17.25 of the comp.lang.c
FAQ, can be ftp'ed from ftp.uu.net, file usenet/net.sources/ansi.c.grammar.Z.
 
I intend to keep this version as close to the current C Standard grammar as 
possible; please let me know if you discover discrepancies. 

Jutta Degener, 1995 
*/

#include "SymbolTable.h"
#include "ParseHelper.h"
#include "../Public/ShaderLang.h"

#ifdef _WIN32
    #define YYPARSE_PARAM parseContext
    #define YYPARSE_PARAM_DECL TParseContext&
    #define YY_DECL int yylex(YYSTYPE* pyylval, TParseContext& parseContext)
    #define YYLEX_PARAM parseContext
#else
    #define YYPARSE_PARAM parseContextLocal
    #define parseContext (*((TParseContext*)(parseContextLocal)))
    #define YY_DECL int yylex(YYSTYPE* pyylval, void* parseContextLocal)
    #define YYLEX_PARAM (void*)(parseContextLocal)
    extern void yyerror(char*);    
#endif

#define FRAG_VERT_ONLY(S, L) {                                                  \
    if (parseContext.language != EShLangFragment &&                             \
        parseContext.language != EShLangVertex) {                               \
        parseContext.error(L, " supported in vertex/fragment shaders only ", S, "", "");   \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define VERTEX_ONLY(S, L) {                                                     \
    if (parseContext.language != EShLangVertex) {                               \
        parseContext.error(L, " supported in vertex shaders only ", S, "", "");            \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define FRAG_ONLY(S, L) {                                                       \
    if (parseContext.language != EShLangFragment) {                             \
        parseContext.error(L, " supported in fragment shaders only ", S, "", "");          \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define PACK_ONLY(S, L) {                                                       \
    if (parseContext.language != EShLangPack) {                                 \
        parseContext.error(L, " supported in pack shaders only ", S, "", "");              \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define UNPACK_ONLY(S, L) {                                                     \
    if (parseContext.language != EShLangUnpack) {                               \
        parseContext.error(L, " supported in unpack shaders only ", S, "", "");            \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define PACK_UNPACK_ONLY(S, L) {                                                \
    if (parseContext.language != EShLangUnpack &&                               \
        parseContext.language != EShLangPack) {                                 \
        parseContext.error(L, " supported in pack/unpack shaders only ", S, "", "");       \
        parseContext.recover();                                                            \
    }                                                                           \
}


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 117 "glslang.y"
typedef union YYSTYPE {
    struct {
        TSourceLoc line;
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TSourceLoc line;
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
        };
        union {
            TPublicType type;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TTypeLine typeLine;
            TTypeList* typeList;
        };
    } interm;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 369 "glslang.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 148 "glslang.y"

#ifndef _WIN32
    extern int yylex(YYSTYPE*, void*);
#endif


/* Line 214 of yacc.c.  */
#line 386 "glslang.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
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
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  59
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1231

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  93
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  75
/* YYNRULES -- Number of rules. */
#define YYNRULES  214
/* YYNRULES -- Number of states. */
#define YYNSTATES  331

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   347

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    17,    19,
      24,    26,    30,    33,    36,    38,    40,    43,    46,    49,
      51,    54,    58,    61,    63,    65,    67,    69,    71,    73,
      75,    77,    79,    81,    83,    85,    87,    89,    91,    93,
      95,    97,    99,   102,   105,   108,   110,   112,   114,   116,
     118,   122,   126,   130,   132,   136,   140,   142,   146,   150,
     152,   156,   160,   164,   168,   170,   174,   178,   180,   184,
     186,   190,   192,   196,   198,   202,   204,   208,   210,   214,
     216,   222,   224,   228,   230,   232,   234,   236,   238,   240,
     242,   244,   246,   248,   250,   252,   256,   258,   261,   264,
     267,   269,   271,   274,   278,   282,   285,   291,   295,   298,
     302,   305,   306,   308,   310,   312,   314,   319,   321,   325,
     331,   338,   344,   346,   349,   354,   360,   365,   367,   370,
     372,   374,   376,   378,   380,   382,   384,   386,   388,   390,
     392,   394,   396,   398,   400,   402,   404,   406,   408,   410,
     412,   414,   416,   418,   420,   422,   424,   426,   432,   437,
     439,   442,   446,   448,   452,   454,   459,   461,   463,   465,
     467,   469,   471,   473,   475,   477,   480,   481,   482,   488,
     490,   492,   495,   499,   501,   504,   506,   509,   515,   519,
     521,   523,   528,   529,   536,   537,   546,   547,   555,   557,
     559,   561,   562,   565,   569,   572,   575,   578,   582,   585,
     587,   590,   592,   594,   595
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     164,     0,    -1,    42,    -1,    94,    -1,    45,    -1,    44,
      -1,    46,    -1,    69,   121,    70,    -1,    95,    -1,    96,
      71,    97,    72,    -1,    98,    -1,    96,    75,    47,    -1,
      96,    50,    -1,    96,    51,    -1,   121,    -1,    99,    -1,
     101,    70,    -1,   100,    70,    -1,   102,    34,    -1,   102,
      -1,   102,   119,    -1,   101,    76,   119,    -1,   103,    69,
      -1,   104,    -1,    42,    -1,     6,    -1,     7,    -1,     5,
      -1,    22,    -1,    23,    -1,    24,    -1,    16,    -1,    17,
      -1,    18,    -1,    19,    -1,    20,    -1,    21,    -1,    25,
      -1,    26,    -1,    27,    -1,    43,    -1,    96,    -1,    50,
     105,    -1,    51,   105,    -1,   106,   105,    -1,    83,    -1,
      81,    -1,    80,    -1,    82,    -1,   105,    -1,   107,    84,
     105,    -1,   107,    85,   105,    -1,   107,    86,   105,    -1,
     107,    -1,   108,    83,   107,    -1,   108,    81,   107,    -1,
     108,    -1,   109,    48,   108,    -1,   109,    49,   108,    -1,
     109,    -1,   110,    87,   109,    -1,   110,    88,   109,    -1,
     110,    52,   109,    -1,   110,    53,   109,    -1,   110,    -1,
     111,    54,   110,    -1,   111,    55,   110,    -1,   111,    -1,
     112,    91,   111,    -1,   112,    -1,   113,    90,   112,    -1,
     113,    -1,   114,    89,   113,    -1,   114,    -1,   115,    56,
     114,    -1,   115,    -1,   116,    58,   115,    -1,   116,    -1,
     117,    57,   116,    -1,   117,    -1,   117,    92,   121,    77,
     119,    -1,   118,    -1,   105,   120,   119,    -1,    78,    -1,
      59,    -1,    60,    -1,    62,    -1,    61,    -1,    68,    -1,
      63,    -1,    64,    -1,    65,    -1,    66,    -1,    67,    -1,
     119,    -1,   121,    76,   119,    -1,   118,    -1,   124,    79,
      -1,   132,    79,    -1,   125,    70,    -1,   127,    -1,   126,
      -1,   127,   129,    -1,   126,    76,   129,    -1,   134,    42,
      69,    -1,   136,    42,    -1,   136,    42,    71,   122,    72,
      -1,   135,   130,   128,    -1,   130,   128,    -1,   135,   130,
     131,    -1,   130,   131,    -1,    -1,    28,    -1,    29,    -1,
      30,    -1,   136,    -1,   136,    71,   122,    72,    -1,   133,
      -1,   132,    76,    42,    -1,   132,    76,    42,    71,    72,
      -1,   132,    76,    42,    71,   122,    72,    -1,   132,    76,
      42,    78,   142,    -1,   134,    -1,   134,    42,    -1,   134,
      42,    71,    72,    -1,   134,    42,    71,   122,    72,    -1,
     134,    42,    78,   142,    -1,   136,    -1,   135,   136,    -1,
       4,    -1,     3,    -1,    32,    -1,    31,    -1,    34,    -1,
       6,    -1,     7,    -1,     5,    -1,    22,    -1,    23,    -1,
      24,    -1,    16,    -1,    17,    -1,    18,    -1,    19,    -1,
      20,    -1,    21,    -1,    25,    -1,    26,    -1,    27,    -1,
      36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,    -1,
      41,    -1,   137,    -1,    43,    -1,    33,    42,    73,   138,
      74,    -1,    33,    73,   138,    74,    -1,   139,    -1,   138,
     139,    -1,   136,   140,    79,    -1,   141,    -1,   140,    76,
     141,    -1,    42,    -1,    42,    71,   122,    72,    -1,   119,
      -1,   123,    -1,   146,    -1,   145,    -1,   143,    -1,   152,
      -1,   153,    -1,   156,    -1,   163,    -1,    73,    74,    -1,
      -1,    -1,    73,   147,   151,   148,    74,    -1,   150,    -1,
     145,    -1,    73,    74,    -1,    73,   151,    74,    -1,   144,
      -1,   151,   144,    -1,    79,    -1,   121,    79,    -1,    13,
      69,   121,    70,   154,    -1,   144,    11,   144,    -1,   144,
      -1,   121,    -1,   134,    42,    78,   142,    -1,    -1,    35,
      69,   157,   155,    70,   149,    -1,    -1,    10,   158,   144,
      35,    69,   121,    70,    79,    -1,    -1,    12,    69,   159,
     160,   162,    70,   149,    -1,   152,    -1,   143,    -1,   155,
      -1,    -1,   161,    79,    -1,   161,    79,   121,    -1,     9,
      79,    -1,     8,    79,    -1,    15,    79,    -1,    15,   121,
      79,    -1,    14,    79,    -1,   165,    -1,   164,   165,    -1,
     166,    -1,   123,    -1,    -1,   124,   167,   150,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   210,   210,   245,   248,   261,   266,   271,   277,   280,
     348,   351,   460,   470,   483,   491,   586,   590,   597,   601,
     608,   614,   623,   629,   640,   656,   657,   658,   659,   660,
     661,   662,   663,   664,   665,   666,   667,   668,   669,   670,
     671,   682,   685,   695,   705,   727,   728,   729,   730,   736,
     737,   746,   755,   767,   768,   776,   787,   788,   797,   809,
     810,   820,   830,   840,   853,   854,   864,   877,   878,   890,
     891,   903,   904,   916,   917,   930,   931,   944,   945,   958,
     959,   976,   977,   990,   991,   992,   993,   994,   995,   996,
     997,   998,   999,  1000,  1004,  1007,  1018,  1026,  1027,  1035,
    1071,  1074,  1081,  1089,  1110,  1129,  1140,  1167,  1172,  1182,
    1187,  1197,  1200,  1203,  1206,  1212,  1217,  1235,  1238,  1246,
    1254,  1262,  1284,  1288,  1297,  1306,  1315,  1405,  1408,  1425,
    1429,  1436,  1444,  1453,  1458,  1463,  1468,  1479,  1484,  1489,
    1494,  1499,  1504,  1509,  1514,  1519,  1524,  1530,  1536,  1542,
    1548,  1554,  1560,  1566,  1572,  1578,  1583,  1596,  1606,  1614,
    1617,  1632,  1650,  1654,  1660,  1665,  1681,  1685,  1689,  1690,
    1696,  1697,  1698,  1699,  1700,  1704,  1705,  1705,  1705,  1713,
    1714,  1719,  1722,  1730,  1733,  1739,  1740,  1744,  1752,  1756,
    1766,  1771,  1788,  1788,  1793,  1793,  1800,  1800,  1813,  1816,
    1822,  1825,  1831,  1835,  1842,  1849,  1856,  1863,  1874,  1883,
    1887,  1894,  1897,  1903,  1903
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ATTRIBUTE", "CONST_QUAL", "BOOL_TYPE", 
  "FLOAT_TYPE", "INT_TYPE", "BREAK", "CONTINUE", "DO", "ELSE", "FOR", 
  "IF", "DISCARD", "RETURN", "BVEC2", "BVEC3", "BVEC4", "IVEC2", "IVEC3", 
  "IVEC4", "VEC2", "VEC3", "VEC4", "MATRIX2", "MATRIX3", "MATRIX4", 
  "IN_QUAL", "OUT_QUAL", "INOUT_QUAL", "UNIFORM", "VARYING", "STRUCT", 
  "VOID_TYPE", "WHILE", "SAMPLER1D", "SAMPLER2D", "SAMPLER3D", 
  "SAMPLERCUBE", "SAMPLER1DSHADOW", "SAMPLER2DSHADOW", "IDENTIFIER", 
  "TYPE_NAME", "FLOATCONSTANT", "INTCONSTANT", "BOOLCONSTANT", 
  "FIELD_SELECTION", "LEFT_OP", "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", 
  "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN", 
  "DIV_ASSIGN", "ADD_ASSIGN", "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", 
  "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN", "SUB_ASSIGN", "LEFT_PAREN", 
  "RIGHT_PAREN", "LEFT_BRACKET", "RIGHT_BRACKET", "LEFT_BRACE", 
  "RIGHT_BRACE", "DOT", "COMMA", "COLON", "EQUAL", "SEMICOLON", "BANG", 
  "DASH", "TILDE", "PLUS", "STAR", "SLASH", "PERCENT", "LEFT_ANGLE", 
  "RIGHT_ANGLE", "VERTICAL_BAR", "CARET", "AMPERSAND", "QUESTION", 
  "$accept", "variable_identifier", "primary_expression", 
  "postfix_expression", "integer_expression", "function_call", 
  "function_call_generic", "function_call_header_no_parameters", 
  "function_call_header_with_parameters", "function_call_header", 
  "function_identifier", "constructor_identifier", "unary_expression", 
  "unary_operator", "multiplicative_expression", "additive_expression", 
  "shift_expression", "relational_expression", "equality_expression", 
  "and_expression", "exclusive_or_expression", "inclusive_or_expression", 
  "logical_and_expression", "logical_xor_expression", 
  "logical_or_expression", "conditional_expression", 
  "assignment_expression", "assignment_operator", "expression", 
  "constant_expression", "declaration", "function_prototype", 
  "function_declarator", "function_header_with_parameters", 
  "function_header", "parameter_declarator", "parameter_declaration", 
  "parameter_qualifier", "parameter_type_specifier", 
  "init_declarator_list", "single_declaration", "fully_specified_type", 
  "type_qualifier", "type_specifier", "struct_specifier", 
  "struct_declaration_list", "struct_declaration", 
  "struct_declarator_list", "struct_declarator", "initializer", 
  "declaration_statement", "statement", "simple_statement", 
  "compound_statement", "@1", "@2", "statement_no_new_scope", 
  "compound_statement_no_new_scope", "statement_list", 
  "expression_statement", "selection_statement", 
  "selection_rest_statement", "condition", "iteration_statement", "@3", 
  "@4", "@5", "for_init_statement", "conditionopt", "for_rest_statement", 
  "jump_statement", "translation_unit", "external_declaration", 
  "function_definition", "@6", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
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
     345,   346,   347
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    93,    94,    95,    95,    95,    95,    95,    96,    96,
      96,    96,    96,    96,    97,    98,    99,    99,   100,   100,
     101,   101,   102,   103,   103,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   105,   105,   105,   105,   106,   106,   106,   106,   107,
     107,   107,   107,   108,   108,   108,   109,   109,   109,   110,
     110,   110,   110,   110,   111,   111,   111,   112,   112,   113,
     113,   114,   114,   115,   115,   116,   116,   117,   117,   118,
     118,   119,   119,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   121,   121,   122,   123,   123,   124,
     125,   125,   126,   126,   127,   128,   128,   129,   129,   129,
     129,   130,   130,   130,   130,   131,   131,   132,   132,   132,
     132,   132,   133,   133,   133,   133,   133,   134,   134,   135,
     135,   135,   135,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   137,   137,   138,
     138,   139,   140,   140,   141,   141,   142,   143,   144,   144,
     145,   145,   145,   145,   145,   146,   147,   148,   146,   149,
     149,   150,   150,   151,   151,   152,   152,   153,   154,   154,
     155,   155,   157,   156,   158,   156,   159,   156,   160,   160,
     161,   161,   162,   162,   163,   163,   163,   163,   163,   164,
     164,   165,   165,   167,   166
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     3,     1,     4,
       1,     3,     2,     2,     1,     1,     2,     2,     2,     1,
       2,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     1,     1,     1,     1,     1,
       3,     3,     3,     1,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     2,     2,     2,
       1,     1,     2,     3,     3,     2,     5,     3,     2,     3,
       2,     0,     1,     1,     1,     1,     4,     1,     3,     5,
       6,     5,     1,     2,     4,     5,     4,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     4,     1,
       2,     3,     1,     3,     1,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     0,     0,     5,     1,
       1,     2,     3,     1,     2,     1,     2,     5,     3,     1,
       1,     4,     0,     6,     0,     8,     0,     7,     1,     1,
       1,     0,     2,     3,     2,     2,     2,     3,     2,     1,
       2,     1,     1,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,   130,   129,   136,   134,   135,   140,   141,   142,   143,
     144,   145,   137,   138,   139,   146,   147,   148,   132,   131,
       0,   133,   149,   150,   151,   152,   153,   154,   156,   212,
     213,     0,   101,   111,     0,   117,   122,     0,   127,   155,
       0,   209,   211,     0,     0,    97,     0,    99,   111,   112,
     113,   114,   102,     0,   111,     0,    98,   123,   128,     1,
     210,     0,     0,     0,   159,     0,   214,   103,   108,   110,
     115,     0,   118,   104,     0,     0,     0,   164,     0,   162,
     158,   160,   136,   134,   135,     0,     0,   194,     0,     0,
       0,     0,   140,   141,   142,   143,   144,   145,   137,   138,
     139,   146,   147,   148,     0,     2,   156,     5,     4,     6,
       0,     0,     0,   176,   181,   185,    47,    46,    48,    45,
       3,     8,    41,    10,    15,     0,     0,    19,     0,    23,
      49,     0,    53,    56,    59,    64,    67,    69,    71,    73,
      75,    77,    79,    81,    94,     0,   167,     0,   170,   183,
     169,   168,     0,   171,   172,   173,   174,   105,     0,   107,
     109,     0,     0,    27,    25,    26,    31,    32,    33,    34,
      35,    36,    28,    29,    30,    37,    38,    39,    40,   124,
      49,    96,     0,   166,   126,   157,     0,     0,   161,   205,
     204,     0,   196,     0,   208,   206,     0,   192,    42,    43,
       0,   175,     0,    12,    13,     0,     0,    17,    16,     0,
      18,    20,    22,    84,    85,    87,    86,    89,    90,    91,
      92,    93,    88,    83,     0,    44,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,   182,   184,
       0,     0,   119,     0,   121,   125,     0,   163,     0,     0,
       0,   207,     0,     7,   177,     0,    14,    11,    21,    82,
      50,    51,    52,    55,    54,    57,    58,    62,    63,    60,
      61,    65,    66,    68,    70,    72,    74,    76,    78,     0,
      95,     0,   116,   120,   165,     0,   199,   198,   201,     0,
     190,     0,     0,     0,     9,     0,   106,     0,   200,     0,
       0,   189,   187,     0,     0,   178,    80,     0,   202,     0,
       0,     0,   180,   193,   179,     0,   203,   197,   188,   191,
     195
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,   120,   121,   122,   265,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   224,   145,   182,
     146,   147,    31,    32,    33,    68,    52,    53,    69,    34,
      35,    36,    37,    38,    39,    63,    64,    78,    79,   184,
     148,   149,   150,   151,   202,   303,   323,   324,   152,   153,
     154,   312,   302,   155,   262,   191,   259,   298,   309,   310,
     156,    40,    41,    42,    46
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -297
static const short yypact[] =
{
    1149,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
     -27,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
     -42,   -28,   -32,     4,    18,  -297,    19,  1188,  -297,  -297,
    1108,  -297,  -297,   -10,  1188,  -297,    -3,  -297,    36,  -297,
    -297,  -297,  -297,  1188,    83,    33,  -297,    -9,  -297,  -297,
    -297,  1188,    39,  1025,  -297,   235,  -297,  -297,  -297,  -297,
     -18,  1188,   -52,  -297,   685,   957,  1064,   -17,    20,  -297,
    -297,  -297,    29,    45,    63,    21,    23,  -297,    75,    77,
      66,   753,    78,    79,    81,    82,    84,    85,    87,    89,
      90,    91,    93,    94,    95,    96,    97,  -297,  -297,  -297,
     957,   957,   957,   120,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,     5,  -297,  -297,    98,     1,   821,   100,  -297,
      57,   957,    42,   -56,    37,   -40,    76,    61,    80,   106,
     111,   138,   -41,  -297,  -297,    30,  -297,   -42,  -297,  -297,
    -297,  -297,   316,  -297,  -297,  -297,  -297,   127,   957,  -297,
    -297,   889,   957,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,   128,  -297,  -297,  -297,   957,    39,  -297,  -297,
    -297,   397,  -297,   957,  -297,  -297,    31,  -297,  -297,  -297,
       3,  -297,   397,  -297,  -297,   957,   152,  -297,  -297,   957,
    -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,  -297,  -297,   957,  -297,   957,   957,   957,   957,
     957,   957,   957,   957,   957,   957,   957,   957,   957,   957,
     957,   957,   957,   957,   957,   957,   957,  -297,  -297,  -297,
     957,   129,  -297,   130,  -297,  -297,   131,  -297,   169,   549,
      12,  -297,   617,  -297,   397,   133,   134,  -297,  -297,  -297,
    -297,  -297,  -297,    42,    42,   -56,   -56,    37,    37,    37,
      37,   -40,   -40,    76,    61,    80,   106,   111,   138,    60,
    -297,   135,  -297,  -297,  -297,   137,  -297,  -297,   617,   397,
     134,   167,   141,   140,  -297,   957,  -297,   957,  -297,   136,
     142,   205,  -297,   143,   478,  -297,  -297,    13,   957,   478,
     397,   957,  -297,  -297,  -297,   139,   134,  -297,  -297,  -297,
    -297
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,   -53,  -297,   -91,   -89,  -143,   -97,   -20,   -16,
     -21,   -15,   -14,   -22,  -297,   -57,   -75,  -297,   -90,  -155,
       9,    10,  -297,  -297,  -297,   154,   175,   172,   160,  -297,
    -297,  -257,   -19,   -33,  -297,   171,    -4,  -297,    46,  -160,
     -25,  -107,  -296,  -297,  -297,  -297,   -84,   190,    35,     6,
    -297,  -297,   -35,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,   224,  -297,  -297
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -101
static const short yytable[] =
{
     183,   196,   254,   251,    58,   301,   253,     1,     2,    29,
      30,    62,   233,   234,    54,    43,   244,   181,   322,   161,
      70,   180,   200,   322,   157,   229,   162,   230,    62,    54,
      62,   256,    49,    50,    51,    18,    19,    45,    70,     1,
       2,   301,    47,    62,    48,   249,    44,   235,   236,    29,
      30,   245,   211,   158,   186,   203,   204,   198,   199,    81,
      73,    57,    74,    61,    49,    50,    51,    18,    19,    75,
      65,   208,    81,   263,  -100,    72,   205,   209,   225,   246,
     206,    77,   299,   325,   258,   231,   232,   183,   246,   246,
     277,   278,   279,   280,    55,   291,   187,    56,   -27,   188,
     189,   181,   190,   260,   181,   180,   246,   246,   180,   247,
     261,    49,    50,    51,   -25,   266,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   226,   227,   228,   181,
     237,   238,   -26,   180,   268,   223,   246,   305,   273,   274,
     281,   282,   275,   276,   192,   194,   193,   -31,   -32,   269,
     -33,   -34,   239,   -35,   -36,   289,   -28,   249,   -29,   -30,
     -37,   329,   -38,   -39,   197,   -24,   -40,   242,   207,   212,
     240,   290,   300,   270,   271,   272,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   180,   180,   180,   180,   180,
     180,   180,   311,   181,   201,   241,   243,   180,   250,   267,
     255,   292,   293,   294,   295,   304,   307,   306,   300,   313,
     246,   314,   319,   328,   315,   318,   320,   317,   330,   283,
     285,   321,   288,    67,   284,   159,    71,   286,   326,   287,
     316,   160,    76,   257,   296,   327,    66,   264,     1,     2,
      82,    83,    84,    85,    86,    87,   183,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   308,    60,   297,    18,    19,    20,    21,
     104,    22,    23,    24,    25,    26,    27,   105,   106,   107,
     108,   109,     0,     0,     0,   110,   111,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,     0,     0,     0,   113,   114,
       0,     0,     0,     0,   115,   116,   117,   118,   119,     1,
       2,    82,    83,    84,    85,    86,    87,     0,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,     0,     0,     0,    18,    19,    20,
      21,   104,    22,    23,    24,    25,    26,    27,   105,   106,
     107,   108,   109,     0,     0,     0,   110,   111,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   112,     0,     0,     0,   113,
     248,     0,     0,     0,     0,   115,   116,   117,   118,   119,
       1,     2,    82,    83,    84,    85,    86,    87,     0,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,     0,     0,     0,    18,    19,
      20,    21,   104,    22,    23,    24,    25,    26,    27,   105,
     106,   107,   108,   109,     0,     0,     0,   110,   111,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
     113,     0,     0,     0,     0,     0,   115,   116,   117,   118,
     119,     1,     2,    82,    83,    84,    85,    86,    87,     0,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,     0,     0,     0,    18,
      19,    20,    21,   104,    22,    23,    24,    25,    26,    27,
     105,   106,   107,   108,   109,     0,     0,     0,   110,   111,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   112,     0,     0,
       0,    65,     1,     2,    82,    83,    84,   115,   116,   117,
     118,   119,     0,     0,     0,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,     0,     0,     0,
      18,    19,    20,    21,     0,    22,    23,    24,    25,    26,
      27,   105,   106,   107,   108,   109,     0,     0,     0,   110,
     111,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   112,     0,
       1,     2,    82,    83,    84,     0,     0,     0,   115,   116,
     117,   118,   119,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,     0,     0,     0,    18,    19,
      20,    21,     0,    22,    23,    24,    25,    26,    27,   105,
     106,   107,   108,   109,     0,     0,     0,   110,   111,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
     163,   164,   165,     0,     0,     0,     0,   116,   117,   118,
     119,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   105,   178,   107,
     108,   109,     0,     0,     0,   110,   111,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   112,     0,     0,   179,   163,   164,
     165,     0,     0,     0,     0,   116,   117,   118,   119,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   105,   178,   107,   108,   109,
       0,     0,     0,   110,   111,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   112,     0,     0,     0,   163,   164,   165,     0,
       0,     0,   195,   116,   117,   118,   119,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,     0,
       0,     0,     0,     0,     0,   210,     0,     0,     0,     0,
       0,     0,     0,   105,   178,   107,   108,   109,     0,     0,
       0,   110,   111,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     112,     0,     0,     0,   163,   164,   165,     0,     0,     0,
       0,   116,   117,   118,   119,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   105,   178,   107,   108,   109,     0,     0,     0,   110,
     111,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   112,     0,
       0,   252,   163,   164,   165,     0,     0,     0,     0,   116,
     117,   118,   119,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   105,
     178,   107,   108,   109,     0,     0,     0,   110,   111,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   112,     0,     0,     0,
       3,     4,     5,     0,     0,     0,     0,   116,   117,   118,
     119,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,     0,     0,     0,     0,     0,    20,    21,
       0,    22,    23,    24,    25,    26,    27,     0,    28,     3,
       4,     5,     0,     0,     0,     0,     0,     0,     0,     0,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,     0,     0,     0,     0,     0,    20,    21,    80,
      22,    23,    24,    25,    26,    27,     0,    28,    59,     0,
       0,     1,     2,     3,     4,     5,     0,     0,     0,     0,
       0,     0,     0,     0,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,     0,     0,   185,    18,
      19,    20,    21,     0,    22,    23,    24,    25,    26,    27,
       0,    28,     1,     2,     3,     4,     5,     0,     0,     0,
       0,     0,     0,     0,     0,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,     0,     0,     0,
      18,    19,    20,    21,     0,    22,    23,    24,    25,    26,
      27,     0,    28,     3,     4,     5,     0,     0,     0,     0,
       0,     0,     0,     0,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,     0,     0,     0,     0,
       0,    20,    21,     0,    22,    23,    24,    25,    26,    27,
       0,    28
};

static const short yycheck[] =
{
      75,    91,   162,   158,    37,   262,   161,     3,     4,     0,
       0,    44,    52,    53,    33,    42,    57,    74,   314,    71,
      53,    74,   112,   319,    42,    81,    78,    83,    61,    48,
      63,   186,    28,    29,    30,    31,    32,    79,    71,     3,
       4,   298,    70,    76,    76,   152,    73,    87,    88,    40,
      40,    92,   127,    71,    71,    50,    51,   110,   111,    63,
      69,    42,    71,    73,    28,    29,    30,    31,    32,    78,
      73,    70,    76,    70,    70,    42,    71,    76,   131,    76,
      75,    42,    70,    70,   191,    48,    49,   162,    76,    76,
     233,   234,   235,   236,    76,   250,    76,    79,    69,    79,
      79,   158,    79,   193,   161,   158,    76,    76,   161,    79,
      79,    28,    29,    30,    69,   205,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    84,    85,    86,   186,
      54,    55,    69,   186,   209,    78,    76,    77,   229,   230,
     237,   238,   231,   232,    69,    79,    69,    69,    69,   224,
      69,    69,    91,    69,    69,   245,    69,   264,    69,    69,
      69,   321,    69,    69,    69,    69,    69,    56,    70,    69,
      90,   246,   262,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   299,   250,    74,    89,    58,   250,    71,    47,
      72,    72,    72,    72,    35,    72,    69,    72,   298,    42,
      76,    70,    70,   320,    74,    79,    11,   307,    79,   239,
     241,    78,   244,    48,   240,    71,    54,   242,   318,   243,
     305,    71,    61,   187,   259,   319,    46,   202,     3,     4,
       5,     6,     7,     8,     9,    10,   321,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   298,    40,   259,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    73,    74,
      -1,    -1,    -1,    -1,    79,    80,    81,    82,    83,     3,
       4,     5,     6,     7,     8,     9,    10,    -1,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    -1,    -1,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    -1,    -1,    -1,    50,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    73,
      74,    -1,    -1,    -1,    -1,    79,    80,    81,    82,    83,
       3,     4,     5,     6,     7,     8,     9,    10,    -1,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    -1,    -1,    -1,    50,    51,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,
      73,    -1,    -1,    -1,    -1,    -1,    79,    80,    81,    82,
      83,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    -1,    -1,    -1,    50,    51,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,
      -1,    73,     3,     4,     5,     6,     7,    79,    80,    81,
      82,    83,    -1,    -1,    -1,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,
      31,    32,    33,    34,    -1,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,
       3,     4,     5,     6,     7,    -1,    -1,    -1,    79,    80,
      81,    82,    83,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    31,    32,
      33,    34,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    -1,    -1,    -1,    50,    51,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,
       5,     6,     7,    -1,    -1,    -1,    -1,    80,    81,    82,
      83,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
      45,    46,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    -1,    -1,    72,     5,     6,
       7,    -1,    -1,    -1,    -1,    80,    81,    82,    83,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,
      -1,    -1,    -1,    50,    51,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    -1,    -1,    -1,     5,     6,     7,    -1,
      -1,    -1,    79,    80,    81,    82,    83,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    34,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    -1,    -1,
      -1,    50,    51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    -1,    -1,    -1,     5,     6,     7,    -1,    -1,    -1,
      -1,    80,    81,    82,    83,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    42,    43,    44,    45,    46,    -1,    -1,    -1,    50,
      51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,
      -1,    72,     5,     6,     7,    -1,    -1,    -1,    -1,    80,
      81,    82,    83,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      43,    44,    45,    46,    -1,    -1,    -1,    50,    51,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,
       5,     6,     7,    -1,    -1,    -1,    -1,    80,    81,    82,
      83,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    -1,    -1,    -1,    -1,    -1,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,    -1,    43,     5,
       6,     7,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    -1,    33,    34,    74,
      36,    37,    38,    39,    40,    41,    -1,    43,     0,    -1,
      -1,     3,     4,     5,     6,     7,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    -1,    -1,    74,    31,
      32,    33,    34,    -1,    36,    37,    38,    39,    40,    41,
      -1,    43,     3,     4,     5,     6,     7,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,
      31,    32,    33,    34,    -1,    36,    37,    38,    39,    40,
      41,    -1,    43,     5,     6,     7,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
      -1,    33,    34,    -1,    36,    37,    38,    39,    40,    41,
      -1,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,     7,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    31,    32,
      33,    34,    36,    37,    38,    39,    40,    41,    43,   123,
     124,   125,   126,   127,   132,   133,   134,   135,   136,   137,
     164,   165,   166,    42,    73,    79,   167,    70,    76,    28,
      29,    30,   129,   130,   135,    76,    79,    42,   136,     0,
     165,    73,   136,   138,   139,    73,   150,   129,   128,   131,
     136,   130,    42,    69,    71,    78,   138,    42,   140,   141,
      74,   139,     5,     6,     7,     8,     9,    10,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    35,    42,    43,    44,    45,    46,
      50,    51,    69,    73,    74,    79,    80,    81,    82,    83,
      94,    95,    96,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   121,   123,   124,   143,   144,
     145,   146,   151,   152,   153,   156,   163,    42,    71,   128,
     131,    71,    78,     5,     6,     7,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    43,    72,
     105,   118,   122,   119,   142,    74,    71,    76,    79,    79,
      79,   158,    69,    69,    79,    79,   121,    69,   105,   105,
     121,    74,   147,    50,    51,    71,    75,    70,    70,    76,
      34,   119,    69,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    78,   120,   105,    84,    85,    86,    81,
      83,    48,    49,    52,    53,    87,    88,    54,    55,    91,
      90,    89,    56,    58,    57,    92,    76,    79,    74,   144,
      71,   122,    72,   122,   142,    72,   122,   141,   144,   159,
     121,    79,   157,    70,   151,    97,   121,    47,   119,   119,
     105,   105,   105,   107,   107,   108,   108,   109,   109,   109,
     109,   110,   110,   111,   112,   113,   114,   115,   116,   121,
     119,   122,    72,    72,    72,    35,   143,   152,   160,    70,
     121,   134,   155,   148,    72,    77,    72,    69,   155,   161,
     162,   144,   154,    42,    70,    74,   119,   121,    79,    70,
      11,    78,   145,   149,   150,    70,   121,   149,   144,   142,
      79
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
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
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

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

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


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

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

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
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 210 "glslang.y"
    {
        // The symbol table search was done in the lexical phase
        const TSymbol* symbol = yyvsp[0].lex.symbol;
        const TVariable* variable;
        if (symbol == 0) {
            parseContext.error(yyvsp[0].lex.line, "undeclared identifier", yyvsp[0].lex.string->c_str(), "");
            parseContext.recover();
            TType type(EbtFloat);
            TVariable* fakeVariable = new TVariable(yyvsp[0].lex.string, type);
            parseContext.symbolTable.insert(*fakeVariable);
            variable = fakeVariable;
        } else {
            // This identifier can only be a variable type symbol 
            if (! symbol->isVariable()) {
                parseContext.error(yyvsp[0].lex.line, "variable expected", yyvsp[0].lex.string->c_str(), "");
                parseContext.recover();
            }
            variable = static_cast<const TVariable*>(symbol);
        }

        // don't delete $1.string, it's used by error recovery, and the pool
        // pop will reclaim the memory

        if (variable->getType().getQualifier() == EvqConst ) {
            constUnion* constArray = variable->getConstPointer();
            TType t(variable->getType());
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(constArray, t, yyvsp[0].lex.line);        
        } else
            yyval.interm.intermTypedNode = parseContext.intermediate.addSymbol(variable->getUniqueId(), 
                                                     variable->getName(), 
                                                     variable->getType(), yyvsp[0].lex.line);
    ;}
    break;

  case 3:
#line 245 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 4:
#line 248 "glslang.y"
    {
        //
        // INT_TYPE is only 16-bit plus sign bit for vertex/fragment shaders, 
        // check for overflow for constants
        //
        if (abs(yyvsp[0].lex.i) >= (1 << 16)) {
            parseContext.error(yyvsp[0].lex.line, " integer constant overflow", "", "");
            parseContext.recover();
        }
        constUnion *unionArray = new constUnion[1];
        unionArray->iConst = yyvsp[0].lex.i;
        yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), yyvsp[0].lex.line);
    ;}
    break;

  case 5:
#line 261 "glslang.y"
    {
        constUnion *unionArray = new constUnion[1];
        unionArray->fConst = yyvsp[0].lex.f;
        yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), yyvsp[0].lex.line);
    ;}
    break;

  case 6:
#line 266 "glslang.y"
    {
        constUnion *unionArray = new constUnion[1];
        unionArray->bConst = yyvsp[0].lex.b;
        yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[0].lex.line);
    ;}
    break;

  case 7:
#line 271 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[-1].interm.intermTypedNode;
    ;}
    break;

  case 8:
#line 277 "glslang.y"
    { 
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 9:
#line 280 "glslang.y"
    {
        if (!yyvsp[-3].interm.intermTypedNode->isArray() && !yyvsp[-3].interm.intermTypedNode->isMatrix() && !yyvsp[-3].interm.intermTypedNode->isVector()) {
            if (yyvsp[-3].interm.intermTypedNode->getAsSymbolNode())
                parseContext.error(yyvsp[-2].lex.line, " left of '[' is not of type array, matrix, or vector ", yyvsp[-3].interm.intermTypedNode->getAsSymbolNode()->getSymbol().c_str(), "");
            else
                parseContext.error(yyvsp[-2].lex.line, " left of '[' is not of type array, matrix, or vector ", "expression", "");
            parseContext.recover();
        }
        if (yyvsp[-3].interm.intermTypedNode->getType().getQualifier() == EvqConst && !yyvsp[-3].interm.intermTypedNode->isArray() && yyvsp[-1].interm.intermTypedNode->getQualifier() == EvqConst) {
             if (yyvsp[-3].interm.intermTypedNode->isVector()) {  // constant folding for vectors
                TVectorFields fields;
                fields.num = 1;
                fields.offsets[0] = yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst; // need to do it this way because v.xy sends fields integer array
                yyval.interm.intermTypedNode = parseContext.addConstVectorNode(fields, yyvsp[-3].interm.intermTypedNode, yyvsp[-2].lex.line);
            } else if (yyvsp[-3].interm.intermTypedNode->isMatrix()) { // constant folding for matrices
                yyval.interm.intermTypedNode = parseContext.addConstMatrixNode(yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst, yyvsp[-3].interm.intermTypedNode, yyvsp[-2].lex.line);
            }
        } else {
            if (yyvsp[-1].interm.intermTypedNode->getQualifier() == EvqConst) {
                if ((yyvsp[-3].interm.intermTypedNode->isVector() || yyvsp[-3].interm.intermTypedNode->isMatrix()) && yyvsp[-3].interm.intermTypedNode->getType().getNominalSize() <= yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst && !yyvsp[-3].interm.intermTypedNode->isArray() ) {
                    parseContext.error(yyvsp[-2].lex.line, "", "[", "field selection out of range '%d'", yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst);
                    parseContext.recover();
                } else {
                    if (yyvsp[-3].interm.intermTypedNode->isArray()) {
                        if (yyvsp[-3].interm.intermTypedNode->getType().getArraySize() == 0) {
                            if (yyvsp[-3].interm.intermTypedNode->getType().getMaxArraySize() <= yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst) {
                                if (parseContext.arraySetMaxSize(yyvsp[-3].interm.intermTypedNode->getAsSymbolNode(), yyvsp[-3].interm.intermTypedNode->getTypePointer(), yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst, true, yyvsp[-2].lex.line))
                                    parseContext.recover(); 
                            } else {
                                if (parseContext.arraySetMaxSize(yyvsp[-3].interm.intermTypedNode->getAsSymbolNode(), yyvsp[-3].interm.intermTypedNode->getTypePointer(), 0, false, yyvsp[-2].lex.line))
                                    parseContext.recover(); 
                            }
                        } else if ( yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst >= yyvsp[-3].interm.intermTypedNode->getType().getArraySize()) {
                            parseContext.error(yyvsp[-2].lex.line, "", "[", "array index out of range '%d'", yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst);
                            parseContext.recover();
                        }
                    }
                    yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexDirect, yyvsp[-3].interm.intermTypedNode, yyvsp[-1].interm.intermTypedNode, yyvsp[-2].lex.line);
                }
            } else {
                if (yyvsp[-3].interm.intermTypedNode->isArray() && yyvsp[-3].interm.intermTypedNode->getType().getArraySize() == 0) {
                    parseContext.error(yyvsp[-2].lex.line, "", "[", "array must be redeclared with a size before being indexed with a variable");
                    parseContext.recover();
                }
                
                yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexIndirect, yyvsp[-3].interm.intermTypedNode, yyvsp[-1].interm.intermTypedNode, yyvsp[-2].lex.line);
            }
        } 
        if (yyval.interm.intermTypedNode == 0) {
            constUnion *unionArray = new constUnion[1];
            unionArray->fConst = 0.0;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), yyvsp[-2].lex.line);
        } else if (yyvsp[-3].interm.intermTypedNode->isArray()) {
            if (yyvsp[-3].interm.intermTypedNode->getType().getStruct())
                yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getType().getStruct(), yyvsp[-3].interm.intermTypedNode->getType().getTypeName()));
            else
                yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getBasicType(), EvqTemporary, yyvsp[-3].interm.intermTypedNode->getNominalSize(), yyvsp[-3].interm.intermTypedNode->isMatrix()));
        } else if (yyvsp[-3].interm.intermTypedNode->isMatrix() && yyvsp[-3].interm.intermTypedNode->getType().getQualifier() == EvqConst)         
            yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getBasicType(), EvqConst, yyvsp[-3].interm.intermTypedNode->getNominalSize()));     
        else if (yyvsp[-3].interm.intermTypedNode->isMatrix())            
            yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getBasicType(), EvqTemporary, yyvsp[-3].interm.intermTypedNode->getNominalSize()));     
        else if (yyvsp[-3].interm.intermTypedNode->isVector() && yyvsp[-3].interm.intermTypedNode->getType().getQualifier() == EvqConst)          
            yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getBasicType(), EvqConst));     
        else if (yyvsp[-3].interm.intermTypedNode->isVector())       
            yyval.interm.intermTypedNode->setType(TType(yyvsp[-3].interm.intermTypedNode->getBasicType(), EvqTemporary));
        else
            yyval.interm.intermTypedNode->setType(yyvsp[-3].interm.intermTypedNode->getType()); 
    ;}
    break;

  case 10:
#line 348 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 11:
#line 351 "glslang.y"
    {        
        if (yyvsp[-2].interm.intermTypedNode->isArray()) {
            parseContext.error(yyvsp[0].lex.line, "cannot apply dot operator to an array", ".", "");
            parseContext.recover();
        }

        if (yyvsp[-2].interm.intermTypedNode->isVector()) {
            TVectorFields fields;
            if (! parseContext.parseVectorFields(*yyvsp[0].lex.string, yyvsp[-2].interm.intermTypedNode->getNominalSize(), fields, yyvsp[0].lex.line)) {
                fields.num = 1;
                fields.offsets[0] = 0;
                parseContext.recover();
            }

            if (yyvsp[-2].interm.intermTypedNode->getType().getQualifier() == EvqConst) { // constant folding for vector fields
                yyval.interm.intermTypedNode = parseContext.addConstVectorNode(fields, yyvsp[-2].interm.intermTypedNode, yyvsp[0].lex.line);
                if (yyval.interm.intermTypedNode == 0) {
                    parseContext.recover();
                    yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
                }
                else
                    yyval.interm.intermTypedNode->setType(TType(yyvsp[-2].interm.intermTypedNode->getBasicType(), EvqConst, (int) (*yyvsp[0].lex.string).size()));
            } else {
                if (fields.num == 1) {
                    constUnion *unionArray = new constUnion[1];
                    unionArray->iConst = fields.offsets[0];
                    TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), yyvsp[0].lex.line);
                    yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexDirect, yyvsp[-2].interm.intermTypedNode, index, yyvsp[-1].lex.line);
                    yyval.interm.intermTypedNode->setType(TType(yyvsp[-2].interm.intermTypedNode->getBasicType()));
                } else {
                    TString vectorString = *yyvsp[0].lex.string;
                    TIntermTyped* index = parseContext.intermediate.addSwizzle(fields, yyvsp[0].lex.line);                
                    yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpVectorSwizzle, yyvsp[-2].interm.intermTypedNode, index, yyvsp[-1].lex.line);
                    yyval.interm.intermTypedNode->setType(TType(yyvsp[-2].interm.intermTypedNode->getBasicType(),EvqTemporary, (int) vectorString.size()));  
                }
            }
        } else if (yyvsp[-2].interm.intermTypedNode->isMatrix()) {
            TMatrixFields fields;
            if (! parseContext.parseMatrixFields(*yyvsp[0].lex.string, yyvsp[-2].interm.intermTypedNode->getNominalSize(), fields, yyvsp[0].lex.line)) {
                fields.wholeRow = false;
                fields.wholeCol = false;
                fields.row = 0;
                fields.col = 0;
                parseContext.recover();
            }

            if (fields.wholeRow || fields.wholeCol) {
                parseContext.error(yyvsp[-1].lex.line, " non-scalar fields not implemented yet", ".", "");
                parseContext.recover();
                constUnion *unionArray = new constUnion[1];
                unionArray->iConst = 0;
                TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), yyvsp[0].lex.line);
                yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexDirect, yyvsp[-2].interm.intermTypedNode, index, yyvsp[-1].lex.line);                
                yyval.interm.intermTypedNode->setType(TType(yyvsp[-2].interm.intermTypedNode->getBasicType(), EvqTemporary, yyvsp[-2].interm.intermTypedNode->getNominalSize()));
            } else {
                constUnion *unionArray = new constUnion[1];
                unionArray->iConst = fields.col * yyvsp[-2].interm.intermTypedNode->getNominalSize() + fields.row;
                TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), yyvsp[0].lex.line);
                yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexDirect, yyvsp[-2].interm.intermTypedNode, index, yyvsp[-1].lex.line);                
                yyval.interm.intermTypedNode->setType(TType(yyvsp[-2].interm.intermTypedNode->getBasicType()));
            }
        } else if (yyvsp[-2].interm.intermTypedNode->getBasicType() == EbtStruct) {
            bool fieldFound = false;
            TTypeList* fields = yyvsp[-2].interm.intermTypedNode->getType().getStruct();
            if (fields == 0) {
                parseContext.error(yyvsp[-1].lex.line, "structure has no fields", "Internal Error", "");
                parseContext.recover();
                yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
            } else {
                unsigned int i;
                for (i = 0; i < fields->size(); ++i) {
                    if ((*fields)[i].type->getFieldName() == *yyvsp[0].lex.string) {
                        fieldFound = true;
                        break;
                    }                
                }
                if (fieldFound) {
                    if (yyvsp[-2].interm.intermTypedNode->getType().getQualifier() == EvqConst) {
                        yyval.interm.intermTypedNode = parseContext.addConstStruct(*yyvsp[0].lex.string, yyvsp[-2].interm.intermTypedNode, yyvsp[-1].lex.line);
                        if (yyval.interm.intermTypedNode == 0) {
                            parseContext.recover();
                            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
                        }
                        else {
                            yyval.interm.intermTypedNode->setType(*(*fields)[i].type);
                            // change the qualifier of the return type, not of the structure field
                            // as the structure definition is shared between various structures.
                            yyval.interm.intermTypedNode->getTypePointer()->changeQualifier(EvqConst);
                        }
                    } else {
                        constUnion *unionArray = new constUnion[1];
                        unionArray->iConst = i;
                        TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), yyvsp[0].lex.line);
                        yyval.interm.intermTypedNode = parseContext.intermediate.addIndex(EOpIndexDirectStruct, yyvsp[-2].interm.intermTypedNode, index, yyvsp[-1].lex.line);                
                        yyval.interm.intermTypedNode->setType(*(*fields)[i].type);
                    }
                } else {
                    parseContext.error(yyvsp[-1].lex.line, " no such field in structure", yyvsp[0].lex.string->c_str(), "");
                    parseContext.recover();
                    yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
                }
            }
        } else {
            parseContext.error(yyvsp[-1].lex.line, " field selection requires structure, vector, or matrix on left hand side", yyvsp[0].lex.string->c_str(), "");
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
        // don't delete $3.string, it's from the pool
    ;}
    break;

  case 12:
#line 460 "glslang.y"
    {
        if (parseContext.lValueErrorCheck(yyvsp[0].lex.line, "++", yyvsp[-1].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(EOpPostIncrement, yyvsp[-1].interm.intermTypedNode, yyvsp[0].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.unaryOpError(yyvsp[0].lex.line, "++", yyvsp[-1].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-1].interm.intermTypedNode;
        }
    ;}
    break;

  case 13:
#line 470 "glslang.y"
    {
        if (parseContext.lValueErrorCheck(yyvsp[0].lex.line, "--", yyvsp[-1].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(EOpPostDecrement, yyvsp[-1].interm.intermTypedNode, yyvsp[0].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.unaryOpError(yyvsp[0].lex.line, "--", yyvsp[-1].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-1].interm.intermTypedNode;
        }
    ;}
    break;

  case 14:
#line 483 "glslang.y"
    {
        if (parseContext.integerErrorCheck(yyvsp[0].interm.intermTypedNode, "[]"))
            parseContext.recover();
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; 
    ;}
    break;

  case 15:
#line 491 "glslang.y"
    {
        TFunction* fnCall = yyvsp[0].interm.function;
        TOperator op = fnCall->getBuiltInOp();
        
        if (op != EOpNull) {
            //
            // Then this should be a constructor.
            //
            TType type(EbtVoid);  // use this to get the type back
            if (parseContext.constructorErrorCheck(yyvsp[0].interm.line, yyvsp[0].interm.intermNode, *fnCall, op, &type)) {
                yyval.interm.intermTypedNode = 0;
            } else {
                //
                // It's a constructor, of type 'type'.
                //
                yyval.interm.intermTypedNode = parseContext.addConstructor(yyvsp[0].interm.intermNode, &type, op, fnCall, yyvsp[0].interm.line);
            }
            
            if (yyval.interm.intermTypedNode == 0) {        
                parseContext.recover();
                yyval.interm.intermTypedNode = parseContext.intermediate.setAggregateOperator(0, op, yyvsp[0].interm.line);
            }
            yyval.interm.intermTypedNode->setType(type);
        } else {
            //
            // Not a constructor.  Find it in the symbol table.
            //
            const TFunction* fnCandidate;
            bool builtIn;
            fnCandidate = parseContext.findFunction(yyvsp[0].interm.line, fnCall, &builtIn);
            if (fnCandidate) {
                //
                // A declared function.  But, it might still map to a built-in
                // operation.
                //
                op = fnCandidate->getBuiltInOp();
                if (builtIn && op != EOpNull) {
                    //
                    // A function call mapped to a built-in operation.
                    //
                    if (fnCandidate->getParamCount() == 1) {
                        //
                        // Treat it like a built-in unary operator.
                        //
                        yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(op, yyvsp[0].interm.intermNode, 0, parseContext.symbolTable);
                        if (yyval.interm.intermTypedNode == 0)  {
                            parseContext.error(yyvsp[0].interm.intermNode->getLine(), " wrong operand type", "Internal Error", 
                                "built in unary operator function.  Type: %s",
                                static_cast<TIntermTyped*>(yyvsp[0].interm.intermNode)->getCompleteString().c_str());
                            YYERROR;
                        }
                    } else {
                        yyval.interm.intermTypedNode = parseContext.intermediate.setAggregateOperator(yyvsp[0].interm.intermAggregate, op, yyvsp[0].interm.line);
                    }
                } else {
                    // This is a real function call
                    
                    yyval.interm.intermTypedNode = parseContext.intermediate.setAggregateOperator(yyvsp[0].interm.intermAggregate, EOpFunctionCall, yyvsp[0].interm.line);
                    yyval.interm.intermTypedNode->setType(fnCandidate->getReturnType());                   
                    
                    // this is how we know whether the given function is a builtIn function or a user defined function
                    // if builtIn == false, it's a userDefined -> could be an overloaded builtIn function also
                    // if builtIn == true, it's definitely a builtIn function with EOpNull
                    if (!builtIn) 
                        yyval.interm.intermTypedNode->getAsAggregate()->setUserDefined(); 
                    yyval.interm.intermTypedNode->getAsAggregate()->setName(fnCandidate->getMangledName());

                    TQualifier qual;
                    TQualifierList& qualifierList = yyval.interm.intermTypedNode->getAsAggregate()->getQualifier();
                    for (int i = 0; i < fnCandidate->getParamCount(); ++i) {
                        qual = (*fnCandidate)[i].type->getQualifier();
                        if (qual == EvqOut || qual == EvqInOut) {
                            if (parseContext.lValueErrorCheck(yyval.interm.intermTypedNode->getLine(), "assign", yyval.interm.intermTypedNode->getAsAggregate()->getSequence()[i]->getAsTyped())) {
                                parseContext.error(yyvsp[0].interm.intermNode->getLine(), "Constant value cannot be passed for 'out' or 'inout' parameters.", "Error", "");
                                parseContext.recover();
                            }
                        }
                        qualifierList.push_back(qual);
                    }
                }
                yyval.interm.intermTypedNode->setType(fnCandidate->getReturnType());
            } else {
                // error message was put out by PaFindFunction()
                // Put on a dummy node for error recovery
                constUnion *unionArray = new constUnion[1];
                unionArray->fConst = 0.0;
                yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), yyvsp[0].interm.line);
                parseContext.recover();
            }
        }
        delete fnCall;
    ;}
    break;

  case 16:
#line 586 "glslang.y"
    {
        yyval.interm = yyvsp[-1].interm;
        yyval.interm.line = yyvsp[0].lex.line;
    ;}
    break;

  case 17:
#line 590 "glslang.y"
    {
        yyval.interm = yyvsp[-1].interm;
        yyval.interm.line = yyvsp[0].lex.line;
    ;}
    break;

  case 18:
#line 597 "glslang.y"
    {
        yyval.interm.function = yyvsp[-1].interm.function;
        yyval.interm.intermNode = 0;
    ;}
    break;

  case 19:
#line 601 "glslang.y"
    {
        yyval.interm.function = yyvsp[0].interm.function;
        yyval.interm.intermNode = 0;
    ;}
    break;

  case 20:
#line 608 "glslang.y"
    {
        TParameter param = { 0, new TType(yyvsp[0].interm.intermTypedNode->getType()) };
        yyvsp[-1].interm.function->addParameter(param);
        yyval.interm.function = yyvsp[-1].interm.function;
        yyval.interm.intermNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 21:
#line 614 "glslang.y"
    {
        TParameter param = { 0, new TType(yyvsp[0].interm.intermTypedNode->getType()) };
        yyvsp[-2].interm.function->addParameter(param);
        yyval.interm.function = yyvsp[-2].interm.function;
        yyval.interm.intermNode = parseContext.intermediate.growAggregate(yyvsp[-2].interm.intermNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line);
    ;}
    break;

  case 22:
#line 623 "glslang.y"
    {
        yyval.interm.function = yyvsp[-1].interm.function;
    ;}
    break;

  case 23:
#line 629 "glslang.y"
    {
        if (yyvsp[0].interm.op == EOpConstructStruct) {
            TString tempString = "";
            TFunction *function = new TFunction(&tempString, *(yyvsp[0].interm.type.userDef), yyvsp[0].interm.op);
            yyval.interm.function = function;
        }
        else {
            TFunction *function = new TFunction(yyvsp[0].interm.op);
            yyval.interm.function = function;
        }
    ;}
    break;

  case 24:
#line 640 "glslang.y"
    {
        if (parseContext.reservedErrorCheck(yyvsp[0].lex.line, *yyvsp[0].lex.string)) 
            parseContext.recover();
        TType type(EbtVoid);
        TFunction *function = new TFunction(yyvsp[0].lex.string, type);
        yyval.interm.function = function;        
    ;}
    break;

  case 25:
#line 656 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructFloat; ;}
    break;

  case 26:
#line 657 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructInt;   ;}
    break;

  case 27:
#line 658 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructBool;  ;}
    break;

  case 28:
#line 659 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructVec2;  ;}
    break;

  case 29:
#line 660 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructVec3;  ;}
    break;

  case 30:
#line 661 "glslang.y"
    {                                   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructVec4;  ;}
    break;

  case 31:
#line 662 "glslang.y"
    { FRAG_VERT_ONLY("bvec2", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructBVec2; ;}
    break;

  case 32:
#line 663 "glslang.y"
    { FRAG_VERT_ONLY("bvec3", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructBVec3; ;}
    break;

  case 33:
#line 664 "glslang.y"
    { FRAG_VERT_ONLY("bvec4", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructBVec4; ;}
    break;

  case 34:
#line 665 "glslang.y"
    { FRAG_VERT_ONLY("ivec2", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructIVec2; ;}
    break;

  case 35:
#line 666 "glslang.y"
    { FRAG_VERT_ONLY("ivec3", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructIVec3; ;}
    break;

  case 36:
#line 667 "glslang.y"
    { FRAG_VERT_ONLY("ivec4", yyvsp[0].lex.line); yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructIVec4; ;}
    break;

  case 37:
#line 668 "glslang.y"
    { FRAG_VERT_ONLY("mat2", yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructMat2;  ;}
    break;

  case 38:
#line 669 "glslang.y"
    { FRAG_VERT_ONLY("mat3", yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructMat3;  ;}
    break;

  case 39:
#line 670 "glslang.y"
    { FRAG_VERT_ONLY("mat4", yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpConstructMat4;  ;}
    break;

  case 40:
#line 671 "glslang.y"
    {                                   
        TType& structure = static_cast<TVariable*>(yyvsp[0].lex.symbol)->getType();
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtStruct, qual, 1, false, false, &structure, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
        yyval.interm.line = yyvsp[0].lex.line; 
        yyval.interm.op = EOpConstructStruct; 
    ;}
    break;

  case 41:
#line 682 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 42:
#line 685 "glslang.y"
    {
        if (parseContext.lValueErrorCheck(yyvsp[-1].lex.line, "++", yyvsp[0].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(EOpPreIncrement, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.unaryOpError(yyvsp[-1].lex.line, "++", yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        }
    ;}
    break;

  case 43:
#line 695 "glslang.y"
    {
        if (parseContext.lValueErrorCheck(yyvsp[-1].lex.line, "--", yyvsp[0].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(EOpPreDecrement, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.unaryOpError(yyvsp[-1].lex.line, "--", yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        }
    ;}
    break;

  case 44:
#line 705 "glslang.y"
    {
        if (yyvsp[-1].interm.op != EOpNull) {
            yyval.interm.intermTypedNode = parseContext.intermediate.addUnaryMath(yyvsp[-1].interm.op, yyvsp[0].interm.intermTypedNode, yyvsp[-1].interm.line, parseContext.symbolTable);
            if (yyval.interm.intermTypedNode == 0) {
                char* errorOp = "";
                switch(yyvsp[-1].interm.op) {
                case EOpNegative:   errorOp = "-"; break;
                case EOpLogicalNot: errorOp = "!"; break;
                case EOpBitwiseNot: errorOp = "~"; break;
				default: break;
                }
                parseContext.unaryOpError(yyvsp[-1].interm.line, errorOp, yyvsp[0].interm.intermTypedNode->getCompleteString());
                parseContext.recover();
                yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
            }
        } else
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 45:
#line 727 "glslang.y"
    { yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpNull; ;}
    break;

  case 46:
#line 728 "glslang.y"
    { yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpNegative; ;}
    break;

  case 47:
#line 729 "glslang.y"
    { yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpLogicalNot; ;}
    break;

  case 48:
#line 730 "glslang.y"
    { PACK_UNPACK_ONLY("~", yyvsp[0].lex.line);  
              yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpBitwiseNot; ;}
    break;

  case 49:
#line 736 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 50:
#line 737 "glslang.y"
    {
        FRAG_VERT_ONLY("*", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpMul, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "*", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 51:
#line 746 "glslang.y"
    {
        FRAG_VERT_ONLY("/", yyvsp[-1].lex.line); 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpDiv, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "/", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 52:
#line 755 "glslang.y"
    {
        PACK_UNPACK_ONLY("%", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpMod, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "%", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 53:
#line 767 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 54:
#line 768 "glslang.y"
    {  
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpAdd, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "+", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 55:
#line 776 "glslang.y"
    {
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpSub, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "-", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        } 
    ;}
    break;

  case 56:
#line 787 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 57:
#line 788 "glslang.y"
    {
        PACK_UNPACK_ONLY("<<", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLeftShift, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "<<", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 58:
#line 797 "glslang.y"
    {
        PACK_UNPACK_ONLY(">>", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpRightShift, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, ">>", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 59:
#line 809 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 60:
#line 810 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLessThan, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "<", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 61:
#line 820 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpGreaterThan, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, ">", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 62:
#line 830 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLessThanEqual, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "<=", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 63:
#line 840 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpGreaterThanEqual, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, ">=", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 64:
#line 853 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 65:
#line 854 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpEqual, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "==", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 66:
#line 864 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpNotEqual, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "!=", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 67:
#line 877 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 68:
#line 878 "glslang.y"
    {
        PACK_UNPACK_ONLY("&", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpAnd, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "&", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 69:
#line 890 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 70:
#line 891 "glslang.y"
    {
        PACK_UNPACK_ONLY("^", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpExclusiveOr, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "^", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 71:
#line 903 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 72:
#line 904 "glslang.y"
    {
        PACK_UNPACK_ONLY("|", yyvsp[-1].lex.line);
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpInclusiveOr, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "|", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 73:
#line 916 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 74:
#line 917 "glslang.y"
    {
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLogicalAnd, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "&&", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 75:
#line 930 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 76:
#line 931 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLogicalXor, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "^^", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 77:
#line 944 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 78:
#line 945 "glslang.y"
    { 
        yyval.interm.intermTypedNode = parseContext.intermediate.addBinaryMath(EOpLogicalOr, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line, parseContext.symbolTable);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, "||", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            yyval.interm.intermTypedNode = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), yyvsp[-1].lex.line);
        }
    ;}
    break;

  case 79:
#line 958 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 80:
#line 959 "glslang.y"
    {
       if (parseContext.boolErrorCheck(yyvsp[-3].lex.line, yyvsp[-4].interm.intermTypedNode))
            parseContext.recover();
       
        yyval.interm.intermTypedNode = parseContext.intermediate.addSelection(yyvsp[-4].interm.intermTypedNode, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-3].lex.line);
        if (yyvsp[-2].interm.intermTypedNode->getType() != yyvsp[0].interm.intermTypedNode->getType())
            yyval.interm.intermTypedNode = 0;
            
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-3].lex.line, ":", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        }
    ;}
    break;

  case 81:
#line 976 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 82:
#line 977 "glslang.y"
    {
        if (parseContext.lValueErrorCheck(yyvsp[-1].interm.line, "assign", yyvsp[-2].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = parseContext.intermediate.addAssign(yyvsp[-1].interm.op, yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].interm.line);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.assignError(yyvsp[-1].interm.line, "assign", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[-2].interm.intermTypedNode;
        }
    ;}
    break;

  case 83:
#line 990 "glslang.y"
    {                                    yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpAssign; ;}
    break;

  case 84:
#line 991 "glslang.y"
    { FRAG_VERT_ONLY("*=", yyvsp[0].lex.line);     yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpMulAssign; ;}
    break;

  case 85:
#line 992 "glslang.y"
    { FRAG_VERT_ONLY("/=", yyvsp[0].lex.line);     yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpDivAssign; ;}
    break;

  case 86:
#line 993 "glslang.y"
    { PACK_UNPACK_ONLY("%=", yyvsp[0].lex.line);   yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpModAssign; ;}
    break;

  case 87:
#line 994 "glslang.y"
    {                                    yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpAddAssign; ;}
    break;

  case 88:
#line 995 "glslang.y"
    {                                    yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpSubAssign; ;}
    break;

  case 89:
#line 996 "glslang.y"
    { PACK_UNPACK_ONLY("<<=", yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpLeftShiftAssign; ;}
    break;

  case 90:
#line 997 "glslang.y"
    { PACK_UNPACK_ONLY("<<=", yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpRightShiftAssign; ;}
    break;

  case 91:
#line 998 "glslang.y"
    { PACK_UNPACK_ONLY("&=",  yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpAndAssign; ;}
    break;

  case 92:
#line 999 "glslang.y"
    { PACK_UNPACK_ONLY("^=",  yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpExclusiveOrAssign; ;}
    break;

  case 93:
#line 1000 "glslang.y"
    { PACK_UNPACK_ONLY("|=",  yyvsp[0].lex.line);  yyval.interm.line = yyvsp[0].lex.line; yyval.interm.op = EOpInclusiveOrAssign; ;}
    break;

  case 94:
#line 1004 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 95:
#line 1007 "glslang.y"
    {
        yyval.interm.intermTypedNode = parseContext.intermediate.addComma(yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.intermTypedNode, yyvsp[-1].lex.line);
        if (yyval.interm.intermTypedNode == 0) {
            parseContext.binaryOpError(yyvsp[-1].lex.line, ",", yyvsp[-2].interm.intermTypedNode->getCompleteString(), yyvsp[0].interm.intermTypedNode->getCompleteString());
            parseContext.recover();
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        }
    ;}
    break;

  case 96:
#line 1018 "glslang.y"
    {
        if (parseContext.constErrorCheck(yyvsp[0].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 97:
#line 1026 "glslang.y"
    { yyval.interm.intermNode = 0; ;}
    break;

  case 98:
#line 1027 "glslang.y"
    { 
        if (yyvsp[-1].interm.intermAggregate)
            yyvsp[-1].interm.intermAggregate->setOperator(EOpSequence); 
        yyval.interm.intermNode = yyvsp[-1].interm.intermAggregate; 
    ;}
    break;

  case 99:
#line 1035 "glslang.y"
    {
        //
        // Multiple declarations of the same function are allowed.
        //
        // If this is a definition, the definition production code will check for redefinitions 
        // (we don't know at this point if it's a definition or not).
        //
        // Redeclarations are allowed.  But, return types and parameter qualifiers must match.
        //        
        TFunction* prevDec = static_cast<TFunction*>(parseContext.symbolTable.find(yyvsp[-1].interm.function->getMangledName()));
        if (prevDec) {
            if (prevDec->getReturnType() != yyvsp[-1].interm.function->getReturnType()) {
                parseContext.error(yyvsp[0].lex.line, "overloaded functions must have the same return type", yyvsp[-1].interm.function->getReturnType().getBasicString(), "");
                parseContext.recover();
            }
            for (int i = 0; i < prevDec->getParamCount(); ++i) {
                if ((*prevDec)[i].type->getQualifier() != (*yyvsp[-1].interm.function)[i].type->getQualifier()) {
                    parseContext.error(yyvsp[0].lex.line, "overloaded functions must have the same parameter qualifiers", (*yyvsp[-1].interm.function)[i].type->getQualifierString(), "");
                    parseContext.recover();
                }
            }
        }
        
        //
        // If this is a redeclaration, it could also be a definition,
        // in which case, we want to use the variable names from this one, and not the one that's
        // being redeclared.  So, pass back up this declaration, not the one in the symbol table.
        //
        yyval.interm.function = yyvsp[-1].interm.function;
        yyval.interm.line = yyvsp[0].lex.line;

        parseContext.symbolTable.insert(*yyval.interm.function);
    ;}
    break;

  case 100:
#line 1071 "glslang.y"
    {
        yyval.interm.function = yyvsp[0].interm.function;
    ;}
    break;

  case 101:
#line 1074 "glslang.y"
    { 
        yyval.interm.function = yyvsp[0].interm.function;  
    ;}
    break;

  case 102:
#line 1081 "glslang.y"
    {
        // Add the parameter 
        yyval.interm.function = yyvsp[-1].interm.function;
        if (yyvsp[0].interm.param.type->getBasicType() != EbtVoid)
            yyvsp[-1].interm.function->addParameter(yyvsp[0].interm.param);
        else
            delete yyvsp[0].interm.param.type;
    ;}
    break;

  case 103:
#line 1089 "glslang.y"
    {   
        //
        // Only first parameter of one-parameter functions can be void
        // The check for named parameters not being void is done in parameter_declarator 
        //
        if (yyvsp[0].interm.param.type->getBasicType() == EbtVoid) {
            //
            // This parameter > first is void
            //
            parseContext.error(yyvsp[-1].lex.line, "cannot be an argument type except for '(void)'", "void", "");
            parseContext.recover();
            delete yyvsp[0].interm.param.type;
        } else {
            // Add the parameter 
            yyval.interm.function = yyvsp[-2].interm.function; 
            yyvsp[-2].interm.function->addParameter(yyvsp[0].interm.param);
        }
    ;}
    break;

  case 104:
#line 1110 "glslang.y"
    {
        if (yyvsp[-2].interm.type.qualifier != EvqGlobal && yyvsp[-2].interm.type.qualifier != EvqTemporary) {
            parseContext.error(yyvsp[-1].lex.line, "no qualifiers allowed for function return", getQualifierString(yyvsp[-2].interm.type.qualifier), "");
            parseContext.recover();
        }
        // make sure a sampler is not involved as well...
        if (parseContext.structQualifierErrorCheck(yyvsp[-1].lex.line, yyvsp[-2].interm.type))
            parseContext.recover();
        
        // Add the function as a prototype after parsing it (we do not support recursion) 
        TFunction *function;
        TType type(yyvsp[-2].interm.type);
        function = new TFunction(yyvsp[-1].lex.string, type);
        yyval.interm.function = function;
    ;}
    break;

  case 105:
#line 1129 "glslang.y"
    {
        if (yyvsp[-1].interm.type.type == EbtVoid) {
            parseContext.error(yyvsp[0].lex.line, "illegal use of type 'void'", yyvsp[0].lex.string->c_str(), "");
            parseContext.recover();
        }
        if (parseContext.reservedErrorCheck(yyvsp[0].lex.line, *yyvsp[0].lex.string))
            parseContext.recover();
        TParameter param = {yyvsp[0].lex.string, new TType(yyvsp[-1].interm.type)};
        yyval.interm.line = yyvsp[0].lex.line;
        yyval.interm.param = param;
    ;}
    break;

  case 106:
#line 1140 "glslang.y"
    {
        // Check that we can make an array out of this type
        if (yyvsp[-4].interm.type.array) {
            parseContext.error(yyvsp[-2].lex.line, "cannot declare arrays of this type", TType(yyvsp[-4].interm.type).getCompleteString().c_str(), "");
            parseContext.recover();
        }
        if (parseContext.reservedErrorCheck(yyvsp[-3].lex.line, *yyvsp[-3].lex.string))
            parseContext.recover();
        yyvsp[-4].interm.type.array = true;
        TType* type = new TType(yyvsp[-4].interm.type);        
        if (yyvsp[-1].interm.intermTypedNode->getAsConstantUnion())
            type->setArraySize(yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst);
        TParameter param = { yyvsp[-3].lex.string, type };
        yyval.interm.line = yyvsp[-3].lex.line;
        yyval.interm.param = param;
    ;}
    break;

  case 107:
#line 1167 "glslang.y"
    {
        yyval.interm = yyvsp[0].interm;
        if (parseContext.paramErrorCheck(yyvsp[0].interm.line, yyvsp[-2].interm.type.qualifier, yyvsp[-1].interm.qualifier, yyval.interm.param.type))
            parseContext.recover();
    ;}
    break;

  case 108:
#line 1172 "glslang.y"
    {
        yyval.interm = yyvsp[0].interm;
        if (parseContext.parameterSamplerErrorCheck(yyvsp[0].interm.line, yyvsp[-1].interm.qualifier, *yyvsp[0].interm.param.type))
            parseContext.recover();
        if (parseContext.paramErrorCheck(yyvsp[0].interm.line, EvqTemporary, yyvsp[-1].interm.qualifier, yyval.interm.param.type))
            parseContext.recover();
    ;}
    break;

  case 109:
#line 1182 "glslang.y"
    {
        yyval.interm = yyvsp[0].interm;
        if (parseContext.paramErrorCheck(yyvsp[0].interm.line, yyvsp[-2].interm.type.qualifier, yyvsp[-1].interm.qualifier, yyval.interm.param.type))
            parseContext.recover();
    ;}
    break;

  case 110:
#line 1187 "glslang.y"
    {
        yyval.interm = yyvsp[0].interm;
        if (parseContext.parameterSamplerErrorCheck(yyvsp[0].interm.line, yyvsp[-1].interm.qualifier, *yyvsp[0].interm.param.type))
            parseContext.recover();
        if (parseContext.paramErrorCheck(yyvsp[0].interm.line, EvqTemporary, yyvsp[-1].interm.qualifier, yyval.interm.param.type))
            parseContext.recover();
    ;}
    break;

  case 111:
#line 1197 "glslang.y"
    {
        yyval.interm.qualifier = EvqIn;
    ;}
    break;

  case 112:
#line 1200 "glslang.y"
    {
        yyval.interm.qualifier = EvqIn;
    ;}
    break;

  case 113:
#line 1203 "glslang.y"
    {
        yyval.interm.qualifier = EvqOut;
    ;}
    break;

  case 114:
#line 1206 "glslang.y"
    {
        yyval.interm.qualifier = EvqInOut;
    ;}
    break;

  case 115:
#line 1212 "glslang.y"
    {
        TParameter param = { 0, new TType(yyvsp[0].interm.type) };
        yyval.interm.param = param;
        
    ;}
    break;

  case 116:
#line 1217 "glslang.y"
    {
        // Check that we can make an array out of this type 
        if (yyvsp[-3].interm.type.array) {
            parseContext.error(yyvsp[-2].lex.line, "cannot declare arrays of this type", TType(yyvsp[-3].interm.type).getCompleteString().c_str(), "");
            parseContext.recover();
        }
        yyvsp[-3].interm.type.array = true;
        TType* type = new TType(yyvsp[-3].interm.type);       
        if (yyvsp[-1].interm.intermTypedNode->getAsConstantUnion())
            type->setArraySize(yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst);

        TParameter param = { 0, type };
        yyval.interm.line = yyvsp[-2].lex.line;
        yyval.interm.param = param;
    ;}
    break;

  case 117:
#line 1235 "glslang.y"
    {
        yyval.interm = yyvsp[0].interm;
    ;}
    break;

  case 118:
#line 1238 "glslang.y"
    {
        yyval.interm = yyvsp[-2].interm;
        if (parseContext.structQualifierErrorCheck(yyvsp[0].lex.line, yyvsp[-2].interm.type))
            parseContext.recover();
        
        if (parseContext.nonInitErrorCheck(yyvsp[0].lex.line, *yyvsp[0].lex.string, yyval.interm.type))
            parseContext.recover();
    ;}
    break;

  case 119:
#line 1246 "glslang.y"
    {
        yyval.interm = yyvsp[-4].interm;
        if (parseContext.structQualifierErrorCheck(yyvsp[-2].lex.line, yyvsp[-4].interm.type))
            parseContext.recover();
            
        if (parseContext.arrayErrorCheck(yyvsp[-1].lex.line, *yyvsp[-2].lex.string, yyval.interm.type, 0))
            parseContext.recover();
    ;}
    break;

  case 120:
#line 1254 "glslang.y"
    {
        yyval.interm = yyvsp[-5].interm;
        if (parseContext.structQualifierErrorCheck(yyvsp[-3].lex.line, yyvsp[-5].interm.type))
            parseContext.recover();
            
        if (parseContext.arrayErrorCheck(yyvsp[-2].lex.line, *yyvsp[-3].lex.string, yyval.interm.type, yyvsp[-1].interm.intermTypedNode))
            parseContext.recover();
    ;}
    break;

  case 121:
#line 1262 "glslang.y"
    {
        yyval.interm = yyvsp[-4].interm;
        if (parseContext.structQualifierErrorCheck(yyvsp[-2].lex.line, yyvsp[-4].interm.type))
            parseContext.recover();
        
        TIntermNode* intermNode;
        if (!parseContext.executeInitializer(yyvsp[-2].lex.line, *yyvsp[-2].lex.string, yyvsp[-4].interm.type, yyvsp[0].interm.intermTypedNode, intermNode)) {
            //
            // build the intermediate representation
            //
            if (intermNode)
                yyval.interm.intermAggregate = parseContext.intermediate.growAggregate(yyvsp[-4].interm.intermNode, intermNode, yyvsp[-1].lex.line);
            else
                yyval.interm.intermAggregate = yyvsp[-4].interm.intermAggregate;
        } else {
            parseContext.recover();
            yyval.interm.intermAggregate = 0;
        }
    ;}
    break;

  case 122:
#line 1284 "glslang.y"
    {
        yyval.interm.type = yyvsp[0].interm.type;
        yyval.interm.intermAggregate = 0;
    ;}
    break;

  case 123:
#line 1288 "glslang.y"
    {
        yyval.interm.intermAggregate = 0;
        yyval.interm.type = yyvsp[-1].interm.type;
        if (parseContext.structQualifierErrorCheck(yyvsp[0].lex.line, yyvsp[-1].interm.type))
            parseContext.recover();
        
        if (parseContext.nonInitErrorCheck(yyvsp[0].lex.line, *yyvsp[0].lex.string, yyval.interm.type))
            parseContext.recover();
    ;}
    break;

  case 124:
#line 1297 "glslang.y"
    {
        yyval.interm.intermAggregate = 0;
        yyval.interm.type = yyvsp[-3].interm.type;
        if (parseContext.structQualifierErrorCheck(yyvsp[-2].lex.line, yyvsp[-3].interm.type))
            parseContext.recover();
        
        if (parseContext.arrayErrorCheck(yyvsp[-1].lex.line, *yyvsp[-2].lex.string, yyval.interm.type, 0))
            parseContext.recover();
    ;}
    break;

  case 125:
#line 1306 "glslang.y"
    {
        yyval.interm.intermAggregate = 0;
        yyval.interm.type = yyvsp[-4].interm.type;
        if (parseContext.structQualifierErrorCheck(yyvsp[-3].lex.line, yyvsp[-4].interm.type))
            parseContext.recover();
        
        if (parseContext.arrayErrorCheck(yyvsp[-2].lex.line, *yyvsp[-3].lex.string, yyval.interm.type, yyvsp[-1].interm.intermTypedNode))
            parseContext.recover();
    ;}
    break;

  case 126:
#line 1315 "glslang.y"
    {
        yyval.interm.type = yyvsp[-3].interm.type;
        if (parseContext.structQualifierErrorCheck(yyvsp[-2].lex.line, yyvsp[-3].interm.type))
            parseContext.recover();
        
        TIntermNode* intermNode;
        if (!parseContext.executeInitializer(yyvsp[-2].lex.line, *yyvsp[-2].lex.string, yyvsp[-3].interm.type, yyvsp[0].interm.intermTypedNode, intermNode)) {
            //
            // Build intermediate representation
            //
            if (intermNode)
                yyval.interm.intermAggregate = parseContext.intermediate.makeAggregate(intermNode, yyvsp[-1].lex.line);
            else
                yyval.interm.intermAggregate = 0;
        } else {
            parseContext.recover();
            yyval.interm.intermAggregate = 0;
        }
    ;}
    break;

  case 127:
#line 1405 "glslang.y"
    {
        yyval.interm.type = yyvsp[0].interm.type;
    ;}
    break;

  case 128:
#line 1408 "glslang.y"
    { 
        TPublicType t = { yyvsp[0].interm.type.type, yyvsp[-1].interm.type.qualifier, yyvsp[0].interm.type.size, yyvsp[0].interm.type.matrix, false, yyvsp[0].interm.type.userDef, 0 };
        if (yyvsp[-1].interm.type.qualifier == EvqAttribute &&
            (yyvsp[0].interm.type.type == EbtBool || yyvsp[0].interm.type.type == EbtInt)) {
            parseContext.error(yyvsp[0].interm.type.line, "cannot be bool or int", getQualifierString(yyvsp[-1].interm.type.qualifier), "");
            parseContext.recover();
        }
        if ((yyvsp[-1].interm.type.qualifier == EvqVaryingIn || yyvsp[-1].interm.type.qualifier == EvqVaryingOut) &&
            (yyvsp[0].interm.type.type == EbtBool || yyvsp[0].interm.type.type == EbtInt)) {
            parseContext.error(yyvsp[0].interm.type.line, "cannot be bool or int", getQualifierString(yyvsp[-1].interm.type.qualifier), "");
            parseContext.recover();
        }
        yyval.interm.type = t;
    ;}
    break;

  case 129:
#line 1425 "glslang.y"
    { 
        TPublicType t = { EbtVoid,  EvqConst,     1, false, false, 0 }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 130:
#line 1429 "glslang.y"
    { 
        VERTEX_ONLY("attribute", yyvsp[0].lex.line);
        if (parseContext.globalErrorCheck(yyvsp[0].lex.line, parseContext.symbolTable.atGlobalLevel(), "attribute"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqAttribute, 1, false, false, 0 }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 131:
#line 1436 "glslang.y"
    {
        if (parseContext.globalErrorCheck(yyvsp[0].lex.line, parseContext.symbolTable.atGlobalLevel(), "varying"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqVaryingIn, 1, false, false, 0 };
        if (parseContext.language == EShLangVertex)
            t.qualifier = EvqVaryingOut;
        yyval.interm.type = t; 
    ;}
    break;

  case 132:
#line 1444 "glslang.y"
    {
        if (parseContext.globalErrorCheck(yyvsp[0].lex.line, parseContext.symbolTable.atGlobalLevel(), "uniform"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqUniform,   1, false, false, 0 }; 
        yyval.interm.type = t;
    ;}
    break;

  case 133:
#line 1453 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtVoid, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 134:
#line 1458 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 135:
#line 1463 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 136:
#line 1468 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 137:
#line 1479 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 2, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 138:
#line 1484 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 3, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 139:
#line 1489 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 4, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 140:
#line 1494 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 2, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 141:
#line 1499 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 3, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 142:
#line 1504 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 4, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 143:
#line 1509 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 2, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 144:
#line 1514 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 3, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 145:
#line 1519 "glslang.y"
    {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 4, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 146:
#line 1524 "glslang.y"
    {
        FRAG_VERT_ONLY("mat2", yyvsp[0].lex.line); 
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 2, true, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 147:
#line 1530 "glslang.y"
    { 
        FRAG_VERT_ONLY("mat3", yyvsp[0].lex.line); 
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 3, true, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 148:
#line 1536 "glslang.y"
    { 
        FRAG_VERT_ONLY("mat4", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 4, true, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t; 
    ;}
    break;

  case 149:
#line 1542 "glslang.y"
    {
        FRAG_VERT_ONLY("sampler1D", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler1D, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 150:
#line 1548 "glslang.y"
    {
        FRAG_VERT_ONLY("sampler2D", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler2D, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 151:
#line 1554 "glslang.y"
    {
        FRAG_VERT_ONLY("sampler3D", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler3D, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 152:
#line 1560 "glslang.y"
    {
        FRAG_VERT_ONLY("samplerCube", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSamplerCube, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 153:
#line 1566 "glslang.y"
    {
        FRAG_VERT_ONLY("sampler1DShadow", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler1DShadow, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 154:
#line 1572 "glslang.y"
    {
        FRAG_VERT_ONLY("sampler2DShadow", yyvsp[0].lex.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler2DShadow, qual, 1, false, false, 0, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 155:
#line 1578 "glslang.y"
    {
        FRAG_VERT_ONLY("struct", yyvsp[0].interm.type.line);
        yyval.interm.type = yyvsp[0].interm.type;
        yyval.interm.type.qualifier = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
    ;}
    break;

  case 156:
#line 1583 "glslang.y"
    {     
        //
        // This is for user defined type names.  The lexical phase looked up the 
        // type.
        //
        TType& structure = static_cast<TVariable*>(yyvsp[0].lex.symbol)->getType();
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtStruct, qual, 1, false, false, &structure, yyvsp[0].lex.line }; 
        yyval.interm.type = t;
    ;}
    break;

  case 157:
#line 1596 "glslang.y"
    {
        TType* structure = new TType(yyvsp[-1].interm.typeList, *yyvsp[-3].lex.string);
        TVariable* userTypeDef = new TVariable(yyvsp[-3].lex.string, *structure, true);
        if (! parseContext.symbolTable.insert(*userTypeDef)) {
            parseContext.error(yyvsp[-3].lex.line, "redefinition", yyvsp[-3].lex.string->c_str(), "struct");
            parseContext.recover();
        }
        TPublicType t = { EbtStruct, EvqTemporary, 1, false, false, structure, yyvsp[-4].lex.line };
        yyval.interm.type = t;
    ;}
    break;

  case 158:
#line 1606 "glslang.y"
    {
        TType* structure = new TType(yyvsp[-1].interm.typeList, TString(""));
        TPublicType t = { EbtStruct, EvqTemporary, 1, false, false, structure, yyvsp[-3].lex.line };
        yyval.interm.type = t;
    ;}
    break;

  case 159:
#line 1614 "glslang.y"
    {
        yyval.interm.typeList = yyvsp[0].interm.typeList;
    ;}
    break;

  case 160:
#line 1617 "glslang.y"
    {
        yyval.interm.typeList = yyvsp[-1].interm.typeList;
        for (unsigned int i = 0; i < yyvsp[0].interm.typeList->size(); ++i) {
            for (unsigned int j = 0; j < yyval.interm.typeList->size(); ++j) {
                if ((*yyval.interm.typeList)[j].type->getFieldName() == (*yyvsp[0].interm.typeList)[i].type->getFieldName()) {
                    parseContext.error((*yyvsp[0].interm.typeList)[i].line, "duplicate field name in structure:", "struct", (*yyvsp[0].interm.typeList)[i].type->getFieldName().c_str());
                    parseContext.recover();
                }
            }
            yyval.interm.typeList->push_back((*yyvsp[0].interm.typeList)[i]);
        }
    ;}
    break;

  case 161:
#line 1632 "glslang.y"
    {
        yyval.interm.typeList = yyvsp[-1].interm.typeList;
        
        if (parseContext.voidErrorCheck(yyvsp[-2].interm.type.line, (*yyvsp[-1].interm.typeList)[0].type->getFieldName(), yyvsp[-2].interm.type)) {
            parseContext.recover();
        }
        for (unsigned int i = 0; i < yyval.interm.typeList->size(); ++i) {
            //
            // Careful not to replace already know aspects of type, like array-ness
            //
            (*yyval.interm.typeList)[i].type->setType(yyvsp[-2].interm.type.type, yyvsp[-2].interm.type.size, yyvsp[-2].interm.type.matrix, yyvsp[-2].interm.type.userDef);
            if (yyvsp[-2].interm.type.userDef)
                (*yyval.interm.typeList)[i].type->setTypeName(yyvsp[-2].interm.type.userDef->getTypeName());
        }
    ;}
    break;

  case 162:
#line 1650 "glslang.y"
    {
        yyval.interm.typeList = NewPoolTTypeList();
        yyval.interm.typeList->push_back(yyvsp[0].interm.typeLine);
    ;}
    break;

  case 163:
#line 1654 "glslang.y"
    {
        yyval.interm.typeList->push_back(yyvsp[0].interm.typeLine);
    ;}
    break;

  case 164:
#line 1660 "glslang.y"
    {
        yyval.interm.typeLine.type = new TType(EbtVoid);
        yyval.interm.typeLine.line = yyvsp[0].lex.line;
        yyval.interm.typeLine.type->setFieldName(*yyvsp[0].lex.string);
    ;}
    break;

  case 165:
#line 1665 "glslang.y"
    {
        yyval.interm.typeLine.type = new TType(EbtVoid);
        yyval.interm.typeLine.line = yyvsp[-3].lex.line;
        yyval.interm.typeLine.type->setFieldName(*yyvsp[-3].lex.string);
        
        if (yyvsp[-1].interm.intermTypedNode->getAsConstantUnion() == 0 || yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getBasicType() != EbtInt ||
            yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst <= 0) {
            parseContext.error(yyvsp[-2].lex.line, "structure field array size must be a positive integer", yyvsp[-3].lex.string->c_str(), "");
            parseContext.recover();
        } else {           
            yyval.interm.typeLine.type->setArraySize(yyvsp[-1].interm.intermTypedNode->getAsConstantUnion()->getUnionArrayPointer()->iConst);
        }
    ;}
    break;

  case 166:
#line 1681 "glslang.y"
    { yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; ;}
    break;

  case 167:
#line 1685 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 168:
#line 1689 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermAggregate; ;}
    break;

  case 169:
#line 1690 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 170:
#line 1696 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 171:
#line 1697 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 172:
#line 1698 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 173:
#line 1699 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 174:
#line 1700 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 175:
#line 1704 "glslang.y"
    { yyval.interm.intermAggregate = 0; ;}
    break;

  case 176:
#line 1705 "glslang.y"
    { parseContext.symbolTable.push(); ;}
    break;

  case 177:
#line 1705 "glslang.y"
    { parseContext.symbolTable.pop(); ;}
    break;

  case 178:
#line 1705 "glslang.y"
    {
        if (yyvsp[-2].interm.intermAggregate != 0)            
            yyvsp[-2].interm.intermAggregate->setOperator(EOpSequence); 
        yyval.interm.intermAggregate = yyvsp[-2].interm.intermAggregate;
    ;}
    break;

  case 179:
#line 1713 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 180:
#line 1714 "glslang.y"
    { yyval.interm.intermNode = yyvsp[0].interm.intermNode; ;}
    break;

  case 181:
#line 1719 "glslang.y"
    { 
        yyval.interm.intermNode = 0; 
    ;}
    break;

  case 182:
#line 1722 "glslang.y"
    { 
        if (yyvsp[-1].interm.intermAggregate)
            yyvsp[-1].interm.intermAggregate->setOperator(EOpSequence); 
        yyval.interm.intermNode = yyvsp[-1].interm.intermAggregate; 
    ;}
    break;

  case 183:
#line 1730 "glslang.y"
    {
        yyval.interm.intermAggregate = parseContext.intermediate.makeAggregate(yyvsp[0].interm.intermNode, 0); 
    ;}
    break;

  case 184:
#line 1733 "glslang.y"
    { 
        yyval.interm.intermAggregate = parseContext.intermediate.growAggregate(yyvsp[-1].interm.intermAggregate, yyvsp[0].interm.intermNode, 0);
    ;}
    break;

  case 185:
#line 1739 "glslang.y"
    { yyval.interm.intermNode = 0; ;}
    break;

  case 186:
#line 1740 "glslang.y"
    { yyval.interm.intermNode = static_cast<TIntermNode*>(yyvsp[-1].interm.intermTypedNode); ;}
    break;

  case 187:
#line 1744 "glslang.y"
    { 
        if (parseContext.boolErrorCheck(yyvsp[-4].lex.line, yyvsp[-2].interm.intermTypedNode))
            parseContext.recover();
        yyval.interm.intermNode = parseContext.intermediate.addSelection(yyvsp[-2].interm.intermTypedNode, yyvsp[0].interm.nodePair, yyvsp[-4].lex.line);
    ;}
    break;

  case 188:
#line 1752 "glslang.y"
    {
        yyval.interm.nodePair.node1 = yyvsp[-2].interm.intermNode;
        yyval.interm.nodePair.node2 = yyvsp[0].interm.intermNode;
    ;}
    break;

  case 189:
#line 1756 "glslang.y"
    { 
        yyval.interm.nodePair.node1 = yyvsp[0].interm.intermNode;
        yyval.interm.nodePair.node2 = 0;
    ;}
    break;

  case 190:
#line 1766 "glslang.y"
    {
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        if (parseContext.boolErrorCheck(yyvsp[0].interm.intermTypedNode->getLine(), yyvsp[0].interm.intermTypedNode))
            parseContext.recover();          
    ;}
    break;

  case 191:
#line 1771 "glslang.y"
    {
        TIntermNode* intermNode;
        if (parseContext.structQualifierErrorCheck(yyvsp[-2].lex.line, yyvsp[-3].interm.type))
            parseContext.recover();
        if (parseContext.boolErrorCheck(yyvsp[-2].lex.line, yyvsp[-3].interm.type))
            parseContext.recover();
        
        if (!parseContext.executeInitializer(yyvsp[-2].lex.line, *yyvsp[-2].lex.string, yyvsp[-3].interm.type, yyvsp[0].interm.intermTypedNode, intermNode))
            yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode;
        else {
            parseContext.recover();
            yyval.interm.intermTypedNode = 0;
        }
    ;}
    break;

  case 192:
#line 1788 "glslang.y"
    { parseContext.symbolTable.push(); ++parseContext.loopNestingLevel; ;}
    break;

  case 193:
#line 1788 "glslang.y"
    { 
        parseContext.symbolTable.pop();
        yyval.interm.intermNode = parseContext.intermediate.addLoop(yyvsp[0].interm.intermNode, yyvsp[-2].interm.intermTypedNode, 0, true, yyvsp[-5].lex.line);
        --parseContext.loopNestingLevel;
    ;}
    break;

  case 194:
#line 1793 "glslang.y"
    { ++parseContext.loopNestingLevel; ;}
    break;

  case 195:
#line 1793 "glslang.y"
    {
        if (parseContext.boolErrorCheck(yyvsp[0].lex.line, yyvsp[-2].interm.intermTypedNode))
            parseContext.recover();
                    
        yyval.interm.intermNode = parseContext.intermediate.addLoop(yyvsp[-5].interm.intermNode, yyvsp[-2].interm.intermTypedNode, 0, false, yyvsp[-4].lex.line);
        --parseContext.loopNestingLevel;
    ;}
    break;

  case 196:
#line 1800 "glslang.y"
    { parseContext.symbolTable.push(); ++parseContext.loopNestingLevel; ;}
    break;

  case 197:
#line 1800 "glslang.y"
    {
        parseContext.symbolTable.pop();
        yyval.interm.intermNode = parseContext.intermediate.makeAggregate(yyvsp[-3].interm.intermNode, yyvsp[-5].lex.line);
        yyval.interm.intermNode = parseContext.intermediate.growAggregate(
                yyval.interm.intermNode,
                parseContext.intermediate.addLoop(yyvsp[0].interm.intermNode, reinterpret_cast<TIntermTyped*>(yyvsp[-2].interm.nodePair.node1), reinterpret_cast<TIntermTyped*>(yyvsp[-2].interm.nodePair.node2), true, yyvsp[-6].lex.line),
                yyvsp[-6].lex.line);
        yyval.interm.intermNode->getAsAggregate()->setOperator(EOpSequence);
        --parseContext.loopNestingLevel;
    ;}
    break;

  case 198:
#line 1813 "glslang.y"
    {
        yyval.interm.intermNode = yyvsp[0].interm.intermNode; 
    ;}
    break;

  case 199:
#line 1816 "glslang.y"
    {
        yyval.interm.intermNode = yyvsp[0].interm.intermNode;
    ;}
    break;

  case 200:
#line 1822 "glslang.y"
    { 
        yyval.interm.intermTypedNode = yyvsp[0].interm.intermTypedNode; 
    ;}
    break;

  case 201:
#line 1825 "glslang.y"
    { 
        yyval.interm.intermTypedNode = 0; 
    ;}
    break;

  case 202:
#line 1831 "glslang.y"
    { 
        yyval.interm.nodePair.node1 = yyvsp[-1].interm.intermTypedNode;
        yyval.interm.nodePair.node2 = 0;
    ;}
    break;

  case 203:
#line 1835 "glslang.y"
    {
        yyval.interm.nodePair.node1 = yyvsp[-2].interm.intermTypedNode;
        yyval.interm.nodePair.node2 = yyvsp[0].interm.intermTypedNode;
    ;}
    break;

  case 204:
#line 1842 "glslang.y"
    {
        if (parseContext.loopNestingLevel <= 0) {
            parseContext.error(yyvsp[-1].lex.line, "continue statement only allowed in loops", "", "");
            parseContext.recover();
        }        
        yyval.interm.intermNode = parseContext.intermediate.addBranch(EOpContinue, yyvsp[-1].lex.line);
    ;}
    break;

  case 205:
#line 1849 "glslang.y"
    {
        if (parseContext.loopNestingLevel <= 0) {
            parseContext.error(yyvsp[-1].lex.line, "break statement only allowed in loops", "", "");
            parseContext.recover();
        }        
        yyval.interm.intermNode = parseContext.intermediate.addBranch(EOpBreak, yyvsp[-1].lex.line);
    ;}
    break;

  case 206:
#line 1856 "glslang.y"
    {
        yyval.interm.intermNode = parseContext.intermediate.addBranch(EOpReturn, yyvsp[-1].lex.line);
        if (parseContext.currentFunctionType->getBasicType() != EbtVoid) {
            parseContext.error(yyvsp[-1].lex.line, "non-void function must return a value", "return", "");
            parseContext.recover();
        }
    ;}
    break;

  case 207:
#line 1863 "glslang.y"
    {        
        yyval.interm.intermNode = parseContext.intermediate.addBranch(EOpReturn, yyvsp[-1].interm.intermTypedNode, yyvsp[-2].lex.line);
        parseContext.functionReturnsValue = true;
        if (parseContext.currentFunctionType->getBasicType() == EbtVoid) {
            parseContext.error(yyvsp[-2].lex.line, "void function cannot return a value", "return", "");
            parseContext.recover();
        } else if (*(parseContext.currentFunctionType) != yyvsp[-1].interm.intermTypedNode->getType()) {
            parseContext.error(yyvsp[-2].lex.line, "function return is not matching type:", "return", "");
            parseContext.recover();
        }
    ;}
    break;

  case 208:
#line 1874 "glslang.y"
    {
        FRAG_ONLY("discard", yyvsp[-1].lex.line);
        yyval.interm.intermNode = parseContext.intermediate.addBranch(EOpKill, yyvsp[-1].lex.line);
    ;}
    break;

  case 209:
#line 1883 "glslang.y"
    { 
        yyval.interm.intermNode = yyvsp[0].interm.intermNode; 
        parseContext.treeRoot = yyval.interm.intermNode; 
    ;}
    break;

  case 210:
#line 1887 "glslang.y"
    {
        yyval.interm.intermNode = parseContext.intermediate.growAggregate(yyvsp[-1].interm.intermNode, yyvsp[0].interm.intermNode, 0);
        parseContext.treeRoot = yyval.interm.intermNode;
    ;}
    break;

  case 211:
#line 1894 "glslang.y"
    { 
        yyval.interm.intermNode = yyvsp[0].interm.intermNode; 
    ;}
    break;

  case 212:
#line 1897 "glslang.y"
    { 
        yyval.interm.intermNode = yyvsp[0].interm.intermNode; 
    ;}
    break;

  case 213:
#line 1903 "glslang.y"
    {
        TFunction& function = *(yyvsp[0].interm.function);
        TFunction* prevDec = static_cast<TFunction*>(parseContext.symbolTable.find(function.getMangledName()));
        //
        // Note:  'prevDec' could be 'function' if this is the first time we've seen function
        // as it would have just been put in the symbol table.  Otherwise, we're looking up
        // an earlier occurance.
        //
        if (prevDec->isDefined()) {
            //
            // Then this function already has a body.
            //
            parseContext.error(yyvsp[0].interm.line, "function already has a body", function.getName().c_str(), "");
            parseContext.recover();
        }
        prevDec->setDefined();
        
        //
        // Raise error message if main function takes any parameters or return anything other than void
        //
        if (function.getName() == "main") {
            if (function.getParamCount() > 0) {
                parseContext.error(yyvsp[0].interm.line, "function cannot take any parameter(s)", function.getName().c_str(), "");
                parseContext.recover();
            }
            if (function.getReturnType().getBasicType() != EbtVoid) {
                parseContext.error(yyvsp[0].interm.line, "", function.getReturnType().getBasicString(), "main function cannot return a value" );
                parseContext.recover();
            }            
        }
   
        //
        // New symbol table scope for body of function plus its arguments
        //
        parseContext.symbolTable.push();
        
        //
        // Remember the return type for later checking for RETURN statements.
        //
        parseContext.currentFunctionType = &(prevDec->getReturnType());
        parseContext.functionReturnsValue = false;
        
        // 
        // Insert parameters into the symbol table.
        // If the parameter has no name, it's not an error, just don't insert it 
        // (could be used for unused args).
        //
        // Also, accumulate the list of parameters into the HIL, so lower level code
        // knows where to find parameters.
        //
        TIntermAggregate* paramNodes = new TIntermAggregate;
        for (int i = 0; i < function.getParamCount(); i++) {
            TParameter& param = function[i];
            if (param.name != 0) {
                TVariable *variable = new TVariable(param.name, *param.type);
                // 
                // Insert the parameters with name in the symbol table.
                //
                if (! parseContext.symbolTable.insert(*variable)) {
                    parseContext.error(yyvsp[0].interm.line, "redefinition", variable->getName().c_str(), "");
                    parseContext.recover();
                    delete variable;
                }
                //
                // Transfer ownership of name pointer to symbol table.
                //
                param.name = 0;
                
                //
                // Add the parameter to the HIL
                //                
                paramNodes = parseContext.intermediate.growAggregate(
                                               paramNodes, 
                                               parseContext.intermediate.addSymbol(variable->getUniqueId(),
                                                                       variable->getName(),
                                                                       variable->getType(), yyvsp[0].interm.line), 
                                               yyvsp[0].interm.line);
            } else {
                paramNodes = parseContext.intermediate.growAggregate(paramNodes, parseContext.intermediate.addSymbol(0, "", *param.type, yyvsp[0].interm.line), yyvsp[0].interm.line);
            }
        }
        parseContext.intermediate.setAggregateOperator(paramNodes, EOpParameters, yyvsp[0].interm.line);
        yyvsp[0].interm.intermAggregate = paramNodes;
        parseContext.loopNestingLevel = 0;
    ;}
    break;

  case 214:
#line 1988 "glslang.y"
    {
        //?? Check that all paths return a value if return type != void ?
        //   May be best done as post process phase on intermediate code
        if (parseContext.currentFunctionType->getBasicType() != EbtVoid && ! parseContext.functionReturnsValue) {
            parseContext.error(yyvsp[-2].interm.line, "function does not return a value:", "", yyvsp[-2].interm.function->getName().c_str());
            parseContext.recover();
        }
        parseContext.symbolTable.pop();
        yyval.interm.intermNode = parseContext.intermediate.growAggregate(yyvsp[-2].interm.intermAggregate, yyvsp[0].interm.intermNode, 0);
        parseContext.intermediate.setAggregateOperator(yyval.interm.intermNode, EOpFunction, yyvsp[-2].interm.line);
        yyval.interm.intermNode->getAsAggregate()->setName(yyvsp[-2].interm.function->getMangledName().c_str());
        yyval.interm.intermNode->getAsAggregate()->setType(yyvsp[-2].interm.function->getReturnType());
        
        // store the pragma information for debug and optimize and other vendor specific 
        // information. This information can be queried from the parse tree
        yyval.interm.intermNode->getAsAggregate()->setOptimize(parseContext.contextPragma.optimize);
        yyval.interm.intermNode->getAsAggregate()->setDebug(parseContext.contextPragma.debug);
        yyval.interm.intermNode->getAsAggregate()->addToPragmaTable(parseContext.contextPragma.pragmaTable);
    ;}
    break;


    }

/* Line 999 of yacc.c.  */
#line 4158 "glslang.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 209 "glslang.y"


