
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 *    Gareth Hughes
 */

#ifndef PRE_LOOPBACK
#define PRE_LOOPBACK( FUNC )
#endif

static void TAG(ArrayElement)( GLint i )
{
   PRE_LOOPBACK( ArrayElement );
   _glapi_Dispatch->ArrayElement( i );
}

static void TAG(Color3f)( GLfloat r, GLfloat g, GLfloat b )
{
   PRE_LOOPBACK( Color3f );
   _glapi_Dispatch->Color3f( r, g, b );
}

static void TAG(Color3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color3fv );
   _glapi_Dispatch->Color3fv( v );
}

static void TAG(Color3ub)( GLubyte r, GLubyte g, GLubyte b )
{
   PRE_LOOPBACK( Color3ub );
   _glapi_Dispatch->Color3ub( r, g, b );
}

static void TAG(Color3ubv)( const GLubyte *v )
{
   PRE_LOOPBACK( Color3ubv );
   _glapi_Dispatch->Color3ubv( v );
}

static void TAG(Color4f)( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   PRE_LOOPBACK( Color4f );
   _glapi_Dispatch->Color4f( r, g, b, a );
}

static void TAG(Color4fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color4fv );
   _glapi_Dispatch->Color4fv( v );
}

static void TAG(Color4ub)( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   PRE_LOOPBACK( Color4ub );
   _glapi_Dispatch->Color4ub( r, g, b, a );
}

static void TAG(Color4ubv)( const GLubyte *v )
{
   PRE_LOOPBACK( Color4ubv );
   _glapi_Dispatch->Color4ubv( v );
}

static void TAG(EdgeFlag)( GLboolean e )
{
   PRE_LOOPBACK( EdgeFlag );
   _glapi_Dispatch->EdgeFlag( e );
}

static void TAG(EdgeFlagv)( const GLboolean *v )
{
   PRE_LOOPBACK( EdgeFlagv );
   _glapi_Dispatch->EdgeFlagv( v );
}

static void TAG(EvalCoord1f)( GLfloat s )
{
   PRE_LOOPBACK( EvalCoord1f );
   _glapi_Dispatch->EvalCoord1f( s );
}

static void TAG(EvalCoord1fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord1fv );
   _glapi_Dispatch->EvalCoord1fv( v );
}

static void TAG(EvalCoord2f)( GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( EvalCoord2f );
   _glapi_Dispatch->EvalCoord2f( s, t );
}

static void TAG(EvalCoord2fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord2fv );
   _glapi_Dispatch->EvalCoord2fv( v );
}

static void TAG(EvalPoint1)( GLint i )
{
   PRE_LOOPBACK( EvalPoint1 );
   _glapi_Dispatch->EvalPoint1( i );
}

static void TAG(EvalPoint2)( GLint i, GLint j )
{
   PRE_LOOPBACK( EvalPoint2 );
   _glapi_Dispatch->EvalPoint2( i, j );
}

static void TAG(FogCoordfEXT)( GLfloat f )
{
   PRE_LOOPBACK( FogCoordfEXT );
   _glapi_Dispatch->FogCoordfEXT( f );
}

static void TAG(FogCoordfvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( FogCoordfvEXT );
   _glapi_Dispatch->FogCoordfvEXT( v );
}

static void TAG(Indexi)( GLint i )
{
   PRE_LOOPBACK( Indexi );
   _glapi_Dispatch->Indexi( i );
}

static void TAG(Indexiv)( const GLint *v )
{
   PRE_LOOPBACK( Indexiv );
   _glapi_Dispatch->Indexiv( v );
}

static void TAG(Materialfv)( GLenum face, GLenum pname, const GLfloat *v )
{
   PRE_LOOPBACK( Materialfv );
   _glapi_Dispatch->Materialfv( face, pname, v );
}

static void TAG(MultiTexCoord1fARB)( GLenum target, GLfloat a )
{
   PRE_LOOPBACK( MultiTexCoord1fARB );
   _glapi_Dispatch->MultiTexCoord1fARB( target, a );
}

static void TAG(MultiTexCoord1fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord1fvARB );
   _glapi_Dispatch->MultiTexCoord1fvARB( target, tc );
}

static void TAG(MultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( MultiTexCoord2fARB );
   _glapi_Dispatch->MultiTexCoord2fARB( target, s, t );
}

static void TAG(MultiTexCoord2fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord2fvARB );
   _glapi_Dispatch->MultiTexCoord2fvARB( target, tc );
}

static void TAG(MultiTexCoord3fARB)( GLenum target, GLfloat s,
				     GLfloat t, GLfloat r )
{
   PRE_LOOPBACK( MultiTexCoord3fARB );
   _glapi_Dispatch->MultiTexCoord3fARB( target, s, t, r );
}

static void TAG(MultiTexCoord3fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord3fvARB );
   _glapi_Dispatch->MultiTexCoord3fvARB( target, tc );
}

static void TAG(MultiTexCoord4fARB)( GLenum target, GLfloat s,
				     GLfloat t, GLfloat r, GLfloat q )
{
   PRE_LOOPBACK( MultiTexCoord4fARB );
   _glapi_Dispatch->MultiTexCoord4fARB( target, s, t, r, q );
}

static void TAG(MultiTexCoord4fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord4fvARB );
   _glapi_Dispatch->MultiTexCoord4fvARB( target, tc );
}

static void TAG(Normal3f)( GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( Normal3f );
   _glapi_Dispatch->Normal3f( x, y, z );
}

static void TAG(Normal3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Normal3fv );
   _glapi_Dispatch->Normal3fv( v );
}

static void TAG(SecondaryColor3fEXT)( GLfloat r, GLfloat g, GLfloat b )
{
   PRE_LOOPBACK( SecondaryColor3fEXT );
   _glapi_Dispatch->SecondaryColor3fEXT( r, g, b );
}

static void TAG(SecondaryColor3fvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( SecondaryColor3fvEXT );
   _glapi_Dispatch->SecondaryColor3fvEXT( v );
}

static void TAG(SecondaryColor3ubEXT)( GLubyte r, GLubyte g, GLubyte b )
{
   PRE_LOOPBACK( SecondaryColor3ubEXT );
   _glapi_Dispatch->SecondaryColor3ubEXT( r, g, b );
}

static void TAG(SecondaryColor3ubvEXT)( const GLubyte *v )
{
   PRE_LOOPBACK( SecondaryColor3ubvEXT );
   _glapi_Dispatch->SecondaryColor3ubvEXT( v );
}

static void TAG(TexCoord1f)( GLfloat s )
{
   PRE_LOOPBACK( TexCoord1f );
   _glapi_Dispatch->TexCoord1f( s );
}

static void TAG(TexCoord1fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord1fv );
   _glapi_Dispatch->TexCoord1fv( tc );
}

static void TAG(TexCoord2f)( GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( TexCoord2f );
   _glapi_Dispatch->TexCoord2f( s, t );
}

static void TAG(TexCoord2fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord2fv );
   _glapi_Dispatch->TexCoord2fv( tc );
}

static void TAG(TexCoord3f)( GLfloat s, GLfloat t, GLfloat r )
{
   PRE_LOOPBACK( TexCoord3f );
   _glapi_Dispatch->TexCoord3f( s, t, r );
}

static void TAG(TexCoord3fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord3fv );
   _glapi_Dispatch->TexCoord3fv( tc );
}

static void TAG(TexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   PRE_LOOPBACK( TexCoord4f );
   _glapi_Dispatch->TexCoord4f( s, t, r, q );
}

static void TAG(TexCoord4fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord4fv );
   _glapi_Dispatch->TexCoord4fv( tc );
}

static void TAG(Vertex2f)( GLfloat x, GLfloat y )
{
   PRE_LOOPBACK( Vertex2f );
   _glapi_Dispatch->Vertex2f( x, y );
}

static void TAG(Vertex2fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex2fv );
   _glapi_Dispatch->Vertex2fv( v );
}

static void TAG(Vertex3f)( GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( Vertex3f );
   _glapi_Dispatch->Vertex3f( x, y, z );
}

static void TAG(Vertex3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex3fv );
   _glapi_Dispatch->Vertex3fv( v );
}

static void TAG(Vertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   PRE_LOOPBACK( Vertex4f );
   _glapi_Dispatch->Vertex4f( x, y, z, w );
}

static void TAG(Vertex4fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex4fv );
   _glapi_Dispatch->Vertex4fv( v );
}

static void TAG(CallList)( GLuint i )
{
   PRE_LOOPBACK( CallList );
   _glapi_Dispatch->CallList( i );
}

static void TAG(Begin)( GLenum mode )
{
   PRE_LOOPBACK( Begin );
   _glapi_Dispatch->Begin( mode );
}

static void TAG(End)( void )
{
   PRE_LOOPBACK( End );
   _glapi_Dispatch->End();
}

static void TAG(Rectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   PRE_LOOPBACK( Rectf );
   _glapi_Dispatch->Rectf( x1, y1, x2, y2 );
}

static void TAG(DrawArrays)( GLenum mode, GLint start, GLsizei count )
{
   PRE_LOOPBACK( DrawArrays );
   _glapi_Dispatch->DrawArrays( mode, start, count );
}

static void TAG(DrawElements)( GLenum mode, GLsizei count, GLenum type,
			       const void *indices )
{
   PRE_LOOPBACK( DrawElements );
   _glapi_Dispatch->DrawElements( mode, count, type, indices );
}

static void TAG(DrawRangeElements)( GLenum mode, GLuint start,
				    GLuint end, GLsizei count,
				    GLenum type, const void *indices )
{
   PRE_LOOPBACK( DrawRangeElements );
   _glapi_Dispatch->DrawRangeElements( mode, start, end, count, type, indices );
}

static void TAG(EvalMesh1)( GLenum mode, GLint i1, GLint i2 )
{
   PRE_LOOPBACK( EvalMesh1 );
   _glapi_Dispatch->EvalMesh1( mode, i1, i2 );
}

static void TAG(EvalMesh2)( GLenum mode, GLint i1, GLint i2,
			    GLint j1, GLint j2 )
{
   PRE_LOOPBACK( EvalMesh2 );
   _glapi_Dispatch->EvalMesh2( mode, i1, i2, j1, j2 );
}

static void TAG(VertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   PRE_LOOPBACK( VertexAttrib4fNV );
   _glapi_Dispatch->VertexAttrib4fNV( index, x, y, z, w );
}

static void TAG(VertexAttrib4fvNV)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib4fNV );
   _glapi_Dispatch->VertexAttrib4fvNV( index, v );
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
   TAG(VertexAttrib4fNV),
   TAG(VertexAttrib4fvNV),
   TAG(Rectf),
   TAG(DrawArrays),
   TAG(DrawElements),
   TAG(DrawRangeElements),
   TAG(EvalMesh1),
   TAG(EvalMesh2)
};

#undef TAG
#undef PRE_LOOPBACK
