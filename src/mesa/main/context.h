/* $Id: context.h,v 1.18 2000/05/24 15:04:45 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "types.h"


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

/* this function is obsolete */
extern GLvisual *
gl_create_visual( GLboolean rgbFlag,
                  GLboolean alphaFlag,
                  GLboolean dbFlag,
                  GLboolean stereoFlag,
                  GLint depthBits,
                  GLint stencilBits,
                  GLint accumBits,
                  GLint indexBits,
                  GLint redBits,
                  GLint greenBits,
                  GLint blueBits,
                  GLint alphaBits );


extern void
_mesa_destroy_visual( GLvisual *vis );

/*obsolete */ extern void gl_destroy_visual( GLvisual *vis );



/*
 * Create/destroy a GLframebuffer.  A GLframebuffer is like a GLX drawable.
 * It bundles up the depth buffer, stencil buffer and accum buffers into a
 * single entity.
 */
extern GLframebuffer *
gl_create_framebuffer( GLvisual *visual,
                       GLboolean softwareDepth,
                       GLboolean softwareStencil,
                       GLboolean softwareAccum,
                       GLboolean softwareAlpha );

extern void
_mesa_initialize_framebuffer( GLframebuffer *fb,
                              GLvisual *visual,
                              GLboolean softwareDepth,
                              GLboolean softwareStencil,
                              GLboolean softwareAccum,
                              GLboolean softwareAlpha );

extern void
gl_destroy_framebuffer( GLframebuffer *buffer );



/*
 * Create/destroy a GLcontext.  A GLcontext is like a GLX context.  It
 * contains the rendering state.
 */
extern GLcontext *
gl_create_context( GLvisual *visual,
                   GLcontext *share_list,
                   void *driver_ctx,
                   GLboolean direct);

extern GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          GLvisual *visual,
                          GLcontext *share_list,
                          void *driver_ctx,
                          GLboolean direct );

extern void
gl_free_context_data( GLcontext *ctx );

extern void
gl_destroy_context( GLcontext *ctx );


extern void
gl_context_initialize( GLcontext *ctx );


extern void
gl_copy_context(const GLcontext *src, GLcontext *dst, GLuint mask);


extern void
gl_make_current( GLcontext *ctx, GLframebuffer *buffer );


extern void
gl_make_current2( GLcontext *ctx, GLframebuffer *drawBuffer,
                  GLframebuffer *readBuffer );


extern GLcontext *
gl_get_current_context(void);



/*
 * Macros for fetching current context, input buffer, etc.
 */
#ifdef THREADS

#define GET_CURRENT_CONTEXT(C)	GLcontext *C = (GLcontext *) (_glapi_Context ? _glapi_Context : _glapi_get_context())

#define GET_IMMEDIATE  struct immediate *IM = ((GLcontext *) (_glapi_Context ? _glapi_Context : _glapi_get_context()))->input

#define SET_IMMEDIATE(ctx, im)		\
do {					\
   ctx->input = im;			\
} while (0)

#else

extern struct immediate *_mesa_CurrentInput;

#define GET_CURRENT_CONTEXT(C)  GLcontext *C = (GLcontext *) _glapi_Context

#define GET_IMMEDIATE struct immediate *IM = _mesa_CurrentInput

#define SET_IMMEDIATE(ctx, im)		\
do {					\
   ctx->input = im;			\
   _mesa_CurrentInput = im;		\
} while (0)

#endif



extern void
_mesa_swapbuffers(GLcontext *ctx);


extern struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx);



/*
 * Miscellaneous
 */

extern void
gl_problem( const GLcontext *ctx, const char *s );

extern void
gl_warning( const GLcontext *ctx, const char *s );

extern void
gl_error( GLcontext *ctx, GLenum error, const char *s );

extern void
gl_compile_error( GLcontext *ctx, GLenum error, const char *s );



extern void
_mesa_Finish( void );

extern void
_mesa_Flush( void );



#endif
