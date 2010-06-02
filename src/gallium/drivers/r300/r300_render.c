/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

/* r300_render: Vertex and index buffer primitive emission. Contains both
 * HW TCL fastpath rendering, and SW TCL Draw-assisted rendering. */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"

#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include "util/u_prim.h"

#include "r300_cs.h"
#include "r300_context.h"
#include "r300_screen_buffer.h"
#include "r300_emit.h"
#include "r300_reg.h"
#include "r300_state_derived.h"

static uint32_t r300_translate_primitive(unsigned prim)
{
    switch (prim) {
        case PIPE_PRIM_POINTS:
            return R300_VAP_VF_CNTL__PRIM_POINTS;
        case PIPE_PRIM_LINES:
            return R300_VAP_VF_CNTL__PRIM_LINES;
        case PIPE_PRIM_LINE_LOOP:
            return R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
        case PIPE_PRIM_LINE_STRIP:
            return R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
        case PIPE_PRIM_TRIANGLES:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLES;
        case PIPE_PRIM_TRIANGLE_STRIP:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
        case PIPE_PRIM_TRIANGLE_FAN:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
        case PIPE_PRIM_QUADS:
            return R300_VAP_VF_CNTL__PRIM_QUADS;
        case PIPE_PRIM_QUAD_STRIP:
            return R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
        case PIPE_PRIM_POLYGON:
            return R300_VAP_VF_CNTL__PRIM_POLYGON;
        default:
            return 0;
    }
}

static uint32_t r300_provoking_vertex_fixes(struct r300_context *r300,
                                            unsigned mode)
{
    struct r300_rs_state* rs = (struct r300_rs_state*)r300->rs_state.state;
    uint32_t color_control = rs->color_control;

    /* By default (see r300_state.c:r300_create_rs_state) color_control is
     * initialized to provoking the first vertex.
     *
     * Triangle fans must be reduced to the second vertex, not the first, in
     * Gallium flatshade-first mode, as per the GL spec.
     * (http://www.opengl.org/registry/specs/ARB/provoking_vertex.txt)
     *
     * Quads never provoke correctly in flatshade-first mode. The first
     * vertex is never considered as provoking, so only the second, third,
     * and fourth vertices can be selected, and both "third" and "last" modes
     * select the fourth vertex. This is probably due to D3D lacking quads.
     *
     * Similarly, polygons reduce to the first, not the last, vertex, when in
     * "last" mode, and all other modes start from the second vertex.
     *
     * ~ C.
     */

    if (rs->rs.flatshade_first) {
        switch (mode) {
            case PIPE_PRIM_TRIANGLE_FAN:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_SECOND;
                break;
            case PIPE_PRIM_QUADS:
            case PIPE_PRIM_QUAD_STRIP:
            case PIPE_PRIM_POLYGON:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
                break;
            default:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_FIRST;
                break;
        }
    } else {
        color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
    }

    return color_control;
}

static void r500_emit_index_offset(struct r300_context *r300, int index_bias)
{
    CS_LOCALS(r300);

    if (r300->screen->caps.is_r500 &&
        r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0)) {
        BEGIN_CS(2);
        OUT_CS_REG(R500_VAP_INDEX_OFFSET,
                   (index_bias & 0xFFFFFF) | (index_bias < 0 ? 1<<24 : 0));
        END_CS;
    } else {
        if (index_bias) {
            fprintf(stderr, "r300: Non-zero index bias is unsupported "
                            "on this hardware.\n");
            assert(0);
        }
    }
}

enum r300_prepare_flags {
    PREP_FIRST_DRAW     = (1 << 0),
    PREP_VALIDATE_VBOS  = (1 << 1),
    PREP_EMIT_AOS       = (1 << 2),
    PREP_EMIT_AOS_SWTCL = (1 << 3),
    PREP_INDEXED        = (1 << 4)
};

/* Check if the requested number of dwords is available in the CS and
 * if not, flush. Then validate buffers and emit dirty state.
 * Return TRUE if flush occured. */
static void r300_prepare_for_rendering(struct r300_context *r300,
                                       enum r300_prepare_flags flags,
                                       struct pipe_resource *index_buffer,
                                       unsigned cs_dwords,
                                       unsigned aos_offset,
                                       int index_bias,
                                       unsigned *end_cs_dwords)
{
    boolean flushed = FALSE;
    boolean first_draw = flags & PREP_FIRST_DRAW;
    boolean emit_aos = flags & PREP_EMIT_AOS;
    boolean emit_aos_swtcl = flags & PREP_EMIT_AOS_SWTCL;
    unsigned end_dwords = 0;

    /* Add dirty state, index offset, and AOS. */
    if (first_draw) {
        cs_dwords += r300_get_num_dirty_dwords(r300);

        if (r300->screen->caps.is_r500)
            cs_dwords += 2; /* emit_index_offset */

        if (emit_aos)
            cs_dwords += 55; /* emit_aos */

        if (emit_aos_swtcl)
            cs_dwords += 7; /* emit_aos_swtcl */
    }

    /* Emitted in flush. */
    end_dwords += 26; /* emit_query_end */

    cs_dwords += end_dwords;

    /* Reserve requested CS space. */
    if (!r300_check_cs(r300, cs_dwords)) {
        r300->context.flush(&r300->context, 0, NULL);
        flushed = TRUE;
    }

    /* Validate buffers and emit dirty state if needed. */
    if (first_draw || flushed) {
        r300_emit_buffer_validate(r300, flags & PREP_VALIDATE_VBOS, index_buffer);
        r300_emit_dirty_state(r300);
        r500_emit_index_offset(r300, index_bias);
        if (emit_aos)
            r300_emit_aos(r300, aos_offset, flags & PREP_INDEXED);
        if (emit_aos_swtcl)
            r300_emit_aos_swtcl(r300, flags & PREP_INDEXED);
    }

    if (end_cs_dwords)
        *end_cs_dwords = end_dwords;
}

static boolean immd_is_good_idea(struct r300_context *r300,
                                 unsigned count)
{
    struct pipe_vertex_element* velem;
    struct pipe_vertex_buffer* vbuf;
    boolean checked[PIPE_MAX_ATTRIBS] = {0};
    unsigned vertex_element_count = r300->velems->count;
    unsigned i, vbi;

    if (DBG_ON(r300, DBG_NO_IMMD)) {
        return FALSE;
    }

    if (r300->draw) {
        return FALSE;
    }

    if (count > 10) {
        return FALSE;
    }

    /* We shouldn't map buffers referenced by CS, busy buffers,
     * and ones placed in VRAM. */
    /* XXX Check for VRAM buffers. */
    for (i = 0; i < vertex_element_count; i++) {
        velem = &r300->velems->velem[i];
        vbi = velem->vertex_buffer_index;

        if (!checked[vbi]) {
            vbuf = &r300->vertex_buffer[vbi];

            if (r300_buffer_is_referenced(&r300->context,
                                          vbuf->buffer,
                                          R300_REF_CS | R300_REF_HW)) {
                /* It's a very bad idea to map it... */
                return FALSE;
            }
            checked[vbi] = TRUE;
        }
    }
    return TRUE;
}

/*****************************************************************************
 * The emission of draw packets for r500. Older GPUs may use these functions *
 * after resolving fallback issues (e.g. stencil ref two-sided).             *
 ****************************************************************************/

static void r300_emit_draw_arrays_immediate(struct r300_context *r300,
                                            unsigned mode,
                                            unsigned start,
                                            unsigned count)
{
    struct pipe_vertex_element* velem;
    struct pipe_vertex_buffer* vbuf;
    unsigned vertex_element_count = r300->velems->count;
    unsigned i, v, vbi, dw, elem_offset, dwords;

    /* Size of the vertex, in dwords. */
    unsigned vertex_size = 0;

    /* Offsets of the attribute, in dwords, from the start of the vertex. */
    unsigned offset[PIPE_MAX_ATTRIBS];

    /* Size of the vertex element, in dwords. */
    unsigned size[PIPE_MAX_ATTRIBS];

    /* Stride to the same attrib in the next vertex in the vertex buffer,
     * in dwords. */
    unsigned stride[PIPE_MAX_ATTRIBS] = {0};

    /* Mapped vertex buffers. */
    uint32_t* map[PIPE_MAX_ATTRIBS] = {0};
    struct pipe_transfer* transfer[PIPE_MAX_ATTRIBS] = {NULL};

    CS_LOCALS(r300);

    /* Calculate the vertex size, offsets, strides etc. and map the buffers. */
    for (i = 0; i < vertex_element_count; i++) {
        velem = &r300->velems->velem[i];
        offset[i] = velem->src_offset / 4;
        size[i] = util_format_get_blocksize(velem->src_format) / 4;
        vertex_size += size[i];
        vbi = velem->vertex_buffer_index;

        /* Map the buffer. */
        if (!map[vbi]) {
            vbuf = &r300->vertex_buffer[vbi];
            map[vbi] = (uint32_t*)pipe_buffer_map(&r300->context,
                                                  vbuf->buffer,
                                                  PIPE_TRANSFER_READ,
						  &transfer[vbi]);
            map[vbi] += vbuf->buffer_offset / 4;
            stride[vbi] = vbuf->stride / 4;
        }
    }

    dwords = 9 + count * vertex_size;

    r300_prepare_for_rendering(r300, PREP_FIRST_DRAW, NULL, dwords, 0, 0, NULL);

    BEGIN_CS(dwords);
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG(R300_VAP_VTX_SIZE, vertex_size);
    OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
    OUT_CS(count - 1);
    OUT_CS(0);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_IMMD_2, count * vertex_size);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (count << 16) |
            r300_translate_primitive(mode));

    /* Emit vertices. */
    for (v = 0; v < count; v++) {
        for (i = 0; i < vertex_element_count; i++) {
            velem = &r300->velems->velem[i];
            vbi = velem->vertex_buffer_index;
            elem_offset = offset[i] + stride[vbi] * (v + start);

            for (dw = 0; dw < size[i]; dw++) {
                OUT_CS(map[vbi][elem_offset + dw]);
            }
        }
    }
    END_CS;

    /* Unmap buffers. */
    for (i = 0; i < vertex_element_count; i++) {
        vbi = r300->velems->velem[i].vertex_buffer_index;

        if (map[vbi]) {
            vbuf = &r300->vertex_buffer[vbi];
            pipe_buffer_unmap(&r300->context, vbuf->buffer, transfer[vbi]);
            map[vbi] = NULL;
        }
    }
}

static void r300_emit_draw_arrays(struct r300_context *r300,
                                  unsigned mode,
                                  unsigned count)
{
    boolean alt_num_verts = count > 65535;
    CS_LOCALS(r300);

    if (count >= (1 << 24)) {
        fprintf(stderr, "r300: Got a huge number of vertices: %i, "
                "refusing to render.\n", count);
        return;
    }

    BEGIN_CS(7 + (alt_num_verts ? 2 : 0));
    if (alt_num_verts) {
        OUT_CS_REG(R500_VAP_ALT_NUM_VERTICES, count);
    }
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
    OUT_CS(count - 1);
    OUT_CS(0);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
           r300_translate_primitive(mode) |
           (alt_num_verts ? R500_VAP_VF_CNTL__USE_ALT_NUM_VERTS : 0));
    END_CS;
}

static void r300_emit_draw_elements(struct r300_context *r300,
                                    struct pipe_resource* indexBuffer,
                                    unsigned indexSize,
                                    unsigned minIndex,
                                    unsigned maxIndex,
                                    unsigned mode,
                                    unsigned start,
                                    unsigned count)
{
    uint32_t count_dwords;
    uint32_t offset_dwords = indexSize * start / sizeof(uint32_t);
    boolean alt_num_verts = count > 65535;
    CS_LOCALS(r300);

    if (count >= (1 << 24)) {
        fprintf(stderr, "r300: Got a huge number of vertices: %i, "
                "refusing to render.\n", count);
        return;
    }

    maxIndex = MIN2(maxIndex, r300->vertex_buffer_max_index);

    DBG(r300, DBG_DRAW, "r300: Indexbuf of %u indices, min %u max %u\n",
        count, minIndex, maxIndex);

    BEGIN_CS(13 + (alt_num_verts ? 2 : 0));
    if (alt_num_verts) {
        OUT_CS_REG(R500_VAP_ALT_NUM_VERTICES, count);
    }
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
    OUT_CS(maxIndex);
    OUT_CS(minIndex);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 0);
    if (indexSize == 4) {
        count_dwords = count;
        OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
               R300_VAP_VF_CNTL__INDEX_SIZE_32bit |
               r300_translate_primitive(mode) |
               (alt_num_verts ? R500_VAP_VF_CNTL__USE_ALT_NUM_VERTS : 0));
    } else {
        count_dwords = (count + 1) / 2;
        OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
               r300_translate_primitive(mode) |
               (alt_num_verts ? R500_VAP_VF_CNTL__USE_ALT_NUM_VERTS : 0));
    }

    /* INDX_BUFFER is a truly special packet3.
     * Unlike most other packet3, where the offset is after the count,
     * the order is reversed, so the relocation ends up carrying the
     * size of the indexbuf instead of the offset.
     */
    OUT_CS_PKT3(R300_PACKET3_INDX_BUFFER, 2);
    OUT_CS(R300_INDX_BUFFER_ONE_REG_WR | (R300_VAP_PORT_IDX0 >> 2) |
           (0 << R300_INDX_BUFFER_SKIP_SHIFT));
    OUT_CS(offset_dwords << 2);
    OUT_CS_BUF_RELOC(indexBuffer, count_dwords,
		     r300_buffer(indexBuffer)->domain, 0, 0);

    END_CS;
}

static void r300_shorten_ubyte_elts(struct r300_context* r300,
                                    struct pipe_resource** elts,
                                    unsigned start,
                                    unsigned count)
{
    struct pipe_context* context = &r300->context;
    struct pipe_screen* screen = r300->context.screen;
    struct pipe_resource* new_elts;
    unsigned char *in_map;
    unsigned short *out_map;
    struct pipe_transfer *src_transfer, *dst_transfer;
    unsigned i;

    new_elts = pipe_buffer_create(screen,
				  PIPE_BIND_INDEX_BUFFER,
				  2 * count);

    in_map = pipe_buffer_map(context, *elts, PIPE_TRANSFER_READ, &src_transfer);
    out_map = pipe_buffer_map(context, new_elts, PIPE_TRANSFER_WRITE, &dst_transfer);

    in_map += start;

    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)*in_map;
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, *elts, src_transfer);
    pipe_buffer_unmap(context, new_elts, dst_transfer);

    *elts = new_elts;
}

static void r300_align_ushort_elts(struct r300_context *r300,
                                   struct pipe_resource **elts,
                                   unsigned start, unsigned count)
{
    struct pipe_context* context = &r300->context;
    struct pipe_transfer *in_transfer = NULL;
    struct pipe_transfer *out_transfer = NULL;
    struct pipe_resource* new_elts;
    unsigned short *in_map;
    unsigned short *out_map;

    new_elts = pipe_buffer_create(context->screen,
				  PIPE_BIND_INDEX_BUFFER,
				  2 * count);

    in_map = pipe_buffer_map(context, *elts,
			     PIPE_TRANSFER_READ, &in_transfer);
    out_map = pipe_buffer_map(context, new_elts,
			      PIPE_TRANSFER_WRITE, &out_transfer);

    memcpy(out_map, in_map+start, 2 * count);

    pipe_buffer_unmap(context, *elts, in_transfer);
    pipe_buffer_unmap(context, new_elts, out_transfer);

    *elts = new_elts;
}

/* This is the fast-path drawing & emission for HW TCL. */
static void r300_draw_range_elements(struct pipe_context* pipe,
                                     struct pipe_resource* indexBuffer,
                                     unsigned indexSize,
                                     int indexBias,
                                     unsigned minIndex,
                                     unsigned maxIndex,
                                     unsigned mode,
                                     unsigned start,
                                     unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_resource* orgIndexBuffer = indexBuffer;
    boolean alt_num_verts = r300->screen->caps.is_r500 &&
                            count > 65536 &&
                            r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0);
    unsigned short_count;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    if (indexSize == 1) {
        r300_shorten_ubyte_elts(r300, &indexBuffer, start, count);
        indexSize = 2;
        start = 0;
    } else if (indexSize == 2 && start % 2 != 0) {
        r300_align_ushort_elts(r300, &indexBuffer, start, count);
        start = 0;
    }

    r300_update_derived_state(r300);
    r300_upload_index_buffer(r300, &indexBuffer, indexSize, start, count);

    /* 15 dwords for emit_draw_elements */
    r300_prepare_for_rendering(r300,
        PREP_FIRST_DRAW | PREP_VALIDATE_VBOS | PREP_EMIT_AOS | PREP_INDEXED,
        indexBuffer, 15, 0, indexBias, NULL);

    u_upload_flush(r300->upload_vb);
    u_upload_flush(r300->upload_ib);
    if (alt_num_verts || count <= 65535) {
        r300_emit_draw_elements(r300, indexBuffer, indexSize,
                                 minIndex, maxIndex, mode, start, count);
    } else {
        do {
            short_count = MIN2(count, 65534);
            r300_emit_draw_elements(r300, indexBuffer, indexSize,
                                     minIndex, maxIndex,
                                     mode, start, short_count);

            start += short_count;
            count -= short_count;

            /* 15 dwords for emit_draw_elements */
            if (count) {
                r300_prepare_for_rendering(r300,
                    PREP_VALIDATE_VBOS | PREP_EMIT_AOS | PREP_INDEXED,
                    indexBuffer, 15, 0, indexBias, NULL);
            }
        } while (count);
    }

    if (indexBuffer != orgIndexBuffer) {
        pipe_resource_reference( &indexBuffer, NULL );
    }
}

/* Simple helpers for context setup. Should probably be moved to util. */
static void r300_draw_elements(struct pipe_context* pipe,
                               struct pipe_resource* indexBuffer,
                               unsigned indexSize, int indexBias, unsigned mode,
                               unsigned start, unsigned count)
{
    struct r300_context *r300 = r300_context(pipe);

    pipe->draw_range_elements(pipe, indexBuffer, indexSize, indexBias,
                              0, r300->vertex_buffer_max_index,
                              mode, start, count);
}

static void r300_draw_arrays(struct pipe_context* pipe, unsigned mode,
                             unsigned start, unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    boolean alt_num_verts = r300->screen->caps.is_r500 &&
                            count > 65536 &&
                            r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0);
    unsigned short_count;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    r300_update_derived_state(r300);

    if (immd_is_good_idea(r300, count)) {
        r300_emit_draw_arrays_immediate(r300, mode, start, count);
    } else {
        /* 9 spare dwords for emit_draw_arrays. */
        r300_prepare_for_rendering(r300, PREP_FIRST_DRAW | PREP_VALIDATE_VBOS | PREP_EMIT_AOS,
                               NULL, 9, start, 0, NULL);

        if (alt_num_verts || count <= 65535) {
            r300_emit_draw_arrays(r300, mode, count);
        } else {
            do {
                short_count = MIN2(count, 65535);
                r300_emit_draw_arrays(r300, mode, short_count);

                start += short_count;
                count -= short_count;

                /* 9 spare dwords for emit_draw_arrays. */
                if (count) {
                    r300_prepare_for_rendering(r300,
                        PREP_VALIDATE_VBOS | PREP_EMIT_AOS, NULL, 9,
                        start, 0, NULL);
                }
            } while (count);
        }
	u_upload_flush(r300->upload_vb);
    }
}

/****************************************************************************
 * The rest of this file is for SW TCL rendering only. Please be polite and *
 * keep these functions separated so that they are easier to locate. ~C.    *
 ***************************************************************************/

/* SW TCL arrays, using Draw. */
static void r300_swtcl_draw_arrays(struct pipe_context* pipe,
                                   unsigned mode,
                                   unsigned start,
                                   unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS];
    int i;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    r300_update_derived_state(r300);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        void* buf = pipe_buffer_map(pipe,
                                    r300->vertex_buffer[i].buffer,
                                    PIPE_TRANSFER_READ,
				    &vb_transfer[i]);
        draw_set_mapped_vertex_buffer(r300->draw, i, buf);
    }

    draw_set_mapped_element_buffer(r300->draw, 0, 0, NULL);

    draw_arrays(r300->draw, mode, start, count);

    /* XXX Not sure whether this is the best fix.
     * It prevents CS from being rejected and weird assertion failures. */
    draw_flush(r300->draw);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        pipe_buffer_unmap(pipe, r300->vertex_buffer[i].buffer,
			  vb_transfer[i]);
        draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
    }
}

/* SW TCL elements, using Draw. */
static void r300_swtcl_draw_range_elements(struct pipe_context* pipe,
                                           struct pipe_resource* indexBuffer,
                                           unsigned indexSize,
                                           int indexBias,
                                           unsigned minIndex,
                                           unsigned maxIndex,
                                           unsigned mode,
                                           unsigned start,
                                           unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS];
    struct pipe_transfer *ib_transfer;
    int i;
    void* indices;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    r300_update_derived_state(r300);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        void* buf = pipe_buffer_map(pipe,
                                    r300->vertex_buffer[i].buffer,
                                    PIPE_TRANSFER_READ,
				    &vb_transfer[i]);
        draw_set_mapped_vertex_buffer(r300->draw, i, buf);
    }

    indices = pipe_buffer_map(pipe, indexBuffer,
                              PIPE_TRANSFER_READ, &ib_transfer);
    draw_set_mapped_element_buffer_range(r300->draw, indexSize, indexBias,
                                         minIndex, maxIndex, indices);

    draw_arrays(r300->draw, mode, start, count);

    /* XXX Not sure whether this is the best fix.
     * It prevents CS from being rejected and weird assertion failures. */
    draw_flush(r300->draw);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        pipe_buffer_unmap(pipe, r300->vertex_buffer[i].buffer,
			  vb_transfer[i]);
        draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
    }

    pipe_buffer_unmap(pipe, indexBuffer,
		      ib_transfer);
    draw_set_mapped_element_buffer_range(r300->draw, 0, 0,
                                         start, start + count - 1,
                                         NULL);
}

/* Object for rendering using Draw. */
struct r300_render {
    /* Parent class */
    struct vbuf_render base;

    /* Pipe context */
    struct r300_context* r300;

    /* Vertex information */
    size_t vertex_size;
    unsigned prim;
    unsigned hwprim;

    /* VBO */
    struct pipe_resource* vbo;
    size_t vbo_size;
    size_t vbo_offset;
    size_t vbo_max_used;
    void * vbo_ptr;

    struct pipe_transfer *vbo_transfer;
};

static INLINE struct r300_render*
r300_render(struct vbuf_render* render)
{
    return (struct r300_render*)render;
}

static const struct vertex_info*
r300_render_get_vertex_info(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    return &r300->vertex_info;
}

static boolean r300_render_allocate_vertices(struct vbuf_render* render,
                                                   ushort vertex_size,
                                                   ushort count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    struct pipe_screen* screen = r300->context.screen;
    size_t size = (size_t)vertex_size * (size_t)count;

    if (size + r300render->vbo_offset > r300render->vbo_size)
    {
        pipe_resource_reference(&r300->vbo, NULL);
        r300render->vbo = pipe_buffer_create(screen,
                                             PIPE_BIND_VERTEX_BUFFER,
                                             R300_MAX_DRAW_VBO_SIZE);
        r300render->vbo_offset = 0;
        r300render->vbo_size = R300_MAX_DRAW_VBO_SIZE;
    }

    r300render->vertex_size = vertex_size;
    r300->vbo = r300render->vbo;
    r300->vbo_offset = r300render->vbo_offset;

    return (r300render->vbo) ? TRUE : FALSE;
}

static void* r300_render_map_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);

    assert(!r300render->vbo_transfer);

    r300render->vbo_ptr = pipe_buffer_map(&r300render->r300->context,
					  r300render->vbo,
                                          PIPE_TRANSFER_WRITE,
					  &r300render->vbo_transfer);

    return ((uint8_t*)r300render->vbo_ptr + r300render->vbo_offset);
}

static void r300_render_unmap_vertices(struct vbuf_render* render,
                                             ushort min,
                                             ushort max)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_context* context = &r300render->r300->context;

    assert(r300render->vbo_transfer);

    r300render->vbo_max_used = MAX2(r300render->vbo_max_used,
                                    r300render->vertex_size * (max + 1));
    pipe_buffer_unmap(context, r300render->vbo, r300render->vbo_transfer);

    r300render->vbo_transfer = NULL;
}

static void r300_render_release_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);

    r300render->vbo_offset += r300render->vbo_max_used;
    r300render->vbo_max_used = 0;
}

static boolean r300_render_set_primitive(struct vbuf_render* render,
                                               unsigned prim)
{
    struct r300_render* r300render = r300_render(render);

    r300render->prim = prim;
    r300render->hwprim = r300_translate_primitive(prim);

    return TRUE;
}

static void r300_render_draw_arrays(struct vbuf_render* render,
                                    unsigned start,
                                    unsigned count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    uint8_t* ptr;
    unsigned i;
    unsigned dwords = 6;

    CS_LOCALS(r300);

    (void) i; (void) ptr;

    r300_prepare_for_rendering(r300, PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL,
                               NULL, dwords, 0, 0, NULL);

    DBG(r300, DBG_DRAW, "r300: Doing vbuf render, count %d\n", count);

    /* Uncomment to dump all VBOs rendered through this interface.
     * Slow and noisy!
    ptr = pipe_buffer_map(&r300render->r300->context,
                          r300render->vbo, PIPE_TRANSFER_READ,
                          &r300render->vbo_transfer);

    for (i = 0; i < count; i++) {
        printf("r300: Vertex %d\n", i);
        draw_dump_emitted_vertex(&r300->vertex_info, ptr);
        ptr += r300->vertex_info.size * 4;
        printf("\n");
    }

    pipe_buffer_unmap(&r300render->r300->context, r300render->vbo,
        r300render->vbo_transfer);
    */

    BEGIN_CS(dwords);
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, r300render->prim));
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, count - 1);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
           r300render->hwprim);
    END_CS;
}

static void r300_render_draw_elements(struct vbuf_render* render,
                                      const ushort* indices,
                                      uint count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    int i;
    unsigned end_cs_dwords;
    unsigned max_index = (r300render->vbo_size - r300render->vbo_offset) /
                         (r300render->r300->vertex_info.size * 4) - 1;
    unsigned short_count;
    struct r300_cs_info cs_info;

    CS_LOCALS(r300);

    /* Reserve at least 256 dwords.
     *
     * Below we manage the CS space manually because there may be more
     * indices than it can fit in CS. */
    r300_prepare_for_rendering(r300,
        PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL | PREP_INDEXED,
        NULL, 256, 0, 0, &end_cs_dwords);

    while (count) {
        r300->rws->get_cs_info(r300->rws, &cs_info);

        short_count = MIN2(count, (cs_info.free - end_cs_dwords - 6) * 2);

        BEGIN_CS(6 + (short_count+1)/2);
        OUT_CS_REG(R300_GA_COLOR_CONTROL,
                r300_provoking_vertex_fixes(r300, r300render->prim));
        OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, max_index);
        OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, (short_count+1)/2);
        OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (short_count << 16) |
               r300render->hwprim);
        for (i = 0; i < short_count-1; i += 2) {
            OUT_CS(indices[i+1] << 16 | indices[i]);
        }
        if (short_count % 2) {
            OUT_CS(indices[short_count-1]);
        }
        END_CS;

        /* OK now subtract the emitted indices and see if we need to emit
         * another draw packet. */
        indices += short_count;
        count -= short_count;

        if (count) {
            r300_prepare_for_rendering(r300,
                PREP_EMIT_AOS_SWTCL | PREP_INDEXED,
                NULL, 256, 0, 0, &end_cs_dwords);
        }
    }
}

static void r300_render_destroy(struct vbuf_render* render)
{
    FREE(render);
}

static struct vbuf_render* r300_render_create(struct r300_context* r300)
{
    struct r300_render* r300render = CALLOC_STRUCT(r300_render);

    r300render->r300 = r300;

    /* XXX find real numbers plz */
    r300render->base.max_vertex_buffer_bytes = 128 * 1024;
    r300render->base.max_indices = 16 * 1024;

    r300render->base.get_vertex_info = r300_render_get_vertex_info;
    r300render->base.allocate_vertices = r300_render_allocate_vertices;
    r300render->base.map_vertices = r300_render_map_vertices;
    r300render->base.unmap_vertices = r300_render_unmap_vertices;
    r300render->base.set_primitive = r300_render_set_primitive;
    r300render->base.draw_elements = r300_render_draw_elements;
    r300render->base.draw_arrays = r300_render_draw_arrays;
    r300render->base.release_vertices = r300_render_release_vertices;
    r300render->base.destroy = r300_render_destroy;

    r300render->vbo = NULL;
    r300render->vbo_size = 0;
    r300render->vbo_offset = 0;

    return &r300render->base;
}

struct draw_stage* r300_draw_stage(struct r300_context* r300)
{
    struct vbuf_render* render;
    struct draw_stage* stage;

    render = r300_render_create(r300);

    if (!render) {
        return NULL;
    }

    stage = draw_vbuf_stage(r300->draw, render);

    if (!stage) {
        render->destroy(render);
        return NULL;
    }

    draw_set_render(r300->draw, render);

    return stage;
}

/****************************************************************************
 * Two-sided stencil reference value fallback. It's designed to be as much
 * separate from rest of the driver as possible.
 ***************************************************************************/

struct r300_stencilref_context {
    void (*draw_arrays)(struct pipe_context *pipe,
                        unsigned mode, unsigned start, unsigned count);

    void (*draw_range_elements)(
        struct pipe_context *pipe, struct pipe_resource *indexBuffer,
        unsigned indexSize, int indexBias, unsigned minIndex, unsigned maxIndex,
        unsigned mode, unsigned start, unsigned count);

    uint32_t rs_cull_mode;
    uint32_t zb_stencilrefmask;
    ubyte ref_value_front;
};

static boolean r300_stencilref_needed(struct r300_context *r300)
{
    struct r300_dsa_state *dsa = (struct r300_dsa_state*)r300->dsa_state.state;

    return dsa->two_sided_stencil_ref ||
           (dsa->two_sided &&
            r300->stencil_ref.ref_value[0] != r300->stencil_ref.ref_value[1]);
}

/* Set drawing for front faces. */
static void r300_stencilref_begin(struct r300_context *r300)
{
    struct r300_stencilref_context *sr = r300->stencilref_fallback;
    struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
    struct r300_dsa_state *dsa = (struct r300_dsa_state*)r300->dsa_state.state;

    /* Save state. */
    sr->rs_cull_mode = rs->cull_mode;
    sr->zb_stencilrefmask = dsa->stencil_ref_mask;
    sr->ref_value_front = r300->stencil_ref.ref_value[0];

    /* We *cull* pixels, therefore no need to mask out the bits. */
    rs->cull_mode |= R300_CULL_BACK;

    r300->rs_state.dirty = TRUE;
}

/* Set drawing for back faces. */
static void r300_stencilref_switch_side(struct r300_context *r300)
{
    struct r300_stencilref_context *sr = r300->stencilref_fallback;
    struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
    struct r300_dsa_state *dsa = (struct r300_dsa_state*)r300->dsa_state.state;

    rs->cull_mode = sr->rs_cull_mode | R300_CULL_FRONT;
    dsa->stencil_ref_mask = dsa->stencil_ref_bf;
    r300->stencil_ref.ref_value[0] = r300->stencil_ref.ref_value[1];

    r300->rs_state.dirty = TRUE;
    r300->dsa_state.dirty = TRUE;
}

/* Restore the original state. */
static void r300_stencilref_end(struct r300_context *r300)
{
    struct r300_stencilref_context *sr = r300->stencilref_fallback;
    struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
    struct r300_dsa_state *dsa = (struct r300_dsa_state*)r300->dsa_state.state;

    /* Restore state. */
    rs->cull_mode = sr->rs_cull_mode;
    dsa->stencil_ref_mask = sr->zb_stencilrefmask;
    r300->stencil_ref.ref_value[0] = sr->ref_value_front;

    r300->rs_state.dirty = TRUE;
    r300->dsa_state.dirty = TRUE;
}

static void r300_stencilref_draw_arrays(struct pipe_context *pipe, unsigned mode,
                                        unsigned start, unsigned count)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_stencilref_context *sr = r300->stencilref_fallback;

    if (!r300_stencilref_needed(r300)) {
        sr->draw_arrays(pipe, mode, start, count);
    } else {
        r300_stencilref_begin(r300);
        sr->draw_arrays(pipe, mode, start, count);
        r300_stencilref_switch_side(r300);
        sr->draw_arrays(pipe, mode, start, count);
        r300_stencilref_end(r300);
    }
}

static void r300_stencilref_draw_range_elements(
    struct pipe_context *pipe, struct pipe_resource *indexBuffer,
    unsigned indexSize, int indexBias, unsigned minIndex, unsigned maxIndex,
    unsigned mode, unsigned start, unsigned count)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_stencilref_context *sr = r300->stencilref_fallback;

    if (!r300_stencilref_needed(r300)) {
        sr->draw_range_elements(pipe, indexBuffer, indexSize, indexBias,
                                minIndex, maxIndex, mode, start, count);
    } else {
        r300_stencilref_begin(r300);
        sr->draw_range_elements(pipe, indexBuffer, indexSize, indexBias,
                                minIndex, maxIndex, mode, start, count);
        r300_stencilref_switch_side(r300);
        sr->draw_range_elements(pipe, indexBuffer, indexSize, indexBias,
                                minIndex, maxIndex, mode, start, count);
        r300_stencilref_end(r300);
    }
}

static void r300_plug_in_stencil_ref_fallback(struct r300_context *r300)
{
    r300->stencilref_fallback = CALLOC_STRUCT(r300_stencilref_context);

    /* Save original draw functions. */
    r300->stencilref_fallback->draw_arrays = r300->context.draw_arrays;
    r300->stencilref_fallback->draw_range_elements = r300->context.draw_range_elements;

    /* Override the draw functions. */
    r300->context.draw_arrays = r300_stencilref_draw_arrays;
    r300->context.draw_range_elements = r300_stencilref_draw_range_elements;
}

void r300_init_render_functions(struct r300_context *r300)
{
    /* Set generic functions. */
    r300->context.draw_elements = r300_draw_elements;

    /* Set draw functions based on presence of HW TCL. */
    if (r300->screen->caps.has_tcl) {
        r300->context.draw_arrays = r300_draw_arrays;
        r300->context.draw_range_elements = r300_draw_range_elements;
    } else {
        r300->context.draw_arrays = r300_swtcl_draw_arrays;
        r300->context.draw_range_elements = r300_swtcl_draw_range_elements;
    }

    /* Plug in two-sided stencil reference value fallback if needed. */
    if (!r300->screen->caps.is_r500)
        r300_plug_in_stencil_ref_fallback(r300);
}
