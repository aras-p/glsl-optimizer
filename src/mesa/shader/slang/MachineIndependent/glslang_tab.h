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
/* Line 1240 of yacc.c.  */
#line 251 "glslang.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





