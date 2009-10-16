/*
 * Copyright © 2009 Corbin Simpson
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 *      Joakim Sindholt <opensource@zhasha.com>
 */

#include "radeon_drm.h"

/* Create a pipe_screen. */
struct pipe_screen* radeon_create_screen(struct drm_api* api,
                                         int drmFB,
                                         struct drm_create_screen_arg *arg)
{
    struct radeon_winsys* winsys = radeon_pipe_winsys(drmFB);

    if (debug_get_bool_option("RADEON_SOFTPIPE", FALSE)) {
        return softpipe_create_screen((struct pipe_winsys*)winsys);
    } else {
        struct r300_winsys* r300 = radeon_create_r300_winsys(drmFB, winsys);
        FREE(winsys);
        return r300_create_screen(r300);
    }
}

/* Create a pipe_context. */
struct pipe_context* radeon_create_context(struct drm_api* api,
                                           struct pipe_screen* screen)
{
    if (debug_get_bool_option("RADEON_SOFTPIPE", FALSE)) {
        return radeon_create_softpipe(screen->winsys);
    } else {
        return r300_create_context(screen,
                                   (struct r300_winsys*)screen->winsys);
    }
}

boolean radeon_buffer_from_texture(struct drm_api* api,
                                   struct pipe_texture* texture,
                                   struct pipe_buffer** buffer,
                                   unsigned* stride)
{
    /* XXX fix this */
    return r300_get_texture_buffer(texture, buffer, stride);
}

/* Create a buffer from a handle. */
/* XXX what's up with name? */
struct pipe_buffer* radeon_buffer_from_handle(struct drm_api* api,
                                              struct pipe_screen* screen,
                                              const char* name,
                                              unsigned handle)
{
    struct radeon_bo_manager* bom =
        ((struct radeon_winsys*)screen->winsys)->priv->bom;
    struct radeon_pipe_buffer* radeon_buffer;
    struct radeon_bo* bo = NULL;

    bo = radeon_bo_open(bom, handle, 0, 0, 0, 0);
    if (bo == NULL) {
        return NULL;
    }

    radeon_buffer = CALLOC_STRUCT(radeon_pipe_buffer);
    if (radeon_buffer == NULL) {
        radeon_bo_unref(bo);
        return NULL;
    }

    pipe_reference_init(&radeon_buffer->base.reference, 1);
    radeon_buffer->base.screen = screen;
    radeon_buffer->base.usage = PIPE_BUFFER_USAGE_PIXEL;
    radeon_buffer->bo = bo;
    return &radeon_buffer->base;
}

static struct pipe_texture*
radeon_texture_from_shared_handle(struct drm_api *api,
                                  struct pipe_screen *screen,
                                  struct pipe_texture *templ,
                                  const char *name,
                                  unsigned stride,
                                  unsigned handle)
{
    struct pipe_buffer *buffer;
    struct pipe_texture *blanket;

    buffer = radeon_buffer_from_handle(api, screen, name, handle);
    if (!buffer) {
        return NULL;
    }

    blanket = screen->texture_blanket(screen, templ, &stride, buffer);

    pipe_buffer_reference(&buffer, NULL);

    return blanket;
}

static boolean radeon_shared_handle_from_texture(struct drm_api *api,
                                                 struct pipe_screen *screen,
                                                 struct pipe_texture *texture,
                                                 unsigned *stride,
                                                 unsigned *handle)
{
    int retval, fd;
    struct drm_gem_flink flink;
    struct radeon_pipe_buffer* radeon_buffer;
    struct pipe_buffer *buffer;

    if (!radeon_buffer_from_texture(api, texture, &buffer, stride)) {
        return FALSE;
    }

    radeon_buffer = (struct radeon_pipe_buffer*)buffer;
    if (!radeon_buffer->flinked) {
        fd = ((struct radeon_winsys*)screen->winsys)->priv->fd;

        flink.handle = radeon_buffer->bo->handle;

        retval = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
        if (retval) {
            debug_printf("radeon: DRM_IOCTL_GEM_FLINK failed, error %d\n",
                    retval);
            return FALSE;
        }

        radeon_buffer->flink = flink.name;
        radeon_buffer->flinked = TRUE;
    }

    *handle = radeon_buffer->flink;
    return TRUE;
}

static boolean radeon_local_handle_from_texture(struct drm_api *api,
                                                struct pipe_screen *screen,
                                                struct pipe_texture *texture,
                                                unsigned *stride,
                                                unsigned *handle)
{
    struct pipe_buffer *buffer;
    if (!radeon_buffer_from_texture(api, texture, &buffer, stride)) {
        return FALSE;
    }

    *handle = ((struct radeon_pipe_buffer*)buffer)->bo->handle;

    pipe_buffer_reference(&buffer, NULL);

    return TRUE;
}

struct drm_api drm_api_hooks = {
    .create_screen = radeon_create_screen,
    .create_context = radeon_create_context,
    .texture_from_shared_handle = radeon_texture_from_shared_handle,
    .shared_handle_from_texture = radeon_shared_handle_from_texture,
    .local_handle_from_texture = radeon_local_handle_from_texture,
};

struct drm_api* drm_api_create()
{
#ifdef DEBUG
    return trace_drm_create(&drm_api_hooks);
#else
    return &drm_api_hooks;
#endif
}
