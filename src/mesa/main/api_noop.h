
#ifndef _API_NOOP_H
#define _API_NOOP_H


#include "glheader.h"
#include "mtypes.h"
#include "context.h"

/* In states where certain vertex components are required for t&l or
 * rasterization, we still need to keep track of the current values.
 * These functions provide this service by keeping uptodate the
 * 'ctx->Current' struct for all data elements not included in the
 * currently enabled hardware vertex.
 *
 */
extern void _mesa_noop_EdgeFlag( GLboolean b );

extern void _mesa_noop_EdgeFlagv( const GLboolean *b );

extern void _mesa_noop_FogCoordfEXT( GLfloat a );

extern void _mesa_noop_FogCoordfvEXT( const GLfloat *v );

extern void _mesa_noop_Indexi( GLint i );

extern void _mesa_noop_Indexiv( const GLint *v );

extern void _mesa_noop_Normal3f( GLfloat a, GLfloat b, GLfloat c );

extern void _mesa_noop_Normal3fv( const GLfloat *v );

extern void _mesa_noop_Materialfv(  GLenum face, GLenum pname,  const GLfloat *param );

extern void _mesa_noop_Color4ub( GLubyte a, GLubyte b, GLubyte c, GLubyte d );

extern void _mesa_noop_Color4ubv( const GLubyte *v );

extern void _mesa_noop_Color4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d );

extern void _mesa_noop_Color4fv( const GLfloat *v );

extern void _mesa_noop_Color3ub( GLubyte a, GLubyte b, GLubyte c );

extern void _mesa_noop_Color3ubv( const GLubyte *v );

extern void _mesa_noop_Color3f( GLfloat a, GLfloat b, GLfloat c );

extern void _mesa_noop_Color3fv( const GLfloat *v );

extern void _mesa_noop_MultiTexCoord1fARB( GLenum target, GLfloat a );

extern void _mesa_noop_MultiTexCoord1fvARB( GLenum target, GLfloat *v );

extern void _mesa_noop_MultiTexCoord2fARB( GLenum target, GLfloat a, 
					   GLfloat b );

extern void _mesa_noop_MultiTexCoord2fvARB( GLenum target, GLfloat *v );

extern void _mesa_noop_MultiTexCoord3fARB( GLenum target, GLfloat a, 
					GLfloat b, GLfloat c);

extern void _mesa_noop_MultiTexCoord3fvARB( GLenum target, GLfloat *v );

extern void _mesa_noop_MultiTexCoord4fARB( GLenum target, GLfloat a, 
					GLfloat b, GLfloat c, GLfloat d );

extern void _mesa_noop_MultiTexCoord4fvARB( GLenum target, GLfloat *v );

extern void _mesa_noop_SecondaryColor3ubEXT( GLubyte a, GLubyte b, GLubyte c );

extern void _mesa_noop_SecondaryColor3ubvEXT( const GLubyte *v );

extern void _mesa_noop_SecondaryColor3fEXT( GLfloat a, GLfloat b, GLfloat c );

extern void _mesa_noop_SecondaryColor3fvEXT( const GLfloat *v );

extern void _mesa_noop_TexCoord1f( GLfloat a );

extern void _mesa_noop_TexCoord1fv( GLfloat *v );

extern void _mesa_noop_TexCoord2f( GLfloat a, GLfloat b );

extern void _mesa_noop_TexCoord2fv( GLfloat *v );

extern void _mesa_noop_TexCoord3f( GLfloat a, GLfloat b, GLfloat c );

extern void _mesa_noop_TexCoord3fv( GLfloat *v );

extern void _mesa_noop_TexCoord4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d );

extern void _mesa_noop_TexCoord4fv( GLfloat *v );


/* Not strictly a noop -- translate Rectf down to Begin/End and
 * vertices.  Closer to the loopback operations, but doesn't meet the
 * criteria for inclusion there (cannot be used in the Save table).  
 */
extern void _mesa_noop_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );

#endif
