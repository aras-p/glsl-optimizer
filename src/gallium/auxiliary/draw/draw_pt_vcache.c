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
  */

#include "pipe/p_util.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"


#define CACHE_MAX 32
#define FETCH_MAX 128
#define DRAW_MAX (16*1024)

struct vcache_frontend {
   struct draw_pt_front_end base;
   struct draw_context *draw;

   unsigned in[CACHE_MAX];
   ushort out[CACHE_MAX];

   ushort draw_elts[DRAW_MAX];
   unsigned fetch_elts[FETCH_MAX];

   unsigned draw_count;
   unsigned fetch_count;
   
   struct draw_pt_middle_end *middle;

   unsigned input_prim;
   unsigned output_prim;
};

static void vcache_flush( struct vcache_frontend *vcache )
{
   if (vcache->draw_count) {
      vcache->middle->run( vcache->middle,
                           vcache->fetch_elts,
                           vcache->fetch_count,
                           vcache->draw_elts,
                           vcache->draw_count );
   }

   memset(vcache->in, ~0, sizeof(vcache->in));
   vcache->fetch_count = 0;
   vcache->draw_count = 0;
}

static void vcache_check_flush( struct vcache_frontend *vcache )
{
   if ( vcache->draw_count + 6 >= DRAW_MAX ||
        vcache->fetch_count + 4 >= FETCH_MAX )
   {
      vcache_flush( vcache );
   }
}


static INLINE void vcache_elt( struct vcache_frontend *vcache,
                               unsigned felt,
                               ushort flags )
{
   unsigned idx = felt % CACHE_MAX;

   if (vcache->in[idx] != felt) {
      assert(vcache->fetch_count < FETCH_MAX);

      vcache->in[idx] = felt;
      vcache->out[idx] = (ushort)vcache->fetch_count;
      vcache->fetch_elts[vcache->fetch_count++] = felt;
   }

   vcache->draw_elts[vcache->draw_count++] = vcache->out[idx] | flags;
}


                   
static void vcache_triangle( struct vcache_frontend *vcache,
                             unsigned i0,
                             unsigned i1,
                             unsigned i2 )
{
   vcache_elt(vcache, i0, 0);
   vcache_elt(vcache, i1, 0);
   vcache_elt(vcache, i2, 0);
   vcache_check_flush(vcache);
}

			  
static void vcache_triangle_flags( struct vcache_frontend *vcache,
                                   ushort flags,
                                   unsigned i0,
                                   unsigned i1,
                                   unsigned i2 )
{
   vcache_elt(vcache, i0, flags);
   vcache_elt(vcache, i1, 0);
   vcache_elt(vcache, i2, 0);
   vcache_check_flush(vcache);
}

static void vcache_line( struct vcache_frontend *vcache,
                         unsigned i0,
                         unsigned i1 )
{
   vcache_elt(vcache, i0, 0);
   vcache_elt(vcache, i1, 0);
   vcache_check_flush(vcache);
}


static void vcache_line_flags( struct vcache_frontend *vcache,
                               ushort flags,
                               unsigned i0,
                               unsigned i1 )
{
   vcache_elt(vcache, i0, flags);
   vcache_elt(vcache, i1, 0);
   vcache_check_flush(vcache);
}


static void vcache_point( struct vcache_frontend *vcache,
                          unsigned i0 )
{
   vcache_elt(vcache, i0, 0);
   vcache_check_flush(vcache);
}

static void vcache_quad( struct vcache_frontend *vcache,
                         unsigned i0,
                         unsigned i1,
                         unsigned i2,
                         unsigned i3 )
{
   vcache_triangle( vcache, i0, i1, i3 );
   vcache_triangle( vcache, i1, i2, i3 );
}

static void vcache_ef_quad( struct vcache_frontend *vcache,
                            unsigned i0,
                            unsigned i1,
                            unsigned i2,
                            unsigned i3 )
{
   const unsigned omitEdge1 = DRAW_PIPE_EDGE_FLAG_0 | DRAW_PIPE_EDGE_FLAG_2;
   const unsigned omitEdge2 = DRAW_PIPE_EDGE_FLAG_0 | DRAW_PIPE_EDGE_FLAG_1;

   vcache_triangle_flags( vcache, 
                          DRAW_PIPE_RESET_STIPPLE | omitEdge1, 
                          i0, i1, i3 );

   vcache_triangle_flags( vcache, 
                          omitEdge2, 
                          i1, i2, i3 );
}

/* At least for now, we're back to using a template include file for
 * this.  The two paths aren't too different though - it may be
 * possible to reunify them.
 */
#define TRIANGLE(vc,flags,i0,i1,i2) vcache_triangle_flags(vc,flags,i0,i1,i2)
#define QUAD(vc,i0,i1,i2,i3)        vcache_ef_quad(vc,i0,i1,i2,i3)
#define LINE(vc,flags,i0,i1)        vcache_line_flags(vc,flags,i0,i1)
#define POINT(vc,i0)                vcache_point(vc,i0)
#define FUNC vcache_run_extras
#include "draw_pt_vcache_tmp.h"

#define TRIANGLE(vc,flags,i0,i1,i2) vcache_triangle(vc,i0,i1,i2)
#define QUAD(vc,i0,i1,i2,i3)        vcache_quad(vc,i0,i1,i2,i3)
#define LINE(vc,flags,i0,i1)        vcache_line(vc,i0,i1)
#define POINT(vc,i0)                vcache_point(vc,i0)
#define FUNC vcache_run
#include "draw_pt_vcache_tmp.h"




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



static void vcache_prepare( struct draw_pt_front_end *frontend,
                            unsigned prim,
                            struct draw_pt_middle_end *middle,
			    unsigned opt )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;

   if (opt & PT_PIPELINE)
   {
      vcache->base.run = vcache_run_extras;
   }
   else 
   {
      vcache->base.run = vcache_run;
   }

   vcache->input_prim = prim;
   vcache->output_prim = reduced_prim[prim];

   vcache->middle = middle;
   middle->prepare( middle, vcache->output_prim, opt );
}




static void vcache_finish( struct draw_pt_front_end *frontend )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   vcache->middle->finish( vcache->middle );
   vcache->middle = NULL;
}

static void vcache_destroy( struct draw_pt_front_end *frontend )
{
   FREE(frontend);
}


struct draw_pt_front_end *draw_pt_vcache( struct draw_context *draw )
{
   struct vcache_frontend *vcache = CALLOC_STRUCT( vcache_frontend );
   if (vcache == NULL)
      return NULL;
 
   vcache->base.prepare = vcache_prepare;
   vcache->base.run     = NULL;
   vcache->base.finish  = vcache_finish;
   vcache->base.destroy = vcache_destroy;
   vcache->draw = draw;
   
   memset(vcache->in, ~0, sizeof(vcache->in));
  
   return &vcache->base;
}
