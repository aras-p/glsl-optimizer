#if !defined TGSI_UTIL_H
#define TGSI_UTIL_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

void *
tgsi_align_128bit(
   void *unaligned );

unsigned
tgsi_util_get_src_register_swizzle(
   const struct tgsi_src_register *reg,
   unsigned component );

unsigned
tgsi_util_get_src_register_extswizzle(
   const struct tgsi_src_register_ext_swz *reg,
   unsigned component);

unsigned
tgsi_util_get_full_src_register_extswizzle(
   const struct tgsi_full_src_register *reg,
   unsigned component );

void
tgsi_util_set_src_register_swizzle(
   struct tgsi_src_register *reg,
   unsigned swizzle,
   unsigned component );

void
tgsi_util_set_src_register_extswizzle(
   struct tgsi_src_register_ext_swz *reg,
   unsigned swizzle,
   unsigned component );

unsigned
tgsi_util_get_src_register_extnegate(
   const struct tgsi_src_register_ext_swz *reg,
   unsigned component );

void
tgsi_util_set_src_register_extnegate(
   struct tgsi_src_register_ext_swz *reg,
   unsigned negate,
   unsigned component );

#define TGSI_UTIL_SIGN_CLEAR    0   /* Force positive */
#define TGSI_UTIL_SIGN_SET      1   /* Force negative */
#define TGSI_UTIL_SIGN_TOGGLE   2   /* Negate */
#define TGSI_UTIL_SIGN_KEEP     3   /* No change */

unsigned
tgsi_util_get_full_src_register_sign_mode(
   const struct tgsi_full_src_register *reg,
   unsigned component );

void
tgsi_util_set_full_src_register_sign_mode(
   struct tgsi_full_src_register *reg,
   unsigned sign_mode );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_UTIL_H

