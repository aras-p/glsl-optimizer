/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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
/* Line 1529 of yacc.c.  */
#line 366 "src/compiler/glsl/glsl_parser.h"
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


