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
*/

/**
 * \file
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/mipmap.h"
#include "main/simple_list.h"
#include "main/texstore.h"
#include "main/texobj.h"

#include "texmem.h"

#include "r600_context.h"
#include "radeon_mipmap_tree.h"
#include "r600_tex.h"


static unsigned int translate_wrap_mode(GLenum wrapmode)
{
	switch(wrapmode) {
	case GL_REPEAT: return SQ_TEX_WRAP;
	case GL_CLAMP: return SQ_TEX_CLAMP_HALF_BORDER;
	case GL_CLAMP_TO_EDGE: return SQ_TEX_CLAMP_LAST_TEXEL;
	case GL_CLAMP_TO_BORDER: return SQ_TEX_CLAMP_BORDER;
	case GL_MIRRORED_REPEAT: return SQ_TEX_MIRROR;
	case GL_MIRROR_CLAMP_EXT: return SQ_TEX_MIRROR_ONCE_HALF_BORDER;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT: return SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT: return SQ_TEX_MIRROR_ONCE_BORDER;
	default:
		radeon_error("bad wrap mode in %s", __FUNCTION__);
		return 0;
	}
}


/**
 * Update the cached hardware registers based on the current texture wrap modes.
 *
 * \param t Texture object whose wrap modes are to be set
 */
static void r600UpdateTexWrap(radeonTexObjPtr t)
{
	struct gl_texture_object *tObj = &t->base;

        SETfield(t->SQ_TEX_SAMPLER0, translate_wrap_mode(tObj->WrapS),
                 SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_shift, SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_mask);

	if (tObj->Target != GL_TEXTURE_1D) {
		SETfield(t->SQ_TEX_SAMPLER0, translate_wrap_mode(tObj->WrapT),
			 CLAMP_Y_shift, CLAMP_Y_mask);

		if (tObj->Target == GL_TEXTURE_3D)
			SETfield(t->SQ_TEX_SAMPLER0, translate_wrap_mode(tObj->WrapR),
				 CLAMP_Z_shift, CLAMP_Z_mask);
	}
}

static void r600SetTexDefaultState(radeonTexObjPtr t)
{
        /* Init text object to default states. */
        t->SQ_TEX_RESOURCE0              = 0;
        SETfield(t->SQ_TEX_RESOURCE0, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);
        SETfield(t->SQ_TEX_RESOURCE0, ARRAY_LINEAR_GENERAL,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift, SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
        CLEARbit(t->SQ_TEX_RESOURCE0, TILE_TYPE_bit);

        t->SQ_TEX_RESOURCE1                = 0;
        SETfield(t->SQ_TEX_RESOURCE1, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        t->SQ_TEX_RESOURCE2                = 0;
        t->SQ_TEX_RESOURCE3                = 0;

        t->SQ_TEX_RESOURCE4                   = 0;
        SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
                 FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
                 FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
                 FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_FORMAT_COMP_UNSIGNED,
                 FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_NUM_FORMAT_NORM,
                 SQ_TEX_RESOURCE_WORD4_0__NUM_FORMAT_ALL_shift, SQ_TEX_RESOURCE_WORD4_0__NUM_FORMAT_ALL_mask);
        CLEARbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__SRF_MODE_ALL_bit);
        CLEARbit(t->SQ_TEX_RESOURCE4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_ENDIAN_NONE,
                 SQ_TEX_RESOURCE_WORD4_0__ENDIAN_SWAP_shift, SQ_TEX_RESOURCE_WORD4_0__ENDIAN_SWAP_mask);
        SETfield(t->SQ_TEX_RESOURCE4, 1, REQUEST_SIZE_shift, REQUEST_SIZE_mask);
        SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_X,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Y,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_Z,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	SETfield(t->SQ_TEX_RESOURCE4, SQ_SEL_W,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift,
		 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
        SETfield(t->SQ_TEX_RESOURCE4, 0, BASE_LEVEL_shift, BASE_LEVEL_mask); /* mip-maps */

        t->SQ_TEX_RESOURCE5 = 0;
        t->SQ_TEX_RESOURCE6 = 0;

        SETfield(t->SQ_TEX_RESOURCE6, SQ_TEX_VTX_VALID_TEXTURE,
                 SQ_TEX_RESOURCE_WORD6_0__TYPE_shift, SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

        /* Initialize sampler registers */
        t->SQ_TEX_SAMPLER0                           = 0;
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_WRAP, SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_shift,
		 SQ_TEX_SAMPLER_WORD0_0__CLAMP_X_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_WRAP, CLAMP_Y_shift, CLAMP_Y_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_WRAP, CLAMP_Z_shift, CLAMP_Z_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_XY_FILTER_POINT, XY_MAG_FILTER_shift, XY_MAG_FILTER_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_XY_FILTER_POINT, XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_Z_FILTER_NONE, Z_FILTER_shift, Z_FILTER_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_Z_FILTER_NONE, MIP_FILTER_shift, MIP_FILTER_mask);
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_BORDER_COLOR_TRANS_BLACK, BORDER_COLOR_TYPE_shift, BORDER_COLOR_TYPE_mask);

        t->SQ_TEX_SAMPLER1                           = 0;
        SETfield(t->SQ_TEX_SAMPLER1, 0x3ff, MAX_LOD_shift, MAX_LOD_mask);

        t->SQ_TEX_SAMPLER2                          = 0;
        SETbit(t->SQ_TEX_SAMPLER2, SQ_TEX_SAMPLER_WORD2_0__TYPE_bit);
}


#if 0
static GLuint aniso_filter(GLfloat anisotropy)
{
	if (anisotropy >= 16.0) {
		return R300_TX_MAX_ANISO_16_TO_1;
	} else if (anisotropy >= 8.0) {
		return R300_TX_MAX_ANISO_8_TO_1;
	} else if (anisotropy >= 4.0) {
		return R300_TX_MAX_ANISO_4_TO_1;
	} else if (anisotropy >= 2.0) {
		return R300_TX_MAX_ANISO_2_TO_1;
	} else {
		return R300_TX_MAX_ANISO_1_TO_1;
	}
	return 0;
}
#endif

/**
 * Set the texture magnification and minification modes.
 *
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 * \param anisotropy Maximum anisotropy level
 */
static void r600SetTexFilter(radeonTexObjPtr t, GLenum minf, GLenum magf, GLfloat anisotropy)
{
	/* Force revalidation to account for switches from/to mipmapping. */
	t->validated = GL_FALSE;

	/* Note that EXT_texture_filter_anisotropic is extremely vague about
	 * how anisotropic filtering interacts with the "normal" filter modes.
	 * When anisotropic filtering is enabled, we override min and mag
	 * filter settings completely. This includes driconf's settings.
	 */
	if (anisotropy >= 2.0 && (minf != GL_NEAREST) && (magf != GL_NEAREST)) {
		/*t->pp_txfilter |= R300_TX_MAG_FILTER_ANISO
			| R300_TX_MIN_FILTER_ANISO
			| R300_TX_MIN_FILTER_MIP_LINEAR
			| aniso_filter(anisotropy);*/
		radeon_print(RADEON_TEXTURE, RADEON_NORMAL, "Using maximum anisotropy of %f\n", anisotropy);
		return;
	}

	switch (minf) {
	case GL_NEAREST:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Point,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_None,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	case GL_LINEAR:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Linear,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_None,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Point,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_Point,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Point,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_Linear,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Linear,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_Point,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Linear,
			 XY_MIN_FILTER_shift, XY_MIN_FILTER_mask);
		SETfield(t->SQ_TEX_SAMPLER0, TEX_MipFilter_Linear,
			 MIP_FILTER_shift, MIP_FILTER_mask);
		break;
	}

	/* Note we don't have 3D mipmaps so only use the mag filter setting
	 * to set the 3D texture filter mode.
	 */
	switch (magf) {
	case GL_NEAREST:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Point,
			 XY_MAG_FILTER_shift, XY_MAG_FILTER_mask);
		break;
	case GL_LINEAR:
		SETfield(t->SQ_TEX_SAMPLER0, TEX_XYFilter_Linear,
			 XY_MAG_FILTER_shift, XY_MAG_FILTER_mask);
		break;
	}
}

static void r600SetTexBorderColor(radeonTexObjPtr t, const GLfloat color[4])
{
	t->TD_PS_SAMPLER0_BORDER_ALPHA = *((uint32_t*)&(color[3]));
	t->TD_PS_SAMPLER0_BORDER_RED = *((uint32_t*)&(color[2]));
	t->TD_PS_SAMPLER0_BORDER_GREEN = *((uint32_t*)&(color[1]));
	t->TD_PS_SAMPLER0_BORDER_BLUE = *((uint32_t*)&(color[0]));
        SETfield(t->SQ_TEX_SAMPLER0, SQ_TEX_BORDER_COLOR_REGISTER,
		 BORDER_COLOR_TYPE_shift, BORDER_COLOR_TYPE_mask);
}

/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void r600TexParameter(GLcontext * ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     GLenum pname, const GLfloat * params)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	GLenum baseFormat;

	radeon_print(RADEON_STATE | RADEON_TEXTURE, RADEON_VERBOSE,
			"%s( %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(pname));

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		r600SetTexFilter(t, texObj->MinFilter, texObj->MagFilter, texObj->MaxAnisotropy);
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
		r600UpdateTexWrap(t);
		break;

	case GL_TEXTURE_BORDER_COLOR:
		r600SetTexBorderColor(t, texObj->BorderColor.f);
		break;

	case GL_TEXTURE_BASE_LEVEL:
	case GL_TEXTURE_MAX_LEVEL:
	case GL_TEXTURE_MIN_LOD:
	case GL_TEXTURE_MAX_LOD:
		t->validated = GL_FALSE;
		break;

	case GL_DEPTH_TEXTURE_MODE:
		if (!texObj->Image[0][texObj->BaseLevel])
			return;
		baseFormat = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;
		if (baseFormat == GL_DEPTH_COMPONENT ||
		    baseFormat == GL_DEPTH_STENCIL) {
			r600SetDepthTexMode(texObj);
			break;
		} else {
			/* If the texture isn't a depth texture, changing this
			 * state won't cause any changes to the hardware.
			 * Don't force a flush of texture state.
			 */
			return;
		}

	default:
		return;
	}
}

static void r600DeleteTexture(GLcontext * ctx, struct gl_texture_object *texObj)
{
	context_t* rmesa = R700_CONTEXT(ctx);
	radeonTexObj* t = radeon_tex_obj(texObj);

	radeon_print(RADEON_STATE | RADEON_TEXTURE, RADEON_NORMAL,
		"%s( %p (target = %s) )\n", __FUNCTION__,
			(void *)texObj,
			_mesa_lookup_enum_by_nr(texObj->Target));

	if (rmesa) {
		int i;
		radeon_firevertices(&rmesa->radeon);

		for(i = 0; i < R700_MAX_TEXTURE_UNITS; ++i)
			if (rmesa->hw.textures[i] == t)
				rmesa->hw.textures[i] = 0;
	}

	if (t->bo) {
		radeon_bo_unref(t->bo);
		t->bo = NULL;
	}

	radeon_miptree_unreference(&t->mt);

	_mesa_delete_texture_object(ctx, texObj);
}

/**
 * Allocate a new texture object.
 * Called via ctx->Driver.NewTextureObject.
 * Note: this function will be called during context creation to
 * allocate the default texture objects.
 * Fixup MaxAnisotropy according to user preference.
 */
static struct gl_texture_object *r600NewTextureObject(GLcontext * ctx,
						      GLuint name,
						      GLenum target)
{
	context_t* rmesa = R700_CONTEXT(ctx);
	radeonTexObj* t = CALLOC_STRUCT(radeon_tex_obj);


	radeon_print(RADEON_STATE | RADEON_TEXTURE, RADEON_NORMAL,
		"%s( %p (target = %s) )\n", __FUNCTION__,
			t, _mesa_lookup_enum_by_nr(target));

	_mesa_initialize_texture_object(&t->base, name, target);
	t->base.MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;

	/* Initialize hardware state */
	r600SetTexDefaultState(t);
	r600UpdateTexWrap(t);
	r600SetTexFilter(t, t->base.MinFilter, t->base.MagFilter, t->base.MaxAnisotropy);
	r600SetTexBorderColor(t, t->base.BorderColor.f);

	return &t->base;
}

void r600InitTextureFuncs(radeonContextPtr radeon, struct dd_function_table *functions)
{
	/* Note: we only plug in the functions we implement in the driver
	 * since _mesa_init_driver_functions() was already called.
	 */
	functions->NewTextureImage = radeonNewTextureImage;
	functions->FreeTexImageData = radeonFreeTexImageData;
	functions->MapTexture = radeonMapTexture;
	functions->UnmapTexture = radeonUnmapTexture;

	functions->ChooseTextureFormat = radeonChooseTextureFormat_mesa;
	functions->TexImage1D = radeonTexImage1D;
	functions->TexImage2D = radeonTexImage2D;
	functions->TexImage3D = radeonTexImage3D;
	functions->TexSubImage1D = radeonTexSubImage1D;
	functions->TexSubImage2D = radeonTexSubImage2D;
	functions->TexSubImage3D = radeonTexSubImage3D;
	functions->GetTexImage = radeonGetTexImage;
	functions->GetCompressedTexImage = radeonGetCompressedTexImage;
	functions->NewTextureObject = r600NewTextureObject;
	functions->DeleteTexture = r600DeleteTexture;
	functions->IsTextureResident = driIsTextureResident;

	functions->TexParameter = r600TexParameter;

	functions->CompressedTexImage2D = radeonCompressedTexImage2D;
	functions->CompressedTexSubImage2D = radeonCompressedTexSubImage2D;

	if (radeon->radeonScreen->kernel_mm) {
		functions->CopyTexImage2D = radeonCopyTexImage2D;
		functions->CopyTexSubImage2D = radeonCopyTexSubImage2D;
	}

	functions->GenerateMipmap = radeonGenerateMipmap;

	driInitTextureFormats();
}
