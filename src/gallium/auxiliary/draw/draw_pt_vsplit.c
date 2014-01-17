/*
 * Mesa 3-D graphics library
 *
 * Copyright 2007-2008 VMware, Inc.
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_pt.h"

#define SEGMENT_SIZE 1024
#define MAP_SIZE     256

/* The largest possible index withing an index buffer */
#define MAX_ELT_IDX 0xffffffff

struct vsplit_frontend {
   struct draw_pt_front_end base;
   struct draw_context *draw;

   unsigned prim;

   struct draw_pt_middle_end *middle;

   unsigned max_vertices;
   ushort segment_size;

   /* buffers for splitting */
   unsigned fetch_elts[SEGMENT_SIZE];
   ushort draw_elts[SEGMENT_SIZE];
   ushort identity_draw_elts[SEGMENT_SIZE];

   struct {
      /* map a fetch element to a draw element */
      unsigned fetches[MAP_SIZE];
      ushort draws[MAP_SIZE];
      boolean has_max_fetch;

      ushort num_fetch_elts;
      ushort num_draw_elts;
   } cache;
};


static void
vsplit_clear_cache(struct vsplit_frontend *vsplit)
{
   memset(vsplit->cache.fetches, 0xff, sizeof(vsplit->cache.fetches));
   vsplit->cache.has_max_fetch = FALSE;
   vsplit->cache.num_fetch_elts = 0;
   vsplit->cache.num_draw_elts = 0;
}

static void
vsplit_flush_cache(struct vsplit_frontend *vsplit, unsigned flags)
{
   vsplit->middle->run(vsplit->middle,
         vsplit->fetch_elts, vsplit->cache.num_fetch_elts,
         vsplit->draw_elts, vsplit->cache.num_draw_elts, flags);
}

/**
 * Add a fetch element and add it to the draw elements.
 */
static INLINE void
vsplit_add_cache(struct vsplit_frontend *vsplit, unsigned fetch, unsigned ofbias)
{
   unsigned hash;

   hash = fetch % MAP_SIZE;

   /* If the value isn't in the cache of it's an overflow due to the
    * element bias */
   if (vsplit->cache.fetches[hash] != fetch || ofbias) {
      /* update cache */
      vsplit->cache.fetches[hash] = fetch;
      vsplit->cache.draws[hash] = vsplit->cache.num_fetch_elts;

      /* add fetch */
      assert(vsplit->cache.num_fetch_elts < vsplit->segment_size);
      vsplit->fetch_elts[vsplit->cache.num_fetch_elts++] = fetch;
   }

   vsplit->draw_elts[vsplit->cache.num_draw_elts++] = vsplit->cache.draws[hash];
}

/**
 * Returns the base index to the elements array.
 * The value is checked for overflows (both integer overflows
 * and the elements array overflow).
 */
static INLINE unsigned
vsplit_get_base_idx(struct vsplit_frontend *vsplit,
                    unsigned start, unsigned fetch, unsigned *ofbit)
{
   struct draw_context *draw = vsplit->draw;
   unsigned elt_idx = draw_overflow_uadd(start, fetch, MAX_ELT_IDX);
   if (ofbit)
      *ofbit = 0;

   /* Overflown indices need to wrap to the first element
    * in the index buffer */
   if (elt_idx >= draw->pt.user.eltMax) {
      if (ofbit)
         *ofbit = 1;
      elt_idx = 0;
   }

   return elt_idx;
}

/**
 * Returns the element index adjust for the element bias.
 * The final element index is created from the actual element
 * index, plus the element bias, clamped to maximum elememt
 * index if that addition overflows.
 */
static INLINE unsigned
vsplit_get_bias_idx(struct vsplit_frontend *vsplit,
                    int idx, int bias, unsigned *ofbias)
{
   int res = idx + bias;

   if (ofbias)
      *ofbias = 0;

   if (idx > 0 && bias > 0) {
      if (res < idx || res < bias) {
         res = DRAW_MAX_FETCH_IDX;
         if (ofbias)
            *ofbias = 1;
      }
   } else if (idx < 0 && bias < 0) {
      if (res > idx || res > bias) {
         res = DRAW_MAX_FETCH_IDX;
         if (ofbias)
            *ofbias = 1;
      }
   }

   return res;
}

#define VSPLIT_CREATE_IDX(elts, start, fetch, elt_bias)    \
   unsigned elt_idx;                                       \
   unsigned ofbit;                                         \
   unsigned ofbias;                                        \
   elt_idx = vsplit_get_base_idx(vsplit, start, fetch, &ofbit);          \
   elt_idx = vsplit_get_bias_idx(vsplit, ofbit ? 0 : DRAW_GET_IDX(elts, elt_idx), elt_bias, &ofbias)

static INLINE void
vsplit_add_cache_ubyte(struct vsplit_frontend *vsplit, const ubyte *elts,
                       unsigned start, unsigned fetch, int elt_bias)
{
   struct draw_context *draw = vsplit->draw;
   VSPLIT_CREATE_IDX(elts, start, fetch, elt_bias);
   vsplit_add_cache(vsplit, elt_idx, ofbias);
}

static INLINE void
vsplit_add_cache_ushort(struct vsplit_frontend *vsplit, const ushort *elts,
                       unsigned start, unsigned fetch, int elt_bias)
{
   struct draw_context *draw = vsplit->draw;
   VSPLIT_CREATE_IDX(elts, start, fetch, elt_bias);
   vsplit_add_cache(vsplit, elt_idx, ofbias);
}


/**
 * Add a fetch element and add it to the draw elements.  The fetch element is
 * in full range (uint).
 */
static INLINE void
vsplit_add_cache_uint(struct vsplit_frontend *vsplit, const uint *elts,
                      unsigned start, unsigned fetch, int elt_bias)
{
   struct draw_context *draw = vsplit->draw;
   unsigned raw_elem_idx = start + fetch + elt_bias;
   VSPLIT_CREATE_IDX(elts, start, fetch, elt_bias);

   /* special care for DRAW_MAX_FETCH_IDX */
   if (raw_elem_idx == DRAW_MAX_FETCH_IDX && !vsplit->cache.has_max_fetch) {
      unsigned hash = fetch % MAP_SIZE;
      vsplit->cache.fetches[hash] = raw_elem_idx - 1; /* force update */
      vsplit->cache.has_max_fetch = TRUE;
   }

   vsplit_add_cache(vsplit, elt_idx, ofbias);
}


#define FUNC vsplit_run_linear
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_ubyte
#define ELT_TYPE ubyte
#define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_ubyte(vsplit,ib,start,fetch,bias)
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_ushort
#define ELT_TYPE ushort
#define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_ushort(vsplit,ib,start,fetch, bias)
#include "draw_pt_vsplit_tmp.h"

#define FUNC vsplit_run_uint
#define ELT_TYPE uint
#define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_uint(vsplit, ib, start, fetch, bias)
#include "draw_pt_vsplit_tmp.h"


static void vsplit_prepare(struct draw_pt_front_end *frontend,
                           unsigned in_prim,
                           struct draw_pt_middle_end *middle,
                           unsigned opt)
{
   struct vsplit_frontend *vsplit = (struct vsplit_frontend *) frontend;

   switch (vsplit->draw->pt.user.eltSize) {
   case 0:
      vsplit->base.run = vsplit_run_linear;
      break;
   case 1:
      vsplit->base.run = vsplit_run_ubyte;
      break;
   case 2:
      vsplit->base.run = vsplit_run_ushort;
      break;
   case 4:
      vsplit->base.run = vsplit_run_uint;
      break;
   default:
      assert(0);
      break;
   }

   /* split only */
   vsplit->prim = in_prim;

   vsplit->middle = middle;
   middle->prepare(middle, vsplit->prim, opt, &vsplit->max_vertices);

   vsplit->segment_size = MIN2(SEGMENT_SIZE, vsplit->max_vertices);
}


static void vsplit_flush(struct draw_pt_front_end *frontend, unsigned flags)
{
   struct vsplit_frontend *vsplit = (struct vsplit_frontend *) frontend;

   if (flags & DRAW_FLUSH_STATE_CHANGE) {
      vsplit->middle->finish(vsplit->middle);
      vsplit->middle = NULL;
   }
}


static void vsplit_destroy(struct draw_pt_front_end *frontend)
{
   FREE(frontend);
}


struct draw_pt_front_end *draw_pt_vsplit(struct draw_context *draw)
{
   struct vsplit_frontend *vsplit = CALLOC_STRUCT(vsplit_frontend);
   ushort i;

   if (!vsplit)
      return NULL;

   vsplit->base.prepare = vsplit_prepare;
   vsplit->base.run     = NULL;
   vsplit->base.flush   = vsplit_flush;
   vsplit->base.destroy = vsplit_destroy;
   vsplit->draw = draw;

   for (i = 0; i < SEGMENT_SIZE; i++)
      vsplit->identity_draw_elts[i] = i;

   return &vsplit->base;
}
