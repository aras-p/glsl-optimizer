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
/* Line 1529 of yacc.c.  */
#line 550 "src/glsl/glsl_parser.h"
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


