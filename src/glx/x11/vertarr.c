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

#define NEED_GL_FUNCS_WRAPPED
#include <assert.h>
#include "glxclient.h"
#include "packrender.h"
#include <string.h>
#include <limits.h>		/* INT_MAX */

/* macros for setting function pointers */
#define __GL_VERTEX_FUNC(NAME, let) \
    case GL_##NAME: \
      if (size == 2) \
	vertexPointer->proc = (void (*)(const void *))glVertex2##let##v; \
      else if (size == 3) \
	vertexPointer->proc = (void (*)(const void *))glVertex3##let##v; \
      else if (size == 4) \
	vertexPointer->proc = (void (*)(const void *))glVertex4##let##v; \
      break

#define __GL_NORMAL_FUNC(NAME, let) \
    case GL_##NAME: \
      normalPointer->proc = (void (*)(const void *))glNormal3##let##v; \
      break

#define __GL_COLOR_FUNC(NAME, let) \
    case GL_##NAME: \
      if (size == 3) \
	colorPointer->proc = (void (*)(const void *))glColor3##let##v; \
      else if (size == 4)\
	colorPointer->proc = (void (*)(const void *))glColor4##let##v; \
      break

#define __GL_SEC_COLOR_FUNC(NAME, let) \
    case GL_##NAME: \
      seccolorPointer->proc = (void (*)(const void *))glSecondaryColor3##let##v; \

#define __GL_FOG_FUNC(NAME, let) \
    case GL_##NAME: \
      fogPointer->proc = (void (*)(const void *))glFogCoord##let##v; \

#define __GL_INDEX_FUNC(NAME, let) \
    case GL_##NAME: \
      indexPointer->proc = (void (*)(const void *))glIndex##let##v; \
      break

#define __GL_TEXTURE_FUNC(NAME, let) \
    case GL_##NAME: \
      if (size == 1) { \
	texCoordPointer->proc = (void (*)(const void *))glTexCoord1##let##v; \
	texCoordPointer->mtex_proc = (void (*)(GLenum, const void *))glMultiTexCoord1##let##vARB; \
      } else if (size == 2) { \
	texCoordPointer->proc = (void (*)(const void *))glTexCoord2##let##v; \
	texCoordPointer->mtex_proc = (void (*)(GLenum, const void *))glMultiTexCoord2##let##vARB; \
      } else if (size == 3) { \
	texCoordPointer->proc = (void (*)(const void *))glTexCoord3##let##v; \
	texCoordPointer->mtex_proc = (void (*)(GLenum, const void *))glMultiTexCoord2##let##vARB; \
      } else if (size == 4) { \
	texCoordPointer->proc = (void (*)(const void *))glTexCoord4##let##v; \
	texCoordPointer->mtex_proc = (void (*)(GLenum, const void *))glMultiTexCoord4##let##vARB; \
      } break

/**
 * Table of sizes, in bytes, of a GL types.  All of the type enums are be in 
 * the range 0x1400 - 0x140F.  That includes types added by extensions (i.e.,
 * \c GL_HALF_FLOAT_NV).  This elements of this table correspond to the
 * type enums masked with 0x0f.
 * 
 * \notes
 * \c GL_HAVE_FLOAT_NV is not included.  Neither are \c GL_2_BYTES,
 * \c GL_3_BYTES, or \c GL_4_BYTES.
 */
static const GLuint __glXTypeSize_table[16] = {
    1, 1, 2, 2, 4, 4, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0
};

#define __glXTypeSize(e) ((((e) & ~0x0f) != 0x1400) \
    ? 0 : __glXTypeSize_table[ (e) & 0x0f ])


/**
 * Initialize vertex array state for a GLX context.
 * 
 * \param gc  GLX context whose vertex array state is to be initialized.
 * 
 * \todo
 * Someone is going to have to check the spec.  This function takes greate
 * care to initialize the \c size and \c type fields to "correct" values
 * for each array.  I'm not sure this is necessary.  I think it should be
 * acceptable to just \c memset the whole \c arrays and \c texCoord arrays
 * to zero and be done with it.  The spec may say something to the contrary,
 * however.
 */
void __glXInitVertexArrayState(__GLXcontext *gc)
{
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertArrayState *va = &state->vertArray;
    GLint i;

    va->enables = 0;
    va->texture_enables = 0;

    for ( i = 0 ; i < __GLX_MAX_ARRAYS ; i++ ) {
	va->arrays[ i ].proc = NULL;
	va->arrays[ i ].skip = 0;
	va->arrays[ i ].ptr = 0;
	va->arrays[ i ].size = 1;
	va->arrays[ i ].type = GL_FLOAT;
	va->arrays[ i ].stride = 0;
    }

    va->arrays[ edgeFlag_ARRAY ].type = GL_UNSIGNED_BYTE;;

    va->arrays[ secondaryColor_ARRAY ].size = 3;
    va->arrays[ color_ARRAY ].size = 4;
    va->arrays[ normal_ARRAY ].size = 3;
    va->arrays[ vertex_ARRAY ].size = 4;

    for ( i = 0 ; i < __GLX_MAX_TEXTURE_UNITS ; i++ ) {
	va->texCoord[ i ].proc = NULL;
	va->texCoord[ i ].skip = 0;
	va->texCoord[ i ].ptr = 0;
	va->texCoord[ i ].size = 4;
	va->texCoord[ i ].type = GL_FLOAT;
	va->texCoord[ i ].stride = 0;
    }

    va->maxElementsVertices = INT_MAX;
    va->maxElementsIndices = INT_MAX;
}

/*****************************************************************************/

void glVertexPointer(GLint size, GLenum type, GLsizei stride,
		     const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *vertexPointer = &state->vertArray.arrays[ vertex_ARRAY ];

    /* Check arguments */
    if (size < 2 || size > 4 || stride < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }

    /* Choose appropriate api proc */
    switch(type) {
        __GL_VERTEX_FUNC(SHORT, s);
        __GL_VERTEX_FUNC(INT, i);
        __GL_VERTEX_FUNC(FLOAT, f);
        __GL_VERTEX_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    vertexPointer->size = size;
    vertexPointer->type = type;
    vertexPointer->stride = stride;
    vertexPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
	vertexPointer->skip = __glXTypeSize(type) * size;
    } else {
	vertexPointer->skip = stride;
    }
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *normalPointer = &state->vertArray.arrays[ normal_ARRAY ];

    /* Check arguments */
    if (stride < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    switch(type) {
	__GL_NORMAL_FUNC(BYTE, b);
	__GL_NORMAL_FUNC(SHORT, s);
	__GL_NORMAL_FUNC(INT, i);
	__GL_NORMAL_FUNC(FLOAT, f);
	__GL_NORMAL_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    normalPointer->type = type;
    normalPointer->stride = stride;
    normalPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
	normalPointer->skip = 3 * __glXTypeSize(type);
    } else {
	normalPointer->skip = stride;
    }
}

void glColorPointer(GLint size, GLenum type, GLsizei stride,
		    const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *colorPointer = &state->vertArray.arrays[ color_ARRAY ];

    /* Check arguments */
    if (stride < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    switch(type) {
	__GL_COLOR_FUNC(BYTE, b);
	__GL_COLOR_FUNC(UNSIGNED_BYTE, ub);
	__GL_COLOR_FUNC(SHORT, s);
	__GL_COLOR_FUNC(UNSIGNED_SHORT, us);
	__GL_COLOR_FUNC(INT, i);
	__GL_COLOR_FUNC(UNSIGNED_INT, ui);
	__GL_COLOR_FUNC(FLOAT, f);
	__GL_COLOR_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    colorPointer->size = size;
    colorPointer->type = type;
    colorPointer->stride = stride;
    colorPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
        colorPointer->skip = size * __glXTypeSize(type);
    } else {
        colorPointer->skip = stride;
    }
}

void glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *indexPointer = &state->vertArray.arrays[ index_ARRAY ];

    /* Check arguments */
    if (stride < 0) {
        __glXSetError(gc, GL_INVALID_VALUE);
        return;
    }

    /* Choose appropriate api proc */
    switch(type) {
	__GL_INDEX_FUNC(UNSIGNED_BYTE, ub);
        __GL_INDEX_FUNC(SHORT, s);
        __GL_INDEX_FUNC(INT, i);
        __GL_INDEX_FUNC(FLOAT, f);
        __GL_INDEX_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    indexPointer->type = type;
    indexPointer->stride = stride;
    indexPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
	indexPointer->skip = __glXTypeSize(type);
    } else {
	indexPointer->skip = stride;
    }
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride,
		       const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *texCoordPointer =
    	&state->vertArray.texCoord[state->vertArray.activeTexture];

    /* Check arguments */
    if (size < 1 || size > 4 || stride < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    switch(type) {
	__GL_TEXTURE_FUNC(SHORT, s);
	__GL_TEXTURE_FUNC(INT, i);
	__GL_TEXTURE_FUNC(FLOAT, f);
	__GL_TEXTURE_FUNC(DOUBLE,  d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    texCoordPointer->size = size;
    texCoordPointer->type = type;
    texCoordPointer->stride = stride;
    texCoordPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
	texCoordPointer->skip = __glXTypeSize(type) * size;
    } else {
	texCoordPointer->skip = stride;
    }
}

void glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *edgeFlagPointer = &state->vertArray.arrays[ edgeFlag_ARRAY ];

    /* Check arguments */
    if (stride < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    edgeFlagPointer->proc = (void (*)(const void *))glEdgeFlagv;

    edgeFlagPointer->stride = stride;
    edgeFlagPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
	edgeFlagPointer->skip = sizeof(GLboolean);
    } else {
	edgeFlagPointer->skip = stride;
    }

}

void glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride,
			     const GLvoid * pointer )
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *seccolorPointer = &state->vertArray.arrays[ secondaryColor_ARRAY ];

    /* Check arguments */
    if ( (stride < 0) || (size != 3) ) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    switch(type) {
	__GL_SEC_COLOR_FUNC(BYTE, b);
	__GL_SEC_COLOR_FUNC(UNSIGNED_BYTE, ub);
	__GL_SEC_COLOR_FUNC(SHORT, s);
	__GL_SEC_COLOR_FUNC(UNSIGNED_SHORT, us);
	__GL_SEC_COLOR_FUNC(INT, i);
	__GL_SEC_COLOR_FUNC(UNSIGNED_INT, ui);
	__GL_SEC_COLOR_FUNC(FLOAT, f);
	__GL_SEC_COLOR_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    seccolorPointer->size = size;
    seccolorPointer->type = type;
    seccolorPointer->stride = stride;
    seccolorPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
        seccolorPointer->skip = size * __glXTypeSize(type);
    } else {
        seccolorPointer->skip = stride;
    }
}

void glFogCoordPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertexArrayPointerState *fogPointer = &state->vertArray.arrays[ fogCoord_ARRAY ];

    /* Check arguments */
    if (stride < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    } 

    /* Choose appropriate api proc */
    switch(type) {
	__GL_FOG_FUNC(FLOAT, f);
	__GL_FOG_FUNC(DOUBLE, d);
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    fogPointer->size = 1;
    fogPointer->type = type;
    fogPointer->stride = stride;
    fogPointer->ptr = pointer;

    /* Set internal state */
    if (stride == 0) {
        fogPointer->skip = __glXTypeSize(type);
    } else {
        fogPointer->skip = stride;
    }
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
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

    state->vertArray.enables = 0;
    state->vertArray.texture_enables = 0;
    if (tEnable) {
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(tSize, tType, trueStride, (const char *)pointer);
    }
    if (cEnable) {
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(cSize, cType, trueStride, (const char *)pointer+cOffset);
    }
    if (nEnable) {
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(nType, trueStride, (const char *)pointer+nOffset);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(vSize, vType, trueStride, (const char *)pointer+vOffset);
}

/*****************************************************************************/

void glArrayElement(GLint i)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertArrayState *va = &state->vertArray;
    GLint j;


    if (IS_TEXARRAY_ENABLED(state, 0)) {
	(*va->texCoord[0].proc)(va->texCoord[0].ptr+i*va->texCoord[0].skip);
    }

    /* Multitexturing is handled specially because the protocol
     * requires an extra parameter.
     */
    for (j=1; j<__GLX_MAX_TEXTURE_UNITS; ++j) {
	if (IS_TEXARRAY_ENABLED(state, j)) {
	    (*va->texCoord[j].mtex_proc)(GL_TEXTURE0 + j, va->texCoord[j].ptr+i*va->texCoord[j].skip);
	}
    }

    for ( j = 0 ; j < __GLX_MAX_ARRAYS ; j++ ) {
	if (IS_ARRAY_ENABLED_BY_INDEX(state, j)) {
	    (*va->arrays[ j ].proc)(va->arrays[ j ].ptr+i*va->arrays[ j ].skip);
	}
    }
}


struct array_info {
    __GLXdispatchDrawArraysComponentHeader ai;
    GLsizei bytes;
    const GLubyte *ptr;
    GLsizei skip;
};


/**
 * Initialize a \c array_info structure for each array that is enabled in
 * \c state.  Determine how many arrays are enabled, and store the result
 * in \c num_arrays.  Determine how big each vertex is, and store the result
 * in \c total_vertex_size.
 * 
 * \returns The size of the final request.  This is the size, in bytes, of
 * the DrawArrays header, the ARRAY_INFO structures, and all the vertex data.
 * This value \b assumes a \c X_GLXRender command is used.  The true size
 * will be 4 bytes larger if a \c X_GLXRenderLarge command is used.
 */
static GLuint
prep_arrays(const __GLXattribute * const state, struct array_info * arrays,
	    GLint count,
	    GLsizei *num_arrays, GLsizei *total_vertex_size)
{
    GLsizei na = 0;
    GLsizei vs = 0;

#define ASSIGN_ARRAY_INFO(state, enum_name, arr) \
    do { \
	arrays[ na ].ai.datatype = state->vertArray. arr .type ; \
	arrays[ na ].ai.numVals = state->vertArray. arr .size ; \
	arrays[ na ].ai.component = GL_ ## enum_name ## _ARRAY; \
\
	arrays[ na ].bytes = state->vertArray. arr .size \
	    * __glXTypeSize( state->vertArray. arr .type ); \
	arrays[ na ].ptr = state->vertArray. arr .ptr; \
	arrays[ na ].skip = state->vertArray. arr .skip; \
\
	vs += __GLX_PAD(arrays[ na ].bytes); \
	na++; \
    } while( 0 )

#define ADD_ARRAY_IF_ENABLED(state, enum_name, arr) \
    do { if ( IS_ARRAY_ENABLED(state, arr) ) { \
	ASSIGN_ARRAY_INFO(state, enum_name, arrays[ arr ## _ARRAY ] ); \
    } } while( 0 )
    
    ADD_ARRAY_IF_ENABLED(state, VERTEX, vertex);
    ADD_ARRAY_IF_ENABLED(state, NORMAL, normal);
    ADD_ARRAY_IF_ENABLED(state, COLOR, color);
    ADD_ARRAY_IF_ENABLED(state, SECONDARY_COLOR, secondaryColor);
    ADD_ARRAY_IF_ENABLED(state, FOG_COORD, fogCoord);
    ADD_ARRAY_IF_ENABLED(state, EDGE_FLAG, edgeFlag);
    ADD_ARRAY_IF_ENABLED(state, INDEX, index);

    /* The standard DrawArrays protocol *only* supports a single array of
     * texture coordinates.
     */
    if ( IS_TEXARRAY_ENABLED(state, 0) ) {
	ASSIGN_ARRAY_INFO(state, TEXTURE_COORD, texCoord[0]);
    }

    *num_arrays = na;
    *total_vertex_size = vs;

    return __GLX_PAD((__GLX_COMPONENT_HDR_SIZE * na)
		     + (vs * count)
		     + __GLX_DRAWARRAYS_CMD_HDR_SIZE);
}


/**
 * Emits the vertex data for the DrawArrays GLX protocol.
 */
static GLsizei
emit_vertex(GLubyte * data, const struct array_info * arrays,
	    GLsizei num_arrays, GLint element, GLsizei offset)
{
    GLint i;

    for ( i = 0 ; i < num_arrays ; i++ ) {
	(void) memcpy( data + offset,
		       arrays[i].ptr + (arrays[i].skip * element),
		       arrays[i].bytes );
	offset += __GLX_PAD(arrays[i].bytes);
    }

    return offset;
}


static void
emit_header(GLubyte * pc, const struct array_info * arrays,
	    GLsizei num_arrays, GLsizei count, GLenum mode)
{
    __GLXdispatchDrawArraysComponentHeader *arrayInfo;
    GLsizei i;

    __GLX_PUT_LONG(0, count);
    __GLX_PUT_LONG(4, num_arrays);
    __GLX_PUT_LONG(8, mode);

    arrayInfo = (__GLXdispatchDrawArraysComponentHeader *)
	(pc + __GLX_DRAWARRAYS_HDR_SIZE);


    /* Write the ARRAY_INFO data.
     */

    for ( i = 0 ; i < num_arrays ; i++ ) {
	arrayInfo[i] = arrays[i].ai;
    }
}


/**
 * Emit GLX DrawArrays protocol using a GLXRender packet.
 */
static void   
emit_Render_DrawArrays(__GLXcontext * gc, const struct array_info * arrays,
		       GLsizei first, GLsizei count, GLsizei num_arrays, GLenum mode,
		       GLsizei cmdlen, GLsizei total_vertex_size)
{
    GLubyte * pc = gc->pc;
    GLsizei offset;
    GLsizei i;

    __GLX_BEGIN_VARIABLE(X_GLrop_DrawArrays, cmdlen);
    emit_header(pc + 4, arrays, num_arrays, count, mode);


    /* Write the actual array data.
     */

    offset = __GLX_DRAWARRAYS_CMD_HDR_SIZE 
	+ (num_arrays * __GLX_COMPONENT_HDR_SIZE);
    for ( i = 0 ; i < count ; i++ ) {
	offset = emit_vertex(pc, arrays, num_arrays, i + first, offset);
    }

    __GLX_END(cmdlen);
}


/**
 * Emit GLX DrawArrays protocol using a GLXRenderLarge packet.
 */
static void   
emit_RenderLarge_DrawArrays(__GLXcontext * gc, const struct array_info * arrays,
		       GLsizei first, GLsizei count, GLsizei num_arrays, GLenum mode,
		       GLsizei cmdlen, GLsizei total_vertex_size)
{
    GLubyte * pc = gc->pc;
    GLsizei offset;
    GLsizei i;
    GLint maxSize;
    GLint totalRequests;
    GLint requestNumber;
    GLsizei elements_per_request;


    /* Calculate the maximum amount of data can be stuffed into a single
     * packet.  sz_xGLXRenderReq is added because bufSize is the maximum
     * packet size minus sz_xGLXRenderReq.
     * 
     * The important value here is elements_per_request.  This is the number
     * of complete array elements that will fit in a single buffer.  There
     * may be some wasted space at the end of the buffer, but splitting
     * elements across buffer boundries would be painful.
     */

    maxSize = (gc->bufSize + sz_xGLXRenderReq) - sz_xGLXRenderLargeReq;

    elements_per_request = maxSize / total_vertex_size;

    totalRequests = ((count + (elements_per_request - 1))
		     / elements_per_request) + 1;


    /* Fill in the header data and send it away.
     */

    __GLX_BEGIN_VARIABLE_LARGE(X_GLrop_DrawArrays, cmdlen+4);
    emit_header(pc + 8, arrays, num_arrays, count, mode);

    gc->pc = pc + (__GLX_DRAWARRAYS_CMD_HDR_SIZE + 4)
	    + (__GLX_COMPONENT_HDR_SIZE * num_arrays);
    __glXSendLargeChunk(gc, 1, totalRequests, gc->buf, gc->pc - gc->buf);


    /* Write the actual array data.
     */
    offset = 0;
    requestNumber = 2;
    for ( i = 0 ; i < count ; i++ ) {
	if ( i == elements_per_request ) {
	    __glXSendLargeChunk(gc, requestNumber, totalRequests, 
				gc->buf, offset);
	    requestNumber++;
	    offset = 0;

	    count -= i;
	    first += i;
	    i = 0;
	}

	offset = emit_vertex(gc->buf, arrays, num_arrays, i + first, offset);
    }

    /* If the buffer isn't empty, emit the last, partial request.
     */
    if ( offset != 0 ) {
	assert(requestNumber == totalRequests);
	__glXSendLargeChunk(gc, requestNumber, totalRequests, gc->buf, offset);
    }

    gc->pc = gc->buf;
}


/**
 * Emit DrawArrays protocol.  This function acts as a switch betteen
 * \c emit_Render_DrawArrays and \c emit_RenderLarge_DrawArrays depending
 * on how much array data is to be sent.
 */
static void
emit_DrawArraysEXT(const __GLXattribute * const state,
		   GLint first, GLsizei count, GLenum mode)
{
    struct array_info arrays[32];
    GLsizei num_arrays;
    GLsizei total_vertex_size;
     __GLXcontext *gc = __glXGetCurrentContext();
    GLuint cmdlen;


    /* Determine how big the final request will be.  This depends on a number
     * of factors.  It depends on how many array elemets there are (which is
     * the passed-in 'count'), how many arrays are enabled, how many elements
     * are in each array entry, and what the types are for each array.
     */

    cmdlen = prep_arrays(state, arrays, count, & num_arrays,
			 & total_vertex_size);


    /* If the data payload and the protocol header is too large for a Render
     * command, use a RenderLarge command.
     */
    if (cmdlen > gc->maxSmallRenderCommandSize) {
	emit_RenderLarge_DrawArrays(gc, arrays, first, count, num_arrays,
				    mode, cmdlen, total_vertex_size);
    }
    else {
	emit_Render_DrawArrays(gc, arrays, first, count, num_arrays,
			       mode, cmdlen, total_vertex_size);
    }
}


/**
 * Emit a DrawArrays call using the old "protocol."  This isn't really
 * DrawArrays protocol at all.  It just simulates DrawArrays by using
 * immediate-mode vertex calls.  Very, very slow for large arrays, but works
 * with every GLX server.
 */
static void
emit_DrawArrays_old(const __GLXattribute * const state, 
		    GLint first, GLsizei count, GLenum mode)
{
    const __GLXvertArrayState *va = &state->vertArray;
    const GLubyte *vaPtr[__GLX_MAX_ARRAYS];
    const GLubyte *tcaPtr[__GLX_MAX_TEXTURE_UNITS];
    GLint i, j;

    /*
    ** Set up pointers for quick array traversal.
    */
    
    (void) memset( vaPtr,  0, sizeof(vaPtr) );
    (void) memset( tcaPtr, 0, sizeof(tcaPtr) );

    for ( j = 0 ; j < __GLX_MAX_ARRAYS ; j++ ) {
	if (IS_ARRAY_ENABLED_BY_INDEX(state, j)) {
	    vaPtr[ j ] = va->arrays[ j ].ptr + first * va->arrays[ j ].skip;
	}
    }

    for ( j = 0 ; j < __GLX_MAX_TEXTURE_UNITS ; j++ ) {
	if (IS_TEXARRAY_ENABLED(state, j)) 
	    tcaPtr[ j ] = va->texCoord[ j ].ptr + first * va->texCoord[ j ].skip;
    }

    glBegin(mode);
        for (i = 0; i < count; i++) {
	    if (IS_TEXARRAY_ENABLED(state, 0)) {
		(*va->texCoord[0].proc)(tcaPtr[0]);
		tcaPtr[0] += va->texCoord[0].skip;
	    }

	    /* Multitexturing is handled specially because the protocol
	     * requires an extra parameter.
	     */
	    for (j=1; j<__GLX_MAX_TEXTURE_UNITS; ++j) {
		if (IS_TEXARRAY_ENABLED(state, j)) {
		    (*va->texCoord[j].mtex_proc)(GL_TEXTURE0 + j, tcaPtr[j]);
		    tcaPtr[j] += va->texCoord[j].skip;
		}
	    }

	    for ( j = 0 ; j < __GLX_MAX_ARRAYS ; j++ ) {
		if (IS_ARRAY_ENABLED_BY_INDEX(state, j)) {
		    (*va->arrays[ j ].proc)(vaPtr[ j ]);
		    vaPtr[ j ] += va->arrays[ j ].skip;
		}
            }
        }
    glEnd();
}


/**
 * Validate that the \c mode and \c count parameters to \c glDrawArrays or
 * \c glDrawElements are valid.  If the arguments are not valid, then an
 * error code is set in the GLX context.
 * 
 * \returns \c GL_TRUE if the arguments are valide, \c GL_FALSE if they are
 *          not.
 */
static GLboolean
glx_validate_array_args(__GLXcontext *gc, GLenum mode, GLsizei count)
{
    switch(mode) {
      case GL_POINTS:
      case GL_LINE_STRIP:
      case GL_LINE_LOOP:
      case GL_LINES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLES:
      case GL_QUAD_STRIP:
      case GL_QUADS:
      case GL_POLYGON:
	break;
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return GL_FALSE;
    }

    if (count < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return GL_FALSE;
    }

    return GL_TRUE;
}


void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    const __GLXattribute * state = 
       (const __GLXattribute *)(gc->client_state_private);


    if ( ! glx_validate_array_args(gc, mode, count) ) {
	return;
    }

    /* The "true" DrawArrays protocol does not support generic attributes,
     * multiple vertex arrays, or multiple texture coordinate arrays.
     */
    if ( state->NoDrawArraysProtocol 
	 || (state->vertArray.texture_enables > 1) ) {
	emit_DrawArrays_old(state, first, count, mode);
    }
    else {
	emit_DrawArraysEXT(state, first, count, mode);
    }
}


/**
 * \todo Modify this to use the "true" DrawArrays protocol if possible.  This
 * would probably require refactoring out parts of \c emit_DrawArraysEXT into
 * more general functions that could be used in either place.
 */
void glDrawElements(GLenum mode, GLsizei count, GLenum type,
		    const GLvoid *indices)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    __GLXvertArrayState *va = &state->vertArray;
    const GLubyte *iPtr1 = NULL;
    const GLushort *iPtr2 = NULL;
    const GLuint *iPtr3 = NULL;
    GLint i, j, offset = 0;

    if ( ! glx_validate_array_args(gc, mode, count) ) {
	return;
    }

    switch (type) {
      case GL_UNSIGNED_BYTE:
	iPtr1 = (const GLubyte *)indices;
	break;
      case GL_UNSIGNED_SHORT:
	iPtr2 = (const GLushort *)indices;
	break;
      case GL_UNSIGNED_INT:
	iPtr3 = (const GLuint *)indices;
	break;
      default:
        __glXSetError(gc, GL_INVALID_ENUM);
        return;
    }

    glBegin(mode);
        for (i = 0; i < count; i++) {
	    switch (type) {
	      case GL_UNSIGNED_BYTE:
		offset = (GLint)(*iPtr1++);
		break;
	      case GL_UNSIGNED_SHORT:
		offset = (GLint)(*iPtr2++);
		break;
	      case GL_UNSIGNED_INT:
		offset = (GLint)(*iPtr3++);
		break;
	    }

	    if (IS_TEXARRAY_ENABLED(state, 0)) {
		(*va->texCoord[0].proc)(va->texCoord[0].ptr+
					(offset*va->texCoord[0].skip));
	    }

	    /* Multitexturing is handled specially because the protocol
	     * requires an extra parameter.
	     */
	    for (j=1; j<__GLX_MAX_TEXTURE_UNITS; ++j) {
		if (IS_TEXARRAY_ENABLED(state, j)) {
		    (*va->texCoord[j].mtex_proc)(GL_TEXTURE0 + j,
						 va->texCoord[j].ptr+
						 (offset*va->texCoord[j].skip));
		}
	    }

	    for ( j = 0 ; j < __GLX_MAX_ARRAYS ; j++ ) {
		if (IS_ARRAY_ENABLED_BY_INDEX(state, j)) {
		    (*va->arrays[ j ].proc)(va->arrays[ j ].ptr
					    +(offset*va->arrays[ j ].skip));
		}
            }
        }
    glEnd();
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end,
			 GLsizei count, GLenum type,
			 const GLvoid *indices)
{
    __GLXcontext *gc = __glXGetCurrentContext();

    if (end < start) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }

    glDrawElements(mode,count,type,indices);
}

void glMultiDrawArrays(GLenum mode, GLint *first, GLsizei *count,
		       GLsizei primcount)
{
   GLsizei  i;

   for(i=0; i<primcount; i++) {
      if ( count[i] > 0 ) {
	  glDrawArrays( mode, first[i], count[i] );
      }
   }
}

void glMultiDrawElements(GLenum mode, const GLsizei *count,
			 GLenum type, const GLvoid ** indices,
			 GLsizei primcount)
{
   GLsizei  i;

   for(i=0; i<primcount; i++) {
      if ( count[i] > 0 ) {
	  glDrawElements( mode, count[i], type, indices[i] );
      }
   }
}

void glClientActiveTextureARB(GLenum texture)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    GLint unit = (GLint) texture - GL_TEXTURE0;

    if (unit < 0 || __GLX_MAX_TEXTURE_UNITS <= unit) {
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    }
    state->vertArray.activeTexture = unit;
}
