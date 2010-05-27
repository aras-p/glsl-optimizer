/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


#include "main/glheader.h"
#include "main/context.h"
#include "main/hash.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_uniform.h"
#include "shader/shader_api.h"
#include "shader/uniforms.h"
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
   _mesa_reference_vertprog(ctx, &shProg->VertexProgram, NULL);
   _mesa_reference_fragprog(ctx, &shProg->FragmentProgram, NULL);

   if (shProg->Uniforms) {
      _mesa_free_uniform_list(shProg->Uniforms);
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
   shProg->NumShaders = 0;

   if (shProg->Shaders) {
      free(shProg->Shaders);
      shProg->Shaders = NULL;
   }

   if (shProg->InfoLog) {
      free(shProg->InfoLog);
      shProg->InfoLog = NULL;
   }

   /* Transform feedback varying vars */
   for (i = 0; i < shProg->TransformFeedback.NumVarying; i++) {
      free(shProg->TransformFeedback.VaryingNames[i]);
   }
   free(shProg->TransformFeedback.VaryingNames);
   shProg->TransformFeedback.VaryingNames = NULL;
   shProg->TransformFeedback.NumVarying = 0;
}


/**
 * Free/delete a shader program object.
 */
void
_mesa_free_shader_program(GLcontext *ctx, struct gl_shader_program *shProg)
{
   _mesa_free_shader_program_data(ctx, shProg);

   free(shProg);
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
#if 0
      printf("ShaderProgram %p ID=%u  RefCount-- to %d\n",
             (void *) old, old->Name, old->RefCount);
#endif
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
#if 0
      printf("ShaderProgram %p ID=%u  RefCount++ to %d\n",
             (void *) shProg, shProg->Name, shProg->RefCount);
#endif
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
 * As above, but record an error if program is not found.
 */
struct gl_shader_program *
_mesa_lookup_shader_program_err(GLcontext *ctx, GLuint name,
                                const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, caller);
      return NULL;
   }
   else {
      struct gl_shader_program *shProg = (struct gl_shader_program *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!shProg) {
         _mesa_error(ctx, GL_INVALID_VALUE, caller);
         return NULL;
      }
      if (shProg->Type != GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
         return NULL;
      }
      return shProg;
   }
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
   if (sh->Source)
      free((void *) sh->Source);
   if (sh->InfoLog)
      free(sh->InfoLog);
   _mesa_reference_program(ctx, &sh->Program, NULL);
   free(sh);
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
 * As above, but record an error if shader is not found.
 */
static struct gl_shader *
_mesa_lookup_shader_err(GLcontext *ctx, GLuint name, const char *caller)
{
   if (!name) {
      _mesa_error(ctx, GL_INVALID_VALUE, caller);
      return NULL;
   }
   else {
      struct gl_shader *sh = (struct gl_shader *)
         _mesa_HashLookup(ctx->Shared->ShaderObjects, name);
      if (!sh) {
         _mesa_error(ctx, GL_INVALID_VALUE, caller);
         return NULL;
      }
      if (sh->Type == GL_SHADER_PROGRAM_MESA) {
         _mesa_error(ctx, GL_INVALID_OPERATION, caller);
         return NULL;
      }
      return sh;
   }
}


/**
 * Return mask of GLSL_x flags by examining the MESA_GLSL env var.
 */
static GLbitfield
get_shader_flags(void)
{
   GLbitfield flags = 0x0;
   const char *env = _mesa_getenv("MESA_GLSL");

   if (env) {
      if (strstr(env, "dump"))
         flags |= GLSL_DUMP;
      if (strstr(env, "log"))
         flags |= GLSL_LOG;
      if (strstr(env, "nopvert"))
         flags |= GLSL_NOP_VERT;
      if (strstr(env, "nopfrag"))
         flags |= GLSL_NOP_FRAG;
      if (strstr(env, "nopt"))
         flags |= GLSL_NO_OPT;
      else if (strstr(env, "opt"))
         flags |= GLSL_OPT;
      if (strstr(env, "uniform"))
         flags |= GLSL_UNIFORMS;
      if (strstr(env, "useprog"))
         flags |= GLSL_USE_PROG;
   }

   return flags;
}


/**
 * Find the length of the longest transform feedback varying name
 * which was specified with glTransformFeedbackVaryings().
 */
static GLint
longest_feedback_varying_name(const struct gl_shader_program *shProg)
{
   GLuint i;
   GLint max = 0;
   for (i = 0; i < shProg->TransformFeedback.NumVarying; i++) {
      GLint len = strlen(shProg->TransformFeedback.VaryingNames[i]);
      if (len > max)
         max = len;
   }
   return max;
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
   ctx->Shader.EmitContReturn = GL_TRUE;
   ctx->Shader.EmitCondCodes = GL_FALSE;
   ctx->Shader.EmitComments = GL_FALSE;
   ctx->Shader.Flags = get_shader_flags();

   /* Default pragma settings */
   ctx->Shader.DefaultPragmas.IgnoreOptimize = GL_FALSE;
   ctx->Shader.DefaultPragmas.IgnoreDebug = GL_FALSE;
   ctx->Shader.DefaultPragmas.Optimize = GL_TRUE;
   ctx->Shader.DefaultPragmas.Debug = GL_FALSE;
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
void
_mesa_copy_string(GLchar *dst, GLsizei maxLength,
                  GLsizei *length, const GLchar *src)
{
   GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
      dst[len] = src[len];
   if (maxLength > 0)
      dst[len] = 0;
   if (length)
      *length = len;
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
 * Called via ctx->Driver.AttachShader()
 */
static void
_mesa_attach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg;
   struct gl_shader *sh;
   GLuint i, n;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glAttachShader");
   if (!shProg)
      return;

   sh = _mesa_lookup_shader_err(ctx, shader, "glAttachShader");
   if (!sh) {
      return;
   }

   n = shProg->NumShaders;
   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i] == sh) {
         /* The shader is already attched to this program.  The
          * GL_ARB_shader_objects spec says:
          *
          *     "The error INVALID_OPERATION is generated by AttachObjectARB
          *     if <obj> is already attached to <containerObj>."
          */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glAttachShader");
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
      = _mesa_lookup_shader_program_err(ctx, program, "glGetAttribLocation");

   if (!shProg) {
      return -1;
   }

   if (!shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetAttribLocation(program not linked)");
      return -1;
   }

   if (!name)
      return -1;

   if (shProg->VertexProgram) {
      const struct gl_program_parameter_list *attribs =
         shProg->VertexProgram->Base.Attributes;
      if (attribs) {
         GLint i = _mesa_lookup_parameter_index(attribs, -1, name);
         if (i >= 0) {
            return attribs->Parameters[i].StateIndexes[0];
         }
      }
   }
   return -1;
}


static void
_mesa_bind_attrib_location(GLcontext *ctx, GLuint program, GLuint index,
                           const GLchar *name)
{
   struct gl_shader_program *shProg;
   const GLint size = -1; /* unknown size */
   GLint i, oldIndex;
   GLenum datatype = GL_FLOAT_VEC4;

   shProg = _mesa_lookup_shader_program_err(ctx, program,
                                            "glBindAttribLocation");
   if (!shProg) {
      return;
   }

   if (!name)
      return;

   if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindAttribLocation(illegal name)");
      return;
   }

   if (index >= ctx->Const.VertexProgram.MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(index)");
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
   i = _mesa_add_attribute(shProg->Attributes, name, size, datatype, index);
   if (i < 0) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindAttribLocation");
      return;
   }

   /*
    * Note that this attribute binding won't go into effect until
    * glLinkProgram is called again.
    */
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

   shProg = _mesa_lookup_shader_program_err(ctx, name, "glDeleteProgram");
   if (!shProg)
      return;

   shProg->DeletePending = GL_TRUE;

   /* effectively, decr shProg's refcount */
   _mesa_reference_shader_program(ctx, &shProg, NULL);
}


static void
_mesa_delete_shader(GLcontext *ctx, GLuint shader)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shader, "glDeleteShader");
   if (!sh)
      return;

   sh->DeletePending = GL_TRUE;

   /* effectively, decr sh's refcount */
   _mesa_reference_shader(ctx, &sh, NULL);
}


static void
_mesa_detach_shader(GLcontext *ctx, GLuint program, GLuint shader)
{
   struct gl_shader_program *shProg;
   GLuint n;
   GLuint i, j;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glDetachShader");
   if (!shProg)
      return;

   n = shProg->NumShaders;

   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i]->Name == shader) {
         /* found it */
         struct gl_shader **newList;

         /* release */
         _mesa_reference_shader(ctx, &shProg->Shaders[i], NULL);

         /* alloc new, smaller array */
         newList = (struct gl_shader **)
            malloc((n - 1) * sizeof(struct gl_shader *));
         if (!newList) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachShader");
            return;
         }
         for (j = 0; j < i; j++) {
            newList[j] = shProg->Shaders[j];
         }
         while (++i < n)
            newList[j++] = shProg->Shaders[i];
         free(shProg->Shaders);

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
   {
      GLenum err;
      if (_mesa_is_shader(ctx, shader))
         err = GL_INVALID_OPERATION;
      else if (_mesa_is_program(ctx, shader))
         err = GL_INVALID_OPERATION;
      else
         err = GL_INVALID_VALUE;
      _mesa_error(ctx, err, "glDetachProgram(shader)");
      return;
   }
}


/**
 * Return the size of the given GLSL datatype, in floats (components).
 */
GLint
_mesa_sizeof_glsl_type(GLenum type)
{
   switch (type) {
   case GL_FLOAT:
   case GL_INT:
   case GL_BOOL:
   case GL_SAMPLER_1D:
   case GL_SAMPLER_2D:
   case GL_SAMPLER_3D:
   case GL_SAMPLER_CUBE:
   case GL_SAMPLER_1D_SHADOW:
   case GL_SAMPLER_2D_SHADOW:
   case GL_SAMPLER_2D_RECT_ARB:
   case GL_SAMPLER_2D_RECT_SHADOW_ARB:
   case GL_SAMPLER_1D_ARRAY_EXT:
   case GL_SAMPLER_2D_ARRAY_EXT:
   case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
   case GL_SAMPLER_CUBE_SHADOW_EXT:
      return 1;
   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
   case GL_UNSIGNED_INT_VEC2:
   case GL_BOOL_VEC2:
      return 2;
   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
   case GL_UNSIGNED_INT_VEC3:
   case GL_BOOL_VEC3:
      return 3;
   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
   case GL_UNSIGNED_INT_VEC4:
   case GL_BOOL_VEC4:
      return 4;
   case GL_FLOAT_MAT2:
   case GL_FLOAT_MAT2x3:
   case GL_FLOAT_MAT2x4:
      return 8; /* two float[4] vectors */
   case GL_FLOAT_MAT3:
   case GL_FLOAT_MAT3x2:
   case GL_FLOAT_MAT3x4:
      return 12; /* three float[4] vectors */
   case GL_FLOAT_MAT4:
   case GL_FLOAT_MAT4x2:
   case GL_FLOAT_MAT4x3:
      return 16;  /* four float[4] vectors */
   default:
      _mesa_problem(NULL, "Invalid type in _mesa_sizeof_glsl_type()");
      return 1;
   }
}


static void
_mesa_get_active_attrib(GLcontext *ctx, GLuint program, GLuint index,
                        GLsizei maxLength, GLsizei *length, GLint *size,
                        GLenum *type, GLchar *nameOut)
{
   const struct gl_program_parameter_list *attribs = NULL;
   struct gl_shader_program *shProg;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
      return;

   if (shProg->VertexProgram)
      attribs = shProg->VertexProgram->Base.Attributes;

   if (!attribs || index >= attribs->NumParameters) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
      return;
   }

   _mesa_copy_string(nameOut, maxLength, length,
                     attribs->Parameters[index].Name);

   if (size)
      *size = attribs->Parameters[index].Size
         / _mesa_sizeof_glsl_type(attribs->Parameters[index].DataType);

   if (type)
      *type = attribs->Parameters[index].DataType;
}


/**
 * Called via ctx->Driver.GetAttachedShaders().
 */
static void
_mesa_get_attached_shaders(GLcontext *ctx, GLuint program, GLsizei maxCount,
                           GLsizei *count, GLuint *obj)
{
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program, "glGetAttachedShaders");
   if (shProg) {
      GLuint i;
      for (i = 0; i < (GLuint) maxCount && i < shProg->NumShaders; i++) {
         obj[i] = shProg->Shaders[i]->Name;
      }
      if (count)
         *count = i;
   }
}


/** glGetHandleARB() - return ID/name of currently bound shader program */
static GLuint
_mesa_get_handle(GLcontext *ctx, GLenum pname)
{
   if (pname == GL_PROGRAM_OBJECT_ARB) {
      if (ctx->Shader.CurrentProgram)
         return ctx->Shader.CurrentProgram->Name;
      else
         return 0;
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHandleARB");
      return 0;
   }
}


/**
 * glGetProgramiv() - get shader program state.
 * Note that this is for GLSL shader programs, not ARB vertex/fragment
 * programs (see glGetProgramivARB).
 */
static void
_mesa_get_programiv(GLcontext *ctx, GLuint program,
                    GLenum pname, GLint *params)
{
   const struct gl_program_parameter_list *attribs;
   struct gl_shader_program *shProg
      = _mesa_lookup_shader_program(ctx, program);

   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramiv(program)");
      return;
   }

   if (shProg->VertexProgram)
      attribs = shProg->VertexProgram->Base.Attributes;
   else
      attribs = NULL;

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
      *params = attribs ? attribs->NumParameters : 0;
      break;
   case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = _mesa_longest_parameter_name(attribs, PROGRAM_INPUT) + 1;
      break;
   case GL_ACTIVE_UNIFORMS:
      *params = shProg->Uniforms ? shProg->Uniforms->NumUniforms : 0;
      break;
   case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      *params = _mesa_longest_uniform_name(shProg->Uniforms);
      if (*params > 0)
         (*params)++;  /* add one for terminating zero */
      break;
   case GL_PROGRAM_BINARY_LENGTH_OES:
      *params = 0;
      break;
#if FEATURE_EXT_transform_feedback
   case GL_TRANSFORM_FEEDBACK_VARYINGS:
      *params = shProg->TransformFeedback.NumVarying;
      break;
   case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH:
      *params = longest_feedback_varying_name(shProg) + 1;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
      *params = shProg->TransformFeedback.BufferMode;
      break;
#endif
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname)");
      return;
   }
}


/** glGetShaderiv() - get GLSL shader state */
static void
_mesa_get_shaderiv(GLcontext *ctx, GLuint name, GLenum pname, GLint *params)
{
   struct gl_shader *shader = _mesa_lookup_shader_err(ctx, name, "glGetShaderiv");

   if (!shader) {
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
   _mesa_copy_string(infoLog, bufSize, length, shProg->InfoLog);
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
   _mesa_copy_string(infoLog, bufSize, length, sh->InfoLog);
}


/**
 * Called via ctx->Driver.GetShaderSource().
 */
static void
_mesa_get_shader_source(GLcontext *ctx, GLuint shader, GLsizei maxLength,
                        GLsizei *length, GLchar *sourceOut)
{
   struct gl_shader *sh;
   sh = _mesa_lookup_shader_err(ctx, shader, "glGetShaderSource");
   if (!sh) {
      return;
   }
   _mesa_copy_string(sourceOut, maxLength, length, sh->Source);
}


/**
 * Called via ctx->Driver.ShaderSource()
 */
static void
_mesa_shader_source(GLcontext *ctx, GLuint shader, const GLchar *source)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shader, "glShaderSource");
   if (!sh)
      return;

   /* free old shader source string and install new one */
   if (sh->Source) {
      free((void *) sh->Source);
   }
   sh->Source = source;
   sh->CompileStatus = GL_FALSE;
#ifdef DEBUG
   sh->SourceChecksum = _mesa_str_checksum(sh->Source);
#endif
}


/**
 * Called via ctx->Driver.CompileShader()
 */
static void
_mesa_compile_shader(GLcontext *ctx, GLuint shaderObj)
{
   struct gl_shader *sh;

   sh = _mesa_lookup_shader_err(ctx, shaderObj, "glCompileShader");
   if (!sh)
      return;

   /* set default pragma state for shader */
   sh->Pragmas = ctx->Shader.DefaultPragmas;

   /* this call will set the sh->CompileStatus field to indicate if
    * compilation was successful.
    */
   (void) _slang_compile(ctx, sh);
}


/**
 * Called via ctx->Driver.LinkProgram()
 */
static void
_mesa_link_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;
   struct gl_transform_feedback_object *obj =
      ctx->TransformFeedback.CurrentObject;

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glLinkProgram");
   if (!shProg)
      return;

   if (obj->Active && shProg == ctx->Shader.CurrentProgram) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glLinkProgram(transform feedback active");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);

   _slang_link(ctx, program, shProg);

   /* debug code */
   if (0) {
      GLuint i;

      printf("Link %u shaders in program %u: %s\n",
                   shProg->NumShaders, shProg->Name,
                   shProg->LinkStatus ? "Success" : "Failed");

      for (i = 0; i < shProg->NumShaders; i++) {
         printf(" shader %u, type 0x%x\n",
                      shProg->Shaders[i]->Name,
                      shProg->Shaders[i]->Type);
      }
   }
}


/**
 * Print basic shader info (for debug).
 */
static void
print_shader_info(const struct gl_shader_program *shProg)
{
   GLuint i;

   printf("Mesa: glUseProgram(%u)\n", shProg->Name);
   for (i = 0; i < shProg->NumShaders; i++) {
      const char *s;
      switch (shProg->Shaders[i]->Type) {
      case GL_VERTEX_SHADER:
         s = "vertex";
         break;
      case GL_FRAGMENT_SHADER:
         s = "fragment";
         break;
      case GL_GEOMETRY_SHADER:
         s = "geometry";
         break;
      default:
         s = "";
      }
      printf("  %s shader %u, checksum %u\n", s, 
	     shProg->Shaders[i]->Name,
	     shProg->Shaders[i]->SourceChecksum);
   }
   if (shProg->VertexProgram)
      printf("  vert prog %u\n", shProg->VertexProgram->Base.Id);
   if (shProg->FragmentProgram)
      printf("  frag prog %u\n", shProg->FragmentProgram->Base.Id);
}


/**
 * Called via ctx->Driver.UseProgram()
 */
void
_mesa_use_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;
   struct gl_transform_feedback_object *obj =
      ctx->TransformFeedback.CurrentObject;

   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glUseProgram(transform feedback active)");
      return;
   }

   if (ctx->Shader.CurrentProgram &&
       ctx->Shader.CurrentProgram->Name == program) {
      /* no-op */
      return;
   }

   if (program) {
      shProg = _mesa_lookup_shader_program_err(ctx, program, "glUseProgram");
      if (!shProg) {
         return;
      }
      if (!shProg->LinkStatus) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUseProgram(program %u not linked)", program);
         return;
      }

      /* debug code */
      if (ctx->Shader.Flags & GLSL_USE_PROG) {
         print_shader_info(shProg);
      }
   }
   else {
      shProg = NULL;
   }

   if (ctx->Shader.CurrentProgram != shProg) {
      FLUSH_VERTICES(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);
      _mesa_reference_shader_program(ctx, &ctx->Shader.CurrentProgram, shProg);
   }
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
 * Validate a program's samplers.
 * Specifically, check that there aren't two samplers of different types
 * pointing to the same texture unit.
 * \return GL_TRUE if valid, GL_FALSE if invalid
 */
static GLboolean
validate_samplers(GLcontext *ctx, const struct gl_program *prog, char *errMsg)
{
   static const char *targetName[] = {
      "TEXTURE_2D_ARRAY",
      "TEXTURE_1D_ARRAY",
      "TEXTURE_CUBE",
      "TEXTURE_3D",
      "TEXTURE_RECT",
      "TEXTURE_2D",
      "TEXTURE_1D",
   };
   GLint targetUsed[MAX_TEXTURE_IMAGE_UNITS];
   GLbitfield samplersUsed = prog->SamplersUsed;
   GLuint i;

   assert(Elements(targetName) == NUM_TEXTURE_TARGETS);

   if (samplersUsed == 0x0)
      return GL_TRUE;

   for (i = 0; i < Elements(targetUsed); i++)
      targetUsed[i] = -1;

   /* walk over bits which are set in 'samplers' */
   while (samplersUsed) {
      GLuint unit;
      gl_texture_index target;
      GLint sampler = _mesa_ffs(samplersUsed) - 1;
      assert(sampler >= 0);
      assert(sampler < MAX_TEXTURE_IMAGE_UNITS);
      unit = prog->SamplerUnits[sampler];
      target = prog->SamplerTargets[sampler];
      if (targetUsed[unit] != -1 && targetUsed[unit] != target) {
         _mesa_snprintf(errMsg, 100,
		  "Texture unit %d is accessed both as %s and %s",
		  unit, targetName[targetUsed[unit]], targetName[target]);
         return GL_FALSE;
      }
      targetUsed[unit] = target;
      samplersUsed ^= (1 << sampler);
   }

   return GL_TRUE;
}


/**
 * Do validation of the given shader program.
 * \param errMsg  returns error message if validation fails.
 * \return GL_TRUE if valid, GL_FALSE if invalid (and set errMsg)
 */
GLboolean
_mesa_validate_shader_program(GLcontext *ctx,
                              const struct gl_shader_program *shProg,
                              char *errMsg)
{
   const struct gl_vertex_program *vp = shProg->VertexProgram;
   const struct gl_fragment_program *fp = shProg->FragmentProgram;

   if (!shProg->LinkStatus) {
      return GL_FALSE;
   }

   /* From the GL spec, a program is invalid if any of these are true:

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


   /*
    * Check: any two active samplers in the current program object are of
    * different types, but refer to the same texture image unit,
    */
   if (vp && !validate_samplers(ctx, &vp->Base, errMsg)) {
      return GL_FALSE;
   }
   if (fp && !validate_samplers(ctx, &fp->Base, errMsg)) {
      return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Called via glValidateProgram()
 */
static void
_mesa_validate_program(GLcontext *ctx, GLuint program)
{
   struct gl_shader_program *shProg;
   char errMsg[100];

   shProg = _mesa_lookup_shader_program_err(ctx, program, "glValidateProgram");
   if (!shProg) {
      return;
   }

   shProg->Validated = _mesa_validate_shader_program(ctx, shProg, errMsg);
   if (!shProg->Validated) {
      /* update info log */
      if (shProg->InfoLog) {
         free(shProg->InfoLog);
      }
      shProg->InfoLog = _mesa_strdup(errMsg);
   }
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
   driver->GetAttachedShaders = _mesa_get_attached_shaders;
   driver->GetAttribLocation = _mesa_get_attrib_location;
   driver->GetHandle = _mesa_get_handle;
   driver->GetProgramiv = _mesa_get_programiv;
   driver->GetProgramInfoLog = _mesa_get_program_info_log;
   driver->GetShaderiv = _mesa_get_shaderiv;
   driver->GetShaderInfoLog = _mesa_get_shader_info_log;
   driver->GetShaderSource = _mesa_get_shader_source;
   driver->IsProgram = _mesa_is_program;
   driver->IsShader = _mesa_is_shader;
   driver->LinkProgram = _mesa_link_program;
   driver->ShaderSource = _mesa_shader_source;
   driver->UseProgram = _mesa_use_program;
   driver->ValidateProgram = _mesa_validate_program;

   _mesa_init_uniform_functions(driver);
}
