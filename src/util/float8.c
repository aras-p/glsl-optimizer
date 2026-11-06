/*
 * Copyright 2025 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#include "float8.h"

#include <assert.h>
#include <math.h>
#include "bitscan.h"
#include "u_math.h"

#define E4M3FN_NAN 0xff
#define E4M3FN_MAX 0x7e

#define E5M2_NAN 0xfe
#define E5M2_MAX 0x7b
#define E5M2_INF 0x7c


uint8_t
_mesa_float_to_e4m3fn(float val)
{
   /* This is a finite only format, out of range values (after rounding)
    * are converted to NaN.
    */
   if (fabs(val) > 464.0f || isnan(val))
      return E4M3FN_NAN;

   bool s = fui(val) & 0x80000000;
   int e = ((fui(val) >> 23) & 0xff) - 127 + 7;
   uint32_t m = fui(val) & 0x7fffff;

   uint8_t res = s ? 0x80 : 0;

   /* Zero, underflow. */
   if (e < -3)
      return res;

   bool is_denorm = e <= 0;

   bool round_up = false;

   if (is_denorm) {
      unsigned offset = 1 - e;
      round_up |= m & ((1 << offset) - 1);
      m = (m | 0x800000) >> offset;
   }

   round_up |= m & 0x17ffff;

   if ((m & 0x080000) && round_up) {
      m += 0x100000;

      if (m & 0x800000) {
         m = 0;
         e += 1;
      }
   }


   if (e > 0)
      res |= (e << 3);
   res |= (m >> 20);

   return res;
}

uint8_t
_mesa_float_to_e4m3fn_sat(float val)
{
   if (val > 448.0f)
      return E4M3FN_MAX;
   else if (val < -448.0f)
      return 0x80 | E4M3FN_MAX;
   else
      return _mesa_float_to_e4m3fn(val);
}

float
_mesa_e4m3fn_to_float(uint8_t val)
{
   bool s = val & 0x80;
   uint32_t e = (val >> 3) & 0xf;
   uint32_t m = val & 0x7;

   if (e == 0xf && m == 0x7)
      return uif(0xffc00000);

   uint32_t res = s ? 0x80000000 : 0;

   if (e == 0 && m == 0) {
      /* Zero. */
   } else if (e == 0) {
      /* Denorm. */
      unsigned shift = (4 - util_last_bit(m));
      res |= (127 - 6 - shift) << 23;
      res |= ((m << shift) & 0x7) << (23 - 3);
   } else {
      res |= (e + (127 - 7)) << 23;
      res |= m << (23 - 3);
   }

   return uif(res);
}

uint8_t
_mesa_float_to_e5m2(float val)
{
   bool s = fui(val) & 0x80000000;
   uint8_t res = s ? 0x80 : 0;
   if (isnan(val))
      return E5M2_NAN;
   else if (fabs(val) >= 61440.0f)
      return res | E5M2_INF;

   int e = ((fui(val) >> 23) & 0xff) - 127 + 15;
   uint32_t m = fui(val) & 0x7fffff;

   /* Zero, underflow. */
   if (e < -2)
      return res;

   bool is_denorm = e <= 0;

   bool round_up = false;

   if (is_denorm) {
      unsigned offset = 1 - e;
      round_up |= m & ((1 << offset) - 1);
      m = (m | 0x800000) >> offset;
   }

   round_up |= m & 0x2fffff;

   if ((m & 0x100000) && round_up) {
      m += 0x200000;

      if (m & 0x800000) {
         m = 0;
         e += 1;
      }
   }


   if (e > 0)
      res |= (e << 2);
   res |= (m >> 21);

   return res;
}

uint8_t
_mesa_float_to_e5m2_sat(float val)
{
   if (val > 57344.0f)
      return E5M2_MAX;
   else if (val < -57344.0f)
      return 0x80 | E5M2_MAX;
   else
      return _mesa_float_to_e5m2(val);
}

float
_mesa_e5m2_to_float(uint8_t val)
{
   bool s = val & 0x80;
   uint32_t e = (val >> 2) & 0x1f;
   uint32_t m = val & 0x3;

   if (e == 0x1f && m != 0)
      return uif(0xffc00000);

   uint32_t res = s ? 0x80000000 : 0;

   if (e == 0x1f) {
      /* Infinity. */
      res |= 0x7f800000;
   } else if (e == 0 && m == 0) {
      /* Zero. */
   } else if (e == 0) {
      /* Denorm. */
      unsigned shift = (3 - util_last_bit(m));
      res |= (127 - 14 - shift) << 23;
      res |= ((m << shift) & 0x3) << (23 - 2);
   } else {
      res |= (e + (127 - 15)) << 23;
      res |= m << (23 - 2);
   }

   return uif(res);
}
