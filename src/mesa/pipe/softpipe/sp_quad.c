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


#include "sp_context.h"



void
sp_build_quad_pipeline(struct softpipe_context *sp)
{
   /* build up the pipeline in reverse order... */

   sp->quad.first = sp->quad.output;

   if (sp->blend->colormask != 0xf) {
      sp->quad.colormask->next = sp->quad.first;
      sp->quad.first = sp->quad.colormask;
   }

   if (sp->blend->blend_enable ||
       sp->blend->logicop_enable) {
      sp->quad.blend->next = sp->quad.first;
      sp->quad.first = sp->quad.blend;
   }

   if (sp->framebuffer.num_cbufs == 1) {
      /* the usual case: write to exactly one colorbuf */
      sp->cbuf = sp->framebuffer.cbufs[0];
   }
   else {
      /* insert bufloop stage */
      sp->quad.bufloop->next = sp->quad.first;
      sp->quad.first = sp->quad.bufloop;
   }

   if (sp->depth_stencil->depth.occlusion_count) {
      sp->quad.occlusion->next = sp->quad.first;
      sp->quad.first = sp->quad.occlusion;
   }

   if (sp->rasterizer->poly_smooth ||
       sp->rasterizer->line_smooth ||
       sp->rasterizer->point_smooth) {
      sp->quad.coverage->next = sp->quad.first;
      sp->quad.first = sp->quad.coverage;
   }

   if (   sp->depth_stencil->stencil.front_enabled
       || sp->depth_stencil->stencil.back_enabled) {
      sp->quad.stencil_test->next = sp->quad.first;
      sp->quad.first = sp->quad.stencil_test;
   }
   else if (sp->depth_stencil->depth.enabled &&
            sp->framebuffer.zbuf) {
      sp->quad.depth_test->next = sp->quad.first;
      sp->quad.first = sp->quad.depth_test;
   }

   if (sp->alpha_test->enabled) {
      sp->quad.alpha_test->next = sp->quad.first;
      sp->quad.first = sp->quad.alpha_test;
   }

   /* XXX always enable shader? */
   if (1) {
      sp->quad.shade->next = sp->quad.first;
      sp->quad.first = sp->quad.shade;
   }

   if (sp->rasterizer->poly_stipple_enable) {
      sp->quad.polygon_stipple->next = sp->quad.first;
      sp->quad.first = sp->quad.polygon_stipple;
   }
}
