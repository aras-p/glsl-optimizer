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
 * \brief  Primitive rasterization/rendering (points, lines, triangles)
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "sp_setup.h"

#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_state.h"
#include "sp_prim_setup.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vertex.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"


#define DEBUG_VERTS 0
#define DEBUG_FRAGS 0

/**
 * Triangle edge info
 */
struct edge {
   float dx;		/**< X(v1) - X(v0), used only during setup */
   float dy;		/**< Y(v1) - Y(v0), used only during setup */
   float dxdy;		/**< dx/dy */
   float sx, sy;	/**< first sample point coord */
   int lines;		/**< number of lines on this edge */
};

#if SP_NUM_QUAD_THREADS > 1

/* Set to 1 if you want other threads to be instantly
 * notified of pending jobs.
 */
#define INSTANT_NOTEMPTY_NOTIFY 0

struct thread_info
{
   struct setup_context *setup;
   uint id;
   pipe_thread handle;
};

struct quad_job;

typedef void (* quad_job_routine)( struct setup_context *setup, uint thread, struct quad_job *job );

struct quad_job
{
   struct quad_header_input input;
   struct quad_header_inout inout;
   quad_job_routine routine;
};

#define NUM_QUAD_JOBS 64

struct quad_job_que
{
   struct quad_job jobs[NUM_QUAD_JOBS];
   uint first;
   uint last;
   pipe_mutex que_mutex;
   pipe_condvar que_notfull_condvar;
   pipe_condvar que_notempty_condvar;
   uint jobs_added;
   uint jobs_done;
   pipe_condvar que_done_condvar;
};

static void
add_quad_job( struct quad_job_que *que, struct quad_header *quad, quad_job_routine routine )
{
#if INSTANT_NOTEMPTY_NOTIFY
   boolean empty;
#endif

   /* Wait for empty slot, see if the que is empty.
    */
   pipe_mutex_lock( que->que_mutex );
   while ((que->last + 1) % NUM_QUAD_JOBS == que->first) {
#if !INSTANT_NOTEMPTY_NOTIFY
      pipe_condvar_broadcast( que->que_notempty_condvar );
#endif
      pipe_condvar_wait( que->que_notfull_condvar, que->que_mutex );
   }
#if INSTANT_NOTEMPTY_NOTIFY
   empty = que->last == que->first;
#endif
   que->jobs_added++;
   pipe_mutex_unlock( que->que_mutex );

   /* Submit new job.
    */
   que->jobs[que->last].input = quad->input;
   que->jobs[que->last].inout = quad->inout;
   que->jobs[que->last].routine = routine;
   que->last = (que->last + 1) % NUM_QUAD_JOBS;

#if INSTANT_NOTEMPTY_NOTIFY
   /* If the que was empty, notify consumers there's a job to be done.
    */
   if (empty) {
      pipe_mutex_lock( que->que_mutex );
      pipe_condvar_broadcast( que->que_notempty_condvar );
      pipe_mutex_unlock( que->que_mutex );
   }
#endif
}

#endif

/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_context {
   struct softpipe_context *softpipe;

   /* Vertices are just an array of floats making up each attribute in
    * turn.  Currently fixed at 4 floats, but should change in time.
    * Codegen will help cope with this.
    */
   const float (*vmax)[4];
   const float (*vmid)[4];
   const float (*vmin)[4];
   const float (*vprovoke)[4];

   struct edge ebot;
   struct edge etop;
   struct edge emaj;

   float oneoverarea;

   struct tgsi_interp_coef coef[PIPE_MAX_SHADER_INPUTS];
   struct tgsi_interp_coef posCoef;  /* For Z, W */
   struct quad_header quad;

#if SP_NUM_QUAD_THREADS > 1
   struct quad_job_que que;
   struct thread_info threads[SP_NUM_QUAD_THREADS];
#endif

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
      unsigned y_flags;
      unsigned mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;

#if DEBUG_FRAGS
   uint numFragsEmitted;  /**< per primitive */
   uint numFragsWritten;  /**< per primitive */
#endif

   unsigned winding;		/* which winding to cull */
};

#if SP_NUM_QUAD_THREADS > 1

static PIPE_THREAD_ROUTINE( quad_thread, param )
{
   struct thread_info *info = (struct thread_info *) param;
   struct quad_job_que *que = &info->setup->que;

   for (;;) {
      struct quad_job job;
      boolean full;

      /* Wait for an available job.
       */
      pipe_mutex_lock( que->que_mutex );
      while (que->last == que->first)
         pipe_condvar_wait( que->que_notempty_condvar, que->que_mutex );

      /* See if the que is full.
       */
      full = (que->last + 1) % NUM_QUAD_JOBS == que->first;

      /* Take a job and remove it from que.
       */
      job = que->jobs[que->first];
      que->first = (que->first + 1) % NUM_QUAD_JOBS;

      /* Notify the producer if the que is not full.
       */
      if (full)
         pipe_condvar_signal( que->que_notfull_condvar );
      pipe_mutex_unlock( que->que_mutex );

      job.routine( info->setup, info->id, &job );

      /* Notify the producer if that's the last finished job.
       */
      pipe_mutex_lock( que->que_mutex );
      que->jobs_done++;
      if (que->jobs_added == que->jobs_done)
         pipe_condvar_signal( que->que_done_condvar );
      pipe_mutex_unlock( que->que_mutex );
   }

   return NULL;
}

#define WAIT_FOR_COMPLETION(setup) \
   do {\
      pipe_mutex_lock( setup->que.que_mutex );\
      if (!INSTANT_NOTEMPTY_NOTIFY)\
         pipe_condvar_broadcast( setup->que.que_notempty_condvar );\
      while (setup->que.jobs_added != setup->que.jobs_done)\
         pipe_condvar_wait( setup->que.que_done_condvar, setup->que.que_mutex );\
      pipe_mutex_unlock( setup->que.que_mutex );\
   } while (0)

#else

#define WAIT_FOR_COMPLETION(setup) ((void) 0)

#endif

/**
 * Test if x is NaN or +/- infinity.
 */
static INLINE boolean
is_inf_or_nan(float x)
{
   union fi tmp;
   tmp.f = x;
   return !(int)((unsigned int)((tmp.i & 0x7fffffff)-0x7f800000) >> 31);
}


static boolean cull_tri( struct setup_context *setup,
		      float det )
{
   if (det != 0) 
   {   
      /* if (det < 0 then Z points toward camera and triangle is 
       * counter-clockwise winding.
       */
      unsigned winding = (det < 0) ? PIPE_WINDING_CCW : PIPE_WINDING_CW;
      
      if ((winding & setup->winding) == 0) 
	 return FALSE;
   }

   /* Culled:
    */
   return TRUE;
}



/**
 * Clip setup->quad against the scissor/surface bounds.
 */
static INLINE void
quad_clip( struct setup_context *setup, struct quad_header *quad )
{
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;

   if (quad->input.x0 >= maxx ||
       quad->input.y0 >= maxy ||
       quad->input.x0 + 1 < minx ||
       quad->input.y0 + 1 < miny) {
      /* totally clipped */
      quad->inout.mask = 0x0;
      return;
   }
   if (quad->input.x0 < minx)
      quad->inout.mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
   if (quad->input.y0 < miny)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
   if (quad->input.x0 == maxx - 1)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
   if (quad->input.y0 == maxy - 1)
      quad->inout.mask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
}


/**
 * Emit a quad (pass to next stage) with clipping.
 */
static INLINE void
clip_emit_quad( struct setup_context *setup, struct quad_header *quad, uint thread )
{
   quad_clip( setup, quad );
   if (quad->inout.mask) {
      struct softpipe_context *sp = setup->softpipe;

      sp->quad[thread].first->run( sp->quad[thread].first, quad );
   }
}

#if SP_NUM_QUAD_THREADS > 1

static void
clip_emit_quad_job( struct setup_context *setup, uint thread, struct quad_job *job )
{
   struct quad_header quad;

   quad.input = job->input;
   quad.inout = job->inout;
   quad.coef = setup->quad.coef;
   quad.posCoef = setup->quad.posCoef;
   quad.nr_attrs = setup->quad.nr_attrs;
   clip_emit_quad( setup, &quad, thread );
}

#define CLIP_EMIT_QUAD(setup) add_quad_job( &setup->que, &setup->quad, clip_emit_quad_job )

#else

#define CLIP_EMIT_QUAD(setup) clip_emit_quad( setup, &setup->quad, 0 )

#endif

/**
 * Emit a quad (pass to next stage).  No clipping is done.
 */
static INLINE void
emit_quad( struct setup_context *setup, struct quad_header *quad, uint thread )
{
   struct softpipe_context *sp = setup->softpipe;
#if DEBUG_FRAGS
   uint mask = quad->inout.mask;
#endif

#if DEBUG_FRAGS
   if (mask & 1) setup->numFragsEmitted++;
   if (mask & 2) setup->numFragsEmitted++;
   if (mask & 4) setup->numFragsEmitted++;
   if (mask & 8) setup->numFragsEmitted++;
#endif
   sp->quad[thread].first->run( sp->quad[thread].first, quad );
#if DEBUG_FRAGS
   mask = quad->inout.mask;
   if (mask & 1) setup->numFragsWritten++;
   if (mask & 2) setup->numFragsWritten++;
   if (mask & 4) setup->numFragsWritten++;
   if (mask & 8) setup->numFragsWritten++;
#endif
}

#if SP_NUM_QUAD_THREADS > 1

static void
emit_quad_job( struct setup_context *setup, uint thread, struct quad_job *job )
{
   struct quad_header quad;

   quad.input = job->input;
   quad.inout = job->inout;
   quad.coef = setup->quad.coef;
   quad.posCoef = setup->quad.posCoef;
   quad.nr_attrs = setup->quad.nr_attrs;
   emit_quad( setup, &quad, thread );
}

#define EMIT_QUAD(setup,x,y,mask) do {\
      setup->quad.input.x0 = x;\
      setup->quad.input.y0 = y;\
      setup->quad.inout.mask = mask;\
      add_quad_job( &setup->que, &setup->quad, emit_quad_job );\
   } while (0)

#else

#define EMIT_QUAD(setup,x,y,mask) do {\
      setup->quad.input.x0 = x;\
      setup->quad.input.y0 = y;\
      setup->quad.inout.mask = mask;\
      emit_quad( setup, &setup->quad, 0 );\
   } while (0)

#endif

/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static INLINE int block( int x )
{
   return x & ~1;
}


/**
 * Render a horizontal span of quads
 */
static void flush_spans( struct setup_context *setup )
{
   const int xleft0 = setup->span.left[0];
   const int xleft1 = setup->span.left[1];
   const int xright0 = setup->span.right[0];
   const int xright1 = setup->span.right[1];
   int minleft, maxright;
   int x;

   switch (setup->span.y_flags) {
   case 0x3:
      /* both odd and even lines written (both quad rows) */
      minleft = block(MIN2(xleft0, xleft1));
      maxright = block(MAX2(xright0, xright1));
      for (x = minleft; x <= maxright; x += 2) {
         /* determine which of the four pixels is inside the span bounds */
         uint mask = 0x0;
         if (x >= xleft0 && x < xright0)
            mask |= MASK_TOP_LEFT;
         if (x >= xleft1 && x < xright1)
            mask |= MASK_BOTTOM_LEFT;
         if (x+1 >= xleft0 && x+1 < xright0)
            mask |= MASK_TOP_RIGHT;
         if (x+1 >= xleft1 && x+1 < xright1)
            mask |= MASK_BOTTOM_RIGHT;
         EMIT_QUAD( setup, x, setup->span.y, mask );
      }
      break;

   case 0x1:
      /* only even line written (quad top row) */
      minleft = block(xleft0);
      maxright = block(xright0);
      for (x = minleft; x <= maxright; x += 2) {
         uint mask = 0x0;
         if (x >= xleft0 && x < xright0)
            mask |= MASK_TOP_LEFT;
         if (x+1 >= xleft0 && x+1 < xright0)
            mask |= MASK_TOP_RIGHT;
         EMIT_QUAD( setup, x, setup->span.y, mask );
      }
      break;

   case 0x2:
      /* only odd line written (quad bottom row) */
      minleft = block(xleft1);
      maxright = block(xright1);
      for (x = minleft; x <= maxright; x += 2) {
         uint mask = 0x0;
         if (x >= xleft1 && x < xright1)
            mask |= MASK_BOTTOM_LEFT;
         if (x+1 >= xleft1 && x+1 < xright1)
            mask |= MASK_BOTTOM_RIGHT;
         EMIT_QUAD( setup, x, setup->span.y, mask );
      }
      break;

   default:
      return;
   }

   setup->span.y = 0;
   setup->span.y_flags = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
}


#if DEBUG_VERTS
static void print_vertex(const struct setup_context *setup,
                         const float (*v)[4])
{
   int i;
   debug_printf("   Vertex: (%p)\n", v);
   for (i = 0; i < setup->quad.nr_attrs; i++) {
      debug_printf("     %d: %f %f %f %f\n",  i,
              v[i][0], v[i][1], v[i][2], v[i][3]);
   }
}
#endif

/**
 * \return FALSE if coords are inf/nan (cull the tri), TRUE otherwise
 */
static boolean setup_sort_vertices( struct setup_context *setup,
                                    float det,
                                    const float (*v0)[4],
                                    const float (*v1)[4],
                                    const float (*v2)[4] )
{
   setup->vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      float y0 = v0[0][1];
      float y1 = v1[0][1];
      float y2 = v2[0][1];
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    /* y0<=y1<=y2 */
	    setup->vmin = v0;
	    setup->vmid = v1;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y0) {
	    /* y2<=y0<=y1 */
	    setup->vmin = v2;
	    setup->vmid = v0;
	    setup->vmax = v1;
	 }
	 else {
	    /* y0<=y2<=y1 */
	    setup->vmin = v0;
	    setup->vmid = v2;
	    setup->vmax = v1;
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup->vmin = v1;
	    setup->vmid = v0;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup->vmin = v2;
	    setup->vmid = v1;
	    setup->vmax = v0;
	 }
	 else {
	    /* y1<=y2<=y0 */
	    setup->vmin = v1;
	    setup->vmid = v2;
	    setup->vmax = v0;
	 }
      }
   }

   setup->ebot.dx = setup->vmid[0][0] - setup->vmin[0][0];
   setup->ebot.dy = setup->vmid[0][1] - setup->vmin[0][1];
   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];
   setup->etop.dx = setup->vmax[0][0] - setup->vmid[0][0];
   setup->etop.dy = setup->vmax[0][1] - setup->vmid[0][1];

   /*
    * Compute triangle's area.  Use 1/area to compute partial
    * derivatives of attributes later.
    *
    * The area will be the same as prim->det, but the sign may be
    * different depending on how the vertices get sorted above.
    *
    * To determine whether the primitive is front or back facing we
    * use the prim->det value because its sign is correct.
    */
   {
      const float area = (setup->emaj.dx * setup->ebot.dy -
			    setup->ebot.dx * setup->emaj.dy);

      setup->oneoverarea = 1.0f / area;

      /*
      debug_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup->oneoverarea, area, det );
      */
      if (is_inf_or_nan(setup->oneoverarea))
         return FALSE;
   }

   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup->quad.input.facing = (det > 0.0) ^ (setup->softpipe->rasterizer->front_winding == PIPE_WINDING_CW);

   return TRUE;
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot
 * \param i  which component of the slot (0..3)
 */
static void const_coeff( struct setup_context *setup,
                         struct tgsi_interp_coef *coef,
                         uint vertSlot, uint i)
{
   assert(i <= 3);

   coef->dadx[i] = 0;
   coef->dady[i] = 0;

   /* need provoking vertex info!
    */
   coef->a0[i] = setup->vprovoke[vertSlot][i];
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void tri_linear_coeff( struct setup_context *setup,
                              struct tgsi_interp_coef *coef,
                              uint vertSlot, uint i)
{
   float botda = setup->vmid[vertSlot][i] - setup->vmin[vertSlot][i];
   float majda = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
   float dady = b * setup->oneoverarea;

   assert(i <= 3);

   coef->dadx[i] = dadx;
   coef->dady[i] = dady;

   /* calculate a0 as the value which would be sampled for the
    * fragment at (0,0), taking into account that we want to sample at
    * pixel centers, in other words (0.5, 0.5).
    *
    * this is neat but unfortunately not a good way to do things for
    * triangles with very large values of dadx or dady as it will
    * result in the subtraction and re-addition from a0 of a very
    * large number, which means we'll end up loosing a lot of the
    * fractional bits and precision from a0.  the way to fix this is
    * to define a0 as the sample at a pixel center somewhere near vmin
    * instead - i'll switch to this later.
    */
   coef->a0[i] = (setup->vmin[vertSlot][i] -
                  (dadx * (setup->vmin[0][0] - 0.5f) +
                   dady * (setup->vmin[0][1] - 0.5f)));

   /*
   debug_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i],
		setup->coef[slot].a0[i],
		setup->coef[slot].dadx[i],
		setup->coef[slot].dady[i]);
   */
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void tri_persp_coeff( struct setup_context *setup,
                             struct tgsi_interp_coef *coef,
                             uint vertSlot, uint i)
{
   /* premultiply by 1/w  (v[0][3] is always W):
    */
   float mina = setup->vmin[vertSlot][i] * setup->vmin[0][3];
   float mida = setup->vmid[vertSlot][i] * setup->vmid[0][3];
   float maxa = setup->vmax[vertSlot][i] * setup->vmax[0][3];
   float botda = mida - mina;
   float majda = maxa - mina;
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
   float dady = b * setup->oneoverarea;

   /*
   debug_printf("tri persp %d,%d: %f %f %f\n", vertSlot, i,
          	setup->vmin[vertSlot][i],
          	setup->vmid[vertSlot][i],
       		setup->vmax[vertSlot][i]
          );
   */
   assert(i <= 3);

   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (mina -
                  (dadx * (setup->vmin[0][0] - 0.5f) +
                   dady * (setup->vmin[0][1] - 0.5f)));
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial, though Y has to be inverted for OpenGL.
 * Z and W are copied from posCoef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coeff(struct setup_context *setup, uint slot)
{
   /*X*/
   setup->coef[slot].a0[0] = 0;
   setup->coef[slot].dadx[0] = 1.0;
   setup->coef[slot].dady[0] = 0.0;
   /*Y*/
   if (setup->softpipe->rasterizer->origin_lower_left) {
      /* y=0=bottom */
      const int winHeight = setup->softpipe->framebuffer.height;
      setup->coef[slot].a0[1] = (float) (winHeight - 1);
      setup->coef[slot].dady[1] = -1.0;
   }
   else {
      /* y=0=top */
      setup->coef[slot].a0[1] = 0.0;
      setup->coef[slot].dady[1] = 1.0;
   }
   setup->coef[slot].dadx[1] = 0.0;
   /*Z*/
   setup->coef[slot].a0[2] = setup->posCoef.a0[2];
   setup->coef[slot].dadx[2] = setup->posCoef.dadx[2];
   setup->coef[slot].dady[2] = setup->posCoef.dady[2];
   /*W*/
   setup->coef[slot].a0[3] = setup->posCoef.a0[3];
   setup->coef[slot].dadx[3] = setup->posCoef.dadx[3];
   setup->coef[slot].dady[3] = setup->posCoef.dady[3];
}



/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients( struct setup_context *setup )
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct sp_fragment_shader *spfs = softpipe->fs;
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;

   /* z and w are done by linear interpolation:
    */
   tri_linear_coeff(setup, &setup->posCoef, 0, 2);
   tri_linear_coeff(setup, &setup->posCoef, 0, 3);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < spfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->src_index[fragSlot];
      uint j;

      switch (vinfo->interp_mode[fragSlot]) {
      case INTERP_CONSTANT:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++)
            tri_linear_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++)
            tri_persp_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (spfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FOG) {
         /* FOG.y = front/back facing  XXX fix this */
         setup->coef[fragSlot].a0[1] = 1.0f - setup->quad.input.facing;
         setup->coef[fragSlot].dadx[1] = 0.0;
         setup->coef[fragSlot].dady[1] = 0.0;
      }
   }
}



static void setup_tri_edges( struct setup_context *setup )
{
   float vmin_x = setup->vmin[0][0] + 0.5f;
   float vmid_x = setup->vmid[0][0] + 0.5f;

   float vmin_y = setup->vmin[0][1] - 0.5f;
   float vmid_y = setup->vmid[0][1] - 0.5f;
   float vmax_y = setup->vmax[0][1] - 0.5f;

   setup->emaj.sy = ceilf(vmin_y);
   setup->emaj.lines = (int) ceilf(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dx / setup->emaj.dy;
   setup->emaj.sx = vmin_x + (setup->emaj.sy - vmin_y) * setup->emaj.dxdy;

   setup->etop.sy = ceilf(vmid_y);
   setup->etop.lines = (int) ceilf(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dx / setup->etop.dy;
   setup->etop.sx = vmid_x + (setup->etop.sy - vmid_y) * setup->etop.dxdy;

   setup->ebot.sy = ceilf(vmin_y);
   setup->ebot.lines = (int) ceilf(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dx / setup->ebot.dy;
   setup->ebot.sx = vmin_x + (setup->ebot.sy - vmin_y) * setup->ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring/cliprect is applied here too.
 */
static void subtriangle( struct setup_context *setup,
			 struct edge *eleft,
			 struct edge *eright,
			 unsigned lines )
{
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;
   int y, start_y, finish_y;
   int sy = (int)eleft->sy;

   assert((int)eleft->sy == (int) eright->sy);

   /* clip top/bottom */
   start_y = sy;
   finish_y = sy + lines;

   if (start_y < miny)
      start_y = miny;

   if (finish_y > maxy)
      finish_y = maxy;

   start_y -= sy;
   finish_y -= sy;

   /*
   debug_printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);
   */

   for (y = start_y; y < finish_y; y++) {

      /* avoid accumulating adds as floats don't have the precision to
       * accurately iterate large triangle edges that way.  luckily we
       * can just multiply these days.
       *
       * this is all drowned out by the attribute interpolation anyway.
       */
      int left = (int)(eleft->sx + y * eleft->dxdy);
      int right = (int)(eright->sx + y * eright->dxdy);

      /* clip left/right */
      if (left < minx)
         left = minx;
      if (right > maxx)
         right = maxx;

      if (left < right) {
         int _y = sy + y;
         if (block(_y) != setup->span.y) {
            flush_spans(setup);
            setup->span.y = block(_y);
         }

         setup->span.left[_y&1] = left;
         setup->span.right[_y&1] = right;
         setup->span.y_flags |= 1<<(_y&1);
      }
   }


   /* save the values so that emaj can be restarted:
    */
   eleft->sx += lines * eleft->dxdy;
   eright->sx += lines * eright->dxdy;
   eleft->sy += lines;
   eright->sy += lines;
}


/**
 * Recalculate prim's determinant.  This is needed as we don't have
 * get this information through the vbuf_render interface & we must
 * calculate it here.
 */
static float
calc_det( const float (*v0)[4],
          const float (*v1)[4],
          const float (*v2)[4] )
{
   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
   const float fy = v1[0][1] - v2[0][1];

   /* det = cross(e,f).z */
   return ex * fy - ey * fx;
}


/**
 * Do setup for triangle rasterization, then render the triangle.
 */
void setup_tri( struct setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4] )
{
   float det;

#if DEBUG_VERTS
   debug_printf("Setup triangle:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
   print_vertex(setup, v2);
#endif

   if (setup->softpipe->no_rast)
      return;
   
   det = calc_det(v0, v1, v2);
   /*
   debug_printf("%s\n", __FUNCTION__ );
   */

#if DEBUG_FRAGS
   setup->numFragsEmitted = 0;
   setup->numFragsWritten = 0;
#endif

   if (cull_tri( setup, det ))
      return;

   if (!setup_sort_vertices( setup, det, v0, v1, v2 ))
      return;
   setup_tri_coefficients( setup );
   setup_tri_edges( setup );

   setup->quad.input.prim = PRIM_TRI;

   setup->span.y = 0;
   setup->span.y_flags = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   /*   setup->span.z_mode = tri_z_mode( setup->ctx ); */

   /*   init_constant_attribs( setup ); */

   if (setup->oneoverarea < 0.0) {
      /* emaj on left:
       */
      subtriangle( setup, &setup->emaj, &setup->ebot, setup->ebot.lines );
      subtriangle( setup, &setup->emaj, &setup->etop, setup->etop.lines );
   }
   else {
      /* emaj on right:
       */
      subtriangle( setup, &setup->ebot, &setup->emaj, setup->ebot.lines );
      subtriangle( setup, &setup->etop, &setup->emaj, setup->etop.lines );
   }

   flush_spans( setup );

   WAIT_FOR_COMPLETION(setup);

#if DEBUG_FRAGS
   printf("Tri: %u frags emitted, %u written\n",
          setup->numFragsEmitted,
          setup->numFragsWritten);
#endif
}



/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a line.
 */
static void
line_linear_coeff(struct setup_context *setup,
                  struct tgsi_interp_coef *coef,
                  uint vertSlot, uint i)
{
   const float da = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (setup->vmin[vertSlot][i] -
                  (dadx * (setup->vmin[0][0] - 0.5f) +
                   dady * (setup->vmin[0][1] - 0.5f)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a line.
 */
static void
line_persp_coeff(struct setup_context *setup,
                  struct tgsi_interp_coef *coef,
                  uint vertSlot, uint i)
{
   /* XXX double-check/verify this arithmetic */
   const float a0 = setup->vmin[vertSlot][i] * setup->vmin[0][3];
   const float a1 = setup->vmax[vertSlot][i] * setup->vmax[0][3];
   const float da = a1 - a0;
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   coef->dadx[i] = dadx;
   coef->dady[i] = dady;
   coef->a0[i] = (setup->vmin[vertSlot][i] -
                  (dadx * (setup->vmin[0][0] - 0.5f) +
                   dady * (setup->vmin[0][1] - 0.5f)));
}


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmax are initialized.
 */
static INLINE boolean
setup_line_coefficients(struct setup_context *setup,
                        const float (*v0)[4],
                        const float (*v1)[4])
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct sp_fragment_shader *spfs = softpipe->fs;
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;
   float area;

   /* use setup->vmin, vmax to point to vertices */
   setup->vprovoke = v1;
   setup->vmin = v0;
   setup->vmax = v1;

   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];

   /* NOTE: this is not really area but something proportional to it */
   area = setup->emaj.dx * setup->emaj.dx + setup->emaj.dy * setup->emaj.dy;
   if (area == 0.0f || is_inf_or_nan(area))
      return FALSE;
   setup->oneoverarea = 1.0f / area;

   /* z and w are done by linear interpolation:
    */
   line_linear_coeff(setup, &setup->posCoef, 0, 2);
   line_linear_coeff(setup, &setup->posCoef, 0, 3);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < spfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->src_index[fragSlot];
      uint j;

      switch (vinfo->interp_mode[fragSlot]) {
      case INTERP_CONSTANT:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++)
            line_linear_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++)
            line_persp_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (spfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FOG) {
         /* FOG.y = front/back facing  XXX fix this */
         setup->coef[fragSlot].a0[1] = 1.0f - setup->quad.input.facing;
         setup->coef[fragSlot].dadx[1] = 0.0;
         setup->coef[fragSlot].dady[1] = 0.0;
      }
   }
   return TRUE;
}


/**
 * Plot a pixel in a line segment.
 */
static INLINE void
plot(struct setup_context *setup, int x, int y)
{
   const int iy = y & 1;
   const int ix = x & 1;
   const int quadX = x - ix;
   const int quadY = y - iy;
   const int mask = (1 << ix) << (2 * iy);

   if (quadX != setup->quad.input.x0 ||
       quadY != setup->quad.input.y0)
   {
      /* flush prev quad, start new quad */

      if (setup->quad.input.x0 != -1)
         CLIP_EMIT_QUAD(setup);

      setup->quad.input.x0 = quadX;
      setup->quad.input.y0 = quadY;
      setup->quad.inout.mask = 0x0;
   }

   setup->quad.inout.mask |= mask;
}


/**
 * Do setup for line rasterization, then render the line.
 * Single-pixel width, no stipple, etc.  We rely on the 'draw' module
 * to handle stippling and wide lines.
 */
void
setup_line(struct setup_context *setup,
           const float (*v0)[4],
           const float (*v1)[4])
{
   int x0 = (int) v0[0][0];
   int x1 = (int) v1[0][0];
   int y0 = (int) v0[0][1];
   int y1 = (int) v1[0][1];
   int dx = x1 - x0;
   int dy = y1 - y0;
   int xstep, ystep;

#if DEBUG_VERTS
   debug_printf("Setup line:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
#endif

   if (setup->softpipe->no_rast)
      return;

   if (dx == 0 && dy == 0)
      return;

   if (!setup_line_coefficients(setup, v0, v1))
      return;

   assert(v0[0][0] < 1.0e9);
   assert(v0[0][1] < 1.0e9);
   assert(v1[0][0] < 1.0e9);
   assert(v1[0][1] < 1.0e9);

   if (dx < 0) {
      dx = -dx;   /* make positive */
      xstep = -1;
   }
   else {
      xstep = 1;
   }

   if (dy < 0) {
      dy = -dy;   /* make positive */
      ystep = -1;
   }
   else {
      ystep = 1;
   }

   assert(dx >= 0);
   assert(dy >= 0);

   setup->quad.input.x0 = setup->quad.input.y0 = -1;
   setup->quad.inout.mask = 0x0;
   setup->quad.input.prim = PRIM_LINE;
   /* XXX temporary: set coverage to 1.0 so the line appears
    * if AA mode happens to be enabled.
    */
   setup->quad.input.coverage[0] =
   setup->quad.input.coverage[1] =
   setup->quad.input.coverage[2] =
   setup->quad.input.coverage[3] = 1.0;

   if (dx > dy) {
      /*** X-major line ***/
      int i;
      const int errorInc = dy + dy;
      int error = errorInc - dx;
      const int errorDec = error - dx;

      for (i = 0; i < dx; i++) {
         plot(setup, x0, y0);

         x0 += xstep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            y0 += ystep;
         }
      }
   }
   else {
      /*** Y-major line ***/
      int i;
      const int errorInc = dx + dx;
      int error = errorInc - dy;
      const int errorDec = error - dy;

      for (i = 0; i < dy; i++) {
         plot(setup, x0, y0);

         y0 += ystep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            x0 += xstep;
         }
      }
   }

   /* draw final quad */
   if (setup->quad.inout.mask) {
      CLIP_EMIT_QUAD(setup);
   }

   WAIT_FOR_COMPLETION(setup);
}


static void
point_persp_coeff(struct setup_context *setup,
                  const float (*vert)[4],
                  struct tgsi_interp_coef *coef,
                  uint vertSlot, uint i)
{
   assert(i <= 3);
   coef->dadx[i] = 0.0F;
   coef->dady[i] = 0.0F;
   coef->a0[i] = vert[vertSlot][i] * vert[0][3];
}


/**
 * Do setup for point rasterization, then render the point.
 * Round or square points...
 * XXX could optimize a lot for 1-pixel points.
 */
void
setup_point( struct setup_context *setup,
             const float (*v0)[4] )
{
   struct softpipe_context *softpipe = setup->softpipe;
   const struct sp_fragment_shader *spfs = softpipe->fs;
   const int sizeAttr = setup->softpipe->psize_slot;
   const float size
      = sizeAttr > 0 ? v0[sizeAttr][0]
      : setup->softpipe->rasterizer->point_size;
   const float halfSize = 0.5F * size;
   const boolean round = (boolean) setup->softpipe->rasterizer->point_smooth;
   const float x = v0[0][0];  /* Note: data[0] is always position */
   const float y = v0[0][1];
   const struct vertex_info *vinfo = softpipe_get_vertex_info(softpipe);
   uint fragSlot;

#if DEBUG_VERTS
   debug_printf("Setup point:\n");
   print_vertex(setup, v0);
#endif

   if (softpipe->no_rast)
      return;

   /* For points, all interpolants are constant-valued.
    * However, for point sprites, we'll need to setup texcoords appropriately.
    * XXX: which coefficients are the texcoords???
    * We may do point sprites as textured quads...
    *
    * KW: We don't know which coefficients are texcoords - ultimately
    * the choice of what interpolation mode to use for each attribute
    * should be determined by the fragment program, using
    * per-attribute declaration statements that include interpolation
    * mode as a parameter.  So either the fragment program will have
    * to be adjusted for pointsprite vs normal point behaviour, or
    * otherwise a special interpolation mode will have to be defined
    * which matches the required behaviour for point sprites.  But -
    * the latter is not a feature of normal hardware, and as such
    * probably should be ruled out on that basis.
    */
   setup->vprovoke = v0;

   /* setup Z, W */
   const_coeff(setup, &setup->posCoef, 0, 2);
   const_coeff(setup, &setup->posCoef, 0, 3);

   for (fragSlot = 0; fragSlot < spfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->src_index[fragSlot];
      uint j;

      switch (vinfo->interp_mode[fragSlot]) {
      case INTERP_CONSTANT:
         /* fall-through */
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++)
            point_persp_coeff(setup, setup->vprovoke,
                              &setup->coef[fragSlot], vertSlot, j);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (spfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FOG) {
         /* FOG.y = front/back facing  XXX fix this */
         setup->coef[fragSlot].a0[1] = 1.0f - setup->quad.input.facing;
         setup->coef[fragSlot].dadx[1] = 0.0;
         setup->coef[fragSlot].dady[1] = 0.0;
      }
   }

   setup->quad.input.prim = PRIM_POINT;

   if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
      const int ix = ((int) x) & 1;
      const int iy = ((int) y) & 1;
      setup->quad.input.x0 = (int) x - ix;
      setup->quad.input.y0 = (int) y - iy;
      setup->quad.inout.mask = (1 << ix) << (2 * iy);
      CLIP_EMIT_QUAD(setup);
   }
   else {
      if (round) {
         /* rounded points */
         const int ixmin = block((int) (x - halfSize));
         const int ixmax = block((int) (x + halfSize));
         const int iymin = block((int) (y - halfSize));
         const int iymax = block((int) (y + halfSize));
         const float rmin = halfSize - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
         const float rmax = halfSize + 0.7071F;
         const float rmin2 = MAX2(0.0F, rmin * rmin);
         const float rmax2 = rmax * rmax;
         const float cscale = 1.0F / (rmax2 - rmin2);
         int ix, iy;

         for (iy = iymin; iy <= iymax; iy += 2) {
            for (ix = ixmin; ix <= ixmax; ix += 2) {
               float dx, dy, dist2, cover;

               setup->quad.inout.mask = 0x0;

               dx = (ix + 0.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.input.coverage[QUAD_TOP_LEFT] = MIN2(cover, 1.0f);
                  setup->quad.inout.mask |= MASK_TOP_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.input.coverage[QUAD_TOP_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad.inout.mask |= MASK_TOP_RIGHT;
               }

               dx = (ix + 0.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.input.coverage[QUAD_BOTTOM_LEFT] = MIN2(cover, 1.0f);
                  setup->quad.inout.mask |= MASK_BOTTOM_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.input.coverage[QUAD_BOTTOM_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad.inout.mask |= MASK_BOTTOM_RIGHT;
               }

               if (setup->quad.inout.mask) {
                  setup->quad.input.x0 = ix;
                  setup->quad.input.y0 = iy;
                  CLIP_EMIT_QUAD(setup);
               }
            }
         }
      }
      else {
         /* square points */
         const int xmin = (int) (x + 0.75 - halfSize);
         const int ymin = (int) (y + 0.25 - halfSize);
         const int xmax = xmin + (int) size;
         const int ymax = ymin + (int) size;
         /* XXX could apply scissor to xmin,ymin,xmax,ymax now */
         const int ixmin = block(xmin);
         const int ixmax = block(xmax - 1);
         const int iymin = block(ymin);
         const int iymax = block(ymax - 1);
         int ix, iy;

         /*
         debug_printf("(%f, %f) -> X:%d..%d Y:%d..%d\n", x, y, xmin, xmax,ymin,ymax);
         */
         for (iy = iymin; iy <= iymax; iy += 2) {
            uint rowMask = 0xf;
            if (iy < ymin) {
               /* above the top edge */
               rowMask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
            }
            if (iy + 1 >= ymax) {
               /* below the bottom edge */
               rowMask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
            }

            for (ix = ixmin; ix <= ixmax; ix += 2) {
               uint mask = rowMask;

               if (ix < xmin) {
                  /* fragment is past left edge of point, turn off left bits */
                  mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
               }
               if (ix + 1 >= xmax) {
                  /* past the right edge */
                  mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
               }

               setup->quad.inout.mask = mask;
               setup->quad.input.x0 = ix;
               setup->quad.input.y0 = iy;
               CLIP_EMIT_QUAD(setup);
            }
         }
      }
   }

   WAIT_FOR_COMPLETION(setup);
}

void setup_prepare( struct setup_context *setup )
{
   struct softpipe_context *sp = setup->softpipe;
   unsigned i;

   if (sp->dirty) {
      softpipe_update_derived(sp);
   }

   /* Mark surfaces as defined now */
   for (i = 0; i < sp->framebuffer.num_cbufs; i++){
      if (sp->framebuffer.cbufs[i]) {
         sp->framebuffer.cbufs[i]->status = PIPE_SURFACE_STATUS_DEFINED;
      }
   }
   if (sp->framebuffer.zsbuf) {
      sp->framebuffer.zsbuf->status = PIPE_SURFACE_STATUS_DEFINED;
   }

   /* Note: nr_attrs is only used for debugging (vertex printing) */
   setup->quad.nr_attrs = draw_num_vs_outputs(sp->draw);

   for (i = 0; i < SP_NUM_QUAD_THREADS; i++) {
      sp->quad[i].first->begin( sp->quad[i].first );
   }

   if (sp->reduced_api_prim == PIPE_PRIM_TRIANGLES &&
       sp->rasterizer->fill_cw == PIPE_POLYGON_MODE_FILL &&
       sp->rasterizer->fill_ccw == PIPE_POLYGON_MODE_FILL) {
      /* we'll do culling */
      setup->winding = sp->rasterizer->cull_mode;
   }
   else {
      /* 'draw' will do culling */
      setup->winding = PIPE_WINDING_NONE;
   }
}



void setup_destroy_context( struct setup_context *setup )
{
   FREE( setup );
}


/**
 * Create a new primitive setup/render stage.
 */
struct setup_context *setup_create_context( struct softpipe_context *softpipe )
{
   struct setup_context *setup = CALLOC_STRUCT(setup_context);
#if SP_NUM_QUAD_THREADS > 1
   uint i;
#endif

   setup->softpipe = softpipe;

   setup->quad.coef = setup->coef;
   setup->quad.posCoef = &setup->posCoef;

#if SP_NUM_QUAD_THREADS > 1
   setup->que.first = 0;
   setup->que.last = 0;
   pipe_mutex_init( setup->que.que_mutex );
   pipe_condvar_init( setup->que.que_notfull_condvar );
   pipe_condvar_init( setup->que.que_notempty_condvar );
   setup->que.jobs_added = 0;
   setup->que.jobs_done = 0;
   pipe_condvar_init( setup->que.que_done_condvar );
   for (i = 0; i < SP_NUM_QUAD_THREADS; i++) {
      setup->threads[i].setup = setup;
      setup->threads[i].id = i;
      setup->threads[i].handle = pipe_thread_create( quad_thread, &setup->threads[i] );
   }
#endif

   return setup;
}

