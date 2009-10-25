/*
 * Copyright 2009 Maciej Cencora <m.cencora@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "r300_vbo.h"

#include "pipe/p_format.h"

#include "r300_cs.h"
#include "r300_context.h"
#include "r300_reg.h"
#include "r300_winsys.h"

static void translate_vertex_format(enum pipe_format format,
                                    unsigned nr_comps,
                                    unsigned component_size,
                                    unsigned dst_loc,
                                    uint32_t *hw_fmt1,
                                    uint32_t *hw_fmt2)
{
    uint32_t fmt1 = 0;

    switch (pf_type(format))
    {
        case PIPE_FORMAT_TYPE_FLOAT:
            assert(component_size == 4);
            fmt1 = R300_DATA_TYPE_FLOAT_1 + nr_comps - 1;
            break;
        case PIPE_FORMAT_TYPE_UNORM:
        case PIPE_FORMAT_TYPE_SNORM:
        case PIPE_FORMAT_TYPE_USCALED:
        case PIPE_FORMAT_TYPE_SSCALED:
            if (component_size == 1)
            {
                assert(nr_comps == 4);
                fmt1 = R300_DATA_TYPE_BYTE;
            }
            else if (component_size == 2)
            {
                if (nr_comps == 2)
                    fmt1 = R300_DATA_TYPE_SHORT_2;
                else if (nr_comps == 4)
                    fmt1 = R300_DATA_TYPE_SHORT_4;
                else
                    assert(0);
            }
            else
            {
                assert(0);
            }

            if (pf_type(format) == PIPE_FORMAT_TYPE_SNORM)
            {
                fmt1 |= R300_SIGNED;
            }
            else if (pf_type(format) == PIPE_FORMAT_TYPE_SSCALED)
            {
                fmt1 |= R300_SIGNED;
                fmt1 |= R300_NORMALIZE;
            }
            else if (pf_type(format) == PIPE_FORMAT_TYPE_USCALED)
            {
                fmt1 |= R300_NORMALIZE;
            }
            break;
        default:
            assert(0);
            break;
    }

    *hw_fmt1 = fmt1 | (dst_loc << R300_DST_VEC_LOC_SHIFT);
    *hw_fmt2 = (pf_swizzle_x(format) << R300_SWIZZLE_SELECT_X_SHIFT) |
               (pf_swizzle_y(format) << R300_SWIZZLE_SELECT_Y_SHIFT) |
               (pf_swizzle_z(format) << R300_SWIZZLE_SELECT_Z_SHIFT) |
               (pf_swizzle_w(format) << R300_SWIZZLE_SELECT_W_SHIFT) |
               (0xf << R300_WRITE_ENA_SHIFT);
}

static INLINE void setup_vertex_attribute(struct r300_vertex_info *vinfo,
                                          struct pipe_vertex_element *vert_elem,
                                          unsigned attr_num)
{
    uint32_t hw_fmt1, hw_fmt2;
    translate_vertex_format(vert_elem->src_format,
                            vert_elem->nr_components,
                            pf_size_x(vert_elem->src_format),
                            attr_num,
                            &hw_fmt1,
                            &hw_fmt2);

    if (attr_num % 2 == 0)
    {
        vinfo->vap_prog_stream_cntl[attr_num >> 1] = hw_fmt1;
        vinfo->vap_prog_stream_cntl_ext[attr_num >> 1] = hw_fmt2;
    }
    else
    {
        vinfo->vap_prog_stream_cntl[attr_num >> 1] |= hw_fmt1 << 16;
        vinfo->vap_prog_stream_cntl_ext[attr_num >> 1] |= hw_fmt2 << 16;
    }
}

static void finish_vertex_attribs_setup(struct r300_vertex_info *vinfo,
                                        unsigned attribs_num)
{
    uint32_t last_vec_bit = (attribs_num % 2 == 0) ? (R300_LAST_VEC << 16) : R300_LAST_VEC;

    assert(attribs_num > 0 && attribs_num <= 16);
    vinfo->vap_prog_stream_cntl[(attribs_num - 1) >> 1] |= last_vec_bit;
}

void setup_vertex_attributes(struct r300_context *r300)
{
    for (int i=0; i<r300->aos_count; i++)
    {
        struct pipe_vertex_element *vert_elem = &r300->vertex_element[i];

        setup_vertex_attribute(r300->vertex_info, vert_elem, i);
    }

    finish_vertex_attribs_setup(r300->vertex_info, r300->aos_count);
}

static void setup_vertex_array(struct r300_context *r300, struct pipe_vertex_element *element)
{
}

static void finish_vertex_arrays_setup(struct r300_context *r300)
{
}

static bool format_is_supported(enum pipe_format format, int nr_components)
{
    if (pf_layout(format) != PIPE_FORMAT_LAYOUT_RGBAZS)
        return false;

    if ((pf_size_x(format) != pf_size_y(format)) ||
        (pf_size_x(format) != pf_size_z(format)) ||
        (pf_size_x(format) != pf_size_w(format)))
        return false;

    /* Following should be supported as long as stride is 4 bytes aligned */
    if (pf_size_x(format) != 1 && nr_components != 4)
        return false;

    if (pf_size_x(format) != 2 && !(nr_components == 2 || nr_components == 4))
        return false;

    if (pf_size_x(format) == 3 || pf_size_x(format) > 4)
        return false;

    return true;
}

static INLINE int get_buffer_offset(struct r300_context *r300,
                                    unsigned int buf_nr,
                                    unsigned int elem_offset)
{
    return r300->vertex_buffer[buf_nr].buffer_offset + elem_offset;
}

/**
 */
static void setup_vertex_buffers(struct r300_context *r300)
{
    for (int i=0; i<r300->aos_count; i++)
    {
        struct pipe_vertex_element *vert_elem = &r300->vertex_element[i];
        if (!format_is_supported(vert_elem->src_format, vert_elem->nr_components))
        {
            assert(0);
            /* use translate module to convert the data */
            /*
            struct pipe_buffer *buf;
            const unsigned int max_index = r300->vertex_buffers[vert_elem->vertex_buffer_index].max_index;
            buf = pipe_buffer_create(r300->context.screen, 4, usage, vert_elem->nr_components * max_index * sizeof(float));
            */
        }

        if (get_buffer_offset(r300, vert_elem->vertex_buffer_index, vert_elem->src_offset) % 4 != 0)
        {
            /* need to align buffer */
            assert(0);
        }
        setup_vertex_array(r300, vert_elem);
    }

    finish_vertex_arrays_setup(r300);
}

void setup_index_buffer(struct r300_context *r300,
                        struct pipe_buffer* indexBuffer,
                        unsigned indexSize)
{
    assert(indexSize = 2);

    if (!r300->winsys->add_buffer(r300->winsys, indexBuffer, RADEON_GEM_DOMAIN_GTT, 0))
    {
        assert(0);
    }

    if (!r300->winsys->validate(r300->winsys))
    {
        assert(0);
    }
}

