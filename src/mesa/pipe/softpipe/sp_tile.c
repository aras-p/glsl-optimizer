

#include "sp_context.h"



void
sp_build_quad_pipeline(struct softpipe_context *sp)
{

   sp->quad.first = sp->quad.output;

   if (sp->blend.blend_enable) {
      sp->quad.blend->next = sp->quad.first;
      sp->quad.first = sp->quad.blend;
   }

   /* XXX always enable shader? */
   if (1) {
      sp->quad.shade->next = sp->quad.first;
      sp->quad.first = sp->quad.shade;
   }

}
