/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_screen.h"

static const char* r300_get_vendor(struct pipe_screen* pscreen) {
    return "X.Org R300 Project";
}

static const char* r300_get_name(struct pipe_screen* pscreen) {
    struct r300_screen* r300screen = r300_screen(pscreen);

    return chip_families[r300screen->caps->family];
}

static int r300_get_param(struct pipe_screen* pscreen, int param) {
    struct r300_screen* r300screen = r300_screen(pscreen);

    switch (param) {
        /* XXX cases marked "IN THEORY" are possible on the hardware,
         * but haven't been implemented yet. */
        case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
            /* XXX I'm told this goes up to 16 */
            return 8;
        case PIPE_CAP_NPOT_TEXTURES:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_S3TC:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_TWO_SIDED_STENCIL:
            /* IN THEORY */
            /* if (r300screen->is_r500) {
             * return 1;
             * } else {
             * return 0;
             * } */
            return 0;
        case PIPE_CAP_ANISOTROPIC_FILTER:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_POINT_SPRITE:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_OCCLUSION_QUERY:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_GLSL:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
            /* 12 == 2048x2048 */
            return 12;
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
            /* XXX educated guess */
            return 8;
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            /* XXX educated guess */
            return 11;
        case PIPE_CAP_MAX_RENDER_TARGETS:
            /* XXX 4 eventually */
            return 1;
        default:
            return 0;
    }
}

static float r300_get_paramf(struct pipe_screen* pscreen, int param) {
    switch (param) {
        case PIPE_CAP_MAX_LINE_WIDTH:
        case PIPE_CAP_MAX_LINE_WIDTH_AA:
            /* XXX look this up, lazy ass! */
            return 0.0;
        case PIPE_CAP_MAX_POINT_WIDTH:
        case PIPE_CAP_MAX_POINT_WIDTH_AA:
            /* XXX see above */
            return 255.0;
        case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
            return 16.0;
        case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
            return 16.0;
        default:
            return 0.0;
    }
}

static boolean r300_is_format_supported(struct pipe_screen* pscreen,
                                        enum pipe_format format,
                                        enum pipe_texture_target target,
                                        unsigned tex_usage,
                                        unsigned geom_flags)
{
    return FALSE;
}

static void* r300_surface_map(struct pipe_screen* screen,
                              struct pipe_surface* surface,
                              unsigned flags)
{
    char* map = pipe_buffer_map(screen, surface->buffer, flags);

    if (!map) {
        return NULL;
    }

    return map + surface->offset;
}

static void r300_surface_unmap(struct pipe_screen* screen,
                               struct pipe_surface* surface)
{
    pipe_buffer_unmap(screen, surface->buffer);
}

static void r300_destroy_screen(struct pipe_screen* pscreen) {
    struct r300_screen* r300screen = r300_screen(pscreen);

    FREE(r300screen->caps);
    FREE(r300screen);
}

struct pipe_screen* r300_create_screen(struct pipe_winsys* winsys, uint32_t pci_id)
{
    struct r300_screen* r300screen = CALLOC_STRUCT(r300_screen);
    struct r300_capabilities* caps = CALLOC_STRUCT(r300_capabilities);

    if (!r300screen || !caps)
        return NULL;

    r300_parse_chipset(pci_id, caps);

    r300screen->caps = caps;
    r300screen->screen.winsys = winsys;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.surface_map = r300_surface_map;
    r300screen->screen.surface_unmap = r300_surface_unmap;

    return &r300screen->screen;
}
