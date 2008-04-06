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
//#include "draw/draw_vbuf.h"
//#include "draw/draw_vertex.h"
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


static void vcache_elt( struct vcache_frontend *vcache,
                        unsigned felt )
{
   unsigned idx = felt % CACHE_MAX;

   if (vcache->in[idx] != felt) {
      assert(vcache->fetch_count < FETCH_MAX);

      vcache->in[idx] = felt;
      vcache->out[idx] = (ushort)vcache->fetch_count;
      vcache->fetch_elts[vcache->fetch_count++] = felt;
   }

   vcache->draw_elts[vcache->draw_count++] = vcache->out[idx];
}

static unsigned add_edgeflag( struct vcache_frontend *vcache,
                              unsigned idx, 
                              unsigned mask )
{
   if (mask && draw_get_edgeflag(vcache->draw, idx)) 
      return idx | DRAW_PT_EDGEFLAG;
   else
      return idx;
}


static unsigned add_reset_stipple( unsigned idx,
                                   unsigned reset )
{
   if (reset)
      return idx | DRAW_PT_RESET_STIPPLE;
   else
      return idx;
}

                   
static void vcache_triangle( struct vcache_frontend *vcache,
                             unsigned i0,
                             unsigned i1,
                             unsigned i2 )
{
   vcache_elt(vcache, i0 | DRAW_PT_EDGEFLAG | DRAW_PT_RESET_STIPPLE);
   vcache_elt(vcache, i1 | DRAW_PT_EDGEFLAG);
   vcache_elt(vcache, i2 | DRAW_PT_EDGEFLAG);
   vcache_check_flush(vcache);
}

			  
static void vcache_ef_triangle( struct vcache_frontend *vcache,
                                boolean reset_stipple,
                                unsigned ef_mask,
                                unsigned i0,
                                unsigned i1,
                                unsigned i2 )
{
   i0 = add_edgeflag( vcache, i0, (ef_mask >> 0) & 1 );
   i1 = add_edgeflag( vcache, i1, (ef_mask >> 1) & 1 );
   i2 = add_edgeflag( vcache, i2, (ef_mask >> 2) & 1 );

   i0 = add_reset_stipple( i0, reset_stipple );

   vcache_elt(vcache, i0);
   vcache_elt(vcache, i1);
   vcache_elt(vcache, i2);
   vcache_check_flush(vcache);

   if (0) debug_printf("emit tri ef: %d %d %d\n", 
                       !!(i0 & DRAW_PT_EDGEFLAG),
                       !!(i1 & DRAW_PT_EDGEFLAG),
                       !!(i2 & DRAW_PT_EDGEFLAG));
   
}


static void vcache_line( struct vcache_frontend *vcache,
                         boolean reset_stipple,
                         unsigned i0,
                         unsigned i1 )
{
   i0 = add_reset_stipple( i0, reset_stipple );

   vcache_elt(vcache, i0);
   vcache_elt(vcache, i1);
   vcache_check_flush(vcache);
}


static void vcache_point( struct vcache_frontend *vcache,
                          unsigned i0 )
{
   vcache_elt(vcache, i0);
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
   const unsigned omitEdge2 = ~(1 << 1);
   const unsigned omitEdge3 = ~(1 << 2);
   vcache_ef_triangle( vcache, 1, omitEdge2, i0, i1, i3 );
   vcache_ef_triangle( vcache, 0, omitEdge3, i1, i2, i3 );
}




static void vcache_run( struct draw_pt_front_end *frontend, 
                        pt_elt_func get_elt,
                        const void *elts,
                        unsigned count )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   struct draw_context *draw = vcache->draw;

   boolean unfilled = (draw->rasterizer->fill_cw != PIPE_POLYGON_MODE_FILL ||
		       draw->rasterizer->fill_ccw != PIPE_POLYGON_MODE_FILL);

   boolean flatfirst = (draw->rasterizer->flatshade && 
                        draw->rasterizer->flatshade_first);
   unsigned i;

//   debug_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   switch (vcache->input_prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 vcache_point( vcache,
                       get_elt(elts, i + 0) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
         vcache_line( vcache, 
                      TRUE,
                      get_elt(elts, i + 0),
                      get_elt(elts, i + 1));
      }
      break;

   case PIPE_PRIM_LINE_LOOP:  
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    vcache_line( vcache, 
                         i == 1, 	/* XXX: only if vb not split */
                         get_elt(elts, i - 1),
                         get_elt(elts, i ));
	 }

	 vcache_line( vcache, 
                      0,
                      get_elt(elts, count - 1),
                      get_elt(elts, 0 ));
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < count; i++) {
         vcache_line( vcache,
                      i == 1,
                      get_elt(elts, i - 1),
                      get_elt(elts, i ));
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (unfilled) {
         for (i = 0; i+2 < count; i += 3) {
            vcache_ef_triangle( vcache,
                                1, 
                                ~0,
                                get_elt(elts, i + 0),
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2 ));
         }
      } 
      else {
         for (i = 0; i+2 < count; i += 3) {
            vcache_triangle( vcache,
                             get_elt(elts, i + 0),
                             get_elt(elts, i + 1),
                             get_elt(elts, i + 2 ));
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (flatfirst) {
         for (i = 0; i+2 < count; i++) {
            if (i & 1) {
               vcache_triangle( vcache,
                                get_elt(elts, i + 0),
                                get_elt(elts, i + 2),
                                get_elt(elts, i + 1 ));
            }
            else {
               vcache_triangle( vcache,
                                get_elt(elts, i + 0),
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2 ));
            }
         }
      }
      else {
         for (i = 0; i+2 < count; i++) {
            if (i & 1) {
               vcache_triangle( vcache,
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 0),
                                get_elt(elts, i + 2 ));
            }
            else {
               vcache_triangle( vcache,
                                get_elt(elts, i + 0),
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2 ));
            }
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
         if (flatfirst) {
            for (i = 0; i+2 < count; i++) {
               vcache_triangle( vcache,
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2),
                                get_elt(elts, 0 ));
            }
         }
         else {
            for (i = 0; i+2 < count; i++) {
               vcache_triangle( vcache,
                                get_elt(elts, 0),
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2 ));
            }
         }
      }
      break;


   case PIPE_PRIM_QUADS:
      if (unfilled) {
	 for (i = 0; i+3 < count; i += 4) {
	    vcache_ef_quad( vcache,
                            get_elt(elts, i + 0),
                            get_elt(elts, i + 1),
                            get_elt(elts, i + 2),
                            get_elt(elts, i + 3));
	 }
      }
      else {
	 for (i = 0; i+3 < count; i += 4) {
	    vcache_quad( vcache,
                         get_elt(elts, i + 0),
                         get_elt(elts, i + 1),
                         get_elt(elts, i + 2),
                         get_elt(elts, i + 3));
	 }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (unfilled) {
	 for (i = 0; i+3 < count; i += 2) {
	    vcache_ef_quad( vcache,
                            get_elt(elts, i + 2),
                            get_elt(elts, i + 0),
                            get_elt(elts, i + 1),
                            get_elt(elts, i + 3));
	 }
      }
      else {
	 for (i = 0; i+3 < count; i += 2) {
	    vcache_quad( vcache,
                         get_elt(elts, i + 2),
                         get_elt(elts, i + 0),
                         get_elt(elts, i + 1),
                         get_elt(elts, i + 3));
	 }
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (unfilled) {
         /* These bitflags look a little odd because we submit the
          * vertices as (1,2,0) to satisfy flatshade requirements.  
          */
         const unsigned edge_first  = (1<<2);
         const unsigned edge_middle = (1<<0);
         const unsigned edge_last   = (1<<1);

	 for (i = 0; i+2 < count; i++) {
            unsigned ef_mask = edge_middle;

            if (i == 0)
               ef_mask |= edge_first;

            if (i + 3 == count)
	       ef_mask |= edge_last;

	    vcache_ef_triangle( vcache,
                                i == 0,
                                ef_mask,
                                get_elt(elts, i + 1),
                                get_elt(elts, i + 2),
                                get_elt(elts, 0));
	 }
      }
      else {
	 for (i = 0; i+2 < count; i++) {
	    vcache_triangle( vcache,
                             get_elt(elts, i + 1),
                             get_elt(elts, i + 2),
                             get_elt(elts, 0));
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
   
   vcache_flush( vcache );
}



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
                            struct draw_pt_middle_end *middle )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;

/*
   if (vcache->draw->rasterizer->flatshade_first)
      vcache->base.run = vcache_run_pv0;
   else
      vcache->base.run = vcache_run_pv2;
*/ 

   vcache->base.run = vcache_run;
   vcache->input_prim = prim;
   vcache->output_prim = reduced_prim[prim];

   vcache->middle = middle;
   middle->prepare( middle, vcache->output_prim );
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
 
   vcache->base.prepare = vcache_prepare;
   vcache->base.run     = NULL;
   vcache->base.finish  = vcache_finish;
   vcache->base.destroy = vcache_destroy;
   vcache->draw = draw;
   
   memset(vcache->in, ~0, sizeof(vcache->in));
  
   return &vcache->base;
}
