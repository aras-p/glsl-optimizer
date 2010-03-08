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

#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/extensions.h"
#include "main/bufferobj.h"
#include "main/texobj.h"

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
#include "r600_emit.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_queryobj.h"
#include "r600_blit.h"

#include "r700_state.h"
#include "r700_ioctl.h"


#include "utils.h"

#define R600_ENABLE_GLSL_TEST 1

#define need_GL_VERSION_2_0
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_vertex_program
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_fog_coord
#define need_GL_EXT_gpu_program_parameters
#define need_GL_EXT_provoking_vertex
#define need_GL_EXT_secondary_color
#define need_GL_EXT_stencil_two_side
#define need_GL_ATI_separate_stencil
#define need_GL_NV_vertex_program

#include "main/remap_helper.h"

static const struct dri_extension card_extensions[] = {
  /* *INDENT-OFF* */
  {"GL_ARB_depth_clamp",                NULL},
  {"GL_ARB_depth_texture",		NULL},
  {"GL_ARB_fragment_program",		NULL},
  {"GL_ARB_fragment_program_shadow",	NULL},
  {"GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions},
  {"GL_ARB_multitexture",		NULL},
  {"GL_ARB_point_parameters",		GL_ARB_point_parameters_functions},
  {"GL_ARB_shadow",			NULL},
  {"GL_ARB_shadow_ambient",		NULL},
  {"GL_ARB_texture_border_clamp",	NULL},
  {"GL_ARB_texture_cube_map",		NULL},
  {"GL_ARB_texture_env_add",		NULL},
  {"GL_ARB_texture_env_combine",	NULL},
  {"GL_ARB_texture_env_crossbar",	NULL},
  {"GL_ARB_texture_env_dot3",		NULL},
  {"GL_ARB_texture_mirrored_repeat",	NULL},
  {"GL_ARB_texture_non_power_of_two",   NULL},
  {"GL_ARB_vertex_program",		GL_ARB_vertex_program_functions},
  {"GL_EXT_blend_equation_separate",	GL_EXT_blend_equation_separate_functions},
  {"GL_EXT_blend_func_separate",	GL_EXT_blend_func_separate_functions},
  {"GL_EXT_blend_minmax",		GL_EXT_blend_minmax_functions},
  {"GL_EXT_blend_subtract",		NULL},
  {"GL_EXT_packed_depth_stencil",	NULL},
  {"GL_EXT_fog_coord",			GL_EXT_fog_coord_functions },
  {"GL_EXT_gpu_program_parameters",     GL_EXT_gpu_program_parameters_functions},
  {"GL_EXT_provoking_vertex",           GL_EXT_provoking_vertex_functions },
  {"GL_EXT_secondary_color", 		GL_EXT_secondary_color_functions},
  {"GL_EXT_shadow_funcs",		NULL},
  {"GL_EXT_stencil_two_side",		GL_EXT_stencil_two_side_functions},
  {"GL_EXT_stencil_wrap",		NULL},
  {"GL_EXT_texture_edge_clamp",		NULL},
  {"GL_EXT_texture_env_combine", 	NULL},
  {"GL_EXT_texture_env_dot3", 		NULL},
  {"GL_EXT_texture_filter_anisotropic",	NULL},
  {"GL_EXT_texture_lod_bias",		NULL},
  {"GL_EXT_texture_mirror_clamp",	NULL},
  {"GL_EXT_texture_rectangle",		NULL},
  {"GL_EXT_vertex_array_bgra",          NULL},
  {"GL_EXT_texture_sRGB",               NULL},
  {"GL_ATI_separate_stencil",		GL_ATI_separate_stencil_functions},
  {"GL_ATI_texture_env_combine3",	NULL},
  {"GL_ATI_texture_mirror_once",	NULL},
  {"GL_MESA_pack_invert",		NULL},
  {"GL_MESA_ycbcr_texture",		NULL},
  {"GL_MESAX_texture_float",		NULL},
  {"GL_NV_blend_square",		NULL},
  {"GL_NV_vertex_program",		GL_NV_vertex_program_functions},
  {"GL_SGIS_generate_mipmap",		NULL},
  {"GL_ARB_pixel_buffer_object",        NULL},
  {NULL,				NULL}
  /* *INDENT-ON* */
};


static const struct dri_extension mm_extensions[] = {
  { "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
  { NULL, NULL }
};

/**
 * The GL 2.0 functions are needed to make display lists work with
 * functions added by GL_ATI_separate_stencil.
 */
static const struct dri_extension gl_20_extension[] = {
#ifdef R600_ENABLE_GLSL_TEST
    {"GL_ARB_shading_language_100",			GL_VERSION_2_0_functions },
#else
  {"GL_VERSION_2_0",			GL_VERSION_2_0_functions },
#endif /* R600_ENABLE_GLSL_TEST */
  {NULL, NULL}
};

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

static void r600_fallback(GLcontext *ctx, GLuint bit, GLboolean mode)
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
	R600_OUT_BATCH(ZPASS_DONE);
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
}

static void r600InitConstValues(GLcontext *ctx, radeonScreenPtr screen)
{
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

	ctx->Const.MaxTextureLevels = 13; /* hw support 14 */
	ctx->Const.MaxTextureRectSize = 4096; /* hw support 8192 */

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

static void r600InitGLExtensions(GLcontext *ctx)
{
	context_t *r600 = R700_CONTEXT(ctx);

	driInitExtensions(ctx, card_extensions, GL_TRUE);
	if (r600->radeon.radeonScreen->kernel_mm)
	  driInitExtensions(ctx, mm_extensions, GL_FALSE);

#ifdef R600_ENABLE_GLSL_TEST
    driInitExtensions(ctx, gl_20_extension, GL_TRUE);
    _mesa_enable_2_0_extensions(ctx);
    
    /* glsl compiler has problem if this is not GL_TRUE */
    ctx->Shader.EmitCondCodes = GL_TRUE;
#endif /* R600_ENABLE_GLSL_TEST */

	if (driQueryOptionb
	    (&r600->radeon.optionCache, "disable_stencil_two_side"))
		_mesa_disable_extension(ctx, "GL_EXT_stencil_two_side");

	if (r600->radeon.glCtx->Mesa_DXTn
	    && !driQueryOptionb(&r600->radeon.optionCache, "disable_s3tc")) {
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
		_mesa_enable_extension(ctx, "GL_S3_s3tc");
	} else
	    if (driQueryOptionb(&r600->radeon.optionCache, "force_s3tc_enable"))
	{
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
	}

	/* RV740 had a broken pipe config prior to drm 1.32 */
	if (!r600->radeon.radeonScreen->kernel_mm) {
		if ((r600->radeon.dri.drmMinor < 32) &&
		    (r600->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV740))
			_mesa_disable_extension(ctx, "GL_ARB_occlusion_query");
	}
}

/* Create the device specific rendering context.
 */
GLboolean r600CreateContext(const __GLcontextModes * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreen *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	struct dd_function_table functions;
	context_t *r600;
	GLcontext *ctx;

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
	r600_init_vtbl(&r600->radeon);

	/* Init default driver functions then plug in our R600-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions(&functions);

	r700InitStateFuncs(&functions);
	r600InitTextureFuncs(&r600->radeon, &functions);
	r700InitShaderFuncs(&functions);
	radeonInitQueryObjFunctions(&functions);
	r700InitIoctlFuncs(&functions);
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

	r700InitDraw(ctx);

	radeon_fbo_init(&r600->radeon);
   	radeonInitSpanFuncs( ctx );
	r600InitCmdBuf(r600);
	r700InitState(r600->radeon.glCtx);

	r600InitGLExtensions(ctx);

	return GL_TRUE;
}


