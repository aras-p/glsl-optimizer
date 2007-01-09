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
#include "slang_emit.h"


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
   { IR_MIN, "IR_MIN", OPCODE_MIN, 4, 2 },
   { IR_MAX, "IR_MAX", OPCODE_MAX, 4, 2 },
   { IR_SEQUAL, "IR_SEQUAL", OPCODE_SEQ, 4, 2 },
   { IR_SNEQUAL, "IR_SNEQUAL", OPCODE_SNE, 4, 2 },
   { IR_SGE, "IR_SGE", OPCODE_SGE, 4, 2 },
   { IR_SGT, "IR_SGT", OPCODE_SGT, 4, 2 },
   { IR_POW, "IR_POW", OPCODE_POW, 1, 2 },
   /* unary ops */
   { IR_I_TO_F, "IR_I_TO_F", OPCODE_NOP, 1, 1 },
   { IR_EXP, "IR_EXP", OPCODE_EXP, 1, 1 },
   { IR_EXP2, "IR_EXP2", OPCODE_EX2, 1, 1 },
   { IR_LOG2, "IR_LOG2", OPCODE_LG2, 1, 1 },
   { IR_RSQ, "IR_RSQ", OPCODE_RSQ, 1, 1 },
   { IR_RCP, "IR_RCP", OPCODE_RCP, 1, 1 },
   { IR_FLOOR, "IR_FLOOR", OPCODE_FLR, 4, 1 },
   { IR_FRAC, "IR_FRAC", OPCODE_FRC, 4, 1 },
   { IR_ABS, "IR_ABS", OPCODE_ABS, 4, 1 },
   { IR_NEG, "IR_NEG", 0/*spec case*/, 4, 1 },
   { IR_DDX, "IR_DDX", OPCODE_DDX, 4, 1 },
   { IR_DDX, "IR_DDY", OPCODE_DDX, 4, 1 },
   { IR_SIN, "IR_SIN", OPCODE_SIN, 1, 1 },
   { IR_COS, "IR_COS", OPCODE_COS, 1, 1 },
   /* other */
   { IR_SEQ, "IR_SEQ", 0, 0, 0 },
   { IR_LABEL, "IR_LABEL", 0, 0, 0 },
   { IR_JUMP, "IR_JUMP", 0, 0, 0 },
   { IR_CJUMP, "IR_CJUMP", 0, 0, 0 },
   { IR_COND, "IR_COND", 0, 0, 0 },
   { IR_CALL, "IR_CALL", 0, 0, 0 },
   { IR_MOVE, "IR_MOVE", 0, 0, 1 },
   { IR_NOT, "IR_NOT", 0, 1, 1 },
   { IR_VAR, "IR_VAR", 0, 0, 0 },
   { IR_VAR_DECL, "IR_VAR_DECL", 0, 0, 0 },
   { IR_TEX, "IR_TEX", OPCODE_TEX, 4, 1 },
   { IR_TEXB, "IR_TEXB", OPCODE_TXB, 4, 1 },
   { IR_TEXP, "IR_TEXP", OPCODE_TXP, 4, 1 },
   { IR_FLOAT, "IR_FLOAT", 0, 0, 0 },
   { IR_FIELD, "IR_FIELD", 0, 0, 0 },
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


slang_ir_storage *
_slang_new_ir_storage(enum register_file file, GLint index, GLint size)
{
   slang_ir_storage *st;
   st = (slang_ir_storage *) _mesa_calloc(sizeof(slang_ir_storage));
   if (st) {
      st->File = file;
      st->Index = index;
      st->Size = size;
   }
   return st;
}


slang_ir_storage *
_slang_clone_ir_storage(slang_ir_storage *store)
{
   slang_ir_storage *clone
      = _slang_new_ir_storage(store->File, store->Index, store->Size);
   return clone;
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
   assert(st->File < sizeof(files) / sizeof(files[0]));
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
   case IR_CJUMP:
      printf("CJUMP %s\n", n->Target);
      slang_print_ir(n->Children[0], indent+3);
      break;
   case IR_VAR:
      printf("VAR %s%s at %s  store %p\n",
             (char *) n->Var->a_name, swizzle_string(n->Swizzle),
             storage_string(n->Store), (void*) n->Store);
      break;
   case IR_VAR_DECL:
      printf("VAR_DECL %s (%p) at %s  store %p\n",
             (char *) n->Var->a_name, (void*) n->Var, storage_string(n->Store),
             (void*) n->Store);
      break;
   case IR_FIELD:
      printf("FIELD %s of\n", n->Target);
      slang_print_ir(n->Children[0], indent+3);
      break;
   case IR_CALL:
      printf("ASMCALL %s(%d args)\n", n->Target, n->Swizzle);
      break;
   case IR_FLOAT:
      printf("FLOAT %f %f %f %f\n",
             n->Value[0], n->Value[1], n->Value[2], n->Value[3]);
      break;
   case IR_I_TO_F:
      printf("INT_TO_FLOAT %d\n", (int) n->Value[0]);
      break;
   default:
      printf("%s (%p, %p)\n", slang_ir_name(n->Opcode),
             (void*) n->Children[0], (void*) n->Children[1]);
      slang_print_ir(n->Children[0], indent+3);
      slang_print_ir(n->Children[1], indent+3);
   }
}


GLint
_slang_alloc_temporary(slang_gen_context *gc, GLint size)
{
   const GLuint sz4 = (size + 3) / 4;
   GLuint i, j;
   ASSERT(size > 0); /* number of floats */
   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      GLuint found = 0;
      for (j = 0; j < sz4; j++) {
         if (!gc->TempUsed[i + j]) {
            found++;
         }
      }
      if (found == sz4) {
         /* found block of size/4 free regs */
         for (j = 0; j < sz4; j++)
            gc->TempUsed[i + j] = GL_TRUE;
         return i;
      }
   }
   return -1;
}



static GLboolean
is_temporary(const slang_gen_context *gc, const slang_ir_storage *st)
{
   if (st->File == PROGRAM_TEMPORARY && gc->TempUsed[st->Index])
      return gc->TempUsed[st->Index];
   else
      return GL_FALSE;
}


static void
free_temporary(slang_gen_context *gc, GLuint r, GLint size)
{
   const GLuint sz4 = (size + 3) / 4;
   GLuint i;
   for (i = 0; i < sz4; i++) {
      if (gc->TempUsed[r + i])
         gc->TempUsed[r + i] = GL_FALSE;
   }
}


/**
 * Allocate temporary storage for an intermediate result (such as for
 * a multiply or add, etc.
 */
static void
slang_alloc_temp_storage(slang_gen_context *gc, slang_ir_node *n, GLint size)
{
   GLint indx;
   assert(!n->Var);
   assert(!n->Store);
   assert(size > 0);
   indx = _slang_alloc_temporary(gc, size);
   n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, indx, size);
}


static slang_ir_storage *
alloc_constant(const GLfloat v[], GLuint size, struct gl_program *prog)
{
   GLuint swizzle;
   GLint ind = _mesa_add_unnamed_constant(prog->Parameters, v, size, &swizzle);
   slang_ir_storage *st = _slang_new_ir_storage(PROGRAM_CONSTANT, ind, size);
   return st;
}


/**
 * Swizzle a swizzle.
 */
#if 0
static GLuint
swizzle_compose(GLuint swz1, GLuint swz2)
{
   GLuint i, swz, s[4];
   for (i = 0; i < 4; i++) {
      GLuint c = GET_SWZ(swz1, i);
      s[i] = GET_SWZ(swz2, c);
   }
   swz = MAKE_SWIZZLE4(s[0], s[1], s[2], s[3]);
   return swz;
}
#endif


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
   dst->File = st->File;
   dst->Index = st->Index;
   assert(st->File != PROGRAM_UNDEFINED);
   assert(st->Size >= 1);
   assert(st->Size <= 4);
   dst->WriteMask = defaultWritemask[st->Size - 1] & writemask;
}


/**
 * Convert IR storage to an instruction src register.
 */
static void
storage_to_src_reg(struct prog_src_register *src, const slang_ir_storage *st,
                   GLuint swizzle)
{
   static const GLuint defaultSwizzle[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W)
   };
     
   src->File = st->File;
   src->Index = st->Index;
   assert(st->File != PROGRAM_UNDEFINED);
   assert(st->Size >= 1);
   assert(st->Size <= 4);
   /* XXX swizzling logic here may need some work */
   /*src->Swizzle = swizzle_compose(swizzle, defaultSwizzle[st->Size - 1]);*/
   if (swizzle != SWIZZLE_NOOP)
      src->Swizzle = swizzle;
   else
      src->Swizzle = defaultSwizzle[st->Size - 1];
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


static struct prog_instruction *
emit(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog);


/**
 * Generate code for a simple binary-op instruction.
 */
static struct prog_instruction *
emit_binop(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   const slang_ir_info *info = slang_find_ir_info(n->Opcode);
   assert(info);

   assert(info->InstOpcode != OPCODE_NOP);

   emit(gc, n->Children[0], prog);
   emit(gc, n->Children[1], prog);
   inst = new_instruction(prog, info->InstOpcode);
   /* alloc temp storage for the result: */
   if (!n->Store || n->Store->File == PROGRAM_UNDEFINED) {
      slang_alloc_temp_storage(gc, n, info->ResultSize);
   }
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store,
                      n->Children[0]->Swizzle);
   storage_to_src_reg(&inst->SrcReg[1], n->Children[1]->Store,
                      n->Children[1]->Swizzle);
   inst->Comment = n->Comment;
   return inst;
}


static struct prog_instruction *
emit_unop(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   const slang_ir_info *info = slang_find_ir_info(n->Opcode);
   assert(info);

   assert(info->NumParams == 1);

   emit(gc, n->Children[0], prog);

   inst = new_instruction(prog, info->InstOpcode);

   if (!n->Store)
      slang_alloc_temp_storage(gc, n, info->ResultSize);

   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store,
                      n->Children[0]->Swizzle);

   inst->Comment = n->Comment;

   return inst;
}


static struct prog_instruction *
emit_negation(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog)
{
   /* Implement as MOV dst, -src; */
   /* XXX we could look at the previous instruction and in some circumstances
    * modify it to accomplish the negation.
    */
   struct prog_instruction *inst;

   emit(gc, n->Children[0], prog);

   if (!n->Store)
      slang_alloc_temp_storage(gc, n, n->Children[0]->Store->Size);

   inst = new_instruction(prog, OPCODE_MOV);
   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store,
                      n->Children[0]->Swizzle);
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
emit_cjump(const char *target, struct gl_program *prog)
{
   struct prog_instruction *inst;
   inst = new_instruction(prog, OPCODE_BRA);
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
emit_tex(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog)
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
      slang_alloc_temp_storage(gc, n, 4);

   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   /* Child[1] is the coord */
   storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store,
                      n->Children[1]->Swizzle);

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
emit(slang_gen_context *gc, slang_ir_node *n, struct gl_program *prog)
{
   struct prog_instruction *inst;
   if (!n)
      return NULL;

   switch (n->Opcode) {
   case IR_SEQ:
      assert(n->Children[0]);
      assert(n->Children[1]);
      emit(gc, n->Children[0], prog);
      inst = emit(gc, n->Children[1], prog);
      n->Store = n->Children[1]->Store;
      return inst;
      break;
   case IR_VAR_DECL:
   case IR_VAR:
      /* Storage should have already been resolved/allocated */
      assert(n->Store);
      assert(n->Store->File != PROGRAM_UNDEFINED);
      assert(n->Store->Index >= 0);
      assert(n->Store->Size > 0);
      break;
   case IR_MOVE:
      /* rhs */
      assert(n->Children[1]);
      inst = emit(gc, n->Children[1], prog);
      /* lhs */
      emit(gc, n->Children[0], prog);

#if 1
      if (inst && is_temporary(gc, n->Children[1]->Store)) {
         /* Peephole optimization:
          * Just modify the RHS to put its result into the dest of this
          * MOVE operation.  Then, this MOVE is a no-op.
          */
         free_temporary(gc, n->Children[1]->Store->Index,
                        n->Children[1]->Store->Size);
         *n->Children[1]->Store = *n->Children[0]->Store;
         /* fixup the prev (RHS) instruction */
         storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
         return inst;
      }
      else
#endif
      {
         if (n->Children[0]->Store->Size > 4) {
            /* move matrix/struct etc */
            slang_ir_storage dstStore = *n->Children[0]->Store;
            slang_ir_storage srcStore = *n->Children[1]->Store;
            GLint size = srcStore.Size;
            ASSERT(n->Children[0]->Writemask == WRITEMASK_XYZW);
            ASSERT(n->Children[1]->Swizzle == SWIZZLE_NOOP);
            dstStore.Size = 4;
            srcStore.Size = 4;
            while (size >= 4) {
               inst = new_instruction(prog, OPCODE_MOV);
               inst->Comment = _mesa_strdup("IR_MOVE block");
               storage_to_dst_reg(&inst->DstReg, &dstStore, n->Writemask);
               storage_to_src_reg(&inst->SrcReg[0], &srcStore,
                                  n->Children[1]->Swizzle);
               srcStore.Index++;
               dstStore.Index++;
               size -= 4;
            }
         }
         else {
            inst = new_instruction(prog, OPCODE_MOV);
            storage_to_dst_reg(&inst->DstReg, n->Children[0]->Store, n->Writemask);
            storage_to_src_reg(&inst->SrcReg[0], n->Children[1]->Store,
                               n->Children[1]->Swizzle);
         }
         /* XXX is this test correct? */
         if (n->Children[1]->Store->File == PROGRAM_TEMPORARY) {
            free_temporary(gc, n->Children[1]->Store->Index,
                           n->Children[1]->Store->Size);
         }
         /*inst->Comment = _mesa_strdup("IR_MOVE");*/
         n->Store = n->Children[0]->Store; /*XXX new */
         return inst;
      }
      break;
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
      return emit_binop(gc, n, prog);
   case IR_RSQ:
   case IR_RCP:
   case IR_FLOOR:
   case IR_FRAC:
   case IR_ABS:
   case IR_SIN:
   case IR_COS:
   case IR_DDX:
   case IR_DDY:
      return emit_unop(gc, n, prog);
   case IR_TEX:
   case IR_TEXB:
   case IR_TEXP:
      return emit_tex(gc, n, prog);
   case IR_NEG:
      return emit_negation(gc, n, prog);
   case IR_LABEL:
      return emit_label(n->Target, prog);
   case IR_FLOAT:
      n->Store = alloc_constant(n->Value, 4, prog); /*XXX fix size */
      break;
   case IR_COND:
      {
         /* Conditional expression (in if/while/for stmts).
          * Need to update condition code register.
          * Next instruction is typically an IR_CJUMP.
          */
         /* last child expr instruction: */
         struct prog_instruction *inst = emit(gc, n->Children[0], prog);
         if (inst) {
            /* set inst's CondUpdate flag */
            inst->CondUpdate = GL_TRUE;
            return inst; /* XXX or null? */
         }
         else {
            /* This'll happen for things like "if (i) ..." where no code
             * is normally generated for the expression "i".
             * Generate a move instruction just to set condition codes.
             */
            slang_alloc_temp_storage(gc, n, 1);
            inst = new_instruction(prog, OPCODE_MOV);
            inst->CondUpdate = GL_TRUE;
            storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);
            storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store,
                               n->Children[0]->Swizzle);
            free_temporary(gc, n->Store->Index, n->Store->Size);
            return inst; /* XXX or null? */
         }
      }
      return NULL;
   case IR_JUMP:
      return emit_jump(n->Target, prog);
   case IR_CJUMP:
      return emit_cjump(n->Target, prog);
   default:
      printf("emit: ?\n");
      abort();
   }
   return NULL;
}


slang_gen_context *
_slang_new_codegen_context(void)
{
   slang_gen_context *gc = (slang_gen_context *) _mesa_calloc(sizeof(*gc));
   return gc;
}



GLboolean
_slang_emit_code(slang_ir_node *n, slang_gen_context *gc,
                 struct gl_program *prog)
{
   /*GET_CURRENT_CONTEXT(ctx);*/

   /*
   gc = _slang_new_codegen_context();
   */

   printf("************ Begin generate code\n");

   (void) emit(gc, n, prog);

   {
      struct prog_instruction *inst;
      inst = new_instruction(prog, OPCODE_END);
   }

   printf("************ End generate code (%u inst):\n", prog->NumInstructions);

#if 0
   _mesa_print_program(prog);
   _mesa_print_program_parameters(ctx,prog);
#endif

   _mesa_free(gc);

   return GL_FALSE;
}
