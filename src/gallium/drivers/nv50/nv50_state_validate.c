/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "util/u_format.h"

#include "nv50_context.h"
#include "nouveau/nouveau_stateobj.h"

static struct nouveau_stateobj *
validate_fb(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(32, 79, 18);
	struct pipe_framebuffer_state *fb = &nv50->framebuffer;
	unsigned i, w, h, gw = 0;

	/* Set nr of active RTs and select RT for each colour output.
	 * FP result 0 always goes to RT[0], bits 4 - 6 are ignored.
	 * Ambiguous assignment results in no rendering (no DATA_ERROR).
	 */
	so_method(so, tesla, NV50TCL_RT_CONTROL, 1);
	so_data  (so, fb->nr_cbufs |
		  (0 <<  4) | (1 <<  7) | (2 << 10) | (3 << 13) |
		  (4 << 16) | (5 << 19) | (6 << 22) | (7 << 25));

	for (i = 0; i < fb->nr_cbufs; i++) {
		struct pipe_texture *pt = fb->cbufs[i]->texture;
		struct nouveau_bo *bo = nv50_miptree(pt)->base.bo;

		if (!gw) {
			w = fb->cbufs[i]->width;
			h = fb->cbufs[i]->height;
			gw = 1;
		} else {
			assert(w == fb->cbufs[i]->width);
			assert(h == fb->cbufs[i]->height);
		}

		so_method(so, tesla, NV50TCL_RT_HORIZ(i), 2);
		so_data  (so, fb->cbufs[i]->width);
		so_data  (so, fb->cbufs[i]->height);

		so_method(so, tesla, NV50TCL_RT_ADDRESS_HIGH(i), 5);
		so_reloc (so, bo, fb->cbufs[i]->offset, NOUVEAU_BO_VRAM |
			      NOUVEAU_BO_HIGH | NOUVEAU_BO_RDWR, 0, 0);
		so_reloc (so, bo, fb->cbufs[i]->offset, NOUVEAU_BO_VRAM |
			      NOUVEAU_BO_LOW | NOUVEAU_BO_RDWR, 0, 0);
		switch (fb->cbufs[i]->format) {
		case PIPE_FORMAT_B8G8R8A8_UNORM:
			so_data(so, NV50TCL_RT_FORMAT_A8R8G8B8_UNORM);
			break;
		case PIPE_FORMAT_B8G8R8X8_UNORM:
			so_data(so, NV50TCL_RT_FORMAT_X8R8G8B8_UNORM);
			break;
		case PIPE_FORMAT_B5G6R5_UNORM:
			so_data(so, NV50TCL_RT_FORMAT_R5G6B5_UNORM);
			break;
		case PIPE_FORMAT_R16G16B16A16_SNORM:
			so_data(so, NV50TCL_RT_FORMAT_R16G16B16A16_SNORM);
			break;
		case PIPE_FORMAT_R16G16B16A16_UNORM:
			so_data(so, NV50TCL_RT_FORMAT_R16G16B16A16_UNORM);
			break;
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
			so_data(so, NV50TCL_RT_FORMAT_R32G32B32A32_FLOAT);
			break;
		case PIPE_FORMAT_R16G16_SNORM:
			so_data(so, NV50TCL_RT_FORMAT_R16G16_SNORM);
			break;
		case PIPE_FORMAT_R16G16_UNORM:
			so_data(so, NV50TCL_RT_FORMAT_R16G16_UNORM);
			break;
		default:
			NOUVEAU_ERR("AIIII unknown format %s\n",
			            util_format_name(fb->cbufs[i]->format));
			so_data(so, NV50TCL_RT_FORMAT_X8R8G8B8_UNORM);
			break;
		}
		so_data(so, nv50_miptree(pt)->
				level[fb->cbufs[i]->level].tile_mode << 4);
		so_data(so, 0x00000000);

		so_method(so, tesla, NV50TCL_RT_ARRAY_MODE, 1);
		so_data  (so, 1);
	}

	if (fb->zsbuf) {
		struct pipe_texture *pt = fb->zsbuf->texture;
		struct nouveau_bo *bo = nv50_miptree(pt)->base.bo;

		if (!gw) {
			w = fb->zsbuf->width;
			h = fb->zsbuf->height;
			gw = 1;
		} else {
			assert(w == fb->zsbuf->width);
			assert(h == fb->zsbuf->height);
		}

		so_method(so, tesla, NV50TCL_ZETA_ADDRESS_HIGH, 5);
		so_reloc (so, bo, fb->zsbuf->offset, NOUVEAU_BO_VRAM |
			      NOUVEAU_BO_HIGH | NOUVEAU_BO_RDWR, 0, 0);
		so_reloc (so, bo, fb->zsbuf->offset, NOUVEAU_BO_VRAM |
			      NOUVEAU_BO_LOW | NOUVEAU_BO_RDWR, 0, 0);
		switch (fb->zsbuf->format) {
		case PIPE_FORMAT_Z24S8_UNORM:
			so_data(so, NV50TCL_ZETA_FORMAT_S8Z24_UNORM);
			break;
		case PIPE_FORMAT_Z24X8_UNORM:
			so_data(so, NV50TCL_ZETA_FORMAT_X8Z24_UNORM);
			break;
		case PIPE_FORMAT_S8Z24_UNORM:
			so_data(so, NV50TCL_ZETA_FORMAT_Z24S8_UNORM);
			break;
		case PIPE_FORMAT_Z32_FLOAT:
			so_data(so, NV50TCL_ZETA_FORMAT_Z32_FLOAT);
			break;
		default:
			NOUVEAU_ERR("AIIII unknown format %s\n",
			            util_format_name(fb->zsbuf->format));
			so_data(so, NV50TCL_ZETA_FORMAT_S8Z24_UNORM);
			break;
		}
		so_data(so, nv50_miptree(pt)->
				level[fb->zsbuf->level].tile_mode << 4);
		so_data(so, 0x00000000);

		so_method(so, tesla, NV50TCL_ZETA_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_ZETA_HORIZ, 3);
		so_data  (so, fb->zsbuf->width);
		so_data  (so, fb->zsbuf->height);
		so_data  (so, 0x00010001);
	} else {
		so_method(so, tesla, NV50TCL_ZETA_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, tesla, NV50TCL_VIEWPORT_HORIZ(0), 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);
	/* set window lower left corner */
	so_method(so, tesla, NV50TCL_WINDOW_OFFSET_X, 2);
	so_data  (so, 0);
	so_data  (so, 0);
	/* set screen scissor rectangle */
	so_method(so, tesla, NV50TCL_SCREEN_SCISSOR_HORIZ, 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);

	return so;
}

static void
nv50_validate_samplers(struct nv50_context *nv50, struct nouveau_stateobj *so,
		       unsigned p)
{
	struct nouveau_grobj *eng2d = nv50->screen->eng2d;
	unsigned i, j, dw = nv50->sampler_nr[p] * 8;

	if (!dw)
		return;
	nv50_so_init_sifc(nv50, so, nv50->screen->tsc, NOUVEAU_BO_VRAM,
			  p * (32 * 8 * 4), dw * 4);

	so_method(so, eng2d, NV50_2D_SIFC_DATA | (2 << 29), dw);

	for (i = 0; i < nv50->sampler_nr[p]; ++i) {
		if (nv50->sampler[p][i])
			so_datap(so, nv50->sampler[p][i]->tsc, 8);
		else {
			for (j = 0; j < 8; ++j) /* you get punished */
				so_data(so, 0); /* ... for leaving holes */
		}
	}
}

static struct nouveau_stateobj *
validate_blend(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so = NULL;
	so_ref(nv50->blend->so, &so);
	return so;
}

static struct nouveau_stateobj *
validate_zsa(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so = NULL;
	so_ref(nv50->zsa->so, &so);
	return so;
}

static struct nouveau_stateobj *
validate_rast(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so = NULL;
	so_ref(nv50->rasterizer->so, &so);
	return so;
}

static struct nouveau_stateobj *
validate_blend_colour(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(1, 4, 0);

	so_method(so, tesla, NV50TCL_BLEND_COLOR(0), 4);
	so_data  (so, fui(nv50->blend_colour.color[0]));
	so_data  (so, fui(nv50->blend_colour.color[1]));
	so_data  (so, fui(nv50->blend_colour.color[2]));
	so_data  (so, fui(nv50->blend_colour.color[3]));
	return so;
}

static struct nouveau_stateobj *
validate_stencil_ref(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so = so_new(2, 2, 0);

	so_method(so, tesla, NV50TCL_STENCIL_FRONT_FUNC_REF, 1);
	so_data  (so, nv50->stencil_ref.ref_value[0]);
	so_method(so, tesla, NV50TCL_STENCIL_BACK_FUNC_REF, 1);
	so_data  (so, nv50->stencil_ref.ref_value[1]);
	return so;
}

static struct nouveau_stateobj *
validate_stipple(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(1, 32, 0);
	int i;

	so_method(so, tesla, NV50TCL_POLYGON_STIPPLE_PATTERN(0), 32);
	for (i = 0; i < 32; i++)
		so_data(so, util_bswap32(nv50->stipple.stipple[i]));
	return so;
}

static struct nouveau_stateobj *
validate_scissor(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
        struct pipe_scissor_state *s = &nv50->scissor;
	struct nouveau_stateobj *so;

	so = so_new(1, 2, 0);
	so_method(so, tesla, NV50TCL_SCISSOR_HORIZ(0), 2);
	so_data  (so, (s->maxx << 16) | s->minx);
	so_data  (so, (s->maxy << 16) | s->miny);
	return so;
}

static struct nouveau_stateobj *
validate_viewport(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(5, 9, 0);

	so_method(so, tesla, NV50TCL_VIEWPORT_TRANSLATE_X(0), 3);
	so_data  (so, fui(nv50->viewport.translate[0]));
	so_data  (so, fui(nv50->viewport.translate[1]));
	so_data  (so, fui(nv50->viewport.translate[2]));
	so_method(so, tesla, NV50TCL_VIEWPORT_SCALE_X(0), 3);
	so_data  (so, fui(nv50->viewport.scale[0]));
	so_data  (so, fui(nv50->viewport.scale[1]));
	so_data  (so, fui(nv50->viewport.scale[2]));

	so_method(so, tesla, NV50TCL_VIEWPORT_TRANSFORM_EN, 1);
	so_data  (so, 1);
	/* 0x0000 = remove whole primitive only (xyz)
	 * 0x1018 = remove whole primitive only (xy), clamp z
	 * 0x1080 = clip primitive (xyz)
	 * 0x1098 = clip primitive (xy), clamp z
	 */
	so_method(so, tesla, NV50TCL_VIEW_VOLUME_CLIP_CTRL, 1);
	so_data  (so, 0x1080);
	/* no idea what 0f90 does */
	so_method(so, tesla, 0x0f90, 1);
	so_data  (so, 0);

	return so;
}

static struct nouveau_stateobj *
validate_sampler(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	unsigned nr = 0, i;

	for (i = 0; i < 3; ++i)
		nr += nv50->sampler_nr[i];

	so = so_new(1 + 5 * 3, 1 + 19 * 3 + nr * 8, 3 * 2);

	nv50_validate_samplers(nv50, so, 0); /* VP */
	nv50_validate_samplers(nv50, so, 2); /* FP */

	so_method(so, tesla, 0x1334, 1); /* flush TSC */
	so_data  (so, 0);

	return so;
}

static struct nouveau_stateobj *
validate_vtxbuf(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so = NULL;
	so_ref(nv50->state.vtxbuf, &so);
	return so;
}

static struct nouveau_stateobj *
validate_vtxattr(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so = NULL;
	so_ref(nv50->state.vtxattr, &so);
	return so;
}

struct state_validate {
	struct nouveau_stateobj *(*func)(struct nv50_context *nv50);
	unsigned states;
} validate_list[] = {
	{ validate_fb             , NV50_NEW_FRAMEBUFFER                      },
	{ validate_blend          , NV50_NEW_BLEND                            },
	{ validate_zsa            , NV50_NEW_ZSA                              },
	{ nv50_vertprog_validate  , NV50_NEW_VERTPROG | NV50_NEW_VERTPROG_CB  },
	{ nv50_fragprog_validate  , NV50_NEW_FRAGPROG | NV50_NEW_FRAGPROG_CB  },
	{ nv50_geomprog_validate  , NV50_NEW_GEOMPROG | NV50_NEW_GEOMPROG_CB  },
	{ nv50_fp_linkage_validate, NV50_NEW_VERTPROG | NV50_NEW_GEOMPROG |
				    NV50_NEW_FRAGPROG | NV50_NEW_RASTERIZER   },
	{ nv50_gp_linkage_validate, NV50_NEW_VERTPROG | NV50_NEW_GEOMPROG     },
	{ validate_rast           , NV50_NEW_RASTERIZER                       },
	{ validate_blend_colour   , NV50_NEW_BLEND_COLOUR                     },
	{ validate_stencil_ref    , NV50_NEW_STENCIL_REF                      },
	{ validate_stipple        , NV50_NEW_STIPPLE                          },
	{ validate_scissor        , NV50_NEW_SCISSOR                          },
	{ validate_viewport       , NV50_NEW_VIEWPORT                         },
	{ validate_sampler        , NV50_NEW_SAMPLER                          },
	{ nv50_tex_validate       , NV50_NEW_TEXTURE | NV50_NEW_SAMPLER       },
	{ nv50_vbo_validate       , NV50_NEW_ARRAYS                           },
	{ validate_vtxbuf         , NV50_NEW_ARRAYS                           },
	{ validate_vtxattr        , NV50_NEW_ARRAYS                           },
	{}
};
#define validate_list_len (sizeof(validate_list) / sizeof(validate_list[0]))

boolean
nv50_state_validate(struct nv50_context *nv50, unsigned wait_dwords)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned nr_relocs = 128, nr_dwords = wait_dwords + 128 + 4;
	int ret, i;

	for (i = 0; i < validate_list_len; i++) {
		struct state_validate *validate = &validate_list[i];
		struct nouveau_stateobj *so;

		if (!(nv50->dirty & validate->states))
			continue;

		so = validate->func(nv50);
		if (!so)
			continue;

		nr_dwords += (so->total + so->cur);
		nr_relocs += so->cur_reloc;

		so_ref(so, &nv50->state.hw[i]);
		so_ref(NULL, &so);
		nv50->state.hw_dirty |= (1 << i);
	}
	nv50->dirty = 0;

	if (nv50->screen->cur_ctx != nv50) {
		for (i = 0; i < validate_list_len; i++) {
			if (!nv50->state.hw[i] ||
			    (nv50->state.hw_dirty & (1 << i)))
				continue;

			nr_dwords += (nv50->state.hw[i]->total +
				      nv50->state.hw[i]->cur);
			nr_relocs += nv50->state.hw[i]->cur_reloc;
			nv50->state.hw_dirty |= (1 << i);
		}

		nv50->screen->cur_ctx = nv50;
	}

	ret = MARK_RING(chan, nr_dwords, nr_relocs);
	if (ret) {
		debug_printf("MARK_RING(%d, %d) failed: %d\n",
			     nr_dwords, nr_relocs, ret);
		return FALSE;
	}

	while (nv50->state.hw_dirty) {
		i = ffs(nv50->state.hw_dirty) - 1;
		nv50->state.hw_dirty &= ~(1 << i);

		so_emit(chan, nv50->state.hw[i]);
	}

	/* Yes, really, we need to do this.  If a buffer that is referenced
	 * on the hardware isn't part of changed state above, without doing
	 * this the kernel is given no clue that the buffer is being used
	 * still.  This can cause all sorts of fun issues.
	 */
	nv50_tex_relocs(nv50);
	so_emit_reloc_markers(chan, nv50->state.hw[0]); /* fb */
	so_emit_reloc_markers(chan, nv50->state.hw[3]); /* vp */
	so_emit_reloc_markers(chan, nv50->state.hw[4]); /* fp */
	so_emit_reloc_markers(chan, nv50->state.hw[17]); /* vb */
	nv50_screen_relocs(nv50->screen);

	/* No idea.. */
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	return TRUE;
}

void nv50_so_init_sifc(struct nv50_context *nv50,
		       struct nouveau_stateobj *so,
		       struct nouveau_bo *bo, unsigned reloc,
		       unsigned offset, unsigned size)
{
	struct nouveau_grobj *eng2d = nv50->screen->eng2d;

	reloc |= NOUVEAU_BO_WR;

	so_method(so, eng2d, NV50_2D_DST_FORMAT, 2);
	so_data  (so, NV50_2D_DST_FORMAT_R8_UNORM);
	so_data  (so, 1);
	so_method(so, eng2d, NV50_2D_DST_PITCH, 5);
	so_data  (so, 262144);
	so_data  (so, 65536);
	so_data  (so, 1);
	so_reloc (so, bo, offset, reloc | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, bo, offset, reloc | NOUVEAU_BO_LOW, 0, 0);
	so_method(so, eng2d, NV50_2D_SIFC_BITMAP_ENABLE, 2);
	so_data  (so, 0);
	so_data  (so, NV50_2D_SIFC_FORMAT_R8_UNORM);
	so_method(so, eng2d, NV50_2D_SIFC_WIDTH, 10);
	so_data  (so, size);
	so_data  (so, 1);
	so_data  (so, 0);
	so_data  (so, 1);
	so_data  (so, 0);
	so_data  (so, 1);
	so_data  (so, 0);
	so_data  (so, 0);
	so_data  (so, 0);
	so_data  (so, 0);
}
