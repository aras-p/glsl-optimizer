
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "api_noop.h"
#include "api_validate.h"
#include "context.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "mtypes.h"


/* In states where certain vertex components are required for t&l or
 * rasterization, we still need to keep track of the current values.
 * These functions provide this service by keeping uptodate the
 * 'ctx->Current' struct for all data elements not included in the
 * currently enabled hardware vertex.
 *
 */
void _mesa_noop_EdgeFlag( GLboolean b )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.EdgeFlag = b;
}

void _mesa_noop_EdgeFlagv( const GLboolean *b )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.EdgeFlag = *b;
}

void _mesa_noop_FogCoordfEXT( GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.Attrib[VERT_ATTRIB_FOG][0] = a;
}

void _mesa_noop_FogCoordfvEXT( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.Attrib[VERT_ATTRIB_FOG][0] = *v;
}

void _mesa_noop_Indexi( GLint i )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.Index = i;
}

void _mesa_noop_Indexiv( const GLint *v )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.Index = *v;
}

void _mesa_noop_Normal3f( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_NORMAL];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
}

void _mesa_noop_Normal3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_NORMAL];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
}

void _mesa_noop_Materialfv( GLenum face, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i, nr;
   struct gl_material *mat = &ctx->Light.Material;
   GLuint bitmask = _mesa_material_bitmask( ctx, face, pname, ~0,
                                            "_mesa_noop_Materialfv" );

   if (ctx->Light.ColorMaterialEnabled)
      bitmask &= ~ctx->Light.ColorMaterialBitmask;

   if (bitmask == 0)
      return;

   switch (face) {
   case GL_SHININESS: nr = 1; break;
   case GL_COLOR_INDEXES: nr = 3; break;
   default: nr = 4 ; break;
   }

   for (i = 0 ; i < MAT_ATTRIB_MAX ; i++) 
      if (bitmask & (1<<i))
	 COPY_SZ_4V( mat->Attrib[i], nr, params ); 

   _mesa_update_material( ctx, bitmask );
}

void _mesa_noop_Color4ub( GLubyte a, GLubyte b, GLubyte c, GLubyte d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = UBYTE_TO_FLOAT(a);
   color[1] = UBYTE_TO_FLOAT(b);
   color[2] = UBYTE_TO_FLOAT(c);
   color[3] = UBYTE_TO_FLOAT(d);
}

void _mesa_noop_Color4ubv( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = UBYTE_TO_FLOAT(v[0]);
   color[1] = UBYTE_TO_FLOAT(v[1]);
   color[2] = UBYTE_TO_FLOAT(v[2]);
   color[3] = UBYTE_TO_FLOAT(v[3]);
}

void _mesa_noop_Color4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = d;
}

void _mesa_noop_Color4fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = v[3];
}

void _mesa_noop_Color3ub( GLubyte a, GLubyte b, GLubyte c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = UBYTE_TO_FLOAT(a);
   color[1] = UBYTE_TO_FLOAT(b);
   color[2] = UBYTE_TO_FLOAT(c);
   color[3] = 1.0;
}

void _mesa_noop_Color3ubv( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = UBYTE_TO_FLOAT(v[0]);
   color[1] = UBYTE_TO_FLOAT(v[1]);
   color[2] = UBYTE_TO_FLOAT(v[2]);
   color[3] = 1.0;
}

void _mesa_noop_Color3f( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = 1.0;
}

void _mesa_noop_Color3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = 1.0;
}

void _mesa_noop_MultiTexCoord1fARB( GLenum target, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], a);
      dest[1] = 0;
      dest[2] = 0;
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord1fvARB( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], v[0]);
      dest[1] = 0;
      dest[2] = 0;
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord2fARB( GLenum target, GLfloat a, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      dest[2] = 0;
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord2fvARB( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], v[0]);
      COPY_FLOAT(dest[1], v[1]);
      dest[2] = 0;
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord3fARB( GLenum target, GLfloat a, GLfloat b, GLfloat c)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      COPY_FLOAT(dest[2], c);
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord3fvARB( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], v[0]);
      COPY_FLOAT(dest[1], v[1]);
      COPY_FLOAT(dest[2], v[2]);
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord4fARB( GLenum target, GLfloat a, GLfloat b,
			      GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      COPY_FLOAT(dest[2], c);
      dest[3] = d;
   }
}

void _mesa_noop_MultiTexCoord4fvARB( GLenum target, const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_COORD_UNITS)
   {
      GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit];
      COPY_FLOAT(dest[0], v[0]);
      COPY_FLOAT(dest[1], v[1]);
      COPY_FLOAT(dest[2], v[2]);
      COPY_FLOAT(dest[3], v[3]);
   }
}

void _mesa_noop_SecondaryColor3ubEXT( GLubyte a, GLubyte b, GLubyte c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
   color[0] = UBYTE_TO_FLOAT(a);
   color[1] = UBYTE_TO_FLOAT(b);
   color[2] = UBYTE_TO_FLOAT(c);
   color[3] = 1.0;
}

void _mesa_noop_SecondaryColor3ubvEXT( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
   color[0] = UBYTE_TO_FLOAT(v[0]);
   color[1] = UBYTE_TO_FLOAT(v[1]);
   color[2] = UBYTE_TO_FLOAT(v[2]);
   color[3] = 1.0;
}

void _mesa_noop_SecondaryColor3fEXT( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = 1.0;
}

void _mesa_noop_SecondaryColor3fvEXT( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *color = ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = 1.0;
}

void _mesa_noop_TexCoord1f( GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], a);
   dest[1] = 0;
   dest[2] = 0;
   dest[3] = 1;
}

void _mesa_noop_TexCoord1fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], v[0]);
   dest[1] = 0;
   dest[2] = 0;
   dest[3] = 1;
}

void _mesa_noop_TexCoord2f( GLfloat a, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   dest[2] = 0;
   dest[3] = 1;
}

void _mesa_noop_TexCoord2fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   dest[2] = 0;
   dest[3] = 1;
}

void _mesa_noop_TexCoord3f( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
   dest[3] = 1;
}

void _mesa_noop_TexCoord3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
   dest[3] = 1;
}

void _mesa_noop_TexCoord4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
   COPY_FLOAT(dest[3], d);
}

void _mesa_noop_TexCoord4fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
   COPY_FLOAT(dest[3], v[3]);
}

/* Useful outside begin/end?
 */
void _mesa_noop_Vertex2fv( const GLfloat *v )
{
   (void) v;
}

void _mesa_noop_Vertex3fv( const GLfloat *v )
{
   (void) v;
}

void _mesa_noop_Vertex4fv( const GLfloat *v )
{
   (void) v;
}

void _mesa_noop_Vertex2f( GLfloat a, GLfloat b )
{
   (void) a; (void) b;
}

void _mesa_noop_Vertex3f( GLfloat a, GLfloat b, GLfloat c )
{
   (void) a; (void) b; (void) c;
}

void _mesa_noop_Vertex4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   (void) a; (void) b; (void) c; (void) d;
}



void _mesa_noop_VertexAttrib4fNV( GLuint index, GLfloat x,
                                  GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < 16) {
      ASSIGN_4V(ctx->Current.Attrib[index], x, y, z, w);
   }
}

void _mesa_noop_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < 16) {
      ASSIGN_4V(ctx->Current.Attrib[index], v[0], v[1], v[2], v[3]);
   }
}



/* Execute a glRectf() function.  This is not suitable for GL_COMPILE
 * modes (as the test for outside begin/end is not compiled),
 * but may be useful for drivers in circumstances which exclude
 * display list interactions.
 *
 * (None of the functions in this file are suitable for GL_COMPILE
 * modes).
 */
void _mesa_noop_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   {
      GET_CURRENT_CONTEXT(ctx);
      ASSERT_OUTSIDE_BEGIN_END(ctx);
   }

   glBegin( GL_QUADS );
   glVertex2f( x1, y1 );
   glVertex2f( x2, y1 );
   glVertex2f( x2, y2 );
   glVertex2f( x1, y2 );
   glEnd();
}


/* Some very basic support for arrays.  Drivers without explicit array
 * support can hook these in, but still need to supply an array-elt
 * implementation.
 */
void _mesa_noop_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   glBegin(mode);
   for (i = start ; i <= count ; i++)
      glArrayElement( i );
   glEnd();
}


void _mesa_noop_DrawElements(GLenum mode, GLsizei count, GLenum type,
			     const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   glBegin(mode);

   switch (type) {
   case GL_UNSIGNED_BYTE:
      for (i = 0 ; i < count ; i++)
	 glArrayElement( ((GLubyte *)indices)[i] );
      break;
   case GL_UNSIGNED_SHORT:
      for (i = 0 ; i < count ; i++)
	 glArrayElement( ((GLushort *)indices)[i] );
      break;
   case GL_UNSIGNED_INT:
      for (i = 0 ; i < count ; i++)
	 glArrayElement( ((GLuint *)indices)[i] );
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      break;
   }

   glEnd();
}

void _mesa_noop_DrawRangeElements(GLenum mode,
				  GLuint start, GLuint end,
				  GLsizei count, GLenum type,
				  const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);

   if (_mesa_validate_DrawRangeElements( ctx, mode,
					 start, end,
					 count, type, indices ))
      glDrawElements( mode, count, type, indices );
}
