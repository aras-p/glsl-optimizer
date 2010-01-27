/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_debug.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi_parse.h"
#include "tgsi_util.h"

union pointer_hack
{
   void *pointer;
   uint64_t uint64;
};

void *
tgsi_align_128bit(
   void *unaligned )
{
   union pointer_hack ph;

   ph.uint64 = 0;
   ph.pointer = unaligned;
   ph.uint64 = (ph.uint64 + 15) & ~15;
   return ph.pointer;
}

unsigned
tgsi_util_get_src_register_swizzle(
   const struct tgsi_src_register *reg,
   unsigned component )
{
   switch( component ) {
   case 0:
      return reg->SwizzleX;
   case 1:
      return reg->SwizzleY;
   case 2:
      return reg->SwizzleZ;
   case 3:
      return reg->SwizzleW;
   default:
      assert( 0 );
   }
   return 0;
}


unsigned
tgsi_util_get_full_src_register_swizzle(
   const struct tgsi_full_src_register  *reg,
   unsigned component )
{
   return tgsi_util_get_src_register_swizzle(
      &reg->Register,
      component );
}

void
tgsi_util_set_src_register_swizzle(
   struct tgsi_src_register *reg,
   unsigned swizzle,
   unsigned component )
{
   switch( component ) {
   case 0:
      reg->SwizzleX = swizzle;
      break;
   case 1:
      reg->SwizzleY = swizzle;
      break;
   case 2:
      reg->SwizzleZ = swizzle;
      break;
   case 3:
      reg->SwizzleW = swizzle;
      break;
   default:
      assert( 0 );
   }
}

unsigned
tgsi_util_get_full_src_register_sign_mode(
   const struct  tgsi_full_src_register *reg,
   unsigned component )
{
   unsigned sign_mode;

   if( reg->Register.Absolute ) {
      /* Consider only the post-abs negation. */

      if( reg->Register.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_SET;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_CLEAR;
      }
   }
   else {
      if( reg->Register.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_TOGGLE;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_KEEP;
      }
   }

   return sign_mode;
}

void
tgsi_util_set_full_src_register_sign_mode(
   struct tgsi_full_src_register *reg,
   unsigned sign_mode )
{
   switch (sign_mode)
   {
   case TGSI_UTIL_SIGN_CLEAR:
      reg->Register.Negate = 0;
      reg->Register.Absolute = 1;
      break;

   case TGSI_UTIL_SIGN_SET:
      reg->Register.Absolute = 1;
      reg->Register.Negate = 1;
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      reg->Register.Negate = 1;
      reg->Register.Absolute = 0;
      break;

   case TGSI_UTIL_SIGN_KEEP:
      reg->Register.Negate = 0;
      reg->Register.Absolute = 0;
      break;

   default:
      assert( 0 );
   }
}
