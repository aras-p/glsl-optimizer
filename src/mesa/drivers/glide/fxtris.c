

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "mmath.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "fxdrv.h"
#include "fxglidew.h"








static void 
fx_draw_point( GLcontext *ctx, const fxVertex *v )
{
   GLfloat sz = ctx->Point._Size;

   if ( sz <= 1.0 )
   {
      grDrawPoint( &(v->v) );
   }
   else
   {
      GrVertex verts[4];

      sz *= .5;

      verts[0] = v->v;
      verts[1] = v->v;
      verts[2] = v->v;
      verts[3] = v->v;

      verts[0].x = v->v.x - sz;
      verts[0].y = v->v.y - sz;

      verts[1].x = v->v.x + sz;
      verts[1].y = v->v.y - sz;
	 
      verts[2].x = v->v.x + sz;
      verts[2].y = v->v.y + sz;

      verts[3].x = v->v.x - sz;
      verts[3].y = v->v.y + sz;

      grDrawTriangle( &verts[0], &verts[1], &verts[3] );
      grDrawTriangle( &verts[1], &verts[2], &verts[3] );
   }
}


static void 
fx_draw_line( GLcontext *ctx, const fxVertex *v0, const fxVertex *v1 )
{
   float width = ctx->Line.Width;

   if ( width <= 1.0 )
   {
      grDrawLine( &(v0->v), &(v1->v) );
   }
   else
   {
      GrVertex verts[4];
      float dx, dy, ix, iy;

      dx = v0->v.x - v1->v.x;
      dy = v0->v.y - v1->v.y;

      if (dx * dx > dy * dy) {
	 iy = width * .5;
	 ix = 0;
      } else {
	 iy = 0;
	 ix = width * .5;
      }


      verts[0] = v0->v;
      verts[0].x -= ix;
      verts[0].y -= iy;

      verts[1] = v0->v;
      verts[1].x += ix;
      verts[1].y += iy;

      verts[2] = v1->v;
      verts[2].x += ix;
      verts[2].y += iy;

      verts[3] = v1->v;
      verts[3].x -= ix;
      verts[3].y -= iy;

      grDrawTriangle( &verts[0], &verts[1], &verts[3] );
      grDrawTriangle( &verts[1], &verts[2], &verts[3] );
   }
}

static void 
fx_draw_tri( GLcontext *ctx, const fxVertex *v0, const fxVertex *v1, 
	     const fxVertex *v2 )
{
   grDrawTriangle( &(v0->v), &(v1->v), &(v2->v) );
}



#define FX_COLOR(vert, c) {				\
  GLubyte *col = c;					\
  vert->v.r=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[0]);	\
  vert->v.g=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[1]);	\
  vert->v.b=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[2]);	\
  vert->v.a=UBYTE_COLOR_TO_FLOAT_255_COLOR(col[3]);	\
}

#define FX_COPY_COLOR( dst, src ) {		\
   dst->v.r = src->v.r;				\
   dst->v.g = src->v.g;				\
   dst->v.b = src->v.b;				\
   dst->v.a = src->v.a;				\
}



#define FX_FLAT_BIT		0x01
#define FX_OFFSET_BIT		0x02
#define FX_TWOSIDE_BIT	        0x04
#define FX_UNFILLED_BIT	        0x10
#define FX_FALLBACK_BIT	        0x20
#define FX_MAX_TRIFUNC          0x40

static struct {
   points_func		points;
   line_func    	line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[FX_MAX_TRIFUNC];


#define IND (0)
#define TAG(x) x
#include "fxtritmp.h"

#define IND (FX_FLAT_BIT)
#define TAG(x) x##_flat
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT)
#define TAG(x) x##_offset
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "fxtritmp.h"

#define IND (FX_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "fxtritmp.h"

#define IND (FX_FLAT_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_flat_fallback
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_offset_flat_fallback
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_flat_fallback
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_flat_fallback
#include "fxtritmp.h"

#define IND (FX_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "fxtritmp.h"

#define IND (FX_FLAT_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_flat_unfilled
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FLAT_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_offset_flat_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FLAT_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_flat_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FLAT_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_flat_unfilled
#include "fxtritmp.h"

#define IND (FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_FLAT_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_flat_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_offset_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_OFFSET_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_offset_flat_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_flat_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_fallback_unfilled
#include "fxtritmp.h"

#define IND (FX_TWOSIDE_BIT | FX_OFFSET_BIT | FX_FLAT_BIT | FX_FALLBACK_BIT | FX_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_flat_fallback_unfilled
#include "fxtritmp.h"






void fxDDTrifuncInit( void )
{
   init();
   init_flat();
   init_offset();
   init_offset_flat();
   init_twoside();
   init_twoside_flat();
   init_twoside_offset();
   init_twoside_offset_flat();
   init_fallback();
   init_flat_fallback();
   init_offset_fallback();
   init_offset_flat_fallback();
   init_twoside_fallback();
   init_twoside_flat_fallback();
   init_twoside_offset_fallback();
   init_twoside_offset_flat_fallback();

   init_unfilled();
   init_flat_unfilled();
   init_offset_unfilled();
   init_offset_flat_unfilled();
   init_twoside_unfilled();
   init_twoside_flat_unfilled();
   init_twoside_offset_unfilled();
   init_twoside_offset_flat_unfilled();
   init_fallback_unfilled();
   init_flat_fallback_unfilled();
   init_offset_fallback_unfilled();
   init_offset_flat_fallback_unfilled();
   init_twoside_fallback_unfilled();
   init_twoside_flat_fallback_unfilled();
   init_twoside_offset_fallback_unfilled();
   init_twoside_offset_flat_fallback_unfilled();
}


/* Build an SWvertex from a GrVertex.  This is workable because in
 * states where the GrVertex is insufficent (eg seperate-specular),
 * the driver initiates a total fallback, and builds SWvertices
 * directly -- it recognizes that it will never have use for the
 * GrVertex. 
 *
 * This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives. 
 */
static void 
fx_translate_vertex(GLcontext *ctx, const fxVertex *src, SWvertex *dst)
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GLuint ts0 = fxMesa->tmu_source[0];
   GLuint ts1 = fxMesa->tmu_source[1];
   GLfloat w = 1.0 / src->v.oow;	

   dst->win[0] = src->v.x;
   dst->win[1] = src->v.y;
   dst->win[2] = src->v.ooz;
   dst->win[3] = src->v.oow;

   dst->color[0] = (GLubyte) src->v.r;
   dst->color[1] = (GLubyte) src->v.g;
   dst->color[2] = (GLubyte) src->v.b;
   dst->color[3] = (GLubyte) src->v.a;

   dst->texcoord[ts0][0] = fxMesa->inv_s0scale * src->v.tmuvtx[0].sow * w;
   dst->texcoord[ts0][1] = fxMesa->inv_t0scale * src->v.tmuvtx[0].tow * w;

   if (fxMesa->stw_hint_state & GR_STWHINT_W_DIFF_TMU0)
      dst->texcoord[ts0][3] = src->v.tmuvtx[0].oow * w;
   else
      dst->texcoord[ts0][3] = 1.0;

   dst->texcoord[ts1][0] = fxMesa->inv_s1scale * src->v.tmuvtx[1].sow * w;
   dst->texcoord[ts1][1] = fxMesa->inv_t1scale * src->v.tmuvtx[1].tow * w;

   if (fxMesa->stw_hint_state & GR_STWHINT_W_DIFF_TMU1) 
      dst->texcoord[ts1][3] = src->v.tmuvtx[1].oow * w;
   else
      dst->texcoord[ts1][3] = 1.0;
}


static void 
fx_fallback_tri( GLcontext *ctx, 
		 const fxVertex *v0, const fxVertex *v1, const fxVertex *v2 )
{
   SWvertex v[3];
   fx_translate_vertex( ctx, v0, &v[0] );
   fx_translate_vertex( ctx, v1, &v[1] );
   fx_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void 
fx_fallback_line( GLcontext *ctx, const fxVertex *v0, const fxVertex *v1 )
{
   SWvertex v[2];
   fx_translate_vertex( ctx, v0, &v[0] );
   fx_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void 
fx_fallback_point( GLcontext *ctx, const fxVertex *v0 )
{
   SWvertex v[1];
   fx_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}


/* System to turn culling off for rasterized lines and points, and
 * back on for rasterized triangles.
 */
static void 
fx_cull_draw_tri( GLcontext *ctx, 
		 const fxVertex *v0, const fxVertex *v1, const fxVertex *v2 )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   FX_grCullMode(fxMesa->cullMode);

   fxMesa->draw_line = fxMesa->initial_line;
   fxMesa->draw_point = fxMesa->initial_point;
   fxMesa->draw_tri = fxMesa->subsequent_tri;

   fxMesa->draw_tri( ctx, v0, v1, v2 );
}


static void 
fx_cull_draw_line( GLcontext *ctx, const fxVertex *v0, const fxVertex *v1 )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   FX_grCullMode( GR_CULL_DISABLE );

   fxMesa->draw_point = fxMesa->initial_point;
   fxMesa->draw_tri = fxMesa->initial_tri;
   fxMesa->draw_line = fxMesa->subsequent_line;

   fxMesa->draw_line( ctx, v0, v1 );
}


static void 
fx_cull_draw_point( GLcontext *ctx, const fxVertex *v0 )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );

   FX_grCullMode(GR_CULL_DISABLE);

   fxMesa->draw_line = fxMesa->initial_line;
   fxMesa->draw_tri = fxMesa->initial_tri;
   fxMesa->draw_point = fxMesa->subsequent_point;

   fxMesa->draw_point( ctx, v0 );
}


static void 
fx_null_tri( GLcontext *ctx, 
	     const fxVertex *v0, const fxVertex *v1, const fxVertex *v2 )
{
   (void) v0;
   (void) v1;
   (void) v2;
}




/**********************************************************************/
/*                 Render whole begin/end objects                     */
/**********************************************************************/


/* Vertices, no clipping.
 */
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      grDrawPoint( &v[ELT(start)].v );

#define RENDER_LINE( i1, i )			\
   grDrawLine( &v[i1].v, &v[i].v )

#define RENDER_TRI( i2, i1, i )				\
   grDrawTriangle( &v[i2].v, &v[i1].v, &v[i].v )

#define RENDER_QUAD( i3, i2, i1, i )			\
   grDrawTriangle( &v[i3].v, &v[i2].v, &v[i].v );	\
   grDrawTriangle( &v[i2].v, &v[i1].v, &v[i].v )

#define TAG(x) fx_##x##_verts
#define LOCAL_VARS \
    fxVertex *v = FX_CONTEXT(ctx)->verts;	

/* Verts, no clipping.
 */
#define ELT(x) x
#define RESET_STIPPLE 
#define RESET_OCCLUSION 
#define PRESERVE_VB_DEFS
#include "tnl/t_vb_rendertmp.h"


/* Elts, no clipping.
 */
#undef ELT
#undef TAG
#undef LOCAL_VARS
#define TAG(x) fx_##x##_elts
#define ELT(x) elt[x]
#define LOCAL_VARS  				\
    fxVertex *v = FX_CONTEXT(ctx)->verts;   \
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/




#define POINT_FALLBACK	(DD_POINT_SMOOTH )
#define LINE_FALLBACK	(DD_LINE_STIPPLE)
#define TRI_FALLBACK	(DD_TRI_SMOOTH | DD_TRI_STIPPLE )
#define ANY_FALLBACK	(POINT_FALLBACK | LINE_FALLBACK | TRI_FALLBACK)
			 

#define ANY_RENDER_FLAGS (DD_FLATSHADE | 		\
			  DD_TRI_LIGHT_TWOSIDE | 	\
			  DD_TRI_OFFSET |		\
			  DD_TRI_UNFILLED)



/* Setup the Point, Line, Triangle and Quad functions based on the
 * current rendering state.  Wherever possible, use the hardware to
 * render the primitive.  Otherwise, fallback to software rendering.
 */
void fxDDChooseRenderState( GLcontext *ctx )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if ( !fxMesa->is_in_hardware ) {
      /* Build software vertices directly.  No acceleration is
       * possible.  GrVertices may be insufficient for this mode.
       */
      ctx->Driver.PointsFunc = _swsetup_Points;
      ctx->Driver.LineFunc = _swsetup_Line;
      ctx->Driver.TriangleFunc = _swsetup_Triangle;
      ctx->Driver.QuadFunc = _swsetup_Quad;
      ctx->Driver.RenderTabVerts = _tnl_render_tab_verts;
      ctx->Driver.RenderTabElts = _tnl_render_tab_elts;

      fxMesa->render_index = FX_FALLBACK_BIT;
      return;
   }

   if ( flags & ANY_RENDER_FLAGS ) {
      if ( flags & DD_FLATSHADE )		index |= FX_FLAT_BIT;
      if ( flags & DD_TRI_LIGHT_TWOSIDE )	index |= FX_TWOSIDE_BIT;
      if ( flags & DD_TRI_OFFSET )		index |= FX_OFFSET_BIT;
      if ( flags & DD_TRI_UNFILLED )		index |= FX_UNFILLED_BIT;
   }

   if ( flags & (ANY_FALLBACK|
		 DD_LINE_WIDTH|
		 DD_POINT_SIZE|
		 DD_TRI_CULL_FRONT_BACK) ) {
      
      /* Hook in fallbacks for specific primitives.
       *
       * Set up a system to turn culling on/off for wide points and
       * lines.  Alternately: figure out what tris to send so that
       * culling isn't a problem.  
       *
       * This replaces the ReducedPrimitiveChange mechanism.
       */
      index |= FX_FALLBACK_BIT;
      fxMesa->initial_point = fx_cull_draw_point;
      fxMesa->initial_line = fx_cull_draw_line;
      fxMesa->initial_tri = fx_cull_draw_tri;

      fxMesa->subsequent_point = fx_draw_point;
      fxMesa->subsequent_line = fx_draw_line;
      fxMesa->subsequent_tri = fx_draw_tri;

      if ( flags & POINT_FALLBACK ) 
	 fxMesa->initial_point = fx_fallback_point;

      if ( flags & LINE_FALLBACK ) 
	 fxMesa->initial_line = fx_fallback_line;

      if ((flags & DD_LINE_SMOOTH) && ctx->Line.Width != 1.0) 
	 fxMesa->initial_line = fx_fallback_line;

      if ( flags & TRI_FALLBACK ) 
	 fxMesa->initial_tri = fx_fallback_tri;

      if ( flags & DD_TRI_CULL_FRONT_BACK ) 
	 fxMesa->initial_tri = fx_null_tri;

      fxMesa->draw_point = fxMesa->initial_point;
      fxMesa->draw_line = fxMesa->initial_line;
      fxMesa->draw_tri = fxMesa->initial_tri;
   }
   else if (fxMesa->render_index & FX_FALLBACK_BIT) {
      FX_grCullMode(fxMesa->cullMode);
   }
   
   ctx->Driver.PointsFunc = rast_tab[index].points;
   ctx->Driver.LineFunc = rast_tab[index].line;
   ctx->Driver.TriangleFunc = rast_tab[index].triangle;
   ctx->Driver.QuadFunc = rast_tab[index].quad;
   fxMesa->render_index = index;

   if (fxMesa->render_index == 0) {
      ctx->Driver.RenderTabVerts = fx_render_tab_verts;
      ctx->Driver.RenderTabElts = fx_render_tab_elts;
   } else {
      ctx->Driver.RenderTabVerts = _tnl_render_tab_verts;
      ctx->Driver.RenderTabElts = _tnl_render_tab_elts;
   }
}




#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_trifuncs(void);
int gl_fx_dummy_function_trifuncs(void)
{
  return 0;
}

#endif  /* FX */
