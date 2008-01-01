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
 * \brief  Last stage of 'draw' pipeline: send tris to SPUs.
 * \author  Brian Paul
 */

#include "cell_context.h"
#include "cell_render.h"
#include "pipe/p_util.h"
#include "pipe/draw/draw_private.h"


struct render_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct cell_context *cell;
};


static INLINE struct render_stage *
render_stage(struct draw_stage *stage)
{
   return (struct render_stage *) stage;
}


static void render_begin( struct draw_stage *stage )
{
#if 0
   struct render_stage *render = render_stage(stage);
   struct cell_context *sp = render->cell;
   const struct pipe_shader_state *fs = &render->cell->fs->shader;
   render->quad.nr_attrs = render->cell->nr_frag_attrs;

   render->firstFpInput = fs->input_semantic_name[0];

   sp->quad.first->begin(sp->quad.first);
#endif
}


static void render_end( struct draw_stage *stage )
{
}


static void reset_stipple_counter( struct draw_stage *stage )
{
   struct render_stage *render = render_stage(stage);
   /*render->cell->line_stipple_counter = 0;*/
}


static void
render_point(struct draw_stage *stage, struct prim_header *prim)
{
}


static void
render_line(struct draw_stage *stage, struct prim_header *prim)
{
}


static void
render_tri(struct draw_stage *stage, struct prim_header *prim)
{
   printf("Cell render tri\n");
}


static void render_destroy( struct draw_stage *stage )
{
   FREE( stage );
}


/**
 * Create a new draw/render stage.  This will be plugged into the
 * draw module as the last pipeline stage.
 */
struct draw_stage *cell_draw_render_stage( struct cell_context *cell )
{
   struct render_stage *render = CALLOC_STRUCT(render_stage);

   render->cell = cell;
   render->stage.draw = cell->draw;
   render->stage.begin = render_begin;
   render->stage.point = render_point;
   render->stage.line = render_line;
   render->stage.tri = render_tri;
   render->stage.end = render_end;
   render->stage.reset_stipple_counter = reset_stipple_counter;
   render->stage.destroy = render_destroy;

   /*
   render->quad.coef = render->coef;
   render->quad.posCoef = &render->posCoef;
   */

   return &render->stage;
}
