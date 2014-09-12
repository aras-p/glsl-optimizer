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
#include "util/ralloc.h"
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
   unsigned i;

   _mesa_reference_shader_program(ctx, &obj->_CurrentFragmentProgram, NULL);

   for (i = 0; i < MESA_SHADER_STAGES; i++)
      _mesa_reference_shader_program(ctx, &obj->CurrentProgram[i], NULL);

   _mesa_reference_shader_program(ctx, &obj->ActiveProgram, NULL);
   mtx_destroy(&obj->Mutex);
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
      mtx_init(&obj->Mutex, mtx_plain);
      obj->RefCount = 1;
      obj->Flags = _mesa_get_shader_flags();
      obj->InfoLog = NULL;
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

   /* Install a default Pipeline */
   ctx->Pipeline.Default = _mesa_new_pipeline_object(ctx, 0);
   _mesa_reference_pipeline_object(ctx, &ctx->_Shader, ctx->Pipeline.Default);
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
   _mesa_reference_pipeline_object(ctx, &ctx->_Shader, NULL);

   _mesa_HashDeleteAll(ctx->Pipeline.Objects, delete_pipelineobj_cb, ctx);
   _mesa_DeleteHashTable(ctx->Pipeline.Objects);

   _mesa_delete_pipeline_object(ctx, ctx->Pipeline.Default);
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

      mtx_lock(&oldObj->Mutex);
      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;
      deleteFlag = (oldObj->RefCount == 0);
      mtx_unlock(&oldObj->Mutex);

      if (deleteFlag) {
         _mesa_delete_pipeline_object(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (obj) {
      /* reference new pipeline object */
      mtx_lock(&obj->Mutex);
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
      mtx_unlock(&obj->Mutex);
   }
}

/**
 * Bound program to severals stages of the pipeline
 */
void GLAPIENTRY
_mesa_UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_pipeline_object *pipe = lookup_pipeline_object(ctx, pipeline);
   struct gl_shader_program *shProg = NULL;
   GLbitfield any_valid_stages;

   if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUseProgramStages(pipeline)");
      return;
   }

   /* Object is created by any Pipeline call but glGenProgramPipelines,
    * glIsProgramPipeline and GetProgramPipelineInfoLog
    */
   pipe->EverBound = GL_TRUE;

   /* Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec says:
    *
    *     "If stages is not the special value ALL_SHADER_BITS, and has a bit
    *     set that is not recognized, the error INVALID_VALUE is generated."
    *
    * NOT YET SUPPORTED:
    * GL_TESS_CONTROL_SHADER_BIT
    * GL_TESS_EVALUATION_SHADER_BIT
    */
   any_valid_stages = GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT;
   if (_mesa_has_geometry_shaders(ctx))
      any_valid_stages |= GL_GEOMETRY_SHADER_BIT;

   if (stages != GL_ALL_SHADER_BITS && (stages & ~any_valid_stages) != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUseProgramStages(Stages)");
      return;
   }

   /* Section 2.17.2 (Transform Feedback Primitive Capture) of the OpenGL 4.1
    * spec says:
    *
    *     "The error INVALID_OPERATION is generated:
    *
    *      ...
    *
    *         - by UseProgramStages if the program pipeline object it refers
    *           to is current and the current transform feedback object is
    *           active and not paused;
    */
   if (ctx->_Shader == pipe) {
      if (_mesa_is_xfb_active_and_unpaused(ctx)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glUseProgramStages(transform feedback active)");
         return;
      }
   }

   if (program) {
      shProg = _mesa_lookup_shader_program_err(ctx, program,
                                               "glUseProgramStages");
      if (shProg == NULL)
         return;

      /* Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec
       * says:
       *
       *     "If the program object named by program was linked without the
       *     PROGRAM_SEPARABLE parameter set, or was not linked successfully,
       *     the error INVALID_OPERATION is generated and the corresponding
       *     shader stages in the pipeline program pipeline object are not
       *     modified."
       */
      if (!shProg->LinkStatus) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUseProgramStages(program not linked)");
         return;
      }

      if (!shProg->SeparateShader) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glUseProgramStages(program wasn't linked with the "
                     "PROGRAM_SEPARABLE flag)");
         return;
      }
   }

   /* Enable individual stages from the program as requested by the
    * application.  If there is no shader for a requested stage in the
    * program, _mesa_use_shader_program will enable fixed-function processing
    * as dictated by the spec.
    *
    * Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec
    * says:
    *
    *     "If UseProgramStages is called with program set to zero or with a
    *     program object that contains no executable code for the given
    *     stages, it is as if the pipeline object has no programmable stage
    *     configured for the indicated shader stages."
    */
   if ((stages & GL_VERTEX_SHADER_BIT) != 0)
      _mesa_use_shader_program(ctx, GL_VERTEX_SHADER, shProg, pipe);

   if ((stages & GL_FRAGMENT_SHADER_BIT) != 0)
      _mesa_use_shader_program(ctx, GL_FRAGMENT_SHADER, shProg, pipe);

   if ((stages & GL_GEOMETRY_SHADER_BIT) != 0)
      _mesa_use_shader_program(ctx, GL_GEOMETRY_SHADER, shProg, pipe);
}

/**
 * Use the named shader program for subsequent glUniform calls (if pipeline
 * bound)
 */
void GLAPIENTRY
_mesa_ActiveShaderProgram(GLuint pipeline, GLuint program)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *shProg = NULL;
   struct gl_pipeline_object *pipe = lookup_pipeline_object(ctx, pipeline);

   if (program != 0) {
      shProg = _mesa_lookup_shader_program_err(ctx, program,
                                               "glActiveShaderProgram(program)");
      if (shProg == NULL)
         return;
   }

   if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glActiveShaderProgram(pipeline)");
      return;
   }

   /* Object is created by any Pipeline call but glGenProgramPipelines,
    * glIsProgramPipeline and GetProgramPipelineInfoLog
    */
   pipe->EverBound = GL_TRUE;

   if ((shProg != NULL) && !shProg->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glActiveShaderProgram(program %u not linked)", shProg->Name);
      return;
   }

   _mesa_reference_shader_program(ctx, &pipe->ActiveProgram, shProg);
}

/**
 * Make program of the pipeline current
 */
void GLAPIENTRY
_mesa_BindProgramPipeline(GLuint pipeline)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_pipeline_object *newObj = NULL;

   /* Rebinding the same pipeline object: no change.
    */
   if (ctx->_Shader->Name == pipeline)
      return;

   /* Section 2.17.2 (Transform Feedback Primitive Capture) of the OpenGL 4.1
    * spec says:
    *
    *     "The error INVALID_OPERATION is generated:
    *
    *      ...
    *
    *         - by BindProgramPipeline if the current transform feedback
    *           object is active and not paused;
    */
   if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            "glBindProgramPipeline(transform feedback active)");
      return;
   }

   /* Get pointer to new pipeline object (newObj)
    */
   if (pipeline) {
      /* non-default pipeline object */
      newObj = lookup_pipeline_object(ctx, pipeline);
      if (!newObj) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBindProgramPipeline(non-gen name)");
         return;
      }

      /* Object is created by any Pipeline call but glGenProgramPipelines,
       * glIsProgramPipeline and GetProgramPipelineInfoLog
       */
      newObj->EverBound = GL_TRUE;
   }

   _mesa_bind_pipeline(ctx, newObj);
}

void
_mesa_bind_pipeline(struct gl_context *ctx,
                    struct gl_pipeline_object *pipe)
{
   /* First bind the Pipeline to pipeline binding point */
   _mesa_reference_pipeline_object(ctx, &ctx->Pipeline.Current, pipe);

   /* Section 2.11.3 (Program Objects) of the OpenGL 4.1 spec says:
    *
    *     "If there is a current program object established by UseProgram,
    *     that program is considered current for all stages. Otherwise, if
    *     there is a bound program pipeline object (see section 2.11.4), the
    *     program bound to the appropriate stage of the pipeline object is
    *     considered current."
    */
   if (&ctx->Shader != ctx->_Shader) {
      if (pipe != NULL) {
         /* Bound the pipeline to the current program and
          * restore the pipeline state
          */
         _mesa_reference_pipeline_object(ctx, &ctx->_Shader, pipe);
      } else {
         /* Unbind the pipeline */
         _mesa_reference_pipeline_object(ctx, &ctx->_Shader,
                                         ctx->Pipeline.Default);
      }

      FLUSH_VERTICES(ctx, _NEW_PROGRAM | _NEW_PROGRAM_CONSTANTS);

      if (ctx->Driver.UseProgram)
         ctx->Driver.UseProgram(ctx, NULL);
   }
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
   GET_CURRENT_CONTEXT(ctx);

   struct gl_pipeline_object *obj = lookup_pipeline_object(ctx, pipeline);
   if (obj == NULL)
      return GL_FALSE;

   return obj->EverBound;
}

/**
 * glGetProgramPipelineiv() - get pipeline shader state.
 */
void GLAPIENTRY
_mesa_GetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_pipeline_object *pipe = lookup_pipeline_object(ctx, pipeline);

   /* Are geometry shaders available in this context?
    */
   const bool has_gs = _mesa_has_geometry_shaders(ctx);

   if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetProgramPipelineiv(pipeline)");
      return;
   }

   /* Object is created by any Pipeline call but glGenProgramPipelines,
    * glIsProgramPipeline and GetProgramPipelineInfoLog
    */
   pipe->EverBound = GL_TRUE;

   switch (pname) {
   case GL_ACTIVE_PROGRAM:
      *params = pipe->ActiveProgram ? pipe->ActiveProgram->Name : 0;
      return;
   case GL_INFO_LOG_LENGTH:
      *params = pipe->InfoLog ? strlen(pipe->InfoLog) + 1 : 0;
      return;
   case GL_VALIDATE_STATUS:
      *params = pipe->Validated;
      return;
   case GL_VERTEX_SHADER:
      *params = pipe->CurrentProgram[MESA_SHADER_VERTEX]
         ? pipe->CurrentProgram[MESA_SHADER_VERTEX]->Name : 0;
      return;
   case GL_TESS_EVALUATION_SHADER:
      /* NOT YET SUPPORTED */
      break;
   case GL_TESS_CONTROL_SHADER:
      /* NOT YET SUPPORTED */
      break;
   case GL_GEOMETRY_SHADER:
      if (!has_gs)
         break;
      *params = pipe->CurrentProgram[MESA_SHADER_GEOMETRY]
         ? pipe->CurrentProgram[MESA_SHADER_GEOMETRY]->Name : 0;
      return;
   case GL_FRAGMENT_SHADER:
      *params = pipe->CurrentProgram[MESA_SHADER_FRAGMENT]
         ? pipe->CurrentProgram[MESA_SHADER_FRAGMENT]->Name : 0;
      return;
   default:
      break;
   }

   _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramPipelineiv(pname=%s)",
               _mesa_lookup_enum_by_nr(pname));
}

/**
 * Determines whether every stage in a linked program is active in the
 * specified pipeline.
 */
static bool
program_stages_all_active(struct gl_pipeline_object *pipe,
                          const struct gl_shader_program *prog)
{
   unsigned i;
   bool status = true;

   if (!prog)
      return true;

   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i]) {
         if (pipe->CurrentProgram[i]) {
            if (prog->Name != pipe->CurrentProgram[i]->Name) {
               status = false;
            }
         } else {
            status = false;
         }
      }
   }

   if (!status) {
      pipe->InfoLog = ralloc_asprintf(pipe,
                                      "Program %d is not active for all "
                                      "shaders that was linked",
                                      prog->Name);
   }

   return status;
}

extern GLboolean
_mesa_validate_program_pipeline(struct gl_context* ctx,
                                struct gl_pipeline_object *pipe,
                                GLboolean IsBound)
{
   unsigned i;

   pipe->Validated = GL_FALSE;

   /* Release and reset the info log.
    */
   if (pipe->InfoLog != NULL)
      ralloc_free(pipe->InfoLog);

   pipe->InfoLog = NULL;

   /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
    * OpenGL 4.1 spec says:
    *
    *     "[INVALID_OPERATION] is generated by any command that transfers
    *     vertices to the GL if:
    *
    *         - A program object is active for at least one, but not all of
    *           the shader stages that were present when the program was
    *           linked."
    *
    * For each possible program stage, verify that the program bound to that
    * stage has all of its stages active.  In other words, if the program
    * bound to the vertex stage also has a fragment shader, the fragment
    * shader must also be bound to the fragment stage.
    */
   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!program_stages_all_active(pipe, pipe->CurrentProgram[i])) {
         goto err;
      }
   }

   /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
    * OpenGL 4.1 spec says:
    *
    *     "[INVALID_OPERATION] is generated by any command that transfers
    *     vertices to the GL if:
    *
    *         ...
    *
    *         - One program object is active for at least two shader stages
    *           and a second program is active for a shader stage between two
    *           stages for which the first program was active."
    *
    * Without Tesselation, the only case where this can occur is the geometry
    * shader between the fragment shader and vertex shader.
    */
   if (pipe->CurrentProgram[MESA_SHADER_GEOMETRY]
       && pipe->CurrentProgram[MESA_SHADER_FRAGMENT]
       && pipe->CurrentProgram[MESA_SHADER_VERTEX]) {
      if (pipe->CurrentProgram[MESA_SHADER_VERTEX]->Name == pipe->CurrentProgram[MESA_SHADER_FRAGMENT]->Name &&
          pipe->CurrentProgram[MESA_SHADER_GEOMETRY]->Name != pipe->CurrentProgram[MESA_SHADER_VERTEX]->Name) {
         pipe->InfoLog =
            ralloc_asprintf(pipe,
                            "Program %d is active for geometry stage between "
                            "two stages for which another program %d is "
                            "active",
                            pipe->CurrentProgram[MESA_SHADER_GEOMETRY]->Name,
                            pipe->CurrentProgram[MESA_SHADER_VERTEX]->Name);
         goto err;
      }
   }

   /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
    * OpenGL 4.1 spec says:
    *
    *     "[INVALID_OPERATION] is generated by any command that transfers
    *     vertices to the GL if:
    *
    *         ...
    *
    *         - There is an active program for tessellation control,
    *           tessellation evaluation, or geometry stages with corresponding
    *           executable shader, but there is no active program with
    *           executable vertex shader."
    */
   if (!pipe->CurrentProgram[MESA_SHADER_VERTEX]
       && pipe->CurrentProgram[MESA_SHADER_GEOMETRY]) {
      pipe->InfoLog = ralloc_strdup(pipe, "Program lacks a vertex shader");
      goto err;
   }

   /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
    * OpenGL 4.1 spec says:
    *
    *     "[INVALID_OPERATION] is generated by any command that transfers
    *     vertices to the GL if:
    *
    *         ...
    *
    *         - There is no current program object specified by UseProgram,
    *           there is a current program pipeline object, and the current
    *           program for any shader stage has been relinked since being
    *           applied to the pipeline object via UseProgramStages with the
    *           PROGRAM_SEPARABLE parameter set to FALSE.
    */
   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (pipe->CurrentProgram[i] && !pipe->CurrentProgram[i]->SeparateShader) {
         pipe->InfoLog = ralloc_asprintf(pipe,
                                         "Program %d was relinked without "
                                         "PROGRAM_SEPARABLE state",
                                         pipe->CurrentProgram[i]->Name);
         goto err;
      }
   }

   /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
    * OpenGL 4.1 spec says:
    *
    *     "[INVALID_OPERATION] is generated by any command that transfers
    *     vertices to the GL if:
    *
    *         ...
    *
    *         - Any two active samplers in the current program object are of
    *           different types, but refer to the same texture image unit.
    *
    *         - The number of active samplers in the program exceeds the
    *           maximum number of texture image units allowed."
    */
   if (!_mesa_sampler_uniforms_pipeline_are_valid(pipe))
      goto err;

   pipe->Validated = GL_TRUE;
   return GL_TRUE;

err:
   if (IsBound)
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glValidateProgramPipeline failed to validate the pipeline");

   return GL_FALSE;
}

/**
 * Check compatibility of pipeline's program
 */
void GLAPIENTRY
_mesa_ValidateProgramPipeline(GLuint pipeline)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_pipeline_object *pipe = lookup_pipeline_object(ctx, pipeline);

   if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glValidateProgramPipeline(pipeline)");
      return;
   }

   _mesa_validate_program_pipeline(ctx, pipe,
                                   (ctx->_Shader->Name == pipe->Name));
}

void GLAPIENTRY
_mesa_GetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize,
                                GLsizei *length, GLchar *infoLog)
{
   GET_CURRENT_CONTEXT(ctx);

   struct gl_pipeline_object *pipe = lookup_pipeline_object(ctx, pipeline);

   if (!pipe) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetProgramPipelineInfoLog(pipeline)");
      return;
   }

   if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetProgramPipelineInfoLog(bufSize)");
      return;
   }

   if (pipe->InfoLog)
      _mesa_copy_string(infoLog, bufSize, length, pipe->InfoLog);
   else
      *length = 0;
}
