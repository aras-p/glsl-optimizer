/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"

#include "svga_context.h"

/***********************************************************************
 * Constant buffers 
 */

struct svga_constbuf 
{
   unsigned type;
   float (*data)[4];
   unsigned count;
};



static void svga_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     struct pipe_buffer *buf)
{
   struct svga_context *svga = svga_context(pipe);

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   pipe_buffer_reference( &svga->curr.cb[shader],
                          buf );

   if (shader == PIPE_SHADER_FRAGMENT)
      svga->dirty |= SVGA_NEW_FS_CONST_BUFFER;
   else
      svga->dirty |= SVGA_NEW_VS_CONST_BUFFER;
}



void svga_init_constbuffer_functions( struct svga_context *svga )
{
   svga->pipe.set_constant_buffer = svga_set_constant_buffer;
}

