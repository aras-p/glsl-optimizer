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

#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"

#define FETCH_MAX 256
#define DRAW_MAX (FETCH_MAX+8)

struct varray_frontend {
   struct draw_pt_front_end base;
   struct draw_context *draw;

   ushort draw_elts[DRAW_MAX];
   unsigned fetch_elts[FETCH_MAX];

   unsigned driver_fetch_max;
   unsigned fetch_max;

   struct draw_pt_middle_end *middle;

   unsigned input_prim;
   unsigned output_prim;
};


static void varray_flush_linear(struct varray_frontend *varray,
                                unsigned start, unsigned count)
{
   if (count) {
      assert(varray->middle->run_linear);
      varray->middle->run_linear(varray->middle, start, count);
   }
}

static void varray_line_loop_segment(struct varray_frontend *varray,
                                     unsigned start,
                                     unsigned segment_start,
                                     unsigned segment_count,
                                     boolean end )
{
   assert(segment_count < varray->fetch_max);
   if (segment_count >= 1) {
      unsigned nr = 0, i;

      for (i = 0; i < segment_count; i++) 
         varray->fetch_elts[nr++] = start + segment_start + i;

      if (end) 
         varray->fetch_elts[nr++] = start;

      assert(nr <= FETCH_MAX);

      varray->middle->run(varray->middle, 
                          varray->fetch_elts,
                          nr,
                          varray->draw_elts, /* ie. linear */
                          nr);
   }
}



static void varray_fan_segment(struct varray_frontend *varray,
                               unsigned start, 
                               unsigned segment_start,
                               unsigned segment_count )
{
   assert(segment_count < varray->fetch_max);
   if (segment_count >= 2) {
      unsigned nr = 0, i;

      if (segment_start != 0)
         varray->fetch_elts[nr++] = start;

      for (i = 0 ; i < segment_count; i++) 
         varray->fetch_elts[nr++] = start + segment_start + i;

      assert(nr <= FETCH_MAX);

      varray->middle->run(varray->middle, 
                          varray->fetch_elts,
                          nr,
                          varray->draw_elts, /* ie. linear */
                          nr);
   }
}




#define FUNC varray_run
#include "draw_pt_varray_tmp_linear.h"

static unsigned decompose_prim[PIPE_PRIM_POLYGON + 1] = {
   PIPE_PRIM_POINTS,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINE_STRIP,        /* decomposed LINELOOP */
   PIPE_PRIM_LINE_STRIP,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLE_STRIP,
   PIPE_PRIM_TRIANGLE_FAN, 
   PIPE_PRIM_QUADS,
   PIPE_PRIM_QUAD_STRIP,
   PIPE_PRIM_POLYGON
};



static void varray_prepare(struct draw_pt_front_end *frontend,
                           unsigned prim,
                           struct draw_pt_middle_end *middle,
                           unsigned opt)
{
   struct varray_frontend *varray = (struct varray_frontend *)frontend;

   varray->base.run = varray_run;

   varray->input_prim = prim;
   varray->output_prim = decompose_prim[prim];

   varray->middle = middle;
   middle->prepare(middle, varray->output_prim, opt, &varray->driver_fetch_max );

   /* check that the max is even */
   assert((varray->driver_fetch_max & 1) == 0);

   varray->fetch_max = MIN2(FETCH_MAX, varray->driver_fetch_max);
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
   ushort i;
   struct varray_frontend *varray = CALLOC_STRUCT(varray_frontend);
   if (varray == NULL)
      return NULL;

   varray->base.prepare = varray_prepare;
   varray->base.run     = NULL;
   varray->base.finish  = varray_finish;
   varray->base.destroy = varray_destroy;
   varray->draw = draw;

   for (i = 0; i < DRAW_MAX; i++) {
      varray->draw_elts[i] = i;
   }

   return &varray->base;
}
