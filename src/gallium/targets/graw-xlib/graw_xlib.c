#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_text.h"
#include "target-helpers/wrap_screen.h"
#include "state_tracker/xlib_sw_winsys.h"

#ifdef GALLIUM_SOFTPIPE
#include "softpipe/sp_public.h"
#endif

#ifdef GALLIUM_LLVMPIPE
#include "llvmpipe/lp_public.h"
#endif

/* Haven't figured out a decent way to build the helper code yet -
 * #include it here temporarily.
 */
#include "sw/sw_public.h"
#include "sw/sw.c"

#include "state_tracker/graw.h"

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <stdio.h>

static struct {
   Display *display;
   void (*draw)(void);
} graw;


struct pipe_screen *
graw_init( void )
{
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;
   struct sw_winsys *winsys = NULL;

   graw.display = XOpenDisplay(NULL);
   if (graw.display == NULL)
      return NULL;

   /* Create the underlying winsys, which performs presents to Xlib
    * drawables:
    */
   winsys = xlib_create_sw_winsys( graw.display );
   if (winsys == NULL)
      return NULL;

#if defined(GALLIUM_LLVMPIPE)
   default_driver = "llvmpipe";
#elif defined(GALLIUM_SOFTPIPE)
   default_driver = "softpipe";
#else
   default_driver = "";
#endif

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);

#if defined(GALLIUM_LLVMPIPE)
   if (screen == NULL && strcmp(driver, "llvmpipe") == 0)
      screen = llvmpipe_create_screen( winsys );
#endif

#if defined(GALLIUM_SOFTPIPE)
   if (screen == NULL)
      screen = softpipe_create_screen( winsys );
#endif

   /* Inject any wrapping layers we want to here:
    */
   return gallium_wrap_screen( screen );
}


 


void *
graw_create_window( int x,
                    int y,
                    unsigned width,
                    unsigned height,
                    enum pipe_format format )
{
   struct xlib_drawable *handle = NULL;
   XSetWindowAttributes attr;
   Window root;
   Window win = 0;
   XVisualInfo templat, *visinfo = NULL;
   unsigned mask;
   int n;
   int scrnum;


   scrnum = DefaultScreen( graw.display );
   root = RootWindow( graw.display, scrnum );


   if (format != PIPE_FORMAT_R8G8B8A8_UNORM)
      goto fail;

   if (graw.display == NULL)
      goto fail;

   handle = CALLOC_STRUCT(xlib_drawable);
   if (handle == NULL)
      goto fail;


   mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
   templat.screen = DefaultScreen(graw.display);
   templat.depth = 32;
   templat.class = TrueColor;

   visinfo = XGetVisualInfo(graw.display, mask, &templat, &n);
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( graw.display, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   /* XXX this is a bad way to get a borderless window! */
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( graw.display, root, x, y, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );


   /* set hints and properties */
   {
      char *name = NULL;
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(graw.display, win, &sizehints);
      XSetStandardProperties(graw.display, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   XFree(visinfo);
   XMapWindow(graw.display, win);
   while (1) {
      XEvent e;
      XNextEvent( graw.display, &e );
      if (e.type == MapNotify && e.xmap.window == win) {
	 break;
      }
   }
   
   handle->visual = visinfo->visual;
   handle->drawable = (Drawable)win;
   handle->depth = visinfo->depth;
   return (void *)handle;

fail:
   FREE(handle);
   XFree(visinfo);

   if (win)
      XDestroyWindow(graw.display, win);

   return NULL;
}


void
graw_destroy_window( void *xlib_drawable )
{
}

void 
graw_set_display_func( void (*draw)( void ) )
{
   graw.draw = draw;
}

void
graw_main_loop( void )
{
   int i;
   for (i = 0; i < 10; i++) {
      graw.draw();
      sleep(1);
   }
}



/* Helper functions.  These are the same for all graw implementations.
 */
void *graw_parse_vertex_shader(struct pipe_context *pipe,
                               const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_vs_state(pipe, &state);
}

void *graw_parse_fragment_shader(struct pipe_context *pipe,
                                 const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_fs_state(pipe, &state);
}

