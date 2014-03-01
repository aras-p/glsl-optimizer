/**
 * \file api_loopback.c
 *
 * \author Keith Whitwell <keithw@vmware.com>
 */

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "macros.h"
#include "api_loopback.h"
#include "mtypes.h"
#include "glapi/glapi.h"
#include "main/dispatch.h"
#include "main/context.h"

/* KW: A set of functions to convert unusual Color/Normal/Vertex/etc
 * calls to a smaller set of driver-provided formats.  Currently just
 * go back to dispatch to find these (eg. call glNormal3f directly),
 * hence 'loopback'.
 *
 * The driver must supply all of the remaining entry points, which are
 * listed in dd.h.  The easiest way for a driver to do this is to
 * install the supplied software t&l module.
 */
#define COLORF(r,g,b,a)             CALL_Color4f(GET_DISPATCH(), (r,g,b,a))
#define VERTEX2(x,y)	            CALL_Vertex2f(GET_DISPATCH(), (x,y))
#define VERTEX3(x,y,z)	            CALL_Vertex3f(GET_DISPATCH(), (x,y,z))
#define VERTEX4(x,y,z,w)            CALL_Vertex4f(GET_DISPATCH(), (x,y,z,w))
#define NORMAL(x,y,z)               CALL_Normal3f(GET_DISPATCH(), (x,y,z))
#define TEXCOORD1(s)                CALL_TexCoord1f(GET_DISPATCH(), (s))
#define TEXCOORD2(s,t)              CALL_TexCoord2f(GET_DISPATCH(), (s,t))
#define TEXCOORD3(s,t,u)            CALL_TexCoord3f(GET_DISPATCH(), (s,t,u))
#define TEXCOORD4(s,t,u,v)          CALL_TexCoord4f(GET_DISPATCH(), (s,t,u,v))
#define INDEX(c)		    CALL_Indexf(GET_DISPATCH(), (c))
#define MULTI_TEXCOORD1(z,s)	    CALL_MultiTexCoord1fARB(GET_DISPATCH(), (z,s))
#define MULTI_TEXCOORD2(z,s,t)	    CALL_MultiTexCoord2fARB(GET_DISPATCH(), (z,s,t))
#define MULTI_TEXCOORD3(z,s,t,u)    CALL_MultiTexCoord3fARB(GET_DISPATCH(), (z,s,t,u))
#define MULTI_TEXCOORD4(z,s,t,u,v)  CALL_MultiTexCoord4fARB(GET_DISPATCH(), (z,s,t,u,v))
#define EVALCOORD1(x)               CALL_EvalCoord1f(GET_DISPATCH(), (x))
#define EVALCOORD2(x,y)             CALL_EvalCoord2f(GET_DISPATCH(), (x,y))
#define MATERIALFV(a,b,c)           CALL_Materialfv(GET_DISPATCH(), (a,b,c))
#define RECTF(a,b,c,d)              CALL_Rectf(GET_DISPATCH(), (a,b,c,d))

#define FOGCOORDF(x)                CALL_FogCoordfEXT(GET_DISPATCH(), (x))
#define SECONDARYCOLORF(a,b,c)      CALL_SecondaryColor3fEXT(GET_DISPATCH(), (a,b,c))

#define ATTRIB1NV(index,x)          CALL_VertexAttrib1fNV(GET_DISPATCH(), (index,x))
#define ATTRIB2NV(index,x,y)        CALL_VertexAttrib2fNV(GET_DISPATCH(), (index,x,y))
#define ATTRIB3NV(index,x,y,z)      CALL_VertexAttrib3fNV(GET_DISPATCH(), (index,x,y,z))
#define ATTRIB4NV(index,x,y,z,w)    CALL_VertexAttrib4fNV(GET_DISPATCH(), (index,x,y,z,w))

#define ATTRIB1ARB(index,x)         CALL_VertexAttrib1fARB(GET_DISPATCH(), (index,x))
#define ATTRIB2ARB(index,x,y)       CALL_VertexAttrib2fARB(GET_DISPATCH(), (index,x,y))
#define ATTRIB3ARB(index,x,y,z)     CALL_VertexAttrib3fARB(GET_DISPATCH(), (index,x,y,z))
#define ATTRIB4ARB(index,x,y,z,w)   CALL_VertexAttrib4fARB(GET_DISPATCH(), (index,x,y,z,w))

#define ATTRIBI_1I(index,x)   CALL_VertexAttribI1iEXT(GET_DISPATCH(), (index,x))
#define ATTRIBI_1UI(index,x)   CALL_VertexAttribI1uiEXT(GET_DISPATCH(), (index,x))
#define ATTRIBI_4I(index,x,y,z,w)   CALL_VertexAttribI4iEXT(GET_DISPATCH(), (index,x,y,z,w))

#define ATTRIBI_4UI(index,x,y,z,w)   CALL_VertexAttribI4uiEXT(GET_DISPATCH(), (index,x,y,z,w))


void GLAPIENTRY
_mesa_Color3b( GLbyte red, GLbyte green, GLbyte blue )
{
   COLORF( BYTE_TO_FLOAT(red),
	   BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue),
	   1.0 );
}

void GLAPIENTRY
_mesa_Color3d( GLdouble red, GLdouble green, GLdouble blue )
{
   COLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue, 1.0 );
}

void GLAPIENTRY
_mesa_Color3i( GLint red, GLint green, GLint blue )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), 1.0);
}

void GLAPIENTRY
_mesa_Color3s( GLshort red, GLshort green, GLshort blue )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), 1.0);
}

void GLAPIENTRY
_mesa_Color3ui( GLuint red, GLuint green, GLuint blue )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), 1.0 );
}

void GLAPIENTRY
_mesa_Color3us( GLushort red, GLushort green, GLushort blue )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), 1.0 );
}

void GLAPIENTRY
_mesa_Color3ub( GLubyte red, GLubyte green, GLubyte blue )
{
   COLORF( UBYTE_TO_FLOAT(red), UBYTE_TO_FLOAT(green),
	   UBYTE_TO_FLOAT(blue), 1.0 );
}


void GLAPIENTRY
_mesa_Color3bv( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), 1.0 );
}

void GLAPIENTRY
_mesa_Color3dv( const GLdouble *v )
{
   COLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0 );
}

void GLAPIENTRY
_mesa_Color3iv( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), 1.0 );
}

void GLAPIENTRY
_mesa_Color3sv( const GLshort *v )
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), 1.0 );
}

void GLAPIENTRY
_mesa_Color3uiv( const GLuint *v )
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), 1.0 );
}

void GLAPIENTRY
_mesa_Color3usv( const GLushort *v )
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), 1.0 );
}

void GLAPIENTRY
_mesa_Color3ubv( const GLubyte *v )
{
   COLORF( UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
	   UBYTE_TO_FLOAT(v[2]), 1.0 );
}


void GLAPIENTRY
_mesa_Color4b( GLbyte red, GLbyte green, GLbyte blue,
			      GLbyte alpha )
{
   COLORF( BYTE_TO_FLOAT(red), BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue), BYTE_TO_FLOAT(alpha) );
}

void GLAPIENTRY
_mesa_Color4d( GLdouble red, GLdouble green, GLdouble blue,
			      GLdouble alpha )
{
   COLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue, (GLfloat) alpha );
}

void GLAPIENTRY
_mesa_Color4i( GLint red, GLint green, GLint blue, GLint alpha )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), INT_TO_FLOAT(alpha) );
}

void GLAPIENTRY
_mesa_Color4s( GLshort red, GLshort green, GLshort blue,
			      GLshort alpha )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), SHORT_TO_FLOAT(alpha) );
}

void GLAPIENTRY
_mesa_Color4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), UINT_TO_FLOAT(alpha) );
}

void GLAPIENTRY
_mesa_Color4us( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), USHORT_TO_FLOAT(alpha) );
}

void GLAPIENTRY
_mesa_Color4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
   COLORF( UBYTE_TO_FLOAT(red), UBYTE_TO_FLOAT(green),
	   UBYTE_TO_FLOAT(blue), UBYTE_TO_FLOAT(alpha) );
}


void GLAPIENTRY
_mesa_Color4iv( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}


void GLAPIENTRY
_mesa_Color4bv( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]) );
}

void GLAPIENTRY
_mesa_Color4dv( const GLdouble *v )
{
   COLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY
_mesa_Color4sv( const GLshort *v)
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]) );
}


void GLAPIENTRY
_mesa_Color4uiv( const GLuint *v)
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]) );
}

void GLAPIENTRY
_mesa_Color4usv( const GLushort *v)
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]) );
}

void GLAPIENTRY
_mesa_Color4ubv( const GLubyte *v)
{
   COLORF( UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
	   UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]) );
}


void GLAPIENTRY
_mesa_FogCoordd( GLdouble d )
{
   FOGCOORDF( (GLfloat) d );
}

void GLAPIENTRY
_mesa_FogCoorddv( const GLdouble *v )
{
   FOGCOORDF( (GLfloat) *v );
}


void GLAPIENTRY
_mesa_Indexd( GLdouble c )
{
   INDEX( (GLfloat) c );
}

void GLAPIENTRY
_mesa_Indexi( GLint c )
{
   INDEX( (GLfloat) c );
}

void GLAPIENTRY
_mesa_Indexs( GLshort c )
{
   INDEX( (GLfloat) c );
}

void GLAPIENTRY
_mesa_Indexub( GLubyte c )
{
   INDEX( (GLfloat) c );
}

void GLAPIENTRY
_mesa_Indexdv( const GLdouble *c )
{
   INDEX( (GLfloat) *c );
}

void GLAPIENTRY
_mesa_Indexiv( const GLint *c )
{
   INDEX( (GLfloat) *c );
}

void GLAPIENTRY
_mesa_Indexsv( const GLshort *c )
{
   INDEX( (GLfloat) *c );
}

void GLAPIENTRY
_mesa_Indexubv( const GLubyte *c )
{
   INDEX( (GLfloat) *c );
}


void GLAPIENTRY
_mesa_EdgeFlagv(const GLboolean *flag)
{
   CALL_EdgeFlag(GET_DISPATCH(), (*flag));
}


void GLAPIENTRY
_mesa_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz )
{
   NORMAL( BYTE_TO_FLOAT(nx), BYTE_TO_FLOAT(ny), BYTE_TO_FLOAT(nz) );
}

void GLAPIENTRY
_mesa_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz )
{
   NORMAL((GLfloat) nx, (GLfloat) ny, (GLfloat) nz);
}

void GLAPIENTRY
_mesa_Normal3i( GLint nx, GLint ny, GLint nz )
{
   NORMAL( INT_TO_FLOAT(nx), INT_TO_FLOAT(ny), INT_TO_FLOAT(nz) );
}

void GLAPIENTRY
_mesa_Normal3s( GLshort nx, GLshort ny, GLshort nz )
{
   NORMAL( SHORT_TO_FLOAT(nx), SHORT_TO_FLOAT(ny), SHORT_TO_FLOAT(nz) );
}

void GLAPIENTRY
_mesa_Normal3bv( const GLbyte *v )
{
   NORMAL( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]), BYTE_TO_FLOAT(v[2]) );
}

void GLAPIENTRY
_mesa_Normal3dv( const GLdouble *v )
{
   NORMAL( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_Normal3iv( const GLint *v )
{
   NORMAL( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]), INT_TO_FLOAT(v[2]) );
}

void GLAPIENTRY
_mesa_Normal3sv( const GLshort *v )
{
   NORMAL( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]), SHORT_TO_FLOAT(v[2]) );
}

void GLAPIENTRY
_mesa_TexCoord1d( GLdouble s )
{
   TEXCOORD1((GLfloat) s);
}

void GLAPIENTRY
_mesa_TexCoord1i( GLint s )
{
   TEXCOORD1((GLfloat) s);
}

void GLAPIENTRY
_mesa_TexCoord1s( GLshort s )
{
   TEXCOORD1((GLfloat) s);
}

void GLAPIENTRY
_mesa_TexCoord2d( GLdouble s, GLdouble t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

void GLAPIENTRY
_mesa_TexCoord2s( GLshort s, GLshort t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

void GLAPIENTRY
_mesa_TexCoord2i( GLint s, GLint t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

void GLAPIENTRY
_mesa_TexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

void GLAPIENTRY
_mesa_TexCoord3i( GLint s, GLint t, GLint r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

void GLAPIENTRY
_mesa_TexCoord3s( GLshort s, GLshort t, GLshort r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

void GLAPIENTRY
_mesa_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

void GLAPIENTRY
_mesa_TexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

void GLAPIENTRY
_mesa_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

void GLAPIENTRY
_mesa_TexCoord1dv( const GLdouble *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_TexCoord1iv( const GLint *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_TexCoord1sv( const GLshort *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_TexCoord2dv( const GLdouble *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_TexCoord2iv( const GLint *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_TexCoord2sv( const GLshort *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_TexCoord3dv( const GLdouble *v )
{
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_TexCoord3iv( const GLint *v )
{
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_TexCoord3sv( const GLshort *v )
{
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_TexCoord4dv( const GLdouble *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_TexCoord4iv( const GLint *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_TexCoord4sv( const GLshort *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_Vertex2d( GLdouble x, GLdouble y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

void GLAPIENTRY
_mesa_Vertex2i( GLint x, GLint y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

void GLAPIENTRY
_mesa_Vertex2s( GLshort x, GLshort y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

void GLAPIENTRY
_mesa_Vertex3d( GLdouble x, GLdouble y, GLdouble z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

void GLAPIENTRY
_mesa_Vertex3i( GLint x, GLint y, GLint z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

void GLAPIENTRY
_mesa_Vertex3s( GLshort x, GLshort y, GLshort z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

void GLAPIENTRY
_mesa_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

void GLAPIENTRY
_mesa_Vertex4i( GLint x, GLint y, GLint z, GLint w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

void GLAPIENTRY
_mesa_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

void GLAPIENTRY
_mesa_Vertex2dv( const GLdouble *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_Vertex2iv( const GLint *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_Vertex2sv( const GLshort *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_Vertex3dv( const GLdouble *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_Vertex3iv( const GLint *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_Vertex3sv( const GLshort *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_Vertex4dv( const GLdouble *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_Vertex4iv( const GLint *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_Vertex4sv( const GLshort *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_MultiTexCoord1d(GLenum target, GLdouble s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

void GLAPIENTRY
_mesa_MultiTexCoord1dv(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

void GLAPIENTRY
_mesa_MultiTexCoord1i(GLenum target, GLint s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

void GLAPIENTRY
_mesa_MultiTexCoord1iv(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

void GLAPIENTRY
_mesa_MultiTexCoord1s(GLenum target, GLshort s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

void GLAPIENTRY
_mesa_MultiTexCoord1sv(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

void GLAPIENTRY
_mesa_MultiTexCoord2d(GLenum target, GLdouble s, GLdouble t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

void GLAPIENTRY
_mesa_MultiTexCoord2dv(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_MultiTexCoord2i(GLenum target, GLint s, GLint t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

void GLAPIENTRY
_mesa_MultiTexCoord2iv(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_MultiTexCoord2s(GLenum target, GLshort s, GLshort t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

void GLAPIENTRY
_mesa_MultiTexCoord2sv(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

void GLAPIENTRY
_mesa_MultiTexCoord3d(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

void GLAPIENTRY
_mesa_MultiTexCoord3dv(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_MultiTexCoord3i(GLenum target, GLint s, GLint t, GLint r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

void GLAPIENTRY
_mesa_MultiTexCoord3iv(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_MultiTexCoord3s(GLenum target, GLshort s, GLshort t, GLshort r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

void GLAPIENTRY
_mesa_MultiTexCoord3sv(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

void GLAPIENTRY
_mesa_MultiTexCoord4d(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t, 
		    (GLfloat) r, (GLfloat) q );
}

void GLAPIENTRY
_mesa_MultiTexCoord4dv(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1], 
		    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_MultiTexCoord4i(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t,
		    (GLfloat) r, (GLfloat) q );
}

void GLAPIENTRY
_mesa_MultiTexCoord4iv(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_MultiTexCoord4s(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t,
		    (GLfloat) r, (GLfloat) q );
}

void GLAPIENTRY
_mesa_MultiTexCoord4sv(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3] );
}

void GLAPIENTRY
_mesa_EvalCoord2dv( const GLdouble *u )
{
   EVALCOORD2( (GLfloat) u[0], (GLfloat) u[1] );
}

void GLAPIENTRY
_mesa_EvalCoord2fv( const GLfloat *u )
{
   EVALCOORD2( u[0], u[1] );
}

void GLAPIENTRY
_mesa_EvalCoord2d( GLdouble u, GLdouble v )
{
   EVALCOORD2( (GLfloat) u, (GLfloat) v );
}

void GLAPIENTRY
_mesa_EvalCoord1dv( const GLdouble *u )
{
   EVALCOORD1( (GLfloat) *u );
}

void GLAPIENTRY
_mesa_EvalCoord1fv( const GLfloat *u )
{
   EVALCOORD1( (GLfloat) *u );
}

void GLAPIENTRY
_mesa_EvalCoord1d( GLdouble u )
{
   EVALCOORD1( (GLfloat) u );
}

void GLAPIENTRY
_mesa_Materialf( GLenum face, GLenum pname, GLfloat param )
{
   GLfloat fparam[4];
   fparam[0] = param;
   MATERIALFV( face, pname, fparam );
}

void GLAPIENTRY
_mesa_Materiali(GLenum face, GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   MATERIALFV(face, pname, &p);
}

void GLAPIENTRY
_mesa_Materialiv(GLenum face, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_EMISSION:
   case GL_AMBIENT_AND_DIFFUSE:
      fparam[0] = INT_TO_FLOAT( params[0] );
      fparam[1] = INT_TO_FLOAT( params[1] );
      fparam[2] = INT_TO_FLOAT( params[2] );
      fparam[3] = INT_TO_FLOAT( params[3] );
      break;
   case GL_SHININESS:
      fparam[0] = (GLfloat) params[0];
      break;
   case GL_COLOR_INDEXES:
      fparam[0] = (GLfloat) params[0];
      fparam[1] = (GLfloat) params[1];
      fparam[2] = (GLfloat) params[2];
      break;
   default:
      ;
   }
   MATERIALFV(face, pname, fparam);
}


void GLAPIENTRY
_mesa_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void GLAPIENTRY
_mesa_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

void GLAPIENTRY
_mesa_Rectfv(const GLfloat *v1, const GLfloat *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

void GLAPIENTRY
_mesa_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void GLAPIENTRY
_mesa_Rectiv(const GLint *v1, const GLint *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

void GLAPIENTRY
_mesa_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

void GLAPIENTRY
_mesa_Rectsv(const GLshort *v1, const GLshort *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

void GLAPIENTRY
_mesa_SecondaryColor3b( GLbyte red, GLbyte green, GLbyte blue )
{
   SECONDARYCOLORF( BYTE_TO_FLOAT(red),
		    BYTE_TO_FLOAT(green),
		    BYTE_TO_FLOAT(blue) );
}

void GLAPIENTRY
_mesa_SecondaryColor3d( GLdouble red, GLdouble green, GLdouble blue )
{
   SECONDARYCOLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue );
}

void GLAPIENTRY
_mesa_SecondaryColor3i( GLint red, GLint green, GLint blue )
{
   SECONDARYCOLORF( INT_TO_FLOAT(red),
		    INT_TO_FLOAT(green),
		    INT_TO_FLOAT(blue));
}

void GLAPIENTRY
_mesa_SecondaryColor3s( GLshort red, GLshort green, GLshort blue )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(red),
                   SHORT_TO_FLOAT(green),
                   SHORT_TO_FLOAT(blue));
}

void GLAPIENTRY
_mesa_SecondaryColor3ui( GLuint red, GLuint green, GLuint blue )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(red),
                   UINT_TO_FLOAT(green),
                   UINT_TO_FLOAT(blue));
}

void GLAPIENTRY
_mesa_SecondaryColor3us( GLushort red, GLushort green, GLushort blue )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(red),
                   USHORT_TO_FLOAT(green),
                   USHORT_TO_FLOAT(blue));
}

void GLAPIENTRY
_mesa_SecondaryColor3ub( GLubyte red, GLubyte green, GLubyte blue )
{
   SECONDARYCOLORF(UBYTE_TO_FLOAT(red),
                   UBYTE_TO_FLOAT(green),
                   UBYTE_TO_FLOAT(blue));
}

void GLAPIENTRY
_mesa_SecondaryColor3bv( const GLbyte *v )
{
   SECONDARYCOLORF(BYTE_TO_FLOAT(v[0]),
                   BYTE_TO_FLOAT(v[1]),
                   BYTE_TO_FLOAT(v[2]));
}

void GLAPIENTRY
_mesa_SecondaryColor3dv( const GLdouble *v )
{
   SECONDARYCOLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}
void GLAPIENTRY
_mesa_SecondaryColor3iv( const GLint *v )
{
   SECONDARYCOLORF(INT_TO_FLOAT(v[0]),
                   INT_TO_FLOAT(v[1]),
                   INT_TO_FLOAT(v[2]));
}

void GLAPIENTRY
_mesa_SecondaryColor3sv( const GLshort *v )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(v[0]),
                   SHORT_TO_FLOAT(v[1]),
                   SHORT_TO_FLOAT(v[2]));
}

void GLAPIENTRY
_mesa_SecondaryColor3uiv( const GLuint *v )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(v[0]),
                   UINT_TO_FLOAT(v[1]),
                   UINT_TO_FLOAT(v[2]));
}

void GLAPIENTRY
_mesa_SecondaryColor3usv( const GLushort *v )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(v[0]),
                   USHORT_TO_FLOAT(v[1]),
                   USHORT_TO_FLOAT(v[2]));
}

void GLAPIENTRY
_mesa_SecondaryColor3ubv( const GLubyte *v )
{
   SECONDARYCOLORF(UBYTE_TO_FLOAT(v[0]),
                   UBYTE_TO_FLOAT(v[1]),
                   UBYTE_TO_FLOAT(v[2]));
}


/*
 * GL_NV_vertex_program:
 * Always loop-back to one of the VertexAttrib[1234]f[v]NV functions.
 * Note that attribute indexes DO alias conventional vertex attributes.
 */

void GLAPIENTRY
_mesa_VertexAttrib1sNV(GLuint index, GLshort x)
{
   ATTRIB1NV(index, (GLfloat) x);
}

void GLAPIENTRY
_mesa_VertexAttrib1dNV(GLuint index, GLdouble x)
{
   ATTRIB1NV(index, (GLfloat) x);
}

void GLAPIENTRY
_mesa_VertexAttrib2sNV(GLuint index, GLshort x, GLshort y)
{
   ATTRIB2NV(index, (GLfloat) x, y);
}

void GLAPIENTRY
_mesa_VertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y)
{
   ATTRIB2NV(index, (GLfloat) x, (GLfloat) y);
}

void GLAPIENTRY
_mesa_VertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z)
{
   ATTRIB3NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void GLAPIENTRY
_mesa_VertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_VertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_VertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_VertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   ATTRIB4NV(index, UBYTE_TO_FLOAT(x), UBYTE_TO_FLOAT(y),
	UBYTE_TO_FLOAT(z), UBYTE_TO_FLOAT(w));
}

void GLAPIENTRY
_mesa_VertexAttrib1svNV(GLuint index, const GLshort *v)
{
   ATTRIB1NV(index, (GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_VertexAttrib1dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB1NV(index, (GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_VertexAttrib2svNV(GLuint index, const GLshort *v)
{
   ATTRIB2NV(index, (GLfloat) v[0], (GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_VertexAttrib2dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB2NV(index, (GLfloat) v[0], (GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_VertexAttrib3svNV(GLuint index, const GLshort *v)
{
   ATTRIB3NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_VertexAttrib3dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB3NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_VertexAttrib4svNV(GLuint index, const GLshort *v)
{
   ATTRIB4NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 
	  (GLfloat)v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB4NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4ubvNV(GLuint index, const GLubyte *v)
{
   ATTRIB4NV(index, UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
          UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]));
}


void GLAPIENTRY
_mesa_VertexAttribs1svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib1svNV(index + i, v + i);
}

void GLAPIENTRY
_mesa_VertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      ATTRIB1NV(index + i, v[i]);
}

void GLAPIENTRY
_mesa_VertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib1dvNV(index + i, v + i);
}

void GLAPIENTRY
_mesa_VertexAttribs2svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib2svNV(index + i, v + 2 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      ATTRIB2NV(index + i, v[2 * i], v[2 * i + 1]);
}

void GLAPIENTRY
_mesa_VertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib2dvNV(index + i, v + 2 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs3svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib3svNV(index + i, v + 3 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      ATTRIB3NV(index + i, v[3 * i], v[3 * i + 1], v[3 * i + 2]);
}

void GLAPIENTRY
_mesa_VertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib3dvNV(index + i, v + 3 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs4svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib4svNV(index + i, v + 4 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      ATTRIB4NV(index + i, v[4 * i], v[4 * i + 1], v[4 * i + 2], v[4 * i + 3]);
}

void GLAPIENTRY
_mesa_VertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib4dvNV(index + i, v + 4 * i);
}

void GLAPIENTRY
_mesa_VertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      _mesa_VertexAttrib4ubvNV(index + i, v + 4 * i);
}


/*
 * GL_ARB_vertex_program
 * Always loop-back to one of the VertexAttrib[1234]f[v]ARB functions.
 * Note that attribute indexes do NOT alias conventional attributes.
 */

void GLAPIENTRY
_mesa_VertexAttrib1s(GLuint index, GLshort x)
{
   ATTRIB1ARB(index, (GLfloat) x);
}

void GLAPIENTRY
_mesa_VertexAttrib1d(GLuint index, GLdouble x)
{
   ATTRIB1ARB(index, (GLfloat) x);
}

void GLAPIENTRY
_mesa_VertexAttrib2s(GLuint index, GLshort x, GLshort y)
{
   ATTRIB2ARB(index, (GLfloat) x, y);
}

void GLAPIENTRY
_mesa_VertexAttrib2d(GLuint index, GLdouble x, GLdouble y)
{
   ATTRIB2ARB(index, (GLfloat) x, (GLfloat) y);
}

void GLAPIENTRY
_mesa_VertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z)
{
   ATTRIB3ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void GLAPIENTRY
_mesa_VertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

void GLAPIENTRY
_mesa_VertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_VertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void GLAPIENTRY
_mesa_VertexAttrib1sv(GLuint index, const GLshort *v)
{
   ATTRIB1ARB(index, (GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_VertexAttrib1dv(GLuint index, const GLdouble *v)
{
   ATTRIB1ARB(index, (GLfloat) v[0]);
}

void GLAPIENTRY
_mesa_VertexAttrib2sv(GLuint index, const GLshort *v)
{
   ATTRIB2ARB(index, (GLfloat) v[0], (GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_VertexAttrib2dv(GLuint index, const GLdouble *v)
{
   ATTRIB2ARB(index, (GLfloat) v[0], (GLfloat) v[1]);
}

void GLAPIENTRY
_mesa_VertexAttrib3sv(GLuint index, const GLshort *v)
{
   ATTRIB3ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_VertexAttrib3dv(GLuint index, const GLdouble *v)
{
   ATTRIB3ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void GLAPIENTRY
_mesa_VertexAttrib4sv(GLuint index, const GLshort *v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 
	  (GLfloat)v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4dv(GLuint index, const GLdouble *v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4bv(GLuint index, const GLbyte * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4iv(GLuint index, const GLint * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4ubv(GLuint index, const GLubyte * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4usv(GLuint index, const GLushort * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4uiv(GLuint index, const GLuint * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void GLAPIENTRY
_mesa_VertexAttrib4Nbv(GLuint index, const GLbyte * v)
{
   ATTRIB4ARB(index, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
          BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]));
}

void GLAPIENTRY
_mesa_VertexAttrib4Nsv(GLuint index, const GLshort * v)
{
   ATTRIB4ARB(index, SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
          SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]));
}

void GLAPIENTRY
_mesa_VertexAttrib4Niv(GLuint index, const GLint * v)
{
   ATTRIB4ARB(index, INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
          INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]));
}

void GLAPIENTRY
_mesa_VertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   ATTRIB4ARB(index, UBYTE_TO_FLOAT(x), UBYTE_TO_FLOAT(y),
              UBYTE_TO_FLOAT(z), UBYTE_TO_FLOAT(w));
}

void GLAPIENTRY
_mesa_VertexAttrib4Nubv(GLuint index, const GLubyte * v)
{
   ATTRIB4ARB(index, UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
          UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]));
}

void GLAPIENTRY
_mesa_VertexAttrib4Nusv(GLuint index, const GLushort * v)
{
   ATTRIB4ARB(index, USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
          USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]));
}

void GLAPIENTRY
_mesa_VertexAttrib4Nuiv(GLuint index, const GLuint * v)
{
   ATTRIB4ARB(index, UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
          UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]));
}



/**
 * GL_EXT_gpu_shader / GL 3.0 signed/unsigned integer-valued attributes.
 * Note that attribute indexes do NOT alias conventional attributes.
 */

void GLAPIENTRY
_mesa_VertexAttribI1iv(GLuint index, const GLint *v)
{
   ATTRIBI_1I(index, v[0]);
}

void GLAPIENTRY
_mesa_VertexAttribI1uiv(GLuint index, const GLuint *v)
{
   ATTRIBI_1UI(index, v[0]);
}

void GLAPIENTRY
_mesa_VertexAttribI4bv(GLuint index, const GLbyte *v)
{
   ATTRIBI_4I(index, v[0], v[1], v[2], v[3]);
}

void GLAPIENTRY
_mesa_VertexAttribI4sv(GLuint index, const GLshort *v)
{
   ATTRIBI_4I(index, v[0], v[1], v[2], v[3]);
}

void GLAPIENTRY
_mesa_VertexAttribI4ubv(GLuint index, const GLubyte *v)
{
   ATTRIBI_4UI(index, v[0], v[1], v[2], v[3]);
}

void GLAPIENTRY
_mesa_VertexAttribI4usv(GLuint index, const GLushort *v)
{
   ATTRIBI_4UI(index, v[0], v[1], v[2], v[3]);
}




/*
 * This code never registers handlers for any of the entry points
 * listed in vtxfmt.h.
 */
void
_mesa_loopback_init_api_table(const struct gl_context *ctx,
                              struct _glapi_table *dest)
{
   if (ctx->API != API_OPENGL_CORE && ctx->API != API_OPENGLES2) {
      SET_Color4ub(dest, _mesa_Color4ub);
      SET_Materialf(dest, _mesa_Materialf);
   }
   if (ctx->API == API_OPENGL_COMPAT) {
      SET_Color3b(dest, _mesa_Color3b);
      SET_Color3d(dest, _mesa_Color3d);
      SET_Color3i(dest, _mesa_Color3i);
      SET_Color3s(dest, _mesa_Color3s);
      SET_Color3ui(dest, _mesa_Color3ui);
      SET_Color3us(dest, _mesa_Color3us);
      SET_Color3ub(dest, _mesa_Color3ub);
      SET_Color4b(dest, _mesa_Color4b);
      SET_Color4d(dest, _mesa_Color4d);
      SET_Color4i(dest, _mesa_Color4i);
      SET_Color4s(dest, _mesa_Color4s);
      SET_Color4ui(dest, _mesa_Color4ui);
      SET_Color4us(dest, _mesa_Color4us);
      SET_Color3bv(dest, _mesa_Color3bv);
      SET_Color3dv(dest, _mesa_Color3dv);
      SET_Color3iv(dest, _mesa_Color3iv);
      SET_Color3sv(dest, _mesa_Color3sv);
      SET_Color3uiv(dest, _mesa_Color3uiv);
      SET_Color3usv(dest, _mesa_Color3usv);
      SET_Color3ubv(dest, _mesa_Color3ubv);
      SET_Color4bv(dest, _mesa_Color4bv);
      SET_Color4dv(dest, _mesa_Color4dv);
      SET_Color4iv(dest, _mesa_Color4iv);
      SET_Color4sv(dest, _mesa_Color4sv);
      SET_Color4uiv(dest, _mesa_Color4uiv);
      SET_Color4usv(dest, _mesa_Color4usv);
      SET_Color4ubv(dest, _mesa_Color4ubv);

      SET_SecondaryColor3b(dest, _mesa_SecondaryColor3b);
      SET_SecondaryColor3d(dest, _mesa_SecondaryColor3d);
      SET_SecondaryColor3i(dest, _mesa_SecondaryColor3i);
      SET_SecondaryColor3s(dest, _mesa_SecondaryColor3s);
      SET_SecondaryColor3ui(dest, _mesa_SecondaryColor3ui);
      SET_SecondaryColor3us(dest, _mesa_SecondaryColor3us);
      SET_SecondaryColor3ub(dest, _mesa_SecondaryColor3ub);
      SET_SecondaryColor3bv(dest, _mesa_SecondaryColor3bv);
      SET_SecondaryColor3dv(dest, _mesa_SecondaryColor3dv);
      SET_SecondaryColor3iv(dest, _mesa_SecondaryColor3iv);
      SET_SecondaryColor3sv(dest, _mesa_SecondaryColor3sv);
      SET_SecondaryColor3uiv(dest, _mesa_SecondaryColor3uiv);
      SET_SecondaryColor3usv(dest, _mesa_SecondaryColor3usv);
      SET_SecondaryColor3ubv(dest, _mesa_SecondaryColor3ubv);
      
      SET_EdgeFlagv(dest, _mesa_EdgeFlagv);

      SET_Indexd(dest, _mesa_Indexd);
      SET_Indexi(dest, _mesa_Indexi);
      SET_Indexs(dest, _mesa_Indexs);
      SET_Indexub(dest, _mesa_Indexub);
      SET_Indexdv(dest, _mesa_Indexdv);
      SET_Indexiv(dest, _mesa_Indexiv);
      SET_Indexsv(dest, _mesa_Indexsv);
      SET_Indexubv(dest, _mesa_Indexubv);
      SET_Normal3b(dest, _mesa_Normal3b);
      SET_Normal3d(dest, _mesa_Normal3d);
      SET_Normal3i(dest, _mesa_Normal3i);
      SET_Normal3s(dest, _mesa_Normal3s);
      SET_Normal3bv(dest, _mesa_Normal3bv);
      SET_Normal3dv(dest, _mesa_Normal3dv);
      SET_Normal3iv(dest, _mesa_Normal3iv);
      SET_Normal3sv(dest, _mesa_Normal3sv);
      SET_TexCoord1d(dest, _mesa_TexCoord1d);
      SET_TexCoord1i(dest, _mesa_TexCoord1i);
      SET_TexCoord1s(dest, _mesa_TexCoord1s);
      SET_TexCoord2d(dest, _mesa_TexCoord2d);
      SET_TexCoord2s(dest, _mesa_TexCoord2s);
      SET_TexCoord2i(dest, _mesa_TexCoord2i);
      SET_TexCoord3d(dest, _mesa_TexCoord3d);
      SET_TexCoord3i(dest, _mesa_TexCoord3i);
      SET_TexCoord3s(dest, _mesa_TexCoord3s);
      SET_TexCoord4d(dest, _mesa_TexCoord4d);
      SET_TexCoord4i(dest, _mesa_TexCoord4i);
      SET_TexCoord4s(dest, _mesa_TexCoord4s);
      SET_TexCoord1dv(dest, _mesa_TexCoord1dv);
      SET_TexCoord1iv(dest, _mesa_TexCoord1iv);
      SET_TexCoord1sv(dest, _mesa_TexCoord1sv);
      SET_TexCoord2dv(dest, _mesa_TexCoord2dv);
      SET_TexCoord2iv(dest, _mesa_TexCoord2iv);
      SET_TexCoord2sv(dest, _mesa_TexCoord2sv);
      SET_TexCoord3dv(dest, _mesa_TexCoord3dv);
      SET_TexCoord3iv(dest, _mesa_TexCoord3iv);
      SET_TexCoord3sv(dest, _mesa_TexCoord3sv);
      SET_TexCoord4dv(dest, _mesa_TexCoord4dv);
      SET_TexCoord4iv(dest, _mesa_TexCoord4iv);
      SET_TexCoord4sv(dest, _mesa_TexCoord4sv);
      SET_Vertex2d(dest, _mesa_Vertex2d);
      SET_Vertex2i(dest, _mesa_Vertex2i);
      SET_Vertex2s(dest, _mesa_Vertex2s);
      SET_Vertex3d(dest, _mesa_Vertex3d);
      SET_Vertex3i(dest, _mesa_Vertex3i);
      SET_Vertex3s(dest, _mesa_Vertex3s);
      SET_Vertex4d(dest, _mesa_Vertex4d);
      SET_Vertex4i(dest, _mesa_Vertex4i);
      SET_Vertex4s(dest, _mesa_Vertex4s);
      SET_Vertex2dv(dest, _mesa_Vertex2dv);
      SET_Vertex2iv(dest, _mesa_Vertex2iv);
      SET_Vertex2sv(dest, _mesa_Vertex2sv);
      SET_Vertex3dv(dest, _mesa_Vertex3dv);
      SET_Vertex3iv(dest, _mesa_Vertex3iv);
      SET_Vertex3sv(dest, _mesa_Vertex3sv);
      SET_Vertex4dv(dest, _mesa_Vertex4dv);
      SET_Vertex4iv(dest, _mesa_Vertex4iv);
      SET_Vertex4sv(dest, _mesa_Vertex4sv);
      SET_MultiTexCoord1d(dest, _mesa_MultiTexCoord1d);
      SET_MultiTexCoord1dv(dest, _mesa_MultiTexCoord1dv);
      SET_MultiTexCoord1i(dest, _mesa_MultiTexCoord1i);
      SET_MultiTexCoord1iv(dest, _mesa_MultiTexCoord1iv);
      SET_MultiTexCoord1s(dest, _mesa_MultiTexCoord1s);
      SET_MultiTexCoord1sv(dest, _mesa_MultiTexCoord1sv);
      SET_MultiTexCoord2d(dest, _mesa_MultiTexCoord2d);
      SET_MultiTexCoord2dv(dest, _mesa_MultiTexCoord2dv);
      SET_MultiTexCoord2i(dest, _mesa_MultiTexCoord2i);
      SET_MultiTexCoord2iv(dest, _mesa_MultiTexCoord2iv);
      SET_MultiTexCoord2s(dest, _mesa_MultiTexCoord2s);
      SET_MultiTexCoord2sv(dest, _mesa_MultiTexCoord2sv);
      SET_MultiTexCoord3d(dest, _mesa_MultiTexCoord3d);
      SET_MultiTexCoord3dv(dest, _mesa_MultiTexCoord3dv);
      SET_MultiTexCoord3i(dest, _mesa_MultiTexCoord3i);
      SET_MultiTexCoord3iv(dest, _mesa_MultiTexCoord3iv);
      SET_MultiTexCoord3s(dest, _mesa_MultiTexCoord3s);
      SET_MultiTexCoord3sv(dest, _mesa_MultiTexCoord3sv);
      SET_MultiTexCoord4d(dest, _mesa_MultiTexCoord4d);
      SET_MultiTexCoord4dv(dest, _mesa_MultiTexCoord4dv);
      SET_MultiTexCoord4i(dest, _mesa_MultiTexCoord4i);
      SET_MultiTexCoord4iv(dest, _mesa_MultiTexCoord4iv);
      SET_MultiTexCoord4s(dest, _mesa_MultiTexCoord4s);
      SET_MultiTexCoord4sv(dest, _mesa_MultiTexCoord4sv);
      SET_EvalCoord2dv(dest, _mesa_EvalCoord2dv);
      SET_EvalCoord2fv(dest, _mesa_EvalCoord2fv);
      SET_EvalCoord2d(dest, _mesa_EvalCoord2d);
      SET_EvalCoord1dv(dest, _mesa_EvalCoord1dv);
      SET_EvalCoord1fv(dest, _mesa_EvalCoord1fv);
      SET_EvalCoord1d(dest, _mesa_EvalCoord1d);
      SET_Materiali(dest, _mesa_Materiali);
      SET_Materialiv(dest, _mesa_Materialiv);
      SET_Rectd(dest, _mesa_Rectd);
      SET_Rectdv(dest, _mesa_Rectdv);
      SET_Rectfv(dest, _mesa_Rectfv);
      SET_Recti(dest, _mesa_Recti);
      SET_Rectiv(dest, _mesa_Rectiv);
      SET_Rects(dest, _mesa_Rects);
      SET_Rectsv(dest, _mesa_Rectsv);
      SET_FogCoordd(dest, _mesa_FogCoordd);
      SET_FogCoorddv(dest, _mesa_FogCoorddv);
   }

   if (ctx->API == API_OPENGL_COMPAT) {
      SET_VertexAttrib1sNV(dest, _mesa_VertexAttrib1sNV);
      SET_VertexAttrib1dNV(dest, _mesa_VertexAttrib1dNV);
      SET_VertexAttrib2sNV(dest, _mesa_VertexAttrib2sNV);
      SET_VertexAttrib2dNV(dest, _mesa_VertexAttrib2dNV);
      SET_VertexAttrib3sNV(dest, _mesa_VertexAttrib3sNV);
      SET_VertexAttrib3dNV(dest, _mesa_VertexAttrib3dNV);
      SET_VertexAttrib4sNV(dest, _mesa_VertexAttrib4sNV);
      SET_VertexAttrib4dNV(dest, _mesa_VertexAttrib4dNV);
      SET_VertexAttrib4ubNV(dest, _mesa_VertexAttrib4ubNV);
      SET_VertexAttrib1svNV(dest, _mesa_VertexAttrib1svNV);
      SET_VertexAttrib1dvNV(dest, _mesa_VertexAttrib1dvNV);
      SET_VertexAttrib2svNV(dest, _mesa_VertexAttrib2svNV);
      SET_VertexAttrib2dvNV(dest, _mesa_VertexAttrib2dvNV);
      SET_VertexAttrib3svNV(dest, _mesa_VertexAttrib3svNV);
      SET_VertexAttrib3dvNV(dest, _mesa_VertexAttrib3dvNV);
      SET_VertexAttrib4svNV(dest, _mesa_VertexAttrib4svNV);
      SET_VertexAttrib4dvNV(dest, _mesa_VertexAttrib4dvNV);
      SET_VertexAttrib4ubvNV(dest, _mesa_VertexAttrib4ubvNV);
      SET_VertexAttribs1svNV(dest, _mesa_VertexAttribs1svNV);
      SET_VertexAttribs1fvNV(dest, _mesa_VertexAttribs1fvNV);
      SET_VertexAttribs1dvNV(dest, _mesa_VertexAttribs1dvNV);
      SET_VertexAttribs2svNV(dest, _mesa_VertexAttribs2svNV);
      SET_VertexAttribs2fvNV(dest, _mesa_VertexAttribs2fvNV);
      SET_VertexAttribs2dvNV(dest, _mesa_VertexAttribs2dvNV);
      SET_VertexAttribs3svNV(dest, _mesa_VertexAttribs3svNV);
      SET_VertexAttribs3fvNV(dest, _mesa_VertexAttribs3fvNV);
      SET_VertexAttribs3dvNV(dest, _mesa_VertexAttribs3dvNV);
      SET_VertexAttribs4svNV(dest, _mesa_VertexAttribs4svNV);
      SET_VertexAttribs4fvNV(dest, _mesa_VertexAttribs4fvNV);
      SET_VertexAttribs4dvNV(dest, _mesa_VertexAttribs4dvNV);
      SET_VertexAttribs4ubvNV(dest, _mesa_VertexAttribs4ubvNV);
   }

   if (_mesa_is_desktop_gl(ctx)) {
      SET_VertexAttrib1s(dest, _mesa_VertexAttrib1s);
      SET_VertexAttrib1d(dest, _mesa_VertexAttrib1d);
      SET_VertexAttrib2s(dest, _mesa_VertexAttrib2s);
      SET_VertexAttrib2d(dest, _mesa_VertexAttrib2d);
      SET_VertexAttrib3s(dest, _mesa_VertexAttrib3s);
      SET_VertexAttrib3d(dest, _mesa_VertexAttrib3d);
      SET_VertexAttrib4s(dest, _mesa_VertexAttrib4s);
      SET_VertexAttrib4d(dest, _mesa_VertexAttrib4d);
      SET_VertexAttrib1sv(dest, _mesa_VertexAttrib1sv);
      SET_VertexAttrib1dv(dest, _mesa_VertexAttrib1dv);
      SET_VertexAttrib2sv(dest, _mesa_VertexAttrib2sv);
      SET_VertexAttrib2dv(dest, _mesa_VertexAttrib2dv);
      SET_VertexAttrib3sv(dest, _mesa_VertexAttrib3sv);
      SET_VertexAttrib3dv(dest, _mesa_VertexAttrib3dv);
      SET_VertexAttrib4sv(dest, _mesa_VertexAttrib4sv);
      SET_VertexAttrib4dv(dest, _mesa_VertexAttrib4dv);
      SET_VertexAttrib4Nub(dest, _mesa_VertexAttrib4Nub);
      SET_VertexAttrib4Nubv(dest, _mesa_VertexAttrib4Nubv);
      SET_VertexAttrib4bv(dest, _mesa_VertexAttrib4bv);
      SET_VertexAttrib4iv(dest, _mesa_VertexAttrib4iv);
      SET_VertexAttrib4ubv(dest, _mesa_VertexAttrib4ubv);
      SET_VertexAttrib4usv(dest, _mesa_VertexAttrib4usv);
      SET_VertexAttrib4uiv(dest, _mesa_VertexAttrib4uiv);
      SET_VertexAttrib4Nbv(dest, _mesa_VertexAttrib4Nbv);
      SET_VertexAttrib4Nsv(dest, _mesa_VertexAttrib4Nsv);
      SET_VertexAttrib4Nusv(dest, _mesa_VertexAttrib4Nusv);
      SET_VertexAttrib4Niv(dest, _mesa_VertexAttrib4Niv);
      SET_VertexAttrib4Nuiv(dest, _mesa_VertexAttrib4Nuiv);

      /* GL_EXT_gpu_shader4, GL 3.0 */
      SET_VertexAttribI1iv(dest, _mesa_VertexAttribI1iv);
      SET_VertexAttribI1uiv(dest, _mesa_VertexAttribI1uiv);
      SET_VertexAttribI4bv(dest, _mesa_VertexAttribI4bv);
      SET_VertexAttribI4sv(dest, _mesa_VertexAttribI4sv);
      SET_VertexAttribI4ubv(dest, _mesa_VertexAttribI4ubv);
      SET_VertexAttribI4usv(dest, _mesa_VertexAttribI4usv);
   }
}
