/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_context.c,v 1.7 2003/02/08 21:26:45 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"
#include "radeon_vtxfmt.h"
#include "radeon_maos.h"

#define DRIVER_DATE	"20030328"

#include "vblank.h"
#include "utils.h"
#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0);
#endif



/* Return the width and height of the given buffer.
 */
static void radeonGetBufferSize( GLframebuffer *buffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   LOCK_HARDWARE( rmesa );
   *width  = rmesa->dri.drawable->w;
   *height = rmesa->dri.drawable->h;
   UNLOCK_HARDWARE( rmesa );
}

/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString( GLcontext *ctx, GLenum name )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = rmesa->radeonScreen->IsPCI ? 0 :
      rmesa->radeonScreen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "Radeon", DRIVER_DATE,
				     agp_mode );

      sprintf( & buffer[ offset ], "%s %sTCL",
	       ( rmesa->dri.drmMinor < 3 ) ? " DRM-COMPAT" : "",
	       !(rmesa->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
	       ? "" : "NO-" );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


/* Extension strings exported by the R100 driver.
 */
static const char * const card_extensions[] =
{
    "GL_ARB_multisample",
    "GL_ARB_multitexture",
    "GL_ARB_texture_border_clamp",
    "GL_ARB_texture_compression",
    "GL_ARB_texture_env_add",
    "GL_ARB_texture_env_combine",
    "GL_ARB_texture_env_dot3",
    "GL_ARB_texture_mirrored_repeat",
    "GL_EXT_blend_logic_op",
    "GL_EXT_blend_subtract",
    "GL_EXT_secondary_color",
    "GL_EXT_texture_edge_clamp",
    "GL_EXT_texture_env_add",
    "GL_EXT_texture_env_combine",
    "GL_EXT_texture_env_dot3",
    "GL_EXT_texture_filter_anisotropic",
    "GL_EXT_texture_lod_bias",
    "GL_ATI_texture_env_combine3",
    "GL_ATI_texture_mirror_once",
    "GL_IBM_texture_mirrored_repeat",
    "GL_MESA_ycbcr_texture",
    "GL_NV_blend_square",
    "GL_SGIS_generate_mipmap",
    "GL_SGIS_texture_border_clamp",
    "GL_SGIS_texture_edge_clamp",
    NULL
};

extern const struct gl_pipeline_stage _radeon_texrect_stage;
extern const struct gl_pipeline_stage _radeon_render_stage;
extern const struct gl_pipeline_stage _radeon_tcl_stage;

static const struct gl_pipeline_stage *radeon_pipeline[] = {

   /* Try and go straight to t&l
    */
   &_radeon_tcl_stage,  

   /* Catch any t&l fallbacks
    */
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,

   /* Scale texture rectangle to 0..1.
    */
   &_radeon_texrect_stage,

   &_radeon_render_stage,
   &_tnl_render_stage,		/* FALLBACK:  */
   0,
};



/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs( GLcontext *ctx )
{
    ctx->Driver.GetBufferSize		= radeonGetBufferSize;
    ctx->Driver.ResizeBuffers           = _swrast_alloc_buffers;
    ctx->Driver.GetString		= radeonGetString;

    ctx->Driver.Error			= NULL;
    ctx->Driver.DrawPixels		= NULL;
    ctx->Driver.Bitmap			= NULL;
}

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "tex",   DEBUG_TEXTURE },
    { "ioctl", DEBUG_IOCTL },
    { "prim",  DEBUG_PRIMS },
    { "vert",  DEBUG_VERTS },
    { "state", DEBUG_STATE },
    { "code",  DEBUG_CODEGEN },
    { "vfmt",  DEBUG_VFMT },
    { "vtxf",  DEBUG_VFMT },
    { "verb",  DEBUG_VERBOSE },
    { "dri",   DEBUG_DRI },
    { "dma",   DEBUG_DMA },
    { "san",   DEBUG_SANITY },
    { NULL,    0 }
};


static int
get_ust_nop( uint64_t * ust )
{
   *ust = 1;
   return 0;
}


/* Create the device specific context.
 */
GLboolean
radeonCreateContext( const __GLcontextModes *glVisual,
                     __DRIcontextPrivate *driContextPriv,
                     void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr screen = (radeonScreenPtr)(sPriv->private);
   radeonContextPtr rmesa;
   GLcontext *ctx, *shareCtx;
   int i;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the Radeon context */
   rmesa = (radeonContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((radeonContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   rmesa->glCtx = _mesa_create_context(glVisual, shareCtx, (void *) rmesa, GL_TRUE);
   if (!rmesa->glCtx) {
      FREE(rmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = rmesa;

   /* Init radeon context data */
   rmesa->dri.context = driContextPriv;
   rmesa->dri.screen = sPriv;
   rmesa->dri.drawable = NULL; /* Set by XMesaMakeCurrent */
   rmesa->dri.hwContext = driContextPriv->hHWContext;
   rmesa->dri.hwLock = &sPriv->pSAREA->lock;
   rmesa->dri.fd = sPriv->fd;

   /* If we don't have 1.3, fallback to the 1.1 interfaces.
    */
   if (getenv("RADEON_COMPAT") || sPriv->drmMinor < 3 ) 
      rmesa->dri.drmMinor = 1;
   else
      rmesa->dri.drmMinor = sPriv->drmMinor;

   rmesa->radeonScreen = screen;
   rmesa->sarea = (RADEONSAREAPrivPtr)((GLubyte *)sPriv->pSAREA +
				       screen->sarea_priv_offset);


   rmesa->dma.buf0_address = rmesa->radeonScreen->buffers->list[0].address;

   (void) memset( rmesa->texture_heaps, 0, sizeof( rmesa->texture_heaps ) );
   make_empty_list( & rmesa->swapped );

   rmesa->nr_heaps = screen->numTexHeaps;
   for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
      rmesa->texture_heaps[i] = driCreateTextureHeap( i, rmesa,
	    screen->texSize[i],
	    12,
	    RADEON_NR_TEX_REGIONS,
	    rmesa->sarea->texList[i],
	    & rmesa->sarea->texAge[i],
	    & rmesa->swapped,
	    sizeof( radeonTexObj ),
	    (destroy_texture_object_t *) radeonDestroyTexObj );

      driSetTextureSwapCounterLocation( rmesa->texture_heaps[i],
					& rmesa->c_textureSwaps );
   }

   rmesa->swtcl.RenderIndex = ~0;
   rmesa->lost_context = 1;

   /* Set the maximum texture size small enough that we can guarentee that
    * all texture units can bind a maximal texture and have them both in
    * texturable memory at once.
    */

   ctx = rmesa->glCtx;
   ctx->Const.MaxTextureUnits = 2;

   driCalculateMaxTextureLevels( rmesa->texture_heaps,
				 rmesa->nr_heaps,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
				 0,  /* 3D textures unsupported. */
				 0,  /* cube textures unsupported. */
				 11, /* max rect texture size is 2048x2048. */
				 12,
				 GL_FALSE );

   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   /* Set maxlocksize (and hence vb size) small enough to avoid
    * fallbacks in radeon_tcl.c.  ie. guarentee that all vertices can
    * fit in a single dma buffer for indexed rendering of quad strips,
    * etc.
    */
   ctx->Const.MaxArrayLockSize = 
      MIN2( ctx->Const.MaxArrayLockSize, 
 	    RADEON_BUFFER_SIZE / RADEON_MAX_TCL_VERTSIZE ); 

   rmesa->boxes = (getenv("LIBGL_PERFORMANCE_BOXES") != NULL);

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _ae_create_context( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, radeon_pipeline );
   ctx->Driver.FlushVertices = radeonFlushVertices;

   /* Try and keep materials and vertices separate:
    */
   _tnl_isolate_materials( ctx, GL_TRUE );


/*     _mesa_allow_light_in_model( ctx, GL_FALSE ); */

   /* Try and keep materials and vertices separate:
    */
   _tnl_isolate_materials( ctx, GL_TRUE );


   /* Configure swrast to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );


   _math_matrix_ctr( &rmesa->TexGenMatrix[0] );
   _math_matrix_ctr( &rmesa->TexGenMatrix[1] );
   _math_matrix_ctr( &rmesa->tmpmat );
   _math_matrix_set_identity( &rmesa->TexGenMatrix[0] );
   _math_matrix_set_identity( &rmesa->TexGenMatrix[1] );
   _math_matrix_set_identity( &rmesa->tmpmat );

   driInitExtensions( ctx, card_extensions, GL_TRUE );

   if (rmesa->dri.drmMinor >= 9)
      _mesa_enable_extension( ctx, "GL_NV_texture_rectangle");

   radeonInitDriverFuncs( ctx );
   radeonInitIoctlFuncs( ctx );
   radeonInitStateFuncs( ctx );
   radeonInitSpanFuncs( ctx );
   radeonInitTextureFuncs( ctx );
   radeonInitState( rmesa );
   radeonInitSwtcl( ctx );

   rmesa->iw.irq_seq = -1;
   rmesa->irqsEmitted = 0;
   rmesa->do_irqs = (rmesa->radeonScreen->irq && !getenv("RADEON_NO_IRQS"));

   rmesa->do_usleeps = !getenv("RADEON_NO_USLEEPS");

   rmesa->vblank_flags = (rmesa->do_irqs)
       ? driGetDefaultVBlankFlags() : VBLANK_FLAG_NO_IRQ;

#ifndef _SOLO
   rmesa->get_ust = (PFNGLXGETUSTPROC) glXGetProcAddress( "__glXGetUST" );
   if ( rmesa->get_ust == NULL ) 
#endif
   {
      rmesa->get_ust = get_ust_nop;
   }

   (*rmesa->get_ust)( & rmesa->swap_ust );


#if DO_DEBUG
   RADEON_DEBUG = driParseDebugString( getenv( "RADEON_DEBUG" ),
				       debug_control );
#endif

   if (getenv("RADEON_NO_RAST")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, RADEON_FALLBACK_DISABLE, 1); 
   }
   else if (getenv("RADEON_TCL_FORCE_ENABLE")) {
      fprintf(stderr, "Enabling TCL support...  this will probably crash\n");
      fprintf(stderr, "         your card if it isn't capable of TCL!\n");
      rmesa->radeonScreen->chipset |= RADEON_CHIPSET_TCL;
   } else if (getenv("RADEON_TCL_FORCE_DISABLE") ||
	    rmesa->dri.drmMinor < 3 ||
	    !(rmesa->radeonScreen->chipset & RADEON_CHIPSET_TCL)) {
      rmesa->radeonScreen->chipset &= ~RADEON_CHIPSET_TCL;
      fprintf(stderr, "disabling TCL support\n");
      TCL_FALLBACK(rmesa->glCtx, RADEON_TCL_FALLBACK_TCL_DISABLE, 1); 
   }

   if (rmesa->radeonScreen->chipset & RADEON_CHIPSET_TCL) {
      if (!getenv("RADEON_NO_VTXFMT"))
	 radeonVtxfmtInit( ctx );

      _tnl_need_dlist_norm_lengths( ctx, GL_FALSE );
   }
   return GL_TRUE;
}


/* Destroy the device specific context.
 */
/* Destroy the Mesa and driver specific context data.
 */
void radeonDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = (radeonContextPtr) driContextPriv->driverPrivate;
   radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (rmesa == current) {
      RADEON_FIREVERTICES( rmesa );
      _mesa_make_current2(NULL, NULL, NULL);
   }

   /* Free radeon context resources */
   assert(rmesa); /* should never be null */
   if ( rmesa ) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (rmesa->glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( rmesa->glCtx );
      _tnl_DestroyContext( rmesa->glCtx );
      _ac_DestroyContext( rmesa->glCtx );
      _swrast_DestroyContext( rmesa->glCtx );

      radeonDestroySwtcl( rmesa->glCtx );
      radeonReleaseArrays( rmesa->glCtx, ~0 );
      if (rmesa->dma.current.buf) {
	 radeonReleaseDmaRegion( rmesa, &rmesa->dma.current, __FUNCTION__ );
	 radeonFlushCmdBuf( rmesa, __FUNCTION__ );
      }

      if (!rmesa->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
	 if (!getenv("RADEON_NO_VTXFMT"))
	    radeonVtxfmtDestroy( rmesa->glCtx );

      /* free the Mesa context */
      rmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( rmesa->glCtx );

      if (rmesa->state.scissor.pClipRects) {
	 FREE(rmesa->state.scissor.pClipRects);
	 rmesa->state.scissor.pClipRects = 0;
      }

      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

	 /* this assert is not correct, default textures are always on swap list
	 assert( is_empty_list( & rmesa->swapped ) ); */

         for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
	    driDestroyTextureHeap( rmesa->texture_heaps[ i ] );
	    rmesa->texture_heaps[ i ] = NULL;
         }
      }

      FREE( rmesa );
   }
}




void
radeonSwapBuffers( __DRIdrawablePrivate *dPriv )
{

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      radeonContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */

         if ( rmesa->doPageFlip ) {
            radeonPageFlip( dPriv );
         }
         else {
            radeonCopyBuffer( dPriv );
         }
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean
radeonMakeCurrent( __DRIcontextPrivate *driContextPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      radeonContextPtr newCtx = 
	 (radeonContextPtr) driContextPriv->driverPrivate;

      if (RADEON_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx %p\n", __FUNCTION__, newCtx->glCtx);

      if ( newCtx->dri.drawable != driDrawPriv ) {
	 newCtx->dri.drawable = driDrawPriv;
	 radeonUpdateWindow( newCtx->glCtx );
	 radeonUpdateViewportOffset( newCtx->glCtx );
      }

      _mesa_make_current2( newCtx->glCtx,
			   (GLframebuffer *) driDrawPriv->driverPrivate,
			   (GLframebuffer *) driReadPriv->driverPrivate );

      if ( !newCtx->glCtx->Viewport.Width ) {
	 _mesa_set_viewport( newCtx->glCtx, 0, 0,
			     driDrawPriv->w, driDrawPriv->h );
      }

      if (newCtx->vb.enabled)
	 radeonVtxfmtMakeCurrent( newCtx->glCtx );

   } else {
      if (RADEON_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
      _mesa_make_current( 0, 0 );
   }

   if (RADEON_DEBUG & DEBUG_DRI)
      fprintf(stderr, "End %s\n", __FUNCTION__);
   return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean
radeonUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   radeonContextPtr rmesa = (radeonContextPtr) driContextPriv->driverPrivate;

   if (RADEON_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s ctx %p\n", __FUNCTION__, rmesa->glCtx);

   return GL_TRUE;
}
