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

/* r300_vbo: Various helpers for emitting vertex buffers. Needs cleanup,
 * refactoring, etc. */

#include "r300_vbo.h"

#include "pipe/p_format.h"

#include "r300_cs.h"
#include "r300_context.h"
#include "r300_state_inlines.h"
#include "r300_reg.h"
#include "r300_winsys.h"

static INLINE void setup_vertex_attribute(struct r300_vertex_info *vinfo,
                                          struct pipe_vertex_element *vert_elem,
                                          unsigned attr_num)
{
    uint16_t hw_fmt1, hw_fmt2;

    hw_fmt1 = r300_translate_vertex_data_type(vert_elem->src_format) |
        (attr_num << R300_DST_VEC_LOC_SHIFT);
    hw_fmt2 = r300_translate_vertex_data_swizzle(vert_elem->src_format);

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
    uint32_t last_vec_bit = (attribs_num % 2 == 0) ?
        (R300_LAST_VEC << 16) : R300_LAST_VEC;

    assert(attribs_num > 0 && attribs_num <= 16);
    vinfo->vap_prog_stream_cntl[(attribs_num - 1) >> 1] |= last_vec_bit;
}

void setup_vertex_attributes(struct r300_context *r300)
{
    struct pipe_vertex_element *vert_elem;
    int i;

    for (i = 0; i < r300->vertex_element_count; i++) {
        vert_elem = &r300->vertex_element[i];
        setup_vertex_attribute(r300->vertex_info, vert_elem, i);
    }

    finish_vertex_attribs_setup(r300->vertex_info,
        r300->vertex_element_count);
}

static INLINE int get_buffer_offset(struct r300_context *r300,
                                    unsigned int buf_nr,
                                    unsigned int elem_offset)
{
    return r300->vertex_buffer[buf_nr].buffer_offset + elem_offset;
}
#if 0
/* XXX not called at all */
static void setup_vertex_buffers(struct r300_context *r300)
{
    struct pipe_vertex_element *vert_elem;
    int i;

    for (i = 0; i < r300->aos_count; i++)
    {
        vert_elem = &r300->vertex_element[i];
            /* XXX use translate module to convert the data */
        if (!format_is_supported(vert_elem->src_format,
                                 vert_elem->nr_components)) {
            assert(0);
            /*
            struct pipe_buffer *buf;
            const unsigned int max_index = r300->vertex_buffers[vert_elem->vertex_buffer_index].max_index;
            buf = pipe_buffer_create(r300->context.screen, 4, usage, vert_elem->nr_components * max_index * sizeof(float));
            */
        }

        if (get_buffer_offset(r300,
                              vert_elem->vertex_buffer_index,
                              vert_elem->src_offset) % 4) {
            /* XXX need to align buffer */
            assert(0);
        }
    }
}
#endif
/* XXX these shouldn't be asserts since we can work around bad indexbufs */
void setup_index_buffer(struct r300_context *r300,
                        struct pipe_buffer* indexBuffer,
                        unsigned indexSize)
{
    if (!r300->winsys->add_buffer(r300->winsys, indexBuffer,
                                  RADEON_GEM_DOMAIN_GTT, 0)) {
        assert(0);
    }

    if (!r300->winsys->validate(r300->winsys)) {
        assert(0);
    }
}
