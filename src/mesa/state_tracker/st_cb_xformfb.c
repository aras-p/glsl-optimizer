/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * Transform feedback functions.
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/mfeatures.h"
#include "main/transformfeedback.h"

#include "st_cb_xformfb.h"


#if FEATURE_EXT_transform_feedback

#if 0
static struct gl_transform_feedback_object *
st_new_transform_feedback(struct gl_context *ctx, GLuint name)
{
   struct gl_transform_feedback_object *obj;
   obj = CALLOC_STRUCT(gl_transform_feedback_object);
   if (obj) {
      obj->Name = name;
      obj->RefCount = 1;
   }
   return obj;
}
#endif

#if 0
static void
st_delete_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj)
{
   GLuint i;

   for (i = 0; i < Elements(obj->Buffers); i++) {
      _mesa_reference_buffer_object(ctx, &obj->Buffers[i], NULL);
   }

   free(obj);
}
#endif


static void
st_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                            struct gl_transform_feedback_object *obj)
{
   /* to-do */
}


static void
st_end_transform_feedback(struct gl_context *ctx,
                          struct gl_transform_feedback_object *obj)
{
   /* to-do */
}


static void
st_pause_transform_feedback(struct gl_context *ctx,
                            struct gl_transform_feedback_object *obj)
{
   /* to-do */
}


static void
st_resume_transform_feedback(struct gl_context *ctx,
                             struct gl_transform_feedback_object *obj)
{
   /* to-do */
}


static void
st_draw_transform_feedback(struct gl_context *ctx, GLenum mode,
                           struct gl_transform_feedback_object *obj)
{
   /* XXX to do */
   /*
    * Get number of vertices in obj's feedback buffer.
    * Call ctx->Exec.DrawArrays(mode, 0, count);
    */
}


void
st_init_xformfb_functions(struct dd_function_table *functions)
{
   /* let core Mesa plug in its functions */
   _mesa_init_transform_feedback_functions(functions);

   /* then override a few: */
   functions->BeginTransformFeedback = st_begin_transform_feedback;
   functions->EndTransformFeedback = st_end_transform_feedback;
   functions->PauseTransformFeedback = st_pause_transform_feedback;
   functions->ResumeTransformFeedback = st_resume_transform_feedback;
   functions->DrawTransformFeedback = st_draw_transform_feedback;
}

#endif /* FEATURE_EXT_transform_feedback */
