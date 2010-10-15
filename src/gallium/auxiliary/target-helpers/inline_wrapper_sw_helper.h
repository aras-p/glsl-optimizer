
#ifndef INLINE_WRAPPER_SW_HELPER_H
#define INLINE_WRAPPER_SW_HELPER_H

#include "target-helpers/inline_sw_helper.h"
#include "sw/wrapper/wrapper_sw_winsys.h"

/**
 * Try to wrap a hw screen with a software screen.
 * On failure will return given screen.
 */
static INLINE struct pipe_screen *
sw_screen_wrap(struct pipe_screen *screen)
{
   struct sw_winsys *sws;
   struct pipe_screen *sw_screen;

   sws = wrapper_sw_winsys_wrap_pipe_screen(screen);
   if (!sws)
      goto err;

   sw_screen = sw_screen_create(sws);
   if (sw_screen == screen)
      goto err_winsys;

   return sw_screen;

err_winsys:
   sws->destroy(sws);
err:
  return screen;
}

#endif
