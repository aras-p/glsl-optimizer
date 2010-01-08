
#include "cell/common.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "tgsi/tgsi_parse.h"
//#include "tgsi_build.h"
#include "tgsi/tgsi_util.h"

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
      ASSERT( 0 );
   }
   return 0;
}


unsigned
tgsi_util_get_full_src_register_swizzle(
   const struct tgsi_full_src_register  *reg,
   unsigned component )
{
   return tgsi_util_get_src_register_swizzle(
      reg->Register,
      component );
}


unsigned
tgsi_util_get_full_src_register_sign_mode(
   const struct  tgsi_full_src_register *reg,
   unsigned component )
{
   unsigned sign_mode;

   if( reg->RegisterExtMod.Absolute ) {
      /* Consider only the post-abs negation. */

      if( reg->RegisterExtMod.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_SET;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_CLEAR;
      }
   }
   else {
      /* Accumulate the three negations. */

      unsigned negate;

      negate = reg->Register.Negate;
      if( reg->RegisterExtMod.Negate ) {
         negate = !negate;
      }

      if( negate ) {
         sign_mode = TGSI_UTIL_SIGN_TOGGLE;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_KEEP;
      }
   }

   return sign_mode;
}
