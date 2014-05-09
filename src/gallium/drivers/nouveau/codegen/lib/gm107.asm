.section #gm107_builtin_code
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
gm107_div_u32:
   sched 0x7e0 0x7e0 0x7e0
   flo u32 $r2 $r1
   lop xor 1 $r2 $r2 0x1f
   mov $r3 0x1 0xf
   sched 0x7e0 0x7e0 0x7e0
   shl $r2 $r3 $r2
   i2i u32 u32 $r1 neg $r1
   imul u32 u32 $r3 $r1 $r2
   sched 0x7e0 0x7e0 0x7e0
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   sched 0x7e0 0x7e0 0x7e0
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   sched 0x7e0 0x7e0 0x7e0
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   sched 0x7e0 0x7e0 0x7e0
   mov $r3 $r0 0xf
   imul u32 u32 hi $r0 $r0 $r2
   i2i u32 u32 $r2 neg $r1
   sched 0x7e0 0x7e0 0x7e0
   imad u32 u32 $r1 $r1 $r0 $r3
   isetp ge u32 and $p0 1 $r1 $r2 1
   $p0 iadd $r1 $r1 neg $r2
   sched 0x7e0 0x7e0 0x7e0
   $p0 iadd $r0 $r0 0x1
   $p0 isetp ge u32 and $p0 1 $r1 $r2 1
   $p0 iadd $r1 $r1 neg $r2
   sched 0x7e0 0x7e0 0x7e0
   $p0 iadd $r0 $r0 0x1
   ret
   nop 0

// DIV S32, like DIV U32 after taking ABS(inputs)
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p3
//
gm107_div_s32:
   sched 0x7e0 0x7e0 0x7e0
   isetp lt and $p2 0x1 $r0 0 1
   isetp lt xor $p3 1 $r1 0 $p2
   i2i s32 s32 $r0 abs $r0
   sched 0x7e0 0x7e0 0x7e0
   i2i s32 s32 $r1 abs $r1
   flo u32 $r2 $r1
   lop xor 1 $r2 $r2 0x1f
   sched 0x7e0 0x7e0 0x7e0
   mov $r3 0x1 0xf
   shl $r2 $r3 $r2
   i2i u32 u32 $r1 neg $r1
   sched 0x7e0 0x7e0 0x7e0
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   sched 0x7e0 0x7e0 0x7e0
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   sched 0x7e0 0x7e0 0x7e0
   imul u32 u32 $r3 $r1 $r2
   imad u32 u32 hi $r2 $r2 $r3 $r2
   imul u32 u32 $r3 $r1 $r2
   sched 0x7e0 0x7e0 0x7e0
   imad u32 u32 hi $r2 $r2 $r3 $r2
   mov $r3 $r0 0xf
   imul u32 u32 hi $r0 $r0 $r2
   sched 0x7e0 0x7e0 0x7e0
   i2i u32 u32 $r2 neg $r1
   imad u32 u32 $r1 $r1 $r0 $r3
   isetp ge u32 and $p0 1 $r1 $r2 1
   sched 0x7e0 0x7e0 0x7e0
   $p0 iadd $r1 $r1 neg $r2
   $p0 iadd $r0 $r0 0x1
   $p0 isetp ge u32 and $p0 1 $r1 $r2 1
   sched 0x7e0 0x7e0 0x7e0
   $p0 iadd $r1 $r1 neg $r2
   $p0 iadd $r0 $r0 0x1
   $p3 i2i s32 s32 $r0 neg $r0
   sched 0x7e0 0x7e0 0x7e0
   $p2 i2i s32 s32 $r1 neg $r1
   ret
   nop 0

// STUB
gm107_rcp_f64:
gm107_rsq_f64:
   sched 0x7e0 0x7e0 0x7e0
   ret
   nop 0
   nop 0

.section #gm107_builtin_offsets
.b64 #gm107_div_u32
.b64 #gm107_div_s32
.b64 #gm107_rcp_f64
.b64 #gm107_rsq_f64
