#ifndef SWRAST_H
#define SWRAST_H

#include "types.h"

/* These are the functions exported from swrast.  (more to come)
 */
void
_swrast_alloc_buffers( GLcontext *ctx );


void 
_swrast_Bitmap( GLcontext *ctx, 
		GLint px, GLint py,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap );

void
_swrast_CopyPixels( GLcontext *ctx, 
		    GLint srcx, GLint srcy, 
		    GLint destx, GLint desty, 
		    GLsizei width, GLsizei height,
		    GLenum type );

void
_swrast_DrawPixels( GLcontext *ctx, 
		    GLint x, GLint y,
		    GLsizei width, GLsizei height,
		    GLenum format, GLenum type, 
		    const struct gl_pixelstore_attrib *unpack,
		    const GLvoid *pixels );

void
_swrast_ReadPixels( GLcontext *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *unpack,
		    GLvoid *pixels );

void 
_swrast_Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
	       GLint x, GLint y, GLint width, GLint height );

void
_swrast_Accum( GLcontext *ctx, GLenum op, 
	       GLfloat value, GLint xpos, GLint ypos, 
	       GLint width, GLint height );

void 
_swrast_set_line_function( GLcontext *ctx );

void 
_swrast_set_point_function( GLcontext *ctx );

void 
_swrast_set_triangle_function( GLcontext *ctx );

void 
_swrast_set_quad_function( GLcontext *ctx );

void 
_swrast_flush( GLcontext *ctx );

GLboolean
_swrast_create_context( GLcontext *ctx );

void
_swrast_destroy_context( GLcontext *ctx );


/* Replace:
 */
void
_swrast_set_texture_sampler( struct gl_texture_object *t );



#define _SWRAST_NEW_TRIANGLE (_NEW_RENDERMODE|		\
                              _NEW_POLYGON|		\
                              _NEW_DEPTH|		\
                              _NEW_STENCIL|		\
                              _NEW_COLOR|		\
                              _NEW_TEXTURE|		\
                              _NEW_HINT|		\
                              _SWRAST_NEW_RASTERMASK|	\
                              _NEW_LIGHT|		\
                              _NEW_FOG)

#define _SWRAST_NEW_LINE (_NEW_RENDERMODE|	\
                          _NEW_LINE|		\
                          _NEW_TEXTURE|		\
                          _NEW_LIGHT|		\
                          _NEW_FOG|		\
                          _NEW_DEPTH)

#define _SWRAST_NEW_POINT (_NEW_RENDERMODE |	\
			   _NEW_POINT |		\
			   _NEW_TEXTURE |	\
			   _NEW_LIGHT |		\
			   _NEW_FOG)






#endif
