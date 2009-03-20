
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
tgsi_util_get_src_register_extswizzle(
   const struct tgsi_src_register_ext_swz *reg,
   unsigned component )
{
   switch( component ) {
   case 0:
      return reg->ExtSwizzleX;
   case 1:
      return reg->ExtSwizzleY;
   case 2:
      return reg->ExtSwizzleZ;
   case 3:
      return reg->ExtSwizzleW;
   default:
      ASSERT( 0 );
   }
   return 0;
}

unsigned
tgsi_util_get_full_src_register_extswizzle(
   const struct tgsi_full_src_register  *reg,
   unsigned component )
{
   unsigned swizzle;

   /*
    * First, calculate  the   extended swizzle for a given channel. This will give
    * us either a channel index into the simple swizzle or  a constant 1 or   0.
    */
   swizzle = tgsi_util_get_src_register_extswizzle(
      &reg->SrcRegisterExtSwz,
      component );

   ASSERT (TGSI_SWIZZLE_X == TGSI_EXTSWIZZLE_X);
   ASSERT (TGSI_SWIZZLE_Y == TGSI_EXTSWIZZLE_Y);
   ASSERT (TGSI_SWIZZLE_Z == TGSI_EXTSWIZZLE_Z);
   ASSERT (TGSI_SWIZZLE_W == TGSI_EXTSWIZZLE_W);
   ASSERT (TGSI_EXTSWIZZLE_ZERO > TGSI_SWIZZLE_W);
   ASSERT (TGSI_EXTSWIZZLE_ONE > TGSI_SWIZZLE_W);

   /*
    * Second, calculate the simple  swizzle  for   the   unswizzled channel index.
    * Leave the constants intact, they are   not   affected by the   simple swizzle.
    */
   if( swizzle <= TGSI_SWIZZLE_W ) {
      swizzle = tgsi_util_get_src_register_swizzle(
         &reg->SrcRegister,
         component );
   }

   return swizzle;
}

unsigned
tgsi_util_get_src_register_extnegate(
   const  struct tgsi_src_register_ext_swz *reg,
   unsigned component )
{
   switch( component ) {
   case 0:
      return reg->NegateX;
   case 1:
      return reg->NegateY;
   case 2:
      return reg->NegateZ;
   case 3:
      return reg->NegateW;
   default:
      ASSERT( 0 );
   }
   return 0;
}

void
tgsi_util_set_src_register_extnegate(
   struct tgsi_src_register_ext_swz *reg,
   unsigned negate,
   unsigned component )
{
   switch( component ) {
   case 0:
      reg->NegateX = negate;
      break;
   case 1:
      reg->NegateY = negate;
      break;
   case 2:
      reg->NegateZ = negate;
      break;
   case 3:
      reg->NegateW = negate;
      break;
   default:
      ASSERT( 0 );
   }
}

unsigned
tgsi_util_get_full_src_register_sign_mode(
   const struct  tgsi_full_src_register *reg,
   unsigned component )
{
   unsigned sign_mode;

   if( reg->SrcRegisterExtMod.Absolute ) {
      /* Consider only the post-abs negation. */

      if( reg->SrcRegisterExtMod.Negate ) {
         sign_mode = TGSI_UTIL_SIGN_SET;
      }
      else {
         sign_mode = TGSI_UTIL_SIGN_CLEAR;
      }
   }
   else {
      /* Accumulate the three negations. */

      unsigned negate;

      negate = reg->SrcRegister.Negate;
      if( tgsi_util_get_src_register_extnegate( &reg->SrcRegisterExtSwz, component ) ) {
         negate = !negate;
      }
      if( reg->SrcRegisterExtMod.Negate ) {
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
