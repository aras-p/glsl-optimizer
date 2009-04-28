#ifndef _DRI1_API_H_
#define _DRI1_API_H_

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_format.h"

#include "state_tracker/drm_api.h"

#include <drm.h>

struct pipe_screen;
struct pipe_winsys;
struct pipe_buffer;
struct pipe_context;
struct pipe_texture;

struct dri1_api_version
{
   int major;
   int minor;
   int patch_level;
};

/**
 * This callback struct is intended for drivers that need to take
 * the hardware lock on command submission.
 */

struct dri1_api_lock_funcs
{
   void (*lock) (struct pipe_context * pipe);
   void (*unlock) (struct pipe_context * locked_pipe);
      boolean(*is_locked) (struct pipe_context * locked_pipe);
      boolean(*is_lock_lost) (struct pipe_context * locked_pipe);
   void (*clear_lost_lock) (struct pipe_context * locked_pipe);
};

struct dri1_api
{
   /**
    * For flushing to the front buffer. A driver should implement one and only
    * one of the functions below. The present_locked functions allows a dri1
    * driver to pageflip.
    */

   /*@{ */

   struct pipe_surface *(*front_srf_locked) (struct pipe_context *
					     locked_pipe);

   void (*present_locked) (struct pipe_context * locked_pipe,
			   struct pipe_surface * surf,
			   const struct drm_clip_rect * rect,
			   unsigned int num_clip,
			   int x_draw, int y_draw,
			   const struct drm_clip_rect * src_bbox,
			   struct pipe_fence_handle ** fence);
   /*@} */
};

struct dri1_create_screen_arg
{
   struct drm_create_screen_arg base;

   struct dri1_api_lock_funcs *lf;
   void *ddx_info;
   int ddx_info_size;
   void *sarea;

   struct dri1_api_version ddx_version;
   struct dri1_api_version dri_version;
   struct dri1_api_version drm_version;

   /*
    * out parameters;
    */

   struct dri1_api *api;
};

#endif
