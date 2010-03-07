
#include "main/mtypes.h"
#include "main/formats.h"
#include "main/renderbuffer.h"
#include "main/imports.h"
#include "drirenderbuffer.h"


/**
 * This will get called when a window (gl_framebuffer) is resized (probably
 * via driUpdateFramebufferSize(), below).
 * Just update width, height and internal format fields for now.
 * There's usually no memory allocation above because the present
 * DRI drivers use statically-allocated full-screen buffers. If that's not
 * the case for a DRI driver, a different AllocStorage method should
 * be used.
 */
static GLboolean
driRenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLenum internalFormat, GLuint width, GLuint height)
{
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;
   return GL_TRUE;
}


static void
driDeleteRenderbuffer(struct gl_renderbuffer *rb)
{
   /* don't free rb->Data  Chances are it's a memory mapped region for
    * the dri drivers.
    */
   free(rb);
}


/**
 * Allocate a new driRenderbuffer object.
 * Individual drivers are free to implement different versions of
 * this function.
 *
 * At this time, this function can only be used for window-system
 * renderbuffers, not user-created RBOs.
 *
 * \param format  Either GL_RGBA, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24,
 *                GL_DEPTH_COMPONENT32, or GL_STENCIL_INDEX8_EXT (for now).
 * \param addr  address in main memory of the buffer.  Probably a memory
 *              mapped region.
 * \param cpp  chars or bytes per pixel
 * \param offset  start of renderbuffer with respect to start of framebuffer
 * \param pitch   pixels per row
 */
driRenderbuffer *
driNewRenderbuffer(gl_format format, GLvoid *addr,
                   GLint cpp, GLint offset, GLint pitch,
                   __DRIdrawable *dPriv)
{
   driRenderbuffer *drb;

   assert(cpp > 0);
   assert(pitch > 0);

   drb = calloc(1, sizeof(driRenderbuffer));
   if (drb) {
      const GLuint name = 0;

      _mesa_init_renderbuffer(&drb->Base, name);

      /* Make sure we're using a null-valued GetPointer routine */
      assert(drb->Base.GetPointer(NULL, &drb->Base, 0, 0) == NULL);

      switch (format) {
      case MESA_FORMAT_ARGB8888:
         if (cpp == 2) {
            /* override format */
            format = MESA_FORMAT_RGB565;
         }
         drb->Base.DataType = GL_UNSIGNED_BYTE;
         break;
      case MESA_FORMAT_Z16:
         /* Depth */
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
         assert(cpp == 2);
         break;
      case MESA_FORMAT_Z32:
         /* Depth */
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
         assert(cpp == 4);
         break;
      case MESA_FORMAT_Z24_S8:
         drb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
         assert(cpp == 4);
         break;
      case MESA_FORMAT_S8_Z24:
         drb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
         assert(cpp == 4);
         break;
      case MESA_FORMAT_S8:
         /* Stencil */
         drb->Base.DataType = GL_UNSIGNED_BYTE;
         break;
      default:
         _mesa_problem(NULL, "Bad format 0x%x in driNewRenderbuffer", format);
         return NULL;
      }

      drb->Base.Format = format;

      drb->Base.InternalFormat =
      drb->Base._BaseFormat = _mesa_get_format_base_format(format);

      drb->Base.AllocStorage = driRenderbufferStorage;
      drb->Base.Delete = driDeleteRenderbuffer;

      drb->Base.Data = addr;

      /* DRI renderbuffer-specific fields: */
      drb->dPriv = dPriv;
      drb->offset = offset;
      drb->pitch = pitch;
      drb->cpp = cpp;

      /* may be changed if page flipping is active: */
      drb->flippedOffset = offset;
      drb->flippedPitch = pitch;
      drb->flippedData = addr;
   }
   return drb;
}


/**
 * Update the front and back renderbuffers' flippedPitch/Offset/Data fields.
 * If stereo, flip both the left and right pairs.
 * This is used when we do double buffering via page flipping.
 * \param fb  the framebuffer we're page flipping
 * \param flipped  if true, set flipped values, else set non-flipped values
 */
void
driFlipRenderbuffers(struct gl_framebuffer *fb, GLboolean flipped)
{
   const GLuint count = fb->Visual.stereoMode ? 2 : 1;
   GLuint lr; /* left or right */

   /* we shouldn't really call this function if single-buffered, but
    * play it safe.
    */
   if (!fb->Visual.doubleBufferMode)
      return;

   for (lr = 0; lr < count; lr++) {
      GLuint frontBuf = (lr == 0) ? BUFFER_FRONT_LEFT : BUFFER_FRONT_RIGHT;
      GLuint backBuf  = (lr == 0) ? BUFFER_BACK_LEFT  : BUFFER_BACK_RIGHT;
      driRenderbuffer *front_drb
         = (driRenderbuffer *) fb->Attachment[frontBuf].Renderbuffer;
      driRenderbuffer *back_drb
         = (driRenderbuffer *) fb->Attachment[backBuf].Renderbuffer;

      if (flipped) {
         front_drb->flippedOffset = back_drb->offset;
         front_drb->flippedPitch  = back_drb->pitch;
         front_drb->flippedData   = back_drb->Base.Data;
         back_drb->flippedOffset  = front_drb->offset;
         back_drb->flippedPitch   = front_drb->pitch;
         back_drb->flippedData    = front_drb->Base.Data;
      }
      else {
         front_drb->flippedOffset = front_drb->offset;
         front_drb->flippedPitch  = front_drb->pitch;
         front_drb->flippedData   = front_drb->Base.Data;
         back_drb->flippedOffset  = back_drb->offset;
         back_drb->flippedPitch   = back_drb->pitch;
         back_drb->flippedData    = back_drb->Base.Data;
      }
   }
}


/**
 * Check that the gl_framebuffer associated with dPriv is the right size.
 * Resize the gl_framebuffer if needed.
 * It's expected that the dPriv->driverPrivate member points to a
 * gl_framebuffer object.
 */
void
driUpdateFramebufferSize(GLcontext *ctx, const __DRIdrawable *dPriv)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) dPriv->driverPrivate;
   if (fb && (dPriv->w != fb->Width || dPriv->h != fb->Height)) {
      ctx->Driver.ResizeBuffers(ctx, fb, dPriv->w, dPriv->h);
      /* if the driver needs the hw lock for ResizeBuffers, the drawable
         might have changed again by now */
      assert(fb->Width == dPriv->w);
      assert(fb->Height == dPriv->h);
   }
}
