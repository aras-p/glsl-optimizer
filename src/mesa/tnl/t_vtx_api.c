/* $XFree86$ */
/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */
#include "mtypes.h"
#include "colormac.h"
#include "simple_list.h"
#include "vtxfmt.h"

#include "tnl_vtx_api.h"

/* Fallback versions of all the entrypoints for situations where
 * codegen isn't available.  This is slowed significantly by all the
 * gumph necessary to get to the tnl pointer.
 */


/* MultiTexcoord ends up with both of these branches, unfortunately
 * (it may get its own version of the macro after size-tracking is
 * working). 
 *
 * Errors (VertexAttribNV when ATTR>15) are handled at a higher level.  
 */
#define ATTRF( ATTR, N, A, B, C, D )				\
{								\
   GET_CURRENT_CONTEXT( ctx );					\
   TNLcontext *tnl = TNL_CONTEXT(ctx);				\
								\
   if ((ATTR) == 0) {						\
      int i;							\
								\
      if (N>0) tnl->vbptr[0].f = A;				\
      if (N>1) tnl->vbptr[1].f = B;				\
      if (N>2) tnl->vbptr[2].f = C;				\
      if (N>3) tnl->vbptr[3].f = D;				\
								\
      for (i = N; i < tnl->vertex_size; i++)			\
	 *tnl->vbptr[i].i = tnl->vertex[i].i;			\
								\
      tnl->vbptr += tnl->vertex_size;				\
								\
      if (--tnl->counter == 0)					\
	 tnl->notify();						\
   }								\
   else {					\
      GLfloat *dest = tnl->attrptr[ATTR];			\
      if (N>0) dest[0] = A;					\
      if (N>1) dest[1] = B;					\
      if (N>2) dest[2] = C;					\
      if (N>3) dest[3] = D;					\
   }								\
}

#define ATTR4F( ATTR, A, B, C, D )  ATTRF( ATTR, 4, A, B, C, D )
#define ATTR3F( ATTR, A, B, C, D )  ATTRF( ATTR, 3, A, B, C, 1 )
#define ATTR2F( ATTR, A, B, C, D )  ATTRF( ATTR, 2, A, B, 0, 1 )
#define ATTR1F( ATTR, A, B, C, D )  ATTRF( ATTR, 1, A, 0, 0, 1 )

#define ATTR3UB( ATTR, A, B, C )		\
   ATTR3F( ATTR,				\
	   UBYTE_TO_FLOAT(A),			\
	   UBYTE_TO_FLOAT(B),			\
	   UBYTE_TO_FLOAT(C))


#define ATTR4UB( ATTR, A, B, C, D )		\
   ATTR4F( ATTR,				\
	   UBYTE_TO_FLOAT(A),			\
	   UBYTE_TO_FLOAT(B),			\
	   UBYTE_TO_FLOAT(C),			\
	   UBYTE_TO_FLOAT(D))


/* Vertex
 */
static void tnl_Vertex2f( GLfloat x, GLfloat y )
{
   ATTR2F( VERT_ATTRIB_POS, x, y ); 
}

static void tnl_Vertex2fv( const GLfloat *v )
{
   ATTR2F( VERT_ATTRIB_POS, v[0], v[1] ); 
}

static void tnl_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   ATTR3F( VERT_ATTRIB_POS, x, y, z ); 
}

static void tnl_Vertex3fv( const GLfloat *v )
{
   ATTR3F( VERT_ATTRIB_POS, v[0], v[1], v[2] ); 
}

static void tnl_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   ATTR4F( VERT_ATTRIB_POS, x, y, z, w ); 
}

static void tnl_Vertex4fv( const GLfloat *v )
{
   ATTR4F( VERT_ATTRIB_POS, v[0], v[1], v[2], v[3] ); 
}


/* Color
 */
static void tnl_Color3ub( GLubyte r, GLubyte g, GLubyte b )
{
   ATTR3UB( VERT_ATTRIB_COLOR0, r, g, b );
}

static void tnl_Color3ubv( const GLubyte *v )
{
   ATTR3UB( VERT_ATTRIB_COLOR0, v[0], v[1], v[2] );
}

static void tnl_Color4ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   ATTR4UB( VERT_ATTRIB_COLOR0, r, g, b, a );
}

static void tnl_Color4ubv( const GLubyte *v )
{
   ATTR4UB( VERT_ATTRIB_COLOR0, v[0], v[1], v[2], v[3] );
}

static void tnl_Color3f( GLfloat r, GLfloat g, GLfloat b )
{
   ATTR3F( VERT_ATTRIB_COLOR0, r, g, b );
}

static void tnl_Color3fv( const GLfloat *v )
{
   ATTR3F( VERT_ATTRIB_COLOR0, v[0], v[1], v[2] );
}

static void tnl_Color4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   ATTR4F( VERT_ATTRIB_COLOR0, r, g, b, a );
}

static void tnl_Color4fv( const GLfloat *v )
{
   ATTR4F( VERT_ATTRIB_COLOR0, v[0], v[1], v[2], v[3] );
}


/* Secondary Color
 */
static void tnl_SecondaryColor3ubEXT( GLubyte r, GLubyte g, GLubyte b )
{
   ATTR3UB( VERT_ATTRIB_COLOR1, r, g, b );
}

static void tnl_SecondaryColor3ubvEXT( const GLubyte *v )
{
   ATTR3UB( VERT_ATTRIB_COLOR1, v[0], v[1], v[2] );
}

static void tnl_SecondaryColor3fEXT( GLfloat r, GLfloat g, GLfloat b )
{
   ATTR3F( VERT_ATTRIB_COLOR1, r, g, b );
}

static void tnl_SecondaryColor3fvEXT( const GLfloat *v )
{
   ATTR3F( VERT_ATTRIB_COLOR1, v[0], v[1], v[2] );
}



/* Fog Coord
 */
static void tnl_FogCoordfEXT( GLfloat f )
{
   ATTR1F( VERT_ATTRIB_FOG, f );
}

static void tnl_FogCoordfvEXT( const GLfloat *v )
{
   ATTR1F( VERT_ATTRIB_FOG, v[0] );
}



/* Normal
 */
static void tnl_Normal3f( GLfloat n0, GLfloat n1, GLfloat n2 )
{
   ATTR3F( VERT_ATTRIB_NORMAL, n0, n1, n2 );
}

static void tnl_Normal3fv( const GLfloat *v )
{
   ATTR3F( VERT_ATTRIB_COLOR1, v[0], v[1], v[2] );
}


/* TexCoord
 */
static void tnl_TexCoord1f( GLfloat s )
{
   ATTR1F( VERT_ATTRIB_TEX0, s );
}

static void tnl_TexCoord1fv( const GLfloat *v )
{
   ATTR1F( VERT_ATTRIB_TEX0, v[0] );
}

static void tnl_TexCoord2f( GLfloat s, GLfloat t )
{
   ATTR2F( VERT_ATTRIB_TEX0, s, t );
}

static void tnl_TexCoord2fv( const GLfloat *v )
{
   ATTR2F( VERT_ATTRIB_TEX0, v[0], v[1] );
}

static void tnl_TexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
   ATTR3F( VERT_ATTRIB_TEX0, s, t, r );
}

static void tnl_TexCoord3fv( const GLfloat *v )
{
   ATTR3F( VERT_ATTRIB_TEX0, v[0], v[1], v[2] );
}

static void tnl_TexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   ATTR4F( VERT_ATTRIB_TEX0, s, t, r, q );
}

static void tnl_TexCoord4fv( const GLfloat *v )
{
   ATTR4F( VERT_ATTRIB_TEX0, v[0], v[1], v[2], v[3] );
}


/* Miscellaneous: 
 *
 * These don't alias NV attributes, but still need similar treatment.
 * Basically these are attributes with numbers greater than 16.
 */
static void tnl_EdgeFlag( GLboolean flag )
{
   GLfloat f = flag ? 1 : 0;
   ATTR1F( VERT_ATTRIB_EDGEFLAG, f);
}

static void tnl_EdgeFlagv( const GLboolean *flag )
{
   GLfloat f = flag[0] ? 1 : 0;
   ATTR1F( VERT_ATTRIB_EDGEFLAG, f);
}

static void tnl_Indexi( GLint idx )
{
   ATTR1F( VERT_ATTRIB_INDEX, idx );
}

static void tnl_Indexiv( const GLint *idx )
{
   ATTR1F( VERT_ATTRIB_INDEX, idx );
}

/* Use dispatch switching to build 'ranges' of eval vertices for each
 * type, avoiding need for flags.  (Make
 * evalcoords/evalpoints/vertices/attr0 mutually exclusive)
 */
static void _tnl_EvalCoord1f( GLfloat u )
{
   ATTR1F( VERT_ATTRIB_POS, u );
}

static void _tnl_EvalCoord1fv( const GLfloat *v )
{
   ATTR1F( VERT_ATTRIB_POS, v[0] );
}

static void _tnl_EvalCoord2f( GLfloat u, GLfloat v )
{
   ATTR2F( VERT_ATTRIB_POS, u, v );
}

static void _tnl_EvalCoord2fv( const GLfloat *v )
{
   ATTR2F( VERT_ATTRIB_POS, v[0], v[1] );
}



/* Second level dispatch table for MultiTexCoord, Material and 
 * VertexAttribNV.
 *
 * Need this because we want to track things like vertex attribute
 * sizes, presence/otherwise of attribs in recorded vertices, etc, by
 * manipulating the state of dispatch tables.  Need therefore a
 * dispatch slot for each value of 'index' or 'unit' in VertexAttribNV
 * and MultiTexCoordARB.  Also need a mechnism for keeping this data
 * consistent with what's coming in via the Vertex/Normal/etc api
 * above (where aliasing exists with the traditional entrypoints).
 * Note that MultiTexCoordARB aliases with TexCoord when unit==0.
 *
 * Need presence tracking for material components, too, but not size
 * tracking or help with aliasing.  Could move material to seperate
 * dispatch without the "*4" below, or even do the checks every time.
 */
struct attr_dispatch_tab {
   void (*tab[32*4])( void );
   void (*swapped[32*4])( void );
   int swapcount;
   int installed_sizes[32];
};

#define DISPATCH_ATTR1F( ATTR, N,  ) 
   tnl->vb.attr_dispatch 

/* Result at the back end after second dispatch -- could further
 * specialize for attr zero -- maybe just in the codegen version.
 */
static void tnl_Attr1f( GLint attr, GLfloat s )
{
   ATTR1F( attr, s );
}

static void tnl_Attr1fv( GLint attr, const GLfloat *v )
{
   ATTR1F( attr, v[0] );
}

static void tnl_Attr2f( GLint attr, GLfloat s, GLfloat t )
{
   ATTR2F( attr, s, t );
}

static void tnl_Attr2fv( GLint attr, const GLfloat *v )
{
   ATTR2F( attr, v[0], v[1] );
}

static void tnl_Attr3f( GLint attr, GLfloat s, GLfloat t, GLfloat r )
{
   ATTR3F( attr, s, t, r );
}

static void tnl_Attr3fv( GLint attr, const GLfloat *v )
{
   ATTR3F( attr, v[0], v[1], v[2] );
}

static void tnl_Attr4f( GLint attr, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   ATTR4F( attr, s, t, r, q );
}

static void tnl_Attr4fv( GLint attr, const GLfloat *v )
{
   ATTR4F( attr, v[0], v[1], v[2], v[3] );
}


/* MultiTexcoord:  Send through second level dispatch.
 */
static void tnl_MultiTexCoord1fARB( GLenum target, GLfloat s  )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR1F( attr, s );
}

static void tnl_MultiTexCoord1fvARB( GLenum target, const GLfloat *v )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR1F( attr, v[0] );
}

static void tnl_MultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR2F( attr, s, t );
}

static void tnl_MultiTexCoord2fvARB( GLenum target, const GLfloat *v )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR2F( attr, v[0], v[1] );
}

static void tnl_MultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r)
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR3F( attr, s, t, r );
}

static void tnl_MultiTexCoord3fvARB( GLenum target, const GLfloat *v )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR3F( attr, v[0], v[1], v[2] );
}

static void tnl_MultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r, GLfloat q )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR4F( attr, s, t, r, q );
}

static void tnl_MultiTexCoord4fvARB( GLenum target, const GLfloat *v )
{
   GLuint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   if (attr < MAX_VERT_ATTRS)
      DISPATCH_ATTR4F( attr, v[0], v[1], v[2], v[3] );
}


/* NV_vertex_program:
 *
 * Check for errors & reroute through second dispatch layer to get
 * size tracking per-attribute.
 */
static void tnl_VertexAttrib1fNV( GLuint index, GLfloat s )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR1F( index, s );
   else
      DISPATCH_ERROR; 
}

static void tnl_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR1F( index, v[0] );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib2fNV( GLuint index, GLfloat s, GLfloat t )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR2F( index, s, t );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR2F( index, v[0], v[1] );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib3fNV( GLuint index, GLfloat s, GLfloat t, 
				  GLfloat r )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR3F( index, s, t, r );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR3F( index, v[0], v[1], v[2] );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib4fNV( GLuint index, GLfloat s, GLfloat t,
				  GLfloat r, GLfloat q )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR4F( index, s, t, r, q );
   else
      DISPATCH_ERROR;
}

static void tnl_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   if (index < MAX_VERT_ATTRS)
      DISPATCH_ATTR4F( index, v[0], v[1], v[2], v[3] );
   else
      DISPATCH_ERROR;
}







/* Materials:  
 * 
 * These are treated as per-vertex attributes, at indices above where
 * the NV_vertex_program leaves off.  There are a lot of good things
 * about treating materials this way.  
 *
 * *** Need a dispatch step (like VertexAttribute GLint attr, and MultiTexCoord)
 * *** to expand vertex size, etc.  Use the same second level dispatch
 * *** (keyed by attr number) as above.
 */
#define MAT( ATTR, face, params )		\
do {						\
   if (face != GL_BACK)				\
      DISPATCH_ATTRF( ATTR, N, params );	\
   if (face != GL_FRONT)			\
      DISPATCH_ATTRF( ATTR+7, N, params );	\
} while (0)


/* NOTE: Have to remove/dealwith colormaterial crossovers, probably
 * later on - in the meantime just store everything.
 */
static void _tnl_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   switch (pname) {
   case GL_EMISSION:
      MAT( VERT_ATTRIB_FRONT_EMMISSION, 4, face, params );
      break;
   case GL_AMBIENT:
      MAT( VERT_ATTRIB_FRONT_AMBIENT, 4, face, params );
      break;
   case GL_DIFFUSE:
      MAT( VERT_ATTRIB_FRONT_DIFFUSE, 4, face, params );
      break;
   case GL_SPECULAR:
      MAT( VERT_ATTRIB_FRONT_SPECULAR, 4, face, params );
      break;
   case GL_SHININESS:
      MAT( VERT_ATTRIB_FRONT_SHININESS, 1, face, params );
      break;
   case GL_COLOR_INDEXES:
      MAT( VERT_ATTRIB_FRONT_EMMISSION, 3, face, params );
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      MAT( VERT_ATTRIB_FRONT_AMBIENT, 4, face, params );
      MAT( VERT_ATTRIB_FRONT_DIFFUSE, 4, face, params );
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, where );
      return;
   }
}




/* Codegen support
 */
static struct dynfn *lookup( struct dynfn *l, int key )
{
   struct dynfn *f;

   foreach( f, l ) {
      if (f->key == key) 
	 return f;
   }

   return 0;
}

/* Vertex descriptor
 */
struct _tnl_vertex_descriptor {
   GLuint attr_bits[4];
};


/* Can't use the loopback template for this:
 */
#define CHOOSE(FN, FNTYPE, MASK, ACTIVE, ARGS1, ARGS2 )			\
static void choose_##FN ARGS1						\
{									\
   int key = tnl->vertex_format & (MASK|ACTIVE);			\
   struct dynfn *dfn = lookup( &tnl->dfn_cache.FN, key );		\
									\
   if (dfn == 0)							\
      dfn = tnl->codegen.FN( &vb, key );				\
   else if (MESA_VERBOSE & DEBUG_CODEGEN)				\
      _mesa_debug(NULL, "%s -- cached codegen\n", __FUNCTION__ );		\
									\
   if (dfn)								\
      tnl->context->Exec->FN = (FNTYPE)(dfn->code);			\
   else {								\
      if (MESA_VERBOSE & DEBUG_CODEGEN)					\
	 _mesa_debug(NULL, "%s -- generic version\n", __FUNCTION__ );	\
      tnl->context->Exec->FN = tnl_##FN;				\
   }									\
									\
   tnl->context->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;		\
   tnl->context->Exec->FN ARGS2;					\
}



CHOOSE(Normal3f, p3f, 3, VERT_ATTRIB_NORMAL, 
       (GLfloat a,GLfloat b,GLfloat c), (a,b,c))
CHOOSE(Normal3fv, pfv, 3, VERT_ATTRIB_NORMAL, 
       (const GLfloat *v), (v))

CHOOSE(Color4ub, p4ub, 4, VERT_ATTRIB_COLOR0,
	(GLubyte a,GLubyte b, GLubyte c, GLubyte d), (a,b,c,d))
CHOOSE(Color4ubv, pubv, 4, VERT_ATTRIB_COLOR0, 
	(const GLubyte *v), (v))
CHOOSE(Color3ub, p3ub, 3, VERT_ATTRIB_COLOR0, 
	(GLubyte a,GLubyte b, GLubyte c), (a,b,c))
CHOOSE(Color3ubv, pubv, 3, VERT_ATTRIB_COLOR0, 
	(const GLubyte *v), (v))

CHOOSE(Color4f, p4f, 4, VERT_ATTRIB_COLOR0, 
	(GLfloat a,GLfloat b, GLfloat c, GLfloat d), (a,b,c,d))
CHOOSE(Color4fv, pfv, 4, VERT_ATTRIB_COLOR0, 
	(const GLfloat *v), (v))
CHOOSE(Color3f, p3f, 3, VERT_ATTRIB_COLOR0,
	(GLfloat a,GLfloat b, GLfloat c), (a,b,c))
CHOOSE(Color3fv, pfv, 3, VERT_ATTRIB_COLOR0,
	(const GLfloat *v), (v))


CHOOSE(SecondaryColor3ubEXT, p3ub, VERT_ATTRIB_COLOR1, 
	(GLubyte a,GLubyte b, GLubyte c), (a,b,c))
CHOOSE(SecondaryColor3ubvEXT, pubv, VERT_ATTRIB_COLOR1, 
	(const GLubyte *v), (v))
CHOOSE(SecondaryColor3fEXT, p3f, VERT_ATTRIB_COLOR1,
	(GLfloat a,GLfloat b, GLfloat c), (a,b,c))
CHOOSE(SecondaryColor3fvEXT, pfv, VERT_ATTRIB_COLOR1,
	(const GLfloat *v), (v))

CHOOSE(TexCoord2f, p2f, VERT_ATTRIB_TEX0, 
       (GLfloat a,GLfloat b), (a,b))
CHOOSE(TexCoord2fv, pfv, VERT_ATTRIB_TEX0, 
       (const GLfloat *v), (v))
CHOOSE(TexCoord1f, p1f, VERT_ATTRIB_TEX0, 
       (GLfloat a), (a))
CHOOSE(TexCoord1fv, pfv, VERT_ATTRIB_TEX0, 
       (const GLfloat *v), (v))

CHOOSE(MultiTexCoord2fARB, pe2f, VERT_ATTRIB_TEX0,
	 (GLenum u,GLfloat a,GLfloat b), (u,a,b))
CHOOSE(MultiTexCoord2fvARB, pefv, MASK_ST_ALL, ACTIVE_ST_ALL,
	(GLenum u,const GLfloat *v), (u,v))
CHOOSE(MultiTexCoord1fARB, pe1f, MASK_ST_ALL, ACTIVE_ST_ALL,
	 (GLenum u,GLfloat a), (u,a))
CHOOSE(MultiTexCoord1fvARB, pefv, MASK_ST_ALL, ACTIVE_ST_ALL,
	(GLenum u,const GLfloat *v), (u,v))

CHOOSE(Vertex3f, p3f, VERT_ATTRIB_POS, 
       (GLfloat a,GLfloat b,GLfloat c), (a,b,c))
CHOOSE(Vertex3fv, pfv, VERT_ATTRIB_POS, 
       (const GLfloat *v), (v))
CHOOSE(Vertex2f, p2f, VERT_ATTRIB_POS, 
       (GLfloat a,GLfloat b), (a,b))
CHOOSE(Vertex2fv, pfv, VERT_ATTRIB_POS, 
       (const GLfloat *v), (v))





void _tnl_InitVtxfmtChoosers( GLvertexformat *vfmt )
{
   vfmt->Color3f = choose_Color3f;
   vfmt->Color3fv = choose_Color3fv;
   vfmt->Color3ub = choose_Color3ub;
   vfmt->Color3ubv = choose_Color3ubv;
   vfmt->Color4f = choose_Color4f;
   vfmt->Color4fv = choose_Color4fv;
   vfmt->Color4ub = choose_Color4ub;
   vfmt->Color4ubv = choose_Color4ubv;
   vfmt->SecondaryColor3fEXT = choose_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = choose_SecondaryColor3fvEXT;
   vfmt->SecondaryColor3ubEXT = choose_SecondaryColor3ubEXT;
   vfmt->SecondaryColor3ubvEXT = choose_SecondaryColor3ubvEXT;
   vfmt->MultiTexCoord1fARB = dd_MultiTexCoord1fARB;
   vfmt->MultiTexCoord1fvARB = dd_MultiTexCoord1fvARB;
   vfmt->MultiTexCoord2fARB = dd_MultiTexCoord2fARB;
   vfmt->MultiTexCoord2fvARB = dd_MultiTexCoord2fvARB;
   vfmt->MultiTexCoord3fARB = dd_MultiTexCoord3fARB;
   vfmt->MultiTexCoord3fvARB = dd_MultiTexCoord3fvARB;
   vfmt->MultiTexCoord4fARB = dd_MultiTexCoord4fARB;
   vfmt->MultiTexCoord4fvARB = dd_MultiTexCoord4fvARB;
   vfmt->Normal3f = choose_Normal3f;
   vfmt->Normal3fv = choose_Normal3fv;
   vfmt->TexCoord1f = choose_TexCoord1f;
   vfmt->TexCoord1fv = choose_TexCoord1fv;
   vfmt->TexCoord2f = choose_TexCoord2f;
   vfmt->TexCoord2fv = choose_TexCoord2fv;
   vfmt->TexCoord3f = choose_TexCoord3f;
   vfmt->TexCoord3fv = choose_TexCoord3fv;
   vfmt->TexCoord4f = choose_TexCoord4f;
   vfmt->TexCoord4fv = choose_TexCoord4fv;
   vfmt->Vertex2f = choose_Vertex2f;
   vfmt->Vertex2fv = choose_Vertex2fv;
   vfmt->Vertex3f = choose_Vertex3f;
   vfmt->Vertex3fv = choose_Vertex3fv;
   vfmt->Vertex4f = choose_Vertex4f;
   vfmt->Vertex4fv = choose_Vertex4fv;
   vfmt->FogCoordfvEXT = choose_FogCoordfvEXT;
   vfmt->FogCoordfEXT = choose_FogCoordfEXT;
   vfmt->EdgeFlag = choose_EdgeFlag;
   vfmt->EdgeFlagv = choose_EdgeFlagv;
   vfmt->Indexi = choose_Indexi;
   vfmt->Indexiv = choose_Indexiv;
   vfmt->EvalCoord1f = choose_EvalCoord1f;
   vfmt->EvalCoord1fv = choose_EvalCoord1fv;
   vfmt->EvalCoord2f = choose_EvalCoord2f;
   vfmt->EvalCoord2fv = choose_EvalCoord2fv;
   vfmt->Materialfv = dd_Materialfv;
}


static struct dynfn *codegen_noop( struct _vb *vb, int key )
{
   (void) vb; (void) key;
   return 0;
}

void _tnl_InitCodegen( struct dfn_generators *gen )
{
   /* Generate an attribute or vertex command.
    */
   gen->Attr1f = codegen_noop;
   gen->Attr1fv = codegen_noop;
   gen->Attr2f = codegen_noop;
   gen->Attr2fv = codegen_noop;
   gen->Attr3f = codegen_noop;
   gen->Attr3fv = codegen_noop;
   gen->Attr4f = codegen_noop;
   gen->Attr4fv = codegen_noop;
   
   /* Index is never zero for these...
    */
   gen->Attr3ub = codegen_noop;
   gen->Attr3ubv = codegen_noop;
   gen->Attr4ub = codegen_noop;
   gen->Attr4ubv = codegen_noop;

   /* As above, but deal with the extra (redundant by now) index
    * argument to the generated function.
    */
   gen->NVAttr1f = codegen_noop;
   gen->NVAttr1fv = codegen_noop;
   gen->NVAttr2f = codegen_noop;
   gen->NVAttr2fv = codegen_noop;
   gen->NVAttr3f = codegen_noop;
   gen->NVAttr3fv = codegen_noop;
   gen->NVAttr4f = codegen_noop;
   gen->NVAttr4fv = codegen_noop;


   if (!getenv("MESA_NO_CODEGEN")) {
#if defined(USE_X86_ASM)
      _tnl_InitX86Codegen( gen );
#endif

#if defined(USE_SSE_ASM)
      _tnl_InitSSECodegen( gen );
#endif

#if defined(USE_3DNOW_ASM)
#endif

#if defined(USE_SPARC_ASM)
#endif
   }
}
