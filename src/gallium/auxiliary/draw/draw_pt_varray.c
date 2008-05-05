/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"

#define FETCH_MAX 256
#define DRAW_MAX (16*FETCH_MAX)

struct varray_frontend {
   struct draw_pt_front_end base;
   struct draw_context *draw;

   ushort draw_elts[DRAW_MAX];
   unsigned fetch_elts[FETCH_MAX];

   unsigned draw_count;
   unsigned fetch_count;

   struct draw_pt_middle_end *middle;

   unsigned input_prim;
   unsigned output_prim;
};

static void varray_flush(struct varray_frontend *varray)
{
   if (varray->draw_count) {
#if 0
      debug_printf("FLUSH fc = %d, dc = %d\n",
                   varray->fetch_count,
                   varray->draw_count);
#endif
      varray->middle->run(varray->middle,
                          varray->fetch_elts,
                          varray->fetch_count,
                          varray->draw_elts,
                          varray->draw_count);
   }

   varray->fetch_count = 0;
   varray->draw_count = 0;
}

#if 0
static void varray_check_flush(struct varray_frontend *varray)
{
   if (varray->draw_count + 6 >= DRAW_MAX/* ||
       varray->fetch_count + 4 >= FETCH_MAX*/) {
      varray_flush(varray);
   }
}
#endif

static INLINE void add_draw_el(struct varray_frontend *varray,
                               int idx, ushort flags)
{
   varray->draw_elts[varray->draw_count++] = idx | flags;
}


static INLINE void varray_triangle( struct varray_frontend *varray,
                                    unsigned i0,
                                    unsigned i1,
                                    unsigned i2 )
{
   add_draw_el(varray, i0, 0);
   add_draw_el(varray, i1, 0);
   add_draw_el(varray, i2, 0);
}

static INLINE void varray_triangle_flags( struct varray_frontend *varray,
                                          ushort flags,
                                          unsigned i0,
                                          unsigned i1,
                                          unsigned i2 )
{
   add_draw_el(varray, i0, flags);
   add_draw_el(varray, i1, 0);
   add_draw_el(varray, i2, 0);
}

static INLINE void varray_line( struct varray_frontend *varray,
                                unsigned i0,
                                unsigned i1 )
{
   add_draw_el(varray, i0, 0);
   add_draw_el(varray, i1, 0);
}


static INLINE void varray_line_flags( struct varray_frontend *varray,
                                      ushort flags,
                                      unsigned i0,
                                      unsigned i1 )
{
   add_draw_el(varray, i0, flags);
   add_draw_el(varray, i1, 0);
}


static INLINE void varray_point( struct varray_frontend *varray,
                                 unsigned i0 )
{
   add_draw_el(varray, i0, 0);
}

static INLINE void varray_quad( struct varray_frontend *varray,
                                unsigned i0,
                                unsigned i1,
                                unsigned i2,
                                unsigned i3 )
{
   varray_triangle( varray, i0, i1, i3 );
   varray_triangle( varray, i1, i2, i3 );
}

static INLINE void varray_ef_quad( struct varray_frontend *varray,
                                   unsigned i0,
                                   unsigned i1,
                                   unsigned i2,
                                   unsigned i3 )
{
   const unsigned omitEdge1 = DRAW_PIPE_EDGE_FLAG_0 | DRAW_PIPE_EDGE_FLAG_2;
   const unsigned omitEdge2 = DRAW_PIPE_EDGE_FLAG_0 | DRAW_PIPE_EDGE_FLAG_1;

   varray_triangle_flags( varray,
                          DRAW_PIPE_RESET_STIPPLE | omitEdge1,
                          i0, i1, i3 );

   varray_triangle_flags( varray,
                          omitEdge2,
                          i1, i2, i3 );
}

/* At least for now, we're back to using a template include file for
 * this.  The two paths aren't too different though - it may be
 * possible to reunify them.
 */
#define TRIANGLE(vc,flags,i0,i1,i2) varray_triangle_flags(vc,flags,i0,i1,i2)
#define QUAD(vc,i0,i1,i2,i3)        varray_ef_quad(vc,i0,i1,i2,i3)
#define LINE(vc,flags,i0,i1)        varray_line_flags(vc,flags,i0,i1)
#define POINT(vc,i0)                varray_point(vc,i0)
#define FUNC varray_run_extras
#include "draw_pt_varray_tmp.h"

#define TRIANGLE(vc,flags,i0,i1,i2) varray_triangle(vc,i0,i1,i2)
#define QUAD(vc,i0,i1,i2,i3)        varray_quad(vc,i0,i1,i2,i3)
#define LINE(vc,flags,i0,i1)        varray_line(vc,i0,i1)
#define POINT(vc,i0)                varray_point(vc,i0)
#define FUNC varray_run
#include "draw_pt_varray_tmp.h"



static unsigned reduced_prim[PIPE_PRIM_POLYGON + 1] = {
   PIPE_PRIM_POINTS,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES
};



static void varray_prepare(struct draw_pt_front_end *frontend,
                           unsigned prim,
                           struct draw_pt_middle_end *middle,
                           unsigned opt)
{
   struct varray_frontend *varray = (struct varray_frontend *)frontend;

   if (opt & PT_PIPELINE)
   {
      varray->base.run = varray_run_extras;
   } 
   else 
   {
      varray->base.run = varray_run;
   }

   varray->input_prim = prim;
   varray->output_prim = reduced_prim[prim];

   varray->middle = middle;
   middle->prepare(middle, varray->output_prim, opt);
}




static void varray_finish(struct draw_pt_front_end *frontend)
{
   struct varray_frontend *varray = (struct varray_frontend *)frontend;
   varray->middle->finish(varray->middle);
   varray->middle = NULL;
}

static void varray_destroy(struct draw_pt_front_end *frontend)
{
   FREE(frontend);
}


struct draw_pt_front_end *draw_pt_varray(struct draw_context *draw)
{
   struct varray_frontend *varray = CALLOC_STRUCT(varray_frontend);
   if (varray == NULL)
      return NULL;

   varray->base.prepare = varray_prepare;
   varray->base.run     = NULL;
   varray->base.finish  = varray_finish;
   varray->base.destroy = varray_destroy;
   varray->draw = draw;

   return &varray->base;
}
