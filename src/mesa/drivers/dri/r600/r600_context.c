/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#include <stdbool.h>
#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/extensions.h"
#include "main/bufferobj.h"
#include "main/texobj.h"
#include "main/points.h"
#include "main/mfeatures.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_debug.h"
#include "r600_context.h"
#include "radeon_common_context.h"
#include "radeon_buffer_objects.h"
#include "radeon_span.h"
#include "r600_cmdbuf.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_queryobj.h"
#include "r600_blit.h"

#include "r700_state.h"
#include "r700_ioctl.h"

#include "evergreen_context.h"
#include "evergreen_state.h"
#include "evergreen_tex.h"
#include "evergreen_ioctl.h"
#include "evergreen_oglprog.h"

#include "utils.h"

#define R600_ENABLE_GLSL_TEST 1

static const struct tnl_pipeline_stage *r600_pipeline[] = {
	/* Catch any t&l fallbacks
	 */
	&_tnl_vertex_transform_stage,
	&_tnl_normal_transform_stage,
	&_tnl_lighting_stage,
	&_tnl_fog_coordinate_stage,
	&_tnl_texgen_stage,
	&_tnl_texture_transform_stage,
	&_tnl_point_attenuation_stage,
	&_tnl_vertex_program_stage,
	&_tnl_render_stage,
	0,
};

static void r600_get_lock(radeonContextPtr rmesa)
{
	drm_radeon_sarea_t *sarea = rmesa->sarea;

	if (sarea->ctx_owner != rmesa->dri.hwContext) {
		sarea->ctx_owner = rmesa->dri.hwContext;
		if (!rmesa->radeonScreen->kernel_mm)
			radeon_bo_legacy_texture_age(rmesa->radeonScreen->bom);
	}
}

static void r600_vtbl_emit_cs_header(struct radeon_cs *cs, radeonContextPtr rmesa)
{
    /* please flush pipe do all pending work */
    /* to be enabled */
}

static void r600_vtbl_pre_emit_atoms(radeonContextPtr radeon)
{
	r700Start3D((context_t *)radeon);
}

static void r600_fallback(struct gl_context *ctx, GLuint bit, GLboolean mode)
{
	context_t *context = R700_CONTEXT(ctx);
	if (mode)
		context->radeon.Fallback |= bit;
	else
		context->radeon.Fallback &= ~bit;
}

static void r600_emit_query_finish(radeonContextPtr radeon)
{
	context_t *context = (context_t*) radeon;
	BATCH_LOCALS(&context->radeon);

	struct radeon_query_object *query = radeon->query.current;

	BEGIN_BATCH_NO_AUTOSTATE(4 + 2);
	R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 2));
	R600_OUT_BATCH(R600_EVENT_TYPE(ZPASS_DONE) | R600_EVENT_INDEX(1));
	R600_OUT_BATCH(query->curr_offset + 8); /* hw writes qwords */
	R600_OUT_BATCH(0x00000000);
	R600_OUT_BATCH_RELOC(VGT_EVENT_INITIATOR, query->bo, 0, 0, RADEON_GEM_DOMAIN_GTT, 0);
	END_BATCH();
	assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

static void r600_init_vtbl(radeonContextPtr radeon)
{
	radeon->vtbl.get_lock = r600_get_lock;
	radeon->vtbl.update_viewport_offset = r700UpdateViewportOffset;
	radeon->vtbl.emit_cs_header = r600_vtbl_emit_cs_header;
	radeon->vtbl.swtcl_flush = NULL;
	radeon->vtbl.pre_emit_atoms = r600_vtbl_pre_emit_atoms;
	radeon->vtbl.fallback = r600_fallback;
	radeon->vtbl.emit_query_finish = r600_emit_query_finish;
	radeon->vtbl.check_blit = r600_check_blit;
	radeon->vtbl.blit = r600_blit;
	radeon->vtbl.is_format_renderable = r600IsFormatRenderable;
}

static void r600InitConstValues(struct gl_context *ctx, radeonScreenPtr screen)
{
    context_t         *context = R700_CONTEXT(ctx);
    R700_CHIP_CONTEXT *r700    = (R700_CHIP_CONTEXT*)(&context->hw);

    if(  (context->radeon.radeonScreen->chip_family >= CHIP_FAMILY_CEDAR)
       &&(context->radeon.radeonScreen->chip_family <= CHIP_FAMILY_CAICOS) )
    {
        r700->bShaderUseMemConstant = GL_TRUE;
    }
    else
    {
        r700->bShaderUseMemConstant = GL_FALSE;
    }

        ctx->Const.GLSLVersion = 120;
        _mesa_override_glsl_version(ctx);

	ctx->Const.MaxTextureImageUnits = 16;
	/* 8 per clause on r6xx, 16 on r7xx
	 * but I think mesa only supports 8 at the moment
	 */
	ctx->Const.MaxTextureCoordUnits = 8;
	ctx->Const.MaxTextureUnits =
	    MIN2(ctx->Const.MaxTextureImageUnits,
		 ctx->Const.MaxTextureCoordUnits);
	ctx->Const.MaxCombinedTextureImageUnits =
		ctx->Const.MaxVertexTextureImageUnits +
		ctx->Const.MaxTextureImageUnits;

	ctx->Const.MaxTextureMaxAnisotropy = 16.0;
	ctx->Const.MaxTextureLodBias = 16.0;

	if (screen->chip_family >= CHIP_FAMILY_CEDAR) {
		ctx->Const.MaxTextureLevels = 15;
		ctx->Const.MaxTextureRectSize = 16384;
	} else {
		ctx->Const.MaxTextureLevels = 14;
		ctx->Const.MaxTextureRectSize = 8192;
	}

	ctx->Const.MinPointSize   = 0x0001 / 8.0;
	ctx->Const.MinPointSizeAA = 0x0001 / 8.0;
	ctx->Const.MaxPointSize   = 0xffff / 8.0;
	ctx->Const.MaxPointSizeAA = 0xffff / 8.0;

	ctx->Const.MinLineWidth   = 0x0001 / 8.0;
	ctx->Const.MinLineWidthAA = 0x0001 / 8.0;
	ctx->Const.MaxLineWidth   = 0xffff / 8.0;
	ctx->Const.MaxLineWidthAA = 0xffff / 8.0;

	ctx->Const.MaxDrawBuffers = 1; /* hw supports 8 */
	ctx->Const.MaxColorAttachments = 1;
	ctx->Const.MaxRenderbufferSize = 4096;

	/* 256 for reg-based consts, inline consts also supported */
	ctx->Const.VertexProgram.MaxInstructions = 8192; /* in theory no limit */
	ctx->Const.VertexProgram.MaxNativeInstructions = 8192;
	ctx->Const.VertexProgram.MaxNativeAttribs = 160;
	ctx->Const.VertexProgram.MaxTemps = 128;
	ctx->Const.VertexProgram.MaxNativeTemps = 128;
	ctx->Const.VertexProgram.MaxNativeParameters = 256;
	ctx->Const.VertexProgram.MaxNativeAddressRegs = 1; /* ??? */

	ctx->Const.FragmentProgram.MaxNativeTemps = 128;
	ctx->Const.FragmentProgram.MaxNativeAttribs = 32;
	ctx->Const.FragmentProgram.MaxNativeParameters = 256;
	ctx->Const.FragmentProgram.MaxNativeAluInstructions = 8192;
	/* 8 per clause on r6xx, 16 on r7xx */
	if (screen->chip_family >= CHIP_FAMILY_RV770)
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = 16;
	else
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = 8;
	ctx->Const.FragmentProgram.MaxNativeInstructions = 8192;
	ctx->Const.FragmentProgram.MaxNativeTexIndirections = 8; /* ??? */
	ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;	/* and these are?? */
}

static void r600ParseOptions(context_t *r600, radeonScreenPtr screen)
{
	/* Parse configuration files.
	 * Do this here so that initialMaxAnisotropy is set before we create
	 * the default textures.
	 */
	driParseConfigFiles(&r600->radeon.optionCache, &screen->optionCache,
			    screen->driScreen->myNum, "r600");

	r600->radeon.initialMaxAnisotropy = driQueryOptionf(&r600->radeon.optionCache,
							    "def_max_anisotropy");

}

static void r600InitGLExtensions(struct gl_context *ctx)
{
	context_t *r600 = R700_CONTEXT(ctx);
#ifdef R600_ENABLE_GLSL_TEST
	unsigned i;
#endif

	ctx->Extensions.ARB_depth_clamp = true;
	ctx->Extensions.ARB_depth_texture = true;
	ctx->Extensions.ARB_draw_elements_base_vertex = true;
	ctx->Extensions.ARB_fragment_program = true;
	ctx->Extensions.ARB_fragment_program_shadow = true;
	ctx->Extensions.ARB_occlusion_query = true;
	ctx->Extensions.ARB_shadow = true;
	ctx->Extensions.ARB_shadow_ambient = true;
	ctx->Extensions.ARB_texture_border_clamp = true;
	ctx->Extensions.ARB_texture_cube_map = true;
	ctx->Extensions.ARB_texture_env_combine = true;
	ctx->Extensions.ARB_texture_env_crossbar = true;
	ctx->Extensions.ARB_texture_env_dot3 = true;
	ctx->Extensions.ARB_texture_mirrored_repeat = true;
	ctx->Extensions.ARB_texture_non_power_of_two = true;
	ctx->Extensions.ARB_vertex_program = true;
	ctx->Extensions.EXT_blend_color = true;
	ctx->Extensions.EXT_blend_equation_separate = true;
	ctx->Extensions.EXT_blend_func_separate = true;
	ctx->Extensions.EXT_blend_minmax = true;
	ctx->Extensions.EXT_blend_subtract = true;
	ctx->Extensions.EXT_packed_depth_stencil = true;
	ctx->Extensions.EXT_fog_coord = true;
	ctx->Extensions.EXT_gpu_program_parameters = true;
	ctx->Extensions.EXT_pixel_buffer_object = true;
	ctx->Extensions.EXT_point_parameters = true;
	ctx->Extensions.EXT_provoking_vertex = true;
	ctx->Extensions.EXT_secondary_color = true;
	ctx->Extensions.EXT_shadow_funcs = true;
	ctx->Extensions.EXT_stencil_two_side = true;
	ctx->Extensions.EXT_stencil_wrap = true;
	ctx->Extensions.EXT_texture_env_dot3 = true;
	ctx->Extensions.EXT_texture_filter_anisotropic = true;
	ctx->Extensions.EXT_texture_lod_bias = true;
	ctx->Extensions.EXT_texture_mirror_clamp = true;
	ctx->Extensions.EXT_vertex_array_bgra = true;
	ctx->Extensions.EXT_texture_sRGB = true;
	ctx->Extensions.ATI_separate_stencil = true;
	ctx->Extensions.ATI_texture_env_combine3 = true;
	ctx->Extensions.ATI_texture_mirror_once = true;
	ctx->Extensions.MESA_pack_invert = true;
	ctx->Extensions.MESA_ycbcr_texture = true;
	ctx->Extensions.NV_blend_square = true;
	ctx->Extensions.NV_texture_rectangle = true;
	ctx->Extensions.NV_vertex_program = true;
#if FEATURE_OES_EGL_image
	ctx->Extensions.OES_EGL_image = true;
#endif

	if (r600->radeon.radeonScreen->kernel_mm)
		ctx->Extensions.EXT_framebuffer_object = true;

#ifdef R600_ENABLE_GLSL_TEST
	ctx->Extensions.ARB_shading_language_100 = true;
    _mesa_enable_2_0_extensions(ctx);
    
	/* glsl compiler has problem if this is not GL_TRUE */
	for (i = 0; i <= MESA_SHADER_FRAGMENT; i++)
		ctx->ShaderCompilerOptions[i].EmitCondCodes = GL_TRUE;
#endif /* R600_ENABLE_GLSL_TEST */

	if (driQueryOptionb
	    (&r600->radeon.optionCache, "disable_stencil_two_side"))
		ctx->Extensions.EXT_stencil_two_side = false;

	if (r600->radeon.glCtx->Mesa_DXTn
	    && !driQueryOptionb(&r600->radeon.optionCache, "disable_s3tc")) {
		ctx->Extensions.EXT_texture_compression_s3tc = true;
		ctx->Extensions.S3_s3tc = true;
	} else
	    if (driQueryOptionb(&r600->radeon.optionCache, "force_s3tc_enable"))
	{
		ctx->Extensions.EXT_texture_compression_s3tc = true;
	}

	/* RV740 had a broken pipe config prior to drm 1.32 */
	if (!r600->radeon.radeonScreen->kernel_mm) {
		if ((r600->radeon.dri.drmMinor < 32) &&
		    (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV740))
			ctx->Extensions.ARB_occlusion_query = false;
	}
}

/* Create the device specific rendering context.
 */
GLboolean r600CreateContext(gl_api api,
			    const struct gl_config * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreen *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	struct dd_function_table functions;
	context_t *r600;
	struct gl_context *ctx;

	assert(glVisual);
	assert(driContextPriv);
	assert(screen);

	/* Allocate the R600 context */
	r600 = (context_t*) CALLOC(sizeof(*r600));
	if (!r600) {
		radeon_error("Failed to allocate memory for context.\n");
		return GL_FALSE;
	}

	r600ParseOptions(r600, screen);

	r600->radeon.radeonScreen = screen;

    if(screen->chip_family >= CHIP_FAMILY_CEDAR)
    {
	    evergreen_init_vtbl(&r600->radeon);
    }
    else
    {
        r600_init_vtbl(&r600->radeon);
    }
    
	/* Init default driver functions then plug in our R600-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions(&functions);

    if(screen->chip_family >= CHIP_FAMILY_CEDAR)
    {
        evergreenCreateChip(r600);
        evergreenInitStateFuncs(&r600->radeon, &functions);
	    evergreenInitTextureFuncs(&r600->radeon, &functions);
	    evergreenInitShaderFuncs(&functions);
    }
    else
    {
	    r700InitStateFuncs(&r600->radeon, &functions);
	    r600InitTextureFuncs(&r600->radeon, &functions);
	    r700InitShaderFuncs(&functions);
    }
    
	radeonInitQueryObjFunctions(&functions);

    if(screen->chip_family >= CHIP_FAMILY_CEDAR)
    {
        evergreenInitIoctlFuncs(&functions);
    }
    else
    {
	    r700InitIoctlFuncs(&functions);
    }
	radeonInitBufferObjectFuncs(&functions);

	if (!radeonInitContext(&r600->radeon, &functions,
			       glVisual, driContextPriv,
			       sharedContextPrivate)) {
		radeon_error("Initializing context failed.\n");
		FREE(r600);
		return GL_FALSE;
	}

	ctx = r600->radeon.glCtx;

	ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;
	ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	r600InitConstValues(ctx, screen);

	/* reinit, it depends on consts above */
	_mesa_init_point(ctx);

	_mesa_set_mvp_with_dp4( ctx, GL_TRUE );

	/* Initialize the software rasterizer and helper modules.
	 */
	_swrast_CreateContext(ctx);
	_vbo_CreateContext(ctx);
	_tnl_CreateContext(ctx);
	_swsetup_CreateContext(ctx);
	_swsetup_Wakeup(ctx);

	/* Install the customized pipeline:
	 */
	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, r600_pipeline);
	TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

	radeon_init_debug();

    if(screen->chip_family >= CHIP_FAMILY_CEDAR)
    {
        evergreenInitDraw(ctx);
    }
    else
    {
	    r700InitDraw(ctx);
    }

	radeon_fbo_init(&r600->radeon);
   	radeonInitSpanFuncs( ctx );
	r600InitCmdBuf(r600);

    if(screen->chip_family >= CHIP_FAMILY_CEDAR)
    {
        evergreenInitState(r600->radeon.glCtx);
    }
    else
    {
	    r700InitState(r600->radeon.glCtx);
    }

	r600InitGLExtensions(ctx);

	return GL_TRUE;
}

void r600DestroyContext(__DRIcontext *driContextPriv )
{
    void      *pChip;
    context_t *context = (context_t *) driContextPriv->driverPrivate;

    assert(context);

    pChip = context->pChip;

    /* destroy context first, free pChip, in case there are things flush to asic. */
    radeonDestroyContext(driContextPriv);

    FREE(pChip);
}


