/* $XFree86: xc/lib/GL/glx/size.h,v 1.4 2003/09/28 20:15:04 alanh Exp $ */
#ifndef _size_h_
#define _size_h_

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
** These are _size functions that are needed to pack the arguments
** into the protocol
*/
extern GLint __glBitmap_size(GLsizei w, GLsizei h);
extern GLint __glCallLists_size(GLsizei n, GLenum type);
extern GLint __glColorTableParameterfv_size(GLenum pname);
extern GLint __glColorTableParameteriv_size(GLenum pname);
extern GLint __glConvolutionParameterfv_size(GLenum pname);
extern GLint __glConvolutionParameteriv_size(GLenum pname);
extern GLint __glDrawPixels_size(GLenum format, GLenum type, GLsizei w,GLsizei h);
extern GLint __glFogfv_size(GLenum pname);
extern GLint __glFogiv_size(GLenum pname);
extern GLint __glLightModelfv_size(GLenum pname);
extern GLint __glLightModeliv_size(GLenum pname);
extern GLint __glLightfv_size(GLenum pname);
extern GLint __glLightiv_size(GLenum pname);
extern GLint __glMaterialfv_size(GLenum pname);
extern GLint __glMaterialiv_size(GLenum pname);
extern GLint __glTexEnvfv_size(GLenum e);
extern GLint __glTexEnviv_size(GLenum e);
extern GLint __glTexGendv_size(GLenum e);
extern GLint __glTexGenfv_size(GLenum e);
extern GLint __glTexGeniv_size(GLenum pname);
extern GLint __glTexImage1D_size(GLenum format, GLenum type, GLsizei w);
extern GLint __glTexImage2D_size(GLenum format, GLenum type, GLsizei w, GLsizei h);
extern GLint __glTexImage3D_size(GLenum format, GLenum type, GLsizei w, GLsizei h, GLsizei d);
extern GLint __glTexParameterfv_size(GLenum e);
extern GLint __glTexParameteriv_size(GLenum e);
extern GLint __glPointParameterfvARB_size(GLenum e);
extern GLint __glPointParameteriv_size(GLenum e);

#endif /* _size_h_ */
