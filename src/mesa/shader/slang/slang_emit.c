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
#include "get.h"
#include "macros.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
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
   sprintf(s, "%s[%d]", files[st->File], st->Index);
   return s;
}


static GLuint
sizeof_struct(const slang_struct *s)
{
   return 0;
}


GLuint
_slang_sizeof_type_specifier(const slang_type_specifier *spec)
{
   switch (spec->type) {
   case slang_spec_void:
      abort();
      return 0;
   case slang_spec_bool:
      return 1;
   case slang_spec_bvec2:
      return 2;
   case slang_spec_bvec3:
      return 3;
   case slang_spec_bvec4:
      return 4;
   case slang_spec_int:
      return 1;
   case slang_spec_ivec2:
      return 2;
   case slang_spec_ivec3:
      return 3;
   case slang_spec_ivec4:
      return 4;
   case slang_spec_float:
      return 1;
   case slang_spec_vec2:
      return 2;
   case slang_spec_vec3:
      return 3;
   case slang_spec_vec4:
      return 4;
   case slang_spec_mat2:
      return 2 * 2;
   case slang_spec_mat3:
      return 3 * 3;
   case slang_spec_mat4:
      return 4 * 4;
   case slang_spec_sampler1D:
   case slang_spec_sampler2D:
   case slang_spec_sampler3D:
   case slang_spec_samplerCube:
   case slang_spec_sampler1DShadow:
   case slang_spec_sampler2DShadow:
      abort();
      return 0;
   case slang_spec_struct:
      return sizeof_struct(spec->_struct);
   case slang_spec_array:
      return 1; /* XXX */
   default:
      abort();
      return 0;
   }
   return 0;
}



static GLuint
sizeof_type(const slang_fully_specified_type *t)
{
   return _slang_sizeof_type_specifier(&t->specifier);
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


static GLint
alloc_temporary(slang_gen_context *gc, GLint size)
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



static GLint
slang_find_input(GLenum target, const char *name, GLint index)
{
   struct input_info {
      const char *Name;
      GLuint Attrib;
   };
   static const struct input_info vertInputs[] = {
      { "gl_Vertex", VERT_ATTRIB_POS },
      { "gl_Normal", VERT_ATTRIB_NORMAL },
      { "gl_Color", VERT_ATTRIB_COLOR0 },
      { "gl_SecondaryColor", VERT_ATTRIB_COLOR1 },
      { NULL, 0 }
   };
   static const struct input_info fragInputs[] = {
      { NULL, 0 }
   };
   const struct input_info *inputs;
   GLuint i;

   if (target == GL_VERTEX_PROGRAM_ARB) {
      inputs = vertInputs;
   }
   else {
      assert(target == GL_FRAGMENT_PROGRAM_ARB);
      inputs = fragInputs;
   }

   for (i = 0; inputs[i].Name; i++) {
      if (strcmp(inputs[i].Name, name) == 0) {
         /* found */
         return inputs[i].Attrib;
      }
   }
   return -1;
}


static GLint
slang_find_output(GLenum target, const char *name, GLint index)
{
   struct output_info {
      const char *Name;
      GLuint Attrib;
   };
   static const struct output_info vertOutputs[] = {
      { "gl_Position", VERT_RESULT_HPOS },
      { "gl_FrontColor", VERT_RESULT_COL0 },
      { "gl_BackColor", VERT_RESULT_BFC0 },
      { "gl_FrontSecondaryColor", VERT_RESULT_COL1 },
      { "gl_BackSecondaryColor", VERT_RESULT_BFC1 },
      { "gl_TexCoord", VERT_RESULT_TEX0 }, /* XXX indexed */
      { "gl_FogFragCoord", VERT_RESULT_FOGC },
      { NULL, 0 }
   };
   static const struct output_info fragOutputs[] = {
      { "gl_FragColor", FRAG_RESULT_COLR },
      { NULL, 0 }
   };
   const struct output_info *outputs;
   GLuint i;

   if (target == GL_VERTEX_PROGRAM_ARB) {
      outputs = vertOutputs;
   }
   else {
      assert(target == GL_FRAGMENT_PROGRAM_ARB);
      outputs = fragOutputs;
   }

   for (i = 0; outputs[i].Name; i++) {
      if (strcmp(outputs[i].Name, name) == 0) {
         /* found */
         return outputs[i].Attrib;
      }
   }
   return -1;
}


/**
 * Lookup a named constant and allocate storage for the parameter in
 * the given parameter list.
 * \return position of the constant in the paramList.
 */
static GLint
slang_lookup_constant(const char *name, GLint index,
                      struct gl_program_parameter_list *paramList)
{
   struct constant_info {
      const char *Name;
      const GLenum Token;
   };
   static const struct constant_info info[] = {
      { "gl_MaxLights", GL_MAX_LIGHTS },
      { "gl_MaxClipPlanes", GL_MAX_CLIP_PLANES },
      { "gl_MaxTextureUnits", GL_MAX_TEXTURE_UNITS },
      { "gl_MaxTextureCoords", GL_MAX_TEXTURE_COORDS },
      { "gl_MaxVertexAttribs", GL_MAX_VERTEX_ATTRIBS },
      { "gl_MaxVertexUniformComponents", GL_MAX_VERTEX_UNIFORM_COMPONENTS },
      { "gl_MaxVaryingFloats", GL_MAX_VARYING_FLOATS },
      { "gl_MaxVertexTextureImageUnits", GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS },
      { "gl_MaxTextureImageUnits", GL_MAX_TEXTURE_IMAGE_UNITS },
      { "gl_MaxFragmentUniformComponents", GL_MAX_FRAGMENT_UNIFORM_COMPONENTS },
      { "gl_MaxCombinedTextureImageUnits", GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS },
      { NULL, 0 }
   };
   GLuint i;
   GLuint swizzle; /* XXX use this */

   for (i = 0; info[i].Name; i++) {
      if (strcmp(info[i].Name, name) == 0) {
         /* found */
         GLfloat value = -1.0;
         GLint pos;
         _mesa_GetFloatv(info[i].Token, &value);
         ASSERT(value >= 0.0);  /* sanity check that glGetFloatv worked */
         pos = _mesa_add_unnamed_constant(paramList, &value, 1, &swizzle);
         return pos;
      }
   }
   return -1;
}


/**
 * Determine if 'name' is a state variable.  If so, create a new program
 * parameter for it, and return the param's index.  Else, return -1.
 */
static GLint
slang_lookup_statevar(const char *name, GLint index,
                      struct gl_program_parameter_list *paramList)
{
   struct state_info {
      const char *Name;
      const GLuint NumRows;  /** for matrices */
      const GLuint Swizzle;
      const GLint Indexes[6];
   };
   static const struct state_info state[] = {
      { "gl_ModelViewMatrix", 4, SWIZZLE_NOOP,
        { STATE_MATRIX, STATE_MODELVIEW, 0, 0, 0, 0 } },
      { "gl_NormalMatrix", 3, SWIZZLE_NOOP,
        { STATE_MATRIX, STATE_MODELVIEW, 0, 0, 0, 0 } },
      { "gl_ProjectionMatrix", 4, SWIZZLE_NOOP,
        { STATE_MATRIX, STATE_PROJECTION, 0, 0, 0, 0 } },
      { "gl_ModelViewProjectionMatrix", 4, SWIZZLE_NOOP,
        { STATE_MATRIX, STATE_MVP, 0, 0, 0, 0 } },
      { "gl_TextureMatrix", 4, SWIZZLE_NOOP,
        { STATE_MATRIX, STATE_TEXTURE, 0, 0, 0, 0 } },
      { NULL, 0, 0, {0, 0, 0, 0, 0, 0} }
   };
   GLuint i;

   for (i = 0; state[i].Name; i++) {
      if (strcmp(state[i].Name, name) == 0) {
         /* found */
         if (paramList) {
            if (state[i].NumRows > 1) {
               /* a matrix */
               GLuint j;
               GLint pos[4], indexesCopy[6];
               /* make copy of state tokens */
               for (j = 0; j < 6; j++)
                  indexesCopy[j] = state[i].Indexes[j];
               /* load rows */
               for (j = 0; j < state[i].NumRows; j++) {
                  indexesCopy[3] = indexesCopy[4] = j; /* jth row of matrix */
                  pos[j] = _mesa_add_state_reference(paramList, indexesCopy);
                  assert(pos[j] >= 0);
               }
               return pos[0];
            }
            else {
               /* non-matrix state */
               GLint pos
                  = _mesa_add_state_reference(paramList, state[i].Indexes);
               assert(pos >= 0);
               return pos;
            }
         }
      }
   }
   return -1;
}


static GLint
slang_alloc_uniform(struct gl_program *prog, const char *name, GLuint size)
{
   GLint i = _mesa_add_uniform(prog->Parameters, name, size);
   return i;
}


static GLint
slang_alloc_varying(struct gl_program *prog, const char *name)
{
   GLint i = _mesa_add_varying(prog->Varying, name, 4); /* XXX fix size */
#if 0
   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
#ifdef OLD_LINK
      i += VERT_RESULT_VAR0;
      prog->OutputsWritten |= (1 << i);
#else
      prog->OutputsWritten |= (1 << (i + VERT_RESULT_VAR0));
#endif
   }
   else {
#ifdef OLD_LINK
      i += FRAG_ATTRIB_VAR0;
      prog->InputsRead |= (1 << i);
#else
      prog->InputsRead |= (1 << (i + FRAG_ATTRIB_VAR0));
#endif
   }
#endif
   return i;
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
   indx = alloc_temporary(gc, size);
   n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, indx, size);
}


/**
 * Allocate storage info for an IR node (n->Store).
 * We may do any of the following:
 *   1. Compute Store->File/Index for program inputs/outputs/uniforms/etc.
 *   2. Allocate storage for user-declared variables.
 *   3. Allocate intermediate/unnamed storage for complex expressions.
 *   4. other?
 *
 * If gc or prog is NULL, we may only be able to determine the Store->File
 * but not an Index (register).
 */
void
slang_resolve_storage(slang_gen_context *gc, slang_ir_node *n,
                      struct gl_program *prog)
{
   assert(gc);
   assert(n);
   assert(prog);

   if (!n->Store) {
      /* allocate storage info for this node */
      if (n->Var && n->Var->aux) {
         /* node storage info = var storage info */
         n->Store = (slang_ir_storage *) n->Var->aux;
      }
      else {
         /* alloc new storage info */
         n->Store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -1, -5);
         if (n->Var)
            n->Var->aux = n->Store;
      }
   }

   if (n->Opcode == IR_VAR_DECL) {
      /* storage declaration */
      assert(n->Var);
      if (n->Store->Index < 0) { /* XXX assert this? */
         assert(gc);
         n->Store->File = PROGRAM_TEMPORARY;
         n->Store->Size = sizeof_type(&n->Var->type);
         n->Store->Index = alloc_temporary(gc, n->Store->Size);
         printf("alloc var %s storage at %d (size %d)\n",
                (char *) n->Var->a_name,
                n->Store->Index,
                n->Store->Size);
         assert(n->Store->Size > 0);
         n->Var->declared = GL_TRUE;
      }
      assert(n->Store->Size > 0);
      return;
   }

   if (n->Opcode == IR_VAR && n->Store->File == PROGRAM_UNDEFINED) {
      /* try to determine the storage for this variable */
      GLint i;

      assert(n->Var);

      if (n->Store->Size < 0) {
         /* determine var/storage size now */
         n->Store->Size = sizeof_type(&n->Var->type);
         assert(n->Store->Size > 0);
      }

#if 0
      assert(n->Var->declared ||
             n->Var->type.qualifier == slang_qual_uniform ||
             n->Var->type.qualifier == slang_qual_varying ||
             n->Var->type.qualifier == slang_qual_fixedoutput ||
             n->Var->type.qualifier == slang_qual_attribute ||
             n->Var->type.qualifier == slang_qual_out ||
             n->Var->type.qualifier == slang_qual_const);
#endif

      i = slang_find_input(prog->Target, (char *) n->Var->a_name, 0);
      if (i >= 0) {
         n->Store->File = PROGRAM_INPUT;
         n->Store->Index = i;
         assert(n->Store->Size > 0);
         prog->InputsRead |= (1 << i);
         return;
      }

      i = slang_find_output(prog->Target, (char *) n->Var->a_name, 0);
      if (i >= 0) {
         n->Store->File = PROGRAM_OUTPUT;
         n->Store->Index = i;
         prog->OutputsWritten |= (1 << i);
         return;
      }

      i = slang_lookup_statevar((char *) n->Var->a_name, 0, prog->Parameters);
      if (i >= 0) {
         n->Store->File = PROGRAM_STATE_VAR;
         n->Store->Index = i;
         return;
      }

      i = slang_lookup_constant((char *) n->Var->a_name, 0, prog->Parameters);
      if (i >= 0) {
         n->Store->File = PROGRAM_CONSTANT;
         n->Store->Index = i;
         return;
      }

      /* probably a uniform or varying */
      if (n->Var->type.qualifier == slang_qual_uniform) {
         GLint size = n->Store->Size;
         assert(size > 0);
         i = slang_alloc_uniform(prog, (char *) n->Var->a_name, size);
         if (i >= 0) {
            n->Store->File = PROGRAM_UNIFORM;
            n->Store->Index = i;
            return;
         }
      }
      else if (n->Var->type.qualifier == slang_qual_varying) {
         i = slang_alloc_varying(prog, (char *) n->Var->a_name);
         if (i >= 0) {
#ifdef OLD_LINK
            if (prog->Target == GL_VERTEX_PROGRAM_ARB)
               n->Store->File = PROGRAM_OUTPUT;
            else
               n->Store->File = PROGRAM_INPUT;
#else
            n->Store->File = PROGRAM_VARYING;
#endif
            n->Store->Index = i;
            return;
         }
      }

      if (n->Store->File == PROGRAM_UNDEFINED && n->Store->Index < 0) {
         /* ordinary local var */
         assert(n->Store->Size > 0);
         n->Store->File = PROGRAM_TEMPORARY;
         n->Store->Index = alloc_temporary(gc, n->Store->Size);
      }
   }
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
#if 1
      slang_alloc_temp_storage(gc, n, info->ResultSize);
#else
      slang_resolve_storage(gc, n, prog);
#endif
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
   /*slang_resolve_storage(gc, n, prog);*/

   if (!n->Store)
      slang_alloc_temp_storage(gc, n, info->ResultSize);

   storage_to_dst_reg(&inst->DstReg, n->Store, n->Writemask);

   storage_to_src_reg(&inst->SrcReg[0], n->Children[0]->Store,
                      n->Children[0]->Swizzle);

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
      slang_resolve_storage(gc, n, prog);
      assert(n->Store->Index >= 0);
      assert(n->Store->Size > 0);
      break;
   case IR_VAR:
      /*printf("Gen: var ref\n");*/
      {
         int b = !n->Store || n->Store->Index < 0;
         if (b)
            slang_resolve_storage(gc, n, prog);
         /*assert(n->Store->Index >= 0);*/
         assert(n->Store->Size > 0);
      }
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
      return emit_unop(gc, n, prog);
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
