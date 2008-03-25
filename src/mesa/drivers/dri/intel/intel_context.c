/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "extensions.h"
#include "framebuffer.h"
#include "imports.h"
#include "points.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"

#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#include "drivers/common/driverfuncs.h"

#include "intel_screen.h"

#include "i830_dri.h"

#include "intel_chipset.h"
#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_pixel.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"
#include "intel_fbo.h"
#include "intel_decode.h"
#include "intel_bufmgr_ttm.h"

#include "drirenderbuffer.h"
#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"            /* for symbolic values of enum-type options */
#ifndef INTEL_DEBUG
int INTEL_DEBUG = (0);
#endif

#define need_GL_NV_point_sprite
#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
#define need_GL_ARB_occlusion_query
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_fog_coord
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#define need_GL_NV_vertex_program
#define need_GL_ATI_separate_stencil
#define need_GL_EXT_point_parameters
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1
#define need_GL_ARB_shader_objects
#define need_GL_ARB_vertex_shader

#include "extension_helper.h"

#define DRIVER_DATE                     "20061102"

static const GLubyte *
intelGetString(GLcontext * ctx, GLenum name)
{
   const char *chipset;
   static char buffer[128];

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) "Tungsten Graphics, Inc";
      break;

   case GL_RENDERER:
      switch (intel_context(ctx)->intelScreen->deviceID) {
      case PCI_CHIP_845_G:
         chipset = "Intel(R) 845G";
         break;
      case PCI_CHIP_I830_M:
         chipset = "Intel(R) 830M";
         break;
      case PCI_CHIP_I855_GM:
         chipset = "Intel(R) 852GM/855GM";
         break;
      case PCI_CHIP_I865_G:
         chipset = "Intel(R) 865G";
         break;
      case PCI_CHIP_I915_G:
         chipset = "Intel(R) 915G";
         break;
      case PCI_CHIP_E7221_G:
	 chipset = "Intel (R) E7221G (i915)";
	 break;
      case PCI_CHIP_I915_GM:
         chipset = "Intel(R) 915GM";
         break;
      case PCI_CHIP_I945_G:
         chipset = "Intel(R) 945G";
         break;
      case PCI_CHIP_I945_GM:
         chipset = "Intel(R) 945GM";
         break;
      case PCI_CHIP_I945_GME:
         chipset = "Intel(R) 945GME";
         break;
      case PCI_CHIP_G33_G:
	 chipset = "Intel(R) G33";
	 break;
      case PCI_CHIP_Q35_G:
	 chipset = "Intel(R) Q35";
	 break;
      case PCI_CHIP_Q33_G:
	 chipset = "Intel(R) Q33";
	 break;
      case PCI_CHIP_I965_Q:
	 chipset = "Intel(R) 965Q";
	 break;
      case PCI_CHIP_I965_G:
      case PCI_CHIP_I965_G_1:
	 chipset = "Intel(R) 965G";
	 break;
      case PCI_CHIP_I946_GZ:
	 chipset = "Intel(R) 946GZ";
	 break;
      case PCI_CHIP_I965_GM:
	 chipset = "Intel(R) 965GM";
	 break;
      case PCI_CHIP_I965_GME:
	 chipset = "Intel(R) 965GME/GLE";
	 break;
      case PCI_CHIP_IGD_GM:
	 chipset = "Intel(R) Integrated Graphics Device";
	 break;
      default:
         chipset = "Unknown Intel Chipset";
         break;
      }

      (void) driGetRendererString(buffer, chipset, DRIVER_DATE, 0);
      return (GLubyte *) buffer;

   default:
      return NULL;
   }
}

/**
 * Extension strings exported by the intel driver.
 *
 * \note
 * It appears that ARB_texture_env_crossbar has "disappeared" compared to the
 * old i830-specific driver.
 */
static const struct dri_extension card_extensions[] = {
   {"GL_ARB_multisample", GL_ARB_multisample_functions},
   {"GL_ARB_multitexture", NULL},
   {"GL_ARB_point_parameters", GL_ARB_point_parameters_functions},
   {"GL_NV_point_sprite", GL_NV_point_sprite_functions},
   {"GL_ARB_texture_border_clamp", NULL},
   {"GL_ARB_texture_compression", GL_ARB_texture_compression_functions},
   {"GL_ARB_texture_cube_map", NULL},
   {"GL_ARB_texture_env_add", NULL},
   {"GL_ARB_texture_env_combine", NULL},
   {"GL_ARB_texture_env_dot3", NULL},
   {"GL_ARB_texture_mirrored_repeat", NULL},
   {"GL_ARB_texture_non_power_of_two",   NULL },
   {"GL_ARB_texture_rectangle", NULL},
   {"GL_NV_texture_rectangle", NULL},
   {"GL_EXT_texture_rectangle", NULL},
   {"GL_ARB_point_parameters", NULL}, 
   {"GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions},
   {"GL_ARB_vertex_program", GL_ARB_vertex_program_functions},
   {"GL_ARB_window_pos", GL_ARB_window_pos_functions},
   {"GL_EXT_blend_color", GL_EXT_blend_color_functions},
   {"GL_EXT_blend_equation_separate",
    GL_EXT_blend_equation_separate_functions},
   {"GL_EXT_blend_func_separate", GL_EXT_blend_func_separate_functions},
   {"GL_EXT_blend_minmax", GL_EXT_blend_minmax_functions},
   {"GL_EXT_blend_logic_op", NULL},
   {"GL_EXT_blend_subtract", NULL},
   {"GL_EXT_cull_vertex", GL_EXT_cull_vertex_functions},
   {"GL_EXT_fog_coord", GL_EXT_fog_coord_functions},
   {"GL_EXT_multi_draw_arrays", GL_EXT_multi_draw_arrays_functions},
   {"GL_ATI_separate_stencil", GL_ATI_separate_stencil_functions},
#if 1                           /* XXX FBO temporary? */
   {"GL_EXT_packed_depth_stencil", NULL},
#endif
   {"GL_EXT_secondary_color", GL_EXT_secondary_color_functions},
   {"GL_EXT_stencil_wrap", NULL},
   {"GL_EXT_texture_edge_clamp", NULL},
   {"GL_EXT_texture_env_combine", NULL},
   {"GL_EXT_texture_env_dot3", NULL},
   {"GL_EXT_texture_filter_anisotropic", NULL},
   {"GL_EXT_texture_lod_bias", NULL},
   {"GL_3DFX_texture_compression_FXT1", NULL},
   {"GL_APPLE_client_storage", NULL},
   {"GL_MESA_pack_invert", NULL},
   {"GL_MESA_ycbcr_texture", NULL},
   {"GL_NV_blend_square", NULL},
   {"GL_NV_vertex_program", GL_NV_vertex_program_functions},
   {"GL_NV_vertex_program1_1", NULL},
   { "GL_SGIS_generate_mipmap", NULL },
   {NULL, NULL}
};

static const struct dri_extension brw_extensions[] = {
   { "GL_ARB_shading_language_100",       GL_VERSION_2_0_functions},
   { "GL_ARB_shading_language_120",       GL_VERSION_2_1_functions},
   { "GL_ARB_shader_objects",             GL_ARB_shader_objects_functions},
   { "GL_ARB_vertex_shader",              GL_ARB_vertex_shader_functions},
   { "GL_ARB_point_sprite", 		  NULL},
   { "GL_ARB_fragment_shader",            NULL },
   { "GL_ARB_draw_buffers",               NULL },
   { "GL_ARB_depth_texture",              NULL },
   { "GL_ARB_fragment_program",           NULL },
   { "GL_ARB_shadow",                     NULL },
   { "GL_EXT_shadow_funcs",               NULL },
   /* ARB extn won't work if not enabled */
   { "GL_SGIX_depth_texture",             NULL },
   { "GL_ARB_texture_env_crossbar",       NULL },
   { "GL_EXT_texture_sRGB",		  NULL},
   { NULL,                                NULL }
};

static const struct dri_extension arb_oc_extensions[] = {
   {"GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions},
   {NULL, NULL}
};

static const struct dri_extension ttm_extensions[] = {
   {"GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions},
   {"GL_ARB_pixel_buffer_object", NULL},
   {NULL, NULL}
};

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void intelInitExtensions(GLcontext *ctx, GLboolean enable_imaging)
{
   struct intel_context *intel = ctx?intel_context(ctx):NULL;

   /* Disable imaging extension until convolution is working in teximage paths.
    */
   enable_imaging = GL_FALSE;

   driInitExtensions(ctx, card_extensions, enable_imaging);

   if (intel == NULL || intel->ttm)
      driInitExtensions(ctx, ttm_extensions, GL_FALSE);

   if (intel == NULL || 
       (IS_965(intel->intelScreen->deviceID) && 
	intel->intelScreen->drmMinor >= 8))
      driInitExtensions(ctx, arb_oc_extensions, GL_FALSE);

   if (intel == NULL || IS_965(intel->intelScreen->deviceID))
      driInitExtensions(ctx, brw_extensions, GL_FALSE);
}

static const struct dri_debug_control debug_control[] = {
   { "tex",   DEBUG_TEXTURE},
   { "state", DEBUG_STATE},
   { "ioctl", DEBUG_IOCTL},
   { "blit",  DEBUG_BLIT},
   { "mip",   DEBUG_MIPTREE},
   { "fall",  DEBUG_FALLBACKS},
   { "verb",  DEBUG_VERBOSE},
   { "bat",   DEBUG_BATCH},
   { "pix",   DEBUG_PIXEL},
   { "buf",   DEBUG_BUFMGR},
   { "reg",   DEBUG_REGION},
   { "fbo",   DEBUG_FBO},
   { "lock",  DEBUG_LOCK},
   { "sync",  DEBUG_SYNC},
   { "prim",  DEBUG_PRIMS },
   { "vert",  DEBUG_VERTS },
   { "dri",   DEBUG_DRI },
   { "dma",   DEBUG_DMA },
   { "san",   DEBUG_SANITY },
   { "sleep", DEBUG_SLEEP },
   { "stats", DEBUG_STATS },
   { "tile",  DEBUG_TILE },
   { "sing",  DEBUG_SINGLE_THREAD },
   { "thre",  DEBUG_SINGLE_THREAD },
   { "wm",    DEBUG_WM },
   { "urb",   DEBUG_URB },
   { "vs",    DEBUG_VS },
   { NULL,    0 }
};


static void
intelInvalidateState(GLcontext * ctx, GLuint new_state)
{
    struct intel_context *intel = intel_context(ctx);

   _swrast_InvalidateState(ctx, new_state);
   _swsetup_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);
   _tnl_InvalidateState(ctx, new_state);
   _tnl_invalidate_vertex_state(ctx, new_state);

   intel->NewGLState |= new_state;

   if (intel->vtbl.invalidate_state)
      intel->vtbl.invalidate_state( intel, new_state );
}


void
intelFlush(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->Fallback)
      _swrast_flush(ctx);

   if (!IS_965(intel->intelScreen->deviceID))
      INTEL_FIREVERTICES(intel);

   if (intel->batch->map != intel->batch->ptr)
      intel_batchbuffer_flush(intel->batch);

   /* XXX: Need to do an MI_FLUSH here.
    */
}

void
intelFinish(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   intelFlush(ctx);
   if (intel->batch->last_fence) {
      dri_fence_wait(intel->batch->last_fence);
      dri_fence_unreference(intel->batch->last_fence);
      intel->batch->last_fence = NULL;
   }
}

static void
intelBeginQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	struct intel_context *intel = intel_context( ctx );
	struct drm_i915_mmio io = {
		.read_write = I915_MMIO_READ,
		.reg = MMIO_REGS_PS_DEPTH_COUNT,
		.data = &q->Result 
	};
	intel->stats_wm++;
	intelFinish(&intel->ctx);
	drmCommandWrite(intel->driFd, DRM_I915_MMIO, &io, sizeof(io));
}

static void
intelEndQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	struct intel_context *intel = intel_context( ctx );
	GLuint64EXT tmp;	
	struct drm_i915_mmio io = {
		.read_write = I915_MMIO_READ,
		.reg = MMIO_REGS_PS_DEPTH_COUNT,
		.data = &tmp
	};
	intelFinish(&intel->ctx);
	drmCommandWrite(intel->driFd, DRM_I915_MMIO, &io, sizeof(io));
	q->Result = tmp - q->Result;
	q->Ready = GL_TRUE;
	intel->stats_wm--;
}

/** Driver-specific fence emit implementation for the fake memory manager. */
static unsigned int
intel_fence_emit(void *private)
{
   struct intel_context *intel = (struct intel_context *)private;
   unsigned int fence;

   /* XXX: Need to emit a flush, if we haven't already (at least with the
    * current batchbuffer implementation, we have).
    */

   fence = intelEmitIrqLocked(intel);

   return fence;
}

/** Driver-specific fence wait implementation for the fake memory manager. */
static int
intel_fence_wait(void *private, unsigned int cookie)
{
   struct intel_context *intel = (struct intel_context *)private;

   intelWaitIrq(intel, cookie);

   return 0;
}

static GLboolean
intel_init_bufmgr(struct intel_context *intel)
{
   intelScreenPrivate *intelScreen = intel->intelScreen;
   GLboolean ttm_disable = getenv("INTEL_NO_TTM") != NULL;
   GLboolean ttm_supported;

   /* If we've got a new enough DDX that's initializing TTM and giving us
    * object handles for the shared buffers, use that.
    */
   intel->ttm = GL_FALSE;
   if (intel->intelScreen->driScrnPriv->dri2.enabled)
       ttm_supported = GL_TRUE;
   else if (intel->intelScreen->driScrnPriv->ddx_version.minor >= 9 &&
	    intel->intelScreen->drmMinor >= 11 &&
	    intel->intelScreen->front.bo_handle != -1)
       ttm_supported = GL_TRUE;
   else
       ttm_supported = GL_FALSE;

   if (!ttm_disable && ttm_supported) {
      int bo_reuse_mode;
      intel->bufmgr = intel_bufmgr_ttm_init(intel->driFd,
					    DRM_FENCE_TYPE_EXE,
					    DRM_FENCE_TYPE_EXE |
					    DRM_I915_FENCE_TYPE_RW,
					    BATCH_SZ);
      if (intel->bufmgr != NULL)
	 intel->ttm = GL_TRUE;

      bo_reuse_mode = driQueryOptioni(&intel->optionCache, "bo_reuse");
      switch (bo_reuse_mode) {
      case DRI_CONF_BO_REUSE_DISABLED:
	 break;
      case DRI_CONF_BO_REUSE_ALL:
	 intel_ttm_enable_bo_reuse(intel->bufmgr);
	 break;
      }
   }
   /* Otherwise, use the classic buffer manager. */
   if (intel->bufmgr == NULL) {
      if (ttm_disable) {
	 fprintf(stderr, "TTM buffer manager disabled.  Using classic.\n");
      } else {
	 fprintf(stderr, "Failed to initialize TTM buffer manager.  "
		 "Falling back to classic.\n");
      }

      if (intelScreen->tex.size == 0) {
	 fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
		 __func__, __LINE__);
	 return GL_FALSE;
      }

      intel->bufmgr = dri_bufmgr_fake_init(intelScreen->tex.offset,
					   intelScreen->tex.map,
					   intelScreen->tex.size,
					   intel_fence_emit,
					   intel_fence_wait,
					   intel);
   }

   return GL_TRUE;
}

void
intelInitDriverFunctions(struct dd_function_table *functions)
{
   _mesa_init_driver_functions(functions);

   functions->Flush = intelFlush;
   functions->Finish = intelFinish;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;

   functions->CopyColorTable = _swrast_CopyColorTable;
   functions->CopyColorSubTable = _swrast_CopyColorSubTable;
   functions->CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   functions->CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

   functions->BeginQuery = intelBeginQuery;
   functions->EndQuery = intelEndQuery;

   intelInitTextureFuncs(functions);
   intelInitStateFuncs(functions);
   intelInitBufferFuncs(functions);
}


GLboolean
intelInitContext(struct intel_context *intel,
                 const __GLcontextModes * mesaVis,
                 __DRIcontextPrivate * driContextPriv,
                 void *sharedContextPrivate,
                 struct dd_function_table *functions)
{
   GLcontext *ctx = &intel->ctx;
   GLcontext *shareCtx = (GLcontext *) sharedContextPrivate;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;
   volatile struct drm_i915_sarea *saPriv = (struct drm_i915_sarea *)
      (((GLubyte *) sPriv->pSAREA) + intelScreen->sarea_priv_offset);
   int fthrottle_mode;

   if (!_mesa_initialize_context(&intel->ctx, mesaVis, shareCtx,
                                 functions, (void *) intel)) {
      _mesa_printf("%s: failed to init mesa context\n", __FUNCTION__);
      return GL_FALSE;
   }

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driScreen = sPriv;
   intel->sarea = saPriv;

   /* Dri stuff */
   intel->hHWContext = driContextPriv->hHWContext;
   intel->driFd = sPriv->fd;
   intel->driHwLock = sPriv->lock;

   intel->width = intelScreen->width;
   intel->height = intelScreen->height;

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       intel->driScreen->myNum,
		       IS_965(intelScreen->deviceID) ? "i965" : "i915");
   if (intelScreen->deviceID == PCI_CHIP_I865_G)
      intel->maxBatchSize = 4096;
   else
      intel->maxBatchSize = BATCH_SZ;

   if (!intel_init_bufmgr(intel))
      return GL_FALSE;

   ctx->Const.MaxTextureMaxAnisotropy = 2.0;

   /* This doesn't yet catch all non-conformant rendering, but it's a
    * start.
    */
   if (getenv("INTEL_STRICT_CONFORMANCE")) {
      intel->strict_conformance = 1;
   }

   if (intel->strict_conformance) {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 1.0;
      ctx->Const.MaxLineWidthAA = 1.0;
      ctx->Const.LineWidthGranularity = 1.0;
   }
   else {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 5.0;
      ctx->Const.MaxLineWidthAA = 5.0;
      ctx->Const.LineWidthGranularity = 0.5;
   }

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   /* reinitialize the context point state.
    * It depend on constants in __GLcontextRec::Const
    */
   _mesa_init_point(ctx);

   ctx->Const.MaxColorAttachments = 4;  /* XXX FBO: review this */

   /* Initialize the software rasterizer and helper modules. */
   _swrast_CreateContext(ctx);
   _vbo_CreateContext(ctx);
   _tnl_CreateContext(ctx);
   _swsetup_CreateContext(ctx);
 
   /* Configure swrast to match hardware characteristics: */
   _swrast_allow_pixel_fog(ctx, GL_FALSE);
   _swrast_allow_vertex_fog(ctx, GL_TRUE);

   intel->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   intel->hw_stipple = 1;

   /* XXX FBO: this doesn't seem to be used anywhere */
   switch (mesaVis->depthBits) {
   case 0:                     /* what to do in this case? */
   case 16:
      intel->polygon_offset_scale = 1.0;
      break;
   case 24:
      intel->polygon_offset_scale = 2.0;     /* req'd to pass glean */
      break;
   default:
      assert(0);
      break;
   }

   if (IS_965(intelScreen->deviceID))
      intel->polygon_offset_scale /= 0xffff;

   intel->RenderIndex = ~0;

   fthrottle_mode = driQueryOptioni(&intel->optionCache, "fthrottle_mode");
   intel->irqsEmitted = 0;

   intel->do_irqs = (intel->intelScreen->irq_active &&
                     fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS);

   intel->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

   _math_matrix_ctr(&intel->ViewportMatrix);

   if (IS_965(intelScreen->deviceID) && !intel->intelScreen->irq_active) {
      _mesa_printf("IRQs not active.  Exiting\n");
      exit(1);
   }

   intelInitExtensions(ctx, GL_FALSE);

   INTEL_DEBUG = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      dri_bufmgr_set_debug(intel->bufmgr, GL_TRUE);

   if (!sPriv->dri2.enabled)
      intel_recreate_static_regions(intel);

   intel->batch = intel_batchbuffer_alloc(intel);
   intel->last_swap_fence = NULL;
   intel->first_swap_fence = NULL;

   intel_bufferobj_init(intel);
   intel_fbo_init(intel);

   if (intel->ctx.Mesa_DXTn) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
      _mesa_enable_extension(ctx, "GL_S3_s3tc");
   }
   else if (driQueryOptionb(&intel->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
   }

   intel->prim.primitive = ~0;

   /* Force all software fallbacks */
   if (driQueryOptionb(&intel->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D rasterization\n");
      FALLBACK(intel, INTEL_FALLBACK_USER, 1);
      intel->no_rast = 1;
   }

   /* Disable all hardware rendering (skip emitting batches and fences/waits
    * to the kernel)
    */
   intel->no_hw = getenv("INTEL_NO_HW") != NULL;

   return GL_TRUE;
}

void
intelDestroyContext(__DRIcontextPrivate * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;

   assert(intel);               /* should never be null */
   if (intel) {
      GLboolean release_texture_heaps;

      INTEL_FIREVERTICES(intel);

      intel->vtbl.destroy(intel);

      release_texture_heaps = (intel->ctx.Shared->RefCount == 1);
      _swsetup_DestroyContext(&intel->ctx);
      _tnl_DestroyContext(&intel->ctx);
      _vbo_DestroyContext(&intel->ctx);

      _swrast_DestroyContext(&intel->ctx);
      intel->Fallback = 0;      /* don't call _swrast_Flush later */

      intel_batchbuffer_free(intel->batch);

      if (intel->last_swap_fence) {
	 dri_fence_wait(intel->last_swap_fence);
	 dri_fence_unreference(intel->last_swap_fence);
	 intel->last_swap_fence = NULL;
      }
      if (intel->first_swap_fence) {
	 dri_fence_wait(intel->first_swap_fence);
	 dri_fence_unreference(intel->first_swap_fence);
	 intel->first_swap_fence = NULL;
      }

      if (release_texture_heaps) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         if (INTEL_DEBUG & DEBUG_TEXTURE)
            fprintf(stderr, "do something to free texture heaps\n");
      }

      /* free the Mesa context */
      _mesa_free_context_data(&intel->ctx);

      dri_bufmgr_destroy(intel->bufmgr);
   }
}

GLboolean
intelUnbindContext(__DRIcontextPrivate * driContextPriv)
{
   return GL_TRUE;
}

GLboolean
intelMakeCurrent(__DRIcontextPrivate * driContextPriv,
                 __DRIdrawablePrivate * driDrawPriv,
                 __DRIdrawablePrivate * driReadPriv)
{
   __DRIscreenPrivate *psp = driDrawPriv->driScreenPriv;

   if (driContextPriv) {
      struct intel_context *intel =
         (struct intel_context *) driContextPriv->driverPrivate;
      struct intel_framebuffer *intel_fb =
	 (struct intel_framebuffer *) driDrawPriv->driverPrivate;
      GLframebuffer *readFb = (GLframebuffer *) driReadPriv->driverPrivate;


      /* XXX FBO temporary fix-ups! */
      /* if the renderbuffers don't have regions, init them from the context */
      if (!driContextPriv->driScreenPriv->dri2.enabled) {
         struct intel_renderbuffer *irbDepth
            = intel_get_renderbuffer(&intel_fb->Base, BUFFER_DEPTH);
         struct intel_renderbuffer *irbStencil
            = intel_get_renderbuffer(&intel_fb->Base, BUFFER_STENCIL);

         if (intel_fb->color_rb[0]) {
	    intel_renderbuffer_set_region(intel_fb->color_rb[0],
					  intel->front_region);
         }
         if (intel_fb->color_rb[1]) {
	    intel_renderbuffer_set_region(intel_fb->color_rb[1],
					  intel->back_region);
         }
#if 0
         if (intel_fb->color_rb[2]) {
	    intel_renderbuffer_set_region(intel_fb->color_rb[2],
					  intel->third_region);
         }
#endif
         if (irbDepth) {
	    intel_renderbuffer_set_region(irbDepth, intel->depth_region);
         }
         if (irbStencil) {
	    intel_renderbuffer_set_region(irbStencil, intel->depth_region);
         }
      }

      /* set GLframebuffer size to match window, if needed */
      driUpdateFramebufferSize(&intel->ctx, driDrawPriv);

      if (driReadPriv != driDrawPriv) {
	 driUpdateFramebufferSize(&intel->ctx, driReadPriv);
      }

      _mesa_make_current(&intel->ctx, &intel_fb->Base, readFb);

      /* The drawbuffer won't always be updated by _mesa_make_current: 
       */
      if (intel->ctx.DrawBuffer == &intel_fb->Base) {

	 if (intel->driReadDrawable != driReadPriv)
	    intel->driReadDrawable = driReadPriv;

	 if (intel->driDrawable != driDrawPriv) {
	    if (driDrawPriv->swap_interval == (unsigned)-1) {
	       int i;

	       driDrawPriv->vblFlags = (intel->intelScreen->irq_active != 0)
		  ? driGetDefaultVBlankFlags(&intel->optionCache)
		 : VBLANK_FLAG_NO_IRQ;

	       (*psp->systemTime->getUST) (&intel_fb->swap_ust);
	       driDrawableInitVBlank(driDrawPriv);
	       intel_fb->vbl_waited = driDrawPriv->vblSeq;

	       for (i = 0; i < (intel->intelScreen->third.handle ? 3 : 2); i++) {
		  if (intel_fb->color_rb[i])
		     intel_fb->color_rb[i]->vbl_pending = driDrawPriv->vblSeq;
	       }
	    }
	    intel->driDrawable = driDrawPriv;
	    intelWindowMoved(intel);
	 }

	 intel_draw_buffer(&intel->ctx, &intel_fb->Base);
      }
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}

static void
intelContendedLock(struct intel_context *intel, GLuint flags)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   __DRIscreenPrivate *sPriv = intel->driScreen;
   volatile struct drm_i915_sarea *sarea = intel->sarea;
   int drawable_changed = 0;
   int me = intel->hHWContext;

   drmGetLock(intel->driFd, intel->hHWContext, flags);
   intel->locked = 1;

   if (INTEL_DEBUG & DEBUG_LOCK)
      _mesa_printf("%s - got contended lock\n", __progname);

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   if (dPriv) {
      if (sPriv->dri2.enabled)
	drawable_changed = __driParseEvents(dPriv->driContextPriv, dPriv);
      else
	 DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);
   }

   if (sarea && sarea->ctxOwner != me) {
      if (INTEL_DEBUG & DEBUG_BUFMGR) {
	 fprintf(stderr, "Lost Context: sarea->ctxOwner %x me %x\n",
		 sarea->ctxOwner, me);
      }
      sarea->ctxOwner = me;
   }

   /* If the last consumer of the texture memory wasn't us, notify the fake
    * bufmgr and record the new owner.  We should have the memory shared
    * between contexts of a single fake bufmgr, but this will at least make
    * things correct for now.
    */
   if (!intel->ttm && sarea->texAge != intel->hHWContext) {
      sarea->texAge = intel->hHWContext;
      dri_bufmgr_fake_contended_lock_take(intel->bufmgr);
      if (INTEL_DEBUG & DEBUG_BATCH)
	 intel_decode_context_reset();
      if (INTEL_DEBUG & DEBUG_BUFMGR)
	 fprintf(stderr, "Lost Textures: sarea->texAge %x hw context %x\n",
		 sarea->ctxOwner, intel->hHWContext);
   }

   if (!sPriv->dri2.enabled) {
      if (sarea->width != intel->width || sarea->height != intel->height) {
	 int numClipRects = intel->numClipRects;

	 /*
	  * FIXME: Really only need to do this when drawing to a
	  * common back- or front buffer.
	  */

	 /*
	  * This will essentially drop the outstanding batchbuffer on
	  * the floor.
	  */
	 intel->numClipRects = 0;

	 if (intel->Fallback)
	    _swrast_flush(&intel->ctx);

	 if (!IS_965(intel->intelScreen->deviceID))
	    INTEL_FIREVERTICES(intel);

	 if (intel->batch->map != intel->batch->ptr)
	    intel_batchbuffer_flush(intel->batch);

	 intel->numClipRects = numClipRects;

	 /* force window update */
	 intel->lastStamp = 0;

	 intel->width = sarea->width;
	 intel->height = sarea->height;
      }

      /* Drawable changed?
       */
      if (dPriv && intel->lastStamp != dPriv->lastStamp) {
	 intelWindowMoved(intel);
	 intel->lastStamp = dPriv->lastStamp;
      }
   } else if (drawable_changed) {
     intelWindowMoved(intel);                                                 
     intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
   }
}


_glthread_DECLARE_STATIC_MUTEX(lockMutex);

/* Lock the hardware and validate our state.  
 */
void LOCK_HARDWARE( struct intel_context *intel )
{
    __DRIdrawablePrivate *dPriv = intel->driDrawable;
    char __ret = 0;
    struct intel_framebuffer *intel_fb = NULL;
    struct intel_renderbuffer *intel_rb = NULL;

    _glthread_LOCK_MUTEX(lockMutex);
    assert(!intel->locked);

    if (intel->driDrawable) {
       intel_fb = intel->driDrawable->driverPrivate;

       if (intel_fb)
	  intel_rb =
	     intel_get_renderbuffer(&intel_fb->Base,
				    intel_fb->Base._ColorDrawBufferIndexes[0]);
    }

    if (intel_rb && dPriv->vblFlags &&
	!(dPriv->vblFlags & VBLANK_FLAG_NO_IRQ) &&
	(intel_fb->vbl_waited - intel_rb->vbl_pending) > (1<<23)) {
	drmVBlank vbl;

	vbl.request.type = DRM_VBLANK_ABSOLUTE;

	if ( dPriv->vblFlags & VBLANK_FLAG_SECONDARY ) {
	    vbl.request.type |= DRM_VBLANK_SECONDARY;
	}

	vbl.request.sequence = intel_rb->vbl_pending;
	drmWaitVBlank(intel->driFd, &vbl);
	intel_fb->vbl_waited = vbl.reply.sequence;
    }

    DRM_CAS(intel->driHwLock, intel->hHWContext,
        (DRM_LOCK_HELD|intel->hHWContext), __ret);

    if (__ret)
        intelContendedLock( intel, 0 );

    intel->locked = 1;
    if (INTEL_DEBUG & DEBUG_LOCK)
      _mesa_printf("%s - locked\n", __progname);
}


/* Unlock the hardware using the global current context 
 */
void UNLOCK_HARDWARE( struct intel_context *intel )
{
   intel->vtbl.note_unlock( intel );
   intel->locked = 0;

   DRM_UNLOCK(intel->driFd, intel->driHwLock, intel->hHWContext);

   _glthread_UNLOCK_MUTEX(lockMutex);

   if (INTEL_DEBUG & DEBUG_LOCK)
      _mesa_printf("%s - unlocked\n", __progname);

   /**
    * Nothing should be left in batch outside of LOCK/UNLOCK which references
    * cliprects.
    */
   assert(intel->batch->cliprect_mode != REFERENCES_CLIPRECTS);
}

