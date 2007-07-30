#include "glheader.h"
#include "context.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "utils.h"
#include "main/macros.h"


#include "intel_screen.h"

#include "intel_context.h"
#include "intel_buffers.h"
#include "intel_regions.h"
#include "intel_span.h"
#include "intel_fbo.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/softpipe/sp_surface.h"


/*
 * XXX a lof of this is a temporary kludge
 */



static void
read_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                GLfloat (*rrrr)[QUAD_SIZE])
{
   struct intel_surface *is = (struct intel_surface *) sps;
   struct intel_renderbuffer *irb = is->rb;
   const GLubyte *src = (const GLubyte *) irb->region->map
      + (y * irb->region->pitch + x) * irb->region->cpp;
   GLfloat *dst = (GLfloat *) rrrr;
   GLubyte temp[16];
   GLuint i, j;

   memcpy(temp, src, 8);
   memcpy(temp + 8, src + irb->region->pitch * irb->region->cpp, 8);

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         dst[j * 4 + i] = UBYTE_TO_FLOAT(temp[i * 4 + j]);
      }
   }
}


static void
write_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                 GLfloat (*rrrr)[QUAD_SIZE])
{
   struct intel_surface *is = (struct intel_surface *) sps;
   struct intel_renderbuffer *irb = is->rb;
   const GLfloat *src = (const GLfloat *) rrrr;
   GLubyte *dst = (GLubyte *) irb->region->map
      + (y * irb->region->pitch + x) * irb->region->cpp;
   GLubyte temp[16];
   GLuint i, j;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + i], src[i * 4 + j]);
      }
   }

   memcpy(dst, temp, 8);
   memcpy(dst + irb->region->pitch * irb->region->cpp, temp + 8, 8);
}



static void *
map_surface_buffer(struct pipe_buffer *pb, GLuint access_mode)
{
   struct intel_surface *is = (struct intel_surface *) pb;
   struct intel_renderbuffer *irb = is->rb;
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);

   assert(access_mode == PIPE_MAP_READ_WRITE);

   intelFinish(&intel->ctx);

   /*LOCK_HARDWARE(intel);*/

   if (irb->region) {
      intel_region_map(intel->intelScreen, irb->region);
   }
   pb->ptr = irb->region->map;

   return pb->ptr;
}


static void
unmap_surface_buffer(struct pipe_buffer *pb)
{
   struct intel_surface *is = (struct intel_surface *) pb;
   struct intel_renderbuffer *irb = is->rb;
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);

   if (irb->region) {
      intel_region_unmap(intel->intelScreen, irb->region);
   }
   pb->ptr = NULL;

   /*UNLOCK_HARDWARE(intel);*/
}


struct pipe_surface *
xmesa_get_color_surface(GLcontext *ctx, GLuint i)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_framebuffer *intel_fb;
   struct intel_renderbuffer *intel_rb;

   intel_fb = (struct intel_framebuffer *) ctx->DrawBuffer;
   intel_rb = intel_fb->color_rb[1];

   if (!intel_rb->surface) {
      /* create surface and attach to intel_rb */
      struct intel_surface *is;
      is = CALLOC_STRUCT(intel_surface);
      if (is) {
         is->surface.surface.width = intel_rb->Base.Width;
         is->surface.surface.height = intel_rb->Base.Height;

         is->surface.read_quad_f_swz = read_quad_f_swz;
         is->surface.write_quad_f_swz = write_quad_f_swz;

         is->surface.surface.buffer.map = map_surface_buffer;
         is->surface.surface.buffer.unmap = unmap_surface_buffer;

         is->rb = intel_rb;
      }
      intel_rb->surface = is;
   }
   else {
      /* update surface size */
      struct intel_surface *is = intel_rb->surface;
      is->surface.surface.width = intel_rb->Base.Width;
      is->surface.surface.height = intel_rb->Base.Height;
      /* sanity check */
      assert(is->surface.surface.buffer.map == map_surface_buffer);
   }

   return &intel_rb->surface->surface.surface;
}


struct pipe_surface *
xmesa_get_z_surface(GLcontext *ctx)
{
   /* XXX fix */
   return NULL;
}


struct pipe_surface *
xmesa_get_stencil_surface(GLcontext *ctx)
{
   /* XXX fix */
   return NULL;
}



