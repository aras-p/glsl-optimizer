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
#include "context.h"
#include "colormac.h"
#include "simple_list.h"
#include "api_arrayelt.h"

#include "t_context.h"
#include "t_vtx_api.h"


/* Versions of all the entrypoints for situations where codegen isn't
 * available.  This is slowed significantly by all the gumph necessary
 * to get to the tnl pointer, which can be avoided with codegen.
 *
 * Note: Only one size for each attribute may be active at once.
 * Eg. if Color3f is installed/active, then Color4f may not be, even
 * if the vertex actually contains 4 color coordinates.  This is
 * because the 3f version won't otherwise set color[3] to 1.0 -- this
 * is the job of the chooser function when switching between Color4f
 * and Color3f.
 */
#define ATTRF( ATTR, N, A, B, C, D )			\
{							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if ((ATTR) == 0) {					\
      int i;						\
							\
      if (N>0) tnl->vtx.vbptr[0].f = A;			\
      if (N>1) tnl->vtx.vbptr[1].f = B;			\
      if (N>2) tnl->vtx.vbptr[2].f = C;			\
      if (N>3) tnl->vtx.vbptr[3].f = D;			\
							\
      for (i = N; i < tnl->vtx.vertex_size; i++)	\
	 tnl->vtx.vbptr[i].ui = tnl->vtx.vertex[i].ui;	\
							\
      tnl->vtx.vbptr += tnl->vtx.vertex_size;		\
							\
      if (--tnl->vtx.counter == 0)			\
	 _tnl_FlushVertices( ctx, FLUSH_STORED_VERTICES );	\
   }							\
   else {						\
      union uif *dest = tnl->vtx.attrptr[ATTR];		\
      if (N>0) dest[0].f = A;				\
      if (N>1) dest[1].f = B;				\
      if (N>2) dest[2].f = C;				\
      if (N>3) dest[3].f = D;				\
   }							\
}

#define ATTR4F( ATTR, A, B, C, D )  ATTRF( ATTR, 4, A, B, C, D )
#define ATTR3F( ATTR, A, B, C )     ATTRF( ATTR, 3, A, B, C, 1 )
#define ATTR2F( ATTR, A, B )        ATTRF( ATTR, 2, A, B, 0, 1 )
#define ATTR1F( ATTR, A  )          ATTRF( ATTR, 1, A, 0, 0, 1 )

#define ATTRS( ATTRIB )						\
static void attrib_##ATTRIB##_1_0( GLfloat s )			\
{								\
   ATTR1F( ATTRIB, s );						\
}								\
								\
static void attrib_##ATTRIB##_1_1( const GLfloat *v )		\
{								\
   ATTR1F( ATTRIB, v[0] );					\
}								\
								\
static void attrib_##ATTRIB##_2_0( GLfloat s, GLfloat t )	\
{								\
   ATTR2F( ATTRIB, s, t );					\
}								\
								\
static void attrib_##ATTRIB##_2_1( const GLfloat *v )		\
{								\
   ATTR2F( ATTRIB, v[0], v[1] );				\
}								\
								\
static void attrib_##ATTRIB##_3_0( GLfloat s, GLfloat t, 	\
				   GLfloat r )			\
{								\
   ATTR3F( ATTRIB, s, t, r );					\
}								\
								\
static void attrib_##ATTRIB##_3_1( const GLfloat *v )		\
{								\
   ATTR3F( ATTRIB, v[0], v[1], v[2] );				\
}								\
								\
static void attrib_##ATTRIB##_4_0( GLfloat s, GLfloat t,	\
				   GLfloat r, GLfloat q )	\
{								\
   ATTR4F( ATTRIB, s, t, r, q );				\
}								\
								\
static void attrib_##ATTRIB##_4_1( const GLfloat *v )		\
{								\
   ATTR4F( ATTRIB, v[0], v[1], v[2], v[3] );			\
}

/* Generate a lot of functions.  These are the actual worker
 * functions, which are equivalent to those generated via codegen
 * elsewhere.
 */
ATTRS( 0 )
ATTRS( 1 )
ATTRS( 2 )
ATTRS( 3 )
ATTRS( 4 )
ATTRS( 5 )
ATTRS( 6 )
ATTRS( 7 )
ATTRS( 8 )
ATTRS( 9 )
ATTRS( 10 )
ATTRS( 11 )
ATTRS( 12 )
ATTRS( 13 )
ATTRS( 14 )
ATTRS( 15 )

/* Flush existing data, set new attrib size, replay copied vertices.
 */ 
static void _tnl_upgrade_vertex( GLcontext *ctx, 
				 GLuint attr,
				 GLuint newsz )
{
   GLuint oldsz = tnl->vtx.attrib[attr].sz;

   _tnl_flush_immediate( ctx );

   tnl->vtx.attrib[attr].sz = newsz;
   /* What else to do here?
    */

   /* Replay stored vertices to translate them to new format: Use
    * bitmap and ffs() to speed inner loop:
    */
   for (i = 0 ; i < tnl->copied_verts.nr ; i++) {
      GLfloat *data = old_data + tnl->copied_verts.offset[i];

      for (j = 1 ; j < MAX_ATTRIB ; j++) {
	 if (tnl->vtx.attrib[j].sz) {
	    if (j == attr) {
	       GLfloat tmp[4];
	       COPY_4FV( tmp, id );
	       COPY_SZ_4V( tmp, oldsz, data );
	       data += oldsz;
	       tnl->vtx.attrib[attr].fv( tmp );
	    }
	    else {
	       GLfloat *tmp = data;
	       data += tnl->vtx.attrib[j].sz;
	       tnl->vtx.attrib[j].fv( tmp );
	    }
	 }
      }
   }
}

static void _tnl_wrap_buffers( GLcontext *ctx )
{
   _tnl_flush_immediate( ctx );

   /* Replay stored vertices - don't really need to do this, memcpy
    * would be fine.
    */
   for (i = 0 ; i < tnl->copied_verts.nr ; i++) {
      for (j = 1 ; j < MAX_ATTRIB ; j++) {
	 GLfloat *tmp = data;
	 data += tnl->vtx.attrib[j].sz;
	 tnl->vtx.attrib[j].fv( tmp );
      }
   }
}



/* The functions defined below (CHOOSERS) are the initial state for
 * dispatch entries for all entrypoints except those requiring
 * double-dispatch (multitexcoord, material, vertexattrib).
 *
 * These may provoke a vertex-upgrade where the existing vertex buffer
 * is flushed and a new element is added to the active vertex layout.
 * This can happen between begin/end pairs.
 */

/* An active attribute has changed size.
 */
static void _tnl_fixup_vertex( GLcontext *ctx, GLuint attr, GLuint sz )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 

   if (tnl->vtx.attrib_sz[attr] < sz) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      _tnl_upgrade_vertex( tnl, attr, sz );
   }
   else {
      static float id[4] = { 0, 0, 0, 1 };
      int i;

      /* New size is smaller - just need to fill in some zeros.
       */
      for (i = sz ; i < tnl->vtx.attrib_sz[attr] ; i++)
	 tnl->vtx.attrptr[attr][i].f = id[i];
   }
   
   /* Reset the dispatch table - aliasing entrypoints are invalidated.
    */
   _tnl_reset_attr_dispatch_tab( ctx );
}

static int dispatch_offset[TNL_ATTRIB_MAX][4][2];


static void *lookup_or_generate( GLuint attr, GLuint sz, GLuint v,
				 void *fallback_attr_func )
{ 
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   void *ptr = 0;
   struct dynfn *dfn;
   int key;

   /* This will remove any installed handlers for attr with different
    * sz, will flush, copy and expand the copied vertices if sz won't
    * fit in the current vertex, or will clean the current vertex if
    * it already has this attribute in a larger size.
    */
   if (tnl->vtx.attrib_sz[attr] != sz)
      _tnl_fixup_vertex( ctx, attr, sz );


   if (attr == 0) 
      key = tnl->vtx.vertex_size;
   else
      key = (GLuint)tnl->vtx.attrptr[attr];

   for (dfn = tnl->vtx.generated[sz-1][v][isvertex]; dfn; dfn = dfn->next) {
      if (dfn->key == key) {
	 ptr = dfn->code;
	 break;
      }
   }

   if (ptr == 0) {
      dfn = tnl->vtx.codegen[sz-1][v][isvertex]( ctx, key );
      if (dfn) {
	 ptr = dfn->code;
	 dfn->next = tnl->vtx.generated[sz-1][v][isvertex];
	 tnl->vtx.generated[sz-1][v][isvertex] = dfn;
      }
   }
      
   if (ptr == 0)
      ptr = fallback_attr_func;

   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;

   /* Need to set all the aliases to this function, too
    */
   if (dispatch_offset[attr][sz-1][v]) 
      ((void **)tnl->Exec)[dispatch_offset[attr][sz-1][v]] = ptr;

   return ptr;
}


/* These functions choose one of the ATTR's generated above (or from
 * codegen).  Like the ATTR functions, they live in the GL dispatch
 * table and in the second-level dispatch table for MultiTexCoord,
 * AttribNV, etc.
 *
 * Need ATTR1 for use in constructing name of 'attrib_x_y_z' function.
 */
#define CHOOSE( ATTR1, ATTR2, SZ, V, FNTYPE, ARGS1, ARGS2 )	\
static void choose_##ATTR2##_##SZ##_##V ARGS1			\
{								\
   void *ptr = lookup_or_generate(ATTR1, SZ, V,			\
		      (void *)attrib_##ATTR1##_##SZ##_##V );	\
								\
   assert(ATTR1 == ATTR2);					\
   ((FNTYPE) ptr) ARGS2;					\
}

#define afv (const GLfloat *v)
#define a1f (GLfloat a)
#define a2f (GLfloat a, GLfloat b)
#define a3f (GLfloat a, GLfloat b, GLfloat c)
#define a4f (GLfloat a, GLfloat b, GLfloat c, GLfloat d)

/* Not that many entrypoints when it all boils down:
 */
CHOOSE( 0, VERT_ATTRIB_POS, 2, 1, pfv, afv, (v) )
CHOOSE( 0, VERT_ATTRIB_POS, 2, 0, p2f, a2f, (a,b) )
CHOOSE( 0, VERT_ATTRIB_POS, 3, 1, pfv, afv, (v) )
CHOOSE( 0, VERT_ATTRIB_POS, 3, 0, p3f, a3f, (a,b,c) )
CHOOSE( 0, VERT_ATTRIB_POS, 4, 1, pfv, afv, (v) )
CHOOSE( 0, VERT_ATTRIB_POS, 4, 0, p4f, a4f, (a,b,c,d) )
CHOOSE( 2, VERT_ATTRIB_NORMAL, 3, 1, pfv, afv, (v) )
CHOOSE( 2, VERT_ATTRIB_NORMAL, 3, 0, p3f, a3f, (a,b,c) )
CHOOSE( 3, VERT_ATTRIB_COLOR0, 3, 1, pfv, afv, (v) )
CHOOSE( 3, VERT_ATTRIB_COLOR0, 3, 0, p3f, a3f, (a,b,c) )
CHOOSE( 3, VERT_ATTRIB_COLOR0, 4, 1, pfv, afv, (v) )
CHOOSE( 3, VERT_ATTRIB_COLOR0, 4, 0, p4f, a4f, (a,b,c,d) )
CHOOSE( 4, VERT_ATTRIB_COLOR1, 3, 1, pfv, afv, (v) )
CHOOSE( 4, VERT_ATTRIB_COLOR1, 3, 0, p3f, a3f, (a,b,c) )
CHOOSE( 5, VERT_ATTRIB_FOG, 1, 1, pfv, afv, (v) )
CHOOSE( 5, VERT_ATTRIB_FOG, 1, 0, p1f, a1f, (a) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 1, 1, pfv, afv, (v) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 1, 0, p1f, a1f, (a) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 2, 1, pfv, afv, (v) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 2, 0, p2f, a2f, (a,b) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 3, 1, pfv, afv, (v) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 3, 0, p3f, a3f, (a,b,c) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 4, 1, pfv, afv, (v) )
CHOOSE( 8, VERT_ATTRIB_TEX0, 4, 0, p4f, a4f, (a,b,c,d) )


/* Gack.  Need to do this without going through the
 * GET_CURRENT_CONTEXT hoohah.  Could codegen this, I suppose...
 */
#define DISPATCH_ATTRFV( ATTR, COUNT, P )	\
do {						\
   GET_CURRENT_CONTEXT( ctx ); 			\
   TNLcontext *tnl = TNL_CONTEXT(ctx); 		\
   tnl->vtx.tabfv[COUNT-1][ATTR]( P );		\
} while (0)

#define DISPATCH_ATTR1FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 1, V )
#define DISPATCH_ATTR2FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 2, V )
#define DISPATCH_ATTR3FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 3, V )
#define DISPATCH_ATTR4FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 4, V )

#define DISPATCH_ATTR1F( ATTR, S ) DISPATCH_ATTRFV( ATTR, 1, &(S) )
#ifdef USE_X86_ASM
/* Naughty cheat:
 */
#define DISPATCH_ATTR2F( ATTR, S,T ) DISPATCH_ATTRFV( ATTR, 2, &(S) )
#define DISPATCH_ATTR3F( ATTR, S,T,R ) DISPATCH_ATTRFV( ATTR, 3, &(S) )
#define DISPATCH_ATTR4F( ATTR, S,T,R,Q ) DISPATCH_ATTRFV( ATTR, 4, &(S) )
#else
/* Safe:
 */
#define DISPATCH_ATTR2F( ATTR, S,T ) 		\
do { 						\
   GLfloat v[2]; 				\
   v[0] = S; v[1] = T;				\
   DISPATCH_ATTR2FV( ATTR, v );			\
} while (0)
#define DISPATCH_ATTR3F( ATTR, S,T,R ) 		\
do { 						\
   GLfloat v[3]; 				\
   v[0] = S; v[1] = T; v[2] = R;		\
   DISPATCH_ATTR3FV( ATTR, v );			\
} while (0)
#define DISPATCH_ATTR4F( ATTR, S,T,R,Q )	\
do { 						\
   GLfloat v[4]; 				\
   v[0] = S; v[1] = T; v[2] = R; v[3] = Q;	\
   DISPATCH_ATTR4FV( ATTR, v );			\
} while (0)
#endif



static void enum_error( void )
{
   GET_CURRENT_CONTEXT( ctx );
   _mesa_error( ctx, GL_INVALID_ENUM, __FUNCTION__ );
}

static void op_error( void )
{
   GET_CURRENT_CONTEXT( ctx );
   _mesa_error( ctx, GL_INVALID_OPERATION, __FUNCTION__ );
}


/* First level for MultiTexcoord:  Send through second level dispatch.
 * These are permanently installed in the toplevel dispatch.
 *
 * Assembly can optimize the generation of arrays by using &s instead
 * of building 'v'.
 */
static void _tnl_MultiTexCoord1f( GLenum target, GLfloat s  )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, &s );
}

static void _tnl_MultiTexCoord1fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, v );
}

static void _tnl_MultiTexCoord2f( GLenum target, GLfloat s, GLfloat t )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR2F( attr, s, t );
}

static void _tnl_MultiTexCoord2fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR2FV( attr, v );
}

static void _tnl_MultiTexCoord3f( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR3F( attr, s, t, r );
}

static void _tnl_MultiTexCoord3fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR3FV( attr, v );
}

static void _tnl_MultiTexCoord4f( GLenum target, GLfloat s, GLfloat t,
				    GLfloat r, GLfloat q )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR4F( attr, s, t, r, q );
}

static void _tnl_MultiTexCoord4fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   DISPATCH_ATTR4FV( attr, v );
}


/* First level for NV_vertex_program:
 *
 * Check for errors & reroute through second dispatch layer to get
 * size tracking per-attribute.
 */
static void _tnl_VertexAttrib1fNV( GLuint index, GLfloat s )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR1F( index, s );
   else
      enum_error(); 
}

static void _tnl_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR1FV( index, v );
   else
      enum_error();
}

static void _tnl_VertexAttrib2fNV( GLuint index, GLfloat s, GLfloat t )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR2F( index, s, t );
   else
      enum_error();
}

static void _tnl_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR2FV( index, v );
   else
      enum_error();
}

static void _tnl_VertexAttrib3fNV( GLuint index, GLfloat s, GLfloat t, 
				  GLfloat r )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR3F( index, s, t, r );
   else
      enum_error();
}

static void _tnl_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR3FV( index, v );
   else
      enum_error();
}

static void _tnl_VertexAttrib4fNV( GLuint index, GLfloat s, GLfloat t,
				  GLfloat r, GLfloat q )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR4F( index, s, t, r, q );
   else
      enum_error();
}

static void _tnl_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   if (index < TNL_ATTRIB_MAX)
      DISPATCH_ATTR4FV( index, v );
   else
      enum_error();
}


/* Materials:  
 * 
 * These are treated as per-vertex attributes, at indices above where
 * the NV_vertex_program leaves off.  There are a lot of good things
 * about treating materials this way.  
 *
 * However: I don't want to double the number of generated functions
 * just to cope with this, so I unroll the 'C' varients of CHOOSE and
 * ATTRF into this function, and dispense with codegen and
 * second-level dispatch.
 */
#define MAT_ATTR( A, N, params )			\
do {							\
   if (tnl->vtx.attrib_sz[A] != N) {			\
      tnl_fixup_vertex( ctx, A, N );			\
   }							\
							\
   {							\
      union uif *dest = tnl->vtx.attrptr[A];	      	\
      if (N>0) dest[0].f = params[0];			\
      if (N>1) dest[1].f = params[1];			\
      if (N>2) dest[2].f = params[2];			\
      if (N>3) dest[3].f = params[3];			\
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;	\
   }							\
} while (0)


#define MAT( ATTR, N, face, params )			\
do {							\
   if (face != GL_BACK)					\
      MAT_ATTR( ATTR, N, params ); /* front */		\
   if (face != GL_FRONT)				\
      MAT_ATTR( ATTR + 1, N, params ); /* back */	\
} while (0)


/* NOTE: Have to remove/deal-with colormaterial crossovers, probably
 * later on - in the meantime just store everything.  
 */
static void _tnl_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   switch (pname) {
   case GL_EMISSION:
      MAT( VERT_ATTRIB_MAT_FRONT_EMISSION, 4, face, params );
      break;
   case GL_AMBIENT:
      MAT( VERT_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      break;
   case GL_DIFFUSE:
      MAT( VERT_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   case GL_SPECULAR:
      MAT( VERT_ATTRIB_MAT_FRONT_SPECULAR, 4, face, params );
      break;
   case GL_SHININESS:
      MAT( VERT_ATTRIB_MAT_FRONT_SHININESS, 1, face, params );
      break;
   case GL_COLOR_INDEXES:
      MAT( VERT_ATTRIB_MAT_FRONT_INDEXES, 3, face, params );
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      MAT( VERT_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      MAT( VERT_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, __FUNCTION__ );
      return;
   }
}


#define IDX_ATTR( A, IDX )				\
do {							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if (tnl->vtx.attrib_sz[A] != 1) {			\
      tnl_fixup_vertex( ctx, A, 1 );			\
   }							\
							\
   {							\
      union uif *dest = tnl->vtx.attrptr[A];		\
      dest[0].ui = IDX;				\
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;	\
   }							\
} while (0)


static void _tnl_EdgeFlag( GLboolean f )
{
   IDX_ATTR( VERT_ATTRIB_EDGEFLAG, f );
}

static void _tnl_EdgeFlagv( const GLboolean *f )
{
   IDX_ATTR( VERT_ATTRIB_EDGEFLAG, f[0] );
}

static void _tnl_Indexi( GLint i )
{
   IDX_ATTR( VERT_ATTRIB_INDEX, i );
}

static void _tnl_Indexiv( const GLint *i )
{
   IDX_ATTR( VERT_ATTRIB_INDEX, i[0] );
}



/* EvalCoord needs special treatment as ususal:
 */
static void evalcoord( GLfloat a, GLfloat b, GLuint type ) 
{
#if 0
   GET_CURRENT_CONTEXT( ctx );					
   TNLcontext *tnl = TNL_CONTEXT(ctx);				
   
   /* Initialize the list of eval fixups:
    */
   if (!tnl->evalptr) {
      init_eval_ptr( ctx );
   }

   /* Note that this vertex will need to be fixed up:
    */
   tnl->evalptr[0].vert = tnl->initial_counter - tnl->counter;
   tnl->evalptr[0].type = type;

   /* Now emit the vertex with eval data in obj coordinates:
    */
   ATTRF( 0, 2, a, b, 0, 1 );
#endif
}


static void _tnl_EvalCoord1f( GLfloat u )
{
   evalcoord( u, 0, TNL_EVAL_COORD1 );
}

static void _tnl_EvalCoord1fv( const GLfloat *v )
{
   evalcoord( v[0], 0, TNL_EVAL_COORD1 );
}

static void _tnl_EvalCoord2f( GLfloat u, GLfloat v )
{
   evalcoord( u, v, TNL_EVAL_COORD2 );
}

static void _tnl_EvalCoord2fv( const GLfloat *v )
{
   evalcoord( v[0], v[1], TNL_EVAL_COORD2 );
}

static void _tnl_EvalPoint1( GLint i )
{
   evalcoord( (GLfloat)i, 0, TNL_EVAL_POINT1 );
}

static void _tnl_EvalPoint2( GLint i, GLint j )
{
   evalcoord( (GLfloat)i, (GLfloat)j, TNL_EVAL_POINT2 );
}

/* Don't do a lot of processing here - errors are raised when this
 * list is scanned later on (perhaps in display list playback) to
 * build tnl_prim structs.
 */
static void _tnl_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   int i;

   i = tnl->vtx.be_count++;
   tnl->vtx.be[i].type = TNL_BEGIN;
   tnl->vtx.be[i].idx = tnl->vtx.initial_counter - tnl->vtx.counter;
   tnl->vtx.be[i].mode = mode;

   if (tnl->vtx.be_count == TNL_BE_MAX)
      _tnl_FlushVertices( ctx, FLUSH_STORED_VERTICES );	
}

static void _tnl_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   int i;

   i = tnl->vtx.be_count++;
   tnl->vtx.be[i].type = TNL_END;
   tnl->vtx.be[i].idx = tnl->vtx.initial_counter - tnl->vtx.counter;
   tnl->vtx.be[i].mode = 0;

   if (tnl->vtx.be_count == TNL_BE_MAX)
      _tnl_FlushVertices( ctx, FLUSH_STORED_VERTICES );	
}







static void _tnl_InitDispatch( struct _glapi_table *tab )
{
   GLint i;
   
   /* Most operations boil down to error/transition behaviour.
    * However if we transition eagerly, all that's needed is a single
    * 'error' operation.  This will do for now, but requires that the
    * old 'flush' stuff lives on in the state functions, and is
    * wasteful if swapping is expensive (threads?).
    */
   for (i = 0 ; i < sizeof(tab)/sizeof(void*) ; i++) 
      ((void **)tab)[i] = (void *)op_error;

   tab->Begin = _tnl_Begin;
   tab->End = _tnl_End;
   tab->Color3f = choose_VERT_ATTRIB_COLOR0_3_0;
   tab->Color3fv = choose_VERT_ATTRIB_COLOR0_3_1;
   tab->Color4f = choose_VERT_ATTRIB_COLOR0_4_0;
   tab->Color4fv = choose_VERT_ATTRIB_COLOR0_4_1;
   tab->SecondaryColor3fEXT = choose_VERT_ATTRIB_COLOR1_3_0;
   tab->SecondaryColor3fvEXT = choose_VERT_ATTRIB_COLOR1_3_1;
   tab->MultiTexCoord1fARB = _tnl_MultiTexCoord1f;
   tab->MultiTexCoord1fvARB = _tnl_MultiTexCoord1fv;
   tab->MultiTexCoord2fARB = _tnl_MultiTexCoord2f;
   tab->MultiTexCoord2fvARB = _tnl_MultiTexCoord2fv;
   tab->MultiTexCoord3fARB = _tnl_MultiTexCoord3f;
   tab->MultiTexCoord3fvARB = _tnl_MultiTexCoord3fv;
   tab->MultiTexCoord4fARB = _tnl_MultiTexCoord4f;
   tab->MultiTexCoord4fvARB = _tnl_MultiTexCoord4fv;
   tab->Normal3f = choose_VERT_ATTRIB_NORMAL_3_0;
   tab->Normal3fv = choose_VERT_ATTRIB_NORMAL_3_1;
   tab->TexCoord1f = choose_VERT_ATTRIB_TEX0_1_0;
   tab->TexCoord1fv = choose_VERT_ATTRIB_TEX0_1_1;
   tab->TexCoord2f = choose_VERT_ATTRIB_TEX0_2_0;
   tab->TexCoord2fv = choose_VERT_ATTRIB_TEX0_2_1;
   tab->TexCoord3f = choose_VERT_ATTRIB_TEX0_3_0;
   tab->TexCoord3fv = choose_VERT_ATTRIB_TEX0_3_1;
   tab->TexCoord4f = choose_VERT_ATTRIB_TEX0_4_0;
   tab->TexCoord4fv = choose_VERT_ATTRIB_TEX0_4_1;
   tab->Vertex2f = choose_VERT_ATTRIB_POS_2_0;
   tab->Vertex2fv = choose_VERT_ATTRIB_POS_2_1;
   tab->Vertex3f = choose_VERT_ATTRIB_POS_3_0;
   tab->Vertex3fv = choose_VERT_ATTRIB_POS_3_1;
   tab->Vertex4f = choose_VERT_ATTRIB_POS_4_0;
   tab->Vertex4fv = choose_VERT_ATTRIB_POS_4_1;
   tab->FogCoordfEXT = choose_VERT_ATTRIB_FOG_1_0;
   tab->FogCoordfvEXT = choose_VERT_ATTRIB_FOG_1_1;
   tab->EdgeFlag = _tnl_EdgeFlag;
   tab->EdgeFlagv = _tnl_EdgeFlagv;
   tab->Indexi = _tnl_Indexi;
   tab->Indexiv = _tnl_Indexiv;
   tab->EvalCoord1f = _tnl_EvalCoord1f;
   tab->EvalCoord1fv = _tnl_EvalCoord1fv;
   tab->EvalCoord2f = _tnl_EvalCoord2f;
   tab->EvalCoord2fv = _tnl_EvalCoord2fv;
   tab->Materialfv = _tnl_Materialfv;
   tab->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
}



static struct dynfn *codegen_noop( GLcontext *ctx, int key )
{
   (void) ctx; (void) key;
   return 0;
}

static void _tnl_InitCodegen( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);				
   int sz, v, z;

   /* attr[n][v] 
    * vertex[n][v]
    *
    * Generated functions parameterized by:
    *    nr     1..4
    *    v      y/n
    *    vertex y/n
    *
    * Vertex functions also parameterized by:
    *    vertex_size
    *  
    * Attr functions also parameterized by:
    *    pointer   (destination to receive data)
    */
   for (sz = 1 ; sz < 5 ; sz++) {
      for (v = 0 ; v < 2 ; v++) {
	 for (z = 0 ; z < 2 ; z++) {
	    tnl->vtx.codegen[sz-1][v][z] = codegen_noop;
	    tnl->vtx.generated[sz-1][v][z] = 0;
	 }
      }
   }

#if 0
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
#endif
}




void _tnl_vtx_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   _tnl_InitDispatch( tnl->Exec );
   _tnl_InitCodegen( ctx );
}



void _tnl_vtx_destroy( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);				
   int sz, v, z;
   struct dynfn *dfn, *next;

   for (sz = 1 ; sz <= 4 ; sz++) {
      for (v = 0 ; v <= 1 ; v++) {
	 for (z = 0 ; z <= 1 ; z++) {
	    dfn = tnl->vtx.generated[sz-1][v][z];
	    while (dfn) {
	       next = dfn->next;
	       FREE(dfn);
	       dfn = next;
	    }
	 }
      }
   }
}

