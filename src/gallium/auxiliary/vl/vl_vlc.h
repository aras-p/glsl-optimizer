/**************************************************************************
 *
 * Copyright 2011 Christian König.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef vl_vlc_h
#define vl_vlc_h

#include <assert.h>

#include <pipe/p_compiler.h>

#include <util/u_math.h>
#include "util/u_pointer.h"

struct vl_vlc
{
   uint64_t buffer;
   unsigned valid_bits;
   uint32_t *data;
   uint32_t *end;
};

struct vl_vlc_entry
{
   int8_t length;
   int8_t value;
};

struct vl_vlc_compressed
{
   uint16_t bitcode;
   struct vl_vlc_entry entry;
};

static INLINE void
vl_vlc_init_table(struct vl_vlc_entry *dst, unsigned dst_size, const struct vl_vlc_compressed *src, unsigned src_size)
{
   unsigned i, bits = util_logbase2(dst_size);

   for (i=0;i<dst_size;++i) {
      dst[i].length = 0;
      dst[i].value = 0;
   }

   for(; src_size > 0; --src_size, ++src) {
      for(i=0; i<(1 << (bits - src->entry.length)); ++i)
         dst[src->bitcode >> (16 - bits) | i] = src->entry;
   }
}

static INLINE void
vl_vlc_fillbits(struct vl_vlc *vlc)
{
   if (vlc->valid_bits < 32) {
      uint32_t value = *vlc->data;

      //assert(vlc->data <= vlc->end);

#ifndef PIPE_ARCH_BIG_ENDIAN
      value = util_bswap32(value);
#endif

      vlc->buffer |= (uint64_t)value << (32 - vlc->valid_bits);
      ++vlc->data;
      vlc->valid_bits += 32;
   }
}

static INLINE void
vl_vlc_init(struct vl_vlc *vlc, const uint8_t *data, unsigned len)
{
   assert(vlc);
   assert(data && len);

   vlc->buffer = 0;
   vlc->valid_bits = 0;

   /* align the data pointer */
   while (pointer_to_uintptr(data) & 3) {
      vlc->buffer |= (uint64_t)*data << (56 - vlc->valid_bits);
      ++data;
      --len;
      vlc->valid_bits += 8;
   }
   vlc->data = (uint32_t*)data;
   vlc->end = (uint32_t*)(data + len);

   vl_vlc_fillbits(vlc);
   vl_vlc_fillbits(vlc);
}

static INLINE unsigned
vl_vlc_bytes_left(struct vl_vlc *vlc)
{
   return ((uint8_t*)vlc->end)-((uint8_t*)vlc->data);
}

static INLINE unsigned
vl_vlc_peekbits(struct vl_vlc *vlc, unsigned num_bits)
{
   //assert(vlc->valid_bits >= num_bits);

   return vlc->buffer >> (64 - num_bits);
}

static INLINE void
vl_vlc_eatbits(struct vl_vlc *vlc, unsigned num_bits)
{
   //assert(vlc->valid_bits > num_bits);

   vlc->buffer <<= num_bits;
   vlc->valid_bits -= num_bits;
}

static INLINE unsigned
vl_vlc_get_uimsbf(struct vl_vlc *vlc, unsigned num_bits)
{
   unsigned value;

   //assert(vlc->valid_bits >= num_bits);

   value = vlc->buffer >> (64 - num_bits);
   vl_vlc_eatbits(vlc, num_bits);

   return value;
}

static INLINE signed
vl_vlc_get_simsbf(struct vl_vlc *vlc, unsigned num_bits)
{
   signed value;

   //assert(vlc->valid_bits >= num_bits);

   value = ((int64_t)vlc->buffer) >> (64 - num_bits);
   vl_vlc_eatbits(vlc, num_bits);

   return value;
}

static INLINE int8_t
vl_vlc_get_vlclbf(struct vl_vlc *vlc, const struct vl_vlc_entry *tbl, unsigned num_bits)
{
   tbl += vl_vlc_peekbits(vlc, num_bits);
   vl_vlc_eatbits(vlc, tbl->length);
   return tbl->value;
}

#endif /* vl_vlc_h */
