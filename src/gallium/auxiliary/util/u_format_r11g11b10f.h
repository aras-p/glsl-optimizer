/*
 * Copyright (C) 2011 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on code from The OpenGL Programming Guide / 7th Edition, Appendix J.
 * Available here: http://www.opengl-redbook.com/appendices/
 * The algorithm in the book contains a bug though, which is fixed in the code
 * below.
 */

#define UF11_EXPONENT_BIAS   15
#define UF11_EXPONENT_BITS   0x1F
#define UF11_EXPONENT_SHIFT  6
#define UF11_MANTISSA_BITS   0x3F
#define UF11_MANTISSA_SHIFT  (23 - UF11_EXPONENT_SHIFT)
#define UF11_MAX_EXPONENT    (UF11_EXPONENT_BITS << UF11_EXPONENT_SHIFT)

#define UF10_EXPONENT_BIAS   15
#define UF10_EXPONENT_BITS   0x1F
#define UF10_EXPONENT_SHIFT  5
#define UF10_MANTISSA_BITS   0x3F
#define UF10_MANTISSA_SHIFT  (23 - UF10_EXPONENT_SHIFT)
#define UF10_MAX_EXPONENT    (UF10_EXPONENT_BITS << UF10_EXPONENT_SHIFT)

#define F32_INFINITY         0x7f800000

static INLINE unsigned f32_to_uf11(float val)
{
   union {
      float f;
      uint32_t ui;
   } f32 = {val};

   uint16_t uf11 = 0;

   /* Decode little-endian 32-bit floating-point value */
   int sign = (f32.ui >> 16) & 0x8000;
   /* Map exponent to the range [-127,128] */
   int exponent = ((f32.ui >> 23) & 0xff) - 127;
   int mantissa = f32.ui & 0x007fffff;

   if (sign) return 0;

   if (exponent == 128) { /* Infinity or NaN */
      uf11 = UF11_MAX_EXPONENT;
      if (mantissa) uf11 |= (mantissa & UF11_MANTISSA_BITS);
   }
   else if (exponent > 15) { /* Overflow - flush to Infinity */
      uf11 = UF11_MAX_EXPONENT;
   }
   else if (exponent > -15) { /* Representable value */
      exponent += UF11_EXPONENT_BIAS;
      mantissa >>= UF11_MANTISSA_SHIFT;
      uf11 = exponent << UF11_EXPONENT_SHIFT | mantissa;
   }

   return uf11;
}

static INLINE float uf11_to_f32(uint16_t val)
{
   union {
      float f;
      uint32_t ui;
   } f32;

   int exponent = (val & 0x07c0) >> UF11_EXPONENT_SHIFT;
   int mantissa = (val & 0x003f);

   f32.f = 0.0;

   if (exponent == 0) {
      if (mantissa != 0) {
         const float scale = 1.0 / (1 << 20);
         f32.f = scale * mantissa;
      }
   }
   else if (exponent == 31) {
      f32.ui = F32_INFINITY | mantissa;
   }
   else {
      float scale, decimal;
      exponent -= 15;
      if (exponent < 0) {
         scale = 1.0 / (1 << -exponent);
      }
      else {
         scale = 1 << exponent;
      }
      decimal = 1.0 + (float) mantissa / 64;
      f32.f = scale * decimal;
   }

   return f32.f;
}

static INLINE unsigned f32_to_uf10(float val)
{
   union {
      float f;
      uint32_t ui;
   } f32 = {val};

   uint16_t uf10 = 0;

   /* Decode little-endian 32-bit floating-point value */
   int sign = (f32.ui >> 16) & 0x8000;
   /* Map exponent to the range [-127,128] */
   int exponent = ((f32.ui >> 23) & 0xff) - 127;
   int mantissa = f32.ui & 0x007fffff;

   if (sign) return 0;

   if (exponent == 128) { /* Infinity or NaN */
      uf10 = UF10_MAX_EXPONENT;
      if (mantissa) uf10 |= (mantissa & UF10_MANTISSA_BITS);
   }
   else if (exponent > 15) { /* Overflow - flush to Infinity */
      uf10 = UF10_MAX_EXPONENT;
   }
   else if (exponent > -15) { /* Representable value */
      exponent += UF10_EXPONENT_BIAS;
      mantissa >>= UF10_MANTISSA_SHIFT;
      uf10 = exponent << UF10_EXPONENT_SHIFT | mantissa;
   }

   return uf10;
}

static INLINE float uf10_to_f32(uint16_t val)
{
   union {
      float f;
      uint32_t ui;
   } f32;

   int exponent = (val & 0x07c0) >> UF10_EXPONENT_SHIFT;
   int mantissa = (val & 0x003f);

   f32.f = 0.0;

   if (exponent == 0) {
      if (mantissa != 0) {
         const float scale = 1.0 / (1 << 20);
         f32.f = scale * mantissa;
      }
   }
   else if (exponent == 31) {
      f32.ui = F32_INFINITY | mantissa;
   }
   else {
      float scale, decimal;
      exponent -= 15;
      if (exponent < 0) {
         scale = 1.0 / (1 << -exponent);
      }
      else {
         scale = 1 << exponent;
      }
      decimal = 1.0 + (float) mantissa / 32;
      f32.f = scale * decimal;
   }

   return f32.f;
}

static INLINE unsigned float3_to_r11g11b10f(const float rgb[3])
{
   return ( f32_to_uf11(rgb[0]) & 0x7ff) |
          ((f32_to_uf11(rgb[1]) & 0x7ff) << 11) |
          ((f32_to_uf10(rgb[2]) & 0x3ff) << 22);
}

static INLINE void r11g11b10f_to_float3(unsigned rgb, float retval[3])
{
   retval[0] = uf11_to_f32( rgb        & 0x7ff);
   retval[1] = uf11_to_f32((rgb >> 11) & 0x7ff);
   retval[2] = uf10_to_f32((rgb >> 22) & 0x3ff);
}
