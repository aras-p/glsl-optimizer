#include "glheader.h"
#include "api_noop.h"
#include "api_validate.h"
#include "context.h"
#include "colormac.h"
#include "light.h"
#include "macros.h"
#include "mmath.h"
#include "mtypes.h"

#ifdef __i386__
#define COPY_FLOAT(a,b) *(int*)&(a) = *(int*)&(b)
#else
#define COPY_FLOAT(a,b) (a) = (b)
#endif

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
   ctx->Current.FogCoord = a;
}

void _mesa_noop_FogCoordfvEXT( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Current.FogCoord = *v;
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
   GLfloat *dest = ctx->Current.Normal;
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
}

void _mesa_noop_Normal3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Normal;
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
}

void _mesa_noop_Materialfv(  GLenum face, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_material mat[2];
   GLuint bitmask = gl_material_bitmask( ctx, face, pname, ~0, 
					 "_mesa_noop_Materialfv" );
   if (bitmask == 0)
      return;

   if (bitmask & FRONT_AMBIENT_BIT) {
      COPY_4FV( mat[0].Ambient, params );
   }
   if (bitmask & BACK_AMBIENT_BIT) {
      COPY_4FV( mat[1].Ambient, params );
   }
   if (bitmask & FRONT_DIFFUSE_BIT) {
      COPY_4FV( mat[0].Diffuse, params );
   }
   if (bitmask & BACK_DIFFUSE_BIT) {
      COPY_4FV( mat[1].Diffuse, params );
   }
   if (bitmask & FRONT_SPECULAR_BIT) {
      COPY_4FV( mat[0].Specular, params );
   }
   if (bitmask & BACK_SPECULAR_BIT) {
      COPY_4FV( mat[1].Specular, params );
   }
   if (bitmask & FRONT_EMISSION_BIT) {
      COPY_4FV( mat[0].Emission, params );
   }
   if (bitmask & BACK_EMISSION_BIT) {
      COPY_4FV( mat[1].Emission, params );
   }
   if (bitmask & FRONT_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      mat[0].Shininess = shininess;
   }
   if (bitmask & BACK_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      mat[1].Shininess = shininess;
   }
   if (bitmask & FRONT_INDEXES_BIT) {
      mat[0].AmbientIndex = params[0];
      mat[0].DiffuseIndex = params[1];
      mat[0].SpecularIndex = params[2];
   }
   if (bitmask & BACK_INDEXES_BIT) {
      mat[1].AmbientIndex = params[0];
      mat[1].DiffuseIndex = params[1];
      mat[1].SpecularIndex = params[2];
   }

   gl_update_material( ctx, mat, bitmask );
}

void _mesa_noop_Color4ub( GLubyte a, GLubyte b, GLubyte c, GLubyte d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = d;
}

void _mesa_noop_Color4ubv( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   COPY_4UBV( color, v );
}

void _mesa_noop_Color4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   UNCLAMPED_FLOAT_TO_UBYTE(color[0], a);
   UNCLAMPED_FLOAT_TO_UBYTE(color[1], b);
   UNCLAMPED_FLOAT_TO_UBYTE(color[2], c);
   UNCLAMPED_FLOAT_TO_UBYTE(color[3], d);
}

void _mesa_noop_Color4fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   UNCLAMPED_FLOAT_TO_CHAN(color[0], v[0]);
   UNCLAMPED_FLOAT_TO_CHAN(color[1], v[1]);
   UNCLAMPED_FLOAT_TO_CHAN(color[2], v[2]);
}

void _mesa_noop_Color3ub( GLubyte a, GLubyte b, GLubyte c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = 255;
}

void _mesa_noop_Color3ubv( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = 255;
}

void _mesa_noop_Color3f( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   UNCLAMPED_FLOAT_TO_UBYTE(color[0], a);
   UNCLAMPED_FLOAT_TO_UBYTE(color[1], b);
   UNCLAMPED_FLOAT_TO_UBYTE(color[2], c);
   color[3] = 255;
}

void _mesa_noop_Color3fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.Color;
   UNCLAMPED_FLOAT_TO_CHAN(color[0], v[0]);
   UNCLAMPED_FLOAT_TO_CHAN(color[1], v[1]);
   UNCLAMPED_FLOAT_TO_CHAN(color[2], v[2]);
   color[3] = 255;
}

void _mesa_noop_MultiTexCoord1fARB( GLenum target, GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
      COPY_FLOAT(dest[0], a);
      dest[1] = 0;
      dest[2] = 0;
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord1fvARB( GLenum target, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
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
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      dest[2] = 0;	
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord2fvARB( GLenum target, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
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
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      COPY_FLOAT(dest[2], c);
      dest[3] = 1;
   }
}

void _mesa_noop_MultiTexCoord3fvARB( GLenum target, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
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
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
      COPY_FLOAT(dest[0], a);
      COPY_FLOAT(dest[1], b);
      COPY_FLOAT(dest[2], c);
      dest[3] = d;
   }
}

void _mesa_noop_MultiTexCoord4fvARB( GLenum target, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = target - GL_TEXTURE0_ARB;

   /* unit is unsigned -- cannot be less than zero.
    */
   if (unit < MAX_TEXTURE_UNITS)
   {
      GLfloat *dest = ctx->Current.Texcoord[unit];
      COPY_FLOAT(dest[0], v[0]);
      COPY_FLOAT(dest[1], v[1]);
      COPY_FLOAT(dest[2], v[2]);
      COPY_FLOAT(dest[3], v[3]);
   }
}

void _mesa_noop_SecondaryColor3ubEXT( GLubyte a, GLubyte b, GLubyte c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.SecondaryColor;
   color[0] = a;
   color[1] = b;
   color[2] = c;
   color[3] = 255;
}

void _mesa_noop_SecondaryColor3ubvEXT( const GLubyte *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.SecondaryColor;
   color[0] = v[0];
   color[1] = v[1];
   color[2] = v[2];
   color[3] = 255;
}

void _mesa_noop_SecondaryColor3fEXT( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.SecondaryColor;
   UNCLAMPED_FLOAT_TO_UBYTE(color[0], a);
   UNCLAMPED_FLOAT_TO_UBYTE(color[1], b);
   UNCLAMPED_FLOAT_TO_UBYTE(color[2], c);
   color[3] = 255;
}

void _mesa_noop_SecondaryColor3fvEXT( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLubyte *color = ctx->Current.SecondaryColor;
   UNCLAMPED_FLOAT_TO_CHAN(color[0], v[0]);
   UNCLAMPED_FLOAT_TO_CHAN(color[1], v[1]);
   UNCLAMPED_FLOAT_TO_CHAN(color[2], v[2]);
   color[3] = 255;
}

void _mesa_noop_TexCoord1f( GLfloat a )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], a);
   dest[1] = 0;
   dest[2] = 0;	
   dest[3] = 1;
}

void _mesa_noop_TexCoord1fv( GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], v[0]);
   dest[1] = 0;
   dest[2] = 0;	
   dest[3] = 1;
}

void _mesa_noop_TexCoord2f( GLfloat a, GLfloat b )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   dest[2] = 0;	
   dest[3] = 1;
}

void _mesa_noop_TexCoord2fv( GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   dest[2] = 0;	
   dest[3] = 1;
}

void _mesa_noop_TexCoord3f( GLfloat a, GLfloat b, GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
   dest[3] = 1;
}

void _mesa_noop_TexCoord3fv( GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
   dest[3] = 1;
}

void _mesa_noop_TexCoord4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], a);
   COPY_FLOAT(dest[1], b);
   COPY_FLOAT(dest[2], c);
   COPY_FLOAT(dest[3], d);
}

void _mesa_noop_TexCoord4fv( GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest = ctx->Current.Texcoord[0];
   COPY_FLOAT(dest[0], v[0]);
   COPY_FLOAT(dest[1], v[1]);
   COPY_FLOAT(dest[2], v[2]);
   COPY_FLOAT(dest[3], v[3]);
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
      gl_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
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

