/* Build an SWvertex from an tdfxVertex.  This is workable because in
 * states where the GrVertex is insufficent (eg seperate-specular),
 * the driver initiates a total fallback, and builds SWvertices
 * directly -- it recognizes that it will never have use for the
 * tdfxVertex. 
 *
 * This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives. 
 */
static void 
tdfx_translate_vertex(GLcontext *ctx, const tdfxVertex *src, SWvertex *dst)
{
   tdfxContextPtr imesa = TDFX_CONTEXT( ctx );
   
   if (imesa->vertsize == 4) {
      dst->win[0] =   src->tv.x + .5;
      dst->win[1] = - src->tv.y + imesa->driDrawable->h - .5;
      dst->win[2] =   src->tv.z * (GLfloat)0x10000;
      dst->win[3] =   1.0;
      
      dst->color[0] = src->tv.color.red;
      dst->color[1] = src->tv.color.green;
      dst->color[2] = src->tv.color.blue;
      dst->color[3] = src->tv.color.alpha;
   } 
   else {
      dst->win[0] =   src->v.x + .5;
      dst->win[1] = - src->v.y + imesa->driDrawable->h - .5;
      dst->win[2] =   src->v.z * (GLfloat)0x10000;
      dst->win[3] =   src->v.oow;

      dst->color[0] = src->v.color.red;
      dst->color[1] = src->v.color.green;
      dst->color[2] = src->v.color.blue;
      dst->color[3] = src->v.color.alpha;
      
      if (fxMesa->xxx) {
	 dst->texcoord[0][0] = src->v.tu0;
	 dst->texcoord[0][1] = src->v.tv0;
	 dst->texcoord[0][3] = 1.0;
	 
	 dst->texcoord[1][0] = src->v.tu1;
	 dst->texcoord[1][1] = src->v.tv1;
	 dst->texcoord[1][3] = 1.0;
      } 
      else {
	 dst->texcoord[0][0] = src->pv.u0;
	 dst->texcoord[0][1] = src->pv.v0;
	 dst->texcoord[0][3] = src->pv.q0;
	 
	 dst->texcoord[1][0] = src->pv.u1;
	 dst->texcoord[1][1] = src->pv.v1;
	 dst->texcoord[1][3] = src->pv.q1;
      }
   }

   dst->pointSize = ctx->Point._Size;
}


static void 
tdfx_fallback_tri( tdfxContextPtr imesa, 
		   tdfxVertex *v0, 
		   tdfxVertex *v1, 
		   tdfxVertex *v2 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[3];
   tdfx_translate_vertex( ctx, v0, &v[0] );
   tdfx_translate_vertex( ctx, v1, &v[1] );
   tdfx_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void 
tdfx_fallback_line( tdfxContextPtr imesa,
		    tdfxVertex *v0,
		    tdfxVertex *v1 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[2];
   tdfx_translate_vertex( ctx, v0, &v[0] );
   tdfx_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void 
tdfx_fallback_point( tdfxContextPtr imesa, 
		     tdfxVertex *v0 )
{
   GLcontext *ctx = imesa->glCtx;
   SWvertex v[1];
   tdfx_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}

