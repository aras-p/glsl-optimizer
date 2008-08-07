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


#include "main/context.h"
#include "shader/prog_print.h"

#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"

#include "cso_cache/cso_cache.h"

#include "st_context.h"
#include "st_debug.h"
#include "st_program.h"



/**
 * Print current state.  May be called from inside gdb to see currently
 * bound vertex/fragment shaders and associated constants.
 */
void
st_print_current(void)
{
   GET_CURRENT_CONTEXT(ctx);
   struct st_context *st = ctx->st;

#if 0
   int i;

   printf("Vertex Transform Inputs:\n");
   for (i = 0; i < st->vp->state.num_inputs; i++) {
      printf("  Slot %d:  VERT_ATTRIB_%d\n", i, st->vp->index_to_input[i]);
   }
#endif

   tgsi_dump( st->vp->state.tokens, 0 );
   if (st->vp->Base.Base.Parameters)
      _mesa_print_parameter_list(st->vp->Base.Base.Parameters);

   tgsi_dump( st->fp->state.tokens, 0 );
   if (st->fp->Base.Base.Parameters)
      _mesa_print_parameter_list(st->fp->Base.Base.Parameters);
}
