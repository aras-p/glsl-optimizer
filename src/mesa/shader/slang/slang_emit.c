/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * \file slang_emit.c
 * Emit program instructions (PI code) from IR trees.
 * \author Brian Paul
 */

#include "imports.h"
#include "context.h"
#include "macros.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "slang_emit.h"
#include "slang_error.h"


#define PEEPHOLE_OPTIMIZATIONS 1
#define ANNOTATE 1


/**
 * Assembly and IR info
 */
typedef struct
{
   slang_ir_opcode IrOpcode;
   const char *IrName;
   gl_inst_opcode InstOpcode;
   GLuint ResultSize, NumParams;
} slang_ir_info;



static slang_ir_info IrInfo[] = {
   /* binary ops */
   { IR_ADD, "IR_ADD", OPCODE_ADD, 4, 2 },
   { IR_SUB, "IR_SUB", OPCODE_SUB, 4, 2 },
   { IR_MUL, "IR_MUL", OPCODE_MUL, 4, 2 },
   { IR_DIV, "IR_DIV", OPCODE_NOP, 0, 2 }, /* XXX broke */
   { IR_DOT4, "IR_DOT_4", OPCODE_DP4, 1, 2 },
   { IR_DOT3, "IR_DOT_3", OPCODE_DP3, 1, 2 },
   { IR_CROSS, "IR_CROSS", OPCODE_XPD, 3, 2 },
   { IR_LRP, "IR_LRP", OPCODE_LRP, 4, 3 },
   { IR_MIN, "IR_MIN", OPCODE_MIN, 4, 2 },
   { IR_MAX, "IR_MAX", OPCODE_MAX, 4, 2 },
   { IR_CLAMP, "IR_CLAMP", OPCODE_NOP, 4, 3 }, /* special case: emit_clamp() */
   { IR_SEQUAL, "IR_SEQUAL", OPCODE_SEQ, 4, 2 },
   { IR_SNEQUAL, "IR_SNEQUAL", OPCODE_SNE, 4, 2 },
   { IR_SGE, "IR_SGE", OPCODE_SGE, 4, 2 },
   { IR_SGT, "IR_SGT", OPCODE_SGT, 4, 2 },
   { IR_POW, "IR_POW", OPCODE_POW, 1, 2 },
   /* unary ops */
   { IR_I_TO_F, "IR_I_TO_F", OPCODE_NOP, 1, 1 },
   { IR_F_TO_I, "IR_F_TO_I", OPCODE_INT, 4, 1 }, /* 4 floats to 4 ints */
   { IR_EXP, "IR_EXP", OPCODE_EXP, 1, 1 },
   { IR_EXP2, "IR_EXP2", OPCODE_EX2, 1, 1 },
   { IR_LOG2, "IR_LOG2", OPCODE_LG2, 1, 1 },
   { IR_RSQ, "IR_RSQ", OPCODE_RSQ, 1, 1 },
   { IR_RCP, "IR_RCP", OPCODE_RCP, 1, 1 },
   { IR_FLOOR, "IR_FLOOR", OPCODE_FLR, 4, 1 },
   { IR_FRAC, "IR_FRAC", OPCODE_FRC, 4, 1 },
   { IR_ABS, "IR_ABS", OPCODE_ABS, 4, 1 },
   { IR_NEG, "IR_NEG", OPCODE_NOP, 4, 1 }, /* special case: emit_negation() */
   { IR_DDX, "IR_DDX", OPCODE_DDX, 4, 1 },
   { IR_DDX, "IR_DDY", OPCODE_DDX, 4, 1 },
   { IR_SIN, "IR_SIN", OPCODE_SIN, 1, 1 },
   { IR_COS, "IR_COS", OPCODE_COS, 1, 1 },
   { IR_NOISE1, "IR_NOISE1", OPCODE_NOISE1, 1, 1 },
   { IR_NOISE2, "IR_NOISE2", OPCODE_NOISE2, 1, 1 },
   { IR_NOISE3, "IR_NOISE3", OPCODE_NOISE3, 1, 1 },
   { IR_NOISE4, "IR_NOISE4", OPCODE_NOISE4, 1, 1 },

   /* other */
   { IR_SEQ, "IR_SEQ", OPCODE_NOP, 0, 0 },
   { IR_SCOPE, "IR_SCOPE", OPCODE_NOP, 0, 0 },
   { IR_LABEL, "IR_LABEL", OPCODE_NOP, 0, 0 },
   { IR_JUMP, "IR_JUMP", OPCODE_NOP, 0, 0 },
   { IR_CJUMP0, "IR_CJUMP0", OPCODE_NOP, 0, 0 },
   { IR_CJUMP1, "IR_CJUMP1", OPCODE_NOP, 0, 0 },
   { IR_IF, "IR_IF", OPCODE_NOP, 0, 0 },
   { IR_ELSE, "IR_ELSE", OPCODE_NOP, 0, 0 },
   { IR_ENDIF, "IR_ENDIF", OPCODE_NOP, 0, 0 },
   { IR_KILL, "IR_KILL", OPCODE_NOP, 0, 0 },
   { IR_COND, "IR_COND", OPCODE_NOP, 0, 0 },
   { IR_CALL, "IR_CALL", OPCODE_NOP, 0, 0 },
   { IR_MOVE, "IR_MOVE", OPCODE_NOP, 0, 1 },
   { IR_NOT, "IR_NOT", OPCODE_NOP, 1, 1 },
   { IR_VAR, "IR_VAR", OPCODE_NOP, 0, 0 },
   { IR_VAR_DECL, "IR_VAR_DECL", OPCODE_NOP, 0, 0 },
   { IR_TEX, "IR_TEX", OPCODE_TEX, 4, 1 },
   { IR_TEXB, "IR_TEXB", OPCODE_TXB, 4, 1 },
   { IR_TEXP, "IR_TEXP", OPCODE_TXP, 4, 1 },
   { IR_FLOAT, "IR_FLOAT", OPCODE_NOP, 0, 0 },
   { IR_FIELD, "IR_FIELD", OPCODE_NOP, 0, 0 },
   { IR_ELEMENT, "IR_ELEMENT", OPCODE_NOP, 0, 0 },
   { IR_SWIZZLE, "IR_SWIZZLE", OPCODE_NOP, 0, 0 },
   { IR_NOP, NULL, OPCODE_NOP, 0, 0 }
};


static slang_ir_info *
slang_find_ir_info(slang_ir_opcode opcode)
{
   GLuint i;
   for (i = 0; IrInfo[i].IrName; i++) {
      if (IrInfo[i].IrOpcode == opcode) {
	 return IrInfo + i;
      }
   }
   return NULL;
}

static const char *
slang_ir_name(slang_ir_opcode opcode)
{
   return slang_find_ir_info(opcode)->IrName;
}


/**
 * Swizzle a swizzle.  That is, return swz2(swz1)
 */
static GLuint
swizzle_swizzle(GLuint swz1, GLuint swz2)
{
   GLuint i, swz, s[4];
   for (i = 0; i < 4; i++) {
      GLuint c = GET_SWZ(swz2, i);
      s[i] = GET_SWZ(swz1, c);
   }
   swz = MAKE_SWIZZLE4(s[0], s[1], s[2], s[3]);
   return swz;
}


slang_ir_storage *
_slang_new_ir_storage(enum register_file file, GLint index, GLint size)
{
   slang_ir_storage *st;
   st = (slang_ir_storage *) _mesa_calloc(sizeof(slang_ir_storage));
   if (st) {
      st->File = file;
      st->Index = index;
      st->Size = size;
      st->Swizzle = SWIZZLE_NOOP;
   }
   return st;
}


static const char *
swizzle_string(GLuint swizzle)
{
   static char s[6];
   GLuint i;
   s[0] = '.';
   for (i = 1; i < 5; i++) {
      s[i] = "xyzw"[GET_SWZ(swizzle, i-1)];
   }
   s[i] = 0;
   return s;
}

static const char *
writemask_string(GLuint writemask)
{
   static char s[6];
   GLuint i, j = 0;
   s[j++] = '.';
   for (i = 0; i < 4; i++) {
      if (writemask & (1 << i))
         s[j++] = "xyzw"[i];
   }
   s[j] = 0;
   return s;
}

static const char *
storage_string(const slang_ir_storage *st)
{
   static const char *files[] = {
      "TEMP",
      "LOCAL_PARAM",
      "ENV_PARAM",
      "STATE",
      "INPUT",
      "OUTPUT",
      "NAMED_PARAM",
      "CONSTANT",
      "UNIFORM",
      "WRITE_ONLY",
      "ADDRESS",
      "SAMPLER",
      "UNDEFINED"
   };
   static char s[100];
#if 0
   if (st->Size == 1)
      sprintf(s, "%s[%d]", files[st->File], st->Index);
   else
      sprintf(s, "%s[%d..%d]", files[st->File], st->Index,
              st->Index + st->Size - 1);
#endif
   assert(st->File < (GLint) (sizeof(files) / sizeof(files[0])));
   sprintf(s, "%s[%d]", files[st->File], st->Index);
   return s;
}


#define IND 0
void
slang_print_ir(const slang_ir_node *n, int indent)
{
   int i;
   if (!n)
      return;
#if !IND
   if (n->Opcode != IR_SEQ)
#else
      printf("%3d:", indent);
#endif
      for (i = 0; i < indent; i++)
	 printf(" ");

   switch (n->Opcode) {
   case IR_SEQ:
#if IND
      printf("SEQ  at %p\n", (void*) n);
#endif
      assert(n->Children[0]);
      assert(n->Children[1]);
      slang_print_ir(n->Children[0], indent + IND);
      slang_print_ir(n->Children[1], indent + IND);
      break;
   case IR_SCOPE:
      printf("NEW SCOPE\n");
      assert(!n->Children[1]);
      slang_print_ir(n->Children[0], indent + 3);
      break;
   case IR_MOVE:
      printf("MOVE (writemask = %s)\n", writemask_string(n->Writemask));
      slang_print_ir(n->Children[0], indent+3);
      slang_print_ir(n->Children[1], indent+3);
      break;
   case IR_LABEL:
      printf("LABEL: %s\n", n->Target);
      break;
   case IR_COND:
      printf("COND\n");
      slang_print_ir(n->Children[0], indent + 3);
      break;
   case IR_JUMP:
      printf("JUMP %s\n", n->Target);
      break;
   case IR_CJUMP0:
      printf("CJUMP0 %s\n", n->Target);
      slang_print_ir(n->Children[0], indent+3);
      break;
   case IR_CJUMP1:
      printf("CJUMP1 %s\n", n->Target);
      slang_print_ir(n->Children[0], indent+3);
      break;

   case IR_IF:
      printf("IF \n");
      slang_print_ir(n->Children[0], indent+3);
      break;
   case IR_ELSE:
      printf("ELSE\n");
      break;
   case IR_ENDIF:
      printf("ENDIF\n");
      break;

   case IR_BEGIN_SUB:
      printf("BEGIN_SUB\n");
      break;
   case IR_END_SUB:
      printf("END_SUB\n");
      break;
   case IR_RETURN:
      printf("RETURN\n");
      break;
   case IR_CALL:
      printf("CALL\n");
      break;

   case IR_BEGIN_LOOP:
      printf("BEGIN_LOOP\n");
      break;
   case IR_END_LOOP:
      printf("END_LOOP\n");
      break;
   case IR_CONT:
      printf("CONT\n");
      break;
   case IR_BREAK:
      printf("BREAK\n");
      break;

   case IR_VAR:
      printf("VAR %s%s at %s  store %p\n",
             (n->Var ? (char *) n->Var->a_name : "TEMP"),
             swizzle_string(n->Store->Swizzle),
             storage_string(n->Store), (void*) n->Store);
      break;
   case IR_VAR_DECL:
      printf("VAR_DECL %s (%p) at %s  store %p\n",
             (n->Var ? (char *) n->Var->a_name : "TEMP"),
             (void*) n->Var, storage_string(n->Store),
             (void*) n->Store);
      break;
   case IR_FIELD:
      printf("FIELD %s of\n", n->Target);
      slang_print_ir(n->Children[0], indent+3);
      break;
   case IR_FLOAT:
      printf("FLOAT %f %f %f %f\n",
             n->Value[0], n->Value[1], n->Value[2], n->Value[3]);
      break;
   case IR_I_TO_F:
      printf("INT_TO_FLOAT %d\n", (int) n->Value[0]);
      break;
   case IR_SWIZZLE:
      printf("SWIZZLE %s of  (store %p) \n",
             swizzle_string(n->Store->Swizzle), (void*) n->Store);
      slang_print_ir(n->Children[0], indent + 3);
      break;
   default:
      printf("%s (%p, %p)  (store %p)\n", slang_ir_name(n->Opcode),
             (void*) n->Children[0], (void*) n->Children[1], (void*) n->Store);
      slang_print_ir(n->Children[0], indent+3);
      slang_print_ir(n->Children[1], indent+3);
   }
}


/**
 * Allocate temporary storage for an intermediate result (such as for
 * a multiply or add, etc.
 */
static GLboolean
alloc_temp_storage(slang_var_table *vt, slang_ir_node *n, GLint size)
{
   assert(!n->Var);
   assert(!n->Store);
   assert(size > 0);
   n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, size);
   if (!_slang_alloc_temp(vt, n->Store)) {
      RETURN_ERROR("Ran out of registers, too many temporaries", 0);
   }
   return GL_TRUE;
}


/**
 * Free temporary storage, if n->Store is, in fact, temp storage.
 * Otherwise, no-op.
 */
static void
free_temp_storage(slang_var_table *vt, slang_ir_node *n)
{
   if (n->Store->File == PROGRAM_TEMPORARY && n->Store->Index >= 0) {
      if (_slang_is_temp(vt, n->Store)) {
         _slang_free_temp(vt, n->Store);
         n->Store->Index = -1;
         n->Store->Size = -1;
      }
   }
}


/**
 * Convert IR storage to an instruction dst register.
 */
static void
storage_to_dst_reg(struct prog_dst_register *dst, const slang_ir_storage *st,
                   GLuint writemask)
{
   static const GLuint defaultWritemask[4] = {
      WRITEMASK_X,
      WRITEMASK_X | WRITEMASK_Y,
      WRITEMASK_X | WRITEMASK_Y | WRITEMASK_Z,
      WRITEMASK_X | WRITEMASK_Y | WRITEMASK_Z | WRITEMASK_W
   };
   assert(st->Index >= 0 && st->Index <= 16);
   dst->File = st->File;
   dst->Index = st->Index;
   assert(st->File != PROGRAM_UNDEFINED);
   assert(st->Size >= 1);
   assert(st->Size <= 4);
   if (st->Size == 1) {
      GLuint comp = GET_SWZ(st->Swizzle, 0);
      assert(comp < 4);
      assert(writemask & WRITEMASK_X);
      dst->WriteMask = WRITEMASK_X << comp;
   }
   else {
      dst->WriteMask = defaultWritemask[st->Size - 1] & writemask;
   }
}


/**
 * Convert IR storage to an instruction src register.
 */
static void
storage_to_src_reg(struct prog_src_register *src, const slang_ir_storage *st)
{
   static const GLuint defaultSwizzle[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W)
   };
   assert(st->File >= 0 && st->File <= 16);
   src->File = st->File;
   src->Index = st->Index;
   assert(st->File != PROGRAM_UNDEFINED);
   assert(st->Size >= 1);
   assert(st->Size <= 4);
   if (st->Swizzle != SWIZZLE_NOOP)
      src->Swizzle = st->Swizzle;
   else
      src->Swizzle = defaultSwizzle[st->Size - 1]; /*XXX really need this?*/

   assert(GET_SWZ(src->Swizzle, 0) != SWIZZLE_NIL);
   assert(GET_SWZ(src->Swizzle, 1) != SWIZZLE_NIL);
   assert(GET_SWZ(src->Swizzle, 2) != SWIZZLE_NIL);
   assert(GET_SWZ(src->Swizzle, 3) != SWIZZLE_NIL);
}



/**
 * Add new instruction at end of given program.
 * \param prog  the program to append instruction onto
 * \param opcode  opcode for the new instruction
 * \return pointer to the new instruction
 */
static struct prog_instruction *
new_instruction(struct gl_program *prog, gl_inst_opcode opcode)
{
   struct prog_instruction *inst;
   prog->Instructions = _mesa_realloc_instructions(prog->Instructions,
                                                   prog->NumInstructions,
                                                   prog->NumInstructions + 1);
   inst = prog->Instructions + prog->NumInstructions;
   prog->NumInstructions++;
   _mesa_init_instructions(inst, 1);
   inst->Opcode = opcode;
   return inst;
}


/**
 * Return pointer to last instruction in program.
 */
static struct prog_instruction *
prev_instruction(struct gl_program *prog)
{
   if (prog->NumInstructions == 0)
      return NULL;
   else
      return prog->Instructions + prog->NumInstructions - 1;
}


static struct prog_instruction *
emit(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog);


/**
 * Return an annotation string for given node's storage.
 */
static char *
storage_annotation(const slang_ir_node *n, const struct gl_program *prog)
{
#if ANNOTATE
   const slang_ir_storage *st = n->Store;
   static char s[100] = "";

   if (!st)
      return _mesa_strdup("");

   switch (st->File) {
   case PROGRAM_CONSTANT:
      if (st->Index >= 0) {
         const GLfloat *val = prog->Parameters->ParameterValues[st->Index];
         if (st->Swizzle == SWIZZLE_NOOP)
            sprintf(s, "{%f, %f, %f, %f}", val[0], val[1], val[2], val[3]);
         else {
            sprintf(s, "%f", val[GET_SWZ(st->Swizzle, 0)]);
         }
      }
      break;
   case PROGRAM_TEMPORARY:
      if (n->Var)
         sprintf(s, "%s", (char *) n->Var->a_name);
      else
         sprintf(s, "t[%d]", st->Index);
      break;
   case PROGRAM_STATE_VAR:
   case PROGRAM_UNIFORM:
      sprintf(s, "%s", prog->Parameters->Parameters[st->Index].Name);
      break;
   case PROGRAM_VARYING:
      sprintf(s, "%s", prog->Varying->Parameters[st->Index].Name);
      break;
   case PROGRAM_INPUT:
      sprintf(s, "input[%d]", st->Index);
      break;
   case PROGRAM_OUTPUT:
      sprintf(s, "output[%d]", st->Index);
      break;
   default:
      s[0] = 0;
   }
   return _mesa_strdup(s);
#else
   return NULL;
#endif
}


/**
 * Return an annotation string for an instruction.
 */
static char *
instruction_annotation(gl_inst_opcode opcode, char *dstAnnot,
                       char *srcAnnot0, char *srcAnnot1, char *srcAnnot2)
{
#if ANNOTATE
   const char *operator;
   char *s;
   int len = 50;

   if (dstAnnot)
      len += strlen(dstAnnot);
   else
      dstAnnot = _mesa_strdup("");

   if (srcAnnot0)
      len += strlen(srcAnnot0);
   else
      srcAnnot0 = _mesa_strdup("");

   if (srcAnnot1)
      len += strlen(srcAnnot1);
   else
      srcAnnot1 = _mesa_strdup("");

   if (srcAnnot2)
      len += strlen(srcAnnot2);
   else
      srcAnnot2 = _mesa_strdup("");

   switch (opcode) {
   case OPCODE_ADD:
      operator = "+";
      break;
   case OPCODE_SUB:
      operator = "-";
      break;
   case OPCODE_MUL:
      operator = "*";
      break;
   case OPCODE_DP3:
      operator = "DP3";
      break;
   case OPCODE_DP4:
      operator = "DP4";
      break;
   case OPCODE_XPD:
      operator = "XPD";
      break;
   case OPCODE_RSQ:
      operator = "RSQ";
      break;
   case OPCODE_SGT:
      operator = ">";
      break;
   default:
      operator = ",";
   }

   s = (char *) malloc(len);
   sprintf(s, "%s = %s %s %s %s", dstAnnot,
           srcAnnot0, operator, srcAnnot1, srcAnnot2);
   assert(_mesa_strlen(s) < len);

   free(dstAnnot);
   free(srcAnnot0);
   free(srcAnnot1);
   free(srcAnnot2);

   return s;
#else
   return NULL;
#endif
}



/**
 * Generate code for a simple arithmetic instruction.
 * Either 1, 2 or 3 operands.
 */
static struct prog_instruction *
emit_arith(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   const slang_ir_info *info = slang_find_ir_info(n->Opcode);
   char *srcAnnot[3], *dstAnnot;
   GLuint i;

   assert(info);
   assert(info->InstOpcode != OPCODE_NOP);

   srcAnnot[0] = srcAnnot[1] = srcAnnot[2] = dstAnnot = NULL;

#if PEEPHOLE_OPTIMIZATIONS
   /* Look for MAD opportunity */
   if (info->NumParams == 2 &&
       n->Opcode == IR_ADD && n->Children[0]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(IR_MUL(A, B), C) */
      emit(vt, n->Children[0]->Children[0], prog);  /* A */
      emit(vt, n->Children[0]->Children[1], prog);  /* B */
      emit(vt, n->Children[1], prog);  /* C */
      /* generate MAD instruction */
      inst = new_instruction(prog, OPCODE_MAD);
      /* operands: A, B, C: */
      storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[0]->Children[1]->Store);
      storage_to_src_reg(&inst->SrcReg[2], n->Children[1]->Store);
      free_temp_storage(vt, n->Children[0]->Children[0]);
      free_temp_storage(vt, n->Children[0]->Children[1]);
      free_temp_storage(vt, n->Children[1]);
   }
   else if (info->NumParams == 2 &&
            n->Opcode == IR_ADD && n->Children[1]->Opcode == IR_MUL) {
      /* found pattern IR_ADD(A, IR_MUL(B, C)) */
      emit(vt, n->Children[0], prog);  /* A */
      emit(vt, n->Children[1]->Children[0], prog);  /* B */
      emit(vt, n->Children[1]->Children[1], prog);  /* C */
      /* generate MAD instruction */
      inst = new_instruction(prog, OPCODE_MAD);
      /* operands: B, C, A */
      storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Children[0]->Store);
      storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Children[1]->Store);
      storage_to_src_reg(&inst->SrcReg[2], n->Children[0]->Store);
      free_temp_storage(vt, n->Children[1]->Children[0]);
      free_temp_storage(vt, n->Children[1]->Children[1]);
      free_temp_storage(vt, n->Children[0]);
   }
   else
#endif
   {
      /* normal case */

      /* gen code for children */
      for (i = 0; i < info->NumParams; i++)
         emit(vt, n->Children[i], prog);

      /* gen this instruction and src registers */
      inst = new_instruction(prog, info->InstOpcode);
      for (i = 0; i < info->NumParams; i++)
         storage_to_src_reg(&inst->SrcReg[i], n->Children[i]->Store);

      /* annotation */
      for (i = 0; i < info->NumParams; i++)
         srcAnnot[i] = storage_annotation(n->Children[i], prog);

      /* free temps */
      for (i = 0; i < info->NumParams; i++)
         free_temp_storage(vt, n->Children[i]);
   }

   /* result storage */
   if (!n->Store) {
      if (!alloc_temp_storage(vt, n, info->ResultSize))
         return NULL;
   }
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   dstAnnot = storage_annotation(n, prog);

   inst->Comment = instruction_annotation(inst->Opcode, dstAnnot, srcAnnot[0],
                                          srcAnnot[1], srcAnnot[2]);

   /*_mesa_print_instruction(inst);*/
   return inst;
}


/**
 * Generate code for an IR_CLAMP instruction.
 */
static struct prog_instruction *
emit_clamp(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;

   assert(n->Opcode == IR_CLAMP);
   /* ch[0] = value
    * ch[1] = min limit
    * ch[2] = max limit
    */

   inst = emit(vt, n->Children[0], prog);

   /* If lower limit == 0.0 and upper limit == 1.0,
    *    set prev instruction's SaturateMode field to SATURATE_ZERO_ONE.
    * Else,
    *    emit OPCODE_MIN, OPCODE_MAX sequence.
    */
#if 0
   /* XXX this isn't quite finished yet */
   if (n->Children[1]->Opcode == IR_FLOAT &&
       n->Children[1]->Value[0] == 0.0 &&
       n->Children[1]->Value[1] == 0.0 &&
       n->Children[1]->Value[2] == 0.0 &&
       n->Children[1]->Value[3] == 0.0 &&
       n->Children[2]->Opcode == IR_FLOAT &&
       n->Children[2]->Value[0] == 1.0 &&
       n->Children[2]->Value[1] == 1.0 &&
       n->Children[2]->Value[2] == 1.0 &&
       n->Children[2]->Value[3] == 1.0) {
      if (!inst) {
         inst = prev_instruction(prog);
      }
      if (inst && inst->Opcode != OPCODE_NOP) {
         /* and prev instruction's DstReg matches n->Children[0]->Store */
         inst->SaturateMode = SATURATE_ZERO_ONE;
         n->Store = n->Children[0]->Store;
         return inst;
      }
   }
#endif

   if (!n->Store)
      if (!alloc_temp_storage(vt, n, n->Children[0]->Store->Size))
         return NULL;

   emit(vt, n->Children[1], prog);
   emit(vt, n->Children[2], prog);

   /* tmp = max(ch[0], ch[1]) */
   inst = new_instruction(prog, OPCODE_MAX);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store);

   /* tmp = min(tmp, ch[2]) */
   inst = new_instruction(prog, OPCODE_MIN);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Store);
   storage_to_src_reg(&inst->SrcReg[1], n->Children[2]->Store);

   return inst;
}


static struct prog_instruction *
emit_negation(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   /* Implement as MOV dst, -src; */
   /* XXX we could look at the previous instruction and in some circumstances
    * modify it to accomplish the negation.
    */
   struct prog_instruction *inst;

   emit(vt, n->Children[0], prog);

   if (!n->Store)
      if (!alloc_temp_storage(vt, n, n->Children[0]->Store->Size))
         return NULL;

   inst = new_instruction(prog, OPCODE_MOV);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   inst->SrcReg[0].NegateBase = NEGATE_XYZW;
   inst->Comment = n->Comment;
   return inst;
}


static struct prog_instruction *
emit_label(const char *target, struct gl_program *prog)
{
   struct prog_instruction *inst;
   inst = new_instruction(prog, OPCODE_NOP);
   inst->Comment = _mesa_strdup(target);
   return inst;
}


static struct prog_instruction *
emit_cjump(const char *target, struct gl_program *prog, GLuint zeroOrOne)
{
   struct prog_instruction *inst;
   inst = new_instruction(prog, OPCODE_BRA);
   if (zeroOrOne)
      inst->DstReg.CondMask = COND_NE;  /* branch if non-zero */
   else
      inst->DstReg.CondMask = COND_EQ;  /* branch if equal to zero */
   inst->DstReg.CondSwizzle = SWIZZLE_X;
   inst->Comment = _mesa_strdup(target);
   return inst;
}


static struct prog_instruction *
emit_jump(const char *target, struct gl_program *prog)
{
   struct prog_instruction *inst;
   inst = new_instruction(prog, OPCODE_BRA);
   inst->DstReg.CondMask = COND_TR;  /* always branch */
   /*inst->DstReg.CondSwizzle = SWIZZLE_X;*/
   inst->Comment = _mesa_strdup(target);
   return inst;
}


static struct prog_instruction *
emit_kill(struct gl_program *prog)
{
   struct prog_instruction *inst;
   /* NV-KILL - discard fragment depending on condition code.
    * Note that ARB-KILL depends on sign of vector operand.
    */
   inst = new_instruction(prog, OPCODE_KIL_NV);
   inst->DstReg.CondMask = COND_TR;  /* always branch */
   return inst;
}


static struct prog_instruction *
emit_tex(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   if (n->Opcode == IR_TEX) {
      inst = new_instruction(prog, OPCODE_TEX);
   }
   else if (n->Opcode == IR_TEXB) {
      inst = new_instruction(prog, OPCODE_TXB);
   }
   else {
      assert(n->Opcode == IR_TEXP);
      inst = new_instruction(prog, OPCODE_TXP);
   }

   if (!n->Store)
      if (!alloc_temp_storage(vt, n, 4))
         return NULL;

   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   (void) emit(vt, n->Children[1], prog);

   /* Child[1] is the coord */
   storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store);

   /* Child[0] is the sampler (a uniform which'll indicate the texture unit) */
   assert(n->Children[0]->Store);
   assert(n->Children[0]->Store->Size >= TEXTURE_1D_INDEX);

   inst->Sampler = n->Children[0]->Store->Index; /* i.e. uniform's index */
   inst->TexSrcTarget = n->Children[0]->Store->Size;
   inst->TexSrcUnit = 27; /* Dummy value; the TexSrcUnit will be computed at
                           * link time, using the sampler uniform's value.
                           */
   return inst;
}


static struct prog_instruction *
emit_move(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;

   /* rhs */
   assert(n->Children[1]);
   inst = emit(vt, n->Children[1], prog);

   assert(n->Children[1]->Store->Index >= 0);

   /* lhs */
   emit(vt, n->Children[0], prog);

   assert(!n->Store);
   n->Store = n->Children[0]->Store;

#if PEEPHOLE_OPTIMIZATIONS
   if (inst && _slang_is_temp(vt, n->Children[1]->Store)) {
      /* Peephole optimization:
       * Just modify the RHS to put its result into the dest of this
       * MOVE operation.  Then, this MOVE is a no-op.
       */
      _slang_free_temp(vt, n->Children[1]->Store);
      *n->Children[1]->Store = *n->Children[0]->Store;
      /* fixup the prev (RHS) instruction */
      assert(n->Children[0]->Store->Index >= 0);
      assert(n->Children[0]->Store->Index < 16);
      storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
      return inst;
   }
   else
#endif
   {
      if (n->Children[0]->Store->Size > 4) {
         /* move matrix/struct etc (block of registers) */
         slang_ir_storage dstStore = *n->Children[0]->Store;
         slang_ir_storage srcStore = *n->Children[1]->Store;
         GLint size = srcStore.Size;
         ASSERT(n->Children[0]->Writemask == WRITEMASK_XYZW);
         ASSERT(n->Children[1]->Store->Swizzle == SWIZZLE_NOOP);
         dstStore.Size = 4;
         srcStore.Size = 4;
         while (size >= 4) {
            inst = new_instruction(prog, OPCODE_MOV);
            inst->Comment = _mesa_strdup("IR_MOVE block");
            storage_to_dst_reg(&inst->DstReg, &dstStore, n->Writemask);
            storage_to_src_reg(&inst->SrcReg[0], &srcStore);
            srcStore.Index++;
            dstStore.Index++;
            size -= 4;
         }
      }
      else {
         /* single register move */
         char *srcAnnot, *dstAnnot;
         inst = new_instruction(prog, OPCODE_MOV);
         assert(n->Children[0]->Store->Index >= 0);
         assert(n->Children[0]->Store->Index < 16);
         storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
         storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store);
         dstAnnot = storage_annotation(n->Children[0], prog);
         srcAnnot = storage_annotation(n->Children[1], prog);
         inst->Comment = instruction_annotation(inst->Opcode, dstAnnot,
                                                srcAnnot, NULL, NULL);
      }
      free_temp_storage(vt, n->Children[1]);
      return inst;
   }
}


static struct prog_instruction *
emit_cond(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   /* Conditional expression (in if/while/for stmts).
    * Need to update condition code register.
    * Next instruction is typically an IR_CJUMP0/1.
    */
   /* last child expr instruction: */
   struct prog_instruction *inst = emit(vt, n->Children[0], prog);
   if (inst) {
      /* set inst's CondUpdate flag */
      inst->CondUpdate = GL_TRUE;
      return inst; /* XXX or null? */
   }
   else {
      /* This'll happen for things like "if (i) ..." where no code
       * is normally generated for the expression "i".
       * Generate a move instruction just to set condition codes.
       * Note: must use full 4-component vector since all four
       * condition codes must be set identically.
       */
      if (!alloc_temp_storage(vt, n, 4))
         return NULL;
      inst = new_instruction(prog, OPCODE_MOV);
      inst->CondUpdate = GL_TRUE;
      storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
      storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
      _slang_free_temp(vt, n->Store);
      inst->Comment = _mesa_strdup("COND expr");
      return inst; /* XXX or null? */
   }
}


/**
 * Logical-NOT
 */
static struct prog_instruction *
emit_not(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   GLfloat zero = 0.0;
   slang_ir_storage st;
   struct prog_instruction *inst;

   /* need zero constant */
   st.File = PROGRAM_CONSTANT;
   st.Size = 1;
   st.Index = _mesa_add_unnamed_constant(prog->Parameters, &zero,
                                         1, &st.Swizzle);

   /* child expr */
   (void) emit(vt, n->Children[0], prog);
   /* XXXX if child instr is SGT convert to SLE, if SEQ, SNE, etc */

   if (!n->Store)
      if (!alloc_temp_storage(vt, n, n->Children[0]->Store->Size))
         return NULL;

   inst = new_instruction(prog, OPCODE_SEQ);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store);
   storage_to_src_reg(&inst->SrcReg[1], &st);

   free_temp_storage(vt, n->Children[0]);

   inst->Comment = _mesa_strdup("NOT");
   return inst;
}



/**
 * Remove any SWIZZLE_NIL terms from given swizzle mask (smear prev term).
 * Ex: fix_swizzle("zyNN") -> "zyyy"
 */
static GLuint
fix_swizzle(GLuint swizzle)
{
   GLuint swz[4], i;
   for (i = 0; i < 4; i++) {
      swz[i] = GET_SWZ(swizzle, i);
      if (swz[i] == SWIZZLE_NIL) {
         swz[i] = swz[i - 1];
      }
   }
   return MAKE_SWIZZLE4(swz[0], swz[1], swz[2], swz[3]);
}


static struct prog_instruction *
emit_swizzle(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   GLuint swizzle;

   /* swizzled storage access */
   (void) emit(vt, n->Children[0], prog);

   /* "pull-up" the child's storage info, applying our swizzle info */
   n->Store->File  = n->Children[0]->Store->File;
   n->Store->Index = n->Children[0]->Store->Index;
   n->Store->Size  = n->Children[0]->Store->Size;
   /*n->Var = n->Children[0]->Var; XXX for debug */
   assert(n->Store->Index >= 0);

   swizzle = fix_swizzle(n->Store->Swizzle);
#ifdef DEBUG
   {
      GLuint s = n->Children[0]->Store->Swizzle;
      assert(GET_SWZ(s, 0) != SWIZZLE_NIL);
      assert(GET_SWZ(s, 1) != SWIZZLE_NIL);
      assert(GET_SWZ(s, 2) != SWIZZLE_NIL);
      assert(GET_SWZ(s, 3) != SWIZZLE_NIL);
   }
#endif

   /* apply this swizzle to child's swizzle to get composed swizzle */
   n->Store->Swizzle = swizzle_swizzle(n->Children[0]->Store->Swizzle,
                                       swizzle);
   return NULL;
}


static struct prog_instruction *
emit(slang_var_table *vt, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   if (!n)
      return NULL;

   switch (n->Opcode) {
   case IR_SEQ:
      /* sequence of two sub-trees */
      assert(n->Children[0]);
      assert(n->Children[1]);
      emit(vt, n->Children[0], prog);
      inst = emit(vt, n->Children[1], prog);
      assert(!n->Store);
      n->Store = n->Children[1]->Store;
      return inst;

   case IR_SCOPE:
      /* new variable scope */
      _slang_push_var_table(vt);
      inst = emit(vt, n->Children[0], prog);
      _slang_pop_var_table(vt);
      return inst;

   case IR_VAR_DECL:
      /* Variable declaration - allocate a register for it */
      assert(n->Store);
      assert(n->Store->File != PROGRAM_UNDEFINED);
      assert(n->Store->Size > 0);
      assert(n->Store->Index < 0);
      if (!n->Var || n->Var->isTemp) {
         /* a nameless/temporary variable, will be freed after first use */
         if (!_slang_alloc_temp(vt, n->Store))
            RETURN_ERROR("Ran out of registers, too many temporaries", 0);
      }
      else {
         /* a regular variable */
         _slang_add_variable(vt, n->Var);
         if (!_slang_alloc_var(vt, n->Store))
            RETURN_ERROR("Ran out of registers, too many variables", 0);
         /*
         printf("IR_VAR_DECL %s %d store %p\n",
                (char*) n->Var->a_name, n->Store->Index, (void*) n->Store);
         */
         assert(n->Var->aux == n->Store);
      }
      break;

   case IR_VAR:
      /* Reference to a variable
       * Storage should have already been resolved/allocated.
       */
      assert(n->Store);
      assert(n->Store->File != PROGRAM_UNDEFINED);
      if (n->Store->Index < 0) {
         printf("#### VAR %s not allocated!\n", (char*)n->Var->a_name);
      }
      assert(n->Store->Index >= 0);
      assert(n->Store->Size > 0);
      break;

   case IR_ELEMENT:
      /* Dereference array element.  Just resolve storage for the array
       * element represented by this node.
       */
      assert(n->Store);
      assert(n->Store->File != PROGRAM_UNDEFINED);
      assert(n->Store->Size > 0);
      if (n->Children[1]->Opcode == IR_FLOAT) {
         /* OK, constant index */
         const GLint arrayAddr = n->Children[0]->Store->Index;
         const GLint index = (GLint) n->Children[1]->Value[0];
         n->Store->Index = arrayAddr + index;
      }
      else {
         /* Problem: variable index */
         const GLint arrayAddr = n->Children[0]->Store->Index;
         const GLint index = 0;
         _mesa_problem(NULL, "variable array indexes not supported yet!");
         n->Store->Index = arrayAddr + index;
      }
      return NULL; /* no instruction */

   case IR_SWIZZLE:
      return emit_swizzle(vt, n, prog);

   /* Simple arithmetic */
   /* unary */
   case IR_RSQ:
   case IR_RCP:
   case IR_FLOOR:
   case IR_FRAC:
   case IR_F_TO_I:
   case IR_ABS:
   case IR_SIN:
   case IR_COS:
   case IR_DDX:
   case IR_DDY:
   case IR_NOISE1:
   case IR_NOISE2:
   case IR_NOISE3:
   case IR_NOISE4:
   /* binary */
   case IR_ADD:
   case IR_SUB:
   case IR_MUL:
   case IR_DOT4:
   case IR_DOT3:
   case IR_CROSS:
   case IR_MIN:
   case IR_MAX:
   case IR_SEQUAL:
   case IR_SNEQUAL:
   case IR_SGE:
   case IR_SGT:
   case IR_POW:
   case IR_EXP:
   case IR_EXP2:
   /* trinary operators */
   case IR_LRP:
      return emit_arith(vt, n, prog);
   case IR_CLAMP:
      return emit_clamp(vt, n, prog);
   case IR_TEX:
   case IR_TEXB:
   case IR_TEXP:
      return emit_tex(vt, n, prog);
   case IR_NEG:
      return emit_negation(vt, n, prog);
   case IR_FLOAT:
      /* find storage location for this float constant */
      n->Store->Index = _mesa_add_unnamed_constant(prog->Parameters, n->Value,
                                                   n->Store->Size,
                                                   &n->Store->Swizzle);
      if (n->Store->Index < 0) {
         RETURN_ERROR("Ran out of space for constants.", 0);
      }
      return NULL;

   case IR_MOVE:
      return emit_move(vt, n, prog);

   case IR_COND:
      return emit_cond(vt, n, prog);

   case IR_NOT:
      return emit_not(vt, n, prog);

   case IR_LABEL:
      return emit_label(n->Target, prog);
   case IR_JUMP:
      return emit_jump(n->Target, prog);
   case IR_CJUMP0:
      return emit_cjump(n->Target, prog, 0);
   case IR_CJUMP1:
      return emit_cjump(n->Target, prog, 1);
   case IR_KILL:
      return emit_kill(prog);

   case IR_IF:
      {
         struct prog_instruction *inst;
         emit(vt, n->Children[0], prog);  /* the condition */
         inst = new_instruction(prog, OPCODE_IF);
         inst->DstReg.CondMask = COND_NE;  /* if cond is non-zero */
         inst->DstReg.CondSwizzle = SWIZZLE_X;
         n->InstLocation = prog->NumInstructions - 1;
         return inst;
      }
   case IR_ELSE:
      {
         struct prog_instruction *inst;
         n->InstLocation = prog->NumInstructions;
         inst = new_instruction(prog, OPCODE_ELSE);
         /* point IF's BranchTarget just after this instruction */
         assert(n->BranchNode);
         assert(n->BranchNode->InstLocation >= 0);
         prog->Instructions[n->BranchNode->InstLocation].BranchTarget = prog->NumInstructions;
         return inst;
      }
   case IR_ENDIF:
      {
         struct prog_instruction *inst;
         n->InstLocation = prog->NumInstructions;
         inst = new_instruction(prog, OPCODE_ENDIF);
         /* point ELSE's BranchTarget to just after this inst */
         assert(n->BranchNode);
         assert(n->BranchNode->InstLocation >= 0);
         prog->Instructions[n->BranchNode->InstLocation].BranchTarget = prog->NumInstructions;
         return inst;
      }

   case IR_BEGIN_LOOP:
      {
         /* save location of this instruction, used by OPCODE_ENDLOOP */
         n->InstLocation = prog->NumInstructions;
         (void) new_instruction(prog, OPCODE_BGNLOOP);
      }
      break;
   case IR_END_LOOP:
      {
         struct prog_instruction *inst;
         inst = new_instruction(prog, OPCODE_ENDLOOP);
         assert(n->BranchNode);
         assert(n->BranchNode->InstLocation >= 0);
         /* The instruction BranchTarget points to top of loop */
         inst->BranchTarget = n->BranchNode->InstLocation;
         return inst;
      }
   case IR_CONT:
      return new_instruction(prog, OPCODE_CONT);
   case IR_BREAK:
      {
         struct prog_instruction *inst;
         inst = new_instruction(prog, OPCODE_BRK);
         inst->DstReg.CondMask = COND_TR;  /* always true */
         return inst;
      }
   case IR_BEGIN_SUB:
      return new_instruction(prog, OPCODE_BGNSUB);
   case IR_END_SUB:
      return new_instruction(prog, OPCODE_ENDSUB);
   case IR_RETURN:
      return new_instruction(prog, OPCODE_RET);

   default:
      _mesa_problem(NULL, "Unexpected IR opcode in emit()\n");
      abort();
   }
   return NULL;
}


GLboolean
_slang_emit_code(slang_ir_node *n, slang_var_table *vt,
                 struct gl_program *prog, GLboolean withEnd)
{
   GLboolean success;

   if (emit(vt, n, prog)) {
      /* finish up by adding the END opcode to program */
      if (withEnd) {
         struct prog_instruction *inst;
         inst = new_instruction(prog, OPCODE_END);
      }
      success = GL_TRUE;
   }
   else {
      /* record an error? */
      success = GL_FALSE;
   }

   printf("*********** End generate code (%u inst):\n", prog->NumInstructions);
#if 0
   _mesa_print_program(prog);
   _mesa_print_program_parameters(ctx,prog);
#endif

   return success;
}
