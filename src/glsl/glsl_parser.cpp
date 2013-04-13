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
     SAMPLERCUBEARRAY = 316,
     SAMPLERCUBEARRAYSHADOW = 317,
     ISAMPLER1D = 318,
     ISAMPLER2D = 319,
     ISAMPLER3D = 320,
     ISAMPLERCUBE = 321,
     ISAMPLER1DARRAY = 322,
     ISAMPLER2DARRAY = 323,
     ISAMPLERCUBEARRAY = 324,
     USAMPLER1D = 325,
     USAMPLER2D = 326,
     USAMPLER3D = 327,
     USAMPLERCUBE = 328,
     USAMPLER1DARRAY = 329,
     USAMPLER2DARRAY = 330,
     USAMPLERCUBEARRAY = 331,
     SAMPLER2DRECT = 332,
     ISAMPLER2DRECT = 333,
     USAMPLER2DRECT = 334,
     SAMPLER2DRECTSHADOW = 335,
     SAMPLERBUFFER = 336,
     ISAMPLERBUFFER = 337,
     USAMPLERBUFFER = 338,
     SAMPLER2DMS = 339,
     ISAMPLER2DMS = 340,
     USAMPLER2DMS = 341,
     SAMPLER2DMSARRAY = 342,
     ISAMPLER2DMSARRAY = 343,
     USAMPLER2DMSARRAY = 344,
     SAMPLEREXTERNALOES = 345,
     STRUCT = 346,
     VOID_TOK = 347,
     WHILE = 348,
     IDENTIFIER = 349,
     TYPE_IDENTIFIER = 350,
     NEW_IDENTIFIER = 351,
     FLOATCONSTANT = 352,
     INTCONSTANT = 353,
     UINTCONSTANT = 354,
     BOOLCONSTANT = 355,
     FIELD_SELECTION = 356,
     LEFT_OP = 357,
     RIGHT_OP = 358,
     INC_OP = 359,
     DEC_OP = 360,
     LE_OP = 361,
     GE_OP = 362,
     EQ_OP = 363,
     NE_OP = 364,
     AND_OP = 365,
     OR_OP = 366,
     XOR_OP = 367,
     MUL_ASSIGN = 368,
     DIV_ASSIGN = 369,
     ADD_ASSIGN = 370,
     MOD_ASSIGN = 371,
     LEFT_ASSIGN = 372,
     RIGHT_ASSIGN = 373,
     AND_ASSIGN = 374,
     XOR_ASSIGN = 375,
     OR_ASSIGN = 376,
     SUB_ASSIGN = 377,
     INVARIANT = 378,
     LOWP = 379,
     MEDIUMP = 380,
     HIGHP = 381,
     SUPERP = 382,
     PRECISION = 383,
     VERSION_TOK = 384,
     EXTENSION = 385,
     LINE = 386,
     COLON = 387,
     EOL = 388,
     INTERFACE = 389,
     OUTPUT = 390,
     PRAGMA_DEBUG_ON = 391,
     PRAGMA_DEBUG_OFF = 392,
     PRAGMA_OPTIMIZE_ON = 393,
     PRAGMA_OPTIMIZE_OFF = 394,
     PRAGMA_INVARIANT_ALL = 395,
     LAYOUT_TOK = 396,
     ASM = 397,
     CLASS = 398,
     UNION = 399,
     ENUM = 400,
     TYPEDEF = 401,
     TEMPLATE = 402,
     THIS = 403,
     PACKED_TOK = 404,
     GOTO = 405,
     INLINE_TOK = 406,
     NOINLINE = 407,
     VOLATILE = 408,
     PUBLIC_TOK = 409,
     STATIC = 410,
     EXTERN = 411,
     EXTERNAL = 412,
     LONG_TOK = 413,
     SHORT_TOK = 414,
     DOUBLE_TOK = 415,
     HALF = 416,
     FIXED_TOK = 417,
     UNSIGNED = 418,
     INPUT_TOK = 419,
     OUPTUT = 420,
     HVEC2 = 421,
     HVEC3 = 422,
     HVEC4 = 423,
     DVEC2 = 424,
     DVEC3 = 425,
     DVEC4 = 426,
     FVEC2 = 427,
     FVEC3 = 428,
     FVEC4 = 429,
     SAMPLER3DRECT = 430,
     SIZEOF = 431,
     CAST = 432,
     NAMESPACE = 433,
     USING = 434,
     COHERENT = 435,
     RESTRICT = 436,
     READONLY = 437,
     WRITEONLY = 438,
     RESOURCE = 439,
     ATOMIC_UINT = 440,
     PATCH = 441,
     SAMPLE = 442,
     SUBROUTINE = 443,
     ERROR_TOK = 444,
     COMMON = 445,
     PARTITION = 446,
     ACTIVE = 447,
     FILTER = 448,
     IMAGE1D = 449,
     IMAGE2D = 450,
     IMAGE3D = 451,
     IMAGECUBE = 452,
     IMAGE1DARRAY = 453,
     IMAGE2DARRAY = 454,
     IIMAGE1D = 455,
     IIMAGE2D = 456,
     IIMAGE3D = 457,
     IIMAGECUBE = 458,
     IIMAGE1DARRAY = 459,
     IIMAGE2DARRAY = 460,
     UIMAGE1D = 461,
     UIMAGE2D = 462,
     UIMAGE3D = 463,
     UIMAGECUBE = 464,
     UIMAGE1DARRAY = 465,
     UIMAGE2DARRAY = 466,
     IMAGE1DSHADOW = 467,
     IMAGE2DSHADOW = 468,
     IMAGEBUFFER = 469,
     IIMAGEBUFFER = 470,
     UIMAGEBUFFER = 471,
     IMAGE1DARRAYSHADOW = 472,
     IMAGE2DARRAYSHADOW = 473,
     ROW_MAJOR = 474
   };
#endif
/* Tokens.  */
#define ATTRIBUTE 258
#define CONST_TOK 259
#define BOOL_TOK 260
#define FLOAT_TOK 261
#define INT_TOK 262
#define UINT_TOK 263
#define BREAK 264
#define CONTINUE 265
#define DO 266
#define ELSE 267
#define FOR 268
#define IF 269
#define DISCARD 270
#define RETURN 271
#define SWITCH 272
#define CASE 273
#define DEFAULT 274
#define BVEC2 275
#define BVEC3 276
#define BVEC4 277
#define IVEC2 278
#define IVEC3 279
#define IVEC4 280
#define UVEC2 281
#define UVEC3 282
#define UVEC4 283
#define VEC2 284
#define VEC3 285
#define VEC4 286
#define CENTROID 287
#define IN_TOK 288
#define OUT_TOK 289
#define INOUT_TOK 290
#define UNIFORM 291
#define VARYING 292
#define NOPERSPECTIVE 293
#define FLAT 294
#define SMOOTH 295
#define MAT2X2 296
#define MAT2X3 297
#define MAT2X4 298
#define MAT3X2 299
#define MAT3X3 300
#define MAT3X4 301
#define MAT4X2 302
#define MAT4X3 303
#define MAT4X4 304
#define SAMPLER1D 305
#define SAMPLER2D 306
#define SAMPLER3D 307
#define SAMPLERCUBE 308
#define SAMPLER1DSHADOW 309
#define SAMPLER2DSHADOW 310
#define SAMPLERCUBESHADOW 311
#define SAMPLER1DARRAY 312
#define SAMPLER2DARRAY 313
#define SAMPLER1DARRAYSHADOW 314
#define SAMPLER2DARRAYSHADOW 315
#define SAMPLERCUBEARRAY 316
#define SAMPLERCUBEARRAYSHADOW 317
#define ISAMPLER1D 318
#define ISAMPLER2D 319
#define ISAMPLER3D 320
#define ISAMPLERCUBE 321
#define ISAMPLER1DARRAY 322
#define ISAMPLER2DARRAY 323
#define ISAMPLERCUBEARRAY 324
#define USAMPLER1D 325
#define USAMPLER2D 326
#define USAMPLER3D 327
#define USAMPLERCUBE 328
#define USAMPLER1DARRAY 329
#define USAMPLER2DARRAY 330
#define USAMPLERCUBEARRAY 331
#define SAMPLER2DRECT 332
#define ISAMPLER2DRECT 333
#define USAMPLER2DRECT 334
#define SAMPLER2DRECTSHADOW 335
#define SAMPLERBUFFER 336
#define ISAMPLERBUFFER 337
#define USAMPLERBUFFER 338
#define SAMPLER2DMS 339
#define ISAMPLER2DMS 340
#define USAMPLER2DMS 341
#define SAMPLER2DMSARRAY 342
#define ISAMPLER2DMSARRAY 343
#define USAMPLER2DMSARRAY 344
#define SAMPLEREXTERNALOES 345
#define STRUCT 346
#define VOID_TOK 347
#define WHILE 348
#define IDENTIFIER 349
#define TYPE_IDENTIFIER 350
#define NEW_IDENTIFIER 351
#define FLOATCONSTANT 352
#define INTCONSTANT 353
#define UINTCONSTANT 354
#define BOOLCONSTANT 355
#define FIELD_SELECTION 356
#define LEFT_OP 357
#define RIGHT_OP 358
#define INC_OP 359
#define DEC_OP 360
#define LE_OP 361
#define GE_OP 362
#define EQ_OP 363
#define NE_OP 364
#define AND_OP 365
#define OR_OP 366
#define XOR_OP 367
#define MUL_ASSIGN 368
#define DIV_ASSIGN 369
#define ADD_ASSIGN 370
#define MOD_ASSIGN 371
#define LEFT_ASSIGN 372
#define RIGHT_ASSIGN 373
#define AND_ASSIGN 374
#define XOR_ASSIGN 375
#define OR_ASSIGN 376
#define SUB_ASSIGN 377
#define INVARIANT 378
#define LOWP 379
#define MEDIUMP 380
#define HIGHP 381
#define SUPERP 382
#define PRECISION 383
#define VERSION_TOK 384
#define EXTENSION 385
#define LINE 386
#define COLON 387
#define EOL 388
#define INTERFACE 389
#define OUTPUT 390
#define PRAGMA_DEBUG_ON 391
#define PRAGMA_DEBUG_OFF 392
#define PRAGMA_OPTIMIZE_ON 393
#define PRAGMA_OPTIMIZE_OFF 394
#define PRAGMA_INVARIANT_ALL 395
#define LAYOUT_TOK 396
#define ASM 397
#define CLASS 398
#define UNION 399
#define ENUM 400
#define TYPEDEF 401
#define TEMPLATE 402
#define THIS 403
#define PACKED_TOK 404
#define GOTO 405
#define INLINE_TOK 406
#define NOINLINE 407
#define VOLATILE 408
#define PUBLIC_TOK 409
#define STATIC 410
#define EXTERN 411
#define EXTERNAL 412
#define LONG_TOK 413
#define SHORT_TOK 414
#define DOUBLE_TOK 415
#define HALF 416
#define FIXED_TOK 417
#define UNSIGNED 418
#define INPUT_TOK 419
#define OUPTUT 420
#define HVEC2 421
#define HVEC3 422
#define HVEC4 423
#define DVEC2 424
#define DVEC3 425
#define DVEC4 426
#define FVEC2 427
#define FVEC3 428
#define FVEC4 429
#define SAMPLER3DRECT 430
#define SIZEOF 431
#define CAST 432
#define NAMESPACE 433
#define USING 434
#define COHERENT 435
#define RESTRICT 436
#define READONLY 437
#define WRITEONLY 438
#define RESOURCE 439
#define ATOMIC_UINT 440
#define PATCH 441
#define SAMPLE 442
#define SUBROUTINE 443
#define ERROR_TOK 444
#define COMMON 445
#define PARTITION 446
#define ACTIVE 447
#define FILTER 448
#define IMAGE1D 449
#define IMAGE2D 450
#define IMAGE3D 451
#define IMAGECUBE 452
#define IMAGE1DARRAY 453
#define IMAGE2DARRAY 454
#define IIMAGE1D 455
#define IIMAGE2D 456
#define IIMAGE3D 457
#define IIMAGECUBE 458
#define IIMAGE1DARRAY 459
#define IIMAGE2DARRAY 460
#define UIMAGE1D 461
#define UIMAGE2D 462
#define UIMAGE3D 463
#define UIMAGECUBE 464
#define UIMAGE1DARRAY 465
#define UIMAGE2DARRAY 466
#define IMAGE1DSHADOW 467
#define IMAGE2DSHADOW 468
#define IMAGEBUFFER 469
#define IIMAGEBUFFER 470
#define UIMAGEBUFFER 471
#define IMAGE1DARRAYSHADOW 472
#define IMAGE2DARRAYSHADOW 473
#define ROW_MAJOR 474




/* Copy the first part of user declarations.  */
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
#line 64 "src/glsl/glsl_parser.yy"
{
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
   ast_uniform_block *uniform_block;

   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;
}
/* Line 193 of yacc.c.  */
#line 626 "src/glsl/glsl_parser.cpp"
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
#line 651 "src/glsl/glsl_parser.cpp"

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
#define YYLAST   3462

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  244
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  106
/* YYNRULES -- Number of rules.  */
#define YYNRULES  336
/* YYNRULES -- Number of states.  */
#define YYNSTATES  499

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   474

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   228,     2,     2,     2,   232,   235,     2,
     220,   221,   230,   226,   225,   227,   224,   231,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   239,   241,
     233,   240,   234,   238,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   222,     2,   223,   236,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   242,   237,   243,   229,     2,     2,     2,
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
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    19,    22,    25,
      28,    31,    34,    35,    38,    40,    42,    44,    50,    52,
      55,    57,    59,    61,    63,    65,    67,    69,    73,    75,
      80,    82,    86,    89,    92,    94,    96,    98,   102,   105,
     108,   111,   113,   116,   120,   123,   125,   127,   129,   132,
     135,   138,   140,   143,   147,   150,   152,   155,   158,   161,
     163,   165,   167,   169,   171,   175,   179,   183,   185,   189,
     193,   195,   199,   203,   205,   209,   213,   217,   221,   223,
     227,   231,   233,   237,   239,   243,   245,   249,   251,   255,
     257,   261,   263,   267,   269,   275,   277,   281,   283,   285,
     287,   289,   291,   293,   295,   297,   299,   301,   303,   305,
     309,   311,   314,   317,   322,   324,   327,   329,   331,   334,
     338,   342,   345,   351,   355,   358,   362,   365,   366,   368,
     370,   372,   374,   376,   380,   386,   393,   401,   410,   416,
     418,   421,   426,   432,   439,   447,   452,   455,   457,   460,
     465,   467,   471,   473,   475,   477,   481,   483,   485,   487,
     489,   491,   493,   495,   497,   499,   502,   504,   507,   510,
     514,   516,   518,   520,   522,   525,   527,   529,   532,   535,
     537,   539,   542,   544,   548,   553,   555,   557,   559,   561,
     563,   565,   567,   569,   571,   573,   575,   577,   579,   581,
     583,   585,   587,   589,   591,   593,   595,   597,   599,   601,
     603,   605,   607,   609,   611,   613,   615,   617,   619,   621,
     623,   625,   627,   629,   631,   633,   635,   637,   639,   641,
     643,   645,   647,   649,   651,   653,   655,   657,   659,   661,
     663,   665,   667,   669,   671,   673,   675,   677,   679,   681,
     683,   685,   687,   689,   691,   693,   695,   697,   699,   705,
     710,   712,   715,   719,   721,   725,   727,   732,   734,   736,
     738,   740,   742,   744,   746,   748,   750,   752,   755,   756,
     761,   763,   765,   768,   772,   774,   777,   779,   782,   788,
     792,   794,   796,   801,   807,   810,   814,   818,   821,   823,
     826,   829,   832,   834,   837,   843,   851,   858,   860,   862,
     864,   865,   868,   872,   875,   878,   881,   885,   888,   890,
     892,   894,   896,   899,   901,   904,   912,   913,   915,   920,
     924,   926,   929,   930,   932,   938,   943
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     245,     0,    -1,    -1,   247,   249,   246,   252,    -1,    -1,
     129,    98,   133,    -1,   129,    98,   250,   133,    -1,   136,
     133,    -1,   137,   133,    -1,   138,   133,    -1,   139,   133,
      -1,   140,   133,    -1,    -1,   249,   251,    -1,    94,    -1,
      95,    -1,    96,    -1,   130,   250,   132,   250,   133,    -1,
     341,    -1,   252,   341,    -1,    94,    -1,    96,    -1,   253,
      -1,    98,    -1,    99,    -1,    97,    -1,   100,    -1,   220,
     284,   221,    -1,   254,    -1,   255,   222,   256,   223,    -1,
     257,    -1,   255,   224,   250,    -1,   255,   104,    -1,   255,
     105,    -1,   284,    -1,   258,    -1,   259,    -1,   255,   224,
     264,    -1,   261,   221,    -1,   260,   221,    -1,   262,    92,
      -1,   262,    -1,   262,   282,    -1,   261,   225,   282,    -1,
     263,   220,    -1,   307,    -1,   253,    -1,   101,    -1,   266,
     221,    -1,   265,   221,    -1,   267,    92,    -1,   267,    -1,
     267,   282,    -1,   266,   225,   282,    -1,   253,   220,    -1,
     255,    -1,   104,   268,    -1,   105,   268,    -1,   269,   268,
      -1,   226,    -1,   227,    -1,   228,    -1,   229,    -1,   268,
      -1,   270,   230,   268,    -1,   270,   231,   268,    -1,   270,
     232,   268,    -1,   270,    -1,   271,   226,   270,    -1,   271,
     227,   270,    -1,   271,    -1,   272,   102,   271,    -1,   272,
     103,   271,    -1,   272,    -1,   273,   233,   272,    -1,   273,
     234,   272,    -1,   273,   106,   272,    -1,   273,   107,   272,
      -1,   273,    -1,   274,   108,   273,    -1,   274,   109,   273,
      -1,   274,    -1,   275,   235,   274,    -1,   275,    -1,   276,
     236,   275,    -1,   276,    -1,   277,   237,   276,    -1,   277,
      -1,   278,   110,   277,    -1,   278,    -1,   279,   112,   278,
      -1,   279,    -1,   280,   111,   279,    -1,   280,    -1,   280,
     238,   284,   239,   282,    -1,   281,    -1,   268,   283,   282,
      -1,   240,    -1,   113,    -1,   114,    -1,   116,    -1,   115,
      -1,   122,    -1,   117,    -1,   118,    -1,   119,    -1,   120,
      -1,   121,    -1,   282,    -1,   284,   225,   282,    -1,   281,
      -1,   287,   241,    -1,   295,   241,    -1,   128,   311,   308,
     241,    -1,   343,    -1,   288,   221,    -1,   290,    -1,   289,
      -1,   290,   292,    -1,   289,   225,   292,    -1,   297,   253,
     220,    -1,   307,   250,    -1,   307,   250,   222,   285,   223,
      -1,   304,   293,   291,    -1,   293,   291,    -1,   304,   293,
     294,    -1,   293,   294,    -1,    -1,    33,    -1,    34,    -1,
      35,    -1,   307,    -1,   296,    -1,   295,   225,   250,    -1,
     295,   225,   250,   222,   223,    -1,   295,   225,   250,   222,
     285,   223,    -1,   295,   225,   250,   222,   223,   240,   317,
      -1,   295,   225,   250,   222,   285,   223,   240,   317,    -1,
     295,   225,   250,   240,   317,    -1,   297,    -1,   297,   250,
      -1,   297,   250,   222,   223,    -1,   297,   250,   222,   285,
     223,    -1,   297,   250,   222,   223,   240,   317,    -1,   297,
     250,   222,   285,   223,   240,   317,    -1,   297,   250,   240,
     317,    -1,   123,   253,    -1,   307,    -1,   305,   307,    -1,
     141,   220,   299,   221,    -1,   301,    -1,   299,   225,   301,
      -1,    98,    -1,    99,    -1,   250,    -1,   250,   240,   300,
      -1,   302,    -1,   219,    -1,   149,    -1,    40,    -1,    39,
      -1,    38,    -1,     4,    -1,   306,    -1,   298,    -1,   298,
     306,    -1,   303,    -1,   303,   306,    -1,   123,   306,    -1,
     123,   303,   306,    -1,   123,    -1,     4,    -1,     3,    -1,
      37,    -1,    32,    37,    -1,    33,    -1,    34,    -1,    32,
      33,    -1,    32,    34,    -1,    36,    -1,   308,    -1,   311,
     308,    -1,   309,    -1,   309,   222,   223,    -1,   309,   222,
     285,   223,    -1,   310,    -1,   312,    -1,    95,    -1,    92,
      -1,     6,    -1,     7,    -1,     8,    -1,     5,    -1,    29,
      -1,    30,    -1,    31,    -1,    20,    -1,    21,    -1,    22,
      -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,    27,
      -1,    28,    -1,    41,    -1,    42,    -1,    43,    -1,    44,
      -1,    45,    -1,    46,    -1,    47,    -1,    48,    -1,    49,
      -1,    50,    -1,    51,    -1,    77,    -1,    52,    -1,    53,
      -1,    90,    -1,    54,    -1,    55,    -1,    80,    -1,    56,
      -1,    57,    -1,    58,    -1,    59,    -1,    60,    -1,    81,
      -1,    61,    -1,    62,    -1,    63,    -1,    64,    -1,    78,
      -1,    65,    -1,    66,    -1,    67,    -1,    68,    -1,    82,
      -1,    69,    -1,    70,    -1,    71,    -1,    79,    -1,    72,
      -1,    73,    -1,    74,    -1,    75,    -1,    83,    -1,    76,
      -1,    84,    -1,    85,    -1,    86,    -1,    87,    -1,    88,
      -1,    89,    -1,   126,    -1,   125,    -1,   124,    -1,    91,
     250,   242,   313,   243,    -1,    91,   242,   313,   243,    -1,
     314,    -1,   313,   314,    -1,   307,   315,   241,    -1,   316,
      -1,   315,   225,   316,    -1,   250,    -1,   250,   222,   285,
     223,    -1,   282,    -1,   286,    -1,   321,    -1,   320,    -1,
     318,    -1,   326,    -1,   327,    -1,   330,    -1,   336,    -1,
     340,    -1,   242,   243,    -1,    -1,   242,   322,   325,   243,
      -1,   324,    -1,   320,    -1,   242,   243,    -1,   242,   325,
     243,    -1,   319,    -1,   325,   319,    -1,   241,    -1,   284,
     241,    -1,    14,   220,   284,   221,   328,    -1,   319,    12,
     319,    -1,   319,    -1,   284,    -1,   297,   250,   240,   317,
      -1,    17,   220,   284,   221,   331,    -1,   242,   243,    -1,
     242,   335,   243,    -1,    18,   284,   239,    -1,    19,   239,
      -1,   332,    -1,   333,   332,    -1,   333,   319,    -1,   334,
     319,    -1,   334,    -1,   335,   334,    -1,    93,   220,   329,
     221,   323,    -1,    11,   319,    93,   220,   284,   221,   241,
      -1,    13,   220,   337,   339,   221,   323,    -1,   326,    -1,
     318,    -1,   329,    -1,    -1,   338,   241,    -1,   338,   241,
     284,    -1,    10,   241,    -1,     9,   241,    -1,    16,   241,
      -1,    16,   284,   241,    -1,    15,   241,    -1,   342,    -1,
     286,    -1,   248,    -1,   349,    -1,   287,   324,    -1,   344,
      -1,   298,   344,    -1,    36,    96,   242,   346,   243,   345,
     241,    -1,    -1,    96,    -1,    96,   222,   285,   223,    -1,
      96,   222,   223,    -1,   348,    -1,   348,   346,    -1,    -1,
      36,    -1,   298,   347,   307,   315,   241,    -1,   347,   307,
     315,   241,    -1,   298,    36,   241,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   258,   258,   257,   269,   271,   275,   282,   283,   284,
     285,   286,   299,   301,   305,   306,   307,   311,   320,   328,
     339,   340,   344,   351,   358,   365,   372,   379,   386,   387,
     393,   397,   404,   410,   419,   423,   427,   428,   437,   438,
     442,   443,   447,   453,   465,   469,   475,   482,   492,   493,
     497,   498,   502,   508,   520,   531,   532,   538,   544,   554,
     555,   556,   557,   561,   562,   568,   574,   583,   584,   590,
     599,   600,   606,   615,   616,   622,   628,   634,   643,   644,
     650,   659,   660,   669,   670,   679,   680,   689,   690,   699,
     700,   709,   710,   719,   720,   729,   730,   739,   740,   741,
     742,   743,   744,   745,   746,   747,   748,   749,   753,   757,
     773,   777,   782,   786,   792,   799,   803,   804,   808,   813,
     821,   835,   845,   860,   867,   872,   883,   896,   899,   904,
     909,   918,   922,   923,   933,   943,   953,   963,   973,   987,
     994,  1003,  1012,  1021,  1030,  1039,  1048,  1062,  1069,  1080,
    1087,  1088,  1098,  1099,  1103,  1173,  1219,  1241,  1246,  1254,
    1259,  1264,  1272,  1280,  1281,  1282,  1287,  1288,  1293,  1298,
    1304,  1312,  1317,  1322,  1327,  1333,  1338,  1343,  1348,  1353,
    1361,  1365,  1373,  1374,  1380,  1389,  1395,  1401,  1410,  1411,
    1412,  1413,  1414,  1415,  1416,  1417,  1418,  1419,  1420,  1421,
    1422,  1423,  1424,  1425,  1426,  1427,  1428,  1429,  1430,  1431,
    1432,  1433,  1434,  1435,  1436,  1437,  1438,  1439,  1440,  1441,
    1442,  1443,  1444,  1445,  1446,  1447,  1448,  1449,  1450,  1451,
    1452,  1453,  1454,  1455,  1456,  1457,  1458,  1459,  1460,  1461,
    1462,  1463,  1464,  1465,  1466,  1467,  1468,  1469,  1470,  1471,
    1472,  1473,  1474,  1475,  1476,  1480,  1485,  1490,  1498,  1505,
    1514,  1519,  1527,  1542,  1547,  1555,  1561,  1570,  1574,  1580,
    1581,  1585,  1586,  1587,  1588,  1589,  1590,  1594,  1601,  1600,
    1614,  1615,  1619,  1625,  1634,  1644,  1656,  1662,  1671,  1680,
    1685,  1693,  1697,  1715,  1723,  1728,  1736,  1741,  1749,  1757,
    1765,  1773,  1781,  1789,  1797,  1804,  1811,  1821,  1822,  1826,
    1828,  1834,  1839,  1848,  1854,  1860,  1866,  1872,  1881,  1882,
    1883,  1884,  1888,  1902,  1906,  1917,  1951,  1956,  1962,  1968,
    1980,  1985,  1993,  1995,  1999,  2014,  2031
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
  "SAMPLER2DARRAYSHADOW", "SAMPLERCUBEARRAY", "SAMPLERCUBEARRAYSHADOW",
  "ISAMPLER1D", "ISAMPLER2D", "ISAMPLER3D", "ISAMPLERCUBE",
  "ISAMPLER1DARRAY", "ISAMPLER2DARRAY", "ISAMPLERCUBEARRAY", "USAMPLER1D",
  "USAMPLER2D", "USAMPLER3D", "USAMPLERCUBE", "USAMPLER1DARRAY",
  "USAMPLER2DARRAY", "USAMPLERCUBEARRAY", "SAMPLER2DRECT",
  "ISAMPLER2DRECT", "USAMPLER2DRECT", "SAMPLER2DRECTSHADOW",
  "SAMPLERBUFFER", "ISAMPLERBUFFER", "USAMPLERBUFFER", "SAMPLER2DMS",
  "ISAMPLER2DMS", "USAMPLER2DMS", "SAMPLER2DMSARRAY", "ISAMPLER2DMSARRAY",
  "USAMPLER2DMSARRAY", "SAMPLEREXTERNALOES", "STRUCT", "VOID_TOK", "WHILE",
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
  "NAMESPACE", "USING", "COHERENT", "RESTRICT", "READONLY", "WRITEONLY",
  "RESOURCE", "ATOMIC_UINT", "PATCH", "SAMPLE", "SUBROUTINE", "ERROR_TOK",
  "COMMON", "PARTITION", "ACTIVE", "FILTER", "IMAGE1D", "IMAGE2D",
  "IMAGE3D", "IMAGECUBE", "IMAGE1DARRAY", "IMAGE2DARRAY", "IIMAGE1D",
  "IIMAGE2D", "IIMAGE3D", "IIMAGECUBE", "IIMAGE1DARRAY", "IIMAGE2DARRAY",
  "UIMAGE1D", "UIMAGE2D", "UIMAGE3D", "UIMAGECUBE", "UIMAGE1DARRAY",
  "UIMAGE2DARRAY", "IMAGE1DSHADOW", "IMAGE2DSHADOW", "IMAGEBUFFER",
  "IIMAGEBUFFER", "UIMAGEBUFFER", "IMAGE1DARRAYSHADOW",
  "IMAGE2DARRAYSHADOW", "ROW_MAJOR", "'('", "')'", "'['", "']'", "'.'",
  "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'", "'%'", "'<'", "'>'",
  "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'", "'{'", "'}'", "$accept",
  "translation_unit", "@1", "version_statement", "pragma_statement",
  "extension_statement_list", "any_identifier", "extension_statement",
  "external_declaration_list", "variable_identifier", "primary_expression",
  "postfix_expression", "integer_expression", "function_call",
  "function_call_or_method", "function_call_generic",
  "function_call_header_no_parameters",
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
  "layout_qualifier", "layout_qualifier_id_list", "integer_constant",
  "layout_qualifier_id", "uniform_block_layout_qualifier",
  "interpolation_qualifier", "parameter_type_qualifier", "type_qualifier",
  "storage_qualifier", "type_specifier", "type_specifier_no_prec",
  "type_specifier_nonarray", "basic_type_specifier_nonarray",
  "precision_qualifier", "struct_specifier", "struct_declaration_list",
  "struct_declaration", "struct_declarator_list", "struct_declarator",
  "initializer", "declaration_statement", "statement", "simple_statement",
  "compound_statement", "@2", "statement_no_new_scope",
  "compound_statement_no_new_scope", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "switch_body", "case_label", "case_label_list", "case_statement",
  "case_statement_list", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "external_declaration", "function_definition", "uniform_block",
  "basic_uniform_block", "instance_name_opt", "member_list", "uniformopt",
  "member_declaration", "layout_defaults", 0
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
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
      40,    41,    91,    93,    46,    44,    43,    45,    33,   126,
      42,    47,    37,    60,    62,    38,    94,   124,    63,    58,
      61,    59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   244,   246,   245,   247,   247,   247,   248,   248,   248,
     248,   248,   249,   249,   250,   250,   250,   251,   252,   252,
     253,   253,   254,   254,   254,   254,   254,   254,   255,   255,
     255,   255,   255,   255,   256,   257,   258,   258,   259,   259,
     260,   260,   261,   261,   262,   263,   263,   263,   264,   264,
     265,   265,   266,   266,   267,   268,   268,   268,   268,   269,
     269,   269,   269,   270,   270,   270,   270,   271,   271,   271,
     272,   272,   272,   273,   273,   273,   273,   273,   274,   274,
     274,   275,   275,   276,   276,   277,   277,   278,   278,   279,
     279,   280,   280,   281,   281,   282,   282,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   284,   284,
     285,   286,   286,   286,   286,   287,   288,   288,   289,   289,
     290,   291,   291,   292,   292,   292,   292,   293,   293,   293,
     293,   294,   295,   295,   295,   295,   295,   295,   295,   296,
     296,   296,   296,   296,   296,   296,   296,   297,   297,   298,
     299,   299,   300,   300,   301,   301,   301,   302,   302,   303,
     303,   303,   304,   305,   305,   305,   305,   305,   305,   305,
     305,   306,   306,   306,   306,   306,   306,   306,   306,   306,
     307,   307,   308,   308,   308,   309,   309,   309,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   310,   310,   310,   310,   310,
     310,   310,   310,   310,   310,   311,   311,   311,   312,   312,
     313,   313,   314,   315,   315,   316,   316,   317,   318,   319,
     319,   320,   320,   320,   320,   320,   320,   321,   322,   321,
     323,   323,   324,   324,   325,   325,   326,   326,   327,   328,
     328,   329,   329,   330,   331,   331,   332,   332,   333,   333,
     334,   334,   335,   335,   336,   336,   336,   337,   337,   338,
     338,   339,   339,   340,   340,   340,   340,   340,   341,   341,
     341,   341,   342,   343,   343,   344,   345,   345,   345,   345,
     346,   346,   347,   347,   348,   348,   349
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     4,     2,     2,     2,
       2,     2,     0,     2,     1,     1,     1,     5,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     3,     1,     4,
       1,     3,     2,     2,     1,     1,     1,     3,     2,     2,
       2,     1,     2,     3,     2,     1,     1,     1,     2,     2,
       2,     1,     2,     3,     2,     1,     2,     2,     2,     1,
       1,     1,     1,     1,     3,     3,     3,     1,     3,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     5,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       1,     2,     2,     4,     1,     2,     1,     1,     2,     3,
       3,     2,     5,     3,     2,     3,     2,     0,     1,     1,
       1,     1,     1,     3,     5,     6,     7,     8,     5,     1,
       2,     4,     5,     6,     7,     4,     2,     1,     2,     4,
       1,     3,     1,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     2,     2,     3,
       1,     1,     1,     1,     2,     1,     1,     2,     2,     1,
       1,     2,     1,     3,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     5,     4,
       1,     2,     3,     1,     3,     1,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     4,
       1,     1,     2,     3,     1,     2,     1,     2,     5,     3,
       1,     1,     4,     5,     2,     3,     3,     2,     1,     2,
       2,     2,     1,     2,     5,     7,     6,     1,     1,     1,
       0,     2,     3,     2,     2,     2,     3,     2,     1,     1,
       1,     1,     2,     1,     2,     7,     0,     1,     4,     3,
       1,     2,     0,     1,     5,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    12,     0,     1,     2,    14,    15,    16,
       5,     0,     0,     0,    13,     6,     0,   172,   171,   192,
     189,   190,   191,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   193,   194,   195,     0,   175,   176,   179,   173,
     161,   160,   159,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   217,   218,   220,   221,   223,   224,
     225,   226,   227,   229,   230,   231,   232,   234,   235,   236,
     237,   239,   240,   241,   243,   244,   245,   246,   248,   216,
     233,   242,   222,   228,   238,   247,   249,   250,   251,   252,
     253,   254,   219,     0,   188,   187,   170,   257,   256,   255,
       0,     0,     0,     0,     0,     0,     0,   320,     3,   319,
       0,     0,   117,   127,     0,   132,   139,   164,   166,     0,
     163,   147,   180,   182,   185,     0,   186,    18,   318,   114,
     323,   321,     0,   177,   178,   174,     0,     0,     0,   179,
      20,    21,   146,     0,   168,     0,     7,     8,     9,    10,
      11,     0,    19,   111,     0,   322,   115,   127,   162,   128,
     129,   130,   118,     0,   127,     0,   112,    14,    16,   140,
       0,   179,   165,   324,   167,   148,     0,   181,     0,   332,
       0,     0,   260,     0,   169,     0,   158,   157,   154,     0,
     150,   156,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,    23,    24,    26,    47,     0,     0,     0,    59,
      60,    61,    62,   286,   278,   282,    22,    28,    55,    30,
      35,    36,     0,     0,    41,     0,    63,     0,    67,    70,
      73,    78,    81,    83,    85,    87,    89,    91,    93,    95,
     108,     0,   268,     0,   164,   147,   271,   284,   270,   269,
       0,   272,   273,   274,   275,   276,   119,   124,   126,   131,
       0,   133,     0,     0,   120,   336,   183,    63,   110,     0,
      45,    17,   333,   332,     0,     0,   332,   265,     0,   263,
     259,   261,     0,   113,     0,   149,     0,   314,   313,     0,
       0,     0,   317,   315,     0,     0,     0,    56,    57,     0,
     277,     0,    32,    33,     0,     0,    39,    38,     0,   188,
      42,    44,    98,    99,   101,   100,   103,   104,   105,   106,
     107,   102,    97,     0,    58,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   287,   283,   285,   121,
     123,   125,     0,     0,   141,     0,   267,   145,   184,     0,
     326,     0,   331,     0,     0,   262,   258,   152,   153,   155,
     151,     0,   308,   307,   310,     0,   316,     0,   170,   291,
       0,   164,     0,    27,     0,     0,    34,    31,     0,    37,
       0,     0,    51,    43,    96,    64,    65,    66,    68,    69,
      71,    72,    76,    77,    74,    75,    79,    80,    82,    84,
      86,    88,    90,    92,     0,   109,     0,   134,     0,   138,
       0,   142,     0,   327,     0,     0,     0,   264,     0,   309,
       0,     0,     0,     0,     0,     0,   279,    29,    54,    49,
      48,     0,   188,    52,     0,     0,     0,   135,   143,     0,
       0,     0,   325,   335,   266,     0,   311,     0,   290,   288,
       0,   293,     0,   281,   304,   280,    53,    94,   122,   136,
       0,   144,   334,   329,     0,     0,   312,   306,     0,     0,
       0,   294,   298,     0,   302,     0,   292,   137,   328,   305,
     289,     0,   297,   300,   299,   301,   295,   303,   296
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    13,     3,   107,     6,   277,    14,   108,   216,
     217,   218,   385,   219,   220,   221,   222,   223,   224,   225,
     389,   390,   391,   392,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   323,
     241,   269,   242,   243,   111,   112,   113,   257,   162,   163,
     258,   114,   115,   116,   244,   189,   369,   190,   191,   118,
     164,   119,   120,   270,   122,   123,   124,   125,   126,   181,
     182,   278,   279,   357,   246,   247,   248,   249,   301,   464,
     465,   250,   251,   252,   459,   382,   253,   461,   482,   483,
     484,   485,   254,   374,   430,   431,   255,   127,   128,   129,
     130,   424,   274,   275,   276,   131
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -405
static const yytype_int16 yypact[] =
{
     -77,   -49,    55,  -405,   -50,  -405,   -51,  -405,  -405,  -405,
    -405,   -48,   165,  3321,  -405,  -405,   -44,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,   127,  -405,  -405,    23,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,   -76,  -405,  -405,   140,  -405,  -405,  -405,
     139,    15,    34,    42,    53,    64,     8,  -405,  3321,  -405,
     -96,     5,    10,     4,  -151,  -405,   211,    44,   305,  1063,
    -405,  -405,  -405,    11,  -405,  1304,  -405,  -405,  -405,  -405,
    -405,  -405,   165,  -405,  -405,  -405,    -1,  1063,    12,  -405,
    -405,  -405,  -405,   305,  -405,  1304,  -405,  -405,  -405,  -405,
    -405,   -60,  -405,  -405,   472,  -405,  -405,    69,  -405,  -405,
    -405,  -405,  -405,  1063,   277,   165,  -405,    36,    52,  -172,
      58,   -70,  -405,  -405,  -405,  -405,  2345,  -405,   147,    -4,
     165,   581,  -405,  1063,  -405,    41,  -405,  -405,    54,  -100,
    -405,  -405,    56,    57,  1435,    75,    82,    60,  2011,    93,
      99,  -405,  -405,  -405,  -405,  -405,  2981,  2981,  2981,  -405,
    -405,  -405,  -405,  -405,    77,  -405,   101,  -405,   -35,  -405,
    -405,  -405,   102,   -68,  3088,   104,   -55,  2981,    86,   -93,
      81,   -82,    95,    87,    89,    90,   216,   228,  -107,  -405,
    -405,  -126,  -405,   108,   311,   130,  -405,  -405,  -405,  -405,
     713,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,   165,
    1063,  -147,  2454,  2981,  -405,  -405,  -405,  -405,  -405,   128,
    -405,  -405,  -405,   316,   110,  1063,   -19,   132,  -125,  -405,
    -405,  -405,   822,  -405,   146,  -405,   -60,  -405,  -405,   262,
    1902,  2981,  -405,  -405,  -114,  2981,  2238,  -405,  -405,   -31,
    -405,  1435,  -405,  -405,  2981,   211,  -405,  -405,  2981,   135,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  2981,  -405,  2981,  2981,  2981,  2981,  2981,
    2981,  2981,  2981,  2981,  2981,  2981,  2981,  2981,  2981,  2981,
    2981,  2981,  2981,  2981,  2981,  2981,  -405,  -405,  -405,   136,
    -405,  -405,  2664,  2981,   117,   137,  -405,  -405,  -405,  1063,
     263,   165,  -405,  2981,   165,  -405,  -405,  -405,  -405,  -405,
    -405,   142,  -405,  -405,  2238,   -29,  -405,   -26,   296,   138,
     165,   305,   144,  -405,   954,   143,   138,  -405,   148,  -405,
     149,   -23,  3197,  -405,  -405,  -405,  -405,  -405,    86,    86,
     -93,   -93,    81,    81,    81,    81,   -82,   -82,    95,    87,
      89,    90,   216,   228,  -143,  -405,  2981,   129,   150,  -405,
    2981,   134,   165,   145,   131,  -112,   152,  -405,  2981,  -405,
     141,   155,  1435,   151,   154,  1675,  -405,  -405,  -405,  -405,
    -405,  2981,   156,  -405,  2981,   157,  2981,   158,  -405,  2981,
     -85,  2771,  -405,  -405,  -405,     6,  2981,  1675,   367,  -405,
      -5,  -405,  2981,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    2981,  -405,  -405,  -405,   160,   159,   138,  -405,  1435,  2981,
     153,  -405,  -405,  1195,  1435,    -3,  -405,  -405,  -405,  -405,
    -405,  -131,  -405,  -405,  -405,  -405,  -405,  1435,  -405
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -405,  -405,  -405,  -405,  -405,  -405,    -2,  -405,  -405,   -75,
    -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,  -405,
    -405,  -405,  -405,  -405,  -120,  -405,   -80,   -79,   -59,   -69,
      43,    45,    46,    47,    48,    59,  -405,  -170,  -191,  -405,
    -186,  -251,    27,    30,  -405,  -405,  -405,   125,   230,   225,
     161,  -405,  -405,  -254,   -10,  -405,  -405,   105,  -405,   -91,
    -405,  -405,   -89,   -13,   -74,  -405,  -405,   295,  -405,   213,
    -154,  -338,    33,  -323,   109,  -193,  -404,  -405,  -405,   -56,
     293,   103,   115,  -405,  -405,    32,  -405,  -405,   -73,  -405,
     -78,  -405,  -405,  -405,  -405,  -405,  -405,   300,  -405,  -405,
    -108,  -405,   133,   162,  -405,  -405
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -331
static const yytype_int16 yytable[] =
{
     121,   289,    11,   117,   343,   143,   268,   144,   158,   173,
      16,   355,   294,   479,   480,   479,   480,   272,     7,     8,
       9,   142,   299,   425,   332,   333,   136,   281,   172,   174,
     419,   463,   272,   310,     7,     8,     9,   159,   160,   161,
     109,   170,   380,   110,     7,     8,     9,    17,    18,     4,
     262,   177,     1,   463,   184,     5,   267,   348,   312,   313,
     314,   315,   316,   317,   318,   319,   320,   321,   263,   302,
     303,   185,   356,   158,   165,   352,    35,    36,    37,    12,
     171,    39,   345,    10,   450,    15,   297,   298,   132,   186,
     166,   138,   268,   353,   345,   121,   444,   448,   117,   345,
     364,   418,   159,   160,   161,   375,   175,   324,   498,   377,
     379,   345,   426,   364,   169,   346,   365,   393,   386,   136,
     380,   285,   106,   469,   180,   286,   471,   376,   281,   453,
     178,   344,   394,   328,   329,   109,   173,   106,   110,   486,
     364,   245,   267,    17,    18,   153,   154,   487,   146,   188,
     259,   334,   335,   307,   415,   172,   472,   308,   414,   187,
     133,   134,   356,   261,   135,   445,   137,   147,   180,   273,
     180,   265,    35,    36,    37,   148,   139,    39,    40,    41,
      42,   245,   268,   330,   331,   322,   149,   304,   379,   305,
     383,   348,   432,   268,   345,   433,   345,   150,   440,   345,
     474,   443,   441,   336,   337,   395,   396,   397,   267,   267,
     267,   267,   267,   267,   267,   267,   267,   267,   267,   267,
     267,   267,   267,   267,  -330,  -116,   156,   475,   151,   356,
     388,   345,   267,   176,   140,   157,   141,   245,   481,   458,
     496,   179,   455,   267,   367,   368,   268,   259,   398,   399,
     466,   400,   401,   467,   183,   356,   -20,   349,   356,     7,
       8,     9,   361,    97,    98,    99,   273,   406,   407,   180,
     476,   356,   -21,   402,   403,   404,   405,   245,   264,   356,
     271,   268,   283,   245,   188,   490,   381,   143,   245,   144,
     493,   495,   172,   491,   284,   290,   267,   287,   288,    17,
      18,   292,   291,   387,   495,   167,     8,   168,    17,    18,
     159,   160,   161,   295,    17,    18,   325,   326,   327,   296,
     300,   -46,   338,   306,   311,   339,   341,   340,    35,    36,
      37,   267,   139,    39,    40,    41,    42,    35,    36,    37,
     342,   139,    39,    35,    36,    37,   422,    38,    39,   153,
     -45,   358,   272,   360,   363,   371,   -40,   420,   416,   423,
     421,   245,   428,   345,   381,   435,   437,   451,   438,   446,
     439,   245,   452,   447,   449,   454,   457,   -50,   434,   478,
     468,   408,   456,   488,   409,   350,   410,   256,   411,   260,
     412,   370,   492,   460,   462,   145,   282,   427,   470,   372,
     489,   477,   413,   155,   384,   373,   429,   497,   152,   362,
     494,     0,     0,     0,     0,     0,     0,     0,     0,   245,
       0,   351,   245,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   359,     0,     0,     0,     0,
       0,     0,     0,     0,   245,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   245,     0,     0,     0,     0,
     245,   245,     0,     0,     0,    17,    18,    19,    20,    21,
      22,   192,   193,   194,   245,   195,   196,   197,   198,   199,
       0,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,     0,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,   200,   140,    95,   141,   201,
     202,   203,   204,   205,     0,     0,   206,   207,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,     0,     0,     0,     0,    96,    97,    98,    99,     0,
     100,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   106,     0,     0,     0,     0,     0,     0,
       0,     0,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,     0,     0,    95,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   208,     0,     0,     0,     0,     0,   209,   210,
     211,   212,     0,     0,     0,    97,    98,    99,     0,     0,
       0,     0,     0,   213,   214,   215,    17,    18,    19,    20,
      21,    22,   192,   193,   194,     0,   195,   196,   197,   198,
     199,     0,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,     0,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,   200,   140,    95,   141,
     201,   202,   203,   204,   205,     0,     0,   206,   207,     0,
       0,     0,     0,     0,   280,     0,     0,    19,    20,    21,
      22,     0,     0,     0,     0,     0,    96,    97,    98,    99,
       0,   100,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   106,     0,     0,     0,     0,     0,
       0,     0,     0,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,     0,     0,    95,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   208,     0,     0,     0,     0,     0,   209,
     210,   211,   212,     0,     0,     0,    97,    98,    99,     0,
       0,     0,     0,     0,   213,   214,   347,    17,    18,    19,
      20,    21,    22,   192,   193,   194,     0,   195,   196,   197,
     198,   199,     0,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,     0,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,   200,   140,    95,
     141,   201,   202,   203,   204,   205,     0,     0,   206,   207,
       0,     0,     0,     0,     0,   366,     0,     0,    19,    20,
      21,    22,     0,     0,     0,     0,     0,    96,    97,    98,
      99,     0,   100,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   106,     0,     0,     0,     0,
       0,     0,     0,     0,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,     0,     0,    95,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   208,     0,     0,     0,     0,     0,
     209,   210,   211,   212,     0,     0,     0,    97,    98,    99,
       0,     0,     0,     0,     0,   213,   214,   436,    17,    18,
      19,    20,    21,    22,   192,   193,   194,     0,   195,   196,
     197,   198,   199,   479,   480,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
       0,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,   200,   140,
      95,   141,   201,   202,   203,   204,   205,     0,     0,   206,
     207,     0,     0,     0,     0,     0,     0,     0,     0,    19,
      20,    21,    22,     0,     0,     0,     0,     0,    96,    97,
      98,    99,     0,   100,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   106,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,     0,     0,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
       0,   209,   210,   211,   212,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,   214,    17,    18,
      19,    20,    21,    22,   192,   193,   194,     0,   195,   196,
     197,   198,   199,     0,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
       0,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,   200,   140,
      95,   141,   201,   202,   203,   204,   205,     0,     0,   206,
     207,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    96,    97,
      98,    99,     0,   100,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   106,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
       0,   209,   210,   211,   212,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,   214,    17,    18,
      19,    20,    21,    22,   192,   193,   194,     0,   195,   196,
     197,   198,   199,     0,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
       0,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,   200,   140,
      95,   141,   201,   202,   203,   204,   205,     0,     0,   206,
     207,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    96,    97,
      98,    99,     0,   100,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   106,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
       0,   209,   210,   211,   212,    17,    18,    19,    20,    21,
      22,     0,     0,     0,     0,     0,   213,   154,     0,     0,
       0,     0,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,     0,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,     0,   140,    95,   141,   201,
     202,   203,   204,   205,     0,     0,   206,   207,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,     0,     0,     0,     0,    96,    97,    98,    99,     0,
     100,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   106,     0,     0,     0,     0,     0,     0,
       0,     0,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,     0,   140,    95,   141,   201,   202,
     203,   204,   205,     0,     0,   206,   207,     0,     0,     0,
       0,     0,   208,     0,     0,     0,     0,     0,   209,   210,
     211,   212,     0,     0,     0,    97,    98,    99,     0,     0,
       0,     0,     0,   213,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   208,     0,     0,     0,     0,     0,   209,   210,   211,
     212,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,   293,     0,     0,     0,     0,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,     0,   139,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,     0,   140,    95,   141,   201,   202,   203,   204,   205,
       0,     0,   206,   207,     0,     0,     0,     0,     0,     0,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,   378,    97,    98,    99,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,     0,     0,   106,
       0,     0,     0,     0,     0,     0,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,     0,   140,
      95,   141,   201,   202,   203,   204,   205,     0,     0,   206,
     207,     0,     0,     0,     0,     0,     0,     0,   208,    19,
      20,    21,    22,     0,   209,   210,   211,   212,     0,    97,
      98,    99,     0,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,     0,   140,    95,
     141,   201,   202,   203,   204,   205,     0,     0,   206,   207,
       0,     0,     0,     0,     0,   208,     0,     0,   266,     0,
       0,   209,   210,   211,   212,     0,     0,     0,    97,    98,
      99,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    19,
      20,    21,    22,     0,   208,     0,     0,   354,     0,     0,
     209,   210,   211,   212,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,     0,   140,    95,
     141,   201,   202,   203,   204,   205,     0,     0,   206,   207,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,     0,     0,     0,     0,     0,     0,     0,    97,    98,
      99,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,     0,   140,    95,   141,   201,   202,
     203,   204,   205,     0,     0,   206,   207,     0,     0,     0,
       0,     0,     0,     0,   208,     0,     0,   417,     0,     0,
     209,   210,   211,   212,     0,    97,    98,    99,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,   208,     0,     0,   473,     0,     0,   209,   210,   211,
     212,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,     0,   140,    95,   141,   201,   202,
     203,   204,   205,     0,     0,   206,   207,     0,     0,     0,
       0,     0,     0,    19,    20,    21,    22,     0,     0,     0,
       0,     0,     0,     0,     0,    97,    98,    99,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
     309,     0,   140,    95,   141,   201,   202,   203,   204,   205,
       0,     0,   206,   207,     0,     0,     0,     0,     0,     0,
       0,   208,    19,    20,    21,    22,     0,   209,   210,   211,
     212,     0,    97,    98,    99,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,   442,
       0,   140,    95,   141,   201,   202,   203,   204,   205,     0,
       0,   206,   207,     0,     0,     0,     0,     0,   208,     0,
       0,     0,     0,     0,   209,   210,   211,   212,     0,     0,
       0,    97,    98,    99,    17,    18,    19,    20,    21,    22,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,     0,     0,    95,   208,     0,     0,
       0,     0,     0,   209,   210,   211,   212,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    96,    97,    98,    99,     0,   100,
       0,     0,     0,     0,     0,     0,     0,   101,   102,   103,
     104,   105,   106
};

static const yytype_int16 yycheck[] =
{
      13,   194,     4,    13,   111,    96,   176,    96,     4,   117,
      12,   262,   198,    18,    19,    18,    19,    36,    94,    95,
      96,    96,   208,   361,   106,   107,    96,   181,   117,   118,
     353,   435,    36,   224,    94,    95,    96,    33,    34,    35,
      13,   116,   296,    13,    94,    95,    96,     3,     4,    98,
     222,   125,   129,   457,   143,     0,   176,   250,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   240,   104,
     105,   145,   263,     4,   225,   222,    32,    33,    34,   130,
      36,    37,   225,   133,   422,   133,   206,   207,   132,   149,
     241,    93,   262,   240,   225,   108,   239,   420,   108,   225,
     225,   352,    33,    34,    35,   291,   119,   227,   239,   295,
     296,   225,   363,   225,   116,   241,   241,   308,   304,    96,
     374,   221,   141,   446,   137,   225,   449,   241,   282,   241,
     132,   238,   323,   226,   227,   108,   244,   141,   108,   462,
     225,   154,   262,     3,     4,   241,   242,   470,   133,   151,
     163,   233,   234,   221,   345,   244,   241,   225,   344,   219,
      33,    34,   353,   165,    37,   416,   242,   133,   181,   179,
     183,   241,    32,    33,    34,   133,    36,    37,    38,    39,
      40,   194,   352,   102,   103,   240,   133,   222,   374,   224,
     221,   384,   221,   363,   225,   221,   225,   133,   221,   225,
     451,   392,   225,   108,   109,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   243,   221,   221,   221,   220,   420,
     305,   225,   352,   222,    94,   225,    96,   250,   243,   432,
     243,   242,   428,   363,    98,    99,   416,   260,   328,   329,
     441,   330,   331,   444,   242,   446,   220,   259,   449,    94,
      95,    96,   275,   124,   125,   126,   276,   336,   337,   282,
     456,   462,   220,   332,   333,   334,   335,   290,   220,   470,
     133,   451,   241,   296,   286,   478,   296,   378,   301,   378,
     483,   484,   381,   479,   240,   220,   416,   241,   241,     3,
       4,   241,   220,   305,   497,    94,    95,    96,     3,     4,
      33,    34,    35,   220,     3,     4,   230,   231,   232,   220,
     243,   220,   235,   221,   220,   236,   110,   237,    32,    33,
      34,   451,    36,    37,    38,    39,    40,    32,    33,    34,
     112,    36,    37,    32,    33,    34,   359,    36,    37,   241,
     220,   223,    36,   243,   222,    93,   221,   240,   222,    96,
     223,   374,   220,   225,   374,   221,   223,   222,   220,   240,
     221,   384,   241,   223,   240,   223,   221,   221,   380,    12,
     223,   338,   241,   223,   339,   260,   340,   157,   341,   164,
     342,   286,   239,   242,   240,   100,   183,   364,   240,   290,
     241,   457,   343,   110,   301,   290,   374,   485,   108,   276,
     483,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   432,
      -1,   260,   435,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   273,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   457,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   478,    -1,    -1,    -1,    -1,
     483,   484,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,   497,    13,    14,    15,    16,    17,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,    -1,    -1,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,   123,   124,   125,   126,    -1,
     128,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   141,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    95,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,   229,    -1,    -1,    -1,   124,   125,   126,    -1,    -1,
      -1,    -1,    -1,   241,   242,   243,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    13,    14,    15,    16,
      17,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    -1,    -1,   104,   105,    -1,
      -1,    -1,    -1,    -1,   243,    -1,    -1,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,   123,   124,   125,   126,
      -1,   128,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,   141,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    -1,    -1,    95,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,   226,
     227,   228,   229,    -1,    -1,    -1,   124,   125,   126,    -1,
      -1,    -1,    -1,    -1,   241,   242,   243,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    13,    14,    15,
      16,    17,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,    -1,    -1,   104,   105,
      -1,    -1,    -1,    -1,    -1,   243,    -1,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   123,   124,   125,
     126,    -1,   128,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,   141,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    -1,    -1,    95,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,
     226,   227,   228,   229,    -1,    -1,    -1,   124,   125,   126,
      -1,    -1,    -1,    -1,    -1,   241,   242,   243,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    -1,    -1,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,   123,   124,
     125,   126,    -1,   128,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,   141,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    -1,    95,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,   226,   227,   228,   229,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   241,   242,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    -1,    -1,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    -1,    -1,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   123,   124,
     125,   126,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,   226,   227,   228,   229,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   241,   242,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    -1,    -1,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,    -1,    -1,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   123,   124,
     125,   126,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   141,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   220,    -1,    -1,    -1,    -1,
      -1,   226,   227,   228,   229,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,   241,   242,    -1,    -1,
      -1,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    -1,    94,    95,    96,    97,
      98,    99,   100,   101,    -1,    -1,   104,   105,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,   123,   124,   125,   126,    -1,
     128,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   141,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    -1,    94,    95,    96,    97,    98,
      99,   100,   101,    -1,    -1,   104,   105,    -1,    -1,    -1,
      -1,    -1,   220,    -1,    -1,    -1,    -1,    -1,   226,   227,
     228,   229,    -1,    -1,    -1,   124,   125,   126,    -1,    -1,
      -1,    -1,    -1,   241,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   220,    -1,    -1,    -1,    -1,    -1,   226,   227,   228,
     229,     3,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,   241,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    -1,    94,    95,    96,    97,    98,    99,   100,   101,
      -1,    -1,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   123,   124,   125,   126,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    -1,    -1,   141,
      -1,    -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    -1,    94,
      95,    96,    97,    98,    99,   100,   101,    -1,    -1,   104,
     105,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   220,     5,
       6,     7,     8,    -1,   226,   227,   228,   229,    -1,   124,
     125,   126,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    94,    95,
      96,    97,    98,    99,   100,   101,    -1,    -1,   104,   105,
      -1,    -1,    -1,    -1,    -1,   220,    -1,    -1,   223,    -1,
      -1,   226,   227,   228,   229,    -1,    -1,    -1,   124,   125,
     126,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    -1,   220,    -1,    -1,   223,    -1,    -1,
     226,   227,   228,   229,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    -1,    94,    95,
      96,    97,    98,    99,   100,   101,    -1,    -1,   104,   105,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   125,
     126,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    -1,    94,    95,    96,    97,    98,
      99,   100,   101,    -1,    -1,   104,   105,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   220,    -1,    -1,   223,    -1,    -1,
     226,   227,   228,   229,    -1,   124,   125,   126,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,   220,    -1,    -1,   223,    -1,    -1,   226,   227,   228,
     229,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    -1,    94,    95,    96,    97,    98,
      99,   100,   101,    -1,    -1,   104,   105,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   124,   125,   126,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    -1,    94,    95,    96,    97,    98,    99,   100,   101,
      -1,    -1,   104,   105,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   220,     5,     6,     7,     8,    -1,   226,   227,   228,
     229,    -1,   124,   125,   126,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      -1,    94,    95,    96,    97,    98,    99,   100,   101,    -1,
      -1,   104,   105,    -1,    -1,    -1,    -1,    -1,   220,    -1,
      -1,    -1,    -1,    -1,   226,   227,   228,   229,    -1,    -1,
      -1,   124,   125,   126,     3,     4,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    -1,    -1,    95,   220,    -1,    -1,
      -1,    -1,    -1,   226,   227,   228,   229,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   123,   124,   125,   126,    -1,   128,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,   138,
     139,   140,   141
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   129,   245,   247,    98,     0,   249,    94,    95,    96,
     133,   250,   130,   246,   251,   133,   250,     3,     4,     5,
       6,     7,     8,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    95,   123,   124,   125,   126,
     128,   136,   137,   138,   139,   140,   141,   248,   252,   286,
     287,   288,   289,   290,   295,   296,   297,   298,   303,   305,
     306,   307,   308,   309,   310,   311,   312,   341,   342,   343,
     344,   349,   132,    33,    34,    37,    96,   242,   250,    36,
      94,    96,   253,   303,   306,   311,   133,   133,   133,   133,
     133,   220,   341,   241,   242,   324,   221,   225,     4,    33,
      34,    35,   292,   293,   304,   225,   241,    94,    96,   250,
     253,    36,   306,   344,   306,   307,   222,   308,   250,   242,
     307,   313,   314,   242,   306,   308,   149,   219,   250,   299,
     301,   302,     9,    10,    11,    13,    14,    15,    16,    17,
      93,    97,    98,    99,   100,   101,   104,   105,   220,   226,
     227,   228,   229,   241,   242,   243,   253,   254,   255,   257,
     258,   259,   260,   261,   262,   263,   268,   269,   270,   271,
     272,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   284,   286,   287,   298,   307,   318,   319,   320,   321,
     325,   326,   327,   330,   336,   340,   292,   291,   294,   307,
     293,   250,   222,   240,   220,   241,   223,   268,   281,   285,
     307,   133,    36,   298,   346,   347,   348,   250,   315,   316,
     243,   314,   313,   241,   240,   221,   225,   241,   241,   319,
     220,   220,   241,   241,   284,   220,   220,   268,   268,   284,
     243,   322,   104,   105,   222,   224,   221,   221,   225,    92,
     282,   220,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   240,   283,   268,   230,   231,   232,   226,   227,
     102,   103,   106,   107,   233,   234,   108,   109,   235,   236,
     237,   110,   112,   111,   238,   225,   241,   243,   319,   250,
     291,   294,   222,   240,   223,   285,   282,   317,   223,   347,
     243,   307,   346,   222,   225,   241,   243,    98,    99,   300,
     301,    93,   318,   326,   337,   284,   241,   284,   123,   284,
     297,   298,   329,   221,   325,   256,   284,   250,   253,   264,
     265,   266,   267,   282,   282,   268,   268,   268,   270,   270,
     271,   271,   272,   272,   272,   272,   273,   273,   274,   275,
     276,   277,   278,   279,   284,   282,   222,   223,   285,   317,
     240,   223,   307,    96,   345,   315,   285,   316,   220,   329,
     338,   339,   221,   221,   250,   221,   243,   223,   220,   221,
     221,   225,    92,   282,   239,   285,   240,   223,   317,   240,
     315,   222,   241,   241,   223,   284,   241,   221,   319,   328,
     242,   331,   240,   320,   323,   324,   282,   282,   223,   317,
     240,   317,   241,   223,   285,   221,   284,   323,    12,    18,
      19,   243,   332,   333,   334,   335,   317,   317,   223,   241,
     319,   284,   239,   319,   332,   319,   243,   334,   239
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
#line 53 "src/glsl/glsl_parser.yy"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
}
/* Line 1078 of yacc.c.  */
#line 2900 "src/glsl/glsl_parser.cpp"
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
#line 258 "src/glsl/glsl_parser.yy"
    {
	   _mesa_glsl_initialize_types(state);
	;}
    break;

  case 3:
#line 262 "src/glsl/glsl_parser.yy"
    {
	   delete state->symbols;
	   state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
	   _mesa_glsl_initialize_types(state);
	;}
    break;

  case 5:
#line 272 "src/glsl/glsl_parser.yy"
    {
           state->process_version_directive(&(yylsp[(2) - (3)]), (yyvsp[(2) - (3)].n), NULL);
	;}
    break;

  case 6:
#line 276 "src/glsl/glsl_parser.yy"
    {
           state->process_version_directive(&(yylsp[(2) - (4)]), (yyvsp[(2) - (4)].n), (yyvsp[(3) - (4)].identifier));
        ;}
    break;

  case 11:
#line 287 "src/glsl/glsl_parser.yy"
    {
	   if (!state->is_version(120, 100)) {
	      _mesa_glsl_warning(& (yylsp[(1) - (2)]), state,
				 "pragma `invariant(all)' not supported in %s "
                                 "(GLSL ES 1.00 or GLSL 1.20 required).",
				 state->get_version_string());
	   } else {
	      state->all_invariant = true;
	   }
	;}
    break;

  case 17:
#line 312 "src/glsl/glsl_parser.yy"
    {
	   if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
	      YYERROR;
	   }
	;}
    break;

  case 18:
#line 321 "src/glsl/glsl_parser.yy"
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[(1) - (1)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
	;}
    break;

  case 19:
#line 329 "src/glsl/glsl_parser.yy"
    {
	   /* FINISHME: The NULL test is required because pragmas are set to
	    * FINISHME: NULL. (See production rule for external_declaration.)
	    */
	   if ((yyvsp[(2) - (2)].node) != NULL)
	      state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 22:
#line 345 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
	;}
    break;

  case 23:
#line 352 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 24:
#line 359 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 25:
#line 366 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
	;}
    break;

  case 26:
#line 373 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
	;}
    break;

  case 27:
#line 380 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(2) - (3)].expression);
	;}
    break;

  case 29:
#line 388 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 30:
#line 394 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 31:
#line 398 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
	;}
    break;

  case 32:
#line 405 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 33:
#line 411 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 37:
#line 429 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 42:
#line 448 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	;}
    break;

  case 43:
#line 454 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	;}
    break;

  case 45:
#line 470 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 46:
#line 476 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 47:
#line 483 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 52:
#line 503 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (2)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
	;}
    break;

  case 53:
#line 509 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (3)].expression);
	   (yyval.expression)->set_location(yylloc);
	   (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
	;}
    break;

  case 54:
#line 521 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (2)].identifier));
	   (yyval.expression) = new(ctx) ast_function_expression(callee);
	   (yyval.expression)->set_location(yylloc);
   	;}
    break;

  case 56:
#line 533 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 57:
#line 539 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 58:
#line 545 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 59:
#line 554 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_plus; ;}
    break;

  case 60:
#line 555 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_neg; ;}
    break;

  case 61:
#line 556 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_logic_not; ;}
    break;

  case 62:
#line 557 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_bit_not; ;}
    break;

  case 64:
#line 563 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 65:
#line 569 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 66:
#line 575 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 68:
#line 585 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 69:
#line 591 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 71:
#line 601 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 72:
#line 607 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 74:
#line 617 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 75:
#line 623 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 76:
#line 629 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 77:
#line 635 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 79:
#line 645 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 80:
#line 651 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 82:
#line 661 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 84:
#line 671 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 86:
#line 681 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 88:
#line 691 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 90:
#line 701 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 92:
#line 711 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 94:
#line 721 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 96:
#line 731 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
	   (yyval.expression)->set_location(yylloc);
	;}
    break;

  case 97:
#line 739 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_assign; ;}
    break;

  case 98:
#line 740 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mul_assign; ;}
    break;

  case 99:
#line 741 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_div_assign; ;}
    break;

  case 100:
#line 742 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mod_assign; ;}
    break;

  case 101:
#line 743 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_add_assign; ;}
    break;

  case 102:
#line 744 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_sub_assign; ;}
    break;

  case 103:
#line 745 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_ls_assign; ;}
    break;

  case 104:
#line 746 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_rs_assign; ;}
    break;

  case 105:
#line 747 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_and_assign; ;}
    break;

  case 106:
#line 748 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_xor_assign; ;}
    break;

  case 107:
#line 749 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_or_assign; ;}
    break;

  case 108:
#line 754 "src/glsl/glsl_parser.yy"
    {
	   (yyval.expression) = (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 109:
#line 758 "src/glsl/glsl_parser.yy"
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

  case 111:
#line 778 "src/glsl/glsl_parser.yy"
    {
	   state->symbols->pop_scope();
	   (yyval.node) = (yyvsp[(1) - (2)].function);
	;}
    break;

  case 112:
#line 783 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
	;}
    break;

  case 113:
#line 787 "src/glsl/glsl_parser.yy"
    {
	   (yyvsp[(3) - (4)].type_specifier)->precision = (yyvsp[(2) - (4)].n);
	   (yyvsp[(3) - (4)].type_specifier)->is_precision_statement = true;
	   (yyval.node) = (yyvsp[(3) - (4)].type_specifier);
	;}
    break;

  case 114:
#line 793 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (yyvsp[(1) - (1)].node);
	;}
    break;

  case 118:
#line 809 "src/glsl/glsl_parser.yy"
    {
	   (yyval.function) = (yyvsp[(1) - (2)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
	;}
    break;

  case 119:
#line 814 "src/glsl/glsl_parser.yy"
    {
	   (yyval.function) = (yyvsp[(1) - (3)].function);
	   (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
	;}
    break;

  case 120:
#line 822 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.function) = new(ctx) ast_function();
	   (yyval.function)->set_location(yylloc);
	   (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
	   (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);

	   state->symbols->add_function(new(state) ir_function((yyvsp[(2) - (3)].identifier)));
	   state->symbols->push_scope();
	;}
    break;

  case 121:
#line 836 "src/glsl/glsl_parser.yy"
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

  case 122:
#line 846 "src/glsl/glsl_parser.yy"
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

  case 123:
#line 861 "src/glsl/glsl_parser.yy"
    {
	   (yyvsp[(1) - (3)].type_qualifier).flags.i |= (yyvsp[(2) - (3)].type_qualifier).flags.i;

	   (yyval.parameter_declarator) = (yyvsp[(3) - (3)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (3)].type_qualifier);
	;}
    break;

  case 124:
#line 868 "src/glsl/glsl_parser.yy"
    {
	   (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	;}
    break;

  case 125:
#line 873 "src/glsl/glsl_parser.yy"
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

  case 126:
#line 884 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
	   (yyval.parameter_declarator)->set_location(yylloc);
	   (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
	   (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 127:
#line 896 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	;}
    break;

  case 128:
#line 900 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 129:
#line 905 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 130:
#line 910 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 133:
#line 924 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), false, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (3)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 134:
#line 934 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), true, NULL, NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 135:
#line 944 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), true, (yyvsp[(5) - (6)].expression), NULL);
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (6)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 136:
#line 954 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (7)].identifier), true, NULL, (yyvsp[(7) - (7)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (7)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (7)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 137:
#line 964 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (8)].identifier), true, (yyvsp[(5) - (8)].expression), (yyvsp[(8) - (8)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (8)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (8)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 138:
#line 974 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), false, NULL, (yyvsp[(5) - (5)].expression));
	   decl->set_location(yylloc);

	   (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	   state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto, glsl_precision_undefined));
	;}
    break;

  case 139:
#line 988 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   /* Empty declaration list is valid. */
	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (1)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	;}
    break;

  case 140:
#line 995 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 141:
#line 1004 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), true, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 142:
#line 1013 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), true, (yyvsp[(4) - (5)].expression), NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 143:
#line 1022 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (6)].identifier), true, NULL, (yyvsp[(6) - (6)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (6)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 144:
#line 1031 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (7)].identifier), true, (yyvsp[(4) - (7)].expression), (yyvsp[(7) - (7)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (7)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 145:
#line 1040 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), false, NULL, (yyvsp[(4) - (4)].expression));

	   (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 146:
#line 1049 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), false, NULL, NULL);

	   (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
	   (yyval.declarator_list)->set_location(yylloc);
	   (yyval.declarator_list)->invariant = true;

	   (yyval.declarator_list)->declarations.push_tail(&decl->link);
	;}
    break;

  case 147:
#line 1063 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
	;}
    break;

  case 148:
#line 1070 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
	   (yyval.fully_specified_type)->set_location(yylloc);
	   (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
	;}
    break;

  case 149:
#line 1081 "src/glsl/glsl_parser.yy"
    {
	  (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
	;}
    break;

  case 151:
#line 1089 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (3)].type_qualifier);
	   if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(3) - (3)]), state, (yyvsp[(3) - (3)].type_qualifier))) {
	      YYERROR;
	   }
	;}
    break;

  case 152:
#line 1098 "src/glsl/glsl_parser.yy"
    { (yyval.n) = (yyvsp[(1) - (1)].n); ;}
    break;

  case 153:
#line 1099 "src/glsl/glsl_parser.yy"
    { (yyval.n) = (yyvsp[(1) - (1)].n); ;}
    break;

  case 154:
#line 1104 "src/glsl/glsl_parser.yy"
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
	      } else if (strcmp((yyvsp[(1) - (1)].identifier), "row_major") == 0) {
	         (yyval.type_qualifier).flags.q.row_major = 1;
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
	;}
    break;

  case 155:
#line 1174 "src/glsl/glsl_parser.yy"
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
	;}
    break;

  case 156:
#line 1220 "src/glsl/glsl_parser.yy"
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
	;}
    break;

  case 157:
#line 1242 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.row_major = 1;
	;}
    break;

  case 158:
#line 1247 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.packed = 1;
	;}
    break;

  case 159:
#line 1255 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.smooth = 1;
	;}
    break;

  case 160:
#line 1260 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.flat = 1;
	;}
    break;

  case 161:
#line 1265 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.noperspective = 1;
	;}
    break;

  case 162:
#line 1273 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	;}
    break;

  case 165:
#line 1283 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	;}
    break;

  case 167:
#line 1289 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(2) - (2)].type_qualifier).flags.i;
	;}
    break;

  case 168:
#line 1294 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 169:
#line 1299 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_qualifier) = (yyvsp[(2) - (3)].type_qualifier);
	   (yyval.type_qualifier).flags.i |= (yyvsp[(3) - (3)].type_qualifier).flags.i;
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 170:
#line 1305 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.invariant = 1;
	;}
    break;

  case 171:
#line 1313 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.constant = 1;
	;}
    break;

  case 172:
#line 1318 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.attribute = 1;
	;}
    break;

  case 173:
#line 1323 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.varying = 1;
	;}
    break;

  case 174:
#line 1328 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1;
	   (yyval.type_qualifier).flags.q.varying = 1;
	;}
    break;

  case 175:
#line 1334 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 176:
#line 1339 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 177:
#line 1344 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.in = 1;
	;}
    break;

  case 178:
#line 1349 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.centroid = 1; (yyval.type_qualifier).flags.q.out = 1;
	;}
    break;

  case 179:
#line 1354 "src/glsl/glsl_parser.yy"
    {
	   memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	   (yyval.type_qualifier).flags.q.uniform = 1;
	;}
    break;

  case 180:
#line 1362 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (1)].type_specifier);
	;}
    break;

  case 181:
#line 1366 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(2) - (2)].type_specifier);
	   (yyval.type_specifier)->precision = (yyvsp[(1) - (2)].n);
	;}
    break;

  case 183:
#line 1375 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (3)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = NULL;
	;}
    break;

  case 184:
#line 1381 "src/glsl/glsl_parser.yy"
    {
	   (yyval.type_specifier) = (yyvsp[(1) - (4)].type_specifier);
	   (yyval.type_specifier)->is_array = true;
	   (yyval.type_specifier)->array_size = (yyvsp[(3) - (4)].expression);
	;}
    break;

  case 185:
#line 1390 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 186:
#line 1396 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 187:
#line 1402 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
	   (yyval.type_specifier)->set_location(yylloc);
	;}
    break;

  case 188:
#line 1410 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "void"; ;}
    break;

  case 189:
#line 1411 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "float"; ;}
    break;

  case 190:
#line 1412 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "int"; ;}
    break;

  case 191:
#line 1413 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uint"; ;}
    break;

  case 192:
#line 1414 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bool"; ;}
    break;

  case 193:
#line 1415 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec2"; ;}
    break;

  case 194:
#line 1416 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec3"; ;}
    break;

  case 195:
#line 1417 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec4"; ;}
    break;

  case 196:
#line 1418 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec2"; ;}
    break;

  case 197:
#line 1419 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec3"; ;}
    break;

  case 198:
#line 1420 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec4"; ;}
    break;

  case 199:
#line 1421 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec2"; ;}
    break;

  case 200:
#line 1422 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec3"; ;}
    break;

  case 201:
#line 1423 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec4"; ;}
    break;

  case 202:
#line 1424 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec2"; ;}
    break;

  case 203:
#line 1425 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec3"; ;}
    break;

  case 204:
#line 1426 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec4"; ;}
    break;

  case 205:
#line 1427 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2"; ;}
    break;

  case 206:
#line 1428 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x3"; ;}
    break;

  case 207:
#line 1429 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x4"; ;}
    break;

  case 208:
#line 1430 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x2"; ;}
    break;

  case 209:
#line 1431 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3"; ;}
    break;

  case 210:
#line 1432 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x4"; ;}
    break;

  case 211:
#line 1433 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x2"; ;}
    break;

  case 212:
#line 1434 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x3"; ;}
    break;

  case 213:
#line 1435 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4"; ;}
    break;

  case 214:
#line 1436 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1D"; ;}
    break;

  case 215:
#line 1437 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2D"; ;}
    break;

  case 216:
#line 1438 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRect"; ;}
    break;

  case 217:
#line 1439 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler3D"; ;}
    break;

  case 218:
#line 1440 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCube"; ;}
    break;

  case 219:
#line 1441 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerExternalOES"; ;}
    break;

  case 220:
#line 1442 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DShadow"; ;}
    break;

  case 221:
#line 1443 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DShadow"; ;}
    break;

  case 222:
#line 1444 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRectShadow"; ;}
    break;

  case 223:
#line 1445 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeShadow"; ;}
    break;

  case 224:
#line 1446 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArray"; ;}
    break;

  case 225:
#line 1447 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArray"; ;}
    break;

  case 226:
#line 1448 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArrayShadow"; ;}
    break;

  case 227:
#line 1449 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArrayShadow"; ;}
    break;

  case 228:
#line 1450 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerBuffer"; ;}
    break;

  case 229:
#line 1451 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeArray"; ;}
    break;

  case 230:
#line 1452 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeArrayShadow"; ;}
    break;

  case 231:
#line 1453 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1D"; ;}
    break;

  case 232:
#line 1454 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2D"; ;}
    break;

  case 233:
#line 1455 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DRect"; ;}
    break;

  case 234:
#line 1456 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler3D"; ;}
    break;

  case 235:
#line 1457 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerCube"; ;}
    break;

  case 236:
#line 1458 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1DArray"; ;}
    break;

  case 237:
#line 1459 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DArray"; ;}
    break;

  case 238:
#line 1460 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerBuffer"; ;}
    break;

  case 239:
#line 1461 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerCubeArray"; ;}
    break;

  case 240:
#line 1462 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1D"; ;}
    break;

  case 241:
#line 1463 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2D"; ;}
    break;

  case 242:
#line 1464 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DRect"; ;}
    break;

  case 243:
#line 1465 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler3D"; ;}
    break;

  case 244:
#line 1466 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerCube"; ;}
    break;

  case 245:
#line 1467 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1DArray"; ;}
    break;

  case 246:
#line 1468 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DArray"; ;}
    break;

  case 247:
#line 1469 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerBuffer"; ;}
    break;

  case 248:
#line 1470 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerCubeArray"; ;}
    break;

  case 249:
#line 1471 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DMS"; ;}
    break;

  case 250:
#line 1472 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DMS"; ;}
    break;

  case 251:
#line 1473 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DMS"; ;}
    break;

  case 252:
#line 1474 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DMSArray"; ;}
    break;

  case 253:
#line 1475 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DMSArray"; ;}
    break;

  case 254:
#line 1476 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DMSArray"; ;}
    break;

  case 255:
#line 1480 "src/glsl/glsl_parser.yy"
    {
                     state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));

		     (yyval.n) = ast_precision_high;
		  ;}
    break;

  case 256:
#line 1485 "src/glsl/glsl_parser.yy"
    {
                     state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));

		     (yyval.n) = ast_precision_medium;
		  ;}
    break;

  case 257:
#line 1490 "src/glsl/glsl_parser.yy"
    {
                     state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));

		     (yyval.n) = ast_precision_low;
		  ;}
    break;

  case 258:
#line 1499 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	   state->symbols->add_type((yyvsp[(2) - (5)].identifier), glsl_type::void_type);
	;}
    break;

  case 259:
#line 1506 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[(3) - (4)].declarator_list));
	   (yyval.struct_specifier)->set_location(yylloc);
	;}
    break;

  case 260:
#line 1515 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	;}
    break;

  case 261:
#line 1520 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
	   (yyval.declarator_list)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
	;}
    break;

  case 262:
#line 1528 "src/glsl/glsl_parser.yy"
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

  case 263:
#line 1543 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
	   (yyvsp[(1) - (1)].declaration)->link.self_link();
	;}
    break;

  case 264:
#line 1548 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
	   (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
	;}
    break;

  case 265:
#line 1556 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), false, NULL, NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 266:
#line 1562 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (4)].identifier), true, (yyvsp[(3) - (4)].expression), NULL);
	   (yyval.declaration)->set_location(yylloc);
	;}
    break;

  case 269:
#line 1580 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 277:
#line 1595 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 278:
#line 1601 "src/glsl/glsl_parser.yy"
    {
	   state->symbols->push_scope();
	;}
    break;

  case 279:
#line 1605 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(3) - (4)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	   state->symbols->pop_scope();
	;}
    break;

  case 280:
#line 1614 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 282:
#line 1620 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 283:
#line 1626 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
	   (yyval.compound_statement)->set_location(yylloc);
	;}
    break;

  case 284:
#line 1635 "src/glsl/glsl_parser.yy"
    {
	   if ((yyvsp[(1) - (1)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement\n");
	      assert((yyvsp[(1) - (1)].node) != NULL);
	   }

	   (yyval.node) = (yyvsp[(1) - (1)].node);
	   (yyval.node)->link.self_link();
	;}
    break;

  case 285:
#line 1645 "src/glsl/glsl_parser.yy"
    {
	   if ((yyvsp[(2) - (2)].node) == NULL) {
	      _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement\n");
	      assert((yyvsp[(2) - (2)].node) != NULL);
	   }
	   (yyval.node) = (yyvsp[(1) - (2)].node);
	   (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 286:
#line 1657 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement(NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 287:
#line 1663 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 288:
#line 1672 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = new(state) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].selection_rest_statement).then_statement,
						   (yyvsp[(5) - (5)].selection_rest_statement).else_statement);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 289:
#line 1681 "src/glsl/glsl_parser.yy"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (3)].node);
	   (yyval.selection_rest_statement).else_statement = (yyvsp[(3) - (3)].node);
	;}
    break;

  case 290:
#line 1686 "src/glsl/glsl_parser.yy"
    {
	   (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (1)].node);
	   (yyval.selection_rest_statement).else_statement = NULL;
	;}
    break;

  case 291:
#line 1694 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
	;}
    break;

  case 292:
#line 1698 "src/glsl/glsl_parser.yy"
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

  case 293:
#line 1716 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = new(state) ast_switch_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].switch_body));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 294:
#line 1724 "src/glsl/glsl_parser.yy"
    {
	   (yyval.switch_body) = new(state) ast_switch_body(NULL);
	   (yyval.switch_body)->set_location(yylloc);
	;}
    break;

  case 295:
#line 1729 "src/glsl/glsl_parser.yy"
    {
	   (yyval.switch_body) = new(state) ast_switch_body((yyvsp[(2) - (3)].case_statement_list));
	   (yyval.switch_body)->set_location(yylloc);
	;}
    break;

  case 296:
#line 1737 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label) = new(state) ast_case_label((yyvsp[(2) - (3)].expression));
	   (yyval.case_label)->set_location(yylloc);
	;}
    break;

  case 297:
#line 1742 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label) = new(state) ast_case_label(NULL);
	   (yyval.case_label)->set_location(yylloc);
	;}
    break;

  case 298:
#line 1750 "src/glsl/glsl_parser.yy"
    {
	   ast_case_label_list *labels = new(state) ast_case_label_list();

	   labels->labels.push_tail(& (yyvsp[(1) - (1)].case_label)->link);
	   (yyval.case_label_list) = labels;
	   (yyval.case_label_list)->set_location(yylloc);
	;}
    break;

  case 299:
#line 1758 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_label_list) = (yyvsp[(1) - (2)].case_label_list);
	   (yyval.case_label_list)->labels.push_tail(& (yyvsp[(2) - (2)].case_label)->link);
	;}
    break;

  case 300:
#line 1766 "src/glsl/glsl_parser.yy"
    {
	   ast_case_statement *stmts = new(state) ast_case_statement((yyvsp[(1) - (2)].case_label_list));
	   stmts->set_location(yylloc);

	   stmts->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
	   (yyval.case_statement) = stmts;
	;}
    break;

  case 301:
#line 1774 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_statement) = (yyvsp[(1) - (2)].case_statement);
	   (yyval.case_statement)->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
	;}
    break;

  case 302:
#line 1782 "src/glsl/glsl_parser.yy"
    {
	   ast_case_statement_list *cases= new(state) ast_case_statement_list();
	   cases->set_location(yylloc);

	   cases->cases.push_tail(& (yyvsp[(1) - (1)].case_statement)->link);
	   (yyval.case_statement_list) = cases;
	;}
    break;

  case 303:
#line 1790 "src/glsl/glsl_parser.yy"
    {
	   (yyval.case_statement_list) = (yyvsp[(1) - (2)].case_statement_list);
	   (yyval.case_statement_list)->cases.push_tail(& (yyvsp[(2) - (2)].case_statement)->link);
	;}
    break;

  case 304:
#line 1798 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
	   					    NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 305:
#line 1805 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
						    NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 306:
#line 1812 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
						    (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 310:
#line 1828 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = NULL;
	;}
    break;

  case 311:
#line 1835 "src/glsl/glsl_parser.yy"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
	   (yyval.for_rest_statement).rest = NULL;
	;}
    break;

  case 312:
#line 1840 "src/glsl/glsl_parser.yy"
    {
	   (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
	   (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
	;}
    break;

  case 313:
#line 1849 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 314:
#line 1855 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 315:
#line 1861 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 316:
#line 1867 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 317:
#line 1873 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
	   (yyval.node)->set_location(yylloc);
	;}
    break;

  case 318:
#line 1881 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); ;}
    break;

  case 319:
#line 1882 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 320:
#line 1883 "src/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 321:
#line 1884 "src/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 322:
#line 1889 "src/glsl/glsl_parser.yy"
    {
	   void *ctx = state;
	   (yyval.function_definition) = new(ctx) ast_function_definition();
	   (yyval.function_definition)->set_location(yylloc);
	   (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
	   (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);

	   state->symbols->pop_scope();
	;}
    break;

  case 323:
#line 1903 "src/glsl/glsl_parser.yy"
    {
	   (yyval.node) = (yyvsp[(1) - (1)].uniform_block);
	;}
    break;

  case 324:
#line 1907 "src/glsl/glsl_parser.yy"
    {
	   ast_uniform_block *block = (yyvsp[(2) - (2)].uniform_block);
	   if (!block->layout.merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(1) - (2)].type_qualifier))) {
	      YYERROR;
	   }
	   (yyval.node) = block;
	;}
    break;

  case 325:
#line 1918 "src/glsl/glsl_parser.yy"
    {
	   ast_uniform_block *const block = (yyvsp[(6) - (7)].uniform_block);

	   block->block_name = (yyvsp[(2) - (7)].identifier);
	   block->declarations.push_degenerate_list_at_head(& (yyvsp[(4) - (7)].declarator_list)->link);

	   if (!state->ARB_uniform_buffer_object_enable) {
	      _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
			       "#version 140 / GL_ARB_uniform_buffer_object "
			       "required for defining uniform blocks\n");
	   } else if (state->ARB_uniform_buffer_object_warn) {
	      _mesa_glsl_warning(& (yylsp[(1) - (7)]), state,
				 "#version 140 / GL_ARB_uniform_buffer_object "
				 "required for defining uniform blocks\n");
	   }

	   /* Since block arrays require names, and both features are added in
	    * the same language versions, we don't have to explicitly
	    * version-check both things.
	    */
	   if (block->instance_name != NULL
	       && !(state->language_version == 300 && state->es_shader)) {
	      _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
			       "#version 300 es required for using uniform "
			       "blocks with an instance name\n");
	   }

	   (yyval.uniform_block) = block;
	;}
    break;

  case 326:
#line 1951 "src/glsl/glsl_parser.yy"
    {
	   (yyval.uniform_block) = new(state) ast_uniform_block(*state->default_uniform_qualifier,
					     NULL,
					     NULL);
	;}
    break;

  case 327:
#line 1957 "src/glsl/glsl_parser.yy"
    {
	   (yyval.uniform_block) = new(state) ast_uniform_block(*state->default_uniform_qualifier,
					     (yyvsp[(1) - (1)].identifier),
					     NULL);
	;}
    break;

  case 328:
#line 1963 "src/glsl/glsl_parser.yy"
    {
	   (yyval.uniform_block) = new(state) ast_uniform_block(*state->default_uniform_qualifier,
					     (yyvsp[(1) - (4)].identifier),
					     (yyvsp[(3) - (4)].expression));
	;}
    break;

  case 329:
#line 1969 "src/glsl/glsl_parser.yy"
    {
	   _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
			    "instance block arrays must be explicitly sized\n");

	   (yyval.uniform_block) = new(state) ast_uniform_block(*state->default_uniform_qualifier,
					     (yyvsp[(1) - (3)].identifier),
					     NULL);
	;}
    break;

  case 330:
#line 1981 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
	   (yyvsp[(1) - (1)].declarator_list)->link.self_link();
	;}
    break;

  case 331:
#line 1986 "src/glsl/glsl_parser.yy"
    {
	   (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
	   (yyvsp[(2) - (2)].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
	;}
    break;

  case 334:
#line 2000 "src/glsl/glsl_parser.yy"
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
	;}
    break;

  case 335:
#line 2015 "src/glsl/glsl_parser.yy"
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
	;}
    break;

  case 336:
#line 2032 "src/glsl/glsl_parser.yy"
    {
	   if (!state->default_uniform_qualifier->merge_qualifier(& (yylsp[(1) - (3)]), state,
								  (yyvsp[(1) - (3)].type_qualifier))) {
	      YYERROR;
	   }
	;}
    break;


/* Line 1267 of yacc.c.  */
#line 5376 "src/glsl/glsl_parser.cpp"
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



