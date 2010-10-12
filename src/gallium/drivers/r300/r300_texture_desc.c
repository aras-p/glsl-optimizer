/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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

#include "r300_texture_desc.h"

#include "r300_context.h"
#include "r300_winsys.h"

#include "util/u_format.h"

/* Returns the number of pixels that the texture should be aligned to
 * in the given dimension. */
unsigned r300_get_pixel_alignment(enum pipe_format format,
                                  unsigned num_samples,
                                  enum r300_buffer_tiling microtile,
                                  enum r300_buffer_tiling macrotile,
                                  enum r300_dim dim)
{
    static const unsigned table[2][5][3][2] =
    {
        {
    /* Macro: linear    linear    linear
       Micro: linear    tiled  square-tiled */
            {{ 32, 1}, { 8,  4}, { 0,  0}}, /*   8 bits per pixel */
            {{ 16, 1}, { 8,  2}, { 4,  4}}, /*  16 bits per pixel */
            {{  8, 1}, { 4,  2}, { 0,  0}}, /*  32 bits per pixel */
            {{  4, 1}, { 2,  2}, { 0,  0}}, /*  64 bits per pixel */
            {{  2, 1}, { 0,  0}, { 0,  0}}  /* 128 bits per pixel */
        },
        {
    /* Macro: tiled     tiled     tiled
       Micro: linear    tiled  square-tiled */
            {{256, 8}, {64, 32}, { 0,  0}}, /*   8 bits per pixel */
            {{128, 8}, {64, 16}, {32, 32}}, /*  16 bits per pixel */
            {{ 64, 8}, {32, 16}, { 0,  0}}, /*  32 bits per pixel */
            {{ 32, 8}, {16, 16}, { 0,  0}}, /*  64 bits per pixel */
            {{ 16, 8}, { 0,  0}, { 0,  0}}  /* 128 bits per pixel */
        }
    };
    static const unsigned aa_block[2] = {4, 8};
    unsigned tile = 0;
    unsigned pixsize = util_format_get_blocksize(format);

    assert(macrotile <= R300_BUFFER_TILED);
    assert(microtile <= R300_BUFFER_SQUARETILED);
    assert(pixsize <= 16);
    assert(dim <= DIM_HEIGHT);

    if (num_samples > 1) {
        /* Multisampled textures have their own alignment scheme. */
        if (pixsize == 4)
            tile = aa_block[dim];
        /* XXX FP16 AA. */
    } else {
        /* Standard alignment. */
        tile = table[macrotile][util_logbase2(pixsize)][microtile][dim];
    }

    assert(tile);
    return tile;
}

/* Return true if macrotiling should be enabled on the miplevel. */
static boolean r300_texture_macro_switch(struct r300_texture_desc *desc,
                                         unsigned level,
                                         boolean rv350_mode,
                                         enum r300_dim dim)
{
    unsigned tile, texdim;

    tile = r300_get_pixel_alignment(desc->b.b.format, desc->b.b.nr_samples,
                                    desc->microtile, R300_BUFFER_TILED, dim);
    if (dim == DIM_WIDTH) {
        texdim = u_minify(desc->width0, level);
    } else {
        texdim = u_minify(desc->height0, level);
    }

    /* See TX_FILTER1_n.MACRO_SWITCH. */
    if (rv350_mode) {
        return texdim >= tile;
    } else {
        return texdim > tile;
    }
}

/**
 * Return the stride, in bytes, of the texture image of the given texture
 * at the given level.
 */
static unsigned r300_texture_get_stride(struct r300_screen *screen,
                                        struct r300_texture_desc *desc,
                                        unsigned level)
{
    unsigned tile_width, width, stride;

    if (desc->stride_in_bytes_override)
        return desc->stride_in_bytes_override;

    /* Check the level. */
    if (level > desc->b.b.last_level) {
        SCREEN_DBG(screen, DBG_TEX, "%s: level (%u) > last_level (%u)\n",
                   __FUNCTION__, level, desc->b.b.last_level);
        return 0;
    }

    width = u_minify(desc->width0, level);

    if (util_format_is_plain(desc->b.b.format)) {
        tile_width = r300_get_pixel_alignment(desc->b.b.format,
                                              desc->b.b.nr_samples,
                                              desc->microtile,
                                              desc->macrotile[level],
                                              DIM_WIDTH);
        width = align(width, tile_width);

        stride = util_format_get_stride(desc->b.b.format, width);

        /* Some IGPs need a minimum stride of 64 bytes, hmm... */
        if (!desc->macrotile[level] &&
            (screen->caps.family == CHIP_FAMILY_RS600 ||
             screen->caps.family == CHIP_FAMILY_RS690 ||
             screen->caps.family == CHIP_FAMILY_RS740)) {
            unsigned min_stride;

            if (desc->microtile) {
                unsigned tile_height =
                        r300_get_pixel_alignment(desc->b.b.format,
                                                 desc->b.b.nr_samples,
                                                 desc->microtile,
                                                 desc->macrotile[level],
                                                 DIM_HEIGHT);

                min_stride = 64 / tile_height;
            } else {
                min_stride = 64;
            }

            return stride < min_stride ? min_stride : stride;
        }

        /* The alignment to 32 bytes is sort of implied by the layout... */
        return stride;
    } else {
        return align(util_format_get_stride(desc->b.b.format, width), 32);
    }
}

static unsigned r300_texture_get_nblocksy(struct r300_texture_desc *desc,
                                          unsigned level,
                                          boolean *out_aligned_for_cbzb)
{
    unsigned height, tile_height;

    height = u_minify(desc->height0, level);

    if (util_format_is_plain(desc->b.b.format)) {
        tile_height = r300_get_pixel_alignment(desc->b.b.format,
                                               desc->b.b.nr_samples,
                                               desc->microtile,
                                               desc->macrotile[level],
                                               DIM_HEIGHT);
        height = align(height, tile_height);

        /* This is needed for the kernel checker, unfortunately. */
        if ((desc->b.b.target != PIPE_TEXTURE_1D &&
             desc->b.b.target != PIPE_TEXTURE_2D &&
             desc->b.b.target != PIPE_TEXTURE_RECT) ||
            desc->b.b.last_level != 0) {
            height = util_next_power_of_two(height);
        }

        /* See if the CBZB clear can be used on the buffer,
         * taking the texture size into account. */
        if (out_aligned_for_cbzb) {
            if (desc->macrotile[level]) {
                /* When clearing, the layer (width*height) is horizontally split
                 * into two, and the upper and lower halves are cleared by the CB
                 * and ZB units, respectively. Therefore, the number of macrotiles
                 * in the Y direction must be even. */

                /* Align the height so that there is an even number of macrotiles.
                 * Do so for 3 or more macrotiles in the Y direction. */
                if (level == 0 && desc->b.b.last_level == 0 &&
                    (desc->b.b.target == PIPE_TEXTURE_1D ||
                     desc->b.b.target == PIPE_TEXTURE_2D ||
                     desc->b.b.target == PIPE_TEXTURE_RECT) &&
                    height >= tile_height * 3) {
                    height = align(height, tile_height * 2);
                }

                *out_aligned_for_cbzb = height % (tile_height * 2) == 0;
            } else {
                *out_aligned_for_cbzb = FALSE;
            }
        }
    }

    return util_format_get_nblocksy(desc->b.b.format, height);
}

static void r300_texture_3d_fix_mipmapping(struct r300_screen *screen,
                                           struct r300_texture_desc *desc)
{
    /* The kernels <= 2.6.34-rc4 compute the size of mipmapped 3D textures
     * incorrectly. This is a workaround to prevent CS from being rejected. */

    unsigned i, size;

    if (!screen->rws->get_value(screen->rws, R300_VID_DRM_2_3_0) &&
        desc->b.b.target == PIPE_TEXTURE_3D &&
        desc->b.b.last_level > 0) {
        size = 0;

        for (i = 0; i <= desc->b.b.last_level; i++) {
            size += desc->stride_in_bytes[i] *
                    r300_texture_get_nblocksy(desc, i, FALSE);
        }

        size *= desc->depth0;
        desc->size_in_bytes = size;
    }
}

/* Get a width in pixels from a stride in bytes. */
static unsigned stride_to_width(enum pipe_format format,
                                unsigned stride_in_bytes)
{
    return (stride_in_bytes / util_format_get_blocksize(format)) *
            util_format_get_blockwidth(format);
}

static void r300_setup_miptree(struct r300_screen *screen,
                               struct r300_texture_desc *desc,
                               boolean align_for_cbzb)
{
    struct pipe_resource *base = &desc->b.b;
    unsigned stride, size, layer_size, nblocksy, i;
    boolean rv350_mode = screen->caps.family >= CHIP_FAMILY_R350;
    boolean aligned_for_cbzb;

    desc->size_in_bytes = 0;

    SCREEN_DBG(screen, DBG_TEXALLOC,
        "r300: Making miptree for texture, format %s\n",
        util_format_short_name(base->format));

    for (i = 0; i <= base->last_level; i++) {
        /* Let's see if this miplevel can be macrotiled. */
        desc->macrotile[i] =
            (desc->macrotile[0] == R300_BUFFER_TILED &&
             r300_texture_macro_switch(desc, i, rv350_mode, DIM_WIDTH) &&
             r300_texture_macro_switch(desc, i, rv350_mode, DIM_HEIGHT)) ?
             R300_BUFFER_TILED : R300_BUFFER_LINEAR;

        stride = r300_texture_get_stride(screen, desc, i);

        /* Compute the number of blocks in Y, see if the CBZB clear can be
         * used on the texture. */
        aligned_for_cbzb = FALSE;
        if (align_for_cbzb && desc->cbzb_allowed[i])
            nblocksy = r300_texture_get_nblocksy(desc, i, &aligned_for_cbzb);
        else
            nblocksy = r300_texture_get_nblocksy(desc, i, NULL);

        layer_size = stride * nblocksy;

        if (base->nr_samples) {
            layer_size *= base->nr_samples;
        }

        if (base->target == PIPE_TEXTURE_CUBE)
            size = layer_size * 6;
        else
            size = layer_size * u_minify(desc->depth0, i);

        desc->offset_in_bytes[i] = desc->size_in_bytes;
        desc->size_in_bytes = desc->offset_in_bytes[i] + size;
        desc->layer_size_in_bytes[i] = layer_size;
        desc->stride_in_bytes[i] = stride;
        desc->stride_in_pixels[i] = stride_to_width(desc->b.b.format, stride);
        desc->cbzb_allowed[i] = desc->cbzb_allowed[i] && aligned_for_cbzb;

        SCREEN_DBG(screen, DBG_TEXALLOC, "r300: Texture miptree: Level %d "
                "(%dx%dx%d px, pitch %d bytes) %d bytes total, macrotiled %s\n",
                i, u_minify(desc->width0, i), u_minify(desc->height0, i),
                u_minify(desc->depth0, i), stride, desc->size_in_bytes,
                desc->macrotile[i] ? "TRUE" : "FALSE");
    }
}

static void r300_setup_flags(struct r300_texture_desc *desc)
{
    desc->uses_stride_addressing =
        !util_is_power_of_two(desc->b.b.width0) ||
        (desc->stride_in_bytes_override &&
         stride_to_width(desc->b.b.format,
                         desc->stride_in_bytes_override) != desc->b.b.width0);

    desc->is_npot =
        desc->uses_stride_addressing ||
        !util_is_power_of_two(desc->b.b.height0) ||
        !util_is_power_of_two(desc->b.b.depth0);
}

static void r300_setup_cbzb_flags(struct r300_screen *rscreen,
                                  struct r300_texture_desc *desc)
{
    unsigned i, bpp;
    boolean first_level_valid;

    bpp = util_format_get_blocksizebits(desc->b.b.format);

    /* 1) The texture must be point-sampled,
     * 2) The depth must be 16 or 32 bits.
     * 3) If the midpoint ZB offset is not aligned to 2048, it returns garbage
     *    with certain texture sizes. Macrotiling ensures the alignment. */
    first_level_valid = desc->b.b.nr_samples <= 1 &&
                       (bpp == 16 || bpp == 32) &&
                       desc->macrotile[0];

    if (SCREEN_DBG_ON(rscreen, DBG_NO_CBZB))
        first_level_valid = FALSE;

    for (i = 0; i <= desc->b.b.last_level; i++)
        desc->cbzb_allowed[i] = first_level_valid && desc->macrotile[i];
}

static void r300_setup_tiling(struct r300_screen *screen,
                              struct r300_texture_desc *desc)
{
    struct r300_winsys_screen *rws = screen->rws;
    enum pipe_format format = desc->b.b.format;
    boolean rv350_mode = screen->caps.family >= CHIP_FAMILY_R350;
    boolean is_zb = util_format_is_depth_or_stencil(format);
    boolean dbg_no_tiling = SCREEN_DBG_ON(screen, DBG_NO_TILING);

    if (!util_format_is_plain(format)) {
        return;
    }

    /* If height == 1, disable microtiling except for zbuffer. */
    if (!is_zb && (desc->b.b.height0 == 1 || dbg_no_tiling)) {
        return;
    }

    /* Set microtiling. */
    switch (util_format_get_blocksize(format)) {
        case 1:
        case 4:
        case 8:
            desc->microtile = R300_BUFFER_TILED;
            break;

        case 2:
            if (rws->get_value(rws, R300_VID_SQUARE_TILING_SUPPORT)) {
                desc->microtile = R300_BUFFER_SQUARETILED;
            }
            break;
    }

    if (dbg_no_tiling) {
        return;
    }

    /* Set macrotiling. */
    if (r300_texture_macro_switch(desc, 0, rv350_mode, DIM_WIDTH) &&
        r300_texture_macro_switch(desc, 0, rv350_mode, DIM_HEIGHT)) {
        desc->macrotile[0] = R300_BUFFER_TILED;
    }
}

static void r300_tex_print_info(struct r300_screen *rscreen,
                                struct r300_texture_desc *desc,
                                const char *func)
{
    fprintf(stderr,
            "r300: %s: Macro: %s, Micro: %s, Pitch: %i, Dim: %ix%ix%i, "
            "LastLevel: %i, Size: %i, Format: %s\n",
            func,
            desc->macrotile[0] ? "YES" : " NO",
            desc->microtile ? "YES" : " NO",
            desc->stride_in_pixels[0],
            desc->b.b.width0, desc->b.b.height0, desc->b.b.depth0,
            desc->b.b.last_level, desc->size_in_bytes,
            util_format_short_name(desc->b.b.format));
}

boolean r300_texture_desc_init(struct r300_screen *rscreen,
                               struct r300_texture_desc *desc,
                               const struct pipe_resource *base,
                               enum r300_buffer_tiling microtile,
                               enum r300_buffer_tiling macrotile,
                               unsigned stride_in_bytes_override,
                               unsigned max_buffer_size)
{
    desc->b.b = *base;
    desc->b.b.screen = &rscreen->screen;
    desc->stride_in_bytes_override = stride_in_bytes_override;
    desc->width0 = base->width0;
    desc->height0 = base->height0;
    desc->depth0 = base->depth0;

    r300_setup_flags(desc);

    /* Align a 3D NPOT texture to POT. */
    if (base->target == PIPE_TEXTURE_3D && desc->is_npot) {
        desc->width0 = util_next_power_of_two(desc->width0);
        desc->height0 = util_next_power_of_two(desc->height0);
        desc->depth0 = util_next_power_of_two(desc->depth0);
    }

    /* Setup tiling. */
    if (microtile == R300_BUFFER_SELECT_LAYOUT ||
        macrotile == R300_BUFFER_SELECT_LAYOUT) {
        r300_setup_tiling(rscreen, desc);
    } else {
        desc->microtile = microtile;
        desc->macrotile[0] = macrotile;
        assert(desc->b.b.last_level == 0);
    }

    r300_setup_cbzb_flags(rscreen, desc);

    /* Setup the miptree description. */
    r300_setup_miptree(rscreen, desc, TRUE);
    /* If the required buffer size is larger the given max size,
     * try again without the alignment for the CBZB clear. */
    if (max_buffer_size && desc->size_in_bytes > max_buffer_size) {
        r300_setup_miptree(rscreen, desc, FALSE);
    }

    r300_texture_3d_fix_mipmapping(rscreen, desc);

    if (max_buffer_size) {
        /* Make sure the buffer we got is large enough. */
        if (desc->size_in_bytes > max_buffer_size) {
            fprintf(stderr, "r300: texture_desc_init: The buffer is not "
                            "large enough. Got: %i, Need: %i, Info:\n",
                            max_buffer_size, desc->size_in_bytes);
            r300_tex_print_info(rscreen, desc, "texture_desc_init");
            return FALSE;
        }

        desc->buffer_size_in_bytes = max_buffer_size;
    } else {
        desc->buffer_size_in_bytes = desc->size_in_bytes;
    }

    if (SCREEN_DBG_ON(rscreen, DBG_TEX))
        r300_tex_print_info(rscreen, desc, "texture_desc_init");

    return TRUE;
}

unsigned r300_texture_get_offset(struct r300_texture_desc *desc,
                                 unsigned level, unsigned zslice,
                                 unsigned face)
{
    unsigned offset = desc->offset_in_bytes[level];

    switch (desc->b.b.target) {
        case PIPE_TEXTURE_3D:
            assert(face == 0);
            return offset + zslice * desc->layer_size_in_bytes[level];

        case PIPE_TEXTURE_CUBE:
            assert(zslice == 0);
            return offset + face * desc->layer_size_in_bytes[level];

        default:
            assert(zslice == 0 && face == 0);
            return offset;
    }
}
