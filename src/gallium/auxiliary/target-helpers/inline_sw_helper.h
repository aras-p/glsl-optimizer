
#ifndef INLINE_SW_HELPER_H
#define INLINE_SW_HELPER_H

#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "state_tracker/sw_winsys.h"


/* Helper function to choose and instantiate one of the software rasterizers:
 * llvmpipe, softpipe.
 */

#ifdef GALLIUM_SOFTPIPE
#include "softpipe/sp_public.h"
#endif

#ifdef GALLIUM_LLVMPIPE
#include "llvmpipe/lp_public.h"
#endif


static INLINE struct pipe_screen *
sw_screen_create_named(struct sw_winsys *winsys, const char *driver)
{
   struct pipe_screen *screen = NULL;

#if defined(GALLIUM_LLVMPIPE)
   if (screen == NULL && strcmp(driver, "llvmpipe") == 0)
      screen = llvmpipe_create_screen(winsys);
#endif

#if defined(GALLIUM_SOFTPIPE)
   if (screen == NULL)
      screen = softpipe_create_screen(winsys);
#endif

   return screen;
}


static INLINE struct pipe_screen *
sw_screen_create(struct sw_winsys *winsys)
{
   const char *default_driver;
   const char *driver;

#if defined(GALLIUM_LLVMPIPE)
   default_driver = "llvmpipe";
#elif defined(GALLIUM_SOFTPIPE)
   default_driver = "softpipe";
#else
   default_driver = "";
#endif

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);
   return sw_screen_create_named(winsys, driver);
}

#if defined(GALLIUM_SOFTPIPE)
#if defined(DRI_TARGET)
#include "target-helpers/inline_debug_helper.h"
#include "sw/dri/dri_sw_winsys.h"
#include "dri_screen.h"

const __DRIextension **__driDriverGetExtensions_swrast(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_swrast(void)
{
   globalDriverAPI = &galliumsw_driver_api;
   return galliumsw_driver_extensions;
}

INLINE struct pipe_screen *
drisw_create_screen(struct drisw_loader_funcs *lf)
{
   struct sw_winsys *winsys = NULL;
   struct pipe_screen *screen = NULL;

   winsys = dri_create_sw_winsys(lf);
   if (winsys == NULL)
      return NULL;

   screen = sw_screen_create(winsys);
   if (screen == NULL) {
      winsys->destroy(winsys);
      return NULL;
   }

   screen = debug_screen_wrap(screen);
   return screen;
}
#endif // DRI_TARGET
#endif // GALLIUM_SOFTPIPE


#endif
