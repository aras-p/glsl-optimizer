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
     SAMPLE = 293,
     NOPERSPECTIVE = 294,
     FLAT = 295,
     SMOOTH = 296,
     MAT2X2 = 297,
     MAT2X3 = 298,
     MAT2X4 = 299,
     MAT3X2 = 300,
     MAT3X3 = 301,
     MAT3X4 = 302,
     MAT4X2 = 303,
     MAT4X3 = 304,
     MAT4X4 = 305,
     SAMPLER1D = 306,
     SAMPLER2D = 307,
     SAMPLER3D = 308,
     SAMPLERCUBE = 309,
     SAMPLER1DSHADOW = 310,
     SAMPLER2DSHADOW = 311,
     SAMPLERCUBESHADOW = 312,
     SAMPLER1DARRAY = 313,
     SAMPLER2DARRAY = 314,
     SAMPLER1DARRAYSHADOW = 315,
     SAMPLER2DARRAYSHADOW = 316,
     SAMPLERCUBEARRAY = 317,
     SAMPLERCUBEARRAYSHADOW = 318,
     ISAMPLER1D = 319,
     ISAMPLER2D = 320,
     ISAMPLER3D = 321,
     ISAMPLERCUBE = 322,
     ISAMPLER1DARRAY = 323,
     ISAMPLER2DARRAY = 324,
     ISAMPLERCUBEARRAY = 325,
     USAMPLER1D = 326,
     USAMPLER2D = 327,
     USAMPLER3D = 328,
     USAMPLERCUBE = 329,
     USAMPLER1DARRAY = 330,
     USAMPLER2DARRAY = 331,
     USAMPLERCUBEARRAY = 332,
     SAMPLER2DRECT = 333,
     ISAMPLER2DRECT = 334,
     USAMPLER2DRECT = 335,
     SAMPLER2DRECTSHADOW = 336,
     SAMPLERBUFFER = 337,
     ISAMPLERBUFFER = 338,
     USAMPLERBUFFER = 339,
     SAMPLER2DMS = 340,
     ISAMPLER2DMS = 341,
     USAMPLER2DMS = 342,
     SAMPLER2DMSARRAY = 343,
     ISAMPLER2DMSARRAY = 344,
     USAMPLER2DMSARRAY = 345,
     SAMPLEREXTERNALOES = 346,
     IMAGE1D = 347,
     IMAGE2D = 348,
     IMAGE3D = 349,
     IMAGE2DRECT = 350,
     IMAGECUBE = 351,
     IMAGEBUFFER = 352,
     IMAGE1DARRAY = 353,
     IMAGE2DARRAY = 354,
     IMAGECUBEARRAY = 355,
     IMAGE2DMS = 356,
     IMAGE2DMSARRAY = 357,
     IIMAGE1D = 358,
     IIMAGE2D = 359,
     IIMAGE3D = 360,
     IIMAGE2DRECT = 361,
     IIMAGECUBE = 362,
     IIMAGEBUFFER = 363,
     IIMAGE1DARRAY = 364,
     IIMAGE2DARRAY = 365,
     IIMAGECUBEARRAY = 366,
     IIMAGE2DMS = 367,
     IIMAGE2DMSARRAY = 368,
     UIMAGE1D = 369,
     UIMAGE2D = 370,
     UIMAGE3D = 371,
     UIMAGE2DRECT = 372,
     UIMAGECUBE = 373,
     UIMAGEBUFFER = 374,
     UIMAGE1DARRAY = 375,
     UIMAGE2DARRAY = 376,
     UIMAGECUBEARRAY = 377,
     UIMAGE2DMS = 378,
     UIMAGE2DMSARRAY = 379,
     IMAGE1DSHADOW = 380,
     IMAGE2DSHADOW = 381,
     IMAGE1DARRAYSHADOW = 382,
     IMAGE2DARRAYSHADOW = 383,
     COHERENT = 384,
     VOLATILE = 385,
     RESTRICT = 386,
     READONLY = 387,
     WRITEONLY = 388,
     ATOMIC_UINT = 389,
     STRUCT = 390,
     VOID_TOK = 391,
     WHILE = 392,
     IDENTIFIER = 393,
     TYPE_IDENTIFIER = 394,
     NEW_IDENTIFIER = 395,
     FLOATCONSTANT = 396,
     INTCONSTANT = 397,
     UINTCONSTANT = 398,
     BOOLCONSTANT = 399,
     FIELD_SELECTION = 400,
     LEFT_OP = 401,
     RIGHT_OP = 402,
     INC_OP = 403,
     DEC_OP = 404,
     LE_OP = 405,
     GE_OP = 406,
     EQ_OP = 407,
     NE_OP = 408,
     AND_OP = 409,
     OR_OP = 410,
     XOR_OP = 411,
     MUL_ASSIGN = 412,
     DIV_ASSIGN = 413,
     ADD_ASSIGN = 414,
     MOD_ASSIGN = 415,
     LEFT_ASSIGN = 416,
     RIGHT_ASSIGN = 417,
     AND_ASSIGN = 418,
     XOR_ASSIGN = 419,
     OR_ASSIGN = 420,
     SUB_ASSIGN = 421,
     INVARIANT = 422,
     PRECISE = 423,
     LOWP = 424,
     MEDIUMP = 425,
     HIGHP = 426,
     SUPERP = 427,
     PRECISION = 428,
     VERSION_TOK = 429,
     EXTENSION = 430,
     LINE = 431,
     COLON = 432,
     EOL = 433,
     INTERFACE = 434,
     OUTPUT = 435,
     PRAGMA_DEBUG_ON = 436,
     PRAGMA_DEBUG_OFF = 437,
     PRAGMA_OPTIMIZE_ON = 438,
     PRAGMA_OPTIMIZE_OFF = 439,
     PRAGMA_INVARIANT_ALL = 440,
     LAYOUT_TOK = 441,
     ASM = 442,
     CLASS = 443,
     UNION = 444,
     ENUM = 445,
     TYPEDEF = 446,
     TEMPLATE = 447,
     THIS = 448,
     PACKED_TOK = 449,
     GOTO = 450,
     INLINE_TOK = 451,
     NOINLINE = 452,
     PUBLIC_TOK = 453,
     STATIC = 454,
     EXTERN = 455,
     EXTERNAL = 456,
     LONG_TOK = 457,
     SHORT_TOK = 458,
     DOUBLE_TOK = 459,
     HALF = 460,
     FIXED_TOK = 461,
     UNSIGNED = 462,
     INPUT_TOK = 463,
     HVEC2 = 464,
     HVEC3 = 465,
     HVEC4 = 466,
     DVEC2 = 467,
     DVEC3 = 468,
     DVEC4 = 469,
     FVEC2 = 470,
     FVEC3 = 471,
     FVEC4 = 472,
     SAMPLER3DRECT = 473,
     SIZEOF = 474,
     CAST = 475,
     NAMESPACE = 476,
     USING = 477,
     RESOURCE = 478,
     PATCH = 479,
     SUBROUTINE = 480,
     ERROR_TOK = 481,
     COMMON = 482,
     PARTITION = 483,
     ACTIVE = 484,
     FILTER = 485,
     ROW_MAJOR = 486,
     THEN = 487
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
#define SAMPLE 293
#define NOPERSPECTIVE 294
#define FLAT 295
#define SMOOTH 296
#define MAT2X2 297
#define MAT2X3 298
#define MAT2X4 299
#define MAT3X2 300
#define MAT3X3 301
#define MAT3X4 302
#define MAT4X2 303
#define MAT4X3 304
#define MAT4X4 305
#define SAMPLER1D 306
#define SAMPLER2D 307
#define SAMPLER3D 308
#define SAMPLERCUBE 309
#define SAMPLER1DSHADOW 310
#define SAMPLER2DSHADOW 311
#define SAMPLERCUBESHADOW 312
#define SAMPLER1DARRAY 313
#define SAMPLER2DARRAY 314
#define SAMPLER1DARRAYSHADOW 315
#define SAMPLER2DARRAYSHADOW 316
#define SAMPLERCUBEARRAY 317
#define SAMPLERCUBEARRAYSHADOW 318
#define ISAMPLER1D 319
#define ISAMPLER2D 320
#define ISAMPLER3D 321
#define ISAMPLERCUBE 322
#define ISAMPLER1DARRAY 323
#define ISAMPLER2DARRAY 324
#define ISAMPLERCUBEARRAY 325
#define USAMPLER1D 326
#define USAMPLER2D 327
#define USAMPLER3D 328
#define USAMPLERCUBE 329
#define USAMPLER1DARRAY 330
#define USAMPLER2DARRAY 331
#define USAMPLERCUBEARRAY 332
#define SAMPLER2DRECT 333
#define ISAMPLER2DRECT 334
#define USAMPLER2DRECT 335
#define SAMPLER2DRECTSHADOW 336
#define SAMPLERBUFFER 337
#define ISAMPLERBUFFER 338
#define USAMPLERBUFFER 339
#define SAMPLER2DMS 340
#define ISAMPLER2DMS 341
#define USAMPLER2DMS 342
#define SAMPLER2DMSARRAY 343
#define ISAMPLER2DMSARRAY 344
#define USAMPLER2DMSARRAY 345
#define SAMPLEREXTERNALOES 346
#define IMAGE1D 347
#define IMAGE2D 348
#define IMAGE3D 349
#define IMAGE2DRECT 350
#define IMAGECUBE 351
#define IMAGEBUFFER 352
#define IMAGE1DARRAY 353
#define IMAGE2DARRAY 354
#define IMAGECUBEARRAY 355
#define IMAGE2DMS 356
#define IMAGE2DMSARRAY 357
#define IIMAGE1D 358
#define IIMAGE2D 359
#define IIMAGE3D 360
#define IIMAGE2DRECT 361
#define IIMAGECUBE 362
#define IIMAGEBUFFER 363
#define IIMAGE1DARRAY 364
#define IIMAGE2DARRAY 365
#define IIMAGECUBEARRAY 366
#define IIMAGE2DMS 367
#define IIMAGE2DMSARRAY 368
#define UIMAGE1D 369
#define UIMAGE2D 370
#define UIMAGE3D 371
#define UIMAGE2DRECT 372
#define UIMAGECUBE 373
#define UIMAGEBUFFER 374
#define UIMAGE1DARRAY 375
#define UIMAGE2DARRAY 376
#define UIMAGECUBEARRAY 377
#define UIMAGE2DMS 378
#define UIMAGE2DMSARRAY 379
#define IMAGE1DSHADOW 380
#define IMAGE2DSHADOW 381
#define IMAGE1DARRAYSHADOW 382
#define IMAGE2DARRAYSHADOW 383
#define COHERENT 384
#define VOLATILE 385
#define RESTRICT 386
#define READONLY 387
#define WRITEONLY 388
#define ATOMIC_UINT 389
#define STRUCT 390
#define VOID_TOK 391
#define WHILE 392
#define IDENTIFIER 393
#define TYPE_IDENTIFIER 394
#define NEW_IDENTIFIER 395
#define FLOATCONSTANT 396
#define INTCONSTANT 397
#define UINTCONSTANT 398
#define BOOLCONSTANT 399
#define FIELD_SELECTION 400
#define LEFT_OP 401
#define RIGHT_OP 402
#define INC_OP 403
#define DEC_OP 404
#define LE_OP 405
#define GE_OP 406
#define EQ_OP 407
#define NE_OP 408
#define AND_OP 409
#define OR_OP 410
#define XOR_OP 411
#define MUL_ASSIGN 412
#define DIV_ASSIGN 413
#define ADD_ASSIGN 414
#define MOD_ASSIGN 415
#define LEFT_ASSIGN 416
#define RIGHT_ASSIGN 417
#define AND_ASSIGN 418
#define XOR_ASSIGN 419
#define OR_ASSIGN 420
#define SUB_ASSIGN 421
#define INVARIANT 422
#define PRECISE 423
#define LOWP 424
#define MEDIUMP 425
#define HIGHP 426
#define SUPERP 427
#define PRECISION 428
#define VERSION_TOK 429
#define EXTENSION 430
#define LINE 431
#define COLON 432
#define EOL 433
#define INTERFACE 434
#define OUTPUT 435
#define PRAGMA_DEBUG_ON 436
#define PRAGMA_DEBUG_OFF 437
#define PRAGMA_OPTIMIZE_ON 438
#define PRAGMA_OPTIMIZE_OFF 439
#define PRAGMA_INVARIANT_ALL 440
#define LAYOUT_TOK 441
#define ASM 442
#define CLASS 443
#define UNION 444
#define ENUM 445
#define TYPEDEF 446
#define TEMPLATE 447
#define THIS 448
#define PACKED_TOK 449
#define GOTO 450
#define INLINE_TOK 451
#define NOINLINE 452
#define PUBLIC_TOK 453
#define STATIC 454
#define EXTERN 455
#define EXTERNAL 456
#define LONG_TOK 457
#define SHORT_TOK 458
#define DOUBLE_TOK 459
#define HALF 460
#define FIXED_TOK 461
#define UNSIGNED 462
#define INPUT_TOK 463
#define HVEC2 464
#define HVEC3 465
#define HVEC4 466
#define DVEC2 467
#define DVEC3 468
#define DVEC4 469
#define FVEC2 470
#define FVEC3 471
#define FVEC4 472
#define SAMPLER3DRECT 473
#define SIZEOF 474
#define CAST 475
#define NAMESPACE 476
#define USING 477
#define RESOURCE 478
#define PATCH 479
#define SUBROUTINE 480
#define ERROR_TOK 481
#define COMMON 482
#define PARTITION 483
#define ACTIVE 484
#define FILTER 485
#define ROW_MAJOR 486
#define THEN 487




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
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <assert.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "main/context.h"

#if defined(_MSC_VER)
#	pragma warning(disable: 4065) // warning C4065: switch statement contains 'default' but no 'case' labels
#	pragma warning(disable: 4244) // warning C4244: '=' : conversion from 'double' to 'float', possible loss of data
#endif // defined(_MSC_VER)

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
#line 99 "src/glsl/glsl_parser.yy"
{
   int n;
   float real;
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
#line 686 "src/glsl/glsl_parser.cpp"
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
#line 711 "src/glsl/glsl_parser.cpp"

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
#define YYLAST   5440

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  257
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  108
/* YYNRULES -- Number of rules.  */
#define YYNRULES  386
/* YYNRULES -- Number of states.  */
#define YYNSTATES  540

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   487

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   241,     2,     2,     2,   245,   248,     2,
     233,   234,   243,   239,   238,   240,   237,   244,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   252,   254,
     246,   253,   247,   251,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   235,     2,   236,   249,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   255,   250,   256,   242,     2,     2,     2,
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
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    14,    19,    22,    25,
      28,    31,    34,    35,    38,    40,    42,    44,    50,    52,
      55,    58,    60,    62,    64,    66,    68,    70,    72,    76,
      78,    83,    85,    89,    92,    95,    97,    99,   101,   105,
     108,   111,   114,   116,   119,   123,   126,   128,   130,   132,
     135,   138,   141,   143,   146,   150,   153,   155,   158,   161,
     164,   166,   168,   170,   172,   174,   178,   182,   186,   188,
     192,   196,   198,   202,   206,   208,   212,   216,   220,   224,
     226,   230,   234,   236,   240,   242,   246,   248,   252,   254,
     258,   260,   264,   266,   270,   272,   278,   280,   284,   286,
     288,   290,   292,   294,   296,   298,   300,   302,   304,   306,
     308,   312,   314,   317,   320,   325,   327,   330,   332,   334,
     337,   341,   345,   348,   352,   355,   358,   359,   362,   365,
     368,   371,   373,   375,   377,   379,   381,   385,   390,   397,
     403,   405,   408,   412,   418,   423,   426,   429,   431,   434,
     439,   441,   445,   447,   449,   451,   455,   457,   459,   461,
     463,   465,   467,   469,   471,   473,   475,   477,   479,   481,
     484,   487,   490,   493,   496,   499,   502,   504,   506,   508,
     510,   512,   514,   516,   518,   520,   522,   524,   526,   528,
     530,   533,   537,   541,   546,   548,   551,   553,   555,   557,
     559,   561,   563,   565,   567,   569,   571,   573,   575,   577,
     579,   581,   583,   585,   587,   589,   591,   593,   595,   597,
     599,   601,   603,   605,   607,   609,   611,   613,   615,   617,
     619,   621,   623,   625,   627,   629,   631,   633,   635,   637,
     639,   641,   643,   645,   647,   649,   651,   653,   655,   657,
     659,   661,   663,   665,   667,   669,   671,   673,   675,   677,
     679,   681,   683,   685,   687,   689,   691,   693,   695,   697,
     699,   701,   703,   705,   707,   709,   711,   713,   715,   717,
     719,   721,   723,   725,   727,   729,   731,   733,   735,   737,
     739,   741,   743,   745,   747,   749,   751,   753,   755,   757,
     759,   761,   763,   765,   771,   776,   778,   781,   785,   787,
     791,   793,   796,   798,   802,   807,   809,   813,   815,   817,
     819,   821,   823,   825,   827,   829,   831,   834,   835,   840,
     842,   844,   847,   851,   853,   856,   858,   861,   867,   871,
     873,   875,   880,   886,   889,   893,   897,   900,   902,   905,
     908,   911,   913,   916,   922,   930,   937,   939,   941,   943,
     944,   947,   951,   954,   957,   960,   964,   967,   969,   971,
     973,   975,   978,   980,   983,   991,   993,   995,   997,   998,
    1000,  1003,  1005,  1008,  1012,  1016,  1020
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     258,     0,    -1,    -1,   260,   262,   259,   265,    -1,    -1,
     174,   142,   178,    -1,   174,   142,   263,   178,    -1,   181,
     178,    -1,   182,   178,    -1,   183,   178,    -1,   184,   178,
      -1,   185,   178,    -1,    -1,   262,   264,    -1,   138,    -1,
     139,    -1,   140,    -1,   175,   263,   177,   263,   178,    -1,
     356,    -1,   265,   356,    -1,   265,   264,    -1,   138,    -1,
     140,    -1,   266,    -1,   142,    -1,   143,    -1,   141,    -1,
     144,    -1,   233,   297,   234,    -1,   267,    -1,   268,   235,
     269,   236,    -1,   270,    -1,   268,   237,   263,    -1,   268,
     148,    -1,   268,   149,    -1,   297,    -1,   271,    -1,   272,
      -1,   268,   237,   277,    -1,   274,   234,    -1,   273,   234,
      -1,   275,   136,    -1,   275,    -1,   275,   295,    -1,   274,
     238,   295,    -1,   276,   233,    -1,   322,    -1,   266,    -1,
     145,    -1,   279,   234,    -1,   278,   234,    -1,   280,   136,
      -1,   280,    -1,   280,   295,    -1,   279,   238,   295,    -1,
     266,   233,    -1,   268,    -1,   148,   281,    -1,   149,   281,
      -1,   282,   281,    -1,   239,    -1,   240,    -1,   241,    -1,
     242,    -1,   281,    -1,   283,   243,   281,    -1,   283,   244,
     281,    -1,   283,   245,   281,    -1,   283,    -1,   284,   239,
     283,    -1,   284,   240,   283,    -1,   284,    -1,   285,   146,
     284,    -1,   285,   147,   284,    -1,   285,    -1,   286,   246,
     285,    -1,   286,   247,   285,    -1,   286,   150,   285,    -1,
     286,   151,   285,    -1,   286,    -1,   287,   152,   286,    -1,
     287,   153,   286,    -1,   287,    -1,   288,   248,   287,    -1,
     288,    -1,   289,   249,   288,    -1,   289,    -1,   290,   250,
     289,    -1,   290,    -1,   291,   154,   290,    -1,   291,    -1,
     292,   156,   291,    -1,   292,    -1,   293,   155,   292,    -1,
     293,    -1,   293,   251,   297,   252,   295,    -1,   294,    -1,
     281,   296,   295,    -1,   253,    -1,   157,    -1,   158,    -1,
     160,    -1,   159,    -1,   166,    -1,   161,    -1,   162,    -1,
     163,    -1,   164,    -1,   165,    -1,   295,    -1,   297,   238,
     295,    -1,   294,    -1,   300,   254,    -1,   309,   254,    -1,
     173,   325,   322,   254,    -1,   358,    -1,   301,   234,    -1,
     303,    -1,   302,    -1,   303,   305,    -1,   302,   238,   305,
      -1,   311,   266,   233,    -1,   322,   263,    -1,   322,   263,
     321,    -1,   306,   304,    -1,   306,   308,    -1,    -1,     4,
     306,    -1,   168,   306,    -1,   307,   306,    -1,   325,   306,
      -1,    33,    -1,    34,    -1,    35,    -1,   322,    -1,   310,
      -1,   309,   238,   263,    -1,   309,   238,   263,   321,    -1,
     309,   238,   263,   321,   253,   331,    -1,   309,   238,   263,
     253,   331,    -1,   311,    -1,   311,   263,    -1,   311,   263,
     321,    -1,   311,   263,   321,   253,   331,    -1,   311,   263,
     253,   331,    -1,   167,   266,    -1,   168,   266,    -1,   322,
      -1,   318,   322,    -1,   186,   233,   313,   234,    -1,   315,
      -1,   313,   238,   315,    -1,   142,    -1,   143,    -1,   263,
      -1,   263,   253,   314,    -1,   316,    -1,   231,    -1,   194,
      -1,    41,    -1,    40,    -1,    39,    -1,   167,    -1,   168,
      -1,   319,    -1,   320,    -1,   317,    -1,   312,    -1,   325,
      -1,   168,   318,    -1,   167,   318,    -1,   317,   318,    -1,
     312,   318,    -1,   319,   318,    -1,   320,   318,    -1,   325,
     318,    -1,    32,    -1,    38,    -1,     4,    -1,     3,    -1,
      37,    -1,    33,    -1,    34,    -1,    35,    -1,    36,    -1,
     129,    -1,   130,    -1,   131,    -1,   132,    -1,   133,    -1,
     235,   236,    -1,   235,   298,   236,    -1,   321,   235,   236,
      -1,   321,   235,   298,   236,    -1,   323,    -1,   323,   321,
      -1,   324,    -1,   326,    -1,   139,    -1,   136,    -1,     6,
      -1,     7,    -1,     8,    -1,     5,    -1,    29,    -1,    30,
      -1,    31,    -1,    20,    -1,    21,    -1,    22,    -1,    23,
      -1,    24,    -1,    25,    -1,    26,    -1,    27,    -1,    28,
      -1,    42,    -1,    43,    -1,    44,    -1,    45,    -1,    46,
      -1,    47,    -1,    48,    -1,    49,    -1,    50,    -1,    51,
      -1,    52,    -1,    78,    -1,    53,    -1,    54,    -1,    91,
      -1,    55,    -1,    56,    -1,    81,    -1,    57,    -1,    58,
      -1,    59,    -1,    60,    -1,    61,    -1,    82,    -1,    62,
      -1,    63,    -1,    64,    -1,    65,    -1,    79,    -1,    66,
      -1,    67,    -1,    68,    -1,    69,    -1,    83,    -1,    70,
      -1,    71,    -1,    72,    -1,    80,    -1,    73,    -1,    74,
      -1,    75,    -1,    76,    -1,    84,    -1,    77,    -1,    85,
      -1,    86,    -1,    87,    -1,    88,    -1,    89,    -1,    90,
      -1,    92,    -1,    93,    -1,    94,    -1,    95,    -1,    96,
      -1,    97,    -1,    98,    -1,    99,    -1,   100,    -1,   101,
      -1,   102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,
      -1,   107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,
      -1,   112,    -1,   113,    -1,   114,    -1,   115,    -1,   116,
      -1,   117,    -1,   118,    -1,   119,    -1,   120,    -1,   121,
      -1,   122,    -1,   123,    -1,   124,    -1,   134,    -1,   171,
      -1,   170,    -1,   169,    -1,   135,   263,   255,   327,   256,
      -1,   135,   255,   327,   256,    -1,   328,    -1,   327,   328,
      -1,   311,   329,   254,    -1,   330,    -1,   329,   238,   330,
      -1,   263,    -1,   263,   321,    -1,   295,    -1,   255,   332,
     256,    -1,   255,   332,   238,   256,    -1,   331,    -1,   332,
     238,   331,    -1,   299,    -1,   336,    -1,   335,    -1,   333,
      -1,   341,    -1,   342,    -1,   345,    -1,   351,    -1,   355,
      -1,   255,   256,    -1,    -1,   255,   337,   340,   256,    -1,
     339,    -1,   335,    -1,   255,   256,    -1,   255,   340,   256,
      -1,   334,    -1,   340,   334,    -1,   254,    -1,   297,   254,
      -1,    14,   233,   297,   234,   343,    -1,   334,    12,   334,
      -1,   334,    -1,   297,    -1,   311,   263,   253,   331,    -1,
      17,   233,   297,   234,   346,    -1,   255,   256,    -1,   255,
     350,   256,    -1,    18,   297,   252,    -1,    19,   252,    -1,
     347,    -1,   348,   347,    -1,   348,   334,    -1,   349,   334,
      -1,   349,    -1,   350,   349,    -1,   137,   233,   344,   234,
     338,    -1,    11,   334,   137,   233,   297,   234,   254,    -1,
      13,   233,   352,   354,   234,   338,    -1,   341,    -1,   333,
      -1,   344,    -1,    -1,   353,   254,    -1,   353,   254,   297,
      -1,    10,   254,    -1,     9,   254,    -1,    16,   254,    -1,
      16,   297,   254,    -1,    15,   254,    -1,   357,    -1,   299,
      -1,   261,    -1,   364,    -1,   300,   339,    -1,   359,    -1,
     312,   359,    -1,   360,   140,   255,   362,   256,   361,   254,
      -1,    33,    -1,    34,    -1,    36,    -1,    -1,   140,    -1,
     140,   321,    -1,   363,    -1,   363,   362,    -1,   311,   329,
     254,    -1,   312,    36,   254,    -1,   312,    33,   254,    -1,
     312,    34,   254,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   303,   303,   302,   314,   316,   323,   333,   334,   335,
     336,   337,   350,   352,   356,   357,   358,   362,   371,   379,
     387,   398,   399,   403,   410,   417,   424,   431,   438,   445,
     446,   452,   456,   463,   469,   478,   482,   486,   487,   496,
     497,   501,   502,   506,   512,   524,   528,   534,   542,   553,
     554,   558,   559,   563,   569,   581,   593,   594,   600,   606,
     616,   617,   618,   619,   623,   624,   630,   636,   645,   646,
     652,   661,   662,   668,   677,   678,   684,   690,   696,   705,
     706,   712,   721,   722,   731,   732,   741,   742,   751,   752,
     761,   762,   771,   772,   781,   782,   791,   792,   801,   802,
     803,   804,   805,   806,   807,   808,   809,   810,   811,   815,
     819,   835,   839,   844,   848,   853,   860,   864,   865,   869,
     874,   882,   896,   906,   920,   925,   939,   943,   951,   959,
     971,   984,   990,   996,  1006,  1011,  1012,  1022,  1032,  1042,
    1056,  1063,  1073,  1083,  1093,  1103,  1115,  1130,  1137,  1148,
    1155,  1156,  1166,  1167,  1171,  1357,  1497,  1523,  1529,  1538,
    1544,  1550,  1560,  1566,  1571,  1572,  1573,  1574,  1575,  1594,
    1602,  1614,  1638,  1655,  1671,  1691,  1705,  1711,  1719,  1725,
    1731,  1737,  1743,  1749,  1770,  1776,  1781,  1786,  1792,  1797,
    1805,  1811,  1817,  1831,  1846,  1847,  1855,  1861,  1867,  1876,
    1877,  1878,  1879,  1880,  1881,  1882,  1883,  1884,  1885,  1886,
    1887,  1888,  1889,  1890,  1891,  1892,  1893,  1894,  1895,  1896,
    1897,  1898,  1899,  1900,  1901,  1902,  1903,  1904,  1905,  1906,
    1907,  1908,  1909,  1910,  1911,  1912,  1913,  1914,  1915,  1916,
    1917,  1918,  1919,  1920,  1921,  1922,  1923,  1924,  1925,  1926,
    1927,  1928,  1929,  1930,  1931,  1932,  1933,  1934,  1935,  1936,
    1937,  1938,  1939,  1940,  1941,  1942,  1943,  1944,  1945,  1946,
    1947,  1948,  1949,  1950,  1951,  1952,  1953,  1954,  1955,  1956,
    1957,  1958,  1959,  1960,  1961,  1962,  1963,  1964,  1965,  1966,
    1967,  1968,  1969,  1970,  1971,  1972,  1973,  1974,  1975,  1976,
    1980,  1985,  1990,  1998,  2005,  2014,  2019,  2027,  2046,  2051,
    2059,  2065,  2074,  2075,  2079,  2086,  2093,  2100,  2106,  2107,
    2111,  2112,  2113,  2114,  2115,  2116,  2120,  2127,  2126,  2140,
    2141,  2145,  2151,  2160,  2170,  2182,  2188,  2197,  2206,  2211,
    2219,  2223,  2241,  2249,  2254,  2262,  2267,  2275,  2283,  2291,
    2299,  2307,  2315,  2323,  2330,  2337,  2347,  2348,  2352,  2354,
    2360,  2365,  2374,  2380,  2386,  2392,  2398,  2407,  2408,  2409,
    2410,  2414,  2428,  2432,  2455,  2560,  2566,  2572,  2582,  2586,
    2592,  2601,  2606,  2614,  2638,  2646,  2654
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
  "UNIFORM", "VARYING", "SAMPLE", "NOPERSPECTIVE", "FLAT", "SMOOTH",
  "MAT2X2", "MAT2X3", "MAT2X4", "MAT3X2", "MAT3X3", "MAT3X4", "MAT4X2",
  "MAT4X3", "MAT4X4", "SAMPLER1D", "SAMPLER2D", "SAMPLER3D", "SAMPLERCUBE",
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
  "USAMPLER2DMSARRAY", "SAMPLEREXTERNALOES", "IMAGE1D", "IMAGE2D",
  "IMAGE3D", "IMAGE2DRECT", "IMAGECUBE", "IMAGEBUFFER", "IMAGE1DARRAY",
  "IMAGE2DARRAY", "IMAGECUBEARRAY", "IMAGE2DMS", "IMAGE2DMSARRAY",
  "IIMAGE1D", "IIMAGE2D", "IIMAGE3D", "IIMAGE2DRECT", "IIMAGECUBE",
  "IIMAGEBUFFER", "IIMAGE1DARRAY", "IIMAGE2DARRAY", "IIMAGECUBEARRAY",
  "IIMAGE2DMS", "IIMAGE2DMSARRAY", "UIMAGE1D", "UIMAGE2D", "UIMAGE3D",
  "UIMAGE2DRECT", "UIMAGECUBE", "UIMAGEBUFFER", "UIMAGE1DARRAY",
  "UIMAGE2DARRAY", "UIMAGECUBEARRAY", "UIMAGE2DMS", "UIMAGE2DMSARRAY",
  "IMAGE1DSHADOW", "IMAGE2DSHADOW", "IMAGE1DARRAYSHADOW",
  "IMAGE2DARRAYSHADOW", "COHERENT", "VOLATILE", "RESTRICT", "READONLY",
  "WRITEONLY", "ATOMIC_UINT", "STRUCT", "VOID_TOK", "WHILE", "IDENTIFIER",
  "TYPE_IDENTIFIER", "NEW_IDENTIFIER", "FLOATCONSTANT", "INTCONSTANT",
  "UINTCONSTANT", "BOOLCONSTANT", "FIELD_SELECTION", "LEFT_OP", "RIGHT_OP",
  "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP",
  "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN", "ADD_ASSIGN",
  "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN",
  "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT", "PRECISE", "LOWP", "MEDIUMP",
  "HIGHP", "SUPERP", "PRECISION", "VERSION_TOK", "EXTENSION", "LINE",
  "COLON", "EOL", "INTERFACE", "OUTPUT", "PRAGMA_DEBUG_ON",
  "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON", "PRAGMA_OPTIMIZE_OFF",
  "PRAGMA_INVARIANT_ALL", "LAYOUT_TOK", "ASM", "CLASS", "UNION", "ENUM",
  "TYPEDEF", "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK",
  "NOINLINE", "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL", "LONG_TOK",
  "SHORT_TOK", "DOUBLE_TOK", "HALF", "FIXED_TOK", "UNSIGNED", "INPUT_TOK",
  "HVEC2", "HVEC3", "HVEC4", "DVEC2", "DVEC3", "DVEC4", "FVEC2", "FVEC3",
  "FVEC4", "SAMPLER3DRECT", "SIZEOF", "CAST", "NAMESPACE", "USING",
  "RESOURCE", "PATCH", "SUBROUTINE", "ERROR_TOK", "COMMON", "PARTITION",
  "ACTIVE", "FILTER", "ROW_MAJOR", "THEN", "'('", "')'", "'['", "']'",
  "'.'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'", "'%'", "'<'",
  "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'", "'{'", "'}'",
  "$accept", "translation_unit", "@1", "version_statement",
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
  "parameter_qualifier", "parameter_direction_qualifier",
  "parameter_type_specifier", "init_declarator_list", "single_declaration",
  "fully_specified_type", "layout_qualifier", "layout_qualifier_id_list",
  "integer_constant", "layout_qualifier_id",
  "interface_block_layout_qualifier", "interpolation_qualifier",
  "type_qualifier", "auxiliary_storage_qualifier", "storage_qualifier",
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
  "external_declaration", "function_definition", "interface_block",
  "basic_interface_block", "interface_qualifier", "instance_name_opt",
  "member_list", "member_declaration", "layout_defaults", 0
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
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,    40,    41,    91,    93,    46,    44,    43,
      45,    33,   126,    42,    47,    37,    60,    62,    38,    94,
     124,    63,    58,    61,    59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   257,   259,   258,   260,   260,   260,   261,   261,   261,
     261,   261,   262,   262,   263,   263,   263,   264,   265,   265,
     265,   266,   266,   267,   267,   267,   267,   267,   267,   268,
     268,   268,   268,   268,   268,   269,   270,   271,   271,   272,
     272,   273,   273,   274,   274,   275,   276,   276,   276,   277,
     277,   278,   278,   279,   279,   280,   281,   281,   281,   281,
     282,   282,   282,   282,   283,   283,   283,   283,   284,   284,
     284,   285,   285,   285,   286,   286,   286,   286,   286,   287,
     287,   287,   288,   288,   289,   289,   290,   290,   291,   291,
     292,   292,   293,   293,   294,   294,   295,   295,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   297,
     297,   298,   299,   299,   299,   299,   300,   301,   301,   302,
     302,   303,   304,   304,   305,   305,   306,   306,   306,   306,
     306,   307,   307,   307,   308,   309,   309,   309,   309,   309,
     310,   310,   310,   310,   310,   310,   310,   311,   311,   312,
     313,   313,   314,   314,   315,   315,   315,   316,   316,   317,
     317,   317,   318,   318,   318,   318,   318,   318,   318,   318,
     318,   318,   318,   318,   318,   318,   319,   319,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     321,   321,   321,   321,   322,   322,   323,   323,   323,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   324,
     325,   325,   325,   326,   326,   327,   327,   328,   329,   329,
     330,   330,   331,   331,   331,   332,   332,   333,   334,   334,
     335,   335,   335,   335,   335,   335,   336,   337,   336,   338,
     338,   339,   339,   340,   340,   341,   341,   342,   343,   343,
     344,   344,   345,   346,   346,   347,   347,   348,   348,   349,
     349,   350,   350,   351,   351,   351,   352,   352,   353,   353,
     354,   354,   355,   355,   355,   355,   355,   356,   356,   356,
     356,   357,   358,   358,   359,   360,   360,   360,   361,   361,
     361,   362,   362,   363,   364,   364,   364
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     4,     2,     2,     2,
       2,     2,     0,     2,     1,     1,     1,     5,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       4,     1,     3,     2,     2,     1,     1,     1,     3,     2,
       2,     2,     1,     2,     3,     2,     1,     1,     1,     2,
       2,     2,     1,     2,     3,     2,     1,     2,     2,     2,
       1,     1,     1,     1,     1,     3,     3,     3,     1,     3,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     5,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     2,     2,     4,     1,     2,     1,     1,     2,
       3,     3,     2,     3,     2,     2,     0,     2,     2,     2,
       2,     1,     1,     1,     1,     1,     3,     4,     6,     5,
       1,     2,     3,     5,     4,     2,     2,     1,     2,     4,
       1,     3,     1,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     3,     3,     4,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     5,     4,     1,     2,     3,     1,     3,
       1,     2,     1,     3,     4,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     0,     4,     1,
       1,     2,     3,     1,     2,     1,     2,     5,     3,     1,
       1,     4,     5,     2,     3,     3,     2,     1,     2,     2,
       2,     1,     2,     5,     7,     6,     1,     1,     1,     0,
       2,     3,     2,     2,     2,     3,     2,     1,     1,     1,
       1,     2,     1,     2,     7,     1,     1,     1,     0,     1,
       2,     1,     2,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       4,     0,     0,    12,     0,     1,     2,    14,    15,    16,
       5,     0,     0,     0,    13,     6,     0,   179,   178,   203,
     200,   201,   202,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   204,   205,   206,   176,   181,   182,   183,   184,
     180,   177,   161,   160,   159,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   228,   229,   231,   232,
     234,   235,   236,   237,   238,   240,   241,   242,   243,   245,
     246,   247,   248,   250,   251,   252,   254,   255,   256,   257,
     259,   227,   244,   253,   233,   239,   249,   258,   260,   261,
     262,   263,   264,   265,   230,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   185,   186,
     187,   188,   189,   299,     0,   199,   198,   162,   163,   302,
     301,   300,     0,     0,     0,     0,     0,     0,     0,   369,
       3,   368,     0,     0,   118,   126,     0,   135,   140,   167,
     166,     0,   164,   165,   147,   194,   196,   168,   197,    18,
     367,   115,   372,     0,   370,     0,     0,     0,   181,   182,
     184,    21,    22,   162,   163,   145,   167,   170,   146,   169,
       0,     7,     8,     9,    10,    11,     0,    20,    19,   112,
       0,   371,   116,   126,   126,   131,   132,   133,   126,   119,
       0,   126,   126,     0,   113,    14,    16,   141,     0,   181,
     182,   184,   172,   373,   171,   148,   173,   174,     0,   195,
     175,     0,     0,     0,     0,   305,     0,     0,   158,   157,
     154,     0,   150,   156,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    24,    25,    27,    48,     0,     0,
       0,    60,    61,    62,    63,   335,   327,   331,    23,    29,
      56,    31,    36,    37,     0,     0,    42,     0,    64,     0,
      68,    71,    74,    79,    82,    84,    86,    88,    90,    92,
      94,    96,   109,     0,   317,     0,   167,   147,   320,   333,
     319,   318,     0,   321,   322,   323,   324,   325,   120,   127,
     128,   124,   125,   134,   129,   130,   136,     0,   142,   121,
     385,   386,   384,   190,    64,   111,     0,    46,     0,     0,
      17,   310,     0,   308,   304,   306,     0,   114,     0,   149,
       0,   363,   362,     0,     0,     0,   366,   364,     0,     0,
       0,    57,    58,     0,   326,     0,    33,    34,     0,     0,
      40,    39,     0,   199,    43,    45,    99,   100,   102,   101,
     104,   105,   106,   107,   108,   103,    98,     0,    59,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     336,   332,   334,   122,     0,   137,     0,   312,   144,     0,
     191,   192,     0,     0,     0,   381,   311,     0,   307,   303,
     152,   153,   155,   151,     0,   357,   356,   359,     0,   365,
       0,   340,     0,     0,    28,     0,     0,    35,    32,     0,
      38,     0,     0,    52,    44,    97,    65,    66,    67,    69,
      70,    72,    73,    77,    78,    75,    76,    80,    81,    83,
      85,    87,    89,    91,    93,     0,   110,   123,   139,     0,
     315,     0,   143,   193,     0,   378,   382,   309,     0,   358,
       0,     0,     0,     0,     0,     0,   328,    30,    55,    50,
      49,     0,   199,    53,     0,   138,     0,   313,   383,   379,
       0,     0,   360,     0,   339,   337,     0,   342,     0,   330,
     353,   329,    54,    95,   314,   316,   380,   374,     0,   361,
     355,     0,     0,     0,   343,   347,     0,   351,     0,   341,
     354,   338,     0,   346,   349,   348,   350,   344,   352,   345
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    13,     3,   149,     6,   331,    14,   150,   268,
     269,   270,   436,   271,   272,   273,   274,   275,   276,   277,
     440,   441,   442,   443,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   377,
     293,   326,   294,   295,   153,   154,   155,   311,   209,   210,
     211,   312,   156,   157,   158,   186,   241,   422,   242,   243,
     160,   161,   162,   163,   229,   327,   165,   166,   167,   168,
     234,   235,   332,   333,   408,   471,   298,   299,   300,   301,
     355,   510,   511,   302,   303,   304,   505,   433,   305,   507,
     525,   526,   527,   528,   306,   427,   480,   481,   307,   169,
     170,   171,   172,   173,   500,   414,   415,   174
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -414
static const yytype_int16 yypact[] =
{
    -129,   -92,    53,  -414,  -103,  -414,   -57,  -414,  -414,  -414,
    -414,   -58,   -54,  4836,  -414,  -414,   -51,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,   -17,   -12,  -414,    13,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -119,  -414,  -414,   266,   266,  -414,
    -414,  -414,    15,   -33,   -14,    -8,    -3,    10,   -37,  -414,
    4652,  -414,  -196,   -26,   -27,    30,  -199,  -414,    54,   314,
    5129,  5301,  5129,  5129,  -414,   -15,  -414,  5129,  -414,  -414,
    -414,  -414,  -414,    82,  -414,   -54,  5007,   -31,  -414,  -414,
    -414,  -414,  -414,  5129,  5129,  -414,  5129,  -414,  -414,  -414,
    5301,  -414,  -414,  -414,  -414,  -414,  -107,  -414,  -414,  -414,
     525,  -414,  -414,    36,    36,  -414,  -414,  -414,    36,  -414,
    5301,    36,    36,   -54,  -414,     3,    11,  -197,    12,  -123,
    -116,  -114,  -414,  -414,  -414,  -414,  -414,  -414,  3625,    -5,
    -414,   -23,    68,   -54,  1273,  -414,  5007,   -16,  -414,  -414,
      -2,  -143,  -414,  -414,     0,     1,  2007,    16,    20,     2,
    3184,    24,    28,  -414,  -414,  -414,  -414,  -414,  4027,  4027,
    4027,  -414,  -414,  -414,  -414,  -414,     6,  -414,    32,  -414,
    -105,  -414,  -414,  -414,    14,  -122,  4228,    34,   222,  4027,
     -18,   -73,    26,  -104,    27,    23,    25,    22,   119,   121,
    -126,  -414,  -414,  -160,  -414,    21,  5149,    46,  -414,  -414,
    -414,  -414,   779,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,   -54,  -414,  -414,  -193,  2961,  -186,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,    47,  -414,  3826,  5007,
    -414,   -15,  -155,  -414,  -414,  -414,  1510,  -414,    60,  -414,
    -107,  -414,  -414,   147,  2500,  4027,  -414,  -414,  -146,  4027,
    3424,  -414,  -414,  -121,  -414,  2007,  -414,  -414,  4027,    54,
    -414,  -414,  4027,    51,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  4027,  -414,  4027,
    4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,
    4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,  4027,
    -414,  -414,  -414,   -15,  2961,  -174,  2961,  -414,  -414,  2961,
    -414,  -414,    50,   -54,    38,  5007,    -5,   -54,  -414,  -414,
    -414,  -414,  -414,  -414,    55,  -414,  -414,  3424,   -88,  -414,
     -87,    52,   -54,    58,  -414,  1033,    59,    52,  -414,    63,
    -414,    74,   -76,  4429,  -414,  -414,  -414,  -414,  -414,   -18,
     -18,   -73,   -73,    26,    26,    26,    26,  -104,  -104,    27,
      23,    25,    22,   119,   121,  -156,  -414,    -5,  -414,  2961,
    -414,  -176,  -414,  -414,  -145,   157,  -414,  -414,  4027,  -414,
      56,    77,  2007,    64,    61,  2260,  -414,  -414,  -414,  -414,
    -414,  4027,    79,  -414,  4027,  -414,  2738,  -414,  -414,   -15,
      66,   -69,  4027,  2260,   303,  -414,    -6,  -414,  2961,  -414,
    -414,  -414,  -414,  -414,  -414,  -414,    -5,  -414,    67,    52,
    -414,  2007,  4027,    70,  -414,  -414,  1754,  2007,    -4,  -414,
    -414,  -414,  -117,  -414,  -414,  -414,  -414,  -414,  2007,  -414
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -414,  -414,  -414,  -414,  -414,  -414,    -1,   174,  -414,  -128,
    -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,  -414,
    -414,  -414,  -414,  -414,   -19,  -414,  -169,  -142,  -170,  -162,
     -67,   -66,   -68,   -65,   -64,   -63,  -414,  -206,  -228,  -414,
    -244,     7,    -9,     5,  -414,  -414,  -414,  -414,   125,   -52,
    -414,  -414,  -414,  -414,  -168,   -11,  -414,  -414,    17,  -414,
    -414,   -86,  -414,  -414,  -212,   -13,  -414,  -414,  -101,  -414,
      93,  -207,   -77,   -79,  -381,  -414,    45,  -245,  -413,  -414,
    -414,  -163,   189,   -10,    48,  -414,  -414,   -84,  -414,  -414,
    -167,  -414,  -172,  -414,  -414,  -414,  -414,  -414,  -414,   240,
    -414,  -414,  -152,  -414,  -414,   -24,  -414,  -414
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -378
static const yytype_int16 yytable[] =
{
     164,   343,   159,    11,   151,   318,   348,   223,   233,   185,
     188,    16,   522,   523,   522,   523,   353,  -375,   152,     7,
       8,     9,   325,   468,  -376,   470,  -377,   335,   472,   397,
     218,     7,     8,     9,   204,     7,     8,     9,   228,   213,
     204,   190,   228,   356,   357,     1,   386,   387,   364,   328,
       4,   187,   189,     5,   212,   214,   317,   402,   199,   200,
     404,   328,   496,   205,   206,   207,   233,   409,   233,   205,
     206,   207,   509,   222,   224,    10,   226,   227,   399,   469,
     497,   230,   399,   417,     7,     8,     9,   238,   495,   407,
     509,   339,   399,   417,   400,   340,   494,   187,   189,   418,
     222,   428,   212,   212,   405,   430,   431,   212,   429,   498,
     212,   212,   361,   434,   437,   515,   362,   399,    12,   416,
      15,   399,   325,  -375,   239,   398,   175,   529,  -376,   335,
     358,   320,   359,   177,   444,   539,   176,   164,   321,   159,
     322,   151,   388,   389,   223,   191,   482,   483,   225,   445,
     399,   399,   309,  -377,   465,   152,   310,   217,   490,   314,
     315,   413,   491,   164,   192,   518,   382,   383,   233,   399,
     193,   466,   384,   385,   232,   194,   407,   237,   407,   390,
     391,   407,   432,   431,   139,   140,   141,   297,   195,   296,
     402,   467,   215,     8,   216,   240,   196,   313,   208,   139,
     140,   141,   420,   421,   208,   139,   140,   141,   202,   324,
     222,   203,   316,   449,   450,   493,   453,   454,   455,   456,
     228,   164,   231,   164,   236,   379,   380,   381,   457,   458,
     328,   439,   329,   297,   501,   296,   -21,   504,   337,   351,
     352,   407,   451,   452,   -22,   319,   330,   413,   360,   344,
     524,   338,   537,   345,   341,   342,   346,   349,   519,   432,
     378,   350,   354,   512,  -117,   -47,   513,   365,   407,    17,
      18,   392,   394,   395,   393,   199,   531,   396,   532,   -46,
     407,   534,   536,   410,   424,   -41,   473,   516,   478,   297,
     399,   296,   485,   536,   475,   487,   488,   499,    35,   178,
     179,    38,   180,    40,    41,    42,    43,    44,   489,   324,
     502,   503,   403,   -51,   508,   521,   164,    17,    18,   506,
     517,   530,   533,   164,   197,   459,   461,   460,   308,   336,
     462,   297,   463,   296,   464,   412,   474,   297,   477,   240,
     520,   201,   297,   479,   296,   435,    35,   219,   220,    38,
     221,    40,    41,    42,    43,    44,   538,   423,   438,   535,
     446,   447,   448,   324,   324,   324,   324,   324,   324,   324,
     324,   324,   324,   324,   324,   324,   324,   324,   324,   366,
     367,   368,   369,   370,   371,   372,   373,   374,   375,   425,
     198,   476,   426,     0,     0,   128,   129,   130,   131,   132,
       0,     0,   164,     0,   181,     0,   182,     0,     0,     0,
       0,     0,     0,     0,   297,     0,     0,     0,     0,     0,
       0,     0,   297,     0,   296,     0,     0,     0,     0,     0,
       0,   484,     0,   183,   184,   139,   140,   141,     0,     0,
       0,     0,     0,   128,   129,   130,   131,   132,     0,     0,
       0,     0,   148,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   297,
       0,   296,   297,     0,   296,   376,     0,     0,     0,     0,
       0,   183,   184,   139,   140,   141,     0,     0,     0,     0,
     297,     0,   296,     0,     0,     0,     0,     0,     0,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   297,     0,
     296,     0,     0,   297,   297,   296,   296,     0,     0,     0,
       0,     0,     0,     0,     0,   297,     0,   296,    17,    18,
      19,    20,    21,    22,   244,   245,   246,     0,   247,   248,
     249,   250,   251,     0,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
       0,     0,     0,     0,   128,   129,   130,   131,   132,   133,
     134,   135,   252,   181,   136,   182,   253,   254,   255,   256,
     257,     0,     0,   258,   259,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   137,   138,   139,   140,   141,     0,   142,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   148,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   260,     0,
       0,     0,     0,     0,   261,   262,   263,   264,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   265,
     266,   267,    17,    18,    19,    20,    21,    22,   244,   245,
     246,     0,   247,   248,   249,   250,   251,     0,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,     0,     0,     0,     0,   128,   129,
     130,   131,   132,   133,   134,   135,   252,   181,   136,   182,
     253,   254,   255,   256,   257,     0,     0,   258,   259,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   137,   138,   139,   140,
     141,     0,   142,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   148,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   260,     0,     0,     0,     0,     0,   261,   262,
     263,   264,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   265,   266,   401,    17,    18,    19,    20,
      21,    22,   244,   245,   246,     0,   247,   248,   249,   250,
     251,     0,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,     0,     0,
       0,     0,   128,   129,   130,   131,   132,   133,   134,   135,
     252,   181,   136,   182,   253,   254,   255,   256,   257,     0,
       0,   258,   259,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     137,   138,   139,   140,   141,     0,   142,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   148,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   260,     0,     0,     0,
       0,     0,   261,   262,   263,   264,    17,    18,    19,    20,
      21,    22,     0,     0,     0,     0,     0,   265,   266,   486,
       0,     0,     0,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,   178,   179,    38,   180,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,     0,     0,
       0,     0,   128,   129,   130,   131,   132,   133,   134,   135,
       0,     0,   136,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     183,   184,   139,   140,   141,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   148,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   334,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,   178,   179,    38,   180,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,     0,     0,   136,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   183,   184,   139,
     140,   141,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   148,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    17,    18,    19,
      20,    21,    22,   244,   245,   246,   419,   247,   248,   249,
     250,   251,   522,   523,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,   252,   181,   136,   182,   253,   254,   255,   256,   257,
       0,     0,   258,   259,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   137,   138,   139,   140,   141,     0,   142,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     148,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   260,     0,     0,
       0,     0,     0,   261,   262,   263,   264,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   265,   266,
      17,    18,    19,    20,    21,    22,   244,   245,   246,     0,
     247,   248,   249,   250,   251,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,     0,     0,     0,     0,   128,   129,   130,   131,
     132,   133,   134,   135,   252,   181,   136,   182,   253,   254,
     255,   256,   257,     0,     0,   258,   259,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   137,   138,   139,   140,   141,     0,
     142,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   148,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     260,     0,     0,     0,     0,     0,   261,   262,   263,   264,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   265,   266,    17,    18,    19,    20,    21,    22,   244,
     245,   246,     0,   247,   248,   249,   250,   251,     0,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,   252,   181,   136,
     182,   253,   254,   255,   256,   257,     0,     0,   258,   259,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   137,   138,   139,
     140,   141,     0,   142,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   148,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   260,     0,     0,     0,     0,     0,   261,
     262,   263,   264,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,   265,   200,     0,     0,     0,     0,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,     0,   181,   136,
     182,   253,   254,   255,   256,   257,     0,     0,   258,   259,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   137,   138,   139,
     140,   141,     0,   142,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   148,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   260,     0,     0,     0,     0,     0,   261,
     262,   263,   264,    19,    20,    21,    22,     0,     0,     0,
       0,     0,     0,     0,   265,     0,     0,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   133,   134,   135,     0,   181,   136,   182,   253,
     254,   255,   256,   257,     0,     0,   258,   259,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,   260,     0,     0,     0,     0,     0,   261,   262,   263,
     264,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,   406,   514,     0,     0,     0,     0,     0,
       0,     0,     0,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   133,   134,   135,     0,   181,
     136,   182,   253,   254,   255,   256,   257,     0,     0,   258,
     259,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    19,
      20,    21,    22,     0,   260,     0,     0,     0,     0,     0,
     261,   262,   263,   264,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   406,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   133,   134,
     135,     0,   181,   136,   182,   253,   254,   255,   256,   257,
       0,     0,   258,   259,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   260,     0,     0,
       0,     0,     0,   261,   262,   263,   264,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,   347,     0,
       0,     0,     0,     0,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,   178,   179,    38,
     180,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,     0,   181,   136,   182,   253,   254,   255,   256,   257,
       0,     0,   258,   259,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   183,   184,   139,   140,   141,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     148,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,   260,     0,     0,
       0,     0,     0,   261,   262,   263,   264,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   133,
     134,   135,     0,   181,   136,   182,   253,   254,   255,   256,
     257,     0,     0,   258,   259,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    19,    20,    21,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,   260,     0,
       0,   323,     0,     0,   261,   262,   263,   264,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     133,   134,   135,     0,   181,   136,   182,   253,   254,   255,
     256,   257,     0,     0,   258,   259,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    19,    20,    21,    22,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   260,
       0,     0,   411,     0,     0,   261,   262,   263,   264,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   133,   134,   135,     0,   181,   136,   182,   253,   254,
     255,   256,   257,     0,     0,   258,   259,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    19,    20,    21,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     260,     0,     0,     0,     0,     0,   261,   262,   263,   264,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   133,   134,   363,     0,   181,   136,   182,   253,
     254,   255,   256,   257,     0,     0,   258,   259,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    19,    20,    21,    22,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   260,     0,     0,     0,     0,     0,   261,   262,   263,
     264,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   133,   134,   492,     0,   181,   136,   182,
     253,   254,   255,   256,   257,     0,     0,   258,   259,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    17,    18,    19,    20,    21,
      22,     0,   260,     0,     0,     0,     0,     0,   261,   262,
     263,   264,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,     0,     0,     0,
       0,   128,   129,   130,   131,   132,   133,   134,   135,     0,
       0,   136,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   137,
     138,   139,   140,   141,     0,   142,     0,    12,     0,     0,
       0,     0,     0,   143,   144,   145,   146,   147,   148,    17,
      18,    19,    20,    21,    22,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   135,     0,     0,   136,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   137,   138,   139,   140,   141,     0,   142,
      17,    18,    19,    20,    21,    22,     0,   143,   144,   145,
     146,   147,   148,     0,     0,     0,     0,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
     178,   179,    38,   180,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    17,    18,     0,     0,   128,   129,   130,   131,
     132,   133,   134,   135,     0,     0,   136,     0,     0,     0,
       0,     0,    17,    18,     0,     0,     0,     0,     0,     0,
       0,    35,   178,   179,    38,   180,    40,    41,    42,    43,
      44,     0,     0,     0,   183,   184,   139,   140,   141,     0,
       0,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,     0,     0,   148,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   128,   129,
     130,   131,   132,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   128,   129,
     130,   131,   132,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   183,   184,   139,   140,
     141,     0,     0,     0,     0,     0,    19,    20,    21,    22,
       0,     0,     0,     0,     0,   148,   183,   184,   139,   140,
     141,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,     0,     0,   148,     0,     0,     0,     0,
       0,     0,     0,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   133,   134,   135,     0,     0,
     136
};

static const yytype_int16 yycheck[] =
{
      13,   246,    13,     4,    13,   217,   250,   159,   176,   137,
     138,    12,    18,    19,    18,    19,   260,   140,    13,   138,
     139,   140,   228,   404,   140,   406,   140,   234,   409,   155,
     158,   138,   139,   140,     4,   138,   139,   140,   235,   238,
       4,   142,   235,   148,   149,   174,   150,   151,   276,   235,
     142,   137,   138,     0,   155,   254,   253,   302,   254,   255,
     253,   235,   238,    33,    34,    35,   234,   253,   236,    33,
      34,    35,   485,   159,   160,   178,   162,   163,   238,   253,
     256,   167,   238,   238,   138,   139,   140,   194,   469,   317,
     503,   234,   238,   238,   254,   238,   252,   183,   184,   254,
     186,   345,   203,   204,   316,   349,   350,   208,   254,   254,
     211,   212,   234,   234,   358,   496,   238,   238,   175,   331,
     178,   238,   328,   140,   231,   251,   177,   508,   140,   336,
     235,   254,   237,   134,   362,   252,   255,   150,   254,   150,
     254,   150,   246,   247,   296,   178,   234,   234,   161,   377,
     238,   238,   204,   140,   398,   150,   208,   158,   234,   211,
     212,   329,   238,   176,   178,   234,   239,   240,   336,   238,
     178,   399,   146,   147,   175,   178,   404,   190,   406,   152,
     153,   409,   350,   427,   169,   170,   171,   200,   178,   200,
     435,   403,   138,   139,   140,   196,   233,   210,   168,   169,
     170,   171,   142,   143,   168,   169,   170,   171,   234,   228,
     296,   238,   213,   382,   383,   443,   386,   387,   388,   389,
     235,   234,   140,   236,   255,   243,   244,   245,   390,   391,
     235,   359,   255,   246,   478,   246,   233,   482,   254,   258,
     259,   469,   384,   385,   233,   233,   178,   415,   234,   233,
     256,   253,   256,   233,   254,   254,   254,   233,   502,   427,
     279,   233,   256,   491,   234,   233,   494,   233,   496,     3,
       4,   248,   250,   154,   249,   254,   521,   156,   522,   233,
     508,   526,   527,   236,   137,   234,   236,   499,   233,   302,
     238,   302,   234,   538,   256,   236,   233,   140,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,   234,   328,
     254,   234,   313,   234,   253,    12,   329,     3,     4,   255,
     254,   254,   252,   336,   150,   392,   394,   393,   203,   236,
     395,   344,   396,   344,   397,   328,   413,   350,   417,   340,
     503,   152,   355,   427,   355,   355,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,   528,   340,   359,   526,
     379,   380,   381,   382,   383,   384,   385,   386,   387,   388,
     389,   390,   391,   392,   393,   394,   395,   396,   397,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   344,
     150,   415,   344,    -1,    -1,   129,   130,   131,   132,   133,
      -1,    -1,   415,    -1,   138,    -1,   140,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   427,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   435,    -1,   435,    -1,    -1,    -1,    -1,    -1,
      -1,   432,    -1,   167,   168,   169,   170,   171,    -1,    -1,
      -1,    -1,    -1,   129,   130,   131,   132,   133,    -1,    -1,
      -1,    -1,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   482,
      -1,   482,   485,    -1,   485,   253,    -1,    -1,    -1,    -1,
      -1,   167,   168,   169,   170,   171,    -1,    -1,    -1,    -1,
     503,    -1,   503,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   521,    -1,
     521,    -1,    -1,   526,   527,   526,   527,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   538,    -1,   538,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    13,    14,
      15,    16,    17,    -1,    -1,    20,    21,    22,    23,    24,
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
      -1,    -1,    -1,    -1,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,    -1,    -1,   148,   149,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   167,   168,   169,   170,   171,    -1,   173,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   233,    -1,
      -1,    -1,    -1,    -1,   239,   240,   241,   242,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   254,
     255,   256,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    13,    14,    15,    16,    17,    -1,    -1,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,    -1,    -1,   148,   149,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,   169,   170,
     171,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   233,    -1,    -1,    -1,    -1,    -1,   239,   240,
     241,   242,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   254,   255,   256,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    13,    14,    15,    16,
      17,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,    -1,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,    -1,
      -1,   148,   149,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,   168,   169,   170,   171,    -1,   173,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   233,    -1,    -1,    -1,
      -1,    -1,   239,   240,   241,   242,     3,     4,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,   254,   255,   256,
      -1,    -1,    -1,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,    -1,   129,   130,   131,   132,   133,   134,   135,   136,
      -1,    -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,   168,   169,   170,   171,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   256,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,
     130,   131,   132,   133,   134,   135,   136,    -1,    -1,   139,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,   169,
     170,   171,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   186,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,   256,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
      -1,    -1,   148,   149,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,   169,   170,   171,    -1,   173,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   233,    -1,    -1,
      -1,    -1,    -1,   239,   240,   241,   242,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   254,   255,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      13,    14,    15,    16,    17,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,    -1,    -1,   148,   149,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   167,   168,   169,   170,   171,    -1,
     173,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     233,    -1,    -1,    -1,    -1,    -1,   239,   240,   241,   242,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   254,   255,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    13,    14,    15,    16,    17,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,    -1,    -1,   148,   149,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,   169,
     170,   171,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   186,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   233,    -1,    -1,    -1,    -1,    -1,   239,
     240,   241,   242,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,   254,   255,    -1,    -1,    -1,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,
     130,   131,   132,   133,   134,   135,   136,    -1,   138,   139,
     140,   141,   142,   143,   144,   145,    -1,    -1,   148,   149,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   168,   169,
     170,   171,    -1,   173,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   186,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   233,    -1,    -1,    -1,    -1,    -1,   239,
     240,   241,   242,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   254,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,   136,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   148,   149,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,   233,    -1,    -1,    -1,    -1,    -1,   239,   240,   241,
     242,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   255,   256,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   134,   135,   136,    -1,   138,
     139,   140,   141,   142,   143,   144,   145,    -1,    -1,   148,
     149,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     5,
       6,     7,     8,    -1,   233,    -1,    -1,    -1,    -1,    -1,
     239,   240,   241,   242,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,   255,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,   145,
      -1,    -1,   148,   149,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   233,    -1,    -1,
      -1,    -1,    -1,   239,   240,   241,   242,     3,     4,     5,
       6,     7,     8,    -1,    -1,    -1,    -1,    -1,   254,    -1,
      -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,   129,   130,   131,   132,   133,   134,   135,
     136,    -1,   138,   139,   140,   141,   142,   143,   144,   145,
      -1,    -1,   148,   149,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   167,   168,   169,   170,   171,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,   233,    -1,    -1,
      -1,    -1,    -1,   239,   240,   241,   242,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,
     135,   136,    -1,   138,   139,   140,   141,   142,   143,   144,
     145,    -1,    -1,   148,   149,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,   233,    -1,
      -1,   236,    -1,    -1,   239,   240,   241,   242,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     134,   135,   136,    -1,   138,   139,   140,   141,   142,   143,
     144,   145,    -1,    -1,   148,   149,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,   233,
      -1,    -1,   236,    -1,    -1,   239,   240,   241,   242,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   134,   135,   136,    -1,   138,   139,   140,   141,   142,
     143,   144,   145,    -1,    -1,   148,   149,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
     233,    -1,    -1,    -1,    -1,    -1,   239,   240,   241,   242,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   134,   135,   136,    -1,   138,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   148,   149,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     5,     6,     7,     8,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,   233,    -1,    -1,    -1,    -1,    -1,   239,   240,   241,
     242,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   134,   135,   136,    -1,   138,   139,   140,
     141,   142,   143,   144,   145,    -1,    -1,   148,   149,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,    -1,   233,    -1,    -1,    -1,    -1,    -1,   239,   240,
     241,   242,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,   129,   130,   131,   132,   133,   134,   135,   136,    -1,
      -1,   139,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,    -1,    -1,
      -1,    -1,    -1,   181,   182,   183,   184,   185,   186,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,    -1,    -1,   129,   130,   131,   132,   133,
     134,   135,   136,    -1,    -1,   139,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   167,   168,   169,   170,   171,    -1,   173,
       3,     4,     5,     6,     7,     8,    -1,   181,   182,   183,
     184,   185,   186,    -1,    -1,    -1,    -1,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,     3,     4,    -1,    -1,   129,   130,   131,   132,
     133,   134,   135,   136,    -1,    -1,   139,    -1,    -1,    -1,
      -1,    -1,     3,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    -1,    -1,   167,   168,   169,   170,   171,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    -1,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,
     131,   132,   133,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   130,
     131,   132,   133,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,   169,   170,
     171,    -1,    -1,    -1,    -1,    -1,     5,     6,     7,     8,
      -1,    -1,    -1,    -1,    -1,   186,   167,   168,   169,   170,
     171,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   134,   135,   136,    -1,    -1,
     139
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   174,   258,   260,   142,     0,   262,   138,   139,   140,
     178,   263,   175,   259,   264,   178,   263,     3,     4,     5,
       6,     7,     8,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   129,   130,
     131,   132,   133,   134,   135,   136,   139,   167,   168,   169,
     170,   171,   173,   181,   182,   183,   184,   185,   186,   261,
     265,   299,   300,   301,   302,   303,   309,   310,   311,   312,
     317,   318,   319,   320,   322,   323,   324,   325,   326,   356,
     357,   358,   359,   360,   364,   177,   255,   263,    33,    34,
      36,   138,   140,   167,   168,   266,   312,   318,   266,   318,
     325,   178,   178,   178,   178,   178,   233,   264,   356,   254,
     255,   339,   234,   238,     4,    33,    34,    35,   168,   305,
     306,   307,   325,   238,   254,   138,   140,   263,   266,    33,
      34,    36,   318,   359,   318,   322,   318,   318,   235,   321,
     318,   140,   263,   311,   327,   328,   255,   322,   194,   231,
     263,   313,   315,   316,     9,    10,    11,    13,    14,    15,
      16,    17,   137,   141,   142,   143,   144,   145,   148,   149,
     233,   239,   240,   241,   242,   254,   255,   256,   266,   267,
     268,   270,   271,   272,   273,   274,   275,   276,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   297,   299,   300,   312,   322,   333,   334,
     335,   336,   340,   341,   342,   345,   351,   355,   305,   306,
     306,   304,   308,   322,   306,   306,   263,   253,   321,   233,
     254,   254,   254,   236,   281,   294,   298,   322,   235,   255,
     178,   263,   329,   330,   256,   328,   327,   254,   253,   234,
     238,   254,   254,   334,   233,   233,   254,   254,   297,   233,
     233,   281,   281,   297,   256,   337,   148,   149,   235,   237,
     234,   234,   238,   136,   295,   233,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   253,   296,   281,   243,
     244,   245,   239,   240,   146,   147,   150,   151,   246,   247,
     152,   153,   248,   249,   250,   154,   156,   155,   251,   238,
     254,   256,   334,   263,   253,   321,   255,   295,   331,   253,
     236,   236,   298,   311,   362,   363,   321,   238,   254,   256,
     142,   143,   314,   315,   137,   333,   341,   352,   297,   254,
     297,   297,   311,   344,   234,   340,   269,   297,   263,   266,
     277,   278,   279,   280,   295,   295,   281,   281,   281,   283,
     283,   284,   284,   285,   285,   285,   285,   286,   286,   287,
     288,   289,   290,   291,   292,   297,   295,   321,   331,   253,
     331,   332,   331,   236,   329,   256,   362,   330,   233,   344,
     353,   354,   234,   234,   263,   234,   256,   236,   233,   234,
     234,   238,   136,   295,   252,   331,   238,   256,   254,   140,
     361,   297,   254,   234,   334,   343,   255,   346,   253,   335,
     338,   339,   295,   295,   256,   331,   321,   254,   234,   297,
     338,    12,    18,    19,   256,   347,   348,   349,   350,   331,
     254,   334,   297,   252,   334,   347,   334,   256,   349,   252
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
#line 88 "src/glsl/glsl_parser.yy"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
}
/* Line 1078 of yacc.c.  */
#line 3402 "src/glsl/glsl_parser.cpp"
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
#line 303 "src/glsl/glsl_parser.yy"
    {
      _mesa_glsl_initialize_types(state);
   ;}
    break;

  case 3:
#line 307 "src/glsl/glsl_parser.yy"
    {
      delete state->symbols;
      state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
      _mesa_glsl_initialize_types(state);
   ;}
    break;

  case 5:
#line 317 "src/glsl/glsl_parser.yy"
    {
      state->process_version_directive(&(yylsp[(2) - (3)]), (yyvsp[(2) - (3)].n), NULL);
      if (state->error) {
         YYERROR;
      }
   ;}
    break;

  case 6:
#line 324 "src/glsl/glsl_parser.yy"
    {
      state->process_version_directive(&(yylsp[(2) - (4)]), (yyvsp[(2) - (4)].n), (yyvsp[(3) - (4)].identifier));
      if (state->error) {
         YYERROR;
      }
   ;}
    break;

  case 11:
#line 338 "src/glsl/glsl_parser.yy"
    {
      if (!state->is_version(120, 100)) {
         _mesa_glsl_warning(& (yylsp[(1) - (2)]), state,
                            "pragma `invariant(all)' not supported in %s "
                            "(GLSL ES 1.00 or GLSL 1.20 required)",
                            state->get_version_string());
      } else {
         state->all_invariant = true;
      }
   ;}
    break;

  case 17:
#line 363 "src/glsl/glsl_parser.yy"
    {
      if (!_mesa_glsl_process_extension((yyvsp[(2) - (5)].identifier), & (yylsp[(2) - (5)]), (yyvsp[(4) - (5)].identifier), & (yylsp[(4) - (5)]), state)) {
         YYERROR;
      }
   ;}
    break;

  case 18:
#line 372 "src/glsl/glsl_parser.yy"
    {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[(1) - (1)].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[(1) - (1)].node)->link);
   ;}
    break;

  case 19:
#line 380 "src/glsl/glsl_parser.yy"
    {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[(2) - (2)].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 20:
#line 387 "src/glsl/glsl_parser.yy"
    {
      if (!state->allow_extension_directive_midshader) {
         _mesa_glsl_error(& (yylsp[(2) - (2)]), state,
                          "#extension directive is not allowed "
                          "in the middle of a shader");
         YYERROR;
      }
   ;}
    break;

  case 23:
#line 404 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[(1) - (1)].identifier);
   ;}
    break;

  case 24:
#line 411 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.int_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 25:
#line 418 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.uint_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 26:
#line 425 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.float_constant = (yyvsp[(1) - (1)].real);
   ;}
    break;

  case 27:
#line 432 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->primary_expression.bool_constant = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 28:
#line 439 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (3)].expression);
   ;}
    break;

  case 30:
#line 447 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[(1) - (4)].expression), (yyvsp[(3) - (4)].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (4)]), (yylsp[(4) - (4)]));
   ;}
    break;

  case 31:
#line 453 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 32:
#line 457 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[(3) - (3)].identifier);
   ;}
    break;

  case 33:
#line 464 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[(1) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 34:
#line 470 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[(1) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 38:
#line 488 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 43:
#line 507 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (2)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
   ;}
    break;

  case 44:
#line 513 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (3)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (3)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 46:
#line 529 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_function_expression((yyvsp[(1) - (1)].type_specifier));
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      ;}
    break;

  case 47:
#line 535 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
      callee->set_location((yylsp[(1) - (1)]));
      (yyval.expression) = new(ctx) ast_function_expression(callee);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      ;}
    break;

  case 48:
#line 543 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (1)].identifier));
      callee->set_location((yylsp[(1) - (1)]));
      (yyval.expression) = new(ctx) ast_function_expression(callee);
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      ;}
    break;

  case 53:
#line 564 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (2)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(2) - (2)].expression)->link);
   ;}
    break;

  case 54:
#line 570 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (3)].expression);
      (yyval.expression)->set_location((yylsp[(1) - (3)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 55:
#line 582 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_expression *callee = new(ctx) ast_expression((yyvsp[(1) - (2)].identifier));
      callee->set_location((yylsp[(1) - (2)]));
      (yyval.expression) = new(ctx) ast_function_expression(callee);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 57:
#line 595 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 58:
#line 601 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 59:
#line 607 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[(1) - (2)].n), (yyvsp[(2) - (2)].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 60:
#line 616 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_plus; ;}
    break;

  case 61:
#line 617 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_neg; ;}
    break;

  case 62:
#line 618 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_logic_not; ;}
    break;

  case 63:
#line 619 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_bit_not; ;}
    break;

  case 65:
#line 625 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 66:
#line 631 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 67:
#line 637 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 69:
#line 647 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 70:
#line 653 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 72:
#line 663 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 73:
#line 669 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 75:
#line 679 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 76:
#line 685 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 77:
#line 691 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 78:
#line 697 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 80:
#line 707 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 81:
#line 713 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 83:
#line 723 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 85:
#line 733 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 87:
#line 743 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 89:
#line 753 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 91:
#line 763 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 93:
#line 773 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 95:
#line 783 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[(1) - (5)].expression), (yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].expression));
      (yyval.expression)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 97:
#line 793 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[(2) - (3)].n), (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 98:
#line 801 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_assign; ;}
    break;

  case 99:
#line 802 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mul_assign; ;}
    break;

  case 100:
#line 803 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_div_assign; ;}
    break;

  case 101:
#line 804 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_mod_assign; ;}
    break;

  case 102:
#line 805 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_add_assign; ;}
    break;

  case 103:
#line 806 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_sub_assign; ;}
    break;

  case 104:
#line 807 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_ls_assign; ;}
    break;

  case 105:
#line 808 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_rs_assign; ;}
    break;

  case 106:
#line 809 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_and_assign; ;}
    break;

  case 107:
#line 810 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_xor_assign; ;}
    break;

  case 108:
#line 811 "src/glsl/glsl_parser.yy"
    { (yyval.n) = ast_or_assign; ;}
    break;

  case 109:
#line 816 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 110:
#line 820 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
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

  case 112:
#line 840 "src/glsl/glsl_parser.yy"
    {
      state->symbols->pop_scope();
      (yyval.node) = (yyvsp[(1) - (2)].function);
   ;}
    break;

  case 113:
#line 845 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = (yyvsp[(1) - (2)].declarator_list);
   ;}
    break;

  case 114:
#line 849 "src/glsl/glsl_parser.yy"
    {
      (yyvsp[(3) - (4)].type_specifier)->default_precision = (yyvsp[(2) - (4)].n);
      (yyval.node) = (yyvsp[(3) - (4)].type_specifier);
   ;}
    break;

  case 115:
#line 854 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = (yyvsp[(1) - (1)].node);
   ;}
    break;

  case 119:
#line 870 "src/glsl/glsl_parser.yy"
    {
      (yyval.function) = (yyvsp[(1) - (2)].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[(2) - (2)].parameter_declarator)->link);
   ;}
    break;

  case 120:
#line 875 "src/glsl/glsl_parser.yy"
    {
      (yyval.function) = (yyvsp[(1) - (3)].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[(3) - (3)].parameter_declarator)->link);
   ;}
    break;

  case 121:
#line 883 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.function) = new(ctx) ast_function();
      (yyval.function)->set_location((yylsp[(2) - (3)]));
      (yyval.function)->return_type = (yyvsp[(1) - (3)].fully_specified_type);
      (yyval.function)->identifier = (yyvsp[(2) - (3)].identifier);

      state->symbols->add_function(new(state) ir_function((yyvsp[(2) - (3)].identifier)));
      state->symbols->push_scope();
   ;}
    break;

  case 122:
#line 897 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[(1) - (2)]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (2)].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (2)].identifier);
   ;}
    break;

  case 123:
#line 907 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[(1) - (3)]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(1) - (3)].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[(2) - (3)].identifier);
      (yyval.parameter_declarator)->array_specifier = (yyvsp[(3) - (3)].array_specifier);
   ;}
    break;

  case 124:
#line 921 "src/glsl/glsl_parser.yy"
    {
      (yyval.parameter_declarator) = (yyvsp[(2) - (2)].parameter_declarator);
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
   ;}
    break;

  case 125:
#line 926 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location((yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.parameter_declarator)->type->specifier = (yyvsp[(2) - (2)].type_specifier);
   ;}
    break;

  case 126:
#line 939 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
   ;}
    break;

  case 127:
#line 944 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate const qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.constant = 1;
   ;}
    break;

  case 128:
#line 952 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precise qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 129:
#line 960 "src/glsl/glsl_parser.yy"
    {
      if (((yyvsp[(1) - (2)].type_qualifier).flags.q.in || (yyvsp[(1) - (2)].type_qualifier).flags.q.out) && ((yyvsp[(2) - (2)].type_qualifier).flags.q.in || (yyvsp[(2) - (2)].type_qualifier).flags.q.out))
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate in/out/inout qualifier");

      if (!state->ARB_shading_language_420pack_enable && (yyvsp[(2) - (2)].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "in/out/inout must come after const "
                                      "or precise");

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier));
   ;}
    break;

  case 130:
#line 972 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precision qualifier");

      if (!state->ARB_shading_language_420pack_enable && (yyvsp[(2) - (2)].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[(1) - (2)].n);
   ;}
    break;

  case 131:
#line 985 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 132:
#line 991 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 133:
#line 997 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.in = 1;
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 136:
#line 1013 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (3)].identifier), NULL, NULL);
      decl->set_location((yylsp[(3) - (3)]));

      (yyval.declarator_list) = (yyvsp[(1) - (3)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (3)].identifier), ir_var_auto, glsl_precision_undefined));
   ;}
    break;

  case 137:
#line 1023 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (4)].identifier), (yyvsp[(4) - (4)].array_specifier), NULL);
      decl->set_location_range((yylsp[(3) - (4)]), (yylsp[(4) - (4)]));

      (yyval.declarator_list) = (yyvsp[(1) - (4)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (4)].identifier), ir_var_auto, glsl_precision_undefined));
   ;}
    break;

  case 138:
#line 1033 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (6)].identifier), (yyvsp[(4) - (6)].array_specifier), (yyvsp[(6) - (6)].expression));
      decl->set_location_range((yylsp[(3) - (6)]), (yylsp[(4) - (6)]));

      (yyval.declarator_list) = (yyvsp[(1) - (6)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (6)].identifier), ir_var_auto, glsl_precision_undefined));
   ;}
    break;

  case 139:
#line 1043 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(3) - (5)].identifier), NULL, (yyvsp[(5) - (5)].expression));
      decl->set_location((yylsp[(3) - (5)]));

      (yyval.declarator_list) = (yyvsp[(1) - (5)].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(state) ir_variable(NULL, (yyvsp[(3) - (5)].identifier), ir_var_auto, glsl_precision_undefined));
   ;}
    break;

  case 140:
#line 1057 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      /* Empty declaration list is valid. */
      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (1)].fully_specified_type));
      (yyval.declarator_list)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 141:
#line 1064 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (2)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 142:
#line 1074 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (3)].identifier), (yyvsp[(3) - (3)].array_specifier), NULL);
      decl->set_location_range((yylsp[(2) - (3)]), (yylsp[(3) - (3)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (3)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 143:
#line 1084 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (5)].identifier), (yyvsp[(3) - (5)].array_specifier), (yyvsp[(5) - (5)].expression));
      decl->set_location_range((yylsp[(2) - (5)]), (yylsp[(3) - (5)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (5)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (5)]), (yylsp[(3) - (5)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 144:
#line 1094 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), NULL, (yyvsp[(4) - (4)].expression));
      decl->set_location((yylsp[(2) - (4)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (4)]), (yylsp[(2) - (4)]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 145:
#line 1104 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->invariant = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 146:
#line 1116 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (2)].identifier), NULL, NULL);
      decl->set_location((yylsp[(2) - (2)]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.declarator_list)->precise = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   ;}
    break;

  case 147:
#line 1131 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location((yylsp[(1) - (1)]));
      (yyval.fully_specified_type)->specifier = (yyvsp[(1) - (1)].type_specifier);
   ;}
    break;

  case 148:
#line 1138 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.fully_specified_type)->qualifier = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.fully_specified_type)->specifier = (yyvsp[(2) - (2)].type_specifier);
   ;}
    break;

  case 149:
#line 1149 "src/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(3) - (4)].type_qualifier);
   ;}
    break;

  case 151:
#line 1157 "src/glsl/glsl_parser.yy"
    {
      (yyval.type_qualifier) = (yyvsp[(1) - (3)].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[(3) - (3)]), state, (yyvsp[(3) - (3)].type_qualifier))) {
         YYERROR;
      }
   ;}
    break;

  case 152:
#line 1166 "src/glsl/glsl_parser.yy"
    { (yyval.n) = (yyvsp[(1) - (1)].n); ;}
    break;

  case 153:
#line 1167 "src/glsl/glsl_parser.yy"
    { (yyval.n) = (yyvsp[(1) - (1)].n); ;}
    break;

  case 154:
#line 1172 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;

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
           state->ARB_conservative_depth_enable)) {
         if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_any", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_any = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_greater", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_greater = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_less", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_less = 1;
         } else if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), "depth_unchanged",
                                           state) == 0) {
            (yyval.type_qualifier).flags.q.depth_unchanged = 1;
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
         for (unsigned i = 0; i < Elements(map); i++) {
            if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.prim_type = 1;
               (yyval.type_qualifier).prim_type = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->is_version(150, 0)) {
            _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "#version 150 layout "
                             "qualifier `%s' used", (yyvsp[(1) - (1)].identifier));
         }
      }

      /* Layout qualifiers for ARB_shader_image_load_store. */
      if (state->ARB_shader_image_load_store_enable ||
          state->is_version(420, 0)) {
         if (!(yyval.type_qualifier).flags.i) {
            static const struct {
               const char *name;
               GLenum format;
               glsl_base_type base_type;
            } map[] = {
               { "rgba32f", GL_RGBA32F, GLSL_TYPE_FLOAT },
               { "rgba16f", GL_RGBA16F, GLSL_TYPE_FLOAT },
               { "rg32f", GL_RG32F, GLSL_TYPE_FLOAT },
               { "rg16f", GL_RG16F, GLSL_TYPE_FLOAT },
               { "r11f_g11f_b10f", GL_R11F_G11F_B10F, GLSL_TYPE_FLOAT },
               { "r32f", GL_R32F, GLSL_TYPE_FLOAT },
               { "r16f", GL_R16F, GLSL_TYPE_FLOAT },
               { "rgba32ui", GL_RGBA32UI, GLSL_TYPE_UINT },
               { "rgba16ui", GL_RGBA16UI, GLSL_TYPE_UINT },
               { "rgb10_a2ui", GL_RGB10_A2UI, GLSL_TYPE_UINT },
               { "rgba8ui", GL_RGBA8UI, GLSL_TYPE_UINT },
               { "rg32ui", GL_RG32UI, GLSL_TYPE_UINT },
               { "rg16ui", GL_RG16UI, GLSL_TYPE_UINT },
               { "rg8ui", GL_RG8UI, GLSL_TYPE_UINT },
               { "r32ui", GL_R32UI, GLSL_TYPE_UINT },
               { "r16ui", GL_R16UI, GLSL_TYPE_UINT },
               { "r8ui", GL_R8UI, GLSL_TYPE_UINT },
               { "rgba32i", GL_RGBA32I, GLSL_TYPE_INT },
               { "rgba16i", GL_RGBA16I, GLSL_TYPE_INT },
               { "rgba8i", GL_RGBA8I, GLSL_TYPE_INT },
               { "rg32i", GL_RG32I, GLSL_TYPE_INT },
               { "rg16i", GL_RG16I, GLSL_TYPE_INT },
               { "rg8i", GL_RG8I, GLSL_TYPE_INT },
               { "r32i", GL_R32I, GLSL_TYPE_INT },
               { "r16i", GL_R16I, GLSL_TYPE_INT },
               { "r8i", GL_R8I, GLSL_TYPE_INT },
               { "rgba16", GL_RGBA16, GLSL_TYPE_FLOAT },
               { "rgb10_a2", GL_RGB10_A2, GLSL_TYPE_FLOAT },
               { "rgba8", GL_RGBA8, GLSL_TYPE_FLOAT },
               { "rg16", GL_RG16, GLSL_TYPE_FLOAT },
               { "rg8", GL_RG8, GLSL_TYPE_FLOAT },
               { "r16", GL_R16, GLSL_TYPE_FLOAT },
               { "r8", GL_R8, GLSL_TYPE_FLOAT },
               { "rgba16_snorm", GL_RGBA16_SNORM, GLSL_TYPE_FLOAT },
               { "rgba8_snorm", GL_RGBA8_SNORM, GLSL_TYPE_FLOAT },
               { "rg16_snorm", GL_RG16_SNORM, GLSL_TYPE_FLOAT },
               { "rg8_snorm", GL_RG8_SNORM, GLSL_TYPE_FLOAT },
               { "r16_snorm", GL_R16_SNORM, GLSL_TYPE_FLOAT },
               { "r8_snorm", GL_R8_SNORM, GLSL_TYPE_FLOAT }
            };

            for (unsigned i = 0; i < Elements(map); i++) {
               if (match_layout_qualifier((yyvsp[(1) - (1)].identifier), map[i].name, state) == 0) {
                  (yyval.type_qualifier).flags.q.explicit_image_format = 1;
                  (yyval.type_qualifier).image_format = map[i].format;
                  (yyval.type_qualifier).image_base_type = map[i].base_type;
                  break;
               }
            }
         }

         if (!(yyval.type_qualifier).flags.i &&
             match_layout_qualifier((yyvsp[(1) - (1)].identifier), "early_fragment_tests", state) == 0) {
            (yyval.type_qualifier).flags.q.early_fragment_tests = 1;
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "unrecognized layout identifier "
                          "`%s'", (yyvsp[(1) - (1)].identifier));
         YYERROR;
      }
   ;}
    break;

  case 155:
#line 1358 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;

      if (match_layout_qualifier("location", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_location = 1;

         if ((yyval.type_qualifier).flags.q.attribute == 1 &&
             state->ARB_explicit_attrib_location_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (3)]), state,
                               "GL_ARB_explicit_attrib_location layout "
                               "identifier `%s' used", (yyvsp[(1) - (3)].identifier));
         }

         if ((yyvsp[(3) - (3)].n) >= 0) {
            (yyval.type_qualifier).location = (yyvsp[(3) - (3)].n);
         } else {
             _mesa_glsl_error(& (yylsp[(3) - (3)]), state, "invalid location %d specified", (yyvsp[(3) - (3)].n));
             YYERROR;
         }
      }

      if (match_layout_qualifier("index", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_index = 1;

         if ((yyvsp[(3) - (3)].n) >= 0) {
            (yyval.type_qualifier).index = (yyvsp[(3) - (3)].n);
         } else {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state, "invalid index %d specified", (yyvsp[(3) - (3)].n));
            YYERROR;
         }
      }

      if ((state->ARB_shading_language_420pack_enable ||
           state->ARB_shader_atomic_counters_enable) &&
          match_layout_qualifier("binding", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_binding = 1;
         (yyval.type_qualifier).binding = (yyvsp[(3) - (3)].n);
      }

      if (state->ARB_shader_atomic_counters_enable &&
          match_layout_qualifier("offset", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_offset = 1;
         (yyval.type_qualifier).offset = (yyvsp[(3) - (3)].n);
      }

      if (match_layout_qualifier("max_vertices", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.max_vertices = 1;

         if ((yyvsp[(3) - (3)].n) < 0) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                             "invalid max_vertices %d specified", (yyvsp[(3) - (3)].n));
            YYERROR;
         } else {
            (yyval.type_qualifier).max_vertices = (yyvsp[(3) - (3)].n);
            if (!state->is_version(150, 0)) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "#version 150 max_vertices qualifier "
                                "specified", (yyvsp[(3) - (3)].n));
            }
         }
      }

      if (state->stage == MESA_SHADER_GEOMETRY) {
         if (match_layout_qualifier("stream", (yyvsp[(1) - (3)].identifier), state) == 0 &&
             state->check_explicit_attrib_stream_allowed(& (yylsp[(3) - (3)]))) {
            (yyval.type_qualifier).flags.q.stream = 1;

            if ((yyvsp[(3) - (3)].n) < 0) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "invalid stream %d specified", (yyvsp[(3) - (3)].n));
               YYERROR;
            } else {
               (yyval.type_qualifier).flags.q.explicit_stream = 1;
               (yyval.type_qualifier).stream = (yyvsp[(3) - (3)].n);
            }
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
            if ((yyvsp[(3) - (3)].n) <= 0) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "invalid %s of %d specified",
                                local_size_qualifiers[i], (yyvsp[(3) - (3)].n));
               YYERROR;
            } else if (!state->is_version(430, 0) &&
                       !state->ARB_compute_shader_enable) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "%s qualifier requires GLSL 4.30 or "
                                "ARB_compute_shader",
                                local_size_qualifiers[i]);
               YYERROR;
            } else {
               (yyval.type_qualifier).flags.q.local_size |= (1 << i);
               (yyval.type_qualifier).local_size[i] = (yyvsp[(3) - (3)].n);
            }
            break;
         }
      }

      if (match_layout_qualifier("invocations", (yyvsp[(1) - (3)].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.invocations = 1;

         if ((yyvsp[(3) - (3)].n) <= 0) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                             "invalid invocations %d specified", (yyvsp[(3) - (3)].n));
            YYERROR;
         } else if ((yyvsp[(3) - (3)].n) > MAX_GEOMETRY_SHADER_INVOCATIONS) {
            _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                             "invocations (%d) exceeds "
                             "GL_MAX_GEOMETRY_SHADER_INVOCATIONS", (yyvsp[(3) - (3)].n));
            YYERROR;
         } else {
            (yyval.type_qualifier).invocations = (yyvsp[(3) - (3)].n);
            if (!state->is_version(400, 0) &&
                !state->ARB_gpu_shader5_enable) {
               _mesa_glsl_error(& (yylsp[(3) - (3)]), state,
                                "GL_ARB_gpu_shader5 invocations "
                                "qualifier specified", (yyvsp[(3) - (3)].n));
            }
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

  case 156:
#line 1498 "src/glsl/glsl_parser.yy"
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

  case 157:
#line 1524 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.row_major = 1;
   ;}
    break;

  case 158:
#line 1530 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.packed = 1;
   ;}
    break;

  case 159:
#line 1539 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.smooth = 1;
   ;}
    break;

  case 160:
#line 1545 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.flat = 1;
   ;}
    break;

  case 161:
#line 1551 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.noperspective = 1;
   ;}
    break;

  case 162:
#line 1561 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.invariant = 1;
   ;}
    break;

  case 163:
#line 1567 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 168:
#line 1576 "src/glsl/glsl_parser.yy"
    {
      memset(&(yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).precision = (yyvsp[(1) - (1)].n);
   ;}
    break;

  case 169:
#line 1595 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate \"precise\" qualifier");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   ;}
    break;

  case 170:
#line 1603 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).flags.q.invariant)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate \"invariant\" qualifier");

      if (!state->ARB_shading_language_420pack_enable && (yyvsp[(2) - (2)].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state,
                          "\"invariant\" must come after \"precise\"");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).flags.q.invariant = 1;
   ;}
    break;

  case 171:
#line 1615 "src/glsl/glsl_parser.yy"
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

      if (!state->ARB_shading_language_420pack_enable &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant)) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "interpolation qualifiers must come "
                          "after \"precise\" or \"invariant\"");
      }

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier));
   ;}
    break;

  case 172:
#line 1639 "src/glsl/glsl_parser.yy"
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
      if (!state->ARB_shading_language_420pack_enable && (yyvsp[(2) - (2)].type_qualifier).has_layout())
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate layout(...) qualifiers");

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier));
   ;}
    break;

  case 173:
#line 1656 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).has_auxiliary_storage()) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state,
                          "duplicate auxiliary storage qualifier (centroid or sample)");
      }

      if (!state->ARB_shading_language_420pack_enable &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant ||
           (yyvsp[(2) - (2)].type_qualifier).has_interpolation() || (yyvsp[(2) - (2)].type_qualifier).has_layout())) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "auxiliary storage qualifiers must come "
                          "just before storage qualifiers");
      }
      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier));
   ;}
    break;

  case 174:
#line 1672 "src/glsl/glsl_parser.yy"
    {
      /* Section 4.3 of the GLSL 1.20 specification states:
       * "Variable declarations may have a storage qualifier specified..."
       *  1.30 clarifies this to "may have one storage qualifier".
       */
      if ((yyvsp[(2) - (2)].type_qualifier).has_storage())
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate storage qualifier");

      if (!state->ARB_shading_language_420pack_enable &&
          ((yyvsp[(2) - (2)].type_qualifier).flags.q.precise || (yyvsp[(2) - (2)].type_qualifier).flags.q.invariant || (yyvsp[(2) - (2)].type_qualifier).has_interpolation() ||
           (yyvsp[(2) - (2)].type_qualifier).has_layout() || (yyvsp[(2) - (2)].type_qualifier).has_auxiliary_storage())) {
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "storage qualifiers must come after "
                          "precise, invariant, interpolation, layout and auxiliary "
                          "storage qualifiers");
      }

      (yyval.type_qualifier) = (yyvsp[(1) - (2)].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[(1) - (2)]), state, (yyvsp[(2) - (2)].type_qualifier));
   ;}
    break;

  case 175:
#line 1692 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "duplicate precision qualifier");

      if (!state->ARB_shading_language_420pack_enable && (yyvsp[(2) - (2)].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[(1) - (2)]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[(2) - (2)].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[(1) - (2)].n);
   ;}
    break;

  case 176:
#line 1706 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.centroid = 1;
   ;}
    break;

  case 177:
#line 1712 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.sample = 1;
   ;}
    break;

  case 178:
#line 1720 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.constant = 1;
   ;}
    break;

  case 179:
#line 1726 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.attribute = 1;
   ;}
    break;

  case 180:
#line 1732 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.varying = 1;
   ;}
    break;

  case 181:
#line 1738 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 182:
#line 1744 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 183:
#line 1750 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.in = 1;
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
   ;}
    break;

  case 184:
#line 1771 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.uniform = 1;
   ;}
    break;

  case 185:
#line 1777 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.coherent = 1;
   ;}
    break;

  case 186:
#line 1782 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q._volatile = 1;
   ;}
    break;

  case 187:
#line 1787 "src/glsl/glsl_parser.yy"
    {
      STATIC_ASSERT(sizeof((yyval.type_qualifier).flags.q) <= sizeof((yyval.type_qualifier).flags.i));
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.restrict_flag = 1;
   ;}
    break;

  case 188:
#line 1793 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.read_only = 1;
   ;}
    break;

  case 189:
#line 1798 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.write_only = 1;
   ;}
    break;

  case 190:
#line 1806 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[(1) - (2)]));
      (yyval.array_specifier)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 191:
#line 1812 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[(1) - (3)]), (yyvsp[(2) - (3)].expression));
      (yyval.array_specifier)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 192:
#line 1818 "src/glsl/glsl_parser.yy"
    {
      (yyval.array_specifier) = (yyvsp[(1) - (3)].array_specifier);

      if (!state->ARB_arrays_of_arrays_enable) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "GL_ARB_arrays_of_arrays "
                          "required for defining arrays of arrays");
      } else {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "only the outermost array dimension can "
                          "be unsized");
      }
   ;}
    break;

  case 193:
#line 1832 "src/glsl/glsl_parser.yy"
    {
      (yyval.array_specifier) = (yyvsp[(1) - (4)].array_specifier);

      if (!state->ARB_arrays_of_arrays_enable) {
         _mesa_glsl_error(& (yylsp[(1) - (4)]), state,
                          "GL_ARB_arrays_of_arrays "
                          "required for defining arrays of arrays");
      }

      (yyval.array_specifier)->add_dimension((yyvsp[(3) - (4)].expression));
   ;}
    break;

  case 195:
#line 1848 "src/glsl/glsl_parser.yy"
    {
      (yyval.type_specifier) = (yyvsp[(1) - (2)].type_specifier);
      (yyval.type_specifier)->array_specifier = (yyvsp[(2) - (2)].array_specifier);
   ;}
    break;

  case 196:
#line 1856 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 197:
#line 1862 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].struct_specifier));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 198:
#line 1868 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[(1) - (1)].identifier));
      (yyval.type_specifier)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 199:
#line 1876 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "void"; ;}
    break;

  case 200:
#line 1877 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "float"; ;}
    break;

  case 201:
#line 1878 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "int"; ;}
    break;

  case 202:
#line 1879 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uint"; ;}
    break;

  case 203:
#line 1880 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bool"; ;}
    break;

  case 204:
#line 1881 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec2"; ;}
    break;

  case 205:
#line 1882 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec3"; ;}
    break;

  case 206:
#line 1883 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "vec4"; ;}
    break;

  case 207:
#line 1884 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec2"; ;}
    break;

  case 208:
#line 1885 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec3"; ;}
    break;

  case 209:
#line 1886 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "bvec4"; ;}
    break;

  case 210:
#line 1887 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec2"; ;}
    break;

  case 211:
#line 1888 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec3"; ;}
    break;

  case 212:
#line 1889 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "ivec4"; ;}
    break;

  case 213:
#line 1890 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec2"; ;}
    break;

  case 214:
#line 1891 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec3"; ;}
    break;

  case 215:
#line 1892 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uvec4"; ;}
    break;

  case 216:
#line 1893 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2"; ;}
    break;

  case 217:
#line 1894 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x3"; ;}
    break;

  case 218:
#line 1895 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat2x4"; ;}
    break;

  case 219:
#line 1896 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x2"; ;}
    break;

  case 220:
#line 1897 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3"; ;}
    break;

  case 221:
#line 1898 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat3x4"; ;}
    break;

  case 222:
#line 1899 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x2"; ;}
    break;

  case 223:
#line 1900 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4x3"; ;}
    break;

  case 224:
#line 1901 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "mat4"; ;}
    break;

  case 225:
#line 1902 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1D"; ;}
    break;

  case 226:
#line 1903 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2D"; ;}
    break;

  case 227:
#line 1904 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRect"; ;}
    break;

  case 228:
#line 1905 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler3D"; ;}
    break;

  case 229:
#line 1906 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCube"; ;}
    break;

  case 230:
#line 1907 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerExternalOES"; ;}
    break;

  case 231:
#line 1908 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DShadow"; ;}
    break;

  case 232:
#line 1909 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DShadow"; ;}
    break;

  case 233:
#line 1910 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DRectShadow"; ;}
    break;

  case 234:
#line 1911 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeShadow"; ;}
    break;

  case 235:
#line 1912 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArray"; ;}
    break;

  case 236:
#line 1913 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArray"; ;}
    break;

  case 237:
#line 1914 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler1DArrayShadow"; ;}
    break;

  case 238:
#line 1915 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DArrayShadow"; ;}
    break;

  case 239:
#line 1916 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerBuffer"; ;}
    break;

  case 240:
#line 1917 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeArray"; ;}
    break;

  case 241:
#line 1918 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "samplerCubeArrayShadow"; ;}
    break;

  case 242:
#line 1919 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1D"; ;}
    break;

  case 243:
#line 1920 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2D"; ;}
    break;

  case 244:
#line 1921 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DRect"; ;}
    break;

  case 245:
#line 1922 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler3D"; ;}
    break;

  case 246:
#line 1923 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerCube"; ;}
    break;

  case 247:
#line 1924 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler1DArray"; ;}
    break;

  case 248:
#line 1925 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DArray"; ;}
    break;

  case 249:
#line 1926 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerBuffer"; ;}
    break;

  case 250:
#line 1927 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isamplerCubeArray"; ;}
    break;

  case 251:
#line 1928 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1D"; ;}
    break;

  case 252:
#line 1929 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2D"; ;}
    break;

  case 253:
#line 1930 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DRect"; ;}
    break;

  case 254:
#line 1931 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler3D"; ;}
    break;

  case 255:
#line 1932 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerCube"; ;}
    break;

  case 256:
#line 1933 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler1DArray"; ;}
    break;

  case 257:
#line 1934 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DArray"; ;}
    break;

  case 258:
#line 1935 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerBuffer"; ;}
    break;

  case 259:
#line 1936 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usamplerCubeArray"; ;}
    break;

  case 260:
#line 1937 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DMS"; ;}
    break;

  case 261:
#line 1938 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DMS"; ;}
    break;

  case 262:
#line 1939 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DMS"; ;}
    break;

  case 263:
#line 1940 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "sampler2DMSArray"; ;}
    break;

  case 264:
#line 1941 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "isampler2DMSArray"; ;}
    break;

  case 265:
#line 1942 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "usampler2DMSArray"; ;}
    break;

  case 266:
#line 1943 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image1D"; ;}
    break;

  case 267:
#line 1944 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image2D"; ;}
    break;

  case 268:
#line 1945 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image3D"; ;}
    break;

  case 269:
#line 1946 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image2DRect"; ;}
    break;

  case 270:
#line 1947 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "imageCube"; ;}
    break;

  case 271:
#line 1948 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "imageBuffer"; ;}
    break;

  case 272:
#line 1949 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image1DArray"; ;}
    break;

  case 273:
#line 1950 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image2DArray"; ;}
    break;

  case 274:
#line 1951 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "imageCubeArray"; ;}
    break;

  case 275:
#line 1952 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image2DMS"; ;}
    break;

  case 276:
#line 1953 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "image2DMSArray"; ;}
    break;

  case 277:
#line 1954 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage1D"; ;}
    break;

  case 278:
#line 1955 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage2D"; ;}
    break;

  case 279:
#line 1956 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage3D"; ;}
    break;

  case 280:
#line 1957 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage2DRect"; ;}
    break;

  case 281:
#line 1958 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimageCube"; ;}
    break;

  case 282:
#line 1959 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimageBuffer"; ;}
    break;

  case 283:
#line 1960 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage1DArray"; ;}
    break;

  case 284:
#line 1961 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage2DArray"; ;}
    break;

  case 285:
#line 1962 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimageCubeArray"; ;}
    break;

  case 286:
#line 1963 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage2DMS"; ;}
    break;

  case 287:
#line 1964 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "iimage2DMSArray"; ;}
    break;

  case 288:
#line 1965 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage1D"; ;}
    break;

  case 289:
#line 1966 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage2D"; ;}
    break;

  case 290:
#line 1967 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage3D"; ;}
    break;

  case 291:
#line 1968 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage2DRect"; ;}
    break;

  case 292:
#line 1969 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimageCube"; ;}
    break;

  case 293:
#line 1970 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimageBuffer"; ;}
    break;

  case 294:
#line 1971 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage1DArray"; ;}
    break;

  case 295:
#line 1972 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage2DArray"; ;}
    break;

  case 296:
#line 1973 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimageCubeArray"; ;}
    break;

  case 297:
#line 1974 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage2DMS"; ;}
    break;

  case 298:
#line 1975 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "uimage2DMSArray"; ;}
    break;

  case 299:
#line 1976 "src/glsl/glsl_parser.yy"
    { (yyval.identifier) = "atomic_uint"; ;}
    break;

  case 300:
#line 1981 "src/glsl/glsl_parser.yy"
    {
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_high;
   ;}
    break;

  case 301:
#line 1986 "src/glsl/glsl_parser.yy"
    {
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_medium;
   ;}
    break;

  case 302:
#line 1991 "src/glsl/glsl_parser.yy"
    {
      state->check_precision_qualifiers_allowed(&(yylsp[(1) - (1)]));
      (yyval.n) = ast_precision_low;
   ;}
    break;

  case 303:
#line 1999 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[(2) - (5)].identifier), (yyvsp[(4) - (5)].declarator_list));
      (yyval.struct_specifier)->set_location_range((yylsp[(2) - (5)]), (yylsp[(5) - (5)]));
      state->symbols->add_type((yyvsp[(2) - (5)].identifier), glsl_type::void_type);
   ;}
    break;

  case 304:
#line 2006 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier(NULL, (yyvsp[(3) - (4)].declarator_list));
      (yyval.struct_specifier)->set_location_range((yylsp[(2) - (4)]), (yylsp[(4) - (4)]));
   ;}
    break;

  case 305:
#line 2015 "src/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
      (yyvsp[(1) - (1)].declarator_list)->link.self_link();
   ;}
    break;

  case 306:
#line 2020 "src/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
      (yyval.declarator_list)->link.insert_before(& (yyvsp[(2) - (2)].declarator_list)->link);
   ;}
    break;

  case 307:
#line 2028 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_fully_specified_type *const type = (yyvsp[(1) - (3)].fully_specified_type);
      type->set_location((yylsp[(1) - (3)]));

      if (type->qualifier.flags.i != 0)
         _mesa_glsl_error(&(yylsp[(1) - (3)]), state,
			  "only precision qualifiers may be applied to "
			  "structure members");

      (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
      (yyval.declarator_list)->set_location((yylsp[(2) - (3)]));

      (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[(2) - (3)].declaration)->link);
   ;}
    break;

  case 308:
#line 2047 "src/glsl/glsl_parser.yy"
    {
      (yyval.declaration) = (yyvsp[(1) - (1)].declaration);
      (yyvsp[(1) - (1)].declaration)->link.self_link();
   ;}
    break;

  case 309:
#line 2052 "src/glsl/glsl_parser.yy"
    {
      (yyval.declaration) = (yyvsp[(1) - (3)].declaration);
      (yyval.declaration)->link.insert_before(& (yyvsp[(3) - (3)].declaration)->link);
   ;}
    break;

  case 310:
#line 2060 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (1)].identifier), NULL, NULL);
      (yyval.declaration)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 311:
#line 2066 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].array_specifier), NULL);
      (yyval.declaration)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 313:
#line 2076 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (3)].expression);
   ;}
    break;

  case 314:
#line 2080 "src/glsl/glsl_parser.yy"
    {
      (yyval.expression) = (yyvsp[(2) - (4)].expression);
   ;}
    break;

  case 315:
#line 2087 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.expression) = new(ctx) ast_aggregate_initializer();
      (yyval.expression)->set_location((yylsp[(1) - (1)]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[(1) - (1)].expression)->link);
   ;}
    break;

  case 316:
#line 2094 "src/glsl/glsl_parser.yy"
    {
      (yyvsp[(1) - (3)].expression)->expressions.push_tail(& (yyvsp[(3) - (3)].expression)->link);
   ;}
    break;

  case 318:
#line 2106 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 326:
#line 2121 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 327:
#line 2127 "src/glsl/glsl_parser.yy"
    {
      state->symbols->push_scope();
   ;}
    break;

  case 328:
#line 2131 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[(3) - (4)].node));
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (4)]), (yylsp[(4) - (4)]));
      state->symbols->pop_scope();
   ;}
    break;

  case 329:
#line 2140 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].compound_statement); ;}
    break;

  case 331:
#line 2146 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 332:
#line 2152 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[(2) - (3)].node));
      (yyval.compound_statement)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 333:
#line 2161 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(1) - (1)].node) == NULL) {
         _mesa_glsl_error(& (yylsp[(1) - (1)]), state, "<nil> statement");
         assert((yyvsp[(1) - (1)].node) != NULL);
      }

      (yyval.node) = (yyvsp[(1) - (1)].node);
      (yyval.node)->link.self_link();
   ;}
    break;

  case 334:
#line 2171 "src/glsl/glsl_parser.yy"
    {
      if ((yyvsp[(2) - (2)].node) == NULL) {
         _mesa_glsl_error(& (yylsp[(2) - (2)]), state, "<nil> statement");
         assert((yyvsp[(2) - (2)].node) != NULL);
      }
      (yyval.node) = (yyvsp[(1) - (2)].node);
      (yyval.node)->link.insert_before(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 335:
#line 2183 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_expression_statement(NULL);
      (yyval.node)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 336:
#line 2189 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_expression_statement((yyvsp[(1) - (2)].expression));
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 337:
#line 2198 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = new(state) ast_selection_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].selection_rest_statement).then_statement,
                                              (yyvsp[(5) - (5)].selection_rest_statement).else_statement);
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 338:
#line 2207 "src/glsl/glsl_parser.yy"
    {
      (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (3)].node);
      (yyval.selection_rest_statement).else_statement = (yyvsp[(3) - (3)].node);
   ;}
    break;

  case 339:
#line 2212 "src/glsl/glsl_parser.yy"
    {
      (yyval.selection_rest_statement).then_statement = (yyvsp[(1) - (1)].node);
      (yyval.selection_rest_statement).else_statement = NULL;
   ;}
    break;

  case 340:
#line 2220 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = (ast_node *) (yyvsp[(1) - (1)].expression);
   ;}
    break;

  case 341:
#line 2224 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[(2) - (4)].identifier), NULL, (yyvsp[(4) - (4)].expression));
      ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[(1) - (4)].fully_specified_type));
      decl->set_location_range((yylsp[(2) - (4)]), (yylsp[(4) - (4)]));
      declarator->set_location((yylsp[(1) - (4)]));

      declarator->declarations.push_tail(&decl->link);
      (yyval.node) = declarator;
   ;}
    break;

  case 342:
#line 2242 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = new(state) ast_switch_statement((yyvsp[(3) - (5)].expression), (yyvsp[(5) - (5)].switch_body));
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(5) - (5)]));
   ;}
    break;

  case 343:
#line 2250 "src/glsl/glsl_parser.yy"
    {
      (yyval.switch_body) = new(state) ast_switch_body(NULL);
      (yyval.switch_body)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 344:
#line 2255 "src/glsl/glsl_parser.yy"
    {
      (yyval.switch_body) = new(state) ast_switch_body((yyvsp[(2) - (3)].case_statement_list));
      (yyval.switch_body)->set_location_range((yylsp[(1) - (3)]), (yylsp[(3) - (3)]));
   ;}
    break;

  case 345:
#line 2263 "src/glsl/glsl_parser.yy"
    {
      (yyval.case_label) = new(state) ast_case_label((yyvsp[(2) - (3)].expression));
      (yyval.case_label)->set_location((yylsp[(2) - (3)]));
   ;}
    break;

  case 346:
#line 2268 "src/glsl/glsl_parser.yy"
    {
      (yyval.case_label) = new(state) ast_case_label(NULL);
      (yyval.case_label)->set_location((yylsp[(2) - (2)]));
   ;}
    break;

  case 347:
#line 2276 "src/glsl/glsl_parser.yy"
    {
      ast_case_label_list *labels = new(state) ast_case_label_list();

      labels->labels.push_tail(& (yyvsp[(1) - (1)].case_label)->link);
      (yyval.case_label_list) = labels;
      (yyval.case_label_list)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 348:
#line 2284 "src/glsl/glsl_parser.yy"
    {
      (yyval.case_label_list) = (yyvsp[(1) - (2)].case_label_list);
      (yyval.case_label_list)->labels.push_tail(& (yyvsp[(2) - (2)].case_label)->link);
   ;}
    break;

  case 349:
#line 2292 "src/glsl/glsl_parser.yy"
    {
      ast_case_statement *stmts = new(state) ast_case_statement((yyvsp[(1) - (2)].case_label_list));
      stmts->set_location((yylsp[(2) - (2)]));

      stmts->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
      (yyval.case_statement) = stmts;
   ;}
    break;

  case 350:
#line 2300 "src/glsl/glsl_parser.yy"
    {
      (yyval.case_statement) = (yyvsp[(1) - (2)].case_statement);
      (yyval.case_statement)->stmts.push_tail(& (yyvsp[(2) - (2)].node)->link);
   ;}
    break;

  case 351:
#line 2308 "src/glsl/glsl_parser.yy"
    {
      ast_case_statement_list *cases= new(state) ast_case_statement_list();
      cases->set_location((yylsp[(1) - (1)]));

      cases->cases.push_tail(& (yyvsp[(1) - (1)].case_statement)->link);
      (yyval.case_statement_list) = cases;
   ;}
    break;

  case 352:
#line 2316 "src/glsl/glsl_parser.yy"
    {
      (yyval.case_statement_list) = (yyvsp[(1) - (2)].case_statement_list);
      (yyval.case_statement_list)->cases.push_tail(& (yyvsp[(2) - (2)].case_statement)->link);
   ;}
    break;

  case 353:
#line 2324 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
                                            NULL, (yyvsp[(3) - (5)].node), NULL, (yyvsp[(5) - (5)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (5)]), (yylsp[(4) - (5)]));
   ;}
    break;

  case 354:
#line 2331 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
                                            NULL, (yyvsp[(5) - (7)].expression), NULL, (yyvsp[(2) - (7)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (7)]), (yylsp[(6) - (7)]));
   ;}
    break;

  case 355:
#line 2338 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
                                            (yyvsp[(3) - (6)].node), (yyvsp[(4) - (6)].for_rest_statement).cond, (yyvsp[(4) - (6)].for_rest_statement).rest, (yyvsp[(6) - (6)].node));
      (yyval.node)->set_location_range((yylsp[(1) - (6)]), (yylsp[(6) - (6)]));
   ;}
    break;

  case 359:
#line 2354 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
   ;}
    break;

  case 360:
#line 2361 "src/glsl/glsl_parser.yy"
    {
      (yyval.for_rest_statement).cond = (yyvsp[(1) - (2)].node);
      (yyval.for_rest_statement).rest = NULL;
   ;}
    break;

  case 361:
#line 2366 "src/glsl/glsl_parser.yy"
    {
      (yyval.for_rest_statement).cond = (yyvsp[(1) - (3)].node);
      (yyval.for_rest_statement).rest = (yyvsp[(3) - (3)].expression);
   ;}
    break;

  case 362:
#line 2375 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 363:
#line 2381 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 364:
#line 2387 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 365:
#line 2393 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[(2) - (3)].expression));
      (yyval.node)->set_location_range((yylsp[(1) - (3)]), (yylsp[(2) - (3)]));
   ;}
    break;

  case 366:
#line 2399 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
      (yyval.node)->set_location((yylsp[(1) - (2)]));
   ;}
    break;

  case 367:
#line 2407 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].function_definition); ;}
    break;

  case 368:
#line 2408 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 369:
#line 2409 "src/glsl/glsl_parser.yy"
    { (yyval.node) = NULL; ;}
    break;

  case 370:
#line 2410 "src/glsl/glsl_parser.yy"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 371:
#line 2415 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
      (yyval.function_definition) = new(ctx) ast_function_definition();
      (yyval.function_definition)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
      (yyval.function_definition)->prototype = (yyvsp[(1) - (2)].function);
      (yyval.function_definition)->body = (yyvsp[(2) - (2)].compound_statement);

      state->symbols->pop_scope();
   ;}
    break;

  case 372:
#line 2429 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = (yyvsp[(1) - (1)].interface_block);
   ;}
    break;

  case 373:
#line 2433 "src/glsl/glsl_parser.yy"
    {
      ast_interface_block *block = (yyvsp[(2) - (2)].interface_block);
      if (!block->layout.merge_qualifier(& (yylsp[(1) - (2)]), state, (yyvsp[(1) - (2)].type_qualifier))) {
         YYERROR;
      }

      foreach_list_typed (ast_declarator_list, member, link, &block->declarations) {
         ast_type_qualifier& qualifier = member->type->qualifier;
         if (qualifier.flags.q.stream && qualifier.stream != block->layout.stream) {
               _mesa_glsl_error(& (yylsp[(1) - (2)]), state,
                             "stream layout qualifier on "
                             "interface block member does not match "
                             "the interface block (%d vs %d)",
                             qualifier.stream, block->layout.stream);
               YYERROR;
         }
      }
      (yyval.node) = block;
   ;}
    break;

  case 374:
#line 2456 "src/glsl/glsl_parser.yy"
    {
      ast_interface_block *const block = (yyvsp[(6) - (7)].interface_block);

      block->block_name = (yyvsp[(2) - (7)].identifier);
      block->declarations.push_degenerate_list_at_head(& (yyvsp[(4) - (7)].declarator_list)->link);

      if ((yyvsp[(1) - (7)].type_qualifier).flags.q.uniform) {
         if (!state->has_uniform_buffer_objects()) {
            _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
                             "#version 140 / GL_ARB_uniform_buffer_object "
                             "required for defining uniform blocks");
         } else if (state->ARB_uniform_buffer_object_warn) {
            _mesa_glsl_warning(& (yylsp[(1) - (7)]), state,
                               "#version 140 / GL_ARB_uniform_buffer_object "
                               "required for defining uniform blocks");
         }
      } else {
         if (state->es_shader || state->language_version < 150) {
            _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
                             "#version 150 required for using "
                             "interface blocks");
         }
      }

      /* From the GLSL 1.50.11 spec, section 4.3.7 ("Interface Blocks"):
       * "It is illegal to have an input block in a vertex shader
       *  or an output block in a fragment shader"
       */
      if ((state->stage == MESA_SHADER_VERTEX) && (yyvsp[(1) - (7)].type_qualifier).flags.q.in) {
         _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
                          "`in' interface block is not allowed for "
                          "a vertex shader");
      } else if ((state->stage == MESA_SHADER_FRAGMENT) && (yyvsp[(1) - (7)].type_qualifier).flags.q.out) {
         _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
                          "`out' interface block is not allowed for "
                          "a fragment shader");
      }

      /* Since block arrays require names, and both features are added in
       * the same language versions, we don't have to explicitly
       * version-check both things.
       */
      if (block->instance_name != NULL) {
         state->check_version(150, 300, & (yylsp[(1) - (7)]), "interface blocks with "
                               "an instance name are not allowed");
      }

      uint64_t interface_type_mask;
      struct ast_type_qualifier temp_type_qualifier;

      /* Get a bitmask containing only the in/out/uniform flags, allowing us
       * to ignore other irrelevant flags like interpolation qualifiers.
       */
      temp_type_qualifier.flags.i = 0;
      temp_type_qualifier.flags.q.uniform = true;
      temp_type_qualifier.flags.q.in = true;
      temp_type_qualifier.flags.q.out = true;
      interface_type_mask = temp_type_qualifier.flags.i;

      /* Get the block's interface qualifier.  The interface_qualifier
       * production rule guarantees that only one bit will be set (and
       * it will be in/out/uniform).
       */
      uint64_t block_interface_qualifier = (yyvsp[(1) - (7)].type_qualifier).flags.i;

      block->layout.flags.i |= block_interface_qualifier;

      if (state->stage == MESA_SHADER_GEOMETRY &&
          state->has_explicit_attrib_stream()) {
         /* Assign global layout's stream value. */
         block->layout.flags.q.stream = 1;
         block->layout.flags.q.explicit_stream = 0;
         block->layout.stream = state->out_qualifier->stream;
      }

      foreach_list_typed (ast_declarator_list, member, link, &block->declarations) {
         ast_type_qualifier& qualifier = member->type->qualifier;
         if ((qualifier.flags.i & interface_type_mask) == 0) {
            /* GLSLangSpec.1.50.11, 4.3.7 (Interface Blocks):
             * "If no optional qualifier is used in a member declaration, the
             *  qualifier of the variable is just in, out, or uniform as declared
             *  by interface-qualifier."
             */
            qualifier.flags.i |= block_interface_qualifier;
         } else if ((qualifier.flags.i & interface_type_mask) !=
                    block_interface_qualifier) {
            /* GLSLangSpec.1.50.11, 4.3.7 (Interface Blocks):
             * "If optional qualifiers are used, they can include interpolation
             *  and storage qualifiers and they must declare an input, output,
             *  or uniform variable consistent with the interface qualifier of
             *  the block."
             */
            _mesa_glsl_error(& (yylsp[(1) - (7)]), state,
                             "uniform/in/out qualifier on "
                             "interface block member does not match "
                             "the interface block");
         }
      }

      (yyval.interface_block) = block;
   ;}
    break;

  case 375:
#line 2561 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.in = 1;
   ;}
    break;

  case 376:
#line 2567 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.out = 1;
   ;}
    break;

  case 377:
#line 2573 "src/glsl/glsl_parser.yy"
    {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
	  (yyval.type_qualifier).precision = ast_precision_none;
      (yyval.type_qualifier).flags.q.uniform = 1;
   ;}
    break;

  case 378:
#line 2582 "src/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state) ast_interface_block(*state->default_uniform_qualifier,
                                          NULL, NULL);
   ;}
    break;

  case 379:
#line 2587 "src/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state) ast_interface_block(*state->default_uniform_qualifier,
                                          (yyvsp[(1) - (1)].identifier), NULL);
      (yyval.interface_block)->set_location((yylsp[(1) - (1)]));
   ;}
    break;

  case 380:
#line 2593 "src/glsl/glsl_parser.yy"
    {
      (yyval.interface_block) = new(state) ast_interface_block(*state->default_uniform_qualifier,
                                          (yyvsp[(1) - (2)].identifier), (yyvsp[(2) - (2)].array_specifier));
      (yyval.interface_block)->set_location_range((yylsp[(1) - (2)]), (yylsp[(2) - (2)]));
   ;}
    break;

  case 381:
#line 2602 "src/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (1)].declarator_list);
      (yyvsp[(1) - (1)].declarator_list)->link.self_link();
   ;}
    break;

  case 382:
#line 2607 "src/glsl/glsl_parser.yy"
    {
      (yyval.declarator_list) = (yyvsp[(1) - (2)].declarator_list);
      (yyvsp[(2) - (2)].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
   ;}
    break;

  case 383:
#line 2615 "src/glsl/glsl_parser.yy"
    {
      void *ctx = state;
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

  case 384:
#line 2639 "src/glsl/glsl_parser.yy"
    {
      if (!state->default_uniform_qualifier->merge_qualifier(& (yylsp[(1) - (3)]), state, (yyvsp[(1) - (3)].type_qualifier))) {
         YYERROR;
      }
      (yyval.node) = NULL;
   ;}
    break;

  case 385:
#line 2647 "src/glsl/glsl_parser.yy"
    {
      (yyval.node) = NULL;
      if (!state->in_qualifier->merge_in_qualifier(& (yylsp[(1) - (3)]), state, (yyvsp[(1) - (3)].type_qualifier), (yyval.node))) {
         YYERROR;
      }
   ;}
    break;

  case 386:
#line 2655 "src/glsl/glsl_parser.yy"
    {
      if (state->stage != MESA_SHADER_GEOMETRY) {
         _mesa_glsl_error(& (yylsp[(1) - (3)]), state,
                          "out layout qualifiers only valid in "
                          "geometry shaders");
      } else {
         if ((yyvsp[(1) - (3)].type_qualifier).flags.q.prim_type) {
            /* Make sure this is a valid output primitive type. */
            switch ((yyvsp[(1) - (3)].type_qualifier).prim_type) {
            case GL_POINTS:
            case GL_LINE_STRIP:
            case GL_TRIANGLE_STRIP:
               break;
            default:
               _mesa_glsl_error(&(yylsp[(1) - (3)]), state, "invalid geometry shader output "
                                "primitive type");
               break;
            }
         }
         if (!state->out_qualifier->merge_qualifier(& (yylsp[(1) - (3)]), state, (yyvsp[(1) - (3)].type_qualifier)))
            YYERROR;

         /* Allow future assigments of global out's stream id value */
         state->out_qualifier->flags.q.explicit_stream = 0;
      }
      (yyval.node) = NULL;
   ;}
    break;


/* Line 1267 of yacc.c.  */
#line 6638 "src/glsl/glsl_parser.cpp"
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



