/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file via_context.c
 * 
 * \author John Sheng (presumably of either VIA Technologies or S3 Graphics)
 * \author Others at VIA Technologies?
 * \author Others at S3 Graphics?
 */

#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "via_screen.h"
#include "via_dri.h"

#include "via_state.h"
#include "via_tex.h"
#include "via_span.h"
#include "via_tris.h"
#include "via_vb.h"
#include "via_ioctl.h"
#include "via_fb.h"
#include "via_regs.h"

#include <stdio.h>
#include "macros.h"

#define DRIVER_DATE	"20041215"

#include "vblank.h"
#include "utils.h"

#ifdef DEBUG
GLuint VIA_DEBUG = 0;
#endif
GLuint DRAW_FRONT = 0;
#define DMA_SIZE 2
GLuint VIA_PERFORMANCE = 0;
#ifdef PERFORMANCE_MEASURE
GLuint busy = 0;
GLuint idle = 0;
hash_element hash_table[HASH_TABLE_SIZE][HASH_TABLE_DEPTH];
#endif
/*=* John Sheng [2003.5.31]  agp tex *=*/

static GLboolean
AllocateBuffer(viaContextPtr vmesa)
{
    vmesa->front_base = vmesa->driScreen->pFB;
    if (vmesa->drawType == GLX_PBUFFER_BIT) {
        if (vmesa->front.map)
            via_free_front_buffer(vmesa);
        if (!via_alloc_front_buffer(vmesa))
            return GL_FALSE;
    }
    
    if (vmesa->hasBack) {
        if (vmesa->back.map)
            via_free_back_buffer(vmesa);
        if (!via_alloc_back_buffer(vmesa))
            return GL_FALSE;
    }

    if (vmesa->hasDepth || vmesa->hasStencil) {
        if (vmesa->depth.map)
            via_free_depth_buffer(vmesa);
        if (!via_alloc_depth_buffer(vmesa)) {
            via_free_depth_buffer(vmesa);
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

/**
 * Return various strings for \c glGetString.
 *
 * \sa glGetString
 */
static const GLubyte *viaGetString(GLcontext *ctx, GLenum name)
{
   static char buffer[128];
   unsigned   offset;


   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"VIA Technology";

   case GL_RENDERER: {
      static const char * const chipset_names[] = {
	 "UniChrome",
	 "CastleRock (CLE266)",
	 "UniChrome (KM400)",
	 "UniChrome (K8M800)",
	 "UniChrome (PM8x0/CN400)",
      };
      const viaContext * const via = VIA_CONTEXT(ctx);
      const unsigned id = via->viaScreen->deviceID;

      offset = driGetRendererString( buffer, 
				     chipset_names[(id > VIA_PM800) ? 0 : id],
				     DRIVER_DATE, 0 );
      return (GLubyte *)buffer;
   }

   default:
      return NULL;
   }
}


/**
 * Calculate a width that satisfies the hardware's alignment requirements.
 * On the Unichrome hardware, each scanline must be aligned to a multiple of
 * 16 pixels.
 *
 * \param width  Minimum buffer width, in pixels.
 * 
 * \returns A pixel width that meets the alignment requirements.
 */
static __inline__ unsigned
buffer_align( unsigned width )
{
    return (width + 0x0f) & ~0x0f;
}


/**
 * Calculate the framebuffer parameters for all buffers (front, back, depth,
 * and stencil) associated with the specified context.
 * 
 * \warning
 * This function also calls \c AllocateBuffer to actually allocate the
 * buffers.
 * 
 * \sa AllocateBuffer
 */
static GLboolean
calculate_buffer_parameters( viaContextPtr vmesa )
{
    const unsigned shift = vmesa->viaScreen->bitsPerPixel / 16;
    const unsigned extra = (vmesa->drawType == GLX_PBUFFER_BIT) ? 0 : 32;
    unsigned w;
    unsigned h;

    if (vmesa->drawType == GLX_PBUFFER_BIT) {
	w = vmesa->driDrawable->w;
	h = vmesa->driDrawable->h;
    }
    else { 
	w = vmesa->viaScreen->width;
	h = vmesa->viaScreen->height;

	vmesa->front.offset = 0;
	vmesa->front.map = (char *) vmesa->driScreen->pFB;
    }

    vmesa->front.pitch = buffer_align( w ) << shift;
    vmesa->front.size = vmesa->front.pitch * h;


    /* Allocate back-buffer */

    vmesa->back.pitch = (buffer_align( vmesa->driDrawable->w ) << shift) 
      + extra;
    vmesa->back.size = vmesa->back.pitch * vmesa->driDrawable->h;

    if (VIA_DEBUG) fprintf(stderr, "%s backbuffer: w = %d h = %d bpp = %d sizs = %d\n",
			   __FUNCTION__,
			   vmesa->back.pitch, 
			   vmesa->driDrawable->h,
			   8 << shift,
			   vmesa->back.size);

    /* Allocate depth-buffer */
    if ( vmesa->hasStencil || vmesa->hasDepth ) {
	const unsigned dShift = (vmesa->hasStencil)
	  ? 2 : (vmesa->depthBits / 16);

	vmesa->depth.pitch = (buffer_align( vmesa->driDrawable->w ) << dShift)
	  + extra;
	vmesa->depth.bpp = 8 << dShift;
	vmesa->depth.size = vmesa->depth.pitch * vmesa->driDrawable->h;
    }
    else {
	(void) memset( & vmesa->depth, 0, sizeof( vmesa->depth ) );
    }

    if (VIA_DEBUG) fprintf(stderr, "%s depthbuffer: w = %d h = %d bpp = %d sizs = %d\n", 
			   __FUNCTION__,
			   vmesa->depth.pitch,
			   vmesa->driDrawable->h,
			   vmesa->depth.bpp,
			   vmesa->depth.size);

    /*=* John Sheng [2003.5.31] flip *=*/
    if( vmesa->viaScreen->width == vmesa->driDrawable->w && 
	vmesa->viaScreen->height == vmesa->driDrawable->h ) {
#define ALLOW_EXPERIMENTAL_PAGEFLIP 1       
#if ALLOW_EXPERIMENTAL_PAGEFLIP
	vmesa->doPageFlip = GL_TRUE;
#else
	vmesa->doPageFlip = GL_FALSE;
#endif
	vmesa->currentPage = 0;
	vmesa->back.pitch = vmesa->front.pitch;
    }

    if (!AllocateBuffer(vmesa)) {
	FREE(vmesa);
	return GL_FALSE;
    }
    
    return GL_TRUE;
}


void viaReAllocateBuffers(GLframebuffer *drawbuffer)
{
    GET_CURRENT_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    ctx->DrawBuffer->Width = drawbuffer->Width;
    ctx->DrawBuffer->Height = drawbuffer->Height;

    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    ctx->DrawBuffer->Accum = 0;
     
    vmesa->driDrawable->w = ctx->DrawBuffer->Width;
    vmesa->driDrawable->h = ctx->DrawBuffer->Height;

    LOCK_HARDWARE(vmesa);
    calculate_buffer_parameters( vmesa );
    UNLOCK_HARDWARE(vmesa);

    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

static void viaBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
    GET_CURRENT_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);       
    *width = vmesa->driDrawable->w;
    *height = vmesa->driDrawable->h;
}

/* Extension strings exported by the Unichrome driver.
 */
static const char * const card_extensions[] = 
{
   "GL_ARB_multitexture",
   "GL_ARB_point_parameters",
   "GL_ARB_texture_env_add",
   "GL_ARB_texture_env_combine",
   "GL_ARB_texture_env_dot3",
   "GL_ARB_texture_mirrored_repeat",
   "GL_EXT_stencil_wrap",
   "GL_EXT_texture_env_combine",
   "GL_EXT_texture_env_dot3",
   "GL_EXT_texture_lod_bias",
   "GL_NV_blend_square",
   NULL
};

extern const struct tnl_pipeline_stage _via_fastrender_stage;
extern const struct tnl_pipeline_stage _via_render_stage;

static const struct tnl_pipeline_stage *via_pipeline[] = {
    &_tnl_vertex_transform_stage,
    &_tnl_normal_transform_stage,
    &_tnl_lighting_stage,
    &_tnl_fog_coordinate_stage,
    &_tnl_texgen_stage,
    &_tnl_texture_transform_stage,
    /* REMOVE: point attenuation stage */
#if 1
    &_via_fastrender_stage,     /* ADD: unclipped rastersetup-to-dma */
    &_via_render_stage,         /* ADD: modification from _tnl_render_stage */
#endif
    &_tnl_render_stage,
    0,
};


static GLboolean
AllocateDmaBuffer(const GLvisual *visual, viaContextPtr vmesa)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
    
    if (!via_alloc_dma_buffer(vmesa)) {
	if (vmesa->front.map)
	    via_free_front_buffer(vmesa);
        if (vmesa->back.map) 
	    via_free_back_buffer(vmesa);
        if (vmesa->depth.map)
	    via_free_depth_buffer(vmesa);
	    
        return GL_FALSE;
    }   
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
    return GL_TRUE;
}

static void
InitVertexBuffer(viaContextPtr vmesa)
{
    GLuint *addr;

    addr = (GLuint *)vmesa->dma;
    *addr = 0xF210F110;
    *addr = (HC_ParaType_NotTex << 16);
    *addr = 0xcccccccc;
    *addr = 0xdddddddd;

    vmesa->dmaLow = DMA_OFFSET;
    vmesa->dmaHigh = VIA_DMA_BUFSIZ;
    vmesa->dmaAddr = (unsigned char *)vmesa->dma;
    vmesa->dmaLastPrim = vmesa->dmaLow;
}

static void
FreeBuffer(viaContextPtr vmesa)
{
    if (vmesa->front.map)
	via_free_front_buffer(vmesa);

    if (vmesa->back.map)
        via_free_back_buffer(vmesa);

    if (vmesa->depth.map)
        via_free_depth_buffer(vmesa);

    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
}

static int
get_ust_nop( int64_t * ust )
{
   *ust = 1;
   return 0;
}

GLboolean
viaCreateContext(const __GLcontextModes *mesaVis,
                 __DRIcontextPrivate *driContextPriv,
                 void *sharedContextPrivate)
{
    GLcontext *ctx, *shareCtx;
    viaContextPtr vmesa;
    __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
    viaScreenPrivate *viaScreen = (viaScreenPrivate *)sPriv->private;
    drm_via_sarea_t *saPriv = (drm_via_sarea_t *)
        (((GLubyte *)sPriv->pSAREA) + viaScreen->sareaPrivOffset);
    struct dd_function_table functions;

    /* Allocate via context */
    vmesa = (viaContextPtr) CALLOC_STRUCT(via_context_t);
    if (!vmesa) {
        return GL_FALSE;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    

    /* Parse configuration files.
     */
    driParseConfigFiles (&vmesa->optionCache, &viaScreen->optionCache,
			 sPriv->myNum, "via");

    /* pick back buffer */
    if (mesaVis->doubleBufferMode) {
	vmesa->hasBack = GL_TRUE;
    }
    else {
	vmesa->hasBack = GL_FALSE;
    }
    /* pick z buffer */	
    if (mesaVis->haveDepthBuffer) {
	vmesa->hasDepth = GL_TRUE;
	vmesa->depthBits = mesaVis->depthBits;
    }
    else {
	vmesa->hasDepth = GL_FALSE;
	vmesa->depthBits = 0;
    }
    /* pick stencil buffer */
    if (mesaVis->haveStencilBuffer) {
	vmesa->hasStencil = GL_TRUE;
	vmesa->stencilBits = mesaVis->stencilBits;
    }
    else {
	vmesa->hasStencil = GL_FALSE;
	vmesa->stencilBits = 0;
    }

    _mesa_init_driver_functions(&functions);
    viaInitTextureFuncs(&functions);

    /* Allocate the Mesa context */
    if (sharedContextPrivate)
        shareCtx = ((viaContextPtr) sharedContextPrivate)->glCtx;
    else
        shareCtx = NULL;

    vmesa->glCtx = _mesa_create_context(mesaVis, shareCtx, &functions, (void*) vmesa);
    
    vmesa->shareCtx = shareCtx;
    
    if (!vmesa->glCtx) {
        FREE(vmesa);
        return GL_FALSE;
    }
    driContextPriv->driverPrivate = vmesa;

    ctx = vmesa->glCtx;
    
    /* check */
    /*=* John Sheng [2003.7.2] for visual config number can't excess 8 *=*/
    /*if (viaScreen->textureSize < 2 * 1024 * 1024) {
        ctx->Const.MaxTextureLevels = 9;
    }
    else if (viaScreen->textureSize < 8 * 1024 * 1024) {
        ctx->Const.MaxTextureLevels = 10;
    }
    else {
        ctx->Const.MaxTextureLevels = 11;
    }*/
    ctx->Const.MaxTextureLevels = 11;
    
    ctx->Const.MaxTextureUnits = 2;
    ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
    ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

    ctx->Const.MinLineWidth = 1.0;
    ctx->Const.MinLineWidthAA = 1.0;
    ctx->Const.MaxLineWidth = 3.0;
    ctx->Const.MaxLineWidthAA = 3.0;
    ctx->Const.LineWidthGranularity = 1.0;

    ctx->Const.MinPointSize = 1.0;
    ctx->Const.MinPointSizeAA = 1.0;
    ctx->Const.MaxPointSize = 3.0;
    ctx->Const.MaxPointSizeAA = 3.0;
    ctx->Const.PointSizeGranularity = 1.0;

    ctx->Driver.GetBufferSize = viaBufferSize;
/*    ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;  *//* FIXME ?? */
    ctx->Driver.GetString = viaGetString;

    ctx->DriverCtx = (void *)vmesa;
    vmesa->glCtx = ctx;

    /* Initialize the software rasterizer and helper modules.
     */
    _swrast_CreateContext(ctx);
    _ac_CreateContext(ctx);
    _tnl_CreateContext(ctx);
    _swsetup_CreateContext(ctx);

    /* Install the customized pipeline:
     */
    _tnl_destroy_pipeline(ctx);
    _tnl_install_pipeline(ctx, via_pipeline);

    /* Configure swrast and T&L to match hardware characteristics:
     */
    _swrast_allow_pixel_fog(ctx, GL_FALSE);
    _swrast_allow_vertex_fog(ctx, GL_TRUE);
    _tnl_allow_pixel_fog(ctx, GL_FALSE);
    _tnl_allow_vertex_fog(ctx, GL_TRUE);

/*     vmesa->display = dpy; */
    vmesa->display = sPriv->display;
    
    vmesa->hHWContext = driContextPriv->hHWContext;
    vmesa->driFd = sPriv->fd;
    vmesa->driHwLock = &sPriv->pSAREA->lock;

    vmesa->viaScreen = viaScreen;
    vmesa->driScreen = sPriv;
    vmesa->sarea = saPriv;
    vmesa->glBuffer = NULL;

    vmesa->texHeap = mmInit(0, viaScreen->textureSize);
    vmesa->stippleInHw = 1;
    vmesa->renderIndex = ~0;
    vmesa->dirty = VIA_UPLOAD_ALL;
    vmesa->uploadCliprects = GL_TRUE;
    vmesa->needUploadAllState = 1;

    make_empty_list(&vmesa->TexObjList);
    make_empty_list(&vmesa->SwappedOut);

    vmesa->CurrentTexObj[0] = 0;
    vmesa->CurrentTexObj[1] = 0;
    
    _math_matrix_ctr(&vmesa->ViewportMatrix);

   driInitExtensions( ctx, card_extensions, GL_TRUE );
    viaInitStateFuncs(ctx);
    viaInitTextures(ctx);
    viaInitTriFuncs(ctx);
    viaInitSpanFuncs(ctx);
    viaInitIoctlFuncs(ctx);
    viaInitVB(ctx);
    viaInitState(ctx);
        
#ifdef DEBUG
    if (getenv("VIA_DEBUG"))
	VIA_DEBUG = 1;
    else
	VIA_DEBUG = 0;	
#endif	

    if (getenv("DRAW_FRONT"))
	DRAW_FRONT = 1;
    else
	DRAW_FRONT = 0;	
	
#ifdef PERFORMANCE_MEASURE
    if (getenv("VIA_PERFORMANCE"))
	VIA_PERFORMANCE = 1;
    else
	VIA_PERFORMANCE = 0;	
	
    {
	int i, j;
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
	    for (j = 0; j < HASH_TABLE_DEPTH; j ++) {
		hash_table[i][j].count = 0;
		sprintf(hash_table[i][j].func, "%s", "NULL");
	    }
	}
    }
#endif	

    /* I don't understand why this isn't working:
     */
    vmesa->vblank_flags =
       vmesa->viaScreen->irqEnabled ?
        driGetDefaultVBlankFlags(&vmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

    /* Hack this up in its place:
     */
    vmesa->vblank_flags = getenv("VIA_VSYNC") ? VBLANK_FLAG_SYNC : VBLANK_FLAG_NO_IRQ;

    
    vmesa->get_ust = (PFNGLXGETUSTPROC) glXGetProcAddress( (const GLubyte *) "__glXGetUST" );
    if ( vmesa->get_ust == NULL ) {
       vmesa->get_ust = get_ust_nop;
    }
    (*vmesa->get_ust)( & vmesa->swap_ust );


    if (!AllocateDmaBuffer(mesaVis, vmesa)) {
	fprintf(stderr ,"AllocateDmaBuffer fail\n");
        FREE(vmesa);
        return GL_FALSE;
    }

    InitVertexBuffer(vmesa);
    
    vmesa->regMMIOBase = (GLuint *)((GLuint)viaScreen->reg);
    vmesa->pnGEMode = (GLuint *)((GLuint)viaScreen->reg + 0x4);
    vmesa->regEngineStatus = (GLuint *)((GLuint)viaScreen->reg + 0x400);
    vmesa->regTranSet = (GLuint *)((GLuint)viaScreen->reg + 0x43C);
    vmesa->regTranSpace = (GLuint *)((GLuint)viaScreen->reg + 0x440);
    vmesa->agpBase = viaScreen->agpBase;
    if (VIA_DEBUG) {
	fprintf(stderr, "regEngineStatus = %x\n", *vmesa->regEngineStatus);
    }
    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    {
#ifndef USE_XINERAMA
        vmesa->saam = 0;
#else
	GLboolean saam = XineramaIsActive(vmesa->display);
	int count = 0, fbSize;

	if (saam && vmesa->viaScreen->drixinerama) {
	    vmesa->xsi = XineramaQueryScreens(vmesa->display, &count);
	    /* Test RightOf or Down */
	    if (vmesa->xsi[0].x_org == 0 && vmesa->xsi[0].y_org == 0) {
		if (vmesa->xsi[1].x_org == vmesa->xsi[1].width) {
		    vmesa->saam = RightOf;
		}
		else {
		    vmesa->saam = Down;
		}
	    }
	    /* Test LeftOf or Up */
	    else if (vmesa->xsi[0].x_org == vmesa->xsi[0].width) {
		vmesa->saam = LeftOf;
	    }
	    else if (vmesa->xsi[0].y_org == vmesa->xsi[0].height) {
		vmesa->saam = Up;
	    }
	    else
		vmesa->saam = 0;
		
		    
	    fbSize = vmesa->viaScreen->fbSize;
	}
	else
	    vmesa->saam = 0;
#endif
    }
    
    vmesa->pSaamRects = (drm_clip_rect_t *) malloc(sizeof(drm_clip_rect_t));    
    return GL_TRUE;
}

void
viaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    assert(vmesa); /* should never be null */
/*     viaFlushPrimsLocked(vmesa); */
    WAIT_IDLE

    if (vmesa) {
	/*=* John Sheng [2003.5.31]  agp tex *=*/
	if(VIA_DEBUG) fprintf(stderr, "agpFullCount = %d\n", vmesa->agpFullCount);    
	
	_swsetup_DestroyContext(vmesa->glCtx);
        _tnl_DestroyContext(vmesa->glCtx);
        _ac_DestroyContext(vmesa->glCtx);
        _swrast_DestroyContext(vmesa->glCtx);
        viaFreeVB(vmesa->glCtx);
	FreeBuffer(vmesa);
        /* free the Mesa context */
	_mesa_destroy_context(vmesa->glCtx);
	vmesa->glCtx->DriverCtx = NULL;
        FREE(vmesa);
    }
    
    P_M_R;

#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) fprintf(stderr, "idle = %d\n", idle);
    if (VIA_PERFORMANCE) fprintf(stderr, "busy = %d\n", busy);
#endif    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaXMesaSetFrontClipRects(viaContextPtr vmesa)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;

    vmesa->numClipRects = dPriv->numClipRects;
    vmesa->pClipRects = dPriv->pClipRects;
    vmesa->drawX = dPriv->x;
    vmesa->drawY = dPriv->y;
    vmesa->drawW = dPriv->w;
    vmesa->drawH = dPriv->h;

    viaEmitDrawingRectangle(vmesa);
    vmesa->uploadCliprects = GL_TRUE;
}

void viaXMesaSetBackClipRects(viaContextPtr vmesa)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    /*=* John Sheng [2003.6.9] fix glxgears dirty screen */
    /*if (vmesa->saam) {*/
	    vmesa->numClipRects = dPriv->numClipRects;
    	    vmesa->pClipRects = dPriv->pClipRects;
    	    vmesa->drawX = dPriv->x;
    	    vmesa->drawY = dPriv->y;
	    vmesa->drawW = dPriv->w;
	    vmesa->drawH = dPriv->h;
    /*}
    else {
	if (dPriv->numBackClipRects == 0) {
    	    vmesa->numClipRects = dPriv->numClipRects;
    	    vmesa->pClipRects = dPriv->pClipRects;
    	    vmesa->drawX = dPriv->x;
    	    vmesa->drawY = dPriv->y;
	    vmesa->drawW = dPriv->w;
	    vmesa->drawH = dPriv->h;
	}
	else {
    	    vmesa->numClipRects = dPriv->numBackClipRects;
    	    vmesa->pClipRects = dPriv->pBackClipRects;
    	    vmesa->drawX = dPriv->backX;
    	    vmesa->drawY = dPriv->backY;
	    vmesa->drawW = dPriv->w;
	    vmesa->drawH = dPriv->h;
	}
    }*/
    viaEmitDrawingRectangle(vmesa);
    vmesa->uploadCliprects = GL_TRUE;
}

void viaXMesaWindowMoved(viaContextPtr vmesa)
{
    GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;
#ifdef USE_XINERAMA
    GLuint side = 0;
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
#endif
    
    switch (vmesa->glCtx->Color._DrawDestMask[0]) {
    case __GL_FRONT_BUFFER_MASK: 
        viaXMesaSetFrontClipRects(vmesa);
        break;
    case __GL_BACK_BUFFER_MASK:
        viaXMesaSetBackClipRects(vmesa);
        break;
    default:
        break;
    }

#ifndef USE_XINERAMA
    vmesa->viaScreen->fbOffset = 0;
    vmesa->saam &= ~S1;
    vmesa->saam |= S0;
#else
    side = vmesa->saam & P_MASK;
    
    switch (side) {
	case RightOf:
	    /* full in screen 1 */
	    if (vmesa->drawX >= vmesa->xsi[0].width) {
		vmesa->viaScreen->fbOffset = vmesa->viaScreen->fbSize;
		vmesa->drawX = vmesa->drawX - vmesa->xsi[1].width;
		vmesa->numClipRects = dPriv->numBackClipRects;
    		vmesa->pClipRects = dPriv->pBackClipRects;
    		vmesa->drawX = dPriv->backX;
    		vmesa->drawY = dPriv->backY;
		vmesa->saam &= ~S0;
		vmesa->saam |= S1;
	    }
	    /* full in screen 0 */
	    else if ((vmesa->drawX + vmesa->drawW) <= vmesa->xsi[0].width) {
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam &= ~S1;
		vmesa->saam |= S0;
	    }
	    /* between screen 0 && screen 1 */
	    else {
		vmesa->numSaamRects = dPriv->numBackClipRects;
    		vmesa->pSaamRects = dPriv->pBackClipRects;
    		vmesa->drawXSaam = dPriv->backX;
    		vmesa->drawYSaam = dPriv->backY;
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam |= S0;
		vmesa->saam |= S1;
	    }
    	    break;
	case LeftOf:
	    /* full in screen 1 */
	    if (vmesa->drawX + vmesa->drawW <= 0) {
		vmesa->viaScreen->fbOffset = vmesa->viaScreen->fbSize;
		vmesa->drawX = vmesa->drawX + vmesa->xsi[1].width;
		vmesa->numClipRects = dPriv->numBackClipRects;
    		vmesa->pClipRects = dPriv->pBackClipRects;
    		vmesa->drawX = dPriv->backX;
    		vmesa->drawY = dPriv->backY;		
		vmesa->saam &= ~S0;
		vmesa->saam |= S1;
	    }
	    /* full in screen 0 */
	    else if (vmesa->drawX >= 0) {
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam &= ~S1;
		vmesa->saam |= S0;
	    }
	    /* between screen 0 && screen 1 */
	    else {
		vmesa->numSaamRects = dPriv->numBackClipRects;
    		vmesa->pSaamRects = dPriv->pBackClipRects;
    		vmesa->drawXSaam = dPriv->backX;
    		vmesa->drawYSaam = dPriv->backY;
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam |= S0;
		vmesa->saam |= S1;
	    }
    	    break;
	case Down :
	    /* full in screen 1 */
	    if (vmesa->drawY >= vmesa->xsi[0].height) {
		vmesa->viaScreen->fbOffset = vmesa->viaScreen->fbSize;
		vmesa->drawY = vmesa->drawY - vmesa->xsi[1].height;
		vmesa->numClipRects = dPriv->numBackClipRects;
    		vmesa->pClipRects = dPriv->pBackClipRects;
    		vmesa->drawX = dPriv->backX;
    		vmesa->drawY = dPriv->backY;
		vmesa->saam &= ~S0;
		vmesa->saam |= S1;
	    }
	    /* full in screen 0 */
	    else if ((vmesa->drawY + vmesa->drawH) <= vmesa->xsi[0].height) {
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam &= ~S1;
		vmesa->saam |= S0;
	    }
	    /* between screen 0 && screen 1 */
	    else {
		vmesa->numSaamRects = dPriv->numBackClipRects;
    		vmesa->pSaamRects = dPriv->pBackClipRects;
    		vmesa->drawXSaam = dPriv->backX;
    		vmesa->drawYSaam = dPriv->backY;
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam |= S0;
		vmesa->saam |= S1;
	    }
    	    break;
	case Up :
	    /* full in screen 1 */
	    if ((vmesa->drawY + vmesa->drawH) <= 0) {
		vmesa->viaScreen->fbOffset = vmesa->viaScreen->fbSize;
		vmesa->drawY = vmesa->drawY + vmesa->xsi[1].height;
		vmesa->numClipRects = dPriv->numBackClipRects;
    		vmesa->pClipRects = dPriv->pBackClipRects;
    		vmesa->drawX = dPriv->backX;
    		vmesa->drawY = dPriv->backY;
		vmesa->saam &= ~S0;
		vmesa->saam |= S1;
	    }
	    /* full in screen 0 */
	    else if (vmesa->drawY >= 0) {
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam &= ~S1;
		vmesa->saam |= S0;
	    }
	    /* between screen 0 && screen 1 */
	    else {
		vmesa->numSaamRects = dPriv->numBackClipRects;
    		vmesa->pSaamRects = dPriv->pBackClipRects;
    		vmesa->drawXSaam = dPriv->backX;
    		vmesa->drawYSaam = dPriv->backY;
		vmesa->viaScreen->fbOffset = 0;
		vmesa->saam |= S0;
		vmesa->saam |= S1;
	    }
    	    break;
	default:
	    vmesa->viaScreen->fbOffset = 0;
    }
#endif
    
    {
	GLuint pitch, offset;
	pitch = vmesa->front.pitch;
	offset = vmesa->viaScreen->fbOffset + (vmesa->drawY * pitch + vmesa->drawX * bytePerPixel);
	vmesa->drawXoff = (GLuint)((offset & 0x1f) / bytePerPixel);
	if (vmesa->saam) {
	    if (vmesa->pSaamRects) {
		offset = vmesa->viaScreen->fbOffset + (vmesa->pSaamRects[0].y1 * pitch + 
		    vmesa->pSaamRects[0].x1 * bytePerPixel);
		vmesa->drawXoffSaam = (GLuint)((offset & 0x1f) / bytePerPixel);
	    }
	    else
		vmesa->drawXoffSaam = 0;
	}
	else
	    vmesa->drawXoffSaam = 0;
    }
    
    viaCalcViewport(vmesa->glCtx);
}

GLboolean
viaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

GLboolean
viaMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
  
    if (VIA_DEBUG) {
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driContextPriv);
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driDrawPriv);    
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driReadPriv);
    }	

    if (driContextPriv) {
        viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;

	if (VIA_DEBUG) fprintf(stderr, "viaMakeCurrent: w = %d\n", vmesa->driDrawable->w);

	if ( vmesa->driDrawable != driDrawPriv ) {
	   driDrawableInitVBlank( driDrawPriv, vmesa->vblank_flags );
	   vmesa->driDrawable = driDrawPriv;
	   if ( ! calculate_buffer_parameters( vmesa ) ) {
	      return GL_FALSE;
	   }
	}

        _mesa_make_current2(vmesa->glCtx,
                            (GLframebuffer *)driDrawPriv->driverPrivate,
                            (GLframebuffer *)driReadPriv->driverPrivate);
	if (VIA_DEBUG) fprintf(stderr, "Context %d MakeCurrent\n", vmesa->hHWContext);
        viaXMesaWindowMoved(vmesa);
    }
    else {
        _mesa_make_current(0,0);
    }
        
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

void viaGetLock(viaContextPtr vmesa, GLuint flags)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    __DRIscreenPrivate *sPriv = vmesa->driScreen;
    drm_via_sarea_t *sarea = vmesa->sarea;
    int me = vmesa->hHWContext;
    __DRIdrawablePrivate *pdp;
    __DRIscreenPrivate *psp;
    pdp = dPriv;
    psp = sPriv;
    if (VIA_DEBUG) {
       fprintf(stderr, "%s - in\n", __FUNCTION__);  
       fprintf(stderr, "is: %x non-contend: %x want: %x\n",
	       *(GLuint *)vmesa->driHwLock, vmesa->hHWContext, 
	       (DRM_LOCK_HELD|vmesa->hHWContext));
    }

    drmGetLock(vmesa->driFd, vmesa->hHWContext, flags);

    DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

    if (sarea->ctxOwner != me) {
        vmesa->uploadCliprects = GL_TRUE;
        sarea->ctxOwner = me;
	vmesa->needUploadAllState = 1;
    }

    viaXMesaWindowMoved(vmesa);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

#if 0
void viaLock(viaContextPtr vmesa, GLuint flags)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    __DRIscreenPrivate *sPriv = vmesa->driScreen;
    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
   
    /*=* John Sheng [2003.6.16] for xf43 */
    if(dPriv->pStamp == NULL)
	dPriv->pStamp = &dPriv->lastStamp;

    if (*(dPriv->pStamp) != dPriv->lastStamp || vmesa->saam) {
	GLuint scrn;
	scrn = vmesa->saam & S_MASK;
	
	DRM_SPINLOCK(&sPriv->pSAREA->drawable_lock, sPriv->drawLockID);

	if (scrn == S1)
	    __driUtilUpdateDrawableInfo(dPriv);
	else
	    DRI_VALIDATE_DRAWABLE_INFO_ONCE(dPriv);
	    
	viaXMesaWindowMoved(vmesa);
	DRM_SPINUNLOCK(&sPriv->pSAREA->drawable_lock, sPriv->drawLockID);
    }

    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
    
    return;
}
#endif

void viaUnLock(viaContextPtr vmesa, GLuint flags)
{
    drm_via_sarea_t *sarea = vmesa->sarea;
    int me = vmesa->hHWContext;
    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (VIA_DEBUG) fprintf(stderr, "sarea->ctxOwner = %d\n", sarea->ctxOwner);
    if (VIA_DEBUG) fprintf(stderr, "me = %d\n", me);
    if (sarea->ctxOwner == me) {
        sarea->ctxOwner = 0;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

void
viaSwapBuffers(__DRIdrawablePrivate *drawablePrivate)
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *)drawablePrivate;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);	
    if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
        viaContextPtr vmesa;
        GLcontext *ctx;
	
        vmesa = (viaContextPtr)dPriv->driContextPriv->driverPrivate;
        ctx = vmesa->glCtx;
        if (ctx->Visual.doubleBufferMode) {
            _mesa_notifySwapBuffers(ctx);
            if (vmesa->doPageFlip) {
                viaPageFlip(dPriv);
            }
            else {
                viaCopyBuffer(dPriv);
            }
        }
	else
	    VIA_FIREVERTICES(vmesa);
    }
    else {
        _mesa_problem(NULL, "viaSwapBuffers: drawable has no context!\n");
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);	
}
