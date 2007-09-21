

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

   if (sp->blend->blend_enable) {
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
