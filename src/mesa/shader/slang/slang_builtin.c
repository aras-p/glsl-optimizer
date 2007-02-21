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
 * \file slang_builtin.c
 * Resolve built-in uniform vars.
 * \author Brian Paul
 */

#include "imports.h"
#include "slang_builtin.h"
#include "slang_ir.h"
#include "mtypes.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"


/**
 * Lookup GL state given a variable name, 0, 1 or 2 indexes and a field.
 * Allocate room for the state in the given param list and return position
 * in the list.
 * Yes, this is kind of ugly, but it works.
 */
static GLint
lookup_statevar(const char *var, GLint index1, GLint index2, const char *field,
                GLuint *swizzleOut,
                struct gl_program_parameter_list *paramList)
{
   gl_state_index tokens[STATE_LENGTH];
   GLuint i;
   GLboolean isMatrix = GL_FALSE;

   for (i = 0; i < STATE_LENGTH; i++) {
      tokens[i] = 0;
   }
   *swizzleOut = SWIZZLE_NOOP;

   if (strcmp(var, "gl_ModelViewMatrix") == 0) {
      tokens[0] = STATE_MODELVIEW_MATRIX;
      isMatrix = GL_TRUE;
   }
   else if (strcmp(var, "gl_ModelProjectionMatrix") == 0) {
      tokens[0] = STATE_PROJECTION_MATRIX;
      isMatrix = GL_TRUE;
   }
   else if (strcmp(var, "gl_ModelViewProjectionMatrix") == 0) {
      tokens[0] = STATE_MVP_MATRIX;
      isMatrix = GL_TRUE;
   }
   else if (strcmp(var, "gl_NormalMatrix") == 0) {
      tokens[0] = STATE_MODELVIEW_MATRIX;
      isMatrix = GL_TRUE;
   }
   else if (strcmp(var, "gl_TextureMatrix") == 0) {
      tokens[0] = STATE_TEXTURE_MATRIX;
      if (index2 >= 0)
         tokens[1] = index2;
      isMatrix = GL_TRUE;
   }
   else if (strcmp(var, "gl_DepthRange") == 0) {
      tokens[0] = STATE_DEPTH_RANGE;
      if (strcmp(field, "near") == 0) {
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "far") == 0) {
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "diff") == 0) {
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_ClipPlane") == 0) {
      tokens[0] = STATE_CLIPPLANE;
      tokens[1] = index1;
   }
   else if (strcmp(var, "gl_FrontMaterial") == 0 ||
            strcmp(var, "gl_BackMaterial") == 0) {
      tokens[0] = STATE_MATERIAL;
      if (strcmp(var, "gl_FrontMaterial") == 0)
         tokens[1] = 0;
      else
         tokens[1] = 1;
      if (strcmp(field, "emission") == 0) {
         tokens[2] = STATE_EMISSION;
      }
      else if (strcmp(field, "ambient") == 0) {
         tokens[2] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuse") == 0) {
         tokens[2] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[2] = STATE_SPECULAR;
      }
      else if (strcmp(field, "shininess") == 0) {
         tokens[2] = STATE_SHININESS;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_LightSource") == 0) {
      tokens[0] = STATE_LIGHT;
      tokens[1] = index1;
      if (strcmp(field, "ambient") == 0) {
         tokens[2] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuse") == 0) {
         tokens[2] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[2] = STATE_SPECULAR;
      }
      else if (strcmp(field, "position") == 0) {
         tokens[2] = STATE_POSITION;
      }
      else if (strcmp(field, "halfVector") == 0) {
         tokens[2] = STATE_HALF_VECTOR;
      }
      else if (strcmp(field, "spotDirection") == 0) {
         tokens[2] = STATE_SPOT_DIRECTION;
      }
      else if (strcmp(field, "spotCosCutoff") == 0) {
         tokens[2] = STATE_SPOT_DIRECTION;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else if (strcmp(field, "spotCutoff") == 0) {
         tokens[2] = STATE_SPOT_CUTOFF;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "spotExponent") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else if (strcmp(field, "constantAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "linearAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "quadraticAttenuation") == 0) {
         tokens[2] = STATE_ATTENUATION;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_LightModel") == 0) {
      if (strcmp(field, "ambient") == 0) {
         tokens[0] = STATE_LIGHTMODEL_AMBIENT;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_FrontLightModelProduct") == 0) {
      if (strcmp(field, "ambient") == 0) {
         tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
         tokens[1] = 0;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_BackLightModelProduct") == 0) {
      if (strcmp(field, "ambient") == 0) {
         tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
         tokens[1] = 1;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_FrontLightProduct") == 0 ||
            strcmp(var, "gl_BackLightProduct") == 0) {
      tokens[0] = STATE_LIGHTPROD;
      tokens[1] = index1; /* light number */
      if (strcmp(var, "gl_FrontLightProduct") == 0) {
         tokens[2] = 0; /* front */
      }
      else {
         tokens[2] = 1; /* back */
      }
      if (strcmp(field, "ambient") == 0) {
         tokens[3] = STATE_AMBIENT;
      }
      else if (strcmp(field, "diffuset") == 0) {
         tokens[3] = STATE_DIFFUSE;
      }
      else if (strcmp(field, "specular") == 0) {
         tokens[3] = STATE_SPECULAR;
      }
      else {
         return -1;
      }
   }
   else if (strcmp(var, "gl_TextureEnvColor") == 0) {
      tokens[0] = STATE_TEXENV_COLOR;
      tokens[1] = index1;
   }
   else if (strcmp(var, "gl_EyePlaneS") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_S;
   }
   else if (strcmp(var, "gl_EyePlaneT") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_T;
   }
   else if (strcmp(var, "gl_EyePlaneR") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_R;
   }
   else if (strcmp(var, "gl_EyePlaneQ") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_EYE_Q;
   }
   else if (strcmp(var, "gl_ObjectPlaneS") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_S;
   }
   else if (strcmp(var, "gl_ObjectPlaneT") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_T;
   }
   else if (strcmp(var, "gl_ObjectPlaneR") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_R;
   }
   else if (strcmp(var, "gl_ObjectPlaneQ") == 0) {
      tokens[0] = STATE_TEXGEN;
      tokens[1] = index1; /* tex unit */
      tokens[2] = STATE_TEXGEN_OBJECT_Q;
   }
   else if (strcmp(var, "gl_Fog") == 0) {
      tokens[0] = STATE_FOG;
      if (strcmp(field, "color") == 0) {
         tokens[1] = STATE_FOG_COLOR;
      }
      else if (strcmp(field, "density") == 0) {
         tokens[1] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_XXXX;
      }
      else if (strcmp(field, "start") == 0) {
         tokens[1] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_YYYY;
      }
      else if (strcmp(field, "end") == 0) {
         tokens[1] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_ZZZZ;
      }
      else if (strcmp(field, "scale") == 0) {
         tokens[1] = STATE_FOG_PARAMS;
         *swizzleOut = SWIZZLE_WWWW;
      }
      else {
         return -1;
      }
   }
   else {
      return -1;
   }

   if (isMatrix) {
      /* load all four columns of matrix */
      GLint pos[4];
      GLuint j;
      for (j = 0; j < 4; j++) {
         tokens[2] = tokens[3] = j; /* jth row of matrix */
         pos[j] = _mesa_add_state_reference(paramList, (GLint *) tokens);
         assert(pos[j] >= 0);
         ASSERT(pos[j] >= 0);
      }
      return pos[0] + index1;
   }
   else {
      /* allocate a single register */
      GLint pos = _mesa_add_state_reference(paramList, (GLint *) tokens);
      ASSERT(pos >= 0);
      return pos;
   }
}


/**
 * Allocate storage for a pre-defined uniform (a GL state variable).
 * As a memory-saving optimization, we try to only allocate storage for
 * state vars that are actually used.
 * For example, the "gl_LightSource" uniform is huge.  If we only use
 * a handful of gl_LightSource fields, we don't want to allocate storage
 * for all of gl_LightSource.
 *
 * Currently, all pre-defined uniforms are in one of these forms:
 *   var
 *   var[i]
 *   var.field
 *   var[i].field
 *   var[i][j]
 */
GLint
_slang_alloc_statevar(slang_ir_node *n,
                      struct gl_program_parameter_list *paramList)
{
   const char *field = NULL, *var;
   GLint index1 = -1, index2 = -1, pos;
   GLuint swizzle;

   if (n->Opcode == IR_FIELD) {
      field = n->Target;
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      assert(n->Children[1]->Opcode == IR_FLOAT);
      index1 = (GLint) n->Children[1]->Value[0];
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      assert(n->Children[1]->Opcode == IR_FLOAT);
      index2 = (GLint) n->Children[1]->Value[0];
      n = n->Children[0];
   }

   assert(n->Opcode == IR_VAR);
   var = (char *) n->Var->a_name;

   pos = lookup_statevar(var, index1, index2, field, &swizzle, paramList);
   assert(pos >= 0);
   if (pos >= 0) {
      n->Store->Index = pos;
      n->Store->Swizzle = swizzle;
   }
   return pos;
}

