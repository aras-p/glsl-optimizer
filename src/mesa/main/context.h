
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef CONTEXT_H
#define CONTEXT_H


#include "glapi.h"
#include "mtypes.h"


/*
 * There are three Mesa datatypes which are meant to be used by device
 * drivers:
 *   GLcontext:  this contains the Mesa rendering state
 *   GLvisual:  this describes the color buffer (rgb vs. ci), whether
 *              or not there's a depth buffer, stencil buffer, etc.
 *   GLframebuffer:  contains pointers to the depth buffer, stencil
 *                   buffer, accum buffer and alpha buffers.
 *
 * These types should be encapsulated by corresponding device driver
 * datatypes.  See xmesa.h and xmesaP.h for an example.
 *
 * In OOP terms, GLcontext, GLvisual, and GLframebuffer are base classes
 * which the device driver must derive from.
 *
 * The following functions create and destroy these datatypes.
 */


/*
 * Create/destroy a GLvisual.
 */
extern GLvisual *
_mesa_create_visual( GLboolean rgbFlag,
                     GLboolean dbFlag,
                     GLboolean stereoFlag,
                     GLint redBits,
                     GLint greenBits,
                     GLint blueBits,
                     GLint alphaBits,
                     GLint indexBits,
                     GLint depthBits,
                     GLint stencilBits,
                     GLint accumRedBits,
                     GLint accumGreenBits,
                     GLint accumBlueBits,
                     GLint accumAlphaBits,
                     GLint numSamples );

extern GLboolean
_mesa_initialize_visual( GLvisual *v,
                         GLboolean rgbFlag,
                         GLboolean dbFlag,
                         GLboolean stereoFlag,
                         GLint redBits,
                         GLint greenBits,
                         GLint blueBits,
                         GLint alphaBits,
                         GLint indexBits,
                         GLint depthBits,
                         GLint stencilBits,
                         GLint accumRedBits,
                         GLint accumGreenBits,
                         GLint accumBlueBits,
                         GLint accumAlphaBits,
                         GLint numSamples );

extern void
_mesa_destroy_visual( GLvisual *vis );



/*
 * Create/destroy a GLframebuffer.
 */
extern GLframebuffer *
_mesa_create_framebuffer( const GLvisual *visual,
                          GLboolean softwareDepth,
                          GLboolean softwareStencil,
                          GLboolean softwareAccum,
                          GLboolean softwareAlpha );

extern void
_mesa_initialize_framebuffer( GLframebuffer *fb,
                              const GLvisual *visual,
                              GLboolean softwareDepth,
                              GLboolean softwareStencil,
                              GLboolean softwareAccum,
                              GLboolean softwareAlpha );

extern void
_mesa_free_framebuffer_data( GLframebuffer *buffer );

extern void
_mesa_destroy_framebuffer( GLframebuffer *buffer );



/*
 * Create/destroy a GLcontext.
 */
extern GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      void *driver_ctx,
                      GLboolean direct );

extern GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          const GLvisual *visual,
                          GLcontext *share_list,
                          void *driver_ctx,
                          GLboolean direct );

extern void
_mesa_free_context_data( GLcontext *ctx );

extern void
_mesa_destroy_context( GLcontext *ctx );


extern void
_mesa_copy_context(const GLcontext *src, GLcontext *dst, GLuint mask);


extern void
_mesa_make_current( GLcontext *ctx, GLframebuffer *buffer );


extern void
_mesa_make_current2( GLcontext *ctx, GLframebuffer *drawBuffer,
                     GLframebuffer *readBuffer );


extern GLcontext *
_mesa_get_current_context(void);



/*
 * Macros for fetching current context.
 */
#ifdef THREADS

#define GET_CURRENT_CONTEXT(C)	GLcontext *C = (GLcontext *) (_glapi_Context ? _glapi_Context : _glapi_get_context())

#else

#define GET_CURRENT_CONTEXT(C)  GLcontext *C = (GLcontext *) _glapi_Context

#endif



/* OpenGL SI-style export functions. */

extern GLboolean
_mesa_destroyContext(__GLcontext *gc);

extern GLboolean
_mesa_loseCurrent(__GLcontext *gc);

extern GLboolean
_mesa_makeCurrent(__GLcontext *gc);

extern GLboolean
_mesa_shareContext(__GLcontext *gc, __GLcontext *gcShare);

extern GLboolean
_mesa_copyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask);

extern GLboolean
_mesa_forceCurrent(__GLcontext *gc);

extern GLboolean
_mesa_notifyResize(__GLcontext *gc);

extern void
_mesa_notifyDestroy(__GLcontext *gc);

extern void
_mesa_notifySwapBuffers(__GLcontext *gc);

extern struct __GLdispatchStateRec *
_mesa_dispatchExec(__GLcontext *gc);

extern void
_mesa_beginDispatchOverride(__GLcontext *gc);

extern void
_mesa_endDispatchOverride(__GLcontext *gc);



extern struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx);



/*
 * Miscellaneous
 */

extern void
_mesa_record_error( GLcontext *ctx, GLenum error );


extern void
_mesa_Finish( void );

extern void
_mesa_Flush( void );


#endif
