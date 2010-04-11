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
#include "drivers/common/meta.h"

#include "r300_context.h"
#include "radeon_span.h"
#include "r300_blit.h"
#include "r300_cmdbuf.h"
#include "r300_state.h"
#include "r300_tex.h"
#include "r300_emit.h"
#include "r300_render.h"
#include "r300_swtcl.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_buffer_objects.h"
#include "radeon_queryobj.h"

#include "utils.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */

#define need_GL_VERSION_2_0
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_vertex_program
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_framebuffer_blit
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
  {"GL_ARB_depth_texture",		NULL},
  {"GL_ARB_fragment_program",		NULL},
  {"GL_ARB_occlusion_query",		GL_ARB_occlusion_query_functions},
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
  {"GL_ARB_vertex_program",		GL_ARB_vertex_program_functions},
  {"GL_EXT_blend_equation_separate",	GL_EXT_blend_equation_separate_functions},
  {"GL_EXT_blend_func_separate",	GL_EXT_blend_func_separate_functions},
  {"GL_EXT_blend_minmax",		GL_EXT_blend_minmax_functions},
  {"GL_EXT_blend_subtract",		NULL},
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
  {"GL_EXT_texture_sRGB",		NULL},
  {"GL_EXT_vertex_array_bgra",		NULL},
  {"GL_ATI_separate_stencil",		GL_ATI_separate_stencil_functions},
  {"GL_ATI_texture_env_combine3",	NULL},
  {"GL_ATI_texture_mirror_once",	NULL},
  {"GL_MESA_pack_invert",		NULL},
  {"GL_MESA_ycbcr_texture",		NULL},
  {"GL_MESAX_texture_float",		NULL},
  {"GL_NV_blend_square",		NULL},
  {"GL_NV_vertex_program",		GL_NV_vertex_program_functions},
  {"GL_SGIS_generate_mipmap",		NULL},
  {NULL,				NULL}
  /* *INDENT-ON* */
};


static const struct dri_extension mm_extensions[] = {
  { "GL_EXT_framebuffer_blit",	GL_EXT_framebuffer_blit_functions },
  { "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
  { NULL, NULL }
};

/**
 * The GL 2.0 functions are needed to make display lists work with
 * functions added by GL_ATI_separate_stencil.
 */
static const struct dri_extension gl_20_extension[] = {
  {"GL_VERSION_2_0",			GL_VERSION_2_0_functions },
};

static const struct tnl_pipeline_stage *r300_pipeline[] = {
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

static void r300_get_lock(radeonContextPtr rmesa)
{
	drm_radeon_sarea_t *sarea = rmesa->sarea;

	if (sarea->ctx_owner != rmesa->dri.hwContext) {
		sarea->ctx_owner = rmesa->dri.hwContext;
		if (!rmesa->radeonScreen->kernel_mm)
			radeon_bo_legacy_texture_age(rmesa->radeonScreen->bom);
	}
}

static void r300_vtbl_emit_cs_header(struct radeon_cs *cs, radeonContextPtr rmesa)
{
    /* please flush pipe do all pending work */
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_SC_SCREENDOOR, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_SC_SCREENDOOR, 1));
    radeon_cs_write_dword(cs, 0x00FFFFFF);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_SC_HYPERZ, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_US_CONFIG, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_ZB_CNTL, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdwait(rmesa->radeonScreen, R300_WAIT_3D));
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_RB3D_DSTCACHE_CTLSTAT, 1));
    radeon_cs_write_dword(cs, R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R300_ZB_ZCACHE_CTLSTAT, 1));
    radeon_cs_write_dword(cs, R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE);
    radeon_cs_write_dword(cs, cmdwait(rmesa->radeonScreen,
                               R300_WAIT_3D | R300_WAIT_3D_CLEAN));
}

static void r300_vtbl_pre_emit_atoms(radeonContextPtr radeon)
{
	BATCH_LOCALS(radeon);

	cp_wait(radeon, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	BEGIN_BATCH_NO_AUTOSTATE(2);
	OUT_BATCH_REGVAL(R300_TX_INVALTAGS, R300_TX_FLUSH);
	END_BATCH();
	end_3d(radeon);
}

static void r300_fallback(GLcontext *ctx, GLuint bit, GLboolean mode)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	if (mode)
		r300->radeon.Fallback |= bit;
	else
		r300->radeon.Fallback &= ~bit;

	r300SwitchFallback(ctx, R300_FALLBACK_RADEON_COMMON, mode);
}

static void r300_emit_query_finish(radeonContextPtr radeon)
{
	r300ContextPtr r300 = (r300ContextPtr)radeon;
	struct radeon_query_object *query = radeon->query.current;
	BATCH_LOCALS(radeon);

	BEGIN_BATCH_NO_AUTOSTATE(3 * 2 *r300->radeon.radeonScreen->num_gb_pipes + 2);
	switch (r300->radeon.radeonScreen->num_gb_pipes) {
	case 4:
		OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_3);
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset+3*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
	case 3:
		OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_2);
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset+2*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
	case 2:
		if (r300->radeon.radeonScreen->chip_family <= CHIP_FAMILY_RV380) {
			OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_3);
		} else {
			OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_1);
		}
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset+1*sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
	case 1:
	default:
		OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_0);
		OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
		OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
		break;
	}
	OUT_BATCH_REGVAL(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_ALL);
	END_BATCH();
	query->curr_offset += r300->radeon.radeonScreen->num_gb_pipes * sizeof(uint32_t);
	assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

static void rv530_emit_query_finish_single_z(radeonContextPtr radeon)
{
	BATCH_LOCALS(radeon);
	struct radeon_query_object *query = radeon->query.current;

	BEGIN_BATCH_NO_AUTOSTATE(8);
	OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
	OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
	OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
	OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
	END_BATCH();

	query->curr_offset += sizeof(uint32_t);
	assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

static void rv530_emit_query_finish_double_z(radeonContextPtr radeon)
{
	BATCH_LOCALS(radeon);
	struct radeon_query_object *query = radeon->query.current;

	BEGIN_BATCH_NO_AUTOSTATE(14);
	OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
	OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
	OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
	OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_1);
	OUT_BATCH_REGSEQ(R300_ZB_ZPASS_ADDR, 1);
	OUT_BATCH_RELOC(0, query->bo, query->curr_offset + sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
	OUT_BATCH_REGVAL(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
	END_BATCH();

	query->curr_offset += 2 * sizeof(uint32_t);
	assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

static void r300_init_vtbl(radeonContextPtr radeon)
{
	radeon->vtbl.get_lock = r300_get_lock;
	radeon->vtbl.update_viewport_offset = r300UpdateViewportOffset;
	radeon->vtbl.emit_cs_header = r300_vtbl_emit_cs_header;
	radeon->vtbl.swtcl_flush = r300_swtcl_flush;
	radeon->vtbl.pre_emit_atoms = r300_vtbl_pre_emit_atoms;
	radeon->vtbl.fallback = r300_fallback;
	if (radeon->radeonScreen->chip_family == CHIP_FAMILY_RV530) {
		if (radeon->radeonScreen->num_z_pipes == 2)
			radeon->vtbl.emit_query_finish = rv530_emit_query_finish_double_z;
		else
			radeon->vtbl.emit_query_finish = rv530_emit_query_finish_single_z;
	} else
		radeon->vtbl.emit_query_finish = r300_emit_query_finish;

	radeon->vtbl.check_blit = r300_check_blit;
	radeon->vtbl.blit = r300_blit;
}

static void r300InitConstValues(GLcontext *ctx, radeonScreenPtr screen)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	ctx->Const.MaxTextureImageUnits =
	    driQueryOptioni(&r300->radeon.optionCache, "texture_image_units");
	ctx->Const.MaxTextureCoordUnits =
	    driQueryOptioni(&r300->radeon.optionCache, "texture_coord_units");
	ctx->Const.MaxTextureUnits = MIN2(ctx->Const.MaxTextureImageUnits,
		 ctx->Const.MaxTextureCoordUnits);
	ctx->Const.MaxCombinedTextureImageUnits =
		ctx->Const.MaxVertexTextureImageUnits +
		ctx->Const.MaxTextureImageUnits;


	ctx->Const.MaxTextureMaxAnisotropy = 16.0;
	ctx->Const.MaxTextureLodBias = 16.0;

	if (screen->chip_family >= CHIP_FAMILY_RV515) {
		ctx->Const.MaxTextureLevels = 13;
		ctx->Const.MaxCubeTextureLevels = 13;
		ctx->Const.MaxTextureRectSize = 4096;
		ctx->Const.MaxRenderbufferSize = 4096;
	}
	else {
		ctx->Const.MaxTextureLevels = 12;
		ctx->Const.MaxCubeTextureLevels = 12;
		ctx->Const.MaxTextureRectSize = 2048;
		ctx->Const.MaxRenderbufferSize = 2048;
	}

	ctx->Const.MinPointSize = 1.0;
	ctx->Const.MinPointSizeAA = 1.0;
	ctx->Const.MaxPointSize = R300_POINTSIZE_MAX;
	ctx->Const.MaxPointSizeAA = R300_POINTSIZE_MAX;

	ctx->Const.MinLineWidth = 1.0;
	ctx->Const.MinLineWidthAA = 1.0;
	ctx->Const.MaxLineWidth = R300_LINESIZE_MAX;
	ctx->Const.MaxLineWidthAA = R300_LINESIZE_MAX;

	ctx->Const.MaxDrawBuffers = 1;
	ctx->Const.MaxColorAttachments = 1;

	/* currently bogus data */
	if (r300->options.hw_tcl_enabled) {
		ctx->Const.VertexProgram.MaxNativeInstructions = VSF_MAX_FRAGMENT_LENGTH / 4;
		ctx->Const.VertexProgram.MaxNativeAluInstructions = VSF_MAX_FRAGMENT_LENGTH / 4;
		ctx->Const.VertexProgram.MaxNativeAttribs = 16;	/* r420 */
		ctx->Const.VertexProgram.MaxNativeTemps = 32;
		ctx->Const.VertexProgram.MaxNativeParameters = 256;	/* r420 */
		ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;
	}

	if (screen->chip_family >= CHIP_FAMILY_RV515) {
		ctx->Const.FragmentProgram.MaxNativeTemps = R500_PFS_NUM_TEMP_REGS;
		ctx->Const.FragmentProgram.MaxNativeAttribs = 11;	/* copy i915... */

		/* The hardware limits are higher than this,
		 * but the non-KMS DRM interface artificially limits us
		 * to this many instructions.
		 *
		 * We could of course work around it in the KMS path,
		 * but it would be a mess, so it seems wiser
		 * to leave it as is. Going forward, the Gallium driver
		 * will not be subject to these limitations.
		 */
		ctx->Const.FragmentProgram.MaxNativeParameters = 255;
		ctx->Const.FragmentProgram.MaxNativeAluInstructions = 255;
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = 255;
		ctx->Const.FragmentProgram.MaxNativeInstructions = 255;
		ctx->Const.FragmentProgram.MaxNativeTexIndirections = 255;
		ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;
	} else {
		ctx->Const.FragmentProgram.MaxNativeTemps = R300_PFS_NUM_TEMP_REGS;
		ctx->Const.FragmentProgram.MaxNativeAttribs = 11;	/* copy i915... */
		ctx->Const.FragmentProgram.MaxNativeParameters = R300_PFS_NUM_CONST_REGS;
		ctx->Const.FragmentProgram.MaxNativeAluInstructions = R300_PFS_MAX_ALU_INST;
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = R300_PFS_MAX_TEX_INST;
		ctx->Const.FragmentProgram.MaxNativeInstructions = R300_PFS_MAX_ALU_INST + R300_PFS_MAX_TEX_INST;
		ctx->Const.FragmentProgram.MaxNativeTexIndirections = R300_PFS_MAX_TEX_INDIRECT;
		ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;
	}

}

static void r300ParseOptions(r300ContextPtr r300, radeonScreenPtr screen)
{
	struct r300_options options = { 0 };

	driParseConfigFiles(&r300->radeon.optionCache, &screen->optionCache,
			    screen->driScreen->myNum, "r300");

	r300->radeon.initialMaxAnisotropy = driQueryOptionf(&r300->radeon.optionCache, "def_max_anisotropy");

	options.stencil_two_side_disabled = driQueryOptionb(&r300->radeon.optionCache, "disable_stencil_two_side");
	options.s3tc_force_enabled = driQueryOptionb(&r300->radeon.optionCache, "force_s3tc_enable");
	options.s3tc_force_disabled = driQueryOptionb(&r300->radeon.optionCache, "disable_s3tc");

	if (!(screen->chip_flags & RADEON_CHIPSET_TCL) || driQueryOptioni(&r300->radeon.optionCache, "tcl_mode") == DRI_CONF_TCL_SW)
		options.hw_tcl_enabled = 0;
	else
		options.hw_tcl_enabled = 1;

	options.conformance_mode = !driQueryOptionb(&r300->radeon.optionCache, "disable_lowimpact_fallback");

	r300->options = options;
}

static void r300InitGLExtensions(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);

	driInitExtensions(ctx, card_extensions, GL_TRUE);
	if (r300->radeon.radeonScreen->kernel_mm)
		driInitExtensions(ctx, mm_extensions, GL_FALSE);

	if (r300->options.stencil_two_side_disabled)
		_mesa_disable_extension(ctx, "GL_EXT_stencil_two_side");

	if (r300->options.s3tc_force_disabled) {
		_mesa_disable_extension(ctx, "GL_EXT_texture_compression_s3tc");
	} else if (ctx->Mesa_DXTn || r300->options.s3tc_force_enabled) {
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
		_mesa_enable_extension(ctx, "GL_S3_s3tc");
	}

	if (!r300->radeon.radeonScreen->drmSupportsOcclusionQueries) {
		_mesa_disable_extension(ctx, "GL_ARB_occlusion_query");
	}
	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV350)
  		_mesa_enable_extension(ctx, "GL_ARB_half_float_vertex");

	if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515)
		_mesa_enable_extension(ctx, "GL_EXT_packed_depth_stencil");
}

static void r300InitIoctlFuncs(struct dd_function_table *functions)
{
	functions->Clear = _mesa_meta_Clear;
	functions->Finish = radeonFinish;
	functions->Flush = radeonFlush;
}

/* Create the device specific rendering context.
 */
GLboolean r300CreateContext(const __GLcontextModes * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreen *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	struct dd_function_table functions;
	r300ContextPtr r300;
	GLcontext *ctx;

	assert(glVisual);
	assert(driContextPriv);
	assert(screen);

	r300 = (r300ContextPtr) CALLOC(sizeof(*r300));
	if (!r300)
		return GL_FALSE;

	r300ParseOptions(r300, screen);

	r300->radeon.radeonScreen = screen;
	r300_init_vtbl(&r300->radeon);

	_mesa_init_driver_functions(&functions);
	r300InitIoctlFuncs(&functions);
	r300InitStateFuncs(&functions);
	r300InitTextureFuncs(&r300->radeon, &functions);
	r300InitShaderFuncs(&functions);
	radeonInitQueryObjFunctions(&functions);
	radeonInitBufferObjectFuncs(&functions);

	if (!radeonInitContext(&r300->radeon, &functions,
			       glVisual, driContextPriv,
			       sharedContextPrivate)) {
		FREE(r300);
		return GL_FALSE;
	}

	ctx = r300->radeon.glCtx;

	r300->fallback = 0;
	if (r300->options.hw_tcl_enabled)
		ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;

	ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	r300InitConstValues(ctx, screen);

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
	_tnl_install_pipeline(ctx, r300_pipeline);
	TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

	if (r300->options.hw_tcl_enabled) {
		r300InitDraw(ctx);
	} else {
		r300InitSwtcl(ctx);
	}

	r300_blit_init(r300);
	radeon_fbo_init(&r300->radeon);
	radeonInitSpanFuncs( ctx );
	r300InitCmdBuf(r300);
	r300InitState(r300);
	r300InitShaderFunctions(r300);

	r300InitGLExtensions(ctx);

	return GL_TRUE;
}

