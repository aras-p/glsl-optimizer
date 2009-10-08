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
 * \brief  Primitive rasterization/rendering (points, lines)
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "lp_context.h"
#include "lp_quad.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vertex.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"


#define DEBUG_VERTS 0


void
llvmpipe_setup_flush()
{
}

void
llvmpipe_setup_bind_framebuffer()
{
}

void
llvmpipe_setup_clear()
{
}


/* Stubs for lines & points for now:
 */
void
llvmpipe_setup_point(struct setup_context *setup,
		     const float (*v0)[4])
{
}

void
llvmpipe_setup_line(struct setup_context *setup,
		    const float (*v0)[4],
		    const float (*v1)[4])
{
}


/* Called after statechange, before emitting primitives.  If binning
 * is active, this function should store relevant state in the binning
 * context.
 *
 * That includes: 
 *    - current fragment shader function
 *    - bound constant buffer contents
 *    - bound textures
 *    - blend color
 *    - etc.
 *
 * Basically everything needed at some point in the future to
 * rasterize triangles for the current state.
 *
 * Additionally this will set up the state needed for the rasterizer
 * to process and bin incoming triangles.  That would include such
 * things as:
 *    - cull mode
 *    - ???
 *    - etc.
 * 
 */
void setup_prepare( struct setup_context *setup )
{
   struct llvmpipe_context *lp = setup->llvmpipe;

   if (lp->dirty) {
      llvmpipe_update_derived(lp);
   }

   lp->quad.first->begin( lp->quad.first );

   if (lp->reduced_api_prim == PIPE_PRIM_TRIANGLES &&
       lp->rasterizer->fill_cw == PIPE_POLYGON_MODE_FILL &&
       lp->rasterizer->fill_ccw == PIPE_POLYGON_MODE_FILL) {
      /* we'll do culling */
      setup->winding = lp->rasterizer->cull_mode;
   }
   else {
      /* 'draw' will do culling */
      setup->winding = PIPE_WINDING_NONE;
   }

   setup_prepare_tri( setup->llvmpipe );
}



void setup_destroy_context( struct setup_context *setup )
{
   FREE( setup );
}


/**
 * Create a new primitive setup/render stage.
 */
struct setup_context *setup_create_context( struct llvmpipe_context *llvmpipe )
{
   struct setup_context *setup = CALLOC_STRUCT(setup_context);
   unsigned i;

   setup->llvmpipe = llvmpipe;

   return setup;
}

