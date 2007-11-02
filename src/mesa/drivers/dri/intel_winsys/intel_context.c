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
#include "extensions.h"
#include "framebuffer.h"

#include "i830_dri.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_swapbuffers.h"
#include "intel_winsys.h"
#include "intel_batchbuffer.h"

#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_context.h"

/*#include "drirenderbuffer.h"*/
#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"            /* for symbolic values of enum-type options */




#ifdef DEBUG
int __intel_debug = 0;
#endif

#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
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
#include "extension_helper.h"


#define DRIVER_DATE                     "20070731"



/**
 * Extension strings exported by the intel driver.
 *
 * \note
 * It appears that ARB_texture_env_crossbar has "disappeared" compared to the
 * old i830-specific driver.
 */
const struct dri_extension card_extensions[] = {
   {"GL_ARB_multisample", GL_ARB_multisample_functions},
   {"GL_ARB_multitexture", NULL},
   {"GL_ARB_point_parameters", GL_ARB_point_parameters_functions},
   {"GL_ARB_texture_border_clamp", NULL},
   {"GL_ARB_texture_compression", GL_ARB_texture_compression_functions},
   {"GL_ARB_texture_cube_map", NULL},
   {"GL_ARB_texture_env_add", NULL},
   {"GL_ARB_texture_env_combine", NULL},
   {"GL_ARB_texture_env_dot3", NULL},
   {"GL_ARB_texture_mirrored_repeat", NULL},
   {"GL_ARB_texture_rectangle", NULL},
   {"GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions},
   {"GL_ARB_pixel_buffer_object", NULL},
   {"GL_ARB_vertex_program", GL_ARB_vertex_program_functions},
   {"GL_ARB_window_pos", GL_ARB_window_pos_functions},
   {"GL_EXT_blend_color", GL_EXT_blend_color_functions},
   {"GL_EXT_blend_equation_separate",
    GL_EXT_blend_equation_separate_functions},
   {"GL_EXT_blend_func_separate", GL_EXT_blend_func_separate_functions},
   {"GL_EXT_blend_minmax", GL_EXT_blend_minmax_functions},
   {"GL_EXT_blend_subtract", NULL},
   {"GL_EXT_cull_vertex", GL_EXT_cull_vertex_functions},
   {"GL_EXT_fog_coord", GL_EXT_fog_coord_functions},
   {"GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions},
   {"GL_EXT_multi_draw_arrays", GL_EXT_multi_draw_arrays_functions},
#if 1                           /* XXX FBO temporary? */
   {"GL_EXT_packed_depth_stencil", NULL},
#endif
   {"GL_EXT_pixel_buffer_object", NULL},
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
/*     { "GL_SGIS_generate_mipmap",           NULL }, */
   {NULL, NULL}
};



#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
   {"ioctl", DEBUG_IOCTL},
   {"bat", DEBUG_BATCH},
   {"lock", DEBUG_LOCK},
   {"swap", DEBUG_SWAP},
   {NULL, 0}
};
#endif


static void
intelInitDriverFunctions(struct dd_function_table *functions)
{
   memset(functions, 0, sizeof(*functions));
   st_init_driver_functions(functions);
}





GLboolean
intelCreateContext(const __GLcontextModes * mesaVis,
                   __DRIcontextPrivate * driContextPriv,
                   void *sharedContextPrivate)
{
   struct intel_context *intel = CALLOC_STRUCT(intel_context);
#if 0
   struct dd_function_table functions;
   GLcontext *ctx = &intel->ctx;
   GLcontext *shareCtx = (GLcontext *) sharedContextPrivate;
#endif

   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;
   drmI830Sarea *saPriv = intelScreen->sarea;
   int fthrottle_mode;
   GLboolean havePools;

#if 0
   intelInitDriverFunctions(&functions);

   if (!_mesa_initialize_context(&intel->ctx,
                                 mesaVis, shareCtx,
                                 &functions, (void *) intel))
      return GL_FALSE;
#endif

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driScreen = sPriv;
   intel->sarea = saPriv;

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       intel->driScreen->myNum, "i915");


   /*
    * memory pools
    */
   DRM_LIGHT_LOCK(sPriv->fd, &sPriv->pSAREA->lock, driContextPriv->hHWContext);
   havePools = intelCreatePools(intelScreen);
   DRM_UNLOCK(sPriv->fd, &sPriv->pSAREA->lock, driContextPriv->hHWContext);
   if (!havePools)
      return GL_FALSE;


   /* Dri stuff */
   intel->hHWContext = driContextPriv->hHWContext;
   intel->driFd = sPriv->fd;
   intel->driHwLock = (drmLock *) & sPriv->pSAREA->lock;

   fthrottle_mode = driQueryOptioni(&intel->optionCache, "fthrottle_mode");
   intel->iw.irq_seq = -1;
   intel->irqsEmitted = 0;

   intel->batch = intel_batchbuffer_alloc(intel);
   intel->last_swap_fence = NULL;
   intel->first_swap_fence = NULL;

   /* Disable imaging extension until convolution is working in
    * teximage paths:
    */
#if 0
   driInitExtensions(ctx, card_extensions,
/* 		      GL_TRUE, */
                     GL_FALSE);
#endif

#if 0
   if (intel->ctx.Mesa_DXTn) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
      _mesa_enable_extension(ctx, "GL_S3_s3tc");
   }
   else if (driQueryOptionb(&intel->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
   }
#endif

#ifdef DEBUG
   __intel_debug = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
#endif


   /*
    * Pipe-related setup
    */
   if (!getenv("INTEL_HW")) {
      intel->pipe = intel_create_softpipe( intel );
   }
   else {
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_I945_G:
      case PCI_CHIP_I945_GM:
      case PCI_CHIP_I945_GME:
      case PCI_CHIP_G33_G:
      case PCI_CHIP_Q33_G:
      case PCI_CHIP_Q35_G:
      case PCI_CHIP_I915_G:
      case PCI_CHIP_I915_GM:
	 intel->pipe = intel_create_i915simple( intel );
	 break;
      default:
	 _mesa_printf("Unknown PCIID %x in %s, using software driver\n", 
		      intel->intelScreen->deviceID, __FUNCTION__);

	 intel->pipe = intel_create_softpipe( intel );
	 break;
      }
   }

#if 0
   st_create_context( &intel->ctx, intel->pipe ); 
#else
   intel->st = st_create_context2(intel->pipe,  mesaVis, NULL);
   intel->st->ctx->DriverCtx = intel;
#endif

   return GL_TRUE;
}

void
intelDestroyContext(__DRIcontextPrivate * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;
   GLcontext *ctx = intel->st->ctx;

   assert(intel);               /* should never be null */
   if (intel) {
      GLboolean release_texture_heaps;

      INTEL_FIREVERTICES(intel);

      //intel->vtbl.destroy(intel);

      release_texture_heaps = (ctx->Shared->RefCount == 1);

      intel_batchbuffer_free(intel->batch);

      if (intel->last_swap_fence) {
	 driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
	 driFenceUnReference(intel->last_swap_fence);
	 intel->last_swap_fence = NULL;
      }
      if (intel->first_swap_fence) {
	 driFenceFinish(intel->first_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
	 driFenceUnReference(intel->first_swap_fence);
	 intel->first_swap_fence = NULL;
      }


      if (release_texture_heaps) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
      }

#if 0
      /* free the Mesa context data */
      _mesa_free_context_data(ctx);

      st_destroy_context(intel->ctx.st);
#else
      st_destroy_context2(intel->st);
#endif
   }
}

GLboolean
intelUnbindContext(__DRIcontextPrivate * driContextPriv)
{
   struct intel_context *intel = (struct intel_context *) driContextPriv->driverPrivate;
   st_flush(intel->st);
   return GL_TRUE;
}


/**
 * Copied/modified from drirenderbuffer.c
 */
void
intelUpdateFramebufferSize(GLcontext *ctx, const __DRIdrawablePrivate *dPriv)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) dPriv->driverPrivate;
   if (fb && (dPriv->w != fb->Width || dPriv->h != fb->Height)) {
      _mesa_resize_framebuffer(ctx, fb, dPriv->w, dPriv->h);
      /* if the driver needs the hw lock for ResizeBuffers, the drawable
         might have changed again by now */
      assert(fb->Width == dPriv->w);
      assert(fb->Height == dPriv->h);
   }
}


GLboolean
intelMakeCurrent(__DRIcontextPrivate * driContextPriv,
                 __DRIdrawablePrivate * driDrawPriv,
                 __DRIdrawablePrivate * driReadPriv)
{

#if 0
   if (driDrawPriv) {
      fprintf(stderr, "x %d, y %d, width %d, height %d\n",
	      driDrawPriv->x, driDrawPriv->y, driDrawPriv->w, driDrawPriv->h);
   }
#endif

   if (driContextPriv) {
      struct intel_context *intel =
         (struct intel_context *) driContextPriv->driverPrivate;
      struct intel_framebuffer *intel_fb =
	 (struct intel_framebuffer *) driDrawPriv->driverPrivate;
      GLframebuffer *readFb = (GLframebuffer *) driReadPriv->driverPrivate;
      GLcontext *ctx = intel->st->ctx;

      /* this is a hack so we have a valid context when the region allocation
         is done. Need a per-screen context? */
      intel->intelScreen->dummyctxptr = intel;

      /* update GLframebuffer size to match window if needed */
      intelUpdateFramebufferSize(ctx, driDrawPriv);

      if (driReadPriv != driDrawPriv) {
         intelUpdateFramebufferSize(ctx, driReadPriv);
      }

      _mesa_make_current(ctx, &intel_fb->Base, readFb);

      if ((intel->driDrawable != driDrawPriv) ||
	  (intel->lastStamp != driDrawPriv->lastStamp)) {
	    intel->driDrawable = driDrawPriv;
	    intelWindowMoved(intel);
	    intel->lastStamp = driDrawPriv->lastStamp;
      }

   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}

 

struct intel_context *intelScreenContext(intelScreenPrivate *intelScreen)
{
  /*
   * This should probably change to have the screen allocate a dummy
   * context at screen creation. For now just use the current context.
   */

  GET_CURRENT_CONTEXT(ctx);
  if (ctx == NULL) {
/*     _mesa_problem(NULL, "No current context in intelScreenContext\n");
     return NULL; */
     /* need a context for the first time makecurrent is called (for hw lock
        when allocating priv buffers) */
     if (intelScreen->dummyctxptr == NULL) {
        _mesa_problem(NULL, "No current context in intelScreenContext\n");
        return NULL;
     }
     return intelScreen->dummyctxptr;
  }

  return intel_context(ctx);
}

