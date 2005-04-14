/* $XFree86: xc/lib/GL/glx/single2.c,v 1.10 2004/02/11 19:48:16 dawes Exp $ */
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

#include <stdio.h>
#include <assert.h>
#include "glxclient.h"
#include "packsingle.h"
#include "glxextensions.h"
#include "indirect.h"
#include "indirect_vertex_array.h"

/* Used for GL_ARB_transpose_matrix */
static void TransposeMatrixf(GLfloat m[16])
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            GLfloat tmp = m[i*4+j];
            m[i*4+j] = m[j*4+i];
            m[j*4+i] = tmp;
        }
    }
}

/* Used for GL_ARB_transpose_matrix */
static void TransposeMatrixb(GLboolean m[16])
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            GLboolean tmp = m[i*4+j];
            m[i*4+j] = m[j*4+i];
            m[j*4+i] = tmp;
        }
    }
}

/* Used for GL_ARB_transpose_matrix */
static void TransposeMatrixd(GLdouble m[16])
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            GLdouble tmp = m[i*4+j];
            m[i*4+j] = m[j*4+i];
            m[j*4+i] = tmp;
        }
    }
}

/* Used for GL_ARB_transpose_matrix */
static void TransposeMatrixi(GLint m[16])
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < i; j++) {
            GLint tmp = m[i*4+j];
            m[i*4+j] = m[j*4+i];
            m[j*4+i] = tmp;
        }
    }
}


/**
 * Remap a transpose-matrix enum to a non-transpose-matrix enum.  Enums
 * that are not transpose-matrix enums are unaffected.
 */
static GLenum
RemapTransposeEnum( GLenum e )
{
    switch( e ) {
    case GL_TRANSPOSE_MODELVIEW_MATRIX:
    case GL_TRANSPOSE_PROJECTION_MATRIX:
    case GL_TRANSPOSE_TEXTURE_MATRIX:
	return e - (GL_TRANSPOSE_MODELVIEW_MATRIX - GL_MODELVIEW_MATRIX);
    case GL_TRANSPOSE_COLOR_MATRIX:
	return GL_COLOR_MATRIX;
    default:
	return e;
    };
}


GLenum __indirect_glGetError(void)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    GLuint retval = GL_NO_ERROR;
    xGLXGetErrorReply reply;

    if (gc->error) {
	/* Use internal error first */
	retval = gc->error;
	gc->error = GL_NO_ERROR;
	return retval;
    }

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetError,0);
    __GLX_SINGLE_READ_XREPLY();
    retval = reply.error;
    __GLX_SINGLE_END();

    return retval;
}


/**
 * Get the selected attribute from the client state.
 * 
 * \returns
 * On success \c GL_TRUE is returned.  Otherwise, \c GL_FALSE is returned.
 */
static GLboolean
get_client_data( __GLXattribute * state, GLenum cap, GLintptr * data )
{
    GLboolean retval = GL_TRUE;
    const GLint tex_unit = __glXGetActiveTextureUnit( state );


    switch( cap ) {
    case GL_VERTEX_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_INDEX_ARRAY:
    case GL_EDGE_FLAG_ARRAY:
    case GL_SECONDARY_COLOR_ARRAY:
    case GL_FOG_COORD_ARRAY:
	retval = __glXGetArrayEnable( state, cap, 0, data );
	break;

    case GL_VERTEX_ARRAY_SIZE:
	retval = __glXGetArraySize( state, GL_VERTEX_ARRAY, 0, data );
	break;
    case GL_COLOR_ARRAY_SIZE:
	retval = __glXGetArraySize( state, GL_COLOR_ARRAY, 0, data );
	break;
    case GL_SECONDARY_COLOR_ARRAY_SIZE:
	retval = __glXGetArraySize( state, GL_SECONDARY_COLOR_ARRAY, 0, data );
	break;

    case GL_VERTEX_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_VERTEX_ARRAY, 0, data );
	break;
    case GL_NORMAL_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_NORMAL_ARRAY, 0, data );
	break;
    case GL_INDEX_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_INDEX_ARRAY, 0, data );
	break;
    case GL_COLOR_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_COLOR_ARRAY, 0, data );
	break;
    case GL_SECONDARY_COLOR_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_SECONDARY_COLOR_ARRAY, 0, data );
	break;
    case GL_FOG_COORD_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_FOG_COORD_ARRAY, 0, data );
	break;

    case GL_VERTEX_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_VERTEX_ARRAY, 0, data );
	break;
    case GL_NORMAL_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_NORMAL_ARRAY, 0, data );
	break;
    case GL_INDEX_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_INDEX_ARRAY, 0, data );
	break;
    case GL_EDGE_FLAG_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_EDGE_FLAG_ARRAY, 0, data );
	break;
    case GL_COLOR_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_COLOR_ARRAY, 0, data );
	break;
    case GL_SECONDARY_COLOR_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_SECONDARY_COLOR_ARRAY, 0, data );
	break;
    case GL_FOG_COORD_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_FOG_COORD_ARRAY, 0, data );
	break;

    case GL_TEXTURE_COORD_ARRAY:
	retval = __glXGetArrayEnable( state, GL_TEXTURE_COORD_ARRAY, tex_unit, data );
	break;
    case GL_TEXTURE_COORD_ARRAY_SIZE:
	retval = __glXGetArraySize( state, GL_TEXTURE_COORD_ARRAY, tex_unit, data );
	break;
    case GL_TEXTURE_COORD_ARRAY_TYPE:
	retval = __glXGetArrayType( state, GL_TEXTURE_COORD_ARRAY, tex_unit, data );
	break;
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
	retval = __glXGetArrayStride( state, GL_TEXTURE_COORD_ARRAY, tex_unit, data );
	break;

    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_ELEMENTS_INDICES:
	retval = GL_TRUE;
	*data = ~0UL;
	break;

    
    case GL_PACK_ROW_LENGTH:
	*data = (GLintptr)state->storePack.rowLength;
	break;
    case GL_PACK_IMAGE_HEIGHT:
	*data = (GLintptr)state->storePack.imageHeight;
	break;
    case GL_PACK_SKIP_ROWS:
	*data = (GLintptr)state->storePack.skipRows;
	break;
    case GL_PACK_SKIP_PIXELS:
	*data = (GLintptr)state->storePack.skipPixels;
	break;
    case GL_PACK_SKIP_IMAGES:
	*data = (GLintptr)state->storePack.skipImages;
	break;
    case GL_PACK_ALIGNMENT:
	*data = (GLintptr)state->storePack.alignment;
	break;
    case GL_PACK_SWAP_BYTES:
	*data = (GLintptr)state->storePack.swapEndian;
	break;
    case GL_PACK_LSB_FIRST:
	*data = (GLintptr)state->storePack.lsbFirst;
	break;
    case GL_UNPACK_ROW_LENGTH:
	*data = (GLintptr)state->storeUnpack.rowLength;
	break;
    case GL_UNPACK_IMAGE_HEIGHT:
	*data = (GLintptr)state->storeUnpack.imageHeight;
	break;
    case GL_UNPACK_SKIP_ROWS:
	*data = (GLintptr)state->storeUnpack.skipRows;
	break;
    case GL_UNPACK_SKIP_PIXELS:
	*data = (GLintptr)state->storeUnpack.skipPixels;
	break;
    case GL_UNPACK_SKIP_IMAGES:
	*data = (GLintptr)state->storeUnpack.skipImages;
	break;
    case GL_UNPACK_ALIGNMENT:
	*data = (GLintptr)state->storeUnpack.alignment;
	break;
    case GL_UNPACK_SWAP_BYTES:
	*data = (GLintptr)state->storeUnpack.swapEndian;
	break;
    case GL_UNPACK_LSB_FIRST:
	*data = (GLintptr)state->storeUnpack.lsbFirst;
	break;
    case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
	*data = (GLintptr)__GL_CLIENT_ATTRIB_STACK_DEPTH;
	break;
    case GL_CLIENT_ACTIVE_TEXTURE:
	*data = (GLintptr)(tex_unit + GL_TEXTURE0);
	break;

    default:
	retval = GL_FALSE;
	break;
    }


    return retval;
}


void __indirect_glGetBooleanv(GLenum val, GLboolean *b)
{
    const GLenum origVal = val;
    __GLX_SINGLE_DECLARE_VARIABLES();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    xGLXSingleReply reply;

    val = RemapTransposeEnum( val );

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetBooleanv,4);
    __GLX_SINGLE_PUT_LONG(0,val);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_SIZE(compsize);

    if (compsize == 0) {
	/*
	** Error occured; don't modify user's buffer.
	*/
    } else {
	GLintptr data;

	/*
	** We still needed to send the request to the server in order to
	** find out whether it was legal to make a query (it's illegal,
	** for example, to call a query between glBegin() and glEnd()).
	*/

	if ( get_client_data( state, val, & data ) ) {
	    *b = (GLboolean) data;
	}
	else {
	    /*
	     ** Not a local value, so use what we got from the server.
	     */
	    if (compsize == 1) {
		__GLX_SINGLE_GET_CHAR(b);
	    } else {
		__GLX_SINGLE_GET_CHAR_ARRAY(b,compsize);
		if (val != origVal) {
		    /* matrix transpose */
		    TransposeMatrixb(b);
                }
	    }
	}
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetDoublev(GLenum val, GLdouble *d)
{
    const GLenum origVal = val;
    __GLX_SINGLE_DECLARE_VARIABLES();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    xGLXSingleReply reply;

    val = RemapTransposeEnum( val );

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetDoublev,4);
    __GLX_SINGLE_PUT_LONG(0,val);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_SIZE(compsize);

    if (compsize == 0) {
	/*
	** Error occured; don't modify user's buffer.
	*/
    } else {
	GLintptr data;

	/*
	** We still needed to send the request to the server in order to
	** find out whether it was legal to make a query (it's illegal,
	** for example, to call a query between glBegin() and glEnd()).
	*/

	if ( get_client_data( state, val, & data ) ) {
	    *d = (GLdouble) data;
	}
	else {
	    /*
	     ** Not a local value, so use what we got from the server.
	     */
	    if (compsize == 1) {
		__GLX_SINGLE_GET_DOUBLE(d);
	    } else {
		__GLX_SINGLE_GET_DOUBLE_ARRAY(d,compsize);
		if (val != origVal) {
		    /* matrix transpose */
		    TransposeMatrixd(d);
		}
	    }
	}
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetFloatv(GLenum val, GLfloat *f)
{
    const GLenum origVal = val;
    __GLX_SINGLE_DECLARE_VARIABLES();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    xGLXSingleReply reply;

    val = RemapTransposeEnum( val );

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetFloatv,4);
    __GLX_SINGLE_PUT_LONG(0,val);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_SIZE(compsize);

    if (compsize == 0) {
	/*
	** Error occured; don't modify user's buffer.
	*/
    } else {
	GLintptr data;

	/*
	** We still needed to send the request to the server in order to
	** find out whether it was legal to make a query (it's illegal,
	** for example, to call a query between glBegin() and glEnd()).
	*/

	if ( get_client_data( state, val, & data ) ) {
	    *f = (GLfloat) data;
	}
	else {
	    /*
	     ** Not a local value, so use what we got from the server.
	     */
	    if (compsize == 1) {
		__GLX_SINGLE_GET_FLOAT(f);
	    } else {
		__GLX_SINGLE_GET_FLOAT_ARRAY(f,compsize);
		if (val != origVal) {
		    /* matrix transpose */
		    TransposeMatrixf(f);
                }
	    }
	}
    }
    __GLX_SINGLE_END();
}

void __indirect_glGetIntegerv(GLenum val, GLint *i)
{
    const GLenum origVal = val;
    __GLX_SINGLE_DECLARE_VARIABLES();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    xGLXSingleReply reply;

    val = RemapTransposeEnum( val );

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_GetIntegerv,4);
    __GLX_SINGLE_PUT_LONG(0,val);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_SIZE(compsize);

    if (compsize == 0) {
	/*
	** Error occured; don't modify user's buffer.
	*/
    } else {
	GLintptr data;

	/*
	** We still needed to send the request to the server in order to
	** find out whether it was legal to make a query (it's illegal,
	** for example, to call a query between glBegin() and glEnd()).
	*/

	if ( get_client_data( state, val, & data ) ) {
	    *i = (GLint) data;
	}
	else {
	    /*
	     ** Not a local value, so use what we got from the server.
	     */
	    if (compsize == 1) {
		__GLX_SINGLE_GET_LONG(i);
	    } else {
		__GLX_SINGLE_GET_LONG_ARRAY(i,compsize);
		if (val != origVal) {
		    /* matrix transpose */
		    TransposeMatrixi(i);
                }
	    }
	}
    }
    __GLX_SINGLE_END();
}

/*
** Send all pending commands to server.
*/
void __indirect_glFlush(void)
{
    __GLX_SINGLE_DECLARE_VARIABLES();

    if (!dpy) return;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_Flush,0);
    __GLX_SINGLE_END();

    /* And finally flush the X protocol data */
    XFlush(dpy);
}

void __indirect_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    __GLX_SINGLE_DECLARE_VARIABLES();

    if (!dpy) return;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_FeedbackBuffer,8);
    __GLX_SINGLE_PUT_LONG(0,size);
    __GLX_SINGLE_PUT_LONG(4,type);
    __GLX_SINGLE_END();

    gc->feedbackBuf = buffer;
}

void __indirect_glSelectBuffer(GLsizei numnames, GLuint *buffer)
{
    __GLX_SINGLE_DECLARE_VARIABLES();

    if (!dpy) return;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_SelectBuffer,4);
    __GLX_SINGLE_PUT_LONG(0,numnames);
    __GLX_SINGLE_END();

    gc->selectBuf = buffer;
}

GLint __indirect_glRenderMode(GLenum mode)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    GLint retval = 0;
    xGLXRenderModeReply reply;

    if (!dpy) return -1;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_RenderMode,4);
    __GLX_SINGLE_PUT_LONG(0,mode);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_RETVAL(retval,GLint);

    if (reply.newMode != mode) {
	/*
	** Switch to new mode did not take effect, therefore an error
	** occured.  When an error happens the server won't send us any
	** other data.
	*/
    } else {
	/* Read the feedback or selection data */
	if (gc->renderMode == GL_FEEDBACK) {
	    __GLX_SINGLE_GET_SIZE(compsize);
	    __GLX_SINGLE_GET_FLOAT_ARRAY(gc->feedbackBuf, compsize);
	} else
	if (gc->renderMode == GL_SELECT) {
	    __GLX_SINGLE_GET_SIZE(compsize);
	    __GLX_SINGLE_GET_LONG_ARRAY(gc->selectBuf, compsize);
	}
	gc->renderMode = mode;
    }
    __GLX_SINGLE_END();

    return retval;
}

void __indirect_glFinish(void)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    xGLXSingleReply reply;

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_Finish,0);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_END();
}


/**
 * Extract the major and minor version numbers from a version string.
 */
static void
version_from_string( const char * ver, 
		     int * major_version, int * minor_version )
{
    const char * end;
    long major;
    long minor;

    major = strtol( ver, (char **) & end, 10 );
    minor = strtol( end + 1, NULL, 10 );
    *major_version = major;
    *minor_version = minor;
}


const GLubyte *__indirect_glGetString(GLenum name)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    Display *dpy = gc->currentDpy;
    GLubyte *s = NULL;

    if (!dpy) return 0;

    /*
    ** Return the cached copy if the string has already been fetched
    */
    switch(name) {
      case GL_VENDOR:
	if (gc->vendor) return gc->vendor;
	break;
      case GL_RENDERER:
	if (gc->renderer) return gc->renderer;
	break;
      case GL_VERSION:
	if (gc->version) return gc->version;
	break;
      case GL_EXTENSIONS:
	if (gc->extensions) return gc->extensions;
	break;
      default:
	__glXSetError(gc, GL_INVALID_ENUM);
	return 0;
    }

    /*
    ** Get requested string from server
    */

    (void) __glXFlushRenderBuffer( gc, gc->pc );
    s = (GLubyte *) __glXGetStringFromServer( dpy, gc->majorOpcode,
				  X_GLsop_GetString, gc->currentContextTag,
				  name );
    if (!s) {
	/* Throw data on the floor */
	__glXSetError(gc, GL_OUT_OF_MEMORY);
    } else {
	/*
	** Update local cache
	*/
	switch(name) {
	case GL_VENDOR:
	    gc->vendor = s;
	    break;

	case GL_RENDERER:
	    gc->renderer = s;
	    break;

	case GL_VERSION: {
	    int client_major;
	    int client_minor;

	    version_from_string( (char *) s, 
				 & gc->server_major, & gc->server_minor );
	    __glXGetGLVersion( & client_major, & client_minor );

	    if ( (gc->server_major < client_major)
		 || ((gc->server_major == client_major) 
		     && (gc->server_minor <= client_minor)) ) {
		gc->version = s;
	    }
	    else {
		/* Allow 7 bytes for the client-side GL version.  This allows
		 * for upto version 999.999.  I'm not holding my breath for
		 * that one!  The extra 4 is for the ' ()\0' that will be
		 * added.
		 */
		const size_t size = 7 + strlen( (char *) s ) + 4;

		gc->version = Xmalloc( size );
		if ( gc->version == NULL ) {
		    /* If we couldn't allocate memory for the new string,
		     * make a best-effort and just copy the client-side version
		     * to the string and use that.  It probably doesn't
		     * matter what is done here.  If there not memory available
		     * for a short string, the system is probably going to die
		     * soon anyway.
		     */
		    snprintf( (char *) s, strlen( (char *) s ) + 1, "%u.%u",
			      client_major, client_minor );
		    gc->version = s;
		}
		else {
		    snprintf( (char *)gc->version, size, "%u.%u (%s)",
			      client_major, client_minor, s );
		    Xfree( s );
		    s = gc->version;
		}
	    }
	    break;
	}

	case GL_EXTENSIONS: {
	    int major = 1;
	    int minor = 0;

	    /* This code is currently disabled.  I was reminded that some
	     * vendors intentionally exclude some extensions from their
	     * extension string that are part of the core version they
	     * advertise.  In particular, on Nvidia drivers this means that
	     * the functionality is supported by the driver, but is not
	     * hardware accelerated.  For example, a TNT will show core
	     * version 1.5, but most of the post-1.2 functionality is a
	     * software fallback.
	     * 
	     * I don't want to break applications that rely on this odd
	     * behavior.  At the same time, the code is written and tested,
	     * so I didn't want to throw it away.  Therefore, the code is here
	     * but disabled.  In the future, we may wish to and an environment
	     * variable to enable it.
	     */
	    
#if 0
	    /* Call glGetString just to make sure that gc->server_major and
	     * gc->server_minor are set.  This version may be higher than we
	     * can completely support, but it may imply support for some
	     * extensions that we can support.
	     * 
	     * For example, at the time of this writing, the client-side
	     * library only supports upto core GL version 1.2.  However, cubic
	     * textures, multitexture, multisampling, and some other 1.3
	     * features are supported.  If the server reports back version
	     * 1.3, but does not report all of those extensions, we will
	     * enable them.
	     */
	    (void *) glGetString( GL_VERSION );
	    major = gc->server_major,
	    minor = gc->server_minor;
#endif

	    __glXCalculateUsableGLExtensions( gc, (char *) s, major, minor );
	    XFree( s );
	    s = gc->extensions;
	    break;
	}
	}
    }
    return s;
}

GLboolean __indirect_glIsEnabled(GLenum cap)
{
    __GLX_SINGLE_DECLARE_VARIABLES();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    xGLXSingleReply reply;
    GLboolean retval = 0;
    GLintptr enable;

    if (!dpy) return 0;

    switch(cap) {
    case GL_VERTEX_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_INDEX_ARRAY:
    case GL_EDGE_FLAG_ARRAY:
    case GL_SECONDARY_COLOR_ARRAY:
    case GL_FOG_COORD_ARRAY:
	retval = __glXGetArrayEnable( state, cap, 0, & enable );
	assert( retval );
	return (GLboolean) enable;
	break;
    case GL_TEXTURE_COORD_ARRAY:
	retval = __glXGetArrayEnable( state, GL_TEXTURE_COORD_ARRAY,
				      __glXGetActiveTextureUnit( state ), & enable );
	assert( retval );
	return (GLboolean) enable;
	break;
    }

    __GLX_SINGLE_LOAD_VARIABLES();
    __GLX_SINGLE_BEGIN(X_GLsop_IsEnabled,4);
    __GLX_SINGLE_PUT_LONG(0,cap);
    __GLX_SINGLE_READ_XREPLY();
    __GLX_SINGLE_GET_RETVAL(retval, GLboolean);
    __GLX_SINGLE_END();
    return retval;
}

void __indirect_glGetPointerv(GLenum pname, void **params)
{
    __GLXcontext *gc = __glXGetCurrentContext();
    __GLXattribute * state = (__GLXattribute *)(gc->client_state_private);
    Display *dpy = gc->currentDpy;

    if (!dpy) return;

    switch(pname) {
    case GL_VERTEX_ARRAY_POINTER:
    case GL_NORMAL_ARRAY_POINTER:
    case GL_COLOR_ARRAY_POINTER:
    case GL_INDEX_ARRAY_POINTER:
    case GL_EDGE_FLAG_ARRAY_POINTER:
	__glXGetArrayPointer( state, pname - GL_VERTEX_ARRAY_POINTER 
			      + GL_VERTEX_ARRAY,
			      0, params );
	return;
    case GL_TEXTURE_COORD_ARRAY_POINTER:
	__glXGetArrayPointer( state, GL_TEXTURE_COORD_ARRAY, 
			      __glXGetActiveTextureUnit( state ), params );
	return;
    case GL_SECONDARY_COLOR_ARRAY_POINTER:
    case GL_FOG_COORD_ARRAY_POINTER:
	__glXGetArrayPointer( state, pname - GL_FOG_COORD_ARRAY_POINTER
			      + GL_FOG_COORD_ARRAY, 
			      0, params );
	return;
    case GL_FEEDBACK_BUFFER_POINTER:
	*params = (void *)gc->feedbackBuf;
	return;
    case GL_SELECTION_BUFFER_POINTER:
	*params = (void *)gc->selectBuf;
	return;
    default:
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    }
}

