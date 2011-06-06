/*
 * Copyright (C) 2009-2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_bufferobj.h"
#include "nouveau_fbo.h"

#include "main/dd.h"
#include "main/framebuffer.h"
#include "main/light.h"
#include "main/state.h"
#include "drivers/common/meta.h"
#include "drivers/common/driverfuncs.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color

#include "main/remap_helper.h"

static const struct dri_extension nouveau_extensions[] = {
	{ "GL_ARB_multitexture",	NULL },
	{ "GL_ARB_texture_env_add",	NULL },
	{ "GL_ARB_texture_mirrored_repeat", NULL },
	{ "GL_EXT_fog_coord",		GL_EXT_fog_coord_functions },
	{ "GL_EXT_framebuffer_blit",	NULL },
	{ "GL_EXT_framebuffer_object",	GL_EXT_framebuffer_object_functions },
	{ "GL_EXT_packed_depth_stencil", NULL},
	{ "GL_EXT_secondary_color",	GL_EXT_secondary_color_functions },
	{ "GL_EXT_stencil_wrap",	NULL },
	{ "GL_EXT_texture_env_combine",	NULL },
	{ "GL_EXT_texture_filter_anisotropic", NULL },
	{ "GL_EXT_texture_lod_bias",	NULL },
	{ "GL_NV_blend_square",         NULL },
	{ "GL_NV_texture_env_combine4",	NULL },
	{ NULL,				NULL }
};

static void
nouveau_channel_flush_notify(struct nouveau_channel *chan)
{
	struct nouveau_context *nctx = chan->user_private;
	struct gl_context *ctx = &nctx->base;

	if (nctx->fallback < SWRAST)
		nouveau_bo_state_emit(ctx);
}

GLboolean
nouveau_context_create(gl_api api,
		       const struct gl_config *visual, __DRIcontext *dri_ctx,
		       void *share_ctx)
{
	__DRIscreen *dri_screen = dri_ctx->driScreenPriv;
	struct nouveau_screen *screen = dri_screen->private;
	struct nouveau_context *nctx;
	struct gl_context *ctx;

	ctx = screen->driver->context_create(screen, visual, share_ctx);
	if (!ctx)
		return GL_FALSE;

	nctx = to_nouveau_context(ctx);
	nctx->dri_context = dri_ctx;
	dri_ctx->driverPrivate = ctx;

	return GL_TRUE;
}

GLboolean
nouveau_context_init(struct gl_context *ctx, struct nouveau_screen *screen,
		     const struct gl_config *visual, struct gl_context *share_ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct dd_function_table functions;
	int ret;

	nctx->screen = screen;
	nctx->fallback = HWTNL;

	/* Initialize the function pointers. */
	_mesa_init_driver_functions(&functions);
	nouveau_driver_functions_init(&functions);
	nouveau_bufferobj_functions_init(&functions);
	nouveau_texture_functions_init(&functions);
	nouveau_fbo_functions_init(&functions);

	/* Initialize the mesa context. */
	_mesa_initialize_context(ctx, API_OPENGL, visual,
                                 share_ctx, &functions, NULL);

	nouveau_state_init(ctx);
	nouveau_bo_state_init(ctx);
	nouveau_scratch_init(ctx);
	_mesa_meta_init(ctx);
	_swrast_CreateContext(ctx);
	_vbo_CreateContext(ctx);
	_tnl_CreateContext(ctx);
	nouveau_span_functions_init(ctx);
	_mesa_allow_light_in_model(ctx, GL_FALSE);

	/* Allocate a hardware channel. */
	ret = nouveau_channel_alloc(context_dev(ctx), 0xbeef0201, 0xbeef0202,
				    512*1024, &nctx->hw.chan);
	if (ret) {
		nouveau_error("Error initializing the FIFO.\n");
		return GL_FALSE;
	}

	nctx->hw.chan->flush_notify = nouveau_channel_flush_notify;
	nctx->hw.chan->user_private = nctx;

	/* Enable any supported extensions. */
	driInitExtensions(ctx, nouveau_extensions, GL_TRUE);

	return GL_TRUE;
}

void
nouveau_context_deinit(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	if (TNL_CONTEXT(ctx))
		_tnl_DestroyContext(ctx);

	if (vbo_context(ctx))
		_vbo_DestroyContext(ctx);

	if (SWRAST_CONTEXT(ctx))
		_swrast_DestroyContext(ctx);

	if (ctx->Meta)
		_mesa_meta_free(ctx);

	if (nctx->hw.chan)
		nouveau_channel_free(&nctx->hw.chan);

	nouveau_scratch_destroy(ctx);
	nouveau_bo_state_destroy(ctx);
	_mesa_free_context_data(ctx);
}

void
nouveau_context_destroy(__DRIcontext *dri_ctx)
{
	struct nouveau_context *nctx = dri_ctx->driverPrivate;
	struct gl_context *ctx = &nctx->base;

	context_drv(ctx)->context_destroy(ctx);
}

void
nouveau_update_renderbuffers(__DRIcontext *dri_ctx, __DRIdrawable *draw)
{
	struct gl_context *ctx = dri_ctx->driverPrivate;
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	__DRIscreen *screen = dri_ctx->driScreenPriv;
	struct gl_framebuffer *fb = draw->driverPrivate;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	unsigned int attachments[10];
	__DRIbuffer *buffers = NULL;
	int i = 0, count, ret;

	if (draw->lastStamp == *draw->pStamp)
		return;
	draw->lastStamp = *draw->pStamp;

	if (nfb->need_front)
		attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
	if (fb->Visual.doubleBufferMode)
		attachments[i++] = __DRI_BUFFER_BACK_LEFT;
	if (fb->Visual.haveDepthBuffer && fb->Visual.haveStencilBuffer)
		attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
	else if (fb->Visual.haveDepthBuffer)
		attachments[i++] = __DRI_BUFFER_DEPTH;
	else if (fb->Visual.haveStencilBuffer)
		attachments[i++] = __DRI_BUFFER_STENCIL;

	buffers = (*screen->dri2.loader->getBuffers)(draw, &draw->w, &draw->h,
						     attachments, i, &count,
						     draw->loaderPrivate);
	if (buffers == NULL)
		return;

	for (i = 0; i < count; i++) {
		struct gl_renderbuffer *rb;
		struct nouveau_surface *s;
		uint32_t old_name;
		int index;

		switch (buffers[i].attachment) {
		case __DRI_BUFFER_FRONT_LEFT:
		case __DRI_BUFFER_FAKE_FRONT_LEFT:
			index = BUFFER_FRONT_LEFT;
			break;
		case __DRI_BUFFER_BACK_LEFT:
			index = BUFFER_BACK_LEFT;
			break;
		case __DRI_BUFFER_DEPTH:
		case __DRI_BUFFER_DEPTH_STENCIL:
			index = BUFFER_DEPTH;
			break;
		case __DRI_BUFFER_STENCIL:
			index = BUFFER_STENCIL;
			break;
		default:
			assert(0);
		}

		rb = fb->Attachment[index].Renderbuffer;
		s = &to_nouveau_renderbuffer(rb)->surface;

		s->width = draw->w;
		s->height = draw->h;
		s->pitch = buffers[i].pitch;
		s->cpp = buffers[i].cpp;

		if (index == BUFFER_DEPTH && s->bo) {
			ret = nouveau_bo_handle_get(s->bo, &old_name);
			/*
			 * Disable fast Z clears in the next frame, the
			 * depth buffer contents are undefined.
			 */
			if (!ret && old_name != buffers[i].name)
				nctx->hierz.clear_seq = 0;
		}

		nouveau_bo_ref(NULL, &s->bo);
		ret = nouveau_bo_handle_ref(context_dev(ctx),
					    buffers[i].name, &s->bo);
		assert(!ret);
	}

	_mesa_resize_framebuffer(ctx, fb, draw->w, draw->h);
}

static void
update_framebuffer(__DRIcontext *dri_ctx, __DRIdrawable *draw,
		   int *stamp)
{
	struct gl_context *ctx = dri_ctx->driverPrivate;
	struct gl_framebuffer *fb = draw->driverPrivate;

	*stamp = *draw->pStamp;

	nouveau_update_renderbuffers(dri_ctx, draw);
	_mesa_resize_framebuffer(ctx, fb, draw->w, draw->h);

	/* Clean up references to the old framebuffer objects. */
	context_dirty(ctx, FRAMEBUFFER);
	context_bctx(ctx, FRAMEBUFFER);
	FIRE_RING(context_chan(ctx));
}

GLboolean
nouveau_context_make_current(__DRIcontext *dri_ctx, __DRIdrawable *dri_draw,
			     __DRIdrawable *dri_read)
{
	if (dri_ctx) {
		struct nouveau_context *nctx = dri_ctx->driverPrivate;
		struct gl_context *ctx = &nctx->base;

		/* Ask the X server for new renderbuffers. */
		if (dri_draw->driverPrivate != ctx->WinSysDrawBuffer)
			update_framebuffer(dri_ctx, dri_draw,
					   &dri_ctx->dri2.draw_stamp);

		if (dri_draw != dri_read &&
		    dri_read->driverPrivate != ctx->WinSysReadBuffer)
			update_framebuffer(dri_ctx, dri_read,
					   &dri_ctx->dri2.read_stamp);

		/* Pass it down to mesa. */
		_mesa_make_current(ctx, dri_draw->driverPrivate,
				   dri_read->driverPrivate);
		_mesa_update_state(ctx);

	} else {
		_mesa_make_current(NULL, NULL, NULL);
	}

	return GL_TRUE;
}

GLboolean
nouveau_context_unbind(__DRIcontext *dri_ctx)
{
	/* Unset current context and dispatch table */
	_mesa_make_current(NULL, NULL, NULL);

	return GL_TRUE;
}

void
nouveau_fallback(struct gl_context *ctx, enum nouveau_fallback mode)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nctx->fallback = MAX2(HWTNL, mode);

	if (mode < SWRAST) {
		nouveau_state_emit(ctx);
		nouveau_bo_state_emit(ctx);
	} else {
		FIRE_RING(context_chan(ctx));
	}
}

static void
validate_framebuffer(__DRIcontext *dri_ctx, __DRIdrawable *draw,
		     int *stamp)
{
	struct gl_framebuffer *fb = draw->driverPrivate;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	GLboolean need_front =
		(fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT ||
		 fb->_ColorReadBufferIndex == BUFFER_FRONT_LEFT);

	if (nfb->need_front != need_front) {
		nfb->need_front = need_front;
		dri2InvalidateDrawable(draw);
	}

	if (*draw->pStamp != *stamp)
		update_framebuffer(dri_ctx, draw, stamp);
}

void
nouveau_validate_framebuffer(struct gl_context *ctx)
{
	__DRIcontext *dri_ctx = to_nouveau_context(ctx)->dri_context;
	__DRIdrawable *dri_draw = dri_ctx->driDrawablePriv;
	__DRIdrawable *dri_read = dri_ctx->driReadablePriv;

	if (ctx->DrawBuffer->Name == 0)
		validate_framebuffer(dri_ctx, dri_draw,
				     &dri_ctx->dri2.draw_stamp);

	if (ctx->ReadBuffer->Name == 0)
		validate_framebuffer(dri_ctx, dri_read,
				     &dri_ctx->dri2.read_stamp);

	if (ctx->NewState & _NEW_BUFFERS)
		_mesa_update_state(ctx);
}
