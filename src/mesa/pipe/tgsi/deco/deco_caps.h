#if !defined DECO_CAPS_H
#define DECO_CAPS_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

struct tgsi_deco_caps
{
   /*
    * Predicates (D3D9-specific).
    *
    * Constraints:
    *    1. Token tgsi_dst_register_ext_predicate must not be used.
    *    2. Token tgsi_instruction_ext_predicate must not be used.
    */
   unsigned Predicates     : 1;

   /*
    * Destination register post-modulate.
    *
    * Constraints:
    *    1. Field tgsi_dst_register_ext_modulate::Modulate
    *       must be set to TGSI_MODULATE_1X.
    */
   unsigned DstModulate    : 1;

   /*
    * Condition codes (NVIDIA-specific).
    *
    * Constraints:
    *    1. Token tgsi_dst_register_ext_concode must not be used.
    *    2. Field tgsi_instruction_ext_nv::CondDstUpdate must be set to FALSE.
    *    3. Field tgsi_instruction_ext_nv::CondFlowEnable must be set to FALSE.
    */
   unsigned ConCodes       : 1;

   /*
    * Source register invert.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_mod::Complement must be set to FALSE.
    */
   unsigned SrcInvert      : 1;

   /*
    * Source register bias.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_mod::Bias must be set to FALSE.
    */
   unsigned SrcBias        : 1;

   /*
    * Source register scale by 2.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_mod::Scale2X must be set to FALSE.
    */
   unsigned SrcScale       : 1;

   /*
    * Source register absolute.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_mod::Absolute must be set to FALSE.
    */
   unsigned SrcAbsolute    : 1;

   /*
    * Source register force sign.
    *
    * Constraints:
    *    1. Fields tgsi_src_register_ext_mod::Absolute and
    *       tgsi_src_register_ext_mod::Negate must not be both set to TRUE
    *       at the same time.
    */
   unsigned SrcForceSign   : 1;

   /*
    * Source register divide.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_swz::ExtDivide
    *       must be set to TGSI_EXTSWIZZLE_ONE.
    */
   unsigned SrcDivide      : 1;

   /*
    * Source register extended swizzle.
    *
    * Constraints:
    *    1. Field tgsi_src_register_ext_swz::ExtSwizzleX/Y/Z/W
    *       must be set to TGSI_EXTSWIZZLE_X/Y/Z/W.
    *    2. Fields tgsi_src_register_ext_swz::NegateX/Y/Z/W
    *       must all be set to the same value.
    */
   unsigned SrcExtSwizzle  : 1;

   unsigned Padding        : 22;
};

void
tgsi_deco_caps_init(
   struct tgsi_deco_caps *caps );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined DECO_CAPS_H

