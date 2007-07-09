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

/* Authors:
 *  Brian Paul
 */

#include "imports.h"

#include "sp_context.h"
#include "sp_state.h"



void
softpipe_set_sampler_state(struct pipe_context *pipe,
                           GLuint unit,
                           const struct pipe_sampler_state *sampler)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   softpipe->sampler[unit] = *sampler;

   softpipe->dirty |= G_NEW_SAMPLER;
}


void
softpipe_set_texture_state(struct pipe_context *pipe,
                           GLuint unit,
                           struct pipe_texture_object *texture)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   assert(unit < PIPE_MAX_SAMPLERS);
   softpipe->texture[unit] = texture;  /* ptr, not struct */

   softpipe->dirty |= G_NEW_TEXTURE;
}
