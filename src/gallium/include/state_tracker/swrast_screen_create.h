#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "target-helpers/wrap_screen.h"

struct sw_winsys;

#ifdef GALLIUM_SOFTPIPE
#include "softpipe/sp_public.h"
#endif

#ifdef GALLIUM_LLVMPIPE
#include "llvmpipe/lp_public.h"
#endif

#ifdef GALLIUM_CELL
#include "cell/ppu/cell_public.h"
#endif

/*
 * Helper function to choose and instantiate one of the software rasterizers:
 * cell, llvmpipe, softpipe.
 *
 * This function could be shared, but currently causes headaches for
 * the build systems, particularly scons if we try.  Long term, want
 * to avoid having global #defines for things like GALLIUM_LLVMPIPE,
 * GALLIUM_CELL, etc.  Scons already eliminates those #defines, so
 * things that are painful for it now are likely to be painful for
 * other build systems in the future.
 */

static INLINE struct pipe_screen *
swrast_screen_create(struct sw_winsys *winsys)
{
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;

#if defined(GALLIUM_CELL)
   default_driver = "cell";
#elif defined(GALLIUM_LLVMPIPE)
   default_driver = "llvmpipe";
#elif defined(GALLIUM_SOFTPIPE)
   default_driver = "softpipe";
#else
   default_driver = "";
#endif

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);

#if defined(GALLIUM_CELL)
   if (screen == NULL && strcmp(driver, "cell") == 0)
      screen = cell_create_screen( winsys );
#endif

#if defined(GALLIUM_LLVMPIPE)
   if (screen == NULL && strcmp(driver, "llvmpipe") == 0)
      screen = llvmpipe_create_screen( winsys );
#endif

#if defined(GALLIUM_SOFTPIPE)
   if (screen == NULL)
      screen = softpipe_create_screen( winsys );
#endif

   return gallium_wrap_screen( screen );
}

