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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef DRAW_VS_H
#define DRAW_VS_H

#include "draw_context.h"
#include "draw_private.h"


struct draw_context;
struct pipe_shader_state;

/**
 * Private version of the compiled vertex_shader
 */
struct draw_vertex_shader {

   /* This member will disappear shortly:
    */
   struct pipe_shader_state   state;

   struct tgsi_shader_info info;

   void (*prepare)( struct draw_vertex_shader *shader,
		    struct draw_context *draw );

   /* Run the shader - this interface will get cleaned up in the
    * future:
    */
   boolean (*run)( struct draw_vertex_shader *shader,
                   struct draw_context *draw,
                   const unsigned *elts,
                   unsigned count,
                   void *out,
                   unsigned vertex_size);

   void (*run_linear)( struct draw_vertex_shader *shader,
		       const float (*input)[4],
		       float (*output)[4],
		       const float (*constants)[4],
		       unsigned count,
		       unsigned input_stride,
		       unsigned output_stride );


   void (*delete)( struct draw_vertex_shader * );
};


struct draw_vertex_shader *
draw_create_vs_exec(struct draw_context *draw,
		    const struct pipe_shader_state *templ);

struct draw_vertex_shader *
draw_create_vs_sse(struct draw_context *draw,
		   const struct pipe_shader_state *templ);

struct draw_vertex_shader *
draw_create_vs_llvm(struct draw_context *draw,
		    const struct pipe_shader_state *templ);


/* Should be part of the generated shader:
 */
static INLINE unsigned
compute_clipmask(const float *clip, /*const*/ float plane[][4], unsigned nr)
{
   unsigned mask = 0x0;
   unsigned i;

   /* Do the hardwired planes first:
    */
   if (-clip[0] + clip[3] < 0) mask |= CLIP_RIGHT_BIT;
   if ( clip[0] + clip[3] < 0) mask |= CLIP_LEFT_BIT;
   if (-clip[1] + clip[3] < 0) mask |= CLIP_TOP_BIT;
   if ( clip[1] + clip[3] < 0) mask |= CLIP_BOTTOM_BIT;
   if (-clip[2] + clip[3] < 0) mask |= CLIP_FAR_BIT;
   if ( clip[2] + clip[3] < 0) mask |= CLIP_NEAR_BIT;

   /* Followed by any remaining ones:
    */
   for (i = 6; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0) 
         mask |= (1<<i);
   }

   return mask;
}

#define MAX_TGSI_VERTICES 4
   

#endif
