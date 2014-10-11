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


