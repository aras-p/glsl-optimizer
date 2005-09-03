
#include "mtypes.h"
#include "drirenderbuffer.h"
#include "renderbuffer.h"
#include "imports.h"


/**
 * This will get called when a window is resized.
 * Just update width, height and internal format fields for now.
 * There's usually no memory allocation above because the present
 * DRI drivers use statically-allocated full-screen buffers.
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
   _mesa_free(rb);
}


/**
 * Allocate a new driRenderbuffer object.
 * Individual drivers are free to implement different versions of
 * this function.
 * \param format  Either GL_RGBA, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24,
 *                GL_DEPTH_COMPONENT32, or GL_STENCIL_INDEX8_EXT (for now).
 * \param cpp  chars or bytes per pixel
 * \param offset  start of buffer with respect to framebuffer address
 * \param pitch   pixels per row
 */
driRenderbuffer *
driNewRenderbuffer(GLenum format, GLint cpp, GLint offset, GLint pitch)
{
   driRenderbuffer *drb;

   assert(format == GL_RGBA ||
          format == GL_DEPTH_COMPONENT16 ||
          format == GL_DEPTH_COMPONENT24 ||
          format == GL_DEPTH_COMPONENT32 ||
          format == GL_STENCIL_INDEX8_EXT);

   assert(cpp > 0);
   assert(pitch > 0);

   drb = _mesa_calloc(sizeof(driRenderbuffer));
   if (drb) {
      const GLuint name = 0;

      _mesa_init_renderbuffer(&drb->Base, name);

      /* Make sure we're using a null-valued GetPointer routine */
      assert(drb->Base.GetPointer(NULL, &drb->Base, 0, 0) == NULL);

      drb->Base.InternalFormat = format;

      if (format == GL_RGBA) {
         /* Color */
         drb->Base._BaseFormat = GL_RGBA;
         drb->Base.DataType = GL_UNSIGNED_BYTE;
      }
      else if (format == GL_DEPTH_COMPONENT16) {
         /* Depth */
         drb->Base._BaseFormat = GL_DEPTH_COMPONENT;
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
      }
      else if (format == GL_DEPTH_COMPONENT24) {
         /* Depth */
         drb->Base._BaseFormat = GL_DEPTH_COMPONENT;
         /* we always Get/Put 32-bit Z values */
         drb->Base.DataType = GL_UNSIGNED_INT;
      }
      else {
         /* Stencil */
         ASSERT(format == GL_STENCIL_INDEX8);
         drb->Base._BaseFormat = GL_STENCIL_INDEX;
         drb->Base.DataType = GL_UNSIGNED_BYTE;
      }

      /* XXX if we were allocating a user-created renderbuffer, we'd have
       * to fill in the ComponentSizes[] array too.
       */

      drb->Base.AllocStorage = driRenderbufferStorage;
      drb->Base.Delete = driDeleteRenderbuffer;

      /* DRI renderbuffer-specific fields: */
      drb->offset = offset;
      drb->pitch = pitch;
      drb->cpp = cpp;

      /* may be changed if page flipping is active: */
      drb->flippedOffset = offset;
      drb->flippedPitch = pitch;
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

   ASSERT(fb->Visual.doubleBufferMode);

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
