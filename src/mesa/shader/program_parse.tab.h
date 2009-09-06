
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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
     POINT_TOK = 312,
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
     SIZE_TOK = 324,
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
     TEX_ARRAY1D = 346,
     TEX_ARRAY2D = 347,
     TEX_ARRAYSHADOW1D = 348,
     TEX_ARRAYSHADOW2D = 349,
     VERTEX = 350,
     VTXATTRIB = 351,
     WEIGHT = 352,
     IDENTIFIER = 353,
     MASK4 = 354,
     MASK3 = 355,
     MASK2 = 356,
     MASK1 = 357,
     SWIZZLE = 358,
     DOT_DOT = 359,
     DOT = 360
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
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
   gl_state_index state[STATE_LENGTH];
   int negate;
   struct asm_vector vector;
   gl_inst_opcode opcode;

   struct {
      unsigned swz;
      unsigned rgba_valid:1;
      unsigned xyzw_valid:1;
      unsigned negate:1;
   } ext_swizzle;



/* Line 1676 of yacc.c  */
#line 185 "program_parse.tab.h"
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



