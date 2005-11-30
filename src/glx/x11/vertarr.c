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

#define NONE  {0, 0, 0}
#define F(x)  {GL_FLOAT, x, x * sizeof(GLfloat)}
#define UB4   {GL_UNSIGNED_BYTE, 4, 4 * sizeof(GLubyte)}

    /* Each row in this array describes the elements of a particular
     * interleaved array mode.  Each column describes, in the order in which
     * they appear in the interleaved arrays, one of the four possible types
     * of vertex data that can appear in an interleaved array.
     */
    struct {
        /**
	 * The enum describing the GL type, as would be passed to the
	 * appropriate gl*Pointer function.
	 */
	GLushort type;

        /**
	 * Number of elements in the subarray, as would be passed (as the
	 * \c size parameter) to the appropriate gl*Pointer function.
	 */
        GLubyte count;
       
        /**
	 * True size of a single element in the subarray, as would be passed
	 * (as the \c stride parameter) to the appropriate gl*Pointer
	 * function.
	 */
        GLubyte size;
    }
    static const modes[14][4] = {
      /* texture color normal vertex */
	{NONE,   NONE, NONE,  F(2)}, /* GL_V2F */
	{NONE,   NONE, NONE,  F(3)}, /* GL_V3F */
	{NONE,   UB4,  NONE,  F(2)}, /* GL_C4UB_V2F */
	{NONE,   UB4,  NONE,  F(3)}, /* GL_C4UB_V3F */
	{NONE,   F(3), NONE,  F(3)}, /* GL_C3F_V3F */
	{NONE,   NONE, F(3),  F(3)}, /* GL_N3F_V3F */
	{NONE,   F(4), F(3),  F(3)}, /* GL_C4F_N3F_V3F */
	{F(2),   NONE, NONE,  F(3)}, /* GL_T2F_V3F */
	{F(4),   NONE, NONE,  F(4)}, /* GL_T4F_V4F */
	{F(2),   UB4,  NONE,  F(3)}, /* GL_T2F_C4UB_V3F */
	{F(2),   F(3), NONE,  F(3)}, /* GL_T2F_C3F_V3F */
	{F(2),   NONE, F(3),  F(3)}, /* GL_T2F_N3F_V3F */
	{F(2),   F(4), F(3),  F(3)}, /* GL_T2F_C4F_N3F_V3F */
	{F(4),   F(4), F(3),  F(4)}, /* GL_T4F_C4F_N3F_V4F */
    };
#undef NONE
#undef F
#undef UB4

    GLint trueStride, size;
    int offsets[4];
    unsigned i;
    const int idx = format - GL_V2F;

   
    /* All valid formats are on the range [GL_V2F, GL_V2F+0x0D].  Since idx
     * is just the format biased by -GL_V2F, all valid idx values are on the
     * range [0, 0x0D].
     */
    if ( (idx < 0) || (idx > 0x0D) ) {
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    if ( stride < 0 ) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }


    /* If the 'count' for a subarray is non-zero, then the offset of its
     * first element is at the currently accumulated 'size'.
     */
    size = 0;
    for ( i = 0 ; i < 4 ; i++ ) {
        offsets[i] = (modes[idx][i].count != 0) ? size : -1;
        size += modes[idx][i].size;
    }

    trueStride = (stride == 0) ? size : stride;

    __glXArrayDisableAll( state );

    if ( offsets[0] >= 0 ) {
	__indirect_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	__indirect_glTexCoordPointer( modes[idx][0].count, GL_FLOAT,
				      trueStride, 
				      (const char *) pointer );
    }
    if ( offsets[1] >= 0 ) {
	__indirect_glEnableClientState(GL_COLOR_ARRAY);
	__indirect_glColorPointer( modes[idx][1].count, modes[idx][1].type,
				   trueStride, 
				   (const char *) pointer + offsets[1] );
    }
    if ( offsets[2] >= 0 ) {
	__indirect_glEnableClientState(GL_NORMAL_ARRAY);
	__indirect_glNormalPointer( GL_FLOAT, trueStride,
				    (const char *)pointer + offsets[2] );
    }
    __indirect_glEnableClientState(GL_VERTEX_ARRAY);
    __indirect_glVertexPointer( modes[idx][3].count, GL_FLOAT,
				trueStride, 
				(const char *)pointer + offsets[3] );
}
