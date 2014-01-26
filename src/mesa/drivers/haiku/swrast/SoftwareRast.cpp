/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include <kernel/image.h>
#include "SoftwareRast.h"

#include <Autolock.h>
#include <interface/DirectWindowPrivate.h>
#include <GraphicsDefs.h>
#include <Screen.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "extensions.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "main/api_exec.h"
#include "main/colormac.h"
#include "main/cpuinfo.h"
#include "main/buffers.h"
#include "main/formats.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/version.h"
#include "main/vtxfmt.h"
#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "vbo/vbo.h"


#ifdef DEBUG
#	define TRACE(x...) printf("MesaSoftwareRast: " x)
#	define CALLED() printf("MesaSoftwareRast: %s\n", __PRETTY_FUNCTION__)
#else
#	define TRACE(x...)
#	define CALLED()
#endif

#define ERROR(x...) printf("MesaSoftwareRast: " x)
}


extern const char* color_space_name(color_space space);


extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView* view, ulong options,
	BGLDispatcher* dispatcher)
{
	return new MesaSoftwareRast(view, options, dispatcher);
}


MesaSoftwareRast::MesaSoftwareRast(BGLView* view, ulong options,
	BGLDispatcher* dispatcher)
	: BGLRenderer(view, options, dispatcher),
	fBitmap(NULL),
	fDirectModeEnabled(false),
	fInfo(NULL),
	fInfoLocker("info locker"),
	fVisual(NULL),
	fFrameBuffer(NULL),
	fFrontRenderBuffer(NULL),
	fBackRenderBuffer(NULL),
	fColorSpace(B_NO_COLOR_SPACE)
{
	CALLED();

	fColorSpace = BScreen(GLView()->Window()).ColorSpace();

	// We force single buffering for the time being
	options &= ~BGL_DOUBLE;

	const GLboolean rgbFlag = ((options & BGL_INDEX) == 0);
	const GLboolean alphaFlag = ((options & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag = ((options & BGL_DOUBLE) == BGL_DOUBLE);
	const GLboolean stereoFlag = false;
	const GLint depth = (options & BGL_DEPTH) ? 16 : 0;
	const GLint stencil = (options & BGL_STENCIL) ? 8 : 0;
	const GLint accum = (options & BGL_ACCUM) ? 16 : 0;
	const GLint red = rgbFlag ? 8 : 0;
	const GLint green = rgbFlag ? 8 : 0;
	const GLint blue = rgbFlag ? 8 : 0;
	const GLint alpha = alphaFlag ? 8 : 0;

	fOptions = options; // | BGL_INDIRECT;
	struct dd_function_table functions;

	fVisual = _mesa_create_visual(dblFlag, stereoFlag, red, green,
		blue, alpha, depth, stencil, accum, accum, accum,
		alpha ? accum : 0, 1);

	// Initialize device driver function table
	_mesa_init_driver_functions(&functions);

	functions.GetString = _GetString;
	functions.UpdateState = _UpdateState;
	functions.MapRenderbuffer = _RenderBufferMap;
	functions.Flush = _Flush;

	// create core context
	// We inherit gl_context to this class
	_mesa_initialize_context(this, API_OPENGL_COMPAT, fVisual, NULL,
		&functions);

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext(this);
	_vbo_CreateContext(this);
	_tnl_CreateContext(this);
	_swsetup_CreateContext(this);
	_swsetup_Wakeup(this);

	// Use default TCL pipeline
	TNL_CONTEXT(this)->Driver.RunPipeline = _tnl_run_pipeline;

	_mesa_meta_init(this);
	_mesa_enable_sw_extensions(this);

	_mesa_compute_version(this);

	_mesa_initialize_dispatch_tables(this);
	_mesa_initialize_vbo_vtxfmt(this);

	// create core framebuffer
	fFrameBuffer = _mesa_create_framebuffer(fVisual);
	if (fFrameBuffer == NULL) {
		ERROR("%s: Unable to calloc GL FrameBuffer!\n", __func__);
		_mesa_destroy_visual(fVisual);
		return;
	}

	// Setup front render buffer
	fFrontRenderBuffer = _NewRenderBuffer(true);
	if (fFrontRenderBuffer == NULL) {
		ERROR("%s: FrontRenderBuffer is requested but unallocated!\n",
			__func__);
		_mesa_destroy_visual(fVisual);
		free(fFrameBuffer);
		return;
	}
	_mesa_add_renderbuffer(fFrameBuffer, BUFFER_FRONT_LEFT,
		&fFrontRenderBuffer->Base);

	// Setup back render buffer (if requested)
	if (fVisual->doubleBufferMode) {
		fBackRenderBuffer = _NewRenderBuffer(false);
		if (fBackRenderBuffer == NULL) {
			ERROR("%s: BackRenderBuffer is requested but unallocated!\n",
				__func__);
			_mesa_destroy_visual(fVisual);
			free(fFrameBuffer);
			return;
		}
		_mesa_add_renderbuffer(fFrameBuffer, BUFFER_BACK_LEFT,
			&fBackRenderBuffer->Base);
	}

	_swrast_add_soft_renderbuffers(fFrameBuffer, GL_FALSE,
		fVisual->haveDepthBuffer, fVisual->haveStencilBuffer,
		fVisual->haveAccumBuffer, alphaFlag, GL_FALSE);

	BRect bounds = view->Bounds();
	fWidth = (GLint)bounds.Width();
	fHeight = (GLint)bounds.Height();

	// some stupid applications (Quake2) don't even think about calling LockGL()
	// before using glGetString and its glGet*() friends...
	// so make sure there is at least a valid context.

	if (!_mesa_get_current_context()) {
		LockGL();
		// not needed, we don't have a looper yet: UnlockLooper();
	}
}


MesaSoftwareRast::~MesaSoftwareRast()
{
	CALLED();
	_swsetup_DestroyContext(this);
	_swrast_DestroyContext(this);
	_tnl_DestroyContext(this);
	_vbo_DestroyContext(this);
	_mesa_destroy_visual(fVisual);
	_mesa_destroy_framebuffer(fFrameBuffer);
	_mesa_destroy_context(this);

	free(fInfo);
	free(fFrameBuffer);

	delete fBitmap;
}


void
MesaSoftwareRast::LockGL()
{
	CALLED();
	BGLRenderer::LockGL();

	_mesa_make_current(this, fFrameBuffer, fFrameBuffer);

	color_space colorSpace = BScreen(GLView()->Window()).ColorSpace();

	GLuint width = fWidth;
	GLuint height = fHeight;

	BAutolock lock(fInfoLocker);
	if (fDirectModeEnabled && fInfo != NULL) {
		width = fInfo->window_bounds.right
			- fInfo->window_bounds.left + 1;
		height = fInfo->window_bounds.bottom
			- fInfo->window_bounds.top + 1;
	}

	if (fColorSpace != colorSpace) {
		fColorSpace = colorSpace;
		_SetupRenderBuffer(&fFrontRenderBuffer->Base, fColorSpace);
		if (fVisual->doubleBufferMode)
			_SetupRenderBuffer(&fBackRenderBuffer->Base, fColorSpace);
	}

	_CheckResize(width, height);
}


void
MesaSoftwareRast::UnlockGL()
{
	CALLED();
	_mesa_make_current(this, NULL, NULL);
	BGLRenderer::UnlockGL();
}


void
MesaSoftwareRast::SwapBuffers(bool VSync)
{
	CALLED();

	if (!fBitmap)
		return;

	if (fVisual->doubleBufferMode)
		_mesa_notifySwapBuffers(this);

	if (!fDirectModeEnabled || fInfo == NULL) {
		if (GLView()->LockLooperWithTimeout(1000) == B_OK) {
			GLView()->DrawBitmap(fBitmap, B_ORIGIN);
			GLView()->UnlockLooper();
		}
	} else {
		// TODO: Here the BGLView needs to be drawlocked.
		_CopyToDirect();
	}

	if (VSync) {
		BScreen screen(GLView()->Window());
		screen.WaitForRetrace();
	}
}


void
MesaSoftwareRast::Draw(BRect updateRect)
{
	CALLED();
	if (fBitmap && (!fDirectModeEnabled || (fInfo == NULL)))
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
MesaSoftwareRast::CopyPixelsOut(BPoint location, BBitmap* bitmap)
{
	CALLED();
	color_space scs = fBitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsOut(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = fBitmap->Bounds();
	BRect dr = bitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8* ps = (uint8*)fBitmap->Bits();
	uint8* pd = (uint8*)bitmap->Bits();
	uint32* s;
	uint32* d;
	uint32 y;
	for (y = (uint32)sr.top; y <= (uint32)sr.bottom; y++) {
		s = (uint32*)(ps + y * fBitmap->BytesPerRow());
		s += (uint32)sr.left;

		d = (uint32*)(pd + (y + (uint32)(dr.top - sr.top))
			* bitmap->BytesPerRow());
		d += (uint32)dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


status_t
MesaSoftwareRast::CopyPixelsIn(BBitmap* bitmap, BPoint location)
{
	CALLED();
	color_space scs = bitmap->ColorSpace();
	color_space dcs = fBitmap->ColorSpace();

	if (scs != dcs && (dcs != B_RGBA32 || scs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsIn(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = bitmap->Bounds();
	BRect dr = fBitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8* ps = (uint8*)bitmap->Bits();
	uint8* pd = (uint8*)fBitmap->Bits();
	uint32* s;
	uint32* d;
	uint32 y;
	for (y = (uint32)sr.top; y <= (uint32)sr.bottom; y++) {
		s = (uint32*)(ps + y * bitmap->BytesPerRow());
		s += (uint32)sr.left;

		d = (uint32*)(pd + (y + (uint32)(dr.top - sr.top))
			* fBitmap->BytesPerRow());
		d += (uint32)dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


void
MesaSoftwareRast::EnableDirectMode(bool enabled)
{
	fDirectModeEnabled = enabled;
}


void
MesaSoftwareRast::DirectConnected(direct_buffer_info* info)
{
	// TODO: I'm not sure we need to do this: BGLView already
	// keeps a local copy of the direct_buffer_info passed by
	// BDirectWindow::DirectConnected().
	BAutolock lock(fInfoLocker);
	if (info) {
		if (!fInfo) {
			fInfo = (direct_buffer_info*)malloc(DIRECT_BUFFER_INFO_AREA_SIZE);
			if (!fInfo)
				return;
		}
		memcpy(fInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
	} else if (fInfo) {
		free(fInfo);
		fInfo = NULL;
	}
}


void
MesaSoftwareRast::FrameResized(float width, float height)
{
	BAutolock lock(fInfoLocker);
	_CheckResize((GLuint)width, (GLuint)height);
}


void
MesaSoftwareRast::_CheckResize(GLuint newWidth, GLuint newHeight)
{
	CALLED();

	if (fBitmap && newWidth == fWidth
		&& newHeight == fHeight) {
		return;
	}

	_mesa_resize_framebuffer(this, fFrameBuffer, newWidth, newHeight);
	fHeight = newHeight;
	fWidth = newWidth;

	_AllocateBitmap();
}


void
MesaSoftwareRast::_AllocateBitmap()
{
	CALLED();

	// allocate new size of back buffer bitmap
	delete fBitmap;
	fBitmap = NULL;

	if (fWidth < 1 || fHeight < 1) {
		TRACE("%s: Cannot allocate bitmap < 1x1!\n", __func__);
		return;
	}

	BRect rect(0.0, 0.0, fWidth - 1, fHeight - 1);
	fBitmap = new BBitmap(rect, fColorSpace);

	#if 0
	// Used for platform optimized drawing
	for (uint i = 0; i < fHeight; i++) {
		fRowAddr[fHeight - i - 1] = (GLvoid *)((GLubyte *)fBitmap->Bits()
			+ i * fBitmap->BytesPerRow());
	}
	#endif

	fFrameBuffer->Width = fWidth;
	fFrameBuffer->Height = fHeight;
	TRACE("%s: Bitmap Size: %" B_PRIu32 "\n", __func__, fBitmap->BitsLength());

	fFrontRenderBuffer->Buffer = (GLubyte*)fBitmap->Bits();
}


// #pragma mark - static


const GLubyte*
MesaSoftwareRast::_GetString(gl_context* ctx, GLenum name)
{
	switch (name) {
		case GL_VENDOR:
			return (const GLubyte*) "Mesa Project";
		case GL_RENDERER:
			return (const GLubyte*) "Software Rasterizer";
		default:
			// Let core library handle all other cases
			return NULL;
	}
}


void
MesaSoftwareRast::_UpdateState(gl_context* ctx, GLuint new_state)
{
	if (!ctx)
		return;

	CALLED();
	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_vbo_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
}


GLboolean
MesaSoftwareRast::_RenderBufferStorage(gl_context* ctx,
	struct gl_renderbuffer* render, GLenum internalFormat,
	GLuint width, GLuint height)
{
	CALLED();

	render->Width = width;
	render->Height = height;

	struct swrast_renderbuffer *swRenderBuffer = swrast_renderbuffer(render);

	swRenderBuffer->RowStride = width * _mesa_get_format_bytes(render->Format);

	return GL_TRUE;
}


GLboolean
MesaSoftwareRast::_RenderBufferStorageMalloc(gl_context* ctx,
	struct gl_renderbuffer* render, GLenum internalFormat,
	GLuint width, GLuint height)
{
	CALLED();

	render->Width = width;
	render->Height = height;

	struct swrast_renderbuffer *swRenderBuffer = swrast_renderbuffer(render);

	if (swRenderBuffer != NULL) {
		free(swRenderBuffer->Buffer);
		swRenderBuffer->RowStride
			= width * _mesa_get_format_bytes(render->Format);

		uint32 size = swRenderBuffer->RowStride * height;
		TRACE("%s: Allocate %" B_PRIu32 " bytes for RenderBuffer\n",
			__func__, size);
		swRenderBuffer->Buffer = (GLubyte*)malloc(size);
		if (!swRenderBuffer->Buffer) {
			ERROR("%s: Memory allocation failure!\n", __func__);
			return GL_FALSE;
		}
	} else {
		ERROR("%s: Couldn't obtain software renderbuffer!\n",
			__func__);
		return GL_FALSE;
	}

	return GL_TRUE;
}


void
MesaSoftwareRast::_Flush(gl_context* ctx)
{
	CALLED();
	MesaSoftwareRast* driverContext = static_cast<MesaSoftwareRast*>(ctx);

	//MesaSoftwareRast* driverContext = (MesaSoftwareRast*)ctx->DriverCtx;
	if ((driverContext->fOptions & BGL_DOUBLE) == 0) {
		// TODO: SwapBuffers() can call _CopyToDirect(), which should
		// be always called with with the BGLView drawlocked.
		// This is not always the case if called from here.
		driverContext->SwapBuffers();
	}
}


struct swrast_renderbuffer*
MesaSoftwareRast::_NewRenderBuffer(bool front)
{
	CALLED();
	struct swrast_renderbuffer *swRenderBuffer
		= (struct swrast_renderbuffer*)calloc(1, sizeof *swRenderBuffer);

	if (!swRenderBuffer) {
		ERROR("%s: Failed calloc RenderBuffer\n", __func__);
		return NULL;
	}

	_mesa_init_renderbuffer(&swRenderBuffer->Base, 0);

	swRenderBuffer->Base.ClassID = HAIKU_SWRAST_RENDERBUFFER_CLASS;
	swRenderBuffer->Base.RefCount = 1;
	swRenderBuffer->Base.Delete = _RenderBufferDelete;

	if (!front)
		swRenderBuffer->Base.AllocStorage = _RenderBufferStorageMalloc;
	else
		swRenderBuffer->Base.AllocStorage = _RenderBufferStorage;

	if (_SetupRenderBuffer(&swRenderBuffer->Base, fColorSpace) != B_OK) {
		free(swRenderBuffer);
		return NULL;
	}

	return swRenderBuffer;
}


status_t
MesaSoftwareRast::_SetupRenderBuffer(struct gl_renderbuffer* rb,
	color_space colorSpace)
{
	CALLED();

	rb->InternalFormat = GL_RGBA;

	switch (colorSpace) {
		case B_RGBA32:
			rb->_BaseFormat = GL_RGBA;
			rb->Format = MESA_FORMAT_B8G8R8A8_UNORM;
			break;
		case B_RGB32:
			rb->_BaseFormat = GL_RGB;
			rb->Format = MESA_FORMAT_B8G8R8X8_UNORM;
			break;
		case B_RGB24:
			rb->_BaseFormat = GL_RGB;
			rb->Format = MESA_FORMAT_BGR_UNORM8;
			break;
		case B_RGB16:
			rb->_BaseFormat = GL_RGB;
			rb->Format = MESA_FORMAT_B5G6R5_UNORM;
			break;
		case B_RGB15:
			rb->_BaseFormat = GL_RGB;
			rb->Format = MESA_FORMAT_B5G5R5A1_UNORM;
			break;
		default:
			fprintf(stderr, "Unsupported screen color space %s\n",
				color_space_name(fColorSpace));
			debugger("Unsupported OpenGL color space");
			return B_ERROR;
	}
	return B_OK;
}


/*!	Y inverted Map RenderBuffer function
	We use a BBitmap for storage which has Y inverted.
	If the Mesa provided Map function ever allows external
	control of this we can omit this function.
*/
void
MesaSoftwareRast::_RenderBufferMap(gl_context *ctx,
	struct gl_renderbuffer *rb, GLuint x, GLuint y, GLuint w, GLuint h,
	GLbitfield mode, GLubyte **mapOut, GLint *rowStrideOut)
{
	if (rb->ClassID == HAIKU_SWRAST_RENDERBUFFER_CLASS) {
		struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
		const GLuint bpp = _mesa_get_format_bytes(rb->Format);
		GLint rowStride = rb->Width * bpp; // in Bytes

		y = rb->Height - y - 1;

		*rowStrideOut = -rowStride;
		*mapOut = (GLubyte *) srb->Buffer + y * rowStride + x * bpp;
	} else {
		_swrast_map_soft_renderbuffer(ctx, rb, x, y, w, h, mode,
			mapOut, rowStrideOut);
	}
}


void
MesaSoftwareRast::_RenderBufferDelete(struct gl_context *ctx,
	struct gl_renderbuffer* rb)
{
	CALLED();
	if (rb != NULL) {
		struct swrast_renderbuffer *swRenderBuffer
			= swrast_renderbuffer(rb);
		if (swRenderBuffer != NULL)
			free(swRenderBuffer->Buffer);
	}
	free(rb);
}


void
MesaSoftwareRast::_CopyToDirect()
{
	BAutolock lock(fInfoLocker);

	// check the bitmap size still matches the size
	if (fInfo->window_bounds.bottom - fInfo->window_bounds.top
		!= fBitmap->Bounds().IntegerHeight()
		|| fInfo->window_bounds.right - fInfo->window_bounds.left
			!= fBitmap->Bounds().IntegerWidth())
		return;

	uint8 bytesPerPixel = fInfo->bits_per_pixel / 8;
	uint32 bytesPerRow = fBitmap->BytesPerRow();
	for (uint32 i = 0; i < fInfo->clip_list_count; i++) {
		clipping_rect *clip = &fInfo->clip_list[i];
		int32 height = clip->bottom - clip->top + 1;
		int32 bytesWidth
			= (clip->right - clip->left + 1) * bytesPerPixel;
		uint8* p = (uint8*)fInfo->bits + clip->top
			* fInfo->bytes_per_row + clip->left * bytesPerPixel;
		uint8* b = (uint8*)fBitmap->Bits()
			+ (clip->top - fInfo->window_bounds.top) * bytesPerRow
			+ (clip->left - fInfo->window_bounds.left)
				* bytesPerPixel;

		for (int y = 0; y < height; y++) {
			memcpy(p, b, bytesWidth);
			p += fInfo->bytes_per_row;
			b += bytesPerRow;
		}
	}
}
