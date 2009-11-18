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
#include "main/matrix.h"
#include "main/extensions.h"
#include "main/state.h"
#include "main/bufferobj.h"
#include "main/texobj.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vp_build.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_debug.h"
#include "r600_context.h"
#include "radeon_common_context.h"
#include "radeon_span.h"
#include "r600_cmdbuf.h"
#include "r600_emit.h"
#include "radeon_bocs_wrapper.h"

#include "r700_state.h"
#include "r700_ioctl.h"


#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */

/* hw_tcl_on derives from future_hw_tcl_on when its safe to change it. */
int future_hw_tcl_on = 1;
int hw_tcl_on = 1;

#define need_GL_VERSION_2_0
#define need_GL_ARB_point_parameters
#define need_GL_ARB_vertex_program
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_fog_coord
#define need_GL_EXT_gpu_program_parameters
#define need_GL_EXT_secondary_color
#define need_GL_EXT_stencil_two_side
#define need_GL_ATI_separate_stencil
#define need_GL_NV_vertex_program

#include "extension_helper.h"

extern const struct tnl_pipeline_stage *r700_pipeline[];

const struct dri_extension card_extensions[] = {
  /* *INDENT-OFF* */
  {"GL_ARB_depth_texture",		NULL},
  {"GL_ARB_fragment_program",		NULL},
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
  {"GL_EXT_packed_depth_stencil",	NULL},
  {"GL_EXT_fog_coord",			GL_EXT_fog_coord_functions },
  {"GL_EXT_gpu_program_parameters",     GL_EXT_gpu_program_parameters_functions},
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


const struct dri_extension mm_extensions[] = {
  { "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
  { NULL, NULL }
};

/**
 * The GL 2.0 functions are needed to make display lists work with
 * functions added by GL_ATI_separate_stencil.
 */
const struct dri_extension gl_20_extension[] = {
  {"GL_VERSION_2_0",			GL_VERSION_2_0_functions },
};


static void r600RunPipeline(GLcontext * ctx)
{
    _mesa_lock_context_textures(ctx);

    if (ctx->NewState)
        _mesa_update_state_locked(ctx);
    
    _tnl_run_pipeline(ctx);
    _mesa_unlock_context_textures(ctx);
}

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

static void r600_init_vtbl(radeonContextPtr radeon)
{
	radeon->vtbl.get_lock = r600_get_lock;
	radeon->vtbl.update_viewport_offset = r700UpdateViewportOffset;
	radeon->vtbl.emit_cs_header = r600_vtbl_emit_cs_header;
	radeon->vtbl.swtcl_flush = NULL;
	radeon->vtbl.pre_emit_atoms = r600_vtbl_pre_emit_atoms;
	radeon->vtbl.fallback = r600_fallback;
}

/* Create the device specific rendering context.
 */
GLboolean r600CreateContext(const __GLcontextModes * glVisual,
			    __DRIcontextPrivate * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
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

	if (!(screen->chip_flags & RADEON_CHIPSET_TCL))
		hw_tcl_on = future_hw_tcl_on = 0;

	r600_init_vtbl(&r600->radeon);
	/* Parse configuration files.
	 * Do this here so that initialMaxAnisotropy is set before we create
	 * the default textures.
	 */
	driParseConfigFiles(&r600->radeon.optionCache, &screen->optionCache,
			    screen->driScreen->myNum, "r600");

	r600->radeon.initialMaxAnisotropy = driQueryOptionf(&r600->radeon.optionCache,
						     "def_max_anisotropy");

	/* Init default driver functions then plug in our R600-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions(&functions);

	r700InitStateFuncs(&functions);
	r600InitTextureFuncs(&functions);
	r700InitShaderFuncs(&functions);
	r700InitIoctlFuncs(&functions);

	if (!radeonInitContext(&r600->radeon, &functions,
			       glVisual, driContextPriv,
			       sharedContextPrivate)) {
		radeon_error("Initializing context failed.\n");
		FREE(r600);
		return GL_FALSE;
	}

	/* Init r600 context data */
	/* Set the maximum texture size small enough that we can guarentee that
	 * all texture units can bind a maximal texture and have them both in
	 * texturable memory at once.
	 */

	ctx = r600->radeon.glCtx;

	ctx->Const.MaxTextureImageUnits =
	    driQueryOptioni(&r600->radeon.optionCache, "texture_image_units");
	ctx->Const.MaxTextureCoordUnits =
	    driQueryOptioni(&r600->radeon.optionCache, "texture_coord_units");
	ctx->Const.MaxTextureUnits =
	    MIN2(ctx->Const.MaxTextureImageUnits,
		 ctx->Const.MaxTextureCoordUnits);
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

	/* Needs further modifications */
#if 0
	ctx->Const.MaxArrayLockSize =
	    ( /*512 */ RADEON_BUFFER_SIZE * 16 * 1024) / (4 * 4);
#endif

	ctx->Const.MaxDrawBuffers = 1;

	/* Initialize the software rasterizer and helper modules.
	 */
	_swrast_CreateContext(ctx);
	_vbo_CreateContext(ctx);
	_tnl_CreateContext(ctx);
	_swsetup_CreateContext(ctx);
	_swsetup_Wakeup(ctx);
	_ae_create_context(ctx);

	/* Install the customized pipeline:
	 */
	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, r700_pipeline);

	/* Try and keep materials and vertices separate:
	 */
/* 	_tnl_isolate_materials(ctx, GL_TRUE); */

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

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
	/* 8 per clause on r6xx, 16 on rv670/r7xx */
	if ((screen->chip_family == CHIP_FAMILY_RV670) ||
	    (screen->chip_family >= CHIP_FAMILY_RV770))
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = 16;
	else
		ctx->Const.FragmentProgram.MaxNativeTexInstructions = 8;
	ctx->Const.FragmentProgram.MaxNativeInstructions = 8192;
	ctx->Const.FragmentProgram.MaxNativeTexIndirections = 8; /* ??? */
	ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;	/* and these are?? */
	ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;
	ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	radeon_init_debug();

	driInitExtensions(ctx, card_extensions, GL_TRUE);
	if (r600->radeon.radeonScreen->kernel_mm)
	  driInitExtensions(ctx, mm_extensions, GL_FALSE);

	if (driQueryOptionb
	    (&r600->radeon.optionCache, "disable_stencil_two_side"))
		_mesa_disable_extension(ctx, "GL_EXT_stencil_two_side");

#if 0
	if (r600->radeon.glCtx->Mesa_DXTn
	    && !driQueryOptionb(&r600->radeon.optionCache, "disable_s3tc")) {
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
		_mesa_enable_extension(ctx, "GL_S3_s3tc");
	} else
	    if (driQueryOptionb(&r600->radeon.optionCache, "force_s3tc_enable"))
	{
		_mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
	}
#else
	_mesa_disable_extension(ctx, "GL_ARB_texture_compression");
#endif

	radeon_fbo_init(&r600->radeon);
   	radeonInitSpanFuncs( ctx );

	r600InitCmdBuf(r600);

	r700InitState(r600->radeon.glCtx);

	TNL_CONTEXT(ctx)->Driver.RunPipeline = r600RunPipeline;

	if (driQueryOptionb(&r600->radeon.optionCache, "no_rast")) {
		radeon_warning("disabling 3D acceleration\n");
#if R200_MERGED
		FALLBACK(&r600->radeon, RADEON_FALLBACK_DISABLE, 1);
#endif
	}

	return GL_TRUE;
}


