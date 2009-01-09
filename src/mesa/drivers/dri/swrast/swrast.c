/*
 * Copyright (C) 2008 George Sapountzis <gsap7@yahoo.gr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DRI software rasterizer
 *
 * This is the mesa swrast module packaged into a DRI driver structure.
 *
 * The front-buffer is allocated by the loader. The loader provides read/write
 * callbacks for access to the front-buffer. The driver uses a scratch row for
 * front-buffer rendering to avoid repeated calls to the loader.
 *
 * The back-buffer is allocated by the driver and is private.
 */

#include "main/context.h"
#include "main/extensions.h"
#include "main/framebuffer.h"
#include "main/imports.h"
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "vbo/vbo.h"
#include "drivers/common/driverfuncs.h"
#include "utils.h"

#include "swrast_priv.h"


#define need_GL_VERSION_1_3
#define need_GL_VERSION_1_4
#define need_GL_VERSION_1_5
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1

/* sw extensions for imaging */
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_convolution
#define need_GL_EXT_histogram
#define need_GL_SGI_color_table

/* sw extensions not associated with some GL version */
#define need_GL_ARB_shader_objects
#define need_GL_ARB_vertex_program
#define need_GL_APPLE_vertex_array_object
#define need_GL_ATI_fragment_shader
#define need_GL_ATI_separate_stencil
#define need_GL_EXT_depth_bounds_test
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_framebuffer_blit
#define need_GL_EXT_gpu_program_parameters
#define need_GL_EXT_paletted_texture
#define need_GL_EXT_stencil_two_side
#define need_GL_IBM_multimode_draw_arrays
#define need_GL_MESA_resize_buffers
#define need_GL_NV_vertex_program
#define need_GL_NV_fragment_program

#include "extension_helper.h"

const struct dri_extension card_extensions[] =
{
    { "GL_VERSION_1_3",			GL_VERSION_1_3_functions },
    { "GL_VERSION_1_4",			GL_VERSION_1_4_functions },
    { "GL_VERSION_1_5",			GL_VERSION_1_5_functions },
    { "GL_VERSION_2_0",			GL_VERSION_2_0_functions },
    { "GL_VERSION_2_1",			GL_VERSION_2_1_functions },

    { "GL_EXT_blend_color",		GL_EXT_blend_color_functions },
    { "GL_EXT_blend_minmax",		GL_EXT_blend_minmax_functions },
    { "GL_EXT_convolution",		GL_EXT_convolution_functions },
    { "GL_EXT_histogram",		GL_EXT_histogram_functions },
    { "GL_SGI_color_table",		GL_SGI_color_table_functions },

    { "GL_ARB_shader_objects",		GL_ARB_shader_objects_functions },
    { "GL_ARB_vertex_program",		GL_ARB_vertex_program_functions },
    { "GL_APPLE_vertex_array_object",	GL_APPLE_vertex_array_object_functions },
    { "GL_ATI_fragment_shader",		GL_ATI_fragment_shader_functions },
    { "GL_ATI_separate_stencil",	GL_ATI_separate_stencil_functions },
    { "GL_EXT_depth_bounds_test",	GL_EXT_depth_bounds_test_functions },
    { "GL_EXT_framebuffer_object",	GL_EXT_framebuffer_object_functions },
    { "GL_EXT_framebuffer_blit",	GL_EXT_framebuffer_blit_functions },
    { "GL_EXT_gpu_program_parameters",	GL_EXT_gpu_program_parameters_functions },
    { "GL_EXT_paletted_texture",	GL_EXT_paletted_texture_functions },
    { "GL_EXT_stencil_two_side",	GL_EXT_stencil_two_side_functions },
    { "GL_IBM_multimode_draw_arrays",	GL_IBM_multimode_draw_arrays_functions },
    { "GL_MESA_resize_buffers",		GL_MESA_resize_buffers_functions },
    { "GL_NV_vertex_program",		GL_NV_vertex_program_functions },
    { "GL_NV_fragment_program",		GL_NV_fragment_program_functions },
    { NULL,				NULL }
};


/**
 * Screen and config-related functions
 */

static void
setupLoaderExtensions(__DRIscreen *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_SWRAST_LOADER) == 0)
	    psp->swrast_loader = (__DRIswrastLoaderExtension *) extensions[i];
    }
}

static __DRIconfig **
swrastFillInModes(__DRIscreen *psp,
		  unsigned pixel_bits, unsigned depth_bits,
		  unsigned stencil_bits, GLboolean have_back_buffer)
{
    __DRIconfig **configs;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    GLenum fb_format;
    GLenum fb_type;

    /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
     * support pageflipping at all.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML
    };

    uint8_t depth_bits_array[4];
    uint8_t stencil_bits_array[4];

    depth_bits_array[0] = 0;
    depth_bits_array[1] = 0;
    depth_bits_array[2] = depth_bits;
    depth_bits_array[3] = depth_bits;

    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.
     */
    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;
    stencil_bits_array[2] = 0;
    stencil_bits_array[3] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = 4;
    back_buffer_factor = 2;

    if (pixel_bits == 8) {
	fb_format = GL_RGB;
	fb_type = GL_UNSIGNED_BYTE_2_3_3_REV;
    }
    else if (pixel_bits == 16) {
	fb_format = GL_RGB;
	fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
	fb_format = GL_BGRA;
	fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    configs = driCreateConfigs(fb_format, fb_type,
			       depth_bits_array, stencil_bits_array,
			       depth_buffer_factor, back_buffer_modes,
			       back_buffer_factor);
    if (configs == NULL) {
	fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
		__LINE__);
	return NULL;
    }

    return configs;
}

static __DRIscreen *
driCreateNewScreen(int scrn, const __DRIextension **extensions,
		   const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;
    __DRIconfig **configs8, **configs16, **configs32;

    (void) data;

    TRACE;

    psp = _mesa_calloc(sizeof(*psp));
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    psp->num = scrn;
    psp->extensions = emptyExtensionList;

    configs8  = swrastFillInModes(psp,  8,  8, 0, 1);
    configs16 = swrastFillInModes(psp, 16, 16, 0, 1);
    configs32 = swrastFillInModes(psp, 32, 24, 8, 1);

    configs16 = (__DRIconfig **)driConcatConfigs(configs8, configs16);

    *driver_configs = driConcatConfigs(configs16, configs32);

    driInitExtensions( NULL, card_extensions, GL_FALSE );

    return psp;
}

static void driDestroyScreen(__DRIscreen *psp)
{
    TRACE;

    if (psp) {
	_mesa_free(psp);
    }
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    TRACE;

    return psp->extensions;
}


/**
 * Framebuffer and renderbuffer-related functions.
 */

static GLuint
choose_pixel_format(const GLvisual *v)
{
    if (v->rgbMode) {
	int bpp = v->rgbBits;

	if (bpp == 32
	    && v->redMask   == 0xff0000
	    && v->greenMask == 0x00ff00
	    && v->blueMask  == 0x0000ff)
	    return PF_A8R8G8B8;
	else if (bpp == 16
	    && v->redMask   == 0xf800
	    && v->greenMask == 0x07e0
	    && v->blueMask  == 0x001f)
	    return PF_R5G6B5;
	else if (bpp == 8
	    && v->redMask   == 0x07
	    && v->greenMask == 0x38
	    && v->blueMask  == 0xc0)
	    return PF_R3G3B2;
    }
    else {
	if (v->indexBits == 8)
	    return PF_CI8;
    }

    _mesa_problem( NULL, "unexpected format in %s", __FUNCTION__ );
    return 0;
}

static void
swrast_delete_renderbuffer(struct gl_renderbuffer *rb)
{
    TRACE;

    _mesa_free(rb->Data);
    _mesa_free(rb);
}

static GLboolean
swrast_alloc_front_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
			   GLenum internalFormat, GLuint width, GLuint height)
{
    struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
    int bpp;
    unsigned mask = PITCH_ALIGN_BITS - 1;

    TRACE;

    rb->Data = NULL;
    rb->Width = width;
    rb->Height = height;

    switch (internalFormat) {
    case GL_RGB:
	bpp = rb->RedBits + rb->GreenBits + rb->BlueBits;
	break;
    case GL_RGBA:
	bpp = rb->RedBits + rb->GreenBits + rb->BlueBits + rb->AlphaBits;
	break;
    case GL_COLOR_INDEX8_EXT:
	bpp = rb->IndexBits;
	break;
    default:
	_mesa_problem( NULL, "unexpected format in %s", __FUNCTION__ );
	return GL_FALSE;
    }

    /* always pad to PITCH_ALIGN_BITS */
    xrb->pitch = ((width * bpp + mask) & ~mask) / 8;

    return GL_TRUE;
}

static GLboolean
swrast_alloc_back_storage(GLcontext *ctx, struct gl_renderbuffer *rb,
			  GLenum internalFormat, GLuint width, GLuint height)
{
    struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);

    TRACE;

    _mesa_free(rb->Data);

    swrast_alloc_front_storage(ctx, rb, internalFormat, width, height);

    rb->Data = _mesa_malloc(height * xrb->pitch);

    return GL_TRUE;
}

static struct swrast_renderbuffer *
swrast_new_renderbuffer(const GLvisual *visual, GLboolean front)
{
    struct swrast_renderbuffer *xrb = _mesa_calloc(sizeof *xrb);
    GLuint pixel_format;

    TRACE;

    if (!xrb)
	return NULL;

    _mesa_init_renderbuffer(&xrb->Base, 0);

    pixel_format = choose_pixel_format(visual);

    xrb->Base.Delete = swrast_delete_renderbuffer;
    if (front) {
	xrb->Base.AllocStorage = swrast_alloc_front_storage;
	swrast_set_span_funcs_front(xrb, pixel_format);
    }
    else {
	xrb->Base.AllocStorage = swrast_alloc_back_storage;
	swrast_set_span_funcs_back(xrb, pixel_format);
    }

    switch (pixel_format) {
    case PF_A8R8G8B8:
	xrb->Base.InternalFormat = GL_RGBA;
	xrb->Base._BaseFormat = GL_RGBA;
	xrb->Base.DataType = GL_UNSIGNED_BYTE;
	xrb->Base.RedBits   = 8 * sizeof(GLubyte);
	xrb->Base.GreenBits = 8 * sizeof(GLubyte);
	xrb->Base.BlueBits  = 8 * sizeof(GLubyte);
	xrb->Base.AlphaBits = 8 * sizeof(GLubyte);
	break;
    case PF_R5G6B5:
	xrb->Base.InternalFormat = GL_RGB;
	xrb->Base._BaseFormat = GL_RGB;
	xrb->Base.DataType = GL_UNSIGNED_BYTE;
	xrb->Base.RedBits   = 5 * sizeof(GLubyte);
	xrb->Base.GreenBits = 6 * sizeof(GLubyte);
	xrb->Base.BlueBits  = 5 * sizeof(GLubyte);
	xrb->Base.AlphaBits = 0;
	break;
    case PF_R3G3B2:
	xrb->Base.InternalFormat = GL_RGB;
	xrb->Base._BaseFormat = GL_RGB;
	xrb->Base.DataType = GL_UNSIGNED_BYTE;
	xrb->Base.RedBits   = 3 * sizeof(GLubyte);
	xrb->Base.GreenBits = 3 * sizeof(GLubyte);
	xrb->Base.BlueBits  = 2 * sizeof(GLubyte);
	xrb->Base.AlphaBits = 0;
	break;
    case PF_CI8:
	xrb->Base.InternalFormat = GL_COLOR_INDEX8_EXT;
	xrb->Base._BaseFormat = GL_COLOR_INDEX;
	xrb->Base.DataType = GL_UNSIGNED_BYTE;
	xrb->Base.IndexBits = 8 * sizeof(GLubyte);
	break;
    default:
	return NULL;
    }

    return xrb;
}

static __DRIdrawable *
driCreateNewDrawable(__DRIscreen *screen,
		     const __DRIconfig *config, void *data)
{
    __DRIdrawable *buf;
    struct swrast_renderbuffer *frontrb, *backrb;

    TRACE;

    buf = _mesa_calloc(sizeof *buf);
    if (!buf)
	return NULL;

    buf->loaderPrivate = data;

    buf->driScreenPriv = screen;

    buf->row = _mesa_malloc(MAX_WIDTH * 4);

    /* basic framebuffer setup */
    _mesa_initialize_framebuffer(&buf->Base, &config->modes);

    /* add front renderbuffer */
    frontrb = swrast_new_renderbuffer(&config->modes, GL_TRUE);
    _mesa_add_renderbuffer(&buf->Base, BUFFER_FRONT_LEFT, &frontrb->Base);

    /* add back renderbuffer */
    if (config->modes.doubleBufferMode) {
	backrb = swrast_new_renderbuffer(&config->modes, GL_FALSE);
	_mesa_add_renderbuffer(&buf->Base, BUFFER_BACK_LEFT, &backrb->Base);
    }

    /* add software renderbuffers */
    _mesa_add_soft_renderbuffers(&buf->Base,
				 GL_FALSE, /* color */
				 config->modes.haveDepthBuffer,
				 config->modes.haveStencilBuffer,
				 config->modes.haveAccumBuffer,
				 GL_FALSE, /* alpha */
				 GL_FALSE /* aux bufs */);

    return buf;
}

static void
driDestroyDrawable(__DRIdrawable *buf)
{
    TRACE;

    if (buf) {
	struct gl_framebuffer *fb = &buf->Base;

	_mesa_free(buf->row);

	fb->DeletePending = GL_TRUE;
	_mesa_unreference_framebuffer(&fb);
    }
}

static void driSwapBuffers(__DRIdrawable *buf)
{
    GET_CURRENT_CONTEXT(ctx);

    struct swrast_renderbuffer *frontrb =
	swrast_renderbuffer(buf->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
    struct swrast_renderbuffer *backrb =
	swrast_renderbuffer(buf->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer);

    __DRIscreen *screen = buf->driScreenPriv;

    TRACE;

    /* check for signle-buffered */
    if (backrb == NULL)
	return;

    /* check if swapping currently bound buffer */
    if (ctx && ctx->DrawBuffer == &(buf->Base)) {
	/* flush pending rendering */
	_mesa_notifySwapBuffers(ctx);
    }

    screen->swrast_loader->putImage(buf, __DRI_SWRAST_IMAGE_OP_SWAP,
				    0, 0,
				    frontrb->Base.Width,
				    frontrb->Base.Height,
				    backrb->Base.Data,
				    buf->loaderPrivate);
}


/**
 * General device driver functions.
 */

static void
get_window_size( GLframebuffer *fb, GLsizei *w, GLsizei *h )
{
    __DRIdrawable *buf = swrast_drawable(fb);
    __DRIscreen *screen = buf->driScreenPriv;
    int x, y;

    screen->swrast_loader->getDrawableInfo(buf,
					   &x, &y, w, h,
					   buf->loaderPrivate);
}

static void
swrast_check_and_update_window_size( GLcontext *ctx, GLframebuffer *fb )
{
    GLsizei width, height;

    get_window_size(fb, &width, &height);
    if (fb->Width != width || fb->Height != height) {
	_mesa_resize_framebuffer(ctx, fb, width, height);
    }
}

static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
    (void) ctx;
    switch (pname) {
	case GL_VENDOR:
	    return (const GLubyte *) "Mesa Project";
	case GL_RENDERER:
	    return (const GLubyte *) "Software Rasterizer";
	default:
	    return NULL;
    }
}

static void
update_state( GLcontext *ctx, GLuint new_state )
{
    /* not much to do here - pass it on */
    _swrast_InvalidateState( ctx, new_state );
    _swsetup_InvalidateState( ctx, new_state );
    _vbo_InvalidateState( ctx, new_state );
    _tnl_InvalidateState( ctx, new_state );
}

static void
viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    GLframebuffer *draw = ctx->WinSysDrawBuffer;
    GLframebuffer *read = ctx->WinSysReadBuffer;

    swrast_check_and_update_window_size(ctx, draw);
    swrast_check_and_update_window_size(ctx, read);
}

static void
swrast_init_driver_functions(struct dd_function_table *driver)
{
    driver->GetString = get_string;
    driver->UpdateState = update_state;
    driver->GetBufferSize = NULL;
    driver->Viewport = viewport;
}


/**
 * Context-related functions.
 */

static __DRIcontext *
driCreateNewContext(__DRIscreen *screen, const __DRIconfig *config,
		    __DRIcontext *shared, void *data)
{
    __DRIcontext *ctx;
    GLcontext *mesaCtx;
    struct dd_function_table functions;

    TRACE;

    ctx = _mesa_calloc(sizeof *ctx);
    if (!ctx)
	return NULL;

    ctx->loaderPrivate = data;

    ctx->driScreenPriv = screen;

    /* build table of device driver functions */
    _mesa_init_driver_functions(&functions);
    swrast_init_driver_functions(&functions);

    if (!_mesa_initialize_context(&ctx->Base, &config->modes,
				  shared ? &shared->Base : NULL,
				  &functions, (void *) ctx)) {
      _mesa_free(ctx);
      return NULL;
    }

    mesaCtx = &ctx->Base;

    /* do bounds checking to prevent segfaults and server crashes! */
    mesaCtx->Const.CheckArrayBounds = GL_TRUE;

    /* create module contexts */
    _swrast_CreateContext( mesaCtx );
    _vbo_CreateContext( mesaCtx );
    _tnl_CreateContext( mesaCtx );
    _swsetup_CreateContext( mesaCtx );
    _swsetup_Wakeup( mesaCtx );

    /* use default TCL pipeline */
    {
       TNLcontext *tnl = TNL_CONTEXT(mesaCtx);
       tnl->Driver.RunPipeline = _tnl_run_pipeline;
    }

    _mesa_enable_sw_extensions(mesaCtx);
    _mesa_enable_1_3_extensions(mesaCtx);
    _mesa_enable_1_4_extensions(mesaCtx);
    _mesa_enable_1_5_extensions(mesaCtx);
    _mesa_enable_2_0_extensions(mesaCtx);
    _mesa_enable_2_1_extensions(mesaCtx);

    return ctx;
}

static void
driDestroyContext(__DRIcontext *ctx)
{
    GLcontext *mesaCtx;
    TRACE;

    if (ctx) {
	mesaCtx = &ctx->Base;
	_swsetup_DestroyContext( mesaCtx );
	_swrast_DestroyContext( mesaCtx );
	_tnl_DestroyContext( mesaCtx );
	_vbo_DestroyContext( mesaCtx );
	_mesa_destroy_context( mesaCtx );
    }
}

static int
driCopyContext(__DRIcontext *dst, __DRIcontext *src, unsigned long mask)
{
    TRACE;

    _mesa_copy_context(&src->Base, &dst->Base, mask);
    return GL_TRUE;
}

static int driBindContext(__DRIcontext *ctx,
			  __DRIdrawable *draw,
			  __DRIdrawable *read)
{
    GLcontext *mesaCtx;
    GLframebuffer *mesaDraw;
    GLframebuffer *mesaRead;
    TRACE;

    if (ctx) {
	if (!draw || !read)
	    return GL_FALSE;

	mesaCtx = &ctx->Base;
	mesaDraw = &draw->Base;
	mesaRead = &read->Base;

	/* check for same context and buffer */
	if (mesaCtx == _mesa_get_current_context()
	    && mesaCtx->DrawBuffer == mesaDraw
	    && mesaCtx->ReadBuffer == mesaRead) {
	    return GL_TRUE;
	}

	_glapi_check_multithread();

	swrast_check_and_update_window_size(mesaCtx, mesaDraw);
	if (read != draw)
	    swrast_check_and_update_window_size(mesaCtx, mesaRead);

	_mesa_make_current( mesaCtx,
			    mesaDraw,
			    mesaRead );
    }
    else {
	/* unbind */
	_mesa_make_current( NULL, NULL, NULL );
    }

    return GL_TRUE;
}

static int driUnbindContext(__DRIcontext *ctx)
{
    TRACE;
    (void) ctx;
    return GL_TRUE;
}


static const __DRIcoreExtension driCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    NULL, /* driCreateNewScreen */
    driDestroyScreen,
    driGetExtensions,
    driGetConfigAttrib,
    driIndexConfigAttrib,
    NULL, /* driCreateNewDrawable */
    driDestroyDrawable,
    driSwapBuffers,
    driCreateNewContext,
    driCopyContext,
    driDestroyContext,
    driBindContext,
    driUnbindContext
};

static const __DRIswrastExtension driSWRastExtension = {
    { __DRI_SWRAST, __DRI_SWRAST_VERSION },
    driCreateNewScreen,
    driCreateNewDrawable
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driSWRastExtension.base,
    NULL
};
