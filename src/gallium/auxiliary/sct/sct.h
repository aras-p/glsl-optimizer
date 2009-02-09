/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * Surface/Context Tracking
 *
 * For some drivers, we need to monitor the binding between contexts and
 * surfaces/textures.
 * This code may evolve quite a bit...
 */


#ifndef SCT_H
#define SCT_H


#ifdef	__cplusplus
extern "C" {
#endif


struct pipe_context;
struct pipe_surface;

struct sct_context;
struct sct_surface;


/**
 * Per-device info, basically
 */
struct surface_context_tracker
{
   struct sct_context *contexts;
   struct sct_surface *surfaces;
};



/**
 * Simple linked list of contexts
 */
struct sct_context_list
{
   const struct pipe_context *context;
   struct sct_context_list *next;
};



extern void
sct_bind_surfaces(struct surface_context_tracker *sct,
                  struct pipe_context *context,
                  uint num_surf,
                  struct pipe_surface **surfaces);


extern void
sct_bind_texture(struct surface_context_tracker *sct,
                 struct pipe_context *context,
                 uint unit,
                 struct pipe_texture *texture);


extern void
sct_update_texture(struct pipe_texture *tex);


extern boolean
sct_is_texture_used(struct surface_context_tracker *sct,
                    const struct pipe_context *context,
                    const struct pipe_texture *texture);

extern void
sct_flush_textures(struct surface_context_tracker *sct,
                   struct pipe_context *context);


extern const struct sct_context_list *
sct_get_surface_contexts(struct surface_context_tracker *sct,
                         const struct pipe_surface *surf);


extern void
sct_destroy_context(struct surface_context_tracker *sct,
                    struct pipe_context *context);


extern void
sct_destroy_surface(struct surface_context_tracker *sct,
                    struct pipe_surface *surface);



#ifdef	__cplusplus
}
#endif

#endif /* SCT_H */
