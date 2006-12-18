/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2004-2006  Brian Paul   All Rights Reserved.
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
 * \file shader_api.c
 * API functions for shader objects
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 *
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "program.h"
#include "prog_parameter.h"
#include "shaderobjects.h"
#include "shader_api.h"

#include "slang_compile.h"
#include "slang_link.h"



GLvoid GLAPIENTRY
_mesa_DeleteObjectARB(GLhandleARB obj)
{
#if 000
   if (obj != 0) {
      GET_CURRENT_CONTEXT(ctx);
      GET_GENERIC(gen, obj, "glDeleteObjectARB");

      if (gen != NULL) {
         (**gen).Delete(gen);
         RELEASE_GENERIC(gen);
      }
   }
#endif
}


GLhandleARB GLAPIENTRY
_mesa_GetHandleARB(GLenum pname)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);

   switch (pname) {
   case GL_PROGRAM_OBJECT_ARB:
      {
         struct gl2_program_intf **pro = ctx->ShaderObjects.CurrentProgram;

         if (pro != NULL)
            return (**pro)._container._generic.
               GetName((struct gl2_generic_intf **) (pro));
      }
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHandleARB");
   }
#endif
   return 0;
}


GLvoid GLAPIENTRY
_mesa_DetachObjectARB(GLhandleARB program, GLhandleARB shader)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked
      = _mesa_lookup_linked_program(ctx, program);
   struct gl_program *prog = _mesa_lookup_shader(ctx, shader);
   const GLuint n = linked->NumShaders;
   GLuint i, j;

   if (!linked || !prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDetachObjectARB(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (linked->Shaders[i] == prog) {
         struct gl_program **newList;
         /* found it */
         /* alloc new list */
         newList = (struct gl_program **)
            _mesa_malloc((n - 1) * sizeof(struct gl_program *));
         if (!newList) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachObjectARB");
            return;
         }
         for (j = 0; j < i; j++) {
            newList[j] = linked->Shaders[j];
         }
         while (++i < n)
            newList[j++] = linked->Shaders[i];
         _mesa_free(linked->Shaders);
         linked->Shaders = newList;
         return;
      }
   }

   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glDetachObjectARB(shader not found)");
}


GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB(GLenum shaderType)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *newProg;
   GLuint name;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);

   switch (shaderType) {
   case GL_FRAGMENT_SHADER_ARB:
      /* alloc new gl_fragment_program */
      newProg = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, name);
      break;
   case GL_VERTEX_SHADER_ARB:
      /* alloc new gl_vertex_program */
      newProg = ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, name);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "CreateShaderObject(shaderType)");
      return 0;
   }

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, newProg);

   return name;
}



GLvoid GLAPIENTRY
_mesa_ShaderSourceARB(GLhandleARB shaderObj, GLsizei count,
                      const GLcharARB ** string, const GLint * length)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *shader;
   GLint *offsets;
   GLsizei i;
   GLcharARB *source;

   if (string == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSourceARB");
      return;
   }

   shader = _mesa_lookup_shader(ctx, shaderObj);
   if (!shader) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSourceARB(shaderObj)");
      return;
   }

   /*
    * This array holds offsets of where the appropriate string ends, thus the
    * last element will be set to the total length of the source code.
    */
   offsets = (GLint *) _mesa_malloc(count * sizeof(GLint));
   if (offsets == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
      return;
   }

   for (i = 0; i < count; i++) {
      if (string[i] == NULL) {
         _mesa_free((GLvoid *) offsets);
         _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSourceARB(null string)");
         return;
      }
      if (length == NULL || length[i] < 0)
         offsets[i] = _mesa_strlen(string[i]);
      else
         offsets[i] = length[i];
      /* accumulate string lengths */
      if (i > 0)
         offsets[i] += offsets[i - 1];
   }

   source = (GLcharARB *) _mesa_malloc((offsets[count - 1] + 1) *
                                       sizeof(GLcharARB));
   if (source == NULL) {
      _mesa_free((GLvoid *) offsets);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
      return;
   }

   for (i = 0; i < count; i++) {
      GLint start = (i > 0) ? offsets[i - 1] : 0;
      _mesa_memcpy(source + start, string[i],
                   (offsets[i] - start) * sizeof(GLcharARB));
   }
   source[offsets[count - 1]] = '\0';

   /* free old shader source string and install new one */
   if (shader->String) {
      _mesa_free(shader->String);
   }
   shader->String = (GLubyte *) source;
}


GLvoid GLAPIENTRY
_mesa_CompileShaderARB(GLhandleARB shaderObj)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *prog = _mesa_lookup_shader(ctx, shaderObj);
   slang_info_log info_log;
   slang_code_object obj;
   slang_unit_type type;

   if (!prog) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompileShader(shaderObj)");
      return;
   }

   slang_info_log_construct(&info_log);
   _slang_code_object_ctr(&obj);

   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      type = slang_unit_vertex_shader;
   }
   else {
      assert(prog->Target == GL_FRAGMENT_PROGRAM_ARB);
      type = slang_unit_fragment_shader;
   }

   if (_slang_compile((const char*) prog->String, &obj,
                      type, &info_log, prog)) {
      /*
      prog->CompileStatus = GL_TRUE;
      */
   }
   else {
      /*
        prog->CompileStatus = GL_FALSE;
      */
      _mesa_problem(ctx, "Program did not compile!");
   }
}


GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB(GLvoid)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint name;
   struct gl_linked_program *linked;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ProgramObjects, 1);
   linked = _mesa_new_linked_program(ctx, name);

   _mesa_HashInsert(ctx->Shared->ProgramObjects, name, linked);

   return name;
}


GLvoid GLAPIENTRY
_mesa_AttachObjectARB(GLhandleARB program, GLhandleARB shader)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked
      = _mesa_lookup_linked_program(ctx, program);
   struct gl_program *prog = _mesa_lookup_shader(ctx, shader);
   const GLuint n = linked->NumShaders;
   GLuint i;

   if (!linked || !prog) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glAttachShader(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (linked->Shaders[i] == prog) {
         /* already attached */
         return;
      }
   }

   /* grow list */
   linked->Shaders = (struct gl_program **)
      _mesa_realloc(linked->Shaders,
                    n * sizeof(struct gl_program *),
                    (n + 1) * sizeof(struct gl_program *));
   /* append */
   linked->Shaders[n] = prog;
   prog->RefCount++;
   linked->NumShaders++;
}




GLvoid GLAPIENTRY
_mesa_LinkProgramARB(GLhandleARB programObj)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked;

   linked = _mesa_lookup_linked_program(ctx, programObj);
   if (!linked) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glLinkProgram(programObj)");
      return;
   }
   _slang_link2(ctx, programObj, linked);
}



GLvoid GLAPIENTRY
_mesa_UseProgramObjectARB(GLhandleARB programObj)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   linked = _mesa_lookup_linked_program(ctx, programObj);
   if (!linked) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glUseProgramObjectARB(programObj)");
      return;
   }

   ctx->ShaderObjects.Linked = linked;
}


GLvoid GLAPIENTRY
_mesa_ValidateProgramARB(GLhandleARB programObj)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_PROGRAM(pro, programObj, "glValidateProgramARB");

   if (pro != NULL) {
      (**pro).Validate(pro);
      RELEASE_PROGRAM(pro);
   }
#endif
}


/**
 * Helper function for all the _mesa_Uniform*() functions below.
 */
static INLINE void
uniform(GLint location, GLsizei count, const GLvoid *values, GLenum type,
        const char *caller)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->ShaderObjects.Linked) {
      struct gl_linked_program *linked = ctx->ShaderObjects.Linked;
      if (location >= 0 && location < linked->Uniforms->NumParameters) {
         GLfloat *v = linked->Uniforms->ParameterValues[location];
         const GLfloat *fValues = (const GLfloat *) values; /* XXX */
         GLint i;
         if (type == GL_FLOAT_VEC4)
            count *= 4;
         else if (type == GL_FLOAT_VEC3)
            count *= 3;
         else
            abort();
         
         for (i = 0; i < count; i++)
            v[i] = fValues[i];
         return;
      }
   }
}


GLvoid GLAPIENTRY
_mesa_Uniform1fARB(GLint location, GLfloat v0)
{
   uniform(location, 1, &v0, GL_FLOAT, "glUniform1fARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
   GLfloat v[2];
   v[0] = v0;
   v[1] = v1;
   uniform(location, 1, v, GL_FLOAT_VEC2, "glUniform2fARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   GLfloat v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   uniform(location, 1, v, GL_FLOAT_VEC3, "glUniform3fARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                   GLfloat v3)
{
   GLfloat v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   uniform(location, 1, v, GL_FLOAT_VEC4, "glUniform4fARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform1iARB(GLint location, GLint v0)
{
   uniform(location, 1, &v0, GL_INT, "glUniform1iARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform2iARB(GLint location, GLint v0, GLint v1)
{
   GLint v[2];
   v[0] = v0;
   v[1] = v1;
   uniform(location, 1, v, GL_INT_VEC2, "glUniform2iARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
   GLint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   uniform(location, 1, v, GL_INT_VEC3, "glUniform3iARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
   GLint v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   uniform(location, 1, v, GL_INT_VEC4, "glUniform4iARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform1fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   uniform(location, count, value, GL_FLOAT, "glUniform1fvARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform2fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   uniform(location, count, value, GL_FLOAT_VEC2, "glUniform2fvARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform3fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   uniform(location, count, value, GL_FLOAT_VEC3, "glUniform3fvARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform4fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   uniform(location, count, value, GL_FLOAT_VEC4, "glUniform4fvARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform1ivARB(GLint location, GLsizei count, const GLint * value)
{
   uniform(location, count, value, GL_INT, "glUniform1ivARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform2ivARB(GLint location, GLsizei count, const GLint * value)
{
   uniform(location, count, value, GL_INT_VEC2, "glUniform2ivARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform3ivARB(GLint location, GLsizei count, const GLint * value)
{
   uniform(location, count, value, GL_INT_VEC3, "glUniform3ivARB");
}

GLvoid GLAPIENTRY
_mesa_Uniform4ivARB(GLint location, GLsizei count, const GLint * value)
{
   uniform(location, count, value, GL_INT_VEC4, "glUniform4ivARB");
}


/**
 * Helper function used by UniformMatrix**vARB() functions below.
 */
static void
uniform_matrix(GLint cols, GLint rows, const char *caller,
               GLenum matrixType,
               GLint location, GLsizei count, GLboolean transpose,
               const GLfloat *values)
{
   const GLint matElements = rows * cols;
   GET_CURRENT_CONTEXT(ctx);

#ifdef OLD
   GET_CURRENT_LINKED_PROGRAM(pro, caller);
#endif

   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, caller);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (transpose) {
      GLfloat *trans, *pt;
      const GLfloat *pv;
      GLint i, j, k;

      trans = (GLfloat *) _mesa_malloc(count * matElements * sizeof(GLfloat));
      if (!trans) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, caller);
         return;
      }

      pt = trans;
      pv = values;
      for (i = 0; i < count; i++) {
         /* transpose from pv matrix into pt matrix */
         for (j = 0; j < cols; j++) {
            for (k = 0; k < rows; k++) {
               /* XXX verify this */
               pt[j * rows + k] = pv[k * cols + j];
            }
         }
         pt += matElements;
         pv += matElements;
      }

#ifdef OLD
      if (!(**pro).WriteUniform(pro, location, count, trans, matrixType))
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
#endif
      _mesa_free(trans);
   }
   else {
#ifdef OLD
      if (!(**pro).WriteUniform(pro, location, count, values, matrixType))
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
#endif
   }
}


GLvoid GLAPIENTRY
_mesa_UniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   uniform_matrix(2, 2, "glUniformMatrix2fvARB", GL_FLOAT_MAT2,
                  location, count, transpose, value);
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   uniform_matrix(3, 3, "glUniformMatrix3fvARB", GL_FLOAT_MAT3,
                  location, count, transpose, value);
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   uniform_matrix(4, 4, "glUniformMatrix4fvARB", GL_FLOAT_MAT4,
                  location, count, transpose, value);
}

static GLboolean
_mesa_get_object_parameter(GLhandleARB obj, GLenum pname, GLvoid * params,
                           GLboolean * integral, GLint * size)
{
#if 000
   GET_CURRENT_CONTEXT(ctx);
   GLint *ipar = (GLint *) params;

   /* set default values */
   *integral = GL_TRUE; /* indicates param type, TRUE: GLint, FALSE: GLfloat */
   *size = 1;           /* param array size */

   switch (pname) {
   case GL_OBJECT_TYPE_ARB:
   case GL_OBJECT_DELETE_STATUS_ARB:
   case GL_OBJECT_INFO_LOG_LENGTH_ARB:
      {
         GET_GENERIC(gen, obj, "glGetObjectParameterivARB");

         if (gen == NULL)
            return GL_FALSE;

         switch (pname) {
         case GL_OBJECT_TYPE_ARB:
            *ipar = (**gen).GetType(gen);
            break;
         case GL_OBJECT_DELETE_STATUS_ARB:
            *ipar = (**gen).GetDeleteStatus(gen);
            break;
         case GL_OBJECT_INFO_LOG_LENGTH_ARB:
            *ipar = (**gen).GetInfoLogLength(gen);
            break;
         }

         RELEASE_GENERIC(gen);
      }
      break;
   case GL_OBJECT_SUBTYPE_ARB:
   case GL_OBJECT_COMPILE_STATUS_ARB:
   case GL_OBJECT_SHADER_SOURCE_LENGTH_ARB:
      {
         GET_SHADER(sha, obj, "glGetObjectParameterivARB");

         if (sha == NULL)
            return GL_FALSE;

         switch (pname) {
         case GL_OBJECT_SUBTYPE_ARB:
            *ipar = (**sha).GetSubType(sha);
            break;
         case GL_OBJECT_COMPILE_STATUS_ARB:
            *ipar = (**sha).GetCompileStatus(sha);
            break;
         case GL_OBJECT_SHADER_SOURCE_LENGTH_ARB:
            {
               const GLcharARB *src = (**sha).GetSource(sha);
               if (src == NULL)
                  *ipar = 0;
               else
                  *ipar = _mesa_strlen(src) + 1;
            }
            break;
         }

         RELEASE_SHADER(sha);
      }
      break;
   case GL_OBJECT_LINK_STATUS_ARB:
   case GL_OBJECT_VALIDATE_STATUS_ARB:
   case GL_OBJECT_ATTACHED_OBJECTS_ARB:
   case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
   case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
      {
         GET_PROGRAM(pro, obj, "glGetObjectParameterivARB");

         if (pro == NULL)
            return GL_FALSE;

         switch (pname) {
         case GL_OBJECT_LINK_STATUS_ARB:
            *ipar = (**pro).GetLinkStatus(pro);
            break;
         case GL_OBJECT_VALIDATE_STATUS_ARB:
            *ipar = (**pro).GetValidateStatus(pro);
            break;
         case GL_OBJECT_ATTACHED_OBJECTS_ARB:
            *ipar =
               (**pro)._container.
               GetAttachedCount((struct gl2_container_intf **) (pro));
            break;
         case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
            *ipar = (**pro).GetActiveUniformCount(pro);
            break;
         case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
            *ipar = (**pro).GetActiveUniformMaxLength(pro);
            break;
         case GL_OBJECT_ACTIVE_ATTRIBUTES_ARB:
            *ipar = (**pro).GetActiveAttribCount(pro);
            break;
         case GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB:
            *ipar = (**pro).GetActiveAttribMaxLength(pro);
            break;
         }

         RELEASE_PROGRAM(pro);
      }
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetObjectParameterivARB");
      return GL_FALSE;
   }
#endif
   return GL_TRUE;
}

GLvoid GLAPIENTRY
_mesa_GetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat * params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean integral;
   GLint size;

   if (params == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetObjectParameterfvARB");
      return;
   }

   assert(sizeof(GLfloat) == sizeof(GLint));

   if (_mesa_get_object_parameter(obj, pname, (GLvoid *) params,
                                  &integral, &size)) {
      if (integral) {
         GLint i;
         for (i = 0; i < size; i++)
            params[i] = (GLfloat) ((GLint *) params)[i];
      }
   }
}

GLvoid GLAPIENTRY
_mesa_GetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint * params)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean integral;
   GLint size;

   if (params == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetObjectParameterivARB");
      return;
   }

   assert(sizeof(GLfloat) == sizeof(GLint));

   if (_mesa_get_object_parameter(obj, pname, (GLvoid *) params,
                                  &integral, &size)) {
      if (!integral) {
         GLint i;
         for (i = 0; i < size; i++)
            params[i] = (GLint) ((GLfloat *) params)[i];
      }
   }
}


/**
 * Copy string from <src> to <dst>, up to maxLength characters, returning
 * length of <dst> in <length>.
 * \param src  the strings source
 * \param maxLength  max chars to copy
 * \param length  returns numberof chars copied
 * \param dst  the string destination
 */
static GLvoid
copy_string(const GLcharARB * src, GLsizei maxLength, GLsizei * length,
            GLcharARB * dst)
{
   GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
      dst[len] = src[len];
   if (maxLength > 0)
      dst[len] = 0;
   if (length)
      *length = len;
}


GLvoid GLAPIENTRY
_mesa_GetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei * length,
                    GLcharARB * infoLog)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_GENERIC(gen, obj, "glGetInfoLogARB");

   if (gen == NULL)
      return;

   if (infoLog == NULL)
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetInfoLogARB");
   else {
      GLsizei actualsize = (**gen).GetInfoLogLength(gen);
      if (actualsize > maxLength)
         actualsize = maxLength;
      (**gen).GetInfoLog(gen, actualsize, infoLog);
      if (length != NULL)
         *length = (actualsize > 0) ? actualsize - 1 : 0;
   }
   RELEASE_GENERIC(gen);
#endif
}


GLvoid GLAPIENTRY
_mesa_GetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount,
                            GLsizei * count, GLhandleARB * obj)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_CONTAINER(con, containerObj, "glGetAttachedObjectsARB");

   if (con == NULL)
      return;

   if (obj == NULL)
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttachedObjectsARB");
   else {
      GLsizei cnt, i;

      cnt = (**con).GetAttachedCount(con);
      if (cnt > maxCount)
         cnt = maxCount;
      if (count != NULL)
         *count = cnt;

      for (i = 0; i < cnt; i++) {
         struct gl2_generic_intf **x = (**con).GetAttached(con, i);
         obj[i] = (**x).GetName(x);
         RELEASE_GENERIC(x);
      }
   }
   RELEASE_CONTAINER(con);
#endif
}

GLint GLAPIENTRY
_mesa_GetUniformLocationARB(GLhandleARB programObj, const GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->ShaderObjects.Linked) {
      const struct gl_linked_program *linked = ctx->ShaderObjects.Linked;
      GLuint loc;
      for (loc = 0; loc < linked->Uniforms->NumParameters; loc++) {
         const struct gl_program_parameter *u
            = linked->Uniforms->Parameters + loc;
         if (u->Type == PROGRAM_UNIFORM && !strcmp(u->Name, name)) {
            return loc;
         }
      }
   }
   return -1;

}


GLvoid GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB programObj, GLuint index,
                          GLsizei maxLength, GLsizei * length, GLint * size,
                          GLenum * type, GLcharARB * name)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_PROGRAM(pro, programObj, "glGetActiveUniformARB");

   if (pro == NULL)
      return;

   if (size == NULL || type == NULL || name == NULL)
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
   else {
      if (index < (**pro).GetActiveUniformCount(pro))
         (**pro).GetActiveUniform(pro, index, maxLength, length, size, type,
                                  name);
      else
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
   }
   RELEASE_PROGRAM(pro);
#endif
}


GLvoid GLAPIENTRY
_mesa_GetUniformfvARB(GLhandleARB programObj, GLint location, GLfloat * params)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_LINKED_PROGRAM(pro, programObj, "glGetUniformfvARB");

   if (!pro)
      return;

   if (!(**pro).ReadUniform(pro, location, 1, params, GL_FLOAT))
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformfvARB");

   RELEASE_PROGRAM(pro);
#endif
}


GLvoid GLAPIENTRY
_mesa_GetUniformivARB(GLhandleARB programObj, GLint location, GLint * params)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_LINKED_PROGRAM(pro, programObj, "glGetUniformivARB");

   if (!pro)
      return;

   if (!(**pro).ReadUniform(pro, location, 1, params, GL_INT))
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformivARB");
   RELEASE_PROGRAM(pro);
#endif
}

GLvoid GLAPIENTRY
_mesa_GetShaderSourceARB(GLhandleARB obj, GLsizei maxLength, GLsizei * length,
                         GLcharARB * sourceOut)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *shader = _mesa_lookup_shader(ctx, obj);

   if (!shader) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderSourceARB(obj)");
      return;
   }

   copy_string((GLcharARB *) shader->String, maxLength, length, sourceOut);
}


/* GL_ARB_vertex_shader */

GLvoid GLAPIENTRY
_mesa_BindAttribLocationARB(GLhandleARB programObj, GLuint index,
                            const GLcharARB * name)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_PROGRAM(pro, programObj, "glBindAttribLocationARB");

   if (pro == NULL)
      return;

   if (name == NULL || index >= MAX_VERTEX_ATTRIBS)
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocationARB");
   else if (IS_NAME_WITH_GL_PREFIX(name))
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBindAttribLocationARB");
   else
      (**pro).OverrideAttribBinding(pro, index, name);
   RELEASE_PROGRAM(pro);
#endif
}


GLvoid GLAPIENTRY
_mesa_GetActiveAttribARB(GLhandleARB programObj, GLuint index,
                         GLsizei maxLength, GLsizei * length, GLint * size,
                         GLenum * type, GLcharARB * name)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_PROGRAM(pro, programObj, "glGetActiveAttribARB");

   if (pro == NULL)
      return;

   if (name == NULL || index >= (**pro).GetActiveAttribCount(pro))
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttribARB");
   else
      (**pro).GetActiveAttrib(pro, index, maxLength, length, size, type,
                              name);
   RELEASE_PROGRAM(pro);
#endif
}


GLint GLAPIENTRY
_mesa_GetAttribLocationARB(GLhandleARB programObj, const GLcharARB * name)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GLint loc = -1;
   GET_LINKED_PROGRAM(pro, programObj, "glGetAttribLocationARB");

   if (!pro)
      return -1;

   if (name == NULL)
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttribLocationARB");
   else if (!IS_NAME_WITH_GL_PREFIX(name))
      loc = (**pro).GetAttribLocation(pro, name);
   RELEASE_PROGRAM(pro);
   return loc;
#endif
   return 0;
}


/**
 ** OpenGL 2.0 functions which basically wrap the ARB_shader functions
 **/

void GLAPIENTRY
_mesa_AttachShader(GLuint program, GLuint shader)
{
   _mesa_AttachObjectARB(program, shader);
}


GLuint GLAPIENTRY
_mesa_CreateShader(GLenum type)
{
   return (GLuint) _mesa_CreateShaderObjectARB(type);
}

GLuint GLAPIENTRY
_mesa_CreateProgram(void)
{
   return (GLuint) _mesa_CreateProgramObjectARB();
}


void GLAPIENTRY
_mesa_DeleteProgram(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked;

   linked = _mesa_lookup_linked_program(ctx, name);
   if (!linked) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDeleteProgram(name)");
      return;
   }

   _mesa_HashRemove(ctx->Shared->ProgramObjects, name);
   _mesa_delete_linked_program(ctx, linked);
}


void GLAPIENTRY
_mesa_DeleteShader(GLuint shader)
{
   _mesa_DeleteObjectARB(shader);
}

void GLAPIENTRY
_mesa_DetachShader(GLuint program, GLuint shader)
{
   _mesa_DetachObjectARB(program, shader);
}

void GLAPIENTRY
_mesa_GetAttachedShaders(GLuint program, GLsizei maxCount,
                         GLsizei *count, GLuint *obj)
{
   {
      GET_CURRENT_CONTEXT(ctx);
      struct gl_linked_program *linked
         = _mesa_lookup_linked_program(ctx, program);
      if (linked) {
         GLuint i;
         for (i = 0; i < maxCount && i < linked->NumShaders; i++) {
            obj[i] = linked->Shaders[i]->Id;
         }
         if (count)
            *count = i;
      }
      else {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glGetAttachedShaders");
      }
   }
}


void GLAPIENTRY
_mesa_GetProgramiv(GLuint program, GLenum pname, GLint *params)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_PROGRAM(pro, program, "glGetProgramiv");

   if (!pro)
      return;

   switch (pname) {
   case GL_DELETE_STATUS:
      *params = (**pro)._container._generic.GetDeleteStatus((struct gl2_generic_intf **) pro);
      break; 
   case GL_LINK_STATUS:
      *params = (**pro).GetLinkStatus(pro);
      break;
   case GL_VALIDATE_STATUS:
      *params = (**pro).GetValidateStatus(pro);
      break;
   case GL_INFO_LOG_LENGTH:
      *params = (**pro)._container._generic.GetInfoLogLength( (struct gl2_generic_intf **) pro );
      break;
   case GL_ATTACHED_SHADERS:
      *params = (**pro)._container.GetAttachedCount( (struct gl2_container_intf **) pro );
      break;
   case GL_ACTIVE_ATTRIBUTES:
      *params = (**pro).GetActiveAttribCount(pro);
      break;
   case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = (**pro).GetActiveAttribMaxLength(pro);
      break;
   case GL_ACTIVE_UNIFORMS:
      *params = (**pro).GetActiveUniformCount(pro);
      break;
   case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      *params = (**pro).GetActiveUniformMaxLength(pro);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname)");
      return;
   }
#endif
}


void GLAPIENTRY
_mesa_GetProgramInfoLog(GLuint program, GLsizei bufSize,
                        GLsizei *length, GLchar *infoLog)
{
   _mesa_GetInfoLogARB(program, bufSize, length, infoLog);
}


void GLAPIENTRY
_mesa_GetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);
   GET_SHADER(sh, shader, "glGetShaderiv");

   if (!sh)
      return;

   switch (pname) {
   case GL_SHADER_TYPE:
      *params = (**sh).GetSubType(sh);
      break;
   case GL_DELETE_STATUS:
      *params = (**sh)._generic.GetDeleteStatus((struct gl2_generic_intf **) sh);
      break;
   case GL_COMPILE_STATUS:
      *params = (**sh).GetCompileStatus(sh);
      break;
   case GL_INFO_LOG_LENGTH:
      *params = (**sh)._generic.GetInfoLogLength((struct gl2_generic_intf **)sh);
      break;
   case GL_SHADER_SOURCE_LENGTH:
      {
         const GLchar *src = (**sh).GetSource(sh);
         *params = src ? (_mesa_strlen(src) + 1) : 0;
      }
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetShaderiv(pname)");
      return;
   }
#endif
}


void GLAPIENTRY
_mesa_GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                       GLsizei *length, GLchar *infoLog)
{
   _mesa_GetInfoLogARB(shader, bufSize, length, infoLog);
}


GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_linked_program *linked = _mesa_lookup_linked_program(ctx, name);
   if (linked)
      return GL_TRUE;
   else
      return GL_FALSE;
}


GLboolean GLAPIENTRY
_mesa_IsShader(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program *shader = _mesa_lookup_shader(ctx, name);
   if (shader)
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 ** 2.1 functions
 **/

void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(2, 3, "glUniformMatrix2x3fv", GL_FLOAT_MAT2x3,
                  location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(3, 2, "glUniformMatrix3x2fv", GL_FLOAT_MAT3x2,
                  location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(2, 4, "glUniformMatrix2x4fv", GL_FLOAT_MAT2x4,
                  location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(4, 2, "glUniformMatrix4x2fv", GL_FLOAT_MAT4x2,
                  location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(3, 4, "glUniformMatrix3x4fv", GL_FLOAT_MAT3x4,
                  location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   uniform_matrix(4, 3, "glUniformMatrix4x3fv", GL_FLOAT_MAT4x3,
                  location, count, transpose, value);
}


/**
 * Create a new GLSL program object.
 */
struct gl_linked_program *
_mesa_new_linked_program(GLcontext *ctx, GLuint name)
{
   struct gl_linked_program *linked;
   linked = CALLOC_STRUCT(gl_linked_program);
   if (linked) {
      linked->Name = name;
   }
   return linked;
}


void
_mesa_free_linked_program_data(GLcontext *ctx,
                               struct gl_linked_program *linked)
{
   if (linked->VertexProgram) {
      if (linked->VertexProgram->Base.Parameters == linked->Uniforms) {
         /* to prevent a double-free in the next call */
         linked->VertexProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &linked->VertexProgram->Base);
      linked->VertexProgram = NULL;
   }

   if (linked->FragmentProgram) {
      if (linked->FragmentProgram->Base.Parameters == linked->Uniforms) {
         /* to prevent a double-free in the next call */
         linked->FragmentProgram->Base.Parameters = NULL;
      }
      _mesa_delete_program(ctx, &linked->FragmentProgram->Base);
      linked->FragmentProgram = NULL;
   }


   if (linked->Uniforms) {
      _mesa_free_parameter_list(linked->Uniforms);
      linked->Uniforms = NULL;
   }

   if (linked->Varying) {
      _mesa_free_parameter_list(linked->Varying);
      linked->Varying = NULL;
   }
}



void
_mesa_delete_linked_program(GLcontext *ctx, struct gl_linked_program *linked)
{
   _mesa_free_linked_program_data(ctx, linked);
   _mesa_free(linked);
}


/**
 * Lookup a GLSL program object.
 */
struct gl_linked_program *
_mesa_lookup_linked_program(GLcontext *ctx, GLuint name)
{
   if (name)
      return (struct gl_linked_program *)
         _mesa_HashLookup(ctx->Shared->ProgramObjects, name);
   else
      return NULL;
}


struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = CALLOC_STRUCT(gl_shader);
   if (shader) {
      shader->Name = name;
      shader->Type = type;
   }
   return shader;
}


/**
 * Lookup a GLSL shader object.
 */
struct gl_program *
_mesa_lookup_shader(GLcontext *ctx, GLuint name)
{
   if (name)
      return (struct gl_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
   else
      return NULL;
}


GLvoid
_mesa_init_shaderobjects(GLcontext * ctx)
{

   ctx->ShaderObjects.CurrentProgram = NULL;
   ctx->ShaderObjects._FragmentShaderPresent = GL_FALSE;
   ctx->ShaderObjects._VertexShaderPresent = GL_FALSE;

#if 0
   _mesa_init_shaderobjects_3dlabs(ctx);
#endif
}
