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

#include "nvc0_context.h"
#include "nvc0_resource.h"

#include "nvc0_3d.xml.h"

void
nvc0_vertex_state_delete(struct pipe_context *pipe,
                         void *hwcso)
{
   struct nvc0_vertex_stateobj *so = hwcso;

   if (so->translate)
      so->translate->release(so->translate);
   FREE(hwcso);
}

void *
nvc0_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements)
{
    struct nvc0_vertex_stateobj *so;
    struct translate_key transkey;
    unsigned i;

    assert(num_elements);

    so = MALLOC(sizeof(*so) +
                (num_elements - 1) * sizeof(struct nvc0_vertex_element));
    if (!so)
        return NULL;
    so->num_elements = num_elements;
    so->instance_bits = 0;

    transkey.nr_elements = 0;
    transkey.output_stride = 0;

    for (i = 0; i < num_elements; ++i) {
        const struct pipe_vertex_element *ve = &elements[i];
        const unsigned vbi = ve->vertex_buffer_index;
        enum pipe_format fmt = ve->src_format;

        so->element[i].pipe = elements[i];
        so->element[i].state = nvc0_format_table[fmt].vtx;

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
            so->element[i].state = nvc0_format_table[fmt].vtx;
        }
        so->element[i].state |= i;

        if (likely(!ve->instance_divisor)) {
            unsigned j = transkey.nr_elements++;

            transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
            transkey.element[j].input_format = ve->src_format;
            transkey.element[j].input_buffer = vbi;
            transkey.element[j].input_offset = ve->src_offset;
            transkey.element[j].instance_divisor = ve->instance_divisor;

            transkey.element[j].output_format = fmt;
            transkey.element[j].output_offset = transkey.output_stride;
            transkey.output_stride += (util_format_get_stride(fmt, 1) + 3) & ~3;
        } else {
           so->instance_bits |= 1 << i;
        }
    }

    so->translate = translate_create(&transkey);
    so->vtx_size = transkey.output_stride / 4;
    so->vtx_per_packet_max = NV04_PFIFO_MAX_PACKET_LEN / MAX2(so->vtx_size, 1);

    return so;
}

#define NVC0_3D_VERTEX_ATTRIB_INACTIVE                                       \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |                                 \
   NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_32 | NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST

void
nvc0_vertex_arrays_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   struct pipe_vertex_buffer *vb;
   struct nvc0_vertex_element *ve;
   unsigned i;

   nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_VERTEX);

   nvc0->vbo_fifo = 0;

   BEGIN_RING(chan, RING_3D(VERTEX_ATTRIB_FORMAT(0)), vertex->num_elements);
   for (i = 0; i < vertex->num_elements; ++i) {
      ve = &vertex->element[i];
      vb = &nvc0->vtxbuf[ve->pipe.vertex_buffer_index];

      if (!nvc0_resource_mapped_by_gpu(vb->buffer)) {
         if (nvc0->vbo_push_hint) {
            nvc0->vbo_fifo |= 1 << i;
         } else {
            nvc0_migrate_vertices(nvc0_resource(vb->buffer),
                                  vb->buffer_offset,
                                  vb->buffer->width0 - vb->buffer_offset);
            nvc0->vbo_dirty = TRUE;
         }
      }

      if (1 || likely(vb->stride)) {
         OUT_RING(chan, ve->state);
      } else {
         OUT_RING(chan, ve->state | NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST);
      }
   }

   for (i = 0; i < vertex->num_elements; ++i) {
      struct nvc0_resource *res;
      unsigned size, offset;
      
      ve = &vertex->element[i];
      vb = &nvc0->vtxbuf[ve->pipe.vertex_buffer_index];

      if (nvc0->vbo_fifo || (0 && vb->stride == 0)) {
#if 0
         if (!nvc0->vbo_fifo)
            nvc0_vbo_constant_attrib(nvc0, vb, ve);
#endif
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
         OUT_RING  (chan, 0);
         continue;
      }

      res = nvc0_resource(vb->buffer);
      size = vb->buffer->width0;
      offset = ve->pipe.src_offset + vb->buffer_offset;

      if (unlikely(ve->pipe.instance_divisor)) {
         if (!(nvc0->state.instance_bits & (1 << i))) {
            INLIN_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 1);
         }
         BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_DIVISOR(i)), 1);
         OUT_RING  (chan, ve->pipe.instance_divisor);
      } else
      if (unlikely(nvc0->state.instance_bits & (1 << i))) {
         INLIN_RING(chan, RING_3D(VERTEX_ARRAY_PER_INSTANCE(i)), 0);
      }

      nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_VERTEX, res, NOUVEAU_BO_RD);

      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, (1 << 12) | vb->stride);
      BEGIN_RING_1I(chan, RING_3D(VERTEX_ARRAY_SELECT), 5);
      OUT_RING  (chan, i);
      OUT_RESRCh(chan, res, size, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, size, NOUVEAU_BO_RD);
      OUT_RESRCh(chan, res, offset, NOUVEAU_BO_RD);
      OUT_RESRCl(chan, res, offset, NOUVEAU_BO_RD);
   }
   for (; i < nvc0->state.num_vtxelts; ++i) {
      BEGIN_RING(chan, RING_3D(VERTEX_ATTRIB_FORMAT(i)), 1);
      OUT_RING  (chan, NVC0_3D_VERTEX_ATTRIB_INACTIVE);
      BEGIN_RING(chan, RING_3D(VERTEX_ARRAY_FETCH(i)), 1);
      OUT_RING  (chan, 0);
   }

   nvc0->state.num_vtxelts = vertex->num_elements;
   nvc0->state.instance_bits = vertex->instance_bits;
}

#define NVC0_PRIM_GL_CASE(n) \
   case PIPE_PRIM_##n: return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_##n

static INLINE unsigned
nvc0_prim_gl(unsigned prim)
{
   switch (prim) {
   NVC0_PRIM_GL_CASE(POINTS);
   NVC0_PRIM_GL_CASE(LINES);
   NVC0_PRIM_GL_CASE(LINE_LOOP);
   NVC0_PRIM_GL_CASE(LINE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLES);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLE_FAN);
   NVC0_PRIM_GL_CASE(QUADS);
   NVC0_PRIM_GL_CASE(QUAD_STRIP);
   NVC0_PRIM_GL_CASE(POLYGON);
   NVC0_PRIM_GL_CASE(LINES_ADJACENCY);
   NVC0_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   /*
   NVC0_PRIM_GL_CASE(PATCHES); */
   default:
      return NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
      break;
   }
}

static void
nvc0_draw_vbo_flush_notify(struct nouveau_channel *chan)
{
   struct nvc0_context *nvc0 = chan->user_private;

   nvc0_bufctx_emit_relocs(nvc0);
}

#if 0
static struct nouveau_bo *
nvc0_tfb_setup(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nouveau_bo *tfb = NULL;
   int ret, i;

   ret = nouveau_bo_new(nvc0->screen->base.device,
                        NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096, &tfb);
   if (ret)
      return NULL;

   ret = nouveau_bo_map(tfb, NOUVEAU_BO_WR);
   if (ret)
      return NULL;
   memset(tfb->map, 0xee, 8 * 4 * 3);
   nouveau_bo_unmap(tfb);

   BEGIN_RING(chan, RING_3D(TFB_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(TFB_BUFFER_ENABLE(0)), 5);
   OUT_RING  (chan, 1);
   OUT_RELOCh(chan, tfb, 0, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, tfb, 0, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   OUT_RING  (chan, tfb->size);
   OUT_RING  (chan, 0); /* TFB_PRIMITIVE_ID(0) */
   BEGIN_RING(chan, RING_3D(TFB_UNK0700(0)), 3);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 8); /* TFB_VARYING_COUNT(0) */
   OUT_RING  (chan, 32); /* TFB_BUFFER_STRIDE(0) */
   BEGIN_RING(chan, RING_3D(TFB_VARYING_LOCS(0)), 2);
   OUT_RING  (chan, 0x1f1e1d1c);
   OUT_RING  (chan, 0xa3a2a1a0);
   for (i = 1; i < 4; ++i) {
      BEGIN_RING(chan, RING_3D(TFB_BUFFER_ENABLE(i)), 1);
      OUT_RING  (chan, 0);
   }
   BEGIN_RING(chan, RING_3D(TFB_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x135c), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x135c), 1);
   OUT_RING  (chan, 0);

   return tfb;
}
#endif

static void
nvc0_draw_arrays(struct nvc0_context *nvc0,
                 unsigned mode, unsigned start, unsigned count,
                 unsigned instance_count)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   unsigned prim;

   chan->flush_notify = nvc0_draw_vbo_flush_notify;
   chan->user_private = nvc0;

   prim = nvc0_prim_gl(mode);

   while (instance_count--) {
      BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
      OUT_RING  (chan, prim);
      BEGIN_RING(chan, RING_3D(VERTEX_BUFFER_FIRST), 2);
      OUT_RING  (chan, start);
      OUT_RING  (chan, count);
      INLIN_RING(chan, RING_3D(VERTEX_END_GL), 0);

      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
   }

   chan->flush_notify = NULL;
}

static void
nvc0_draw_elements_inline_u08(struct nouveau_channel *chan, uint8_t *map,
                              unsigned start, unsigned count)
{
   map += start;

   if (count & 3) {
      unsigned i;
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_U32), count & 3);
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
nvc0_draw_elements_inline_u16(struct nouveau_channel *chan, uint16_t *map,
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
nvc0_draw_elements_inline_u32(struct nouveau_channel *chan, uint32_t *map,
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
nvc0_draw_elements_inline_u32_short(struct nouveau_channel *chan, uint32_t *map,
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
nvc0_draw_elements(struct nvc0_context *nvc0, boolean shorten,
                   unsigned mode, unsigned start, unsigned count,
                   unsigned instance_count, int32_t index_bias)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   void *data;
   struct pipe_transfer *transfer;
   unsigned prim;
   unsigned index_size = nvc0->idxbuf.index_size;

   chan->flush_notify = nvc0_draw_vbo_flush_notify;
   chan->user_private = nvc0;

   prim = nvc0_prim_gl(mode);

   if (index_bias != nvc0->state.index_bias) {
      BEGIN_RING(chan, RING_3D(VB_ELEMENT_BASE), 1);
      OUT_RING  (chan, index_bias);
      nvc0->state.index_bias = index_bias;
   }

   if (nvc0_resource_mapped_by_gpu(nvc0->idxbuf.buffer)) {
      struct nvc0_resource *res = nvc0_resource(nvc0->idxbuf.buffer);
      unsigned offset = nvc0->idxbuf.offset;
      unsigned limit = nvc0->idxbuf.buffer->width0 - 1;

      if (index_size == 4)
         index_size = 2;
      else
      if (index_size == 2)
         index_size = 1;

      while (instance_count--) {
         MARK_RING (chan, 11, 4);
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, mode);
         BEGIN_RING(chan, RING_3D(INDEX_ARRAY_START_HIGH), 7);
         OUT_RESRCh(chan, res, offset, NOUVEAU_BO_RD);
         OUT_RESRCl(chan, res, offset, NOUVEAU_BO_RD);
         OUT_RESRCh(chan, res, limit, NOUVEAU_BO_RD);
         OUT_RESRCl(chan, res, limit, NOUVEAU_BO_RD);
         OUT_RING  (chan, index_size);
         OUT_RING  (chan, start);
         OUT_RING  (chan, count);
         INLIN_RING(chan, RING_3D(VERTEX_END_GL), 0);

         mode |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   } else {
      data = pipe_buffer_map(&nvc0->pipe, nvc0->idxbuf.buffer,
                             PIPE_TRANSFER_READ, &transfer);
      if (!data)
         return;
      data = (uint8_t *)data + nvc0->idxbuf.offset;

      while (instance_count--) {
         BEGIN_RING(chan, RING_3D(VERTEX_BEGIN_GL), 1);
         OUT_RING  (chan, prim);
         switch (index_size) {
         case 1:
            nvc0_draw_elements_inline_u08(chan, data, start, count);
            break;
         case 2:
            nvc0_draw_elements_inline_u16(chan, data, start, count);
            break;
         case 4:
            if (shorten)
               nvc0_draw_elements_inline_u32_short(chan, data, start, count);
            else
               nvc0_draw_elements_inline_u32(chan, data, start, count);
            break;
         default:
            assert(0);
            return;
         }
         INLIN_RING(chan, RING_3D(VERTEX_END_GL), 0);

         prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   }

   chan->flush_notify = NULL;
}

void
nvc0_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   /* For picking only a few vertices from a large user buffer, push is better,
    * if index count is larger and we expect repeated vertices, suggest upload.
    */
   nvc0->vbo_push_hint = /* the 64 is heuristic */
      !(info->indexed &&
        ((info->max_index - info->min_index + 64) < info->count));

   nvc0_state_validate(nvc0);

   if (nvc0->state.instance_base != info->start_instance) {
      nvc0->state.instance_base = info->start_instance;
      BEGIN_RING(chan, RING_3D(VB_INSTANCE_BASE), 1);
      OUT_RING  (chan, info->start_instance);
   }

   if (nvc0->vbo_fifo) {
      nvc0_push_vbo(nvc0, info);
      return;
   }

   if (nvc0->vbo_dirty) {
      BEGIN_RING(chan, RING_3D_(0x142c), 1);
      OUT_RING  (chan, 0);
      nvc0->vbo_dirty = FALSE;
   }

   if (!info->indexed) {
      nvc0_draw_arrays(nvc0,
                       info->mode, info->start, info->count,
                       info->instance_count);
   } else {
      boolean shorten = info->max_index <= 65535;

      assert(nvc0->idxbuf.buffer);

      if (info->primitive_restart != nvc0->state.prim_restart) {
         if (info->primitive_restart) {
            BEGIN_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 2);
            OUT_RING  (chan, 1);
            OUT_RING  (chan, info->restart_index);

            if (info->restart_index > 65535)
               shorten = FALSE;
         } else {
            INLIN_RING(chan, RING_3D(PRIM_RESTART_ENABLE), 0);
         }
         nvc0->state.prim_restart = info->primitive_restart;
      } else
      if (info->primitive_restart) {
         BEGIN_RING(chan, RING_3D(PRIM_RESTART_INDEX), 1);
         OUT_RING  (chan, info->restart_index);
      }

      nvc0_draw_elements(nvc0, shorten,
                         info->mode, info->start, info->count,
                         info->instance_count, info->index_bias);
   }
}
