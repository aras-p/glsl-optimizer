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

#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/matrix.h"
#include "main/extensions.h"
#include "main/framebuffer.h"
#include "main/state.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"
#include "radeon_maos.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#include "extension_helper.h"

#define DRIVER_DATE	"20061018"

#include "vblank.h"
#include "utils.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */

/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString( GLcontext *ctx, GLenum name )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = (rmesa->radeon.radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
      rmesa->radeon.radeonScreen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "Radeon", DRIVER_DATE,
				     agp_mode );

      sprintf( & buffer[ offset ], " %sTCL",
	       !(rmesa->radeon.TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
	       ? "" : "NO-" );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


/* Extension strings exported by the R100 driver.
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_blend_logic_op",             NULL },
    { "GL_EXT_blend_subtract",             GL_EXT_blend_minmax_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_texture_mirror_clamp",       NULL },
    { "GL_ATI_texture_env_combine3",       NULL },
    { "GL_ATI_texture_mirror_once",        NULL },
    { "GL_MESA_ycbcr_texture",             NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

extern const struct tnl_pipeline_stage _radeon_render_stage;
extern const struct tnl_pipeline_stage _radeon_tcl_stage;

static const struct tnl_pipeline_stage *radeon_pipeline[] = {

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

   &_radeon_render_stage,
   &_tnl_render_stage,		/* FALLBACK:  */
   NULL,
};



/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs( struct dd_function_table *functions )
{
    functions->GetString	= radeonGetString;
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
    { "sync",  DEBUG_SYNC },
    { NULL,    0 }
};

static void r100_get_lock(radeonContextPtr radeon)
{
   r100ContextPtr rmesa = (r100ContextPtr)radeon;
   drm_radeon_sarea_t *sarea = radeon->sarea;

   RADEON_STATECHANGE(rmesa, ctx);
   if (rmesa->radeon.sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |=
	 RADEON_COLOR_TILE_ENABLE;
   } else {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] &=
	 ~RADEON_COLOR_TILE_ENABLE;
   }
   
   if (sarea->ctx_owner != rmesa->radeon.dri.hwContext) {
      int i;
      sarea->ctx_owner = rmesa->radeon.dri.hwContext;
      
      for (i = 0; i < rmesa->radeon.nr_heaps; i++) {
	 DRI_AGE_TEXTURES(rmesa->radeon.texture_heaps[i]);
      }
   }
}

static void r100_vtbl_flush(GLcontext *ctx)
{
   RADEON_FIREVERTICES(R100_CONTEXT(ctx));
}

static void r100_vtbl_set_all_dirty(GLcontext *ctx)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   rmesa->hw.all_dirty = GL_TRUE;
}

static void r100_init_vtbl(radeonContextPtr radeon)
{
   radeon->vtbl.get_lock = r100_get_lock;
   radeon->vtbl.update_viewport_offset = radeonUpdateViewportOffset;
   radeon->vtbl.flush = r100_vtbl_flush;
   radeon->vtbl.set_all_dirty = r100_vtbl_set_all_dirty;
   radeon->vtbl.update_draw_buffer = radeonUpdateDrawBuffer;
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
   struct dd_function_table functions;
   r100ContextPtr rmesa;
   GLcontext *ctx, *shareCtx;
   int i;
   int tcl_mode, fthrottle_mode;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the Radeon context */
   rmesa = (r100ContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;

   r100_init_vtbl(&rmesa->radeon);

   /* init exp fog table data */
   radeonInitStaticFogData();
   
   /* Parse configuration files.
    * Do this here so that initialMaxAnisotropy is set before we create
    * the default textures.
    */
   driParseConfigFiles (&rmesa->radeon.optionCache, &screen->optionCache,
			screen->driScreen->myNum, "radeon");
   rmesa->radeon.initialMaxAnisotropy = driQueryOptionf(&rmesa->radeon.optionCache,
                                                 "def_max_anisotropy");

   if ( driQueryOptionb( &rmesa->radeon.optionCache, "hyperz" ) ) {
      if ( sPriv->drm_version.minor < 13 )
	 fprintf( stderr, "DRM version 1.%d too old to support HyperZ, "
			  "disabling.\n", sPriv->drm_version.minor );
      else
	 rmesa->using_hyperz = GL_TRUE;
   }

   if ( sPriv->drm_version.minor >= 15 )
      rmesa->texmicrotile = GL_TRUE;

   /* Init default driver functions then plug in our Radeon-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   radeonInitDriverFuncs( &functions );
   radeonInitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((radeonContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   rmesa->radeon.glCtx = _mesa_create_context(glVisual, shareCtx,
					      &functions, (void *) rmesa);
   if (!rmesa->radeon.glCtx) {
      FREE(rmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = rmesa;

   /* Init radeon context data */
   rmesa->radeon.dri.context = driContextPriv;
   rmesa->radeon.dri.screen = sPriv;
   rmesa->radeon.dri.drawable = NULL;
   rmesa->radeon.dri.readable = NULL;
   rmesa->radeon.dri.hwContext = driContextPriv->hHWContext;
   rmesa->radeon.dri.hwLock = &sPriv->pSAREA->lock;
   rmesa->radeon.dri.fd = sPriv->fd;
   rmesa->radeon.dri.drmMinor = sPriv->drm_version.minor;

   rmesa->radeon.radeonScreen = screen;
   rmesa->radeon.sarea = (drm_radeon_sarea_t *)((GLubyte *)sPriv->pSAREA +
				       screen->sarea_priv_offset);


   rmesa->dma.buf0_address = rmesa->radeon.radeonScreen->buffers->list[0].address;

   (void) memset( rmesa->radeon.texture_heaps, 0, sizeof( rmesa->radeon.texture_heaps ) );
   make_empty_list( & rmesa->radeon.swapped );

   rmesa->radeon.nr_heaps = screen->numTexHeaps;
   for ( i = 0 ; i < rmesa->radeon.nr_heaps ; i++ ) {
      rmesa->radeon.texture_heaps[i] = driCreateTextureHeap( i, rmesa,
	    screen->texSize[i],
	    12,
	    RADEON_NR_TEX_REGIONS,
	    (drmTextureRegionPtr)rmesa->radeon.sarea->tex_list[i],
	    & rmesa->radeon.sarea->tex_age[i],
	    & rmesa->radeon.swapped,
	    sizeof( radeonTexObj ),
	    (destroy_texture_object_t *) radeonDestroyTexObj );

      driSetTextureSwapCounterLocation( rmesa->radeon.texture_heaps[i],
					& rmesa->c_textureSwaps );
   }
   rmesa->radeon.texture_depth = driQueryOptioni (&rmesa->radeon.optionCache,
					   "texture_depth");
   if (rmesa->radeon.texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
      rmesa->radeon.texture_depth = ( screen->cpp == 4 ) ?
	 DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

   rmesa->swtcl.RenderIndex = ~0;
   rmesa->hw.all_dirty = GL_TRUE;

   /* Set the maximum texture size small enough that we can guarentee that
    * all texture units can bind a maximal texture and have all of them in
    * texturable memory at once. Depending on the allow_large_textures driconf
    * setting allow larger textures.
    */

   ctx = rmesa->radeon.glCtx;
   ctx->Const.MaxTextureUnits = driQueryOptioni (&rmesa->radeon.optionCache,
						 "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   i = driQueryOptioni( &rmesa->radeon.optionCache, "allow_large_textures");

   driCalculateMaxTextureLevels( rmesa->radeon.texture_heaps,
				 rmesa->radeon.nr_heaps,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
				 8,  /* 256^3 */
				 9,  /* \todo: max cube texture size seems to be 512x512(x6) */
				 11, /* max rect texture size is 2048x2048. */
				 12,
				 GL_FALSE,
				 i );


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

   rmesa->boxes = 0;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _ae_create_context( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, radeon_pipeline );

   /* Try and keep materials and vertices separate:
    */
/*    _tnl_isolate_materials( ctx, GL_TRUE ); */

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );


   for ( i = 0 ; i < RADEON_MAX_TEXTURE_UNITS ; i++ ) {
      _math_matrix_ctr( &rmesa->TexGenMatrix[i] );
      _math_matrix_ctr( &rmesa->tmpmat[i] );
      _math_matrix_set_identity( &rmesa->TexGenMatrix[i] );
      _math_matrix_set_identity( &rmesa->tmpmat[i] );
   }

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   if (rmesa->radeon.radeonScreen->drmSupportsCubeMapsR100)
      _mesa_enable_extension( ctx, "GL_ARB_texture_cube_map" );
   if (rmesa->radeon.glCtx->Mesa_DXTn) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
      _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&rmesa->radeon.optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

   if (rmesa->radeon.dri.drmMinor >= 9)
      _mesa_enable_extension( ctx, "GL_NV_texture_rectangle");

   /* XXX these should really go right after _mesa_init_driver_functions() */
   radeonInitIoctlFuncs( ctx );
   radeonInitStateFuncs( ctx );
   radeonInitSpanFuncs( ctx );
   radeonInitState( rmesa );
   radeonInitSwtcl( ctx );

   _mesa_vector4f_alloc( &rmesa->tcl.ObjClean, 0, 
			 ctx->Const.MaxArrayLockSize, 32 );

   fthrottle_mode = driQueryOptioni(&rmesa->radeon.optionCache, "fthrottle_mode");
   rmesa->radeon.iw.irq_seq = -1;
   rmesa->radeon.irqsEmitted = 0;
   rmesa->radeon.do_irqs = (rmesa->radeon.radeonScreen->irq != 0 &&
			    fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS);

   rmesa->radeon.do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

   (*sPriv->systemTime->getUST)( & rmesa->radeon.swap_ust );


#if DO_DEBUG
   RADEON_DEBUG = driParseDebugString( getenv( "RADEON_DEBUG" ),
				       debug_control );
#endif

   tcl_mode = driQueryOptioni(&rmesa->radeon.optionCache, "tcl_mode");
   if (driQueryOptionb(&rmesa->radeon.optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, RADEON_FALLBACK_DISABLE, 1);
   } else if (tcl_mode == DRI_CONF_TCL_SW ||
	      !(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
      if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 rmesa->radeon.radeonScreen->chip_flags &= ~RADEON_CHIPSET_TCL;
	 fprintf(stderr, "Disabling HW TCL support\n");
      }
      TCL_FALLBACK(rmesa->radeon.glCtx, RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
   }

   if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
/*       _tnl_need_dlist_norm_lengths( ctx, GL_FALSE ); */
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
   r100ContextPtr rmesa = (r100ContextPtr) driContextPriv->driverPrivate;
   r100ContextPtr current = ctx ? R100_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (rmesa == current) {
      RADEON_FIREVERTICES( rmesa );
      _mesa_make_current(NULL, NULL, NULL);
   }

   /* Free radeon context resources */
   assert(rmesa); /* should never be null */
   if ( rmesa ) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (rmesa->radeon.glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( rmesa->radeon.glCtx );
      _tnl_DestroyContext( rmesa->radeon.glCtx );
      _vbo_DestroyContext( rmesa->radeon.glCtx );
      _swrast_DestroyContext( rmesa->radeon.glCtx );

      radeonDestroySwtcl( rmesa->radeon.glCtx );
      radeonReleaseArrays( rmesa->radeon.glCtx, ~0 );
      if (rmesa->dma.current.buf) {
	 radeonReleaseDmaRegion( rmesa, &rmesa->dma.current, __FUNCTION__ );
	 radeonFlushCmdBuf( rmesa, __FUNCTION__ );
      }

      _mesa_vector4f_free( &rmesa->tcl.ObjClean );

      if (rmesa->radeon.state.scissor.pClipRects) {
	 FREE(rmesa->radeon.state.scissor.pClipRects);
	 rmesa->radeon.state.scissor.pClipRects = NULL;
      }

      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = 0 ; i < rmesa->radeon.nr_heaps ; i++ ) {
	    driDestroyTextureHeap( rmesa->radeon.texture_heaps[ i ] );
	    rmesa->radeon.texture_heaps[ i ] = NULL;
         }

	 assert( is_empty_list( & rmesa->radeon.swapped ) );
      }

      /* free the Mesa context */
      rmesa->radeon.glCtx->DriverCtx = NULL;
      _mesa_destroy_context( rmesa->radeon.glCtx );

      /* free the option cache */
      driDestroyOptionCache (&rmesa->radeon.optionCache);

      FREE( rmesa );
   }
}

/* Make context `c' the current context and bind it to the given
 * drawing and reading surfaces.
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
	 fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *) newCtx->glCtx);

      newCtx->dri.readable = driReadPriv;

      if ( (newCtx->dri.drawable != driDrawPriv) ||
           newCtx->lastStamp != driDrawPriv->lastStamp ) {
	 if (driDrawPriv->swap_interval == (unsigned)-1) {
	    driDrawPriv->vblFlags = (newCtx->radeonScreen->irq != 0)
	       ? driGetDefaultVBlankFlags(&newCtx->optionCache)
	       : VBLANK_FLAG_NO_IRQ;

	    driDrawableInitVBlank( driDrawPriv );
	 }

	 newCtx->dri.drawable = driDrawPriv;

	 radeonSetCliprects(newCtx);
	 radeonUpdateViewportOffset( newCtx->glCtx );
      }

      _mesa_make_current( newCtx->glCtx,
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate );

      _mesa_update_state( newCtx->glCtx );
   } else {
      if (RADEON_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
      _mesa_make_current( NULL, NULL, NULL );
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
   r100ContextPtr rmesa = (r100ContextPtr) driContextPriv->driverPrivate;

   if (RADEON_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *) rmesa->radeon.glCtx);

   return GL_TRUE;
}
