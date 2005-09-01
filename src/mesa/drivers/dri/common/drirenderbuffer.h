
/**
 * A driRenderbuffer is dervied from gl_renderbuffer.
 * It describes a color buffer (front or back), a depth buffer, or stencil
 * buffer etc.
 * Specific to DRI drivers are the offset and pitch fields.
 */


#ifndef DRIRENDERBUFFER_H
#define DRIRENDERBUFFER_H

#include "mtypes.h"

typedef struct {
   struct gl_renderbuffer Base;

   /* Chars or bytes per pixel.  If Z and Stencil are stored together this
    * will typically be 32 whether this a depth or stencil renderbuffer.
    */
   GLint cpp;

   /* Buffer position and pitch (row stride).  Recall that for today's DRI
    * drivers, we have statically allocated color/depth/stencil buffers.
    * So this information describes the whole screen, not just a window.
    * To address pixels in a window, we need to know the window's position
    * and size with respect to the screen.
    */
   GLint offset;  /* in bytes */
   GLint pitch;   /* in pixels */

   /* If the driver can do page flipping (full-screen double buffering)
    * the current front/back buffers may get swapped.
    * If page flipping is disabled, these  fields will be identical to
    * the offset/pitch above.
    * If page flipping is enabled, and this is the front(back) renderbuffer,
    * flippedOffset/Pitch will have the back(front) renderbuffer's values.
    */
   GLint flippedOffset;
   GLint flippedPitch;

   /* XXX this is for radeon/r200 only.  We should really create a new
    * r200Renderbuffer class, derived from this class...  not a huge deal.
    */
   GLboolean depthHasSurface;
} driRenderbuffer;


extern driRenderbuffer *
driNewRenderbuffer(GLenum format, GLint cpp, GLint offset, GLint pitch);

extern void
driFlipRenderbuffers(struct gl_framebuffer *fb, GLenum flipped);

#endif /* DRIRENDERBUFFER_H */
