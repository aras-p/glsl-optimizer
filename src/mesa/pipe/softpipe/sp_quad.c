

#include "sp_context.h"



void
sp_build_quad_pipeline(struct softpipe_context *sp)
{
   /* build up the pipeline in reverse order... */

   sp->quad.first = sp->quad.output;

   if (sp->blend.blend_enable) {
      sp->quad.blend->next = sp->quad.first;
      sp->quad.first = sp->quad.blend;
   }

   if (sp->depth_test.enabled) {
      sp->quad.depth_test->next = sp->quad.first;
      sp->quad.first = sp->quad.depth_test;
   }

   /* XXX always enable shader? */
   if (1) {
      sp->quad.shade->next = sp->quad.first;
      sp->quad.first = sp->quad.shade;
   }

}
