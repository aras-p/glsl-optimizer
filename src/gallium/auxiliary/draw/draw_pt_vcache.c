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

   ushort in[CACHE_MAX];
   ushort out[CACHE_MAX];

   ushort draw_elts[DRAW_MAX];
   unsigned fetch_elts[FETCH_MAX];

   unsigned draw_count;
   unsigned fetch_count;
   
   pt_elt_func elt_func;
   const void *elt_ptr;

   struct draw_pt_middle_end *middle;
   unsigned output_prim;
};

static void vcache_flush( struct vcache_frontend *vcache )
{
#if 0
   /* Should always be true if output_prim == input_prim, otherwise
    * not so much...
    */
   unsigned i;
   for (i = 0; i < vcache->draw_count; i++) {
      assert( vcache->fetch_elts[vcache->draw_elts[i]] == 
              vcache->elt_func(vcache->elt_ptr, i) );
   }
#endif

   if (vcache->draw_count)
      vcache->middle->run( vcache->middle,
                           vcache->output_prim,
                           vcache->fetch_elts,
                           vcache->fetch_count,
                           vcache->draw_elts,
                           vcache->draw_count );

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
   // ushort felt = elt(draw, i);

   ushort idx = felt % CACHE_MAX;

   if (vcache->in[idx] != felt) {
      assert(vcache->fetch_count < FETCH_MAX);

      vcache->in[idx] = felt;
      vcache->out[idx] = vcache->fetch_count;
      vcache->fetch_elts[vcache->fetch_count++] = felt;
   }

   vcache->draw_elts[vcache->draw_count++] = vcache->out[idx];
}
                   
static void vcache_triangle( struct vcache_frontend *vcache,
                             unsigned i0,
                             unsigned i1,
                             unsigned i2 )
{
   /* TODO: encode edgeflags in draw_elts */
   vcache_elt(vcache, i0);
   vcache_elt(vcache, i1);
   vcache_elt(vcache, i2);
   vcache_check_flush(vcache);
}

static void vcache_line( struct vcache_frontend *vcache,
                         boolean reset,
                         unsigned i0,
                         unsigned i1 )
{
   /* TODO: encode reset-line-stipple in draw_elts */
   (void) reset;
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


static void vcache_prepare( struct draw_pt_front_end *frontend,
                            struct draw_pt_middle_end *middle )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   vcache->middle = middle;
   middle->prepare( middle );
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


static void vcache_run( struct draw_pt_front_end *frontend, 
                         unsigned prim,
                         pt_elt_func get_elt,
                         const void *elts,
                         unsigned count )
{
   struct vcache_frontend *vcache = (struct vcache_frontend *)frontend;
   unsigned i;
   
   /* These are for validation only:
    */
   vcache->elt_func = get_elt;
   vcache->elt_ptr = elts;
   vcache->output_prim = reduced_prim[prim];

   switch (prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
         vcache_point( vcache,
                       get_elt(elts, i) );
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
                         get_elt(elts, i) );
         }

         vcache_line( vcache, 
                      0,
                      get_elt(elts, count - 1),
                      get_elt(elts, 0) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < count; i++) {
         vcache_line( vcache,
                      i == 1,
                      get_elt(elts, i - 1),
                      get_elt(elts, i) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
         vcache_triangle( vcache,
                          get_elt(elts, i + 0),
                          get_elt(elts, i + 1),
                          get_elt(elts, i + 2) );
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
         if (i & 1) {
            vcache_triangle( vcache,
                             get_elt(elts, i + 1),
                             get_elt(elts, i + 0),
                             get_elt(elts, i + 2) );
         }
         else {
            vcache_triangle( vcache,
                             get_elt(elts, i + 0),
                             get_elt(elts, i + 1),
                             get_elt(elts, i + 2) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      for (i = 0; i+2 < count; i++) {
         vcache_triangle( vcache,
                          get_elt(elts, 0),
                          get_elt(elts, i + 1),
                          get_elt(elts, i + 2) );
      }
      break;


   case PIPE_PRIM_QUADS:
      for (i = 0; i+3 < count; i += 4) {
         vcache_quad( vcache,
                      get_elt(elts, i + 0),
                      get_elt(elts, i + 1),
                      get_elt(elts, i + 2),
                      get_elt(elts, i + 3));
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
         vcache_quad( vcache,
                      get_elt(elts, i + 2),
                      get_elt(elts, i + 0),
                      get_elt(elts, i + 1),
                      get_elt(elts, i + 3));
      }
      break;

   case PIPE_PRIM_POLYGON:
      for (i = 0; i+2 < count; i++) {
         vcache_triangle( vcache,
                          get_elt(elts, i + 1),
                          get_elt(elts, i + 2),
                          get_elt(elts, 0));
      }
      break;

   default:
      assert(0);
      break;
   }
   
   vcache_flush( vcache );
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


struct draw_pt_front_end *draw_pt_vcache( void )
{
   struct vcache_frontend *vcache = CALLOC_STRUCT( vcache_frontend );
 
   vcache->base.prepare = vcache_prepare;
   vcache->base.run     = vcache_run;
   vcache->base.finish  = vcache_finish;
   vcache->base.destroy = vcache_destroy;
   
   memset(vcache->in, ~0, sizeof(vcache->in));
  
   return &vcache->base;
}
