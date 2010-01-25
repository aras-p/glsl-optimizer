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

#include "r300_context.h"
#include "radeon_mipmap_tree.h"
#include "r300_tex.h"


static unsigned int translate_wrap_mode(GLenum wrapmode)
{
	switch(wrapmode) {
	case GL_REPEAT: return R300_TX_REPEAT;
	case GL_CLAMP: return R300_TX_CLAMP;
	case GL_CLAMP_TO_EDGE: return R300_TX_CLAMP_TO_EDGE;
	case GL_CLAMP_TO_BORDER: return R300_TX_CLAMP_TO_BORDER;
	case GL_MIRRORED_REPEAT: return R300_TX_REPEAT | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_EXT: return R300_TX_CLAMP | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_TO_EDGE_EXT: return R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
	case GL_MIRROR_CLAMP_TO_BORDER_EXT: return R300_TX_CLAMP_TO_BORDER | R300_TX_MIRRORED;
	default:
		_mesa_problem(NULL, "bad wrap mode in %s", __FUNCTION__);
		return 0;
	}
}


/**
 * Update the cached hardware registers based on the current texture wrap modes.
 *
 * \param t Texture object whose wrap modes are to be set
 */
static void r300UpdateTexWrap(radeonTexObjPtr t)
{
	struct gl_texture_object *tObj = &t->base;

	t->pp_txfilter &=
	    ~(R300_TX_WRAP_S_MASK | R300_TX_WRAP_T_MASK | R300_TX_WRAP_R_MASK);

	t->pp_txfilter |= translate_wrap_mode(tObj->WrapS) << R300_TX_WRAP_S_SHIFT;

	if (tObj->Target != GL_TEXTURE_1D) {
		t->pp_txfilter |= translate_wrap_mode(tObj->WrapT) << R300_TX_WRAP_T_SHIFT;

		if (tObj->Target == GL_TEXTURE_3D)
			t->pp_txfilter |= translate_wrap_mode(tObj->WrapR) << R300_TX_WRAP_R_SHIFT;
	}
}

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
}

/**
 * Set the texture magnification and minification modes.
 *
 * \param t Texture whose filter modes are to be set
 * \param minf Texture minification mode
 * \param magf Texture magnification mode
 * \param anisotropy Maximum anisotropy level
 */
static void r300SetTexFilter(radeonTexObjPtr t, GLenum minf, GLenum magf, GLfloat anisotropy)
{
	/* Force revalidation to account for switches from/to mipmapping. */
	t->validated = GL_FALSE;

	t->pp_txfilter &= ~(R300_TX_MIN_FILTER_MASK | R300_TX_MIN_FILTER_MIP_MASK | R300_TX_MAG_FILTER_MASK | R300_TX_MAX_ANISO_MASK);
	t->pp_txfilter_1 &= ~R300_EDGE_ANISO_EDGE_ONLY;

	/* Note that EXT_texture_filter_anisotropic is extremely vague about
	 * how anisotropic filtering interacts with the "normal" filter modes.
	 * When anisotropic filtering is enabled, we override min and mag
	 * filter settings completely. This includes driconf's settings.
	 */
	if (anisotropy >= 2.0 && (minf != GL_NEAREST) && (magf != GL_NEAREST)) {
		t->pp_txfilter |= R300_TX_MAG_FILTER_ANISO
			| R300_TX_MIN_FILTER_ANISO
			| R300_TX_MIN_FILTER_MIP_LINEAR
			| aniso_filter(anisotropy);
		if (RADEON_DEBUG & RADEON_TEXTURE)
			fprintf(stderr, "Using maximum anisotropy of %f\n", anisotropy);
		return;
	}

	switch (minf) {
	case GL_NEAREST:
		t->pp_txfilter |= R300_TX_MIN_FILTER_NEAREST;
		break;
	case GL_LINEAR:
		t->pp_txfilter |= R300_TX_MIN_FILTER_LINEAR;
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		t->pp_txfilter |= R300_TX_MIN_FILTER_NEAREST|R300_TX_MIN_FILTER_MIP_NEAREST;
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		t->pp_txfilter |= R300_TX_MIN_FILTER_NEAREST|R300_TX_MIN_FILTER_MIP_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		t->pp_txfilter |= R300_TX_MIN_FILTER_LINEAR|R300_TX_MIN_FILTER_MIP_NEAREST;
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		t->pp_txfilter |= R300_TX_MIN_FILTER_LINEAR|R300_TX_MIN_FILTER_MIP_LINEAR;
		break;
	}

	/* Note we don't have 3D mipmaps so only use the mag filter setting
	 * to set the 3D texture filter mode.
	 */
	switch (magf) {
	case GL_NEAREST:
		t->pp_txfilter |= R300_TX_MAG_FILTER_NEAREST;
		break;
	case GL_LINEAR:
		t->pp_txfilter |= R300_TX_MAG_FILTER_LINEAR;
		break;
	}
}

static void r300SetTexBorderColor(radeonTexObjPtr t, const GLfloat color[4])
{
	GLubyte c[4];
	CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
	CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
	CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
	CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);
	t->pp_border_color = PACK_COLOR_8888(c[3], c[0], c[1], c[2]);
}

/**
 * Changes variables and flags for a state update, which will happen at the
 * next UpdateTextureState
 */

static void r300TexParameter(GLcontext * ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     GLenum pname, const GLfloat * params)
{
	radeonTexObj* t = radeon_tex_obj(texObj);
	GLenum texBaseFormat;

	if (RADEON_DEBUG & (RADEON_STATE | RADEON_TEXTURE)) {
		fprintf(stderr, "%s( %s )\n", __FUNCTION__,
			_mesa_lookup_enum_by_nr(pname));
	}

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		r300SetTexFilter(t, texObj->MinFilter, texObj->MagFilter, texObj->MaxAnisotropy);
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_WRAP_R:
		r300UpdateTexWrap(t);
		break;

	case GL_TEXTURE_BORDER_COLOR:
		r300SetTexBorderColor(t, texObj->BorderColor.f);
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
		texBaseFormat = texObj->Image[0][texObj->BaseLevel]->_BaseFormat;

		if (texBaseFormat == GL_DEPTH_COMPONENT ||
			texBaseFormat == GL_DEPTH_STENCIL) {
			r300SetDepthTexMode(texObj);
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

static void r300DeleteTexture(GLcontext * ctx, struct gl_texture_object *texObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	radeonTexObj* t = radeon_tex_obj(texObj);

	if (RADEON_DEBUG & (RADEON_STATE | RADEON_TEXTURE)) {
		fprintf(stderr, "%s( %p (target = %s) )\n", __FUNCTION__,
			(void *)texObj,
			_mesa_lookup_enum_by_nr(texObj->Target));
	}

	if (rmesa) {
		int i;
		struct radeon_bo *bo;
		bo = !t->mt ? t->bo : t->mt->bo;
		if (bo && radeon_bo_is_referenced_by_cs(bo, rmesa->radeon.cmdbuf.cs)) {
			radeon_firevertices(&rmesa->radeon);
		}

		for(i = 0; i < R300_MAX_TEXTURE_UNITS; ++i)
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
static struct gl_texture_object *r300NewTextureObject(GLcontext * ctx,
						      GLuint name,
						      GLenum target)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	radeonTexObj* t = CALLOC_STRUCT(radeon_tex_obj);


	if (RADEON_DEBUG & (RADEON_STATE | RADEON_TEXTURE)) {
		fprintf(stderr, "%s( %p (target = %s) )\n", __FUNCTION__,
			t, _mesa_lookup_enum_by_nr(target));
	}

	_mesa_initialize_texture_object(&t->base, name, target);
	t->base.MaxAnisotropy = rmesa->radeon.initialMaxAnisotropy;

	/* Initialize hardware state */
	r300UpdateTexWrap(t);
	r300SetTexFilter(t, t->base.MinFilter, t->base.MagFilter, t->base.MaxAnisotropy);
	r300SetTexBorderColor(t, t->base.BorderColor.f);

	return &t->base;
}

void r300InitTextureFuncs(radeonContextPtr radeon, struct dd_function_table *functions)
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
	functions->NewTextureObject = r300NewTextureObject;
	functions->DeleteTexture = r300DeleteTexture;
	functions->IsTextureResident = driIsTextureResident;

	functions->TexParameter = r300TexParameter;

	functions->CompressedTexImage2D = radeonCompressedTexImage2D;
	functions->CompressedTexSubImage2D = radeonCompressedTexSubImage2D;

	if (radeon->radeonScreen->kernel_mm) {
		functions->CopyTexImage2D = radeonCopyTexImage2D;
		functions->CopyTexSubImage2D = radeonCopyTexSubImage2D;
	}

	functions->GenerateMipmap = radeonGenerateMipmap;

	driInitTextureFormats();
}
