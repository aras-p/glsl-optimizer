/* $XFree86: xc/lib/GL/glx/vertarr.c,v 1.4 2001/03/25 05:32:00 tsi Exp $ */
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

#include "glxclient.h"
#include "indirect.h"
#include "indirect_vertex_array.h"


/*****************************************************************************/

/**
 * \name Vertex array pointer bridge functions
 *
 * When EXT_vertex_array was moved into the core GL spec, the \c count
 * parameter was lost.  This libGL really only wants to implement the GL 1.1
 * version, but we need to support applications that were written to the old
 * interface.  These bridge functions are part of the glue that makes this
 * happen.
 */
/*@{*/
void __indirect_glColorPointerEXT(GLint size, GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glColorPointer( size, type, stride, pointer );
}

void __indirect_glEdgeFlagPointerEXT(GLsizei stride,
			       GLsizei count, const GLboolean * pointer )
{
    (void) count; __indirect_glEdgeFlagPointer( stride, pointer );
}

void __indirect_glIndexPointerEXT(GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glIndexPointer( type, stride, pointer );
}

void __indirect_glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
			     const GLvoid * pointer )
{
    (void) count; __indirect_glNormalPointer( type, stride, pointer );
}

void __indirect_glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
			       GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glTexCoordPointer( size, type, stride, pointer );
}

void __indirect_glVertexPointerEXT(GLint size, GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glVertexPointer( size, type, stride, pointer );
}
/*@}*/

/*****************************************************************************/

void __indirect_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    GLboolean tEnable = GL_FALSE, cEnable = GL_FALSE, nEnable = GL_FALSE;
    GLenum tType = GL_FLOAT, nType = GL_FLOAT, vType = GL_FLOAT;
    GLenum cType = GL_FALSE;
    GLint tSize = 0, cSize = 0, nSize = 3, vSize;
    int cOffset = 0, nOffset = 0, vOffset = 0;
    GLint trueStride, size;

    switch (format) {
      case GL_V2F:
	vSize = 2;
	size = __glXTypeSize(vType) * vSize;
	break;
      case GL_V3F:
	vSize = 3;
	size = __glXTypeSize(vType) * vSize;
	break;
      case GL_C4UB_V2F:
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_UNSIGNED_BYTE;
	vSize = 2;
	vOffset = __glXTypeSize(cType) * cSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_C4UB_V3F:
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_UNSIGNED_BYTE;
	vSize = 3;
	vOffset = __glXTypeSize(vType) * cSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_C3F_V3F:
	cEnable = GL_TRUE;
	cSize = 3;
	cType = GL_FLOAT;
	vSize = 3;
	vOffset = __glXTypeSize(cType) * cSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_N3F_V3F:
	nEnable = GL_TRUE;
	vSize = 3;
	vOffset = __glXTypeSize(nType) * nSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_C4F_N3F_V3F:
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_FLOAT;
	nEnable = GL_TRUE;
	nOffset = __glXTypeSize(cType) * cSize;
	vSize = 3;
	vOffset = nOffset + __glXTypeSize(nType) * nSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T2F_V3F:
	tEnable = GL_TRUE;
	tSize = 2;
	vSize = 3;
	vOffset = __glXTypeSize(tType) * tSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T4F_V4F:
	tEnable = GL_TRUE;
	tSize = 4;
	vSize = 4;
	vOffset = __glXTypeSize(tType) * tSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T2F_C4UB_V3F:
	tEnable = GL_TRUE;
	tSize = 2;
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_UNSIGNED_BYTE;
	cOffset = __glXTypeSize(tType) * tSize;
	vSize = 3;
	vOffset = cOffset + __glXTypeSize(cType) * cSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T2F_C3F_V3F:
	tEnable = GL_TRUE;
	tSize = 2;
	cEnable = GL_TRUE;
	cSize = 3;
	cType = GL_FLOAT;
	cOffset = __glXTypeSize(tType) * tSize;
	vSize = 3;
	vOffset = cOffset + __glXTypeSize(cType) * cSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T2F_N3F_V3F:
	tEnable = GL_TRUE;
	tSize = 2;
	nEnable = GL_TRUE;
	nOffset = __glXTypeSize(tType) * tSize;
	vSize = 3;
	vOffset = nOffset + __glXTypeSize(nType) * nSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T2F_C4F_N3F_V3F:
	tEnable = GL_TRUE;
	tSize = 2;
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_FLOAT;
	cOffset = __glXTypeSize(tType) * tSize;
	nEnable = GL_TRUE;
	nOffset = cOffset + __glXTypeSize(cType) * cSize;
	vSize = 3;
	vOffset = nOffset + __glXTypeSize(nType) * nSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      case GL_T4F_C4F_N3F_V4F:
	tEnable = GL_TRUE;
	tSize = 4;
	cEnable = GL_TRUE;
	cSize = 4;
	cType = GL_FLOAT;
	cOffset = __glXTypeSize(tType) * tSize;
	nEnable = GL_TRUE;
	nOffset = cOffset + __glXTypeSize(cType) * cSize;
	vSize = 4;
	vOffset = nOffset + __glXTypeSize(nType) * nSize;
	size = vOffset + __glXTypeSize(vType) * vSize;
	break;
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    trueStride = (stride == 0) ? size : stride;

    __glXArrayDisableAll( state );

    if (tEnable) {
	__indirect_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	__indirect_glTexCoordPointer(tSize, tType, trueStride, (const char *)pointer);
    }
    if (cEnable) {
	__indirect_glEnableClientState(GL_COLOR_ARRAY);
	__indirect_glColorPointer(cSize, cType, trueStride, (const char *)pointer+cOffset);
    }
    if (nEnable) {
	__indirect_glEnableClientState(GL_NORMAL_ARRAY);
	__indirect_glNormalPointer(nType, trueStride, (const char *)pointer+nOffset);
    }
    __indirect_glEnableClientState(GL_VERTEX_ARRAY);
    __indirect_glVertexPointer(vSize, vType, trueStride, (const char *)pointer+vOffset);
}
