/* $Id: nvfragparse.c,v 1.3 2003/02/08 15:56:34 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file nvfragparse.c
 * \brief NVIDIA fragment program parser.
 * \author Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mmath.h"
#include "mtypes.h"
#include "nvfragprog.h"
#include "nvfragparse.h"
#include "nvprogram.h"


#define FRAG_ATTRIB_WPOS  0
#define FRAG_ATTRIB_COL0  1
#define FRAG_ATTRIB_COL1  2
#define FRAG_ATTRIB_FOGC  3
#define FRAG_ATTRIB_TEX0  4
#define FRAG_ATTRIB_TEX1  5
#define FRAG_ATTRIB_TEX2  6
#define FRAG_ATTRIB_TEX3  7
#define FRAG_ATTRIB_TEX4  8
#define FRAG_ATTRIB_TEX5  9
#define FRAG_ATTRIB_TEX6  10
#define FRAG_ATTRIB_TEX7  11


#define INPUT_1V     1
#define INPUT_2V     2
#define INPUT_3V     3
#define INPUT_1S     4
#define INPUT_2S     5
#define INPUT_CC     6
#define INPUT_1V_T   7  /* one source vector, plus textureId */
#define INPUT_3V_T   8  /* one source vector, plus textureId */
#define INPUT_NONE   9
#define OUTPUT_V    20
#define OUTPUT_S    21
#define OUTPUT_NONE 22

/* Optional suffixes */
#define _R  0x01  /* real */
#define _H  0x02  /* half */
#define _X  0x04  /* fixed */
#define _C  0x08  /* set cond codes */
#define _S  0x10  /* saturate */

#define SINGLE  _R
#define HALF    _H
#define FIXED   _X

struct instruction_pattern {
   const char *name;
   enum fp_opcode opcode;
   GLuint inputs;
   GLuint outputs;
   GLuint suffixes;
};

static const struct instruction_pattern Instructions[] = {
   { "ADD", FP_OPCODE_ADD, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "COS", FP_OPCODE_COS, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "DDX", FP_OPCODE_DDX, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "DDY", FP_OPCODE_DDY, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "DP3", FP_OPCODE_DP3, INPUT_2V, OUTPUT_S, _R | _H | _X | _C | _S },
   { "DP4", FP_OPCODE_DP4, INPUT_2V, OUTPUT_S, _R | _H | _X | _C | _S },
   { "DST", FP_OPCODE_DP4, INPUT_2V, OUTPUT_V, _R | _H |      _C | _S },
   { "EX2", FP_OPCODE_DP4, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "FLR", FP_OPCODE_FLR, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "FRC", FP_OPCODE_FRC, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "KIL", FP_OPCODE_KIL, INPUT_CC, OUTPUT_NONE, 0                   },
   { "LG2", FP_OPCODE_LG2, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "LIT", FP_OPCODE_LIT, INPUT_1V, OUTPUT_V, _R | _H |      _C | _S },
   { "LRP", FP_OPCODE_LRP, INPUT_3V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MAD", FP_OPCODE_MAD, INPUT_3V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MAX", FP_OPCODE_MAX, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MIN", FP_OPCODE_MIN, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MOV", FP_OPCODE_MOV, INPUT_1V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "MUL", FP_OPCODE_MUL, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "PK2H",  FP_OPCODE_PK2H,  INPUT_1V, OUTPUT_S, 0                  },
   { "PK2US", FP_OPCODE_PK2US, INPUT_1V, OUTPUT_S, 0                  },
   { "PK4B",  FP_OPCODE_PK4B,  INPUT_1V, OUTPUT_S, 0                  },
   { "PK2UB", FP_OPCODE_PK4UB, INPUT_1V, OUTPUT_S, 0                  },
   { "POW", FP_OPCODE_POW, INPUT_2S, OUTPUT_S, _R | _H |      _C | _S },
   { "RCP", FP_OPCODE_RCP, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "RFL", FP_OPCODE_RFL, INPUT_2V, OUTPUT_V, _R | _H |      _C | _S },
   { "RSQ", FP_OPCODE_RSQ, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "SEQ", FP_OPCODE_SEQ, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SFL", FP_OPCODE_SFL, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SGE", FP_OPCODE_SGE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SGT", FP_OPCODE_SGT, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SIN", FP_OPCODE_SIN, INPUT_1S, OUTPUT_S, _R | _H |      _C | _S },
   { "SLE", FP_OPCODE_SLE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SLT", FP_OPCODE_SLT, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SNE", FP_OPCODE_SNE, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "STR", FP_OPCODE_STR, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "SUB", FP_OPCODE_SUB, INPUT_2V, OUTPUT_V, _R | _H | _X | _C | _S },
   { "TEX", FP_OPCODE_SUB, INPUT_1V_T, OUTPUT_V,              _C | _S },
   { "TXD", FP_OPCODE_SUB, INPUT_3V_T, OUTPUT_V,              _C | _S },
   { "TXP", FP_OPCODE_SUB, INPUT_1V_T, OUTPUT_V,              _C | _S },
   { "UP2H",  FP_OPCODE_UP2H,  INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP2US", FP_OPCODE_UP2US, INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP4B",  FP_OPCODE_UP4B,  INPUT_1S, OUTPUT_V,            _C | _S },
   { "UP4UB", FP_OPCODE_UP4UB, INPUT_1S, OUTPUT_V,            _C | _S },
   { "X2D", FP_OPCODE_X2D, INPUT_3V, OUTPUT_V, _R | _H |      _C | _S },
   { NULL, -1, 0, 0, 0 }
};


/**********************************************************************/


struct parse_state {
   const GLubyte *start;    /* start of program */
   const GLubyte *end;      /* one char past end of the program */
   const GLubyte *s;        /* current position */
   GLboolean IsStateProgram;
   GLboolean IsVersion1_1;
};



/*
 * Search a list of instruction structures for a match.
 */
static struct instruction_pattern
MatchInstruction(const char *token)
{
   const struct instruction_pattern *inst;
   struct instruction_pattern result;

   for (inst = Instructions; inst->name; inst++) {
      if (_mesa_strncmp(token, inst->name, 3) == 0) {
         /* matched! */
         int i = 3;
         result = *inst;
         result.suffixes = 0;
         /* look at suffix */
         if (token[i] == 'R') {
            result.suffixes |= _R;
            i++;
         }
         else if (token[i] == 'H') {
            result.suffixes |= _H;
            i++;
         }
         else if (token[i] == 'X') {
            result.suffixes |= _X;
            i++;
         }
         if (token[i] == 'C') {
            result.suffixes |= _C;
            i++;
         }
         if (token[i] == '_' && token[i+1] == 'S' &&
             token[i+2] == 'A' && token[i+3] == 'T') {
            result.suffixes |= _S;
         }
         return result;
      }
   }
   result.opcode = (enum fp_opcode) -1;
   return result;
}




/**********************************************************************/


static GLboolean IsLetter(char b)
{
   return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b == '_');
}


static GLboolean IsDigit(char b)
{
   return b >= '0' && b <= '9';
}


static GLboolean IsWhitespace(char b)
{
   return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}


/**
 * Starting at 'str' find the next token.  A token can be an integer,
 * an identifier or punctuation symbol.
 * \return <= 0 we found an error, else, return number of characters parsed.
 */
static GLint
GetToken(const char *str, char *token)
{
   GLint i = 0, j = 0;

   token[0] = 0;

   /* skip whitespace and comments */
   while (str[i] && (IsWhitespace(str[i]) || str[i] == '#')) {
      if (str[i] == '#') {
         /* skip comment */
         while (str[i] && (str[i] != '\n' && str[i] != '\r')) {
            i++;
         }
      }
      else {
         /* skip whitespace */
         i++;
      }
   }

   if (str[i] == 0)
      return -i;

   /* try matching an integer */
   while (str[i] && IsDigit(str[i])) {
      token[j++] = str[i++];
   }
   if (j > 0 || !str[i]) {
      token[j] = 0;
      return i;
   }

   /* try matching an identifier */
   if (IsLetter(str[i])) {
      while (str[i] && (IsLetter(str[i]) || IsDigit(str[i]))) {
         token[j++] = str[i++];
      }
      token[j] = 0;
      return i;
   }

   /* punctuation */
   if (str[i]) {
      token[0] = str[i++];
      token[1] = 0;
      return i;
   }

   /* end of input */
   token[0] = 0;
   return i;
}


/**
 * Get next token from input stream and increment stream pointer past token.
 */
static GLboolean
Parse_Token(const char **s, char *token)
{
   GLint i;
   i = GetToken(*s, token);
   if (i <= 0) {
      *s += (-i);
      return GL_FALSE;
   }
   *s += i;
   return GL_TRUE;
}


/**
 * Get next token from input stream but don't increment stream pointer.
 */
static GLboolean
Peek_Token(const char **s, char *token)
{
   GLint i, len;
   i = GetToken(*s, token);
   if (i <= 0) {
      *s += (-i);
      return GL_FALSE;
   }
   len = _mesa_strlen(token);
   *s += (i - len);
   return GL_TRUE;
}


/**
 * String equality test
 */
static GLboolean
StrEq(const char *a, const char *b)
{
   GLint i;
   for (i = 0; a[i] && b[i] && a[i] == (char) b[i]; i++)
      ;
   if (a[i] == 0 && b[i] == 0)
      return GL_TRUE;
   else
      return GL_FALSE;
}



/**********************************************************************/

static const char *InputRegisters[] = {
   "WPOS", "COL0", "COL1", "FOGC",
   "TEX0", "TEX1", "TEX2", "TEX3", "TEX4", "TEX5", "TEX6", "TEX7", NULL
};

static const char *OutputRegisters[] = {
   "COLR", "COLH", "TEX0", "TEX1", "TEX2", "TEX3", "DEPR", NULL
};


#ifdef DEBUG

#define PARSE_ERROR						\
do {								\
   _mesa_printf("fpparse.c error at %d: parse error\n", __LINE__);	\
   return GL_FALSE;						\
} while(0)

#define PARSE_ERROR1(msg)					\
do {								\
   _mesa_printf("fpparse.c error at %d: %s\n", __LINE__, msg);	\
   return GL_FALSE;						\
} while(0)

#define PARSE_ERROR2(msg1, msg2)					\
do {									\
   _mesa_printf("fpparse.c error at %d: %s %s\n", __LINE__, msg1, msg2);\
   return GL_FALSE;							\
} while(0)

#else

#define PARSE_ERROR                return GL_FALSE
#define PARSE_ERROR1(msg1)         return GL_FALSE
#define PARSE_ERROR2(msg1, msg2)   return GL_FALSE

#endif


static GLint
TempRegisterNumber(GLuint r)
{
   if (r >= FP_TEMP_REG_START && r <= FP_TEMP_REG_END)
      return r - FP_TEMP_REG_START;
   else
      return -1;
}

static GLint
HalfTempRegisterNumber(GLuint r)
{
   if (r >= FP_TEMP_REG_START + 32 && r <= FP_TEMP_REG_END)
      return r - FP_TEMP_REG_START - 32;
   else
      return -1;
}

static GLint
InputRegisterNumber(GLuint r)
{
   if (r >= FP_INPUT_REG_START && r <= FP_INPUT_REG_END)
      return r - FP_INPUT_REG_START;
   else
      return -1;
}

static GLint
OutputRegisterNumber(GLuint r)
{
   if (r >= FP_OUTPUT_REG_START && r <= FP_OUTPUT_REG_END)
      return r - FP_OUTPUT_REG_START;
   else
      return -1;
}

static GLint
ProgramRegisterNumber(GLuint r)
{
   if (r >= FP_PROG_REG_START && r <= FP_PROG_REG_END)
      return r - FP_PROG_REG_START;
   else
      return -1;
}

static GLint
DummyRegisterNumber(GLuint r)
{
   if (r >= FP_DUMMY_REG_START && r <= FP_DUMMY_REG_END)
      return r - FP_DUMMY_REG_START;
   else
      return -1;
}



/**********************************************************************/


/**
 * Try to match 'pattern' as the next token after any whitespace/comments.
 */
static GLboolean
Parse_String(const char **s, const char *pattern)
{
   GLint i;

   /* skip whitespace and comments */
   while (IsWhitespace(**s) || **s == '#') {
      if (**s == '#') {
         while (**s && (**s != '\n' && **s != '\r')) {
            *s += 1;
         }
      }
      else {
         /* skip whitespace */
         *s += 1;
      }
   }

   /* Try to match the pattern */
   for (i = 0; pattern[i]; i++) {
      if (**s != pattern[i])
         PARSE_ERROR2("failed to match", pattern); /* failure */
      *s += 1;
   }

   return GL_TRUE; /* success */
}


static GLboolean
Parse_Identifier(const char **s, char *ident)
{
   if (!Parse_Token(s, ident))
      PARSE_ERROR;
   if (IsLetter(ident[0]))
      return GL_TRUE;
   else
      PARSE_ERROR1("Expected an identfier");
}


/**
 * Parse a floating point constant.
 * [+/-]N[.N[eN]]
 */
static GLboolean
Parse_ScalarConstant(const char **s, GLfloat *number)
{
   char *end;

   *number = (GLfloat) _mesa_strtod(*s, &end);

   if (end && end > *s) {
      /* got a number */
      *s = end;
      return GL_TRUE;
   }
   else {
      /* should be an identifier */
      char ident[100];
      if (!Parse_Identifier(s, ident))
         PARSE_ERROR1("Expected an identifier");
      /* XXX Look up the value in the symbol table */
      *number = -999;
      return GL_TRUE;
   }
}



/**
 * Parse a vector constant, one of:
 *   { float }
 *   { float, float }
 *   { float, float, float }
 *   { float, float, float, float }
 */
static GLboolean
Parse_VectorConstant(const char **s, GLfloat *vec)
{
   char token[100];

   if (!Parse_String(s, "{"))
      return GL_FALSE;

   if (!Parse_ScalarConstant(s, vec+0))  /* X */
      return GL_FALSE;

   if (!Parse_Token(s, token))  /* , or } */
      return GL_FALSE;

   if (token[0] == '}') {
      vec[1] = vec[2] = vec[3] = vec[0];
      return GL_TRUE;
   }

   if (token[0] != ',')
      PARSE_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(s, vec+1))  /* Y */
      return GL_FALSE;

   if (!Parse_Token(s, token))  /* , or } */
      return GL_FALSE;

   if (token[0] == '}') {
      vec[2] = vec[3] = vec[1];
      return GL_TRUE;
   }

   if (token[0] != ',')
      PARSE_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(s, vec+2))  /* Z */
      return GL_FALSE;

   if (!Parse_Token(s, token))  /* , or } */
      return GL_FALSE;

   if (token[0] == '}') {
      vec[3] = vec[2];
      return GL_TRUE;
   }

   if (token[0] != ',')
      PARSE_ERROR1("Expected comma in vector constant");

   if (!Parse_ScalarConstant(s, vec+3))  /* W */
      return GL_FALSE;

   if (!Parse_String(s, "}"))
      PARSE_ERROR1("Expected closing brace in vector constant");

   return GL_TRUE;
}


static GLboolean
Parse_VectorOrScalarConstant(const char **s, GLfloat *vec)
{
   char token[100];
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '{') {
      return Parse_VectorConstant(s, vec);
   }
   else {
      GLboolean b = Parse_ScalarConstant(s, vec);
      if (b) {
         vec[1] = vec[2] = vec[3] = vec[0];
      }
      return b;
   }
}


/**
 * Parse a texture image source:
 *    [TEX0 | TEX1 | .. | TEX15]
 *    [TEX0 | TEX1 | .. | TEX15] . [1D | 2D | 3D | CUBE | RECT]
 */
static GLboolean
Parse_TextureImageId(const char **s, GLuint *unit, GLenum *target)
{
   return GL_TRUE;
}


/**
 * Parse a swizzle suffix like .x or .z or .wxyz or .xxyy etc and return
 * the swizzle indexes.
 */
static GLboolean
Parse_SwizzleSuffix(const char *token, GLuint swizzle[4])
{
   if (token[1] == 0) {
      /* single letter swizzle (scalar) */
      if (token[0] == 'x')
         ASSIGN_4V(swizzle, 0, 0, 0, 0);
      else if (token[0] == 'y')
         ASSIGN_4V(swizzle, 1, 1, 1, 1);
      else if (token[0] == 'z')
         ASSIGN_4V(swizzle, 2, 2, 2, 2);
      else if (token[0] == 'w')
         ASSIGN_4V(swizzle, 3, 3, 3, 3);
      else
         return GL_FALSE;
   }
   else {
      /* 4-component swizzle (vector) */
      GLint k;
      for (k = 0; token[k] && k < 4; k++) {
         if (token[k] == 'x')
            swizzle[k] = 0;
         else if (token[k] == 'y')
            swizzle[k] = 1;
         else if (token[k] == 'z')
            swizzle[k] = 2;
         else if (token[k] == 'w')
            swizzle[k] = 3;
         else
            return GL_FALSE;
      }
      if (k != 4)
         return GL_FALSE;
   }
   return GL_TRUE;
}


static GLboolean
Parse_CondCodeMask(const char **s, struct fp_dst_register *dstReg)
{
   char token[100];

   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (StrEq(token, "EQ"))
      dstReg->CondMask = COND_EQ;
   else if (StrEq(token, "GE"))
      dstReg->CondMask = COND_GE;
   else if (StrEq(token, "GT"))
      dstReg->CondMask = COND_GT;
   else if (StrEq(token, "LE"))
      dstReg->CondMask = COND_LE;
   else if (StrEq(token, "LT"))
      dstReg->CondMask = COND_LT;
   else if (StrEq(token, "NE"))
      dstReg->CondMask = COND_NE;
   else if (StrEq(token, "TR"))
      dstReg->CondMask = COND_TR;
   else if (StrEq(token, "FL"))
      dstReg->CondMask = COND_FL;
   else
      PARSE_ERROR1("Invalid condition code mask");

   /* look for optional .xyzw swizzle */
   if (!Peek_Token(s, token))
      PARSE_ERROR;

   if (token[0] == '.') {
      Parse_String(s, "."); /* consume '.' */
      if (!Parse_Token(s, token))  /* get xyzw suffix */
         PARSE_ERROR;

      if (!Parse_SwizzleSuffix(token, dstReg->CondSwizzle))
         PARSE_ERROR1("Bad swizzle suffix");
   }

   return GL_TRUE;
}


/**
 * Parse a temporary register: Rnn or Hnn
 */
static GLboolean
Parse_TempReg(const char **s, GLint *tempRegNum)
{
   char token[100];

   /* Should be 'R##' or 'H##' */
   if (!Parse_Token(s, token))
      PARSE_ERROR;
   if (token[0] != 'R' && token[0] != 'H')
      PARSE_ERROR1("Expected R## or H##");

   if (IsDigit(token[1])) {
      GLint reg = _mesa_atoi((token + 1));
      if (token[0] == 'H')
         reg += 32;
      if (reg >= MAX_NV_FRAGMENT_PROGRAM_TEMPS)
         PARSE_ERROR1("Bad temporary register name");
      *tempRegNum = FP_TEMP_REG_START + reg;
   }
   else {
      PARSE_ERROR1("Bad temporary register name");
   }

   return GL_TRUE;
}


static GLboolean
Parse_DummyReg(const char **s, GLint *regNum)
{
   char token[100];

   /* Should be 'RC' or 'HC' */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (_mesa_strcmp(token, "RC")) {
       *regNum = FP_DUMMY_REG_START;
   }
   else if (_mesa_strcmp(token, "HC")) {
       *regNum = FP_DUMMY_REG_START + 1;
   }
   else {
      PARSE_ERROR1("Bad write-only register name");
   }

   return GL_TRUE;
}


/**
 * Parse a program local parameter register "p[##]"
 */
static GLboolean
Parse_ProgramParamReg(const char **s, GLint *regNum)
{
   char token[100];

   if (!Parse_String(s, "p"))
      PARSE_ERROR;

   if (!Parse_String(s, "["))
      PARSE_ERROR;

   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (IsDigit(token[0])) {
      /* a numbered program parameter register */
      GLint reg = _mesa_atoi(token);
      if (reg >= MAX_NV_FRAGMENT_PROGRAM_PARAMS)
         PARSE_ERROR1("Bad constant program number");
      *regNum = FP_PROG_REG_START + reg;
   }
   else {
      PARSE_ERROR;
   }

   if (!Parse_String(s, "]"))
      PARSE_ERROR;

   return GL_TRUE;
}


/**
 * Parse f[name]  - fragment input register
 */
static GLboolean
Parse_AttribReg(const char **s, GLint *tempRegNum)
{
   char token[100];
   GLint j;

   /* Match 'f' */
   if (!Parse_String(s, "f"))
      PARSE_ERROR;

   /* Match '[' */
   if (!Parse_String(s, "["))
      PARSE_ERROR;

   /* get <name> and look for match */
   if (!Parse_Token(s, token)) {
      PARSE_ERROR;
   }
   for (j = 0; InputRegisters[j]; j++) {
      if (StrEq(token, InputRegisters[j])) {
         *tempRegNum = FP_INPUT_REG_START + j;
         break;
      }
   }
   if (!InputRegisters[j]) {
      /* unknown input register label */
      PARSE_ERROR2("Bad register name", token);
   }

   /* Match '[' */
   if (!Parse_String(s, "]"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_OutputReg(const char **s, GLint *outputRegNum)
{
   char token[100];
   GLint j;

   /* Match 'o' */
   if (!Parse_String(s, "o"))
      PARSE_ERROR;

   /* Match '[' */
   if (!Parse_String(s, "["))
      PARSE_ERROR;

   /* Get output reg name */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   /* try to match an output register name */
   for (j = 0; OutputRegisters[j]; j++) {
      if (StrEq(token, OutputRegisters[j])) {
         *outputRegNum = FP_OUTPUT_REG_START + j;
         break;
      }
   }
   if (!OutputRegisters[j])
      PARSE_ERROR1("Unrecognized output register name");

   /* Match ']' */
   if (!Parse_String(s, "]"))
      PARSE_ERROR1("Expected ]");

   return GL_TRUE;
}


static GLboolean
Parse_MaskedDstReg(const char **s, struct fp_dst_register *dstReg)
{
   char token[100];

   /* Dst reg can be R<n>, H<n>, o[n], RC or HC */
   if (!Peek_Token(s, token))
      PARSE_ERROR;

   if (_mesa_strcmp(token, "RC") == 0 ||
       _mesa_strcmp(token, "HC") == 0) {
      /* a write-only register */
      if (!Parse_DummyReg(s, &dstReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'R' || token[0] == 'H') {
      /* a temporary register */
      if (!Parse_TempReg(s, &dstReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'o') {
      /* an output register */
      if (!Parse_OutputReg(s, &dstReg->Register))
         PARSE_ERROR;
   }
   else {
      PARSE_ERROR1("Bad destination register name");
   }

   /* Parse optional write mask */
   if (!Peek_Token(s, token))
      PARSE_ERROR;

   if (token[0] == '.') {
      /* got a mask */
      GLint k = 0;

      if (!Parse_String(s, "."))
         PARSE_ERROR;

      if (!Parse_Token(s, token))  /* get xyzw writemask */
         PARSE_ERROR;

      dstReg->WriteMask[0] = GL_FALSE;
      dstReg->WriteMask[1] = GL_FALSE;
      dstReg->WriteMask[2] = GL_FALSE;
      dstReg->WriteMask[3] = GL_FALSE;

      if (token[k] == 'x') {
         dstReg->WriteMask[0] = GL_TRUE;
         k++;
      }
      if (token[k] == 'y') {
         dstReg->WriteMask[1] = GL_TRUE;
         k++;
      }
      if (token[k] == 'z') {
         dstReg->WriteMask[2] = GL_TRUE;
         k++;
      }
      if (token[k] == 'w') {
         dstReg->WriteMask[3] = GL_TRUE;
         k++;
      }
      if (k == 0) {
         PARSE_ERROR1("Bad writemask character");
      }

      /* peek optional cc mask */
      if (!Peek_Token(s, token))
         PARSE_ERROR;
   }
   else {
      dstReg->WriteMask[0] = GL_TRUE;
      dstReg->WriteMask[1] = GL_TRUE;
      dstReg->WriteMask[2] = GL_TRUE;
      dstReg->WriteMask[3] = GL_TRUE;
   }

   /* optional condition code mask */
   if (token[0] == '(') {
      /* ("EQ" | "GE" | "GT" | "LE" | "LT" | "NE" | "TR" | "FL".x|y|z|w) */
      /* ("EQ" | "GE" | "GT" | "LE" | "LT" | "NE" | "TR" | "FL".[xyzw]) */
      Parse_String(s, "(");

      if (!Parse_CondCodeMask(s, dstReg))
         PARSE_ERROR;

      if (!Parse_String(s, ")"))  /* consume ")" */
         PARSE_ERROR;

      return GL_TRUE;
   }
   else {
      /* no cond code mask */
      dstReg->CondMask = COND_TR;
      dstReg->CondSwizzle[0] = 0;
      dstReg->CondSwizzle[1] = 1;
      dstReg->CondSwizzle[2] = 2;
      dstReg->CondSwizzle[3] = 3;
      return GL_TRUE;
   }
}


static GLboolean
Parse_SwizzleSrcReg(const char **s, struct fp_src_register *srcReg)
{
   char token[100];

   /* XXX need to parse absolute value and another negation ***/
   srcReg->NegateBase = GL_FALSE;
   srcReg->Abs = GL_FALSE;
   srcReg->NegateAbs = GL_FALSE;

   /* check for '-' */
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '-') {
      (void) Parse_String(s, "-");
      srcReg->NegateBase = GL_TRUE;
      if (!Peek_Token(s, token))
         PARSE_ERROR;
   }
   else {
      srcReg->NegateBase = GL_FALSE;
   }

   /* Src reg can be R<n>, H<n> or a named fragment attrib */
   if (token[0] == 'R' || token[0] == 'H') {
      if (!Parse_TempReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'f') {
      if (!Parse_AttribReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'p') {
      if (!Parse_ProgramParamReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else {
      /* Also parse defined/declared constant or vector literal */
      PARSE_ERROR2("Bad source register name", token);
   }

   /* init swizzle fields */
   srcReg->Swizzle[0] = 0;
   srcReg->Swizzle[1] = 1;
   srcReg->Swizzle[2] = 2;
   srcReg->Swizzle[3] = 3;

   /* Look for optional swizzle suffix */
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '.') {
      (void) Parse_String(s, ".");  /* consume . */

      if (!Parse_Token(s, token))
         PARSE_ERROR;

      if (!Parse_SwizzleSuffix(token, srcReg->Swizzle))
         PARSE_ERROR1("Bad swizzle suffix");
   }

   return GL_TRUE;
}


static GLboolean
Parse_ScalarSrcReg(const char **s, struct fp_src_register *srcReg)
{
   char token[100];

   /* check for '-' */
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '-') {
      srcReg->NegateBase = GL_TRUE;
      (void) Parse_String(s, "-"); /* consume '-' */
      if (!Peek_Token(s, token))
         PARSE_ERROR;
   }
   else {
      srcReg->NegateBase = GL_FALSE;
   }

   /* Src reg can be R<n>, H<n> or a named fragment attrib */
   if (token[0] == 'R' || token[0] == 'H') {
      if (!Parse_TempReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'f') {
      if (!Parse_AttribReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else {
      PARSE_ERROR2("Bad source register name", token);
   }

   /* Look for .[xyzw] suffix */
   if (!Parse_String(s, "."))
      PARSE_ERROR;

   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (token[0] == 'x' && token[1] == 0) {
      srcReg->Swizzle[0] = 0;
   }
   else if (token[0] == 'y' && token[1] == 0) {
      srcReg->Swizzle[0] = 1;
   }
   else if (token[0] == 'z' && token[1] == 0) {
      srcReg->Swizzle[0] = 2;
   }
   else if (token[0] == 'w' && token[1] == 0) {
      srcReg->Swizzle[0] = 3;
   }
   else {
      PARSE_ERROR1("Bad scalar source suffix");
   }
   srcReg->Swizzle[1] = srcReg->Swizzle[2] = srcReg->Swizzle[3] = 0;

   return GL_TRUE;
}



static GLboolean
Parse_InstructionSequence(const char **s, struct fp_instruction program[])
{
   char token[100];
   GLint count = 0;

   while (1) {
      struct fp_instruction *inst = program + count;
      struct instruction_pattern instMatch;

      /* Initialize the instruction */
      inst->SrcReg[0].Register = -1;
      inst->SrcReg[1].Register = -1;
      inst->SrcReg[2].Register = -1;
      inst->DstReg.Register = -1;

      /* get token */
      if (!Parse_Token(s, token)) {
         inst->Opcode = FP_OPCODE_END;
         printf("END OF PROGRAM %d\n", count);
         break;
      }

      /* special instructions */
      if (StrEq(token, "DEFINE")) {
         char id[100];
         GLfloat value[4];
         if (!Parse_Identifier(s, id))
            PARSE_ERROR;
         if (!Parse_String(s, "="))
            PARSE_ERROR1("Expected = symbol");
         if (!Parse_VectorOrScalarConstant(s, value))
            PARSE_ERROR;
         printf("Parsed DEFINE %s = %f %f %f %f\n", id, value[0], value[1],
                value[2], value[3]);
      }
      else if (StrEq(token, "DECLARE")) {
         char id[100];
         GLfloat value[4];
         if (!Parse_Identifier(s, id))
            PARSE_ERROR;
         if (!Peek_Token(s, token))
            PARSE_ERROR;
         if (token[0] == '=') {
            Parse_String(s, "=");
            if (!Parse_VectorOrScalarConstant(s, value))
               PARSE_ERROR;
            printf("Parsed DECLARE %s = %f %f %f %f\n", id, value[0], value[1],
                   value[2], value[3]);
         }
         else {
            printf("Parsed DECLARE %s\n", id);
         }
      }

      /* try to find matching instuction */
      instMatch = MatchInstruction(token);
      if (instMatch.opcode < 0) {
         /* bad instruction name */
         PARSE_ERROR2("Unexpected token: ", token);
      }

      inst->Opcode = instMatch.opcode;
      inst->Precision = instMatch.suffixes & (_R | _H | _X);
      inst->Saturate = (instMatch.suffixes & (_S)) ? GL_TRUE : GL_FALSE;
      inst->UpdateCondRegister = (instMatch.suffixes & (_C)) ? GL_TRUE : GL_FALSE;

      /*
       * parse the input and output operands
       */
      if (instMatch.outputs == OUTPUT_S || instMatch.outputs == OUTPUT_V) {
         if (!Parse_MaskedDstReg(s, &inst->DstReg))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
      }
      else if (instMatch.outputs == OUTPUT_NONE) {
         ASSERT(instMatch.opcode == FP_OPCODE_KIL);
         /* This is a little weird, the cond code info is in the dest register */
         if (!Parse_CondCodeMask(s, &inst->DstReg))
            PARSE_ERROR;
      }

      if (instMatch.inputs == INPUT_1V) {
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_2V) {
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_3V) {
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_1S) {
         if (!Parse_ScalarSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_2S) {
         if (!Parse_ScalarSrcReg(s, &inst->SrcReg[0]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_ScalarSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_CC) {
#if 00
         if (!ParseCondCodeSrc(s, &inst->srcReg[0]))
            PARSE_ERROR;
#endif
      }
      else if (instMatch.inputs == INPUT_1V_T) {
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_TextureImageId(s, &inst->TexSrcUnit, &inst->TexSrcTarget))
            PARSE_ERROR;
      }
      else if (instMatch.inputs == INPUT_1V_T) {
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[2]))
            PARSE_ERROR;
         if (!Parse_String(s, ","))
            PARSE_ERROR;
         if (!Parse_TextureImageId(s, &inst->TexSrcUnit, &inst->TexSrcTarget))
            PARSE_ERROR;
      }

      /* end of statement semicolon */
      if (!Parse_String(s, ";"))
         PARSE_ERROR;

      count++;
      if (count >= MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS)
         PARSE_ERROR1("Program too long");
   }
   return GL_TRUE;
}


static GLboolean
Parse_Program(const char **s, struct fp_instruction instBuffer[])
{
   return Parse_InstructionSequence(s, instBuffer);
}


/**
 * Parse/compile the 'str' returning the compiled 'program'.
 * ctx->Program.ErrorPos will be -1 if successful.  Otherwise, ErrorPos
 * indicates the position of the error in 'str'.
 */
void
_mesa_parse_nv_fragment_program(GLcontext *ctx, GLenum dstTarget,
                                const GLubyte *str, GLsizei len,
                                struct fragment_program *program)
{
   const char *s;
   struct fp_instruction instBuffer[MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS];
   GLubyte *newString;
   struct fp_instruction *newInst;
   GLenum target;
   GLubyte *programString;

   /* Make a null-terminated copy of the program string */
   programString = (GLubyte *) MALLOC(len + 1);
   if (!programString) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
      return;
   }
   MEMCPY(programString, str, len);
   programString[len] = 0;


   /* check the program header */
   if (_mesa_strncmp((const char *) programString, "!!FP1.0", 7) == 0) {
      target = GL_FRAGMENT_PROGRAM_NV;
      s = (const char *) programString + 7;
   }
   else {
      /* invalid header */
      _mesa_set_program_error(ctx, 0, "Invalid fragment program header");
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(bad header)");
      return;
   }

   /* make sure target and header match */
   if (target != dstTarget) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glLoadProgramNV(target mismatch 0x%x != 0x%x)",
                  target, dstTarget);
      return;
   }

   if (Parse_Program(&s, instBuffer)) {
      GLuint numInst;
      GLuint strLen;
      GLuint inputsRead = 0;
      GLuint outputsWritten = 0;
      /**GLuint progRegsWritten = 0;**/

      /* Find length of the program and compute bitmasks to indicate which
       * vertex input registers are read, which vertex result registers are
       * written to, and which program registers are written to.
       * We could actually do this while we parse the program.
       */
      for (numInst = 0; instBuffer[numInst].Opcode != FP_OPCODE_END; numInst++) {
#if 0
         const GLint srcReg0 = instBuffer[numInst].SrcReg[0].Register;
         const GLint srcReg1 = instBuffer[numInst].SrcReg[1].Register;
         const GLint srcReg2 = instBuffer[numInst].SrcReg[2].Register;
         const GLint dstReg = instBuffer[numInst].DstReg.Register;

         if ((r = OutputRegisterNumber(dstReg)) >= 0)
            outputsWritten |= (1 << r);
         else if (IsProgRegister(dstReg))
            progRegsWritten |= (1 << (dstReg - FP_PROG_REG_START));
         if ((r = InputRegisterNumber(srcReg0)) >= 0
             && !instBuffer[numInst].SrcReg[0].RelAddr)
            inputsRead |= (1 << r);
         if ((r = InputRegisterNumber(srcReg1) >= 0
             && !instBuffer[numInst].SrcReg[1].RelAddr)
            inputsRead |= (1 << r);
         if ((r = InputRegisterNumber(srcReg2) >= 0
             && !instBuffer[numInst].SrcReg[2].RelAddr)
            inputsRead |= (1 << r);
#endif
      }
      numInst++;
      printf("numInst %d\n", numInst);

      program->InputsRead = inputsRead;
      program->OutputsWritten = outputsWritten;

      /* make copy of the input program string */
      strLen = _mesa_strlen((const char *) str);
      newString = (GLubyte *) MALLOC(strLen + 1);
      if (!newString) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
         return;
      }
      MEMCPY(newString, str, strLen);
      newString[strLen] = 0; /* terminate */

      /* copy the compiled instructions */
      assert(numInst <= MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS);
      newInst = (struct fp_instruction *) MALLOC(numInst * sizeof(struct fp_instruction));
      if (!newInst) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
         return;  /* out of memory */
      }
      MEMCPY(newInst, instBuffer, numInst * sizeof(struct fp_instruction));

      /* install the program */
      program->Base.Target = target;
      if (program->Base.String) {
         FREE(program->Base.String);
      }
      program->Base.String = newString;
      if (program->Instructions) {
         FREE(program->Instructions);
      }
      program->Instructions = newInst;

#ifdef DEBUG
      _mesa_printf("--- glLoadProgramNV result ---\n");
      _mesa_print_nv_fragment_program(program);
      _mesa_printf("------------------------------\n");
#endif
   }
   else {
      /* Error! */
#ifdef DEBUG
      /* print a message showing the program line containing the error */
      ctx->Program.ErrorPos = (GLubyte *) s - str;
      {
         const GLubyte *p = str, *line = str;
         int lineNum = 1, statementNum = 1, column = 0;
         char errorLine[1000];
         int i;
         while (*p && p < (const GLubyte *) s) {  /* s is the error position */
            if (*p == '\n') {
               line = p + 1;
               lineNum++;
               column = 0;
            }
            else if (*p == ';') {
               statementNum++;
            }
            else
               column++;
            p++;
         }
         if (p) {
            /* Copy the line with the error into errorLine so we can null-
             * terminate it.
             */
            for (i = 0; line[i] != '\n' && line[i]; i++)
               errorLine[i] = (char) line[i];
            errorLine[i] = 0;
         }
         /*
         _mesa_debug("Error pos = %d  (%c) col %d\n",
                 ctx->Program.ErrorPos, *s, column);
         */
         _mesa_debug(ctx, "Fragment program error on line %2d: %s\n", lineNum, errorLine);
         _mesa_debug(ctx, "  (statement %2d) near column %2d: ", statementNum, column+1);
         for (i = 0; i < column; i++)
            _mesa_debug(ctx, " ");
         _mesa_debug(ctx, "^\n");
      }
#endif
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV");
   }
}


static void
PrintSrcReg(const struct fp_src_register *src)
{
   static const char comps[5] = "xyzw";
   GLint r;

   if (src->NegateAbs) {
      _mesa_printf("-");
   }
   if (src->Abs) {
      _mesa_printf("|");
   }
   if (src->NegateBase) {
      _mesa_printf("-");
   }
   if ((r = OutputRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("o[%s]", OutputRegisters[r]);
   }
   else if ((r = InputRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("f[%s]", InputRegisters[r]);
   }
   else if ((r = ProgramRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("p[%d]", r);
   }
   else if ((r = HalfTempRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("H%d", r);
   }
   else if ((r = TempRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("R%d", r);
   }
   else if ((r = DummyRegisterNumber(src->Register)) >= 0) {
      _mesa_printf("%cC", "HR"[r]);
   }
   else {
      _mesa_problem(NULL, "Bad fragment register");
      return;
   }
   if (src->Swizzle[0] == src->Swizzle[1] &&
       src->Swizzle[0] == src->Swizzle[2] &&
       src->Swizzle[0] == src->Swizzle[3]) {
      _mesa_printf(".%c", comps[src->Swizzle[0]]);
   }
   else if (src->Swizzle[0] != 0 ||
            src->Swizzle[1] != 1 ||
            src->Swizzle[2] != 2 ||
            src->Swizzle[3] != 3) {
      _mesa_printf(".%c%c%c%c",
                   comps[src->Swizzle[0]],
                   comps[src->Swizzle[1]],
                   comps[src->Swizzle[2]],
                   comps[src->Swizzle[3]]);
   }
   if (src->Abs) {
      _mesa_printf("|");
   }
}

static void
PrintCondCode(const struct fp_dst_register *dst)
{
   static const char *comps = "xyzw";
   static const char *ccString[] = {
      "??", "GT", "EQ", "LT", "UN", "GE", "LE", "NE", "TR", "FL", "??"
   };

   _mesa_printf("%s", ccString[dst->CondMask]);
   if (dst->CondSwizzle[0] == dst->CondSwizzle[1] &&
       dst->CondSwizzle[0] == dst->CondSwizzle[2] &&
       dst->CondSwizzle[0] == dst->CondSwizzle[3]) {
      _mesa_printf(".%c", comps[dst->CondSwizzle[0]]);
   }
   else if (dst->CondSwizzle[0] != 0 ||
            dst->CondSwizzle[1] != 1 ||
            dst->CondSwizzle[2] != 2 ||
            dst->CondSwizzle[3] != 3) {
      _mesa_printf(".%c%c%c%c",
                   comps[dst->CondSwizzle[0]],
                   comps[dst->CondSwizzle[1]],
                   comps[dst->CondSwizzle[2]],
                   comps[dst->CondSwizzle[3]]);
   }
}


static void
PrintDstReg(const struct fp_dst_register *dst)
{
   GLint w = dst->WriteMask[0] + dst->WriteMask[1]
           + dst->WriteMask[2] + dst->WriteMask[3];
   GLint r;

   if ((r = OutputRegisterNumber(dst->Register)) >= 0) {
      _mesa_printf("o[%s]", OutputRegisters[r]);
   }
   else if ((r = HalfTempRegisterNumber(dst->Register)) >= 0) {
      _mesa_printf("H[%s]", InputRegisters[r]);
   }
   else if ((r = TempRegisterNumber(dst->Register)) >= 0) {
      _mesa_printf("R[%s]", InputRegisters[r]);
   }
   else if ((r = ProgramRegisterNumber(dst->Register)) >= 0) {
      _mesa_printf("p[%d]", r);
   }
   else if ((r = DummyRegisterNumber(dst->Register)) >= 0) {
      _mesa_printf("%cC", "HR"[r]);
   }
   else {
      _mesa_printf("???");
   }

   if (w != 0 && w != 4) {
      _mesa_printf(".");
      if (dst->WriteMask[0])
         _mesa_printf("x");
      if (dst->WriteMask[1])
         _mesa_printf("y");
      if (dst->WriteMask[2])
         _mesa_printf("z");
      if (dst->WriteMask[3])
         _mesa_printf("w");
   }

   if (dst->CondMask != COND_TR ||
       dst->CondSwizzle[0] != 0 ||
       dst->CondSwizzle[1] != 1 ||
       dst->CondSwizzle[2] != 2 ||
       dst->CondSwizzle[3] != 3) {
      _mesa_printf(" (");
      PrintCondCode(dst);
      _mesa_printf(")");
   }
}


/**
 * Print (unparse) the given vertex program.  Just for debugging.
 */
void
_mesa_print_nv_fragment_program(const struct fragment_program *program)
{
   const struct fp_instruction *inst;

   for (inst = program->Instructions; inst->Opcode != FP_OPCODE_END; inst++) {
      int i;
      for (i = 0; Instructions[i].name; i++) {
         if (inst->Opcode == Instructions[i].opcode) {
            /* print instruction name */
            _mesa_printf("%s", Instructions[i].name);
            if (inst->Precision == HALF)
               _mesa_printf("H");
            else if (inst->Precision == FIXED)
               _mesa_printf("X");
            if (inst->UpdateCondRegister)
               _mesa_printf("C");
            if (inst->Saturate)
               _mesa_printf("_SAT");
            _mesa_printf(" ");

            if (Instructions[i].inputs == INPUT_CC) {
               PrintCondCode(&inst->DstReg);
            }
            else if (Instructions[i].outputs == OUTPUT_V ||
                     Instructions[i].outputs == OUTPUT_S) {
               /* print dest register */
               PrintDstReg(&inst->DstReg);
               _mesa_printf(", ");
            }

            /* print source register(s) */
            if (Instructions[i].inputs == INPUT_1V ||
                Instructions[i].inputs == INPUT_1S) {
               PrintSrcReg(&inst->SrcReg[0]);
            }
            else if (Instructions[i].inputs == INPUT_2V ||
                     Instructions[i].inputs == INPUT_2S) {
               PrintSrcReg(&inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintSrcReg(&inst->SrcReg[1]);
            }
            else if (Instructions[i].inputs == INPUT_3V) {
               PrintSrcReg(&inst->SrcReg[0]);
               _mesa_printf(", ");
               PrintSrcReg(&inst->SrcReg[1]);
               _mesa_printf(", ");
               PrintSrcReg(&inst->SrcReg[2]);
            }

            _mesa_printf(";\n");
            break;
         }
      }
      if (!Instructions[i].name) {
         _mesa_printf("Bad opcode %d\n", inst->Opcode);
      }
   }
}

