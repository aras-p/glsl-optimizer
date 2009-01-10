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
#include "dri_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "amd_screen.h"
#include "amd_context.h"
#include "amd_buffer.h"
#include "amd_winsys_softpipe.h"

#define need_GL_ARB_fragment_program
#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_shader_objects
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_vertex_shader
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_compiled_vertex_array
#define need_GL_EXT_fog_coord
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1
#include "extension_helper.h"

/**
 * Extension strings exported by the amd driver.
 */
const struct dri_extension amd_card_extensions[] = {
   {"GL_ARB_multitexture", NULL},
   {"GL_ARB_texture_border_clamp", NULL},
   {"GL_ARB_texture_rectangle", NULL},
   {"GL_ARB_pixel_buffer_object", NULL},
   {"GL_ARB_point_parameters", GL_ARB_point_parameters_functions},
   {"GL_ARB_shader_objects", GL_ARB_shader_objects_functions},
   {"GL_ARB_shading_language_100", GL_VERSION_2_0_functions},
   {"GL_ARB_shading_language_120", GL_VERSION_2_1_functions},
   {"GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions},
   {"GL_ARB_vertex_program", GL_ARB_vertex_program_functions},
   {"GL_ARB_vertex_shader", GL_ARB_vertex_shader_functions},
   {"GL_EXT_blend_color", GL_EXT_blend_color_functions},
   {"GL_EXT_blend_equation_separate", GL_EXT_blend_equation_separate_functions},
   {"GL_EXT_blend_func_separate", GL_EXT_blend_func_separate_functions},
   {"GL_EXT_blend_minmax", GL_EXT_blend_minmax_functions},
   {"GL_EXT_blend_subtract", NULL},
   {"GL_EXT_compiled_vertex_array", GL_EXT_compiled_vertex_array_functions},
   {"GL_EXT_cull_vertex", GL_EXT_cull_vertex_functions},
   {"GL_EXT_fog_coord", GL_EXT_fog_coord_functions},
   {"GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions},
   {"GL_EXT_multi_draw_arrays", GL_EXT_multi_draw_arrays_functions},
   {"GL_EXT_packed_depth_stencil", NULL},
   {"GL_EXT_pixel_buffer_object", NULL},
   {"GL_EXT_secondary_color", GL_EXT_secondary_color_functions},
   {"GL_EXT_stencil_wrap", NULL},
   {NULL, NULL}
};

static void amd_update_renderbuffers(__DRIcontext *dri_context,
                                     __DRIdrawable *dri_drawable)
{
    struct amd_framebuffer *amd_fb;
    struct amd_context *amd_context;
    unsigned attachments[10];
    __DRIbuffer *buffers;
    __DRIscreen *screen;
    int i, count;

    amd_context = dri_context->driverPrivate;
    screen = dri_drawable->driScreenPriv;
    amd_fb = dri_drawable->driverPrivate;
    for (count = 0, i = 0; count < 6; count++) {
        if (amd_fb->attachments & (1 << count)) {
            attachments[i++] = count;
        }
    }

    buffers = (*screen->dri2.loader->getBuffers)(dri_drawable,
                                                 &dri_drawable->w,
                                                 &dri_drawable->h,
                                                 attachments,
                                                 i,
                                                 &count,
                                                 dri_drawable->loaderPrivate);
    if (buffers == NULL) {
        return;
    }

    /* set one cliprect to cover the whole dri_drawable */
    dri_drawable->x = 0;
    dri_drawable->y = 0;
    dri_drawable->backX = 0;
    dri_drawable->backY = 0;
    dri_drawable->numClipRects = 1;
    dri_drawable->pClipRects[0].x1 = 0;
    dri_drawable->pClipRects[0].y1 = 0;
    dri_drawable->pClipRects[0].x2 = dri_drawable->w;
    dri_drawable->pClipRects[0].y2 = dri_drawable->h;
    dri_drawable->numBackClipRects = 1;
    dri_drawable->pBackClipRects[0].x1 = 0;
    dri_drawable->pBackClipRects[0].y1 = 0;
    dri_drawable->pBackClipRects[0].x2 = dri_drawable->w;
    dri_drawable->pBackClipRects[0].y2 = dri_drawable->h;

    for (i = 0; i < count; i++) {
        struct pipe_surface *ps;
        enum pipe_format format = 0;
        int index = 0;

        switch (buffers[i].attachment) {
        case __DRI_BUFFER_FRONT_LEFT:
            index = ST_SURFACE_FRONT_LEFT;
            switch (buffers[i].cpp) {
            case 4:
                format = PIPE_FORMAT_A8R8G8B8_UNORM;
                break;
            case 2:
                format = PIPE_FORMAT_R5G6B5_UNORM;
                break;
            default:
                /* FIXME: error */
                return;
            }
            break;
        case __DRI_BUFFER_BACK_LEFT:
            index = ST_SURFACE_BACK_LEFT;
            switch (buffers[i].cpp) {
            case 4:
                format = PIPE_FORMAT_A8R8G8B8_UNORM;
                break;
            case 2:
                format = PIPE_FORMAT_R5G6B5_UNORM;
                break;
            default:
                /* FIXME: error */
                return;
            }
            break;
        case __DRI_BUFFER_STENCIL:
        case __DRI_BUFFER_DEPTH:
            index = ST_SURFACE_DEPTH;
            switch (buffers[i].cpp) {
            case 4:
                format = PIPE_FORMAT_Z24S8_UNORM;
                break;
            case 2:
                format = PIPE_FORMAT_Z16_UNORM;
                break;
            default:
                /* FIXME: error */
                return;
            }
            break;
        case __DRI_BUFFER_ACCUM:
        default:
            fprintf(stderr,
                    "unhandled buffer attach event, attacment type %d\n",
                    buffers[i].attachment);
            return;
        }

        ps = amd_surface_from_handle(amd_context,
                                     buffers[i].name,
                                     format,
                                     dri_drawable->w,
                                     dri_drawable->h,
                                     buffers[i].pitch);
        assert(ps);
        st_set_framebuffer_surface(amd_fb->st_framebuffer, index, ps);
    }
    st_resize_framebuffer(amd_fb->st_framebuffer,
                          dri_drawable->w,
                          dri_drawable->h);
}

GLboolean amd_context_create(const __GLcontextModes *visual,
                             __DRIcontextPrivate *dri_context,
                             void *shared_context)
{
    __DRIscreenPrivate *dri_screen;
    struct amd_context *amd_context;
    struct amd_screen *amd_screen;
    struct pipe_context *pipe;
    struct st_context *shared_st_context = NULL;

    dri_context->driverPrivate = NULL;
    amd_context = calloc(1, sizeof(struct amd_context));
    if (amd_context == NULL) {
        return GL_FALSE;
    }

    if (shared_context) {
        shared_st_context = ((struct amd_context*)shared_context)->st_context;
    }

    dri_screen = dri_context->driScreenPriv;
    amd_screen = dri_screen->private;
    amd_context->dri_screen = dri_screen;
    amd_context->amd_screen = amd_screen;
    amd_context->drm_fd = dri_screen->fd;

    amd_context->pipe_winsys = amd_pipe_winsys(amd_screen);
    if (amd_context->pipe_winsys == NULL) {
        free(amd_context);
        return GL_FALSE;
    }

    if (GL_TRUE) {
        amd_context->pipe_screen = r300_create_screen(amd_context->pipe_winsys,
                                                      0x0);
        pipe = r300_create_context(amd_context->pipe_screen,
                                   amd_context->pipe_winsys,
                                   (struct amd_pipe_winsys*)amd_context->pipe_winsys);
    } else {
        pipe = amd_create_softpipe(amd_context);
    }
    amd_context->st_context = st_create_context(pipe, visual,
                                                shared_st_context);
    driInitExtensions(amd_context->st_context->ctx,
                      amd_card_extensions, GL_TRUE);
    dri_context->driverPrivate = amd_context;
    return GL_TRUE;
}

void amd_context_destroy(__DRIcontextPrivate *dri_context)
{
    struct amd_context *amd_context;

    amd_context = dri_context->driverPrivate;
    st_finish(amd_context->st_context);
    st_destroy_context(amd_context->st_context);
    free(amd_context);
}

GLboolean amd_context_bind(__DRIcontextPrivate *dri_context,
                           __DRIdrawablePrivate *dri_drawable,
                           __DRIdrawablePrivate *dri_readable)
{
    struct amd_framebuffer *drawable;
    struct amd_framebuffer *readable;
    struct amd_context *amd_context;

    if (dri_context == NULL) {
        st_make_current(NULL, NULL, NULL);
        return GL_TRUE;
    }

    amd_context = dri_context->driverPrivate;
    drawable = dri_drawable->driverPrivate;
    readable = dri_readable->driverPrivate;
    st_make_current(amd_context->st_context,
                    drawable->st_framebuffer,
                    readable->st_framebuffer);

    amd_update_renderbuffers(dri_context, dri_drawable);
    if (dri_drawable != dri_readable) {
        amd_update_renderbuffers(dri_context, dri_readable);
    }
    return GL_TRUE;
}

GLboolean amd_context_unbind(__DRIcontextPrivate *dri_context)
{
    struct amd_context *amd_context;

    amd_context = dri_context->driverPrivate;
    st_flush(amd_context->st_context, PIPE_FLUSH_RENDER_CACHE, NULL);
    return GL_TRUE;
}
