/* $Id: t_vb_render.c,v 1.6 2001/01/05 02:26:49 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * Author:
 *    Keith Whitwell <keithw@valinux.com>
 */


/*
 * Render whole vertex buffers, including projection of vertices from
 * clip space and clipping of primitives.
 *
 * This file makes calls to project vertices and to the point, line
 * and triangle rasterizers via the function pointers:
 *
 *    context->Driver.BuildProjectedVertices()
 *
 *    context->Driver.PointsFunc()
 *    context->Driver.LineFunc()
 *    context->Driver.TriangleFunc()
 *    context->Driver.QuadFunc() 
 *
 *    context->Driver.RenderTabVerts[] 
 *    context->Driver.RenderTabElts[]
 *
 * None of these may be null.
 */


#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "mem.h"
#include "mtypes.h"
#include "mmath.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"

#include "t_pipeline.h"



typedef void (*clip_line_func)( GLcontext *ctx,
				GLuint i, GLuint j,
				GLubyte mask);

typedef void (*clip_poly_func)( GLcontext *ctx,
				GLuint n, GLuint vlist[],
				GLubyte mask );




struct render_stage_data {

   /* Clipping functions for current state.
    */
   interp_func interp;		/* Clip interpolation function */
   copy_pv_func copypv;		/* Flatshade fixup function */
   GLuint _ClipInputs;		/* Inputs referenced by interpfunc */
};

#define RENDER_STAGE_DATA(stage) ((struct render_stage_data *)stage->private)

				  
/**********************************************************************/
/*           Interpolate between pairs of vertices                    */
/**********************************************************************/


#define LINTERP_SZ( t, vec, to, a, b, sz )			\
do {								\
   switch (sz) {						\
   case 4: vec[to][3] = LINTERP( t, vec[a][3], vec[b][3] );	\
   case 3: vec[to][2] = LINTERP( t, vec[a][2], vec[b][2] );	\
   case 2: vec[to][1] = LINTERP( t, vec[a][1], vec[b][1] );	\
   case 1: vec[to][0] = LINTERP( t, vec[a][0], vec[b][0] );	\
   }								\
} while(0)


#if 1

#define LINTERP_RGBA(nr, t, out, a, b) {	\
   int i;					\
   for (i = 0; i < nr; i++) {			\
      GLfloat fa = CHAN_TO_FLOAT(a[i]);		\
      GLfloat fb = CHAN_TO_FLOAT(b[i]);		\
      GLfloat fo = LINTERP(t, fa, fb);		\
      CLAMPED_FLOAT_TO_CHAN(out[i], fo);	\
   }						\
}

#else

#define LINTERP_RGBA(nr, t, out, a, b) {			\
   int n;							\
   const GLuint ti = FloatToInt(t*256.0F);			\
   const GLubyte *Ib = (const GLubyte *)&a[0];			\
   const GLubyte *Jb = (const GLubyte *)&b[0];			\
   GLubyte *Ob = (GLubyte *)&out[0];				\
								\
   for (n = 0 ; n < nr ; n++)					\
      Ob[n] = (GLubyte) (Ib[n] + ((ti * (Jb[n] - Ib[n]))/256));	\
}

#endif




#define INTERP_RGBA    0x1
#define INTERP_TEX     0x2
#define INTERP_INDEX   0x4
#define INTERP_SPEC    0x8
#define INTERP_FOG     0x10
#define INTERP_EDGE    0x20
#define MAX_INTERP     0x40

static interp_func interp_tab[MAX_INTERP];
static copy_pv_func copy_tab[MAX_INTERP];


#define IND (0)
#define NAME interp_none
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG)
#define NAME interp_FOG
#include "t_vb_interptmp.h"

#define IND (INTERP_TEX)
#define NAME interp_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_TEX)
#define NAME interp_FOG_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_EDGE)
#define NAME interp_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_EDGE)
#define NAME interp_FOG_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_TEX|INTERP_EDGE)
#define NAME interp_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_TEX|INTERP_EDGE)
#define NAME interp_FOG_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA)
#define NAME interp_RGBA
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC)
#define NAME interp_RGBA_SPEC
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_FOG)
#define NAME interp_RGBA_FOG
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_FOG)
#define NAME interp_RGBA_SPEC_FOG
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_TEX)
#define NAME interp_RGBA_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_TEX)
#define NAME interp_RGBA_SPEC_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_FOG|INTERP_TEX)
#define NAME interp_RGBA_FOG_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_TEX)
#define NAME interp_RGBA_SPEC_FOG_TEX
#include "t_vb_interptmp.h"

#define IND (INTERP_INDEX)
#define NAME interp_INDEX
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_INDEX)
#define NAME interp_FOG_INDEX
#include "t_vb_interptmp.h"

#define IND (INTERP_TEX|INTERP_INDEX)
#define NAME interp_TEX_INDEX
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_TEX|INTERP_INDEX)
#define NAME interp_FOG_TEX_INDEX
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_EDGE)
#define NAME interp_RGBA_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_EDGE)
#define NAME interp_RGBA_SPEC_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_FOG|INTERP_EDGE)
#define NAME interp_RGBA_FOG_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_EDGE)
#define NAME interp_RGBA_SPEC_FOG_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_TEX|INTERP_EDGE)
#define NAME interp_RGBA_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_TEX|INTERP_EDGE)
#define NAME interp_RGBA_SPEC_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_FOG|INTERP_TEX|INTERP_EDGE)
#define NAME interp_RGBA_FOG_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_TEX|INTERP_EDGE)
#define NAME interp_RGBA_SPEC_FOG_TEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_INDEX|INTERP_EDGE)
#define NAME interp_INDEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_INDEX|INTERP_EDGE)
#define NAME interp_FOG_INDEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_TEX|INTERP_INDEX|INTERP_EDGE)
#define NAME interp_TEX_INDEX_EDGE
#include "t_vb_interptmp.h"

#define IND (INTERP_FOG|INTERP_TEX|INTERP_INDEX|INTERP_EDGE)
#define NAME interp_FOG_TEX_INDEX_EDGE
#include "t_vb_interptmp.h"


#define IND (INTERP_RGBA)
#define NAME copy_RGBA
#include "t_vb_flattmp.h"

#define IND (INTERP_RGBA|INTERP_SPEC)
#define NAME copy_RGBA_SPEC
#include "t_vb_flattmp.h"

#define IND (INTERP_INDEX)
#define NAME copy_INDEX
#include "t_vb_flattmp.h"




static void interp_invalid( GLcontext *ctx,
			    GLfloat t, 
			    GLuint dst, GLuint in, GLuint out,
			    GLboolean boundary )
{
   (void)(ctx && t && in && out && boundary);
   fprintf(stderr, "Invalid interpolation function in t_vbrender.c\n");
}

static void copy_invalid( GLcontext *ctx, GLuint dst, GLuint src )
{
   (void)(ctx && dst && src);
   fprintf(stderr, "Invalid copy function in t_vbrender.c\n");
}


static void interp_init( void )
{
   GLuint i;

   /* Use the maximal function as the default.  I don't believe any of
    * the non-implemented combinations are reachable, but this gives
    * some safety from crashes.
    */
   for (i = 0 ; i < Elements(interp_tab) ; i++) {
      interp_tab[i] = interp_invalid;
      copy_tab[i] = copy_invalid;
   }

   interp_tab[0] = interp_none;
   interp_tab[INTERP_FOG] = interp_FOG;
   interp_tab[INTERP_TEX] = interp_TEX;
   interp_tab[INTERP_FOG|INTERP_TEX] = interp_FOG_TEX;
   interp_tab[INTERP_EDGE] = interp_EDGE;
   interp_tab[INTERP_FOG|INTERP_EDGE] = interp_FOG_EDGE;
   interp_tab[INTERP_TEX|INTERP_EDGE] = interp_TEX_EDGE;
   interp_tab[INTERP_FOG|INTERP_TEX|INTERP_EDGE] = interp_FOG_TEX_EDGE;

   interp_tab[INTERP_RGBA] = interp_RGBA;
   interp_tab[INTERP_RGBA|INTERP_SPEC] = interp_RGBA_SPEC;
   interp_tab[INTERP_RGBA|INTERP_FOG] = interp_RGBA_FOG;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_FOG] = interp_RGBA_SPEC_FOG;
   interp_tab[INTERP_RGBA|INTERP_TEX] = interp_RGBA_TEX;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_TEX] = interp_RGBA_SPEC_TEX;
   interp_tab[INTERP_RGBA|INTERP_FOG|INTERP_TEX] = interp_RGBA_FOG_TEX;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_TEX] = 
      interp_RGBA_SPEC_FOG_TEX;
   interp_tab[INTERP_INDEX] = interp_INDEX;
   interp_tab[INTERP_FOG|INTERP_INDEX] = interp_FOG_INDEX;
   interp_tab[INTERP_TEX|INTERP_INDEX] = interp_TEX_INDEX;
   interp_tab[INTERP_FOG|INTERP_TEX|INTERP_INDEX] = interp_FOG_TEX_INDEX;
   interp_tab[INTERP_RGBA|INTERP_EDGE] = interp_RGBA_EDGE;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_EDGE] = interp_RGBA_SPEC_EDGE;
   interp_tab[INTERP_RGBA|INTERP_FOG|INTERP_EDGE] = interp_RGBA_FOG_EDGE;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_EDGE] = 
      interp_RGBA_SPEC_FOG_EDGE;
   interp_tab[INTERP_RGBA|INTERP_TEX|INTERP_EDGE] = interp_RGBA_TEX_EDGE;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_TEX|INTERP_EDGE] = 
      interp_RGBA_SPEC_TEX_EDGE;
   interp_tab[INTERP_RGBA|INTERP_FOG|INTERP_TEX|INTERP_EDGE] = 
      interp_RGBA_FOG_TEX_EDGE;
   interp_tab[INTERP_RGBA|INTERP_SPEC|INTERP_FOG|INTERP_TEX|INTERP_EDGE] = 
      interp_RGBA_SPEC_FOG_TEX_EDGE;
   interp_tab[INTERP_INDEX|INTERP_EDGE] = interp_INDEX_EDGE;
   interp_tab[INTERP_FOG|INTERP_INDEX|INTERP_EDGE] = interp_FOG_INDEX_EDGE;
   interp_tab[INTERP_TEX|INTERP_INDEX|INTERP_EDGE] = interp_TEX_INDEX_EDGE;
   interp_tab[INTERP_FOG|INTERP_TEX|INTERP_INDEX|INTERP_EDGE] = 
      interp_FOG_TEX_INDEX_EDGE;


   copy_tab[INTERP_RGBA] = copy_RGBA;
   copy_tab[INTERP_RGBA|INTERP_SPEC] = copy_RGBA_SPEC;
   copy_tab[INTERP_INDEX] = copy_INDEX;

}


/**********************************************************************/
/*                        Clip single primitives                      */
/**********************************************************************/


#if defined(USE_IEEE) 
#define NEGATIVE(x) ((*(GLuint *)&x) & (1<<31))
#define DIFFERENT_SIGNS(x,y) (((*(GLuint *)&x)^(*(GLuint *)&y)) & (1<<31))
#else
#define NEGATIVE(x) (x < 0)
#define DIFFERENT_SIGNS(x,y) (x * y <= 0 && x - y != 0)
/* Could just use (x*y<0) except for the flatshading requirements.
 * Maybe there's a better way?
 */
#endif

#define W(i) coord[i][3]
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 4
#define TAG(x) x##_4
#include "t_vb_cliptmp.h"

#define W(i) 1.0
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 3
#define TAG(x) x##_3
#include "t_vb_cliptmp.h"

#define W(i) 1.0
#define Z(i) 0.0
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 2
#define TAG(x) x##_2
#include "t_vb_cliptmp.h"

static clip_poly_func clip_poly_tab[5] = {
   0,
   0,
   clip_polygon_2,
   clip_polygon_3,
   clip_polygon_4
};

static clip_line_func clip_line_tab[5] = {
   0,
   0,
   clip_line_2,
   clip_line_3,
   clip_line_4
};


/**********************************************************************/
/*              Clip and render whole begin/end objects               */
/**********************************************************************/

#define NEED_EDGEFLAG_SETUP (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define EDGEFLAG_GET(idx) VB->EdgeFlag[idx]
#define EDGEFLAG_SET(idx, val) VB->EdgeFlag[idx] = val


/* Vertices, with the possibility of clipping.
 */
#define RENDER_POINTS( start, count ) \
   ctx->Driver.PointsFunc( ctx, start, count )

#define RENDER_LINE( v1, v2 )			\
do {						\
   GLubyte c1 = mask[v1], c2 = mask[v2];	\
   GLubyte ormask = c1|c2;			\
   if (!ormask)					\
      LineFunc( ctx, v1, v2 );			\
   else if (!(c1 & c2 & 0x3f))			\
      clip_line_tab[sz]( ctx, v1, v2, ormask );	\
} while (0)

#define RENDER_TRI( v1, v2, v3 )			\
do {							\
   GLubyte c1 = mask[v1], c2 = mask[v2], c3 = mask[v3];	\
   GLubyte ormask = c1|c2|c3;				\
   if (!ormask)						\
      TriangleFunc( ctx, v1, v2, v3 );			\
   else if (!(c1 & c2 & c3 & 0x3f)) {			\
      GLuint vlist[MAX_CLIPPED_VERTICES];		\
      ASSIGN_3V(vlist, v3, v1, v2 );			\
      clip_poly_tab[sz]( ctx, 3, vlist, ormask );	\
   }							\
} while (0)

#define RENDER_QUAD( v1, v2, v3, v4 )			\
do {							\
   GLubyte c1 = mask[v1], c2 = mask[v2];		\
   GLubyte c3 = mask[v3], c4 = mask[v4];		\
   GLubyte ormask = c1|c2|c3|c4;			\
   if (!ormask)						\
      QuadFunc( ctx, v1, v2, v3, v4 );			\
   else if (!(c1 & c2 & c3 & c4 & 0x3f)) {		\
      GLuint vlist[MAX_CLIPPED_VERTICES];		\
      ASSIGN_4V(vlist, v4, v1, v2, v3 );		\
      clip_poly_tab[sz]( ctx, 4, vlist, ormask );	\
   }							\
} while (0)


#define LOCAL_VARS							\
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;			\
    const GLuint * const elt = VB->Elts;				\
    const GLubyte *mask = VB->ClipMask;					\
    const GLuint sz = VB->ClipPtr->size;				\
    const line_func LineFunc = ctx->Driver.LineFunc;			\
    const triangle_func TriangleFunc = ctx->Driver.TriangleFunc;	\
    const quad_func QuadFunc = ctx->Driver.QuadFunc;			\
    (void) (LineFunc && TriangleFunc && QuadFunc);			\
    (void) elt; (void) mask; (void) sz;

#define TAG(x) clip_##x##_verts
#define RESET_STIPPLE ctx->Driver.ResetLineStipple( ctx )
#define RESET_OCCLUSION ctx->OcclusionResult = GL_TRUE;
#define PRESERVE_VB_DEFS
#include "t_vb_rendertmp.h"



/* Elts, with the possibility of clipping.
 */
#undef ELT
#undef TAG
#define ELT(x) elt[x]
#define TAG(x) clip_##x##_elts
#include "t_vb_rendertmp.h"


/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/

#define NEED_EDGEFLAG_SETUP (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define EDGEFLAG_GET(idx) VB->EdgeFlag[idx]
#define EDGEFLAG_SET(idx, val) VB->EdgeFlag[idx] = val


/* Vertices, no clipping.
 */
#define RENDER_POINTS( start, count ) \
   ctx->Driver.PointsFunc( ctx, start, count )

#define RENDER_LINE( v1, v2 ) \
   LineFunc( ctx, v1, v2 )

#define RENDER_TRI( v1, v2, v3 )		\
   TriangleFunc( ctx, v1, v2, v3 )

#define RENDER_QUAD( v1, v2, v3, v4 )	\
   QuadFunc( ctx, v1, v2, v3, v4 )

#define TAG(x) _tnl_##x##_verts

#define LOCAL_VARS							\
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;			\
    const GLuint * const elt = VB->Elts;				\
    const line_func LineFunc = ctx->Driver.LineFunc;			\
    const triangle_func TriangleFunc = ctx->Driver.TriangleFunc;	\
    const quad_func QuadFunc = ctx->Driver.QuadFunc;			\
    (void) (LineFunc && TriangleFunc && QuadFunc);			\
    (void) elt;

#define RESET_STIPPLE ctx->Driver.ResetLineStipple( ctx )
#define RESET_OCCLUSION ctx->OcclusionResult = GL_TRUE;
#define RENDER_TAB_QUALIFIER 
#define PRESERVE_VB_DEFS
#include "t_vb_rendertmp.h"


/* Elts, no clipping.
 */
#undef ELT
#define TAG(x) _tnl_##x##_elts
#define ELT(x) elt[x]
#include "t_vb_rendertmp.h"




/**********************************************************************/
/*              Clip and render whole vertex buffers                  */
/**********************************************************************/


static GLboolean run_render( GLcontext *ctx,
			     struct gl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb; 
   GLuint new_inputs = stage->changed_inputs;
   render_func *tab;
   GLint pass = 0;

   VB->interpfunc = RENDER_STAGE_DATA(stage)->interp;
   VB->copypvfunc = RENDER_STAGE_DATA(stage)->copypv;

   /* Allow the drivers to lock before projected verts are built so
    * that window coordinates are guarenteed not to change before
    * rendering.
    */
   if (ctx->Driver.RenderStart)
      ctx->Driver.RenderStart( ctx );
   
   if (VB->ClipOrMask) {
      tab = VB->Elts ? clip_render_tab_elts : clip_render_tab_verts;

      if (new_inputs & VB->importable_data) 
	 VB->import_data( ctx,
			  new_inputs & VB->importable_data,
			  VEC_NOT_WRITEABLE|VEC_BAD_STRIDE);
   }
   else {
      tab = VB->Elts ? ctx->Driver.RenderTabElts : ctx->Driver.RenderTabVerts;
   } 

   ctx->Driver.BuildProjectedVertices( ctx, 0, VB->Count, new_inputs );

   do
   {
      GLuint i, length, flags = 0;
      for (i = 0 ; !(flags & PRIM_LAST) ; i += length)
      {
	 flags = VB->Primitive[i];
	 length= VB->PrimitiveLength[i];	
	 ASSERT(length || (flags & PRIM_LAST));
	 ASSERT((flags & PRIM_MODE_MASK) <= GL_POLYGON+1);
	 if (length)
	    tab[flags & PRIM_MODE_MASK]( ctx, i, i + length, flags );
      }
   } while (ctx->Driver.MultipassFunc &&
	    ctx->Driver.MultipassFunc( ctx, ++pass ));

   if (ctx->Driver.RenderFinish)
      ctx->Driver.RenderFinish( ctx );

   return GL_FALSE;		/* finished the pipe */
}


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/



/* Quite a bit of work involved in finding out the inputs for the
 * render stage.  This function also identifies which vertex
 * interpolation function to use, as these are essentially the same
 * question.
 */
static void check_render( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   struct render_stage_data *store = RENDER_STAGE_DATA(stage);
   GLuint interp = 0;
   GLuint copy = 0;
   GLuint inputs = VERT_CLIP;
   GLuint i;

   if (ctx->Visual.RGBAflag)
   {
      interp |= INTERP_RGBA;
      inputs |= VERT_RGBA;

      if (ctx->_TriangleCaps & DD_SEPERATE_SPECULAR) {
         interp |= INTERP_SPEC;
	 inputs |= VERT_SPEC_RGB;
      }

      if (ctx->Texture._ReallyEnabled) {
	 interp |= INTERP_TEX;

	 for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	    if (ctx->Texture.Unit[i]._ReallyEnabled)
	       inputs |= VERT_TEX(i);
	 }
      }
   }
   else 
   {
      interp |= INTERP_INDEX;
      inputs |= VERT_INDEX;
   }

   if (ctx->Point._Attenuated) 
      inputs |= VERT_POINT_SIZE;

   /* How do drivers turn this off?
    */
   if (ctx->Fog.Enabled) {
      interp |= INTERP_FOG;
      inputs |= VERT_FOG_COORD;
   }

   if (ctx->_TriangleCaps & DD_TRI_UNFILLED) {
      inputs |= VERT_EDGE;
      interp |= INTERP_EDGE;
   }

   if (ctx->RenderMode==GL_FEEDBACK) {
      interp |= INTERP_TEX;
      inputs |= VERT_TEX_ANY;
   }

   if (ctx->_TriangleCaps & DD_FLATSHADE) {
      copy = interp & (INTERP_RGBA|INTERP_SPEC|INTERP_INDEX);
      interp &= ~copy;
   }

   store->copypv = copy_tab[copy];
   store->interp = interp_tab[interp];
   stage->inputs = inputs;
}


/* Called the first time stage->check() is invoked.
 */
static void alloc_render_data( GLcontext *ctx, 
				 struct gl_pipeline_stage *stage )
{
   struct render_stage_data *store;
   static GLboolean first_time = 1;

   if (first_time) {
      interp_init();
      first_time = 0;
   }

   stage->private = MALLOC(sizeof(*store));
   if (!stage->private)
      return;

   /* Now do the check.
    */
   stage->check = check_render;
   stage->check( ctx, stage );
}



static void dtr( struct gl_pipeline_stage *stage )
{
   struct render_stage_data *store = RENDER_STAGE_DATA(stage);
   if (store) {
      FREE( store );
      stage->private = 0;
   }
}


const struct gl_pipeline_stage _tnl_render_stage = 
{ 
   "render",
   (_NEW_BUFFERS |
    _DD_NEW_SEPERATE_SPECULAR |
    _DD_NEW_FLATSHADE |
    _NEW_TEXTURE|
    _NEW_LIGHT|
    _NEW_POINT|
    _NEW_FOG|
    _DD_NEW_TRI_UNFILLED |
    _NEW_RENDERMODE),		/* re-check (new inputs, interp function) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, 0,			/* changed_inputs, private */
   dtr,				/* destructor */
   alloc_render_data,		/* check - initially set to alloc data */
   run_render			/* run */
};
