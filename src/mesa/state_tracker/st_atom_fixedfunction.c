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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "tnl/t_vp_build.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "st_context.h"
#include "st_atom.h"


#define TGSI_DEBUG 0


/**
 * When TnL state has changed, need to generate new vertex program.
 * This should be done before updating the vertes shader (vs) state.
 */
static void update_tnl( struct st_context *st )
{
   /* Would be good to avoid this when shaders are active:
    */
   _tnl_UpdateFixedFunctionProgram( st->ctx );
}


const struct st_tracked_state st_update_tnl = {
   "st_update_tnl",					/* name */
   {							/* dirty */
      TNL_FIXED_FUNCTION_STATE_FLAGS,			/* mesa */
      0							/* st */
   },
   update_tnl						/* update */
};


