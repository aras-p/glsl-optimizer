/* $XFree86$ */
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
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
*/

#include "packsingle.h"

void glNewList(GLuint list, GLenum mode)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_NewList,8);
	__GLX_SINGLE_PUT_LONG(0,list);
	__GLX_SINGLE_PUT_LONG(4,mode);
	__GLX_SINGLE_END();
}

void glEndList(void)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_EndList,0);
	__GLX_SINGLE_END();
}

void glDeleteLists(GLuint list, GLsizei range)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_DeleteLists,8);
	__GLX_SINGLE_PUT_LONG(0,list);
	__GLX_SINGLE_PUT_LONG(4,range);
	__GLX_SINGLE_END();
}

GLuint glGenLists(GLsizei range)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	GLuint    retval = 0;
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GenLists,4);
	__GLX_SINGLE_PUT_LONG(0,range);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_RETVAL(retval, GLuint);
	__GLX_SINGLE_END();
	return retval;
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetLightfv,8);
	__GLX_SINGLE_PUT_LONG(0,light);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetLightiv,8);
	__GLX_SINGLE_PUT_LONG(0,light);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMapdv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,query);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_DOUBLE(v);
	} else {
	    __GLX_SINGLE_GET_DOUBLE_ARRAY(v,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMapfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,query);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(v);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(v,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMapiv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,query);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(v);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(v,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMaterialfv,8);
	__GLX_SINGLE_PUT_LONG(0,face);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMaterialiv,8);
	__GLX_SINGLE_PUT_LONG(0,face);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetPixelMapfv,4);
	__GLX_SINGLE_PUT_LONG(0,map);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(values);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(values,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetPixelMapuiv,4);
	__GLX_SINGLE_PUT_LONG(0,map);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(values);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(values,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetPixelMapusv,4);
	__GLX_SINGLE_PUT_LONG(0,map);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_SHORT(values);
	} else {
	    __GLX_SINGLE_GET_SHORT_ARRAY(values,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexEnvfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexEnviv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexGendv,8);
	__GLX_SINGLE_PUT_LONG(0,coord);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_DOUBLE(params);
	} else {
	    __GLX_SINGLE_GET_DOUBLE_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexGenfv,8);
	__GLX_SINGLE_PUT_LONG(0,coord);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexGeniv,8);
	__GLX_SINGLE_PUT_LONG(0,coord);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexParameterfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexParameteriv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexLevelParameterfv,12);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,level);
	__GLX_SINGLE_PUT_LONG(8,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetTexLevelParameteriv,12);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,level);
	__GLX_SINGLE_PUT_LONG(8,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

GLboolean glIsList(GLuint list)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	GLboolean    retval = 0;
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_IsList,4);
	__GLX_SINGLE_PUT_LONG(0,list);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_RETVAL(retval, GLboolean);
	__GLX_SINGLE_END();
	return retval;
}

/*
 * Somewhere between GLX 1.2 and 1.3 (in SGI's code anyway) the
 * protocol for glAreTexturesResident, glDeleteTextures, glGenTextures,
 * and glIsTexture() was changed.  Before, calls to these functions
 * generated protocol for the old GL_EXT_texture_object versions of those
 * calls.  In the newer code, this is actually corrected; calls to the
 * 1.1 functions generate 1.1 protocol and calls to the EXT functions
 * generate EXT protocol.
 * Unfortunately, this correction causes an incompatibility.  Specifically,
 * an updated libGL.so will send protocol requests that the server won't
 * be able to handle.  For example, calling glGenTextures will generate a
 * BadRequest error.
 * For now, we'll keep generating EXT protocol from libGL.  We'll update
 * the server to understand both the 1.1 and EXT protocol ASAP.  At some point
 * in the future we'll correct libGL.so as well.  That should be a smoother
 * transition path.
 */

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
#if 0 /* see comments above */
	__GLX_SINGLE_DECLARE_VARIABLES();
	GLboolean    retval = 0;
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	if (n < 0) return retval;
	cmdlen = 4+n*4;
	__GLX_SINGLE_BEGIN(X_GLsop_AreTexturesResident,cmdlen);
	__GLX_SINGLE_PUT_LONG(0,n);
	__GLX_PUT_LONG_ARRAY(4,textures,n);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_RETVAL(retval, GLboolean);
	__GLX_SINGLE_GET_CHAR_ARRAY(residences,n);
	__GLX_SINGLE_END();
	return retval;
#else
        return glAreTexturesResidentEXT(n, textures, residences);
#endif
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
#if 0 /* see comments above */
	__GLX_SINGLE_DECLARE_VARIABLES();
	__GLX_SINGLE_LOAD_VARIABLES();
	if (n < 0) return;
	cmdlen = 4+n*4;
	__GLX_SINGLE_BEGIN(X_GLsop_DeleteTextures,cmdlen);
	__GLX_SINGLE_PUT_LONG(0,n);
	__GLX_PUT_LONG_ARRAY(4,textures,n);
	__GLX_SINGLE_END();
#else
        glDeleteTexturesEXT(n, textures);
#endif
}

void glGenTextures(GLsizei n, GLuint *textures)
{
#if 0 /* see comments above */
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GenTextures,4);
	__GLX_SINGLE_PUT_LONG(0,n);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_LONG_ARRAY(textures,n);
	__GLX_SINGLE_END();
#else
        glGenTexturesEXT(n, textures);
#endif
}

GLboolean glIsTexture(GLuint texture)
{
#if 0 /* see comments above */
	__GLX_SINGLE_DECLARE_VARIABLES();
	GLboolean    retval = 0;
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_IsTexture,4);
	__GLX_SINGLE_PUT_LONG(0,texture);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_RETVAL(retval, GLboolean);
	__GLX_SINGLE_END();
	return retval;
#else
        return glIsTextureEXT(texture);
#endif
}

void glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetColorTableParameterfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetColorTableParameteriv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetConvolutionParameterfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetConvolutionParameteriv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetHistogramParameterfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetHistogramParameteriv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMinmaxParameterfv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_FLOAT(params);
	} else {
	    __GLX_SINGLE_GET_FLOAT_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

void glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
	__GLX_SINGLE_DECLARE_VARIABLES();
	xGLXSingleReply reply;
	__GLX_SINGLE_LOAD_VARIABLES();
	__GLX_SINGLE_BEGIN(X_GLsop_GetMinmaxParameteriv,8);
	__GLX_SINGLE_PUT_LONG(0,target);
	__GLX_SINGLE_PUT_LONG(4,pname);
	__GLX_SINGLE_READ_XREPLY();
	__GLX_SINGLE_GET_SIZE(compsize);
	if (compsize == 1) {
	    __GLX_SINGLE_GET_LONG(params);
	} else {
	    __GLX_SINGLE_GET_LONG_ARRAY(params,compsize);
	}
	__GLX_SINGLE_END();
}

