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

#include "packrender.h"

/*
** This file contains routines that deal with unpacking data from client
** memory using the pixel store unpack modes and then shipping it to
** the server.  For all of these routines (except glPolygonStipple) there
** are two forms of the transport - small and large.  Small commands are
** the commands that fit into the "rendering" transport buffer.  Large
** commands are sent to the server in chunks by __glXSendLargeCommand.
**
** All of the commands send over a pixel header (see glxproto.h) which
** describes the pixel store modes that the server must use to properly
** handle the data.  Any pixel store modes not done by the __glFillImage
** routine are passed on to the server.
*/

/*
** Send a large image to the server.  If necessary, a buffer is allocated
** to hold the unpacked data that is copied from the clients memory.
*/
static void SendLargeImage(__GLXcontext *gc, GLint compsize, GLint dim,
			   GLint width, GLint height, GLint depth,
			   GLenum format, GLenum type, const GLvoid *src,
			   GLubyte *pc, GLubyte *modes)
{
    if (!gc->fastImageUnpack) {
	/* Allocate a temporary holding buffer */
	GLubyte *buf = (GLubyte *) Xmalloc(compsize);
	if (!buf) {
	    __glXSetError(gc, GL_OUT_OF_MEMORY);
	    return;
	}

	/* Apply pixel store unpack modes to copy data into buf */
	(*gc->fillImage)(gc, dim, width, height, depth, format, type, src, buf,
			 modes);

	/* Send large command */
	__glXSendLargeCommand(gc, gc->pc, pc - gc->pc, buf, compsize);

	/* Free buffer */
	Xfree((char*) buf);
    } else {
	/* Just send the data straight as is */
	__glXSendLargeCommand(gc, gc->pc, pc - gc->pc, pc, compsize);
    }
}

/*
** Send a large null image to the server.  To be backwards compatible,
** data must be sent to the server even when the application has passed
** a null pointer into glTexImage1D, glTexImage2D or glTexImage3D.
*/
static void SendLargeNULLImage(__GLXcontext *gc, GLint compsize,
			       GLint width, GLint height, GLint depth,
			       GLenum format, GLenum type, const GLvoid *src,
			       GLubyte *pc, GLubyte *modes)
{
    GLubyte *buf = (GLubyte *) Xmalloc(compsize);

    /* Allocate a temporary holding buffer */
    if (!buf) {
	__glXSetError(gc, GL_OUT_OF_MEMORY);
	return;
    }

    /* Send large command */
    __glXSendLargeCommand(gc, gc->pc, pc - gc->pc, buf, compsize);

    /* Free buffer */
    Xfree((char*) buf);
}

/************************************************************************/

void glPolygonStipple(const GLubyte *mask)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(32, 32, 1, GL_COLOR_INDEX, GL_BITMAP);
    cmdlen = __GLX_PAD(__GLX_POLYGONSTIPPLE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    __GLX_BEGIN(X_GLrop_PolygonStipple,cmdlen);
    pc += __GLX_RENDER_HDR_SIZE;
    pixelHeaderPC = pc;
    pc += __GLX_PIXEL_HDR_SIZE;
    (*gc->fillImage)(gc, 2, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP,
		     mask, pc, pixelHeaderPC);
    __GLX_END(__GLX_PAD(compsize));
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
	      GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(width, height, 1, GL_COLOR_INDEX, GL_BITMAP);
    cmdlen = __GLX_PAD(__GLX_BITMAP_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_Bitmap,cmdlen);
	__GLX_PUT_LONG(0,width);
	__GLX_PUT_LONG(4,height);
	__GLX_PUT_FLOAT(8,xorig);
	__GLX_PUT_FLOAT(12,yorig);
	__GLX_PUT_FLOAT(16,xmove);
	__GLX_PUT_FLOAT(20,ymove);
	pc += __GLX_BITMAP_HDR_SIZE;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, 2, width, height, 1, GL_COLOR_INDEX,
			     GL_BITMAP, bitmap, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_Bitmap,cmdlen+4);
	__GLX_PUT_LONG(0,width);
	__GLX_PUT_LONG(4,height);
	__GLX_PUT_FLOAT(8,xorig);
	__GLX_PUT_FLOAT(12,yorig);
	__GLX_PUT_FLOAT(16,xmove);
	__GLX_PUT_FLOAT(20,ymove);
	pc += __GLX_BITMAP_HDR_SIZE;
	SendLargeImage(gc, compsize, 2, width, height, 1, GL_COLOR_INDEX,
		       GL_BITMAP, bitmap, pc, pixelHeaderPC);
    }
}

void glTexImage1D(GLenum target, GLint level, GLint components,
		  GLsizei width, GLint border, GLenum format, GLenum type,
		  const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (target == GL_PROXY_TEXTURE_1D) {
	compsize = 0;
    } else {
	compsize = __glImageSize(width, 1, 1, format, type);
    }
    cmdlen = __GLX_PAD(__GLX_TEXIMAGE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_TexImage1D,cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,components);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(20,border);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	pc += __GLX_TEXIMAGE_HDR_SIZE;
	if (compsize > 0 && image != NULL) {
	    (*gc->fillImage)(gc, 1, width, 1, 1, format, type,
			     image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_TexImage1D,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,components);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,1);
	__GLX_PUT_LONG(20,border);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	pc += __GLX_TEXIMAGE_HDR_SIZE;
	if (image != NULL) {
	    SendLargeImage(gc, compsize, 1, width, 1, 1, format,
			   type, image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    {
		GLubyte *pc = pixelHeaderPC;
		__GLX_PUT_CHAR(0,GL_FALSE);
		__GLX_PUT_CHAR(1,GL_FALSE);
		__GLX_PUT_CHAR(2,0);
		__GLX_PUT_CHAR(3,0);
		__GLX_PUT_LONG(4,0);
		__GLX_PUT_LONG(8,0);
		__GLX_PUT_LONG(12,0);
		__GLX_PUT_LONG(16,1);
	    }
	    SendLargeNULLImage(gc, compsize, width, 1, 1, format,
			       type, image, pc, pixelHeaderPC);
	}
    }
}

void glTexImage2D(GLenum target, GLint level, GLint components,
		  GLsizei width, GLsizei height, GLint border, GLenum format,
		  GLenum type, const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (target == GL_PROXY_TEXTURE_2D ||
        target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
	compsize = 0;
    } else {
	compsize = __glImageSize(width, height, 1, format, type);
    }
    cmdlen = __GLX_PAD(__GLX_TEXIMAGE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_TexImage2D,cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,components);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_PUT_LONG(20,border);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	pc += __GLX_TEXIMAGE_HDR_SIZE;
	if (compsize > 0 && image != NULL) {
	    (*gc->fillImage)(gc, 2, width, height, 1, format, type,
			     image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_TexImage2D,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,components);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_PUT_LONG(20,border);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	pc += __GLX_TEXIMAGE_HDR_SIZE;
	if (image != NULL) {
	    SendLargeImage(gc, compsize, 2, width, height, 1, format,
			   type, image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    {
		GLubyte *pc = pixelHeaderPC;
		__GLX_PUT_CHAR(0,GL_FALSE);
		__GLX_PUT_CHAR(1,GL_FALSE);
		__GLX_PUT_CHAR(2,0);
		__GLX_PUT_CHAR(3,0);
		__GLX_PUT_LONG(4,0);
		__GLX_PUT_LONG(8,0);
		__GLX_PUT_LONG(12,0);
		__GLX_PUT_LONG(16,1);
	    }
	    SendLargeNULLImage(gc, compsize, width, height, 1, format,
			       type, image, pc, pixelHeaderPC);
	}
    }
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
		  const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(width, height, 1, format, type);
    cmdlen = __GLX_PAD(__GLX_DRAWPIXELS_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_DrawPixels,cmdlen);
	__GLX_PUT_LONG(0,width);
	__GLX_PUT_LONG(4,height);
	__GLX_PUT_LONG(8,format);
	__GLX_PUT_LONG(12,type);
	pc += __GLX_DRAWPIXELS_HDR_SIZE;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, 2, width, height, 1, format, type,
			     image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_DrawPixels,cmdlen+4);
	__GLX_PUT_LONG(0,width);
	__GLX_PUT_LONG(4,height);
	__GLX_PUT_LONG(8,format);
	__GLX_PUT_LONG(12,type);
	pc += __GLX_DRAWPIXELS_HDR_SIZE;
	SendLargeImage(gc, compsize, 2, width, height, 1, format,
		       type, image, pc, pixelHeaderPC);
    }
}

static void __glx_TexSubImage1D2D(GLshort opcode, GLenum target, GLint level,
			GLint xoffset, GLint yoffset, GLsizei width, 	
			GLsizei height, GLenum format, GLenum type, 	
			const GLvoid *image, GLint dim)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (image == NULL) {
	compsize = 0;
    } else {
        compsize = __glImageSize(width, height, 1, format, type);
    }

    cmdlen = __GLX_PAD(__GLX_TEXSUBIMAGE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(opcode, cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,xoffset);
	__GLX_PUT_LONG(12,yoffset);
	__GLX_PUT_LONG(16,width);
	__GLX_PUT_LONG(20,height);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(32,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(32,GL_FALSE);
	}
	pc += __GLX_TEXSUBIMAGE_HDR_SIZE;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, dim, width, height, 1, format, type, image, 
			     pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(opcode,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,xoffset);
	__GLX_PUT_LONG(12,yoffset);
	__GLX_PUT_LONG(16,width);
	__GLX_PUT_LONG(20,height);
	__GLX_PUT_LONG(24,format);
	__GLX_PUT_LONG(28,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(32,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(32,GL_FALSE);
	}
	pc += __GLX_TEXSUBIMAGE_HDR_SIZE;
	SendLargeImage(gc, compsize, dim, width, height, 1,
		       format, type, image, pc, pixelHeaderPC);
    }
}
	
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, 
			GLsizei width, GLenum format, GLenum type,
		      	const GLvoid *image)
{
    __glx_TexSubImage1D2D(X_GLrop_TexSubImage1D, target, level, xoffset, 
			 0, width, 1, format, type, image, 1);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, 
			GLint yoffset, GLsizei width, GLsizei height, 
			GLenum format, GLenum type, const GLvoid *image)
{
    __glx_TexSubImage1D2D(X_GLrop_TexSubImage2D, target, level, xoffset, 
			 yoffset, width, height, format, type, image, 2);
}

void glColorTable(GLenum target, GLenum internalformat, GLsizei width,
		  GLenum format, GLenum type, const GLvoid *table)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    switch (target) {
	case GL_PROXY_TEXTURE_1D:
	case GL_PROXY_TEXTURE_2D:
	case GL_PROXY_TEXTURE_3D:
	case GL_PROXY_COLOR_TABLE:
	case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
	case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
	case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
	    compsize = 0;
	    break;
	default:
	    compsize = __glImageSize(width, 1, 1, format, type);
	    break;
    }
    cmdlen = __GLX_PAD(__GLX_COLOR_TABLE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) {
	 return;
    }

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
        /* Use GLXRender protocol to send small command */
        __GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_ColorTable, (short)cmdlen);
        __GLX_PUT_LONG(0, (long)target);
        __GLX_PUT_LONG(4, (long)internalformat);
        __GLX_PUT_LONG(8, width);
        __GLX_PUT_LONG(12, (long)format);
        __GLX_PUT_LONG(16, (long)type);
        pc += __GLX_COLOR_TABLE_HDR_SIZE;
        if (compsize > 0 && table != NULL) {
            (*gc->fillImage)(gc, 1, width, 1, 1, format, type, table, pc,
                             pixelHeaderPC);
        } else {
            /* Setup default store modes */
            GLubyte *pc = pixelHeaderPC;
            __GLX_PUT_CHAR(0, GL_FALSE);
            __GLX_PUT_CHAR(1, GL_FALSE);
            __GLX_PUT_CHAR(2, 0);
            __GLX_PUT_CHAR(3, 0);
            __GLX_PUT_LONG(4, 0);
            __GLX_PUT_LONG(8, 0);
            __GLX_PUT_LONG(12, 0);
            __GLX_PUT_LONG(16, 1);
        }
        __GLX_END(__GLX_PAD(compsize));
    } else {
        /* Use GLXRenderLarge protocol to send command */
        __GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_ColorTable, cmdlen+4);
        __GLX_PUT_LONG(0, (long)target);
        __GLX_PUT_LONG(4, (long)internalformat);
        __GLX_PUT_LONG(8, width);
        __GLX_PUT_LONG(12, (long)format);
        __GLX_PUT_LONG(16, (long)type);
        pc += __GLX_COLOR_TABLE_HDR_SIZE;
        SendLargeImage(gc, compsize, 1, width, 1, 1, format,
                       type, table, pc, pixelHeaderPC);
    }
}

void glColorSubTable(GLenum target, GLsizei start, GLsizei count,
		     GLenum format, GLenum type, const GLvoid *table)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(count, 1, 1, format, type);
    cmdlen = __GLX_PAD(__GLX_COLOR_SUBTABLE_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) {
	 return;
    }

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
        /* Use GLXRender protocol to send small command */
        __GLX_BEGIN_VARIABLE_WITH_PIXEL(X_GLrop_ColorSubTable, (short)cmdlen);
        __GLX_PUT_LONG(0, (long)target);
        __GLX_PUT_LONG(4, start);
        __GLX_PUT_LONG(8, count);
        __GLX_PUT_LONG(12, (long)format);
        __GLX_PUT_LONG(16, (long)type);
        pc += __GLX_COLOR_SUBTABLE_HDR_SIZE;
        if (compsize > 0 && table != NULL) {
            (*gc->fillImage)(gc, 1, start+count, 1, 1, format, type, table, pc,
                             pixelHeaderPC);
        } else {
            /* Setup default store modes */
            GLubyte *pc = pixelHeaderPC;
            __GLX_PUT_CHAR(0, GL_FALSE);
            __GLX_PUT_CHAR(1, GL_FALSE);
            __GLX_PUT_CHAR(2, 0);
            __GLX_PUT_CHAR(3, 0);
            __GLX_PUT_LONG(4, 0);
            __GLX_PUT_LONG(8, 0);
            __GLX_PUT_LONG(12, 0);
            __GLX_PUT_LONG(16, 1);
        }
        __GLX_END(__GLX_PAD(compsize));
    } else {
        /* Use GLXRenderLarge protocol to send command */
        __GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(X_GLrop_ColorSubTable, cmdlen+4);
        __GLX_PUT_LONG(0, (long)target);
        __GLX_PUT_LONG(4, start);
        __GLX_PUT_LONG(8, count);
        __GLX_PUT_LONG(12, (long)format);
        __GLX_PUT_LONG(16, (long)type);
        pc += __GLX_COLOR_SUBTABLE_HDR_SIZE;
        SendLargeImage(gc, compsize, 1, start+count, 1, 1, format,
                       type, table, pc, pixelHeaderPC);
    }
}

static void __glx_ConvolutionFilter1D2D(GLshort opcode, GLint dim,
			GLenum target,
			GLenum internalformat,
			GLsizei width, GLsizei height,
			GLenum format, GLenum type, const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(width, height, 1, format, type);
    cmdlen = __GLX_PAD(__GLX_CONV_FILT_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL(opcode, cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,internalformat);
	__GLX_PUT_LONG(8,width);
	__GLX_PUT_LONG(12,height);
	__GLX_PUT_LONG(16,format);
	__GLX_PUT_LONG(20,type);
	pc += __GLX_CONV_FILT_HDR_SIZE;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, dim, width, height, 1, format, type,
			     image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL(opcode,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,internalformat);
	__GLX_PUT_LONG(8,width);
	__GLX_PUT_LONG(12,height);
	__GLX_PUT_LONG(16,format);
	__GLX_PUT_LONG(20,type);
	pc += __GLX_CONV_FILT_HDR_SIZE;
	SendLargeImage(gc, compsize, dim, width, height, 1, format,
		       type, image, pc, pixelHeaderPC);
    }
}

void glConvolutionFilter1D(GLenum target, GLenum internalformat,
				GLsizei width, GLenum format,
				GLenum type, const GLvoid *image)
{
     __glx_ConvolutionFilter1D2D(X_GLrop_ConvolutionFilter1D, 1, target, 
				 internalformat, width, 1, format, type,
				 image);
}

void glConvolutionFilter2D(GLenum target, GLenum internalformat,
				GLsizei width, GLsizei height, GLenum format,
				GLenum type, const GLvoid *image)
{
     __glx_ConvolutionFilter1D2D(X_GLrop_ConvolutionFilter2D, 2, target,
				internalformat, width, height, format, type,
				image);
}

void glSeparableFilter2D(GLenum target, GLenum internalformat,
				GLsizei width, GLsizei height, GLenum format,
				GLenum type, const GLvoid *row,
				const GLvoid *column)
{
    __GLX_DECLARE_VARIABLES();
    GLuint compsize2, hdrlen, totalhdrlen, image1len, image2len;

    __GLX_LOAD_VARIABLES();
    compsize = __glImageSize(width, 1, 1, format, type);
    compsize2 = __glImageSize(height, 1, 1, format, type);
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
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,1);
	}
	__GLX_END(0);
    } else {
	GLint bufsize;

	bufsize = image1len + image2len;

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

void glTexImage3D(GLenum target, GLint level, GLint internalformat,
		  GLsizei width, GLsizei height, GLsizei depth, GLint border,
		  GLenum format, GLenum type, const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if ((target == GL_PROXY_TEXTURE_3D) || (image == NULL)) {
	compsize = 0;
    } else {
	compsize = __glImageSize(width, height, depth, format, type);
    }
    cmdlen = __GLX_PAD(__GLX_TEXIMAGE_3D_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL_3D(X_GLrop_TexImage3D,cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,internalformat);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_PUT_LONG(20,depth);
	__GLX_PUT_LONG(24,0);    /* size4d */
	__GLX_PUT_LONG(28,border);
	__GLX_PUT_LONG(32,format);
	__GLX_PUT_LONG(36,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(40,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(40,GL_FALSE);
	}
	pc += __GLX_TEXIMAGE_3D_HDR_SIZE;
	if (compsize > 0 && image != NULL) {
	    (*gc->fillImage)(gc, 3, width, height, depth, format, type,
			     image, pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,0);
	    __GLX_PUT_LONG(20,0);
	    __GLX_PUT_LONG(24,0);
	    __GLX_PUT_LONG(28,0);
	    __GLX_PUT_LONG(32,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL_3D(X_GLrop_TexImage3D,cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,internalformat);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_PUT_LONG(20,depth);
	__GLX_PUT_LONG(24,0);    /* size4d */
	__GLX_PUT_LONG(28,border);
	__GLX_PUT_LONG(32,format);
	__GLX_PUT_LONG(36,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(40,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(40,GL_FALSE);
	}
	pc += __GLX_TEXIMAGE_3D_HDR_SIZE;
	SendLargeImage(gc, compsize, 3, width, height, depth, format,
		       type, image, pc, pixelHeaderPC);
    }
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
		     GLint zoffset, GLsizei width, GLsizei height,
		     GLsizei depth, GLenum format, GLenum type,
		     const GLvoid *image)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (image == NULL) {
	compsize = 0;
    } else {
        compsize = __glImageSize(width, height, depth, format, type);
    }
    cmdlen = __GLX_PAD(__GLX_TEXSUBIMAGE_3D_CMD_HDR_SIZE + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE_WITH_PIXEL_3D(X_GLrop_TexSubImage3D,cmdlen);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,xoffset);
	__GLX_PUT_LONG(12,yoffset);
	__GLX_PUT_LONG(16,zoffset);
	__GLX_PUT_LONG(20,0); /* woffset */
	__GLX_PUT_LONG(24,width);
	__GLX_PUT_LONG(28,height);
	__GLX_PUT_LONG(32,depth);
	__GLX_PUT_LONG(36,0);      /* size4d */
	__GLX_PUT_LONG(40,format);
	__GLX_PUT_LONG(44,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(48,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(48,GL_FALSE);
	}
	pc += __GLX_TEXSUBIMAGE_3D_HDR_SIZE;
	if (compsize > 0) {
	    (*gc->fillImage)(gc, 3, width, height, depth, format, type, image,
			     pc, pixelHeaderPC);
	} else {
	    /* Setup default store modes */
	    GLubyte *pc = pixelHeaderPC;
	    __GLX_PUT_CHAR(0,GL_FALSE);
	    __GLX_PUT_CHAR(1,GL_FALSE);
	    __GLX_PUT_CHAR(2,0);
	    __GLX_PUT_CHAR(3,0);
	    __GLX_PUT_LONG(4,0);
	    __GLX_PUT_LONG(8,0);
	    __GLX_PUT_LONG(12,0);
	    __GLX_PUT_LONG(16,0);
	    __GLX_PUT_LONG(20,0);
	    __GLX_PUT_LONG(24,0);
	    __GLX_PUT_LONG(28,0);
	    __GLX_PUT_LONG(32,1);
	}
	__GLX_END(__GLX_PAD(compsize));
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE_WITH_PIXEL_3D(X_GLrop_TexSubImage3D,
						 cmdlen+4);
	__GLX_PUT_LONG(0,target);
	__GLX_PUT_LONG(4,level);
	__GLX_PUT_LONG(8,xoffset);
	__GLX_PUT_LONG(12,yoffset);
	__GLX_PUT_LONG(16,zoffset);
	__GLX_PUT_LONG(20,0);    /* woffset */
	__GLX_PUT_LONG(24,width);
	__GLX_PUT_LONG(28,height);
	__GLX_PUT_LONG(32,depth);
	__GLX_PUT_LONG(36,0);    /* size4d */
	__GLX_PUT_LONG(40,format);
	__GLX_PUT_LONG(44,type);
	if (image == NULL) {
	    __GLX_PUT_LONG(48,GL_TRUE);
	} else {
	    __GLX_PUT_LONG(48,GL_FALSE);
	}
	pc += __GLX_TEXSUBIMAGE_3D_HDR_SIZE;
	SendLargeImage(gc, compsize, 3, width, height, depth, format, type,
		       image, pc, pixelHeaderPC);
    }
}
