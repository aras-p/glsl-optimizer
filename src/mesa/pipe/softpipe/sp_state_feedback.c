/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * Authors:
 *  Brian Paul
 */


#include "sp_context.h"
#include "sp_state.h"
#include "sp_surface.h"

#include "pipe/p_winsys.h"
#include "pipe/draw/draw_context.h"


void
softpipe_set_feedback_state(struct pipe_context *pipe,
                            const struct pipe_feedback_state *feedback)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->feedback = *feedback; /* struct copy */
   /*
   softpipe->dirty |= SP_NEW_FEEDBACK;
   */

   draw_set_feedback_state(softpipe->draw, feedback);
}


void
softpipe_set_feedback_buffer(struct pipe_context *pipe,
                             unsigned index,
                             const struct pipe_feedback_buffer *feedback)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(index < PIPE_MAX_FEEDBACK_ATTRIBS);

   /* Need to be careful with referencing */
   pipe->winsys->buffer_reference(pipe->winsys,
                                  &softpipe->feedback_buffer[index].buffer,
                                  feedback->buffer);
   softpipe->feedback_buffer[index].size = feedback->size;
   softpipe->feedback_buffer[index].start_offset = feedback->start_offset;
}
