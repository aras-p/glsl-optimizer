/* -*- mode: C; tab-width:8;  -*-

             fxdd.c - 3Dfx VooDoo Mesa device driver functions
*/

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * See the file fxapi.c for more informations about authors
 *
 */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "types.h"
#include "fxdrv.h"
#include "enums.h"
#include "extensions.h"

/**********************************************************************/
/*****                 Miscellaneous functions                    *****/
/**********************************************************************/

/* Enalbe/Disable dithering */
void fxDDDither(GLcontext *ctx, GLboolean enable)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDDither()\n");
  }

  if(enable)
    grDitherMode(GR_DITHER_4x4);
  else
    grDitherMode(GR_DITHER_DISABLE);
}


/* Return buffer size information */
void fxDDBufferSize(GLcontext *ctx, GLuint *width, GLuint *height)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDBufferSize(...) Start\n");
  }

  *width=fxMesa->width;
  *height=fxMesa->height;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDBufferSize(...) End\n");
  }
}


/* Set current drawing color */
static void fxDDSetColor(GLcontext *ctx, GLubyte red, GLubyte green,
                         GLubyte blue, GLubyte alpha )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLubyte col[4];
  ASSIGN_4V( col, red, green, blue, alpha );
  
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDSetColor(%d,%d,%d,%d)\n",red,green,blue,alpha);
  }

  fxMesa->color=FXCOLOR4(col);
}


/* Implements glClearColor() */
static void fxDDClearColor(GLcontext *ctx, GLubyte red, GLubyte green,
                           GLubyte blue, GLubyte alpha )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLubyte col[4];



  ASSIGN_4V( col, red, green, blue, 255 );

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDClearColor(%d,%d,%d,%d)\n",red,green,blue,alpha);
  }
 
  fxMesa->clearC=FXCOLOR4( col );
  fxMesa->clearA=alpha;
}

/* Clear the color and/or depth buffers */
static GLbitfield fxDDClear(GLcontext *ctx, GLbitfield mask, GLboolean all,
                            GLint x, GLint y, GLint width, GLint height )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLbitfield newmask;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDClear(%d,%d,%d,%d)\n",x,y,width,height);
  }

  switch(mask & (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)) {
  case (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT):
    /* clear color and depth buffer */

    if (ctx->Color.DrawDestMask & BACK_LEFT_BIT) {
      grRenderBuffer(GR_BUFFER_BACKBUFFER);
      grBufferClear(fxMesa->clearC, fxMesa->clearA,
                    (FxU16)(ctx->Depth.Clear*0xffff));
    }
    if (ctx->Color.DrawDestMask & FRONT_LEFT_BIT) {
       grRenderBuffer(GR_BUFFER_FRONTBUFFER);
       grBufferClear(fxMesa->clearC, fxMesa->clearA,
                     (FxU16)(ctx->Depth.Clear*0xffff));
    }

    newmask=mask & (~(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
    break;
  case (GL_COLOR_BUFFER_BIT):
    /* clear color buffer */

    if(ctx->Color.ColorMask) {
      grDepthMask(FXFALSE);

      if (ctx->Color.DrawDestMask & BACK_LEFT_BIT) {
        grRenderBuffer(GR_BUFFER_BACKBUFFER);
        grBufferClear(fxMesa->clearC, fxMesa->clearA, 0);
      }
      if (ctx->Color.DrawDestMask & FRONT_LEFT_BIT) {
        grRenderBuffer(GR_BUFFER_FRONTBUFFER);
        grBufferClear(fxMesa->clearC, fxMesa->clearA, 0);
      }

      if(ctx->Depth.Mask)
        grDepthMask(FXTRUE);
    }

    newmask=mask & (~(GL_COLOR_BUFFER_BIT));
    break;
  case (GL_DEPTH_BUFFER_BIT):
    /* clear depth buffer */

    if(ctx->Depth.Mask) {
      grColorMask(FXFALSE,FXFALSE);
      grBufferClear(fxMesa->clearC, fxMesa->clearA,
                    (FxU16)(ctx->Depth.Clear*0xffff));

      grColorMask(ctx->Color.ColorMask[RCOMP] ||
                  ctx->Color.ColorMask[GCOMP] ||
                  ctx->Color.ColorMask[BCOMP],
                  ctx->Color.ColorMask[ACOMP] && fxMesa->haveAlphaBuffer);
    }

    newmask=mask & (~(GL_DEPTH_BUFFER_BIT));
    break;
  default:
    newmask=mask;
    break;
  }
   
  return newmask;
}


/*  Set the buffer used in double buffering */
static GLboolean fxDDSetBuffer(GLcontext *ctx, GLenum mode )
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDSetBuffer(%x)\n",mode);
  }

  if (mode == GL_FRONT_LEFT) {
    fxMesa->currentFB = GR_BUFFER_FRONTBUFFER;
    grRenderBuffer(fxMesa->currentFB);
    return GL_TRUE;
  }
  else if (mode == GL_BACK_LEFT) {
    fxMesa->currentFB = GR_BUFFER_BACKBUFFER;
    grRenderBuffer(fxMesa->currentFB);
    return GL_TRUE;
  }
  else {
    return GL_FALSE;
  }
}



static GLboolean fxDDDrawBitMap(GLcontext *ctx, GLint px, GLint py,
                                GLsizei width, GLsizei height,
                                const struct gl_pixelstore_attrib *unpack,
                                const GLubyte *bitmap)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  FxU16 *p;
  GrLfbInfo_t info;
  const GLubyte *pb;
  int x,y,xmin,xmax,ymin,ymax;
  GLint r,g,b,a,scrwidth,scrheight,stride;
  FxU16 color;

  /* TODO: with a little work, these bitmap unpacking parameter restrictions
   * could be removed.
   */
  if((unpack->Alignment!=1) ||
     (unpack->RowLength!=0) ||
     (unpack->SkipPixels!=0) ||
     (unpack->SkipRows!=0) ||
     (unpack->SwapBytes) ||
     (unpack->LsbFirst))
    return GL_FALSE;

  if (ctx->Scissor.Enabled) {
        xmin=ctx->Scissor.X;
        xmax=ctx->Scissor.X+ctx->Scissor.Width;
        ymin=ctx->Scissor.Y;
        ymax=ctx->Scissor.Y+ctx->Scissor.Height;
  } else {
        xmin=0;
        xmax=fxMesa->width;
        ymin=0;
        ymax=fxMesa->height;
  }


#define ISCLIPPED(rx) ( ((rx)<xmin) || ((rx)>=xmax) )
#define DRAWBIT(i) {       \
  if(!ISCLIPPED(x+px))     \
    if( (*pb) & (1<<(i)) ) \
      (*p)=color;          \
  p++;                     \
  x++;                     \
  if(x>=width) {           \
    pb++;                  \
    break;                 \
  }                        \
}

  scrwidth=fxMesa->width;
  scrheight=fxMesa->height;

  if((px>=scrwidth) || (px+width<=0) || (py>=scrheight) || (py+height<=0))
    return GL_TRUE;

  pb=bitmap;

  if(py<0) {
    pb+=(height*(-py)) >> (3+1);
    height+=py;
    py=0;
  }

  if(py+height>=scrheight)
    height-=(py+height)-scrheight;

  info.size=sizeof(info);
  if(!grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                GR_LFBWRITEMODE_565,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
#ifndef FX_SILENT
    fprintf(stderr,"fx Driver: error locking the linear frame buffer\n");
#endif
    return GL_TRUE;
  }

  r=(GLint)(ctx->Current.RasterColor[0]*255.0f);
  g=(GLint)(ctx->Current.RasterColor[1]*255.0f);
  b=(GLint)(ctx->Current.RasterColor[2]*255.0f);
  a=(GLint)(ctx->Current.RasterColor[3]*255.0f);
  color=(FxU16)
    ( ((FxU16)0xf8 & b) <<(11-3))  |
    ( ((FxU16)0xfc & g) <<(5-3+1)) |
    ( ((FxU16)0xf8 & r) >> 3);

  stride=info.strideInBytes>>1;

  /* This code is a bit slow... */

  for(y=py;y<(py+height);y++) {

    if (y>=ymax)
        break;

    if (y<=ymin)
        continue;

    p=((FxU16 *)info.lfbPtr)+px+((scrheight-y)*stride);

    for(x=0;;) {
      DRAWBIT(7);       DRAWBIT(6);     DRAWBIT(5);     DRAWBIT(4);
      DRAWBIT(3);       DRAWBIT(2);     DRAWBIT(1);     DRAWBIT(0);
      pb++;
    }
  }

  grLfbUnlock(GR_LFB_WRITE_ONLY,fxMesa->currentFB);

#undef ISCLIPPED
#undef DRAWBIT

  return GL_TRUE;
}

static void fxDDFinish(GLcontext *ctx)
{
  FX_grFlush();
}


static GLint fxDDGetParameteri(const GLcontext *ctx, GLint param)
{
  switch(param) {
  case DD_HAVE_HARDWARE_FOG:
    return 1;
  default:
    fprintf(stderr,"fx Driver: internal error in fxDDGetParameteri(): %x\n",param);
    fxCloseHardware();
    exit(-1);
    return 0;
  }
}


void fxDDSetNearFar(GLcontext *ctx, GLfloat n, GLfloat f)
{
   FX_CONTEXT(ctx)->new_state |= FX_NEW_FOG;
   ctx->Driver.RenderStart = fxSetupFXUnits;
}

/* KW: Put the word Mesa in the render string because quakeworld
 * checks for this rather than doing a glGet(GL_MAX_TEXTURE_SIZE).
 * Why?
 */
static const GLubyte *fxDDGetString(GLcontext *ctx, GLenum name)
{
   switch (name) {
   case GL_RENDERER:
#if defined(GLX_DIRECT_RENDERING)
      return "Mesa Glide - DRI VB/V3";
#else
      return "Mesa Glide";
#endif
   default:
      return NULL;
   }
}


int fxDDInitFxMesaContext( fxMesaContext fxMesa )
{
  
   FX_setupGrVertexLayout();
   
   if (getenv("FX_EMULATE_SINGLE_TMU")) 
      fxMesa->haveTwoTMUs = GL_FALSE;
      
   fxMesa->emulateTwoTMUs = fxMesa->haveTwoTMUs;
   
   if (!getenv("FX_DONT_FAKE_MULTITEX")) 
      fxMesa->emulateTwoTMUs = GL_TRUE;
      
   if(getenv("FX_GLIDE_SWAPINTERVAL"))
      fxMesa->swapInterval=atoi(getenv("FX_GLIDE_SWAPINTERVAL"));
   else
      fxMesa->swapInterval=1;

   if(getenv("MESA_FX_SWAP_PENDING"))
      fxMesa->maxPendingSwapBuffers=atoi(getenv("MESA_FX_SWAP_PENDING"));
   else
      fxMesa->maxPendingSwapBuffers=2;
   
   fxMesa->color=0xffffffff;
   fxMesa->clearC=0;
   fxMesa->clearA=0;

   fxMesa->stats.swapBuffer=0;
   fxMesa->stats.reqTexUpload=0;
   fxMesa->stats.texUpload=0;
   fxMesa->stats.memTexUpload=0;

   fxMesa->tmuSrc=FX_TMU_NONE;
   fxMesa->lastUnitsMode=FX_UM_NONE;
   fxTMInit(fxMesa);

   /* FX units setup */

   fxMesa->unitsState.alphaTestEnabled=GL_FALSE;
   fxMesa->unitsState.alphaTestFunc=GR_CMP_ALWAYS;
   fxMesa->unitsState.alphaTestRefValue=0;

   fxMesa->unitsState.blendEnabled=GL_FALSE;
   fxMesa->unitsState.blendSrcFuncRGB=GR_BLEND_ONE;
   fxMesa->unitsState.blendDstFuncRGB=GR_BLEND_ZERO;
   fxMesa->unitsState.blendSrcFuncAlpha=GR_BLEND_ONE;
   fxMesa->unitsState.blendDstFuncAlpha=GR_BLEND_ZERO;

   fxMesa->unitsState.depthTestEnabled	=GL_FALSE;
   fxMesa->unitsState.depthMask		=GL_TRUE;
   fxMesa->unitsState.depthTestFunc	=GR_CMP_LESS;

   grColorMask(FXTRUE, fxMesa->haveAlphaBuffer ? FXTRUE : FXFALSE);
   if(fxMesa->haveDoubleBuffer) {
      fxMesa->currentFB=GR_BUFFER_BACKBUFFER;
      grRenderBuffer(GR_BUFFER_BACKBUFFER);
   } else {
      fxMesa->currentFB=GR_BUFFER_FRONTBUFFER;
      grRenderBuffer(GR_BUFFER_FRONTBUFFER);
   }
  
   fxMesa->state 	= NULL;
   fxMesa->fogTable 	= NULL;
  
   fxMesa->state 	= malloc(FX_grGetInteger(FX_GLIDE_STATE_SIZE));
   fxMesa->fogTable 	= malloc(FX_grGetInteger(FX_FOG_TABLE_ENTRIES)*sizeof(GrFog_t));
  
   if (!fxMesa->state || !fxMesa->fogTable) {
      if (fxMesa->state) free(fxMesa->state);
      if (fxMesa->fogTable) free(fxMesa->fogTable);
      return 0;
   }

   if(fxMesa->haveZBuffer)
      grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    
#if (!FXMESA_USE_ARGB)
   grLfbWriteColorFormat(GR_COLORFORMAT_ABGR); /* Not every Glide has this  */
#endif

   fxMesa->glCtx->Const.MaxTextureLevels=9;
   fxMesa->glCtx->Const.MaxTextureSize=256;
   fxMesa->glCtx->Const.MaxTextureUnits=fxMesa->emulateTwoTMUs ? 2 : 1;
   fxMesa->glCtx->NewState|=NEW_DRVSTATE1;
   fxMesa->new_state = NEW_ALL;
  
   fxDDSetupInit();
   fxDDCvaInit();
   fxDDClipInit();
   fxDDTrifuncInit();
   fxDDFastPathInit();

   fxSetupDDPointers(fxMesa->glCtx);
   fxDDRenderInit(fxMesa->glCtx);
   fxDDInitExtensions(fxMesa->glCtx);  

   fxDDSetNearFar(fxMesa->glCtx,1.0,100.0);
  
   grGlideGetState((GrState*)fxMesa->state);

   /* XXX Fix me: callback not registered when main VB is created.
    */
   if (fxMesa->glCtx->VB) 
      fxDDRegisterVB( fxMesa->glCtx->VB );
  
   /* XXX Fix me too: need to have the 'struct dd' prepared prior to
    * creating the context... The below is broken if you try to insert
    * new stages.  
    */
   if (fxMesa->glCtx->NrPipelineStages)
      fxMesa->glCtx->NrPipelineStages = fxDDRegisterPipelineStages( 
	 fxMesa->glCtx->PipelineStage,
	 fxMesa->glCtx->PipelineStage,
	 fxMesa->glCtx->NrPipelineStages);

   /* Run the config file */
   gl_context_initialize( fxMesa->glCtx );

   return 1;
}





void fxDDInitExtensions( GLcontext *ctx )
{
   fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

   gl_extensions_add( ctx, DEFAULT_ON, "3DFX_set_global_palette", 0 );
   gl_extensions_add( ctx, DEFAULT_ON, "GL_FXMESA_global_texture_lod_bias", 0);
   
   if(fxMesa->haveTwoTMUs)
      gl_extensions_add( ctx, DEFAULT_ON, "GL_EXT_texture_env_add", 0);
   
   if (!fxMesa->emulateTwoTMUs) 
      gl_extensions_disable( ctx, "GL_ARB_multitexture" );
}

/************************************************************************/
/************************************************************************/
/************************************************************************/

/* This is a no-op, since the z-buffer is in hardware */
static void fxAllocDepthBuffer(GLcontext *ctx)
{
   if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxAllocDepthBuffer()\n");
   }
}

/************************************************************************/
/************************************************************************/
/************************************************************************/

/* Check if the hardware supports the current context 
 *
 * Performs similar work to fxDDChooseRenderState() - should be merged.
 */
static GLboolean fxIsInHardware(GLcontext *ctx)
{
   fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;

  if (!ctx->Hint.AllowDrawMem)
     return GL_TRUE;		/* you'll take it and like it */

  if((ctx->RasterMask & STENCIL_BIT) ||
     ((ctx->Color.BlendEnabled) && (ctx->Color.BlendEquation!=GL_FUNC_ADD_EXT)) ||
     ((ctx->Color.ColorLogicOpEnabled) && (ctx->Color.LogicOp!=GL_COPY)) ||
     (ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) ||
     (!((ctx->Color.ColorMask[RCOMP]==ctx->Color.ColorMask[GCOMP]) &&
        (ctx->Color.ColorMask[GCOMP]==ctx->Color.ColorMask[BCOMP]) &&
        (ctx->Color.ColorMask[ACOMP]==ctx->Color.ColorMask[ACOMP])))
     )
  {
     return GL_FALSE;
  }
  /* Unsupported texture/multitexture cases */

  if(fxMesa->emulateTwoTMUs) {
    if((ctx->Enabled & (TEXTURE0_3D | TEXTURE1_3D)) ||
       /* Not very well written ... */
       ((ctx->Enabled & (TEXTURE0_1D | TEXTURE1_1D)) && 
        ((ctx->Enabled & (TEXTURE0_2D | TEXTURE1_2D))!=(TEXTURE0_2D | TEXTURE1_2D)))
       )
      return GL_FALSE;

    if((ctx->Texture.ReallyEnabled & TEXTURE0_2D) &&
       (ctx->Texture.Unit[0].EnvMode==GL_BLEND))
      return GL_FALSE;

    if((ctx->Texture.ReallyEnabled & TEXTURE1_2D) &&
       (ctx->Texture.Unit[1].EnvMode==GL_BLEND))
      return GL_FALSE;


    if (MESA_VERBOSE & (VERBOSE_DRIVER|VERBOSE_TEXTURE))
       fprintf(stderr, "fxMesa: fxIsInHardware, envmode is %s/%s\n",
	       gl_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
	       gl_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));

    /* KW: This was wrong (I think) and I changed it... which doesn't mean
     * it is now correct...
     */
    if((ctx->Enabled & (TEXTURE0_1D | TEXTURE0_2D | TEXTURE0_3D)) &&
       (ctx->Enabled & (TEXTURE1_1D | TEXTURE1_2D | TEXTURE1_3D)))
    {
       /* Can't use multipass to blend a multitextured triangle - fall
	* back to software.
	*/
       if (!fxMesa->haveTwoTMUs && ctx->Color.BlendEnabled) 
	  return GL_FALSE;
	  
       if ((ctx->Texture.Unit[0].EnvMode!=ctx->Texture.Unit[1].EnvMode) &&
	   (ctx->Texture.Unit[0].EnvMode!=GL_MODULATE) &&
	   (ctx->Texture.Unit[0].EnvMode!=GL_REPLACE)) /* q2, seems ok... */
       {
	  if (MESA_VERBOSE&VERBOSE_DRIVER)
	    fprintf(stderr, "fxMesa: unsupported multitex env mode\n");

	  return GL_FALSE;
       }
    }
  } else {
    if((ctx->Enabled & (TEXTURE1_1D | TEXTURE1_2D | TEXTURE1_3D)) ||
       /* Not very well written ... */
       ((ctx->Enabled & TEXTURE0_1D) && 
        (!(ctx->Enabled & TEXTURE0_2D)))
       )
      return GL_FALSE;

    
    if((ctx->Texture.ReallyEnabled & TEXTURE0_2D) &&
       (ctx->Texture.Unit[0].EnvMode==GL_BLEND))
      return GL_FALSE;
  }

  return GL_TRUE;
}



#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|NEW_PROJECTION|NEW_TEXTURE_MATRIX|NEW_USER_CLIP|NEW_CLIENT_STATE|NEW_TEXTURE_ENABLE))

static void fxDDUpdateDDPointers(GLcontext *ctx)
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint new_state = ctx->NewState;

  if (MESA_VERBOSE&(VERBOSE_DRIVER|VERBOSE_STATE)) 
    fprintf(stderr,"fxmesa: fxDDUpdateDDPointers(...)\n");

  if (new_state & (NEW_RASTER_OPS|NEW_TEXTURING)) 
     fxMesa->is_in_hardware = fxIsInHardware(ctx);

  if (fxMesa->is_in_hardware) {
    if (fxMesa->new_state)
      fxSetupFXUnits(ctx);

    if(new_state & INTERESTED) {
      fxDDChooseRenderState( ctx );
      fxMesa->RenderVBTables=fxDDChooseRenderVBTables(ctx);
      fxMesa->RenderVBClippedTab=fxMesa->RenderVBTables[0];
      fxMesa->RenderVBCulledTab=fxMesa->RenderVBTables[1];
      fxMesa->RenderVBRawTab=fxMesa->RenderVBTables[2];

      ctx->Driver.RasterSetup=fxDDChooseSetupFunction(ctx);
    }
      
    ctx->Driver.PointsFunc=fxMesa->PointsFunc;
    ctx->Driver.LineFunc=fxMesa->LineFunc;
    ctx->Driver.TriangleFunc=fxMesa->TriangleFunc;
    ctx->Driver.QuadFunc=fxMesa->QuadFunc;
  } else 
     fxMesa->render_index = FX_FALLBACK;
}


void fxSetupDDPointers(GLcontext *ctx)
{
  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxSetupDDPointers()\n");
  }

  ctx->Driver.UpdateState=fxDDUpdateDDPointers;

  ctx->Driver.AllocDepthBuffer=fxAllocDepthBuffer;
  ctx->Driver.DepthTestSpan=fxDDDepthTestSpanGeneric;
  ctx->Driver.DepthTestPixels=fxDDDepthTestPixelsGeneric;
  ctx->Driver.ReadDepthSpanFloat=fxDDReadDepthSpanFloat;
  ctx->Driver.ReadDepthSpanInt=fxDDReadDepthSpanInt;
         
  ctx->Driver.GetString=fxDDGetString;

  ctx->Driver.Dither=fxDDDither;

  ctx->Driver.NearFar=fxDDSetNearFar;

  ctx->Driver.GetParameteri=fxDDGetParameteri;

  ctx->Driver.ClearIndex=NULL;
  ctx->Driver.ClearColor=fxDDClearColor;
  ctx->Driver.Clear=fxDDClear;

  ctx->Driver.Index=NULL;
  ctx->Driver.Color=fxDDSetColor;

  ctx->Driver.SetBuffer=fxDDSetBuffer;
  ctx->Driver.GetBufferSize=fxDDBufferSize;

  ctx->Driver.Bitmap=fxDDDrawBitMap;
  ctx->Driver.DrawPixels=NULL;

  ctx->Driver.Finish=fxDDFinish;
  ctx->Driver.Flush=NULL;

  ctx->Driver.RenderStart=NULL;
  ctx->Driver.RenderFinish=NULL;

  ctx->Driver.TexEnv=fxDDTexEnv;
  ctx->Driver.TexImage=fxDDTexImg;
  ctx->Driver.TexSubImage=fxDDTexSubImg;
  ctx->Driver.TexParameter=fxDDTexParam;
  ctx->Driver.BindTexture=fxDDTexBind;
  ctx->Driver.DeleteTexture=fxDDTexDel;
  ctx->Driver.UpdateTexturePalette=fxDDTexPalette;
  ctx->Driver.UseGlobalTexturePalette=fxDDTexUseGlbPalette;

  ctx->Driver.RectFunc=NULL;

  ctx->Driver.AlphaFunc=fxDDAlphaFunc;
  ctx->Driver.BlendFunc=fxDDBlendFunc;
  ctx->Driver.DepthFunc=fxDDDepthFunc;
  ctx->Driver.DepthMask=fxDDDepthMask;
  ctx->Driver.ColorMask=fxDDColorMask;
  ctx->Driver.Fogfv=fxDDFogfv;
  ctx->Driver.Scissor=fxDDScissor;
  ctx->Driver.FrontFace=fxDDFrontFace;
  ctx->Driver.CullFace=fxDDCullFace;
  ctx->Driver.ShadeModel=fxDDShadeModel;
  ctx->Driver.Enable=fxDDEnable;
  

  ctx->Driver.RegisterVB=fxDDRegisterVB;
  ctx->Driver.UnregisterVB=fxDDUnregisterVB;

  ctx->Driver.RegisterPipelineStages = fxDDRegisterPipelineStages;

  ctx->Driver.OptimizeImmediatePipeline = 0; /* nothing done yet */
  ctx->Driver.OptimizePrecalcPipeline = 0;

/*    if (getenv("MESA_USE_FAST") || getenv("FX_USE_FAST")) */
/*       ctx->Driver.OptimizePrecalcPipeline = fxDDOptimizePrecalcPipeline; */

  if (!getenv("FX_NO_FAST")) 
      ctx->Driver.BuildPrecalcPipeline = fxDDBuildPrecalcPipeline; 

  ctx->Driver.TriangleCaps = DD_TRI_CULL|DD_TRI_OFFSET|DD_TRI_LIGHT_TWOSIDE;

  fxSetupDDSpanPointers(ctx);

  FX_CONTEXT(ctx)->render_index = 1; /* force an update */
  fxDDUpdateDDPointers(ctx);
}


#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_dd(void)
{
  return 0;
}

#endif  /* FX */
