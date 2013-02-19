//
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
sched 0x28 0x4 0x28 0x4 0x28 0x28 0x28
bfind u32 $r2 $r1
long xor b32 $r2 $r2 0x1f
long mov b32 $r3 0x1
shl b32 $r2 $r3 clamp $r2
long cvt u32 $r1 neg u32 $r1
long mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
sched 0x28 0x28 0x28 0x28 0x28 0x28 0x28
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
sched 0x4 0x28 0x4 0x28 0x28 0x2c 0x4
add $r2 (mul high u32 $r2 u32 $r3) $r2
mov b32 $r3 $r0
mul high $r0 u32 $r0 u32 $r2
long cvt u32 $r2 neg u32 $r1
long add $r1 (mul u32 $r1 u32 $r0) $r3
set $p0 0x1 ge u32 $r1 $r2
$p0 sub b32 $r1 $r1 $r2
sched 0x28 0x2c 0x4 0x20 0x2e 0x28 0x20
$p0 add b32 $r0 $r0 0x1
$p0 set $p0 0x1 ge u32 $r1 $r2
$p0 sub b32 $r1 $r1 $r2
$p0 add b32 $r0 $r0 0x1
long ret
//
// DIV S32, like DIV U32 after taking ABS(inputs)
//
// INPUT:   $r0: dividend, $r1: divisor
// OUTPUT:  $r0: result, $r1: modulus
// CLOBBER: $r2 - $r3, $p0 - $p3
//
set $p2 0x1 lt s32 $r0 0x0
set $p3 0x1 lt s32 $r1 0x0 xor $p2
sched 0x20 0x28 0x28 0x4 0x28 0x04 0x28
long cvt s32 $r0 abs s32 $r0
long cvt s32 $r1 abs s32 $r1
bfind u32 $r2 $r1
long xor b32 $r2 $r2 0x1f
long mov b32 $r3 0x1
shl b32 $r2 $r3 clamp $r2
cvt u32 $r1 neg u32 $r1
sched 0x28 0x28 0x28 0x28 0x28 0x28 0x28
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
sched 0x28 0x28 0x4 0x28 0x04 0x28 0x28
add $r2 (mul high u32 $r2 u32 $r3) $r2
mul $r3 u32 $r1 u32 $r2
add $r2 (mul high u32 $r2 u32 $r3) $r2
mov b32 $r3 $r0
mul high $r0 u32 $r0 u32 $r2
long cvt u32 $r2 neg u32 $r1
long add $r1 (mul u32 $r1 u32 $r0) $r3
sched 0x2c 0x04 0x28 0x2c 0x04 0x28 0x20
set $p0 0x1 ge u32 $r1 $r2
$p0 sub b32 $r1 $r1 $r2
$p0 add b32 $r0 $r0 0x1
$p0 set $p0 0x1 ge u32 $r1 $r2
$p0 sub b32 $r1 $r1 $r2
long $p0 add b32 $r0 $r0 0x1
long $p3 cvt s32 $r0 neg s32 $r0
sched 0x04 0x2e 0x04 0x28 0x04 0x20 0x2c
$p2 cvt s32 $r1 neg s32 $r1
long ret
//
// SULDP [for each format]
// $r4d: address
// $r2: surface info (format)
// $p0: access predicate
// $p1, $p2: caching predicate (00: cv, 01: ca, 10: cg)
//
// RGBA32
$p1 suldgb b128 $r0q ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b128 $r0q cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b128 $r0q cv zero u8 g[$r4d] $r2 $p0
long ret
// RGBA16_UNORM
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b128 $r0q ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b128 $r0q cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b128 $r0q cv zero u8 g[$r4d] $r2 $p0
cvt rn f32 $r3 u16 1 $r1
cvt rn f32 $r2 u16 0 $r1
mul f32 $r3 $r3 0x37800074
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt rn f32 $r1 u16 1 $r0
mul f32 $r2 $r2 0x37800074
cvt rn f32 $r0 u16 0 $r0
mul f32 $r1 $r1 0x37800074
mul f32 $r0 $r0 0x37800074
long ret
// RGBA16_SNORM
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
cvt rn f32 $r3 s16 1 $r1
cvt rn f32 $r2 s16 0 $r1
mul f32 $r3 $r3 0x38000187
cvt rn f32 $r1 s16 1 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r2 $r2 0x38000187
cvt rn f32 $r0 s16 0 $r0
mul f32 $r1 $r1 0x38000187
mul f32 $r0 $r0 0x38000187
long ret
// RGBA16_SINT
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
cvt s32 $r3 s16 1 $r1
cvt s32 $r2 s16 0 $r1
cvt s32 $r1 s16 1 $r0
cvt s32 $r0 s16 0 $r0
long ret
// RGBA16_UINT
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
cvt u32 $r3 u16 1 $r1
cvt u32 $r2 u16 0 $r1
cvt u32 $r1 u16 1 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt u32 $r0 u16 0 $r0
long ret
// RGBA16_FLOAT
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
cvt f32 $r3 f16 $r1 1
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt f32 $r2 f16 $r1 0
cvt f32 $r1 f16 $r0 1
cvt f32 $r0 f16 $r0 0
long ret
// RG32_FLOAT
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r2 0x00000000
long mov b32 $r3 0x3f800000
long ret
// RG32_xINT
$p1 suldgb b64 $r0d ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b64 $r0d cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b64 $r0d cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r2 0x00000000
long mov b32 $r3 0x00000001
long ret
// RGB10A2_UNORM
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
ext u32 $r1 $r0 0x0a0a
long mov b32 $r3 0x3f800000
ext u32 $r2 $r0 0x0a14
long and b32 $r0 $r0 0x3ff
cvt rn f32 $r2 u16 0 $r2
cvt rn f32 $r1 u16 0 $r1
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r2 $r2 0x3a802007
cvt rn f32 $r0 u16 0 $r0
mul f32 $r1 $r1 0x3a802007
mul f32 $r0 $r0 0x3a802007
long ret
// RGB10A2_UINT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
ext u32 $r1 $r0 0x0a0a
long mov b32 $r3 0x00000001
ext u32 $r2 $r0 0x0a14
long and b32 $r0 $r0 0x3ff
long ret
// RGBA8_UNORM
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
cvt rn f32 $r3 u8 3 $r0
cvt rn f32 $r2 u8 2 $r0
mul f32 $r3 $r3 0x3b808081
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt rn f32 $r1 u8 1 $r0
mul f32 $r2 $r2 0x3b808081
cvt rn f32 $r0 u8 0 $r0
mul f32 $r1 $r1 0x3b808081
mul f32 $r0 $r0 0x3b808081
long ret
// RGBA8_SNORM
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
cvt rn f32 $r3 s8 3 $r0
cvt rn f32 $r2 s8 2 $r0
mul f32 $r3 $r3 0x3c010204
cvt rn f32 $r1 s8 1 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r2 $r2 0x3c010204
cvt rn f32 $r0 s8 0 $r0
mul f32 $r1 $r1 0x3c010204
mul f32 $r0 $r0 0x3c010204
long ret
// RGBA8_SINT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
cvt s32 $r3 s8 3 $r0
cvt s32 $r2 s8 2 $r0
cvt s32 $r1 s8 1 $r0
cvt s32 $r0 s8 0 $r0
long ret
// RGBA8_UINT
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
cvt u32 $r3 u8 3 $r0
cvt u32 $r2 u8 2 $r0
cvt u32 $r1 u8 1 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt u32 $r0 u8 0 $r0
long ret
// R5G6B5_UNORM
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
ext u32 $r1 $r0 0x0605
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long mov b32 $r3 0x3f800000
ext u32 $r2 $r0 0x050b
long and b32 $r0 $r0 0x1f
cvt rn f32 $r2 u8 0 $r2
cvt rn f32 $r1 u8 0 $r1
mul f32 $r2 $r2 0x3d042108
cvt rn f32 $r0 u8 0 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r1 $r1 0x3c820821
mul f32 $r0 $r0 0x3d042108
long ret
// R5G5B5X1_UNORM
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
ext u32 $r1 $r0 0x0505
ext u32 $r2 $r0 0x050a
long and b32 $r0 $r0 0x1f
long mov b32 $r3 0x3f800000
cvt rn f32 $r2 u8 0 $r2
cvt rn f32 $r1 u8 0 $r1
cvt rn f32 $r0 u8 0 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r2 $r2 0x3d042108
mul f32 $r1 $r1 0x3d042108
mul f32 $r0 $r0 0x3d042108
long ret
// RG16_UNORM
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
cvt rn f32 $r1 u16 1 $r0
cvt rn f32 $r0 u16 0 $r0
mul f32 $r1 $r1 0x37800074
mul f32 $r0 $r0 0x37800074
long mov b32 $r2 0x00000000
long mov b32 $r3 0x3f800000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long ret
// RG16_SNORM
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x3f800000
cvt rn f32 $r1 s16 1 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mov b32 $r2 0x00000000
cvt rn f32 $r0 s16 0 $r0
mul f32 $r1 $r1 0x38000187
mul f32 $r0 $r0 0x38000187
long ret
// RG16_SINT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x00000001
cvt s32 $r1 s16 1 $r0
mov b32 $r2 0x00000000
cvt s32 $r0 s16 0 $r0
long ret
// RG16_UINT
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x00000001
cvt u32 $r1 u16 1 $r0
mov b32 $r2 0x00000000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt u32 $r0 u16 0 $r0
long ret
// RG16_FLOAT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x3f800000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt f32 $r1 f16 $r0 1
mov b32 $r2 0x00000000
cvt f32 $r0 f16 $r0 0
long ret
// R32_FLOAT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x3f800000
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
long ret
// R32_xINT
$p1 suldgb b32 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb b32 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
long ret
// RG8_UNORM
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x3f800000
cvt rn f32 $r1 u8 1 $r0
mov b32 $r2 0x00000000
cvt rn f32 $r0 u8 0 $r0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r1 $r1 0x3b808081
mul f32 $r0 $r0 0x3b808081
long ret
// RG8_SNORM
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long mov b32 $r3 0x3f800000
cvt rn f32 $r1 s8 1 $r0
long mov b32 $r2 0x00000000
cvt rn f32 $r0 s8 0 $r0
mul f32 $r1 $r1 0x3c010204
mul f32 $r0 $r0 0x3c010204
long ret
// RG8_UINT
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
cvt u32 $r1 u8 1 $r0
long mov b32 $r2 0x00000000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt u32 $r0 u8 0 $r0
long ret
// RG8_SINT
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
cvt s32 $r1 s8 1 $r0
long mov b32 $r2 0x00000000
cvt s32 $r0 s8 0 $r0
long ret
// R16_UNORM
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x3f800000
cvt rn f32 $r0 u16 0 $r0
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
mul f32 $r0 $r0 0x37800074
long ret
// R16_SNORM
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x3f800000
cvt rn f32 $r0 s16 0 $r0
long mov b32 $r2 0x00000000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long mov b32 $r1 0x00000000
mul f32 $r0 $r0 0x38000187
long ret
// R16_SINT
$p1 suldgb s16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb s16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb s16 $r0 cv zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long mov b32 $r3 0x00000001
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
long ret
// R16_UINT
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
long ret
// R16_FLOAT
$p1 suldgb u16 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p2 suldgb u16 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u16 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x3f800000
long mov b32 $r2 0x00000000
cvt f32 $r0 f16 $r0 0
mov b32 $r1 0x00000000
long ret
// R8_UNORM
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u8 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u8 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u8 $r0 cv zero u8 g[$r4d] $r2 $p0
mov b32 $r3 0x3f800000
cvt rn f32 $r0 u8 0 $r0
mov b32 $r2 0x00000000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mul f32 $r0 $r0 0x3b808081
mov b32 $r1 0x00000000
long ret
// R8_SNORM
$p1 suldgb u8 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u8 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u8 $r0 cv zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
mov b32 $r3 0x3f800000
cvt rn f32 $r0 s8 0 $r0
mov b32 $r2 0x00000000
mul f32 $r0 $r0 0x3c010204
mov b32 $r1 0x00000000
long ret
// R8_SINT
$p1 suldgb s8 $r0 ca zero u8 g[$r4d] $r2 $p0
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
set $p1 0x1 $p1 xor not $p2
$p2 suldgb s8 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb s8 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
long ret
// R8_UINT
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
$p1 suldgb u8 $r0 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb u8 $r0 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb u8 $r0 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x00000001
long mov b32 $r2 0x00000000
long mov b32 $r1 0x00000000
sched 0x00 0x00 0x00 0x00 0x00 0x00 0x00
long ret
// R11G11B10_FLOAT TODO
$p1 suldgb b32 $r3 ca zero u8 g[$r4d] $r2 $p0
set $p1 0x1 $p1 xor not $p2
$p2 suldgb b32 $r3 cg zero u8 g[$r4d] $r2 $p0
$p1 suldgb b32 $r3 cv zero u8 g[$r4d] $r2 $p0
long mov b32 $r3 0x3f800000
long nop
long ret
//
// RCP F64: Newton Raphson reciprocal(x): r_{i+1} = r_i * (2.0 - x * r_i)
//
// INPUT:   $r0d (x)
// OUTPUT:  $r0d (rcp(x))
// CLOBBER: $r2 - $r7
// SIZE:    9 * 8 bytes
//
long nop
long ret
// RSQ F64: Newton Raphson rsqrt(x): r_{i+1} = r_i * (1.5 - 0.5 * x * r_i * r_i)
//
// INPUT:   $r0d (x)
// OUTPUT:  $r0d (rsqrt(x))
// CLOBBER: $r2 - $r7
// SIZE:    14 * 8 bytes
//
long nop
long ret
