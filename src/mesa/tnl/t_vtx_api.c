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
 * (it may its own version of the macro after size-tracking is working).
 */
#define ATTRF( ATTR, N, A, B, C, D )			\
{							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if (((ATTR) & 0xf) == 0) {				\
      int i;						\
							\
      if (N>0) tnl->dmaptr[0].f = A;			\
      if (N>1) tnl->dmaptr[1].f = B;			\
      if (N>2) tnl->dmaptr[2].f = C;			\
      if (N>3) tnl->dmaptr[3].f = D;			\
							\
      for (i = N; i < tnl->vertex_size; i++)		\
	 *tnl->dmaptr[i].i = tnl->vertex[i].i;		\
							\
      tnl->dmaptr += tnl->vertex_size;			\
							\
      if (--tnl->counter == 0)				\
	 tnl->notify();					\
   }							\
   else {						\
      GLfloat *dest = tnl->attrptr[(ATTR) & 0xf];	\
      if (N>0) dest[0] = A;				\
      if (N>1) dest[1] = B;				\
      if (N>2) dest[2] = C;				\
      if (N>3) dest[3] = D;				\
   }							\
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


/* MultiTexcoord
 */
static void tnl_MultiTexCoord1fARB( GLenum target, GLfloat s  )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR1F( attr, s );
}

static void tnl_MultiTexCoord1fvARB( GLenum target, const GLfloat *v )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR1F( attr, v[0] );
}

static void tnl_MultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR2F( attr, s, t );
}

static void tnl_MultiTexCoord2fvARB( GLenum target, const GLfloat *v )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR2F( attr, v[0], v[1] );
}

static void tnl_MultiTexCoord3fARB( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r)
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR3F( attr, s, t, r );
}

static void tnl_MultiTexCoord3fvARB( GLenum target, const GLfloat *v )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR3F( attr, v[0], v[1], v[2] );
}

static void tnl_MultiTexCoord4fARB( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r, GLfloat q )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR4F( attr, s, t, r, q );
}

static void tnl_MultiTexCoord4fvARB( GLenum target, const GLfloat *v )
{
   GLint attr = (target - GL_TEXTURE0_ARB) + VERT_ATTRIB_TEX0;
   ATTR4F( attr, v[0], v[1], v[2], v[3] );
}


/* NV_vertex_program:  
 *
 * *** Need second dispatch layer above this for size tracking.  One
 * *** dispatch layer handles both VertexAttribute and MultiTexCoord 
 */
static void tnl_VertexAttrib1fNV( GLuint index, GLfloat s )
{
   ATTR1F( index, s );
}

static void tnl_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   ATTR1F( index, v[0] );
}

static void tnl_VertexAttrib2fNV( GLuint index, GLfloat s, GLfloat t )
{
   ATTR2F( index, s, t );
}

static void tnl_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   ATTR2F( index, v[0], v[1] );
}

static void tnl_VertexAttrib3fNV( GLuint index, GLfloat s, GLfloat t, 
				  GLfloat r )
{
   ATTR3F( index, s, t, r );
}

static void tnl_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   ATTR3F( index, v[0], v[1], v[2] );
}

static void tnl_VertexAttrib4fNV( GLuint index, GLfloat s, GLfloat t,
				  GLfloat r, GLfloat q )
{
   ATTR4F( index, s, t, r, q );
}

static void tnl_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   ATTR4F( index, v[0], v[1], v[2], v[3] );
}


/* Miscellaneous:  (These don't alias NV attributes, right?)
 */
static void tnl_EdgeFlag( GLboolean flag )
{
   GET_TNL;
   tnl->edgeflagptr[0] = flag;
}

static void tnl_EdgeFlagv( const GLboolean *flag )
{
   GET_TNL;
   tnl->edgeflagptr[0] = *flag;
}

static void tnl_Indexi( GLint idx )
{
   GET_TNL;
   tnl->indexptr[0] = idx;
}

static void tnl_Indexiv( const GLint *idx )
{
   GET_TNL;
   tnl->indexptr[0] = *idx;
}



/* Could use dispatch switching to build 'ranges' of eval vertices for
 * each type, avoiding need for flags.  (Make
 * evalcoords/evalpoints/vertices/attr0 mutually exclusive)
 *  --> In which case, may as well use Vertex{12}f{v} here.
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


/* Materials:  
 *  *** Treat as more vertex attributes
 */
static void _tnl_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   if (MESA_VERBOSE & DEBUG_VFMT)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (tnl->prim[0] != GL_POLYGON+1) {
      VFMT_FALLBACK( __FUNCTION__ );
      glMaterialfv( face, pname, params );
      return;
   }
   _mesa_noop_Materialfv( face, pname, params );
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
      fprintf(stderr, "%s -- cached codegen\n", __FUNCTION__ );		\
									\
   if (dfn)								\
      tnl->context->Exec->FN = (FNTYPE)(dfn->code);			\
   else {								\
      if (MESA_VERBOSE & DEBUG_CODEGEN)					\
	 fprintf(stderr, "%s -- generic version\n", __FUNCTION__ );	\
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
   vfmt->MultiTexCoord1fARB = choose_MultiTexCoord1fARB;
   vfmt->MultiTexCoord1fvARB = choose_MultiTexCoord1fvARB;
   vfmt->MultiTexCoord2fARB = choose_MultiTexCoord2fARB;
   vfmt->MultiTexCoord2fvARB = choose_MultiTexCoord2fvARB;
   vfmt->Normal3f = choose_Normal3f;
   vfmt->Normal3fv = choose_Normal3fv;
   vfmt->TexCoord1f = choose_TexCoord1f;
   vfmt->TexCoord1fv = choose_TexCoord1fv;
   vfmt->TexCoord2f = choose_TexCoord2f;
   vfmt->TexCoord2fv = choose_TexCoord2fv;
   vfmt->Vertex2f = choose_Vertex2f;
   vfmt->Vertex2fv = choose_Vertex2fv;
   vfmt->Vertex3f = choose_Vertex3f;
   vfmt->Vertex3fv = choose_Vertex3fv;
   vfmt->TexCoord3f = choose_TexCoord3f;
   vfmt->TexCoord3fv = choose_TexCoord3fv;
   vfmt->TexCoord4f = choose_TexCoord4f;
   vfmt->TexCoord4fv = choose_TexCoord4fv;
   vfmt->MultiTexCoord3fARB = choose_MultiTexCoord3fARB;
   vfmt->MultiTexCoord3fvARB = choose_MultiTexCoord3fvARB;
   vfmt->MultiTexCoord4fARB = choose_MultiTexCoord4fARB;
   vfmt->MultiTexCoord4fvARB = choose_MultiTexCoord4fvARB;
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
   vfmt->EvalMesh1 = choose_EvalMesh1;
   vfmt->EvalMesh2 = choose_EvalMesh2;
   vfmt->EvalPoint1 = choose_EvalPoint1;
   vfmt->EvalPoint2 = choose_EvalPoint2;

   vfmt->Materialfv = _tnl_Materialfv;
}


static struct dynfn *codegen_noop( struct _vb *vb, int key )
{
   (void) vb; (void) key;
   return 0;
}

void _tnl_InitCodegen( struct dfn_generators *gen )
{
   gen->Vertex2f = codegen_noop;
   gen->Vertex2fv = codegen_noop;
   gen->Vertex3f = codegen_noop;
   gen->Vertex3fv = codegen_noop;
   gen->Vertex4f = codegen_noop;
   gen->Vertex4fv = codegen_noop;

   gen->Attr1f = codegen_noop;
   gen->Attr1fv = codegen_noop;
   gen->Attr2f = codegen_noop;
   gen->Attr2fv = codegen_noop;
   gen->Attr3f = codegen_noop;
   gen->Attr3fv = codegen_noop;
   gen->Attr4f = codegen_noop;
   gen->Attr4fv = codegen_noop;
   gen->Attr3ub = codegen_noop;
   gen->Attr3ubv = codegen_noop;
   gen->Attr4ub = codegen_noop;
   gen->Attr4ubv = codegen_noop;

   /* Probably need two versions of this, one for the front end
    * (double dispatch), one for the back end (do the work) -- but
    * will also need a second level of CHOOSE functions?
    *   -- Generate the dispatch layer using the existing templates somehow.
    *   -- Generate the backend and 2nd level choosers here.
    *   -- No need for a chooser on the top level.
    *   -- Can aliasing help -- ie can NVAttr1f == Attr1f/Vertex2f at this level (index is known)
    */
   gen->NVAttr1f = codegen_noop;
   gen->NVAttr1fv = codegen_noop;
   gen->NVAttr2f = codegen_noop;
   gen->NVAttr2fv = codegen_noop;
   gen->NVAttr3f = codegen_noop;
   gen->NVAttr3fv = codegen_noop;
   gen->NVAttr4f = codegen_noop;
   gen->NVAttr4fv = codegen_noop;

   gen->MTAttr1f = codegen_noop;
   gen->MTAttr1fv = codegen_noop;
   gen->MTAttr2f = codegen_noop;
   gen->MTAttr2fv = codegen_noop;
   gen->MTAttr3f = codegen_noop;
   gen->MTAttr3fv = codegen_noop;
   gen->MTAttr4f = codegen_noop;
   gen->MTAttr4fv = codegen_noop;

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
