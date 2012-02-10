/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "translate/translate.h"

#include "nv50_context.h"
#include "nv50_resource.h"

#include "nv50_3d.xml.h"

void
nv50_vertex_state_delete(struct pipe_context *pipe,
                         void *hwcso)
{
   struct nv50_vertex_stateobj *so = hwcso;

   if (so->translate)
      so->translate->release(so->translate);
   FREE(hwcso);
}

void *
nv50_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements)
{
    struct nv50_vertex_stateobj *so;
    struct translate_key transkey;
    unsigned i;

    so = MALLOC(sizeof(*so) +
                num_elements * sizeof(struct nv50_vertex_element));
    if (!so)
        return NULL;
    so->num_elements = num_elements;
    so->instance_elts = 0;
    so->instance_bufs = 0;
    so->need_conversion = FALSE;

    transkey.nr_elements = 0;
    transkey.output_stride = 0;

    for (i = 0; i < num_elements; ++i) {
        const struct pipe_vertex_element *ve = &elements[i];
        const unsigned vbi = ve->vertex_buffer_index;
        enum pipe_format fmt = ve->src_format;

        so->element[i].pipe = elements[i];
        so->element[i].state = nv50_format_table[fmt].vtx;

        if (!so->element[i].state) {
            switch (util_format_get_nr_components(fmt)) {
            case 1: fmt = PIPE_FORMAT_R32_FLOAT; break;
            case 2: fmt = PIPE_FORMAT_R32G32_FLOAT; break;
            case 3: fmt = PIPE_FORMAT_R32G32B32_FLOAT; break;
            case 4: fmt = PIPE_FORMAT_R32G32B32A32_FLOAT; break;
            default:
                assert(0);
                return NULL;
            }
            so->element[i].state = nv50_format_table[fmt].vtx;
            so->need_conversion = TRUE;
        }
        so->element[i].state |= i;

        if (1) {
            unsigned j = transkey.nr_elements++;

            transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
            transkey.element[j].input_format = ve->src_format;
            transkey.element[j].input_buffer = vbi;
            transkey.element[j].input_offset = ve->src_offset;
            transkey.element[j].instance_divisor = ve->instance_divisor;

            transkey.element[j].output_format = fmt;
            transkey.element[j].output_offset = transkey.output_stride;
            transkey.output_stride += (util_format_get_stride(fmt, 1) + 3) & ~3;

            if (unlikely(ve->instance_divisor)) {
               so->instance_elts |= 1 << i;
               so->instance_bufs |= 1 << vbi;
            }
        }
    }

    so->translate = translate_create(&transkey);
    so->vertex_size = transkey.output_stride / 4;
    so->packet_vertex_limit = NV04_PFIFO_MAX_PACKET_LEN /
       MAX2(so->vertex_size, 1);

    return so;
}

#define NV50_3D_VERTEX_ATTRIB_INACTIVE              \
   NV50_3D_VERTEX_ARRAY_ATTRIB_TYPE_FLOAT |         \
   NV50_3D_VERTEX_ARRAY_ATTRIB_FORMAT_32_32_32_32 | \
   NV50_3D_VERTEX_ARRAY_ATTRIB_CONST

static void
nv50_emit_vtxattr(struct nv50_context *nv50, struct pipe_vertex_buffer *vb,
                  struct pipe_vertex_element *ve, unsigned attr)
{
   const void *data;
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct nv04_resource *res = nv04_resource(vb->buffer);
   float v[4];
   const unsigned nc = util_format_get_nr_components(ve->src_format);

   data = nouveau_resource_map_offset(&nv50->base, res, vb->buffer_offset +
                                      ve->src_offset, NOUVEAU_BO_RD);

   util_format_read_4f(ve->src_format, v, 0, data, 0, 0, 0, 1, 1);

   switch (nc) {
   case 4:
      BEGIN_RING(chan, RING_3D(VTX_ATTR_4F_X(attr)), 4);
      OUT_RINGf (chan, v[0]);
      OUT_RINGf (chan, v[1]);
      OUT_RINGf (chan, v[2]);
      OUT_RINGf (chan, v[3]);
      break;
   case 3:
      BEGIN_RING(chan, RING_3D(VTX_ATTR_3F_X(attr)), 3);
      OUT_RINGf (chan, v[0]);
      OUT_RINGf (chan, v[1]);
      OUT_RINGf (chan, v[2]);
      break;
   case 2:
      BEGIN_RING(chan, RING_3D(VTX_ATTR_2F_X(attr)), 2);
      OUT_RINGf (chan, v[0]);
      OUT_RINGf (chan, v[1]);
      break;
   case 1:
      if (attr == nv50->vertprog->vp.edgeflag) {
         BEGIN_RING(chan, RING_3D(EDGEFLAG_ENABLE), 1);
         OUT_RING  (chan, v[0] ? 1 : 0);
      }
      BEGIN_RING(chan, RING_3D(VTX_ATTR_1F(attr)), 1);
      OUT_RINGf (chan, v[0]);
      break;
   default:
      assert(0);
      break;
   }
}

static INLINE void
nv50_vbuf_range(struct nv50_context *nv50, int vbi,
                uint32_t *base, uint32_t *size)
{
   if (unlikely(nv50->vertex->instance_bufs & (1 << vbi))) {
      /* TODO: use min and max instance divisor to get a proper range */
      *base = 0;
      *size = nv50->vtxbuf[vbi].buffer->width0;
   } else {
      assert(nv50->vbo_max_index != ~0);
      *base = nv50->vbo_min_index * nv50->vtxbuf[vbi].stride;
      *size = (nv50->vbo_max_index -
               nv50->vbo_min_index + 1) * nv50->vtxbuf[vbi].stride;
   }
}

static void
nv50_prevalidate_vbufs(struct nv50_context *nv50)
{
   struct pipe_vertex_buffer *vb;
   struct nv04_resource *buf;
   int i;
   uint32_t base, size;

   nv50->vbo_fifo = nv50->vbo_user = 0;

   nv50_bufctx_reset(nv50, NV50_BUFCTX_VERTEX);

   for (i = 0; i < nv50->num_vtxbufs; ++i) {
      vb = &nv50->vtxbuf[i];
      if (!vb->stride)
         continue;
      buf = nv04_resource(vb->buffer);

      /* NOTE: user buffers with temporary storage count as mapped by GPU */
      if (!nouveau_resource_mapped_by_gpu(vb->buffer)) {
         if (nv50->vbo_push_hint) {
            nv50->vbo_fifo = ~0;
            continue;
         } else {
            if (buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY) {
               nv50->vbo_user |= 1 << i;
               assert(vb->stride > vb->buffer_offset);
               nv50_vbuf_range(nv50, i, &base, &size);
               nouveau_user_buffer_upload(buf, base, size);
            } else {
               nouveau_buffer_migrate(&nv50->base, buf, NOUVEAU_BO_GART);
            }
            nv50->base.vbo_dirty = TRUE;
         }
      }
      nv50_bufctx_add_resident(nv50, NV50_BUFCTX_VERTEX, buf, NOUVEAU_BO_RD);
   }
}

static void
nv50_update_user_vbufs(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   uint32_t base, offset, size;
   int i;
   uint32_t written = 0;

   for (i = 0; i < nv50->vertex->num_elements; ++i) {
      struct pipe_vertex_element *ve = &nv50->vertex->element[i].pipe;
      const int b = ve->vertex_buffer_index;
      struct pipe_vertex_buffer *vb = &nv50->vtxbuf[b];
      struct nv04_resource *buf = nv04_resource(vb->buffer);

      if (!(nv50->vbo_user & (1 << b)))
         continue;

      if (!vb->stride) {
         nv50_emit_vtxattr(nv50, vb, ve, i);
         continue;
      }
      nv50_vbuf_range(nv50, b, &base, &size);

      if (!(written & (1 << b))) {
         written |= 1 << b;
         nouveau_user_buffer_upload(buf, base, size);
      }
      offset = vb->buffer_offset + ve->src_offset;

      MARK_RING (chan, 6, 4);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_LIMIT_HIGH(i)), 2);
      OUT_RESRCh(chan, buf, base + size - 1, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, buf, base + size - 1, NOUVEAU_BO_RD);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_START_HIGH(i)), 2);
      OUT_RESRCh(chan, buf, offset, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, buf, offset, NOUVEAU_BO_RD);
   }
   nv50->base.vbo_dirty = TRUE;
}

static INLINE void
nv50_release_user_vbufs(struct nv50_context *nv50)
{
   uint32_t vbo_user = nv50->vbo_user;

   while (vbo_user) {
      int i = ffs(vbo_user) - 1;
      vbo_user &= ~(1 << i);

      nouveau_buffer_release_gpu_storage(nv04_resource(nv50->vtxbuf[i].buffer));
   }
}

void
nv50_vertex_arrays_validate(struct nv50_context *nv50)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct nv50_vertex_stateobj *vertex = nv50->vertex;
   struct pipe_vertex_buffer *vb;
   struct nv50_vertex_element *ve;
   unsigned i;

   if (unlikely(vertex->need_conversion)) {
      nv50->vbo_fifo = ~0;
      nv50->vbo_user = 0;
   } else {
      nv50_prevalidate_vbufs(nv50);
   }

   BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_ATTRIB(0)), vertex->num_elements);
   for (i = 0; i < vertex->num_elements; ++i) {
      ve = &vertex->element[i];
      vb = &nv50->vtxbuf[ve->pipe.vertex_buffer_index];

      if (likely(vb->stride) || nv50->vbo_fifo) {
         OUT_RING(chan, ve->state);
      } else {
         OUT_RING(chan, ve->state | NV50_3D_VERTEX_ARRAY_ATTRIB_CONST);
         nv50->vbo_fifo &= ~(1 << i);
      }
   }

   for (i = 0; i < vertex->num_elements; ++i) {
      struct nv04_resource *res;
      unsigned size, offset;
      
      ve = &vertex->element[i];
      vb = &nv50->vtxbuf[ve->pipe.vertex_buffer_index];

      if (unlikely(ve->pipe.instance_divisor)) {
         if (!(nv50->state.instance_elts & (1 << i))) {
            BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 1);
            OUT_RING  (chan, 1);
         }
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_DIVISOR(i)), 1);
         OUT_RING  (chan, ve->pipe.instance_divisor);
      } else
      if (unlikely(nv50->state.instance_elts & (1 << i))) {
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 1);
         OUT_RING  (chan, 0);
      }

      res = nv04_resource(vb->buffer);

      if (nv50->vbo_fifo || unlikely(vb->stride == 0)) {
         if (!nv50->vbo_fifo)
            nv50_emit_vtxattr(nv50, vb, &ve->pipe, i);
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
         OUT_RING  (chan, 0);
         continue;
      }

      size = vb->buffer->width0;
      offset = ve->pipe.src_offset + vb->buffer_offset;

      MARK_RING (chan, 8, 4);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, NV50_3D_VERTEX_ARRAY_FETCH_ENABLE | vb->stride);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_LIMIT_HIGH(i)), 2);
      OUT_RESRCh(chan, res, size - 1, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, size - 1, NOUVEAU_BO_RD);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_START_HIGH(i)), 2);
      OUT_RESRCh(chan, res, offset, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, offset, NOUVEAU_BO_RD);
   }
   for (; i < nv50->state.num_vtxelts; ++i) {
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_ATTRIB(i)), 1);
      OUT_RING  (chan, NV50_3D_VERTEX_ATTRIB_INACTIVE);
      if (unlikely(nv50->state.instance_elts & (1 << i))) {
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 1);
         OUT_RING  (chan, 0);
      }
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, 0);
   }

   nv50->state.num_vtxelts = vertex->num_elements;
   nv50->state.instance_elts = vertex->instance_elts;
}

#define NV50_PRIM_GL_CASE(n) \
   case PIPE_PRIM_##n: return NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_##n

static INLINE unsigned
nv50_prim_gl(unsigned prim)
{
   switch (prim) {
   NV50_PRIM_GL_CASE(POINTS);
   NV50_PRIM_GL_CASE(LINES);
   NV50_PRIM_GL_CASE(LINE_LOOP);
   NV50_PRIM_GL_CASE(LINE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLES);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLE_FAN);
   NV50_PRIM_GL_CASE(QUADS);
   NV50_PRIM_GL_CASE(QUAD_STRIP);
   NV50_PRIM_GL_CASE(POLYGON);
   NV50_PRIM_GL_CASE(LINES_ADJACENCY);
   NV50_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   default:
      return NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
      break;
   }
}

static void
nv50_draw_vbo_flush_notify(struct nouveau_channel *chan)
{
   struct nv50_screen *screen = chan->user_private;

   nouveau_fence_update(&screen->base, TRUE);

   nv50_bufctx_emit_relocs(screen->cur_ctx);
}

static void
nv50_draw_arrays(struct nv50_context *nv50,
                 unsigned mode, unsigned start, unsigned count,
                 unsigned instance_count)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   unsigned prim;

   if (nv50->state.index_bias) {
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_BASE), 1);
      OUT_RING  (chan, 0);
      nv50->state.index_bias = 0;
   }

   prim = nv50_prim_gl(mode);

   while (instance_count--) {
      BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (chan, prim);
      BEGIN_RING(chan, RING_3D(VERTEX_BUFFER_FIRST), 2);
      OUT_RING  (chan, start);
      OUT_RING  (chan, count);
      BEGIN_RING(chan, RING_3D(VERTEX_END_GL), 1);
      OUT_RING  (chan, 0);

      prim |= NV50_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }
}

static void
nv50_draw_elements_inline_u08(struct nouveau_channel *chan, uint8_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 3) {
      unsigned i;
      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U32), count & 3);
      for (i = 0; i < (count & 3); ++i)
         OUT_RING(chan, *map++);
      count &= ~3;
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 4) / 4;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U8), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan,
                  (map[3] << 24) | (map[2] << 16) | (map[1] << 8) | map[0]);
         map += 4;
      }
      count -= nr * 4;
   }
}

static void
nv50_draw_elements_inline_u16(struct nouveau_channel *chan, uint16_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count &= ~1;
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32), 1);
      OUT_RING  (chan, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nv50_draw_elements_inline_u32(struct nouveau_channel *chan, uint32_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   while (count) {
      const unsigned nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U32), nr);
      OUT_RINGp    (chan, map, nr);

      map += nr;
      count -= nr;
   }
}

static void
nv50_draw_elements_inline_u32_short(struct nouveau_channel *chan, uint32_t *map,
                                    unsigned start, unsigned count)
{
   map += start;

   if (count & 1) {
      count--;
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32), 1);
      OUT_RING  (chan, *map++);
   }
   while (count) {
      unsigned i, nr = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN * 2) / 2;

      BEGIN_RING_NI(chan, RING_3D(VB_ELEMENT_U16), nr);
      for (i = 0; i < nr; ++i) {
         OUT_RING(chan, (map[1] << 16) | map[0]);
         map += 2;
      }
      count -= nr * 2;
   }
}

static void
nv50_draw_elements(struct nv50_context *nv50, boolean shorten,
                   unsigned mode, unsigned start, unsigned count,
                   unsigned instance_count, int32_t index_bias)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   void *data;
   unsigned prim;
   const unsigned index_size = nv50->idxbuf.index_size;

   prim = nv50_prim_gl(mode);

   if (index_bias != nv50->state.index_bias) {
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_BASE), 1);
      OUT_RING  (chan, index_bias);
      nv50->state.index_bias = index_bias;
   }

   if (nouveau_resource_mapped_by_gpu(nv50->idxbuf.buffer)) {
      struct nv04_resource *res = nv04_resource(nv50->idxbuf.buffer);

      start += nv50->idxbuf.offset >> (index_size >> 1);

      while (instance_count--) {
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, mode);

         switch (index_size) {
         case 4:
         {
            WAIT_RING (chan, 2);
            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32) | 0x30000, 0);
            OUT_RING  (chan, count);
            nouveau_pushbuf_submit(chan, res->bo, res->offset + start * 4,
                                   count * 4);
         }
            break;
         case 2:
         {
            unsigned pb_start = (start & ~1);
            unsigned pb_words = (((start + count + 1) & ~1) - pb_start) >> 1;

            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U16_SETUP), 1);
            OUT_RING  (chan, (start << 31) | count);
            WAIT_RING (chan, 2);
            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U16) | 0x30000, 0);
            OUT_RING  (chan, pb_words);
            nouveau_pushbuf_submit(chan, res->bo, res->offset + pb_start * 2,
                                   pb_words * 4);
            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U16_SETUP), 1);
            OUT_RING  (chan, 0);
            break;
         }
         case 1:
         {
            unsigned pb_start = (start & ~3);
            unsigned pb_words = (((start + count + 3) & ~3) - pb_start) >> 1;

            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U8_SETUP), 1);
            OUT_RING  (chan, (start << 30) | count);
            WAIT_RING (chan, 2);
            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U8) | 0x30000, 0);
            OUT_RING  (chan, pb_words);
            nouveau_pushbuf_submit(chan, res->bo, res->offset + pb_start,
                                   pb_words * 4);
            BEGIN_RING(chan, RING_3D(VB_ELEMENT_U8_SETUP), 1);
            OUT_RING  (chan, 0);
            break;
         }
         default:
            assert(0);
            return;
         }
         BEGIN_RING(chan, RING_3D(VERTEX_END_GL), 1);
         OUT_RING  (chan, 0);

         nv50_resource_fence(res, NOUVEAU_BO_RD);

         mode |= NV50_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   } else {
      data = nouveau_resource_map_offset(&nv50->base,
                                         nv04_resource(nv50->idxbuf.buffer),
                                         nv50->idxbuf.offset, NOUVEAU_BO_RD);
      if (!data)
         return;

      while (instance_count--) {
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, prim);
         switch (index_size) {
         case 1:
            nv50_draw_elements_inline_u08(chan, data, start, count);
            break;
         case 2:
            nv50_draw_elements_inline_u16(chan, data, start, count);
            break;
         case 4:
            if (shorten)
               nv50_draw_elements_inline_u32_short(chan, data, start, count);
            else
               nv50_draw_elements_inline_u32(chan, data, start, count);
            break;
         default:
            assert(0);
            return;
         }
         BEGIN_RING(chan, RING_3D(VERTEX_END_GL), 1);
         OUT_RING  (chan, 0);

         prim |= NV50_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   }
}

void
nv50_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_channel *chan = nv50->screen->base.channel;

   /* For picking only a few vertices from a large user buffer, push is better,
    * if index count is larger and we expect repeated vertices, suggest upload.
    */
   nv50->vbo_push_hint = /* the 64 is heuristic */
      !(info->indexed &&
        ((info->max_index - info->min_index + 64) < info->count));

   nv50->vbo_min_index = info->min_index;
   nv50->vbo_max_index = info->max_index;

   if (nv50->vbo_push_hint != !!nv50->vbo_fifo)
      nv50->dirty |= NV50_NEW_ARRAYS;

   if (nv50->vbo_user && !(nv50->dirty & (NV50_NEW_VERTEX | NV50_NEW_ARRAYS)))
      nv50_update_user_vbufs(nv50);

   nv50_state_validate(nv50, ~0, 8); /* 8 as minimum, we use flush_notify */

   chan->flush_notify = nv50_draw_vbo_flush_notify;

   if (nv50->vbo_fifo) {
      nv50_push_vbo(nv50, info);
      chan->flush_notify = nv50_default_flush_notify;
      return;
   }

   if (nv50->state.instance_base != info->start_instance) {
      nv50->state.instance_base = info->start_instance;
      /* NOTE: this does not affect the shader input, should it ? */
      BEGIN_RING(chan, RING_3D(VB_INSTANCE_BASE), 1);
      OUT_RING  (chan, info->start_instance);
   }

   if (nv50->base.vbo_dirty) {
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FLUSH), 1);
      OUT_RING  (chan, 0);
      nv50->base.vbo_dirty = FALSE;
   }

   if (!info->indexed) {
      nv50_draw_arrays(nv50,
                       info->mode, info->start, info->count,
                       info->instance_count);
   } else {
      boolean shorten = info->max_index <= 65535;

      assert(nv50->idxbuf.buffer);

      if (info->primitive_restart != nv50->state.prim_restart) {
         if (info->primitive_restart) {
            BEGIN_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 2);
            OUT_RING  (chan, 1);
            OUT_RING  (chan, info->restart_index);

            if (info->restart_index > 65535)
               shorten = FALSE;
         } else {
            BEGIN_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 1);
            OUT_RING  (chan, 0);
         }
         nv50->state.prim_restart = info->primitive_restart;
      } else
      if (info->primitive_restart) {
         BEGIN_RING(chan, RING_3D(PRIM_RESTART_INDEX), 1);
         OUT_RING  (chan, info->restart_index);

         if (info->restart_index > 65535)
            shorten = FALSE;
      }

      nv50_draw_elements(nv50, shorten,
                         info->mode, info->start, info->count,
                         info->instance_count, info->index_bias);
   }
   chan->flush_notify = nv50_default_flush_notify;

   nv50_release_user_vbufs(nv50);
}
