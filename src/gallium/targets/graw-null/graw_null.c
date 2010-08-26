#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "target-helpers/wrap_screen.h"
#include "sw/null/null_sw_winsys.h"
#include "os/os_time.h"
#include "state_tracker/graw.h"

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

#include <stdio.h>


static struct {
   void (*draw)(void);
} graw;



struct pipe_screen *
graw_create_window_and_screen( int x,
                               int y,
                               unsigned width,
                               unsigned height,
                               enum pipe_format format,
                               void **handle)
{
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;
   struct sw_winsys *winsys = NULL;
   static int dummy;


   /* Create the underlying winsys, which performs presents to Xlib
    * drawables:
    */
   winsys = null_sw_create();
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

   *handle = &dummy;

   /* Inject any wrapping layers we want to here:
    */
   return gallium_wrap_screen( screen );
}



void 
graw_set_display_func( void (*draw)( void ) )
{
   graw.draw = draw;
}


void
graw_main_loop( void )
{
   graw.draw();
   os_time_sleep(100000);
}
