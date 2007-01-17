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
 * \file slang_codegen.c
 * Mesa GLSL code generator.  Convert AST to IR tree.
 * \author Brian Paul
 */

#include "imports.h"
#include "get.h"
#include "macros.h"
#include "slang_assemble.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_storage.h"
#include "slang_error.h"
#include "slang_simplify.h"
#include "slang_emit.h"
#include "slang_vartable.h"
#include "slang_ir.h"
#include "mtypes.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
#include "slang_print.h"


static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper);




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
         /* XXX named constant! */
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
      const GLint Indexes[STATE_LENGTH];
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
               GLint pos[4], indexesCopy[STATE_LENGTH];
               /* make copy of state tokens */
               for (j = 0; j < STATE_LENGTH; j++)
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


static GLboolean
is_sampler_type(const slang_fully_specified_type *t)
{
   switch (t->specifier.type) {
   case slang_spec_sampler1D:
   case slang_spec_sampler2D:
   case slang_spec_sampler3D:
   case slang_spec_samplerCube:
   case slang_spec_sampler1DShadow:
   case slang_spec_sampler2DShadow:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static GLuint
_slang_sizeof_struct(const slang_struct *s)
{
   /* XXX TBD */
   return 0;
}


static GLuint
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
      return 1; /* special case */
   case slang_spec_struct:
      return _slang_sizeof_struct(spec->_struct);
   case slang_spec_array:
      return 1; /* XXX */
   default:
      abort();
      return 0;
   }
   return 0;
}


/**
 * Allocate storage info for an IR node (n->Store).
 * If n is an IR_VAR_DECL, allocate a temporary for the variable.
 * Otherwise, if n is an IR_VAR, check if it's a uniform or constant
 * that needs to have storage allocated.
 */
static void
slang_allocate_storage(slang_assemble_ctx *A, slang_ir_node *n)
{
   assert(A->vartable);
   assert(n);

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
         assert(n->Var->aux);
      }
   }

   if (n->Opcode == IR_VAR_DECL) {
      /* variable declaration */
      assert(n->Var);
      assert(!is_sampler_type(&n->Var->type));
      n->Store->File = PROGRAM_TEMPORARY;
      n->Store->Size = _slang_sizeof_type_specifier(&n->Var->type.specifier);
      assert(n->Store->Size > 0);
      return;
   }
   else {
      assert(n->Opcode == IR_VAR);
      assert(n->Var);

      if (n->Store->Index < 0) {
         const char *varName = (char *) n->Var->a_name;
         struct gl_program *prog = A->program;
         assert(prog);

         /* determine storage location for this var.
          * This is probably a pre-defined uniform or constant.
          * We don't allocate storage for these until they're actually
          * used to avoid wasting registers.
          */
         if (n->Store->File == PROGRAM_STATE_VAR) {
            GLint i = slang_lookup_statevar(varName, 0, prog->Parameters);
            assert(i >= 0);
            n->Store->Index = i;
         }
         else if (n->Store->File == PROGRAM_CONSTANT) {
            GLint i = slang_lookup_constant(varName, 0, prog->Parameters);
            assert(i >= 0);
            n->Store->Index = i;
         }
      }
   }
}


/**
 * Return the TEXTURE_*_INDEX value that corresponds to a sampler type,
 * or -1 if the type is not a sampler.
 */
static GLint
sampler_to_texture_index(const slang_type_specifier_type type)
{
   switch (type) {
   case slang_spec_sampler1D:
      return TEXTURE_1D_INDEX;
   case slang_spec_sampler2D:
      return TEXTURE_2D_INDEX;
   case slang_spec_sampler3D:
      return TEXTURE_3D_INDEX;
   case slang_spec_samplerCube:
      return TEXTURE_CUBE_INDEX;
   case slang_spec_sampler1DShadow:
      return TEXTURE_1D_INDEX; /* XXX fix */
   case slang_spec_sampler2DShadow:
      return TEXTURE_2D_INDEX; /* XXX fix */
   default:
      return -1;
   }
}


/**
 * Return the VERT_ATTRIB_* or FRAG_ATTRIB_* value that corresponds to
 * a vertex or fragment program input variable.  Return -1 if the input
 * name is invalid.
 * XXX return size too
 */
static GLint
_slang_input_index(const char *name, GLenum target)
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
      { "gl_FogCoord", VERT_ATTRIB_FOG },
      { "gl_MultiTexCoord0", VERT_ATTRIB_TEX0 },
      { "gl_MultiTexCoord1", VERT_ATTRIB_TEX1 },
      { "gl_MultiTexCoord2", VERT_ATTRIB_TEX2 },
      { "gl_MultiTexCoord3", VERT_ATTRIB_TEX3 },
      { "gl_MultiTexCoord4", VERT_ATTRIB_TEX4 },
      { "gl_MultiTexCoord5", VERT_ATTRIB_TEX5 },
      { "gl_MultiTexCoord6", VERT_ATTRIB_TEX6 },
      { "gl_MultiTexCoord7", VERT_ATTRIB_TEX7 },
      { NULL, 0 }
   };
   static const struct input_info fragInputs[] = {
      { "gl_FragCoord", FRAG_ATTRIB_WPOS },
      { "gl_Color", FRAG_ATTRIB_COL0 },
      { "gl_SecondaryColor", FRAG_ATTRIB_COL1 },
      { "gl_FogFragCoord", FRAG_ATTRIB_FOGC },
      { "gl_TexCoord", FRAG_ATTRIB_TEX0 },
      { NULL, 0 }
   };
   GLuint i;
   const struct input_info *inputs
      = (target == GL_VERTEX_PROGRAM_ARB) ? vertInputs : fragInputs;

   ASSERT(MAX_TEXTURE_UNITS == 8); /* if this fails, fix vertInputs above */

   for (i = 0; inputs[i].Name; i++) {
      if (strcmp(inputs[i].Name, name) == 0) {
         /* found */
         return inputs[i].Attrib;
      }
   }
   return -1;
}


/**
 * Return the VERT_RESULT_* or FRAG_RESULT_* value that corresponds to
 * a vertex or fragment program output variable.  Return -1 for an invalid
 * output name.
 */
static GLint
_slang_output_index(const char *name, GLenum target)
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
      { "gl_PointSize", VERT_RESULT_PSIZ },
      { NULL, 0 }
   };
   static const struct output_info fragOutputs[] = {
      { "gl_FragColor", FRAG_RESULT_COLR },
      { "gl_FragDepth", FRAG_RESULT_DEPR },
      { NULL, 0 }
   };
   GLuint i;
   const struct output_info *outputs
      = (target == GL_VERTEX_PROGRAM_ARB) ? vertOutputs : fragOutputs;

   for (i = 0; outputs[i].Name; i++) {
      if (strcmp(outputs[i].Name, name) == 0) {
         /* found */
         return outputs[i].Attrib;
      }
   }
   return -1;
}



/**********************************************************************/


/**
 * Map "_asm foo" to IR_FOO, etc.
 */
typedef struct
{
   const char *Name;
   slang_ir_opcode Opcode;
   GLuint HaveRetValue, NumParams;
} slang_asm_info;


static slang_asm_info AsmInfo[] = {
   /* vec4 binary op */
   { "vec4_add", IR_ADD, 1, 2 },
   { "vec4_subtract", IR_SUB, 1, 2 },
   { "vec4_multiply", IR_MUL, 1, 2 },
   { "vec4_dot", IR_DOT4, 1, 2 },
   { "vec3_dot", IR_DOT3, 1, 2 },
   { "vec3_cross", IR_CROSS, 1, 2 },
   { "vec4_min", IR_MIN, 1, 2 },
   { "vec4_max", IR_MAX, 1, 2 },
   { "vec4_seq", IR_SEQ, 1, 2 },
   { "vec4_sge", IR_SGE, 1, 2 },
   { "vec4_sgt", IR_SGT, 1, 2 },
   /* vec4 unary */
   { "vec4_floor", IR_FLOOR, 1, 1 },
   { "vec4_frac", IR_FRAC, 1, 1 },
   { "vec4_abs", IR_ABS, 1, 1 },
   { "vec4_negate", IR_NEG, 1, 1 },
   { "vec4_ddx", IR_DDX, 1, 1 },
   { "vec4_ddy", IR_DDY, 1, 1 },
   /* float binary op */
   { "float_add", IR_ADD, 1, 2 },
   { "float_multiply", IR_MUL, 1, 2 },
   { "float_divide", IR_DIV, 1, 2 },
   { "float_power", IR_POW, 1, 2 },
   /* texture / sampler */
   { "vec4_tex1d", IR_TEX, 1, 2 },
   { "vec4_texb1d", IR_TEXB, 1, 2 },  /* 1d w/ bias */
   { "vec4_texp1d", IR_TEXP, 1, 2 },  /* 1d w/ projection */
   { "vec4_tex2d", IR_TEX, 1, 2 },
   { "vec4_texb2d", IR_TEXB, 1, 2 },  /* 2d w/ bias */
   { "vec4_texp2d", IR_TEXP, 1, 2 },  /* 2d w/ projection */
   { "vec4_tex3d", IR_TEX, 1, 2 },
   { "vec4_texb3d", IR_TEXB, 1, 2 },  /* 3d w/ bias */
   { "vec4_texp3d", IR_TEXP, 1, 2 },  /* 3d w/ projection */

   /* unary op */
   { "int_to_float", IR_I_TO_F, 1, 1 },
   { "float_exp", IR_EXP, 1, 1 },
   { "float_exp2", IR_EXP2, 1, 1 },
   { "float_log2", IR_LOG2, 1, 1 },
   { "float_rsq", IR_RSQ, 1, 1 },
   { "float_rcp", IR_RCP, 1, 1 },
   { "float_sine", IR_SIN, 1, 1 },
   { "float_cosine", IR_COS, 1, 1 },
   { NULL, IR_NOP, 0, 0 }
};


/**
 * Recursively free an IR tree.
 */
static void
_slang_free_ir_tree(slang_ir_node *n)
{
#if 0
   if (!n)
      return;
   _slang_free_ir_tree(n->Children[0]);
   _slang_free_ir_tree(n->Children[1]);
   free(n);
#endif
}


static slang_ir_node *
new_node(slang_ir_opcode op, slang_ir_node *left, slang_ir_node *right)
{
   slang_ir_node *n = (slang_ir_node *) calloc(1, sizeof(slang_ir_node));
   if (n) {
      n->Opcode = op;
      n->Children[0] = left;
      n->Children[1] = right;
      n->Swizzle = SWIZZLE_NOOP;
      n->Writemask = WRITEMASK_XYZW;
   }
   return n;
}

static slang_ir_node *
new_seq(slang_ir_node *left, slang_ir_node *right)
{
   /* XXX if either left or right is null, just return pointer to other?? */
   assert(left);
   assert(right);
   return new_node(IR_SEQ, left, right);
}

static slang_ir_node *
new_label(slang_atom labName)
{
   slang_ir_node *n = new_node(IR_LABEL, NULL, NULL);
   n->Target = (char *) labName; /*_mesa_strdup(name);*/
   return n;
}

static slang_ir_node *
new_float_literal(float x, float y, float z, float w)
{
   slang_ir_node *n = new_node(IR_FLOAT, NULL, NULL);
   n->Value[0] = x;
   n->Value[1] = y;
   n->Value[2] = z;
   n->Value[3] = w;
   return n;
}

/**
 * XXX maybe pass an IR node as second param to indicate the jump target???
 */
static slang_ir_node *
new_cjump(slang_atom target)
{
   slang_ir_node *n = new_node(IR_CJUMP, NULL, NULL);
   if (n)
      n->Target = (char *) target;
   return n;
}

/**
 * XXX maybe pass an IR node as second param to indicate the jump target???
 */
static slang_ir_node *
new_jump(slang_atom target)
{
   slang_ir_node *n = new_node(IR_JUMP, NULL, NULL);
   if (n)
      n->Target = (char *) target;
   return n;
}


/**
 * New IR_VAR node - a reference to a previously declared variable.
 */
static slang_ir_node *
new_var(slang_assemble_ctx *A, slang_operation *oper,
        slang_atom name, GLuint swizzle)
{
   slang_variable *v = _slang_locate_variable(oper->locals, name, GL_TRUE);
   slang_ir_node *n = new_node(IR_VAR, NULL, NULL);
   if (!v) {
      printf("VAR NOT FOUND %s\n", (char *) name);
      assert(v);
   }
   assert(!oper->var || oper->var == v);
   v->used = GL_TRUE;

   n->Swizzle = swizzle;
   n->Var = v;

   slang_allocate_storage(A, n);

   return n;
}


static GLboolean
slang_is_writemask(const char *field, GLuint *mask)
{
   const GLuint n = 4;
   GLuint i, bit, c = 0;

   for (i = 0; i < n && field[i]; i++) {
      switch (field[i]) {
      case 'x':
      case 'r':
         bit = WRITEMASK_X;
         break;
      case 'y':
      case 'g':
         bit = WRITEMASK_Y;
         break;
      case 'z':
      case 'b':
         bit = WRITEMASK_Z;
         break;
      case 'w':
      case 'a':
         bit = WRITEMASK_W;
         break;
      default:
         return GL_FALSE;
      }
      if (c & bit)
         return GL_FALSE;
      c |= bit;
   }
   *mask = c;
   return GL_TRUE;
}


/**
 * Check if the given function is really just a wrapper for a
 * basic assembly instruction.
 */
static GLboolean
slang_is_asm_function(const slang_function *fun)
{
   if (fun->body->type == slang_oper_block_no_new_scope &&
       fun->body->num_children == 1 &&
       fun->body->children[0].type == slang_oper_asm) {
      return GL_TRUE;
   }
   return GL_FALSE;
}


/**
 * Produce inline code for a call to an assembly instruction.
 */
static slang_operation *
slang_inline_asm_function(slang_assemble_ctx *A,
                          slang_function *fun, slang_operation *oper)
{
   const int numArgs = oper->num_children;
   const slang_operation *args = oper->children;
   GLuint i;
   slang_operation *inlined = slang_operation_new(1);

   /*assert(oper->type == slang_oper_call);  or vec4_add, etc */
   printf("Inline asm %s\n", (char*) fun->header.a_name);
   inlined->type = fun->body->children[0].type;
   inlined->a_id = fun->body->children[0].a_id;
   inlined->num_children = numArgs;
   inlined->children = slang_operation_new(numArgs);
#if 0
   inlined->locals = slang_variable_scope_copy(oper->locals);
#else
   assert(inlined->locals);
   inlined->locals->outer_scope = oper->locals->outer_scope;
#endif

   for (i = 0; i < numArgs; i++) {
      slang_operation_copy(inlined->children + i, args + i);
   }

   return inlined;
}


static void
slang_resolve_variable(slang_operation *oper)
{
   if (oper->type != slang_oper_identifier)
      return;
   if (!oper->var) {
      oper->var = _slang_locate_variable(oper->locals,
                                         (const slang_atom) oper->a_id,
                                         GL_TRUE);
      if (oper->var)
         oper->var->used = GL_TRUE;
   }
}


/**
 * Replace particular variables (slang_oper_identifier) with new expressions.
 */
static void
slang_substitute(slang_assemble_ctx *A, slang_operation *oper,
                 GLuint substCount, slang_variable **substOld,
		 slang_operation **substNew, GLboolean isLHS)
{
   switch (oper->type) {
   case slang_oper_variable_decl:
      {
         slang_variable *v = _slang_locate_variable(oper->locals,
                                                    oper->a_id, GL_TRUE);
         assert(v);
         if (v->initializer && oper->num_children == 0) {
            /* set child of oper to copy of initializer */
            oper->num_children = 1;
            oper->children = slang_operation_new(1);
            slang_operation_copy(&oper->children[0], v->initializer);
         }
         if (oper->num_children == 1) {
            /* the initializer */
            slang_substitute(A, &oper->children[0], substCount, substOld, substNew, GL_FALSE);
         }
      }
      break;
   case slang_oper_identifier:
      assert(oper->num_children == 0);
      if (1/**!isLHS XXX FIX */) {
         slang_atom id = oper->a_id;
         slang_variable *v;
	 GLuint i;
         v = _slang_locate_variable(oper->locals, id, GL_TRUE);
	 if (!v) {
	    printf("var %s not found!\n", (char *) oper->a_id);
            _slang_print_var_scope(oper->locals, 6);

            abort();
	    break;
	 }

	 /* look for a substitution */
	 for (i = 0; i < substCount; i++) {
	    if (v == substOld[i]) {
               /* OK, replace this slang_oper_identifier with a new expr */
#if 0 /* DEBUG only */
	       if (substNew[i]->type == slang_oper_identifier) {
                  assert(substNew[i]->var);
                  assert(substNew[i]->var->a_name);
		  printf("Substitute %s with %s in id node %p\n",
			 (char*)v->a_name, (char*) substNew[i]->var->a_name,
			 (void*) oper);
               }
	       else {
		  printf("Substitute %s with %f in id node %p\n",
			 (char*)v->a_name, substNew[i]->literal[0],
			 (void*) oper);
#endif
	       slang_operation_copy(oper, substNew[i]);
	       break;
	    }
	 }
      }
      break;
#if 1 /* XXX rely on default case below */
   case slang_oper_return:
      /* do return replacement here too */
      assert(oper->num_children == 0 || oper->num_children == 1);
      if (oper->num_children == 1) {
         /* replace:
          *   return expr;
          * with:
          *   __retVal = expr;
          *   return;
          * then do substitutions on the assignment.
          */
         slang_operation *blockOper, *assignOper, *returnOper;
         blockOper = slang_operation_new(1);
         blockOper->type = slang_oper_block_no_new_scope;
         blockOper->num_children = 2;
         blockOper->children = slang_operation_new(2);
         assignOper = blockOper->children + 0;
         returnOper = blockOper->children + 1;

         assignOper->type = slang_oper_assign;
         assignOper->num_children = 2;
         assignOper->children = slang_operation_new(2);
         assignOper->children[0].type = slang_oper_identifier;
         assignOper->children[0].a_id = slang_atom_pool_atom(A->atoms, "__retVal");
         assignOper->children[0].locals->outer_scope = oper->locals;
         assignOper->locals = oper->locals;
         slang_operation_copy(&assignOper->children[1],
                              &oper->children[0]);

         returnOper->type = slang_oper_return;
         assert(returnOper->num_children == 0);

         /* do substitutions on the "__retVal = expr" sub-tree */
         slang_substitute(A, assignOper,
                          substCount, substOld, substNew, GL_FALSE);

         /* install new code */
         slang_operation_copy(oper, blockOper);
         slang_operation_destruct(blockOper);
      }
      break;
#endif
   case slang_oper_assign:
   case slang_oper_subscript:
      /* special case:
       * child[0] can't have substitutions but child[1] can.
       */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      slang_substitute(A, &oper->children[1],
                       substCount, substOld, substNew, GL_FALSE);
      break;
   case slang_oper_field:
      /* XXX NEW - test */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      break;
   default:
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) 
            slang_substitute(A, &oper->children[i],
                             substCount, substOld, substNew, GL_FALSE);
      }
   }
}



/**
 * Inline the given function call operation.
 * Return a new slang_operation that corresponds to the inlined code.
 */
static slang_operation *
slang_inline_function_call(slang_assemble_ctx * A, slang_function *fun,
			   slang_operation *oper, slang_operation *returnOper)
{
   typedef enum {
      SUBST = 1,
      COPY_IN,
      COPY_OUT
   } ParamMode;
   ParamMode *paramMode;
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   const GLuint numArgs = oper->num_children;
   const GLuint totalArgs = numArgs + haveRetValue;
   slang_operation *args = oper->children;
   slang_operation *inlined, *top;
   slang_variable **substOld;
   slang_operation **substNew;
   GLuint substCount, numCopyIn, i;

   /*assert(oper->type == slang_oper_call); (or (matrix) multiply, etc) */
   assert(fun->param_count == totalArgs);

   /* allocate temporary arrays */
   paramMode = (ParamMode *)
      _mesa_calloc(totalArgs * sizeof(ParamMode));
   substOld = (slang_variable **)
      _mesa_calloc(totalArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _mesa_calloc(totalArgs * sizeof(slang_operation *));

   printf("Inline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);

   if (haveRetValue && !returnOper) {
      /* Create 3-child comma sequence for inlined code:
       * child[0]:  declare __resultTmp
       * child[1]:  inlined function body
       * child[2]:  __resultTmp
       */
      slang_operation *commaSeq;
      slang_operation *declOper = NULL;
      slang_variable *resultVar;

      commaSeq = slang_operation_new(1);
      commaSeq->type = slang_oper_sequence;
      assert(commaSeq->locals);
      commaSeq->locals->outer_scope = oper->locals->outer_scope;
      commaSeq->num_children = 3;
      commaSeq->children = slang_operation_new(3);
      /* allocate the return var */
      resultVar = slang_variable_scope_grow(commaSeq->locals);
      /*
      printf("Alloc __resultTmp in scope %p for retval of calling %s\n",
             (void*)commaSeq->locals, (char *) fun->header.a_name);
      */

      resultVar->a_name = slang_atom_pool_atom(A->atoms, "__resultTmp");
      resultVar->type = fun->header.type; /* XXX copy? */
      resultVar->isTemp = GL_TRUE;

      /* child[0] = __resultTmp declaration */
      declOper = &commaSeq->children[0];
      declOper->type = slang_oper_variable_decl;
      declOper->a_id = resultVar->a_name;
      declOper->locals->outer_scope = commaSeq->locals; /*** ??? **/

      /* child[1] = function body */
      inlined = &commaSeq->children[1];
      /* XXXX this may be inappropriate!!!!: */
      inlined->locals->outer_scope = commaSeq->locals;

      /* child[2] = __resultTmp reference */
      returnOper = &commaSeq->children[2];
      returnOper->type = slang_oper_identifier;
      returnOper->a_id = resultVar->a_name;
      returnOper->locals->outer_scope = commaSeq->locals;
      declOper->locals->outer_scope = commaSeq->locals;

      top = commaSeq;
   }
   else {
      top = inlined = slang_operation_new(1);
      /* XXXX this may be inappropriate!!!! */
      inlined->locals->outer_scope = oper->locals->outer_scope;
   }


   assert(inlined->locals);

   /* Examine the parameters, look for inout/out params, look for possible
    * substitutions, etc:
    *    param type      behaviour
    *     in             copy actual to local
    *     const in       substitute param with actual
    *     out            copy out
    */
   substCount = 0;
   for (i = 0; i < totalArgs; i++) {
      slang_variable *p = fun->parameters->variables[i];
      /*
      printf("Param %d: %s %s \n", i,
             slang_type_qual_string(p->type.qualifier),
	     (char *) p->a_name);
      */
      if (p->type.qualifier == slang_qual_inout ||
	  p->type.qualifier == slang_qual_out) {
	 /* an output param */
         slang_operation *arg;
         if (i < numArgs)
            arg = &args[i];
         else
            arg = returnOper;
	 paramMode[i] = SUBST;

	 if (arg->type == slang_oper_identifier)
            slang_resolve_variable(arg);

         /* replace parameter 'p' with argument 'arg' */
	 substOld[substCount] = p;
	 substNew[substCount] = arg; /* will get copied */
	 substCount++;
      }
      else if (p->type.qualifier == slang_qual_const) {
	 /* a constant input param */
	 if (args[i].type == slang_oper_identifier ||
	     args[i].type == slang_oper_literal_float) {
	    /* replace all occurances of this parameter variable with the
	     * actual argument variable or a literal.
	     */
	    paramMode[i] = SUBST;
            slang_resolve_variable(&args[i]);
	    substOld[substCount] = p;
	    substNew[substCount] = &args[i]; /* will get copied */
	    substCount++;
	 }
	 else {
	    paramMode[i] = COPY_IN;
	 }
      }
      else {
	 paramMode[i] = COPY_IN;
      }
      assert(paramMode[i]);
   }

   /* actual code inlining: */
   slang_operation_copy(inlined, fun->body);

   /*** XXX review this */
   assert(inlined->type = slang_oper_block_no_new_scope);
   inlined->type = slang_oper_block_new_scope;

#if 0
   printf("======================= orig body code ======================\n");
   printf("=== params scope = %p\n", (void*) fun->parameters);
   slang_print_tree(fun->body, 8);
   printf("======================= copied code =========================\n");
   slang_print_tree(inlined, 8);
#endif

   /* do parameter substitution in inlined code: */
   slang_substitute(A, inlined, substCount, substOld, substNew, GL_FALSE);

#if 0
   printf("======================= subst code ==========================\n");
   slang_print_tree(inlined, 8);
   printf("=============================================================\n");
#endif

   /* New prolog statements: (inserted before the inlined code)
    * Copy the 'in' arguments.
    */
   numCopyIn = 0;
   for (i = 0; i < numArgs; i++) {
      if (paramMode[i] == COPY_IN) {
	 slang_variable *p = fun->parameters->variables[i];
	 /* declare parameter 'p' */
	 slang_operation *decl = slang_operation_insert(&inlined->num_children,
							&inlined->children,
							numCopyIn);
         printf("COPY_IN %s from expr\n", (char*)p->a_name);
	 decl->type = slang_oper_variable_decl;
         assert(decl->locals);
	 decl->locals = fun->parameters;
	 decl->a_id = p->a_name;
	 decl->num_children = 1;
	 decl->children = slang_operation_new(1);

         /* child[0] is the var's initializer */
         slang_operation_copy(&decl->children[0], args + i);

	 numCopyIn++;
      }
   }

   /* New epilog statements:
    * 1. Create end of function label to jump to from return statements.
    * 2. Copy the 'out' parameter vars
    */
   {
      slang_operation *lab = slang_operation_insert(&inlined->num_children,
                                                    &inlined->children,
                                                    inlined->num_children);
      lab->type = slang_oper_label;
      lab->a_id = slang_atom_pool_atom(A->atoms, A->CurFunction->end_label);
   }

   for (i = 0; i < totalArgs; i++) {
      if (paramMode[i] == COPY_OUT) {
	 const slang_variable *p = fun->parameters->variables[i];
	 /* actualCallVar = outParam */
	 /*if (i > 0 || !haveRetValue)*/
	 slang_operation *ass = slang_operation_insert(&inlined->num_children,
						       &inlined->children,
						       inlined->num_children);
	 ass->type = slang_oper_assign;
	 ass->num_children = 2;
	 ass->locals = _slang_variable_scope_new(inlined->locals);
	 assert(ass->locals);
	 ass->children = slang_operation_new(2);
	 ass->children[0] = args[i]; /*XXX copy */
	 ass->children[1].type = slang_oper_identifier;
	 ass->children[1].a_id = p->a_name;
	 ass->children[1].locals = _slang_variable_scope_new(ass->locals);
      }
   }

   _mesa_free(paramMode);
   _mesa_free(substOld);
   _mesa_free(substNew);

   printf("Done Inline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);

   /*
   slang_print_tree(top, 0);
   */
   return top;
}


static slang_ir_node *
_slang_gen_function_call(slang_assemble_ctx *A, slang_function *fun,
			     slang_operation *oper, slang_operation *dest)
{
   slang_ir_node *n;
   slang_operation *inlined;
   slang_function *prevFunc;

   prevFunc = A->CurFunction;
   A->CurFunction = fun;

   if (!A->CurFunction->end_label) {
      char name[200];
      sprintf(name, "__endOfFunc_%s_", (char *) A->CurFunction->header.a_name);
      A->CurFunction->end_label = slang_atom_pool_gen(A->atoms, name);
   }

   if (slang_is_asm_function(fun) && !dest) {
      /* assemble assembly function - tree style */
      inlined = slang_inline_asm_function(A, fun, oper);
   }
   else {
      /* non-assembly function */
      inlined = slang_inline_function_call(A, fun, oper, dest);
   }

   /* Replace the function call with the inlined block */
#if 0
   slang_operation_construct(oper);
   slang_operation_copy(oper, inlined);
#else
   *oper = *inlined;
#endif


#if 0
   assert(inlined->locals);
   printf("*** Inlined code for call to %s:\n",
          (char*) fun->header.a_name);

   slang_print_tree(oper, 10);
   printf("\n");
#endif

   n = _slang_gen_operation(A, oper);

   A->CurFunction->end_label = NULL;

   A->CurFunction = prevFunc;

   return n;
}


static slang_asm_info *
slang_find_asm_info(const char *name)
{
   GLuint i;
   for (i = 0; AsmInfo[i].Name; i++) {
      if (_mesa_strcmp(AsmInfo[i].Name, name) == 0) {
         return AsmInfo + i;
      }
   }
   return NULL;
}


static GLuint
make_writemask(char *field)
{
   GLuint mask = 0x0;
   while (*field) {
      switch (*field) {
      case 'x':
         mask |= WRITEMASK_X;
         break;
      case 'y':
         mask |= WRITEMASK_Y;
         break;
      case 'z':
         mask |= WRITEMASK_Z;
         break;
      case 'w':
         mask |= WRITEMASK_W;
         break;
      default:
         abort();
      }
      field++;
   }
   if (mask == 0x0)
      return WRITEMASK_XYZW;
   else
      return mask;
}


/**
 * Generate IR tree for an asm instruction/operation such as:
 *    __asm vec4_dot __retVal.x, v1, v2;
 */
static slang_ir_node *
_slang_gen_asm(slang_assemble_ctx *A, slang_operation *oper,
                   slang_operation *dest)
{
   const slang_asm_info *info;
   slang_ir_node *kids[2], *n;
   GLuint j, firstOperand;

   assert(oper->type == slang_oper_asm);

   info = slang_find_asm_info((char *) oper->a_id);
   if (!info) {
      _mesa_problem(NULL, "undefined __asm function %s\n",
                    (char *) oper->a_id);
      assert(info);
   }
   assert(info->NumParams <= 2);

   if (info->NumParams == oper->num_children) {
      /* storage for result not specified */
      firstOperand = 0;
   }
   else {
      /* storage for result (child[0]) is specified */
      firstOperand = 1;
   }

   /* assemble child(ren) */
   kids[0] = kids[1] = NULL;
   for (j = 0; j < info->NumParams; j++) {
      kids[j] = _slang_gen_operation(A, &oper->children[firstOperand + j]);
   }

   n = new_node(info->Opcode, kids[0], kids[1]);

   if (firstOperand) {
      /* Setup n->Store to be a particular location.  Otherwise, storage
       * for the result (a temporary) will be allocated later.
       */
      GLuint writemask = WRITEMASK_XYZW;
      slang_operation *dest_oper;
      slang_ir_node *n0;

      dest_oper = &oper->children[0];
      if (dest_oper->type == slang_oper_field) {
         /* writemask */
         writemask = make_writemask((char*) dest_oper->a_id);
         dest_oper = &dest_oper->children[0];
      }

      n0 = _slang_gen_operation(A, dest_oper);
      assert(n0->Var);
      assert(n0->Store);
      assert(!n->Store);
      n->Store = n0->Store;
      n->Writemask = writemask;

      free(n0);
   }

   return n;
}



static GLboolean
_slang_is_noop(const slang_operation *oper)
{
   if (!oper ||
       oper->type == slang_oper_void ||
       (oper->num_children == 1 && oper->children[0].type == slang_oper_void))
      return GL_TRUE;
   else
      return GL_FALSE;
}


static slang_ir_node *
_slang_gen_cond(slang_ir_node *n)
{
   slang_ir_node *c = new_node(IR_COND, n, NULL);
   return c;
}


static void print_funcs(struct slang_function_scope_ *scope)
{
   int i;
   for (i = 0; i < scope->num_functions; i++) {
      slang_function *f = &scope->functions[i];
      printf("func %s\n", (char *) f->header.a_name);
      if (strcmp("vec3", (char*) f->header.a_name) == 0)
         printf("VEC3!\n");

   }
   if (scope->outer_scope)
      print_funcs(scope->outer_scope);
}


/**
 * Assemble a function call, given a particular function name.
 * \param name  the function's name (operators like '*' are possible).
 */
static slang_ir_node *
_slang_gen_function_call_name(slang_assemble_ctx *A, const char *name,
                              slang_operation *oper, slang_operation *dest)
{
   slang_operation *params = oper->children;
   const GLuint param_count = oper->num_children;
   slang_atom atom;
   slang_function *fun;

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return NULL;

   fun = _slang_locate_function(A->space.funcs, atom, params, param_count,
				&A->space, A->atoms);
   if (!fun) {
      /* XXX temporary */
      print_funcs(A->space.funcs);
      fun = _slang_locate_function(A->space.funcs, atom, params, param_count,
                                   &A->space, A->atoms);

      RETURN_ERROR2("Undefined function", name, 0);
   }

   return _slang_gen_function_call(A, fun, oper, dest);
}


/**
 * Generate IR tree for a while-loop.
 */
static slang_ir_node *
_slang_gen_while(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * label "__startWhile"
    * eval expr (child[0]), updating condcodes
    * branch if false to "__endWhile"
    * code body
    * jump "__startWhile"
    * label "__endWhile"
    */
   slang_atom startAtom = slang_atom_pool_gen(A->atoms, "__startWhile");
   slang_atom endAtom = slang_atom_pool_gen(A->atoms, "__endWhile");
   slang_ir_node *startLab, *cond, *bra, *body, *jump, *endLab, *tree;
   slang_atom prevLoopBreak = A->CurLoopBreak;
   slang_atom prevLoopCont = A->CurLoopCont;

   /* Push this loop */
   A->CurLoopBreak = endAtom;
   A->CurLoopCont = startAtom;

   startLab = new_label(startAtom);
   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = _slang_gen_cond(cond);
   tree = new_seq(startLab, cond);

   bra = new_cjump(endAtom);
   tree = new_seq(tree, bra);

   body = _slang_gen_operation(A, &oper->children[1]);
   tree = new_seq(tree, body);

   jump = new_jump(startAtom);
   tree = new_seq(tree, jump);

   endLab = new_label(endAtom);
   tree = new_seq(tree, endLab);

   /* Pop this loop */
   A->CurLoopBreak = prevLoopBreak;
   A->CurLoopCont = prevLoopCont;

   return tree;
}


/**
 * Generate IR tree for a for-loop.
 */
static slang_ir_node *
_slang_gen_for(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * init code (child[0])
    * label "__startFor"
    * eval expr (child[1]), updating condcodes
    * branch if false to "__endFor"
    * code body (child[3])
    * label "__continueFor"
    * incr code (child[2])
    * jump "__startFor"
    * label "__endFor"
    */
   slang_atom startAtom = slang_atom_pool_gen(A->atoms, "__startFor");
   slang_atom contAtom = slang_atom_pool_gen(A->atoms, "__continueFor");
   slang_atom endAtom = slang_atom_pool_gen(A->atoms, "__endFor");
   slang_ir_node *init, *startLab, *cond, *bra, *body, *contLab;
   slang_ir_node *incr, *jump, *endLab, *tree;
   slang_atom prevLoopBreak = A->CurLoopBreak;
   slang_atom prevLoopCont = A->CurLoopCont;

   /* Push this loop */
   A->CurLoopBreak = endAtom;
   A->CurLoopCont = contAtom;

   init = _slang_gen_operation(A, &oper->children[0]);
   startLab = new_label(startAtom);
   tree = new_seq(init, startLab);

   cond = _slang_gen_operation(A, &oper->children[1]);
   cond = _slang_gen_cond(cond);
   tree = new_seq(tree, cond);

   bra = new_cjump(endAtom);
   tree = new_seq(tree, bra);

   body = _slang_gen_operation(A, &oper->children[3]);
   tree = new_seq(tree, body);

   contLab = new_label(contAtom);
   tree = new_seq(tree, contLab);

   incr = _slang_gen_operation(A, &oper->children[2]);
   tree = new_seq(tree, incr);

   jump = new_jump(startAtom);
   tree = new_seq(tree, jump);

   endLab = new_label(endAtom);
   tree = new_seq(tree, endLab);

   /* Pop this loop */
   A->CurLoopBreak = prevLoopBreak;
   A->CurLoopCont = prevLoopCont;

   return tree;
}


/**
 * Generate IR tree for an if/then/else conditional.
 */
static slang_ir_node *
_slang_gen_if(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * eval expr (child[0]), updating condcodes
    * branch if false to _else or _endif
    * "true" code block
    * if haveElseClause clause:
    *    jump "__endif"
    *    label "__else"
    *    "false" code block
    * label "__endif"
    */
   const GLboolean haveElseClause = !_slang_is_noop(&oper->children[2]);
   slang_ir_node *cond, *bra, *trueBody, *endifLab, *tree;
   slang_atom elseAtom = slang_atom_pool_gen(A->atoms, "__else");
   slang_atom endifAtom = slang_atom_pool_gen(A->atoms, "__endif");

   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = _slang_gen_cond(cond);
   /*assert(cond->Store);*/
   bra = new_cjump(haveElseClause ? elseAtom : endifAtom);
   tree = new_seq(cond, bra);

   trueBody = _slang_gen_operation(A, &oper->children[1]);
   tree = new_seq(tree, trueBody);

   if (haveElseClause) {
      /* else clause */
      slang_ir_node *jump, *elseLab, *falseBody;
      jump = new_jump(endifAtom);
      tree = new_seq(tree, jump);

      elseLab = new_label(elseAtom);
      tree = new_seq(tree, elseLab);

      falseBody = _slang_gen_operation(A, &oper->children[2]);
      tree = new_seq(tree, falseBody);
   }

   endifLab = new_label(endifAtom);
   tree = new_seq(tree, endifLab);

   return tree;
}


/**
 * Generate IR node for storage of a temporary of given size.
 */
static slang_ir_node *
_slang_gen_temporary(GLint size)
{
   slang_ir_storage *store;
   slang_ir_node *n;

   store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, size);
   if (store) {
      n = new_node(IR_VAR_DECL, NULL, NULL);
      if (n) {
         n->Store = store;
      }
      else {
         free(store);
      }
   }
   return n;
}


/**
 * Generate IR node for allocating/declaring a variable.
 */
static slang_ir_node *
_slang_gen_var_decl(slang_assemble_ctx *A, slang_variable *var)
{
   slang_ir_node *n;
   n = new_node(IR_VAR_DECL, NULL, NULL);
   if (n) {
      n->Var = var;
      slang_allocate_storage(A, n);
      assert(n->Store);
      assert(n->Store->Index < 0);
      assert(n->Store->Size > 0);
      assert(var->aux);
      assert(n->Store == var->aux);
   }
   return n;
}




/**
 * Generate code for a selection expression:   b ? x : y
 */
static slang_ir_node *
_slang_gen_select(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_atom altAtom = slang_atom_pool_gen(A->atoms, "__selectAlt");
   slang_atom endAtom = slang_atom_pool_gen(A->atoms, "__selectEnd");
   slang_ir_node *altLab, *endLab;
   slang_ir_node *tree, *tmpDecl, *tmpVar, *cond, *cjump, *jump;
   slang_ir_node *bodx, *body, *assignx, *assigny;
    slang_assembly_typeinfo type;
   int size;

   /* size of x or y's type */
   slang_assembly_typeinfo_construct(&type);
   _slang_typeof_operation(A, &oper->children[1], &type);
   size = _slang_sizeof_type_specifier(&type.spec);
   assert(size > 0);

   /* temporary var */
   tmpDecl = _slang_gen_temporary(size);

   /* eval condition */
   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = _slang_gen_cond(cond);
   tree = new_seq(tmpDecl, cond);

   /* jump if true to "alt" label */
   cjump = new_cjump(altAtom);
   tree = new_seq(tree, cjump);

   /* evaluate child 2 (y) and assign to tmp */
   tmpVar = new_node(IR_VAR, NULL, NULL);
   tmpVar->Store = tmpDecl->Store;
   body = _slang_gen_operation(A, &oper->children[2]);
   assigny = new_node(IR_MOVE, tmpVar, body);
   tree = new_seq(tree, assigny);

   /* jump to "end" label */
   jump = new_jump(endAtom);
   tree = new_seq(tree, jump);

   /* "alt" label */
   altLab = new_label(altAtom);
   tree = new_seq(tree, altLab);

   /* evaluate child 1 (x) and assign to tmp */
   tmpVar = new_node(IR_VAR, NULL, NULL);
   tmpVar->Store = tmpDecl->Store;
   bodx = _slang_gen_operation(A, &oper->children[1]);
   assignx = new_node(IR_MOVE, tmpVar, bodx);
   tree = new_seq(tree, assignx);

   /* "end" label */
   endLab = new_label(endAtom);
   tree = new_seq(tree, endLab);
   
   /* tmp var value */
   tmpVar = new_node(IR_VAR, NULL, NULL);
   tmpVar->Store = tmpDecl->Store;
   tree = new_seq(tree, tmpVar);

   return tree;
}


/**
 * Generate IR tree for a return statement.
 */
static slang_ir_node *
_slang_gen_return(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->num_children == 0 ||
       (oper->num_children == 1 &&
        oper->children[0].type == slang_oper_void)) {
      /* Convert from:
       *   return;
       * To:
       *   goto __endOfFunction;
       */
      slang_ir_node *n;
      slang_operation gotoOp;
      slang_operation_construct(&gotoOp);
      gotoOp.type = slang_oper_goto;
      gotoOp.a_id = slang_atom_pool_atom(A->atoms, A->CurFunction->end_label);
      /* assemble the new code */
      n = _slang_gen_operation(A, &gotoOp);
      /* destroy temp code */
      slang_operation_destruct(&gotoOp);
      return n;
   }
   else {
      /*
       * Convert from:
       *   return expr;
       * To:
       *   __retVal = expr;
       *   goto __endOfFunction;
       */
      slang_operation *block, *assign, *jump;
      slang_atom a_retVal;
      slang_ir_node *n;

      a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);

#if 1 /* DEBUG */
      {
         slang_variable *v
            = _slang_locate_variable(oper->locals, a_retVal, GL_TRUE);
         assert(v);
      }
#endif

      block = slang_operation_new(1);
      block->type = slang_oper_block_no_new_scope;
      block->num_children = 2;
      block->children = slang_operation_new(2);
      assert(block->locals);
      block->locals->outer_scope = oper->locals->outer_scope;

      /* child[0]: __retVal = expr; */
      assign = &block->children[0];
      assign->type = slang_oper_assign;
      assign->locals->outer_scope = block->locals;
      assign->num_children = 2;
      assign->children = slang_operation_new(2);
      /* lhs (__retVal) */
      assign->children[0].type = slang_oper_identifier;
      assign->children[0].a_id = a_retVal;
      assign->children[0].locals->outer_scope = assign->locals;
      /* rhs (expr) */
      /* XXX we might be able to avoid this copy someday */
      slang_operation_copy(&assign->children[1], &oper->children[0]);

      /* child[1]: goto __endOfFunction */
      jump = &block->children[1];
      jump->type = slang_oper_goto;
      assert(A->CurFunction->end_label);
      jump->a_id = slang_atom_pool_atom(A->atoms, A->CurFunction->end_label);

#if 0 /* debug */
      printf("NEW RETURN:\n");
      slang_print_tree(block, 0);
#endif

      /* assemble the new code */
      n = _slang_gen_operation(A, block);
      slang_operation_delete(block);
      return n;
   }
}


/**
 * Generate IR tree for a variable declaration.
 */
static slang_ir_node *
_slang_gen_declaration(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_ir_node *n;
   slang_ir_node *varDecl;
   slang_variable *v;

   assert(oper->num_children == 0 || oper->num_children == 1);

   v = _slang_locate_variable(oper->locals, oper->a_id, GL_TRUE);
   assert(v);

   varDecl = _slang_gen_var_decl(A, v);

   if (oper->num_children > 0) {
      /* child is initializer */
      slang_ir_node *var, *init, *rhs;
      assert(oper->num_children == 1);
      var = new_var(A, oper, oper->a_id, SWIZZLE_NOOP);
      /* XXX make copy of this initializer? */
      /*
        printf("\n*** ASSEMBLE INITIALIZER %p\n", (void*) v->initializer);
      */
      rhs = _slang_gen_operation(A, &oper->children[0]);
      assert(rhs);
      init = new_node(IR_MOVE, var, rhs);
      /*assert(rhs->Opcode != IR_SEQ);*/
      n = new_seq(varDecl, init);
   }
   else if (v->initializer) {
      slang_ir_node *var, *init, *rhs;
      var = new_var(A, oper, oper->a_id, SWIZZLE_NOOP);
      /* XXX make copy of this initializer? */
      /*
        printf("\n*** ASSEMBLE INITIALIZER %p\n", (void*) v->initializer);
      */
      rhs = _slang_gen_operation(A, v->initializer);
      assert(rhs);
      init = new_node(IR_MOVE, var, rhs);
      /*
        assert(rhs->Opcode != IR_SEQ);
      */
      n = new_seq(varDecl, init);
   }
   else {
      n = varDecl;
   }
   return n;
}


/**
 * Generate IR tree for a variable (such as in an expression).
 */
static slang_ir_node *
_slang_gen_variable(slang_assemble_ctx * A, slang_operation *oper)
{
   /* If there's a variable associated with this oper (from inlining)
    * use it.  Otherwise, use the oper's var id.
    */
   slang_atom aVar = oper->var ? oper->var->a_name : oper->a_id;
   slang_ir_node *n = new_var(A, oper, aVar, SWIZZLE_NOOP);
   /*
   assert(oper->var);
   */
   return n;
}


/**
 * Generate IR tree for an assignment (=).
 */
static slang_ir_node *
_slang_gen_assignment(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->children[0].type == slang_oper_identifier &&
       oper->children[1].type == slang_oper_call) {
      /* Special case of:  x = f(a, b)
       * Replace with f(a, b, x)  (where x == hidden __retVal out param)
       *
       * XXX this could be even more effective if we could accomodate
       * cases such as "v.x = f();"  - would help with typical vertex
       * transformation.
       */
      slang_ir_node *n;
      n = _slang_gen_function_call_name(A,
                                      (const char *) oper->children[1].a_id,
                                      &oper->children[1], &oper->children[0]);
      return n;
   }
   else {
      slang_operation *lhs = &oper->children[0];
      slang_ir_node *n, *c0, *c1;
      GLuint mask = WRITEMASK_XYZW;
      if (lhs->type == slang_oper_field) {
         /* XXXX this is a hack! */
         /* writemask */
         if (!slang_is_writemask((char *) lhs->a_id, &mask))
            mask = WRITEMASK_XYZW;
         lhs = &lhs->children[0];
      }
      c0 = _slang_gen_operation(A, lhs);
      c1 = _slang_gen_operation(A, &oper->children[1]);

      assert(c1);
      n = new_node(IR_MOVE, c0, c1);
      /*
        assert(c1->Opcode != IR_SEQ);
      */
      if (c0->Writemask != WRITEMASK_XYZW)
         /* XXX this is a hack! */
         n->Writemask = c0->Writemask;
      else
         n->Writemask = mask;
      return n;
   }
}


/**
 * Generate IR tree for referencing a field in a struct (or basic vector type)
 */
static slang_ir_node *
_slang_gen_field(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_assembly_typeinfo ti;

   slang_assembly_typeinfo_construct(&ti);
   _slang_typeof_operation(A, &oper->children[0], &ti);

   if (_slang_type_is_vector(ti.spec.type)) {
      /* the field should be a swizzle */
      const GLuint rows = _slang_type_dim(ti.spec.type);
      slang_swizzle swz;
      slang_ir_node *n;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         RETURN_ERROR("Bad swizzle", 0);
      }
      n = _slang_gen_operation(A, &oper->children[0]);
      n->Swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                 swz.swizzle[1],
                                 swz.swizzle[2],
                                 swz.swizzle[3]);
      return n;
   }
   else if (ti.spec.type == slang_spec_float) {
      const GLuint rows = 1;
      slang_swizzle swz;
      slang_ir_node *n;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         RETURN_ERROR("Bad swizzle", 0);
      }
      n = _slang_gen_operation(A, &oper->children[0]);
      n->Swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                 swz.swizzle[1],
                                 swz.swizzle[2],
                                 swz.swizzle[3]);
      return n;
   }
   else {
      /* the field is a structure member */
      abort();
   }
}


/**
 * Gen code for array indexing.
 */
static slang_ir_node *
_slang_gen_subscript(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_assembly_typeinfo array_ti;

   /* get array's type info */
   slang_assembly_typeinfo_construct(&array_ti);
   _slang_typeof_operation(A, &oper->children[0], &array_ti);

   if (_slang_type_is_vector(array_ti.spec.type)) {
      /* indexing a simple vector type: "vec4 v; v[0]=p;" */
      /* translate the index into a swizzle/writemask: "v.x=p" */
      const GLuint max = _slang_type_dim(array_ti.spec.type);
      GLint index;
      slang_ir_node *n;

      index = (GLint) oper->children[1].literal[0];
      if (oper->children[1].type != slang_oper_literal_int ||
          index >= max) {
         RETURN_ERROR("Invalid array index", 0);
      }

      n = _slang_gen_operation(A, &oper->children[0]);
      if (n) {
         /* use swizzle to access the element */
         n->Swizzle = SWIZZLE_X + index;
         n->Writemask = WRITEMASK_X << index;
      }
      return n;
   }
   else {
      /* conventional array */
      slang_assembly_typeinfo elem_ti;
      slang_ir_node *elem, *array, *index;
      GLint elemSize;

      /* size of array element */
      slang_assembly_typeinfo_construct(&elem_ti);
      _slang_typeof_operation(A, oper, &elem_ti);
      elemSize = _slang_sizeof_type_specifier(&elem_ti.spec);
      assert(elemSize >= 1);

      array = _slang_gen_operation(A, &oper->children[0]);
      index = _slang_gen_operation(A, &oper->children[1]);
      if (array && index) {
         elem = new_node(IR_ELEMENT, array, index);
         elem->Store = _slang_new_ir_storage(array->Store->File,
                                             array->Store->Index,
                                             elemSize);
         return elem;
      }
      else {
         return NULL;
      }
   }
}



/**
 * Generate IR tree for a slang_operation (AST node)
 */
static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper)
{
   switch (oper->type) {
   case slang_oper_block_new_scope:
      {
         slang_ir_node *n;

         A->vartable = _slang_push_var_table(A->vartable);

         oper->type = slang_oper_block_no_new_scope; /* temp change */
         n = _slang_gen_operation(A, oper);
         oper->type = slang_oper_block_new_scope; /* restore */

         A->vartable = _slang_pop_var_table(A->vartable);

         if (n)
            n = new_node(IR_SCOPE, n, NULL);
         return n;
      }
      break;

   case slang_oper_block_no_new_scope:
      /* list of operations */
      assert(oper->num_children > 0);
      {
         slang_ir_node *n, *tree = NULL;
         GLuint i;

         for (i = 0; i < oper->num_children; i++) {
            n = _slang_gen_operation(A, &oper->children[i]);
            if (!n) {
               _slang_free_ir_tree(tree);
               return NULL; /* error must have occured */
            }
            tree = tree ? new_seq(tree, n) : n;
         }

#if 00
         if (oper->locals->num_variables > 0) {
            int i;
            /*
            printf("\n****** Deallocate vars in scope!\n");
            */
            for (i = 0; i < oper->locals->num_variables; i++) {
               slang_variable *v = oper->locals->variables + i;
               if (v->aux) {
                  slang_ir_storage *store = (slang_ir_storage *) v->aux;
                  /*
                  printf("  Deallocate var %s\n", (char*) v->a_name);
                  */
                  assert(store->File == PROGRAM_TEMPORARY);
                  assert(store->Index >= 0);
                  _slang_free_temp(A->vartable, store->Index, store->Size);
               }
            }
         }
#endif
         return tree;
      }
      break;
   case slang_oper_expression:
      return _slang_gen_operation(A, &oper->children[0]);
      break;
   case slang_oper_while:
      return _slang_gen_while(A, oper);
   case slang_oper_for:
      return _slang_gen_for(A, oper);
   case slang_oper_break:
      if (!A->CurLoopBreak) {
         RETURN_ERROR("'break' not in loop", 0);
      }
      return new_jump(A->CurLoopBreak);
   case slang_oper_continue:
      if (!A->CurLoopCont) {
         RETURN_ERROR("'continue' not in loop", 0);
      }
      return new_jump(A->CurLoopCont);
   case slang_oper_equal:
      return new_node(IR_SEQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_notequal:
      return new_node(IR_SNEQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_greater:
      return new_node(IR_SGT,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_less:
      /* child[0] < child[1]  ---->   child[1] > child[0] */
      return new_node(IR_SGT,
                      _slang_gen_operation(A, &oper->children[1]),
                      _slang_gen_operation(A, &oper->children[0]));
   case slang_oper_greaterequal:
      return new_node(IR_SGE,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_lessequal:
      /* child[0] <= child[1]  ---->   child[1] >= child[0] */
      return new_node(IR_SGE,
                      _slang_gen_operation(A, &oper->children[1]),
                      _slang_gen_operation(A, &oper->children[0]));
   case slang_oper_add:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+", oper, NULL);
	 return n;
      }
   case slang_oper_subtract:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case slang_oper_multiply:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
         n = _slang_gen_function_call_name(A, "*", oper, NULL);
	 return n;
      }
   case slang_oper_divide:
      {
         slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/", oper, NULL);
	 return n;
      }
   case slang_oper_minus:
      {
         slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case slang_oper_plus:
      /* +expr   --> do nothing */
      return _slang_gen_operation(A, &oper->children[0]);
   case slang_oper_variable_decl:
      return _slang_gen_declaration(A, oper);
   case slang_oper_assign:
      return _slang_gen_assignment(A, oper);
   case slang_oper_addassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+=", oper, &oper->children[0]);
	 return n;
      }
   case slang_oper_subassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-=", oper, &oper->children[0]);
	 return n;
      }
      break;
   case slang_oper_mulassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "*=", oper, &oper->children[0]);
	 return n;
      }
   case slang_oper_divassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/=", oper, &oper->children[0]);
	 return n;
      }
   case slang_oper_logicalor:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "__logicalOr", oper, NULL);
	 return n;
      }
   case slang_oper_logicalxor:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "__logicalXor", oper, NULL);
	 return n;
      }
   case slang_oper_logicaland:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "__logicalAnd", oper, NULL);
	 return n;
      }
   case slang_oper_not:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__logicalNot", oper, NULL);
	 return n;
      }

   case slang_oper_select:  /* b ? x : y */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 3);
	 n = _slang_gen_select(A, oper );
	 return n;
      }

   case slang_oper_asm:
      return _slang_gen_asm(A, oper, NULL);
   case slang_oper_call:
      return _slang_gen_function_call_name(A, (const char *) oper->a_id,
                                           oper, NULL);
   case slang_oper_return:
      return _slang_gen_return(A, oper);
   case slang_oper_goto:
      return new_jump((char*) oper->a_id);
   case slang_oper_label:
      return new_label((char*) oper->a_id);
   case slang_oper_identifier:
      return _slang_gen_variable(A, oper);
   case slang_oper_if:
      return _slang_gen_if(A, oper);
   case slang_oper_field:
      return _slang_gen_field(A, oper);
   case slang_oper_subscript:
      return _slang_gen_subscript(A, oper);
   case slang_oper_literal_float:
      return new_float_literal(oper->literal[0], oper->literal[1],
                               oper->literal[2], oper->literal[3]);
   case slang_oper_literal_int:
      return new_float_literal(oper->literal[0], 0, 0, 0);
   case slang_oper_literal_bool:
      return new_float_literal(oper->literal[0], 0, 0, 0);

   case slang_oper_postincrement:   /* var++ */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postIncr", oper, NULL);
	 return n;
      }
   case slang_oper_postdecrement:   /* var-- */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postDecr", oper, NULL);
	 return n;
      }
   case slang_oper_preincrement:   /* ++var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "++", oper, NULL);
	 return n;
      }
   case slang_oper_predecrement:   /* --var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "--", oper, NULL);
	 return n;
      }

   case slang_oper_sequence:
      {
         slang_ir_node *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            slang_ir_node *n = _slang_gen_operation(A, &oper->children[i]);
            tree = tree ? new_seq(tree, n) : n;
         }
         return tree;
      }

   case slang_oper_none:
      return NULL;
   default:
      printf("Unhandled node type %d\n", oper->type);
      abort();
      return new_node(IR_NOP, NULL, NULL);
   }
      abort();
   return NULL;
}



/**
 * Called by compiler when a global variable has been parsed/compiled.
 * Here we examine the variable's type to determine what kind of register
 * storage will be used.
 *
 * A uniform such as "gl_Position" will become the register specification
 * (PROGRAM_OUTPUT, VERT_RESULT_HPOS).  Or, uniform "gl_FogFragCoord"
 * will be (PROGRAM_INPUT, FRAG_ATTRIB_FOGC).
 *
 * Samplers are interesting.  For "uniform sampler2D tex;" we'll specify
 * (PROGRAM_SAMPLER, index) where index is resolved at link-time to an
 * actual texture unit (as specified by the user calling glUniform1i()).
 */
GLboolean
_slang_codegen_global_variable(slang_assemble_ctx *A, slang_variable *var,
                               slang_unit_type type)
{
   struct gl_program *prog = A->program;
   const char *varName = (char *) var->a_name;
   GLboolean success = GL_TRUE;
   GLint texIndex;
   slang_ir_storage *store = NULL;
   int dbg = 0;

   texIndex = sampler_to_texture_index(var->type.specifier.type);

   if (texIndex != -1) {
      /* Texture sampler:
       * store->File = PROGRAM_SAMPLER
       * store->Index = sampler uniform location
       * store->Size = texture type index (1D, 2D, 3D, cube, etc)
       */
      GLint samplerUniform = _mesa_add_sampler(prog->Parameters, varName);
      store = _slang_new_ir_storage(PROGRAM_SAMPLER, samplerUniform, texIndex);
      if (dbg) printf("SAMPLER ");
   }
   else if (var->type.qualifier == slang_qual_uniform) {
      /* Uniform variable */
      const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
      if (prog) {
         /* user-defined uniform */
         GLint uniformLoc = _mesa_add_uniform(prog->Parameters, varName, size);
         store = _slang_new_ir_storage(PROGRAM_UNIFORM, uniformLoc, size);
      }
      else {
         /* pre-defined uniform, like gl_ModelviewMatrix */
         /* We know it's a uniform, but don't allocate storage unless
          * it's really used.
          */
         store = _slang_new_ir_storage(PROGRAM_STATE_VAR, -1, size);
      }
      if (dbg) printf("UNIFORM ");
   }
   else if (var->type.qualifier == slang_qual_varying) {
      const GLint size = 4; /* XXX fix */
      if (prog) {
         /* user-defined varying */
         GLint varyingLoc = _mesa_add_varying(prog->Varying, varName, size);
         store = _slang_new_ir_storage(PROGRAM_VARYING, varyingLoc, size);
      }
      else {
         /* pre-defined varying, like gl_Color or gl_TexCoord */
         if (type == slang_unit_fragment_builtin) {
            GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB);
            assert(index >= 0);
            store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
            assert(index < FRAG_ATTRIB_MAX);
         }
         else {
            GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
            assert(index >= 0);
            assert(type == slang_unit_vertex_builtin);
            store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
            assert(index < VERT_RESULT_MAX);
         }
         if (dbg) printf("V/F ");
      }
      if (dbg) printf("VARYING ");
   }
   else if (var->type.qualifier == slang_qual_attribute) {
      if (prog) {
         /* user-defined vertex attribute */
         const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
         const GLint attr = -1; /* unknown */
         GLint index = _mesa_add_attribute(prog->Attributes, varName,
                                           size, attr);
         assert(index >= 0);
         store = _slang_new_ir_storage(PROGRAM_INPUT,
                                       VERT_ATTRIB_GENERIC0 + index, size);
      }
      else {
         /* pre-defined vertex attrib */
         GLint index = _slang_input_index(varName, GL_VERTEX_PROGRAM_ARB);
         GLint size = 4; /* XXX? */
         assert(index >= 0);
         store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
      }
      if (dbg) printf("ATTRIB ");
   }
   else if (var->type.qualifier == slang_qual_fixedinput) {
      GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB);
      GLint size = 4; /* XXX? */
      store = _slang_new_ir_storage(PROGRAM_INPUT, index, size);
      if (dbg) printf("INPUT ");
   }
   else if (var->type.qualifier == slang_qual_fixedoutput) {
      if (type == slang_unit_vertex_builtin) {
         GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
         GLint size = 4; /* XXX? */
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      else {
         assert(type == slang_unit_fragment_builtin);
         GLint index = _slang_output_index(varName, GL_FRAGMENT_PROGRAM_ARB);
         GLint size = 4; /* XXX? */
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      if (dbg) printf("OUTPUT ");
   }
   else if (var->type.qualifier == slang_qual_const && !prog) {
      /* pre-defined global constant, like gl_MaxLights */
      const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
      store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
      if (dbg) printf("CONST ");
   }
   else {
      /* ordinary variable (may be const) */
      slang_ir_node *n;

      /* IR node to declare the variable */
      n = _slang_gen_var_decl(A, var);

      /* IR code for the var's initializer, if present */
      if (var->initializer) {
         slang_ir_node *lhs, *rhs, *init;

         /* Generate IR_MOVE instruction to initialize the variable */
         lhs = new_node(IR_VAR, NULL, NULL);
         lhs->Var = var;
         lhs->Swizzle = SWIZZLE_NOOP;
         lhs->Store = n->Store;

         /* constant folding, etc */
         slang_simplify(var->initializer, &A->space, A->atoms);

         rhs = _slang_gen_operation(A, var->initializer);
         assert(rhs);
         init = new_node(IR_MOVE, lhs, rhs);
         n = new_seq(n, init);
      }

      success = _slang_emit_code(n, A->vartable, A->program, GL_FALSE);

      _slang_free_ir_tree(n);
   }

   if (dbg) printf("GLOBAL VAR %s  idx %d\n", (char*) var->a_name,
                   store ? store->Index : -2);

   if (store)
      var->aux = store;  /* save var's storage info */

   return success;
}


/**
 * Produce an IR tree from a function AST (fun->body).
 * Then call the code emitter to convert the IR tree into gl_program
 * instructions.
 */
GLboolean
_slang_codegen_function(slang_assemble_ctx * A, slang_function * fun)
{
   slang_ir_node *n, *endLabel;
   GLboolean success = GL_TRUE;

   if (_mesa_strcmp((char *) fun->header.a_name, "main") != 0) {
      /* we only really generate code for main, all other functions get
       * inlined.
       */
      return GL_TRUE;  /* not an error */
   }

#if 1
   printf("\n*********** codegen_function %s\n", (char *) fun->header.a_name);
#endif
#if 0
   slang_print_function(fun, 1);
#endif

   /* should have been allocated earlier: */
   assert(A->program->Parameters );
   assert(A->program->Varying);
   assert(A->vartable);

   /* fold constant expressions, etc. */
   slang_simplify(fun->body, &A->space, A->atoms);

   A->CurFunction = fun;

   /* Create an end-of-function label */
   if (!A->CurFunction->end_label)
      A->CurFunction->end_label = slang_atom_pool_gen(A->atoms, "__endOfFunc_main_");

   /* push new vartable scope */
   A->vartable = _slang_push_var_table(A->vartable);

   /* Generate IR tree for the function body code */
   n = _slang_gen_operation(A, fun->body);
   if (n)
      n = new_node(IR_SCOPE, n, NULL);

   /* pop vartable, restore previous */
   A->vartable = _slang_pop_var_table(A->vartable);

   if (!n) {
      /* XXX record error */
      return GL_FALSE;
   }

   /* append an end-of-function-label to IR tree */
   endLabel = new_label(fun->end_label);
   n = new_seq(n, endLabel);

   A->CurFunction = NULL;

#if 0
   printf("************* New AST for %s *****\n", (char*)fun->header.a_name);
   slang_print_function(fun, 1);
#endif
#if 0
   printf("************* IR for %s *******\n", (char*)fun->header.a_name);
   slang_print_ir(n, 0);
#endif
#if 1
   printf("************* End codegen function ************\n\n");
#endif

   /* Emit program instructions */
   success = _slang_emit_code(n, A->vartable, A->program, GL_TRUE);
   _slang_free_ir_tree(n);

   /* free codegen context */
   /*
   _mesa_free(A->codegen);
   */

   return success;
}

