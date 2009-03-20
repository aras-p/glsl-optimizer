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

/* Return the identifier behind whom the brave coders responsible for this
 * amalgamation of code, sweat, and duct tape, routinely obscure their names.
 *
 * ...I should have just put "Corbin Simpson", but I'm not that cool.
 *
 * (Or egotistical. Yet.) */
static const char* r300_get_vendor(struct pipe_screen* pscreen)
{
    return "X.Org R300 Project";
}

static const char* chip_families[] = {
    "R300",
    "R350",
    "R360",
    "RV350",
    "RV370",
    "RV380",
    "R420",
    "R423",
    "R430",
    "R480",
    "R481",
    "RV410",
    "RS400",
    "RC410",
    "RS480",
    "RS482",
    "RS600",
    "RS690",
    "RS740",
    "RV515",
    "R520",
    "RV530",
    "R580",
    "RV560",
    "RV570"
};

static const char* r300_get_name(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    return chip_families[r300screen->caps->family];
}

static int r300_get_param(struct pipe_screen* pscreen, int param)
{
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
        case PIPE_CAP_TWO_SIDED_STENCIL:
            if (r300screen->caps->is_r500) {
                return 1;
            } else {
                return 0;
            }
            return 0;
        case PIPE_CAP_GLSL:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_S3TC:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_ANISOTROPIC_FILTER:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_POINT_SPRITE:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_MAX_RENDER_TARGETS:
            return 4;
        case PIPE_CAP_OCCLUSION_QUERY:
            return 1;
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
            if (r300screen->caps->is_r500) {
                /* 13 == 4096x4096 */
                return 13;
            } else {
                /* 12 == 2048x2048 */
                return 12;
            }
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
            /* So, technically, the limit is the same as above, but some math
             * shows why this is silly. Assuming RGBA, 4cpp, we can see that
             * 4096*4096*4096 = 64.0 GiB exactly, so it's not exactly
             * practical. However, if at some point a game really wants this,
             * then we can remove or raise this limit. */
            if (r300screen->caps->is_r500) {
                /* 9 == 256x256x256 */
                return 9;
            } else {
                /* 8 == 128*128*128 */
                return 8;
            }
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            if (r300screen->caps->is_r500) {
                /* 13 == 4096x4096 */
                return 13;
            } else {
                /* 12 == 2048x2048 */
                return 12;
            }
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
            return 1;
        case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
            return 1;
        case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
            /* XXX guessing (what a terrible guess) */
            return 2;
        default:
            debug_printf("r300: Implementation error: Bad param %d\n",
                param);
            return 0;
    }
}

static float r300_get_paramf(struct pipe_screen* pscreen, int param)
{
    switch (param) {
        case PIPE_CAP_MAX_LINE_WIDTH:
        case PIPE_CAP_MAX_LINE_WIDTH_AA:
            /* XXX this is the biggest thing that will fit in that register.
            * Perhaps the actual rendering limits are less? */
            return 10922.0f;
        case PIPE_CAP_MAX_POINT_WIDTH:
        case PIPE_CAP_MAX_POINT_WIDTH_AA:
            /* XXX this is the biggest thing that will fit in that register.
             * Perhaps the actual rendering limits are less? */
            return 10922.0f;
        case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
            return 16.0f;
        case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
            return 16.0f;
        default:
            debug_printf("r300: Implementation error: Bad paramf %d\n",
                param);
            return 0.0f;
    }
}

static boolean check_tex_2d_format(enum pipe_format format, boolean is_r500)
{
    switch (format) {
        /* Colorbuffer */
        case PIPE_FORMAT_A4R4G4B4_UNORM:
        case PIPE_FORMAT_R5G6B5_UNORM:
        case PIPE_FORMAT_A1R5G5B5_UNORM:
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        /* Colorbuffer or texture */
        case PIPE_FORMAT_I8_UNORM:
        /* Z buffer */
        case PIPE_FORMAT_Z16_UNORM:
        /* Z buffer with stencil */
        case PIPE_FORMAT_Z24S8_UNORM:
            return TRUE;

        /* XXX These don't even exist
        case PIPE_FORMAT_A32R32G32B32:
        case PIPE_FORMAT_A16R16G16B16: */
        /* XXX Insert YUV422 packed VYUY and YVYU here */
        /* XXX What the deuce is UV88? (r3xx accel page 14)
            debug_printf("r300: Warning: Got unimplemented format: %s in %s\n",
                pf_name(format), __FUNCTION__);
            return FALSE; */

        /* XXX Supported yet unimplemented r5xx formats: */
        /* XXX Again, what is UV1010 this time? (r5xx accel page 148) */
        /* XXX Even more that don't exist
        case PIPE_FORMAT_A10R10G10B10_UNORM:
        case PIPE_FORMAT_A2R10G10B10_UNORM:
        case PIPE_FORMAT_I10_UNORM:
            debug_printf(
                "r300: Warning: Got unimplemented r500 format: %s in %s\n",
                pf_name(format), __FUNCTION__);
            return FALSE; */

        default:
            debug_printf("r300: Warning: Got unsupported format: %s in %s\n",
                pf_name(format), __FUNCTION__);
            break;
    }

    return FALSE;
}

/* XXX moar targets */
static boolean r300_is_format_supported(struct pipe_screen* pscreen,
                                        enum pipe_format format,
                                        enum pipe_texture_target target,
                                        unsigned tex_usage,
                                        unsigned geom_flags)
{
    switch (target) {
        case PIPE_TEXTURE_2D:
            return check_tex_2d_format(format,
                r300_screen(pscreen)->caps->is_r500);
        default:
            debug_printf("r300: Warning: Got unknown format target: %d\n",
                format);
            break;
    }

    return FALSE;
}

static struct pipe_transfer*
r300_get_tex_transfer(struct pipe_screen *screen,
                      struct pipe_texture *texture,
                      unsigned face, unsigned level, unsigned zslice,
                      enum pipe_transfer_usage usage, unsigned x, unsigned y,
                      unsigned w, unsigned h)
{
    struct r300_texture *tex = (struct r300_texture *)texture;
    struct r300_transfer *trans;
    unsigned offset;  /* in bytes */

    /* XXX Add support for these things */
    if (texture->target == PIPE_TEXTURE_CUBE) {
        debug_printf("PIPE_TEXTURE_CUBE is not yet supported.\n");
        /* offset = tex->image_offset[level][face]; */
    }
    else if (texture->target == PIPE_TEXTURE_3D) {
        debug_printf("PIPE_TEXTURE_3D is not yet supported.\n");
        /* offset = tex->image_offset[level][zslice]; */
    }
    else {
        offset = tex->offset[level];
        assert(face == 0);
        assert(zslice == 0);
    }

    trans = CALLOC_STRUCT(r300_transfer);
    if (trans) {
        pipe_texture_reference(&trans->transfer.texture, texture);
        trans->transfer.format = trans->transfer.format;
        trans->transfer.width = w;
        trans->transfer.height = h;
        trans->transfer.block = texture->block;
        trans->transfer.nblocksx = texture->nblocksx[level];
        trans->transfer.nblocksy = texture->nblocksy[level];
        trans->transfer.stride = tex->stride;
        trans->transfer.usage = usage;
        trans->offset = offset;
    }
    return &trans->transfer;
}

static void
r300_tex_transfer_destroy(struct pipe_transfer *trans)
{
   pipe_texture_reference(&trans->texture, NULL);
   FREE(trans);
}

static void* r300_transfer_map(struct pipe_screen* screen,
                              struct pipe_transfer* transfer)
{
    struct r300_texture* tex = (struct r300_texture*)transfer->texture;
    char* map;
    unsigned flags = 0;

    if (transfer->usage != PIPE_TRANSFER_WRITE) {
        flags |= PIPE_BUFFER_USAGE_CPU_READ;
    }
    if (transfer->usage != PIPE_TRANSFER_READ) {
        flags |= PIPE_BUFFER_USAGE_CPU_WRITE;
    }
    
    map = pipe_buffer_map(screen, tex->buffer, flags);

    if (!map) {
        return NULL;
    }

    return map + r300_transfer(transfer)->offset +
        transfer->y / transfer->block.height * transfer->stride +
        transfer->x / transfer->block.width * transfer->block.size;
}

static void r300_transfer_unmap(struct pipe_screen* screen,
                                struct pipe_transfer* transfer)
{
    struct r300_texture* tex = (struct r300_texture*)transfer->texture;
    pipe_buffer_unmap(screen, tex->buffer);
}

static void r300_destroy_screen(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    FREE(r300screen->caps);
    FREE(r300screen);
}

struct pipe_screen* r300_create_screen(struct r300_winsys* r300_winsys)
{
    struct r300_screen* r300screen = CALLOC_STRUCT(r300_screen);
    struct r300_capabilities* caps = CALLOC_STRUCT(r300_capabilities);

    if (!r300screen || !caps)
        return NULL;

    caps->pci_id = r300_winsys->pci_id;
    caps->num_frag_pipes = r300_winsys->gb_pipes;

    r300_parse_chipset(caps);

    r300screen->caps = caps;
    r300screen->screen.winsys = (struct pipe_winsys*)r300_winsys;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.get_tex_transfer = r300_get_tex_transfer;
    r300screen->screen.tex_transfer_destroy = r300_tex_transfer_destroy;
    r300screen->screen.transfer_map = r300_transfer_map;
    r300screen->screen.transfer_unmap = r300_transfer_unmap;

    r300_init_screen_texture_functions(&r300screen->screen);
    u_simple_screen_init(&r300screen->screen);

    return &r300screen->screen;
}
