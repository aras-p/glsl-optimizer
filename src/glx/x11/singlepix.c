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

void __indirect_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXReadPixelsReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_ReadPixels,__GLX_PAD(26));
    __GLX_SINGLE_PUT_LONG(0,x);
    __GLX_SINGLE_PUT_LONG(4,y);
    __GLX_SINGLE_PUT_LONG(8,width);
    __GLX_SINGLE_PUT_LONG(12,height);
    __GLX_SINGLE_PUT_LONG(16,format);
    __GLX_SINGLE_PUT_LONG(20,type);
    __GLX_SINGLE_PUT_CHAR(24,state->storePack.swapEndian);
    __GLX_SINGLE_PUT_CHAR(25,GL_FALSE);
    __GLX_SINGLE_READ_XREPLY();
    compsize = reply.length << 2;

    if (compsize != 0) {
	/* Allocate a holding buffer to transform the data from */
	buf = (GLubyte*) Xmalloc(compsize);
	if (!buf) {
	    /* Throw data away */
	    _XEatData(dpy, compsize);
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	} else {
	    /*
	    ** Fetch data into holding buffer.  Apply pixel store pack modes
	    ** to put data back into client memory
	    */
	    __GLX_SINGLE_GET_CHAR_ARRAY(buf,compsize);
	    __glEmptyImage(gc, 2, width, height, 1, format, type, buf, pixels);
	    Xfree((char*) buf);
	}
    } else {
	/*
	** GL error occurred; don't modify user's buffer.
	*/
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
		   GLvoid *texels)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetTexImageReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetTexImage,__GLX_PAD(17));
    __GLX_SINGLE_PUT_LONG(0,target);
    __GLX_SINGLE_PUT_LONG(4,level);
    __GLX_SINGLE_PUT_LONG(8,format);
    __GLX_SINGLE_PUT_LONG(12,type);
    __GLX_SINGLE_PUT_CHAR(16,state->storePack.swapEndian);
    __GLX_SINGLE_READ_XREPLY();
    compsize = reply.length << 2;

    if (compsize != 0) {
	/* Allocate a holding buffer to transform the data from */
	buf = (GLubyte*) Xmalloc(compsize);
	if (!buf) {
	    /* Throw data away */
	    _XEatData(dpy, compsize);
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	} else {
	    GLint width, height, depth;

	    /*
	    ** Fetch data into holding buffer.  Apply pixel store pack modes
	    ** to put data back into client memory
	    */
	    width = reply.width;
	    height = reply.height;
	    depth = reply.depth;
	    __GLX_SINGLE_GET_CHAR_ARRAY(buf,compsize);
	    __glEmptyImage(gc, 2, width, height, depth, format, type, buf,
			   texels);
	    Xfree((char*) buf);
	}
    } else {
	/*
	** GL error occured, don't modify user's buffer.
	*/
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetPolygonStipple(GLubyte *mask)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    xGLXSingleReply reply;
    GLubyte buf[128];

    if (!dpy) return;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetPolygonStipple,__GLX_PAD(1));
    __GLX_SINGLE_PUT_CHAR(0,GL_FALSE);
    __GLX_SINGLE_READ_XREPLY();
    if (reply.length == 32) {
	__GLX_SINGLE_GET_CHAR_ARRAY(buf,128);
	__glEmptyImage(gc, 2, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP, buf, mask);
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetColorTableReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetColorTable,__GLX_PAD(13));
    __GLX_SINGLE_PUT_LONG(0,(long)target);
    __GLX_SINGLE_PUT_LONG(4,(long)format);
    __GLX_SINGLE_PUT_LONG(8,(long)type);
    __GLX_SINGLE_PUT_CHAR(12,state->storePack.swapEndian);
    __GLX_SINGLE_READ_XREPLY();
    compsize = (long)reply.length << 2;

    if (compsize != 0) {
        /* Allocate a holding buffer to transform the data from */
        buf = (GLubyte*)Xmalloc(compsize);
        if (!buf) {
            /* Throw data away */
            _XEatData(dpy, compsize);
            __glXSetError(gc, GL_OUT_OF_MEMORY);
        } else {
            GLint width;

            /*
            ** Fetch data into holding buffer.  Apply pixel store pack modes
            ** to put data back into client memory
            */
            width = (int)reply.width;
            __GLX_SINGLE_GET_CHAR_ARRAY(((char*)buf),(long)compsize);
            __glEmptyImage(gc, 1, width, 1, 1, format, type, buf, table);
            Xfree((char*) buf);
        }
    } else {
        /*
        ** GL error occured, don't modify user's buffer.
        */
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetConvolutionFilter(GLenum target, GLenum format, GLenum type,
			    GLvoid *image)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetConvolutionFilterReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetConvolutionFilter, __GLX_PAD(13));
    __GLX_SINGLE_PUT_LONG(0,target);
    __GLX_SINGLE_PUT_LONG(4,format);
    __GLX_SINGLE_PUT_LONG(8,type);
    __GLX_SINGLE_PUT_CHAR(12,state->storePack.swapEndian);
    __GLX_SINGLE_READ_XREPLY();
    compsize = reply.length << 2;

    if (compsize != 0) {
	/* Allocate a holding buffer to transform the data from */
	buf = (GLubyte*) Xmalloc(compsize);
	if (!buf) {
	    /* Throw data away */
	    _XEatData(dpy, compsize);
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	} else {
	    GLint width, height;

	    /*
	    ** Fetch data into holding buffer.  Apply pixel store pack modes
	    ** to put data back into client memory
	    */
	    width = reply.width;
	    height = reply.height;
	    __GLX_SINGLE_GET_CHAR_ARRAY(((char*)buf),compsize);
	    __glEmptyImage(gc, 2, width, height, 1, format, type, buf, image);
	    Xfree((char*) buf);
	}
    } else {
	/*
	** GL error occured, don't modify user's buffer.
	*/
    }
    __GLX_SINGLE_END();
}

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

void __indirect_glGetHistogram(GLenum target, GLboolean reset, GLenum format,
		    GLenum type, GLvoid *values)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetHistogramReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetHistogram,__GLX_PAD(14));
    __GLX_SINGLE_PUT_LONG(0,(long)target);
    __GLX_SINGLE_PUT_LONG(4,(long)format);
    __GLX_SINGLE_PUT_LONG(8,(long)type);
    __GLX_SINGLE_PUT_CHAR(12,state->storePack.swapEndian);
    __GLX_SINGLE_PUT_CHAR(13,reset);
    __GLX_SINGLE_READ_XREPLY();
    compsize = (long)reply.length << 2;

    if (compsize != 0) {
        /* Allocate a holding buffer to transform the data from */
        buf = (GLubyte*)Xmalloc(compsize);
        if (!buf) {
            /* Throw data away */
            _XEatData(dpy, compsize);
            __glXSetError(gc, GL_OUT_OF_MEMORY);
        } else {
            GLint width;

            /*
            ** Fetch data into holding buffer.  Apply pixel store pack modes
            ** to put data back into client memory
            */
            width = (int)reply.width;
            __GLX_SINGLE_GET_CHAR_ARRAY(((char*)buf),(long)compsize);
            __glEmptyImage(gc, 1, width, 1, 1, format, type, buf, values);
            Xfree((char*) buf);
        }
    } else {
        /*
        ** GL error occured, don't modify user's buffer.
        */
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type,
		 GLvoid *values)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    const __GLXattribute * state;
    xGLXGetMinmaxReply reply;
    GLubyte *buf;

    if (!dpy) return;
    __GLX_SINGLE_LOAD_VARIABLES();
    state = gc->client_state_private;

    /* Send request */
    __GLX_SINGLE_BEGIN(X_GLsop_GetMinmax,__GLX_PAD(14));
    __GLX_SINGLE_PUT_LONG(0,(long)target);
    __GLX_SINGLE_PUT_LONG(4,(long)format);
    __GLX_SINGLE_PUT_LONG(8,(long)type);
    __GLX_SINGLE_PUT_CHAR(12,state->storePack.swapEndian);
    __GLX_SINGLE_PUT_CHAR(13,reset);
    __GLX_SINGLE_READ_XREPLY();
    compsize = (long)reply.length << 2;

    if (compsize != 0) {
        /* Allocate a holding buffer to transform the data from */
        buf = (GLubyte*)Xmalloc(compsize);
        if (!buf) {
            /* Throw data away */
            _XEatData(dpy, compsize);
            __glXSetError(gc, GL_OUT_OF_MEMORY);
        } else {
            /*
            ** Fetch data into holding buffer.  Apply pixel store pack modes
            ** to put data back into client memory
            */
            __GLX_SINGLE_GET_CHAR_ARRAY(((char*)buf),(long)compsize);
            __glEmptyImage(gc, 1, 2, 1, 1, format, type, buf, values);
            Xfree((char*) buf);
        }
    } else {
        /*
        ** GL error occured, don't modify user's buffer.
        */
    }
    __GLX_SINGLE_END();
}
