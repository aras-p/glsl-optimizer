/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

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

#include "radeon_common.h"
#include "xmlpool.h"		/* for symbolic values of enum-type options */
#include "utils.h"
#include "vblank.h"
#include "drirenderbuffer.h"
#include "drivers/common/meta.h"
#include "main/context.h"
#include "main/renderbuffer.h"
#include "main/state.h"
#include "main/simple_list.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"

#define DRIVER_DATE "20090101"

#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0);
#endif


static const char* get_chip_family_name(int chip_family)
{
	switch(chip_family) {
	case CHIP_FAMILY_R100: return "R100";
	case CHIP_FAMILY_RV100: return "RV100";
	case CHIP_FAMILY_RS100: return "RS100";
	case CHIP_FAMILY_RV200: return "RV200";
	case CHIP_FAMILY_RS200: return "RS200";
	case CHIP_FAMILY_R200: return "R200";
	case CHIP_FAMILY_RV250: return "RV250";
	case CHIP_FAMILY_RS300: return "RS300";
	case CHIP_FAMILY_RV280: return "RV280";
	case CHIP_FAMILY_R300: return "R300";
	case CHIP_FAMILY_R350: return "R350";
	case CHIP_FAMILY_RV350: return "RV350";
	case CHIP_FAMILY_RV380: return "RV380";
	case CHIP_FAMILY_R420: return "R420";
	case CHIP_FAMILY_RV410: return "RV410";
	case CHIP_FAMILY_RS400: return "RS400";
	case CHIP_FAMILY_RS600: return "RS600";
	case CHIP_FAMILY_RS690: return "RS690";
	case CHIP_FAMILY_RS740: return "RS740";
	case CHIP_FAMILY_RV515: return "RV515";
	case CHIP_FAMILY_R520: return "R520";
	case CHIP_FAMILY_RV530: return "RV530";
	case CHIP_FAMILY_R580: return "R580";
	case CHIP_FAMILY_RV560: return "RV560";
	case CHIP_FAMILY_RV570: return "RV570";
	case CHIP_FAMILY_R600: return "R600";
	case CHIP_FAMILY_RV610: return "RV610";
	case CHIP_FAMILY_RV630: return "RV630";
	case CHIP_FAMILY_RV670: return "RV670";
	case CHIP_FAMILY_RV620: return "RV620";
	case CHIP_FAMILY_RV635: return "RV635";
	case CHIP_FAMILY_RS780: return "RS780";
	case CHIP_FAMILY_RS880: return "RS880";
	case CHIP_FAMILY_RV770: return "RV770";
	case CHIP_FAMILY_RV730: return "RV730";
	case CHIP_FAMILY_RV710: return "RV710";
	case CHIP_FAMILY_RV740: return "RV740";
	default: return "unknown";
	}
}


/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString(GLcontext * ctx, GLenum name)
{
	radeonContextPtr radeon = RADEON_CONTEXT(ctx);
	static char buffer[128];

	switch (name) {
	case GL_VENDOR:
		if (IS_R600_CLASS(radeon->radeonScreen))
			return (GLubyte *) "Advanced Micro Devices, Inc.";
		else if (IS_R300_CLASS(radeon->radeonScreen))
			return (GLubyte *) "DRI R300 Project";
		else
			return (GLubyte *) "Tungsten Graphics, Inc.";

	case GL_RENDERER:
	{
		unsigned offset;
		GLuint agp_mode = (radeon->radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
			radeon->radeonScreen->AGPMode;
		const char* chipclass;
		char hardwarename[32];

		if (IS_R600_CLASS(radeon->radeonScreen))
			chipclass = "R600";
		else if (IS_R300_CLASS(radeon->radeonScreen))
			chipclass = "R300";
		else if (IS_R200_CLASS(radeon->radeonScreen))
			chipclass = "R200";
		else
			chipclass = "R100";

		sprintf(hardwarename, "%s (%s %04X)",
		        chipclass,
		        get_chip_family_name(radeon->radeonScreen->chip_family),
		        radeon->radeonScreen->device_id);

		offset = driGetRendererString(buffer, hardwarename, DRIVER_DATE,
					      agp_mode);

		if (IS_R600_CLASS(radeon->radeonScreen)) {
			sprintf(&buffer[offset], " TCL");
		} else if (IS_R300_CLASS(radeon->radeonScreen)) {
			sprintf(&buffer[offset], " %sTCL",
				(radeon->radeonScreen->chip_flags & RADEON_CHIPSET_TCL)
				? "" : "NO-");
		} else {
			sprintf(&buffer[offset], " %sTCL",
				!(radeon->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
				? "" : "NO-");
		}

		if (radeon->radeonScreen->driScreen->dri2.enabled)
			strcat(buffer, " DRI2");

		return (GLubyte *) buffer;
	}

	default:
		return NULL;
	}
}

/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs(struct dd_function_table *functions)
{
	functions->GetString = radeonGetString;
}

/**
 * Create and initialize all common fields of the context,
 * including the Mesa context itself.
 */
GLboolean radeonInitContext(radeonContextPtr radeon,
			    struct dd_function_table* functions,
			    const __GLcontextModes * glVisual,
			    __DRIcontext * driContextPriv,
			    void *sharedContextPrivate)
{
	__DRIscreen *sPriv = driContextPriv->driScreenPriv;
	radeonScreenPtr screen = (radeonScreenPtr) (sPriv->private);
	GLcontext* ctx;
	GLcontext* shareCtx;
	int fthrottle_mode;

	/* Fill in additional standard functions. */
	radeonInitDriverFuncs(functions);

	radeon->radeonScreen = screen;
	/* Allocate and initialize the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((radeonContextPtr)sharedContextPrivate)->glCtx;
	else
		shareCtx = NULL;
	radeon->glCtx = _mesa_create_context(glVisual, shareCtx,
					    functions, (void *)radeon);
	if (!radeon->glCtx)
		return GL_FALSE;

	ctx = radeon->glCtx;
	driContextPriv->driverPrivate = radeon;

	meta_init_metaops(ctx, &radeon->meta);

	_mesa_meta_init(ctx);

	/* DRI fields */
	radeon->dri.context = driContextPriv;
	radeon->dri.screen = sPriv;
	radeon->dri.hwContext = driContextPriv->hHWContext;
	radeon->dri.hwLock = &sPriv->pSAREA->lock;
	radeon->dri.hwLockCount = 0;
	radeon->dri.fd = sPriv->fd;
	radeon->dri.drmMinor = sPriv->drm_version.minor;

	radeon->sarea = (drm_radeon_sarea_t *) ((GLubyte *) sPriv->pSAREA +
					       screen->sarea_priv_offset);

	/* Setup IRQs */
	fthrottle_mode = driQueryOptioni(&radeon->optionCache, "fthrottle_mode");
	radeon->iw.irq_seq = -1;
	radeon->irqsEmitted = 0;
	radeon->do_irqs = (fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS &&
			   radeon->radeonScreen->irq);

	radeon->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

	if (!radeon->do_irqs)
		fprintf(stderr,
			"IRQ's not enabled, falling back to %s: %d %d\n",
			radeon->do_usleeps ? "usleeps" : "busy waits",
			fthrottle_mode, radeon->radeonScreen->irq);

        radeon->texture_depth = driQueryOptioni (&radeon->optionCache,
					        "texture_depth");
        if (radeon->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
                radeon->texture_depth = ( glVisual->rgbBits > 16 ) ?
	        DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

	if (IS_R600_CLASS(radeon->radeonScreen)) {
		radeon->texture_row_align = 256;
		radeon->texture_rect_row_align = 256;
		radeon->texture_compressed_row_align = 256;
	} else if (IS_R200_CLASS(radeon->radeonScreen) ||
		   IS_R100_CLASS(radeon->radeonScreen)) {
		radeon->texture_row_align = 32;
		radeon->texture_rect_row_align = 64;
		radeon->texture_compressed_row_align = 32;
	} else { /* R300 - not sure this is all correct */
		int chip_family = radeon->radeonScreen->chip_family;
		if (chip_family == CHIP_FAMILY_RS600 ||
		    chip_family == CHIP_FAMILY_RS690 ||
		    chip_family == CHIP_FAMILY_RS740)
			radeon->texture_row_align = 64;
		else
			radeon->texture_row_align = 32;
		radeon->texture_rect_row_align = 64;
		radeon->texture_compressed_row_align = 32;
	}

	radeon_init_dma(radeon);

	return GL_TRUE;
}



/**
 * Destroy the command buffer and state atoms.
 */
static void radeon_destroy_atom_list(radeonContextPtr radeon)
{
	struct radeon_state_atom *atom;

	foreach(atom, &radeon->hw.atomlist) {
		FREE(atom->cmd);
		if (atom->lastcmd)
			FREE(atom->lastcmd);
	}

}

/**
 * Cleanup common context fields.
 * Called by r200DestroyContext/r300DestroyContext
 */
void radeonDestroyContext(__DRIcontext *driContextPriv )
{
#ifdef RADEON_BO_TRACK
	FILE *track;
#endif
	GET_CURRENT_CONTEXT(ctx);
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;
	radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

	assert(radeon);

	_mesa_meta_free(radeon->glCtx);

	if (radeon == current) {
		radeon_firevertices(radeon);
		_mesa_make_current(NULL, NULL, NULL);
	}

	if (!is_empty_list(&radeon->dma.reserved)) {
		rcommonFlushCmdBuf( radeon, __FUNCTION__ );
	}

	radeonFreeDmaRegions(radeon);
	radeonReleaseArrays(radeon->glCtx, ~0);
	meta_destroy_metaops(&radeon->meta);
	if (radeon->vtbl.free_context)
		radeon->vtbl.free_context(radeon->glCtx);
	_swsetup_DestroyContext( radeon->glCtx );
	_tnl_DestroyContext( radeon->glCtx );
	_vbo_DestroyContext( radeon->glCtx );
	_swrast_DestroyContext( radeon->glCtx );

	/* free atom list */
	/* free the Mesa context */
	_mesa_destroy_context(radeon->glCtx);

	/* _mesa_destroy_context() might result in calls to functions that
	 * depend on the DriverCtx, so don't set it to NULL before.
	 *
	 * radeon->glCtx->DriverCtx = NULL;
	 */
	/* free the option cache */
	driDestroyOptionCache(&radeon->optionCache);

	rcommonDestroyCmdBuf(radeon);

	radeon_destroy_atom_list(radeon);

	if (radeon->state.scissor.pClipRects) {
		FREE(radeon->state.scissor.pClipRects);
		radeon->state.scissor.pClipRects = 0;
	}
#ifdef RADEON_BO_TRACK
	track = fopen("/tmp/tracklog", "w");
	if (track) {
		radeon_tracker_print(&radeon->radeonScreen->bom->tracker, track);
		fclose(track);
	}
#endif
	FREE(radeon);
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean radeonUnbindContext(__DRIcontext * driContextPriv)
{
	radeonContextPtr radeon = (radeonContextPtr) driContextPriv->driverPrivate;

	if (RADEON_DEBUG & RADEON_DRI)
		fprintf(stderr, "%s ctx %p\n", __FUNCTION__,
			radeon->glCtx);

	return GL_TRUE;
}


static void
radeon_make_kernel_renderbuffer_current(radeonContextPtr radeon,
					struct radeon_framebuffer *draw)
{
	/* if radeon->fake */
	struct radeon_renderbuffer *rb;

	if ((rb = (void *)draw->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->frontOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->frontPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_BACK_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->backOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->backPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_DEPTH].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_STENCIL].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
}

static void
radeon_make_renderbuffer_current(radeonContextPtr radeon,
				 struct radeon_framebuffer *draw)
{
	int size = 4096*4096*4;
	/* if radeon->fake */
	struct radeon_renderbuffer *rb;

	if (radeon->radeonScreen->kernel_mm) {
		radeon_make_kernel_renderbuffer_current(radeon, draw);
		return;
	}


	if ((rb = (void *)draw->base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->frontOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->frontPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_BACK_LEFT].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->backOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->backPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_DEPTH].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
	if ((rb = (void *)draw->base.Attachment[BUFFER_STENCIL].Renderbuffer)) {
		if (!rb->bo) {
			rb->bo = radeon_bo_open(radeon->radeonScreen->bom,
						radeon->radeonScreen->depthOffset +
						radeon->radeonScreen->fbLocation,
						size,
						4096,
						RADEON_GEM_DOMAIN_VRAM,
						0);
		}
		rb->cpp = radeon->radeonScreen->cpp;
		rb->pitch = radeon->radeonScreen->depthPitch * rb->cpp;
	}
}

static unsigned
radeon_bits_per_pixel(const struct radeon_renderbuffer *rb)
{
   return _mesa_get_format_bytes(rb->base.Format) * 8; 
}

void
radeon_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable,
			    GLboolean front_only)
{
	unsigned int attachments[10];
	__DRIbuffer *buffers = NULL;
	__DRIscreen *screen;
	struct radeon_renderbuffer *rb;
	int i, count;
	struct radeon_framebuffer *draw;
	radeonContextPtr radeon;
	char *regname;
	struct radeon_bo *depth_bo = NULL, *bo;

	if (RADEON_DEBUG & RADEON_DRI)
	    fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

	draw = drawable->driverPrivate;
	screen = context->driScreenPriv;
	radeon = (radeonContextPtr) context->driverPrivate;

	if (screen->dri2.loader
	   && (screen->dri2.loader->base.version > 2)
	   && (screen->dri2.loader->getBuffersWithFormat != NULL)) {
		struct radeon_renderbuffer *depth_rb;
		struct radeon_renderbuffer *stencil_rb;

		i = 0;
		if ((front_only || radeon->is_front_buffer_rendering ||
		     radeon->is_front_buffer_reading ||
		     !draw->color_rb[1])
		    && draw->color_rb[0]) {
			attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
			attachments[i++] = radeon_bits_per_pixel(draw->color_rb[0]);
		}

		if (!front_only) {
			if (draw->color_rb[1]) {
				attachments[i++] = __DRI_BUFFER_BACK_LEFT;
				attachments[i++] = radeon_bits_per_pixel(draw->color_rb[1]);
			}

			depth_rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			stencil_rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);

			if ((depth_rb != NULL) && (stencil_rb != NULL)) {
				attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
				attachments[i++] = radeon_bits_per_pixel(depth_rb);
			} else if (depth_rb != NULL) {
				attachments[i++] = __DRI_BUFFER_DEPTH;
				attachments[i++] = radeon_bits_per_pixel(depth_rb);
			} else if (stencil_rb != NULL) {
				attachments[i++] = __DRI_BUFFER_STENCIL;
				attachments[i++] = radeon_bits_per_pixel(stencil_rb);
			}
		}

		buffers = (*screen->dri2.loader->getBuffersWithFormat)(drawable,
								&drawable->w,
								&drawable->h,
								attachments, i / 2,
								&count,
								drawable->loaderPrivate);
	} else if (screen->dri2.loader) {
		i = 0;
		if (draw->color_rb[0])
			attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
		if (!front_only) {
			if (draw->color_rb[1])
				attachments[i++] = __DRI_BUFFER_BACK_LEFT;
			if (radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH))
				attachments[i++] = __DRI_BUFFER_DEPTH;
			if (radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL))
				attachments[i++] = __DRI_BUFFER_STENCIL;
		}

		buffers = (*screen->dri2.loader->getBuffers)(drawable,
								 &drawable->w,
								 &drawable->h,
								 attachments, i,
								 &count,
								 drawable->loaderPrivate);
	}

	if (buffers == NULL)
		return;

	/* set one cliprect to cover the whole drawable */
	drawable->x = 0;
	drawable->y = 0;
	drawable->backX = 0;
	drawable->backY = 0;
	drawable->numClipRects = 1;
	drawable->pClipRects[0].x1 = 0;
	drawable->pClipRects[0].y1 = 0;
	drawable->pClipRects[0].x2 = drawable->w;
	drawable->pClipRects[0].y2 = drawable->h;
	drawable->numBackClipRects = 1;
	drawable->pBackClipRects[0].x1 = 0;
	drawable->pBackClipRects[0].y1 = 0;
	drawable->pBackClipRects[0].x2 = drawable->w;
	drawable->pBackClipRects[0].y2 = drawable->h;
	for (i = 0; i < count; i++) {
		switch (buffers[i].attachment) {
		case __DRI_BUFFER_FRONT_LEFT:
			rb = draw->color_rb[0];
			regname = "dri2 front buffer";
			break;
		case __DRI_BUFFER_FAKE_FRONT_LEFT:
			rb = draw->color_rb[0];
			regname = "dri2 fake front buffer";
			break;
		case __DRI_BUFFER_BACK_LEFT:
			rb = draw->color_rb[1];
			regname = "dri2 back buffer";
			break;
		case __DRI_BUFFER_DEPTH:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			regname = "dri2 depth buffer";
			break;
		case __DRI_BUFFER_DEPTH_STENCIL:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_DEPTH);
			regname = "dri2 depth / stencil buffer";
			break;
		case __DRI_BUFFER_STENCIL:
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);
			regname = "dri2 stencil buffer";
			break;
		case __DRI_BUFFER_ACCUM:
		default:
			fprintf(stderr,
				"unhandled buffer attach event, attacment type %d\n",
				buffers[i].attachment);
			return;
		}

		if (rb == NULL)
			continue;

		if (rb->bo) {
			uint32_t name = radeon_gem_name_bo(rb->bo);
			if (name == buffers[i].name)
				continue;
		}

		if (RADEON_DEBUG & RADEON_DRI)
			fprintf(stderr,
				"attaching buffer %s, %d, at %d, cpp %d, pitch %d\n",
				regname, buffers[i].name, buffers[i].attachment,
				buffers[i].cpp, buffers[i].pitch);

		rb->cpp = buffers[i].cpp;
		rb->pitch = buffers[i].pitch;
		rb->base.Width = drawable->w;
		rb->base.Height = drawable->h;
		rb->has_surface = 0;

		if (buffers[i].attachment == __DRI_BUFFER_STENCIL && depth_bo) {
			if (RADEON_DEBUG & RADEON_DRI)
				fprintf(stderr, "(reusing depth buffer as stencil)\n");
			bo = depth_bo;
			radeon_bo_ref(bo);
		} else {
			uint32_t tiling_flags = 0, pitch = 0;
			int ret;

			bo = radeon_bo_open(radeon->radeonScreen->bom,
						buffers[i].name,
						0,
						0,
						RADEON_GEM_DOMAIN_VRAM,
						buffers[i].flags);

			if (bo == NULL) {

				fprintf(stderr, "failed to attach %s %d\n",
					regname, buffers[i].name);

			}

			ret = radeon_bo_get_tiling(bo, &tiling_flags, &pitch);
			if (tiling_flags & RADEON_TILING_MACRO)
				bo->flags |= RADEON_BO_FLAGS_MACRO_TILE;
			if (tiling_flags & RADEON_TILING_MICRO)
				bo->flags |= RADEON_BO_FLAGS_MICRO_TILE;
			
		}

		if (buffers[i].attachment == __DRI_BUFFER_DEPTH) {
			if (draw->base.Visual.depthBits == 16)
				rb->cpp = 2;
			depth_bo = bo;
		}

		radeon_renderbuffer_set_bo(rb, bo);
		radeon_bo_unref(bo);

		if (buffers[i].attachment == __DRI_BUFFER_DEPTH_STENCIL) {
			rb = radeon_get_renderbuffer(&draw->base, BUFFER_STENCIL);
			if (rb != NULL) {
				struct radeon_bo *stencil_bo = NULL;

				if (rb->bo) {
					uint32_t name = radeon_gem_name_bo(rb->bo);
					if (name == buffers[i].name)
						continue;
				}

				stencil_bo = bo;
				radeon_bo_ref(stencil_bo);
				radeon_renderbuffer_set_bo(rb, stencil_bo);
				radeon_bo_unref(stencil_bo);
			}
		}
	}

	driUpdateFramebufferSize(radeon->glCtx, drawable);
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean radeonMakeCurrent(__DRIcontext * driContextPriv,
			    __DRIdrawable * driDrawPriv,
			    __DRIdrawable * driReadPriv)
{
	radeonContextPtr radeon;
	struct radeon_framebuffer *drfb;
	struct gl_framebuffer *readfb;

	if (!driContextPriv) {
		if (RADEON_DEBUG & RADEON_DRI)
			fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
		_mesa_make_current(NULL, NULL, NULL);
		return GL_TRUE;
	}

	radeon = (radeonContextPtr) driContextPriv->driverPrivate;
	drfb = driDrawPriv->driverPrivate;
	readfb = driReadPriv->driverPrivate;

	if (driContextPriv->driScreenPriv->dri2.enabled) {
		radeon_update_renderbuffers(driContextPriv, driDrawPriv, GL_FALSE);
		if (driDrawPriv != driReadPriv)
			radeon_update_renderbuffers(driContextPriv, driReadPriv, GL_FALSE);
		_mesa_reference_renderbuffer(&radeon->state.color.rb,
			&(radeon_get_renderbuffer(&drfb->base, BUFFER_BACK_LEFT)->base));
		_mesa_reference_renderbuffer(&radeon->state.depth.rb,
			&(radeon_get_renderbuffer(&drfb->base, BUFFER_DEPTH)->base));
	} else {
		radeon_make_renderbuffer_current(radeon, drfb);
	}

	if (RADEON_DEBUG & RADEON_DRI)
	     fprintf(stderr, "%s ctx %p dfb %p rfb %p\n", __FUNCTION__, radeon->glCtx, drfb, readfb);

	driUpdateFramebufferSize(radeon->glCtx, driDrawPriv);
	if (driReadPriv != driDrawPriv)
		driUpdateFramebufferSize(radeon->glCtx, driReadPriv);

	_mesa_make_current(radeon->glCtx, &drfb->base, readfb);

	_mesa_update_state(radeon->glCtx);

	if (radeon->glCtx->DrawBuffer == &drfb->base) {
		if (driDrawPriv->swap_interval == (unsigned)-1) {
			int i;
			driDrawPriv->vblFlags =
				(radeon->radeonScreen->irq != 0)
				? driGetDefaultVBlankFlags(&radeon->
							   optionCache)
				: VBLANK_FLAG_NO_IRQ;

			driDrawableInitVBlank(driDrawPriv);
			drfb->vbl_waited = driDrawPriv->vblSeq;

			for (i = 0; i < 2; i++) {
				if (drfb->color_rb[i])
					drfb->color_rb[i]->vbl_pending = driDrawPriv->vblSeq;
			}

		}

		radeon_window_moved(radeon);
		radeon_draw_buffer(radeon->glCtx, &drfb->base);
	}


	if (RADEON_DEBUG & RADEON_DRI)
		fprintf(stderr, "End %s\n", __FUNCTION__);

	return GL_TRUE;
}

