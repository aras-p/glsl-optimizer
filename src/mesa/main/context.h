/* $Id: context.h,v 1.27 2001/05/03 14:11:18 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 * Create/destroy a GLvisual.  A GLvisual is like a GLX visual.  It describes
 * the colorbuffer, depth buffer, stencil buffer and accum buffer which will
 * be used by the GL context and framebuffer.
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
 * Create/destroy a GLframebuffer.  A GLframebuffer is like a GLX drawable.
 * It bundles up the depth buffer, stencil buffer and accum buffers into a
 * single entity.
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
 * Create/destroy a GLcontext.  A GLcontext is like a GLX context.  It
 * contains the rendering state.
 */
extern GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      void *driver_ctx,
                      GLboolean direct);

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



extern void
_mesa_swapbuffers(GLcontext *ctx);


extern struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx);



/*
 * Miscellaneous
 */

extern void
_mesa_problem( const GLcontext *ctx, const char *s );

extern void
_mesa_warning( const GLcontext *ctx, const char *s );

extern void
_mesa_error( GLcontext *ctx, GLenum error, const char *s );

extern void
_mesa_compile_error( GLcontext *ctx, GLenum error, const char *s );



extern void
_mesa_Finish( void );

extern void
_mesa_Flush( void );



extern void
_mesa_read_config_file(GLcontext *ctx);

extern void
_mesa_register_config_var(const char *name,
                          void (*notify)( const char *, int ));


#endif
