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
#include "macros.h"
#include "slang_builtin.h"
#include "slang_typeinfo.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_ir.h"
#include "mtypes.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
#include "slang_print.h"



/**
 * XXX we might consider moving much of this into the prog_statevars.c file
 */


/**
 * Determine if 'name' is a state variable (pre-defined uniform).
 * If so, create a new program parameter for it, and return the
 * param's index.
 *
 * \param swizzleOut  returns the swizzle needed to access 'float' values
 * \return the state value's position in the parameter list, or -1 if error
 */
GLint
_slang_lookup_statevar(const char *name, GLint index,
                       struct gl_program_parameter_list *paramList,
                       GLuint *swizzleOut)
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
      { "gl_NormalScale", 1, SWIZZLE_NOOP,
        { STATE_INTERNAL, STATE_NORMAL_SCALE, 0, 0, 0, 0} },

      /* For aggregate/structs we need entries for both the base name
       * and base.field.
       */
      { "gl_DepthRange", 1, SWIZZLE_NOOP,
        { STATE_DEPTH_RANGE, 0, 0, 0, 0, 0 } },
      { "gl_DepthRange.near", 1, SWIZZLE_XXXX,
        { STATE_DEPTH_RANGE, 0, 0, 0, 0, 0 } },
      { "gl_DepthRange.far", 1, SWIZZLE_YYYY,
        { STATE_DEPTH_RANGE, 0, 0, 0, 0, 0 } },
      { "gl_DepthRange.diff", 1, SWIZZLE_ZZZZ,
        { STATE_DEPTH_RANGE, 0, 0, 0, 0, 0 } },

      { "gl_Point", 1, SWIZZLE_NOOP,
        { STATE_POINT_SIZE, 0, 0, 0, 0, 0 } },
      { "gl_Point.size", 1, SWIZZLE_XXXX,
        { STATE_POINT_SIZE, 0, 0, 0, 0, 0 } },
      { "gl_Point.sizeMin", 1, SWIZZLE_YYYY,
        { STATE_POINT_SIZE, 0, 0, 0, 0, 0 } },
      { "gl_Point.sizeMax", 1, SWIZZLE_ZZZZ,
        { STATE_POINT_SIZE, 0, 0, 0, 0, 0 } },
      { "gl_Point.fadeThresholdSize", 1, SWIZZLE_WWWW,
        { STATE_POINT_SIZE, 0, 0, 0, 0, 0 } },
      { "gl_Point.distanceConstantAttenuation", 1, SWIZZLE_XXXX,
        { STATE_POINT_ATTENUATION, 0, 0, 0, 0, 0 } },
      { "gl_Point.distanceLinearAttenuation", 1, SWIZZLE_YYYY,
        { STATE_POINT_ATTENUATION, 0, 0, 0, 0, 0 } },
      { "gl_Point.distanceQuadraticAttenuation", 1, SWIZZLE_ZZZZ,
        { STATE_POINT_ATTENUATION, 0, 0, 0, 0, 0 } },

      { "gl_FrontMaterial", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 0, STATE_EMISSION, 0, 0, 0 } },
      { "gl_FrontMaterial.emission", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 0, STATE_EMISSION, 0, 0, 0 } },
      { "gl_FrontMaterial.ambient", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 0, STATE_AMBIENT, 0, 0, 0 } },
      { "gl_FrontMaterial.diffuse", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 0, STATE_DIFFUSE, 0, 0, 0 } },
      { "gl_FrontMaterial.specular", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 0, STATE_SPECULAR, 0, 0, 0 } },
      { "gl_FrontMaterial.shininess", 1, SWIZZLE_XXXX,
        { STATE_MATERIAL, 0, STATE_SHININESS, 0, 0, 0 } },

      { "gl_BackMaterial", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 1, STATE_EMISSION, 0, 0, 0 } },
      { "gl_BackMaterial.emission", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 1, STATE_EMISSION, 0, 0, 0 } },
      { "gl_BackMaterial.ambient", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 1, STATE_AMBIENT, 0, 0, 0 } },
      { "gl_BackMaterial.diffuse", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 1, STATE_DIFFUSE, 0, 0, 0 } },
      { "gl_BackMaterial.specular", 1, SWIZZLE_NOOP,
        { STATE_MATERIAL, 1, STATE_SPECULAR, 0, 0, 0 } },
      { "gl_BackMaterial.shininess", 1, SWIZZLE_XXXX,
        { STATE_MATERIAL, 1, STATE_SHININESS, 0, 0, 0 } },

      { "gl_LightModel", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_AMBIENT, 0, 0, 0, 0, 0 } },
      { "gl_LightModel.ambient", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_AMBIENT, 0, 0, 0, 0, 0 } },

      { "gl_FrontLightModelProduct", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_SCENECOLOR, 0, 0, 0, 0, 0 } },
      { "gl_FrontLightModelProduct.sceneColor", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_SCENECOLOR, 0, 0, 0, 0, 0 } },

      { "gl_BackLightModelProduct", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_SCENECOLOR, 1, 0, 0, 0, 0 } },
      { "gl_BackLightModelProduct.sceneColor", 1, SWIZZLE_NOOP,
        { STATE_LIGHTMODEL_SCENECOLOR, 1, 0, 0, 0, 0 } },

      { "gl_FrontLightProduct", 1, SWIZZLE_NOOP,
        { STATE_LIGHTPROD, 0, STATE_AMBIENT, 0, 0, 0 } },


      { "gl_Fog", 1, SWIZZLE_NOOP,
        { STATE_FOG_COLOR, 0, 0, 0, 0, 0 } },
      { "gl_Fog.color", 1, SWIZZLE_NOOP,
        { STATE_FOG_COLOR, 0, 0, 0, 0, 0 } },
      { "gl_Fog.density", 1, SWIZZLE_XXXX,
        { STATE_FOG_PARAMS, 0, 0, 0, 0, 0 } },
      { "gl_Fog.start", 1, SWIZZLE_YYYY,
        { STATE_FOG_PARAMS, 0, 0, 0, 0, 0 } },
      { "gl_Fog.end", 1, SWIZZLE_ZZZZ,
        { STATE_FOG_PARAMS, 0, 0, 0, 0, 0 } },
      { "gl_Fog.scale", 1, SWIZZLE_WWWW,
        { STATE_FOG_PARAMS, 0, 0, 0, 0, 0 } },

      { "gl_ClipPlane", 1, SWIZZLE_NOOP,
        { STATE_CLIPPLANE, 0, 0, 0, 0, 0 } },

      { NULL, 0, 0, {0, 0, 0, 0, 0, 0} }
   };
   GLuint i;

   for (i = 0; state[i].Name; i++) {
      if (strcmp(state[i].Name, name) == 0) {
         /* found */
         *swizzleOut = state[i].Swizzle;
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


GLint
_slang_lookup_statevar_field(const char *base, const char *field,
                             struct gl_program_parameter_list *paramList,
                             GLuint *swizzleOut)
{
   GLint pos = -1;
   const GLint len = _mesa_strlen(base)
                   + _mesa_strlen(field) + 2;
   char *name = (char *) _mesa_malloc(len);

   if (!name)
      return -1;

   _mesa_strcpy(name, base);
   /*_mesa_*/strcat(name, ".");
   /*_mesa_*/strcat(name, field);
   printf("FULL NAME: %s\n", name);

   pos = _slang_lookup_statevar(name, 0, paramList, swizzleOut);

   _mesa_free(name);

   return pos;
}


struct field_info {
   const char *Name;
   gl_state_index Token;
   GLint TokenPos;
   GLuint Swizzle;
};

#define MT_FIELD { NULL, 0, -1, 0 }
#define MAX_FIELDS 5
#define INDEX_POS 1000

struct state_uniform_info {
   const char *Name;
   gl_state_index StateTokens[2];
   struct field_info Fields[MAX_FIELDS];
};



static const struct state_uniform_info Uniforms[] = {
   { "gl_ModelViewMatrix", { STATE_MATRIX, STATE_MODELVIEW },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },
   { "gl_ProjectionMatrix", { STATE_MATRIX, STATE_PROJECTION },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },
   { "gl_ModelViewProjectionMatrix", { STATE_MATRIX, STATE_MVP },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },
   { "gl_NormalMatrix", { STATE_MATRIX, STATE_MODELVIEW },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },
   { "gl_TextureMatrix", { STATE_MATRIX, STATE_TEXTURE },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },

   { "gl_ClipPlane", { STATE_CLIPPLANE, INDEX_POS },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD}
   },

   { "gl_DepthRange", { STATE_DEPTH_RANGE, 0 },
     {
        { "near", 0, -1, SWIZZLE_XXXX },
        { "far", 0, -1, SWIZZLE_YYYY },
        { "diff", 0, -1, SWIZZLE_ZZZZ },
        MT_FIELD,
        MT_FIELD
     }
   },

   { "gl_Fog", { 0, 0 },
     {
        { "color", STATE_FOG_COLOR, 0, SWIZZLE_NOOP },
        { "density", STATE_FOG_PARAMS, 0, SWIZZLE_XXXX },
        { "start", STATE_FOG_PARAMS, 0, SWIZZLE_YYYY },
        { "end", STATE_FOG_PARAMS, 0, SWIZZLE_ZZZZ },
        { "scale", STATE_FOG_PARAMS, 0, SWIZZLE_WWWW }
     }
   },

   { NULL, { 0, 0 },
     { MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD, MT_FIELD }
   }
};


static GLint
lookup_statevar(const char *var, GLint index, const char *field,
                GLuint *swizzleOut,
                struct gl_program_parameter_list *paramList)
{
   gl_state_index tokens[STATE_LENGTH];
   GLuint i, j;
   GLint pos;

   for (i = 0; i < STATE_LENGTH; i++) {
      tokens[i] = 0;
   }

   for (i = 0; Uniforms[i].Name; i++) {
      if (strcmp(var, Uniforms[i].Name) == 0) {
         /* found the uniform */

         for (j = 0; j < 2; j++) {
            tokens[j] = Uniforms[i].StateTokens[j];
            if (tokens[j] == INDEX_POS) {
               /* replace INDEX_POS with actual array index */
               assert(index >= 0);
               tokens[j] = index;
            }
         }

         if (field) {
            /* extra work for var.field */
            for (j = 0; j < MAX_FIELDS; j++) {
               if (!Uniforms[i].Fields[j].Name) {
                  /* field not found! */
                  _mesa_problem(NULL, "field not found");
                  return -1;
               }
               else if (strcmp(field, Uniforms[i].Fields[j].Name) == 0) {
                  /* found the field */
                  GLint tokenPos = Uniforms[i].Fields[j].TokenPos;
                  if (tokenPos>= 0) {
                     tokens[tokenPos] = Uniforms[i].Fields[j].Token;
                  }
                  *swizzleOut = Uniforms[i].Fields[j].Swizzle;
                  break;
               }

            }
         }

         if (tokens[0] == STATE_MATRIX) {
            /* a matrix */
            GLuint j;
            GLint pos[4];
            gl_state_index indexesCopy[STATE_LENGTH];
            /* make copy of state tokens */
            for (j = 0; j < STATE_LENGTH; j++)
               indexesCopy[j] = tokens[j];
            /* load rows */
            for (j = 0; j < 4/*state[i].NumRows*/; j++) {
               indexesCopy[3] = indexesCopy[4] = j; /* jth row of matrix */
               pos[j] = _mesa_add_state_reference(paramList, (GLint*) indexesCopy);
               assert(pos[j] >= 0);
            }
            return pos[0] + index;
         }

         pos = _mesa_add_state_reference(paramList, (GLint *) tokens);
         assert(pos >= 0);
         return pos;
      }
   }
   return -1;
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
 *   var[index]
 *   var.field
 *   var[index].field
 */
GLint
_slang_alloc_statevar(slang_ir_node *n,
                      struct gl_program_parameter_list *paramList)
{
   const char *field = NULL, *var;
   GLint index = -1, pos;
   GLuint swizzle;

   if (n->Opcode == IR_FIELD) {
      field = n->Target;
      n = n->Children[0];
   }

   if (n->Opcode == IR_ELEMENT) {
      /* XXX can only handle constant indexes for now */
      assert(n->Children[1]->Opcode == IR_FLOAT);
      index = (GLint) n->Children[1]->Value[0];
      n = n->Children[0];
   }

   assert(n->Opcode == IR_VAR);
   var = (char *) n->Var->a_name;

   pos = lookup_statevar(var, index, field, &swizzle, paramList);
   assert(pos >= 0);
   if (pos >= 0) {
      n->Store->Index = pos;
      n->Store->Swizzle = swizzle;
   }
   return pos;
}

