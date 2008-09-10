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
 * \brief A draw stage that drives our triangle setup routines from
 * within the draw pipeline.  One of two ways to drive setup, the
 * other being in sp_prim_vbuf.c.
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */


#include "sp_context.h"
#include "sp_setup.h"
#include "sp_state.h"
#include "sp_prim_setup.h"
#include "draw/draw_pipe.h"
#include "draw/draw_vertex.h"
#include "util/u_memory.h"

/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct setup_context *setup;
};



/**
 * Basically a cast wrapper.
 */
static INLINE struct setup_stage *setup_stage( struct draw_stage *stage )
{
   return (struct setup_stage *)stage;
}


typedef const float (*cptrf4)[4];

static void
do_tri(struct draw_stage *stage, struct prim_header *prim)
{
   struct setup_stage *setup = setup_stage( stage );
   
   setup_tri( setup->setup,
              (cptrf4)prim->v[0]->data,
              (cptrf4)prim->v[1]->data,
              (cptrf4)prim->v[2]->data );
}

static void
do_line(struct draw_stage *stage, struct prim_header *prim)
{
   struct setup_stage *setup = setup_stage( stage );

   setup_line( setup->setup,
               (cptrf4)prim->v[0]->data,
               (cptrf4)prim->v[1]->data );
}

static void
do_point(struct draw_stage *stage, struct prim_header *prim)
{
   struct setup_stage *setup = setup_stage( stage );

   setup_point( setup->setup,
                (cptrf4)prim->v[0]->data );
}




static void setup_begin( struct draw_stage *stage )
{
   struct setup_stage *setup = setup_stage(stage);

   setup_prepare( setup->setup );

   stage->point = do_point;
   stage->line = do_line;
   stage->tri = do_tri;
}


static void setup_first_point( struct draw_stage *stage,
			       struct prim_header *header )
{
   setup_begin(stage);
   stage->point( stage, header );
}

static void setup_first_line( struct draw_stage *stage,
			       struct prim_header *header )
{
   setup_begin(stage);
   stage->line( stage, header );
}


static void setup_first_tri( struct draw_stage *stage,
			       struct prim_header *header )
{
   setup_begin(stage);
   stage->tri( stage, header );
}



static void setup_flush( struct draw_stage *stage,
			 unsigned flags )
{
   stage->point = setup_first_point;
   stage->line = setup_first_line;
   stage->tri = setup_first_tri;
}


static void reset_stipple_counter( struct draw_stage *stage )
{
}


static void render_destroy( struct draw_stage *stage )
{
   struct setup_stage *ssetup = setup_stage(stage);
   setup_destroy_context(ssetup->setup);
   FREE( stage );
}


/**
 * Create a new primitive setup/render stage.
 */
struct draw_stage *sp_draw_render_stage( struct softpipe_context *softpipe )
{
   struct setup_stage *sstage = CALLOC_STRUCT(setup_stage);

   sstage->setup = setup_create_context(softpipe);
   sstage->stage.draw = softpipe->draw;
   sstage->stage.point = setup_first_point;
   sstage->stage.line = setup_first_line;
   sstage->stage.tri = setup_first_tri;
   sstage->stage.flush = setup_flush;
   sstage->stage.reset_stipple_counter = reset_stipple_counter;
   sstage->stage.destroy = render_destroy;

   return (struct draw_stage *)sstage;
}

struct setup_context *
sp_draw_setup_context( struct draw_stage *stage )
{
   struct setup_stage *ssetup = setup_stage(stage);
   return ssetup->setup;
}

void
sp_draw_flush( struct draw_stage *stage )
{
   stage->flush( stage, 0 );
}
