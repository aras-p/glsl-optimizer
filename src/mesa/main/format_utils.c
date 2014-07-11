/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014  Intel Corporation  All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "format_utils.h"
#include "glformats.h"

static const uint8_t map_identity[7] = { 0, 1, 2, 3, 4, 5, 6 };
static const uint8_t map_3210[7] = { 3, 2, 1, 0, 4, 5, 6 };
static const uint8_t map_1032[7] = { 1, 0, 3, 2, 4, 5, 6 };

/**
 * Describes a format as an array format, if possible
 *
 * A helper function for figuring out if a (possibly packed) format is
 * actually an array format and, if so, what the array parameters are.
 *
 * \param[in]  format         the mesa format
 * \param[out] type           the GL type of the array (GL_BYTE, etc.)
 * \param[out] num_components the number of components in the array
 * \param[out] swizzle        a swizzle describing how to get from the
 *                            given format to RGBA
 * \param[out] normalized     for integer formats, this represents whether
 *                            the format is a normalized integer or a
 *                            regular integer
 * \return  true if this format is an array format, false otherwise
 */
bool
_mesa_format_to_array(mesa_format format, GLenum *type, int *num_components,
                      uint8_t swizzle[4], bool *normalized)
{
   int i;
   GLuint format_components;
   uint8_t packed_swizzle[4];
   const uint8_t *endian;

   if (_mesa_is_format_compressed(format))
      return false;

   *normalized = !_mesa_is_format_integer(format);

   _mesa_format_to_type_and_comps(format, type, &format_components);

   switch (_mesa_get_format_layout(format)) {
   case MESA_FORMAT_LAYOUT_ARRAY:
      *num_components = format_components;
      _mesa_get_format_swizzle(format, swizzle);
      return true;
   case MESA_FORMAT_LAYOUT_PACKED:
      switch (*type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
         if (_mesa_get_format_max_bits(format) != 8)
            return false;
         *num_components = _mesa_get_format_bytes(format);
         switch (*num_components) {
         case 1:
            endian = map_identity;
            break;
         case 2:
            endian = _mesa_little_endian() ? map_identity : map_1032;
            break;
         case 4:
            endian = _mesa_little_endian() ? map_identity : map_3210;
            break;
         default:
            endian = map_identity;
            assert(!"Invalid number of components");
         }
         break;
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_HALF_FLOAT:
         if (_mesa_get_format_max_bits(format) != 16)
            return false;
         *num_components = _mesa_get_format_bytes(format) / 2;
         switch (*num_components) {
         case 1:
            endian = map_identity;
            break;
         case 2:
            endian = _mesa_little_endian() ? map_identity : map_1032;
            break;
         default:
            endian = map_identity;
            assert(!"Invalid number of components");
         }
         break;
      case GL_UNSIGNED_INT:
      case GL_INT:
      case GL_FLOAT:
         /* This isn't packed.  At least not really. */
         assert(format_components == 1);
         if (_mesa_get_format_max_bits(format) != 32)
            return false;
         *num_components = format_components;
         endian = map_identity;
         break;
      default:
         return false;
      }

      _mesa_get_format_swizzle(format, packed_swizzle);

      for (i = 0; i < 4; ++i)
         swizzle[i] = endian[packed_swizzle[i]];

      return true;
   case MESA_FORMAT_LAYOUT_OTHER:
   default:
      return false;
   }
}

/* A bunch of format conversion macros and helper functions used below */

/* Only guaranteed to work for BITS <= 32 */
#define MAX_UINT(BITS) ((BITS) == 32 ? UINT32_MAX : ((1u << (BITS)) - 1))
#define MAX_INT(BITS) ((int)MAX_UINT((BITS) - 1))

/* Extends an integer of size SRC_BITS to one of size DST_BITS linearly */
#define EXTEND_NORMALIZED_INT(X, SRC_BITS, DST_BITS) \
      (((X) * (int)(MAX_UINT(DST_BITS) / MAX_UINT(SRC_BITS))) + \
       ((DST_BITS % SRC_BITS) ? ((X) >> (SRC_BITS - DST_BITS % SRC_BITS)) : 0))

static inline float
unorm_to_float(unsigned x, unsigned src_bits)
{
   return x * (1.0f / (float)MAX_UINT(src_bits));
}

static inline float
snorm_to_float(int x, unsigned src_bits)
{
   if (x == -MAX_INT(src_bits))
      return -1.0f;
   else
      return x * (1.0f / (float)MAX_INT(src_bits));
}

static inline uint16_t
unorm_to_half(unsigned x, unsigned src_bits)
{
   return _mesa_float_to_half(unorm_to_float(x, src_bits));
}

static inline uint16_t
snorm_to_half(int x, unsigned src_bits)
{
   return _mesa_float_to_half(snorm_to_float(x, src_bits));
}

static inline unsigned
float_to_unorm(float x, unsigned dst_bits)
{
   if (x < 0.0f)
      return 0;
   else if (x > 1.0f)
      return MAX_UINT(dst_bits);
   else
      return F_TO_I(x * MAX_UINT(dst_bits));
}

static inline unsigned
half_to_unorm(uint16_t x, unsigned dst_bits)
{
   return float_to_unorm(_mesa_half_to_float(x), dst_bits);
}

static inline unsigned
unorm_to_unorm(unsigned x, unsigned src_bits, unsigned dst_bits)
{
   if (src_bits < dst_bits)
      return EXTEND_NORMALIZED_INT(x, src_bits, dst_bits);
   else
      return x >> (src_bits - dst_bits);
}

static inline unsigned
snorm_to_unorm(int x, unsigned src_bits, unsigned dst_bits)
{
   if (x < 0)
      return 0;
   else
      return unorm_to_unorm(x, src_bits - 1, dst_bits);
}

static inline int
float_to_snorm(float x, unsigned dst_bits)
{
   if (x < -1.0f)
      return -MAX_INT(dst_bits);
   else if (x > 1.0f)
      return MAX_INT(dst_bits);
   else
      return F_TO_I(x * MAX_INT(dst_bits));
}

static inline int
half_to_snorm(uint16_t x, unsigned dst_bits)
{
   return float_to_snorm(_mesa_half_to_float(x), dst_bits);
}

static inline int
unorm_to_snorm(unsigned x, unsigned src_bits, unsigned dst_bits)
{
   return unorm_to_unorm(x, src_bits, dst_bits - 1);
}

static inline int
snorm_to_snorm(int x, unsigned src_bits, unsigned dst_bits)
{
   if (x < -MAX_INT(src_bits))
      return -MAX_INT(dst_bits);
   else if (src_bits < dst_bits)
      return EXTEND_NORMALIZED_INT(x, src_bits - 1, dst_bits - 1);
   else
      return x >> (src_bits - dst_bits);
}

static inline unsigned
float_to_uint(float x)
{
   if (x < 0.0f)
      return 0;
   else
      return x;
}

static inline unsigned
half_to_uint(uint16_t x)
{
   if (_mesa_half_is_negative(x))
      return 0;
   else
      return _mesa_float_to_half(x);
}

/**
 * Attempts to perform the given swizzle-and-convert operation with memcpy
 *
 * This function determines if the given swizzle-and-convert operation can
 * be done with a simple memcpy and, if so, does the memcpy.  If not, it
 * returns false and we fall back to the standard version below.
 *
 * The arguments are exactly the same as for _mesa_swizzle_and_convert
 *
 * \return  true if it successfully performed the swizzle-and-convert
 *          operation with memcpy, false otherwise
 */
static bool
swizzle_convert_try_memcpy(void *dst, GLenum dst_type, int num_dst_channels,
                           const void *src, GLenum src_type, int num_src_channels,
                           const uint8_t swizzle[4], bool normalized, int count)
{
   int i;

   if (src_type != dst_type)
      return false;
   if (num_src_channels != num_dst_channels)
      return false;

   for (i = 0; i < num_dst_channels; ++i)
      if (swizzle[i] != i && swizzle[i] != MESA_FORMAT_SWIZZLE_NONE)
         return false;

   memcpy(dst, src, count * num_src_channels * _mesa_sizeof_type(src_type));

   return true;
}

/**
 * Represents a single instance of the standard swizzle-and-convert loop
 *
 * Any swizzle-and-convert operation simply loops through the pixels and
 * performs the transformation operation one pixel at a time.  This macro
 * embodies one instance of the conversion loop.  This way we can do all
 * control flow outside of the loop and allow the compiler to unroll
 * everything inside the loop.
 *
 * Note: This loop is carefully crafted for performance.  Be careful when
 * changing it and run some benchmarks to ensure no performance regressions
 * if you do.
 *
 * \param   DST_TYPE    the C datatype of the destination
 * \param   DST_CHANS   the number of destination channels
 * \param   SRC_TYPE    the C datatype of the source
 * \param   SRC_CHANS   the number of source channels
 * \param   CONV        an expression for converting from the source data,
 *                      storred in the variable "src", to the destination
 *                      format
 */
#define SWIZZLE_CONVERT_LOOP(DST_TYPE, DST_CHANS, SRC_TYPE, SRC_CHANS, CONV) \
   for (s = 0; s < count; ++s) {                                  \
      for (j = 0; j < SRC_CHANS; ++j) {                           \
         SRC_TYPE src = typed_src[j];                             \
         tmp[j] = CONV;                                           \
      }                                                           \
                                                                  \
      typed_dst[0] = tmp[swizzle_x];                              \
      if (DST_CHANS > 1) {                                        \
         typed_dst[1] = tmp[swizzle_y];                           \
         if (DST_CHANS > 2) {                                     \
            typed_dst[2] = tmp[swizzle_z];                        \
            if (DST_CHANS > 3) {                                  \
               typed_dst[3] = tmp[swizzle_w];                     \
            }                                                     \
         }                                                        \
      }                                                           \
      typed_src += SRC_CHANS;                                     \
      typed_dst += DST_CHANS;                                     \
   }                                                              \

/**
 * Represents a single swizzle-and-convert operation
 *
 * This macro represents everything done in a single swizzle-and-convert
 * operation.  The actual work is done by the SWIZZLE_CONVERT_LOOP macro.
 * This macro acts as a wrapper that uses a nested switch to ensure that
 * all looping parameters get unrolled.
 *
 * This macro makes assumptions about variables etc. in the calling
 * function.  Changes to _mesa_swizzle_and_convert may require changes to
 * this macro.
 *
 * \param   DST_TYPE    the C datatype of the destination
 * \param   SRC_TYPE    the C datatype of the source
 * \param   CONV        an expression for converting from the source data,
 *                      storred in the variable "src", to the destination
 *                      format
 */
#define SWIZZLE_CONVERT(DST_TYPE, SRC_TYPE, CONV)                 \
   do {                                                           \
      const SRC_TYPE *typed_src = void_src;                       \
      DST_TYPE *typed_dst = void_dst;                             \
      DST_TYPE tmp[7];                                            \
      tmp[4] = 0;                                                 \
      tmp[5] = one;                                               \
      switch (num_dst_channels) {                                 \
      case 1:                                                     \
         switch (num_src_channels) {                              \
         case 1:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 1, CONV)  \
            break;                                                \
         case 2:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 2, CONV)  \
            break;                                                \
         case 3:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 3, CONV)  \
            break;                                                \
         case 4:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 4, CONV)  \
            break;                                                \
         }                                                        \
         break;                                                   \
      case 2:                                                     \
         switch (num_src_channels) {                              \
         case 1:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 1, CONV)  \
            break;                                                \
         case 2:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 2, CONV)  \
            break;                                                \
         case 3:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 3, CONV)  \
            break;                                                \
         case 4:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 4, CONV)  \
            break;                                                \
         }                                                        \
         break;                                                   \
      case 3:                                                     \
         switch (num_src_channels) {                              \
         case 1:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 1, CONV)  \
            break;                                                \
         case 2:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 2, CONV)  \
            break;                                                \
         case 3:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 3, CONV)  \
            break;                                                \
         case 4:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 4, CONV)  \
            break;                                                \
         }                                                        \
         break;                                                   \
      case 4:                                                     \
         switch (num_src_channels) {                              \
         case 1:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 1, CONV)  \
            break;                                                \
         case 2:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 2, CONV)  \
            break;                                                \
         case 3:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 3, CONV)  \
            break;                                                \
         case 4:                                                  \
            SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 4, CONV)  \
            break;                                                \
         }                                                        \
         break;                                                   \
      }                                                           \
   } while (0);

/**
 * Convert between array-based color formats.
 *
 * Most format conversion operations required by GL can be performed by
 * converting one channel at a time, shuffling the channels around, and
 * optionally filling missing channels with zeros and ones.  This function
 * does just that in a general, yet efficient, way.
 *
 * The swizzle parameter is an array of 4 numbers (see
 * _mesa_get_format_swizzle) that describes where each channel in the
 * destination should come from in the source.  If swizzle[i] < 4 then it
 * means that dst[i] = CONVERT(src[swizzle[i]]).  If swizzle[i] is
 * MESA_FORMAT_SWIZZLE_ZERO or MESA_FORMAT_SWIZZLE_ONE, the corresponding
 * dst[i] will be filled with the appropreate representation of zero or one
 * respectively.
 *
 * Under most circumstances, the source and destination images must be
 * different as no care is taken not to clobber one with the other.
 * However, if they have the same number of bits per pixel, it is safe to
 * do an in-place conversion.
 *
 * \param[out] dst               pointer to where the converted data should
 *                               be stored
 *
 * \param[in]  dst_type          the destination GL type of the converted
 *                               data (GL_BYTE, etc.)
 *
 * \param[in]  num_dst_channels  the number of channels in the converted
 *                               data
 *
 * \param[in]  src               pointer to the source data
 *
 * \param[in]  src_type          the GL type of the source data (GL_BYTE,
 *                               etc.)
 *
 * \param[in]  num_src_channels  the number of channels in the source data
 *                               (the number of channels total, not just
 *                               the number used)
 *
 * \param[in]  swizzle           describes how to get the destination data
 *                               from the source data.
 *
 * \param[in]  normalized        for integer types, this indicates whether
 *                               the data should be considered as integers
 *                               or as normalized integers;
 *
 * \param[in]  count             the number of pixels to convert
 */
void
_mesa_swizzle_and_convert(void *void_dst, GLenum dst_type, int num_dst_channels,
                          const void *void_src, GLenum src_type, int num_src_channels,
                          const uint8_t swizzle[4], bool normalized, int count)
{
   int s, j;
   register uint8_t swizzle_x, swizzle_y, swizzle_z, swizzle_w;

   if (swizzle_convert_try_memcpy(void_dst, dst_type, num_dst_channels,
                                  void_src, src_type, num_src_channels,
                                  swizzle, normalized, count))
      return;

   swizzle_x = swizzle[0];
   swizzle_y = swizzle[1];
   swizzle_z = swizzle[2];
   swizzle_w = swizzle[3];

   switch (dst_type) {
   case GL_FLOAT:
   {
      const float one = 1.0f;
      switch (src_type) {
      case GL_FLOAT:
         SWIZZLE_CONVERT(float, float, src)
         break;
      case GL_HALF_FLOAT:
         SWIZZLE_CONVERT(float, uint16_t, _mesa_half_to_float(src))
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(float, uint8_t, unorm_to_float(src, 8))
         } else {
            SWIZZLE_CONVERT(float, uint8_t, src)
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(float, int8_t, snorm_to_float(src, 8))
         } else {
            SWIZZLE_CONVERT(float, int8_t, src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(float, uint16_t, unorm_to_float(src, 16))
         } else {
            SWIZZLE_CONVERT(float, uint16_t, src)
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(float, int16_t, snorm_to_float(src, 16))
         } else {
            SWIZZLE_CONVERT(float, int16_t, src)
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(float, uint32_t, unorm_to_float(src, 32))
         } else {
            SWIZZLE_CONVERT(float, uint32_t, src)
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(float, int32_t, snorm_to_float(src, 32))
         } else {
            SWIZZLE_CONVERT(float, int32_t, src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_HALF_FLOAT:
   {
      const uint16_t one = _mesa_float_to_half(1.0f);
      switch (src_type) {
      case GL_FLOAT:
         SWIZZLE_CONVERT(uint16_t, float, _mesa_float_to_half(src))
         break;
      case GL_HALF_FLOAT:
         SWIZZLE_CONVERT(uint16_t, uint16_t, src)
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint8_t, unorm_to_half(src, 8))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint8_t, _mesa_float_to_half(src))
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int8_t, snorm_to_half(src, 8))
         } else {
            SWIZZLE_CONVERT(uint16_t, int8_t, _mesa_float_to_half(src))
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint16_t, unorm_to_half(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint16_t, _mesa_float_to_half(src))
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int16_t, snorm_to_half(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, int16_t, _mesa_float_to_half(src))
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint32_t, unorm_to_half(src, 32))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint32_t, _mesa_float_to_half(src))
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int32_t, snorm_to_half(src, 32))
         } else {
            SWIZZLE_CONVERT(uint16_t, int32_t, _mesa_float_to_half(src))
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_UNSIGNED_BYTE:
   {
      const uint8_t one = normalized ? UINT8_MAX : 1;
      switch (src_type) {
      case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, float, float_to_unorm(src, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, float, (src < 0) ? 0 : src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, uint16_t, half_to_unorm(src, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, uint16_t, half_to_uint(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         SWIZZLE_CONVERT(uint8_t, uint8_t, src)
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, int8_t, snorm_to_unorm(src, 8, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, int8_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, uint16_t, unorm_to_unorm(src, 16, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, uint16_t, src)
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, int16_t, snorm_to_unorm(src, 16, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, int16_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, uint32_t, unorm_to_unorm(src, 32, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, uint32_t, src)
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, int32_t, snorm_to_unorm(src, 32, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, int32_t, (src < 0) ? 0 : src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_BYTE:
   {
      const int8_t one = normalized ? INT8_MAX : 1;
      switch (src_type) {
      case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, float, float_to_snorm(src, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, float, src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint8_t, uint16_t, half_to_snorm(src, 8))
         } else {
            SWIZZLE_CONVERT(uint8_t, uint16_t, _mesa_half_to_float(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(int8_t, uint8_t, unorm_to_snorm(src, 8, 8))
         } else {
            SWIZZLE_CONVERT(int8_t, uint8_t, src)
         }
         break;
      case GL_BYTE:
         SWIZZLE_CONVERT(int8_t, int8_t, src)
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(int8_t, uint16_t, unorm_to_snorm(src, 16, 8))
         } else {
            SWIZZLE_CONVERT(int8_t, uint16_t, src)
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(int8_t, int16_t, snorm_to_snorm(src, 16, 8))
         } else {
            SWIZZLE_CONVERT(int8_t, int16_t, src)
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(int8_t, uint32_t, unorm_to_snorm(src, 32, 8))
         } else {
            SWIZZLE_CONVERT(int8_t, uint32_t, src)
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(int8_t, int32_t, snorm_to_snorm(src, 32, 8))
         } else {
            SWIZZLE_CONVERT(int8_t, int32_t, src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_UNSIGNED_SHORT:
   {
      const uint16_t one = normalized ? UINT16_MAX : 1;
      switch (src_type) {
      case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, float, float_to_unorm(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, float, (src < 0) ? 0 : src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint16_t, half_to_unorm(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint16_t, half_to_uint(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint8_t, unorm_to_unorm(src, 8, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint8_t, src)
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int8_t, snorm_to_unorm(src, 8, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, int8_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         SWIZZLE_CONVERT(uint16_t, uint16_t, src)
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int16_t, snorm_to_unorm(src, 16, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, int16_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint32_t, unorm_to_unorm(src, 32, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint32_t, src)
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, int32_t, snorm_to_unorm(src, 32, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, int32_t, (src < 0) ? 0 : src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_SHORT:
   {
      const int16_t one = normalized ? INT16_MAX : 1;
      switch (src_type) {
      case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, float, float_to_snorm(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, float, src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint16_t, uint16_t, half_to_snorm(src, 16))
         } else {
            SWIZZLE_CONVERT(uint16_t, uint16_t, _mesa_half_to_float(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(int16_t, uint8_t, unorm_to_snorm(src, 8, 16))
         } else {
            SWIZZLE_CONVERT(int16_t, uint8_t, src)
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(int16_t, int8_t, snorm_to_snorm(src, 8, 16))
         } else {
            SWIZZLE_CONVERT(int16_t, int8_t, src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(int16_t, uint16_t, unorm_to_snorm(src, 16, 16))
         } else {
            SWIZZLE_CONVERT(int16_t, uint16_t, src)
         }
         break;
      case GL_SHORT:
         SWIZZLE_CONVERT(int16_t, int16_t, src)
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(int16_t, uint32_t, unorm_to_snorm(src, 32, 16))
         } else {
            SWIZZLE_CONVERT(int16_t, uint32_t, src)
         }
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(int16_t, int32_t, snorm_to_snorm(src, 32, 16))
         } else {
            SWIZZLE_CONVERT(int16_t, int32_t, src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_UNSIGNED_INT:
   {
      const uint32_t one = normalized ? UINT32_MAX : 1;
      switch (src_type) { case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, float, float_to_unorm(src, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, float, (src < 0) ? 0 : src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, uint16_t, half_to_unorm(src, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, uint16_t, half_to_uint(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, uint8_t, unorm_to_unorm(src, 8, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, uint8_t, src)
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, int8_t, snorm_to_unorm(src, 8, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, int8_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, uint16_t, unorm_to_unorm(src, 16, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, uint16_t, src)
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, int16_t, snorm_to_unorm(src, 16, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, int16_t, (src < 0) ? 0 : src)
         }
         break;
      case GL_UNSIGNED_INT:
         SWIZZLE_CONVERT(uint32_t, uint32_t, src)
         break;
      case GL_INT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, int32_t, snorm_to_unorm(src, 32, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, int32_t, (src < 0) ? 0 : src)
         }
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   case GL_INT:
   {
      const int32_t one = normalized ? INT32_MAX : 1;
      switch (src_type) {
      case GL_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, float, float_to_snorm(src, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, float, src)
         }
         break;
      case GL_HALF_FLOAT:
         if (normalized) {
            SWIZZLE_CONVERT(uint32_t, uint16_t, half_to_snorm(src, 32))
         } else {
            SWIZZLE_CONVERT(uint32_t, uint16_t, _mesa_half_to_float(src))
         }
         break;
      case GL_UNSIGNED_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(int32_t, uint8_t, unorm_to_snorm(src, 8, 32))
         } else {
            SWIZZLE_CONVERT(int32_t, uint8_t, src)
         }
         break;
      case GL_BYTE:
         if (normalized) {
            SWIZZLE_CONVERT(int32_t, int8_t, snorm_to_snorm(src, 8, 32))
         } else {
            SWIZZLE_CONVERT(int32_t, int8_t, src)
         }
         break;
      case GL_UNSIGNED_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(int32_t, uint16_t, unorm_to_snorm(src, 16, 32))
         } else {
            SWIZZLE_CONVERT(int32_t, uint16_t, src)
         }
         break;
      case GL_SHORT:
         if (normalized) {
            SWIZZLE_CONVERT(int32_t, int16_t, snorm_to_snorm(src, 16, 32))
         } else {
            SWIZZLE_CONVERT(int32_t, int16_t, src)
         }
         break;
      case GL_UNSIGNED_INT:
         if (normalized) {
            SWIZZLE_CONVERT(int32_t, uint32_t, unorm_to_snorm(src, 32, 32))
         } else {
            SWIZZLE_CONVERT(int32_t, uint32_t, src)
         }
         break;
      case GL_INT:
         SWIZZLE_CONVERT(int32_t, int32_t, src)
         break;
      default:
         assert(!"Invalid channel type combination");
      }
   }
   break;
   default:
      assert(!"Invalid channel type");
   }
}
