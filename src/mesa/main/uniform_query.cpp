/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
 * Copyright © 2010, 2011 Intel Corporation
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
#include "main/core.h"
#include "main/context.h"
#include "ir.h"
#include "ir_uniform.h"
#include "../glsl/program.h"
#include "../glsl/ir_uniform.h"

extern "C" {
#include "main/image.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "uniforms.h"
}

extern "C" void GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB program, GLuint index,
                          GLsizei maxLength, GLsizei *length, GLint *size,
                          GLenum *type, GLcharARB *nameOut)
{
   GET_CURRENT_CONTEXT(ctx);
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

   const struct gl_uniform *const uni = &shProg->Uniforms->Uniforms[index];

   if (nameOut) {
      _mesa_copy_string(nameOut, maxLength, length, param->Name);
   }

   if (size) {
      GLint typeSize = _mesa_sizeof_glsl_type(uni->Type->gl_type);
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
      *type = uni->Type->gl_type;
   }
}

static GLenum
base_uniform_type(GLenum type)
{
   switch (type) {
   case GL_BOOL:
   case GL_BOOL_VEC2:
   case GL_BOOL_VEC3:
   case GL_BOOL_VEC4:
      return GL_BOOL;
   case GL_FLOAT:
   case GL_FLOAT_VEC2:
   case GL_FLOAT_VEC3:
   case GL_FLOAT_VEC4:
   case GL_FLOAT_MAT2:
   case GL_FLOAT_MAT2x3:
   case GL_FLOAT_MAT2x4:
   case GL_FLOAT_MAT3x2:
   case GL_FLOAT_MAT3:
   case GL_FLOAT_MAT3x4:
   case GL_FLOAT_MAT4x2:
   case GL_FLOAT_MAT4x3:
   case GL_FLOAT_MAT4:
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
   case GL_INT_SAMPLER_1D:
   case GL_UNSIGNED_INT_SAMPLER_1D:
   case GL_SAMPLER_2D:
   case GL_INT_SAMPLER_2D:
   case GL_UNSIGNED_INT_SAMPLER_2D:
   case GL_SAMPLER_3D:
   case GL_INT_SAMPLER_3D:
   case GL_UNSIGNED_INT_SAMPLER_3D:
   case GL_SAMPLER_CUBE:
   case GL_INT_SAMPLER_CUBE:
   case GL_UNSIGNED_INT_SAMPLER_CUBE:
   case GL_SAMPLER_1D_SHADOW:
   case GL_SAMPLER_2D_SHADOW:
   case GL_SAMPLER_CUBE_SHADOW:
   case GL_SAMPLER_2D_RECT_ARB:
   case GL_INT_SAMPLER_2D_RECT:
   case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
   case GL_SAMPLER_2D_RECT_SHADOW_ARB:
   case GL_SAMPLER_1D_ARRAY_EXT:
   case GL_INT_SAMPLER_1D_ARRAY:
   case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
   case GL_SAMPLER_2D_ARRAY_EXT:
   case GL_INT_SAMPLER_2D_ARRAY:
   case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
   case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_CUBE_MAP_ARRAY:
   case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
   case GL_SAMPLER_BUFFER:
   case GL_INT_SAMPLER_BUFFER:
   case GL_UNSIGNED_INT_SAMPLER_BUFFER:
   case GL_SAMPLER_2D_MULTISAMPLE:
   case GL_INT_SAMPLER_2D_MULTISAMPLE:
   case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
   case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
   case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
   case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
   case GL_SAMPLER_EXTERNAL_OES:
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
      prog = shProg->_LinkedShaders[MESA_SHADER_VERTEX]->Program;
   }
   else {
      pos = shProg->Uniforms->Uniforms[index].FragPos;
      if (pos >= 0) {
         prog = shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;
      }
      else {
         pos = shProg->Uniforms->Uniforms[index].GeomPos;
         if (pos >= 0) {
            prog = shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program;
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
const struct gl_program_parameter *
get_uniform_parameter(struct gl_shader_program *shProg, GLint index)
{
   struct gl_program *prog;
   GLint progPos;

   if (find_uniform_parameter_pos(shProg, index, &prog, &progPos))
      return &prog->Parameters->Parameters[progPos];
   else
      return NULL;
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

static bool
validate_uniform_parameters(struct gl_context *ctx,
			    struct gl_shader_program *shProg,
			    GLint location, GLsizei count,
			    unsigned *loc,
			    unsigned *array_index,
			    const char *caller,
			    bool negative_one_is_not_valid)
{
   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(program not linked)", caller);
      return false;
   }

   if (location == -1) {
      /* For glGetUniform, page 264 (page 278 of the PDF) of the OpenGL 2.1
       * spec says:
       *
       *     "The error INVALID_OPERATION is generated if program has not been
       *     linked successfully, or if location is not a valid location for
       *     program."
       *
       * For glUniform, page 82 (page 96 of the PDF) of the OpenGL 2.1 spec
       * says:
       *
       *     "If the value of location is -1, the Uniform* commands will
       *     silently ignore the data passed in, and the current uniform
       *     values will not be changed."
       *
       * Allowing -1 for the location parameter of glUniform allows
       * applications to avoid error paths in the case that, for example, some
       * uniform variable is removed by the compiler / linker after
       * optimization.  In this case, the new value of the uniform is dropped
       * on the floor.  For the case of glGetUniform, there is nothing
       * sensible to do for a location of -1.
       *
       * The negative_one_is_not_valid flag selects between the two behaviors.
       */
      if (negative_one_is_not_valid) {
	 _mesa_error(ctx, GL_INVALID_OPERATION, "%s(location=%d)",
		     caller, location);
      }

      return false;
   }

   /* From page 12 (page 26 of the PDF) of the OpenGL 2.1 spec:
    *
    *     "If a negative number is provided where an argument of type sizei or
    *     sizeiptr is specified, the error INVALID_VALUE is generated."
    */
   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(count < 0)", caller);
      return false;
   }

   /* Page 82 (page 96 of the PDF) of the OpenGL 2.1 spec says:
    *
    *     "If any of the following conditions occur, an INVALID_OPERATION
    *     error is generated by the Uniform* commands, and no uniform values
    *     are changed:
    *
    *     ...
    *
    *         - if no variable with a location of location exists in the
    *           program object currently in use and location is not -1,
    */
   if (location < -1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(location=%d)",
                  caller, location);
      return false;
   }

   _mesa_uniform_split_location_offset(location, loc, array_index);

   if (*loc >= shProg->Uniforms->NumUniforms) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(location=%d)",
		  caller, location);
      return false;
   }

   return true;
}

/**
 * Called via glGetUniform[fiui]v() to get the current value of a uniform.
 */
extern "C" void
_mesa_get_uniform(struct gl_context *ctx, GLuint program, GLint location,
		  GLsizei bufSize, GLenum returnType, GLvoid *paramsOut)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetUniformfv");
   struct gl_program *prog;
   GLint paramPos;
   unsigned loc, offset;

   if (!validate_uniform_parameters(ctx, shProg, location, 1,
				    &loc, &offset, "glGetUniform", true))
      return;

   if (!find_uniform_parameter_pos(shProg, loc, &prog, &paramPos)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,  "glGetUniformfv(location)");
   }
   else {
      const struct gl_program_parameter *p =
         &prog->Parameters->Parameters[paramPos];
      gl_constant_value (*values)[4];
      GLint rows, cols, i, j, k;
      GLsizei numBytes;
      GLenum storage_type;

      values = prog->Parameters->ParameterValues + paramPos + offset;

      get_uniform_rows_cols(p, &rows, &cols);

      numBytes = rows * cols * _mesa_sizeof_type(returnType);
      if (bufSize < numBytes) {
         _mesa_error( ctx, GL_INVALID_OPERATION,
                     "glGetnUniformfvARB(out of bounds: bufSize is %d,"
                     " but %d bytes are required)", bufSize, numBytes );
         return;
      }

      if (ctx->Const.NativeIntegers) {
	 storage_type = base_uniform_type(p->DataType);
      } else {
	 storage_type = GL_FLOAT;
      }

      k = 0;
      for (i = 0; i < rows; i++) {
	 for (j = 0; j < cols; j++ ) {
	    void *out = (char *)paramsOut + 4 * k;

	    switch (returnType) {
	    case GL_FLOAT:
	       switch (storage_type) {
	       case GL_FLOAT:
		  *(float *)out = values[i][j].f;
		  break;
	       case GL_INT:
	       case GL_BOOL: /* boolean is just an integer 1 or 0. */
		  *(float *)out = values[i][j].i;
		  break;
	       case GL_UNSIGNED_INT:
		  *(float *)out = values[i][j].u;
		  break;
	       }
	       break;

	    case GL_INT:
	    case GL_UNSIGNED_INT:
	       switch (storage_type) {
	       case GL_FLOAT:
		  /* While the GL 3.2 core spec doesn't explicitly
		   * state how conversion of float uniforms to integer
		   * values works, in section 6.2 "State Tables" on
		   * page 267 it says:
		   *
		   *     "Unless otherwise specified, when floating
		   *      point state is returned as integer values or
		   *      integer state is returned as floating-point
		   *      values it is converted in the fashion
		   *      described in section 6.1.2"
		   *
		   * That section, on page 248, says:
		   *
		   *     "If GetIntegerv or GetInteger64v are called,
		   *      a floating-point value is rounded to the
		   *      nearest integer..."
		   */
		  *(int *)out = IROUND(values[i][j].f);
		  break;

	       case GL_INT:
	       case GL_UNSIGNED_INT:
	       case GL_BOOL:
		  /* type conversions for these to int/uint are just
		   * copying the data.
		   */
		  *(int *)out = values[i][j].i;
		  break;
		  break;
	       }
	       break;
	    }

	    k++;
	 }
      }
   }
}

static void
log_uniform(const void *values, enum glsl_base_type basicType,
	    unsigned rows, unsigned cols, unsigned count,
	    bool transpose,
	    const struct gl_shader_program *shProg,
	    GLint location,
	    const struct gl_uniform_storage *uni)
{

   const union gl_constant_value *v = (const union gl_constant_value *) values;
   const unsigned elems = rows * cols * count;
   const char *const extra = (cols == 1) ? "uniform" : "uniform matrix";

   printf("Mesa: set program %u %s \"%s\" (loc %d, type \"%s\", "
	  "transpose = %s) to: ",
	  shProg->Name, extra, uni->name, location, uni->type->name,
	  transpose ? "true" : "false");
   for (unsigned i = 0; i < elems; i++) {
      if (i != 0 && ((i % rows) == 0))
	 printf(", ");

      switch (basicType) {
      case GLSL_TYPE_UINT:
	 printf("%u ", v[i].u);
	 break;
      case GLSL_TYPE_INT:
	 printf("%d ", v[i].i);
	 break;
      case GLSL_TYPE_FLOAT:
	 printf("%g ", v[i].f);
	 break;
      default:
	 assert(!"Should not get here.");
	 break;
      }
   }
   printf("\n");
   fflush(stdout);
}

#if 0
static void
log_program_parameters(const struct gl_shader_program *shProg)
{
   static const char *stages[] = {
      "vertex", "fragment", "geometry"
   };

   assert(Elements(stages) == MESA_SHADER_TYPES);

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (shProg->_LinkedShaders[i] == NULL)
	 continue;

      const struct gl_program *const prog = shProg->_LinkedShaders[i]->Program;

      printf("Program %d %s shader parameters:\n",
	     shProg->Name, stages[i]);
      for (unsigned j = 0; j < prog->Parameters->NumParameters; j++) {
	 printf("%s: %p %f %f %f %f\n",
		prog->Parameters->Parameters[j].Name,
		prog->Parameters->ParameterValues[j],
		prog->Parameters->ParameterValues[j][0].f,
		prog->Parameters->ParameterValues[j][1].f,
		prog->Parameters->ParameterValues[j][2].f,
		prog->Parameters->ParameterValues[j][3].f);
      }
   }
   fflush(stdout);
}
#endif

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
            program->Parameters->ParameterValues[index+offset + i][0].f;
         GLuint texUnit = ((GLuint *) values)[i];

         /* check that the sampler (tex unit index) is legal */
         if (texUnit >= ctx->Const.MaxCombinedTextureImageUnits) {
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
         gl_constant_value *uniformVal;

         if (offset + k >= slots) {
            /* Extra array data is ignored */
            break;
         }

         /* uniformVal (the destination) is always gl_constant_value[4] */
         uniformVal = program->Parameters->ParameterValues[index + offset + k];

         if (basicType == GL_INT) {
            const GLint *iValues = ((const GLint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               if (!ctx->Const.NativeIntegers)
                  uniformVal[i].f = (GLfloat) iValues[i];
               else
                  uniformVal[i].i = iValues[i];
            }
         }
         else if (basicType == GL_UNSIGNED_INT) {
            const GLuint *iValues = ((const GLuint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               if (!ctx->Const.NativeIntegers)
                  uniformVal[i].f = (GLfloat)(GLuint) iValues[i];
               else
                  uniformVal[i].u = iValues[i];
            }
         }
         else {
            const GLfloat *fValues = ((const GLfloat *) values) + k * elems;
            assert(basicType == GL_FLOAT);
            for (i = 0; i < elems; i++) {
               uniformVal[i].f = fValues[i];
            }
         }

         /* if the uniform is bool-valued, convert to 1 or 0 */
         if (isUniformBool) {
            for (i = 0; i < elems; i++) {
               if (basicType == GL_FLOAT)
                  uniformVal[i].b = uniformVal[i].f != 0.0f ? 1 : 0;
               else
                  uniformVal[i].b = uniformVal[i].u ? 1 : 0;

               if (ctx->Const.NativeIntegers)
                  uniformVal[i].u =
                        uniformVal[i].b ? ctx->Const.UniformBooleanTrue : 0;
               else
                  uniformVal[i].f = uniformVal[i].b ? 1.0f : 0.0f;
            }
         }
      }
   }
}

/**
 * Propagate some values from uniform backing storage to driver storage
 *
 * Values propagated from uniform backing storage to driver storage
 * have all format / type conversions previously requested by the
 * driver applied.  This function is most often called by the
 * implementations of \c glUniform1f, etc. and \c glUniformMatrix2f,
 * etc.
 *
 * \param uni          Uniform whose data is to be propagated to driver storage
 * \param array_index  If \c uni is an array, this is the element of
 *                     the array to be propagated.
 * \param count        Number of array elements to propagate.
 */
extern "C" void
_mesa_propagate_uniforms_to_driver_storage(struct gl_uniform_storage *uni,
					   unsigned array_index,
					   unsigned count)
{
   unsigned i;

   /* vector_elements and matrix_columns can be 0 for samplers.
    */
   const unsigned components = MAX2(1, uni->type->vector_elements);
   const unsigned vectors = MAX2(1, uni->type->matrix_columns);

   /* Store the data in the driver's requested type in the driver's storage
    * areas.
    */
   unsigned src_vector_byte_stride = components * 4;

   for (i = 0; i < uni->num_driver_storage; i++) {
      struct gl_uniform_driver_storage *const store = &uni->driver_storage[i];
      uint8_t *dst = (uint8_t *) store->data;
      const unsigned extra_stride =
	 store->element_stride - (vectors * store->vector_stride);
      const uint8_t *src =
	 (uint8_t *) (&uni->storage[array_index * (components * vectors)].i);

#if 0
      printf("%s: %p[%d] components=%u vectors=%u count=%u vector_stride=%u "
	     "extra_stride=%u\n",
	     __func__, dst, array_index, components,
	     vectors, count, store->vector_stride, extra_stride);
#endif

      dst += array_index * store->element_stride;

      switch (store->format) {
      case uniform_native:
      case uniform_bool_int_0_1: {
	 unsigned j;
	 unsigned v;

	 for (j = 0; j < count; j++) {
	    for (v = 0; v < vectors; v++) {
	       memcpy(dst, src, src_vector_byte_stride);
	       src += src_vector_byte_stride;
	       dst += store->vector_stride;
	    }

	    dst += extra_stride;
	 }
	 break;
      }

      case uniform_int_float:
      case uniform_bool_float: {
	 const int *isrc = (const int *) src;
	 unsigned j;
	 unsigned v;
	 unsigned c;

	 for (j = 0; j < count; j++) {
	    for (v = 0; v < vectors; v++) {
	       for (c = 0; c < components; c++) {
		  ((float *) dst)[c] = (float) *isrc;
		  isrc++;
	       }

	       dst += store->vector_stride;
	    }

	    dst += extra_stride;
	 }
	 break;
      }

      case uniform_bool_int_0_not0: {
	 const int *isrc = (const int *) src;
	 unsigned j;
	 unsigned v;
	 unsigned c;

	 for (j = 0; j < count; j++) {
	    for (v = 0; v < vectors; v++) {
	       for (c = 0; c < components; c++) {
		  ((int *) dst)[c] = *isrc == 0 ? 0 : ~0;
		  isrc++;
	       }

	       dst += store->vector_stride;
	    }

	    dst += extra_stride;
	 }
	 break;
      }

      default:
	 assert(!"Should not get here.");
	 break;
      }
   }
}

/**
 * Called via glUniform*() functions.
 */
extern "C" void
_mesa_uniform(struct gl_context *ctx, struct gl_shader_program *shProg,
	      GLint location, GLsizei count,
              const GLvoid *values, GLenum type)
{
   struct gl_uniform *uniform;
   GLint elems;
   unsigned loc, offset;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_uniform_parameters(ctx, shProg, location, count,
				    &loc, &offset, "glUniform", false))
      return;

   elems = _mesa_sizeof_glsl_type(type);

   FLUSH_VERTICES(ctx, _NEW_PROGRAM_CONSTANTS);

   uniform = &shProg->Uniforms->Uniforms[loc];

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
   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform(ctx,
                             shProg->_LinkedShaders[MESA_SHADER_VERTEX]->Program,
                             index, offset, type, count, elems, values);
      }
   }

   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform(ctx,
			     shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program,
                             index, offset, type, count, elems, values);
      }
   }

   if (shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->GeomPos;
      if (index >= 0) {
         set_program_uniform(ctx,
			     shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program,
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
         v = (GLfloat *) program->Parameters->ParameterValues[index + offset];
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
extern "C" void
_mesa_uniform_matrix(struct gl_context *ctx, struct gl_shader_program *shProg,
		     GLint cols, GLint rows,
                     GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values)
{
   struct gl_uniform *uniform;
   unsigned loc, offset;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!validate_uniform_parameters(ctx, shProg, location, count,
				    &loc, &offset, "glUniformMatrix", false))
      return;

   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM_CONSTANTS);

   uniform = &shProg->Uniforms->Uniforms[loc];

   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->VertPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx,
				    shProg->_LinkedShaders[MESA_SHADER_VERTEX]->Program,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->FragPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx,
				    shProg->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   if (shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]) {
      /* convert uniform location to program parameter index */
      GLint index = uniform->GeomPos;
      if (index >= 0) {
         set_program_uniform_matrix(ctx,
                                    shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program,
                                    index, offset,
                                    count, rows, cols, transpose, values);
      }
   }

   uniform->Initialized = GL_TRUE;
}

/**
 * Called via glGetUniformLocation().
 *
 * The return value will encode two values, the uniform location and an
 * offset (used for arrays, structs).
 */
extern "C" GLint
_mesa_get_uniform_location(struct gl_context *ctx,
                           struct gl_shader_program *shProg,
			   const GLchar *name)
{
   GLint offset = 0, location = -1;

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
      char *c = strchr((char *)name, '[');
      if (c) {
         /* truncate name at [ */
         const GLint len = c - name;
         GLchar *newName = (GLchar *) malloc(len + 1);
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

   if (location < 0) {
      return -1;
   }

   return _mesa_uniform_merge_location_offset(location, offset);
}
