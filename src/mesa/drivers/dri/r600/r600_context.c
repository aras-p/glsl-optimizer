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

#include "r600_context.h"
#include "radeon_context.h"
#include "radeon_span.h"
#include "r600_cmdbuf.h"
#include "r600_state.h"
#include "r600_ioctl.h"
#include "r600_tex.h"
#include "r600_emit.h"
#include "r600_swtcl.h"
#include "radeon_bocs_wrapper.h"


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


extern struct tnl_pipeline_stage _r600_render_stage;
extern const struct tnl_pipeline_stage _r600_tcl_stage;

static const struct tnl_pipeline_stage *r600_pipeline[] = {

	/* Try and go straight to t&l
	 */
	&_r600_tcl_stage,

	/* Catch any t&l fallbacks
	 */
	&_tnl_vertex_transform_stage,
	&_tnl_normal_transform_stage,
	&_tnl_lighting_stage,
	&_tnl_fog_coordinate_stage,
	&_tnl_texgen_stage,
	&_tnl_texture_transform_stage,
	&_tnl_vertex_program_stage,

	/* Try again to go to tcl?
	 *     - no good for asymmetric-twoside (do with multipass)
	 *     - no good for asymmetric-unfilled (do with multipass)
	 *     - good for material
	 *     - good for texgen
	 *     - need to manipulate a bit of state
	 *
	 * - worth it/not worth it?
	 */

	/* Else do them here.
	 */
	&_r600_render_stage,
	&_tnl_render_stage,	/* FALLBACK  */
	0,
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
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_SC_SCREENDOOR, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_SC_SCREENDOOR, 1));
    radeon_cs_write_dword(cs, 0x00FFFFFF);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_SC_HYPERZ, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_US_CONFIG, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_ZB_CNTL, 1));
    radeon_cs_write_dword(cs, 0x0);
    radeon_cs_write_dword(cs, cmdwait(rmesa->radeonScreen, R300_WAIT_3D));
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_RB3D_DSTCACHE_CTLSTAT, 1));
    radeon_cs_write_dword(cs, R600_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
    radeon_cs_write_dword(cs, cmdpacket0(rmesa->radeonScreen,
                                  R600_ZB_ZCACHE_CTLSTAT, 1));
    radeon_cs_write_dword(cs, R600_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE);
    radeon_cs_write_dword(cs, cmdwait(rmesa->radeonScreen,
                               R300_WAIT_3D | R300_WAIT_3D_CLEAN));
}

static void r600_vtbl_pre_emit_atoms(radeonContextPtr radeon)
{
	r600ContextPtr r600 = (r600ContextPtr)radeon;
	BATCH_LOCALS(radeon);
	
	r600->vap_flush_needed = GL_TRUE;
	
	cp_wait(radeon, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
	BEGIN_BATCH_NO_AUTOSTATE(2);
	OUT_BATCH_REGVAL(R600_TX_INVALTAGS, R600_TX_FLUSH);
	END_BATCH();
	end_3d(radeon);
}

static void r600_fallback(GLcontext *ctx, GLuint bit, GLboolean mode)
{
	r600ContextPtr r600 = R600_CONTEXT(ctx);
	if (mode)
		r600->radeon.Fallback |= bit;
	else
		r600->radeon.Fallback &= ~bit;
}

static void r600_init_vtbl(radeonContextPtr radeon)
{
	radeon->vtbl.get_lock = r600_get_lock;
	radeon->vtbl.update_viewport_offset = r600UpdateViewportOffset;
	radeon->vtbl.emit_cs_header = r600_vtbl_emit_cs_header;
	radeon->vtbl.swtcl_flush = r600_swtcl_flush;
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
	r600ContextPtr r600;
	GLcontext *ctx;
	int tcl_mode;

	assert(glVisual);
	assert(driContextPriv);
	assert(screen);

	/* Allocate the R600 context */
	r600 = (r600ContextPtr) CALLOC(sizeof(*r600));
	if (!r600)
		return GL_FALSE;

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
	r600InitIoctlFuncs(&functions);
	r600InitStateFuncs(&functions);
	r600InitTextureFuncs(&functions);
	r600InitShaderFuncs(&functions);

	if (!radeonInitContext(&r600->radeon, &functions,
			       glVisual, driContextPriv,
			       sharedContextPrivate)) {
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

	ctx->Const.MaxTextureLevels = 13;
	ctx->Const.MaxTextureRectSize = 4096;

	ctx->Const.MinPointSize = 1.0;
	ctx->Const.MinPointSizeAA = 1.0;
	ctx->Const.MaxPointSize = R600_POINTSIZE_MAX;
	ctx->Const.MaxPointSizeAA = R600_POINTSIZE_MAX;

	ctx->Const.MinLineWidth = 1.0;
	ctx->Const.MinLineWidthAA = 1.0;
	ctx->Const.MaxLineWidth = R600_LINESIZE_MAX;
	ctx->Const.MaxLineWidthAA = R600_LINESIZE_MAX;

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
	_tnl_install_pipeline(ctx, r600_pipeline);

	/* Try and keep materials and vertices separate:
	 */
/* 	_tnl_isolate_materials(ctx, GL_TRUE); */

	/* Configure swrast and TNL to match hardware characteristics:
	 */
	_swrast_allow_pixel_fog(ctx, GL_FALSE);
	_swrast_allow_vertex_fog(ctx, GL_TRUE);
	_tnl_allow_pixel_fog(ctx, GL_FALSE);
	_tnl_allow_vertex_fog(ctx, GL_TRUE);

	/* currently bogus data */
	ctx->Const.VertexProgram.MaxInstructions = VSF_MAX_FRAGMENT_LENGTH / 4;
	ctx->Const.VertexProgram.MaxNativeInstructions =
		VSF_MAX_FRAGMENT_LENGTH / 4;
	ctx->Const.VertexProgram.MaxNativeAttribs = 16;	/* r420 */
	ctx->Const.VertexProgram.MaxTemps = 32;
	ctx->Const.VertexProgram.MaxNativeTemps =
		/*VSF_MAX_FRAGMENT_TEMPS */ 32;
	ctx->Const.VertexProgram.MaxNativeParameters = 256;	/* r420 */
	ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;

	ctx->Const.FragmentProgram.MaxNativeTemps = PFS_NUM_TEMP_REGS;
	ctx->Const.FragmentProgram.MaxNativeAttribs = 11;	/* copy i915... */
	ctx->Const.FragmentProgram.MaxNativeParameters = PFS_NUM_CONST_REGS;
	ctx->Const.FragmentProgram.MaxNativeAluInstructions = PFS_MAX_ALU_INST;
	ctx->Const.FragmentProgram.MaxNativeTexInstructions = PFS_MAX_TEX_INST;
	ctx->Const.FragmentProgram.MaxNativeInstructions =
	    PFS_MAX_ALU_INST + PFS_MAX_TEX_INST;
	ctx->Const.FragmentProgram.MaxNativeTexIndirections =
	    PFS_MAX_TEX_INDIRECT;
	ctx->Const.FragmentProgram.MaxNativeAddressRegs = 0;	/* and these are?? */
	ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;
	ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	driInitExtensions(ctx, card_extensions, GL_TRUE);
	if (r600->radeon.radeonScreen->kernel_mm)
	  driInitExtensions(ctx, mm_extensions, GL_FALSE);

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

	r600->disable_lowimpact_fallback =
	    driQueryOptionb(&r600->radeon.optionCache,
			    "disable_lowimpact_fallback");
	radeon_fbo_init(&r600->radeon);
   	radeonInitSpanFuncs( ctx );
	r600InitCmdBuf(r600);
	r600InitState(r600);
	if (!(screen->chip_flags & RADEON_CHIPSET_TCL))
	        r600InitSwtcl(ctx);

	TNL_CONTEXT(ctx)->Driver.RunPipeline = r600RunPipeline;

	tcl_mode = driQueryOptioni(&r600->radeon.optionCache, "tcl_mode");
	if (driQueryOptionb(&r600->radeon.optionCache, "no_rast")) {
		fprintf(stderr, "disabling 3D acceleration\n");
#if R200_MERGED
		FALLBACK(&r600->radeon, RADEON_FALLBACK_DISABLE, 1);
#endif
	}
	if (tcl_mode == DRI_CONF_TCL_SW ||
	    !(r600->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
		if (r600->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
			r600->radeon.radeonScreen->chip_flags &=
			    ~RADEON_CHIPSET_TCL;
			fprintf(stderr, "Disabling HW TCL support\n");
		}
		TCL_FALLBACK(r600->radeon.glCtx,
			     RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
	}

	return GL_TRUE;
}

