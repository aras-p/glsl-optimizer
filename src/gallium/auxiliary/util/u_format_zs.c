/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#include "u_debug.h"
#include "u_math.h"
#include "u_format_zs.h"


void
util_format_s8_uscaled_unpack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uscaled_pack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint16_t *src = (const uint16_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint16_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         dst[0] = (float)(value * (1.0f/0xffff));
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint16_t *dst = (uint16_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint16_t value;
         value = (uint16_t)(*src * 0xffff);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = value;
         src += 1;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z16_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint16_t *src = (const uint16_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint16_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         /* value * 0xffffffff / 0xffff */
         *dst++ = (value << 16) | value;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z16_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint16_t *dst = (uint16_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint16_t value;
         value = (uint16_t)(*src++ >> 16);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap16(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = (float)(value * (1.0/0xffffffff));
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = (uint32_t)(*src * (double)0xffffffff);
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
         ++src;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_unpack_z_float(float *dst_row, unsigned dst_stride,
                                     const uint8_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                   const float *src_row, unsigned src_stride,
                                   unsigned width, unsigned height)
{
   unsigned y;
   for(y = 0; y < height; ++y) {
      memcpy(dst_row, src_row, width * 4);
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst++ = (uint32_t)(*src++ * (double)0xffffffff);
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                     const uint32_t *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst++ = (float)(*src++ * (1.0/0xffffffff));
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                const uint8_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = (value) & 0xffffff;
         *dst++ = (float)(z * (1.0/0xffffff));
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                              const float *src_row, unsigned src_stride,
                                              unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *dst;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         value &= 0xff000000;
         value |= ((uint32_t)(*src++ * (double)0xffffff)) & 0xffffff;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                  const uint8_t *src_row, unsigned src_stride,
                                                  unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint32_t *src = (const uint32_t *)src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *src++;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = value & 0xffffff;
         *dst++ = (z << 8) | (z >> 16); /* z * 0xffffffff / 0xffffff */;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                const uint32_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint32_t *dst = (uint32_t *)dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(*src >> 8)) & 0xffffff;
         value = ((uint32_t)(((uint64_t)src[1]) * 0x1 / 0xffffffff)) << 24;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *dst++ = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_unpack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t z;
         uint32_t s;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = (value) & 0xffffff;
         s = value >> 24;
         dst[1] = s;
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24_unorm_s8_uscaled_pack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                 const uint8_t *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(((uint32_t)MIN2(*src, 1)) * 0xffffff / 0x1)) & 0xffffff;
         value = (src[1]) << 24;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                const uint8_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t s;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         s = (value) & 0xff;
         z = value >> 8;
         dst[0] = (float)(z * (1.0/0xffffff));
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                              const float *src_row, unsigned src_stride,
                                              unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)CLAMP(src[1], 0, 255)) & 0xff;
         value = ((uint32_t)(*src * (double)0xffffff)) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                  const uint8_t *src_row, unsigned src_stride,
                                                  unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t s;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         s = (value) & 0xff;
         z = value >> 8;
         dst[0] = (uint32_t)(((uint64_t)z) * 0xffffffff / 0xffffff);
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                const uint32_t *src_row, unsigned src_stride,
                                                unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(((uint64_t)src[1]) * 0x1 / 0xffffffff)) & 0xff;
         value = ((uint32_t)(*src >> 8)) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_unpack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t s;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         s = (value) & 0xff;
         z = value >> 8;
         dst[1] = s;
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_s8_uscaled_z24_unorm_pack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                 const uint8_t *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = (src[1]) & 0xff;
         value = ((uint32_t)(((uint32_t)MIN2(*src, 1)) * 0xffffff / 0x1)) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24x8_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = (value) & 0xffffff;
         dst[0] = (float)(z * (1.0/0xffffff));
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24x8_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                     const float *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(*src * (double)0xffffff)) & 0xffffff;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z24x8_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = (value) & 0xffffff;
         dst[0] = (uint32_t)(((uint64_t)z) * 0xffffffff / 0xffffff);
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z24x8_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint32_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(*src >> 8)) & 0xffffff;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_x8z24_unorm_unpack_z_float(float *dst_row, unsigned dst_stride,
                                       const uint8_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = value >> 8;
         dst[0] = (float)(z * (1.0/0xffffff));
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_x8z24_unorm_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                     const float *src_row, unsigned src_stride,
                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(*src * (double)0xffffff)) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_x8z24_unorm_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                         const uint8_t *src_row, unsigned src_stride,
                                         unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const uint8_t *src = src_row;
      for(x = 0; x < width; ++x) {
         uint32_t value = *(const uint32_t *)src;
         uint32_t z;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         z = value >> 8;
         dst[0] = (uint32_t)(((uint64_t)z) * 0xffffffff / 0xffffff);
         src += 4;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_x8z24_unorm_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                       const uint32_t *src_row, unsigned src_stride,
                                       unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      uint8_t *dst = dst_row;
      for(x = 0; x < width; ++x) {
         uint32_t value;
         value = ((uint32_t)(*src >> 8)) << 8;
#ifdef PIPE_ARCH_BIG_ENDIAN
         value = util_bswap32(value);
#endif
         *(uint32_t *)dst = value;
         src += 1;
         dst += 4;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_unpack_z_float(float *dst_row, unsigned dst_stride,
                                                   const uint8_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      float *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 2;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_pack_z_float(uint8_t *dst_row, unsigned dst_stride,
                                                 const float *src_row, unsigned src_stride,
                                                 unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const float *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 1;
         dst += 2;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_unpack_z_32unorm(uint32_t *dst_row, unsigned dst_stride,
                                                     const uint8_t *src_row, unsigned src_stride,
                                                     unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint32_t *dst = dst_row;
      const float *src = (const float *)src_row;
      for(x = 0; x < width; ++x) {
         *dst = (uint32_t)(*src * (double)0xffffffff);
         src += 2;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_pack_z_32unorm(uint8_t *dst_row, unsigned dst_stride,
                                                   const uint32_t *src_row, unsigned src_stride,
                                                   unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint32_t *src = src_row;
      float *dst = (float *)dst_row;
      for(x = 0; x < width; ++x) {
         *dst = (float)(*src * (1.0/0xffffffff));
         src += 2;
         dst += 1;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_unpack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                      const uint8_t *src_row, unsigned src_stride,
                                                      unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      uint8_t *dst = dst_row;
      const uint8_t *src = src_row + 4;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 8;
         dst += 1;
      }
      src_row += src_stride/sizeof(*src_row);
      dst_row += dst_stride/sizeof(*dst_row);
   }
}

void
util_format_z32_float_s8x24_uscaled_pack_s_8uscaled(uint8_t *dst_row, unsigned dst_stride,
                                                    const uint8_t *src_row, unsigned src_stride,
                                                    unsigned width, unsigned height)
{
   unsigned x, y;
   for(y = 0; y < height; ++y) {
      const uint8_t *src = src_row;
      uint8_t *dst = dst_row + 4;
      for(x = 0; x < width; ++x) {
         *dst = *src;
         src += 1;
         dst += 8;
      }
      dst_row += dst_stride/sizeof(*dst_row);
      src_row += src_stride/sizeof(*src_row);
   }
}

