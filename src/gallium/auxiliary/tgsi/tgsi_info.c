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

#include "pipe/p_debug.h"
#include "tgsi_info.h"

static const struct tgsi_opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
   { 1, 1, 0, 0, "ARL" },
   { 1, 1, 0, 0, "MOV" },
   { 1, 1, 0, 0, "LIT" },
   { 1, 1, 0, 0, "RCP" },
   { 1, 1, 0, 0, "RSQ" },
   { 1, 1, 0, 0, "EXP" },
   { 1, 1, 0, 0, "LOG" },
   { 1, 2, 0, 0, "MUL" },
   { 1, 2, 0, 0, "ADD" },
   { 1, 2, 0, 0, "DP3" },
   { 1, 2, 0, 0, "DP4" },
   { 1, 2, 0, 0, "DST" },
   { 1, 2, 0, 0, "MIN" },
   { 1, 2, 0, 0, "MAX" },
   { 1, 2, 0, 0, "SLT" },
   { 1, 2, 0, 0, "SGE" },
   { 1, 3, 0, 0, "MAD" },
   { 1, 2, 0, 0, "SUB" },
   { 1, 3, 0, 0, "LERP" },
   { 1, 3, 0, 0, "CND" },
   { 1, 3, 0, 0, "CND0" },
   { 1, 3, 0, 0, "DOT2ADD" },
   { 1, 2, 0, 0, "INDEX" },
   { 1, 1, 0, 0, "NEGATE" },
   { 1, 1, 0, 0, "FRAC" },
   { 1, 3, 0, 0, "CLAMP" },
   { 1, 1, 0, 0, "FLOOR" },
   { 1, 1, 0, 0, "ROUND" },
   { 1, 1, 0, 0, "EXPBASE2" },
   { 1, 1, 0, 0, "LOGBASE2" },
   { 1, 2, 0, 0, "POWER" },
   { 1, 2, 0, 0, "CROSSPRODUCT" },
   { 1, 2, 0, 0, "MULTIPLYMATRIX" },
   { 1, 1, 0, 0, "ABS" },
   { 1, 1, 0, 0, "RCC" },
   { 1, 2, 0, 0, "DPH" },
   { 1, 1, 0, 0, "COS" },
   { 1, 1, 0, 0, "DDX" },
   { 1, 1, 0, 0, "DDY" },
   { 0, 1, 0, 0, "KILP" },
   { 1, 1, 0, 0, "PK2H" },
   { 1, 1, 0, 0, "PK2US" },
   { 1, 1, 0, 0, "PK4B" },
   { 1, 1, 0, 0, "PK4UB" },
   { 1, 2, 0, 0, "RFL" },
   { 1, 2, 0, 0, "SEQ" },
   { 1, 2, 0, 0, "SFL" },
   { 1, 2, 0, 0, "SGT" },
   { 1, 1, 0, 0, "SIN" },
   { 1, 2, 0, 0, "SLE" },
   { 1, 2, 0, 0, "SNE" },
   { 1, 2, 0, 0, "STR" },
   { 1, 2, 1, 0, "TEX" },
   { 1, 4, 1, 0, "TXD" },
   { 1, 2, 1, 0, "TXP" },
   { 1, 1, 0, 0, "UP2H" },
   { 1, 1, 0, 0, "UP2US" },
   { 1, 1, 0, 0, "UP4B" },
   { 1, 1, 0, 0, "UP4UB" },
   { 1, 3, 0, 0, "X2D" },
   { 1, 1, 0, 0, "ARA" },
   { 1, 1, 0, 0, "ARR" },
   { 0, 1, 0, 0, "BRA" },
   { 0, 0, 0, 1, "CAL" },
   { 0, 0, 0, 0, "RET" },
   { 1, 1, 0, 0, "SSG" },
   { 1, 3, 0, 0, "CMP" },
   { 1, 1, 0, 0, "SCS" },
   { 1, 2, 1, 0, "TXB" },
   { 1, 1, 0, 0, "NRM" },
   { 1, 2, 0, 0, "DIV" },
   { 1, 2, 0, 0, "DP2" },
   { 1, 2, 1, 0, "TXL" },
   { 0, 0, 0, 0, "BRK" },
   { 0, 1, 0, 1, "IF" },
   { 0, 0, 0, 0, "LOOP" },
   { 0, 1, 0, 0, "REP" },
   { 0, 0, 0, 1, "ELSE" },
   { 0, 0, 0, 0, "ENDIF" },
   { 0, 0, 0, 0, "ENDLOOP" },
   { 0, 0, 0, 0, "ENDREP" },
   { 0, 1, 0, 0, "PUSHA" },
   { 1, 0, 0, 0, "POPA" },
   { 1, 1, 0, 0, "CEIL" },
   { 1, 1, 0, 0, "I2F" },
   { 1, 1, 0, 0, "NOT" },
   { 1, 1, 0, 0, "TRUNC" },
   { 1, 2, 0, 0, "SHL" },
   { 1, 2, 0, 0, "SHR" },
   { 1, 2, 0, 0, "AND" },
   { 1, 2, 0, 0, "OR" },
   { 1, 2, 0, 0, "MOD" },
   { 1, 2, 0, 0, "XOR" },
   { 1, 3, 0, 0, "SAD" },
   { 1, 2, 1, 0, "TXF" },
   { 1, 2, 1, 0, "TXQ" },
   { 0, 0, 0, 0, "CONT" },
   { 0, 0, 0, 0, "EMIT" },
   { 0, 0, 0, 0, "ENDPRIM" },
   { 0, 0, 0, 1, "BGNLOOP2" },
   { 0, 0, 0, 0, "BGNSUB" },
   { 0, 0, 0, 1, "ENDLOOP2" },
   { 0, 0, 0, 0, "ENDSUB" },
   { 1, 1, 0, 0, "NOISE1" },
   { 1, 1, 0, 0, "NOISE2" },
   { 1, 1, 0, 0, "NOISE3" },
   { 1, 1, 0, 0, "NOISE4" },
   { 0, 0, 0, 0, "NOP" },
   { 1, 2, 0, 0, "M4X3" },
   { 1, 2, 0, 0, "M3X4" },
   { 1, 2, 0, 0, "M3X3" },
   { 1, 2, 0, 0, "M3X2" },
   { 1, 1, 0, 0, "NRM4" },
   { 0, 1, 0, 0, "CALLNZ" },
   { 0, 1, 0, 0, "IFC" },
   { 0, 1, 0, 0, "BREAKC" },
   { 0, 0, 0, 0, "KIL" },
   { 0, 0, 0, 0, "END" },
   { 1, 1, 0, 0, "SWZ" }
};

const struct tgsi_opcode_info *
tgsi_get_opcode_info( uint opcode )
{
   if (opcode < TGSI_OPCODE_LAST)
      return &opcode_info[opcode];
   assert( 0 );
   return NULL;
}
