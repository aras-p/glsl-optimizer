/* $Id: api_loopback.c,v 1.8 2001/05/10 15:42:42 keithw Exp $ */

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
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */


#include "glheader.h"
#include "glapitable.h"
#include "macros.h"
#include "colormac.h"
#include "api_loopback.h"

/* KW: A set of functions to convert unusual Color/Normal/Vertex/etc
 * calls to a smaller set of driver-provided formats.  Currently just
 * go back to dispatch to find these (eg. call glNormal3f directly),
 * hence 'loopback'.
 *
 * The driver must supply all of the remaining entry points, which are
 * listed in dd.h.  The easiest way for a driver to do this is to
 * install the supplied software t&l module.
 */
#define DRIVER(x) gl##x
#define COLORUBV(v)                 DRIVER(Color4ubv)(v)
#define COLORF(r,g,b,a)             DRIVER(Color4f)(r,g,b,a)
#define FOGCOORDF(x)                DRIVER(FogCoordfEXT)(x)
#define VERTEX2(x,y)	            DRIVER(Vertex2f)(x,y)
#define VERTEX3(x,y,z)	            DRIVER(Vertex3f)(x,y,z)
#define VERTEX4(x,y,z,w)            DRIVER(Vertex4f)(x,y,z,w)
#define NORMAL(x,y,z)               DRIVER(Normal3f)(x,y,z)
#define TEXCOORD1(s)                DRIVER(TexCoord1f)(s)
#define TEXCOORD2(s,t)              DRIVER(TexCoord2f)(s,t)
#define TEXCOORD3(s,t,u)            DRIVER(TexCoord3f)(s,t,u)
#define TEXCOORD4(s,t,u,v)          DRIVER(TexCoord4f)(s,t,u,v)
#define INDEX(c)		    DRIVER(Indexi)(c)
#define MULTI_TEXCOORD1(z,s)	    DRIVER(MultiTexCoord1fARB)(z,s)
#define MULTI_TEXCOORD2(z,s,t)	    DRIVER(MultiTexCoord2fARB)(z,s,t)
#define MULTI_TEXCOORD3(z,s,t,u)    DRIVER(MultiTexCoord3fARB)(z,s,t,u)
#define MULTI_TEXCOORD4(z,s,t,u,v)  DRIVER(MultiTexCoord4fARB)(z,s,t,u,v)
#define EVALCOORD1(x)               DRIVER(EvalCoord1f)(x)
#define EVALCOORD2(x,y)             DRIVER(EvalCoord2f)(x,y)
#define MATERIALFV(a,b,c)           DRIVER(Materialfv)(a,b,c)
#define RECTF(a,b,c,d)              DRIVER(Rectf)(a,b,c,d)
#define SECONDARYCOLORUB(a,b,c)     DRIVER(SecondaryColor3ubEXT)(a,b,c)
#define SECONDARYCOLORF(a,b,c)      DRIVER(SecondaryColor3fEXT)(a,b,c)


static void
loopback_Color3b( GLbyte red, GLbyte green, GLbyte blue )
{
   GLubyte col[4];
   col[0] = BYTE_TO_UBYTE(red);
   col[1] = BYTE_TO_UBYTE(green);
   col[2] = BYTE_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3d( GLdouble red, GLdouble green, GLdouble blue )
{
   GLubyte col[4];
   GLfloat r = red;
   GLfloat g = green;
   GLfloat b = blue;
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   col[3] = 255;
   COLORUBV( col );
}

static void
loopback_Color3i( GLint red, GLint green, GLint blue )
{
   GLubyte col[4];
   col[0] = INT_TO_UBYTE(red);
   col[1] = INT_TO_UBYTE(green);
   col[2] = INT_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3s( GLshort red, GLshort green, GLshort blue )
{
   GLubyte col[4];
   col[0] = SHORT_TO_UBYTE(red);
   col[1] = SHORT_TO_UBYTE(green);
   col[2] = SHORT_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3ui( GLuint red, GLuint green, GLuint blue )
{
   GLubyte col[4];
   col[0] = UINT_TO_UBYTE(red);
   col[1] = UINT_TO_UBYTE(green);
   col[2] = UINT_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3us( GLushort red, GLushort green, GLushort blue )
{
   GLubyte col[4];
   col[0] = USHORT_TO_UBYTE(red);
   col[1] = USHORT_TO_UBYTE(green);
   col[2] = USHORT_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha )
{
   GLubyte col[4];
   col[0] = BYTE_TO_UBYTE(red);
   col[1] = BYTE_TO_UBYTE(green);
   col[2] = BYTE_TO_UBYTE(blue);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha )
{
   GLubyte col[4];
   GLfloat r = red;
   GLfloat g = green;
   GLfloat b = blue;
   GLfloat a = alpha;
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   UNCLAMPED_FLOAT_TO_UBYTE(col[3], a);
   COLORUBV( col );
}

static void
loopback_Color4i( GLint red, GLint green, GLint blue, GLint alpha )
{
   GLubyte col[4];
   col[0] = INT_TO_UBYTE(red);
   col[1] = INT_TO_UBYTE(green);
   col[2] = INT_TO_UBYTE(blue);
   col[3] = INT_TO_UBYTE(alpha);
   COLORUBV(col);
}

static void
loopback_Color4s( GLshort red, GLshort green, GLshort blue,
			GLshort alpha )
{
   GLubyte col[4];
   col[0] = SHORT_TO_UBYTE(red);
   col[1] = SHORT_TO_UBYTE(green);
   col[2] = SHORT_TO_UBYTE(blue);
   col[3] = SHORT_TO_UBYTE(alpha);
   COLORUBV(col);
}

static void
loopback_Color4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
   GLubyte col[4];
   col[0] = UINT_TO_UBYTE(red);
   col[1] = UINT_TO_UBYTE(green);
   col[2] = UINT_TO_UBYTE(blue);
   col[3] = UINT_TO_UBYTE(alpha);
   COLORUBV(col);
}

static void
loopback_Color4us( GLushort red, GLushort green, GLushort blue,
			 GLushort alpha )
{
   GLubyte col[4];
   col[0] = USHORT_TO_UBYTE(red);
   col[1] = USHORT_TO_UBYTE(green);
   col[2] = USHORT_TO_UBYTE(blue);
   col[3] = USHORT_TO_UBYTE(alpha);
   COLORUBV(col);
}

static void
loopback_Color3bv( const GLbyte *v )
{
   GLubyte col[4];
   col[0] = BYTE_TO_UBYTE(v[0]);
   col[1] = BYTE_TO_UBYTE(v[1]);
   col[2] = BYTE_TO_UBYTE(v[2]);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3dv( const GLdouble *v )
{
   GLubyte col[4];
   GLfloat r = v[0];
   GLfloat g = v[1];
   GLfloat b = v[2];
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   col[3] = 255;
   COLORUBV( col );
}

static void
loopback_Color3iv( const GLint *v )
{
   GLubyte col[4];
   col[0] = INT_TO_UBYTE(v[0]);
   col[1] = INT_TO_UBYTE(v[1]);
   col[2] = INT_TO_UBYTE(v[2]);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3sv( const GLshort *v )
{
   GLubyte col[4];
   col[0] = SHORT_TO_UBYTE(v[0]);
   col[1] = SHORT_TO_UBYTE(v[1]);
   col[2] = SHORT_TO_UBYTE(v[2]);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3uiv( const GLuint *v )
{
   GLubyte col[4];
   col[0] = UINT_TO_UBYTE(v[0]);
   col[1] = UINT_TO_UBYTE(v[1]);
   col[2] = UINT_TO_UBYTE(v[2]);
   col[3] = 255;
   COLORUBV(col);
}

static void
loopback_Color3usv( const GLushort *v )
{
   GLubyte col[4];
   col[0] = USHORT_TO_UBYTE(v[0]);
   col[1] = USHORT_TO_UBYTE(v[1]);
   col[2] = USHORT_TO_UBYTE(v[2]);
   col[3] = 255;
   COLORUBV(col);

}

static void
loopback_Color4bv( const GLbyte *v )
{
   GLubyte col[4];
   col[0] = BYTE_TO_UBYTE(v[0]);
   col[1] = BYTE_TO_UBYTE(v[1]);
   col[2] = BYTE_TO_UBYTE(v[2]);
   col[3] = BYTE_TO_UBYTE(v[3]);
   COLORUBV(col);
}

static void
loopback_Color4dv( const GLdouble *v )
{
   GLubyte col[4];
   GLfloat r = v[0];
   GLfloat g = v[1];
   GLfloat b = v[2];
   GLfloat a = v[3];
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   UNCLAMPED_FLOAT_TO_UBYTE(col[3], a);
   COLORUBV( col );
}

static void
loopback_Color4iv( const GLint *v )
{
   GLubyte col[4];
   col[0] = INT_TO_UBYTE(v[0]);
   col[1] = INT_TO_UBYTE(v[1]);
   col[2] = INT_TO_UBYTE(v[2]);
   col[3] = INT_TO_UBYTE(v[3]);
   COLORUBV(col);
}

static void
loopback_Color4sv( const GLshort *v)
{
   GLubyte col[4];
   col[0] = SHORT_TO_UBYTE(v[0]);
   col[1] = SHORT_TO_UBYTE(v[1]);
   col[2] = SHORT_TO_UBYTE(v[2]);
   col[3] = SHORT_TO_UBYTE(v[3]);
   COLORUBV(col);
}

static void
loopback_Color4uiv( const GLuint *v)
{
   GLubyte col[4];
   col[0] = UINT_TO_UBYTE(v[0]);
   col[1] = UINT_TO_UBYTE(v[1]);
   col[2] = UINT_TO_UBYTE(v[2]);
   col[3] = UINT_TO_UBYTE(v[3]);
   COLORUBV(col);
}

static void
loopback_Color4usv( const GLushort *v)
{
   GLubyte col[4];
   col[0] = USHORT_TO_UBYTE(v[0]);
   col[1] = USHORT_TO_UBYTE(v[1]);
   col[2] = USHORT_TO_UBYTE(v[2]);
   col[3] = USHORT_TO_UBYTE(v[3]);
   COLORUBV(col);
}

static void
loopback_Color3b_f( GLbyte red, GLbyte green, GLbyte blue )
{
   COLORF( BYTE_TO_FLOAT(red),
	   BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue),
	   1.0 );
}

static void
loopback_Color3d_f( GLdouble red, GLdouble green, GLdouble blue )
{
   COLORF( red, green, blue, 1.0 );
}

static void
loopback_Color3i_f( GLint red, GLint green, GLint blue )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), 1.0);
}

static void
loopback_Color3s_f( GLshort red, GLshort green, GLshort blue )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), 1.0);
}

static void
loopback_Color3ui_f( GLuint red, GLuint green, GLuint blue )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), 1.0 );
}

static void
loopback_Color3us_f( GLushort red, GLushort green, GLushort blue )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), 1.0 );
}


static void
loopback_Color3bv_f( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), 1.0 );
}

static void
loopback_Color3dv_f( const GLdouble *v )
{
   COLORF( v[0], v[1], v[2], 1.0 );
}

static void
loopback_Color3iv_f( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}

static void
loopback_Color3sv_f( const GLshort *v )
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), 1.0 );
}

static void
loopback_Color3uiv_f( const GLuint *v )
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), 1.0 );
}

static void
loopback_Color3usv_f( const GLushort *v )
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), 1.0 );
}


static void
loopback_Color4b_f( GLbyte red, GLbyte green, GLbyte blue,
			      GLbyte alpha )
{
   COLORF( BYTE_TO_FLOAT(red), BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue), BYTE_TO_FLOAT(alpha) );
}

static void
loopback_Color4d_f( GLdouble red, GLdouble green, GLdouble blue,
			      GLdouble alpha )
{
   COLORF( red, green, blue, alpha );
}

static void
loopback_Color4i_f( GLint red, GLint green, GLint blue, GLint alpha )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), INT_TO_FLOAT(alpha) );
}

static void
loopback_Color4s_f( GLshort red, GLshort green, GLshort blue,
			      GLshort alpha )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), SHORT_TO_FLOAT(alpha) );
}

static void
loopback_Color4ui_f( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), UINT_TO_FLOAT(alpha) );
}

static void
loopback_Color4us_f( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), USHORT_TO_FLOAT(alpha) );
}


static void
loopback_Color4iv_f( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}


static void
loopback_Color4bv_f( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]) );
}

static void
loopback_Color4dv_f( const GLdouble *v )
{
   COLORF( v[0], v[1], v[2], v[3] );
}


static void
loopback_Color4sv_f( const GLshort *v)
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]) );
}


static void
loopback_Color4uiv_f( const GLuint *v)
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]) );
}

static void
loopback_Color4usv_f( const GLushort *v)
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]) );
}

static void
loopback_FogCoorddEXT( GLdouble d )
{
   FOGCOORDF( d );
}

static void
loopback_FogCoorddvEXT( const GLdouble *v )
{
   FOGCOORDF( *v );
}


static void
loopback_Indexd( GLdouble c )
{
   INDEX( (GLint) c );
}

static void
loopback_Indexf( GLfloat c )
{
   INDEX( (GLuint) (GLint) c );
}

static void
loopback_Indexs( GLshort c )
{
   INDEX( (GLint) c );
}

static void
loopback_Indexub( GLubyte c )
{
   INDEX( (GLint) c );
}

static void
loopback_Indexdv( const GLdouble *c )
{
   INDEX( (GLint) (GLint) *c );
}

static void
loopback_Indexfv( const GLfloat *c )
{
   INDEX( (GLint) (GLint) *c );
}

static void
loopback_Indexiv( const GLint *c )
{
   INDEX( *c );
}

static void
loopback_Indexsv( const GLshort *c )
{
   INDEX( (GLint) *c );
}

static void
loopback_Indexubv( const GLubyte *c )
{
   INDEX( (GLint) *c );
}

static void
loopback_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz )
{
   NORMAL( BYTE_TO_FLOAT(nx), BYTE_TO_FLOAT(ny), BYTE_TO_FLOAT(nz) );
}

static void
loopback_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz )
{
   NORMAL(nx, ny, nz);
}

static void
loopback_Normal3i( GLint nx, GLint ny, GLint nz )
{
   NORMAL( INT_TO_FLOAT(nx), INT_TO_FLOAT(ny), INT_TO_FLOAT(nz) );
}

static void
loopback_Normal3s( GLshort nx, GLshort ny, GLshort nz )
{
   NORMAL( SHORT_TO_FLOAT(nx), SHORT_TO_FLOAT(ny), SHORT_TO_FLOAT(nz) );
}

static void
loopback_Normal3bv( const GLbyte *v )
{
   NORMAL( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]), BYTE_TO_FLOAT(v[2]) );
}

static void
loopback_Normal3dv( const GLdouble *v )
{
   NORMAL( v[0], v[1], v[2] );
}

static void
loopback_Normal3iv( const GLint *v )
{
   NORMAL( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]), INT_TO_FLOAT(v[2]) );
}

static void
loopback_Normal3sv( const GLshort *v )
{
   NORMAL( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]), SHORT_TO_FLOAT(v[2]) );
}

static void
loopback_TexCoord1d( GLdouble s )
{
   TEXCOORD1(s);
}

static void
loopback_TexCoord1i( GLint s )
{
   TEXCOORD1(s);
}

static void
loopback_TexCoord1s( GLshort s )
{
   TEXCOORD1(s);
}

static void
loopback_TexCoord2d( GLdouble s, GLdouble t )
{
   TEXCOORD2(s,t);
}

static void
loopback_TexCoord2s( GLshort s, GLshort t )
{
   TEXCOORD2(s,t);
}

static void
loopback_TexCoord2i( GLint s, GLint t )
{
   TEXCOORD2(s,t);
}

static void
loopback_TexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
   TEXCOORD3(s,t,r);
}

static void
loopback_TexCoord3i( GLint s, GLint t, GLint r )
{
   TEXCOORD3(s,t,r);
}

static void
loopback_TexCoord3s( GLshort s, GLshort t, GLshort r )
{
   TEXCOORD3(s,t,r);
}

static void
loopback_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
   TEXCOORD4(s,t,r,q);
}

static void
loopback_TexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
   TEXCOORD4(s,t,r,q);
}

static void
loopback_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
   TEXCOORD4(s,t,r,q);
}

static void
loopback_TexCoord1dv( const GLdouble *v )
{
   TEXCOORD1(v[0]);
}

static void
loopback_TexCoord1iv( const GLint *v )
{
   TEXCOORD1(v[0]);
}

static void
loopback_TexCoord1sv( const GLshort *v )
{
   TEXCOORD1(v[0]);
}

static void
loopback_TexCoord2dv( const GLdouble *v )
{
   TEXCOORD2(v[0],v[1]);
}

static void
loopback_TexCoord2iv( const GLint *v )
{
   TEXCOORD2(v[0],v[1]);
}

static void
loopback_TexCoord2sv( const GLshort *v )
{
   TEXCOORD2(v[0],v[1]);
}

static void
loopback_TexCoord3dv( const GLdouble *v )
{
   TEXCOORD2(v[0],v[1]);
}

static void
loopback_TexCoord3iv( const GLint *v )
{
   TEXCOORD3(v[0],v[1],v[2]);
}

static void
loopback_TexCoord3sv( const GLshort *v )
{
   TEXCOORD3(v[0],v[1],v[2]);
}

static void
loopback_TexCoord4dv( const GLdouble *v )
{
   TEXCOORD4(v[0],v[1],v[2],v[3]);
}

static void
loopback_TexCoord4iv( const GLint *v )
{
   TEXCOORD4(v[0],v[1],v[2],v[3]);
}

static void
loopback_TexCoord4sv( const GLshort *v )
{
   TEXCOORD4(v[0],v[1],v[2],v[3]);
}

static void
loopback_Vertex2d( GLdouble x, GLdouble y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void
loopback_Vertex2i( GLint x, GLint y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void
loopback_Vertex2s( GLshort x, GLshort y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void
loopback_Vertex3d( GLdouble x, GLdouble y, GLdouble z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void
loopback_Vertex3i( GLint x, GLint y, GLint z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void
loopback_Vertex3s( GLshort x, GLshort y, GLshort z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void
loopback_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void
loopback_Vertex4i( GLint x, GLint y, GLint z, GLint w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void
loopback_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void
loopback_Vertex2dv( const GLdouble *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void
loopback_Vertex2iv( const GLint *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void
loopback_Vertex2sv( const GLshort *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void
loopback_Vertex3dv( const GLdouble *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void
loopback_Vertex3iv( const GLint *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void
loopback_Vertex3sv( const GLshort *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void
loopback_Vertex4dv( const GLdouble *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void
loopback_Vertex4iv( const GLint *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void
loopback_Vertex4sv( const GLshort *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void
loopback_MultiTexCoord1dARB(GLenum target, GLdouble s)
{
   MULTI_TEXCOORD1( target, s );
}

static void
loopback_MultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD1( target, v[0] );
}

static void
loopback_MultiTexCoord1iARB(GLenum target, GLint s)
{
   MULTI_TEXCOORD1( target, s );
}

static void
loopback_MultiTexCoord1ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD1( target, v[0] );
}

static void
loopback_MultiTexCoord1sARB(GLenum target, GLshort s)
{
   MULTI_TEXCOORD1( target, s );
}

static void
loopback_MultiTexCoord1svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD1( target, v[0] );
}

static void
loopback_MultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
   MULTI_TEXCOORD2( target, s, t );
}

static void
loopback_MultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD2( target, v[0], v[1] );
}

static void
loopback_MultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
   MULTI_TEXCOORD2( target, s, t );
}

static void
loopback_MultiTexCoord2ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD2( target, v[0], v[1] );
}

static void
loopback_MultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
   MULTI_TEXCOORD2( target, s, t );
}

static void
loopback_MultiTexCoord2svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD2( target, v[0], v[1] );
}

static void
loopback_MultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   MULTI_TEXCOORD3( target, s, t, r );
}

static void
loopback_MultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD3( target, v[0], v[1], v[2] );
}

static void
loopback_MultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
   MULTI_TEXCOORD3( target, s, t, r );
}

static void
loopback_MultiTexCoord3ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD3( target, v[0], v[1], v[2] );
}

static void
loopback_MultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
   MULTI_TEXCOORD3( target, s, t, r );
}

static void
loopback_MultiTexCoord3svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD3( target, v[0], v[1], v[2] );
}

static void
loopback_MultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   MULTI_TEXCOORD4( target, s, t, r, q );
}

static void
loopback_MultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD4( target, v[0], v[1], v[2], v[3] );
}

static void
loopback_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   MULTI_TEXCOORD4( target, s, t, r, q );
}

static void
loopback_MultiTexCoord4ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD4( target, v[0], v[1], v[2], v[3] );
}

static void
loopback_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   MULTI_TEXCOORD4( target, s, t, r, q );
}

static void
loopback_MultiTexCoord4svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD4( target, v[0], v[1], v[2], v[3] );
}

static void
loopback_EvalCoord2dv( const GLdouble *u )
{
   EVALCOORD2( (GLfloat) u[0], (GLfloat) u[1] );
}

static void
loopback_EvalCoord2fv( const GLfloat *u )
{
   EVALCOORD2( u[0], u[1] );
}

static void
loopback_EvalCoord2d( GLdouble u, GLdouble v )
{
   EVALCOORD2( (GLfloat) u, (GLfloat) v );
}

static void
loopback_EvalCoord1dv( const GLdouble *u )
{
   EVALCOORD1( (GLfloat) *u );
}

static void
loopback_EvalCoord1fv( const GLfloat *u )
{
   EVALCOORD1( (GLfloat) *u );
}

static void
loopback_EvalCoord1d( GLdouble u )
{
   EVALCOORD1( (GLfloat) u );
}

static void
loopback_Materialf( GLenum face, GLenum pname, GLfloat param )
{
   GLfloat fparam[4];
   fparam[0] = param;
   MATERIALFV( face, pname, fparam );
}

static void
loopback_Materiali(GLenum face, GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   MATERIALFV(face, pname, &p);
}

static void
loopback_Materialiv(GLenum face, GLenum pname, const GLint *params )
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


static void
loopback_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   RECTF(x1, y1, x2, y2);
}

static void
loopback_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

static void
loopback_Rectfv(const GLfloat *v1, const GLfloat *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

static void
loopback_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   RECTF(x1, y1, x2, y2);
}

static void
loopback_Rectiv(const GLint *v1, const GLint *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

static void
loopback_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   RECTF(x1, y1, x2, y2);
}

static void
loopback_Rectsv(const GLshort *v1, const GLshort *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

static void
loopback_SecondaryColor3bEXT( GLbyte red, GLbyte green, GLbyte blue )
{
   SECONDARYCOLORUB( BYTE_TO_UBYTE(red),
		     BYTE_TO_UBYTE(green),
		     BYTE_TO_UBYTE(blue) );
}

static void
loopback_SecondaryColor3dEXT( GLdouble red, GLdouble green, GLdouble blue )
{
   GLubyte col[3];
   GLfloat r = red;
   GLfloat g = green;
   GLfloat b = blue;
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   SECONDARYCOLORUB( col[0], col[1], col[2] );
}

static void
loopback_SecondaryColor3iEXT( GLint red, GLint green, GLint blue )
{
   SECONDARYCOLORUB( INT_TO_UBYTE(red),
		     INT_TO_UBYTE(green),
		     INT_TO_UBYTE(blue));
}

static void
loopback_SecondaryColor3sEXT( GLshort red, GLshort green, GLshort blue )
{
   SECONDARYCOLORUB(SHORT_TO_UBYTE(red),
		    SHORT_TO_UBYTE(green),
		    SHORT_TO_UBYTE(blue));
}

static void
loopback_SecondaryColor3uiEXT( GLuint red, GLuint green, GLuint blue )
{
   SECONDARYCOLORUB(UINT_TO_UBYTE(red),
		    UINT_TO_UBYTE(green),
		    UINT_TO_UBYTE(blue));
}

static void
loopback_SecondaryColor3usEXT( GLushort red, GLushort green, GLushort blue )
{
   SECONDARYCOLORUB(USHORT_TO_UBYTE(red),
		    USHORT_TO_UBYTE(green),
		    USHORT_TO_UBYTE(blue));
}

static void
loopback_SecondaryColor3bvEXT( const GLbyte *v )
{
   const GLfloat r = BYTE_TO_FLOAT(v[0]);
   const GLfloat g = BYTE_TO_FLOAT(v[1]);
   const GLfloat b = BYTE_TO_FLOAT(v[2]);
   SECONDARYCOLORF(r, g, b);
}

static void
loopback_SecondaryColor3dvEXT( const GLdouble *v )
{
   GLubyte col[3];
   GLfloat r = v[0];
   GLfloat g = v[1];
   GLfloat b = v[2];
   UNCLAMPED_FLOAT_TO_UBYTE(col[0], r);
   UNCLAMPED_FLOAT_TO_UBYTE(col[1], g);
   UNCLAMPED_FLOAT_TO_UBYTE(col[2], b);
   SECONDARYCOLORUB( col[0], col[1], col[2] );
}

static void
loopback_SecondaryColor3ivEXT( const GLint *v )
{
   SECONDARYCOLORUB(INT_TO_UBYTE(v[0]),
		    INT_TO_UBYTE(v[1]),
		    INT_TO_UBYTE(v[2]));
}

static void
loopback_SecondaryColor3svEXT( const GLshort *v )
{
   SECONDARYCOLORUB(SHORT_TO_UBYTE(v[0]),
		    SHORT_TO_UBYTE(v[1]),
		    SHORT_TO_UBYTE(v[2]));
}

static void
loopback_SecondaryColor3uivEXT( const GLuint *v )
{
   SECONDARYCOLORUB(UINT_TO_UBYTE(v[0]),
		    UINT_TO_UBYTE(v[1]),
		    UINT_TO_UBYTE(v[2]));
}

static void
loopback_SecondaryColor3usvEXT( const GLushort *v )
{
   SECONDARYCOLORUB(USHORT_TO_UBYTE(v[0]),
		    USHORT_TO_UBYTE(v[1]),
		    USHORT_TO_UBYTE(v[2]));
}


static void
loopback_SecondaryColor3bEXT_f( GLbyte red, GLbyte green, GLbyte blue )
{
   SECONDARYCOLORF( BYTE_TO_FLOAT(red),
		    BYTE_TO_FLOAT(green),
		    BYTE_TO_FLOAT(blue) );
}

static void
loopback_SecondaryColor3dEXT_f( GLdouble red, GLdouble green, GLdouble blue )
{
   SECONDARYCOLORF( red, green, blue );
}

static void
loopback_SecondaryColor3iEXT_f( GLint red, GLint green, GLint blue )
{
   SECONDARYCOLORF( INT_TO_FLOAT(red),
		    INT_TO_FLOAT(green),
		    INT_TO_FLOAT(blue));
}

static void
loopback_SecondaryColor3sEXT_f( GLshort red, GLshort green, GLshort blue )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(red),
                   SHORT_TO_FLOAT(green),
                   SHORT_TO_FLOAT(blue));
}

static void
loopback_SecondaryColor3uiEXT_f( GLuint red, GLuint green, GLuint blue )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(red),
                   UINT_TO_FLOAT(green),
                   UINT_TO_FLOAT(blue));
}

static void
loopback_SecondaryColor3usEXT_f( GLushort red, GLushort green, GLushort blue )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(red),
                   USHORT_TO_FLOAT(green),
                   USHORT_TO_FLOAT(blue));
}

static void
loopback_SecondaryColor3bvEXT_f( const GLbyte *v )
{
   SECONDARYCOLORF(BYTE_TO_FLOAT(v[0]),
                   BYTE_TO_FLOAT(v[1]),
                   BYTE_TO_FLOAT(v[2]));
}

static void
loopback_SecondaryColor3dvEXT_f( const GLdouble *v )
{
   SECONDARYCOLORF( v[0], v[1], v[2] );
}
static void
loopback_SecondaryColor3ivEXT_f( const GLint *v )
{
   SECONDARYCOLORF(INT_TO_FLOAT(v[0]),
                   INT_TO_FLOAT(v[1]),
                   INT_TO_FLOAT(v[2]));
}

static void
loopback_SecondaryColor3svEXT_f( const GLshort *v )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(v[0]),
                   SHORT_TO_FLOAT(v[1]),
                   SHORT_TO_FLOAT(v[2]));
}

static void
loopback_SecondaryColor3uivEXT_f( const GLuint *v )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(v[0]),
                   UINT_TO_FLOAT(v[1]),
                   UINT_TO_FLOAT(v[2]));
}

static void
loopback_SecondaryColor3usvEXT_f( const GLushort *v )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(v[0]),
                   USHORT_TO_FLOAT(v[1]),
                   USHORT_TO_FLOAT(v[2]));
}



void
_mesa_loopback_prefer_float( struct _glapi_table *dest,
			     GLboolean prefer_float_colors )
{
   if (!prefer_float_colors) {
      dest->Color3b = loopback_Color3b;
      dest->Color3d = loopback_Color3d;
      dest->Color3i = loopback_Color3i;
      dest->Color3s = loopback_Color3s;
      dest->Color3ui = loopback_Color3ui;
      dest->Color3us = loopback_Color3us;
      dest->Color4b = loopback_Color4b;
      dest->Color4d = loopback_Color4d;
      dest->Color4i = loopback_Color4i;
      dest->Color4s = loopback_Color4s;
      dest->Color4ui = loopback_Color4ui;
      dest->Color4us = loopback_Color4us;
      dest->Color3bv = loopback_Color3bv;
      dest->Color3dv = loopback_Color3dv;
      dest->Color3iv = loopback_Color3iv;
      dest->Color3sv = loopback_Color3sv;
      dest->Color3uiv = loopback_Color3uiv;
      dest->Color3usv = loopback_Color3usv;
      dest->Color4bv = loopback_Color4bv;
      dest->Color4dv = loopback_Color4dv;
      dest->Color4iv = loopback_Color4iv;
      dest->Color4sv = loopback_Color4sv;
      dest->Color4uiv = loopback_Color4uiv;
      dest->Color4usv = loopback_Color4usv;
      dest->SecondaryColor3bEXT = loopback_SecondaryColor3bEXT;
      dest->SecondaryColor3dEXT = loopback_SecondaryColor3dEXT;
      dest->SecondaryColor3iEXT = loopback_SecondaryColor3iEXT;
      dest->SecondaryColor3sEXT = loopback_SecondaryColor3sEXT;
      dest->SecondaryColor3uiEXT = loopback_SecondaryColor3uiEXT;
      dest->SecondaryColor3usEXT = loopback_SecondaryColor3usEXT;
      dest->SecondaryColor3bvEXT = loopback_SecondaryColor3bvEXT;
      dest->SecondaryColor3dvEXT = loopback_SecondaryColor3dvEXT;
      dest->SecondaryColor3ivEXT = loopback_SecondaryColor3ivEXT;
      dest->SecondaryColor3svEXT = loopback_SecondaryColor3svEXT;
      dest->SecondaryColor3uivEXT = loopback_SecondaryColor3uivEXT;
      dest->SecondaryColor3usvEXT = loopback_SecondaryColor3usvEXT;
   }
   else {
      dest->Color3b = loopback_Color3b_f;
      dest->Color3d = loopback_Color3d_f;
      dest->Color3i = loopback_Color3i_f;
      dest->Color3s = loopback_Color3s_f;
      dest->Color3ui = loopback_Color3ui_f;
      dest->Color3us = loopback_Color3us_f;
      dest->Color4b = loopback_Color4b_f;
      dest->Color4d = loopback_Color4d_f;
      dest->Color4i = loopback_Color4i_f;
      dest->Color4s = loopback_Color4s_f;
      dest->Color4ui = loopback_Color4ui_f;
      dest->Color4us = loopback_Color4us_f;
      dest->Color3bv = loopback_Color3bv_f;
      dest->Color3dv = loopback_Color3dv_f;
      dest->Color3iv = loopback_Color3iv_f;
      dest->Color3sv = loopback_Color3sv_f;
      dest->Color3uiv = loopback_Color3uiv_f;
      dest->Color3usv = loopback_Color3usv_f;
      dest->Color4bv = loopback_Color4bv_f;
      dest->Color4dv = loopback_Color4dv_f;
      dest->Color4iv = loopback_Color4iv_f;
      dest->Color4sv = loopback_Color4sv_f;
      dest->Color4uiv = loopback_Color4uiv_f;
      dest->Color4usv = loopback_Color4usv_f;
      dest->SecondaryColor3bEXT = loopback_SecondaryColor3bEXT_f;
      dest->SecondaryColor3dEXT = loopback_SecondaryColor3dEXT_f;
      dest->SecondaryColor3iEXT = loopback_SecondaryColor3iEXT_f;
      dest->SecondaryColor3sEXT = loopback_SecondaryColor3sEXT_f;
      dest->SecondaryColor3uiEXT = loopback_SecondaryColor3uiEXT_f;
      dest->SecondaryColor3usEXT = loopback_SecondaryColor3usEXT_f;
      dest->SecondaryColor3bvEXT = loopback_SecondaryColor3bvEXT_f;
      dest->SecondaryColor3dvEXT = loopback_SecondaryColor3dvEXT_f;
      dest->SecondaryColor3ivEXT = loopback_SecondaryColor3ivEXT_f;
      dest->SecondaryColor3svEXT = loopback_SecondaryColor3svEXT_f;
      dest->SecondaryColor3uivEXT = loopback_SecondaryColor3uivEXT_f;
      dest->SecondaryColor3usvEXT = loopback_SecondaryColor3usvEXT_f;
   }
}

/* Passing prefer_float_colors as true will mean that all colors
 * *except* Color{34}ub{v} are passed as floats.  Setting it false will
 * mean all colors *except* Color{34}f{v} are passed as ubytes.
 *
 * This code never registers handlers for any of the entry points
 * listed in vtxfmt.h.
 */
void
_mesa_loopback_init_api_table( struct _glapi_table *dest,
			       GLboolean prefer_float_colors )
{
   _mesa_loopback_prefer_float( dest, prefer_float_colors );

   dest->Indexd = loopback_Indexd;
   dest->Indexf = loopback_Indexf;
   dest->Indexs = loopback_Indexs;
   dest->Indexub = loopback_Indexub;
   dest->Indexdv = loopback_Indexdv;
   dest->Indexfv = loopback_Indexfv;
   dest->Indexiv = loopback_Indexiv;
   dest->Indexsv = loopback_Indexsv;
   dest->Indexubv = loopback_Indexubv;
   dest->Normal3b = loopback_Normal3b;
   dest->Normal3d = loopback_Normal3d;
   dest->Normal3i = loopback_Normal3i;
   dest->Normal3s = loopback_Normal3s;
   dest->Normal3bv = loopback_Normal3bv;
   dest->Normal3dv = loopback_Normal3dv;
   dest->Normal3iv = loopback_Normal3iv;
   dest->Normal3sv = loopback_Normal3sv;
   dest->TexCoord1d = loopback_TexCoord1d;
   dest->TexCoord1i = loopback_TexCoord1i;
   dest->TexCoord1s = loopback_TexCoord1s;
   dest->TexCoord2d = loopback_TexCoord2d;
   dest->TexCoord2s = loopback_TexCoord2s;
   dest->TexCoord2i = loopback_TexCoord2i;
   dest->TexCoord3d = loopback_TexCoord3d;
   dest->TexCoord3i = loopback_TexCoord3i;
   dest->TexCoord3s = loopback_TexCoord3s;
   dest->TexCoord4d = loopback_TexCoord4d;
   dest->TexCoord4i = loopback_TexCoord4i;
   dest->TexCoord4s = loopback_TexCoord4s;
   dest->TexCoord1dv = loopback_TexCoord1dv;
   dest->TexCoord1iv = loopback_TexCoord1iv;
   dest->TexCoord1sv = loopback_TexCoord1sv;
   dest->TexCoord2dv = loopback_TexCoord2dv;
   dest->TexCoord2iv = loopback_TexCoord2iv;
   dest->TexCoord2sv = loopback_TexCoord2sv;
   dest->TexCoord3dv = loopback_TexCoord3dv;
   dest->TexCoord3iv = loopback_TexCoord3iv;
   dest->TexCoord3sv = loopback_TexCoord3sv;
   dest->TexCoord4dv = loopback_TexCoord4dv;
   dest->TexCoord4iv = loopback_TexCoord4iv;
   dest->TexCoord4sv = loopback_TexCoord4sv;
   dest->Vertex2d = loopback_Vertex2d;
   dest->Vertex2i = loopback_Vertex2i;
   dest->Vertex2s = loopback_Vertex2s;
   dest->Vertex3d = loopback_Vertex3d;
   dest->Vertex3i = loopback_Vertex3i;
   dest->Vertex3s = loopback_Vertex3s;
   dest->Vertex4d = loopback_Vertex4d;
   dest->Vertex4i = loopback_Vertex4i;
   dest->Vertex4s = loopback_Vertex4s;
   dest->Vertex2dv = loopback_Vertex2dv;
   dest->Vertex2iv = loopback_Vertex2iv;
   dest->Vertex2sv = loopback_Vertex2sv;
   dest->Vertex3dv = loopback_Vertex3dv;
   dest->Vertex3iv = loopback_Vertex3iv;
   dest->Vertex3sv = loopback_Vertex3sv;
   dest->Vertex4dv = loopback_Vertex4dv;
   dest->Vertex4iv = loopback_Vertex4iv;
   dest->Vertex4sv = loopback_Vertex4sv;
   dest->MultiTexCoord1dARB = loopback_MultiTexCoord1dARB;
   dest->MultiTexCoord1dvARB = loopback_MultiTexCoord1dvARB;
   dest->MultiTexCoord1iARB = loopback_MultiTexCoord1iARB;
   dest->MultiTexCoord1ivARB = loopback_MultiTexCoord1ivARB;
   dest->MultiTexCoord1sARB = loopback_MultiTexCoord1sARB;
   dest->MultiTexCoord1svARB = loopback_MultiTexCoord1svARB;
   dest->MultiTexCoord2dARB = loopback_MultiTexCoord2dARB;
   dest->MultiTexCoord2dvARB = loopback_MultiTexCoord2dvARB;
   dest->MultiTexCoord2iARB = loopback_MultiTexCoord2iARB;
   dest->MultiTexCoord2ivARB = loopback_MultiTexCoord2ivARB;
   dest->MultiTexCoord2sARB = loopback_MultiTexCoord2sARB;
   dest->MultiTexCoord2svARB = loopback_MultiTexCoord2svARB;
   dest->MultiTexCoord3dARB = loopback_MultiTexCoord3dARB;
   dest->MultiTexCoord3dvARB = loopback_MultiTexCoord3dvARB;
   dest->MultiTexCoord3iARB = loopback_MultiTexCoord3iARB;
   dest->MultiTexCoord3ivARB = loopback_MultiTexCoord3ivARB;
   dest->MultiTexCoord3sARB = loopback_MultiTexCoord3sARB;
   dest->MultiTexCoord3svARB = loopback_MultiTexCoord3svARB;
   dest->MultiTexCoord4dARB = loopback_MultiTexCoord4dARB;
   dest->MultiTexCoord4dvARB = loopback_MultiTexCoord4dvARB;
   dest->MultiTexCoord4iARB = loopback_MultiTexCoord4iARB;
   dest->MultiTexCoord4ivARB = loopback_MultiTexCoord4ivARB;
   dest->MultiTexCoord4sARB = loopback_MultiTexCoord4sARB;
   dest->MultiTexCoord4svARB = loopback_MultiTexCoord4svARB;
   dest->EvalCoord2dv = loopback_EvalCoord2dv;
   dest->EvalCoord2fv = loopback_EvalCoord2fv;
   dest->EvalCoord2d = loopback_EvalCoord2d;
   dest->EvalCoord1dv = loopback_EvalCoord1dv;
   dest->EvalCoord1fv = loopback_EvalCoord1fv;
   dest->EvalCoord1d = loopback_EvalCoord1d;
   dest->Materialf = loopback_Materialf;
   dest->Materiali = loopback_Materiali;
   dest->Materialiv = loopback_Materialiv;
   dest->Rectd = loopback_Rectd;
   dest->Rectdv = loopback_Rectdv;
   dest->Rectfv = loopback_Rectfv;
   dest->Recti = loopback_Recti;
   dest->Rectiv = loopback_Rectiv;
   dest->Rects = loopback_Rects;
   dest->Rectsv = loopback_Rectsv;
   dest->FogCoorddEXT = loopback_FogCoorddEXT;
   dest->FogCoorddvEXT = loopback_FogCoorddvEXT;
}
