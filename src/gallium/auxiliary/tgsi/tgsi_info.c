/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "util/u_debug.h"
#include "tgsi_info.h"

static const struct tgsi_opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
   { 1, 1, 0, 0, "ARL", NULL, NULL },
   { 1, 1, 0, 0, "MOV", NULL, NULL },
   { 1, 1, 0, 0, "LIT", NULL, NULL },
   { 1, 1, 0, 0, "RCP", "RECIP", NULL },
   { 1, 1, 0, 0, "RSQ", "RECIPSQRT", NULL },
   { 1, 1, 0, 0, "EXP", "EXPP", NULL },
   { 1, 1, 0, 0, "LOG", NULL, NULL },
   { 1, 2, 0, 0, "MUL", NULL, NULL },
   { 1, 2, 0, 0, "ADD", NULL, NULL },
   { 1, 2, 0, 0, "DP3", "DOT3", NULL },
   { 1, 2, 0, 0, "DP4", "DOT4", NULL },
   { 1, 2, 0, 0, "DST", NULL, NULL },
   { 1, 2, 0, 0, "MIN", NULL, NULL },
   { 1, 2, 0, 0, "MAX", NULL, NULL },
   { 1, 2, 0, 0, "SLT", "SETLT", NULL },
   { 1, 2, 0, 0, "SGE", "SETGE", NULL },
   { 1, 3, 0, 0, "MAD", "MADD", NULL },
   { 1, 2, 0, 0, "SUB", NULL, NULL },
   { 1, 3, 0, 0, "LRP", "LERP", NULL },
   { 1, 3, 0, 0, "CND", NULL, NULL },
   { 1, 3, 0, 0, "CND0", NULL, NULL },
   { 1, 3, 0, 0, "DP2A", "DP2ADD", "DOT2ADD" },
   { 1, 2, 0, 0, "INDEX", NULL, NULL },
   { 1, 1, 0, 0, "NEGATE", NULL, NULL },
   { 1, 1, 0, 0, "FRC", "FRAC", NULL },
   { 1, 3, 0, 0, "CLAMP", NULL, NULL },
   { 1, 1, 0, 0, "FLR", "FLOOR", NULL },
   { 1, 1, 0, 0, "ROUND", NULL, NULL },
   { 1, 1, 0, 0, "EX2", "EXPBASE2", NULL },
   { 1, 1, 0, 0, "LG2", "LOGBASE2", "LOGP" },
   { 1, 2, 0, 0, "POW", "POWER", NULL },
   { 1, 2, 0, 0, "XPD", "CRS", "CROSSPRODUCT" },
   { 1, 2, 0, 0, "M4X4", "MULTIPLYMATRIX", NULL },
   { 1, 1, 0, 0, "ABS", NULL, NULL },
   { 1, 1, 0, 0, "RCC", NULL, NULL },
   { 1, 2, 0, 0, "DPH", NULL, NULL },
   { 1, 1, 0, 0, "COS", NULL, NULL },
   { 1, 1, 0, 0, "DDX", "DSX", NULL },
   { 1, 1, 0, 0, "DDY", "DSY", NULL },
   { 0, 0, 0, 0, "KILP", NULL, NULL },
   { 1, 1, 0, 0, "PK2H", NULL, NULL },
   { 1, 1, 0, 0, "PK2US", NULL, NULL },
   { 1, 1, 0, 0, "PK4B", NULL, NULL },
   { 1, 1, 0, 0, "PK4UB", NULL, NULL },
   { 1, 2, 0, 0, "RFL", NULL, NULL },
   { 1, 2, 0, 0, "SEQ", NULL, NULL },
   { 1, 2, 0, 0, "SFL", NULL, NULL },
   { 1, 2, 0, 0, "SGT", NULL, NULL },
   { 1, 1, 0, 0, "SIN", NULL, NULL },
   { 1, 2, 0, 0, "SLE", NULL, NULL },
   { 1, 2, 0, 0, "SNE", NULL, NULL },
   { 1, 2, 0, 0, "STR", NULL, NULL },
   { 1, 2, 1, 0, "TEX", "TEXLD", NULL },
   { 1, 4, 1, 0, "TXD", "TEXLDD", NULL },
   { 1, 2, 1, 0, "TXP", NULL, NULL },
   { 1, 1, 0, 0, "UP2H", NULL, NULL },
   { 1, 1, 0, 0, "UP2US", NULL, NULL },
   { 1, 1, 0, 0, "UP4B", NULL, NULL },
   { 1, 1, 0, 0, "UP4UB", NULL, NULL },
   { 1, 3, 0, 0, "X2D", NULL, NULL },
   { 1, 1, 0, 0, "ARA", NULL, NULL },
   { 1, 1, 0, 0, "ARR", "MOVA", NULL },
   { 0, 1, 0, 0, "BRA", NULL, NULL },
   { 0, 0, 0, 1, "CAL", "CALL", NULL },
   { 0, 0, 0, 0, "RET", NULL, NULL },
   { 1, 1, 0, 0, "SGN", "SSG", NULL },
   { 1, 3, 0, 0, "CMP", NULL, NULL },
   { 1, 1, 0, 0, "SCS", "SINCOS", NULL },
   { 1, 2, 1, 0, "TXB", "TEXLDB", NULL },
   { 1, 1, 0, 0, "NRM", NULL, NULL },
   { 1, 2, 0, 0, "DIV", NULL, NULL },
   { 1, 2, 0, 0, "DP2", NULL, NULL },
   { 1, 2, 1, 0, "TXL", NULL, NULL },
   { 0, 0, 0, 0, "BRK", "BREAK", NULL },
   { 0, 1, 0, 1, "IF", NULL, NULL },
   { 0, 0, 0, 0, "LOOP", NULL, NULL },
   { 0, 1, 0, 0, "REP", NULL, NULL },
   { 0, 0, 0, 1, "ELSE", NULL, NULL },
   { 0, 0, 0, 0, "ENDIF", NULL, NULL },
   { 0, 0, 0, 0, "ENDLOOP", NULL, NULL },
   { 0, 0, 0, 0, "ENDREP", NULL, NULL },
   { 0, 1, 0, 0, "PUSHA", NULL, NULL },
   { 1, 0, 0, 0, "POPA", NULL, NULL },
   { 1, 1, 0, 0, "CEIL", NULL, NULL },
   { 1, 1, 0, 0, "I2F", NULL, NULL },
   { 1, 1, 0, 0, "NOT", NULL, NULL },
   { 1, 1, 0, 0, "INT", "TRUNC", NULL },
   { 1, 2, 0, 0, "SHL", NULL, NULL },
   { 1, 2, 0, 0, "SHR", NULL, NULL },
   { 1, 2, 0, 0, "AND", NULL, NULL },
   { 1, 2, 0, 0, "OR", NULL, NULL },
   { 1, 2, 0, 0, "MOD", NULL, NULL },
   { 1, 2, 0, 0, "XOR", NULL, NULL },
   { 1, 3, 0, 0, "SAD", NULL, NULL },
   { 1, 2, 1, 0, "TXF", NULL, NULL },
   { 1, 2, 1, 0, "TXQ", NULL, NULL },
   { 0, 0, 0, 0, "CONT", NULL, NULL },
   { 0, 0, 0, 0, "EMIT", NULL, NULL },
   { 0, 0, 0, 0, "ENDPRIM", NULL, NULL },
   { 0, 0, 0, 1, "BGNLOOP2", NULL, NULL },
   { 0, 0, 0, 0, "BGNSUB", NULL, NULL },
   { 0, 0, 0, 1, "ENDLOOP2", NULL, NULL },
   { 0, 0, 0, 0, "ENDSUB", NULL, NULL },
   { 1, 1, 0, 0, "NOISE1", NULL, NULL },
   { 1, 1, 0, 0, "NOISE2", NULL, NULL },
   { 1, 1, 0, 0, "NOISE3", NULL, NULL },
   { 1, 1, 0, 0, "NOISE4", NULL, NULL },
   { 0, 0, 0, 0, "NOP", NULL, NULL },
   { 1, 2, 0, 0, "M4X3", NULL, NULL },
   { 1, 2, 0, 0, "M3X4", NULL, NULL },
   { 1, 2, 0, 0, "M3X3", NULL, NULL },
   { 1, 2, 0, 0, "M3X2", NULL, NULL },
   { 1, 1, 0, 0, "NRM4", NULL, NULL },
   { 0, 1, 0, 0, "CALLNZ", NULL, NULL },
   { 0, 1, 0, 0, "IFC", NULL, NULL },
   { 0, 1, 0, 0, "BREAKC", NULL, NULL },
   { 0, 1, 0, 0, "KIL", "TEXKILL", NULL },
   { 0, 0, 0, 0, "END", NULL, NULL },
   { 1, 1, 0, 0, "SWZ", NULL, NULL }
};

const struct tgsi_opcode_info *
tgsi_get_opcode_info( uint opcode )
{
   if (opcode < TGSI_OPCODE_LAST)
      return &opcode_info[opcode];
   assert( 0 );
   return NULL;
}
