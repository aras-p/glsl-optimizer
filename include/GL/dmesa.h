/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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

/*
 * DOS/DJGPP device driver v1.0 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef DMESA_included
#define DMESA_included

#define DMESA_MAJOR_VERSION 4
#define DMESA_MINOR_VERSION 0

typedef struct dmesa_context *DMesaContext;
typedef struct dmesa_visual *DMesaVisual;
typedef struct dmesa_buffer *DMesaBuffer;

#ifdef __cplusplus
extern "C" {
#endif

DMesaVisual DMesaCreateVisual (GLint width, GLint height, GLint colDepth,
                               GLboolean dbFlag, GLint depthSize,
                               GLint stencilSize,
                               GLint accumSize);

void DMesaDestroyVisual (DMesaVisual v);

DMesaBuffer DMesaCreateBuffer (DMesaVisual visual,
                               GLint xpos, GLint ypos,
                               GLint width, GLint height);

void DMesaDestroyBuffer (DMesaBuffer b);

DMesaContext DMesaCreateContext (DMesaVisual visual, DMesaContext share);

void DMesaDestroyContext (DMesaContext c);

GLboolean DMesaViewport (DMesaBuffer b,
                         GLint xpos, GLint ypos,
                         GLint width, GLint height);

GLboolean DMesaMakeCurrent (DMesaContext c, DMesaBuffer b);

void DMesaSwapBuffers (DMesaBuffer b);

#ifdef __cplusplus
}
#endif

#endif
