/* $Id: vtxfmt_tmp.h,v 1.4 2001/03/12 00:48:39 gareth Exp $ */

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
 *    Gareth Hughes <gareth@valinux.com>
 */

#ifndef PRE_LOOPBACK
#define PRE_LOOPBACK( FUNC )
#endif

static void TAG(ArrayElement)( GLint i )
{
   PRE_LOOPBACK( ArrayElement );
   glArrayElement( i );
}

static void TAG(Color3f)( GLfloat a, GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( Color3f );
   glColor3f( a, b, c );
}

static void TAG(Color3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color3fv );
   glColor3fv( v );
}

static void TAG(Color3ub)( GLubyte a, GLubyte b, GLubyte c )
{
   PRE_LOOPBACK( Color3ub );
   glColor3ub( a, b, c );
}

static void TAG(Color3ubv)( const GLubyte *v )
{
   PRE_LOOPBACK( Color3ubv );
   glColor3ubv( v );
}

static void TAG(Color4f)( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   PRE_LOOPBACK( Color4f );
   glColor4f( a, b, c, d );
}

static void TAG(Color4fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color4fv );
   glColor4fv( v );
}

static void TAG(Color4ub)( GLubyte a, GLubyte b, GLubyte c, GLubyte d )
{
   PRE_LOOPBACK( Color4ub );
   glColor4ub( a, b, c, d );
}

static void TAG(Color4ubv)( const GLubyte *v )
{
   PRE_LOOPBACK( Color4ubv );
   glColor4ubv( v );
}

static void TAG(EdgeFlag)( GLboolean a )
{
   PRE_LOOPBACK( EdgeFlag );
   glEdgeFlag( a );
}

static void TAG(EdgeFlagv)( const GLboolean *v )
{
   PRE_LOOPBACK( EdgeFlagv );
   glEdgeFlagv( v );
}

static void TAG(EvalCoord1f)( GLfloat a )
{
   PRE_LOOPBACK( EvalCoord1f );
   glEvalCoord1f( a );
}

static void TAG(EvalCoord1fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord1fv );
   glEvalCoord1fv( v );
}

static void TAG(EvalCoord2f)( GLfloat a, GLfloat b )
{
   PRE_LOOPBACK( EvalCoord2f );
   glEvalCoord2f( a, b );
}

static void TAG(EvalCoord2fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord2fv );
   glEvalCoord2fv( v );
}

static void TAG(EvalPoint1)( GLint a )
{
   PRE_LOOPBACK( EvalPoint1 );
   glEvalPoint1( a );
}

static void TAG(EvalPoint2)( GLint a, GLint b )
{
   PRE_LOOPBACK( EvalPoint2 );
   glEvalPoint2( a, b );
}

static void TAG(FogCoordfEXT)( GLfloat a )
{
   PRE_LOOPBACK( FogCoordfEXT );
   glFogCoordfEXT( a );
}

static void TAG(FogCoordfvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( FogCoordfvEXT );
   glFogCoordfvEXT( v );
}

static void TAG(Indexi)( GLint a )
{
   PRE_LOOPBACK( Indexi );
   glIndexi( a );
}

static void TAG(Indexiv)( const GLint *v )
{
   PRE_LOOPBACK( Indexiv );
   glIndexiv( v );
}

static void TAG(Materialfv)( GLenum face, GLenum pname, const GLfloat *v )
{
   PRE_LOOPBACK( Materialfv );
   glMaterialfv( face, pname, v );
}

static void TAG(MultiTexCoord1fARB)( GLenum target, GLfloat a )
{
   PRE_LOOPBACK( MultiTexCoord1fARB );
   glMultiTexCoord1fARB( target, a );
}

static void TAG(MultiTexCoord1fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord1fvARB );
   glMultiTexCoord1fvARB( target, tc );
}

static void TAG(MultiTexCoord2fARB)( GLenum target, GLfloat a, GLfloat b )
{
   PRE_LOOPBACK( MultiTexCoord2fARB );
   glMultiTexCoord2fARB( target, a, b );
}

static void TAG(MultiTexCoord2fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord2fvARB );
   glMultiTexCoord2fvARB( target, tc );
}

static void TAG(MultiTexCoord3fARB)( GLenum target, GLfloat a,
				     GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( MultiTexCoord3fARB );
   glMultiTexCoord3fARB( target, a, b, c );
}

static void TAG(MultiTexCoord3fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord3fvARB );
   glMultiTexCoord3fvARB( target, tc );
}

static void TAG(MultiTexCoord4fARB)( GLenum target, GLfloat a,
				     GLfloat b, GLfloat c, GLfloat d )
{
   PRE_LOOPBACK( MultiTexCoord4fARB );
   glMultiTexCoord4fARB( target, a, b, c, d );
}

static void TAG(MultiTexCoord4fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord4fvARB );
   glMultiTexCoord4fvARB( target, tc );
}

static void TAG(Normal3f)( GLfloat a, GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( Normal3f );
   glNormal3f( a, b, c );
}

static void TAG(Normal3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Normal3fv );
   glNormal3fv( v );
}

static void TAG(SecondaryColor3fEXT)( GLfloat a, GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( SecondaryColor3fEXT );
   glSecondaryColor3fEXT( a, b, c );
}

static void TAG(SecondaryColor3fvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( SecondaryColor3fvEXT );
   glSecondaryColor3fvEXT( v );
}

static void TAG(SecondaryColor3ubEXT)( GLubyte a, GLubyte b, GLubyte c )
{
   PRE_LOOPBACK( SecondaryColor3ubEXT );
   glSecondaryColor3ubEXT( a, b, c );
}

static void TAG(SecondaryColor3ubvEXT)( const GLubyte *v )
{
   PRE_LOOPBACK( SecondaryColor3ubvEXT );
   glSecondaryColor3ubvEXT( v );
}

static void TAG(TexCoord1f)( GLfloat a )
{
   PRE_LOOPBACK( TexCoord1f );
   glTexCoord1f( a );
}

static void TAG(TexCoord1fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord1fv );
   glTexCoord1fv( tc );
}

static void TAG(TexCoord2f)( GLfloat a, GLfloat b )
{
   PRE_LOOPBACK( TexCoord2f );
   glTexCoord2f( a, b );
}

static void TAG(TexCoord2fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord2fv );
   glTexCoord2fv( tc );
}

static void TAG(TexCoord3f)( GLfloat a, GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( TexCoord3f );
   glTexCoord3f( a, b, c );
}

static void TAG(TexCoord3fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord3fv );
   glTexCoord3fv( tc );
}

static void TAG(TexCoord4f)( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   PRE_LOOPBACK( TexCoord4f );
   glTexCoord4f( a, b, c, d );
}

static void TAG(TexCoord4fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord4fv );
   glTexCoord4fv( tc );
}

static void TAG(Vertex2f)( GLfloat a, GLfloat b )
{
   PRE_LOOPBACK( Vertex2f );
   glVertex2f( a, b );
}

static void TAG(Vertex2fv)( const GLfloat *obj )
{
   PRE_LOOPBACK( Vertex2fv );
   glVertex2fv( obj );
}

static void TAG(Vertex3f)( GLfloat a, GLfloat b, GLfloat c )
{
   PRE_LOOPBACK( Vertex3f );
   glVertex3f( a, b, c );
}

static void TAG(Vertex3fv)( const GLfloat *obj )
{
   PRE_LOOPBACK( Vertex3fv );
   glVertex3fv( obj );
}

static void TAG(Vertex4f)( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   PRE_LOOPBACK( Vertex4f );
   glVertex4f( a, b, c, d );
}

static void TAG(Vertex4fv)( const GLfloat *obj )
{
   PRE_LOOPBACK( Vertex4fv );
   glVertex4fv( obj );
}

static void TAG(CallList)( GLuint i )
{
   PRE_LOOPBACK( CallList );
   glCallList( i );
}

static void TAG(Begin)( GLenum mode )
{
   PRE_LOOPBACK( Begin );
   glBegin( mode );
}

static void TAG(End)( void )
{
   PRE_LOOPBACK( End );
   glEnd();
}

static void TAG(Rectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   PRE_LOOPBACK( Rectf );
   glRectf( x1, y1, x2, y2 );
}

static void TAG(DrawArrays)( GLenum mode, GLint start, GLsizei count )
{
   PRE_LOOPBACK( DrawArrays );
   glDrawArrays( mode, start, count );
}

static void TAG(DrawElements)( GLenum mode, GLsizei count, GLenum type,
			       const void *indices )
{
   PRE_LOOPBACK( DrawElements );
   glDrawElements( mode, count, type, indices );
}

static void TAG(DrawRangeElements)( GLenum mode, GLuint start,
				    GLuint end, GLsizei count,
				    GLenum type, const void *indices )
{
   PRE_LOOPBACK( DrawRangeElements );
   glDrawRangeElements( mode, start, end, count, type, indices );
}

static void TAG(EvalMesh1)( GLenum mode, GLint i1, GLint i2 )
{
   PRE_LOOPBACK( EvalMesh1 );
   glEvalMesh1( mode, i1, i2 );
}

static void TAG(EvalMesh2)( GLenum mode, GLint i1, GLint i2,
			    GLint j1, GLint j2 )
{
   PRE_LOOPBACK( EvalMesh2 );
   glEvalMesh2( mode, i1, i2, j1, j2 );
}


static GLvertexformat TAG(vtxfmt) = {
   TAG(ArrayElement),
   TAG(Color3f),
   TAG(Color3fv),
   TAG(Color3ub),
   TAG(Color3ubv),
   TAG(Color4f),
   TAG(Color4fv),
   TAG(Color4ub),
   TAG(Color4ubv),
   TAG(EdgeFlag),
   TAG(EdgeFlagv),
   TAG(EvalCoord1f),
   TAG(EvalCoord1fv),
   TAG(EvalCoord2f),
   TAG(EvalCoord2fv),
   TAG(EvalPoint1),
   TAG(EvalPoint2),
   TAG(FogCoordfEXT),
   TAG(FogCoordfvEXT),
   TAG(Indexi),
   TAG(Indexiv),
   TAG(Materialfv),
   TAG(MultiTexCoord1fARB),
   TAG(MultiTexCoord1fvARB),
   TAG(MultiTexCoord2fARB),
   TAG(MultiTexCoord2fvARB),
   TAG(MultiTexCoord3fARB),
   TAG(MultiTexCoord3fvARB),
   TAG(MultiTexCoord4fARB),
   TAG(MultiTexCoord4fvARB),
   TAG(Normal3f),
   TAG(Normal3fv),
   TAG(SecondaryColor3fEXT),
   TAG(SecondaryColor3fvEXT),
   TAG(SecondaryColor3ubEXT),
   TAG(SecondaryColor3ubvEXT),
   TAG(TexCoord1f),
   TAG(TexCoord1fv),
   TAG(TexCoord2f),
   TAG(TexCoord2fv),
   TAG(TexCoord3f),
   TAG(TexCoord3fv),
   TAG(TexCoord4f),
   TAG(TexCoord4fv),
   TAG(Vertex2f),
   TAG(Vertex2fv),
   TAG(Vertex3f),
   TAG(Vertex3fv),
   TAG(Vertex4f),
   TAG(Vertex4fv),
   TAG(CallList),
   TAG(Begin),
   TAG(End),
   TAG(Rectf),
   TAG(DrawArrays),
   TAG(DrawElements),
   TAG(DrawRangeElements),
   TAG(EvalMesh1),
   TAG(EvalMesh2),
};

#undef TAG
#undef PRE_LOOPBACK
