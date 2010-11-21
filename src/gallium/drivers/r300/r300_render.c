/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include <limits.h>

#define IMMD_DWORDS 32

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

boolean r500_index_bias_supported(struct r300_context *r300)
{
    return r300->screen->caps.is_r500 &&
           r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0);
}

void r500_emit_index_bias(struct r300_context *r300, int index_bias)
{
    CS_LOCALS(r300);

    BEGIN_CS(2);
    OUT_CS_REG(R500_VAP_INDEX_OFFSET,
               (index_bias & 0xFFFFFF) | (index_bias < 0 ? 1<<24 : 0));
    END_CS;
}

/* This function splits the index bias value into two parts:
 * - buffer_offset: the value that can be safely added to buffer offsets
 *   in r300_emit_aos (it must yield a positive offset when added to
 *   a vertex buffer offset)
 * - index_offset: the value that must be manually subtracted from indices
 *   in an index buffer to achieve negative offsets. */
static void r300_split_index_bias(struct r300_context *r300, int index_bias,
                                  int *buffer_offset, int *index_offset)
{
    struct pipe_vertex_buffer *vb, *vbufs = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->velems->velem;
    unsigned i, size;
    int max_neg_bias;

    if (index_bias < 0) {
        /* See how large index bias we may subtract. We must be careful
         * here because negative buffer offsets are not allowed
         * by the DRM API. */
        max_neg_bias = INT_MAX;
        for (i = 0; i < r300->velems->count; i++) {
            vb = &vbufs[velem[i].vertex_buffer_index];
            size = (vb->buffer_offset + velem[i].src_offset) / vb->stride;
            max_neg_bias = MIN2(max_neg_bias, size);
        }

        /* Now set the minimum allowed value. */
        *buffer_offset = MAX2(-max_neg_bias, index_bias);
    } else {
        /* A positive index bias is OK. */
        *buffer_offset = index_bias;
    }

    *index_offset = index_bias - *buffer_offset;
}

enum r300_prepare_flags {
    PREP_FIRST_DRAW     = (1 << 0), /* call emit_dirty_state and friends? */
    PREP_VALIDATE_VBOS  = (1 << 1), /* validate VBOs? */
    PREP_EMIT_AOS       = (1 << 2), /* call emit_aos? */
    PREP_EMIT_AOS_SWTCL = (1 << 3), /* call emit_aos_swtcl? */
    PREP_INDEXED        = (1 << 4)  /* is this draw_elements? */
};

/**
 * Check if the requested number of dwords is available in the CS and
 * if not, flush.
 * \param r300          The context.
 * \param flags         See r300_prepare_flags.
 * \param cs_dwords     The number of dwords to reserve in CS.
 * \return TRUE if the CS was flushed
 */
static boolean r300_reserve_cs_dwords(struct r300_context *r300,
                                   enum r300_prepare_flags flags,
                                   unsigned cs_dwords)
{
    boolean flushed        = FALSE;
    boolean first_draw     = flags & PREP_FIRST_DRAW;
    boolean emit_aos       = flags & PREP_EMIT_AOS;
    boolean emit_aos_swtcl = flags & PREP_EMIT_AOS_SWTCL;
    boolean hw_index_bias  = r500_index_bias_supported(r300);

    /* Add dirty state, index offset, and AOS. */
    if (first_draw) {
        cs_dwords += r300_get_num_dirty_dwords(r300);

        if (hw_index_bias)
            cs_dwords += 2; /* emit_index_offset */

        if (emit_aos)
            cs_dwords += 55; /* emit_aos */

        if (emit_aos_swtcl)
            cs_dwords += 7; /* emit_aos_swtcl */
    }

    cs_dwords += r300_get_num_cs_end_dwords(r300);

    /* Reserve requested CS space. */
    if (cs_dwords > (r300->cs->ndw - r300->cs->cdw)) {
        r300->context.flush(&r300->context, 0, NULL);
        flushed = TRUE;
    }

    return flushed;
}

/**
 * Validate buffers and emit dirty state.
 * \param r300          The context.
 * \param flags         See r300_prepare_flags.
 * \param index_buffer  The index buffer to validate. The parameter may be NULL.
 * \param aos_offset    The offset passed to emit_aos.
 * \param index_bias    The index bias to emit.
 * \return TRUE if rendering should be skipped
 */
static boolean r300_emit_states(struct r300_context *r300,
                                enum r300_prepare_flags flags,
                                struct pipe_resource *index_buffer,
                                int aos_offset,
                                int index_bias)
{
    boolean first_draw     = flags & PREP_FIRST_DRAW;
    boolean emit_aos       = flags & PREP_EMIT_AOS;
    boolean emit_aos_swtcl = flags & PREP_EMIT_AOS_SWTCL;
    boolean indexed        = flags & PREP_INDEXED;
    boolean hw_index_bias  = r500_index_bias_supported(r300);

    /* Validate buffers and emit dirty state if needed. */
    if (first_draw) {
        if (!r300_emit_buffer_validate(r300, flags & PREP_VALIDATE_VBOS,
                                       index_buffer)) {
            fprintf(stderr, "r300: CS space validation failed. "
                    "(not enough memory?) Skipping rendering.\n");
            return FALSE;
        }

        r300_emit_dirty_state(r300);
        if (hw_index_bias) {
            if (r300->screen->caps.has_tcl)
                r500_emit_index_bias(r300, index_bias);
            else
                r500_emit_index_bias(r300, 0);
        }

        if (emit_aos)
            r300_emit_aos(r300, aos_offset, indexed);

        if (emit_aos_swtcl)
            r300_emit_aos_swtcl(r300, indexed);
    }

    return TRUE;
}

/**
 * Check if the requested number of dwords is available in the CS and
 * if not, flush. Then validate buffers and emit dirty state.
 * \param r300          The context.
 * \param flags         See r300_prepare_flags.
 * \param index_buffer  The index buffer to validate. The parameter may be NULL.
 * \param cs_dwords     The number of dwords to reserve in CS.
 * \param aos_offset    The offset passed to emit_aos.
 * \param index_bias    The index bias to emit.
 * \return TRUE if rendering should be skipped
 */
static boolean r300_prepare_for_rendering(struct r300_context *r300,
                                          enum r300_prepare_flags flags,
                                          struct pipe_resource *index_buffer,
                                          unsigned cs_dwords,
                                          int aos_offset,
                                          int index_bias)
{
    if (r300_reserve_cs_dwords(r300, flags, cs_dwords))
        flags |= PREP_FIRST_DRAW;

    return r300_emit_states(r300, flags, index_buffer, aos_offset, index_bias);
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

    if (count * r300->velems->vertex_size_dwords > IMMD_DWORDS) {
        return FALSE;
    }

    /* We shouldn't map buffers referenced by CS, busy buffers,
     * and ones placed in VRAM. */
    for (i = 0; i < vertex_element_count; i++) {
        velem = &r300->velems->velem[i];
        vbi = velem->vertex_buffer_index;

        if (!checked[vbi]) {
            vbuf = &r300->vertex_buffer[vbi];

            if (!(r300_buffer(vbuf->buffer)->domain & R300_DOMAIN_GTT)) {
                return FALSE;
            }

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
 * The HWTCL draw functions.                                                 *
 ****************************************************************************/

static void r300_emit_draw_arrays_immediate(struct r300_context *r300,
                                            unsigned mode,
                                            unsigned start,
                                            unsigned count)
{
    struct pipe_vertex_element* velem;
    struct pipe_vertex_buffer* vbuf;
    unsigned vertex_element_count = r300->velems->count;
    unsigned i, v, vbi;

    /* Size of the vertex, in dwords. */
    unsigned vertex_size = r300->velems->vertex_size_dwords;

    /* The number of dwords for this draw operation. */
    unsigned dwords = 9 + count * vertex_size;

    /* Size of the vertex element, in dwords. */
    unsigned size[PIPE_MAX_ATTRIBS];

    /* Stride to the same attrib in the next vertex in the vertex buffer,
     * in dwords. */
    unsigned stride[PIPE_MAX_ATTRIBS];

    /* Mapped vertex buffers. */
    uint32_t* map[PIPE_MAX_ATTRIBS];
    uint32_t* mapelem[PIPE_MAX_ATTRIBS];
    struct pipe_transfer* transfer[PIPE_MAX_ATTRIBS] = {0};

    CS_LOCALS(r300);

    if (!r300_prepare_for_rendering(r300, PREP_FIRST_DRAW, NULL, dwords, 0, 0))
        return;

    /* Calculate the vertex size, offsets, strides etc. and map the buffers. */
    for (i = 0; i < vertex_element_count; i++) {
        velem = &r300->velems->velem[i];
        size[i] = r300->velems->hw_format_size[i] / 4;
        vbi = velem->vertex_buffer_index;
        vbuf = &r300->vertex_buffer[vbi];
        stride[i] = vbuf->stride / 4;

        /* Map the buffer. */
        if (!transfer[vbi]) {
            map[vbi] = (uint32_t*)pipe_buffer_map(&r300->context,
                                                  vbuf->buffer,
                                                  PIPE_TRANSFER_READ,
						  &transfer[vbi]);
            map[vbi] += (vbuf->buffer_offset / 4) + stride[i] * start;
        }
        mapelem[i] = map[vbi] + (velem->src_offset / 4);
    }

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
            OUT_CS_TABLE(&mapelem[i][stride[i] * v], size[i]);
        }
    }
    END_CS;

    /* Unmap buffers. */
    for (i = 0; i < vertex_element_count; i++) {
        vbi = r300->velems->velem[i].vertex_buffer_index;

        if (transfer[vbi]) {
            vbuf = &r300->vertex_buffer[vbi];
            pipe_buffer_unmap(&r300->context, vbuf->buffer, transfer[vbi]);
            transfer[vbi] = NULL;
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
                     r300_buffer(indexBuffer)->domain, 0);

    END_CS;
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
    int buffer_offset = 0, index_offset = 0; /* for index bias emulation */
    unsigned new_offset;

    if (indexBias && !r500_index_bias_supported(r300)) {
        r300_split_index_bias(r300, indexBias, &buffer_offset, &index_offset);
    }

    r300_translate_index_buffer(r300, &indexBuffer, &indexSize, index_offset,
                                &start, count);

    r300_update_derived_state(r300);
    r300_upload_index_buffer(r300, &indexBuffer, indexSize, start, count, &new_offset);

    start = new_offset;

    /* 15 dwords for emit_draw_elements. Give up if the function fails. */
    if (!r300_prepare_for_rendering(r300,
            PREP_FIRST_DRAW | PREP_VALIDATE_VBOS | PREP_EMIT_AOS |
            PREP_INDEXED, indexBuffer, 15, buffer_offset, indexBias))
        goto done;

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
                if (!r300_prepare_for_rendering(r300,
                        PREP_VALIDATE_VBOS | PREP_EMIT_AOS | PREP_INDEXED,
                        indexBuffer, 15, buffer_offset, indexBias))
                    goto done;
            }
        } while (count);
    }

done:
    if (indexBuffer != orgIndexBuffer) {
        pipe_resource_reference( &indexBuffer, NULL );
    }
}

static void r300_draw_arrays(struct pipe_context* pipe, unsigned mode,
                             unsigned start, unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    boolean alt_num_verts = r300->screen->caps.is_r500 &&
                            count > 65536 &&
                            r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0);
    unsigned short_count;

    r300_update_derived_state(r300);

    if (immd_is_good_idea(r300, count)) {
        r300_emit_draw_arrays_immediate(r300, mode, start, count);
    } else {
        /* 9 spare dwords for emit_draw_arrays. Give up if the function fails. */
        if (!r300_prepare_for_rendering(r300,
                PREP_FIRST_DRAW | PREP_VALIDATE_VBOS | PREP_EMIT_AOS,
                NULL, 9, start, 0))
            return;

        if (alt_num_verts || count <= 65535) {
            r300_emit_draw_arrays(r300, mode, count);
        } else {
            do {
                short_count = MIN2(count, 65535);
                r300_emit_draw_arrays(r300, mode, short_count);

                start += short_count;
                count -= short_count;

                /* 9 spare dwords for emit_draw_arrays. Give up if the function fails. */
                if (count) {
                    if (!r300_prepare_for_rendering(r300,
                            PREP_VALIDATE_VBOS | PREP_EMIT_AOS, NULL, 9,
                            start, 0))
                        return;
                }
            } while (count);
        }
    }
}

static void r300_draw_vbo(struct pipe_context* pipe,
                          const struct pipe_draw_info *info)
{
    struct r300_context* r300 = r300_context(pipe);
    unsigned count = info->count;
    boolean translate = FALSE;
    boolean indexed = info->indexed && r300->index_buffer.buffer;
    unsigned start_indexed = 0;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(info->mode, &count)) {
        return;
    }

    /* Index buffer range checking. */
    if (indexed) {
        assert(r300->index_buffer.offset % r300->index_buffer.index_size == 0);

        /* Compute start for draw_elements, taking the offset into account. */
        start_indexed =
            info->start +
            (r300->index_buffer.offset / r300->index_buffer.index_size);

        if ((start_indexed + count) * r300->index_buffer.index_size >
            r300->index_buffer.buffer->width0) {
            fprintf(stderr, "r300: Invalid index buffer range. Skipping rendering.\n");
            return;
        }
    }

    /* Set up fallback for incompatible vertex layout if needed. */
    if (r300->incompatible_vb_layout || r300->velems->incompatible_layout) {
        r300_begin_vertex_translate(r300);
        translate = TRUE;
    }

    if (indexed) {
        r300_draw_range_elements(pipe,
                                 r300->index_buffer.buffer,
                                 r300->index_buffer.index_size,
                                 info->index_bias,
                                 info->min_index,
                                 info->max_index,
                                 info->mode,
                                 start_indexed,
                                 count);
    } else {
        r300_draw_arrays(pipe,
                         info->mode,
                         info->start,
                         count);
    }

    if (translate) {
        r300_end_vertex_translate(r300);
    }
}

/****************************************************************************
 * The rest of this file is for SW TCL rendering only. Please be polite and *
 * keep these functions separated so that they are easier to locate. ~C.    *
 ***************************************************************************/

/* SW TCL elements, using Draw. */
static void r300_swtcl_draw_vbo(struct pipe_context* pipe,
                                const struct pipe_draw_info *info)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS];
    struct pipe_transfer *ib_transfer = NULL;
    unsigned count = info->count;
    int i;
    void *indices = NULL;
    boolean indexed = info->indexed && r300->index_buffer.buffer;

    if (r300->skip_rendering) {
        return;
    }

    if (!u_trim_pipe_prim(info->mode, &count)) {
        return;
    }

    r300_update_derived_state(r300);

    r300_reserve_cs_dwords(r300,
            PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL |
            (indexed ? PREP_INDEXED : 0),
            indexed ? 256 : 6);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        if (r300->vertex_buffer[i].buffer) {
            void *buf = pipe_buffer_map(pipe,
                                  r300->vertex_buffer[i].buffer,
                                  PIPE_TRANSFER_READ,
                                  &vb_transfer[i]);
            draw_set_mapped_vertex_buffer(r300->draw, i, buf);
        }
    }

    if (indexed) {
        indices = pipe_buffer_map(pipe, r300->index_buffer.buffer,
                                  PIPE_TRANSFER_READ, &ib_transfer);
    }

    draw_set_mapped_index_buffer(r300->draw, indices);

    r300->draw_vbo_locked = TRUE;
    r300->draw_first_emitted = FALSE;
    draw_vbo(r300->draw, info);
    draw_flush(r300->draw);
    r300->draw_vbo_locked = FALSE;

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        if (r300->vertex_buffer[i].buffer) {
            pipe_buffer_unmap(pipe, r300->vertex_buffer[i].buffer,
                              vb_transfer[i]);
            draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
        }
    }

    if (indexed) {
        pipe_buffer_unmap(pipe, r300->index_buffer.buffer, ib_transfer);
        draw_set_mapped_index_buffer(r300->draw, NULL);
    }
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

    DBG(r300, DBG_DRAW, "r300: render_allocate_vertices (size: %d)\n", size);

    if (size + r300->draw_vbo_offset > r300->draw_vbo_size)
    {
	pipe_resource_reference(&r300->vbo, NULL);
        r300->vbo = pipe_buffer_create(screen,
				       PIPE_BIND_VERTEX_BUFFER,
				       R300_MAX_DRAW_VBO_SIZE);
        r300->draw_vbo_offset = 0;
        r300->draw_vbo_size = R300_MAX_DRAW_VBO_SIZE;
    }

    r300render->vertex_size = vertex_size;

    return (r300->vbo) ? TRUE : FALSE;
}

static void* r300_render_map_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    assert(!r300render->vbo_transfer);

    DBG(r300, DBG_DRAW, "r300: render_map_vertices\n");

    r300render->vbo_ptr = pipe_buffer_map(&r300render->r300->context,
					  r300->vbo,
                                          PIPE_TRANSFER_WRITE,
					  &r300render->vbo_transfer);

    assert(r300render->vbo_ptr);

    return ((uint8_t*)r300render->vbo_ptr + r300->draw_vbo_offset);
}

static void r300_render_unmap_vertices(struct vbuf_render* render,
                                             ushort min,
                                             ushort max)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_context* context = &r300render->r300->context;
    struct r300_context* r300 = r300render->r300;

    assert(r300render->vbo_transfer);

    DBG(r300, DBG_DRAW, "r300: render_unmap_vertices\n");

    r300render->vbo_max_used = MAX2(r300render->vbo_max_used,
                                    r300render->vertex_size * (max + 1));
    pipe_buffer_unmap(context, r300->vbo, r300render->vbo_transfer);

    r300render->vbo_transfer = NULL;
}

static void r300_render_release_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    DBG(r300, DBG_DRAW, "r300: render_release_vertices\n");

    r300->draw_vbo_offset += r300render->vbo_max_used;
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

    DBG(r300, DBG_DRAW, "r300: render_draw_arrays (count: %d)\n", count);

    if (r300->draw_first_emitted) {
        if (!r300_prepare_for_rendering(r300,
                PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL,
                NULL, 6, 0, 0))
            return;
    } else {
        if (!r300_emit_states(r300,
                PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL,
                NULL, 0, 0))
            return;
    }

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

    r300->draw_first_emitted = TRUE;
}

static void r300_render_draw_elements(struct vbuf_render* render,
                                      const ushort* indices,
                                      uint count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    int i;
    unsigned end_cs_dwords;
    unsigned max_index = (r300->draw_vbo_size - r300->draw_vbo_offset) /
                         (r300render->r300->vertex_info.size * 4) - 1;
    unsigned short_count;
    unsigned free_dwords;

    CS_LOCALS(r300);
    DBG(r300, DBG_DRAW, "r300: render_draw_elements (count: %d)\n", count);

    if (r300->draw_first_emitted) {
        if (!r300_prepare_for_rendering(r300,
                PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL | PREP_INDEXED,
                NULL, 256, 0, 0))
            return;
    } else {
        if (!r300_emit_states(r300,
                PREP_FIRST_DRAW | PREP_EMIT_AOS_SWTCL | PREP_INDEXED,
                NULL, 0, 0))
            return;
    }

    /* Below we manage the CS space manually because there may be more
     * indices than it can fit in CS. */

    end_cs_dwords = r300_get_num_cs_end_dwords(r300);

    while (count) {
        free_dwords = r300->cs->ndw - r300->cs->cdw;

        short_count = MIN2(count, (free_dwords - end_cs_dwords - 6) * 2);

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
            if (!r300_prepare_for_rendering(r300,
                    PREP_EMIT_AOS_SWTCL | PREP_INDEXED,
                    NULL, 256, 0, 0))
                return;

            end_cs_dwords = r300_get_num_cs_end_dwords(r300);
        }
    }

    r300->draw_first_emitted = TRUE;
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

void r300_draw_flush_vbuf(struct r300_context *r300)
{
    pipe_resource_reference(&r300->vbo, NULL);
    r300->draw_vbo_size = 0;
}

/****************************************************************************
 *                         End of SW TCL functions                          *
 ***************************************************************************/

/* This functions is used to draw a rectangle for the blitter module.
 *
 * If we rendered a quad, the pixels on the main diagonal
 * would be computed and stored twice, which makes the clear/copy codepaths
 * somewhat inefficient. Instead we use a rectangular point sprite. */
static void r300_blitter_draw_rectangle(struct blitter_context *blitter,
                                        unsigned x1, unsigned y1,
                                        unsigned x2, unsigned y2,
                                        float depth,
                                        enum blitter_attrib_type type,
                                        const float attrib[4])
{
    struct r300_context *r300 = r300_context(util_blitter_get_pipe(blitter));
    unsigned last_sprite_coord_enable = r300->sprite_coord_enable;
    unsigned width = x2 - x1;
    unsigned height = y2 - y1;
    unsigned vertex_size =
            type == UTIL_BLITTER_ATTRIB_COLOR || !r300->draw ? 8 : 4;
    unsigned dwords = 13 + vertex_size +
                      (type == UTIL_BLITTER_ATTRIB_TEXCOORD ? 7 : 0);
    const float zeros[4] = {0, 0, 0, 0};
    CS_LOCALS(r300);

    if (type == UTIL_BLITTER_ATTRIB_TEXCOORD)
        r300->sprite_coord_enable = 1;

    r300_update_derived_state(r300);

    /* Mark some states we don't care about as non-dirty. */
    r300->clip_state.dirty = FALSE;
    r300->viewport_state.dirty = FALSE;

    if (!r300_prepare_for_rendering(r300, PREP_FIRST_DRAW, NULL, dwords, 0, 0))
        goto done;

    DBG(r300, DBG_DRAW, "r300: draw_rectangle\n");

    BEGIN_CS(dwords);
    /* Set up GA. */
    OUT_CS_REG(R300_GA_POINT_SIZE, (height * 6) | ((width * 6) << 16));

    if (type == UTIL_BLITTER_ATTRIB_TEXCOORD) {
        /* Set up the GA to generate texcoords. */
        OUT_CS_REG(R300_GB_ENABLE, R300_GB_POINT_STUFF_ENABLE |
                   (R300_GB_TEX_STR << R300_GB_TEX0_SOURCE_SHIFT));
        OUT_CS_REG_SEQ(R300_GA_POINT_S0, 4);
        OUT_CS_32F(attrib[0]);
        OUT_CS_32F(attrib[3]);
        OUT_CS_32F(attrib[2]);
        OUT_CS_32F(attrib[1]);
    }

    /* Set up VAP controls. */
    OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE);
    OUT_CS_REG(R300_VAP_VTE_CNTL, R300_VTX_XY_FMT | R300_VTX_Z_FMT);
    OUT_CS_REG(R300_VAP_VTX_SIZE, vertex_size);
    OUT_CS_REG_SEQ(R300_VAP_VF_MAX_VTX_INDX, 2);
    OUT_CS(1);
    OUT_CS(0);

    /* Draw. */
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_IMMD_2, vertex_size);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (1 << 16) |
           R300_VAP_VF_CNTL__PRIM_POINTS);

    OUT_CS_32F(x1 + width * 0.5f);
    OUT_CS_32F(y1 + height * 0.5f);
    OUT_CS_32F(depth);
    OUT_CS_32F(1);

    if (vertex_size == 8) {
        if (!attrib)
            attrib = zeros;
        OUT_CS_TABLE(attrib, 4);
    }
    END_CS;

done:
    /* Restore the state. */
    r300->clip_state.dirty = TRUE;
    r300->rs_state.dirty = TRUE;
    r300->viewport_state.dirty = TRUE;

    r300->sprite_coord_enable = last_sprite_coord_enable;
}

static void r300_resource_resolve(struct pipe_context* pipe,
                                  struct pipe_resource* dest,
                                  struct pipe_subresource subdest,
                                  struct pipe_resource* src,
                                  struct pipe_subresource subsrc)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_aa_state *aa = (struct r300_aa_state*)r300->aa_state.state;
    struct pipe_surface* srcsurf = src->screen->get_tex_surface(src->screen,
            src, subsrc.face, subsrc.level, 0, 0);
    float color[] = {0, 0, 0, 0};

    DBG(r300, DBG_DRAW, "r300: Resolving resource...\n");

    /* Enable AA resolve. */
    aa->dest = r300_surface(
            dest->screen->get_tex_surface(dest->screen, dest, subdest.face,
                                          subdest.level, 0, 0));

    aa->aaresolve_ctl =
        R300_RB3D_AARESOLVE_CTL_AARESOLVE_MODE_RESOLVE |
        R300_RB3D_AARESOLVE_CTL_AARESOLVE_ALPHA_AVERAGE;
    r300->aa_state.size = 12;
    r300->aa_state.dirty = TRUE;

    /* Resolve the surface. */
    r300->context.clear_render_target(pipe,
        srcsurf, color, 0, 0, src->width0, src->height0);

    /* Disable AA resolve. */
    aa->aaresolve_ctl = 0;
    r300->aa_state.size = 4;
    r300->aa_state.dirty = TRUE;

    pipe_surface_reference((struct pipe_surface**)&srcsurf, NULL);
    pipe_surface_reference((struct pipe_surface**)&aa->dest, NULL);
}

void r300_init_render_functions(struct r300_context *r300)
{
    /* Set draw functions based on presence of HW TCL. */
    if (r300->screen->caps.has_tcl) {
        r300->context.draw_vbo = r300_draw_vbo;
    } else {
        r300->context.draw_vbo = r300_swtcl_draw_vbo;
    }

    r300->context.resource_resolve = r300_resource_resolve;
    r300->blitter->draw_rectangle = r300_blitter_draw_rectangle;

    /* Plug in the two-sided stencil reference value fallback if needed. */
    if (!r300->screen->caps.is_r500)
        r300_plug_in_stencil_ref_fallback(r300);
}
