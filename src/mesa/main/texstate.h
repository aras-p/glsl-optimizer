/* $Id: texstate.h,v 1.8 2001/06/18 17:26:08 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef TEXSTATE_H
#define TEXSTATE_H


#include "mtypes.h"


/*** Called from API ***/

extern void
_mesa_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params );

extern void
_mesa_GetTexEnviv( GLenum target, GLenum pname, GLint *params );

extern void
_mesa_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params );

extern void
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params );

extern void
_mesa_GetTexGeniv( GLenum coord, GLenum pname, GLint *params );

extern void
_mesa_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params );

extern void
_mesa_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params );

extern void
_mesa_GetTexParameterfv( GLenum target, GLenum pname, GLfloat *params );

extern void
_mesa_GetTexParameteriv( GLenum target, GLenum pname, GLint *params );


extern void
_mesa_TexEnvf( GLenum target, GLenum pname, GLfloat param );

extern void
_mesa_TexEnvfv( GLenum target, GLenum pname, const GLfloat *param );

extern void
_mesa_TexEnvi( GLenum target, GLenum pname, GLint param );

extern void
_mesa_TexEnviv( GLenum target, GLenum pname, const GLint *param );


extern void
_mesa_TexParameterfv( GLenum target, GLenum pname, const GLfloat *params );

extern void
_mesa_TexParameterf( GLenum target, GLenum pname, GLfloat param );


extern void
_mesa_TexParameteri( GLenum target, GLenum pname, GLint param );

extern void
_mesa_TexParameteriv( GLenum target, GLenum pname, const GLint *params );


extern void
_mesa_TexGend( GLenum coord, GLenum pname, GLdouble param );

extern void
_mesa_TexGendv( GLenum coord, GLenum pname, const GLdouble *params );

extern void
_mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param );

extern void
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params );

extern void
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param );

extern void
_mesa_TexGeniv( GLenum coord, GLenum pname, const GLint *params );




/*
 * GL_ARB_multitexture
 */
extern void
_mesa_ActiveTextureARB( GLenum target );

extern void
_mesa_ClientActiveTextureARB( GLenum target );


/*
 * Pixel Texture Extensions
 */

extern void
_mesa_PixelTexGenSGIX(GLenum mode);

extern void
_mesa_PixelTexGenParameterfSGIS(GLenum target, GLfloat value);

#ifdef VMS
#define _mesa_PixelTexGenParameterfvSGIS _mesa_PixelTexGenParameterfv
#endif
extern void
_mesa_PixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value);

extern void
_mesa_PixelTexGenParameteriSGIS(GLenum target, GLint value);

#ifdef VMS
#define _mesa_PixelTexGenParameterivSGIS _mesa_PixelTexGenParameteriv
#endif
extern void
_mesa_PixelTexGenParameterivSGIS(GLenum target, const GLint *value);

#ifdef VMS
#define _mesa_GetPixelTexGenParameterfvSGIS _mesa_GetPixelTexGenParameterfv
#endif
extern void
_mesa_GetPixelTexGenParameterfvSGIS(GLenum target, GLfloat *value);

#ifdef VMS
#define _mesa_GetPixelTexGenParameterivSGIS _mesa_GetPixelTexGenParameteriv
#endif
extern void
_mesa_GetPixelTexGenParameterivSGIS(GLenum target, GLint *value);


#endif
