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

#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_simple_screen.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_texture.h"

#include "radeon_winsys.h"
#include "r300_winsys.h"

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
        case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
        case PIPE_CAP_MAX_COMBINED_SAMPLERS:
            /* XXX I'm told this goes up to 16 */
            return 8;
        case PIPE_CAP_NPOT_TEXTURES:
            /* XXX enable now to get GL2.1 API,
             * figure out later how to emulate this */
            return 1;
        case PIPE_CAP_TWO_SIDED_STENCIL:
            if (r300screen->caps->is_r500) {
                return 1;
            } else {
                return 0;
            }
        case PIPE_CAP_GLSL:
            /* I'll be frank. This is a lie.
             *
             * We don't truly support GLSL on any of this driver's chipsets.
             * To be fair, no chipset supports the full GLSL specification
             * to the best of our knowledge, but some of the less esoteric
             * features are still missing here.
             *
             * Rather than cripple ourselves intentionally, I'm going to set
             * this flag, and as Gallium's interface continues to change, I
             * hope that this single monolithic GLSL enable can slowly get
             * split down into many different pieces and the state tracker
             * will handle fallbacks transparently, like it should.
             *
             * ~ C.
             */
            return 1;
        case PIPE_CAP_DUAL_SOURCE_BLEND:
            return 0;
        case PIPE_CAP_ANISOTROPIC_FILTER:
            return 1;
        case PIPE_CAP_POINT_SPRITE:
            return 1;
        case PIPE_CAP_MAX_RENDER_TARGETS:
            return 4;
        case PIPE_CAP_OCCLUSION_QUERY:
            return 1;
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
            return 1;
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            if (r300screen->caps->is_r500) {
                /* 13 == 4096 */
                return 13;
            } else {
                /* 12 == 2048 */
                return 12;
            }
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
            return 1;
        case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
            return 1;
        case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
            return 0;
        case PIPE_CAP_TGSI_CONT_SUPPORTED:
            return 0;
        case PIPE_CAP_BLEND_EQUATION_SEPARATE:
            return 1;
        case PIPE_CAP_SM3:
            if (r300screen->caps->is_r500) {
                return 1;
            } else {
                return 0;
            }
        case PIPE_CAP_MAX_CONST_BUFFERS:
            return 1;
        case PIPE_CAP_MAX_CONST_BUFFER_SIZE:
            return 256;
        case PIPE_CAP_INDEP_BLEND_ENABLE:
            return 0;
        case PIPE_CAP_INDEP_BLEND_FUNC:
            return 0;
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
	    return 1;
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
            return 0;
        default:
            debug_printf("r300: Implementation error: Bad param %d\n",
                param);
            return 0;
    }
}

static float r300_get_paramf(struct pipe_screen* pscreen, int param)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    switch (param) {
        case PIPE_CAP_MAX_LINE_WIDTH:
        case PIPE_CAP_MAX_LINE_WIDTH_AA:
        case PIPE_CAP_MAX_POINT_WIDTH:
        case PIPE_CAP_MAX_POINT_WIDTH_AA:
            /* The maximum dimensions of the colorbuffer are our practical
             * rendering limits. 2048 pixels should be enough for anybody. */
            if (r300screen->caps->is_r500) {
                return 4096.0f;
            } else if (r300screen->caps->is_r400) {
                return 4021.0f;
            } else {
                return 2560.0f;
            }
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

static boolean r300_is_format_supported(struct pipe_screen* screen,
                                        enum pipe_format format,
                                        enum pipe_texture_target target,
                                        unsigned usage,
                                        unsigned geom_flags)
{
    uint32_t retval = 0;
    boolean is_r500 = r300_screen(screen)->caps->is_r500;
    boolean is_z24 = format == PIPE_FORMAT_X8Z24_UNORM ||
                     format == PIPE_FORMAT_S8Z24_UNORM;
    boolean is_color2101010 = format == PIPE_FORMAT_R10G10B10A2_UNORM;

    if (target >= PIPE_MAX_TEXTURE_TYPES) {
        debug_printf("r300: Implementation error: Received bogus texture "
            "target %d in %s\n", target, __FUNCTION__);
        return FALSE;
    }

    /* Check sampler format support. */
    if ((usage & PIPE_TEXTURE_USAGE_SAMPLER) &&
        /* Z24 cannot be sampled from on non-r5xx. */
        (is_r500 || !is_z24) &&
        r300_is_sampler_format_supported(format)) {
        retval |= PIPE_TEXTURE_USAGE_SAMPLER;
    }

    /* Check colorbuffer format support. */
    if ((usage & (PIPE_TEXTURE_USAGE_RENDER_TARGET |
                  PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                  PIPE_TEXTURE_USAGE_PRIMARY)) &&
        /* 2101010 cannot be rendered to on non-r5xx. */
        (is_r500 || !is_color2101010) &&
        r300_is_colorbuffer_format_supported(format)) {
        retval |= usage &
            (PIPE_TEXTURE_USAGE_RENDER_TARGET |
             PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
             PIPE_TEXTURE_USAGE_PRIMARY);
    }

    /* Check depth-stencil format support. */
    if (usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL &&
        r300_is_zs_format_supported(format)) {
        retval |= PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
    }

    return retval == usage;
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
    struct r300_screen *rscreen = r300_screen(screen);
    unsigned offset;

    offset = r300_texture_get_offset(tex, level, zslice, face);  /* in bytes */

    trans = CALLOC_STRUCT(r300_transfer);
    if (trans) {
        pipe_texture_reference(&trans->transfer.texture, texture);
        trans->transfer.x = x;
        trans->transfer.y = y;
        trans->transfer.width = w;
        trans->transfer.height = h;
        trans->transfer.stride = r300_texture_get_stride(rscreen, tex, level);
        trans->transfer.usage = usage;
        trans->transfer.zslice = zslice;
        trans->transfer.face = face;

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
    enum pipe_format format = tex->tex.format;

    map = pipe_buffer_map(screen, tex->buffer,
                          pipe_transfer_buffer_flags(transfer));

    if (!map) {
        return NULL;
    }

    return map + r300_transfer(transfer)->offset +
        transfer->y / util_format_get_blockheight(format) * transfer->stride +
        transfer->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
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

struct pipe_screen* r300_create_screen(struct radeon_winsys* radeon_winsys)
{
    struct r300_screen* r300screen = CALLOC_STRUCT(r300_screen);
    struct r300_capabilities* caps = CALLOC_STRUCT(r300_capabilities);

    if (!r300screen || !caps) {
        FREE(r300screen);
        FREE(caps);
        return NULL;
    }

    caps->pci_id = radeon_winsys->pci_id;
    caps->num_frag_pipes = radeon_winsys->gb_pipes;
    caps->num_z_pipes = radeon_winsys->z_pipes;

    r300_init_debug(r300screen);
    r300_parse_chipset(caps);

    r300screen->caps = caps;
    r300screen->radeon_winsys = radeon_winsys;
    r300screen->screen.winsys = (struct pipe_winsys*)radeon_winsys;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.context_create = r300_create_context;
    r300screen->screen.get_tex_transfer = r300_get_tex_transfer;
    r300screen->screen.tex_transfer_destroy = r300_tex_transfer_destroy;
    r300screen->screen.transfer_map = r300_transfer_map;
    r300screen->screen.transfer_unmap = r300_transfer_unmap;

    r300_init_screen_texture_functions(&r300screen->screen);
    u_simple_screen_init(&r300screen->screen);

    return &r300screen->screen;
}
