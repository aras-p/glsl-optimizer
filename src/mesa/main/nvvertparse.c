/* $Id: nvvertparse.c,v 1.1 2003/01/14 04:55:46 brianp Exp $ */

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
 * \file nvvertparse.c
 * \brief NVIDIA vertex program parser.
 * \author Brian Paul
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvprogram.h"
#include "nvvertparse.h"
#include "nvvertprog.h"


/************************ Symbol Table ******************************/

/* A simple symbol table implementation for ARB_vertex_program
 * (not used yet)
 */

#if 000
struct symbol
{
   GLubyte *name;
   GLint value;
   struct symbol *next;
};

static struct symbol *SymbolTable = NULL;

static GLboolean
IsSymbol(const GLubyte *symbol)
{
   struct symbol *s;
   for (s = SymbolTable; s; s = s->next) {
      if (strcmp((char *) symbol, (char *)s->name) == 0)
         return GL_TRUE;
   }
   return GL_FALSE;
}

static GLint
GetSymbolValue(const GLubyte *symbol)
{
   struct symbol *s;
   for (s = SymbolTable; s; s = s->next) {
      if (strcmp((char *) symbol, (char *)s->name) == 0)
         return s->value;
   }
   return 0;
}

static void
AddSymbol(const GLubyte *symbol, GLint value)
{
   struct symbol *s = MALLOC_STRUCT(symbol);
   if (s) {
      s->name = (GLubyte *) strdup((char *) symbol);
      s->value = value;
      s->next = SymbolTable;
      SymbolTable = s;
   }
}

static void
ResetSymbolTable(void)
{
   struct symbol *s, *next;
   for (s = SymbolTable; s; s = next) {
      next = s->next;
      FREE(s->name);
      FREE(s);
      s = next;
   }   
   SymbolTable = NULL;
}
#endif

/***************************** Parsing ******************************/


static GLboolean IsLetter(GLubyte b)
{
   return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
}


static GLboolean IsDigit(GLubyte b)
{
   return b >= '0' && b <= '9';
}


static GLboolean IsWhitespace(GLubyte b)
{
   return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}


/**
 * Starting at 'str' find the next token.  A token can be an integer,
 * an identifier or punctuation symbol.
 * \return <= 0 we found an error, else, return number of characters parsed.
 */
static GLint
GetToken(const GLubyte *str, GLubyte *token)
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
Parse_Token(const GLubyte **s, GLubyte *token)
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
Peek_Token(const GLubyte **s, GLubyte *token)
{
   GLint i, len;
   i = GetToken(*s, token);
   if (i <= 0) {
      *s += (-i);
      return GL_FALSE;
   }
   len = _mesa_strlen((char *) token);
   *s += (i - len);
   return GL_TRUE;
}


/**
 * String equality test
 */
static GLboolean
StrEq(const GLubyte *a, const GLubyte *b)
{
   GLint i;
   for (i = 0; a[i] && b[i] && a[i] == b[i]; i++)
      ;
   if (a[i] == 0 && b[i] == 0)
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**********************************************************************/

static const char *InputRegisters[] = {
   "OPOS", "WGHT", "NRML", "COL0", "COL1", "FOGC", "6", "7",
   "TEX0", "TEX1", "TEX2", "TEX3", "TEX4", "TEX5", "TEX6", "TEX7", NULL
};

static const char *OutputRegisters[] = {
   "HPOS", "COL0", "COL1", "BFC0", "BFC1", "FOGC", "PSIZ",
   "TEX0", "TEX1", "TEX2", "TEX3", "TEX4", "TEX5", "TEX6", "TEX7", NULL
};

static const char *Opcodes[] = {
   "MOV", "LIT", "RCP", "RSQ", "EXP", "LOG", "MUL", "ADD", "DP3", "DP4",
   "DST", "MIN", "MAX", "SLT", "SGE", "MAD", "ARL", "DPH", "RCC", "SUB",
   "ABS", "END", NULL
};


#ifdef DEBUG

#define PARSE_ERROR						\
do {								\
   _mesa_printf("vpparse.c error at %d: parse error\n", __LINE__);	\
   return GL_FALSE;						\
} while(0)

#define PARSE_ERROR1(msg)					\
do {								\
   _mesa_printf("vpparse.c error at %d: %s\n", __LINE__, msg);	\
   return GL_FALSE;						\
} while(0)

#define PARSE_ERROR2(msg1, msg2)					\
do {									\
   _mesa_printf("vpparse.c error at %d: %s %s\n", __LINE__, msg1, msg2);	\
   return GL_FALSE;							\
} while(0)

#else

#define PARSE_ERROR                return GL_FALSE
#define PARSE_ERROR1(msg1)         return GL_FALSE
#define PARSE_ERROR2(msg1, msg2)   return GL_FALSE

#endif


static GLuint
IsProgRegister(GLuint r)
{
   return (GLuint) (r >= VP_PROG_REG_START && r <= VP_PROG_REG_END);
}

static GLuint
IsInputRegister(GLuint r)
{
   return (GLuint) (r >= VP_INPUT_REG_START && r <= VP_INPUT_REG_END);
}

static GLuint
IsOutputRegister(GLuint r)
{
   return (GLuint) (r >= VP_OUTPUT_REG_START && r <= VP_OUTPUT_REG_END);
}



/**********************************************************************/

/* XXX
 * These shouldn't be globals as that makes the parser non-reentrant.
 * We should really define a "ParserContext" class which contains these
 * and the <s> pointer into the program text.
 */
static GLboolean IsStateProgram = GL_FALSE;
static GLboolean IsPositionInvariant = GL_FALSE;
static GLboolean IsVersion1_1 = GL_FALSE;

/**
 * Try to match 'pattern' as the next token after any whitespace/comments.
 */
static GLboolean
Parse_String(const GLubyte **s, const char *pattern)
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


/**
 * Parse a temporary register: Rnn
 */
static GLboolean
Parse_TempReg(const GLubyte **s, GLint *tempRegNum)
{
   GLubyte token[100];

   /* Should be 'R##' */
   if (!Parse_Token(s, token))
      PARSE_ERROR;
   if (token[0] != 'R')
      PARSE_ERROR1("Expected R##");

   if (IsDigit(token[1])) {
      GLint reg = _mesa_atoi((char *) (token + 1));
      if (reg >= VP_NUM_TEMP_REGS)
         PARSE_ERROR1("Bad temporary register name");
      *tempRegNum = VP_TEMP_REG_START + reg;
   }
   else {
      PARSE_ERROR1("Bad temporary register name");
   }

   return GL_TRUE;
}


/**
 * Parse address register "A0.x"
 */
static GLboolean
Parse_AddrReg(const GLubyte **s)
{
   /* match 'A0' */
   if (!Parse_String(s, "A0"))
      PARSE_ERROR;

   /* match '.' */
   if (!Parse_String(s, "."))
      PARSE_ERROR;

   /* match 'x' */
   if (!Parse_String(s, "x"))
      PARSE_ERROR;

   return GL_TRUE;
}


/**
 * Parse absolute program parameter register "c[##]"
 */
static GLboolean
Parse_AbsParamReg(const GLubyte **s, GLint *regNum)
{
   GLubyte token[100];

   if (!Parse_String(s, "c"))
      PARSE_ERROR;

   if (!Parse_String(s, "["))
      PARSE_ERROR;

   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (IsDigit(token[0])) {
      /* a numbered program parameter register */
      GLint reg = _mesa_atoi((char *) token);
      if (reg >= VP_NUM_PROG_REGS)
         PARSE_ERROR1("Bad constant program number");
      *regNum = VP_PROG_REG_START + reg;
   }
   else {
      PARSE_ERROR;
   }

   if (!Parse_String(s, "]"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_ParamReg(const GLubyte **s, struct vp_src_register *srcReg)
{
   GLubyte token[100];

   if (!Parse_String(s, "c"))
      PARSE_ERROR;

   if (!Parse_String(s, "["))
      PARSE_ERROR;

   if (!Peek_Token(s, token))
      PARSE_ERROR;

   if (IsDigit(token[0])) {
      /* a numbered program parameter register */
      GLint reg;
      (void) Parse_Token(s, token);
      reg = _mesa_atoi((char *) token);
      if (reg >= VP_NUM_PROG_REGS)
         PARSE_ERROR1("Bad constant program number");
      srcReg->Register = VP_PROG_REG_START + reg;
   }
   else if (StrEq(token, (GLubyte *) "A0")) {
      /* address register "A0.x" */
      if (!Parse_AddrReg(s))
         PARSE_ERROR;

      srcReg->RelAddr = GL_TRUE;
      srcReg->Register = 0;

      /* Look for +/-N offset */
      if (!Peek_Token(s, token))
         PARSE_ERROR;

      if (token[0] == '-' || token[0] == '+') {
         const GLubyte sign = token[0];
         (void) Parse_Token(s, token); /* consume +/- */

         /* an integer should be next */
         if (!Parse_Token(s, token))
            PARSE_ERROR;

         if (IsDigit(token[0])) {
            const GLint k = _mesa_atoi((char *) token);
            if (sign == '-') {
               if (k > 64)
                  PARSE_ERROR1("Bad address offset");
               srcReg->Register = -k;
            }
            else {
               if (k > 63)
                  PARSE_ERROR1("Bad address offset");
               srcReg->Register = k;
            }
         }
         else {
            PARSE_ERROR;
         }
      }
      else {
         /* probably got a ']', catch it below */
      }
   }
   else {
      PARSE_ERROR;
   }

   /* Match closing ']' */
   if (!Parse_String(s, "]"))
      PARSE_ERROR;

   return GL_TRUE;
}


/**
 * Parse v[#] or v[<name>]
 */
static GLboolean
Parse_AttribReg(const GLubyte **s, GLint *tempRegNum)
{
   GLubyte token[100];
   GLint j;

   /* Match 'v' */
   if (!Parse_String(s, "v"))
      PARSE_ERROR;

   /* Match '[' */
   if (!Parse_String(s, "["))
      PARSE_ERROR;

   /* match number or named register */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (IsStateProgram && token[0] != '0')
      PARSE_ERROR1("Only v[0] accessible in vertex state programs");

   if (IsDigit(token[0])) {
      GLint reg = _mesa_atoi((char *) token);
      if (reg >= VP_NUM_INPUT_REGS)
         PARSE_ERROR1("Bad vertex attribute register name");
      *tempRegNum = VP_INPUT_REG_START + reg;
   }
   else {
      for (j = 0; InputRegisters[j]; j++) {
         if (StrEq(token, (const GLubyte *) InputRegisters[j])) {
            *tempRegNum = VP_INPUT_REG_START + j;
            break;
         }
      }
      if (!InputRegisters[j]) {
         /* unknown input register label */
         PARSE_ERROR2("Bad register name", token);
      }
   }

   /* Match '[' */
   if (!Parse_String(s, "]"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_OutputReg(const GLubyte **s, GLint *outputRegNum)
{
   GLubyte token[100];
   GLint start, j;

   /* Match 'o' */
   if (!Parse_String(s, "o"))
      PARSE_ERROR;

   /* Match '[' */
   if (!Parse_String(s, "["))
      PARSE_ERROR;

   /* Get output reg name */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (IsPositionInvariant)
      start = 1; /* skip HPOS register name */
   else
      start = 0;

   /* try to match an output register name */
   for (j = start; OutputRegisters[j]; j++) {
      if (StrEq(token, (const GLubyte *) OutputRegisters[j])) {
         *outputRegNum = VP_OUTPUT_REG_START + j;
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
Parse_MaskedDstReg(const GLubyte **s, struct vp_dst_register *dstReg)
{
   GLubyte token[100];

   /* Dst reg can be R<n> or o[n] */
   if (!Peek_Token(s, token))
      PARSE_ERROR;

   if (token[0] == 'R') {
      /* a temporary register */
      if (!Parse_TempReg(s, &dstReg->Register))
         PARSE_ERROR;
   }
   else if (!IsStateProgram && token[0] == 'o') {
      /* an output register */
      if (!Parse_OutputReg(s, &dstReg->Register))
         PARSE_ERROR;
   }
   else if (IsStateProgram && token[0] == 'c') {
      /* absolute program parameter register */
      if (!Parse_AbsParamReg(s, &dstReg->Register))
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

      if (!Parse_Token(s, token))
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
      return GL_TRUE;
   }
   else {
      dstReg->WriteMask[0] = GL_TRUE;
      dstReg->WriteMask[1] = GL_TRUE;
      dstReg->WriteMask[2] = GL_TRUE;
      dstReg->WriteMask[3] = GL_TRUE;
      return GL_TRUE;
   }
}


static GLboolean
Parse_SwizzleSrcReg(const GLubyte **s, struct vp_src_register *srcReg)
{
   GLubyte token[100];

   srcReg->RelAddr = GL_FALSE;

   /* check for '-' */
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '-') {
      (void) Parse_String(s, "-");
      srcReg->Negate = GL_TRUE;
      if (!Peek_Token(s, token))
         PARSE_ERROR;
   }
   else {
      srcReg->Negate = GL_FALSE;
   }

   /* Src reg can be R<n>, c[n], c[n +/- offset], or a named vertex attrib */
   if (token[0] == 'R') {
      if (!Parse_TempReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'c') {
      if (!Parse_ParamReg(s, srcReg))
         PARSE_ERROR;
   }
   else if (token[0] == 'v') {
      if (!Parse_AttribReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else {
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

      if (token[1] == 0) {
         /* single letter swizzle */
         if (token[0] == 'x')
            ASSIGN_4V(srcReg->Swizzle, 0, 0, 0, 0);
         else if (token[0] == 'y')
            ASSIGN_4V(srcReg->Swizzle, 1, 1, 1, 1);
         else if (token[0] == 'z')
            ASSIGN_4V(srcReg->Swizzle, 2, 2, 2, 2);
         else if (token[0] == 'w')
            ASSIGN_4V(srcReg->Swizzle, 3, 3, 3, 3);
         else
            PARSE_ERROR1("Expected x, y, z, or w");
      }
      else {
         /* 2, 3 or 4-component swizzle */
         GLint k;
         for (k = 0; token[k] && k < 5; k++) {
            if (token[k] == 'x')
               srcReg->Swizzle[k] = 0;
            else if (token[k] == 'y')
               srcReg->Swizzle[k] = 1;
            else if (token[k] == 'z')
               srcReg->Swizzle[k] = 2;
            else if (token[k] == 'w')
               srcReg->Swizzle[k] = 3;
            else
               PARSE_ERROR;
         }
         if (k >= 5)
            PARSE_ERROR;
      }
   }

   return GL_TRUE;
}


static GLboolean
Parse_ScalarSrcReg(const GLubyte **s, struct vp_src_register *srcReg)
{
   GLubyte token[100];

   srcReg->RelAddr = GL_FALSE;

   /* check for '-' */
   if (!Peek_Token(s, token))
      PARSE_ERROR;
   if (token[0] == '-') {
      srcReg->Negate = GL_TRUE;
      (void) Parse_String(s, "-"); /* consume '-' */
      if (!Peek_Token(s, token))
         PARSE_ERROR;
   }
   else {
      srcReg->Negate = GL_FALSE;
   }

   /* Src reg can be R<n>, c[n], c[n +/- offset], or a named vertex attrib */
   if (token[0] == 'R') {
      if (!Parse_TempReg(s, &srcReg->Register))
         PARSE_ERROR;
   }
   else if (token[0] == 'c') {
      if (!Parse_ParamReg(s, srcReg))
         PARSE_ERROR;
   }
   else if (token[0] == 'v') {
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


static GLint
Parse_UnaryOpInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   GLubyte token[100];

   /* opcode */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (StrEq(token, (GLubyte *) "MOV")) {
      inst->Opcode = VP_OPCODE_MOV;
   }
   else if (StrEq(token, (GLubyte *) "LIT")) {
      inst->Opcode = VP_OPCODE_LIT;
   }
   else if (StrEq(token, (GLubyte *) "ABS") && IsVersion1_1) {
      inst->Opcode = VP_OPCODE_ABS;
   }
   else {
      PARSE_ERROR;
   }

   /* dest reg */
   if (!Parse_MaskedDstReg(s, &inst->DstReg))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
      PARSE_ERROR;

   /* semicolon */
   if (!Parse_String(s, ";"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_BiOpInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   GLubyte token[100];

   /* opcode */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (StrEq(token, (GLubyte *) "MUL")) {
      inst->Opcode = VP_OPCODE_MUL;
   }
   else if (StrEq(token, (GLubyte *) "ADD")) {
      inst->Opcode = VP_OPCODE_ADD;
   }
   else if (StrEq(token, (GLubyte *) "DP3")) {
      inst->Opcode = VP_OPCODE_DP3;
   }
   else if (StrEq(token, (GLubyte *) "DP4")) {
      inst->Opcode = VP_OPCODE_DP4;
   }
   else if (StrEq(token, (GLubyte *) "DST")) {
      inst->Opcode = VP_OPCODE_DST;
   }
   else if (StrEq(token, (GLubyte *) "MIN")) {
      inst->Opcode = VP_OPCODE_ADD;
   }
   else if (StrEq(token, (GLubyte *) "MAX")) {
      inst->Opcode = VP_OPCODE_ADD;
   }
   else if (StrEq(token, (GLubyte *) "SLT")) {
      inst->Opcode = VP_OPCODE_SLT;
   }
   else if (StrEq(token, (GLubyte *) "SGE")) {
      inst->Opcode = VP_OPCODE_SGE;
   }
   else if (StrEq(token, (GLubyte *) "DPH") && IsVersion1_1) {
      inst->Opcode = VP_OPCODE_DPH;
   }
   else if (StrEq(token, (GLubyte *) "SUB") && IsVersion1_1) {
      inst->Opcode = VP_OPCODE_SUB;
   }
   else {
      PARSE_ERROR;
   }

   /* dest reg */
   if (!Parse_MaskedDstReg(s, &inst->DstReg))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* first src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* second src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
      PARSE_ERROR;

   /* semicolon */
   if (!Parse_String(s, ";"))
      PARSE_ERROR;

   /* make sure we don't reference more than one program parameter register */
   if (IsProgRegister(inst->SrcReg[0].Register) &&
       IsProgRegister(inst->SrcReg[1].Register) &&
       inst->SrcReg[0].Register != inst->SrcReg[1].Register)
      PARSE_ERROR1("Can't reference two program parameter registers");

   /* make sure we don't reference more than one vertex attribute register */
   if (IsInputRegister(inst->SrcReg[0].Register) &&
       IsInputRegister(inst->SrcReg[1].Register) &&
       inst->SrcReg[0].Register != inst->SrcReg[1].Register)
      PARSE_ERROR1("Can't reference two vertex attribute registers");

   return GL_TRUE;
}


static GLboolean
Parse_TriOpInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   GLubyte token[100];

   /* opcode */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (StrEq(token, (GLubyte *) "MAD")) {
      inst->Opcode = VP_OPCODE_MAD;
   }
   else {
      PARSE_ERROR;
   }

   /* dest reg */
   if (!Parse_MaskedDstReg(s, &inst->DstReg))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* first src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[0]))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* second src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[1]))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* third src arg */
   if (!Parse_SwizzleSrcReg(s, &inst->SrcReg[2]))
      PARSE_ERROR;

   /* semicolon */
   if (!Parse_String(s, ";"))
      PARSE_ERROR;

   /* make sure we don't reference more than one program parameter register */
   if ((IsProgRegister(inst->SrcReg[0].Register) &&
        IsProgRegister(inst->SrcReg[1].Register) &&
        inst->SrcReg[0].Register != inst->SrcReg[1].Register) ||
       (IsProgRegister(inst->SrcReg[0].Register) &&
        IsProgRegister(inst->SrcReg[2].Register) &&
        inst->SrcReg[0].Register != inst->SrcReg[2].Register) ||
       (IsProgRegister(inst->SrcReg[1].Register) &&
        IsProgRegister(inst->SrcReg[2].Register) &&
        inst->SrcReg[1].Register != inst->SrcReg[2].Register))
      PARSE_ERROR1("Can only reference one program register");

   /* make sure we don't reference more than one vertex attribute register */
   if ((IsInputRegister(inst->SrcReg[0].Register) &&
        IsInputRegister(inst->SrcReg[1].Register) &&
        inst->SrcReg[0].Register != inst->SrcReg[1].Register) ||
       (IsInputRegister(inst->SrcReg[0].Register) &&
        IsInputRegister(inst->SrcReg[2].Register) &&
        inst->SrcReg[0].Register != inst->SrcReg[2].Register) ||
       (IsInputRegister(inst->SrcReg[1].Register) &&
        IsInputRegister(inst->SrcReg[2].Register) &&
        inst->SrcReg[1].Register != inst->SrcReg[2].Register))
      PARSE_ERROR1("Can only reference one input register");

   return GL_TRUE;
}


static GLboolean
Parse_ScalarInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   GLubyte token[100];

   /* opcode */
   if (!Parse_Token(s, token))
      PARSE_ERROR;

   if (StrEq(token, (GLubyte *) "RCP")) {
      inst->Opcode = VP_OPCODE_RCP;
   }
   else if (StrEq(token, (GLubyte *) "RSQ")) {
      inst->Opcode = VP_OPCODE_RSQ;
   }
   else if (StrEq(token, (GLubyte *) "EXP")) {
      inst->Opcode = VP_OPCODE_EXP;
   }
   else if (StrEq(token, (GLubyte *) "LOG")) {
      inst->Opcode = VP_OPCODE_LOG;
   }
   else if (StrEq(token, (GLubyte *) "RCC") && IsVersion1_1) {
      inst->Opcode = VP_OPCODE_RCC;
   }
   else {
      PARSE_ERROR;
   }

   /* dest reg */
   if (!Parse_MaskedDstReg(s, &inst->DstReg))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* first src arg */
   if (!Parse_ScalarSrcReg(s, &inst->SrcReg[0]))
      PARSE_ERROR;

   /* semicolon */
   if (!Parse_String(s, ";"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_AddressInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   inst->Opcode = VP_OPCODE_ARL;

   /* opcode */
   if (!Parse_String(s, "ARL"))
      PARSE_ERROR;

   /* dest A0 reg */
   if (!Parse_AddrReg(s))
      PARSE_ERROR;

   /* comma */
   if (!Parse_String(s, ","))
      PARSE_ERROR;

   /* parse src reg */
   if (!Parse_ScalarSrcReg(s, &inst->SrcReg[0]))
      PARSE_ERROR;

   /* semicolon */
   if (!Parse_String(s, ";"))
      PARSE_ERROR;

   return GL_TRUE;
}


static GLboolean
Parse_EndInstruction(const GLubyte **s, struct vp_instruction *inst)
{
   GLubyte token[100];

   /* opcode */
   if (!Parse_String(s, "END"))
      PARSE_ERROR;

   inst->Opcode = VP_OPCODE_END;

   /* this should fail! */
   if (Parse_Token(s, token))
      PARSE_ERROR2("Unexpected token after END:", token);
   else
      return GL_TRUE;
}


static GLboolean
Parse_OptionSequence(const GLubyte **s, struct vp_instruction program[])
{
   while (1) {
      GLubyte token[100];
      if (!Peek_Token(s, token)) {
         PARSE_ERROR1("Unexpected end of input");
         return GL_FALSE; /* end of input */
      }

      if (!StrEq(token, (GLubyte *) "OPTION"))
         return GL_TRUE;  /* probably an instruction */

      Parse_Token(s, token);

      if (!Parse_String(s, "NV_position_invariant"))
         return GL_FALSE;
      if (!Parse_String(s, ";"))
         return GL_FALSE;
      IsPositionInvariant = GL_TRUE;
   }
}


static GLboolean
Parse_InstructionSequence(const GLubyte **s, struct vp_instruction program[])
{
   GLubyte token[100];
   GLint count = 0;

   while (1) {
      struct vp_instruction *inst = program + count;

      /* Initialize the instruction */
      inst->SrcReg[0].Register = -1;
      inst->SrcReg[1].Register = -1;
      inst->SrcReg[2].Register = -1;
      inst->DstReg.Register = -1;

      if (!Peek_Token(s, token))
         PARSE_ERROR;

      if (StrEq(token, (GLubyte *) "MOV") ||
          StrEq(token, (GLubyte *) "LIT") ||
          StrEq(token, (GLubyte *) "ABS")) {
         if (!Parse_UnaryOpInstruction(s, inst))
            PARSE_ERROR;
      }
      else if (StrEq(token, (GLubyte *) "MUL") ||
          StrEq(token, (GLubyte *) "ADD") ||
          StrEq(token, (GLubyte *) "DP3") ||
          StrEq(token, (GLubyte *) "DP4") ||
          StrEq(token, (GLubyte *) "DST") ||
          StrEq(token, (GLubyte *) "MIN") ||
          StrEq(token, (GLubyte *) "MAX") ||
          StrEq(token, (GLubyte *) "SLT") ||
          StrEq(token, (GLubyte *) "SGE") ||
          StrEq(token, (GLubyte *) "DPH") ||
          StrEq(token, (GLubyte *) "SUB")) {
         if (!Parse_BiOpInstruction(s, inst))
            PARSE_ERROR;
      }
      else if (StrEq(token, (GLubyte *) "MAD")) {
         if (!Parse_TriOpInstruction(s, inst))
            PARSE_ERROR;
      }
      else if (StrEq(token, (GLubyte *) "RCP") ||
               StrEq(token, (GLubyte *) "RSQ") ||
               StrEq(token, (GLubyte *) "EXP") ||
               StrEq(token, (GLubyte *) "LOG") ||
               StrEq(token, (GLubyte *) "RCC")) {
         if (!Parse_ScalarInstruction(s, inst))
            PARSE_ERROR;
      }
      else if (StrEq(token, (GLubyte *) "ARL")) {
         if (!Parse_AddressInstruction(s, inst))
            PARSE_ERROR;
      }
      else if (StrEq(token, (GLubyte *) "END")) {
         if (!Parse_EndInstruction(s, inst))
            PARSE_ERROR;
         else
            return GL_TRUE;  /* all done */
      }
      else {
         /* bad instruction name */
         PARSE_ERROR2("Unexpected token: ", token);
      }

      count++;
      if (count >= MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS)
         PARSE_ERROR1("Program too long");
   }

   PARSE_ERROR;
}


static GLboolean
Parse_Program(const GLubyte **s, struct vp_instruction instBuffer[])
{
   if (IsVersion1_1) {
      if (!Parse_OptionSequence(s, instBuffer)) {
         return GL_FALSE;
      }
   }
   return Parse_InstructionSequence(s, instBuffer);
}


/**
 * Parse/compile the 'str' returning the compiled 'program'.
 * ctx->Program.ErrorPos will be -1 if successful.  Otherwise, ErrorPos
 * indicates the position of the error in 'str'.
 */
void
_mesa_parse_nv_vertex_program(GLcontext *ctx, GLenum dstTarget,
                              const GLubyte *str, GLsizei len,
                              struct vertex_program *program)
{
   const GLubyte *s;
   struct vp_instruction instBuffer[MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS];
   struct vp_instruction *newInst;
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

   IsPositionInvariant = GL_FALSE;
   IsVersion1_1 = GL_FALSE;

   /* check the program header */
   if (_mesa_strncmp((const char *) programString, "!!VP1.0", 7) == 0) {
      target = GL_VERTEX_PROGRAM_NV;
      s = programString + 7;
      IsStateProgram = GL_FALSE;
   }
   else if (_mesa_strncmp((const char *) programString, "!!VP1.1", 7) == 0) {
      target = GL_VERTEX_PROGRAM_NV;
      s = programString + 7;
      IsStateProgram = GL_FALSE;
      IsVersion1_1 = GL_TRUE;
   }
   else if (_mesa_strncmp((const char *) programString, "!!VSP1.0", 8) == 0) {
      target = GL_VERTEX_STATE_PROGRAM_NV;
      s = programString + 8;
      IsStateProgram = GL_TRUE;
   }
   else {
      /* invalid header */
      ctx->Program.ErrorPos = 0;
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLoadProgramNV(bad header)");
      return;
   }

   /* make sure target and header match */
   if (target != dstTarget) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glLoadProgramNV(target mismatch)");
      return;
   }

   if (Parse_Program(&s, instBuffer)) {
      GLuint numInst;
      GLuint inputsRead = 0;
      GLuint outputsWritten = 0;
      GLuint progRegsWritten = 0;

      /* Find length of the program and compute bitmasks to indicate which
       * vertex input registers are read, which vertex result registers are
       * written to, and which program registers are written to.
       * We could actually do this while we parse the program.
       */
      for (numInst = 0; instBuffer[numInst].Opcode != VP_OPCODE_END; numInst++) {
         const GLint srcReg0 = instBuffer[numInst].SrcReg[0].Register;
         const GLint srcReg1 = instBuffer[numInst].SrcReg[1].Register;
         const GLint srcReg2 = instBuffer[numInst].SrcReg[2].Register;
         const GLint dstReg = instBuffer[numInst].DstReg.Register;

         if (IsOutputRegister(dstReg))
            outputsWritten |= (1 << (dstReg - VP_OUTPUT_REG_START));
         else if (IsProgRegister(dstReg))
            progRegsWritten |= (1 << (dstReg - VP_PROG_REG_START));
         if (IsInputRegister(srcReg0)
             && !instBuffer[numInst].SrcReg[0].RelAddr)
            inputsRead |= (1 << (srcReg0 - VP_INPUT_REG_START));
         if (IsInputRegister(srcReg1)
             && !instBuffer[numInst].SrcReg[1].RelAddr)
            inputsRead |= (1 << (srcReg1 - VP_INPUT_REG_START));
         if (IsInputRegister(srcReg2)
             && !instBuffer[numInst].SrcReg[2].RelAddr)
            inputsRead |= (1 << (srcReg2 - VP_INPUT_REG_START));
      }
      numInst++;

      if (IsStateProgram) {
         if (progRegsWritten == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glLoadProgramNV(c[#] not written)");
            return;
         }
      }
      else {
         if (!IsPositionInvariant && !(outputsWritten & 1)) {
            /* bit 1 = HPOS register */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glLoadProgramNV(HPOS not written)");
            return;
         }
      }

      program->InputsRead = inputsRead;
      program->OutputsWritten = outputsWritten;
      program->IsPositionInvariant = IsPositionInvariant;

      /* copy the compiled instructions */
      assert(numInst <= MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS);
      newInst = (struct vp_instruction *) MALLOC(numInst * sizeof(struct vp_instruction));
      if (!newInst) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
         FREE(programString);
         return;  /* out of memory */
      }
      MEMCPY(newInst, instBuffer, numInst * sizeof(struct vp_instruction));

      /* install the program */
      program->Base.Target = target;
      if (program->Base.String) {
         FREE(program->Base.String);
      }
      program->Base.String = programString;
      if (program->Instructions) {
         FREE(program->Instructions);
      }
      program->Instructions = newInst;

#ifdef DEBUG_foo
      _mesa_printf("--- glLoadProgramNV result ---\n");
      _mesa_print_nv_vertex_program(program);
      _mesa_printf("------------------------------\n");
#endif
   }
   else {
      /* Error! */
#ifdef DEBUG
      /* print a message showing the program line containing the error */
      ctx->Program.ErrorPos = s - str;
      {
         const GLubyte *p = str, *line = str;
         int lineNum = 1, statementNum = 1, column = 0;
         char errorLine[1000];
         int i;
         while (*p && p < s) {  /* s is the error position */
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
         _mesa_debug(ctx, "Vertex program error on line %2d: %s\n", lineNum, errorLine);
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
PrintSrcReg(const struct vp_src_register *src)
{
   static const char comps[5] = "xyzw";
   if (src->Negate)
      _mesa_printf("-");
   if (src->RelAddr) {
      if (src->Register > 0)
         _mesa_printf("c[A0.x + %d]", src->Register);
      else if (src->Register < 0)
         _mesa_printf("c[A0.x - %d]", -src->Register);
      else
         _mesa_printf("c[A0.x]");
   }
   else if (src->Register >= VP_OUTPUT_REG_START
       && src->Register <= VP_OUTPUT_REG_END) {
      _mesa_printf("o[%s]", OutputRegisters[src->Register - VP_OUTPUT_REG_START]);
   }
   else if (src->Register >= VP_INPUT_REG_START
            && src->Register <= VP_INPUT_REG_END) {
      _mesa_printf("v[%s]", InputRegisters[src->Register - VP_INPUT_REG_START]);
   }
   else if (src->Register >= VP_PROG_REG_START
            && src->Register <= VP_PROG_REG_END) {
      _mesa_printf("c[%d]", src->Register - VP_PROG_REG_START);
   }
   else {
      _mesa_printf("R%d", src->Register - VP_TEMP_REG_START);
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
}


static void
PrintDstReg(const struct vp_dst_register *dst)
{
   GLint w = dst->WriteMask[0] + dst->WriteMask[1]
           + dst->WriteMask[2] + dst->WriteMask[3];

   if (dst->Register >= VP_OUTPUT_REG_START
       && dst->Register <= VP_OUTPUT_REG_END) {
      _mesa_printf("o[%s]", OutputRegisters[dst->Register - VP_OUTPUT_REG_START]);
   }
   else if (dst->Register >= VP_INPUT_REG_START
            && dst->Register <= VP_INPUT_REG_END) {
      _mesa_printf("v[%s]", InputRegisters[dst->Register - VP_INPUT_REG_START]);
   }
   else if (dst->Register >= VP_PROG_REG_START
            && dst->Register <= VP_PROG_REG_END) {
      _mesa_printf("c[%d]", dst->Register - VP_PROG_REG_START);
   }
   else {
      _mesa_printf("R%d", dst->Register - VP_TEMP_REG_START);
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
}


/**
 * Print (unparse) the given vertex program.  Just for debugging.
 */
void
_mesa_print_nv_vertex_program(const struct vertex_program *program)
{
   const struct vp_instruction *inst;

   for (inst = program->Instructions; ; inst++) {
      switch (inst->Opcode) {
      case VP_OPCODE_MOV:
      case VP_OPCODE_LIT:
      case VP_OPCODE_RCP:
      case VP_OPCODE_RSQ:
      case VP_OPCODE_EXP:
      case VP_OPCODE_LOG:
      case VP_OPCODE_RCC:
      case VP_OPCODE_ABS:
         _mesa_printf("%s ", Opcodes[(int) inst->Opcode]);
         PrintDstReg(&inst->DstReg);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[0]);
         _mesa_printf(";\n");
         break;
      case VP_OPCODE_MUL:
      case VP_OPCODE_ADD:
      case VP_OPCODE_DP3:
      case VP_OPCODE_DP4:
      case VP_OPCODE_DST:
      case VP_OPCODE_MIN:
      case VP_OPCODE_MAX:
      case VP_OPCODE_SLT:
      case VP_OPCODE_SGE:
      case VP_OPCODE_DPH:
      case VP_OPCODE_SUB:
         _mesa_printf("%s ", Opcodes[(int) inst->Opcode]);
         PrintDstReg(&inst->DstReg);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[0]);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[1]);
         _mesa_printf(";\n");
         break;
      case VP_OPCODE_MAD:
         _mesa_printf("MAD ");
         PrintDstReg(&inst->DstReg);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[0]);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[1]);
         _mesa_printf(", ");
         PrintSrcReg(&inst->SrcReg[2]);
         _mesa_printf(";\n");
         break;
      case VP_OPCODE_ARL:
         _mesa_printf("ARL A0.x, ");
         PrintSrcReg(&inst->SrcReg[0]);
         _mesa_printf(";\n");
         break;
      case VP_OPCODE_END:
         _mesa_printf("END\n");
         return;
      default:
         _mesa_printf("BAD INSTRUCTION\n");
      }
   }
}

