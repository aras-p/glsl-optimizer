/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgastate.c,v 1.13 2002/10/30 12:51:36 alanh Exp $ */

#include <stdio.h>

#include "mtypes.h"
#include "dd.h"

#include "mm.h"
#include "mgacontext.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgavb.h"
#include "mgatris.h"
#include "mgaioctl.h"
#include "mgaregs.h"
#include "mgabuffers.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"



/* Some outstanding problems with accelerating logic ops...
 */
#if defined(ACCEL_ROP)
static GLuint mgarop_NoBLK[16] = {
   DC_atype_rpl  | 0x00000000, DC_atype_rstr | 0x00080000,
   DC_atype_rstr | 0x00040000, DC_atype_rpl  | 0x000c0000,
   DC_atype_rstr | 0x00020000, DC_atype_rstr | 0x000a0000,
   DC_atype_rstr | 0x00060000, DC_atype_rstr | 0x000e0000,
   DC_atype_rstr | 0x00010000, DC_atype_rstr | 0x00090000,
   DC_atype_rstr | 0x00050000, DC_atype_rstr | 0x000d0000,
   DC_atype_rpl  | 0x00030000, DC_atype_rstr | 0x000b0000,
   DC_atype_rstr | 0x00070000, DC_atype_rpl  | 0x000f0000
};
#endif


static void mgaUpdateStencil(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint stencil = 0, stencilctl = 0;

   if (ctx->Stencil.Enabled)
   {
      stencil = ctx->Stencil.Ref[0] |
	 ( ctx->Stencil.ValueMask[0] << 8 ) |
	 ( ctx->Stencil.WriteMask[0] << 16 );

      switch (ctx->Stencil.Function[0])
      {
      case GL_NEVER:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_snever);
	 break;
      case GL_LESS:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_slt);
	 break;
      case GL_LEQUAL:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_slte);
	 break;
      case GL_GREATER:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_sgt);
	 break;
      case GL_GEQUAL:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_sgte);
	 break;
      case GL_NOTEQUAL:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_sne);
	 break;
      case GL_EQUAL:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_se);
	 break;
      case GL_ALWAYS:
	 MGA_SET_FIELD(stencilctl, SC_smode_MASK, SC_smode_salways);
      default:
	 break;
      }

      switch (ctx->Stencil.FailFunc[0])
      {
      case GL_KEEP:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_keep);
	 break;
      case GL_ZERO:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_zero);
	 break;
      case GL_REPLACE:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_replace);
	 break;
      case GL_INCR:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_incrsat);
	 break;
      case GL_DECR:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_decrsat);
	 break;
      case GL_INVERT:
	 MGA_SET_FIELD(stencilctl, SC_sfailop_MASK, SC_sfailop_invert);
	 break;
      default:
	 break;
      }

      switch (ctx->Stencil.ZFailFunc[0])
      {
      case GL_KEEP:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_keep);
	 break;
      case GL_ZERO:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_zero);
	 break;
      case GL_REPLACE:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_replace);
	 break;
      case GL_INCR:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_incrsat);
	 break;
      case GL_DECR:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_decrsat);
	 break;
      case GL_INVERT:
	 MGA_SET_FIELD(stencilctl, SC_szfailop_MASK, SC_szfailop_invert);
	 break;
      default:
	 break;
      }

      switch (ctx->Stencil.ZPassFunc[0])
      {
      case GL_KEEP:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_keep);
	 break;
      case GL_ZERO:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_zero);
	 break;
      case GL_REPLACE:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_replace);
	 break;
      case GL_INCR:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_incrsat);
	 break;
      case GL_DECR:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_decrsat);
	 break;
      case GL_INVERT:
	 MGA_SET_FIELD(stencilctl, SC_szpassop_MASK, SC_szpassop_invert);
	 break;
      default:
	 break;
      }
   }

   mmesa->setup.stencil = stencil;
   mmesa->setup.stencilctl = stencilctl;
   mmesa->dirty |= MGA_UPLOAD_CONTEXT;
}

static void mgaDDStencilFunc(GLcontext *ctx, GLenum func, GLint ref,
			     GLuint mask)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_STENCIL;
}

static void mgaDDStencilMask(GLcontext *ctx, GLuint mask)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_STENCIL;
}

static void mgaDDStencilOp(GLcontext *ctx, GLenum fail, GLenum zfail,
			   GLenum zpass)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_STENCIL;
}

static void mgaDDClearDepth(GLcontext *ctx, GLclampd d)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   /* KW: should the ~ be there? */
   switch (mmesa->setup.maccess & ~MA_zwidth_MASK) {
   case MA_zwidth_16: mmesa->ClearDepth = d * 0x0000ffff; break;
   case MA_zwidth_24: mmesa->ClearDepth = d * 0xffffff00; break;
   case MA_zwidth_32: mmesa->ClearDepth = d * 0xffffffff; break;
   default: return;
   }
}

static void mgaUpdateZMode(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int zmode = 0;

   if (ctx->Depth.Test) {
      switch(ctx->Depth.Func)  {
      case GL_NEVER:
         /* can't do this in h/w, we'll use a s/w fallback */
	 zmode = DC_zmode_nozcmp;
         break;
      case GL_ALWAYS:
	 zmode = DC_zmode_nozcmp; break;
      case GL_LESS:
	 zmode = DC_zmode_zlt; break;
      case GL_LEQUAL:
	 zmode = DC_zmode_zlte; break;
      case GL_EQUAL:
	 zmode = DC_zmode_ze; break;
      case GL_GREATER:
	 zmode = DC_zmode_zgt; break;
      case GL_GEQUAL:
	 zmode = DC_zmode_zgte; break;
      case GL_NOTEQUAL:
	 zmode = DC_zmode_zne; break;
      default:
	 break;
      }

      if (ctx->Depth.Mask)
         zmode |= DC_atype_zi;
      else
         zmode |= DC_atype_i;
   }
   else {
      zmode |= DC_zmode_nozcmp;
      zmode |= DC_atype_i;  /* don't write to zbuffer */
   }

#if defined(ACCEL_ROP)
   mmesa->setup.dwgctl &= DC_bop_MASK;
   if (ctx->Color.ColorLogicOpEnabled)
      zmode |= mgarop_NoBLK[(ctx->Color.LogicOp)&0xf];
   else
      zmode |= mgarop_NoBLK[GL_COPY & 0xf];
#endif

   mmesa->setup.dwgctl &= DC_zmode_MASK & DC_atype_MASK;
   mmesa->setup.dwgctl |= zmode;
   mmesa->dirty |= MGA_UPLOAD_CONTEXT;
}


static void mgaDDAlphaFunc(GLcontext *ctx, GLenum func, GLfloat ref)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}


static void mgaDDBlendEquation(GLcontext *ctx, GLenum mode)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;

   /* BlendEquation sets ColorLogicOpEnabled in an unexpected 
    * manner.  
    */
   FALLBACK( ctx, MGA_FALLBACK_LOGICOP,
	     (ctx->Color.ColorLogicOpEnabled && 
	      ctx->Color.LogicOp != GL_COPY));
}

static void mgaDDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}

static void mgaDDBlendFuncSeparate( GLcontext *ctx, GLenum sfactorRGB,
				    GLenum dfactorRGB, GLenum sfactorA,
				    GLenum dfactorA )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}



static void mgaDDLightModelfv(GLcontext *ctx, GLenum pname,
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      FLUSH_BATCH( MGA_CONTEXT(ctx) );
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
   }
}


static void mgaDDShadeModel(GLcontext *ctx, GLenum mode)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
}


static void mgaDDDepthFunc(GLcontext *ctx, GLenum func)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}

static void mgaDDDepthMask(GLcontext *ctx, GLboolean flag)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}

#if defined(ACCEL_ROP)
static void mgaDDLogicOp( GLcontext *ctx, GLenum opcode )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}
#else
static void mgaDDLogicOp( GLcontext *ctx, GLenum opcode )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   FALLBACK( ctx, MGA_FALLBACK_LOGICOP, 
	     (ctx->Color.ColorLogicOpEnabled && opcode != GL_COPY) );
}
#endif



static void mgaDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   if (pname == GL_FOG_COLOR) {
      GLuint color = MGAPACKCOLOR888((GLubyte)(ctx->Fog.Color[0]*255.0F), 
				     (GLubyte)(ctx->Fog.Color[1]*255.0F), 
				     (GLubyte)(ctx->Fog.Color[2]*255.0F));

      MGA_STATECHANGE(mmesa, MGA_UPLOAD_CONTEXT);   
      mmesa->setup.fogcolor = color;
   }
}




/* =============================================================
 * Alpha blending
 */


static void mgaUpdateAlphaMode(GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;
   int a = 0;

   /* determine source of alpha for blending and testing */
   if ( !ctx->Texture.Unit[0]._ReallyEnabled ) {
      a |= AC_alphasel_diffused;
   }
   else {
      /* G400: Regardless of texture env mode, we use the alpha from the
       * texture unit (AC_alphasel_fromtex) since it will have already
       * been modulated by the incoming fragment color, if needed.
       * We don't want (AC_alphasel_modulate) since that'll effectively
       * do the modulation twice.
       */
      if (MGA_IS_G400(mmesa)) {
         a |= AC_alphasel_fromtex;
      }
      else {
         /* G200 */
         switch (ctx->Texture.Unit[0].EnvMode) {
         case GL_DECAL:
            a |= AC_alphasel_diffused;
         case GL_REPLACE:
            a |= AC_alphasel_fromtex;
            break;
         case GL_BLEND:
         case GL_MODULATE:
            a |= AC_alphasel_modulated;
            break;
         default:
            break;
         }
      }
   }


   /* alpha test control.
    */
   if (ctx->Color.AlphaEnabled) {
      GLubyte ref = ctx->Color.AlphaRef;
      switch (ctx->Color.AlphaFunc) {
      case GL_NEVER:
	 a |= AC_atmode_alt;
	 ref = 0;
	 break;
      case GL_LESS:
	 a |= AC_atmode_alt;
	 break;
      case GL_GEQUAL:
	 a |= AC_atmode_agte;
	 break;
      case GL_LEQUAL:
	 a |= AC_atmode_alte;
	 break;
      case GL_GREATER:
	 a |= AC_atmode_agt;
	 break;
      case GL_NOTEQUAL:
	 a |= AC_atmode_ane;
	 break;
      case GL_EQUAL:
	 a |= AC_atmode_ae;
	 break;
      case GL_ALWAYS:
	 a |= AC_atmode_noacmp;
	 break;
      default:
	 break;
      }
      a |= MGA_FIELD(AC_atref,ref);
   }

   /* blending control */
   if (ctx->Color.BlendEnabled) {
      switch (ctx->Color.BlendSrcRGB) {
      case GL_ZERO:
	 a |= AC_src_zero; break;
      case GL_SRC_ALPHA:
	 a |= AC_src_src_alpha; break;
      case GL_ONE:
	 a |= AC_src_one; break;
      case GL_DST_COLOR:
	 a |= AC_src_dst_color; break;
      case GL_ONE_MINUS_DST_COLOR:
	 a |= AC_src_om_dst_color; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_src_om_src_alpha; break;
      case GL_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_src_dst_alpha;
	 else
	    a |= AC_src_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_src_om_dst_alpha;
	 else
	    a |= AC_src_zero;
	 break;
      case GL_SRC_ALPHA_SATURATE:
         if (ctx->Visual.alphaBits > 0)
            a |= AC_src_src_alpha_sat;
         else
            a |= AC_src_zero;
	 break;
      default:		/* never happens */
	 break;
      }

      switch (ctx->Color.BlendDstRGB) {
      case GL_SRC_ALPHA:
	 a |= AC_dst_src_alpha; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_dst_om_src_alpha; break;
      case GL_ZERO:
	 a |= AC_dst_zero; break;
      case GL_ONE:
	 a |= AC_dst_one; break;
      case GL_SRC_COLOR:
	 a |= AC_dst_src_color; break;
      case GL_ONE_MINUS_SRC_COLOR:
	 a |= AC_dst_om_src_color; break;
      case GL_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_dst_dst_alpha;
	 else
	    a |= AC_dst_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (mgaScreen->cpp == 4)
	    a |= AC_dst_om_dst_alpha;
	 else
	    a |= AC_dst_zero;
	 break;
      default:		/* never happens */
	 break;
      }
   } else {
      a |= AC_src_one|AC_dst_zero;
   }

   mmesa->setup.alphactrl = (AC_amode_alpha_channel |
			     AC_astipple_disable |
			     AC_aten_disable |
			     AC_atmode_noacmp |
			     a);

   mmesa->dirty |= MGA_UPLOAD_CONTEXT;
}



/* =============================================================
 * Hardware clipping
 */

void mgaUpdateClipping(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   if (mmesa->driDrawable)
   {
      int x1 = mmesa->driDrawable->x + ctx->Scissor.X;
      int y1 = mmesa->driDrawable->y + mmesa->driDrawable->h
	 - (ctx->Scissor.Y + ctx->Scissor.Height);
      int x2 = x1 + ctx->Scissor.Width - 1;
      int y2 = y1 + ctx->Scissor.Height - 1;

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 < 0) x2 = 0;
      if (y2 < 0) y2 = 0;

      mmesa->scissor_rect.x1 = x1;
      mmesa->scissor_rect.y1 = y1;
      mmesa->scissor_rect.x2 = x2;
      mmesa->scissor_rect.y2 = y2;

      if (MGA_DEBUG&DEBUG_VERBOSE_2D)
	 fprintf(stderr, "SET SCISSOR %d,%d-%d,%d\n",
		 mmesa->scissor_rect.x1,
		 mmesa->scissor_rect.y1,
		 mmesa->scissor_rect.x2,
		 mmesa->scissor_rect.y2);

      mmesa->dirty |= MGA_UPLOAD_CLIPRECTS;
   }
}


static void mgaDDScissor( GLcontext *ctx, GLint x, GLint y,
			  GLsizei w, GLsizei h )
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CLIP;
}


static void mgaDDClearColor(GLcontext *ctx, 
			    const GLfloat color[4] )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->ClearColor = mgaPackColor( mmesa->mgaScreen->cpp,
				     color[0], color[1], 
				     color[2], color[3]);
}


/* =============================================================
 * Culling
 */

#define _CULL_DISABLE 0
#define _CULL_NEGATIVE ((1<<11)|(1<<5)|(1<<16))
#define _CULL_POSITIVE (1<<11)


void mgaUpdateCull( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint mode = _CULL_DISABLE;

   if (ctx->Polygon.CullFlag && 
       mmesa->raster_primitive == GL_TRIANGLES &&       
       ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) 
   {
      mode = _CULL_NEGATIVE;
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if ((ctx->Texture.Unit[0]._ReallyEnabled & TEXTURE_2D_BIT) &&
          (ctx->Texture.Unit[1]._ReallyEnabled & TEXTURE_2D_BIT))
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE); /* warp bug? */
   }

   mmesa->setup.wflag = mode;
   mmesa->dirty |= MGA_UPLOAD_CONTEXT;
}


static void mgaDDCullFaceFrontFace(GLcontext *ctx, GLenum mode)
{
   FLUSH_BATCH( MGA_CONTEXT(ctx) );
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CULL;
}




/* =============================================================
 * Color masks
 */

static void mgaDDColorMask(GLcontext *ctx, 
			   GLboolean r, GLboolean g, 
			   GLboolean b, GLboolean a )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;


   GLuint mask = mgaPackColor(mgaScreen->cpp,
			      ctx->Color.ColorMask[RCOMP],
			      ctx->Color.ColorMask[GCOMP],
			      ctx->Color.ColorMask[BCOMP],
			      ctx->Color.ColorMask[ACOMP]);

   if (mgaScreen->cpp == 2)
      mask = mask | (mask << 16);

   if (mmesa->setup.plnwt != mask) {
      MGA_STATECHANGE( mmesa, MGA_UPLOAD_CONTEXT );
      mmesa->setup.plnwt = mask;      
   }
}

/* =============================================================
 * Polygon stipple
 *
 * The mga supports a subset of possible 4x4 stipples natively, GL
 * wants 32x32.  Fortunately stipple is usually a repeating pattern.
 *
 * Note: the fully opaque pattern (0xffff) has been disabled in order
 * to work around a conformance issue.
 */
static int mgaStipples[16] = {
   0xffff1,  /* See above note */
   0xa5a5,
   0x5a5a,
   0xa0a0,
   0x5050,
   0x0a0a,
   0x0505,
   0x8020,
   0x0401,
   0x1040,
   0x0208,
   0x0802,
   0x4010,
   0x0104,
   0x2080,
   0x0000
};

static void mgaDDPolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const GLubyte *m = mask;
   GLubyte p[4];
   int i,j,k;
   int active = (ctx->Polygon.StippleFlag && 
		 mmesa->raster_primitive == GL_TRIANGLES);
   GLuint stipple;

   FLUSH_BATCH(mmesa);
   mmesa->haveHwStipple = 0;

   if (active) {
      mmesa->dirty |= MGA_UPLOAD_CONTEXT;
      mmesa->setup.dwgctl &= ~(0xf<<20);
   }

   p[0] = mask[0] & 0xf; p[0] |= p[0] << 4;
   p[1] = mask[4] & 0xf; p[1] |= p[1] << 4;
   p[2] = mask[8] & 0xf; p[2] |= p[2] << 4;
   p[3] = mask[12] & 0xf; p[3] |= p[3] << 4;

   for (k = 0 ; k < 8 ; k++)
      for (j = 0 ; j < 4; j++)
	 for (i = 0 ; i < 4 ; i++)
	    if (*m++ != p[j]) {
	       return;
	    }

   stipple = ( ((p[0] & 0xf) << 0) |
	       ((p[1] & 0xf) << 4) |
	       ((p[2] & 0xf) << 8) |
	       ((p[3] & 0xf) << 12) );

   for (i = 0 ; i < 16 ; i++)
      if (mgaStipples[i] == stipple) {
	 mmesa->poly_stipple = i<<20;
	 mmesa->haveHwStipple = 1;
	 break;
      }
   
   if (active) {
      mmesa->setup.dwgctl &= ~(0xf<<20);
      mmesa->setup.dwgctl |= mmesa->poly_stipple;
   }
}

/* =============================================================
 */

static void mgaDDPrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s%s\n",
	   msg,
	   (unsigned int) state,
	   (state & MGA_WAIT_AGE) ? "wait-age, " : "",
	   (state & MGA_UPLOAD_TEX0IMAGE)  ? "upload-tex0-img, " : "",
	   (state & MGA_UPLOAD_TEX1IMAGE)  ? "upload-tex1-img, " : "",
	   (state & MGA_UPLOAD_CONTEXT)        ? "upload-ctx, " : "",
	   (state & MGA_UPLOAD_TEX0)       ? "upload-tex0, " : "",
	   (state & MGA_UPLOAD_TEX1)       ? "upload-tex1, " : "",
	   (state & MGA_UPLOAD_PIPE)       ? "upload-pipe, " : ""
      );
}

/* Push the state into the sarea and/or texture memory.
 */
void mgaEmitHwStateLocked( mgaContextPtr mmesa )
{
   MGASAREAPrivPtr sarea = mmesa->sarea;

   if (MGA_DEBUG & DEBUG_VERBOSE_MSG)
      mgaDDPrintDirty( "mgaEmitHwStateLocked", mmesa->dirty );

   if ((mmesa->dirty & MGA_UPLOAD_TEX0IMAGE) && mmesa->CurrentTexObj[0])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[0]);

   if ((mmesa->dirty & MGA_UPLOAD_TEX1IMAGE) && mmesa->CurrentTexObj[1])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[1]);

   if (mmesa->dirty & MGA_UPLOAD_CONTEXT) {
      memcpy( &sarea->ContextState, &mmesa->setup, sizeof(mmesa->setup));
   }

   if ((mmesa->dirty & MGA_UPLOAD_TEX0) && mmesa->CurrentTexObj[0]) {
      memcpy(&sarea->TexState[0],
	     &mmesa->CurrentTexObj[0]->setup,
	     sizeof(sarea->TexState[0]));
   }

   if ((mmesa->dirty & MGA_UPLOAD_TEX1) && mmesa->CurrentTexObj[1]) {
      memcpy(&sarea->TexState[1],
	     &mmesa->CurrentTexObj[1]->setup,
	     sizeof(sarea->TexState[1]));
   }

   if (sarea->TexState[0].texctl2 !=
       sarea->TexState[1].texctl2) {
      memcpy(&sarea->TexState[1],
	     &sarea->TexState[0],
	     sizeof(sarea->TexState[0]));
      mmesa->dirty |= MGA_UPLOAD_TEX1|MGA_UPLOAD_TEX0;
   }

   if (mmesa->dirty & MGA_UPLOAD_PIPE) {
/*        mmesa->sarea->wacceptseq = mmesa->hw_primitive; */
      mmesa->sarea->WarpPipe = mmesa->vertex_format;
      mmesa->sarea->vertsize = mmesa->vertex_size;
   }

   mmesa->sarea->dirty |= mmesa->dirty;

   mmesa->dirty &= (MGA_UPLOAD_CLIPRECTS|MGA_WAIT_AGE);

   /* This is a bit of a hack but seems to be the best place to ensure
    * that separate specular is disabled when not needed.
    */
   if (mmesa->glCtx->Texture.Unit[0]._ReallyEnabled == 0 ||
       !mmesa->glCtx->Light.Enabled ||
       mmesa->glCtx->Light.Model.ColorControl == GL_SINGLE_COLOR) {
      sarea->TexState[0].texctl2 &= ~TMC_specen_enable;
      sarea->TexState[1].texctl2 &= ~TMC_specen_enable;
   }
}

/* Fallback to swrast for select and feedback.
 */
static void mgaRenderMode( GLcontext *ctx, GLenum mode )
{
   FALLBACK( ctx, MGA_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}


/* =============================================================
 */

void mgaCalcViewport( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = mmesa->hw_viewport;

   /* See also mga_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + mmesa->drawX + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + mmesa->driDrawable->h + mmesa->drawY + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * mmesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * mmesa->depth_scale;

   mmesa->SetupNewInputs = ~0;
}

static void mgaViewport( GLcontext *ctx, 
			  GLint x, GLint y, 
			  GLsizei width, GLsizei height )
{
   mgaCalcViewport( ctx );
}

static void mgaDepthRange( GLcontext *ctx, 
			    GLclampd nearval, GLclampd farval )
{
   mgaCalcViewport( ctx );
}

/* =============================================================
 */

static void mgaDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );

   switch(cap) {
   case GL_ALPHA_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_ALPHA;
      break;
   case GL_BLEND:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_ALPHA;

      /* For some reason enable(GL_BLEND) affects ColorLogicOpEnabled.
       */
      FALLBACK( ctx, MGA_FALLBACK_LOGICOP,
		(ctx->Color.ColorLogicOpEnabled && 
		 ctx->Color.LogicOp != GL_COPY));
      break;
   case GL_DEPTH_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_DEPTH;
      FALLBACK (ctx, MGA_FALLBACK_DEPTH,
		ctx->Depth.Func == GL_NEVER && ctx->Depth.Test);
      break;
   case GL_SCISSOR_TEST:
      FLUSH_BATCH( mmesa );
      mmesa->scissor = state;
      mmesa->new_state |= MGA_NEW_CLIP;
      break;
   case GL_FOG:
      MGA_STATECHANGE( mmesa, MGA_UPLOAD_CONTEXT );
      if (ctx->Fog.Enabled) 
	 mmesa->setup.maccess |= MA_fogen_enable;
      else
	 mmesa->setup.maccess &= ~MA_fogen_enable;
      break;
   case GL_CULL_FACE:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= MGA_NEW_CULL;
      break;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      FLUSH_BATCH( mmesa );
      mmesa->new_state |= (MGA_NEW_TEXTURE|MGA_NEW_ALPHA);
      break;
   case GL_POLYGON_STIPPLE:
      if (mmesa->haveHwStipple && mmesa->raster_primitive == GL_TRIANGLES) {
	 FLUSH_BATCH(mmesa);
	 mmesa->dirty |= MGA_UPLOAD_CONTEXT;
	 mmesa->setup.dwgctl &= ~(0xf<<20);
	 if (state)
	    mmesa->setup.dwgctl |= mmesa->poly_stipple;
      }
      break;
   case GL_COLOR_LOGIC_OP:
      FLUSH_BATCH( mmesa );
#if !defined(ACCEL_ROP)
      FALLBACK( ctx, MGA_FALLBACK_LOGICOP, 
		(state && ctx->Color.LogicOp != GL_COPY));
#else
      mmesa->new_state |= MGA_NEW_DEPTH;
#endif
      break;
   case GL_STENCIL_TEST:
      FLUSH_BATCH( mmesa );
      if (mmesa->hw_stencil)
	 mmesa->new_state |= MGA_NEW_STENCIL;
      else
	 FALLBACK( ctx, MGA_FALLBACK_STENCIL, state );
   default:
      break;
   }
}


/* =============================================================
 */



/* =============================================================
 */

static void mgaDDPrintState( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & MGA_NEW_DEPTH)   ? "depth, " : "",
	   (state & MGA_NEW_ALPHA)   ? "alpha, " : "",
	   (state & MGA_NEW_CLIP)    ? "clip, " : "",
	   (state & MGA_NEW_CULL)    ? "cull, " : "",
	   (state & MGA_NEW_TEXTURE) ? "texture, " : "",
	   (state & MGA_NEW_CONTEXT) ? "context, " : "");
}

void mgaDDUpdateHwState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int new_state = mmesa->new_state;

   if (new_state)
   {
      FLUSH_BATCH( mmesa );

      mmesa->new_state = 0;

      if (MESA_VERBOSE&VERBOSE_DRIVER)
	 mgaDDPrintState("UpdateHwState", new_state);

      if (new_state & MGA_NEW_DEPTH)
	 mgaUpdateZMode(ctx);

      if (new_state & MGA_NEW_ALPHA)
	 mgaUpdateAlphaMode(ctx);

      if (new_state & MGA_NEW_CLIP)
	 mgaUpdateClipping(ctx);

      if (new_state & MGA_NEW_STENCIL)
	 mgaUpdateStencil(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_CULL))
	 mgaUpdateCull(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_TEXTURE))
	 mgaUpdateTextureState(ctx);
   }
}


static void mgaDDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   MGA_CONTEXT(ctx)->new_gl_state |= new_state;
}



void mgaInitState( mgaContextPtr mmesa )
{
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;
   GLcontext *ctx = mmesa->glCtx;

   if (ctx->Color._DrawDestMask == BACK_LEFT_BIT) {
      mmesa->draw_buffer = MGA_BACK;
      mmesa->read_buffer = MGA_BACK;
      mmesa->drawOffset = mmesa->mgaScreen->backOffset;
      mmesa->readOffset = mmesa->mgaScreen->backOffset;
      mmesa->setup.dstorg = mgaScreen->backOffset;
   } else {
      mmesa->draw_buffer = MGA_FRONT;
      mmesa->read_buffer = MGA_FRONT;
      mmesa->drawOffset = mmesa->mgaScreen->frontOffset;
      mmesa->readOffset = mmesa->mgaScreen->frontOffset;
      mmesa->setup.dstorg = mgaScreen->frontOffset;
   }

   mmesa->setup.maccess = (MA_memreset_disable |
			   MA_fogen_disable |
			   MA_tlutload_disable |
			   MA_nodither_disable |
			   MA_dit555_disable);

   switch (mmesa->mgaScreen->cpp) {
   case 2:
      mmesa->setup.maccess |= MA_pwidth_16;
      break;
   case 4:
      mmesa->setup.maccess |= MA_pwidth_32;
      break;
   default:
      fprintf( stderr, "Error: unknown cpp %d, exiting...\n",
	       mmesa->mgaScreen->cpp );
      exit( 1 );
   }

   switch (mmesa->glCtx->Visual.depthBits) {
   case 16:
      mmesa->setup.maccess |= MA_zwidth_16;
      break;
   case 24:
      mmesa->setup.maccess |= MA_zwidth_24;
      break;
   case 32:
      mmesa->setup.maccess |= MA_pwidth_32;
      break;
   }

   mmesa->setup.dwgctl = (DC_opcod_trap |
			  DC_atype_i |
			  DC_linear_xy |
			  DC_zmode_nozcmp |
			  DC_solid_disable |
			  DC_arzero_disable |
			  DC_sgnzero_disable |
			  DC_shftzero_enable |
			  (0xC << DC_bop_SHIFT) |
			  (0x0 << DC_trans_SHIFT) |
			  DC_bltmod_bmonolef |
			  DC_pattern_disable |
			  DC_transc_disable |
			  DC_clipdis_disable);


   mmesa->setup.plnwt = ~0;
   mmesa->setup.alphactrl = ( AC_src_one |
			      AC_dst_zero |
			      AC_amode_FCOL |
			      AC_astipple_disable |
			      AC_aten_disable |
			      AC_atmode_noacmp |
			      AC_alphasel_fromtex );

   mmesa->setup.fogcolor =
      MGAPACKCOLOR888((GLubyte)(ctx->Fog.Color[0]*255.0F),
		      (GLubyte)(ctx->Fog.Color[1]*255.0F),
		      (GLubyte)(ctx->Fog.Color[2]*255.0F));

   mmesa->setup.wflag = 0;
   mmesa->setup.tdualstage0 = 0;
   mmesa->setup.tdualstage1 = 0;
   mmesa->setup.fcol = 0;
   mmesa->new_state = ~0;
}


void mgaDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState = mgaDDInvalidateState;
   ctx->Driver.Enable = mgaDDEnable;
   ctx->Driver.LightModelfv = mgaDDLightModelfv;
   ctx->Driver.AlphaFunc = mgaDDAlphaFunc;
   ctx->Driver.BlendEquation = mgaDDBlendEquation;
   ctx->Driver.BlendFunc = mgaDDBlendFunc;
   ctx->Driver.BlendFuncSeparate = mgaDDBlendFuncSeparate;
   ctx->Driver.DepthFunc = mgaDDDepthFunc;
   ctx->Driver.DepthMask = mgaDDDepthMask;
   ctx->Driver.Fogfv = mgaDDFogfv;
   ctx->Driver.Scissor = mgaDDScissor;
   ctx->Driver.ShadeModel = mgaDDShadeModel;
   ctx->Driver.CullFace = mgaDDCullFaceFrontFace;
   ctx->Driver.FrontFace = mgaDDCullFaceFrontFace;
   ctx->Driver.ColorMask = mgaDDColorMask;

   ctx->Driver.DrawBuffer = mgaDDSetDrawBuffer;
   ctx->Driver.ReadBuffer = mgaDDSetReadBuffer;
   ctx->Driver.ClearColor = mgaDDClearColor;
   ctx->Driver.ClearDepth = mgaDDClearDepth;
   ctx->Driver.LogicOpcode = mgaDDLogicOp;

   ctx->Driver.PolygonStipple = mgaDDPolygonStipple;

   ctx->Driver.StencilFunc = mgaDDStencilFunc;
   ctx->Driver.StencilMask = mgaDDStencilMask;
   ctx->Driver.StencilOp = mgaDDStencilOp;

   ctx->Driver.DepthRange = mgaDepthRange;
   ctx->Driver.Viewport = mgaViewport;
   ctx->Driver.RenderMode = mgaRenderMode;

   ctx->Driver.ClearIndex = 0;
   ctx->Driver.IndexMask = 0;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
