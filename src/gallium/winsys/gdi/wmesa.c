/*
 * Windows (Win32/Win64) device driver for Mesa
 *
 */

#include "mtypes.h"
#include <GL/wmesa.h>
#include "wmesadef.h"

#undef Elements

#include "pipe/p_winsys.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"
#include "glapi/glapi.h"
#include "colors.h"

extern GLvisual *
_mesa_create_visual( GLboolean rgbFlag,
                     GLboolean dbFlag,
                     GLboolean stereoFlag,
                     GLint redBits,
                     GLint greenBits,
                     GLint blueBits,
                     GLint alphaBits,
                     GLint indexBits,
                     GLint depthBits,
                     GLint stencilBits,
                     GLint accumRedBits,
                     GLint accumGreenBits,
                     GLint accumBlueBits,
                     GLint accumAlphaBits,
                     GLint numSamples );

/* linked list of our Framebuffers (windows) */
WMesaFramebuffer FirstFramebuffer = NULL;

struct wmesa_pipe_winsys
{
   struct pipe_winsys base;
};

/**
 * Choose the pixel format for the given visual.
 * This will tell the gallium driver how to pack pixel data into
 * drawing surfaces.
 */
static GLuint
choose_pixel_format(GLvisual *v)
{
#if 1
   return PIPE_FORMAT_A8R8G8B8_UNORM;
#else
   if (   GET_REDMASK(v)   == 0x0000ff
       && GET_GREENMASK(v) == 0x00ff00
       && GET_BLUEMASK(v)  == 0xff0000
       && v->BitsPerPixel == 32) {
      if (CHECK_BYTE_ORDER(v)) {
         /* no byteswapping needed */
         return 0 /* PIXEL_FORMAT_U_A8_B8_G8_R8 */;
      }
      else {
         return PIPE_FORMAT_R8G8B8A8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0xff0000
            && GET_GREENMASK(v) == 0x00ff00
            && GET_BLUEMASK(v)  == 0x0000ff
            && v->BitsPerPixel == 32) {
      if (CHECK_BYTE_ORDER(v)) {
         /* no byteswapping needed */
         return PIPE_FORMAT_A8R8G8B8_UNORM;
      }
      else {
         return PIPE_FORMAT_B8G8R8A8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0xf800
            && GET_GREENMASK(v) == 0x07e0
            && GET_BLUEMASK(v)  == 0x001f
            && CHECK_BYTE_ORDER(v)
            && v->BitsPerPixel == 16) {
      /* 5-6-5 RGB */
      return PIPE_FORMAT_R5G6B5_UNORM;
   }

printf("BITS %d\n",v->BitsPerPixel);
   assert(0);
   return 0;
#endif
}

/*
 * Determine the pixel format based on the pixel size.
 */
static void wmSetPixelFormat(WMesaFramebuffer pwfb, HDC hDC)
{
    /* Only 16 and 32 bit targets are supported now */
    assert(pwfb->cColorBits == 0 ||
	   pwfb->cColorBits == 16 || 
	   pwfb->cColorBits == 32);

    switch(pwfb->cColorBits){
    case 8:
	pwfb->pixelformat = PF_INDEX8;
	break;
    case 16:
	pwfb->pixelformat = PF_5R6G5B;
	break;
    case 32:
	pwfb->pixelformat = PF_8R8G8B;
	break;
    default:
	pwfb->pixelformat = PF_BADFORMAT;
    }
}

/**
 * Create a new WMesaFramebuffer object which will correspond to the
 * given HDC (Window handle).
 */
WMesaFramebuffer
wmesa_new_framebuffer(HDC hdc, GLvisual *visual, GLuint width, GLuint height)
{
    WMesaFramebuffer pwfb
        = (WMesaFramebuffer) malloc(sizeof(struct wmesa_framebuffer));
    if (pwfb) {
   	enum pipe_format colorFormat, depthFormat, stencilFormat;

   	/* determine PIPE_FORMATs for buffers */
   	colorFormat = choose_pixel_format(visual);

   	if (visual->depthBits == 0)
      		depthFormat = PIPE_FORMAT_NONE;
	else if (visual->depthBits <= 16)
      		depthFormat = PIPE_FORMAT_Z16_UNORM;
	else if (visual->depthBits <= 24)
      		depthFormat = PIPE_FORMAT_S8Z24_UNORM;
	else
      		depthFormat = PIPE_FORMAT_Z32_UNORM;

	if (visual->stencilBits == 8) {
      		if (depthFormat == PIPE_FORMAT_S8Z24_UNORM)
			stencilFormat = depthFormat;
		else
			stencilFormat = PIPE_FORMAT_S8_UNORM;
   	}
   	else {
      		stencilFormat = PIPE_FORMAT_NONE;
   	}

   	pwfb->stfb = st_create_framebuffer(visual,
                                   colorFormat, depthFormat, stencilFormat,
                                   width, height,
                                   (void *) pwfb);

        pwfb->cColorBits = GetDeviceCaps(hdc, BITSPIXEL);

        pwfb->hDC = hdc;
        /* insert at head of list */
	pwfb->next = FirstFramebuffer;
        FirstFramebuffer = pwfb;
    }
    return pwfb;
}

/**
 * Given an hdc, free the corresponding WMesaFramebuffer
 */
void
wmesa_free_framebuffer(HDC hdc)
{
    WMesaFramebuffer pwfb, prev;
    for (pwfb = FirstFramebuffer; pwfb; pwfb = pwfb->next) {
        if (pwfb->hDC == hdc)
            break;
	prev = pwfb;
    }
    if (pwfb) {
	if (pwfb == FirstFramebuffer)
	    FirstFramebuffer = pwfb->next;
	else
	    prev->next = pwfb->next;
	free(pwfb);
    }
}

/**
 * Given an hdc, return the corresponding WMesaFramebuffer
 */
WMesaFramebuffer
wmesa_lookup_framebuffer(HDC hdc)
{
    WMesaFramebuffer pwfb;
    for (pwfb = FirstFramebuffer; pwfb; pwfb = pwfb->next) {
        if (pwfb->hDC == hdc)
            return pwfb;
    }
    return NULL;
}


/**
 * Given a GLframebuffer, return the corresponding WMesaFramebuffer.
 */
static WMesaFramebuffer wmesa_framebuffer(GLframebuffer *fb)
{
    return (WMesaFramebuffer) fb;
}


/**
 * Given a GLcontext, return the corresponding WMesaContext.
 */
static WMesaContext wmesa_context(const GLcontext *ctx)
{
    return (WMesaContext) ctx;
}

/**
 * Find the width and height of the window named by hdc.
 */
static void
get_window_size(HDC hdc, GLuint *width, GLuint *height)
{
    if (WindowFromDC(hdc)) {
        RECT rect;
        GetClientRect(WindowFromDC(hdc), &rect);
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
    else { /* Memory context */
        /* From contributed code - use the size of the desktop
         * for the size of a memory context (?) */
        *width = GetDeviceCaps(hdc, HORZRES);
        *height = GetDeviceCaps(hdc, VERTRES);
    }
}

/**
 * Low-level OS/window system memory buffer
 */
struct wm_buffer
{
   struct pipe_buffer base;
   boolean userBuffer;  /** Is this a user-space buffer? */
   void *data;
   void *mapped;
};

struct wmesa_surface
{
   struct pipe_surface surface;

   int no_swap;
};


/** Cast wrapper */
static INLINE struct wmesa_surface *
wmesa_surface(struct pipe_surface *ps)
{
//   assert(0);
   return (struct wmesa_surface *) ps;
}

/**
 * Turn the softpipe opaque buffer pointer into a dri_bufmgr opaque
 * buffer pointer...
 */
static INLINE struct wm_buffer *
wm_buffer( struct pipe_buffer *buf )
{
   return (struct wm_buffer *)buf;
}



/* Most callbacks map direcly onto dri_bufmgr operations:
 */
static void *
wm_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
              unsigned flags)
{
   struct wm_buffer *wm_buf = wm_buffer(buf);
   wm_buf->mapped = wm_buf->data;
   return wm_buf->mapped;
}

static void
wm_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
   struct wm_buffer *wm_buf = wm_buffer(buf);
   wm_buf->mapped = NULL;
}

static void
wm_buffer_destroy(struct pipe_winsys *pws,
                  struct pipe_buffer *buf)
{
   struct wm_buffer *oldBuf = wm_buffer(buf);

   if (oldBuf->data) {
      {
         if (!oldBuf->userBuffer) {
            align_free(oldBuf->data);
         }
      }

      oldBuf->data = NULL;
   }

   free(oldBuf);
}


static void
wm_flush_frontbuffer(struct pipe_winsys *pws,
                     struct pipe_surface *surf,
                     void *context_private)
{
    WMesaContext pwc = context_private;
    WMesaFramebuffer pwfb = wmesa_lookup_framebuffer(pwc->hDC);
   struct wm_buffer *wm_buf;
    BITMAPINFO bmi, *pbmi;

    wm_buf = wm_buffer(surf->buffer);

    pbmi = &bmi;
    memset(pbmi, 0, sizeof(BITMAPINFO));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = pwfb->stfb->Base.Width;
    pbmi->bmiHeader.biHeight= -((long)pwfb->stfb->Base.Height);
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = pwfb->cColorBits;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    StretchDIBits(pwfb->hDC, 0, 0, pwfb->stfb->Base.Width, pwfb->stfb->Base.Height, 0, 0, pwfb->stfb->Base.Width, pwfb->stfb->Base.Height, wm_buf->data, pbmi, 0, SRCCOPY);
}



static const char *
wm_get_name(struct pipe_winsys *pws)
{
   return "gdi";
}

static struct pipe_buffer *
wm_buffer_create(struct pipe_winsys *pws, 
                 unsigned alignment, 
                 unsigned usage,
                 unsigned size)
{
   struct wm_buffer *buffer = CALLOC_STRUCT(wm_buffer);

   buffer->base.refcount = 1;
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;

   if (buffer->data == NULL) {
      /* align to 16-byte multiple for Cell */
      buffer->data = align_malloc(size, max(alignment, 16));
   }

   return &buffer->base;
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
wm_user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct wm_buffer *buffer = CALLOC_STRUCT(wm_buffer);
   buffer->base.refcount = 1;
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}



/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}

static int
wm_surface_alloc_storage(struct pipe_winsys *winsys,
                         struct pipe_surface *surf,
                         unsigned width, unsigned height,
                         enum pipe_format format, 
                         unsigned flags,
                         unsigned tex_usage)
{
   const unsigned alignment = 64;

   surf->width = width;
   surf->height = height;
   surf->format = format;
   pf_get_block(format, &surf->block);
   surf->nblocksx = pf_get_nblocksx(&surf->block, width);
   surf->nblocksy = pf_get_nblocksy(&surf->block, height);
   surf->stride = round_up(surf->nblocksx * surf->block.size, alignment);

   assert(!surf->buffer);
   surf->buffer = winsys->buffer_create(winsys, alignment,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        surf->nblocksy * surf->stride);
   if(!surf->buffer)
      return -1;
   
   return 0;
}


/**
 * Called via winsys->surface_alloc() to create new surfaces.
 */
static struct pipe_surface *
wm_surface_alloc(struct pipe_winsys *ws)
{
   struct wmesa_surface *wms = CALLOC_STRUCT(wmesa_surface);
   static boolean no_swap = 0;
   static boolean firsttime = 1;

   if (firsttime) {
      no_swap = getenv("SP_NO_RAST") != NULL;
      firsttime = 0;
   }

   assert(ws);

   wms->surface.refcount = 1;
   wms->surface.winsys = ws;

   wms->no_swap = no_swap;
   
   return &wms->surface;
}

static void
wm_surface_release(struct pipe_winsys *winsys, struct pipe_surface **s)
{
   struct pipe_surface *surf = *s;
   surf->refcount--;
   if (surf->refcount == 0) {
      if (surf->buffer)
	winsys_buffer_reference(winsys, &surf->buffer, NULL);
      free(surf);
   }
   *s = NULL;
}


/*
 * Fence functions - basically nothing to do, as we don't create any actual
 * fence objects.
 */

static void
wm_fence_reference(struct pipe_winsys *sws, struct pipe_fence_handle **ptr,
                   struct pipe_fence_handle *fence)
{
}


static int
wm_fence_signalled(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
                   unsigned flag)
{
   return 0;
}


static int
wm_fence_finish(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
                unsigned flag)
{
   return 0;
}



struct pipe_winsys *
wmesa_get_pipe_winsys(GLvisual *visual)
{
   static struct wmesa_pipe_winsys *ws = NULL;

   if (!ws) {
      ws = CALLOC_STRUCT(wmesa_pipe_winsys);

      /* Fill in this struct with callbacks that pipe will need to
       * communicate with the window system, buffer manager, etc. 
       */
      ws->base.buffer_create = wm_buffer_create;
      ws->base.user_buffer_create = wm_user_buffer_create;
      ws->base.buffer_map = wm_buffer_map;
      ws->base.buffer_unmap = wm_buffer_unmap;
      ws->base.buffer_destroy = wm_buffer_destroy;

      ws->base.surface_alloc = wm_surface_alloc;
      ws->base.surface_alloc_storage = wm_surface_alloc_storage;
      ws->base.surface_release = wm_surface_release;

      ws->base.fence_reference = wm_fence_reference;
      ws->base.fence_signalled = wm_fence_signalled;
      ws->base.fence_finish = wm_fence_finish;

      ws->base.flush_frontbuffer = wm_flush_frontbuffer;
      ws->base.get_name = wm_get_name;
   }

   return &ws->base;
}



/**********************************************************************/
/*****                   WMESA Functions                          *****/
/**********************************************************************/

WMesaContext WMesaCreateContext(HDC hDC, 
				HPALETTE* Pal,
				GLboolean rgb_flag,
				GLboolean db_flag,
				GLboolean alpha_flag)
{
   WMesaContext c;
   struct pipe_winsys *pws;
   struct pipe_context *pipe;
   struct pipe_screen *screen;
   GLint red_bits, green_bits, blue_bits, alpha_bits;
   GLvisual *visual;

   (void) Pal;
    
   /* Indexed mode not supported */
   if (!rgb_flag)
	return NULL;

   /* Allocate wmesa context */
   c = CALLOC_STRUCT(wmesa_context);
   if (!c)
	return NULL;

   c->hDC = hDC;

    /* Get data for visual */
    /* Dealing with this is actually a bit of overkill because Mesa will end
     * up treating all color component size requests less than 8 by using 
     * a single byte per channel.  In addition, the interface to the span
     * routines passes colors as an entire byte per channel anyway, so there
     * is nothing to be saved by telling the visual to be 16 bits if the device
     * is 16 bits.  That is, Mesa is going to compute colors down to 8 bits per
     * channel anyway.
     * But we go through the motions here anyway.
     */
    c->cColorBits = GetDeviceCaps(c->hDC, BITSPIXEL);

    switch (c->cColorBits) {
    case 16:
	red_bits = green_bits = blue_bits = 5;
	alpha_bits = 0;
	break;
    default:
	red_bits = green_bits = blue_bits = 8;
	alpha_bits = 8;
	break;
    }
    /* Create visual based on flags */
    visual = _mesa_create_visual(rgb_flag,
                                 db_flag,    /* db_flag */
                                 GL_FALSE,   /* stereo */
                                 red_bits, green_bits, blue_bits, /* color RGB */
                                 alpha_flag ? alpha_bits : 0, /* color A */
                                 0,          /* index bits */
                                 DEFAULT_SOFTWARE_DEPTH_BITS, /* depth_bits */
                                 8,          /* stencil_bits */
                                 16,16,16,   /* accum RGB */
                                 alpha_flag ? 16 : 0, /* accum A */
                                 1);         /* num samples */
    
    if (!visual) {
	_mesa_free(c);
	return NULL;
    }

    pws = wmesa_get_pipe_winsys(visual);

    screen = softpipe_create_screen(pws);

    if (!screen) {
        _mesa_free(c);
	return NULL;
   }

   pipe = softpipe_create(screen, pws, NULL);

   if (!pipe) {
      /* FIXME - free screen */
      _mesa_free(c);
      return NULL;
   }

   pipe->priv = c;

   c->st = st_create_context(pipe, visual, NULL);

   c->st->ctx->DriverCtx = c;

   return c;
}


void WMesaDestroyContext( WMesaContext pwc )
{
    GLcontext *ctx = pwc->st->ctx;
    WMesaFramebuffer pwfb;
    GET_CURRENT_CONTEXT(cur_ctx);

    if (cur_ctx == ctx) {
        /* unbind current if deleting current context */
        WMesaMakeCurrent(NULL, NULL);
    }

    /* clean up frame buffer resources */
    pwfb = wmesa_lookup_framebuffer(pwc->hDC);
    if (pwfb) {
	wmesa_free_framebuffer(pwc->hDC);
    }

    /* Release for device, not memory contexts */
    if (WindowFromDC(pwc->hDC) != NULL)
    {
      ReleaseDC(WindowFromDC(pwc->hDC), pwc->hDC);
    }
    
    st_destroy_context(pwc->st);
    _mesa_free(pwc);
}


void WMesaMakeCurrent(WMesaContext c, HDC hdc)
{
    GLuint width = 0, height = 0;
    WMesaFramebuffer pwfb;

    {
        /* return if already current */
        GET_CURRENT_CONTEXT(ctx);
        WMesaContext pwc = wmesa_context(ctx);
        if (pwc && c == pwc && pwc->hDC == hdc)
            return;
    }

    pwfb = wmesa_lookup_framebuffer(hdc);

    if (hdc) {
        get_window_size(hdc, &width, &height);
    }

    /* Lazy creation of framebuffers */
    if (c && !pwfb && (hdc != 0)) {
        GLvisual *visual = &c->st->ctx->Visual;

        pwfb = wmesa_new_framebuffer(hdc, visual, width, height);
    }

    if (c && pwfb) {
      	st_make_current(c->st, pwfb->stfb, pwfb->stfb);

	st_resize_framebuffer(pwfb->stfb, width, height);
   }
   else {
      /* Detach */
      st_make_current( NULL, NULL, NULL );
   }
}


void WMesaSwapBuffers( HDC hdc )
{
   struct pipe_surface *surf;
   struct wm_buffer *wm_buf;
    WMesaFramebuffer pwfb = wmesa_lookup_framebuffer(hdc);
    BITMAPINFO bmi, *pbmi;

    if (!pwfb) {
        _mesa_problem(NULL, "wmesa: swapbuffers on unknown hdc");
        return;
    }


    /* If we're swapping the buffer associated with the current context
     * we have to flush any pending rendering commands first.
     */
    st_notify_swapbuffers(pwfb->stfb);

    surf = st_get_framebuffer_surface(pwfb->stfb, ST_SURFACE_BACK_LEFT);
    wm_buf = wm_buffer(surf->buffer);

    pbmi = &bmi;
    memset(pbmi, 0, sizeof(BITMAPINFO));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = pwfb->stfb->Base.Width;
    pbmi->bmiHeader.biHeight= -((long)pwfb->stfb->Base.Height);
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = pwfb->cColorBits;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    StretchDIBits(pwfb->hDC, 0, 0, pwfb->stfb->Base.Width, pwfb->stfb->Base.Height, 0, 0, pwfb->stfb->Base.Width, pwfb->stfb->Base.Height, wm_buf->data, pbmi, 0, SRCCOPY);

    {
        GLuint width = 0, height = 0;

        get_window_size(pwfb->hDC, &width, &height);

	st_resize_framebuffer(pwfb->stfb, width, height);
    }
}

/* This is hopefully a temporary hack to define some needed dispatch
 * table entries.  Hopefully, I'll find a better solution.  The
 * dispatch table generation scripts ought to be making these dummy
 * stubs as well. */
#if !defined(__MINGW32__) || !defined(GL_NO_STDCALL)
void gl_dispatch_stub_543(void){}
void gl_dispatch_stub_544(void){}
void gl_dispatch_stub_545(void){}
void gl_dispatch_stub_546(void){}
void gl_dispatch_stub_547(void){}
void gl_dispatch_stub_548(void){}
void gl_dispatch_stub_549(void){}
void gl_dispatch_stub_550(void){}
void gl_dispatch_stub_551(void){}
void gl_dispatch_stub_552(void){}
void gl_dispatch_stub_553(void){}
void gl_dispatch_stub_554(void){}
void gl_dispatch_stub_555(void){}
void gl_dispatch_stub_556(void){}
void gl_dispatch_stub_557(void){}
void gl_dispatch_stub_558(void){}
void gl_dispatch_stub_559(void){}
void gl_dispatch_stub_560(void){}
void gl_dispatch_stub_561(void){}
void gl_dispatch_stub_565(void){}
void gl_dispatch_stub_566(void){}
void gl_dispatch_stub_577(void){}
void gl_dispatch_stub_578(void){}
void gl_dispatch_stub_603(void){}
void gl_dispatch_stub_645(void){}
void gl_dispatch_stub_646(void){}
void gl_dispatch_stub_647(void){}
void gl_dispatch_stub_648(void){}
void gl_dispatch_stub_649(void){}
void gl_dispatch_stub_650(void){}
void gl_dispatch_stub_651(void){}
void gl_dispatch_stub_652(void){}
void gl_dispatch_stub_653(void){}
void gl_dispatch_stub_733(void){}
void gl_dispatch_stub_734(void){}
void gl_dispatch_stub_735(void){}
void gl_dispatch_stub_736(void){}
void gl_dispatch_stub_737(void){}
void gl_dispatch_stub_738(void){}
void gl_dispatch_stub_744(void){}
void gl_dispatch_stub_745(void){}
void gl_dispatch_stub_746(void){}
void gl_dispatch_stub_760(void){}
void gl_dispatch_stub_761(void){}
void gl_dispatch_stub_763(void){}
void gl_dispatch_stub_765(void){}
void gl_dispatch_stub_766(void){}
void gl_dispatch_stub_767(void){}
void gl_dispatch_stub_768(void){}

void gl_dispatch_stub_562(void){}
void gl_dispatch_stub_563(void){}
void gl_dispatch_stub_564(void){}
void gl_dispatch_stub_567(void){}
void gl_dispatch_stub_568(void){}
void gl_dispatch_stub_569(void){}
void gl_dispatch_stub_580(void){}
void gl_dispatch_stub_581(void){}
void gl_dispatch_stub_606(void){}
void gl_dispatch_stub_654(void){}
void gl_dispatch_stub_655(void){}
void gl_dispatch_stub_656(void){}
void gl_dispatch_stub_739(void){}
void gl_dispatch_stub_740(void){}
void gl_dispatch_stub_741(void){}
void gl_dispatch_stub_748(void){}
void gl_dispatch_stub_749(void){}
void gl_dispatch_stub_769(void){}
void gl_dispatch_stub_770(void){}
void gl_dispatch_stub_771(void){}
void gl_dispatch_stub_772(void){}
void gl_dispatch_stub_773(void){}

#endif
