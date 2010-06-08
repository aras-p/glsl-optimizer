/* Display a cleared blue window.  This demo has no dependencies on
 * any utility code, just the graw interface and gallium.
 */

#include "state_tracker/graw.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"

#include "util/u_debug.h"       /* debug_dump_surface_bmp() */

enum pipe_format formats[] = {
   PIPE_FORMAT_R8G8B8A8_UNORM,
   PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_NONE
};

static const int WIDTH = 300;
static const int HEIGHT = 300;

struct pipe_screen *screen;
struct pipe_context *ctx;
struct pipe_surface *surf;
static void *window = NULL;

static void draw( void )
{
   float clear_color[4] = {1,0,1,1};

   ctx->clear(ctx, PIPE_CLEAR_COLOR, clear_color, 0, 0);
   ctx->flush(ctx, PIPE_FLUSH_RENDER_CACHE, NULL);

#if 0
   /* At the moment, libgraw leaks out/makes available some of the
    * symbols from gallium/auxiliary, including these debug helpers.
    * Will eventually want to bless some of these paths, and lock the
    * others down so they aren't accessible from test programs.
    *
    * This currently just happens to work on debug builds - a release
    * build will probably fail to link here:
    */
   debug_dump_surface_bmp(ctx, "result.bmp", surf);
#endif

   screen->flush_frontbuffer(screen, surf, window);
}

static void init( void )
{
   struct pipe_framebuffer_state fb;
   struct pipe_resource *tex, templat;
   int i;

   /* It's hard to say whether window or screen should be created
    * first.  Different environments would prefer one or the other.
    *
    * Also, no easy way of querying supported formats if the screen
    * cannot be created first.
    */
   for (i = 0; 
        window == NULL && formats[i] != PIPE_FORMAT_NONE;
        i++) {
      
      screen = graw_create_window_and_screen(0,0,300,300,
                                             formats[i],
                                             &window);
   }
   if (window == NULL)
      exit(2);
   
   ctx = screen->context_create(screen, NULL);
   if (ctx == NULL)
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

   ctx->set_framebuffer_state(ctx, &fb);
}



int main( int argc, char *argv[] )
{
   init();

   graw_set_display_func( draw );
   graw_main_loop();
   return 0;
}
