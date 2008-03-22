/*
 * Mesa 3-D graphics library
 * Version:  7.0
 *
 * Copyright (C) 2004-2007  Brian Paul   All Rights Reserved.
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
 * Implementation of GLSL-related API functions
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 * 2. Insert FLUSH_VERTICES calls in various places
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "program.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_statevars.h"
#include "shader/shader_api.h"
#include "shader/slang/slang_compile.h"
#include "shader/slang/slang_link.h"



/**
 * Allocate a new gl_shader_program object, initialize it.
 */
static struct gl_shader_program *
_mesa_new_shader_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   shProg = CALLOC_STRUCT(gl_shader_program);
   if (shProg) {
      shProg->Type = GL_SHADER_PROGRAM_MESA;
      shProg->Name = name;
      shProg->RefCount = 1;
      shProg->Attributes = _mesa_new_parameter_list();
   }
   return shProg;
}


/**
 * Clear (free) the shader program state that gets produced by linking.
 */
void
_mesa_clear_shader_program_data(GLcontext *ctx,
                                struct gl_shader_program *shProg)
{
   if (shProg->VertexProgram) {
      if (shProg->VertexProgram->Base.Parameters == shProg->Uniforms) {
         /* to prevent a double-free in the next call */
         shProg->VertexProgram->Base.Parameters = NULL;
      }
      ctx->Driver.DeleteProgram(ctx, &shProg->VertexProgram->Base);
      shProg->VertexProgram = NULL;
   }

   if (shProg->FragmentProgram) {
      if (shProg->FragmentProgram->Base.Parameters == shProg->Uniforms) {
         /* to prevent a double-free in the next call */
         shProg->FragmentProgram->Base.Parameters = NULL;
      }
      ctx->Driver.DeleteProgram(ctx, &shProg->FragmentProgram->Base);
      shProg->FragmentProgram = NULL;
   }

   if (shProg->Uniforms) {
      _mesa_free_parameter_list(shProg->Uniforms);
      shProg->Uniforms = NULL;
   }

   if (shProg->Varying) {
      _mesa_free_parameter_list(shProg->Varying);
      shProg->Varying = NULL;
   }
}


/**
 * Free all the data that hangs off a shader program object, but not the
 * object itself.
 */
void
_mesa_free_shader_program_data(GLcontext *ctx,
                               struct gl_shader_program *shProg)
{
   GLuint i;

   assert(shProg->Type == GL_SHADER_PROGRAM_MESA);

   _mesa_clear_shader_program_data(ctx, shProg);

   if (shProg->Attributes) {
      _mesa_free_parameter_list(shProg->Attributes);
      shProg->Attributes = NULL;
   }

   /* detach shaders */
   for (i = 0; i < shProg->NumShaders; i++) {
      _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);
   }
   if (shProg->Shaders) {
      _mesa_free(shProg->Shaders);
      shProg->Shaders = NULL;
   }
}


/**
 * Free/delete a shader program object.
 */
void
_mesa_free_shader_program(GLcontext *ctx, struct gl_shader_program *shProg)
{
   _mesa_free_shader_program_data(ctx, shProg);
   if (shProg->Shaders) {
      _mesa_free(shProg->Shaders);
      shProg->Shaders = NULL;
   }
   _mesa_free(shProg);
}


/**
 * Set ptr to point to shProg.
 * If ptr is pointing to another object, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to shProg, incrementing its refcount.
 */
/* XXX this could be static */
void
_mesa_reference_shader_program(GLcontext *ctx,
                               struct gl_shader_program **ptr,
                               struct gl_shader_program *shProg)
{
   assert(ptr);
   if (*ptr == shProg) {
      /* no-op */
      return;
   }
   if (*ptr) {
      /* Unreference the old shader program */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_shader_program *old = *ptr;

      ASSERT(old->RefCount > 0);
      old->RefCount--;
      /*printf("SHPROG DECR %p (%d) to %d\n",
        (void*) old, old->Name, old->RefCount);*/
      deleteFlag = (old->RefCount == 0);

      if (deleteFlag) {
         _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         _mesa_free_shader_program(ctx, old);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (shProg) {
      shProg->RefCount++;
      /*printf("SHPROG INCR %p (%d) to %d\n",
        (void*) shProg, shProg->Name, shProg->RefCount);*/
      *ptr = shProg;
   }
}


/**
 * Lookup a GLSL program object.
 */
struct gl_shader_program *
_mesa_lookup_shader_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   if (name) {
      shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (shProg && shProg->Type != GL_SHADER_PROGRAM_MESA) {
         return NULL;
      }
      return shProg;
   }
   return NULL;
}


/**
 * Allocate a new gl_shader object, initialize it.
 */
struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = CALLOC_STRUCT(gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}


void
_mesa_free_shader(GLcontext *ctx, struct gl_shader *sh)
{
   GLuint i;
   if (sh->Source)
      _mesa_free((void *) sh->Source);
   if (sh->InfoLog)
      _mesa_free(sh->InfoLog);
   for (i = 0; i < sh->NumPrograms; i++) {
      assert(sh->Programs[i]);
      ctx->Driver.DeleteProgram(ctx, sh->Programs[i]);
   }
   if (sh->Programs)
      _mesa_free(sh->Programs);
   _mesa_free(sh);
}


/**
 * Set ptr to point to sh.
 * If ptr is pointing to another shader, decrement its refcount (and delete
 * if refcount hits zero).
 * Then set ptr to point to sh, incrementing its refcount.
 */
/* XXX this could be static */
void
_mesa_reference_shader(GLcontext *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh)
{
   assert(ptr);
   if (*ptr == sh) {
      /* no-op */
      return;
   }
   if (*ptr) {
      /* Unreference the old shader */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_shader *old = *ptr;

      ASSERT(old->RefCount > 0);
      old->RefCount--;
      /*printf("SHADER DECR %p (%d) to %d\n",
        (void*) old, old->Name, old->RefCount);*/
      deleteFlag = (old->RefCount == 0);

      if (deleteFlag) {
         _mesa_HashRemove(ctx->Shared->ShaderObjects, old->Name);
         _mesa_free_shader(ctx, old);
      }

      *ptr = NULL;
   }
   assert(!*ptr);

   if (sh) {
      /* reference new */
      sh->RefCount++;
      /*printf("SHADER INCR %p (%d) to %d\n",
        (void*) sh, sh->Name, sh->RefCount);*/
      *ptr = sh;
   }
}


/**
 * Lookup a GLSL shader object.
 */
struct gl_shader *
_mesa_lookup_shader(GLcontext *ctx, GLuint name)
{
   if (name) {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      /* Note that both gl_shader and gl_shader_program objects are kept
       * in the same hash table.  Check the object's type to be sure it's
       * what we're expecting.
       */
      if (sh && sh->Type == GL_SHADER_PROGRAM_MESA) {
         return NULL;
      }
      return sh;
   }
   return NULL;
}


/**
 * Initialize context's shader state.
 */
void
_mesa_init_shader_state(GLcontext * ctx)
{
   /* Device drivers may override these to control what kind of instructions
    * are generated by the GLSL compiler.
    */
   ctx->Shader.EmitHighLevelInstructions = GL_TRUE;
   ctx->Shader.EmitCondCodes = GL_FALSE;/*GL_TRUE;*/ /* XXX probably want GL_FALSE... */
   ctx->Shader.EmitComments = GL_FALSE;
}


/**
 * Free the per-context shader-related state.
 */
void
_mesa_free_shader_state(GLcontext *ctx)
{
   _mesa_reference_shader_program(ctx, &ctx->Shader.CurrentProgram, NULL);
}


/**
 * Copy string from <src> to <dst>, up to maxLength characters, returning
 * length of <dst> in <length>.
 * \param src  the strings source
 * \param maxLength  max chars to copy
 * \param length  returns number of chars copied
 * \param dst  the string destination
 */
static void
copy_string(GLchar *dst, GLsizei maxLength, GLsizei *length, const GLchar *src)
{
   GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
      dst[len] = src[len];
   if (maxLength > 0)
      dst[len] = 0;
   if (length)
      *length = len;
}


/**
 * Called via ctx->Driver.AttachShader()
 */
static void
_mesa_attach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   const GLuint n = shProg->NumShaders;
   GLuint i;

   if (!shProg || !sh) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glAttachShader(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i] == sh) {
         /* already attached */
         return;
      }
   }

   /* grow list */
   shProg->Shaders = (struct gl_shader **)
      _mesa_realloc(shProg->Shaders,
                    n * sizeof(struct gl_shader *),
                    (n + 1) * sizeof(struct gl_shader *));
   if (!shProg->Shaders) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAttachShader");
      return;
   }

   /* append */
   shProg->Shaders[n] = NULL; /* since realloc() didn't zero the new space */
   _mesa_reference_shader(ctx, &shProg->Shaders[n], sh);
   shProg->NumShaders++;
}


static GLint
_mesa_get_attrib_location(GLcontext *ctx, GLuint program,
                          const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttribLocation");
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetAttribLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (shProg->Attributes) {
      GLint i = _mesa_lookup_parameter_index(shProg->Attributes, -1, name);
      if (i >= 0) {
         return shProg->Attributes->Parameters[i].StateIndexes[0];
      }
   }
   return -1;
}


static void
_mesa_bind_attrib_location(GLcontext *ctx, GLuint program, GLuint index,
                           const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   const GLint size = -1; /* unknown size */
   GLint i, oldIndex;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(program)");
      return;
   }

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindAttribLocation(illegal name)");
      return;
   }

   if (shProg->LinkStatus) {
      /* get current index/location for the attribute */
      oldIndex = _mesa_get_attrib_location(ctx, program, name);
   }
   else {
      oldIndex = -1;
   }

   /* this will replace the current value if it's already in the list */
   i = _mesa_add_attribute(shProg->Attributes, name, size, index);
   if (i < 0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindAttribLocation");
   }

   if (shProg->VertexProgram && oldIndex >= 0 && oldIndex != index) {
      /* If the index changed, need to search/replace references to that attribute
       * in the vertex program.
       */
      _slang_remap_attribute(&shProg->VertexProgram->Base, oldIndex, index);
   }
}


static GLuint
_mesa_create_shader(GLcontext *ctx, GLenum type)
{
   struct gl_shader *sh;
   GLuint name;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);

   switch (type) {
   case GL_FRAGMENT_SHADER:
   case GL_VERTEX_SHADER:
      sh = _mesa_new_shader(ctx, name, type);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "CreateShader(type)");
      return 0;
   }

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, sh);

   return name;
}


static GLuint 
_mesa_create_program(GLcontext *ctx)
{
   GLuint name;
   struct gl_shader_program *shProg;

   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
   shProg = _mesa_new_shader_program(ctx, name);

   _mesa_HashInsert(ctx->Shared->ShaderObjects, name, shProg);

   assert(shProg->RefCount == 1);

   return name;
}


/**
 * Named w/ "2" to indicate OpenGL 2.x vs GL_ARB_fragment_programs's
 * DeleteProgramARB.
 */
static void
_mesa_delete_program2(GLcontext *ctx, GLuint name)
{
   /*
    * NOTE: deleting shaders/programs works a bit differently than
    * texture objects (and buffer objects, etc).  Shader/program
    * handles/IDs exist in the hash table until the object is really
    * deleted (refcount==0).  With texture objects, the handle/ID is
    * removed from the hash table in glDeleteTextures() while the tex
    * object itself might linger until its refcount goes to zero.
    */
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program(ctx, name);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteProgram(name)");
      return;
   }

   shProg->DeletePending = GL_TRUE;

   /* effectively, decr shProg's refcount */
   _mesa_reference_shader_program(ctx, &shProg, NULL);
}


static void
_mesa_delete_shader(GLcontext *ctx, GLuint shader)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      return;
   }

   sh->DeletePending = GL_TRUE;

   /* effectively, decr sh's refcount */
   _mesa_reference_shader(ctx, &sh, NULL);
}


static void
_mesa_detach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   const GLuint n = shProg->NumShaders;
   GLuint i, j;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDetachShader(bad program or shader name)");
      return;
   }

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i]->Name == shader) {
         /* found it */
         struct gl_shader **newList;

         /* derefernce */
         _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);

         /* alloc new, smaller array */
         newList = (struct gl_shader **)
            _mesa_malloc((n - 1) * sizeof(struct gl_shader *));
         if (!newList) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachShader");
            return;
         }
         for (j = 0; j < i; j++) {
            newList[j] = shProg->Shaders[j];
         }
         while (++i < n)
            newList[j++] = shProg->Shaders[i];
         _mesa_free(shProg->Shaders);

         shProg->Shaders = newList;
         shProg->NumShaders = n - 1;

#ifdef DEBUG
         /* sanity check */
         {
            for (j = 0; j < shProg->NumShaders; j++) {
               assert(shProg->Shaders[j]->Type == GL_VERTEX_SHADER ||
                      shProg->Shaders[j]->Type == GL_FRAGMENT_SHADER);
               assert(shProg->Shaders[j]->RefCount > 0);
            }
         }
#endif

         return;
      }
   }

   /* not found */
   _mesa_error(ctx, GL_INVALID_VALUE,
               "glDetachShader(shader not found)");
}


static void
_mesa_get_active_attrib(GLcontext *ctx, GLuint program, GLuint index,
                        GLsizei maxLength, GLsizei *length, GLint *size,
                        GLenum *type, GLchar *nameOut)
{
   static const GLenum vec_types[] = {
      GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4
   };
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   GLint sz;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib");
      return;
   }

   if (!shProg->Attributes || index >= shProg->Attributes->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
      return;
   }

   copy_string(nameOut, maxLength, length,
               shProg->Attributes->Parameters[index].Name);
   sz = shProg->Attributes->Parameters[index].Size;
   if (size)
      *size = sz;
   if (type)
      *type = vec_types[sz]; /* XXX this is a temporary hack */
}


/**
 * Called via ctx->Driver.GetActiveUniform().
 */
static void
_mesa_get_active_uniform(GLcontext *ctx, GLuint program, GLuint index,
                         GLsizei maxLength, GLsizei *length, GLint *size,
                         GLenum *type, GLchar *nameOut)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   GLuint ind, j;

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform");
      return;
   }

   if (!shProg->Uniforms || index >= shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
      return;
   }

   ind = 0;
   for (j = 0; j < shProg->Uniforms->NumParameters; j++) {
      if (shProg->Uniforms->Parameters[j].Type == PROGRAM_UNIFORM ||
          shProg->Uniforms->Parameters[j].Type == PROGRAM_SAMPLER) {
         if (ind == index) {
            /* found it */
            copy_string(nameOut, maxLength, length,
                        shProg->Uniforms->Parameters[j].Name);
            if (size)
               *size = shProg->Uniforms->Parameters[j].Size;
            if (type)
               *type = shProg->Uniforms->Parameters[j].DataType;
            return;
         }
         ind++;
      }
   }

   _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveUniform(index)");
}


/**
 * Called via ctx->Driver.GetAttachedShaders().
 */
static void
_mesa_get_attached_shaders(GLcontext *ctx, GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *obj)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (shProg) {
      GLint i;
      for (i = 0; i < maxCount && i < shProg->NumShaders; i++) {
         obj[i] = shProg->Shaders[i]->Name;
      }
      if (count)
         *count = i;
   }
   else {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttachedShaders");
   }
}


static GLuint
_mesa_get_handle(GLcontext *ctx, GLenum pname)
{
#if 0
   GET_CURRENT_CONTEXT(ctx);

   switch (pname) {
   case GL_PROGRAM_OBJECT_ARB:
      {
         struct gl2_program_intf **pro = ctx->Shader.CurrentProgram;

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


static void
_mesa_get_programiv(GLcontext *ctx, GLuint program,
                    GLenum pname, GLint *params)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramiv(program)");
      return;
   }

   switch (pname) {
   case GL_DELETE_STATUS:
      *params = shProg->DeletePending;
      break; 
   case GL_LINK_STATUS:
      *params = shProg->LinkStatus;
      break;
   case GL_VALIDATE_STATUS:
      *params = shProg->Validated;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shProg->InfoLog ? strlen(shProg->InfoLog) + 1 : 0;
      break;
   case GL_ATTACHED_SHADERS:
      *params = shProg->NumShaders;
      break;
   case GL_ACTIVE_ATTRIBUTES:
      *params = shProg->Attributes ? shProg->Attributes->NumParameters : 0;
      break;
   case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = _mesa_longest_parameter_name(shProg->Attributes,
                                             PROGRAM_INPUT) + 1;
      break;
   case GL_ACTIVE_UNIFORMS:
      *params
         = _mesa_num_parameters_of_type(shProg->Uniforms, PROGRAM_UNIFORM)
         + _mesa_num_parameters_of_type(shProg->Uniforms, PROGRAM_SAMPLER);
      break;
   case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      *params = MAX2(
             _mesa_longest_parameter_name(shProg->Uniforms, PROGRAM_UNIFORM),
             _mesa_longest_parameter_name(shProg->Uniforms, PROGRAM_SAMPLER));
      if (*params > 0)
         (*params)++;  /* add one for terminating zero */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname)");
      return;
   }
}


static void
_mesa_get_shaderiv(GLcontext *ctx, GLuint name, GLenum pname, GLint *params)
{
   struct gl_shader *shader = _mesa_lookup_shader(ctx, name);

   if (!shader) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderiv(shader)");
      return;
   }

   switch (pname) {
   case GL_SHADER_TYPE:
      *params = shader->Type;
      break;
   case GL_DELETE_STATUS:
      *params = shader->DeletePending;
      break;
   case GL_COMPILE_STATUS:
      *params = shader->CompileStatus;
      break;
   case GL_INFO_LOG_LENGTH:
      *params = shader->InfoLog ? strlen(shader->InfoLog) + 1 : 0;
      break;
   case GL_SHADER_SOURCE_LENGTH:
      *params = shader->Source ? strlen((char *) shader->Source) + 1 : 0;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetShaderiv(pname)");
      return;
   }
}


static void
_mesa_get_program_info_log(GLcontext *ctx, GLuint program, GLsizei bufSize,
                           GLsizei *length, GLchar *infoLog)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramInfoLog(program)");
      return;
   }
   copy_string(infoLog, bufSize, length, shProg->InfoLog);
}


static void
_mesa_get_shader_info_log(GLcontext *ctx, GLuint shader, GLsizei bufSize,
                          GLsizei *length, GLchar *infoLog)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderInfoLog(shader)");
      return;
   }
   copy_string(infoLog, bufSize, length, sh->InfoLog);
}


/**
 * Called via ctx->Driver.GetShaderSource().
 */
static void
_mesa_get_shader_source(GLcontext *ctx, GLuint shader, GLsizei maxLength,
                        GLsizei *length, GLchar *sourceOut)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderSource(shader)");
      return;
   }
   copy_string(sourceOut, maxLength, length, sh->Source);
}


/**
 * Called via ctx->Driver.GetUniformfv().
 */
static void
_mesa_get_uniformfv(GLcontext *ctx, GLuint program, GLint location,
                    GLfloat *params)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (shProg) {
      GLint i;
      if (location >= 0 && location < shProg->Uniforms->NumParameters) {
         for (i = 0; i < shProg->Uniforms->Parameters[location].Size; i++) {
            params[i] = shProg->Uniforms->ParameterValues[location][i];
         }
      }
      else {
         _mesa_error(ctx, GL_INVALID_VALUE, "glGetUniformfv(location)");
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetUniformfv(program)");
   }
}


/**
 * Called via ctx->Driver.GetUniformLocation().
 */
static GLint
_mesa_get_uniform_location(GLcontext *ctx, GLuint program, const GLchar *name)
{
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);
   if (shProg) {
      GLuint loc;
      for (loc = 0; loc < shProg->Uniforms->NumParameters; loc++) {
         const struct gl_program_parameter *u
            = shProg->Uniforms->Parameters + loc;
         /* XXX this is a temporary simplification / short-cut.
          * We need to handle things like "e.c[0].b" as seen in the
          * GLSL orange book, page 189.
          */
         if ((u->Type == PROGRAM_UNIFORM ||
              u->Type == PROGRAM_SAMPLER) && !strcmp(u->Name, name)) {
            return loc;
         }
      }
   }
   return -1;

}


static GLboolean
_mesa_is_program(GLcontext *ctx, GLuint name)
{
   struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, name);
   return shProg ? GL_TRUE : GL_FALSE;
}


static GLboolean
_mesa_is_shader(GLcontext *ctx, GLuint name)
{
   struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
   return shader ? GL_TRUE : GL_FALSE;
}



/**
 * Called via ctx->Driver.ShaderSource()
 */
static void
_mesa_shader_source(GLcontext *ctx, GLuint shader, const GLchar *source)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSource(shaderObj)");
      return;
   }

   /* free old shader source string and install new one */
   if (sh->Source) {
      _mesa_free((void *) sh->Source);
   }
   sh->Source = source;
   sh->CompileStatus = GL_FALSE;
}


/**
 * Called via ctx->Driver.CompileShader()
 */
static void
_mesa_compile_shader(GLcontext *ctx, GLuint shaderObj)
{
   struct gl_shader *sh = _mesa_lookup_shader(ctx, shaderObj);

   if (!sh) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCompileShader(shaderObj)");
      return;
   }

   sh->CompileStatus = _slang_compile(ctx, sh);
}


/**
 * Called via ctx->Driver.LinkProgram()
 */
static void
_mesa_link_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glLinkProgram(program)");
      return;
   }

   _slang_link(ctx, program, shProg);
}


/**
 * Called via ctx->Driver.UseProgram()
 */
void
_mesa_use_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;

   if (ctx->Shader.CurrentProgram &&
       ctx->Shader.CurrentProgram->Name == program) {
      /* no-op */
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (program) {
      shProg = _mesa_lookup_shader_program(ctx, program);
      if (!shProg) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glUseProgramObjectARB(programObj)");
         return;
      }
   }
   else {
      shProg = NULL;
   }

   _mesa_reference_shader_program(ctx, &ctx->Shader.CurrentProgram, shProg);
}



/**
 * Update the vertex and fragment program's TexturesUsed arrays.
 */
static void
update_textures_used(struct gl_program *prog)
{
   GLuint s;

   memset(prog->TexturesUsed, 0, sizeof(prog->TexturesUsed));

   for (s = 0; s < MAX_SAMPLERS; s++) {
      if (prog->SamplersUsed & (1 << s)) {
         GLuint u = prog->SamplerUnits[s];
         GLuint t = prog->SamplerTargets[s];
         assert(u < MAX_TEXTURE_IMAGE_UNITS);
         prog->TexturesUsed[u] |= (1 << t);
      }
   }
}


/**
 * Called via ctx->Driver.Uniform().
 */
static void
_mesa_uniform(GLcontext *ctx, GLint location, GLsizei count,
              const GLvoid *values, GLenum type)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   GLint elems, i, k;

   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(program not linked)");
      return;
   }

   if (location < 0 || location >= (GLint) shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(location)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniform(count < 0)");
      return;
   }

   switch (type) {
   case GL_FLOAT:
   case GL_INT:
      elems = 1;
      break;
   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
      elems = 2;
      break;
   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
      elems = 3;
      break;
   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
      elems = 4;
      break;
   default:
      _mesa_problem(ctx, "Invalid type in _mesa_uniform");
      return;
   }

   if (count * elems > shProg->Uniforms->Parameters[location].Size) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUniform(count too large)");
      return;
   }

   if (shProg->Uniforms->Parameters[location].Type == PROGRAM_SAMPLER) {
      /* This controls which texture unit which is used by a sampler */
      GLuint texUnit, sampler;

      /* data type for setting samplers must be int */
      if (type != GL_INT || count != 1) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUniform(only glUniform1i can be used "
                     "to set sampler uniforms)");
         return;
      }

      sampler = (GLuint) shProg->Uniforms->ParameterValues[location][0];
      texUnit = ((GLuint *) values)[0];

      /* check that the sampler (tex unit index) is legal */
      if (texUnit >= ctx->Const.MaxTextureImageUnits) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glUniform1(invalid sampler/tex unit index)");
         return;
      }

      if (shProg->VertexProgram) {
         shProg->VertexProgram->Base.SamplerUnits[sampler] = texUnit;
         update_textures_used(&shProg->VertexProgram->Base);
      }
      if (shProg->FragmentProgram) {
         shProg->FragmentProgram->Base.SamplerUnits[sampler] = texUnit;
         update_textures_used(&shProg->FragmentProgram->Base);
      }


      FLUSH_VERTICES(ctx, _NEW_TEXTURE);
   }
   else {
      /* ordinary uniform variable */
      for (k = 0; k < count; k++) {
         GLfloat *uniformVal = shProg->Uniforms->ParameterValues[location + k];
         if (type == GL_INT ||
             type == GL_INT_VEC2 ||
             type == GL_INT_VEC3 ||
             type == GL_INT_VEC4) {
            const GLint *iValues = ((const GLint *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = (GLfloat) iValues[i];
            }
         }
         else {
            const GLfloat *fValues = ((const GLfloat *) values) + k * elems;
            for (i = 0; i < elems; i++) {
               uniformVal[i] = fValues[i];
            }
         }
      }
   }
}


/**
 * Called by ctx->Driver.UniformMatrix().
 */
static void
_mesa_uniform_matrix(GLcontext *ctx, GLint cols, GLint rows,
                     GLenum matrixType, GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   if (!shProg || !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         "glUniformMatrix(program not linked)");
      return;
   }
   if (location < 0 || location >= shProg->Uniforms->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix(location)");
      return;
   }
   if (values == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUniformMatrix");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   /*
    * Note: the _columns_ of a matrix are stored in program registers, not
    * the rows.
    */
   /* XXXX need to test 3x3 and 2x2 matrices... */
   if (transpose) {
      GLuint row, col;
      for (col = 0; col < cols; col++) {
         GLfloat *v = shProg->Uniforms->ParameterValues[location + col];
         for (row = 0; row < rows; row++) {
            v[row] = values[row * cols + col];
         }
      }
   }
   else {
      GLuint row, col;
      for (col = 0; col < cols; col++) {
         GLfloat *v = shProg->Uniforms->ParameterValues[location + col];
         for (row = 0; row < rows; row++) {
            v[row] = values[col * rows + row];
         }
      }
   }
}


static void
_mesa_validate_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;
   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glValidateProgram(program)");
      return;
   }
   /* XXX temporary */
   shProg->Validated = GL_TRUE;

   /* From the GL spec:
     any two active samplers in the current program object are of
     different types, but refer to the same texture image unit,

     any active sampler in the current program object refers to a texture
     image unit where fixed-function fragment processing accesses a
     texture target that does not match the sampler type, or 

     the sum of the number of active samplers in the program and the
     number of texture image units enabled for fixed-function fragment
     processing exceeds the combined limit on the total number of texture
     image units allowed.
   */
}


/**
 * Plug in Mesa's GLSL functions into the device driver function table.
 */
void
_mesa_init_glsl_driver_functions(struct dd_function_table *driver)
{
   driver->AttachShader = _mesa_attach_shader;
   driver->BindAttribLocation = _mesa_bind_attrib_location;
   driver->CompileShader = _mesa_compile_shader;
   driver->CreateProgram = _mesa_create_program;
   driver->CreateShader = _mesa_create_shader;
   driver->DeleteProgram2 = _mesa_delete_program2;
   driver->DeleteShader = _mesa_delete_shader;
   driver->DetachShader = _mesa_detach_shader;
   driver->GetActiveAttrib = _mesa_get_active_attrib;
   driver->GetActiveUniform = _mesa_get_active_uniform;
   driver->GetAttachedShaders = _mesa_get_attached_shaders;
   driver->GetAttribLocation = _mesa_get_attrib_location;
   driver->GetHandle = _mesa_get_handle;
   driver->GetProgramiv = _mesa_get_programiv;
   driver->GetProgramInfoLog = _mesa_get_program_info_log;
   driver->GetShaderiv = _mesa_get_shaderiv;
   driver->GetShaderInfoLog = _mesa_get_shader_info_log;
   driver->GetShaderSource = _mesa_get_shader_source;
   driver->GetUniformfv = _mesa_get_uniformfv;
   driver->GetUniformLocation = _mesa_get_uniform_location;
   driver->IsProgram = _mesa_is_program;
   driver->IsShader = _mesa_is_shader;
   driver->LinkProgram = _mesa_link_program;
   driver->ShaderSource = _mesa_shader_source;
   driver->Uniform = _mesa_uniform;
   driver->UniformMatrix = _mesa_uniform_matrix;
   driver->UseProgram = _mesa_use_program;
   driver->ValidateProgram = _mesa_validate_program;
}
