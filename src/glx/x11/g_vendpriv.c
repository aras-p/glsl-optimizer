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

#include "packvendpriv.h"

GLboolean glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences)
{
	__GLX_VENDPRIV_DECLARE_VARIABLES();
	GLboolean    retval = 0;
	xGLXVendorPrivReply reply;
	__GLX_VENDPRIV_LOAD_VARIABLES();
	if (n < 0) return retval;
	cmdlen = 4+n*4;
	__GLX_VENDPRIV_BEGIN(X_GLXVendorPrivateWithReply,X_GLvop_AreTexturesResidentEXT,cmdlen);
	__GLX_VENDPRIV_PUT_LONG(0,n);
	__GLX_PUT_LONG_ARRAY(4,textures,n);
	__GLX_VENDPRIV_READ_XREPLY();
	__GLX_VENDPRIV_GET_RETVAL(retval, GLboolean);
	__GLX_VENDPRIV_GET_CHAR_ARRAY(residences,n);
	__GLX_VENDPRIV_END();
	return retval;
}

void glDeleteTexturesEXT(GLsizei n, const GLuint *textures)
{
	__GLX_VENDPRIV_DECLARE_VARIABLES();
	__GLX_VENDPRIV_LOAD_VARIABLES();
	if (n < 0) return;
	cmdlen = 4+n*4;
	__GLX_VENDPRIV_BEGIN(X_GLXVendorPrivate,X_GLvop_DeleteTexturesEXT,cmdlen);
	__GLX_VENDPRIV_PUT_LONG(0,n);
	__GLX_PUT_LONG_ARRAY(4,textures,n);
	__GLX_VENDPRIV_END();
}

void glGenTexturesEXT(GLsizei n, GLuint *textures)
{
	__GLX_VENDPRIV_DECLARE_VARIABLES();
	xGLXVendorPrivReply reply;
	__GLX_VENDPRIV_LOAD_VARIABLES();
	__GLX_VENDPRIV_BEGIN(X_GLXVendorPrivateWithReply,X_GLvop_GenTexturesEXT,4);
	__GLX_VENDPRIV_PUT_LONG(0,n);
	__GLX_VENDPRIV_READ_XREPLY();
	__GLX_VENDPRIV_GET_LONG_ARRAY(textures,n);
	__GLX_VENDPRIV_END();
}

GLboolean glIsTextureEXT(GLuint texture)
{
	__GLX_VENDPRIV_DECLARE_VARIABLES();
	GLboolean    retval = 0;
	xGLXVendorPrivReply reply;
	__GLX_VENDPRIV_LOAD_VARIABLES();
	__GLX_VENDPRIV_BEGIN(X_GLXVendorPrivateWithReply,X_GLvop_IsTextureEXT,4);
	__GLX_VENDPRIV_PUT_LONG(0,texture);
	__GLX_VENDPRIV_READ_XREPLY();
	__GLX_VENDPRIV_GET_RETVAL(retval, GLboolean);
	__GLX_VENDPRIV_END();
	return retval;
}

