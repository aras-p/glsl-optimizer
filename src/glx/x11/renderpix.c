/* $XFree86: xc/lib/GL/glx/renderpix.c,v 1.5 2003/09/28 20:15:04 alanh Exp $ */
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "packrender.h"
#include "indirect.h"

/**
 * Send a large image to the server.  If necessary, a buffer is allocated
 * to hold the unpacked data that is copied from the clients memory.
 * 
 * \param gc        Current GLX context
 * \param compsize  Size, in bytes, of the image portion
 * \param dim       Number of dimensions of the image
 * \param width     Width of the image
 * \param height    Height of the image, must be 1 for 1D images
 * \param depth     Depth of the image, must be 1 for 1D or 2D images
 * \param format    Format of the image
 * \param type      Data type of the image
 * \param src       Pointer to the image data
 * \param pc        Pointer to end of the command header
 * \param modes     Pointer to the pixel unpack data
 *
 * \todo
 * Modify this function so that \c NULL images are sent using
 * \c __glXSendLargeChunk instead of __glXSendLargeCommand.  Doing this
 * will eliminate the need to allocate a buffer for that case.
 *
 * \bugs
 * The \c fastImageUnpack path, which is thankfully never used, is completely
 * broken.
 */
void
__glXSendLargeImage(__GLXcontext *gc, GLint compsize, GLint dim,
		    GLint width, GLint height, GLint depth,
		    GLenum format, GLenum type, const GLvoid *src,
		    GLubyte *pc, GLubyte *modes)
{
    if ( !gc->fastImageUnpack || (src == NULL) ) {
	/* Allocate a temporary holding buffer */
	GLubyte *buf = (GLubyte *) Xmalloc(compsize);
	if (!buf) {
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	    return;
	}

	/* Apply pixel store unpack modes to copy data into buf */
	if ( src != NULL ) {
	    (*gc->fillImage)(gc, dim, width, height, depth, format, type,
			     src, buf, modes);
	}
	else {
	    if ( dim < 3 ) {
		(void) memcpy( modes, __glXDefaultPixelStore + 4, 20 );
	    }
	    else {
		(void) memcpy( modes, __glXDefaultPixelStore + 0, 36 );
	    }
	}

	/* Send large command */
	__glXSendLargeCommand(gc, gc->pc, pc - gc->pc, buf, compsize);

	/* Free buffer */
	Xfree((char*) buf);
    } else {
	/* Just send the data straight as is */
	__glXSendLargeCommand(gc, gc->pc, pc - gc->pc, pc, compsize);
    }
}

/************************************************************************/

/**
 * Implement GLX protocol for \c glSeparableFilter2D.
 *
 * \bugs
 * The \c fastImageUnpack path, which is thankfully never used, is completely
 * broken.
 */
void __indirect_glSeparableFilter2D(GLenum target, GLenum internalformat,
				GLsizei width, GLsizei height, GLenum format,
				GLenum type, const GLvoid *row,
				const GLvoid *column)
{
    __GLX_DECLARE_VARIABLES();
    GLuint compsize2, hdrlen, totalhdrlen, image1len, image2len;

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(width, 1, 1, format, type, 0);
    compsize2 = __glImageSize(height, 1, 1, format, type, 0);
    totalhdrlen = __GLX_PAD(__GLX_CONV_FILT_CMD_HDR_SIZE);
    hdrlen = __GLX_PAD(__GLX_CONV_FILT_HDR_SIZE);
    image1len = __GLX_PAD(compsize);
    image2len = __GLX_PAD(compsize2);
    cmdlen = totalhdrlen + image1len + image2len;
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_SeparableFilter2D, cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,internalformat);
	__GLX_PUT_LONG(8,width);
	__GLX_PUT_LONG(12,height);
	__GLX_PUT_LONG(16,format);
	__GLX_PUT_LONG(20,type);
	pc += hdrlen;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, 1, width, 1, 1, format, type,
			     row, pc, pixelHeaderPC);
	    pc += image1len;
	}
	if (compsize2 > 0) {
            (*gc->fillImage)(gc, 1, height, 1, 1, format, type,
			     column, pc, NULL);
	    pc += image2len;
	}
	if ((compsize == 0) && (compsize2 == 0)) {
	    /* Setup default store modes */
	    (void) memcpy( pixelHeaderPC, __glXDefaultPixelStore + 4, 20 );
	}
	__GLX_END(0);
    } else {
	const GLint bufsize = image1len + image2len;

	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_SeparableFilter2D,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,internalformat);
	__GLX_PUT_LONG(8,width);
	__GLX_PUT_LONG(12,height);
	__GLX_PUT_LONG(16,format);
	__GLX_PUT_LONG(20,type);
	pc += hdrlen;

	if (!gc->fastImageUnpack) {
	    /* Allocate a temporary holding buffer */
	    GLubyte *buf = (GLubyte *) Xmalloc(bufsize);
	    if (!buf) {
		__glXSetError(gc, GL_OUT_OF_MEMORY);
		return;
	    }
	    (*gc->fillImage)(gc, 1, width, 1, 1, format, type, row, buf, pixelHeaderPC);

	    (*gc->fillImage)(gc, 1, height, 1, 1, format, type, column,
				 buf + image1len, pixelHeaderPC);

	    /* Send large command */
	    __glXSendLargeCommand(gc, gc->pc, (GLint)(pc - gc->pc), buf, bufsize);
	    /* Free buffer */
	    Xfree((char*) buf);
	} else {
	    /* Just send the data straight as is */
	    __glXSendLargeCommand(gc, gc->pc, (GLint)(pc - gc->pc), pc, bufsize);
	}
    }
}
