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

#include "util/u_memory.h"
#include "pipe/p_context.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_pt.h"

struct pt_post_vs {
   struct draw_context *draw;

   boolean (*run)( struct pt_post_vs *pvs,
		struct vertex_header *vertices,
		unsigned count,
		unsigned stride );
};



static INLINE float
dot4(const float *a, const float *b)
{
   return (a[0]*b[0] +
           a[1]*b[1] +
           a[2]*b[2] +
           a[3]*b[3]);
}



static INLINE unsigned
compute_clipmask_gl(const float *clip, /*const*/ float plane[][4], unsigned nr)
{
   unsigned mask = 0x0;
   unsigned i;

#if 0
   debug_printf("compute clipmask %f %f %f %f\n",
                clip[0], clip[1], clip[2], clip[3]);
   assert(clip[3] != 0.0);
#endif

   /* Do the hardwired planes first:
    */
   if (-clip[0] + clip[3] < 0) mask |= (1<<0);
   if ( clip[0] + clip[3] < 0) mask |= (1<<1);
   if (-clip[1] + clip[3] < 0) mask |= (1<<2);
   if ( clip[1] + clip[3] < 0) mask |= (1<<3);
   if ( clip[2] + clip[3] < 0) mask |= (1<<4); /* match mesa clipplane numbering - for now */
   if (-clip[2] + clip[3] < 0) mask |= (1<<5); /* match mesa clipplane numbering - for now */

   /* Followed by any remaining ones:
    */
   for (i = 6; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0) 
         mask |= (1<<i);
   }

   return mask;
}


/* The normal case - cliptest, rhw divide, viewport transform.
 *
 * Also handle identity viewport here at the expense of a few wasted
 * instructions
 */
static boolean post_vs_cliptest_viewport_gl( struct pt_post_vs *pvs,
					  struct vertex_header *vertices,
					  unsigned count,
					  unsigned stride )
{
   struct vertex_header *out = vertices;
   const float *scale = pvs->draw->viewport.scale;
   const float *trans = pvs->draw->viewport.translate;
   const unsigned pos = draw_current_shader_position_output(pvs->draw);
   unsigned clipped = 0;
   unsigned j;

   if (0) debug_printf("%s\n", __FUNCTION__);

   for (j = 0; j < count; j++) {
      float *position = out->data[pos];

      out->clip[0] = position[0];
      out->clip[1] = position[1];
      out->clip[2] = position[2];
      out->clip[3] = position[3];

      out->vertex_id = 0xffff;
      out->clipmask = compute_clipmask_gl(out->clip, 
					  pvs->draw->plane,
					  pvs->draw->nr_planes);
      clipped += out->clipmask;

      if (out->clipmask == 0)
      {
	 /* divide by w */
	 float w = 1.0f / position[3];

	 /* Viewport mapping */
	 position[0] = position[0] * w * scale[0] + trans[0];
	 position[1] = position[1] * w * scale[1] + trans[1];
	 position[2] = position[2] * w * scale[2] + trans[2];
	 position[3] = w;
#if 0
         debug_printf("post viewport: %f %f %f %f\n",
                      position[0],
                      position[1],
                      position[2],
                      position[3]);
#endif
      }

      out = (struct vertex_header *)( (char *)out + stride );
   }

   return clipped != 0;
}



/* As above plus edgeflags
 */
static boolean 
post_vs_cliptest_viewport_gl_edgeflag(struct pt_post_vs *pvs,
                                      struct vertex_header *vertices,
                                      unsigned count,
                                      unsigned stride )
{
   unsigned j;
   boolean needpipe;

   needpipe = post_vs_cliptest_viewport_gl( pvs, vertices, count, stride);

   /* If present, copy edgeflag VS output into vertex header.
    * Otherwise, leave header as is.
    */
   if (pvs->draw->vs.edgeflag_output) {
      struct vertex_header *out = vertices;
      int ef = pvs->draw->vs.edgeflag_output;

      for (j = 0; j < count; j++) {
         const float *edgeflag = out->data[ef];
         out->edgeflag = !(edgeflag[0] != 1.0f);
         needpipe |= !out->edgeflag;
         out = (struct vertex_header *)( (char *)out + stride );
      }
   }
   return needpipe;
}




/* If bypass_clipping is set, skip cliptest and rhw divide.
 */
static boolean post_vs_viewport( struct pt_post_vs *pvs,
			      struct vertex_header *vertices,
			      unsigned count,
			      unsigned stride )
{
   struct vertex_header *out = vertices;
   const float *scale = pvs->draw->viewport.scale;
   const float *trans = pvs->draw->viewport.translate;
   const unsigned pos = draw_current_shader_position_output(pvs->draw);
   unsigned j;

   if (0) debug_printf("%s\n", __FUNCTION__);
   for (j = 0; j < count; j++) {
      float *position = out->data[pos];

      /* Viewport mapping only, no cliptest/rhw divide
       */
      position[0] = position[0] * scale[0] + trans[0];
      position[1] = position[1] * scale[1] + trans[1];
      position[2] = position[2] * scale[2] + trans[2];

      out = (struct vertex_header *)((char *)out + stride);
   }
   
   return FALSE;
}


/* If bypass_clipping is set and we have an identity viewport, nothing
 * to do.
 */
static boolean post_vs_none( struct pt_post_vs *pvs,
			     struct vertex_header *vertices,
			     unsigned count,
			     unsigned stride )
{
   if (0) debug_printf("%s\n", __FUNCTION__);
   return FALSE;
}

boolean draw_pt_post_vs_run( struct pt_post_vs *pvs,
			     struct vertex_header *pipeline_verts,
			     unsigned count,
			     unsigned stride )
{
   return pvs->run( pvs, pipeline_verts, count, stride );
}


void draw_pt_post_vs_prepare( struct pt_post_vs *pvs,
			      boolean bypass_clipping,
			      boolean bypass_viewport,
			      boolean opengl,
			      boolean need_edgeflags )
{
   if (!need_edgeflags) {
      if (bypass_clipping) {
         if (bypass_viewport)
            pvs->run = post_vs_none;
         else
            pvs->run = post_vs_viewport;
      }
      else {
         /* if (opengl) */
         pvs->run = post_vs_cliptest_viewport_gl;
      }
   }
   else {
      /* If we need to copy edgeflags to the vertex header, it should
       * mean we're running the primitive pipeline.  Hence the bypass
       * flags should be false.
       */
      assert(!bypass_clipping);
      assert(!bypass_viewport);
      pvs->run = post_vs_cliptest_viewport_gl_edgeflag;
   }
}


struct pt_post_vs *draw_pt_post_vs_create( struct draw_context *draw )
{
   struct pt_post_vs *pvs = CALLOC_STRUCT( pt_post_vs );
   if (!pvs)
      return NULL;

   pvs->draw = draw;
   
   return pvs;
}

void draw_pt_post_vs_destroy( struct pt_post_vs *pvs )
{
   FREE(pvs);
}
