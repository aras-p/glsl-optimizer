/* 
 * Copyright © 2008 Jérôme Glisse
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
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#include <stdio.h>
#include "pipe/p_screen.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "utils.h"
#include "xf86drm.h"
#include "drm.h"
#include "dri_util.h"
#include "radeon_screen.h"
#include "radeon_context.h"
#include "radeon_buffer.h"
#include "radeon_bo.h"
#include "radeon_bo_gem.h"
#include "radeon_drm.h"

extern const struct dri_extension radeon_card_extensions[];

static const __DRIextension *radeon_screen_extensions[] = {
    &driReadDrawableExtension,
    &driCopySubBufferExtension.base,
    &driSwapControlExtension.base,
    &driFrameTrackingExtension.base,
    &driMediaStreamCounterExtension.base,
    NULL
};

static __DRIconfig **radeon_fill_in_modes(unsigned pixel_bits,
                                       unsigned depth_bits,
                                       GLboolean have_back_buffer)
{
    __DRIconfig **configs;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    unsigned num_modes;
    GLenum fb_format;
    GLenum fb_type;
    uint8_t depth_bits_array[3];
    uint8_t stencil_bits_array[3];
    uint8_t msaa_samples_array[1];
    /* TODO: pageflipping ? */
    static const GLenum back_buffer_modes[] = {
        GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
    };

    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = 0;
    if (depth_bits == 24) {
        stencil_bits_array[2] = 8;
        num_modes = 3;
    }

    depth_bits_array[0] = 0;
    depth_bits_array[1] = depth_bits;
    depth_bits_array[2] = depth_bits;
    depth_buffer_factor = (depth_bits == 24) ? 3 : 2;

    back_buffer_factor = (have_back_buffer) ? 3 : 1;

    msaa_samples_array[0] = 0;

    if (pixel_bits == 16) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    } else {
        fb_format = GL_BGRA;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    configs = (__DRIconfig **)driCreateConfigs(fb_format,
                                               fb_type,
                                               depth_bits_array,
                                               stencil_bits_array,
                                               depth_buffer_factor,
                                               back_buffer_modes,
                                               back_buffer_factor,
                                               msaa_samples_array,
                                               1);
    if (configs == NULL) {
        fprintf(stderr, "[%s:%u] Error creating FBConfig!\n",
                __FILE__, __LINE__);
        return NULL;
    }
    return configs;
}

static void radeon_screen_destroy(__DRIscreenPrivate *dri_screen)
{
     struct radeon_screen *radeon_screen = (struct radeon_screen*)dri_screen->private;

     radeon_bo_manager_gem_dtor(radeon_screen->bom); 
     dri_screen = NULL;
     free(radeon_screen);
}

static const __DRIconfig **radeon_screen_init(__DRIscreenPrivate *dri_screen)
{
    struct radeon_screen *radeon_screen;

    /* Calling driInitExtensions here, with a NULL context pointer,
     * does not actually enable the extensions.  It just makes sure
     * that all the dispatch offsets for all the extensions that
     * *might* be enables are known.  This is needed because the
     * dispatch offsets need to be known when _mesa_context_create is
     * called, but we can't enable the extensions until we have a
     * context pointer.
     *
     * Hello chicken.  Hello egg.  How are you two today?
     */
    driInitExtensions(NULL, radeon_card_extensions, GL_FALSE);

    radeon_screen = calloc(1, sizeof(struct radeon_screen));
    if (radeon_screen == NULL) {
        fprintf(stderr, "\nERROR!  Allocating private area failed\n");
        return NULL;
    }
    dri_screen->private = (void*)radeon_screen;
    dri_screen->extensions = radeon_screen_extensions;
    radeon_screen->dri_screen = dri_screen;

    radeon_screen->bom = radeon_bo_manager_gem_ctor(dri_screen->fd);
    if (radeon_screen->bom == NULL) {
        radeon_screen_destroy(dri_screen);
        return NULL;
    }

    return driConcatConfigs(radeon_fill_in_modes(16, 16, 1),
                            radeon_fill_in_modes(32, 24, 1));
}

static boolean radeon_buffer_create(__DRIscreenPrivate *dri_screen,
                                 __DRIdrawablePrivate *dri_drawable,
                                 const __GLcontextModes *visual,
                                 boolean is_pixmap)
{
    if (is_pixmap) {
        /* TODO: implement ? */
        return GL_FALSE;
    } else {
        enum pipe_format color_format, depth_format, stencil_format;
        struct radeon_framebuffer *radeon_fb;

        radeon_fb = calloc(1, sizeof(struct radeon_framebuffer));
        if (radeon_fb == NULL) {
            return GL_FALSE;
        }

        switch (visual->redBits) {
        case 5:
            color_format = PIPE_FORMAT_R5G6B5_UNORM;
            break;
        default:
            color_format = PIPE_FORMAT_A8R8G8B8_UNORM;
            break;
        }

        switch (visual->depthBits) {
        case 24:
            depth_format = PIPE_FORMAT_S8Z24_UNORM;
            break;
        case 16:
            depth_format = PIPE_FORMAT_Z16_UNORM;
            break;
        default:
            depth_format = PIPE_FORMAT_NONE;
            break;
        }

        switch (visual->stencilBits) {
        case 8:
            /* force depth format */
            depth_format = PIPE_FORMAT_S8Z24_UNORM;
            stencil_format = PIPE_FORMAT_S8Z24_UNORM;
            break;
        default:
            stencil_format = PIPE_FORMAT_NONE;
            break;
        }

        radeon_fb->st_framebuffer = st_create_framebuffer(visual,
                                                       color_format,
                                                       depth_format,
                                                       stencil_format,
                                                       dri_drawable->w,
                                                       dri_drawable->h,
                                                       (void*)radeon_fb);
        if (radeon_fb->st_framebuffer == NULL) {
            free(radeon_fb);
            return GL_FALSE;
        }
        dri_drawable->driverPrivate = (void *) radeon_fb;

        radeon_fb->attachments = (1 << __DRI_BUFFER_FRONT_LEFT);
        if (visual->doubleBufferMode) {
            radeon_fb->attachments |= (1 << __DRI_BUFFER_BACK_LEFT);
        }
        if (visual->depthBits || visual->stencilBits) {
            radeon_fb->attachments |= (1 << __DRI_BUFFER_DEPTH);
        }

        return GL_TRUE;
    }
}

static void radeon_buffer_destroy(__DRIdrawablePrivate * dri_drawable)
{
   struct radeon_framebuffer *radeon_fb;
   
   radeon_fb = dri_drawable->driverPrivate;
   assert(radeon_fb->st_framebuffer);
   st_unreference_framebuffer(radeon_fb->st_framebuffer);
   free(radeon_fb);
}

static void radeon_swap_buffers(__DRIdrawablePrivate *dri_drawable)
{
    struct radeon_framebuffer *radeon_fb;
    struct pipe_surface *back_surf = NULL;

    radeon_fb = dri_drawable->driverPrivate;
    assert(radeon_fb);
    assert(radeon_fb->st_framebuffer);

    st_get_framebuffer_surface(radeon_fb->st_framebuffer,
                               ST_SURFACE_BACK_LEFT,
                               &back_surf);
    if (back_surf) {
        st_notify_swapbuffers(radeon_fb->st_framebuffer);
        /* TODO: do we want to do anythings ? */
        st_notify_swapbuffers_complete(radeon_fb->st_framebuffer);
    }
}

/**
 * Called via glXCopySubBufferMESA() to copy a subrect of the back
 * buffer to the front buffer/screen.
 */
static void radeon_copy_sub_buffer(__DRIdrawablePrivate *dri_drawable,
                         int x, int y, int w, int h)
{
    /* TODO: ... */
}

const struct __DriverAPIRec driDriverAPI = {
    .InitScreen           = NULL,
    .DestroyScreen        = radeon_screen_destroy,
    .CreateContext        = radeon_context_create,
    .DestroyContext       = radeon_context_destroy,
    .CreateBuffer         = radeon_buffer_create,
    .DestroyBuffer        = radeon_buffer_destroy,
    .SwapBuffers          = radeon_swap_buffers,
    .MakeCurrent          = radeon_context_bind,
    .UnbindContext        = radeon_context_unbind,
    .CopySubBuffer        = radeon_copy_sub_buffer,
    .InitScreen2          = radeon_screen_init,
};
