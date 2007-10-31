/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
//#include "nouveau_state.h"
#include "nouveau_lock.h"
#include "nouveau_fifo.h"
#include "nouveau_driver.h"
#include "swrast/swrast.h"

#include "context.h"
#include "framebuffer.h"

#include "utils.h"
#include "colormac.h"

/* Wrapper for DRM_NOUVEAU_GETPARAM ioctl */
GLboolean nouveauDRMGetParam(nouveauContextPtr nmesa,
			     unsigned int      param,
			     uint64_t*         value)
{
	struct drm_nouveau_getparam getp;

	getp.param = param;
	if (!value || drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_GETPARAM,
					  &getp, sizeof(getp)))
		return GL_FALSE;
	*value = getp.value;
	return GL_TRUE;
}

/* Wrapper for DRM_NOUVEAU_GETPARAM ioctl */
GLboolean nouveauDRMSetParam(nouveauContextPtr nmesa,
			     unsigned int      param,
			     uint64_t          value)
{
	struct drm_nouveau_setparam setp;

	setp.param = param;
	setp.value = value;
	if (drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_SETPARAM, &setp,
				sizeof(setp)))
		return GL_FALSE;
	return GL_TRUE;
}

/* Return the width and height of the current color buffer */
static void nouveauGetBufferSize( GLframebuffer *buffer,
		GLuint *width, GLuint *height )
{
	GET_CURRENT_CONTEXT(ctx);
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	LOCK_HARDWARE( nmesa );
	*width  = nmesa->driDrawable->w;
	*height = nmesa->driDrawable->h;
	UNLOCK_HARDWARE( nmesa );
}

/* glGetString */
static const GLubyte *nouveauGetString( GLcontext *ctx, GLenum name )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	static char buffer[128];
	const char * card_name = "Unknown";
	GLuint agp_mode = 0;

	switch ( name ) {
		case GL_VENDOR:
			return (GLubyte *)DRIVER_AUTHOR;

		case GL_RENDERER:
			card_name=nmesa->screen->card->name;

			switch(nmesa->screen->bus_type)
			{
				case NV_PCI:
				case NV_PCIE:
				default:
					agp_mode=0;
					break;
				case NV_AGP:
					agp_mode=nmesa->screen->agp_mode;
					break;
			}
			driGetRendererString( buffer, card_name, DRIVER_DATE,
					agp_mode );
			return (GLubyte *)buffer;
		default:
			return NULL;
	}
}

/* glFlush */
static void nouveauFlush( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	if (ctx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT)
		nouveauDoSwapBuffers(nmesa, nmesa->driDrawable);
	FIRE_RING();
}

/* glFinish */
static void nouveauFinish( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	nouveauFlush( ctx );
	nouveauWaitForIdle( nmesa );
}

/* glClear */
static void nouveauClear( GLcontext *ctx, GLbitfield mask )
{
	uint32_t clear_value;
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	/* FIXME: should we clear front buffer, even if asked to do it? */
	if (mask & (BUFFER_BIT_FRONT_LEFT|BUFFER_BIT_BACK_LEFT)) {
		GLubyte c[4];
		int color_bits = 32;
		int color_mask = 0xffffffff;

		UNCLAMPED_FLOAT_TO_RGBA_CHAN(c,ctx->Color.ClearColor);
		clear_value = PACK_COLOR_8888(c[3],c[0],c[1],c[2]);

		if (ctx->DrawBuffer) {
			/* FIXME: find correct color buffer, instead of [0][0] */
			if (ctx->DrawBuffer->_ColorDrawBuffers[0][0]) {
				color_bits = ctx->DrawBuffer->_ColorDrawBuffers[0][0]->RedBits;
				color_bits += ctx->DrawBuffer->_ColorDrawBuffers[0][0]->GreenBits;
				color_bits += ctx->DrawBuffer->_ColorDrawBuffers[0][0]->BlueBits;
				color_bits += ctx->DrawBuffer->_ColorDrawBuffers[0][0]->AlphaBits;
			}
		}

		if (color_bits<24) {
			clear_value = PACK_COLOR_565(c[0],c[1],c[2]);
			color_mask = 0xffff;
		}

		nouveauClearBuffer(ctx, nmesa->color_buffer,
			clear_value, color_mask);
	}

	if (mask & (BUFFER_BIT_DEPTH)) {
		int depth_bits = 24;
		int depth_mask;
		if (ctx->DrawBuffer) {
			if (ctx->DrawBuffer->_DepthBuffer) {
				depth_bits = ctx->DrawBuffer->_DepthBuffer->DepthBits;
			}
		}

		switch(depth_bits) {
			case 16:
				clear_value = (uint32_t) (ctx->Depth.Clear * 32767.0);
				depth_mask = 0xffff;
				break;
			default:
				clear_value = ((uint32_t) (ctx->Depth.Clear * 16777215.0)) << 8;
				depth_mask = 0xffffff00;
				break;
		}

		nouveauClearBuffer(ctx, nmesa->depth_buffer,
			clear_value, depth_mask);
	}

	if (mask & (BUFFER_BIT_STENCIL)) {
		int stencil_bits = 0;
		if (ctx->DrawBuffer) {
			if (ctx->DrawBuffer->_StencilBuffer) {
				stencil_bits = ctx->DrawBuffer->_StencilBuffer->StencilBits;
			}
		}

		if (stencil_bits>0) {
			nouveauClearBuffer(ctx, nmesa->depth_buffer,
				ctx->Stencil.Clear, (1<<stencil_bits)-1);
		}	
	}
}

void nouveauDriverInitFunctions( struct dd_function_table *functions )
{
	functions->GetBufferSize	= nouveauGetBufferSize;
	functions->ResizeBuffers	= _mesa_resize_framebuffer;
	functions->GetString		= nouveauGetString;
	functions->Flush		= nouveauFlush;
	functions->Finish		= nouveauFinish;
	functions->Clear		= nouveauClear;
}

