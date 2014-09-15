/*
 * Copyright 2008, 2010 George Sapountzis <gsapountzis@gmail.com>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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

#include "main/api_exec.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/formats.h"
#include "main/framebuffer.h"
#include "main/imports.h"
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
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "utils.h"

#include "main/teximage.h"
#include "main/texformat.h"
#include "main/texstate.h"

#include "swrast_priv.h"
#include "swrast/s_context.h"

const __DRIextension **__driDriverGetExtensions_swrast(void);

const char const *swrast_vendor_string = "Mesa Project";
const char const *swrast_renderer_string = "Software Rasterizer";

/**
 * Screen and config-related functions
 */

static void swrastSetTexBuffer2(__DRIcontext *pDRICtx, GLint target,
				GLint texture_format, __DRIdrawable *dPriv)
{
    struct dri_context *dri_ctx;
    int x, y, w, h;
    __DRIscreen *sPriv = dPriv->driScreenPriv;
    struct gl_texture_object *texObj;
    struct gl_texture_image *texImage;
    struct swrast_texture_image *swImage;
    uint32_t internalFormat;
    mesa_format texFormat;

    dri_ctx = pDRICtx->driverPrivate;

    internalFormat = (texture_format == __DRI_TEXTURE_FORMAT_RGB ? 3 : 4);

    texObj = _mesa_get_current_tex_object(&dri_ctx->Base, target);
    texImage = _mesa_get_tex_image(&dri_ctx->Base, texObj, target, 0);
    swImage = swrast_texture_image(texImage);

    _mesa_lock_texture(&dri_ctx->Base, texObj);

    sPriv->swrast_loader->getDrawableInfo(dPriv, &x, &y, &w, &h, dPriv->loaderPrivate);

    if (texture_format == __DRI_TEXTURE_FORMAT_RGB)
	texFormat = MESA_FORMAT_B8G8R8X8_UNORM;
    else
	texFormat = MESA_FORMAT_B8G8R8A8_UNORM;

    _mesa_init_teximage_fields(&dri_ctx->Base, texImage,
			       w, h, 1, 0, internalFormat, texFormat);

    sPriv->swrast_loader->getImage(dPriv, x, y, w, h, (char *)swImage->Buffer,
				   dPriv->loaderPrivate);

    _mesa_unlock_texture(&dri_ctx->Base, texObj);
}

static void swrastSetTexBuffer(__DRIcontext *pDRICtx, GLint target,
			       __DRIdrawable *dPriv)
{
    swrastSetTexBuffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

static const __DRItexBufferExtension swrastTexBufferExtension = {
   .base = { __DRI_TEX_BUFFER, 3 },

   .setTexBuffer        = swrastSetTexBuffer,
   .setTexBuffer2       = swrastSetTexBuffer2,
   .releaseTexBuffer    = NULL,
};


static int
swrast_query_renderer_integer(__DRIscreen *psp, int param,
			       unsigned int *value)
{
   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
   case __DRI2_RENDERER_DEVICE_ID:
      /* Return 0xffffffff for both vendor and device id */
      value[0] = 0xffffffff;
      return 0;
   case __DRI2_RENDERER_ACCELERATED:
      value[0] = 0;
      return 0;
   case __DRI2_RENDERER_VIDEO_MEMORY: {
      /* XXX: Do we want to return the full amount of system memory ? */
      const long system_memory_pages = sysconf(_SC_PHYS_PAGES);
      const long system_page_size = sysconf(_SC_PAGE_SIZE);

      if (system_memory_pages <= 0 || system_page_size <= 0)
         return -1;

      const uint64_t system_memory_bytes = (uint64_t) system_memory_pages
         * (uint64_t) system_page_size;

      const unsigned system_memory_megabytes =
         (unsigned) (system_memory_bytes / (1024 * 1024));

      value[0] = system_memory_megabytes;
      return 0;
   }
   case __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE:
      /**
       * XXX: Perhaps we should return 1 ?
       * See issue #7 from the spec, currently UNRESOLVED.
       */
      value[0] = 0;
      return 0;
   default:
      return driQueryRendererIntegerCommon(psp, param, value);
   }
}

static int
swrast_query_renderer_string(__DRIscreen *psp, int param, const char **value)
{
   switch (param) {
   case __DRI2_RENDERER_VENDOR_ID:
      value[0] = swrast_vendor_string;
      return 0;
   case __DRI2_RENDERER_DEVICE_ID:
      value[0] = swrast_renderer_string;
      return 0;
   default:
      return -1;
   }
}

static const __DRI2rendererQueryExtension swrast_query_renderer_extension = {
   .base = { __DRI2_RENDERER_QUERY, 1 },

   .queryInteger        = swrast_query_renderer_integer,
   .queryString         = swrast_query_renderer_string
};

static const __DRIextension *dri_screen_extensions[] = {
    &swrastTexBufferExtension.base,
    &swrast_query_renderer_extension.base,
    NULL
};

static __DRIconfig **
swrastFillInModes(__DRIscreen *psp,
		  unsigned pixel_bits, unsigned depth_bits,
		  unsigned stencil_bits, GLboolean have_back_buffer)
{
    __DRIconfig **configs;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    mesa_format format;

    /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
     * support pageflipping at all.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML
    };

    uint8_t depth_bits_array[4];
    uint8_t stencil_bits_array[4];
    uint8_t msaa_samples_array[1];

    (void) psp;
    (void) have_back_buffer;

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

    msaa_samples_array[0] = 0;

    depth_buffer_factor = 4;
    back_buffer_factor = 2;

    switch (pixel_bits) {
    case 16:
	format = MESA_FORMAT_B5G6R5_UNORM;
	break;
    case 24:
        format = MESA_FORMAT_B8G8R8X8_UNORM;
	break;
    case 32:
	format = MESA_FORMAT_B8G8R8A8_UNORM;
	break;
    default:
	fprintf(stderr, "[%s:%u] bad depth %d\n", __func__, __LINE__,
		pixel_bits);
	return NULL;
    }

    configs = driCreateConfigs(format,
			       depth_bits_array, stencil_bits_array,
			       depth_buffer_factor, back_buffer_modes,
			       back_buffer_factor, msaa_samples_array, 1,
			       GL_TRUE);
    if (configs == NULL) {
	fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
		__LINE__);
	return NULL;
    }

    return configs;
}

static const __DRIconfig **
dri_init_screen(__DRIscreen * psp)
{
    __DRIconfig **configs16, **configs24, **configs32;

    TRACE;

    psp->max_gl_compat_version = 21;
    psp->max_gl_es1_version = 11;
    psp->max_gl_es2_version = 20;

    psp->extensions = dri_screen_extensions;

    configs16 = swrastFillInModes(psp, 16, 16, 0, 1);
    configs24 = swrastFillInModes(psp, 24, 24, 8, 1);
    configs32 = swrastFillInModes(psp, 32, 24, 8, 1);

    configs24 = driConcatConfigs(configs16, configs24);
    configs32 = driConcatConfigs(configs24, configs32);

    return (const __DRIconfig **)configs32;
}

static void
dri_destroy_screen(__DRIscreen * sPriv)
{
    TRACE;
    (void) sPriv;
}


/**
 * Framebuffer and renderbuffer-related functions.
 */

static GLuint
choose_pixel_format(const struct gl_config *v)
{
    int depth = v->rgbBits;

    if (depth == 32
	&& v->redMask   == 0xff0000
	&& v->greenMask == 0x00ff00
	&& v->blueMask  == 0x0000ff)
	return PF_A8R8G8B8;
    else if (depth == 24
	     && v->redMask   == 0xff0000
	     && v->greenMask == 0x00ff00
	     && v->blueMask  == 0x0000ff)
	return PF_X8R8G8B8;
    else if (depth == 16
	     && v->redMask   == 0xf800
	     && v->greenMask == 0x07e0
	     && v->blueMask  == 0x001f)
	return PF_R5G6B5;
    else if (depth == 8
	     && v->redMask   == 0x07
	     && v->greenMask == 0x38
	     && v->blueMask  == 0xc0)
	return PF_R3G3B2;

    _mesa_problem( NULL, "unexpected format in %s", __FUNCTION__ );
    return 0;
}

static void
swrast_delete_renderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
    struct dri_swrast_renderbuffer *xrb = dri_swrast_renderbuffer(rb);

    TRACE;

    free(xrb->Base.Buffer);
    _mesa_delete_renderbuffer(ctx, rb);
}

/* see bytes_per_line in libGL */
static INLINE int
bytes_per_line(unsigned pitch_bits, unsigned mul)
{
   unsigned mask = mul - 1;

   return ((pitch_bits + mask) & ~mask) / 8;
}

static GLboolean
swrast_alloc_front_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
			   GLenum internalFormat, GLuint width, GLuint height)
{
    struct dri_swrast_renderbuffer *xrb = dri_swrast_renderbuffer(rb);

    TRACE;

    (void) ctx;
    (void) internalFormat;

    xrb->Base.Buffer = NULL;
    rb->Width = width;
    rb->Height = height;
    xrb->pitch = bytes_per_line(width * xrb->bpp, 32);

    return GL_TRUE;
}

static GLboolean
swrast_alloc_back_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
			  GLenum internalFormat, GLuint width, GLuint height)
{
    struct dri_swrast_renderbuffer *xrb = dri_swrast_renderbuffer(rb);

    TRACE;

    free(xrb->Base.Buffer);

    swrast_alloc_front_storage(ctx, rb, internalFormat, width, height);

    xrb->Base.Buffer = malloc(height * xrb->pitch);

    return GL_TRUE;
}

static struct dri_swrast_renderbuffer *
swrast_new_renderbuffer(const struct gl_config *visual, __DRIdrawable *dPriv,
			GLboolean front)
{
    struct dri_swrast_renderbuffer *xrb = calloc(1, sizeof *xrb);
    struct gl_renderbuffer *rb;
    GLuint pixel_format;

    TRACE;

    if (!xrb)
	return NULL;

    rb = &xrb->Base.Base;

    _mesa_init_renderbuffer(rb, 0);

    pixel_format = choose_pixel_format(visual);

    xrb->dPriv = dPriv;
    xrb->Base.Base.Delete = swrast_delete_renderbuffer;
    if (front) {
        rb->AllocStorage = swrast_alloc_front_storage;
    }
    else {
	rb->AllocStorage = swrast_alloc_back_storage;
    }

    switch (pixel_format) {
    case PF_A8R8G8B8:
	rb->Format = MESA_FORMAT_B8G8R8A8_UNORM;
	rb->InternalFormat = GL_RGBA;
	rb->_BaseFormat = GL_RGBA;
	xrb->bpp = 32;
	break;
    case PF_X8R8G8B8:
	rb->Format = MESA_FORMAT_B8G8R8A8_UNORM; /* XXX */
	rb->InternalFormat = GL_RGB;
	rb->_BaseFormat = GL_RGB;
	xrb->bpp = 32;
	break;
    case PF_R5G6B5:
	rb->Format = MESA_FORMAT_B5G6R5_UNORM;
	rb->InternalFormat = GL_RGB;
	rb->_BaseFormat = GL_RGB;
	xrb->bpp = 16;
	break;
    case PF_R3G3B2:
	rb->Format = MESA_FORMAT_B2G3R3_UNORM;
	rb->InternalFormat = GL_RGB;
	rb->_BaseFormat = GL_RGB;
	xrb->bpp = 8;
	break;
    default:
	free(xrb);
	return NULL;
    }

    return xrb;
}

static void
swrast_map_renderbuffer(struct gl_context *ctx,
			struct gl_renderbuffer *rb,
			GLuint x, GLuint y, GLuint w, GLuint h,
			GLbitfield mode,
			GLubyte **out_map,
			GLint *out_stride)
{
   struct dri_swrast_renderbuffer *xrb = dri_swrast_renderbuffer(rb);
   GLubyte *map = xrb->Base.Buffer;
   int cpp = _mesa_get_format_bytes(rb->Format);
   int stride = rb->Width * cpp;

   if (rb->AllocStorage == swrast_alloc_front_storage) {
      __DRIdrawable *dPriv = xrb->dPriv;
      __DRIscreen *sPriv = dPriv->driScreenPriv;

      xrb->map_mode = mode;
      xrb->map_x = x;
      xrb->map_y = y;
      xrb->map_w = w;
      xrb->map_h = h;

      stride = w * cpp;
      xrb->Base.Buffer = malloc(h * stride);

      sPriv->swrast_loader->getImage(dPriv, x, rb->Height - y - h, w, h,
				     (char *) xrb->Base.Buffer,
				     dPriv->loaderPrivate);

      *out_map = xrb->Base.Buffer + (h - 1) * stride;
      *out_stride = -stride;
      return;
   }

   ASSERT(xrb->Base.Buffer);

   if (rb->AllocStorage == swrast_alloc_back_storage) {
      map += (rb->Height - 1) * stride;
      stride = -stride;
   }

   map += (GLsizei)y * stride;
   map += (GLsizei)x * cpp;

   *out_map = map;
   *out_stride = stride;
}

static void
swrast_unmap_renderbuffer(struct gl_context *ctx,
			  struct gl_renderbuffer *rb)
{
   struct dri_swrast_renderbuffer *xrb = dri_swrast_renderbuffer(rb);

   if (rb->AllocStorage == swrast_alloc_front_storage) {
      __DRIdrawable *dPriv = xrb->dPriv;
      __DRIscreen *sPriv = dPriv->driScreenPriv;

      if (xrb->map_mode & GL_MAP_WRITE_BIT) {
	 sPriv->swrast_loader->putImage(dPriv, __DRI_SWRAST_IMAGE_OP_DRAW,
					xrb->map_x, xrb->map_y,
					xrb->map_w, xrb->map_h,
					(char *) xrb->Base.Buffer,
					dPriv->loaderPrivate);
      }

      free(xrb->Base.Buffer);
      xrb->Base.Buffer = NULL;
   }
}

static GLboolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const struct gl_config * visual, GLboolean isPixmap)
{
    struct dri_drawable *drawable = NULL;
    struct gl_framebuffer *fb;
    struct dri_swrast_renderbuffer *frontrb, *backrb;

    TRACE;

    (void) sPriv;
    (void) isPixmap;

    drawable = CALLOC_STRUCT(dri_drawable);
    if (drawable == NULL)
	goto drawable_fail;

    dPriv->driverPrivate = drawable;
    drawable->dPriv = dPriv;

    drawable->row = malloc(SWRAST_MAX_WIDTH * 4);
    if (drawable->row == NULL)
	goto drawable_fail;

    fb = &drawable->Base;

    /* basic framebuffer setup */
    _mesa_initialize_window_framebuffer(fb, visual);

    /* add front renderbuffer */
    frontrb = swrast_new_renderbuffer(visual, dPriv, GL_TRUE);
    _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontrb->Base.Base);

    /* add back renderbuffer */
    if (visual->doubleBufferMode) {
	backrb = swrast_new_renderbuffer(visual, dPriv, GL_FALSE);
	_mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backrb->Base.Base);
    }

    /* add software renderbuffers */
    _swrast_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   visual->haveDepthBuffer,
                                   visual->haveStencilBuffer,
                                   visual->haveAccumBuffer,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux bufs */);

    return GL_TRUE;

drawable_fail:

    if (drawable)
	free(drawable->row);

    free(drawable);

    return GL_FALSE;
}

static void
dri_destroy_buffer(__DRIdrawable * dPriv)
{
    TRACE;

    if (dPriv) {
	struct dri_drawable *drawable = dri_drawable(dPriv);
	struct gl_framebuffer *fb;

	free(drawable->row);

	fb = &drawable->Base;

	fb->DeletePending = GL_TRUE;
	_mesa_reference_framebuffer(&fb, NULL);
    }
}

static void
dri_swap_buffers(__DRIdrawable * dPriv)
{
    __DRIscreen *sPriv = dPriv->driScreenPriv;

    GET_CURRENT_CONTEXT(ctx);

    struct dri_drawable *drawable = dri_drawable(dPriv);
    struct gl_framebuffer *fb;
    struct dri_swrast_renderbuffer *frontrb, *backrb;

    TRACE;

    fb = &drawable->Base;

    frontrb =
	dri_swrast_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
    backrb =
	dri_swrast_renderbuffer(fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);

    /* check for signle-buffered */
    if (backrb == NULL)
	return;

    /* check if swapping currently bound buffer */
    if (ctx && ctx->DrawBuffer == fb) {
	/* flush pending rendering */
	_mesa_notifySwapBuffers(ctx);
    }

    sPriv->swrast_loader->putImage(dPriv, __DRI_SWRAST_IMAGE_OP_SWAP,
				   0, 0,
				   frontrb->Base.Base.Width,
				   frontrb->Base.Base.Height,
				   (char *) backrb->Base.Buffer,
				   dPriv->loaderPrivate);
}


/**
 * General device driver functions.
 */

static void
get_window_size( struct gl_framebuffer *fb, GLsizei *w, GLsizei *h )
{
    __DRIdrawable *dPriv = swrast_drawable(fb)->dPriv;
    __DRIscreen *sPriv = dPriv->driScreenPriv;
    int x, y;

    sPriv->swrast_loader->getDrawableInfo(dPriv,
					  &x, &y, w, h,
					  dPriv->loaderPrivate);
}

static void
swrast_check_and_update_window_size( struct gl_context *ctx, struct gl_framebuffer *fb )
{
    GLsizei width, height;

    get_window_size(fb, &width, &height);
    if (fb->Width != width || fb->Height != height) {
	_mesa_resize_framebuffer(ctx, fb, width, height);
    }
}

static const GLubyte *
get_string(struct gl_context *ctx, GLenum pname)
{
    (void) ctx;
    switch (pname) {
	case GL_VENDOR:
	    return (const GLubyte *) swrast_vendor_string;
	case GL_RENDERER:
	    return (const GLubyte *) swrast_renderer_string;
	default:
	    return NULL;
    }
}

static void
update_state( struct gl_context *ctx, GLuint new_state )
{
    /* not much to do here - pass it on */
    _swrast_InvalidateState( ctx, new_state );
    _swsetup_InvalidateState( ctx, new_state );
    _vbo_InvalidateState( ctx, new_state );
    _tnl_InvalidateState( ctx, new_state );
}

static void
viewport(struct gl_context *ctx)
{
    struct gl_framebuffer *draw = ctx->WinSysDrawBuffer;
    struct gl_framebuffer *read = ctx->WinSysReadBuffer;

    swrast_check_and_update_window_size(ctx, draw);
    swrast_check_and_update_window_size(ctx, read);
}

static mesa_format swrastChooseTextureFormat(struct gl_context * ctx,
                                           GLenum target,
					   GLint internalFormat,
					   GLenum format,
					   GLenum type)
{
    if (internalFormat == GL_RGB)
	return MESA_FORMAT_B8G8R8X8_UNORM;
    return _mesa_choose_tex_format(ctx, target, internalFormat, format, type);
}

static void
swrast_init_driver_functions(struct dd_function_table *driver)
{
    driver->GetString = get_string;
    driver->UpdateState = update_state;
    driver->Viewport = viewport;
    driver->ChooseTextureFormat = swrastChooseTextureFormat;
    driver->MapRenderbuffer = swrast_map_renderbuffer;
    driver->UnmapRenderbuffer = swrast_unmap_renderbuffer;
}

/**
 * Context-related functions.
 */

static GLboolean
dri_create_context(gl_api api,
		   const struct gl_config * visual,
		   __DRIcontext * cPriv,
		   unsigned major_version,
		   unsigned minor_version,
		   uint32_t flags,
		   bool notify_reset,
		   unsigned *error,
		   void *sharedContextPrivate)
{
    struct dri_context *ctx = NULL;
    struct dri_context *share = (struct dri_context *)sharedContextPrivate;
    struct gl_context *mesaCtx = NULL;
    struct gl_context *sharedCtx = NULL;
    struct dd_function_table functions;

    TRACE;

    /* Flag filtering is handled in dri2CreateContextAttribs.
     */
    (void) flags;

    ctx = CALLOC_STRUCT(dri_context);
    if (ctx == NULL) {
	*error = __DRI_CTX_ERROR_NO_MEMORY;
	goto context_fail;
    }

    cPriv->driverPrivate = ctx;
    ctx->cPriv = cPriv;

    /* build table of device driver functions */
    _mesa_init_driver_functions(&functions);
    swrast_init_driver_functions(&functions);

    if (share) {
	sharedCtx = &share->Base;
    }

    mesaCtx = &ctx->Base;

    /* basic context setup */
    if (!_mesa_initialize_context(mesaCtx, api, visual, sharedCtx, &functions)) {
	*error = __DRI_CTX_ERROR_NO_MEMORY;
	goto context_fail;
    }

    driContextSetFlags(mesaCtx, flags);

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

    _mesa_meta_init(mesaCtx);
    _mesa_enable_sw_extensions(mesaCtx);

    _mesa_compute_version(mesaCtx);

    _mesa_initialize_dispatch_tables(mesaCtx);
    _mesa_initialize_vbo_vtxfmt(mesaCtx);

    *error = __DRI_CTX_ERROR_SUCCESS;
    return GL_TRUE;

context_fail:

    free(ctx);

    return GL_FALSE;
}

static void
dri_destroy_context(__DRIcontext * cPriv)
{
    TRACE;

    if (cPriv) {
	struct dri_context *ctx = dri_context(cPriv);
	struct gl_context *mesaCtx;

	mesaCtx = &ctx->Base;

        _mesa_meta_free(mesaCtx);
	_swsetup_DestroyContext( mesaCtx );
	_swrast_DestroyContext( mesaCtx );
	_tnl_DestroyContext( mesaCtx );
	_vbo_DestroyContext( mesaCtx );
	_mesa_destroy_context( mesaCtx );
    }
}

static GLboolean
dri_make_current(__DRIcontext * cPriv,
		 __DRIdrawable * driDrawPriv,
		 __DRIdrawable * driReadPriv)
{
    struct gl_context *mesaCtx;
    struct gl_framebuffer *mesaDraw;
    struct gl_framebuffer *mesaRead;
    TRACE;

    if (cPriv) {
	struct dri_context *ctx = dri_context(cPriv);
	struct dri_drawable *draw;
	struct dri_drawable *read;

	if (!driDrawPriv || !driReadPriv)
	    return GL_FALSE;

	draw = dri_drawable(driDrawPriv);
	read = dri_drawable(driReadPriv);
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
	if (mesaRead != mesaDraw)
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

static GLboolean
dri_unbind_context(__DRIcontext * cPriv)
{
    TRACE;
    (void) cPriv;

    /* Unset current context and dispath table */
    _mesa_make_current(NULL, NULL, NULL);

    return GL_TRUE;
}

static void
dri_copy_sub_buffer(__DRIdrawable *dPriv, int x, int y,
                    int w, int h)
{
    __DRIscreen *sPriv = dPriv->driScreenPriv;
    void *data;
    int iy;
    struct dri_drawable *drawable = dri_drawable(dPriv);
    struct gl_framebuffer *fb;
    struct dri_swrast_renderbuffer *frontrb, *backrb;

    TRACE;

    fb = &drawable->Base;

    frontrb =
	dri_swrast_renderbuffer(fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
    backrb =
	dri_swrast_renderbuffer(fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);

    /* check for signle-buffered */
    if (backrb == NULL)
       return;

    iy = frontrb->Base.Base.Height - y - h;
    data = (char *)backrb->Base.Buffer + (iy * backrb->pitch) + (x * ((backrb->bpp + 7) / 8));
    sPriv->swrast_loader->putImage2(dPriv, __DRI_SWRAST_IMAGE_OP_SWAP,
                                    x, iy, w, h,
                                    frontrb->pitch,
                                    data,
                                    dPriv->loaderPrivate);
}


static const struct __DriverAPIRec swrast_driver_api = {
    .InitScreen = dri_init_screen,
    .DestroyScreen = dri_destroy_screen,
    .CreateContext = dri_create_context,
    .DestroyContext = dri_destroy_context,
    .CreateBuffer = dri_create_buffer,
    .DestroyBuffer = dri_destroy_buffer,
    .SwapBuffers = dri_swap_buffers,
    .MakeCurrent = dri_make_current,
    .UnbindContext = dri_unbind_context,
    .CopySubBuffer = dri_copy_sub_buffer,
};

static const struct __DRIDriverVtableExtensionRec swrast_vtable = {
   .base = { __DRI_DRIVER_VTABLE, 1 },
   .vtable = &swrast_driver_api,
};

static const __DRIextension *swrast_driver_extensions[] = {
    &driCoreExtension.base,
    &driSWRastExtension.base,
    &driCopySubBufferExtension.base,
    &swrast_vtable.base,
    NULL
};

PUBLIC const __DRIextension **__driDriverGetExtensions_swrast(void)
{
   globalDriverAPI = &swrast_driver_api;

   return swrast_driver_extensions;
}
