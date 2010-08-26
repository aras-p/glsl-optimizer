#ifndef GALLIUM_RAW_DL_H
#define GALLIUM_RAW_DL_H

/* This is an API for exercising gallium functionality in a
 * platform-neutral fashion.  Whatever platform integration is
 * necessary to implement this interface is orchestrated by the
 * individual target building this entity.
 *
 * For instance, the graw-xlib target includes code to implent these
 * interfaces on top of the X window system.
 *
 * Programs using this interface may additionally benefit from some of
 * the utilities currently in the libgallium.a library, especially
 * those for parsing text representations of TGSI shaders.
 */

#include  <stdio.h>
#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "util/u_dl.h"
#include "tgsi/tgsi_text.h"


struct pipe_screen;
struct pipe_context;


typedef void *
(*pfn_graw_create_window_and_screen_t)( int x,
                                        int y,
                                        unsigned width,
                                        unsigned height,
                                        enum pipe_format format,
			                void **handle );

typedef void
(*pfn_graw_set_display_func_t)( void (*func)( void ) );

typedef void
(*pfn_graw_main_loop_t)( void );


static pfn_graw_create_window_and_screen_t
pfn_graw_create_window_and_screen = NULL;

static pfn_graw_set_display_func_t
pfn_graw_set_display_func = NULL;

static pfn_graw_main_loop_t
pfn_graw_main_loop = NULL;


static INLINE void *
graw_create_window_and_screen( int x,
                               int y,
                               unsigned width,
                               unsigned height,
                               enum pipe_format format,
                               void **handle )
{
   static struct util_dl_library *lib;
   lib = util_dl_open(UTIL_DL_PREFIX "graw" UTIL_DL_EXT);
   if (!lib)
      goto error;
   pfn_graw_create_window_and_screen = (pfn_graw_create_window_and_screen_t)
      util_dl_get_proc_address(lib, "graw_create_window_and_screen");
   if (!pfn_graw_create_window_and_screen)
      goto error;
   pfn_graw_set_display_func = (pfn_graw_set_display_func_t)
      util_dl_get_proc_address(lib, "graw_set_display_func");
   if (!pfn_graw_set_display_func)
      goto error;
   pfn_graw_main_loop = (pfn_graw_main_loop_t)
      util_dl_get_proc_address(lib, "graw_main_loop");
   if (!pfn_graw_main_loop)
      goto error;
   return pfn_graw_create_window_and_screen(x, y, width, height, format, handle );
error:
   fprintf(stderr, "failed to open " UTIL_DL_PREFIX "graw" UTIL_DL_EXT "\n");
   return NULL;
}

static INLINE void 
graw_set_display_func( void (*func)( void ) )
{
   if (!pfn_graw_set_display_func)
      return;
   pfn_graw_set_display_func(func);
}

static INLINE void 
graw_main_loop( void )
{
   if (!pfn_graw_main_loop)
      return;
   pfn_graw_main_loop();
}


/* 
 * Helper functions.  These are the same for all graw implementations.
 *
 * XXX: These aren't graw related. If they are useful then should go somwhere
 * inside auxiliary/util.
 */

#define GRAW_MAX_NUM_TOKENS 1024

static INLINE void *
graw_parse_geometry_shader(struct pipe_context *pipe,
                           const char *text)
{
   struct tgsi_token tokens[GRAW_MAX_NUM_TOKENS];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, GRAW_MAX_NUM_TOKENS))
      return NULL;

   state.tokens = tokens;
   return pipe->create_gs_state(pipe, &state);
}

static INLINE void *
graw_parse_vertex_shader(struct pipe_context *pipe,
                         const char *text)
{
   struct tgsi_token tokens[GRAW_MAX_NUM_TOKENS];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, GRAW_MAX_NUM_TOKENS))
      return NULL;

   state.tokens = tokens;
   return pipe->create_vs_state(pipe, &state);
}

static INLINE void *
graw_parse_fragment_shader(struct pipe_context *pipe,
                           const char *text)
{
   struct tgsi_token tokens[GRAW_MAX_NUM_TOKENS];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, GRAW_MAX_NUM_TOKENS))
      return NULL;

   state.tokens = tokens;
   return pipe->create_fs_state(pipe, &state);
}

#endif
