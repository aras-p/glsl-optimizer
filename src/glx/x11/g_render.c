/* $XFree86: xc/lib/GL/glx/g_render.c,v 1.8 2004/01/28 18:11:38 alanh Exp $ */
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

#include "packrender.h"
#include "size.h"

#define GLdouble_SIZE   8
#define GLclampd_SIZE   8
#define GLfloat_SIZE    4
#define GLclampf_SIZE   4
#define GLint_SIZE      4
#define GLuint_SIZE     4
#define GLenum_SIZE     4
#define GLbitfield_SIZE 4
#define GLshort_SIZE    2
#define GLushort_SIZE   2
#define GLbyte_SIZE     1
#define GLubyte_SIZE    1
#define GLboolean_SIZE  1

#define __GLX_PUT_GLdouble(offset,value)   __GLX_PUT_DOUBLE(offset,value)
#define __GLX_PUT_GLclampd(offset,value)   __GLX_PUT_DOUBLE(offset,value)
#define __GLX_PUT_GLfloat(offset,value)    __GLX_PUT_FLOAT(offset,value)
#define __GLX_PUT_GLclampf(offset,value)   __GLX_PUT_FLOAT(offset,value)
#define __GLX_PUT_GLint(offset,value)      __GLX_PUT_LONG(offset,value)
#define __GLX_PUT_GLuint(offset,value)     __GLX_PUT_LONG(offset,value)
#define __GLX_PUT_GLenum(offset,value)     __GLX_PUT_LONG(offset,value)
#define __GLX_PUT_GLbitfield(offset,value) __GLX_PUT_LONG(offset,value)
#define __GLX_PUT_GLshort(offset,value)    __GLX_PUT_SHORT(offset,value)
#define __GLX_PUT_GLushort(offset,value)   __GLX_PUT_SHORT(offset,value)
#define __GLX_PUT_GLbyte(offset,value)     __GLX_PUT_CHAR(offset,value)
#define __GLX_PUT_GLubyte(offset,value)    __GLX_PUT_CHAR(offset,value)
#define __GLX_PUT_GLboolean(offset,value)  __GLX_PUT_CHAR(offset,value)

#define __GLX_PUT_GLdouble_ARRAY(offset,ptr,count)   __GLX_PUT_DOUBLE_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLclampd_ARRAY(offset,ptr,count)   __GLX_PUT_DOUBLE_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLfloat_ARRAY(offset,ptr,count)    __GLX_PUT_FLOAT_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLclampf_ARRAY(offset,ptr,count)   __GLX_PUT_FLOAT_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLint_ARRAY(offset,ptr,count)      __GLX_PUT_LONG_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLuint_ARRAY(offset,ptr,count)     __GLX_PUT_LONG_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLenum_ARRAY(offset,ptr,count)     __GLX_PUT_LONG_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLshort_ARRAY(offset,ptr,count)    __GLX_PUT_SHORT_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLushort_ARRAY(offset,ptr,count)   __GLX_PUT_SHORT_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLbyte_ARRAY(offset,ptr,count)     __GLX_PUT_CHAR_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLubyte_ARRAY(offset,ptr,count)    __GLX_PUT_CHAR_ARRAY(offset,ptr,count)
#define __GLX_PUT_GLboolean_ARRAY(offset,ptr,count)  __GLX_PUT_CHAR_ARRAY(offset,ptr,count)

#define RENDER_SIZE(t,c)  (__GLX_PAD(4 + (t ## _SIZE * c)))

/* GLX protocol templates are named in the following manner.  All templates
 * begin with the string 'glxproto_'.  Following is an optional list of
 * scalar parameters.  The scalars are listed as type and number.  The most
 * common being \c enum1 (one scalar enum) and \c enum2 (two scalar enums).
 *
 * The final part of the name describes the number of named-type parameters
 * and how they are passed.
 * - One or more digits followed by the letter s means
 *   that the specified number of parameters are passed as scalars.  The macro
 *   \c glxproto_3s generates a function that takes 3 scalars, such as
 *   \c glVertex3f.
 * - A capital C follwed by a lower-case v means that a constant
 *   sized vector is passed.  Macros of this type take an extra parameter,
 *   which is the size of the vector.  The invocation
 *   'glxproto_Cv(Vertex3fv, X_GLrop_Vertexfv, GLfloat, 3)' would generate the
 *   correct protocol for the \c glVertex3fv function.
 * - A capital V followed by a lower-case v means that a variable sized
 *   vector is passed.  The function generated by these macros will call
 *   a co-function to determine the size of the vector.  The name of the
 *   co-function is generated by prepending \c __gl and appending \c _size
 *   to the base name of the function.  The invocation
 *   'glxproto_enum1_Vv(Fogiv, X_GLrop_Fogiv, GLint)' would generate the
 *   correct protocol for the \c glFogiv function.
 * - One or more digits without a following letter means that a function
 *   taking the specified number of scalar parameters and a function with a
 *   vector parameter of the specified size should be generated.  The letter
 *   v is automatically appended to the name of the vector-based function in
 *   this case.  The invocation
 *   'glxproto_3(Vertex3f, X_GLrop_Vertex3fv, GLfloat)' would generate the
 *   correct protocol for both \c glVertex3f and \c glVertex3fv.
 *
 * glxproto_void is a special case for functions that take no parameters
 * (i.e., glEnd).
 *
 * An addition form is 'glxvendr_'.  This is identical to the other forms
 * with the exception of taking an additional parameter (to the macro) which
 * is a vendor string to append to the function name.  The invocation
 * 'glxproto_3(Foo3f, X_GLrop_Foo3fv, GLfloat)' would generate the functions
 * 'glFoo3fv' and 'glFoo3f', and the invocation
 * 'glxvendr_3(Foo3f, X_GLrop_Foo3fv, GLfloat, EXT)' would generate the
 * functions 'glFoo3fvEXT' and 'glFoo3fEXT'.
 */

#define glxproto_void(name, rop) \
   void __indirect_gl ## name (void) \
   { \
	__GLX_DECLARE_VARIABLES(); \
	__GLX_LOAD_VARIABLES(); \
	__GLX_BEGIN(rop, 4); \
	__GLX_END(4); \
   }

#define glxproto_Cv(name, rop, type, count) \
   void __indirect_gl ## name (const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, count); \
      __GLX_BEGIN(rop, cmdlen); \
      if (count <= 4) { \
	                  __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v[0]); \
	 if (count > 1) { __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v[1]); } \
	 if (count > 2) { __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v[2]); } \
	 if (count > 3) { __GLX_PUT_ ## type (4 + (3 * type ## _SIZE), v[3]); } \
      } else { \
	 __GLX_PUT_ ## type ## _ARRAY(4, v, count); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_Cv_transpose(name, rop, type, w) \
   void __indirect_gl ## name (const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      type t[ w * w ]; unsigned i, j; \
      for (i = 0; i < w; i++) { \
	 for (j = 0; j < w; j++) { \
	    t[i*w+j] = v[j*w+i]; \
	 } \
      } \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, (w * w)); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type ## _ARRAY(4, t, (w * w)); \
      __GLX_END(cmdlen); \
   }

#define glxproto_1s(name, rop, type) \
   void __indirect_gl ## name (type v1) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, 1); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
      __GLX_END(cmdlen); \
   }

#define glxproto_2s(name, rop, type) \
   void __indirect_gl ## name (type v1, type v2) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, 2); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
      __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
      __GLX_END(cmdlen); \
   }

#define glxproto_3s(name, rop, type) \
   void __indirect_gl ## name (type v1, type v2, type v3) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, 3); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
      __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
      __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v3); \
      __GLX_END(cmdlen); \
   }

#define glxproto_4s(name, rop, type) \
   void __indirect_gl ## name (type v1, type v2, type v3, type v4) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, 4); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
      __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
      __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v3); \
      __GLX_PUT_ ## type (4 + (3 * type ## _SIZE), v4); \
      __GLX_END(cmdlen); \
   }

#define glxproto_6s(name, rop, type) \
   void __indirect_gl ## name (type v1, type v2, type v3, type v4, type v5, type v6) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = RENDER_SIZE(type, 6); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
      __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
      __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v3); \
      __GLX_PUT_ ## type (4 + (3 * type ## _SIZE), v4); \
      __GLX_PUT_ ## type (4 + (4 * type ## _SIZE), v5); \
      __GLX_PUT_ ## type (4 + (5 * type ## _SIZE), v6); \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_1s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 1); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_LONG     (4 + (1 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v1); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_1v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 1); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_LONG     (4 + (1 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v[0]); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_2s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 2); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
	 __GLX_PUT_LONG     (4 + (2 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v2); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_2v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 2); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v[1]); \
	 __GLX_PUT_LONG     (4 + (2 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v[1]); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_3s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2, type v3) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 3); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
	 __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v3); \
	 __GLX_PUT_LONG     (4 + (3 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v2); \
	 __GLX_PUT_ ## type (8 + (2 * type ## _SIZE), v3); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_3v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 3); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v[1]); \
	 __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v[2]); \
	 __GLX_PUT_LONG     (4 + (3 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v[1]); \
	 __GLX_PUT_ ## type (8 + (2 * type ## _SIZE), v[2]); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_4s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2, type v3, type v4) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 4); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v2); \
	 __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v3); \
	 __GLX_PUT_ ## type (4 + (3 * type ## _SIZE), v4); \
	 __GLX_PUT_LONG     (4 + (4 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v2); \
	 __GLX_PUT_ ## type (8 + (2 * type ## _SIZE), v3); \
	 __GLX_PUT_ ## type (8 + (3 * type ## _SIZE), v4); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_4v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 4 + RENDER_SIZE(type, 4); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (4 + (1 * type ## _SIZE), v[1]); \
	 __GLX_PUT_ ## type (4 + (2 * type ## _SIZE), v[2]); \
	 __GLX_PUT_ ## type (4 + (3 * type ## _SIZE), v[3]); \
	 __GLX_PUT_LONG     (4 + (4 * type ## _SIZE), e); \
      } else { \
	 __GLX_PUT_LONG(4, e); \
	 __GLX_PUT_ ## type (8 + (0 * type ## _SIZE), v[0]); \
	 __GLX_PUT_ ## type (8 + (1 * type ## _SIZE), v[1]); \
	 __GLX_PUT_ ## type (8 + (2 * type ## _SIZE), v[2]); \
	 __GLX_PUT_ ## type (8 + (3 * type ## _SIZE), v[3]); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum1_Vv(name, rop, type) \
   void __indirect_gl ## name (GLenum pname, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      compsize = __gl ## name ## _size(pname); \
      cmdlen = 4 + RENDER_SIZE(type, compsize); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_LONG(4, pname); \
      __GLX_PUT_ ## type ## _ARRAY(8, v, compsize); \
      __GLX_END(cmdlen); \
   }
    
#define glxproto_enum2_1s(name, rop, type) \
   void __indirect_gl ## name (GLenum target, GLenum pname, type v1) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      cmdlen = 8 + RENDER_SIZE(type, 1); \
      __GLX_BEGIN(rop, cmdlen); \
      if (type ## _SIZE == 8) { \
	 __GLX_PUT_ ## type (4 + (0 * type ## _SIZE), v1); \
	 __GLX_PUT_LONG     (4 + (1 * type ## _SIZE), target); \
	 __GLX_PUT_LONG     (8 + (1 * type ## _SIZE), pname); \
      } else { \
	 __GLX_PUT_LONG(4, target); \
	 __GLX_PUT_LONG(8, pname); \
	 __GLX_PUT_ ## type (12 + (0 * type ## _SIZE), v1); \
      } \
      __GLX_END(cmdlen); \
   }

#define glxproto_enum2_Vv(name, rop, type) \
   void __indirect_gl ## name (GLenum target, GLenum pname, const type * v) \
   { \
      __GLX_DECLARE_VARIABLES(); \
      __GLX_LOAD_VARIABLES(); \
      compsize = __gl ## name ## _size(pname); \
      cmdlen =  8 + RENDER_SIZE(type, compsize); \
      __GLX_BEGIN(rop, cmdlen); \
      __GLX_PUT_LONG(4, target); \
      __GLX_PUT_LONG(8, pname); \
      __GLX_PUT_ ## type ## _ARRAY(12, v, compsize); \
      __GLX_END(cmdlen); \
   }

#define GENERATE_GLX_PROTOCOL_FUNCTIONS
#include "indirect.h"

void __indirect_glRectdv(const GLdouble *v1, const GLdouble *v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Rectdv,36);
	__GLX_PUT_DOUBLE(4,v1[0]);
	__GLX_PUT_DOUBLE(12,v1[1]);
	__GLX_PUT_DOUBLE(20,v2[0]);
	__GLX_PUT_DOUBLE(28,v2[1]);
	__GLX_END(36);
}

void __indirect_glRectfv(const GLfloat *v1, const GLfloat *v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Rectfv,20);
	__GLX_PUT_FLOAT(4,v1[0]);
	__GLX_PUT_FLOAT(8,v1[1]);
	__GLX_PUT_FLOAT(12,v2[0]);
	__GLX_PUT_FLOAT(16,v2[1]);
	__GLX_END(20);
}

void __indirect_glRectiv(const GLint *v1, const GLint *v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Rectiv,20);
	__GLX_PUT_LONG(4,v1[0]);
	__GLX_PUT_LONG(8,v1[1]);
	__GLX_PUT_LONG(12,v2[0]);
	__GLX_PUT_LONG(16,v2[1]);
	__GLX_END(20);
}

void __indirect_glRectsv(const GLshort *v1, const GLshort *v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Rectsv,12);
	__GLX_PUT_SHORT(4,v1[0]);
	__GLX_PUT_SHORT(6,v1[1]);
	__GLX_PUT_SHORT(8,v2[0]);
	__GLX_PUT_SHORT(10,v2[1]);
	__GLX_END(12);
}

void __indirect_glLineStipple(GLint factor, GLushort pattern)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_LineStipple,12);
	__GLX_PUT_LONG(4,factor);
	__GLX_PUT_SHORT(8,pattern);
	__GLX_END(12);
}

void __indirect_glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Scissor,20);
	__GLX_PUT_LONG(4,x);
	__GLX_PUT_LONG(8,y);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_END(20);
}

void __indirect_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_MapGrid1d,24);
	__GLX_PUT_DOUBLE(4,u1);
	__GLX_PUT_DOUBLE(12,u2);
	__GLX_PUT_LONG(20,un);
	__GLX_END(24);
}

void __indirect_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_MapGrid1f,16);
	__GLX_PUT_LONG(4,un);
	__GLX_PUT_FLOAT(8,u1);
	__GLX_PUT_FLOAT(12,u2);
	__GLX_END(16);
}

void __indirect_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_MapGrid2d,44);
	__GLX_PUT_DOUBLE(4,u1);
	__GLX_PUT_DOUBLE(12,u2);
	__GLX_PUT_DOUBLE(20,v1);
	__GLX_PUT_DOUBLE(28,v2);
	__GLX_PUT_LONG(36,un);
	__GLX_PUT_LONG(40,vn);
	__GLX_END(44);
}

void __indirect_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_MapGrid2f,28);
	__GLX_PUT_LONG(4,un);
	__GLX_PUT_FLOAT(8,u1);
	__GLX_PUT_FLOAT(12,u2);
	__GLX_PUT_LONG(16,vn);
	__GLX_PUT_FLOAT(20,v1);
	__GLX_PUT_FLOAT(24,v2);
	__GLX_END(28);
}

void __indirect_glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_StencilFunc,16);
	__GLX_PUT_LONG(4,func);
	__GLX_PUT_LONG(8,ref);
	__GLX_PUT_LONG(12,mask);
	__GLX_END(16);
}

void __indirect_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyPixels,24);
	__GLX_PUT_LONG(4,x);
	__GLX_PUT_LONG(8,y);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_PUT_LONG(20,type);
	__GLX_END(24);
}

void __indirect_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Viewport,20);
	__GLX_PUT_LONG(4,x);
	__GLX_PUT_LONG(8,y);
	__GLX_PUT_LONG(12,width);
	__GLX_PUT_LONG(16,height);
	__GLX_END(20);
}

void __indirect_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyTexImage1D,32);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,level);
	__GLX_PUT_LONG(12,internalformat);
	__GLX_PUT_LONG(16,x);
	__GLX_PUT_LONG(20,y);
	__GLX_PUT_LONG(24,width);
	__GLX_PUT_LONG(28,border);
	__GLX_END(32);
}

void __indirect_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyTexImage2D,36);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,level);
	__GLX_PUT_LONG(12,internalformat);
	__GLX_PUT_LONG(16,x);
	__GLX_PUT_LONG(20,y);
	__GLX_PUT_LONG(24,width);
	__GLX_PUT_LONG(28,height);
	__GLX_PUT_LONG(32,border);
	__GLX_END(36);
}

void __indirect_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyTexSubImage1D,28);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,level);
	__GLX_PUT_LONG(12,xoffset);
	__GLX_PUT_LONG(16,x);
	__GLX_PUT_LONG(20,y);
	__GLX_PUT_LONG(24,width);
	__GLX_END(28);
}

void __indirect_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyTexSubImage2D,36);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,level);
	__GLX_PUT_LONG(12,xoffset);
	__GLX_PUT_LONG(16,yoffset);
	__GLX_PUT_LONG(20,x);
	__GLX_PUT_LONG(24,y);
	__GLX_PUT_LONG(28,width);
	__GLX_PUT_LONG(32,height);
	__GLX_END(36);
}

void __indirect_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	if (n < 0) return;
	cmdlen = 8+n*4+n*4;
	__GLX_BEGIN(X_GLrop_PrioritizeTextures,cmdlen);
	__GLX_PUT_LONG(4,n);
	__GLX_PUT_LONG_ARRAY(8,textures,n);
	__GLX_PUT_FLOAT_ARRAY(8+n*4,priorities,n);
	__GLX_END(cmdlen);
}

void __indirect_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyColorTable,24);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,internalformat);
	__GLX_PUT_LONG(12,x);
	__GLX_PUT_LONG(16,y);
	__GLX_PUT_LONG(20,width);
	__GLX_END(24);
}

void __indirect_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyColorSubTable,24);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,start);
	__GLX_PUT_LONG(12,x);
	__GLX_PUT_LONG(16,y);
	__GLX_PUT_LONG(20,width);
	__GLX_END(24);
}

void __indirect_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyConvolutionFilter1D,24);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,internalformat);
	__GLX_PUT_LONG(12,x);
	__GLX_PUT_LONG(16,y);
	__GLX_PUT_LONG(20,width);
	__GLX_END(24);
}

void __indirect_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyConvolutionFilter2D,28);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,internalformat);
	__GLX_PUT_LONG(12,x);
	__GLX_PUT_LONG(16,y);
	__GLX_PUT_LONG(20,width);
	__GLX_PUT_LONG(24,height);
	__GLX_END(28);
}

void __indirect_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_Histogram,20);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,width);
	__GLX_PUT_LONG(12,internalformat);
	__GLX_PUT_CHAR(16,sink);
	__GLX_END(20);
}

void __indirect_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GLX_DECLARE_VARIABLES();
	__GLX_LOAD_VARIABLES();
	__GLX_BEGIN(X_GLrop_CopyTexSubImage3D,40);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_LONG(8,level);
	__GLX_PUT_LONG(12,xoffset);
	__GLX_PUT_LONG(16,yoffset);
	__GLX_PUT_LONG(20,zoffset);
	__GLX_PUT_LONG(24,x);
	__GLX_PUT_LONG(28,y);
	__GLX_PUT_LONG(32,width);
	__GLX_PUT_LONG(36,height);
	__GLX_END(40);
}

void __indirect_glWindowPos2dARB(GLdouble x, GLdouble y)
{
	__indirect_glWindowPos3fARB(x, y, 0.0);
}

void __indirect_glWindowPos2iARB(GLint x, GLint y)
{
	__indirect_glWindowPos3fARB(x, y, 0.0);
}

void __indirect_glWindowPos2fARB(GLfloat x, GLfloat y)
{
	__indirect_glWindowPos3fARB(x, y, 0.0);
}

void __indirect_glWindowPos2sARB(GLshort x, GLshort y)
{
	__indirect_glWindowPos3fARB(x, y, 0.0);
}

void __indirect_glWindowPos2dvARB(const GLdouble * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], 0.0);
}

void __indirect_glWindowPos2fvARB(const GLfloat * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], 0.0);
}

void __indirect_glWindowPos2ivARB(const GLint * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], 0.0);
}

void __indirect_glWindowPos2svARB(const GLshort * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], 0.0);
}

void __indirect_glWindowPos3dARB(GLdouble x, GLdouble y, GLdouble z)
{
	__indirect_glWindowPos3fARB(x, y, z);
}

void __indirect_glWindowPos3iARB(GLint x, GLint y, GLint z)
{
	__indirect_glWindowPos3fARB(x, y, z);
}

void __indirect_glWindowPos3sARB(GLshort x, GLshort y, GLshort z)
{
	__indirect_glWindowPos3fARB(x, y, z);
}

void __indirect_glWindowPos3dvARB(const GLdouble * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], p[2]);
}

void __indirect_glWindowPos3ivARB(const GLint * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], p[2]);
}

void __indirect_glWindowPos3svARB(const GLshort * p)
{
	__indirect_glWindowPos3fARB(p[0], p[1], p[2]);
}
