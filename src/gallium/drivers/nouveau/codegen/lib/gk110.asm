.section #gk110_builtin_code
// DIV U32
//
// UNR recurrence (q = a / b):
// look for z such that 2^32 - b <= b * z < 2^32
// then q - 1 <= (a * z) / 2^32 <= q
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p1
// SIZE:    22 / 14 * 8 bytes
//
gk110_div_u32:
   sched 0x28282804280428
   bfind u32 $r2 $r1
   xor b32 $r2 $r2 0x1f
   mov b32 $r3 0x1
   shl b32 $r2 $r3 clamp $r2
   cvt u32 $r1 neg u32 $r1
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   sched 0x28282828282828
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   sched 0x042c2828042804
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mov b32 $r3 $r0
   mul high $r0 u32 $r0 u32 $r2
   cvt u32 $r2 neg u32 $r1
   add $r1 (mul u32 $r1 u32 $r0) $r3
   set $p0 0x1 ge u32 $r1 $r2
   $p0 sub b32 $r1 $r1 $r2
   sched 0x20282e20042c28
   $p0 add b32 $r0 $r0 0x1
   $p0 set $p0 0x1 ge u32 $r1 $r2
   $p0 sub b32 $r1 $r1 $r2
   $p0 add b32 $r0 $r0 0x1
   ret

// DIV S32, like DIV U32 after taking ABS(inputs)
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p3
//
gk110_div_s32:
   set $p2 0x1 lt s32 $r0 0x0
   set $p3 0x1 lt s32 $r1 0x0 xor $p2
   sched 0x28042804282820
   cvt s32 $r0 abs s32 $r0
   cvt s32 $r1 abs s32 $r1
   bfind u32 $r2 $r1
   xor b32 $r2 $r2 0x1f
   mov b32 $r3 0x1
   shl b32 $r2 $r3 clamp $r2
   cvt u32 $r1 neg u32 $r1
   sched 0x28282828282828
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   sched 0x28280428042828
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mul $r3 u32 $r1 u32 $r2
   add $r2 (mul high u32 $r2 u32 $r3) $r2
   mov b32 $r3 $r0
   mul high $r0 u32 $r0 u32 $r2
   cvt u32 $r2 neg u32 $r1
   add $r1 (mul u32 $r1 u32 $r0) $r3
   sched 0x2028042c28042c
   set $p0 0x1 ge u32 $r1 $r2
   $p0 sub b32 $r1 $r1 $r2
   $p0 add b32 $r0 $r0 0x1
   $p0 set $p0 0x1 ge u32 $r1 $r2
   $p0 sub b32 $r1 $r1 $r2
   $p0 add b32 $r0 $r0 0x1
   $p3 cvt s32 $r0 neg s32 $r0
   sched 0x2c200428042e04
   $p2 cvt s32 $r1 neg s32 $r1
   ret

gk110_rcp_f64:
gk110_rsq_f64:
   ret

.section #gk110_builtin_offsets
.b64 #gk110_div_u32
.b64 #gk110_div_s32
.b64 #gk110_rcp_f64
.b64 #gk110_rsq_f64
