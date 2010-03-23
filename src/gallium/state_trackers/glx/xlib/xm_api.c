/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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

/**
 * \file xm_api.c
 *
 * All the XMesa* API functions.
 *
 *
 * NOTES:
 *
 * The window coordinate system origin (0,0) is in the lower-left corner
 * of the window.  X11's window coordinate origin is in the upper-left
 * corner of the window.  Therefore, most drawing functions in this
 * file have to flip Y coordinates.
 *
 * Define USE_XSHM in the Makefile with -DUSE_XSHM if you want to compile
 * in support for the MIT Shared Memory extension.  If enabled, when you
 * use an Ximage for the back buffer in double buffered mode, the "swap"
 * operation will be faster.  You must also link with -lXext.
 *
 * Byte swapping:  If the Mesa host and the X display use a different
 * byte order then there's some trickiness to be aware of when using
 * XImages.  The byte ordering used for the XImage is that of the X
 * display, not the Mesa host.
 * The color-to-pixel encoding for True/DirectColor must be done
 * according to the display's visual red_mask, green_mask, and blue_mask.
 * If XPutPixel is used to put a pixel into an XImage then XPutPixel will
 * do byte swapping if needed.  If one wants to directly "poke" the pixel
 * into the XImage's buffer then the pixel must be byte swapped first.
 *
 */

#ifdef __CYGWIN__
#undef WIN32
#undef __WIN32__
#endif

#include "xm_api.h"
#include "main/context.h"
#include "main/framebuffer.h"

#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

#include "trace/tr_screen.h"
#include "trace/tr_context.h"
#include "trace/tr_texture.h"

#include "xm_winsys.h"
#include <GL/glx.h>


/* Driver interface routines, set up by xlib backend on library
 * _init().  These are global in the same way that function names are
 * global.
 */
static struct xm_driver driver;

void xmesa_set_driver( const struct xm_driver *templ )
{
   driver = *templ;
}

/**
 * Global X driver lock
 */
pipe_mutex _xmesa_lock;

static struct pipe_screen *_screen = NULL;
static struct pipe_screen *screen = NULL;


/**********************************************************************/
/*****                     X Utility Functions                    *****/
/**********************************************************************/


/**
 * Return the host's byte order as LSBFirst or MSBFirst ala X.
 */
static int host_byte_order( void )
{
   int i = 1;
   char *cptr = (char *) &i;
   return (*cptr==1) ? LSBFirst : MSBFirst;
}


/**
 * Check if the X Shared Memory extension is available.
 * Return:  0 = not available
 *          1 = shared XImage support available
 *          2 = shared Pixmap support available also
 */
int xmesa_check_for_xshm( Display *display )
{
#if defined(USE_XSHM)
   int major, minor, ignore;
   Bool pixmaps;

   if (getenv("SP_NO_RAST")) 
      return 0;

   if (getenv("MESA_NOSHM")) {
      return 0;
   }

   if (XQueryExtension( display, "MIT-SHM", &ignore, &ignore, &ignore )) {
      if (XShmQueryVersion( display, &major, &minor, &pixmaps )==True) {
	 return (pixmaps==True) ? 2 : 1;
      }
      else {
	 return 0;
      }
   }
   else {
      return 0;
   }
#else
   /* No  XSHM support */
   return 0;
#endif
}


/**
 * Return the true number of bits per pixel for XImages.
 * For example, if we request a 24-bit deep visual we may actually need/get
 * 32bpp XImages.  This function returns the appropriate bpp.
 * Input:  dpy - the X display
 *         visinfo - desribes the visual to be used for XImages
 * Return:  true number of bits per pixel for XImages
 */
static int
bits_per_pixel( XMesaVisual xmv )
{
   Display *dpy = xmv->display;
   XVisualInfo * visinfo = xmv->visinfo;
   XImage *img;
   int bitsPerPixel;
   /* Create a temporary XImage */
   img = XCreateImage( dpy, visinfo->visual, visinfo->depth,
		       ZPixmap, 0,           /*format, offset*/
		       (char*) MALLOC(8),    /*data*/
		       1, 1,                 /*width, height*/
		       32,                   /*bitmap_pad*/
		       0                     /*bytes_per_line*/
                     );
   assert(img);
   /* grab the bits/pixel value */
   bitsPerPixel = img->bits_per_pixel;
   /* free the XImage */
   free( img->data );
   img->data = NULL;
   XDestroyImage( img );
   return bitsPerPixel;
}



/*
 * Determine if a given X window ID is valid (window exists).
 * Do this by calling XGetWindowAttributes() for the window and
 * checking if we catch an X error.
 * Input:  dpy - the display
 *         win - the window to check for existance
 * Return:  GL_TRUE - window exists
 *          GL_FALSE - window doesn't exist
 */
static GLboolean WindowExistsFlag;

static int window_exists_err_handler( Display* dpy, XErrorEvent* xerr )
{
   (void) dpy;
   if (xerr->error_code == BadWindow) {
      WindowExistsFlag = GL_FALSE;
   }
   return 0;
}

static GLboolean window_exists( Display *dpy, Window win )
{
   XWindowAttributes wa;
   int (*old_handler)( Display*, XErrorEvent* );
   WindowExistsFlag = GL_TRUE;
   old_handler = XSetErrorHandler(window_exists_err_handler);
   XGetWindowAttributes( dpy, win, &wa ); /* dummy request */
   XSetErrorHandler(old_handler);
   return WindowExistsFlag;
}

static Status
get_drawable_size( Display *dpy, Drawable d, uint *width, uint *height )
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}


/**
 * Return the size of the window (or pixmap) that corresponds to the
 * given XMesaBuffer.
 * \param width  returns width in pixels
 * \param height  returns height in pixels
 */
void
xmesa_get_window_size(Display *dpy, XMesaBuffer b,
                      GLuint *width, GLuint *height)
{
   Status stat;

   pipe_mutex_lock(_xmesa_lock);
   XSync(b->xm_visual->display, 0); /* added for Chromium */
   stat = get_drawable_size(dpy, b->drawable, width, height);
   pipe_mutex_unlock(_xmesa_lock);

   if (!stat) {
      /* probably querying a window that's recently been destroyed */
      _mesa_warning(NULL, "XGetGeometry failed!\n");
      *width = *height = 1;
   }
}

#define GET_REDMASK(__v)        __v->mesa_visual.redMask
#define GET_GREENMASK(__v)      __v->mesa_visual.greenMask
#define GET_BLUEMASK(__v)       __v->mesa_visual.blueMask


/**
 * Choose the pixel format for the given visual.
 * This will tell the gallium driver how to pack pixel data into
 * drawing surfaces.
 */
static GLuint
choose_pixel_format(XMesaVisual v)
{
   boolean native_byte_order = (host_byte_order() == 
                                ImageByteOrder(v->display));

   if (   GET_REDMASK(v)   == 0x0000ff
       && GET_GREENMASK(v) == 0x00ff00
       && GET_BLUEMASK(v)  == 0xff0000
       && v->BitsPerPixel == 32) {
      if (native_byte_order) {
         /* no byteswapping needed */
         return PIPE_FORMAT_R8G8B8A8_UNORM;
      }
      else {
         return PIPE_FORMAT_A8B8G8R8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0xff0000
            && GET_GREENMASK(v) == 0x00ff00
            && GET_BLUEMASK(v)  == 0x0000ff
            && v->BitsPerPixel == 32) {
      if (native_byte_order) {
         /* no byteswapping needed */
         return PIPE_FORMAT_B8G8R8A8_UNORM;
      }
      else {
         return PIPE_FORMAT_A8R8G8B8_UNORM;
      }
   }
   else if (   GET_REDMASK(v)   == 0x0000ff00
            && GET_GREENMASK(v) == 0x00ff0000
            && GET_BLUEMASK(v)  == 0xff000000
            && v->BitsPerPixel == 32) {
      if (native_byte_order) {
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
            && native_byte_order
            && v->BitsPerPixel == 16) {
      /* 5-6-5 RGB */
      return PIPE_FORMAT_B5G6R5_UNORM;
   }

   assert(0);
   return 0;
}



/**
 * Query the default gallium screen for a Z/Stencil format that
 * at least matches the given depthBits and stencilBits.
 */
static void
xmesa_choose_z_stencil_format(int depth, int stencil,
                              enum pipe_format *depthFormat,
                              enum pipe_format *stencilFormat)
{
   const enum pipe_texture_target target = PIPE_TEXTURE_2D;
   const unsigned tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
   const unsigned geom_flags = (PIPE_TEXTURE_GEOM_NON_SQUARE |
                                PIPE_TEXTURE_GEOM_NON_POWER_OF_TWO);
   enum pipe_format formats[8];
   int count, i;

   count = 0;

   if (depth <= 16 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_Z16_UNORM;
   }
   if (depth <= 24 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_X8Z24_UNORM;
      formats[count++] = PIPE_FORMAT_Z24X8_UNORM;
   }
   if (depth <= 24 && stencil <= 8) {
      formats[count++] = PIPE_FORMAT_S8Z24_UNORM;
      formats[count++] = PIPE_FORMAT_Z24S8_UNORM;
   }
   if (depth <= 32 && stencil == 0) {
      formats[count++] = PIPE_FORMAT_Z32_UNORM;
   }

   *depthFormat = PIPE_FORMAT_NONE;
   for (i = 0; i < count; i++) {
      if (screen->is_format_supported(screen, formats[i],
                                      target, tex_usage, geom_flags)) {
         *depthFormat = formats[i];
         break;
      }
   }

   if (stencil) {
      *stencilFormat = *depthFormat;
   }
   else {
      *stencilFormat = PIPE_FORMAT_NONE;
   }
}



/**********************************************************************/
/*****                Linked list of XMesaBuffers                 *****/
/**********************************************************************/

XMesaBuffer XMesaBufferList = NULL;


/**
 * Allocate a new XMesaBuffer object which corresponds to the given drawable.
 * Note that XMesaBuffer is derived from GLframebuffer.
 * The new XMesaBuffer will not have any size (Width=Height=0).
 *
 * \param d  the corresponding X drawable (window or pixmap)
 * \param type  either WINDOW, PIXMAP or PBUFFER, describing d
 * \param vis  the buffer's visual
 * \param cmap  the window's colormap, if known.
 * \return new XMesaBuffer or NULL if any problem
 */
static XMesaBuffer
create_xmesa_buffer(Drawable d, BufferType type,
                    XMesaVisual vis, Colormap cmap)
{
   XMesaBuffer b;
   GLframebuffer *fb;
   enum pipe_format colorFormat, depthFormat, stencilFormat;
   uint width, height;

   ASSERT(type == WINDOW || type == PIXMAP || type == PBUFFER);

   b = (XMesaBuffer) CALLOC_STRUCT(xmesa_buffer);
   if (!b)
      return NULL;

   b->drawable = d;

   b->xm_visual = vis;
   b->type = type;
   b->cmap = cmap;

   /* determine PIPE_FORMATs for buffers */
   colorFormat = choose_pixel_format(vis);

   xmesa_choose_z_stencil_format(vis->mesa_visual.depthBits,
                                 vis->mesa_visual.stencilBits,
                                 &depthFormat, &stencilFormat);


   get_drawable_size(vis->display, d, &width, &height);

   /*
    * Create framebuffer, but we'll plug in our own renderbuffers below.
    */
   b->stfb = st_create_framebuffer(&vis->mesa_visual,
                                   colorFormat, depthFormat, stencilFormat,
                                   width, height,
                                   (void *) b);
   fb = &b->stfb->Base;

   /*
    * Create scratch XImage for xmesa_display_surface()
    */
   b->tempImage = XCreateImage(vis->display,
                               vis->visinfo->visual,
                               vis->visinfo->depth,
                               ZPixmap, 0,   /* format, offset */
                               NULL,         /* data */
                               0, 0,         /* size */
                               32,           /* bitmap_pad */
                               0);           /* bytes_per_line */

   /* GLX_EXT_texture_from_pixmap */
   b->TextureTarget = 0;
   b->TextureFormat = GLX_TEXTURE_FORMAT_NONE_EXT;
   b->TextureMipmap = 0;

   /* insert buffer into linked list */
   b->Next = XMesaBufferList;
   XMesaBufferList = b;

   return b;
}


/**
 * Find an XMesaBuffer by matching X display and colormap but NOT matching
 * the notThis buffer.
 */
XMesaBuffer
xmesa_find_buffer(Display *dpy, Colormap cmap, XMesaBuffer notThis)
{
   XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->xm_visual->display == dpy &&
          b->cmap == cmap &&
          b != notThis) {
         return b;
      }
   }
   return NULL;
}


/**
 * Remove buffer from linked list, delete if no longer referenced.
 */
static void
xmesa_free_buffer(XMesaBuffer buffer)
{
   XMesaBuffer prev = NULL, b;

   for (b = XMesaBufferList; b; b = b->Next) {
      if (b == buffer) {
         struct gl_framebuffer *fb = &buffer->stfb->Base;

         /* unlink buffer from list */
         if (prev)
            prev->Next = buffer->Next;
         else
            XMesaBufferList = buffer->Next;

         /* mark as delete pending */
         fb->DeletePending = GL_TRUE;

         /* Since the X window for the XMesaBuffer is going away, we don't
          * want to dereference this pointer in the future.
          */
         b->drawable = 0;

         buffer->tempImage->data = NULL;
         XDestroyImage(buffer->tempImage);

         /* Unreference.  If count = zero we'll really delete the buffer */
         _mesa_reference_framebuffer(&fb, NULL);

         XFreeGC(b->xm_visual->display, b->gc);

         free(buffer);

         return;
      }
      /* continue search */
      prev = b;
   }
   /* buffer not found in XMesaBufferList */
   _mesa_problem(NULL,"xmesa_free_buffer() - buffer not found\n");
}



/**********************************************************************/
/*****                   Misc Private Functions                   *****/
/**********************************************************************/


/**
 * When a context is bound for the first time, we can finally finish
 * initializing the context's visual and buffer information.
 * \param v  the XMesaVisual to initialize
 * \param b  the XMesaBuffer to initialize (may be NULL)
 * \param rgb_flag  TRUE = RGBA mode, FALSE = color index mode
 * \param window  the window/pixmap we're rendering into
 * \param cmap  the colormap associated with the window/pixmap
 * \return GL_TRUE=success, GL_FALSE=failure
 */
static GLboolean
initialize_visual_and_buffer(XMesaVisual v, XMesaBuffer b,
                             GLboolean rgb_flag, Drawable window,
                             Colormap cmap)
{
   ASSERT(!b || b->xm_visual == v);

   /* Save true bits/pixel */
   v->BitsPerPixel = bits_per_pixel(v);
   assert(v->BitsPerPixel > 0);

   if (rgb_flag == GL_FALSE) {
      /* COLOR-INDEXED WINDOW: not supported*/
      return GL_FALSE;
   }
   else {
      /* RGB WINDOW:
       * We support RGB rendering into almost any kind of visual.
       */
      const int xclass = v->mesa_visual.visualType;
      if (xclass != GLX_TRUE_COLOR && xclass == !GLX_DIRECT_COLOR) {
	 _mesa_warning(NULL,
            "XMesa: RGB mode rendering not supported in given visual.\n");
	 return GL_FALSE;
      }
      v->mesa_visual.indexBits = 0;

      if (v->BitsPerPixel == 32) {
         /* We use XImages for all front/back buffers.  If an X Window or
          * X Pixmap is 32bpp, there's no guarantee that the alpha channel
          * will be preserved.  For XImages we're in luck.
          */
         v->mesa_visual.alphaBits = 8;
      }
   }

   /*
    * If MESA_INFO env var is set print out some debugging info
    * which can help Brian figure out what's going on when a user
    * reports bugs.
    */
   if (_mesa_getenv("MESA_INFO")) {
      printf("X/Mesa visual = %p\n", (void *) v);
      printf("X/Mesa level = %d\n", v->mesa_visual.level);
      printf("X/Mesa depth = %d\n", v->visinfo->depth);
      printf("X/Mesa bits per pixel = %d\n", v->BitsPerPixel);
   }

   if (b && window) {
      /* these should have been set in create_xmesa_buffer */
      ASSERT(b->drawable == window);

      /* Setup for single/double buffering */
      if (v->mesa_visual.doubleBufferMode) {
         /* Double buffered */
         b->shm = xmesa_check_for_xshm( v->display );
      }

      /* X11 graphics context */
      b->gc = XCreateGC( v->display, window, 0, NULL );
      XSetFunction( v->display, b->gc, GXcopy );
   }

   return GL_TRUE;
}



#define NUM_VISUAL_TYPES   6

/**
 * Convert an X visual type to a GLX visual type.
 * 
 * \param visualType X visual type (i.e., \c TrueColor, \c StaticGray, etc.)
 *        to be converted.
 * \return If \c visualType is a valid X visual type, a GLX visual type will
 *         be returned.  Otherwise \c GLX_NONE will be returned.
 * 
 * \note
 * This code was lifted directly from lib/GL/glx/glcontextmodes.c in the
 * DRI CVS tree.
 */
static GLint
xmesa_convert_from_x_visual_type( int visualType )
{
    static const int glx_visual_types[ NUM_VISUAL_TYPES ] = {
	GLX_STATIC_GRAY,  GLX_GRAY_SCALE,
	GLX_STATIC_COLOR, GLX_PSEUDO_COLOR,
	GLX_TRUE_COLOR,   GLX_DIRECT_COLOR
    };

    return ( (unsigned) visualType < NUM_VISUAL_TYPES )
	? glx_visual_types[ visualType ] : GLX_NONE;
}


/**********************************************************************/
/*****                       Public Functions                     *****/
/**********************************************************************/


/*
 * Create a new X/Mesa visual.
 * Input:  display - X11 display
 *         visinfo - an XVisualInfo pointer
 *         rgb_flag - GL_TRUE = RGB mode,
 *                    GL_FALSE = color index mode
 *         alpha_flag - alpha buffer requested?
 *         db_flag - GL_TRUE = double-buffered,
 *                   GL_FALSE = single buffered
 *         stereo_flag - stereo visual?
 *         ximage_flag - GL_TRUE = use an XImage for back buffer,
 *                       GL_FALSE = use an off-screen pixmap for back buffer
 *         depth_size - requested bits/depth values, or zero
 *         stencil_size - requested bits/stencil values, or zero
 *         accum_red_size - requested bits/red accum values, or zero
 *         accum_green_size - requested bits/green accum values, or zero
 *         accum_blue_size - requested bits/blue accum values, or zero
 *         accum_alpha_size - requested bits/alpha accum values, or zero
 *         num_samples - number of samples/pixel if multisampling, or zero
 *         level - visual level, usually 0
 *         visualCaveat - ala the GLX extension, usually GLX_NONE
 * Return;  a new XMesaVisual or 0 if error.
 */
PUBLIC
XMesaVisual XMesaCreateVisual( Display *display,
                               XVisualInfo * visinfo,
                               GLboolean rgb_flag,
                               GLboolean alpha_flag,
                               GLboolean db_flag,
                               GLboolean stereo_flag,
                               GLboolean ximage_flag,
                               GLint depth_size,
                               GLint stencil_size,
                               GLint accum_red_size,
                               GLint accum_green_size,
                               GLint accum_blue_size,
                               GLint accum_alpha_size,
                               GLint num_samples,
                               GLint level,
                               GLint visualCaveat )
{
   XMesaVisual v;
   GLint red_bits, green_bits, blue_bits, alpha_bits;

   xmesa_init();

   /* For debugging only */
   if (_mesa_getenv("MESA_XSYNC")) {
      /* This makes debugging X easier.
       * In your debugger, set a breakpoint on _XError to stop when an
       * X protocol error is generated.
       */
      XSynchronize( display, 1 );
   }

   v = (XMesaVisual) CALLOC_STRUCT(xmesa_visual);
   if (!v) {
      return NULL;
   }

   v->display = display;

   /* Save a copy of the XVisualInfo struct because the user may Xfree()
    * the struct but we may need some of the information contained in it
    * at a later time.
    */
   v->visinfo = (XVisualInfo *) MALLOC(sizeof(*visinfo));
   if (!v->visinfo) {
      free(v);
      return NULL;
   }
   memcpy(v->visinfo, visinfo, sizeof(*visinfo));

   v->ximage_flag = ximage_flag;

   v->mesa_visual.redMask = visinfo->red_mask;
   v->mesa_visual.greenMask = visinfo->green_mask;
   v->mesa_visual.blueMask = visinfo->blue_mask;
   v->mesa_visual.visualID = visinfo->visualid;
   v->mesa_visual.screen = visinfo->screen;

#if !(defined(__cplusplus) || defined(c_plusplus))
   v->mesa_visual.visualType = xmesa_convert_from_x_visual_type(visinfo->class);
#else
   v->mesa_visual.visualType = xmesa_convert_from_x_visual_type(visinfo->c_class);
#endif

   v->mesa_visual.visualRating = visualCaveat;

   if (alpha_flag)
      v->mesa_visual.alphaBits = 8;

   (void) initialize_visual_and_buffer( v, NULL, rgb_flag, 0, 0 );

   {
      const int xclass = v->mesa_visual.visualType;
      if (xclass == GLX_TRUE_COLOR || xclass == GLX_DIRECT_COLOR) {
         red_bits   = _mesa_bitcount(GET_REDMASK(v));
         green_bits = _mesa_bitcount(GET_GREENMASK(v));
         blue_bits  = _mesa_bitcount(GET_BLUEMASK(v));
      }
      else {
         /* this is an approximation */
         int depth;
         depth = v->visinfo->depth;
         red_bits = depth / 3;
         depth -= red_bits;
         green_bits = depth / 2;
         depth -= green_bits;
         blue_bits = depth;
         alpha_bits = 0;
         assert( red_bits + green_bits + blue_bits == v->visinfo->depth );
      }
      alpha_bits = v->mesa_visual.alphaBits;
   }

   _mesa_initialize_visual( &v->mesa_visual,
                            db_flag, stereo_flag,
                            red_bits, green_bits,
                            blue_bits, alpha_bits,
                            depth_size,
                            stencil_size,
                            accum_red_size, accum_green_size,
                            accum_blue_size, accum_alpha_size,
                            0 );

   /* XXX minor hack */
   v->mesa_visual.level = level;
   return v;
}


PUBLIC
void XMesaDestroyVisual( XMesaVisual v )
{
   free(v->visinfo);
   free(v);
}


/**
 * Do one-time initializations.
 */
void
xmesa_init(void)
{
   static GLboolean firstTime = GL_TRUE;
   if (firstTime) {
      pipe_mutex_init(_xmesa_lock);
      _screen = driver.create_pipe_screen();
      screen = trace_screen_create( _screen );
      firstTime = GL_FALSE;
   }
}


/**
 * Create a new XMesaContext.
 * \param v  the XMesaVisual
 * \param share_list  another XMesaContext with which to share display
 *                    lists or NULL if no sharing is wanted.
 * \return an XMesaContext or NULL if error.
 */
PUBLIC
XMesaContext XMesaCreateContext( XMesaVisual v, XMesaContext share_list )
{
   struct pipe_context *pipe = NULL;
   XMesaContext c;
   GLcontext *mesaCtx;
   uint pf;

   xmesa_init();

   /* Note: the XMesaContext contains a Mesa GLcontext struct (inheritance) */
   c = (XMesaContext) CALLOC_STRUCT(xmesa_context);
   if (!c)
      return NULL;

   pf = choose_pixel_format(v);
   assert(pf);

   c->xm_visual = v;
   c->xm_buffer = NULL;   /* set later by XMesaMakeCurrent */
   c->xm_read_buffer = NULL;

   if (screen == NULL)
      goto fail;

   /* Trace screen knows how to properly wrap context creation in the
    * wrapped screen, so nothing special to do here:
    */
   pipe = screen->context_create(screen, (void *) c);
   if (pipe == NULL)
      goto fail;

   c->st = st_create_context(pipe, 
                             &v->mesa_visual,
                             share_list ? share_list->st : NULL);
   if (c->st == NULL)
      goto fail;

   mesaCtx = c->st->ctx;
   c->st->ctx->DriverCtx = c;

   return c;

fail:
   if (c->st)
      st_destroy_context(c->st);
   else if (pipe)
      pipe->destroy(pipe);

   free(c);
   return NULL;
}



PUBLIC
void XMesaDestroyContext( XMesaContext c )
{
   st_destroy_context(c->st);

   /* FIXME: We should destroy the screen here, but if we do so, surfaces may 
    * outlive it, causing segfaults
   struct pipe_screen *screen = c->st->pipe->screen; 
   screen->destroy(screen);
   */

   free(c);
}



/**
 * Private function for creating an XMesaBuffer which corresponds to an
 * X window or pixmap.
 * \param v  the window's XMesaVisual
 * \param w  the window we're wrapping
 * \return  new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreateWindowBuffer(XMesaVisual v, Window w)
{
   XWindowAttributes attr;
   XMesaBuffer b;
   Colormap cmap;
   int depth;

   assert(v);
   assert(w);

   /* Check that window depth matches visual depth */
   XGetWindowAttributes( v->display, w, &attr );
   depth = attr.depth;
   if (v->visinfo->depth != depth) {
      _mesa_warning(NULL, "XMesaCreateWindowBuffer: depth mismatch between visual (%d) and window (%d)!\n",
                    v->visinfo->depth, depth);
      return NULL;
   }

   /* Find colormap */
   if (attr.colormap) {
      cmap = attr.colormap;
   }
   else {
      _mesa_warning(NULL, "Window %u has no colormap!\n", (unsigned int) w);
      /* this is weird, a window w/out a colormap!? */
      /* OK, let's just allocate a new one and hope for the best */
      cmap = XCreateColormap(v->display, w, attr.visual, AllocNone);
   }

   b = create_xmesa_buffer((Drawable) w, WINDOW, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer( v, b, v->mesa_visual.rgbMode,
                                      (Drawable) w, cmap )) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/**
 * Create a new XMesaBuffer from an X pixmap.
 *
 * \param v    the XMesaVisual
 * \param p    the pixmap
 * \param cmap the colormap, may be 0 if using a \c GLX_TRUE_COLOR or
 *             \c GLX_DIRECT_COLOR visual for the pixmap
 * \returns new XMesaBuffer or NULL if error
 */
PUBLIC XMesaBuffer
XMesaCreatePixmapBuffer(XMesaVisual v, Pixmap p, Colormap cmap)
{
   XMesaBuffer b;

   assert(v);

   b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}


/**
 * For GLX_EXT_texture_from_pixmap
 */
XMesaBuffer
XMesaCreatePixmapTextureBuffer(XMesaVisual v, Pixmap p,
                               Colormap cmap,
                               int format, int target, int mipmap)
{
   GET_CURRENT_CONTEXT(ctx);
   XMesaBuffer b;
   GLuint width, height;

   assert(v);

   b = create_xmesa_buffer((Drawable) p, PIXMAP, v, cmap);
   if (!b)
      return NULL;

   /* get pixmap size, update framebuffer/renderbuffer dims */
   xmesa_get_window_size(v->display, b, &width, &height);
   _mesa_resize_framebuffer(NULL, &(b->stfb->Base), width, height);

   if (target == 0) {
      /* examine dims */
      if (ctx->Extensions.ARB_texture_non_power_of_two) {
         target = GLX_TEXTURE_2D_EXT;
      }
      else if (   _mesa_bitcount(width)  == 1
               && _mesa_bitcount(height) == 1) {
         /* power of two size */
         if (height == 1) {
            target = GLX_TEXTURE_1D_EXT;
         }
         else {
            target = GLX_TEXTURE_2D_EXT;
         }
      }
      else if (ctx->Extensions.NV_texture_rectangle) {
         target = GLX_TEXTURE_RECTANGLE_EXT;
      }
      else {
         /* non power of two textures not supported */
         XMesaDestroyBuffer(b);
         return 0;
      }
   }

   b->TextureTarget = target;
   b->TextureFormat = format;
   b->TextureMipmap = mipmap;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     (Drawable) p, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



XMesaBuffer
XMesaCreatePBuffer(XMesaVisual v, Colormap cmap,
                   unsigned int width, unsigned int height)
{
   Window root;
   Drawable drawable;  /* X Pixmap Drawable */
   XMesaBuffer b;

   /* allocate pixmap for front buffer */
   root = RootWindow( v->display, v->visinfo->screen );
   drawable = XCreatePixmap(v->display, root, width, height,
                            v->visinfo->depth);
   if (!drawable)
      return NULL;

   b = create_xmesa_buffer(drawable, PBUFFER, v, cmap);
   if (!b)
      return NULL;

   if (!initialize_visual_and_buffer(v, b, v->mesa_visual.rgbMode,
				     drawable, cmap)) {
      xmesa_free_buffer(b);
      return NULL;
   }

   return b;
}



/*
 * Deallocate an XMesaBuffer structure and all related info.
 */
PUBLIC void
XMesaDestroyBuffer(XMesaBuffer b)
{
   xmesa_free_buffer(b);
}


/**
 * Query the current window size and update the corresponding GLframebuffer
 * and all attached renderbuffers.
 * Called when:
 *  1. the first time a buffer is bound to a context.
 *  2. SwapBuffers.  XXX probabaly from xm_flush_frontbuffer() too...
 * Note: it's possible (and legal) for xmctx to be NULL.  That can happen
 * when resizing a buffer when no rendering context is bound.
 */
void
xmesa_check_and_update_buffer_size(XMesaContext xmctx, XMesaBuffer drawBuffer)
{
   GLuint width, height;
   xmesa_get_window_size(drawBuffer->xm_visual->display, drawBuffer, &width, &height);
   st_resize_framebuffer(drawBuffer->stfb, width, height);
}




/*
 * Bind buffer b to context c and make c the current rendering context.
 */
PUBLIC
GLboolean XMesaMakeCurrent2( XMesaContext c, XMesaBuffer drawBuffer,
                             XMesaBuffer readBuffer )
{
   XMesaContext old_ctx = XMesaGetCurrentContext();

   if (old_ctx && old_ctx != c) {
      XMesaFlush(old_ctx);
      old_ctx->xm_buffer = NULL;
      old_ctx->xm_read_buffer = NULL;
   }

   if (c) {
      if (!drawBuffer || !readBuffer)
         return GL_FALSE;  /* must specify buffers! */

      if (c == old_ctx &&
	  c->xm_buffer == drawBuffer &&
	  c->xm_read_buffer == readBuffer)
	 return GL_TRUE;

      c->xm_buffer = drawBuffer;
      c->xm_read_buffer = readBuffer;

      st_make_current(c->st, drawBuffer->stfb, readBuffer->stfb);

      xmesa_check_and_update_buffer_size(c, drawBuffer);
      if (readBuffer != drawBuffer)
         xmesa_check_and_update_buffer_size(c, readBuffer);

      /* Solution to Stephane Rehel's problem with glXReleaseBuffersMESA(): */
      drawBuffer->wasCurrent = GL_TRUE;
   }
   else {
      /* Detach */
      st_make_current( NULL, NULL, NULL );

   }
   return GL_TRUE;
}


/*
 * Unbind the context c from its buffer.
 */
GLboolean XMesaUnbindContext( XMesaContext c )
{
   /* A no-op for XFree86 integration purposes */
   return GL_TRUE;
}


XMesaContext XMesaGetCurrentContext( void )
{
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      XMesaContext xmesa = xmesa_context(ctx);
      return xmesa;
   }
   else {
      return 0;
   }
}



/**
 * Swap front and back color buffers and have winsys display front buffer.
 * If there's no front color buffer no swap actually occurs.
 */
PUBLIC
void XMesaSwapBuffers( XMesaBuffer b )
{
   struct pipe_surface *frontLeftSurf;

   st_swapbuffers(b->stfb, &frontLeftSurf, NULL);

   if (frontLeftSurf) {
      if (_screen != screen) {
         struct trace_surface *tr_surf = trace_surface( frontLeftSurf );
         struct pipe_surface *surf = tr_surf->surface;
         frontLeftSurf = surf;
      }

      driver.display_surface(b, frontLeftSurf);
   }

   xmesa_check_and_update_buffer_size(NULL, b);
}



/*
 * Copy sub-region of back buffer to front buffer
 */
void XMesaCopySubBuffer( XMesaBuffer b, int x, int y, int width, int height )
{
   struct pipe_surface *surf_front;
   struct pipe_surface *surf_back;
   struct pipe_context *pipe = NULL; /* XXX fix */

   st_get_framebuffer_surface(b->stfb, ST_SURFACE_FRONT_LEFT, &surf_front);
   st_get_framebuffer_surface(b->stfb, ST_SURFACE_BACK_LEFT, &surf_back);

   if (!surf_front || !surf_back)
      return;

   assert(pipe);
   pipe->surface_copy(pipe,
                      surf_front, x, y,  /* dest */
                      surf_back, x, y,   /* src */
                      width, height);
}



void XMesaFlush( XMesaContext c )
{
   if (c && c->xm_visual->display) {
      st_finish(c->st);
      XSync( c->xm_visual->display, False );
   }
}





XMesaBuffer XMesaFindBuffer( Display *dpy, Drawable d )
{
   XMesaBuffer b;
   for (b = XMesaBufferList; b; b = b->Next) {
      if (b->drawable == d && b->xm_visual->display == dpy) {
         return b;
      }
   }
   return NULL;
}


/**
 * Free/destroy all XMesaBuffers associated with given display.
 */
void xmesa_destroy_buffers_on_display(Display *dpy)
{
   XMesaBuffer b, next;
   for (b = XMesaBufferList; b; b = next) {
      next = b->Next;
      if (b->xm_visual->display == dpy) {
         xmesa_free_buffer(b);
      }
   }
}


/*
 * Look for XMesaBuffers whose X window has been destroyed.
 * Deallocate any such XMesaBuffers.
 */
void XMesaGarbageCollect( void )
{
   XMesaBuffer b, next;
   for (b=XMesaBufferList; b; b=next) {
      next = b->Next;
      if (b->xm_visual &&
          b->xm_visual->display &&
          b->drawable &&
          b->type == WINDOW) {
         XSync(b->xm_visual->display, False);
         if (!window_exists( b->xm_visual->display, b->drawable )) {
            /* found a dead window, free the ancillary info */
            XMesaDestroyBuffer( b );
         }
      }
   }
}




PUBLIC void
XMesaBindTexImage(Display *dpy, XMesaBuffer drawable, int buffer,
                  const int *attrib_list)
{
}



PUBLIC void
XMesaReleaseTexImage(Display *dpy, XMesaBuffer drawable, int buffer)
{
}

