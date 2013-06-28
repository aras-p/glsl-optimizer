/*
 * Mesa 3-D graphics library
 *
 * Copyright © 2013 Gregory Hainaut <gregory.hainaut@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file pipelineobj.c
 * \author Hainaut Gregory <gregory.hainaut@gmail.com>
 *
 * Implementation of pipeline object related API functions. Based on
 * GL_ARB_separate_shader_objects extension.
 */

#include "main/glheader.h"
#include "main/context.h"
#include "main/dispatch.h"
#include "main/enums.h"
#include "main/hash.h"
#include "main/mtypes.h"
#include "main/pipelineobj.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/transformfeedback.h"
#include "main/uniforms.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "ralloc.h"
#include <stdbool.h>
#include "../glsl/glsl_parser_extras.h"
#include "../glsl/ir_uniform.h"

/**
 * Delete a pipeline object.
 */
void
_mesa_delete_pipeline_object(struct gl_context *ctx,
                             struct gl_pipeline_object *obj)
{
   unsinged i;

   _mesa_reference_shader_program(ctx, &obj->_CurrentFragmentProgram, NULL);

   for (i = 0; i < MESA_SHADER_STAGES; i++)
      _mesa_reference_shader_program(ctx, &obj->CurrentProgram[i], NULL);

   _mesa_reference_shader_program(ctx, &obj->ActiveProgram, NULL);
   _glthread_DESTROY_MUTEX(obj->Mutex);
   ralloc_free(obj);
}

/**
 * Allocate and initialize a new pipeline object.
 */
static struct gl_pipeline_object *
_mesa_new_pipeline_object(struct gl_context *ctx, GLuint name)
{
   struct gl_pipeline_object *obj = rzalloc(NULL, struct gl_pipeline_object);
   if (obj) {
      obj->Name = name;
      _glthread_INIT_MUTEX(obj->Mutex);
      obj->RefCount = 1;
      obj->Flags = _mesa_get_shader_flags();
   }

   return obj;
}

/**
 * Initialize pipeline object state for given context.
 */
void
_mesa_init_pipeline(struct gl_context *ctx)
{
   ctx->Pipeline.Objects = _mesa_NewHashTable();

   ctx->Pipeline.Current = NULL;
}


/**
 * Callback for deleting a pipeline object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_pipelineobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_pipeline_object *obj = (struct gl_pipeline_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   _mesa_delete_pipeline_object(ctx, obj);
}


/**
 * Free pipeline state for given context.
 */
void
_mesa_free_pipeline_data(struct gl_context *ctx)
{
   _mesa_HashDeleteAll(ctx->Pipeline.Objects, delete_pipelineobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Pipeline.Objects);
}

/**
 * Look up the pipeline object for the given ID.
 *
 * \returns
 * Either a pointer to the pipeline object with the specified ID or \c NULL for
 * a non-existent ID.  The spec defines ID 0 as being technically
 * non-existent.
 */
static inline struct gl_pipeline_object *
lookup_pipeline_object(struct gl_context *ctx, GLuint id)
{
   if (id == 0)
      return NULL;
   else
      return (struct gl_pipeline_object *)
         _mesa_HashLookup(ctx->Pipeline.Objects, id);
}

/**
 * Add the given pipeline object to the pipeline object pool.
 */
static void
save_pipeline_object(struct gl_context *ctx, struct gl_pipeline_object *obj)
{
   if (obj->Name > 0) {
      _mesa_HashInsert(ctx->Pipeline.Objects, obj->Name, obj);
   }
}

/**
 * Remove the given pipeline object from the pipeline object pool.
 * Do not deallocate the pipeline object though.
 */
static void
remove_pipeline_object(struct gl_context *ctx, struct gl_pipeline_object *obj)
{
   if (obj->Name > 0) {
      _mesa_HashRemove(ctx->Pipeline.Objects, obj->Name);
   }
}

/**
 * Set ptr to obj w/ reference counting.
 * Note: this should only be called from the _mesa_reference_pipeline_object()
 * inline function.
 */
void
_mesa_reference_pipeline_object_(struct gl_context *ctx,
                                 struct gl_pipeline_object **ptr,
                                 struct gl_pipeline_object *obj)
{
   assert(*ptr != obj);

   if (*ptr) {
      /* Unreference the old pipeline object */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_pipeline_object *oldObj = *ptr;

      _glthread_LOCK_MUTEX(oldObj->Mutex);
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
      deleteFlag = (oldObj->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldObj->Mutex);

      if (deleteFlag) {
         _mesa_delete_pipeline_object(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (obj) {
      /* reference new pipeline object */
      _glthread_LOCK_MUTEX(obj->Mutex);
      if (obj->RefCount == 0) {
         /* this pipeline's being deleted (look just above) */
         /* Not sure this can ever really happen.  Warn if it does. */
         _mesa_problem(NULL, "referencing deleted pipeline object");
         *ptr = NULL;
      }
      else {
         obj->RefCount++;
         *ptr = obj;
      }
      _glthread_UNLOCK_MUTEX(obj->Mutex);
   }
}

/**
 * Bound program to severals stages of the pipeline
 */
void GLAPIENTRY
_mesa_UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
}

/**
 * Use the named shader program for subsequent glUniform calls (if pipeline
 * bound)
 */
void GLAPIENTRY
_mesa_ActiveShaderProgram(GLuint pipeline, GLuint program)
{
}

/**
 * Make program of the pipeline current
 */
void GLAPIENTRY
_mesa_BindProgramPipeline(GLuint pipeline)
{
}

/**
 * Delete a set of pipeline objects.
 *
 * \param n      Number of pipeline objects to delete.
 * \param ids    pipeline of \c n pipeline object IDs.
 */
void GLAPIENTRY
_mesa_DeleteProgramPipelines(GLsizei n, const GLuint *pipelines)
{
   GET_CURRENT_CONTEXT(ctx);
   GLsizei i;

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteProgramPipelines(n<0)");
      return;
   }

   for (i = 0; i < n; i++) {
      struct gl_pipeline_object *obj =
         lookup_pipeline_object(ctx, pipelines[i]);

      if (obj) {
         ASSERT(obj->Name == pipelines[i]);

         /* If the pipeline object is currently bound, the spec says "If an
          * object that is currently bound is deleted, the binding for that
          * object reverts to zero and no program pipeline object becomes
          * current."
          */
         if (obj == ctx->Pipeline.Current) {
            _mesa_BindProgramPipeline(0);
         }

         /* The ID is immediately freed for re-use */
         remove_pipeline_object(ctx, obj);

         /* Unreference the pipeline object.
          * If refcount hits zero, the object will be deleted.
          */
         _mesa_reference_pipeline_object(ctx, &obj, NULL);
      }
   }
}

/**
 * Generate a set of unique pipeline object IDs and store them in \c pipelines.
 * \param n       Number of IDs to generate.
 * \param pipelines  pipeline of \c n locations to store the IDs.
 */
void GLAPIENTRY
_mesa_GenProgramPipelines(GLsizei n, GLuint *pipelines)
{
   GET_CURRENT_CONTEXT(ctx);

   GLuint first;
   GLint i;

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenProgramPipelines(n<0)");
      return;
   }

   if (!pipelines) {
      return;
   }

   first = _mesa_HashFindFreeKeyBlock(ctx->Pipeline.Objects, n);

   for (i = 0; i < n; i++) {
      struct gl_pipeline_object *obj;
      GLuint name = first + i;

      obj = _mesa_new_pipeline_object(ctx, name);
      if (!obj) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenProgramPipelines");
         return;
      }

      save_pipeline_object(ctx, obj);
      pipelines[i] = first + i;
   }

}

/**
 * Determine if ID is the name of an pipeline object.
 *
 * \param id  ID of the potential pipeline object.
 * \return  \c GL_TRUE if \c id is the name of a pipeline object,
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsProgramPipeline(GLuint pipeline)
{
   return GL_FALSE;
}

/**
 * glGetProgramPipelineiv() - get pipeline shader state.
 */
void GLAPIENTRY
_mesa_GetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params)
{
}

/**
 * Check compatibility of pipeline's program
 */
void GLAPIENTRY
_mesa_ValidateProgramPipeline(GLuint pipeline)
{
}

void GLAPIENTRY
_mesa_GetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize,
                                GLsizei *length, GLchar *infoLog)
{
}
