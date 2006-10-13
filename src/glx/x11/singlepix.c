/* $XFree86: xc/lib/GL/glx/singlepix.c,v 1.3 2001/03/21 16:04:39 dawes Exp $ */
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

#include "packsingle.h"
#include "indirect.h"
#include "dispatch.h"
#include "glapi.h"
#include "glthread.h"
#include "glapioffsets.h"
#include <GL/glxproto.h>

void __indirect_glGetSeparableFilter(GLenum target, GLenum format, GLenum type,
			  GLvoid *row, GLvoid *column, GLvoid *span)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetSeparableFilterReply reply;
    GLubyte *rowBuf, *colBuf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetSeparableFilter, __GLX_PAD(13));
    __GLX_SINGLE_PUT_LONG(0,target);
    __GLX_SINGLE_PUT_LONG(4,format);
    __GLX_SINGLE_PUT_LONG(8,type);
    __GLX_SINGLE_PUT_CHAR(12,state->storePack.swapEndian);
    __GLX_SINGLE_READ_XREPLY();
    compsize = reply.length << 2;

    if (compsize != 0) {
	GLint width, height;
	GLint widthsize, heightsize;

	width = reply.width;
	height = reply.height;

	widthsize = __glImageSize(width,1,1,format, type, 0);
	heightsize = __glImageSize(height,1,1,format, type, 0);

	/* Allocate a holding buffer to transform the data from */
	rowBuf = (GLubyte*) Xmalloc(widthsize);
	if (!rowBuf) {
	    /* Throw data away */
	    _XEatData(dpy, compsize);
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return;
	} else {
	    __GLX_SINGLE_GET_CHAR_ARRAY(((char*)rowBuf),widthsize);
	    __glEmptyImage(gc, 1, width, 1, 1, format, type, rowBuf, row);
	    Xfree((char*) rowBuf);
	}
	colBuf = (GLubyte*) Xmalloc(heightsize);
	if (!colBuf) {
	    /* Throw data away */
	    _XEatData(dpy, compsize - __GLX_PAD(widthsize));
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return;
	} else {
	    __GLX_SINGLE_GET_CHAR_ARRAY(((char*)colBuf),heightsize);
	    __glEmptyImage(gc, 1, height, 1, 1, format, type, colBuf, column);
	    Xfree((char*) colBuf);
	}
    } else {
	/*
	** don't modify user's buffer.
	*/
    }
    __GLX_SINGLE_END();
    
}


#define CONCAT(a,b) a ## b
#define NAME(o) CONCAT(gl_dispatch_stub_, o)

void NAME(_gloffset_GetSeparableFilter)(GLenum target, GLenum format, GLenum type,
					GLvoid *row, GLvoid *column, GLvoid *span)
{
    __GLXcontext * const gc = __glXGetCurrentContext();

    if (gc->isDirect) {
	CALL_GetSeparableFilter(GET_DISPATCH(),
				(target, format, type, row, column, span));
	return;
    }
    else {
        Display *const dpy = gc->currentDpy;
	const GLuint cmdlen = __GLX_PAD(13);

	if (dpy != NULL) {
	    const __GLXattribute * const state = gc->client_state_private;
	    xGLXGetSeparableFilterReply reply;
	    GLubyte const *pc =
	      __glXSetupVendorRequest(gc, X_GLXVendorPrivateWithReply,
				      X_GLvop_GetSeparableFilterEXT, cmdlen);
	    unsigned compsize;


	    (void) memcpy((void *) (pc + 0), (void *) (&target), 4);
	    (void) memcpy((void *) (pc + 4), (void *) (&format), 4);
	    (void) memcpy((void *) (pc + 8), (void *) (&type), 4);
	    *(int8_t *) (pc + 12) = state->storePack.swapEndian;

	    (void) _XReply(dpy, (xReply *) & reply, 0, False);

	    compsize = reply.length << 2;

	    if (compsize != 0) {
		const GLint width = reply.width;
		const GLint height = reply.height;
		const GLint widthsize =
		  __glImageSize(width, 1, 1, format, type, 0);
		const GLint heightsize =
		  __glImageSize(height, 1, 1, format, type, 0);
		GLubyte * const buf =
		  (GLubyte*) Xmalloc((widthsize > heightsize) ? widthsize : heightsize);

		if (buf == NULL) {
		    /* Throw data away */
		    _XEatData(dpy, compsize);
		    __glXSetError(gc, GL_OUT_OF_MEMORY);

		    UnlockDisplay(dpy);
		    SyncHandle();
		    return;
		} else {
		    int extra;
		    
		    extra = 4 - (widthsize & 3);
		    _XRead(dpy, (char *)buf, widthsize);
		    if (extra < 4) {
			_XEatData(dpy, extra);
		    }

		    __glEmptyImage(gc, 1, width, 1, 1, format, type, buf,
				   row);

		    extra = 4 - (heightsize & 3);
		    _XRead(dpy, (char *)buf, heightsize);
		    if (extra < 4) {
			_XEatData(dpy, extra);
		    }

		    __glEmptyImage(gc, 1, height, 1, 1, format, type, buf,
				   column);

		    Xfree((char*) buf);
		}
	    }
	}
    }
}
