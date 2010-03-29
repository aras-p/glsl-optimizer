#ifndef _DRISW_API_H_
#define _DRISW_API_H_

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_format.h"

#include "state_tracker/drm_api.h"

struct pipe_screen;
struct pipe_winsys;
struct pipe_buffer;
struct pipe_context;
struct pipe_texture;

struct dri_drawable;

/**
 * This callback struct is intended for the winsys to call the loader.
 */

struct drisw_loader_funcs
{
   void (*put_image) (struct dri_drawable *dri_drawable,
                      void *data, unsigned width, unsigned height);
};

struct drisw_create_screen_arg
{
   struct drm_create_screen_arg base;

   struct drisw_loader_funcs *lf;
};

#endif
