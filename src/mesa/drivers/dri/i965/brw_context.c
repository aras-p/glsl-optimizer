/*
 Copyright 2003 VMware, Inc.
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

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

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "main/api_exec.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/points.h"
#include "main/version.h"
#include "main/vtxfmt.h"

#include "vbo/vbo_context.h"

#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "utils.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_pixel.h"
#include "intel_image.h"
#include "intel_tex.h"
#include "intel_tex_obj.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "glsl/ralloc.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

static size_t
brw_query_samples_for_format(struct gl_context *ctx, GLenum target,
                             GLenum internalFormat, int samples[16])
{
   struct brw_context *brw = brw_context(ctx);

   (void) target;

   switch (brw->gen) {
   case 8:
      samples[0] = 8;
      samples[1] = 4;
      samples[2] = 2;
      return 3;

   case 7:
      samples[0] = 8;
      samples[1] = 4;
      return 2;

   case 6:
      samples[0] = 4;
      return 1;

   default:
      samples[0] = 1;
      return 1;
   }
}

const char *const brw_vendor_string = "Intel Open Source Technology Center";

const char *
brw_get_renderer_string(unsigned deviceID)
{
   const char *chipset;
   static char buffer[128];

   switch (deviceID) {
#undef CHIPSET
#define CHIPSET(id, symbol, str) case id: chipset = str; break;
#include "pci_ids/i965_pci_ids.h"
   default:
      chipset = "Unknown Intel Chipset";
      break;
   }

   (void) driGetRendererString(buffer, chipset, 0);
   return buffer;
}

static const GLubyte *
intelGetString(struct gl_context * ctx, GLenum name)
{
   const struct brw_context *const brw = brw_context(ctx);

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) brw_vendor_string;

   case GL_RENDERER:
      return
         (GLubyte *) brw_get_renderer_string(brw->intelScreen->deviceID);

   default:
      return NULL;
   }
}

static void
intel_viewport(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;

   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      dri2InvalidateDrawable(driContext->driDrawablePriv);
      dri2InvalidateDrawable(driContext->driReadablePriv);
   }
}

static void
intelInvalidateState(struct gl_context * ctx, GLuint new_state)
{
   struct brw_context *brw = brw_context(ctx);

   if (ctx->swrast_context)
      _swrast_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);

   brw->NewGLState |= new_state;
}

#define flushFront(screen)      ((screen)->image.loader ? (screen)->image.loader->flushFrontBuffer : (screen)->dri2.loader->flushFrontBuffer)

static void
intel_flush_front(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *driDrawable = driContext->driDrawablePriv;
   __DRIscreen *const screen = brw->intelScreen->driScrnPriv;

   if (brw->front_buffer_dirty && _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (flushFront(screen) && driDrawable &&
          driDrawable->loaderPrivate) {

         /* Resolve before flushing FAKE_FRONT_LEFT to FRONT_LEFT.
          *
          * This potentially resolves both front and back buffer. It
          * is unnecessary to resolve the back, but harms nothing except
          * performance. And no one cares about front-buffer render
          * performance.
          */
         intel_resolve_for_dri2_flush(brw, driDrawable);
         intel_batchbuffer_flush(brw);

         flushFront(screen)(driDrawable, driDrawable->loaderPrivate);

         /* We set the dirty bit in intel_prepare_render() if we're
          * front buffer rendering once we get there.
          */
         brw->front_buffer_dirty = false;
      }
   }
}

static void
intel_glFlush(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);

   intel_batchbuffer_flush(brw);
   intel_flush_front(ctx);
   if (brw_is_front_buffer_drawing(ctx->DrawBuffer))
      brw->need_throttle = true;
}

void
intelFinish(struct gl_context * ctx)
{
   struct brw_context *brw = brw_context(ctx);

   intel_glFlush(ctx);

   if (brw->batch.last_bo)
      drm_intel_bo_wait_rendering(brw->batch.last_bo);
}

static void
brw_init_driver_functions(struct brw_context *brw,
                          struct dd_function_table *functions)
{
   _mesa_init_driver_functions(functions);

   /* GLX uses DRI2 invalidate events to handle window resizing.
    * Unfortunately, EGL does not - libEGL is written in XCB (not Xlib),
    * which doesn't provide a mechanism for snooping the event queues.
    *
    * So EGL still relies on viewport hacks to handle window resizing.
    * This should go away with DRI3000.
    */
   if (!brw->driContext->driScreenPriv->dri2.useInvalidate)
      functions->Viewport = intel_viewport;

   functions->Flush = intel_glFlush;
   functions->Finish = intelFinish;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;

   intelInitTextureFuncs(functions);
   intelInitTextureImageFuncs(functions);
   intelInitTextureSubImageFuncs(functions);
   intelInitTextureCopyImageFuncs(functions);
   intelInitClearFuncs(functions);
   intelInitBufferFuncs(functions);
   intelInitPixelFuncs(functions);
   intelInitBufferObjectFuncs(functions);
   intel_init_syncobj_functions(functions);
   brw_init_object_purgeable_functions(functions);

   brwInitFragProgFuncs( functions );
   brw_init_common_queryobj_functions(functions);
   if (brw->gen >= 6)
      gen6_init_queryobj_functions(functions);
   else
      gen4_init_queryobj_functions(functions);

   functions->QuerySamplesForFormat = brw_query_samples_for_format;

   functions->NewTransformFeedback = brw_new_transform_feedback;
   functions->DeleteTransformFeedback = brw_delete_transform_feedback;
   functions->GetTransformFeedbackVertexCount =
      brw_get_transform_feedback_vertex_count;
   if (brw->gen >= 7) {
      functions->BeginTransformFeedback = gen7_begin_transform_feedback;
      functions->EndTransformFeedback = gen7_end_transform_feedback;
      functions->PauseTransformFeedback = gen7_pause_transform_feedback;
      functions->ResumeTransformFeedback = gen7_resume_transform_feedback;
   } else {
      functions->BeginTransformFeedback = brw_begin_transform_feedback;
      functions->EndTransformFeedback = brw_end_transform_feedback;
   }

   if (brw->gen >= 6)
      functions->GetSamplePosition = gen6_get_sample_position;
}

static void
brw_initialize_context_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   unsigned max_samplers =
      brw->gen >= 8 || brw->is_haswell ? BRW_MAX_TEX_UNIT : 16;

   ctx->Const.QueryCounterBits.Timestamp = 36;

   ctx->Const.StripTextureBorder = true;

   ctx->Const.MaxDualSourceDrawBuffers = 1;
   ctx->Const.MaxDrawBuffers = BRW_MAX_DRAW_BUFFERS;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = max_samplers;
   ctx->Const.MaxTextureCoordUnits = 8; /* Mesa limit */
   ctx->Const.MaxTextureUnits =
      MIN2(ctx->Const.MaxTextureCoordUnits,
           ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits);
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = max_samplers;
   if (brw->gen >= 7)
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = max_samplers;
   else
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = 0;
   if (getenv("INTEL_COMPUTE_SHADER")) {
      ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits = BRW_MAX_TEX_UNIT;
      ctx->Const.MaxUniformBufferBindings += 12;
   } else {
      ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits = 0;
   }
   ctx->Const.MaxCombinedTextureImageUnits =
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits +
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits +
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits +
      ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits;

   ctx->Const.MaxTextureLevels = 14; /* 8192 */
   if (ctx->Const.MaxTextureLevels > MAX_TEXTURE_LEVELS)
      ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.Max3DTextureLevels = 12; /* 2048 */
   ctx->Const.MaxCubeTextureLevels = 14; /* 8192 */
   ctx->Const.MaxTextureMbytes = 1536;

   if (brw->gen >= 7)
      ctx->Const.MaxArrayTextureLayers = 2048;
   else
      ctx->Const.MaxArrayTextureLayers = 512;

   ctx->Const.MaxTextureRectSize = 1 << 12;

   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   ctx->Const.MaxRenderbufferSize = 8192;

   /* Hardware only supports a limited number of transform feedback buffers.
    * So we need to override the Mesa default (which is based only on software
    * limits).
    */
   ctx->Const.MaxTransformFeedbackBuffers = BRW_MAX_SOL_BUFFERS;

   /* On Gen6, in the worst case, we use up one binding table entry per
    * transform feedback component (see comments above the definition of
    * BRW_MAX_SOL_BINDINGS, in brw_context.h), so we need to advertise a value
    * for MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS equal to
    * BRW_MAX_SOL_BINDINGS.
    *
    * In "separate components" mode, we need to divide this value by
    * BRW_MAX_SOL_BUFFERS, so that the total number of binding table entries
    * used up by all buffers will not exceed BRW_MAX_SOL_BINDINGS.
    */
   ctx->Const.MaxTransformFeedbackInterleavedComponents = BRW_MAX_SOL_BINDINGS;
   ctx->Const.MaxTransformFeedbackSeparateComponents =
      BRW_MAX_SOL_BINDINGS / BRW_MAX_SOL_BUFFERS;

   ctx->Const.AlwaysUseGetTransformFeedbackVertexCount = true;

   int max_samples;
   const int *msaa_modes = intel_supported_msaa_modes(brw->intelScreen);
   const int clamp_max_samples =
      driQueryOptioni(&brw->optionCache, "clamp_max_samples");

   if (clamp_max_samples < 0) {
      max_samples = msaa_modes[0];
   } else {
      /* Select the largest supported MSAA mode that does not exceed
       * clamp_max_samples.
       */
      max_samples = 0;
      for (int i = 0; msaa_modes[i] != 0; ++i) {
         if (msaa_modes[i] <= clamp_max_samples) {
            max_samples = msaa_modes[i];
            break;
         }
      }
   }

   ctx->Const.MaxSamples = max_samples;
   ctx->Const.MaxColorTextureSamples = max_samples;
   ctx->Const.MaxDepthTextureSamples = max_samples;
   ctx->Const.MaxIntegerSamples = max_samples;

   if (brw->gen >= 7)
      ctx->Const.MaxProgramTextureGatherComponents = 4;
   else if (brw->gen == 6)
      ctx->Const.MaxProgramTextureGatherComponents = 1;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 5.0;
   ctx->Const.MaxLineWidthAA = 5.0;
   ctx->Const.LineWidthGranularity = 0.5;

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 255.0;
   ctx->Const.PointSizeGranularity = 1.0;

   if (brw->gen >= 5 || brw->is_g4x)
      ctx->Const.MaxClipPlanes = 8;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeInstructions = 16 * 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAddressRegs = 1;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters,
	   ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams);

   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAluInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexIndirections = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAttribs = 12;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAddressRegs = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters,
	   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams);

   /* Fragment shaders use real, 32-bit twos-complement integers for all
    * integer types.
    */
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMin = 31;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMax = 30;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.Precision = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].HighInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MediumInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;

   if (brw->gen >= 7) {
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      ctx->Const.Program[MESA_SHADER_COMPUTE].MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxAtomicBuffers = BRW_MAX_ABO;
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxAtomicBuffers = BRW_MAX_ABO;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxAtomicBuffers = BRW_MAX_ABO;
      ctx->Const.Program[MESA_SHADER_COMPUTE].MaxAtomicBuffers = BRW_MAX_ABO;
      ctx->Const.MaxCombinedAtomicBuffers = 3 * BRW_MAX_ABO;
   }

   /* Gen6 converts quads to polygon in beginning of 3D pipeline,
    * but we're not sure how it's actually done for vertex order,
    * that affect provoking vertex decision. Always use last vertex
    * convention for quad primitive which works as expected for now.
    */
   if (brw->gen >= 6)
      ctx->Const.QuadsFollowProvokingVertexConvention = false;

   ctx->Const.NativeIntegers = true;
   ctx->Const.UniformBooleanTrue = 1;

   /* From the gen4 PRM, volume 4 page 127:
    *
    *     "For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    *
    * However, unaligned accesses are slower, so enforce buffer alignment.
    */
   ctx->Const.UniformBufferOffsetAlignment = 16;
   ctx->Const.TextureBufferOffsetAlignment = 16;

   if (brw->gen >= 6) {
      ctx->Const.MaxVarying = 32;
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents = 64;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents = 128;
   }

   /* We want the GLSL compiler to emit code that uses condition codes */
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      ctx->ShaderCompilerOptions[i].MaxIfDepth = brw->gen < 6 ? 16 : UINT_MAX;
      ctx->ShaderCompilerOptions[i].EmitCondCodes = true;
      ctx->ShaderCompilerOptions[i].EmitNoNoise = true;
      ctx->ShaderCompilerOptions[i].EmitNoMainReturn = true;
      ctx->ShaderCompilerOptions[i].EmitNoIndirectInput = true;
      ctx->ShaderCompilerOptions[i].EmitNoIndirectOutput =
	 (i == MESA_SHADER_FRAGMENT);
      ctx->ShaderCompilerOptions[i].EmitNoIndirectTemp =
	 (i == MESA_SHADER_FRAGMENT);
      ctx->ShaderCompilerOptions[i].EmitNoIndirectUniform = false;
      ctx->ShaderCompilerOptions[i].LowerClipDistance = true;
   }

   ctx->ShaderCompilerOptions[MESA_SHADER_VERTEX].OptimizeForAOS = true;
   ctx->ShaderCompilerOptions[MESA_SHADER_GEOMETRY].OptimizeForAOS = true;

   /* ARB_viewport_array */
   if (brw->gen >= 7 && ctx->API == API_OPENGL_CORE) {
      ctx->Const.MaxViewports = GEN7_NUM_VIEWPORTS;
      ctx->Const.ViewportSubpixelBits = 0;

      /* Cast to float before negating becuase MaxViewportWidth is unsigned.
       */
      ctx->Const.ViewportBounds.Min = -(float)ctx->Const.MaxViewportWidth;
      ctx->Const.ViewportBounds.Max = ctx->Const.MaxViewportWidth;
   }
}

/**
 * Process driconf (drirc) options, setting appropriate context flags.
 *
 * intelInitExtensions still pokes at optionCache directly, in order to
 * avoid advertising various extensions.  No flags are set, so it makes
 * sense to continue doing that there.
 */
static void
brw_process_driconf_options(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   driOptionCache *options = &brw->optionCache;
   driParseConfigFiles(options, &brw->intelScreen->optionCache,
                       brw->driContext->driScreenPriv->myNum, "i965");

   int bo_reuse_mode = driQueryOptioni(options, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      intel_bufmgr_gem_enable_reuse(brw->bufmgr);
      break;
   }

   if (!driQueryOptionb(options, "hiz")) {
       brw->has_hiz = false;
       /* On gen6, you can only do separate stencil with HIZ. */
       if (brw->gen == 6)
          brw->has_separate_stencil = false;
   }

   if (driQueryOptionb(options, "always_flush_batch")) {
      fprintf(stderr, "flushing batchbuffer before/after each draw call\n");
      brw->always_flush_batch = true;
   }

   if (driQueryOptionb(options, "always_flush_cache")) {
      fprintf(stderr, "flushing GPU caches before/after each draw call\n");
      brw->always_flush_cache = true;
   }

   if (driQueryOptionb(options, "disable_throttling")) {
      fprintf(stderr, "disabling flush throttling\n");
      brw->disable_throttling = true;
   }

   brw->disable_derivative_optimization =
      driQueryOptionb(&brw->optionCache, "disable_derivative_optimization");

   brw->precompile = driQueryOptionb(&brw->optionCache, "shader_precompile");

   ctx->Const.ForceGLSLExtensionsWarn =
      driQueryOptionb(options, "force_glsl_extensions_warn");

   ctx->Const.DisableGLSLLineContinuations =
      driQueryOptionb(options, "disable_glsl_line_continuations");
}

GLboolean
brwCreateContext(gl_api api,
	         const struct gl_config *mesaVis,
		 __DRIcontext *driContextPriv,
                 unsigned major_version,
                 unsigned minor_version,
                 uint32_t flags,
                 bool notify_reset,
                 unsigned *dri_ctx_error,
	         void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   struct intel_screen *screen = sPriv->driverPrivate;
   const struct brw_device_info *devinfo = screen->devinfo;
   struct dd_function_table functions;

   /* Only allow the __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS flag if the kernel
    * provides us with context reset notifications.
    */
   uint32_t allowed_flags = __DRI_CTX_FLAG_DEBUG
      | __DRI_CTX_FLAG_FORWARD_COMPATIBLE;

   if (screen->has_context_reset_notification)
      allowed_flags |= __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS;

   if (flags & ~allowed_flags) {
      *dri_ctx_error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
      return false;
   }

   struct brw_context *brw = rzalloc(NULL, struct brw_context);
   if (!brw) {
      fprintf(stderr, "%s: failed to alloc context\n", __FUNCTION__);
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   driContextPriv->driverPrivate = brw;
   brw->driContext = driContextPriv;
   brw->intelScreen = screen;
   brw->bufmgr = screen->bufmgr;

   brw->gen = devinfo->gen;
   brw->gt = devinfo->gt;
   brw->is_g4x = devinfo->is_g4x;
   brw->is_baytrail = devinfo->is_baytrail;
   brw->is_haswell = devinfo->is_haswell;
   brw->has_llc = devinfo->has_llc;
   brw->has_hiz = devinfo->has_hiz_and_separate_stencil;
   brw->has_separate_stencil = devinfo->has_hiz_and_separate_stencil;
   brw->has_pln = devinfo->has_pln;
   brw->has_compr4 = devinfo->has_compr4;
   brw->has_surface_tile_offset = devinfo->has_surface_tile_offset;
   brw->has_negative_rhw_bug = devinfo->has_negative_rhw_bug;
   brw->needs_unlit_centroid_workaround =
      devinfo->needs_unlit_centroid_workaround;

   brw->must_use_separate_stencil = screen->hw_must_use_separate_stencil;
   brw->has_swizzling = screen->hw_has_swizzling;

   brw->vs.base.stage = MESA_SHADER_VERTEX;
   brw->gs.base.stage = MESA_SHADER_GEOMETRY;
   brw->wm.base.stage = MESA_SHADER_FRAGMENT;
   if (brw->gen >= 8) {
      gen8_init_vtable_surface_functions(brw);
      gen7_init_vtable_sampler_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen8_emit_depth_stencil_hiz;
   } else if (brw->gen >= 7) {
      gen7_init_vtable_surface_functions(brw);
      gen7_init_vtable_sampler_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen7_emit_depth_stencil_hiz;
   } else {
      gen4_init_vtable_surface_functions(brw);
      gen4_init_vtable_sampler_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = brw_emit_depth_stencil_hiz;
   }

   brw_init_driver_functions(brw, &functions);

   if (notify_reset)
      functions.GetGraphicsResetStatus = brw_get_graphics_reset_status;

   struct gl_context *ctx = &brw->ctx;

   if (!_mesa_initialize_context(ctx, api, mesaVis, shareCtx, &functions)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      fprintf(stderr, "%s: failed to init mesa context\n", __FUNCTION__);
      intelDestroyContext(driContextPriv);
      return false;
   }

   driContextSetFlags(ctx, flags);

   /* Initialize the software rasterizer and helper modules.
    *
    * As of GL 3.1 core, the gen4+ driver doesn't need the swrast context for
    * software fallbacks (which we have to support on legacy GL to do weird
    * glDrawPixels(), glBitmap(), and other functions).
    */
   if (api != API_OPENGL_CORE && api != API_OPENGLES2) {
      _swrast_CreateContext(ctx);
   }

   _vbo_CreateContext(ctx);
   if (ctx->swrast_context) {
      _tnl_CreateContext(ctx);
      TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;
      _swsetup_CreateContext(ctx);

      /* Configure swrast to match hardware characteristics: */
      _swrast_allow_pixel_fog(ctx, false);
      _swrast_allow_vertex_fog(ctx, true);
   }

   _mesa_meta_init(ctx);

   brw_process_driconf_options(brw);
   brw_process_intel_debug_variable(brw);
   brw_initialize_context_constants(brw);

   ctx->Const.ResetStrategy = notify_reset
      ? GL_LOSE_CONTEXT_ON_RESET_ARB : GL_NO_RESET_NOTIFICATION_ARB;

   /* Reinitialize the context point state.  It depends on ctx->Const values. */
   _mesa_init_point(ctx);

   intel_fbo_init(brw);

   intel_batchbuffer_init(brw);

   if (brw->gen >= 6) {
      /* Create a new hardware context.  Using a hardware context means that
       * our GPU state will be saved/restored on context switch, allowing us
       * to assume that the GPU is in the same state we left it in.
       *
       * This is required for transform feedback buffer offsets, query objects,
       * and also allows us to reduce how much state we have to emit.
       */
      brw->hw_ctx = drm_intel_gem_context_create(brw->bufmgr);

      if (!brw->hw_ctx) {
         fprintf(stderr, "Gen6+ requires Kernel 3.6 or later.\n");
         intelDestroyContext(driContextPriv);
         return false;
      }
   }

   brw_init_state(brw);

   intelInitExtensions(ctx);

   brw_init_surface_formats(brw);

   brw->max_vs_threads = devinfo->max_vs_threads;
   brw->max_gs_threads = devinfo->max_gs_threads;
   brw->max_wm_threads = devinfo->max_wm_threads;
   brw->urb.size = devinfo->urb.size;
   brw->urb.min_vs_entries = devinfo->urb.min_vs_entries;
   brw->urb.max_vs_entries = devinfo->urb.max_vs_entries;
   brw->urb.max_gs_entries = devinfo->urb.max_gs_entries;

   /* Estimate the size of the mappable aperture into the GTT.  There's an
    * ioctl to get the whole GTT size, but not one to get the mappable subset.
    * It turns out it's basically always 256MB, though some ancient hardware
    * was smaller.
    */
   uint32_t gtt_size = 256 * 1024 * 1024;

   /* We don't want to map two objects such that a memcpy between them would
    * just fault one mapping in and then the other over and over forever.  So
    * we would need to divide the GTT size by 2.  Additionally, some GTT is
    * taken up by things like the framebuffer and the ringbuffer and such, so
    * be more conservative.
    */
   brw->max_gtt_map_object_size = gtt_size / 4;

   if (brw->gen == 6)
      brw->urb.gen6_gs_previously_active = false;

   brw->prim_restart.in_progress = false;
   brw->prim_restart.enable_cut_index = false;
   brw->gs.enabled = false;

   if (brw->gen < 6) {
      brw->curbe.last_buf = calloc(1, 4096);
      brw->curbe.next_buf = calloc(1, 4096);
   }

   ctx->VertexProgram._MaintainTnlProgram = true;
   ctx->FragmentProgram._MaintainTexEnvProgram = true;

   brw_draw_init( brw );

   if ((flags & __DRI_CTX_FLAG_DEBUG) != 0) {
      /* Turn on some extra GL_ARB_debug_output generation. */
      brw->perf_debug = true;
   }

   if ((flags & __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS) != 0)
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;

   if (INTEL_DEBUG & DEBUG_SHADER_TIME)
      brw_init_shader_time(brw);

   _mesa_compute_version(ctx);

   _mesa_initialize_dispatch_tables(ctx);
   _mesa_initialize_vbo_vtxfmt(ctx);

   if (ctx->Extensions.AMD_performance_monitor) {
      brw_init_performance_monitors(brw);
   }

   return true;
}

void
intelDestroyContext(__DRIcontext * driContextPriv)
{
   struct brw_context *brw =
      (struct brw_context *) driContextPriv->driverPrivate;
   struct gl_context *ctx = &brw->ctx;

   assert(brw); /* should never be null */
   if (!brw)
      return;

   /* Dump a final BMP in case the application doesn't call SwapBuffers */
   if (INTEL_DEBUG & DEBUG_AUB) {
      intel_batchbuffer_flush(brw);
      aub_dump_bmp(&brw->ctx);
   }

   _mesa_meta_free(&brw->ctx);

   if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
      /* Force a report. */
      brw->shader_time.report_time = 0;

      brw_collect_and_report_shader_time(brw);
      brw_destroy_shader_time(brw);
   }

   brw_destroy_state(brw);
   brw_draw_destroy(brw);

   drm_intel_bo_unreference(brw->curbe.curbe_bo);

   free(brw->curbe.last_buf);
   free(brw->curbe.next_buf);

   drm_intel_gem_context_destroy(brw->hw_ctx);

   if (ctx->swrast_context) {
      _swsetup_DestroyContext(&brw->ctx);
      _tnl_DestroyContext(&brw->ctx);
   }
   _vbo_DestroyContext(&brw->ctx);

   if (ctx->swrast_context)
      _swrast_DestroyContext(&brw->ctx);

   intel_batchbuffer_free(brw);

   drm_intel_bo_unreference(brw->first_post_swapbuffers_batch);
   brw->first_post_swapbuffers_batch = NULL;

   driDestroyOptionCache(&brw->optionCache);

   /* free the Mesa context */
   _mesa_free_context_data(&brw->ctx);

   ralloc_free(brw);
   driContextPriv->driverPrivate = NULL;
}

GLboolean
intelUnbindContext(__DRIcontext * driContextPriv)
{
   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return true;
}

/**
 * Fixes up the context for GLES23 with our default-to-sRGB-capable behavior
 * on window system framebuffers.
 *
 * Desktop GL is fairly reasonable in its handling of sRGB: You can ask if
 * your renderbuffer can do sRGB encode, and you can flip a switch that does
 * sRGB encode if the renderbuffer can handle it.  You can ask specifically
 * for a visual where you're guaranteed to be capable, but it turns out that
 * everyone just makes all their ARGB8888 visuals capable and doesn't offer
 * incapable ones, becuase there's no difference between the two in resources
 * used.  Applications thus get built that accidentally rely on the default
 * visual choice being sRGB, so we make ours sRGB capable.  Everything sounds
 * great...
 *
 * But for GLES2/3, they decided that it was silly to not turn on sRGB encode
 * for sRGB renderbuffers you made with the GL_EXT_texture_sRGB equivalent.
 * So they removed the enable knob and made it "if the renderbuffer is sRGB
 * capable, do sRGB encode".  Then, for your window system renderbuffers, you
 * can ask for sRGB visuals and get sRGB encode, or not ask for sRGB visuals
 * and get no sRGB encode (assuming that both kinds of visual are available).
 * Thus our choice to support sRGB by default on our visuals for desktop would
 * result in broken rendering of GLES apps that aren't expecting sRGB encode.
 *
 * Unfortunately, renderbuffer setup happens before a context is created.  So
 * in intel_screen.c we always set up sRGB, and here, if you're a GLES2/3
 * context (without an sRGB visual, though we don't have sRGB visuals exposed
 * yet), we go turn that back off before anyone finds out.
 */
static void
intel_gles3_srgb_workaround(struct brw_context *brw,
                            struct gl_framebuffer *fb)
{
   struct gl_context *ctx = &brw->ctx;

   if (_mesa_is_desktop_gl(ctx) || !fb->Visual.sRGBCapable)
      return;

   /* Some day when we support the sRGB capable bit on visuals available for
    * GLES, we'll need to respect that and not disable things here.
    */
   fb->Visual.sRGBCapable = false;
   for (int i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Renderbuffer &&
          fb->Attachment[i].Renderbuffer->Format == MESA_FORMAT_B8G8R8A8_SRGB) {
         fb->Attachment[i].Renderbuffer->Format = MESA_FORMAT_B8G8R8A8_UNORM;
      }
   }
}

GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv)
{
   struct brw_context *brw;
   GET_CURRENT_CONTEXT(curCtx);

   if (driContextPriv)
      brw = (struct brw_context *) driContextPriv->driverPrivate;
   else
      brw = NULL;

   /* According to the glXMakeCurrent() man page: "Pending commands to
    * the previous context, if any, are flushed before it is released."
    * But only flush if we're actually changing contexts.
    */
   if (brw_context(curCtx) && brw_context(curCtx) != brw) {
      _mesa_flush(curCtx);
   }

   if (driContextPriv) {
      struct gl_context *ctx = &brw->ctx;
      struct gl_framebuffer *fb, *readFb;

      if (driDrawPriv == NULL && driReadPriv == NULL) {
         fb = _mesa_get_incomplete_framebuffer();
         readFb = _mesa_get_incomplete_framebuffer();
      } else {
         fb = driDrawPriv->driverPrivate;
         readFb = driReadPriv->driverPrivate;
         driContextPriv->dri2.draw_stamp = driDrawPriv->dri2.stamp - 1;
         driContextPriv->dri2.read_stamp = driReadPriv->dri2.stamp - 1;
      }

      /* The sRGB workaround changes the renderbuffer's format. We must change
       * the format before the renderbuffer's miptree get's allocated, otherwise
       * the formats of the renderbuffer and its miptree will differ.
       */
      intel_gles3_srgb_workaround(brw, fb);
      intel_gles3_srgb_workaround(brw, readFb);

      /* If the context viewport hasn't been initialized, force a call out to
       * the loader to get buffers so we have a drawable size for the initial
       * viewport. */
      if (!brw->ctx.ViewportInitialized)
         intel_prepare_render(brw);

      _mesa_make_current(ctx, fb, readFb);
   } else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return true;
}

void
intel_resolve_for_dri2_flush(struct brw_context *brw,
                             __DRIdrawable *drawable)
{
   if (brw->gen < 6) {
      /* MSAA and fast color clear are not supported, so don't waste time
       * checking whether a resolve is needed.
       */
      return;
   }

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;

   /* Usually, only the back buffer will need to be downsampled. However,
    * the front buffer will also need it if the user has rendered into it.
    */
   static const gl_buffer_index buffers[2] = {
         BUFFER_BACK_LEFT,
         BUFFER_FRONT_LEFT,
   };

   for (int i = 0; i < 2; ++i) {
      rb = intel_get_renderbuffer(fb, buffers[i]);
      if (rb == NULL || rb->mt == NULL)
         continue;
      if (rb->mt->num_samples <= 1)
         intel_miptree_resolve_color(brw, rb->mt);
      else
         intel_renderbuffer_downsample(brw, rb);
   }
}

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(intel_rb_format(rb)) * 8;
}

static void
intel_query_dri2_buffers(struct brw_context *brw,
                         __DRIdrawable *drawable,
                         __DRIbuffer **buffers,
                         int *count);

static void
intel_process_dri2_buffer(struct brw_context *brw,
                          __DRIdrawable *drawable,
                          __DRIbuffer *buffer,
                          struct intel_renderbuffer *rb,
                          const char *buffer_name);

static void
intel_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable);

static void
intel_update_dri2_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;
   __DRIbuffer *buffers = NULL;
   int i, count;
   const char *region_name;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   intel_query_dri2_buffers(brw, drawable, &buffers, &count);

   if (buffers == NULL)
      return;

   for (i = 0; i < count; i++) {
       switch (buffers[i].attachment) {
       case __DRI_BUFFER_FRONT_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 front buffer";
           break;

       case __DRI_BUFFER_FAKE_FRONT_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 fake front buffer";
           break;

       case __DRI_BUFFER_BACK_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
           region_name = "dri2 back buffer";
           break;

       case __DRI_BUFFER_DEPTH:
       case __DRI_BUFFER_HIZ:
       case __DRI_BUFFER_DEPTH_STENCIL:
       case __DRI_BUFFER_STENCIL:
       case __DRI_BUFFER_ACCUM:
       default:
           fprintf(stderr,
                   "unhandled buffer attach event, attachment type %d\n",
                   buffers[i].attachment);
           return;
       }

       intel_process_dri2_buffer(brw, drawable, &buffers[i], rb, region_name);
   }

}

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct brw_context *brw = context->driverPrivate;
   __DRIscreen *screen = brw->intelScreen->driScrnPriv;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   if (screen->image.loader)
      intel_update_image_buffers(brw, drawable);
   else
      intel_update_dri2_buffers(brw, drawable);

   driUpdateFramebufferSize(&brw->ctx, drawable);
}

/**
 * intel_prepare_render should be called anywhere that curent read/drawbuffer
 * state is required.
 */
void
intel_prepare_render(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *drawable;

   drawable = driContext->driDrawablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.draw_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.draw_stamp = drawable->dri2.stamp;
   }

   drawable = driContext->driReadablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.read_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.read_stamp = drawable->dri2.stamp;
   }

   /* If we're currently rendering to the front buffer, the rendering
    * that will happen next will probably dirty the front buffer.  So
    * mark it as dirty here.
    */
   if (brw_is_front_buffer_drawing(ctx->DrawBuffer))
      brw->front_buffer_dirty = true;

   /* Wait for the swapbuffers before the one we just emitted, so we
    * don't get too many swaps outstanding for apps that are GPU-heavy
    * but not CPU-heavy.
    *
    * We're using intelDRI2Flush (called from the loader before
    * swapbuffer) and glFlush (for front buffer rendering) as the
    * indicator that a frame is done and then throttle when we get
    * here as we prepare to render the next frame.  At this point for
    * round trips for swap/copy and getting new buffers are done and
    * we'll spend less time waiting on the GPU.
    *
    * Unfortunately, we don't have a handle to the batch containing
    * the swap, and getting our hands on that doesn't seem worth it,
    * so we just us the first batch we emitted after the last swap.
    */
   if (brw->need_throttle && brw->first_post_swapbuffers_batch) {
      if (!brw->disable_throttling)
         drm_intel_bo_wait_rendering(brw->first_post_swapbuffers_batch);
      drm_intel_bo_unreference(brw->first_post_swapbuffers_batch);
      brw->first_post_swapbuffers_batch = NULL;
      brw->need_throttle = false;
   }
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffers() or DRI2GetBuffersWithFormat().
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 * \see DRI2GetBuffers()
 * \see DRI2GetBuffersWithFormat()
 */
static void
intel_query_dri2_buffers(struct brw_context *brw,
                         __DRIdrawable *drawable,
                         __DRIbuffer **buffers,
                         int *buffer_count)
{
   __DRIscreen *screen = brw->intelScreen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;
   int i = 0;
   unsigned attachments[8];

   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   memset(attachments, 0, sizeof(attachments));
   if ((brw_is_front_buffer_drawing(fb) ||
        brw_is_front_buffer_reading(fb) ||
        !back_rb) && front_rb) {
      /* If a fake front buffer is in use, then querying for
       * __DRI_BUFFER_FRONT_LEFT will cause the server to copy the image from
       * the real front buffer to the fake front buffer.  So before doing the
       * query, we need to make sure all the pending drawing has landed in the
       * real front buffer.
       */
      intel_batchbuffer_flush(brw);
      intel_flush_front(&brw->ctx);

      attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      attachments[i++] = intel_bits_per_pixel(front_rb);
   } else if (front_rb && brw->front_buffer_dirty) {
      /* We have pending front buffer rendering, but we aren't querying for a
       * front buffer.  If the front buffer we have is a fake front buffer,
       * the X server is going to throw it away when it processes the query.
       * So before doing the query, make sure all the pending drawing has
       * landed in the real front buffer.
       */
      intel_batchbuffer_flush(brw);
      intel_flush_front(&brw->ctx);
   }

   if (back_rb) {
      attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      attachments[i++] = intel_bits_per_pixel(back_rb);
   }

   assert(i <= ARRAY_SIZE(attachments));

   *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
                                                        &drawable->w,
                                                        &drawable->h,
                                                        attachments, i / 2,
                                                        buffer_count,
                                                        drawable->loaderPrivate);
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or
 *    DRI2BufferDepthStencil are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to drm_intel_bo_gem_create_from_name().
 *
 * \see intel_update_renderbuffers()
 */
static void
intel_process_dri2_buffer(struct brw_context *brw,
                          __DRIdrawable *drawable,
                          __DRIbuffer *buffer,
                          struct intel_renderbuffer *rb,
                          const char *buffer_name)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   drm_intel_bo *bo;

   if (!rb)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* We try to avoid closing and reopening the same BO name, because the first
    * use of a mapping of the buffer involves a bunch of page faulting which is
    * moderately expensive.
    */
   struct intel_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   uint32_t old_name = 0;
   if (last_mt) {
       /* The bo already has a name because the miptree was created by a
	* previous call to intel_process_dri2_buffer(). If a bo already has a
	* name, then drm_intel_bo_flink() is a low-cost getter.  It does not
	* create a new name.
	*/
      drm_intel_bo_flink(last_mt->bo, &old_name);
   }

   if (old_name == buffer->name)
      return;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
      fprintf(stderr,
              "attaching buffer %d, at %d, cpp %d, pitch %d\n",
              buffer->name, buffer->attachment,
              buffer->cpp, buffer->pitch);
   }

   intel_miptree_release(&rb->mt);
   bo = drm_intel_bo_gem_create_from_name(brw->bufmgr, buffer_name,
                                          buffer->name);
   if (!bo) {
      fprintf(stderr,
              "Failed to open BO for returned DRI2 buffer "
              "(%dx%d, %s, named %d).\n"
              "This is likely a bug in the X Server that will lead to a "
              "crash soon.\n",
              drawable->w, drawable->h, buffer_name, buffer->name);
      return;
   }

   intel_update_winsys_renderbuffer_miptree(brw, rb, bo,
                                            drawable->w, drawable->h,
                                            buffer->pitch);

   if (brw_is_front_buffer_drawing(fb) &&
       (buffer->attachment == __DRI_BUFFER_FRONT_LEFT ||
        buffer->attachment == __DRI_BUFFER_FAKE_FRONT_LEFT) &&
       rb->Base.Base.NumSamples > 1) {
      intel_renderbuffer_upsample(brw, rb);
   }

   assert(rb->mt);

   drm_intel_bo_unreference(bo);
}

/**
 * \brief Query DRI image loader to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers from
 * the image loader
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 */

static void
intel_update_image_buffer(struct brw_context *intel,
                          __DRIdrawable *drawable,
                          struct intel_renderbuffer *rb,
                          __DRIimage *buffer,
                          enum __DRIimageBufferMask buffer_type)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;

   if (!rb || !buffer->bo)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* Check and see if we're already bound to the right
    * buffer object
    */
   struct intel_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   if (last_mt && last_mt->bo == buffer->bo)
      return;

   intel_update_winsys_renderbuffer_miptree(intel, rb, buffer->bo,
                                            buffer->width, buffer->height,
                                            buffer->pitch);

   if (brw_is_front_buffer_drawing(fb) &&
       buffer_type == __DRI_IMAGE_BUFFER_FRONT &&
       rb->Base.Base.NumSamples > 1) {
      intel_renderbuffer_upsample(intel, rb);
   }
}

static void
intel_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   __DRIscreen *screen = brw->intelScreen->driScrnPriv;
   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;
   struct __DRIimageList images;
   unsigned int format;
   uint32_t buffer_mask = 0;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   if (back_rb)
      format = intel_rb_format(back_rb);
   else if (front_rb)
      format = intel_rb_format(front_rb);
   else
      return;

   if (front_rb && (brw_is_front_buffer_drawing(fb) ||
                    brw_is_front_buffer_reading(fb) || !back_rb)) {
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;
   }

   if (back_rb)
      buffer_mask |= __DRI_IMAGE_BUFFER_BACK;

   (*screen->image.loader->getBuffers) (drawable,
                                        driGLFormatToImageFormat(format),
                                        &drawable->dri2.stamp,
                                        drawable->loaderPrivate,
                                        buffer_mask,
                                        &images);

   if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      drawable->w = images.front->width;
      drawable->h = images.front->height;
      intel_update_image_buffer(brw,
                                drawable,
                                front_rb,
                                images.front,
                                __DRI_IMAGE_BUFFER_FRONT);
   }
   if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      drawable->w = images.back->width;
      drawable->h = images.back->height;
      intel_update_image_buffer(brw,
                                drawable,
                                back_rb,
                                images.back,
                                __DRI_IMAGE_BUFFER_BACK);
   }
}
