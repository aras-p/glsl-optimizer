/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
 * Copyright © 2010 Intel Corporation
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
 * \file uniforms.c
 * Functions related to GLSL uniform variables.
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 * 2. Insert FLUSH_VERTICES calls in various places
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/dispatch.h"
#include "main/image.h"
#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "program/prog_uniform.h"
#include "program/prog_instruction.h"


static GLenum
base_uniform_type(GLenum type)
{
   switch (type) {
#if 0 /* not needed, for now */
   case GL_BOOL:
   case GL_BOOL_VEC2:
   case GL_BOOL_VEC3:
   case GL_BOOL_VEC4:
      return GL_BOOL;
#endif
   case GL_FLOAT:
   case GL_FLOAT_VEC2:
   case GL_FLOAT_VEC3:
   case GL_FLOAT_VEC4:
      return GL_FLOAT;
   case GL_UNSIGNED_INT:
   case GL_UNSIGNED_INT_VEC2:
   case GL_UNSIGNED_INT_VEC3:
   case GL_UNSIGNED_INT_VEC4:
      return GL_UNSIGNED_INT;
   case GL_INT:
   case GL_INT_VEC2:
   case GL_INT_VEC3:
   case GL_INT_VEC4:
      return GL_INT;
   default:
      _mesa_problem(NULL, "Invalid type in base_uniform_type()");
      return GL_FLOAT;
   }
}


static GLboolean
is_boolean_type(GLenum type)
{
   switch (type) {
   case GL_BOOL:
   case GL_BOOL_VEC2:
   case GL_BOOL_VEC3:
   case GL_BOOL_VEC4:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


static GLboolean
is_sampler_type(GLenum type)
{
   switch (type) {
   case GL_SAMPLER_1D:
   case GL_SAMPLER_2D:
   case GL_SAMPLER_3D:
   case GL_SAMPLER_CUBE:
   case GL_SAMPLER_1D_SHADOW:
   case GL_SAMPLER_2D_SHADOW:
   case GL_SAMPLER_CUBE_SHADOW:
   case GL_SAMPLER_2D_RECT_ARB:
   case GL_SAMPLER_2D_RECT_SHADOW_ARB:
   case GL_SAMPLER_1D_ARRAY_EXT:
   case GL_SAMPLER_2D_ARRAY_EXT:
   case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_CUBE_MAP_ARRAY:
   case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
   case GL_SAMPLER_BUFFER:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Given a uniform index, return the vertex/geometry/fragment program
 * that has that parameter, plus the position of the parameter in the
 * parameter/constant buffer.
 * \param shProg  the shader program
 * \param index  the uniform index in [0, NumUniforms-1]
 * \param progOut  returns containing program
 * \param posOut  returns position of the uniform in the param/const buffer
 * \return GL_TRUE for success, GL_FALSE for invalid index
 */
static GLboolean
find_uniform_parameter_pos(struct gl_shader_program *shProg, GLint index,
                           struct gl_program **progOut, GLint *posOut)
{
   struct gl_program *prog = NULL;
   GLint pos;

   if (!shProg->Uniforms ||
       index < 0 ||
       index >= (GLint) shProg->Uniforms->NumUniforms) {
      return GL_FALSE;
   }

   pos = shProg->Uniforms->Uniforms[index].VertPos;
   if (pos >= 0) {
      prog = &shProg->VertexProgram->Base;
   }
   else {
      pos = shProg->Uniforms->Uniforms[index].FragPos;
      if (pos >= 0) {
         prog = &shProg->FragmentProgram->Base;
      }
      else {
         pos = shProg->Uniforms->Uniforms[index].GeomPos;
         if (pos >= 0) {
            prog = &shProg->GeometryProgram->Base;
         }
      }
   }

   if (!prog || pos < 0)
      return GL_FALSE; /* should really never happen */

   *progOut = prog;
   *posOut = pos;

   return GL_TRUE;
}


/**
 * Return pointer to a gl_program_parameter which corresponds to a uniform.
 * \param shProg  the shader program
 * \param index  the uniform index in [0, NumUniforms-1]
 * \return gl_program_parameter point or NULL if index is invalid
 */
static const struct gl_program_parameter *
get_uniform_parameter(struct gl_shader_program *shProg, GLint index)
{
   struct gl_program *prog;
   GLint progPos;

   if (find_uniform_parameter_pos(shProg, index, &prog, &progPos))
      return &prog->Parameters->Parameters[progPos];
   else
      return NULL;
}


/**
 * Called by glGetActiveUniform().
 */
static void
_mesa_get_active_uniform(struct gl_context *ctx, GLuint program, GLuint index,
                         GLsizei maxLength, GLsizei *length, GLint *size,
                         GLenum *type, GLchar *nameOut)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetActiveUniform");
   const struct gl_program_parameter *param;

   if (!shProg)
      return;

   if (!shProg->Uniforms || index >= shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   param = get_uniform_parameter(shProg, index);
   if (!param)
      return;

   if (nameOut) {
      _mesa_copy_string(nameOut, maxLength, length, param->Name);
   }

   if (size) {
      GLint typeSize = _mesa_sizeof_glsl_type(param->DataType);
      if ((GLint) param->Size > typeSize) {
         /* This is an array.
          * Array elements are placed on vector[4] boundaries so they're
          * a multiple of four floats.  We round typeSize up to next multiple
          * of four to get the right size below.
          */
         typeSize = (typeSize + 3) & ~3;
      }
      /* Note that the returned size is in units of the <type>, not bytes */
      *size = param->Size / typeSize;
   }

   if (type) {
      *type = param->DataType;
   }
}


static unsigned
get_vector_elements(GLenum type)
{
   switch (type) {
   case GL_FLOAT:
   case GL_INT:
   case GL_BOOL:
   case GL_UNSIGNED_INT:
   default: /* Catch all the various sampler types. */
      return 1;

   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
   case GL_BOOL_VEC2:
   case GL_UNSIGNED_INT_VEC2:
      return 2;

   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
   case GL_BOOL_VEC3:
   case GL_UNSIGNED_INT_VEC3:
      return 3;

   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
   case GL_BOOL_VEC4:
   case GL_UNSIGNED_INT_VEC4:
      return 4;
   }
}

static void
get_matrix_dims(GLenum type, GLint *rows, GLint *cols)
{
   switch (type) {
   case GL_FLOAT_MAT2:
      *rows = *cols = 2;
      break;
   case GL_FLOAT_MAT2x3:
      *rows = 3;
      *cols = 2;
      break;
   case GL_FLOAT_MAT2x4:
      *rows = 4;
      *cols = 2;
      break;
   case GL_FLOAT_MAT3:
      *rows = 3;
      *cols = 3;
      break;
   case GL_FLOAT_MAT3x2:
      *rows = 2;
      *cols = 3;
      break;
   case GL_FLOAT_MAT3x4:
      *rows = 4;
      *cols = 3;
      break;
   case GL_FLOAT_MAT4:
      *rows = 4;
      *cols = 4;
      break;
   case GL_FLOAT_MAT4x2:
      *rows = 2;
      *cols = 4;
      break;
   case GL_FLOAT_MAT4x3:
      *rows = 3;
      *cols = 4;
      break;
   default:
      *rows = *cols = 0;
   }
}


/**
 * Determine the number of rows and columns occupied by a uniform
 * according to its datatype.  For non-matrix types (such as GL_FLOAT_VEC4),
 * the number of rows = 1 and cols = number of elements in the vector.
 */
static void
get_uniform_rows_cols(const struct gl_program_parameter *p,
                      GLint *rows, GLint *cols)
{
   get_matrix_dims(p->DataType, rows, cols);
   if (*rows == 0 && *cols == 0) {
      /* not a matrix type, probably a float or vector */
      *rows = 1;
      *cols = get_vector_elements(p->DataType);
   }
}


/**
 * GLSL uniform arrays and structs require special handling.
 *
 * The GL_ARB_shader_objects spec says that if you use
 * glGetUniformLocation to get the location of an array, you CANNOT
 * access other elements of the array by adding an offset to the
 * returned location.  For example, you must call
 * glGetUniformLocation("foo[16]") if you want to set the 16th element
 * of the array with glUniform().
 *
 * HOWEVER, some other OpenGL drivers allow accessing array elements
 * by adding an offset to the returned array location.  And some apps
 * seem to depend on that behaviour.
 *
 * Mesa's gl_uniform_list doesn't directly support this since each
 * entry in the list describes one uniform variable, not one uniform
 * element.  We could insert dummy entries in the list for each array
 * element after [0] but that causes complications elsewhere.
 *
 * We solve this problem by encoding two values in the location that's
 * returned by glGetUniformLocation():
 *  a) index into gl_uniform_list::Uniforms[] for the uniform
 *  b) an array/field offset (0 for simple types)
 *
 * These two values are encoded in the high and low halves of a GLint.
 * By putting the uniform number in the high part and the offset in the
 * low part, we can support the unofficial ability to index into arrays
 * by adding offsets to the location value.
 */
static void
merge_location_offset(GLint *location, GLint offset)
{
   *location = (*location << 16) | offset;
}


/**
 * Separate the uniform location and parameter offset.  See above.
 */
static void
split_location_offset(GLint *location, GLint *offset)
{
   *offset = *location & 0xffff;
   *location = *location >> 16;
}



/**
 * Called via glGetUniform[fiui]v() to get the current value of a uniform.
 */
static void
get_uniform(struct gl_context *ctx, GLuint program, GLint location,
            GLsizei bufSize, GLenum returnType, GLvoid *paramsOut)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetUniformfv");
   struct gl_program *prog;
   GLint paramPos, offset;

   if (!shProg)
      return;

   split_location_offset(&location, &offset);

   if (!find_uniform_parameter_pos(shProg, location, &prog, &paramPos)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,  "glGetUniformfv(location)");
   }
   else {
      const struct gl_program_parameter *p =
         &prog->Parameters->Parameters[paramPos];
      GLint rows, cols, i, j, k;
      GLsizei numBytes;

      get_uniform_rows_cols(p, &rows, &cols);

      numBytes = rows * cols * _mesa_sizeof_type(returnType);
      if (bufSize < numBytes) {
         _mesa_error( ctx, GL_INVALID_OPERATION,
                     "glGetnUniformfvARB(out of bounds: bufSize is %d,"
                     " but %d bytes are required)", bufSize, numBytes );
         return;
      }

      switch (returnType) {
      case GL_FLOAT:
         {
            GLfloat *params = (GLfloat *) paramsOut;
            k = 0;
            for (i = 0; i < rows; i++) {
               const int base = paramPos + offset + i;
               for (j = 0; j < cols; j++ ) {
                  params[k++] = prog->Parameters->ParameterValues[base][j];
               }
            }
         }
         break;
      case GL_DOUBLE:
         {
            GLfloat *params = (GLfloat *) paramsOut;
            k = 0;
            for (i = 0; i < rows; i++) {
               const int base = paramPos + offset + i;
               for (j = 0; j < cols; j++ ) {
                  params[k++] = (GLdouble)
                     prog->Parameters->ParameterValues[base][j];
               }
            }
         }
         break;
      case GL_INT:
         {
            GLint *params = (GLint *) paramsOut;
            k = 0;
            for (i = 0; i < rows; i++) {
               const int base = paramPos + offset + i;
               for (j = 0; j < cols; j++ ) {
                  params[k++] = (GLint)
                     prog->Parameters->ParameterValues[base][j];
               }
            }
         }
         break;
      case GL_UNSIGNED_INT:
         {
            GLuint *params = (GLuint *) paramsOut;
            k = 0;
            for (i = 0; i < rows; i++) {
               const int base = paramPos + offset + i;
               for (j = 0; j < cols; j++ ) {
                  params[k++] = (GLuint)
                     prog->Parameters->ParameterValues[base][j];
               }
            }
         }
         break;
      default:
         _mesa_problem(ctx, "bad returnType in get_uniform()");
      }
   }
}


/**
 * Called via glGetUniformLocation().
 *
 * The return value will encode two values, the uniform location and an
 * offset (used for arrays, structs).
 */
GLint
_mesa_get_uniform_location(struct gl_context *ctx,
                           struct gl_shader_program *shProg,
			   const GLchar *name)
{
   GLint offset = 0, location = -1;

   if (shProg->LinkStatus == GL_FALSE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformfv(program)");
      return -1;
   }

   /* XXX we should return -1 if the uniform was declared, but not
    * actually used.
    */

   /* XXX we need to be able to parse uniform names for structs and arrays
    * such as:
    *   mymatrix[1]
    *   mystruct.field1
    */

   {
      /* handle 1-dimension arrays here... */
      char *c = strchr(name, '[');
      if (c) {
         /* truncate name at [ */
         const GLint len = c - name;
         GLchar *newName = malloc(len + 1);
         if (!newName)
            return -1; /* out of mem */
         memcpy(newName, name, len);
         newName[len] = 0;

         location = _mesa_lookup_uniform(shProg->Uniforms, newName);
         if (location >= 0) {
            const GLint element = atoi(c + 1);
            if (element > 0) {
               /* get type of the uniform array element */
               const struct gl_program_parameter *p =
                  get_uniform_parameter(shProg, location);
               if (p) {
                  GLint rows, cols;
                  get_matrix_dims(p->DataType, &rows, &cols);
                  if (rows < 1)
                     rows = 1;
                  offset = element * rows;
               }
            }
         }

         free(newName);
      }
   }

   if (location < 0) {
      location = _mesa_lookup_uniform(shProg->Uniforms, name);
   }

   if (location >= 0) {
      merge_location_offset(&location, offset);
   }

   return location;
}



/**
 * Update the vertex/fragment program's TexturesUsed array.
 *
 * This needs to be called after glUniform(set sampler var) is called.
 * A call to glUniform(samplerVar, value) causes a sampler to point to a
 * particular texture unit.  We know the sampler's texture target
 * (1D/2D/3D/etc) from compile time but the sampler's texture unit is
 * set by glUniform() calls.
 *
 * So, scan the program->SamplerUnits[] and program->SamplerTargets[]
 * information to update the prog->TexturesUsed[] values.
 * Each value of TexturesUsed[unit] is one of zero, TEXTURE_1D_INDEX,
 * TEXTURE_2D_INDEX, TEXTURE_3D_INDEX, etc.
 * We'll use that info for state validation before rendering.
 */
void
_mesa_update_shader_textures_used(struct gl_program *prog)
{
   GLuint s;

   memset(prog->TexturesUsed, 0, sizeof(prog->TexturesUsed));

   for (s = 0; s < MAX_SAMPLERS; s++) {
      if (prog->SamplersUsed & (1 << s)) {
         GLuint unit = prog->SamplerUnits[s];
         GLuint tgt = prog->SamplerTargets[s];
         assert(unit < MAX_TEXTURE_IMAGE_UNITS);
         assert(tgt < NUM_TEXTURE_TARGETS);
         prog->TexturesUsed[unit] |= (1 << tgt);
      }
   }
}


/**
 * Check if the type given by userType is allowed to set a uniform of the
 * target type.  Generally, equivalence is required, but setting Boolean
 * uniforms can be done with glUniformiv or glUniformfv.
 */
static GLboolean
compatible_types(GLenum userType, GLenum targetType)
{
   if (userType == targetType)
      return GL_TRUE;

   if (targetType == GL_BOOL && (userType == GL_FLOAT ||
                                 userType == GL_UNSIGNED_INT ||
                                 userType == GL_INT))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC2 && (userType == GL_FLOAT_VEC2 ||
                                      userType == GL_UNSIGNED_INT_VEC2 ||
                                      userType == GL_INT_VEC2))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC3 && (userType == GL_FLOAT_VEC3 ||
                                      userType == GL_UNSIGNED_INT_VEC3 ||
                                      userType == GL_INT_VEC3))
      return GL_TRUE;

   if (targetType == GL_BOOL_VEC4 && (userType == GL_FLOAT_VEC4 ||
                                      userType == GL_UNSIGNED_INT_VEC4 ||
                                      userType == GL_INT_VEC4))
      return GL_TRUE;

   if (is_sampler_type(targetType) && userType == GL_INT)
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Set the value of a program's uniform variable.
 * \param program  the program whose uniform to update
 * \param index  the index of the program parameter for the uniform
 * \param offset  additional parameter slot offset (for arrays)
 * \param type  the incoming datatype of 'values'
 * \param count  the number of uniforms to set
 * \param elems  number of elements per uniform (1, 2, 3 or 4)
 * \param values  the new values, of datatype 'type'
 */
static void
set_program_uniform(struct gl_context *ctx, struct gl_program *program,
                    GLint index, GLint offset,
                    GLenum type, GLsizei count, GLint elems,
                    const void *values)
{
   const struct gl_program_parameter *param =
      &program->Parameters->Parameters[index];

   assert(offset >= 0);
   assert(elems >= 1);
   assert(elems <= 4);

   if (!compatible_types(type, param->DataType)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(type mismatch)");
      return;
   }

   if (index + offset > (GLint) program->Parameters->Size) {
      /* out of bounds! */
      return;
   }

   if (param->Type == PROGRAM_SAMPLER) {
      /* This controls which texture unit which is used by a sampler */
      GLboolean changed = GL_FALSE;
      GLint i;

      /* this should have been caught by the compatible_types() check */
      ASSERT(type == GL_INT);

      /* loop over number of samplers to change */
      for (i = 0; i < count; i++) {
         GLuint sampler = (GLuint)
            program->Parameters->ParameterValues[index + offset + i][0];
         GLuint texUnit = ((GLuint *) values)[i];

         /* check that the sampler (tex unit index) is legal */
         if (texUnit >= ctx->Const.MaxTextureImageUnits) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glUniform1(invalid sampler/tex unit index for '%s')",
                        param->Name);
            return;
         }

         /* This maps a sampler to a texture unit: */
         if (sampler < MAX_SAMPLERS) {
#if 0
            printf("Set program %p sampler %d '%s' to unit %u\n",
		   program, sampler, param->Name, texUnit);
#endif
            if (program->SamplerUnits[sampler] != texUnit) {
               program->SamplerUnits[sampler] = texUnit;
               changed = GL_TRUE;
            }
         }
      }

      if (changed) {
         /* When a sampler's value changes it usually requires rewriting
          * a GPU program's TEX instructions since there may not be a
          * sampler->texture lookup table.  We signal this with the
          * ProgramStringNotify() callback.
          */
         FLUSH_VERTICES(ctx, _NEW_TEXTURE | _NEW_PROGRAM);
         _mesa_update_shader_textures_used(program);
         /* Do we need to care about the return value here?
          * This should not be the first time the driver was notified of
          * this program.
          */
         (void) ctx->Driver.ProgramStringNotify(ctx, program->Target, program);
      }
   }
   else {
      /* ordinary uniform variable */
      const GLboolean isUniformBool = is_boolean_type(param->DataType);
      const GLenum basicType = base_uniform_type(type);
      const GLint slots = (param->Size + 3) / 4;
      const GLint typeSize = _mesa_sizeof_glsl_type(param->DataType);
      GLsizei k, i;

      if ((GLint) param->Size > typeSize) {
         /* an array */
         /* we'll ignore extra data below */
      }
      else {
         /* non-array: count must be at most one; count == 0 is handled
          * by the loop below
          */
         if (count > 1) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glUniform(uniform '%s' is not an array)",
                        param->Name);
            return;
         }
      }

      /* loop over number of array elements */
      for (k = 0; k < count; k++) {
         GLfloat *uniformVal;

         if (offset + k >= slots) {
            /* Extra array data is ignored */
            break;
         }

         /* uniformVal (the destination) is always float[4] */
         uniformVal = program->Parameters->ParameterValues[index + offset + k];

         if (basicType == GL_INT) {
            /* convert user's ints to floats */
            const GLint *iValues = ((const GLint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = (GLfloat) iValues[i];
            }
         }
         else if (basicType == GL_UNSIGNED_INT) {
            /* convert user's uints to floats */
            const GLuint *iValues = ((const GLuint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = (GLfloat) iValues[i];
            }
         }
         else {
            const GLfloat *fValues = ((const GLfloat *) values) + k * elems;
            assert(basicType == GL_FLOAT);
            for (i = 0; i < elems; i++) {
               uniformVal[i] = fValues[i];
            }
         }

         /* if the uniform is bool-valued, convert to 1.0 or 0.0 */
         if (isUniformBool) {
            for (i = 0; i < elems; i++) {
               uniformVal[i] = uniformVal[i] ? 1.0f : 0.0f;
            }
         }
      }
   }
}


/**
 * Called via glUniform*() functions.
 */
void
_mesa_uniform(struct gl_context *ctx, struct gl_shader_program *shProg,
	      GLint location, GLsizei count,
              const GLvoid *values, GLenum type)
{
   struct gl_uniform *uniform;
   GLint elems, offset;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(program not linked)");
      return;
   }

   if (location == -1)
      return;   /* The standard specifies this as a no-op */

   if (location < -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(location=%d)",
                  location);
      return;
   }

   split_location_offset(&location, &offset);

   if (location < 0 || location >= (GLint) shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(location=%d)", location);
      return;
   }

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(count < 0)");
      return;
   }

   elems = _mesa_sizeof_glsl_type(type);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM_CONSTANTS);

   uniform = &shProg->Uniforms->Uniforms[location];

   if (ctx->Shader.Flags & GLSL_UNIFORMS) {
      const GLenum basicType = base_uniform_type(type);
      GLint i;
      printf("Mesa: set program %u uniform %s (loc %d) to: ",
	     shProg->Name, uniform->Name, location);
      if (basicType == GL_INT) {
         const GLint *v = (const GLint *) values;
         for (i = 0; i < count * elems; i++) {
            printf("%d ", v[i]);
         }
      }
      else if (basicType == GL_UNSIGNED_INT) {
         const GLuint *v = (const GLuint *) values;
         for (i = 0; i < count * elems; i++) {
            printf("%u ", v[i]);
         }
      }
      else {
         const GLfloat *v = (const GLfloat *) values;
         assert(basicType == GL_FLOAT);
         for (i = 0; i < count * elems; i++) {
            printf("%g ", v[i]);
         }
      }
      printf("\n");
   }

   /* A uniform var may be used by both a vertex shader and a fragment
    * shader.  We may need to update one or both shader's uniform here:
    */
   if (shProg->VertexProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform(ctx, &shProg->VertexProgram->Base,
                             index, offset, type, count, elems, values);
      }
   }

   if (shProg->FragmentProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform(ctx, &shProg->FragmentProgram->Base,
                             index, offset, type, count, elems, values);
      }
   }

   if (shProg->GeometryProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->GeomPos;
      if (index >= 0) {
         set_program_uniform(ctx, &shProg->GeometryProgram->Base,
                             index, offset, type, count, elems, values);
      }
   }

   uniform->Initialized = GL_TRUE;
}


/**
 * Set a matrix-valued program parameter.
 */
static void
set_program_uniform_matrix(struct gl_context *ctx, struct gl_program *program,
                           GLuint index, GLuint offset,
                           GLuint count, GLuint rows, GLuint cols,
                           GLboolean transpose, const GLfloat *values)
{
   GLuint mat, row, col;
   GLuint src = 0;
   const struct gl_program_parameter *param =
      &program->Parameters->Parameters[index];
   const GLuint slots = (param->Size + 3) / 4;
   const GLint typeSize = _mesa_sizeof_glsl_type(param->DataType);
   GLint nr, nc;

   /* check that the number of rows, columns is correct */
   get_matrix_dims(param->DataType, &nr, &nc);
   if (rows != nr || cols != nc) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glUniformMatrix(matrix size mismatch)");
      return;
   }

   if ((GLint) param->Size <= typeSize) {
      /* non-array: count must be at most one; count == 0 is handled
       * by the loop below
       */
      if (count > 1) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUniformMatrix(uniform is not an array)");
         return;
      }
   }

   /*
    * Note: the _columns_ of a matrix are stored in program registers, not
    * the rows.  So, the loops below look a little funny.
    * XXX could optimize this a bit...
    */

   /* loop over matrices */
   for (mat = 0; mat < count; mat++) {

      /* each matrix: */
      for (col = 0; col < cols; col++) {
         GLfloat *v;
         if (offset >= slots) {
            /* Ignore writes beyond the end of (the used part of) an array */
            return;
         }
         v = program->Parameters->ParameterValues[index + offset];
         for (row = 0; row < rows; row++) {
            if (transpose) {
               v[row] = values[src + row * cols + col];
            }
            else {
               v[row] = values[src + col * rows + row];
            }
         }

         offset++;
      }

      src += rows * cols;  /* next matrix */
   }
}


/**
 * Called by glUniformMatrix*() functions.
 * Note: cols=2, rows=4  ==>  array[2] of vec4
 */
void
_mesa_uniform_matrix(struct gl_context *ctx, struct gl_shader_program *shProg,
		     GLint cols, GLint rows,
                     GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values)
{
   struct gl_uniform *uniform;
   GLint offset;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         "glUniformMatrix(program not linked)");
      return;
   }

   if (location == -1)
      return;   /* The standard specifies this as a no-op */

   if (location < -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniformMatrix(location)");
      return;
   }

   split_location_offset(&location, &offset);

   if (location < 0 || location >= (GLint) shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix(location)");
      return;
   }
   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM_CONSTANTS);

   uniform = &shProg->Uniforms->Uniforms[location];

   if (shProg->VertexProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx, &shProg->VertexProgram->Base,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   if (shProg->FragmentProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx, &shProg->FragmentProgram->Base,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   if (shProg->GeometryProgram) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->GeomPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx, &shProg->GeometryProgram->Base,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   uniform->Initialized = GL_TRUE;
}


void GLAPIENTRY
_mesa_Uniform1fARB(GLint location, GLfloat v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                   GLfloat v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1iARB(GLint location, GLint v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2iARB(GLint location, GLint v0, GLint v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC4);
}


/** OpenGL 3.0 GLuint-valued functions **/
void GLAPIENTRY
_mesa_Uniform1ui(GLint location, GLuint v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_UNSIGNED_INT);
}

void GLAPIENTRY
_mesa_Uniform2ui(GLint location, GLuint v0, GLuint v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT);
}

void GLAPIENTRY
_mesa_Uniform2uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC4);
}



void GLAPIENTRY
_mesa_UniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 3, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 4, location, count, transpose, value);
}


/**
 * Non-square UniformMatrix are OpenGL 2.1
 */
void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 3, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 4, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 4, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 3, location, count, transpose, value);
}


void GLAPIENTRY
_mesa_GetnUniformfvARB(GLhandleARB program, GLint location,
                       GLsizei bufSize, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   get_uniform(ctx, program, location, bufSize, GL_FLOAT, params);
}

void GLAPIENTRY
_mesa_GetUniformfvARB(GLhandleARB program, GLint location, GLfloat *params)
{
   _mesa_GetnUniformfvARB(program, location, INT_MAX, params);
}


void GLAPIENTRY
_mesa_GetnUniformivARB(GLhandleARB program, GLint location,
                       GLsizei bufSize, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   get_uniform(ctx, program, location, bufSize, GL_INT, params);
}

void GLAPIENTRY
_mesa_GetUniformivARB(GLhandleARB program, GLint location, GLint *params)
{
   _mesa_GetnUniformivARB(program, location, INT_MAX, params);
}


/* GL3 */
void GLAPIENTRY
_mesa_GetnUniformuivARB(GLhandleARB program, GLint location,
                        GLsizei bufSize, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   get_uniform(ctx, program, location, bufSize, GL_UNSIGNED_INT, params);
}

void GLAPIENTRY
_mesa_GetUniformuiv(GLhandleARB program, GLint location, GLuint *params)
{
   _mesa_GetnUniformuivARB(program, location, INT_MAX, params);
}


/* GL4 */
void GLAPIENTRY
_mesa_GetnUniformdvARB(GLhandleARB program, GLint location,
                        GLsizei bufSize, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   /*
   get_uniform(ctx, program, location, bufSize, GL_DOUBLE, params);
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformdvARB"
               "(GL_ARB_gpu_shader_fp64 not implemented)");
}

void GLAPIENTRY
_mesa_GetUniformdv(GLhandleARB program, GLint location, GLdouble *params)
{
   _mesa_GetnUniformdvARB(program, location, INT_MAX, params);
}


GLint GLAPIENTRY
_mesa_GetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
   struct gl_shader_program *shProg;

   GET_CURRENT_CONTEXT(ctx);

   shProg = _mesa_lookup_shader_program_err(ctx, programObj,
					    "glGetUniformLocation");
   if (!shProg)
      return -1;

   return _mesa_get_uniform_location(ctx, shProg, name);
}


void GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB program, GLuint index,
                          GLsizei maxLength, GLsizei * length, GLint * size,
                          GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_get_active_uniform(ctx, program, index, maxLength, length, size,
                            type, name);
}


/**
 * Plug in shader uniform-related functions into API dispatch table.
 */
void
_mesa_init_shader_uniform_dispatch(struct _glapi_table *exec)
{
#if FEATURE_GL
   SET_Uniform1fARB(exec, _mesa_Uniform1fARB);
   SET_Uniform2fARB(exec, _mesa_Uniform2fARB);
   SET_Uniform3fARB(exec, _mesa_Uniform3fARB);
   SET_Uniform4fARB(exec, _mesa_Uniform4fARB);
   SET_Uniform1iARB(exec, _mesa_Uniform1iARB);
   SET_Uniform2iARB(exec, _mesa_Uniform2iARB);
   SET_Uniform3iARB(exec, _mesa_Uniform3iARB);
   SET_Uniform4iARB(exec, _mesa_Uniform4iARB);
   SET_Uniform1fvARB(exec, _mesa_Uniform1fvARB);
   SET_Uniform2fvARB(exec, _mesa_Uniform2fvARB);
   SET_Uniform3fvARB(exec, _mesa_Uniform3fvARB);
   SET_Uniform4fvARB(exec, _mesa_Uniform4fvARB);
   SET_Uniform1ivARB(exec, _mesa_Uniform1ivARB);
   SET_Uniform2ivARB(exec, _mesa_Uniform2ivARB);
   SET_Uniform3ivARB(exec, _mesa_Uniform3ivARB);
   SET_Uniform4ivARB(exec, _mesa_Uniform4ivARB);
   SET_UniformMatrix2fvARB(exec, _mesa_UniformMatrix2fvARB);
   SET_UniformMatrix3fvARB(exec, _mesa_UniformMatrix3fvARB);
   SET_UniformMatrix4fvARB(exec, _mesa_UniformMatrix4fvARB);

   SET_GetActiveUniformARB(exec, _mesa_GetActiveUniformARB);
   SET_GetUniformLocationARB(exec, _mesa_GetUniformLocationARB);
   SET_GetUniformfvARB(exec, _mesa_GetUniformfvARB);
   SET_GetUniformivARB(exec, _mesa_GetUniformivARB);

   /* OpenGL 2.1 */
   SET_UniformMatrix2x3fv(exec, _mesa_UniformMatrix2x3fv);
   SET_UniformMatrix3x2fv(exec, _mesa_UniformMatrix3x2fv);
   SET_UniformMatrix2x4fv(exec, _mesa_UniformMatrix2x4fv);
   SET_UniformMatrix4x2fv(exec, _mesa_UniformMatrix4x2fv);
   SET_UniformMatrix3x4fv(exec, _mesa_UniformMatrix3x4fv);
   SET_UniformMatrix4x3fv(exec, _mesa_UniformMatrix4x3fv);

   /* OpenGL 3.0 */
   SET_Uniform1uiEXT(exec, _mesa_Uniform1ui);
   SET_Uniform2uiEXT(exec, _mesa_Uniform2ui);
   SET_Uniform3uiEXT(exec, _mesa_Uniform3ui);
   SET_Uniform4uiEXT(exec, _mesa_Uniform4ui);
   SET_Uniform1uivEXT(exec, _mesa_Uniform1uiv);
   SET_Uniform2uivEXT(exec, _mesa_Uniform2uiv);
   SET_Uniform3uivEXT(exec, _mesa_Uniform3uiv);
   SET_Uniform4uivEXT(exec, _mesa_Uniform4uiv);
   SET_GetUniformuivEXT(exec, _mesa_GetUniformuiv);

   /* GL_ARB_robustness */
   SET_GetnUniformfvARB(exec, _mesa_GetnUniformfvARB);
   SET_GetnUniformivARB(exec, _mesa_GetnUniformivARB);
   SET_GetnUniformuivARB(exec, _mesa_GetnUniformuivARB);
   SET_GetnUniformdvARB(exec, _mesa_GetnUniformdvARB); /* GL 4.0 */

#endif /* FEATURE_GL */
}
