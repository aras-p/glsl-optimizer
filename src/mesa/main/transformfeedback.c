/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Vertex transform feedback support.
 *
 * Authors:
 *   Brian Paul
 */


#include "buffers.h"
#include "bufferobj.h"
#include "context.h"
#include "hash.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "transformfeedback.h"
#include "shaderapi.h"
#include "shaderobj.h"
#include "main/dispatch.h"

#include "program/prog_parameter.h"


/**
 * Do reference counting of transform feedback buffers.
 */
static void
reference_transform_feedback_object(struct gl_transform_feedback_object **ptr,
                                    struct gl_transform_feedback_object *obj)
{
   if (*ptr == obj)
      return;

   if (*ptr) {
      /* Unreference the old object */
      struct gl_transform_feedback_object *oldObj = *ptr;

      ASSERT(oldObj->RefCount > 0);
      oldObj->RefCount--;

      if (oldObj->RefCount == 0) {
         GET_CURRENT_CONTEXT(ctx);
         if (ctx)
            ctx->Driver.DeleteTransformFeedback(ctx, oldObj);
      }

      *ptr = NULL;
   }
   ASSERT(!*ptr);

   if (obj) {
      /* reference new object */
      if (obj->RefCount == 0) {
         _mesa_problem(NULL, "referencing deleted transform feedback object");
         *ptr = NULL;
      }
      else {
         obj->RefCount++;
         obj->EverBound = GL_TRUE;
         *ptr = obj;
      }
   }
}


/**
 * Check that all the buffer objects currently bound for transform
 * feedback actually exist.  Raise a GL_INVALID_OPERATION error if
 * any buffers are missing.
 * \return GL_TRUE for success, GL_FALSE if error
 */
GLboolean
_mesa_validate_transform_feedback_buffers(struct gl_context *ctx)
{
   /* XXX to do */
   return GL_TRUE;
}



/**
 * Per-context init for transform feedback.
 */
void
_mesa_init_transform_feedback(struct gl_context *ctx)
{
   /* core mesa expects this, even a dummy one, to be available */
   ASSERT(ctx->Driver.NewTransformFeedback);

   ctx->TransformFeedback.DefaultObject =
      ctx->Driver.NewTransformFeedback(ctx, 0);

   assert(ctx->TransformFeedback.DefaultObject->RefCount == 1);

   reference_transform_feedback_object(&ctx->TransformFeedback.CurrentObject,
                                       ctx->TransformFeedback.DefaultObject);

   assert(ctx->TransformFeedback.DefaultObject->RefCount == 2);

   ctx->TransformFeedback.Objects = _mesa_NewHashTable();

   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 ctx->Shared->NullBufferObj);
}



/**
 * Callback for _mesa_HashDeleteAll().
 */
static void
delete_cb(GLuint key, void *data, void *userData)
{
   struct gl_context *ctx = (struct gl_context *) userData;
   struct gl_transform_feedback_object *obj =
      (struct gl_transform_feedback_object *) data;

   ctx->Driver.DeleteTransformFeedback(ctx, obj);
}


/**
 * Per-context free/clean-up for transform feedback.
 */
void
_mesa_free_transform_feedback(struct gl_context *ctx)
{
   /* core mesa expects this, even a dummy one, to be available */
   ASSERT(ctx->Driver.NewTransformFeedback);

   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 NULL);

   /* Delete all feedback objects */
   _mesa_HashDeleteAll(ctx->TransformFeedback.Objects, delete_cb, ctx);
   _mesa_DeleteHashTable(ctx->TransformFeedback.Objects);

   /* Delete the default feedback object */
   assert(ctx->Driver.DeleteTransformFeedback);
   ctx->Driver.DeleteTransformFeedback(ctx,
                                       ctx->TransformFeedback.DefaultObject);

   ctx->TransformFeedback.CurrentObject = NULL;
}


/** Default fallback for ctx->Driver.NewTransformFeedback() */
static struct gl_transform_feedback_object *
new_transform_feedback(struct gl_context *ctx, GLuint name)
{
   struct gl_transform_feedback_object *obj;
   obj = CALLOC_STRUCT(gl_transform_feedback_object);
   if (obj) {
      obj->Name = name;
      obj->RefCount = 1;
      obj->EverBound = GL_FALSE;
   }
   return obj;
}

/** Default fallback for ctx->Driver.DeleteTransformFeedback() */
static void
delete_transform_feedback(struct gl_context *ctx,
                          struct gl_transform_feedback_object *obj)
{
   GLuint i;

   for (i = 0; i < Elements(obj->Buffers); i++) {
      _mesa_reference_buffer_object(ctx, &obj->Buffers[i], NULL);
   }

   free(obj);
}


/** Default fallback for ctx->Driver.BeginTransformFeedback() */
static void
begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                         struct gl_transform_feedback_object *obj)
{
   /* nop */
}

/** Default fallback for ctx->Driver.EndTransformFeedback() */
static void
end_transform_feedback(struct gl_context *ctx,
                       struct gl_transform_feedback_object *obj)
{
   /* nop */
}

/** Default fallback for ctx->Driver.PauseTransformFeedback() */
static void
pause_transform_feedback(struct gl_context *ctx,
                         struct gl_transform_feedback_object *obj)
{
   /* nop */
}

/** Default fallback for ctx->Driver.ResumeTransformFeedback() */
static void
resume_transform_feedback(struct gl_context *ctx,
                          struct gl_transform_feedback_object *obj)
{
   /* nop */
}


/**
 * Plug in default device driver functions for transform feedback.
 * Most drivers will override some/all of these.
 */
void
_mesa_init_transform_feedback_functions(struct dd_function_table *driver)
{
   driver->NewTransformFeedback = new_transform_feedback;
   driver->DeleteTransformFeedback = delete_transform_feedback;
   driver->BeginTransformFeedback = begin_transform_feedback;
   driver->EndTransformFeedback = end_transform_feedback;
   driver->PauseTransformFeedback = pause_transform_feedback;
   driver->ResumeTransformFeedback = resume_transform_feedback;
}


/**
 * Fill in the correct Size value for each buffer in \c obj.
 *
 * From the GL 4.3 spec, section 6.1.1 ("Binding Buffer Objects to Indexed
 * Targets"):
 *
 *   BindBufferBase binds the entire buffer, even when the size of the buffer
 *   is changed after the binding is established. It is equivalent to calling
 *   BindBufferRange with offset zero, while size is determined by the size of
 *   the bound buffer at the time the binding is used.
 *
 *   Regardless of the size specified with BindBufferRange, or indirectly with
 *   BindBufferBase, the GL will never read or write beyond the end of a bound
 *   buffer. In some cases this constraint may result in visibly different
 *   behavior when a buffer overflow would otherwise result, such as described
 *   for transform feedback operations in section 13.2.2.
 */
static void
compute_transform_feedback_buffer_sizes(
      struct gl_transform_feedback_object *obj)
{
   unsigned i = 0;
   for (i = 0; i < MAX_FEEDBACK_BUFFERS; ++i) {
      GLintptr offset = obj->Offset[i];
      GLsizeiptr buffer_size
         = obj->Buffers[i] == NULL ? 0 : obj->Buffers[i]->Size;
      GLsizeiptr available_space
         = buffer_size <= offset ? 0 : buffer_size - offset;
      GLsizeiptr computed_size;
      if (obj->RequestedSize[i] == 0) {
         /* No size was specified at the time the buffer was bound, so allow
          * writing to all available space in the buffer.
          */
         computed_size = available_space;
      } else {
         /* A size was specified at the time the buffer was bound, however
          * it's possible that the buffer has shrunk since then.  So only
          * allow writing to the minimum of the specified size and the space
          * available.
          */
         computed_size = MIN2(available_space, obj->RequestedSize[i]);
      }

      /* Legal sizes must be multiples of four, so round down if necessary. */
      obj->Size[i] = computed_size & ~0x3;
   }
}


/**
 * Compute the maximum number of vertices that can be written to the currently
 * enabled transform feedback buffers without overflowing any of them.
 */
unsigned
_mesa_compute_max_transform_feedback_vertices(
      const struct gl_transform_feedback_object *obj,
      const struct gl_transform_feedback_info *info)
{
   unsigned max_index = 0xffffffff;
   unsigned i;

   for (i = 0; i < info->NumBuffers; ++i) {
      unsigned stride = info->BufferStride[i];
      unsigned max_for_this_buffer;

      /* Skip any inactive buffers, which have a stride of 0. */
      if (stride == 0)
	 continue;

      max_for_this_buffer = obj->Size[i] / (4 * stride);
      max_index = MIN2(max_index, max_for_this_buffer);
   }

   return max_index;
}


/**
 ** Begin API functions
 **/


void GLAPIENTRY
_mesa_BeginTransformFeedback(GLenum mode)
{
   struct gl_transform_feedback_object *obj;
   struct gl_transform_feedback_info *info;
   GLuint i;
   unsigned vertices_per_prim;
   GET_CURRENT_CONTEXT(ctx);

   obj = ctx->TransformFeedback.CurrentObject;

   if (ctx->Shader.CurrentVertexProgram == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginTransformFeedback(no program active)");
      return;
   }

   info = &ctx->Shader.CurrentVertexProgram->LinkedTransformFeedback;

   if (info->NumOutputs == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginTransformFeedback(no varyings to record)");
      return;
   }

   switch (mode) {
   case GL_POINTS:
      vertices_per_prim = 1;
      break;
   case GL_LINES:
      vertices_per_prim = 2;
      break;
   case GL_TRIANGLES:
      vertices_per_prim = 3;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginTransformFeedback(mode)");
      return;
   }

   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBeginTransformFeedback(already active)");
      return;
   }

   for (i = 0; i < info->NumBuffers; ++i) {
      if (obj->BufferNames[i] == 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBeginTransformFeedback(binding point %d does not have "
                     "a buffer object bound)", i);
         return;
      }
   }

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM_FEEDBACK);
   obj->Active = GL_TRUE;
   ctx->TransformFeedback.Mode = mode;

   compute_transform_feedback_buffer_sizes(obj);

   if (_mesa_is_gles3(ctx)) {
      /* In GLES3, we are required to track the usage of the transform
       * feedback buffer and report INVALID_OPERATION if a draw call tries to
       * exceed it.  So compute the maximum number of vertices that we can
       * write without overflowing any of the buffers currently being used for
       * feedback.
       */
      unsigned max_vertices
         = _mesa_compute_max_transform_feedback_vertices(obj, info);
      obj->GlesRemainingPrims = max_vertices / vertices_per_prim;
   }

   assert(ctx->Driver.BeginTransformFeedback);
   ctx->Driver.BeginTransformFeedback(ctx, mode, obj);
}


void GLAPIENTRY
_mesa_EndTransformFeedback(void)
{
   struct gl_transform_feedback_object *obj;
   GET_CURRENT_CONTEXT(ctx);

   obj = ctx->TransformFeedback.CurrentObject;

   if (!obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glEndTransformFeedback(not active)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM_FEEDBACK);
   ctx->TransformFeedback.CurrentObject->Active = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->Paused = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->EndedAnytime = GL_TRUE;

   assert(ctx->Driver.EndTransformFeedback);
   ctx->Driver.EndTransformFeedback(ctx, obj);
}


/**
 * Helper used by BindBufferRange() and BindBufferBase().
 */
static void
bind_buffer_range(struct gl_context *ctx, GLuint index,
                  struct gl_buffer_object *bufObj,
                  GLintptr offset, GLsizeiptr size)
{
   struct gl_transform_feedback_object *obj =
      ctx->TransformFeedback.CurrentObject;

   /* Note: no need to FLUSH_VERTICES or flag _NEW_TRANSFORM_FEEDBACK, because
    * transform feedback buffers can't be changed while transform feedback is
    * active.
    */

   /* The general binding point */
   _mesa_reference_buffer_object(ctx,
                                 &ctx->TransformFeedback.CurrentBuffer,
                                 bufObj);

   /* The per-attribute binding point */
   _mesa_reference_buffer_object(ctx,
                                 &obj->Buffers[index],
                                 bufObj);

   obj->BufferNames[index] = bufObj->Name;

   obj->Offset[index] = offset;
   obj->RequestedSize[index] = size;
}


/**
 * Specify a buffer object to receive vertex shader results.  Plus,
 * specify the starting offset to place the results, and max size.
 * Called from the glBindBufferRange() function.
 */
void
_mesa_bind_buffer_range_transform_feedback(struct gl_context *ctx,
					   GLuint index,
					   struct gl_buffer_object *bufObj,
					   GLintptr offset,
					   GLsizeiptr size)
{
   struct gl_transform_feedback_object *obj;

   obj = ctx->TransformFeedback.CurrentObject;

   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferRange(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(index=%d)", index);
      return;
   }

   if (size & 0x3) {
      /* must a multiple of four */
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferRange(size=%d)", (int) size);
      return;
   }  

   if (offset & 0x3) {
      /* must be multiple of four */
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferRange(offset=%d)", (int) offset);
      return;
   }  

   bind_buffer_range(ctx, index, bufObj, offset, size);
}


/**
 * Specify a buffer object to receive vertex shader results.
 * As above, but start at offset = 0.
 * Called from the glBindBufferBase() function.
 */
void
_mesa_bind_buffer_base_transform_feedback(struct gl_context *ctx,
					  GLuint index,
					  struct gl_buffer_object *bufObj)
{
   struct gl_transform_feedback_object *obj;

   obj = ctx->TransformFeedback.CurrentObject;

   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferBase(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindBufferBase(index=%d)", index);
      return;
   }

   bind_buffer_range(ctx, index, bufObj, 0, 0);
}


/**
 * Specify a buffer object to receive vertex shader results, plus the
 * offset in the buffer to start placing results.
 * This function is part of GL_EXT_transform_feedback, but not GL3.
 */
void GLAPIENTRY
_mesa_BindBufferOffsetEXT(GLenum target, GLuint index, GLuint buffer,
                          GLintptr offset)
{
   struct gl_transform_feedback_object *obj;
   struct gl_buffer_object *bufObj;
   GET_CURRENT_CONTEXT(ctx);

   if (target != GL_TRANSFORM_FEEDBACK_BUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferOffsetEXT(target)");
      return;
   }

   obj = ctx->TransformFeedback.CurrentObject;

   if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferOffsetEXT(transform feedback active)");
      return;
   }

   if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferOffsetEXT(index=%d)", index);
      return;
   }

   if (offset & 0x3) {
      /* must be multiple of four */
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glBindBufferOffsetEXT(offset=%d)", (int) offset);
      return;
   }

   if (buffer == 0) {
      bufObj = ctx->Shared->NullBufferObj;
   } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   }

   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindBufferOffsetEXT(invalid buffer=%u)", buffer);
      return;
   }

   bind_buffer_range(ctx, index, bufObj, offset, 0);
}


/**
 * This function specifies the vertex shader outputs to be written
 * to the feedback buffer(s), and in what order.
 */
void GLAPIENTRY
_mesa_TransformFeedbackVaryings(GLuint program, GLsizei count,
                                const GLchar * const *varyings,
                                GLenum bufferMode)
{
   struct gl_shader_program *shProg;
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   switch (bufferMode) {
   case GL_INTERLEAVED_ATTRIBS:
      break;
   case GL_SEPARATE_ATTRIBS:
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glTransformFeedbackVaryings(bufferMode)");
      return;
   }

   if (count < 0 ||
       (bufferMode == GL_SEPARATE_ATTRIBS &&
        (GLuint) count > ctx->Const.MaxTransformFeedbackBuffers)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTransformFeedbackVaryings(count=%d)", count);
      return;
   }

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTransformFeedbackVaryings(program=%u)", program);
      return;
   }

   if (ctx->Extensions.ARB_transform_feedback3) {
      if (bufferMode == GL_INTERLEAVED_ATTRIBS) {
         unsigned buffers = 1;

         for (i = 0; i < count; i++) {
            if (strcmp(varyings[i], "gl_NextBuffer") == 0)
               buffers++;
         }

         if (buffers > ctx->Const.MaxTransformFeedbackBuffers) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glTransformFeedbackVaryings(too many gl_NextBuffer "
                        "occurences)");
            return;
         }
      } else {
         for (i = 0; i < count; i++) {
            if (strcmp(varyings[i], "gl_NextBuffer") == 0 ||
                strcmp(varyings[i], "gl_SkipComponents1") == 0 ||
                strcmp(varyings[i], "gl_SkipComponents2") == 0 ||
                strcmp(varyings[i], "gl_SkipComponents3") == 0 ||
                strcmp(varyings[i], "gl_SkipComponents4") == 0) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glTransformFeedbackVaryings(SEPARATE_ATTRIBS,"
                           "varying=%s)",
                           varyings[i]);
               return;
            }
         }
      }
   }

   /* free existing varyings, if any */
   for (i = 0; i < (GLint) shProg->TransformFeedback.NumVarying; i++) {
      free(shProg->TransformFeedback.VaryingNames[i]);
   }
   free(shProg->TransformFeedback.VaryingNames);

   /* allocate new memory for varying names */
   shProg->TransformFeedback.VaryingNames =
      malloc(count * sizeof(GLchar *));

   if (!shProg->TransformFeedback.VaryingNames) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTransformFeedbackVaryings()");
      return;
   }

   /* Save the new names and the count */
   for (i = 0; i < count; i++) {
      shProg->TransformFeedback.VaryingNames[i] = _mesa_strdup(varyings[i]);
   }
   shProg->TransformFeedback.NumVarying = count;

   shProg->TransformFeedback.BufferMode = bufferMode;

   /* No need to set _NEW_TRANSFORM_FEEDBACK (or invoke FLUSH_VERTICES) since
    * the varyings won't be used until shader link time.
    */
}


/**
 * Get info about the vertex shader's outputs which are to be written
 * to the feedback buffer(s).
 */
void GLAPIENTRY
_mesa_GetTransformFeedbackVarying(GLuint program, GLuint index,
                                  GLsizei bufSize, GLsizei *length,
                                  GLsizei *size, GLenum *type, GLchar *name)
{
   const struct gl_shader_program *shProg;
   const struct gl_transform_feedback_info *linked_xfb_info;
   GET_CURRENT_CONTEXT(ctx);

   shProg = _mesa_lookup_shader_program(ctx, program);
   if (!shProg) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetTransformFeedbackVarying(program=%u)", program);
      return;
   }

   linked_xfb_info = &shProg->LinkedTransformFeedback;
   if (index >= (GLuint) linked_xfb_info->NumVarying) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glGetTransformFeedbackVarying(index=%u)", index);
      return;
   }

   /* return the varying's name and length */
   _mesa_copy_string(name, bufSize, length,
		     linked_xfb_info->Varyings[index].Name);

   /* return the datatype and value's size (in datatype units) */
   if (type)
      *type = linked_xfb_info->Varyings[index].Type;
   if (size)
      *size = linked_xfb_info->Varyings[index].Size;
}



struct gl_transform_feedback_object *
_mesa_lookup_transform_feedback_object(struct gl_context *ctx, GLuint name)
{
   if (name == 0) {
      return ctx->TransformFeedback.DefaultObject;
   }
   else
      return (struct gl_transform_feedback_object *)
         _mesa_HashLookup(ctx->TransformFeedback.Objects, name);
}


/**
 * Create new transform feedback objects.   Transform feedback objects
 * encapsulate the state related to transform feedback to allow quickly
 * switching state (and drawing the results, below).
 * Part of GL_ARB_transform_feedback2.
 */
void GLAPIENTRY
_mesa_GenTransformFeedbacks(GLsizei n, GLuint *names)
{
   GLuint first;
   GET_CURRENT_CONTEXT(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenTransformFeedbacks(n < 0)");
      return;
   }

   if (!names)
      return;

   /* we don't need contiguous IDs, but this might be faster */
   first = _mesa_HashFindFreeKeyBlock(ctx->TransformFeedback.Objects, n);
   if (first) {
      GLsizei i;
      for (i = 0; i < n; i++) {
         struct gl_transform_feedback_object *obj
            = ctx->Driver.NewTransformFeedback(ctx, first + i);
         if (!obj) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenTransformFeedbacks");
            return;
         }
         names[i] = first + i;
         _mesa_HashInsert(ctx->TransformFeedback.Objects, first + i, obj);
      }
   }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGenTransformFeedbacks");
   }
}


/**
 * Is the given ID a transform feedback object?
 * Part of GL_ARB_transform_feedback2.
 */
GLboolean GLAPIENTRY
_mesa_IsTransformFeedback(GLuint name)
{
   struct gl_transform_feedback_object *obj;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (name == 0)
      return GL_FALSE;

   obj = _mesa_lookup_transform_feedback_object(ctx, name);
   if (obj == NULL)
      return GL_FALSE;

   return obj->EverBound;
}


/**
 * Bind the given transform feedback object.
 * Part of GL_ARB_transform_feedback2.
 */
void GLAPIENTRY
_mesa_BindTransformFeedback(GLenum target, GLuint name)
{
   struct gl_transform_feedback_object *obj;
   GET_CURRENT_CONTEXT(ctx);

   if (target != GL_TRANSFORM_FEEDBACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindTransformFeedback(target)");
      return;
   }

   if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
              "glBindTransformFeedback(transform is active, or not paused)");
      return;
   }

   obj = _mesa_lookup_transform_feedback_object(ctx, name);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindTransformFeedback(name=%u)", name);
      return;
   }

   reference_transform_feedback_object(&ctx->TransformFeedback.CurrentObject,
                                       obj);
}


/**
 * Delete the given transform feedback objects.
 * Part of GL_ARB_transform_feedback2.
 */
void GLAPIENTRY
_mesa_DeleteTransformFeedbacks(GLsizei n, const GLuint *names)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteTransformFeedbacks(n < 0)");
      return;
   }

   if (!names)
      return;

   for (i = 0; i < n; i++) {
      if (names[i] > 0) {
         struct gl_transform_feedback_object *obj
            = _mesa_lookup_transform_feedback_object(ctx, names[i]);
         if (obj) {
            if (obj->Active) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glDeleteTransformFeedbacks(object %u is active)",
                           names[i]);
               return;
            }
            _mesa_HashRemove(ctx->TransformFeedback.Objects, names[i]);
            /* unref, but object may not be deleted until later */
            reference_transform_feedback_object(&obj, NULL);
         }
      }
   }
}


/**
 * Pause transform feedback.
 * Part of GL_ARB_transform_feedback2.
 */
void GLAPIENTRY
_mesa_PauseTransformFeedback(void)
{
   struct gl_transform_feedback_object *obj;
   GET_CURRENT_CONTEXT(ctx);

   obj = ctx->TransformFeedback.CurrentObject;

   if (!_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
           "glPauseTransformFeedback(feedback not active or already paused)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM_FEEDBACK);
   obj->Paused = GL_TRUE;

   assert(ctx->Driver.PauseTransformFeedback);
   ctx->Driver.PauseTransformFeedback(ctx, obj);
}


/**
 * Resume transform feedback.
 * Part of GL_ARB_transform_feedback2.
 */
void GLAPIENTRY
_mesa_ResumeTransformFeedback(void)
{
   struct gl_transform_feedback_object *obj;
   GET_CURRENT_CONTEXT(ctx);

   obj = ctx->TransformFeedback.CurrentObject;

   if (!obj->Active || !obj->Paused) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "glResumeTransformFeedback(feedback not active or not paused)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_TRANSFORM_FEEDBACK);
   obj->Paused = GL_FALSE;

   assert(ctx->Driver.ResumeTransformFeedback);
   ctx->Driver.ResumeTransformFeedback(ctx, obj);
}
