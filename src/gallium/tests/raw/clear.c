/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include "state_tracker/graw.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include <unistd.h>             /* for sleep() */

#include "util/u_debug.h"       /* debug_dump_surface_bmp() */

enum pipe_format formats[] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE
};

static const int WIDTH = 300;
static const int HEIGHT = 300;

int main( int argc, char *argv[] )
{
   struct pipe_screen *screen;
   struct pipe_context *pipe;
   struct pipe_surface *surf;
   struct pipe_framebuffer_state fb;
   struct pipe_resource *tex, templat;
   void *window = NULL;
   float clear_color[4] = {1,0,1,1};
   int i;

   screen = graw_init();
   if (screen == NULL)
      exit(1);

   for (i = 0; 
        window == NULL && formats[i] != PIPE_FORMAT_NONE;
        i++) {
      
      window = graw_create_window(0,0,300,300, formats[i]);
   }
   
   if (window == NULL)
      exit(2);
   
   pipe = screen->context_create(screen, NULL);
   if (pipe == NULL)
      exit(3);

   templat.target = PIPE_TEXTURE_2D;
   templat.format = formats[i];
   templat.width0 = WIDTH;
   templat.height0 = HEIGHT;
   templat.depth0 = 1;
   templat.last_level = 0;
   templat.nr_samples = 1;
   templat.bind = (PIPE_BIND_RENDER_TARGET |
                        PIPE_BIND_DISPLAY_TARGET);
   
   tex = screen->resource_create(screen,
                                &templat);
   if (tex == NULL)
      exit(4);

   surf = screen->get_tex_surface(screen, tex, 0, 0, 0,
                                  PIPE_BIND_RENDER_TARGET |
                                  PIPE_BIND_DISPLAY_TARGET);
   if (surf == NULL)
      exit(5);

   memset(&fb, 0, sizeof fb);
   fb.nr_cbufs = 1;
   fb.width = WIDTH;
   fb.height = HEIGHT;
   fb.cbufs[0] = surf;

   pipe->set_framebuffer_state(pipe, &fb);
   pipe->clear(pipe, PIPE_CLEAR_COLOR, clear_color, 0, 0);
   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* At the moment, libgraw includes/makes available all the symbols
    * from gallium/auxiliary, including these debug helpers.  Will
    * eventually want to bless some of these paths, and lock the
    * others down so they aren't accessible from test programs.
    */
   if (0)
      debug_dump_surface_bmp(pipe, "result.bmp", surf);

   screen->flush_frontbuffer(screen, surf, window);

   sleep(100);
   return 0;
}
