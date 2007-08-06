#include "glheader.h"
#include "context.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "utils.h"
#include "main/macros.h"


#include "intel_screen.h"

#include "intel_context.h"
#include "intel_buffers.h"
#include "intel_fbo.h"

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/softpipe/sp_surface.h"


/*
 * XXX a lof of this is a temporary kludge
 */

/**
 * Note: the arithmetic/addressing in these functions is a little
 * tricky since we need to invert the Y axis.
 */


static void
read_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                GLfloat (*rrrr)[QUAD_SIZE])
{
   const GLint bytesPerRow = sps->surface.region->pitch * sps->surface.region->cpp;
   const GLint invY = sps->surface.height - y - 1;
   const GLubyte *src = sps->surface.region->map + invY * bytesPerRow + x * sps->surface.region->cpp;
   GLfloat *dst = (GLfloat *) rrrr;
   GLubyte temp[16];
   GLuint j;

   assert(sps->surface.format == PIPE_FORMAT_U_A8_R8_G8_B8);

   memcpy(temp + 8, src, 8);
   memcpy(temp + 0, src + bytesPerRow, 8);

   for (j = 0; j < 4; j++) {
      dst[0 * 4 + j] = UBYTE_TO_FLOAT(temp[j * 4 + 2]); /*R*/
      dst[1 * 4 + j] = UBYTE_TO_FLOAT(temp[j * 4 + 1]); /*G*/
      dst[2 * 4 + j] = UBYTE_TO_FLOAT(temp[j * 4 + 0]); /*B*/
      dst[3 * 4 + j] = UBYTE_TO_FLOAT(temp[j * 4 + 3]); /*A*/
   }
}


static void
write_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                 GLfloat (*rrrr)[QUAD_SIZE])
{
   const GLfloat *src = (const GLfloat *) rrrr;
   const GLint bytesPerRow = sps->surface.region->pitch * sps->surface.region->cpp;
   const GLint invY = sps->surface.height - y - 1;
   GLubyte *dst = sps->surface.region->map + invY * bytesPerRow + x * sps->surface.region->cpp;
   GLubyte temp[16];
   GLuint j;

   assert(sps->surface.format == PIPE_FORMAT_U_A8_R8_G8_B8);

   for (j = 0; j < 4; j++) {
      UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + 2], src[0 * 4 + j]); /*R*/
      UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + 1], src[1 * 4 + j]); /*G*/
      UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + 0], src[2 * 4 + j]); /*B*/
      UNCLAMPED_FLOAT_TO_UBYTE(temp[j * 4 + 3], src[3 * 4 + j]); /*A*/
   }

   memcpy(dst, temp + 8, 8);
   memcpy(dst + bytesPerRow, temp + 0, 8);
}



static void
read_quad_z24(struct softpipe_surface *sps,
              GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   static const GLuint mask = 0xffffff;
   const GLint invY = sps->surface.height - y - 1;
   const GLuint *src
      = (GLuint *) (sps->surface.region->map
                    + (invY * sps->surface.region->pitch + x) * sps->surface.region->cpp);

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   /* extract lower three bytes */
   zzzz[0] = src[0] & mask;
   zzzz[1] = src[1] & mask;
   zzzz[2] = src[-sps->surface.region->pitch] & mask;
   zzzz[3] = src[-sps->surface.region->pitch + 1] & mask;
}

static void
write_quad_z24(struct softpipe_surface *sps,
               GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   static const GLuint mask = 0xff000000;
   const GLint invY = sps->surface.height - y - 1;
   GLuint *dst
      = (GLuint *) (sps->surface.region->map
                    + (invY * sps->surface.region->pitch + x) * sps->surface.region->cpp);

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   /* write lower three bytes */
   dst[0] = (dst[0] & mask) | zzzz[0];
   dst[1] = (dst[1] & mask) | zzzz[1];
   dst -= sps->surface.region->pitch;
   dst[0] = (dst[0] & mask) | zzzz[2];
   dst[1] = (dst[1] & mask) | zzzz[3];
}


static void
read_quad_stencil(struct softpipe_surface *sps,
                  GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLint invY = sps->surface.height - y - 1;
   const GLuint *src = (const GLuint *) (sps->surface.region->map
                     + (invY * sps->surface.region->pitch + x) * sps->surface.region->cpp);

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   /* extract high byte */
   ssss[0] = src[0] >> 24;
   ssss[1] = src[1] >> 24;
   src -= sps->surface.region->pitch;
   ssss[2] = src[0] >> 24;
   ssss[3] = src[1] >> 24;
}

static void
write_quad_stencil(struct softpipe_surface *sps,
                   GLint x, GLint y, const GLubyte ssss[QUAD_SIZE])
{
   static const GLuint mask = 0x00ffffff;
   const GLint invY = sps->surface.height - y - 1;
   GLuint *dst = (GLuint *) (sps->surface.region->map
               + (invY * sps->surface.region->pitch + x) * sps->surface.region->cpp);

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   /* write high byte */
   dst[0] = (dst[0] & mask) | (ssss[0] << 24);
   dst[1] = (dst[1] & mask) | (ssss[1] << 24);
   dst -= sps->surface.region->pitch;
   dst[0] = (dst[0] & mask) | (ssss[2] << 24);
   dst[1] = (dst[1] & mask) | (ssss[3] << 24);
}


struct pipe_surface *
intel_new_surface(GLuint intFormat)
{
   struct softpipe_surface *sps = CALLOC_STRUCT(softpipe_surface);
   if (!sps)
      return NULL;

   sps->surface.width = 0; /* set in intel_alloc_renderbuffer_storage() */
   sps->surface.height = 0;

   if (intFormat == GL_RGBA8) {
      sps->surface.format = PIPE_FORMAT_U_A8_R8_G8_B8;
      sps->read_quad_f_swz = read_quad_f_swz;
      sps->write_quad_f_swz = write_quad_f_swz;
   }
   else if (intFormat == GL_RGB5) {
      sps->surface.format = PIPE_FORMAT_U_R5_G6_B5;

   }
   else if (intFormat == GL_DEPTH_COMPONENT16) {
      sps->surface.format = PIPE_FORMAT_U_Z16;

   }
   else if (intFormat == GL_DEPTH24_STENCIL8_EXT) {
      sps->surface.format = PIPE_FORMAT_S8_Z24;
      sps->read_quad_z = read_quad_z24;
      sps->write_quad_z = write_quad_z24;
      sps->read_quad_stencil = read_quad_stencil;
      sps->write_quad_stencil = write_quad_stencil;
   }
   else {
      /* TBD / unknown */

   }

   return &sps->surface;
}
