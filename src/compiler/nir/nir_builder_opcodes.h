/* Copyright (C) 2015 Broadcom
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _NIR_BUILDER_OPCODES_
#define _NIR_BUILDER_OPCODES_



static inline nir_def *
nir_alignbyte_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_alignbyte_amd, src0, src1, src2);
}
static inline nir_def *
nir_amul(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_amul, src0, src1);
}
static inline nir_def *
nir_andg_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_andg_ir3, src0, src1, src2);
}
static inline nir_def *
nir_arshift_and_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_arshift_and_pan, src0, src1, src2);
}
static inline nir_def *
nir_arshift_or_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_arshift_or_pan, src0, src1, src2);
}
static inline nir_def *
nir_arshift_xor_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_arshift_xor_pan, src0, src1, src2);
}
static inline nir_def *
nir_b2b1(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 1)
      return src0;
   return nir_build_alu1(build, nir_op_b2b1, src0);
}
static inline nir_def *
nir_b2b32(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 32)
      return src0;
   return nir_build_alu1(build, nir_op_b2b32, src0);
}
static inline nir_def *
nir_b2f16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2f16, src0);
}
static inline nir_def *
nir_b2f32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2f32, src0);
}
static inline nir_def *
nir_b2f64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2f64, src0);
}
static inline nir_def *
nir_b2i1(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2i1, src0);
}
static inline nir_def *
nir_b2i16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2i16, src0);
}
static inline nir_def *
nir_b2i32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2i32, src0);
}
static inline nir_def *
nir_b2i64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2i64, src0);
}
static inline nir_def *
nir_b2i8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_b2i8, src0);
}
static inline nir_def *
nir_b32all_fequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal16, src0, src1);
}
static inline nir_def *
nir_b32all_fequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal2, src0, src1);
}
static inline nir_def *
nir_b32all_fequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal3, src0, src1);
}
static inline nir_def *
nir_b32all_fequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal4, src0, src1);
}
static inline nir_def *
nir_b32all_fequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal5, src0, src1);
}
static inline nir_def *
nir_b32all_fequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_fequal8, src0, src1);
}
static inline nir_def *
nir_b32all_iequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal16, src0, src1);
}
static inline nir_def *
nir_b32all_iequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal2, src0, src1);
}
static inline nir_def *
nir_b32all_iequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal3, src0, src1);
}
static inline nir_def *
nir_b32all_iequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal4, src0, src1);
}
static inline nir_def *
nir_b32all_iequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal5, src0, src1);
}
static inline nir_def *
nir_b32all_iequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32all_iequal8, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal16, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal2, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal3, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal4, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal5, src0, src1);
}
static inline nir_def *
nir_b32any_fnequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_fnequal8, src0, src1);
}
static inline nir_def *
nir_b32any_inequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal16, src0, src1);
}
static inline nir_def *
nir_b32any_inequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal2, src0, src1);
}
static inline nir_def *
nir_b32any_inequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal3, src0, src1);
}
static inline nir_def *
nir_b32any_inequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal4, src0, src1);
}
static inline nir_def *
nir_b32any_inequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal5, src0, src1);
}
static inline nir_def *
nir_b32any_inequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_b32any_inequal8, src0, src1);
}
static inline nir_def *
nir_b32csel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_b32csel, src0, src1, src2);
}
static inline nir_def *
nir_b32fcsel_mdg(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_b32fcsel_mdg, src0, src1, src2);
}
static inline nir_def *
nir_ball_fequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal16, src0, src1);
}
static inline nir_def *
nir_ball_fequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal2, src0, src1);
}
static inline nir_def *
nir_ball_fequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal3, src0, src1);
}
static inline nir_def *
nir_ball_fequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal4, src0, src1);
}
static inline nir_def *
nir_ball_fequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal5, src0, src1);
}
static inline nir_def *
nir_ball_fequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_fequal8, src0, src1);
}
static inline nir_def *
nir_ball_iequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal16, src0, src1);
}
static inline nir_def *
nir_ball_iequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal2, src0, src1);
}
static inline nir_def *
nir_ball_iequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal3, src0, src1);
}
static inline nir_def *
nir_ball_iequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal4, src0, src1);
}
static inline nir_def *
nir_ball_iequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal5, src0, src1);
}
static inline nir_def *
nir_ball_iequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ball_iequal8, src0, src1);
}
static inline nir_def *
nir_bany_fnequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal16, src0, src1);
}
static inline nir_def *
nir_bany_fnequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal2, src0, src1);
}
static inline nir_def *
nir_bany_fnequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal3, src0, src1);
}
static inline nir_def *
nir_bany_fnequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal4, src0, src1);
}
static inline nir_def *
nir_bany_fnequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal5, src0, src1);
}
static inline nir_def *
nir_bany_fnequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_fnequal8, src0, src1);
}
static inline nir_def *
nir_bany_inequal16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal16, src0, src1);
}
static inline nir_def *
nir_bany_inequal2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal2, src0, src1);
}
static inline nir_def *
nir_bany_inequal3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal3, src0, src1);
}
static inline nir_def *
nir_bany_inequal4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal4, src0, src1);
}
static inline nir_def *
nir_bany_inequal5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal5, src0, src1);
}
static inline nir_def *
nir_bany_inequal8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bany_inequal8, src0, src1);
}
static inline nir_def *
nir_bcsel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bcsel, src0, src1, src2);
}
static inline nir_def *
nir_bcsel_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bcsel_pan, src0, src1, src2);
}
static inline nir_def *
nir_bf2f(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_bf2f, src0);
}
static inline nir_def *
nir_bfdot16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot16, src0, src1);
}
static inline nir_def *
nir_bfdot2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot2, src0, src1);
}
static inline nir_def *
nir_bfdot2_bfadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bfdot2_bfadd, src0, src1, src2);
}
static inline nir_def *
nir_bfdot2_fadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bfdot2_fadd, src0, src1, src2);
}
static inline nir_def *
nir_bfdot3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot3, src0, src1);
}
static inline nir_def *
nir_bfdot4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot4, src0, src1);
}
static inline nir_def *
nir_bfdot5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot5, src0, src1);
}
static inline nir_def *
nir_bfdot8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfdot8, src0, src1);
}
static inline nir_def *
nir_bffma(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bffma, src0, src1, src2);
}
static inline nir_def *
nir_bffma_mixed_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bffma_mixed_intel, src0, src1, src2);
}
static inline nir_def *
nir_bfi(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bfi, src0, src1, src2);
}
static inline nir_def *
nir_bfm(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfm, src0, src1);
}
static inline nir_def *
nir_bfmul(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfmul, src0, src1);
}
static inline nir_def *
nir_bfmul_mixed_intel(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bfmul_mixed_intel, src0, src1);
}
static inline nir_def *
nir_bit_count(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_bit_count, src0);
}
static inline nir_def *
nir_bitfield_insert(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_bitfield_insert, src0, src1, src2, src3);
}
static inline nir_def *
nir_bitfield_insert_etna(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bitfield_insert_etna, src0, src1, src2);
}
static inline nir_def *
nir_bitfield_reverse(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_bitfield_reverse, src0);
}
static inline nir_def *
nir_bitfield_select(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bitfield_select, src0, src1, src2);
}
static inline nir_def *
nir_bitnz(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bitnz, src0, src1);
}
static inline nir_def *
nir_bitz(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_bitz, src0, src1);
}
static inline nir_def *
nir_bounds_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_bounds_agx, src0, src1, src2);
}
static inline nir_def *
nir_byte_perm_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_byte_perm_amd, src0, src1, src2);
}
static inline nir_def *
nir_cube_amd(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_cube_amd, src0);
}
static inline nir_def *
nir_e4m3fn2f(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_e4m3fn2f, src0);
}
static inline nir_def *
nir_e4m3fn_dot4_fadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_e4m3fn_dot4_fadd, src0, src1, src2);
}
static inline nir_def *
nir_e4m3fn_e5m2_dot4_fadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_e4m3fn_e5m2_dot4_fadd, src0, src1, src2);
}
static inline nir_def *
nir_e5m22f(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_e5m22f, src0);
}
static inline nir_def *
nir_e5m2_dot4_fadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_e5m2_dot4_fadd, src0, src1, src2);
}
static inline nir_def *
nir_extr_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_extr_agx, src0, src1, src2, src3);
}
static inline nir_def *
nir_extract_i16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_extract_i16, src0, src1);
}
static inline nir_def *
nir_extract_i8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_extract_i8, src0, src1);
}
static inline nir_def *
nir_extract_u16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_extract_u16, src0, src1);
}
static inline nir_def *
nir_extract_u8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_extract_u8, src0, src1);
}
static inline nir_def *
nir_f16dot2_fadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_f16dot2_fadd, src0, src1, src2);
}
static inline nir_def *
nir_f2bf(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2bf, src0);
}
static inline nir_def *
nir_f2e4m3fn(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2e4m3fn, src0);
}
static inline nir_def *
nir_f2e4m3fn_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2e4m3fn_sat, src0);
}
static inline nir_def *
nir_f2e4m3fn_satfn(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2e4m3fn_satfn, src0);
}
static inline nir_def *
nir_f2e5m2(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2e5m2, src0);
}
static inline nir_def *
nir_f2e5m2_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2e5m2_sat, src0);
}
static inline nir_def *
nir_f2f16(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_f2f16, src0);
}
static inline nir_def *
nir_f2f16_rd(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_f2f16_rd, src0);
}
static inline nir_def *
nir_f2f16_rtne(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_f2f16_rtne, src0);
}
static inline nir_def *
nir_f2f16_rtz(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_f2f16_rtz, src0);
}
static inline nir_def *
nir_f2f16_ru(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_f2f16_ru, src0);
}
static inline nir_def *
nir_f2f32(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 32)
      return src0;
   return nir_build_alu1(build, nir_op_f2f32, src0);
}
static inline nir_def *
nir_f2f64(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 64)
      return src0;
   return nir_build_alu1(build, nir_op_f2f64, src0);
}
static inline nir_def *
nir_f2fmp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2fmp, src0);
}
static inline nir_def *
nir_f2i1(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i1, src0);
}
static inline nir_def *
nir_f2i16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i16, src0);
}
static inline nir_def *
nir_f2i16_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i16_sat, src0);
}
static inline nir_def *
nir_f2i32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i32, src0);
}
static inline nir_def *
nir_f2i32_rtne(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i32_rtne, src0);
}
static inline nir_def *
nir_f2i32_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i32_sat, src0);
}
static inline nir_def *
nir_f2i64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i64, src0);
}
static inline nir_def *
nir_f2i8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i8, src0);
}
static inline nir_def *
nir_f2i8_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2i8_sat, src0);
}
static inline nir_def *
nir_f2imp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2imp, src0);
}
static inline nir_def *
nir_f2snorm_16_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2snorm_16_v3d, src0);
}
static inline nir_def *
nir_f2u1(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u1, src0);
}
static inline nir_def *
nir_f2u16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u16, src0);
}
static inline nir_def *
nir_f2u16_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u16_sat, src0);
}
static inline nir_def *
nir_f2u32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u32, src0);
}
static inline nir_def *
nir_f2u32_rtne(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u32_rtne, src0);
}
static inline nir_def *
nir_f2u32_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u32_sat, src0);
}
static inline nir_def *
nir_f2u64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u64, src0);
}
static inline nir_def *
nir_f2u8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u8, src0);
}
static inline nir_def *
nir_f2u8_sat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2u8_sat, src0);
}
static inline nir_def *
nir_f2ump(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2ump, src0);
}
static inline nir_def *
nir_f2unorm_16_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_f2unorm_16_v3d, src0);
}
static inline nir_def *
nir_fabs(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fabs, src0);
}
static inline nir_def *
nir_fadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fadd, src0, src1);
}
static inline nir_def *
nir_fcanonicalize(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fcanonicalize, src0);
}
static inline nir_def *
nir_fceil(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fceil, src0);
}
static inline nir_def *
nir_fclamp_pos(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fclamp_pos, src0);
}
static inline nir_def *
nir_fcopysign_pco(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fcopysign_pco, src0, src1);
}
static inline nir_def *
nir_fcos(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fcos, src0);
}
static inline nir_def *
nir_fcos_mdg(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fcos_mdg, src0);
}
static inline nir_def *
nir_fcos_normalized_2_pi(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fcos_normalized_2_pi, src0);
}
static inline nir_def *
nir_fcsel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_fcsel, src0, src1, src2);
}
static inline nir_def *
nir_fcsel_ge(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_fcsel_ge, src0, src1, src2);
}
static inline nir_def *
nir_fcsel_gt(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_fcsel_gt, src0, src1, src2);
}
static inline nir_def *
nir_fdiv(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdiv, src0, src1);
}
static inline nir_def *
nir_fdot16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot16, src0, src1);
}
static inline nir_def *
nir_fdot2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot2, src0, src1);
}
static inline nir_def *
nir_fdot3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot3, src0, src1);
}
static inline nir_def *
nir_fdot4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot4, src0, src1);
}
static inline nir_def *
nir_fdot5(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot5, src0, src1);
}
static inline nir_def *
nir_fdot8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdot8, src0, src1);
}
static inline nir_def *
nir_fdph(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fdph, src0, src1);
}
static inline nir_def *
nir_feq(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_feq, src0, src1);
}
static inline nir_def *
nir_feq32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_feq32, src0, src1);
}
static inline nir_def *
nir_feq_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_feq_pan, src0, src1);
}
static inline nir_def *
nir_fequ(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fequ, src0, src1);
}
static inline nir_def *
nir_fequ32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fequ32, src0, src1);
}
static inline nir_def *
nir_fequ_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fequ_pan, src0, src1);
}
static inline nir_def *
nir_fexp2(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fexp2, src0);
}
static inline nir_def *
nir_ffloor(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ffloor, src0);
}
static inline nir_def *
nir_ffma(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ffma, src0, src1, src2);
}
static inline nir_def *
nir_ffma_weak(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ffma_weak, src0, src1, src2);
}
static inline nir_def *
nir_ffmaz(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ffmaz, src0, src1, src2);
}
static inline nir_def *
nir_ffract(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ffract, src0);
}
static inline nir_def *
nir_fge(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fge, src0, src1);
}
static inline nir_def *
nir_fge32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fge32, src0, src1);
}
static inline nir_def *
nir_fge_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fge_pan, src0, src1);
}
static inline nir_def *
nir_fgeu(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fgeu, src0, src1);
}
static inline nir_def *
nir_fgeu32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fgeu32, src0, src1);
}
static inline nir_def *
nir_fgeu_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fgeu_pan, src0, src1);
}
static inline nir_def *
nir_find_lsb(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_find_lsb, src0);
}
static inline nir_def *
nir_fisfinite(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fisfinite, src0);
}
static inline nir_def *
nir_fisfinite32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fisfinite32, src0);
}
static inline nir_def *
nir_fisnormal(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fisnormal, src0);
}
static inline nir_def *
nir_flog2(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_flog2, src0);
}
static inline nir_def *
nir_flrp(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_flrp, src0, src1, src2);
}
static inline nir_def *
nir_flt(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_flt, src0, src1);
}
static inline nir_def *
nir_flt32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_flt32, src0, src1);
}
static inline nir_def *
nir_flt_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_flt_pan, src0, src1);
}
static inline nir_def *
nir_fltu(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fltu, src0, src1);
}
static inline nir_def *
nir_fltu32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fltu32, src0, src1);
}
static inline nir_def *
nir_fltu_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fltu_pan, src0, src1);
}
static inline nir_def *
nir_fmad(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_fmad, src0, src1, src2);
}
static inline nir_def *
nir_fmadz(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_fmadz, src0, src1, src2);
}
static inline nir_def *
nir_fmax(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmax, src0, src1);
}
static inline nir_def *
nir_fmax_agx(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmax_agx, src0, src1);
}
static inline nir_def *
nir_fmin(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmin, src0, src1);
}
static inline nir_def *
nir_fmin_agx(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmin_agx, src0, src1);
}
static inline nir_def *
nir_fmod(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmod, src0, src1);
}
static inline nir_def *
nir_fmul(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmul, src0, src1);
}
static inline nir_def *
nir_fmul_rtz(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmul_rtz, src0, src1);
}
static inline nir_def *
nir_fmulz(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fmulz, src0, src1);
}
static inline nir_def *
nir_fneg(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fneg, src0);
}
static inline nir_def *
nir_fneo(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneo, src0, src1);
}
static inline nir_def *
nir_fneo32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneo32, src0, src1);
}
static inline nir_def *
nir_fneo_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneo_pan, src0, src1);
}
static inline nir_def *
nir_fneu(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneu, src0, src1);
}
static inline nir_def *
nir_fneu32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneu32, src0, src1);
}
static inline nir_def *
nir_fneu_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fneu_pan, src0, src1);
}
static inline nir_def *
nir_ford(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ford, src0, src1);
}
static inline nir_def *
nir_ford32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ford32, src0, src1);
}
static inline nir_def *
nir_ford_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ford_pan, src0, src1);
}
static inline nir_def *
nir_fpow(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fpow, src0, src1);
}
static inline nir_def *
nir_fquantize2f16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fquantize2f16, src0);
}
static inline nir_def *
nir_frcp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_frcp, src0);
}
static inline nir_def *
nir_frem(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_frem, src0, src1);
}
static inline nir_def *
nir_frexp_exp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_frexp_exp, src0);
}
static inline nir_def *
nir_frexp_sig(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_frexp_sig, src0);
}
static inline nir_def *
nir_fround_even(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fround_even, src0);
}
static inline nir_def *
nir_frsq(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_frsq, src0);
}
static inline nir_def *
nir_fsat(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsat, src0);
}
static inline nir_def *
nir_fsat_signed(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsat_signed, src0);
}
static inline nir_def *
nir_fsign(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsign, src0);
}
static inline nir_def *
nir_fsin(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsin, src0);
}
static inline nir_def *
nir_fsin_agx(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsin_agx, src0);
}
static inline nir_def *
nir_fsin_mdg(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsin_mdg, src0);
}
static inline nir_def *
nir_fsin_normalized_2_pi(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsin_normalized_2_pi, src0);
}
static inline nir_def *
nir_fsqrt(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsqrt, src0);
}
static inline nir_def *
nir_fsub(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_fsub, src0, src1);
}
static inline nir_def *
nir_fsum2(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsum2, src0);
}
static inline nir_def *
nir_fsum3(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsum3, src0);
}
static inline nir_def *
nir_fsum4(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_fsum4, src0);
}
static inline nir_def *
nir_ftrunc(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ftrunc, src0);
}
static inline nir_def *
nir_funord(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_funord, src0, src1);
}
static inline nir_def *
nir_funord32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_funord32, src0, src1);
}
static inline nir_def *
nir_funord_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_funord_pan, src0, src1);
}
static inline nir_def *
nir_i2f16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_i2f16, src0);
}
static inline nir_def *
nir_i2f32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_i2f32, src0);
}
static inline nir_def *
nir_i2f64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_i2f64, src0);
}
static inline nir_def *
nir_i2fmp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_i2fmp, src0);
}
static inline nir_def *
nir_i2i1(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 1)
      return src0;
   return nir_build_alu1(build, nir_op_i2i1, src0);
}
static inline nir_def *
nir_i2i16(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_i2i16, src0);
}
static inline nir_def *
nir_i2i32(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 32)
      return src0;
   return nir_build_alu1(build, nir_op_i2i32, src0);
}
static inline nir_def *
nir_i2i64(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 64)
      return src0;
   return nir_build_alu1(build, nir_op_i2i64, src0);
}
static inline nir_def *
nir_i2i8(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 8)
      return src0;
   return nir_build_alu1(build, nir_op_i2i8, src0);
}
static inline nir_def *
nir_i2imp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_i2imp, src0);
}
static inline nir_def *
nir_i32csel_ge(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_i32csel_ge, src0, src1, src2);
}
static inline nir_def *
nir_i32csel_gt(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_i32csel_gt, src0, src1, src2);
}
static inline nir_def *
nir_iabs(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_iabs, src0);
}
static inline nir_def *
nir_iadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_iadd, src0, src1);
}
static inline nir_def *
nir_iadd3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_iadd3, src0, src1, src2);
}
static inline nir_def *
nir_iadd_sat(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_iadd_sat, src0, src1);
}
static inline nir_def *
nir_iand(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_iand, src0, src1);
}
static inline nir_def *
nir_ibfe(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ibfe, src0, src1, src2);
}
static inline nir_def *
nir_ibitfield_extract(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ibitfield_extract, src0, src1, src2);
}
static inline nir_def *
nir_icsel_eqz(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_icsel_eqz, src0, src1, src2);
}
static inline nir_def *
nir_idiv(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_idiv, src0, src1);
}
static inline nir_def *
nir_ieq(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ieq, src0, src1);
}
static inline nir_def *
nir_ieq32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ieq32, src0, src1);
}
static inline nir_def *
nir_ieq_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ieq_pan, src0, src1);
}
static inline nir_def *
nir_ifind_msb(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ifind_msb, src0);
}
static inline nir_def *
nir_ifind_msb_rev(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ifind_msb_rev, src0);
}
static inline nir_def *
nir_ige(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ige, src0, src1);
}
static inline nir_def *
nir_ige32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ige32, src0, src1);
}
static inline nir_def *
nir_ige_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ige_pan, src0, src1);
}
static inline nir_def *
nir_ihadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ihadd, src0, src1);
}
static inline nir_def *
nir_ilea_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ilea_agx, src0, src1, src2);
}
static inline nir_def *
nir_ilt(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ilt, src0, src1);
}
static inline nir_def *
nir_ilt32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ilt32, src0, src1);
}
static inline nir_def *
nir_ilt_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ilt_pan, src0, src1);
}
static inline nir_def *
nir_imad(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_imad, src0, src1, src2);
}
static inline nir_def *
nir_imad24_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_imad24_ir3, src0, src1, src2);
}
static inline nir_def *
nir_imadsh_mix16(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_imadsh_mix16, src0, src1, src2);
}
static inline nir_def *
nir_imadshl_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_imadshl_agx, src0, src1, src2, src3);
}
static inline nir_def *
nir_imax(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imax, src0, src1);
}
static inline nir_def *
nir_imin(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imin, src0, src1);
}
static inline nir_def *
nir_imod(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imod, src0, src1);
}
static inline nir_def *
nir_imsubshl_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_imsubshl_agx, src0, src1, src2, src3);
}
static inline nir_def *
nir_imul(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul, src0, src1);
}
static inline nir_def *
nir_imul24(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul24, src0, src1);
}
static inline nir_def *
nir_imul24_relaxed(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul24_relaxed, src0, src1);
}
static inline nir_def *
nir_imul_2x32_64(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul_2x32_64, src0, src1);
}
static inline nir_def *
nir_imul_32x16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul_32x16, src0, src1);
}
static inline nir_def *
nir_imul_high(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_imul_high, src0, src1);
}
static inline nir_def *
nir_ine(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ine, src0, src1);
}
static inline nir_def *
nir_ine32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ine32, src0, src1);
}
static inline nir_def *
nir_ine_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ine_pan, src0, src1);
}
static inline nir_def *
nir_ineg(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ineg, src0);
}
static inline nir_def *
nir_inot(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_inot, src0);
}
static inline nir_def *
nir_insert_u16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_insert_u16, src0, src1);
}
static inline nir_def *
nir_insert_u8(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_insert_u8, src0, src1);
}
static inline nir_def *
nir_interleave(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_interleave, src0, src1);
}
static inline nir_def *
nir_ior(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ior, src0, src1);
}
static inline nir_def *
nir_irem(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_irem, src0, src1);
}
static inline nir_def *
nir_irhadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_irhadd, src0, src1);
}
static inline nir_def *
nir_ishl(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ishl, src0, src1);
}
static inline nir_def *
nir_ishr(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ishr, src0, src1);
}
static inline nir_def *
nir_isign(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_isign, src0);
}
static inline nir_def *
nir_isub(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_isub, src0, src1);
}
static inline nir_def *
nir_isub_sat(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_isub_sat, src0, src1);
}
static inline nir_def *
nir_ixor(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ixor, src0, src1);
}
static inline nir_def *
nir_ldexp(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ldexp, src0, src1);
}
static inline nir_def *
nir_ldexp16_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ldexp16_pan, src0, src1);
}
static inline nir_def *
nir_lea_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lea_nv, src0, src1, src2);
}
static inline nir_def *
nir_lrot_and_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lrot_and_pan, src0, src1, src2);
}
static inline nir_def *
nir_lrot_or_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lrot_or_pan, src0, src1, src2);
}
static inline nir_def *
nir_lrot_xor_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lrot_xor_pan, src0, src1, src2);
}
static inline nir_def *
nir_lshift_and_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lshift_and_pan, src0, src1, src2);
}
static inline nir_def *
nir_lshift_or_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lshift_or_pan, src0, src1, src2);
}
static inline nir_def *
nir_lshift_xor_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_lshift_xor_pan, src0, src1, src2);
}
static inline nir_def *
nir_mov(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_mov, src0);
}
static inline nir_def *
nir_mqsad_4x8(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_mqsad_4x8, src0, src1, src2);
}
static inline nir_def *
nir_msad_4x8(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_msad_4x8, src0, src1, src2);
}
static inline nir_def *
nir_pack_2x16_to_snorm_2x8_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_2x16_to_snorm_2x8_v3d, src0);
}
static inline nir_def *
nir_pack_2x16_to_unorm_10_2_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_2x16_to_unorm_10_2_v3d, src0);
}
static inline nir_def *
nir_pack_2x16_to_unorm_2x10_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_2x16_to_unorm_2x10_v3d, src0);
}
static inline nir_def *
nir_pack_2x16_to_unorm_2x8_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_2x16_to_unorm_2x8_v3d, src0);
}
static inline nir_def *
nir_pack_2x32_to_2x16_v3d(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_2x32_to_2x16_v3d, src0, src1);
}
static inline nir_def *
nir_pack_32_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_32_2x16, src0);
}
static inline nir_def *
nir_pack_32_2x16_split(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_32_2x16_split, src0, src1);
}
static inline nir_def *
nir_pack_32_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_32_4x8, src0);
}
static inline nir_def *
nir_pack_32_4x8_split(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_pack_32_4x8_split, src0, src1, src2, src3);
}
static inline nir_def *
nir_pack_32_to_r11g11b10_v3d(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_32_to_r11g11b10_v3d, src0, src1);
}
static inline nir_def *
nir_pack_4x16_to_4x8_v3d(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_4x16_to_4x8_v3d, src0, src1);
}
static inline nir_def *
nir_pack_64_2x32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_64_2x32, src0);
}
static inline nir_def *
nir_pack_64_2x32_split(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_64_2x32_split, src0, src1);
}
static inline nir_def *
nir_pack_64_4x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_64_4x16, src0);
}
static inline nir_def *
nir_pack_double_2x32_dxil(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_double_2x32_dxil, src0);
}
static inline nir_def *
nir_pack_half_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_half_2x16, src0);
}
static inline nir_def *
nir_pack_half_2x16_rtz_split(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_half_2x16_rtz_split, src0, src1);
}
static inline nir_def *
nir_pack_half_2x16_split(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_half_2x16_split, src0, src1);
}
static inline nir_def *
nir_pack_sint_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_sint_2x16, src0);
}
static inline nir_def *
nir_pack_snorm_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_snorm_2x16, src0);
}
static inline nir_def *
nir_pack_snorm_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_snorm_4x8, src0);
}
static inline nir_def *
nir_pack_uint_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_uint_2x16, src0);
}
static inline nir_def *
nir_pack_uint_32_to_r10g10b10a2_v3d(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_pack_uint_32_to_r10g10b10a2_v3d, src0, src1);
}
static inline nir_def *
nir_pack_unorm_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_unorm_2x16, src0);
}
static inline nir_def *
nir_pack_unorm_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_unorm_4x8, src0);
}
static inline nir_def *
nir_pack_uvec2_to_uint(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_uvec2_to_uint, src0);
}
static inline nir_def *
nir_pack_uvec4_to_uint(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_pack_uvec4_to_uint, src0);
}
static inline nir_def *
nir_prmt_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_prmt_nv, src0, src1, src2);
}
static inline nir_def *
nir_rrot_and_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rrot_and_pan, src0, src1, src2);
}
static inline nir_def *
nir_rrot_or_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rrot_or_pan, src0, src1, src2);
}
static inline nir_def *
nir_rrot_xor_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rrot_xor_pan, src0, src1, src2);
}
static inline nir_def *
nir_rshift_and_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rshift_and_pan, src0, src1, src2);
}
static inline nir_def *
nir_rshift_or_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rshift_or_pan, src0, src1, src2);
}
static inline nir_def *
nir_rshift_xor_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_rshift_xor_pan, src0, src1, src2);
}
static inline nir_def *
nir_sdot_2x16_iadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sdot_2x16_iadd, src0, src1, src2);
}
static inline nir_def *
nir_sdot_2x16_iadd_sat(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sdot_2x16_iadd_sat, src0, src1, src2);
}
static inline nir_def *
nir_sdot_4x8_iadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sdot_4x8_iadd, src0, src1, src2);
}
static inline nir_def *
nir_sdot_4x8_iadd_sat(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sdot_4x8_iadd_sat, src0, src1, src2);
}
static inline nir_def *
nir_seq(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_seq, src0, src1);
}
static inline nir_def *
nir_sge(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_sge, src0, src1);
}
static inline nir_def *
nir_shfr(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_shfr, src0, src1, src2);
}
static inline nir_def *
nir_shlg_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_shlg_ir3, src0, src1, src2);
}
static inline nir_def *
nir_shlm_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_shlm_ir3, src0, src1, src2);
}
static inline nir_def *
nir_shrg_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_shrg_ir3, src0, src1, src2);
}
static inline nir_def *
nir_shrm_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_shrm_ir3, src0, src1, src2);
}
static inline nir_def *
nir_slt(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_slt, src0, src1);
}
static inline nir_def *
nir_sne(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_sne, src0, src1);
}
static inline nir_def *
nir_snorm2f_16_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_snorm2f_16_v3d, src0);
}
static inline nir_def *
nir_sudot_4x8_iadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sudot_4x8_iadd, src0, src1, src2);
}
static inline nir_def *
nir_sudot_4x8_iadd_sat(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_sudot_4x8_iadd_sat, src0, src1, src2);
}
static inline nir_def *
nir_u2f16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_u2f16, src0);
}
static inline nir_def *
nir_u2f32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_u2f32, src0);
}
static inline nir_def *
nir_u2f64(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_u2f64, src0);
}
static inline nir_def *
nir_u2fmp(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_u2fmp, src0);
}
static inline nir_def *
nir_u2u1(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 1)
      return src0;
   return nir_build_alu1(build, nir_op_u2u1, src0);
}
static inline nir_def *
nir_u2u16(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 16)
      return src0;
   return nir_build_alu1(build, nir_op_u2u16, src0);
}
static inline nir_def *
nir_u2u32(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 32)
      return src0;
   return nir_build_alu1(build, nir_op_u2u32, src0);
}
static inline nir_def *
nir_u2u64(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 64)
      return src0;
   return nir_build_alu1(build, nir_op_u2u64, src0);
}
static inline nir_def *
nir_u2u8(nir_builder *build, nir_def *src0)
{
   if (src0->bit_size == 8)
      return src0;
   return nir_build_alu1(build, nir_op_u2u8, src0);
}
static inline nir_def *
nir_uabs_isub(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uabs_isub, src0, src1);
}
static inline nir_def *
nir_uabs_usub(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uabs_usub, src0, src1);
}
static inline nir_def *
nir_uadd64_32(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_uadd64_32, src0, src1, src2);
}
static inline nir_def *
nir_uadd_carry(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uadd_carry, src0, src1);
}
static inline nir_def *
nir_uadd_sat(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uadd_sat, src0, src1);
}
static inline nir_def *
nir_ubfe(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ubfe, src0, src1, src2);
}
static inline nir_def *
nir_ubitfield_extract(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ubitfield_extract, src0, src1, src2);
}
static inline nir_def *
nir_uclz(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_uclz, src0);
}
static inline nir_def *
nir_udiv(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_udiv, src0, src1);
}
static inline nir_def *
nir_udiv_aligned_4(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_udiv_aligned_4, src0);
}
static inline nir_def *
nir_udot_2x16_uadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_udot_2x16_uadd, src0, src1, src2);
}
static inline nir_def *
nir_udot_2x16_uadd_sat(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_udot_2x16_uadd_sat, src0, src1, src2);
}
static inline nir_def *
nir_udot_4x8_uadd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_udot_4x8_uadd, src0, src1, src2);
}
static inline nir_def *
nir_udot_4x8_uadd_sat(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_udot_4x8_uadd_sat, src0, src1, src2);
}
static inline nir_def *
nir_ufind_msb(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ufind_msb, src0);
}
static inline nir_def *
nir_ufind_msb_rev(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_ufind_msb_rev, src0);
}
static inline nir_def *
nir_uge(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uge, src0, src1);
}
static inline nir_def *
nir_uge32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uge32, src0, src1);
}
static inline nir_def *
nir_uge_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uge_pan, src0, src1);
}
static inline nir_def *
nir_uhadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uhadd, src0, src1);
}
static inline nir_def *
nir_ulea_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_ulea_agx, src0, src1, src2);
}
static inline nir_def *
nir_ult(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ult, src0, src1);
}
static inline nir_def *
nir_ult32(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ult32, src0, src1);
}
static inline nir_def *
nir_ult_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ult_pan, src0, src1);
}
static inline nir_def *
nir_umad24(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_umad24, src0, src1, src2);
}
static inline nir_def *
nir_umad24_relaxed(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_umad24_relaxed, src0, src1, src2);
}
static inline nir_def *
nir_umad64_32(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_umad64_32, src0, src1, src2, src3);
}
static inline nir_def *
nir_umax(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umax, src0, src1);
}
static inline nir_def *
nir_umax_4x8_vc4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umax_4x8_vc4, src0, src1);
}
static inline nir_def *
nir_umin(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umin, src0, src1);
}
static inline nir_def *
nir_umin_4x8_vc4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umin_4x8_vc4, src0, src1);
}
static inline nir_def *
nir_umod(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umod, src0, src1);
}
static inline nir_def *
nir_umul24(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul24, src0, src1);
}
static inline nir_def *
nir_umul24_relaxed(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul24_relaxed, src0, src1);
}
static inline nir_def *
nir_umul_16x16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul_16x16, src0, src1);
}
static inline nir_def *
nir_umul_2x32_64(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul_2x32_64, src0, src1);
}
static inline nir_def *
nir_umul_32x16(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul_32x16, src0, src1);
}
static inline nir_def *
nir_umul_high(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul_high, src0, src1);
}
static inline nir_def *
nir_umul_unorm_4x8_vc4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_umul_unorm_4x8_vc4, src0, src1);
}
static inline nir_def *
nir_unorm2f_16_v3d(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unorm2f_16_v3d, src0);
}
static inline nir_def *
nir_unpack_32_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_32_2x16, src0);
}
static inline nir_def *
nir_unpack_32_2x16_split_x(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_32_2x16_split_x, src0);
}
static inline nir_def *
nir_unpack_32_2x16_split_y(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_32_2x16_split_y, src0);
}
static inline nir_def *
nir_unpack_32_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_32_4x8, src0);
}
static inline nir_def *
nir_unpack_64_2x32(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_64_2x32, src0);
}
static inline nir_def *
nir_unpack_64_2x32_split_x(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_64_2x32_split_x, src0);
}
static inline nir_def *
nir_unpack_64_2x32_split_y(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_64_2x32_split_y, src0);
}
static inline nir_def *
nir_unpack_64_4x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_64_4x16, src0);
}
static inline nir_def *
nir_unpack_double_2x32_dxil(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_double_2x32_dxil, src0);
}
static inline nir_def *
nir_unpack_half_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_half_2x16, src0);
}
static inline nir_def *
nir_unpack_half_x_dxil(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_half_x_dxil, src0);
}
static inline nir_def *
nir_unpack_snorm_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_snorm_2x16, src0);
}
static inline nir_def *
nir_unpack_snorm_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_snorm_4x8, src0);
}
static inline nir_def *
nir_unpack_unorm_2x16(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_unorm_2x16, src0);
}
static inline nir_def *
nir_unpack_unorm_4x8(nir_builder *build, nir_def *src0)
{
   return nir_build_alu1(build, nir_op_unpack_unorm_4x8, src0);
}
static inline nir_def *
nir_urhadd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_urhadd, src0, src1);
}
static inline nir_def *
nir_urol(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_urol, src0, src1);
}
static inline nir_def *
nir_uror(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_uror, src0, src1);
}
static inline nir_def *
nir_usadd_4x8_vc4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_usadd_4x8_vc4, src0, src1);
}
static inline nir_def *
nir_ushr(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ushr, src0, src1);
}
static inline nir_def *
nir_ussub_4x8_vc4(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_ussub_4x8_vc4, src0, src1);
}
static inline nir_def *
nir_usub_borrow(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_usub_borrow, src0, src1);
}
static inline nir_def *
nir_usub_sat(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_usub_sat, src0, src1);
}
static inline nir_def *
nir_vec16(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6, nir_def *src7, nir_def *src8, nir_def *src9, nir_def *src10, nir_def *src11, nir_def *src12, nir_def *src13, nir_def *src14, nir_def *src15)
{
   nir_def *srcs[16] = {src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10, src11, src12, src13, src14, src15};
   return nir_build_alu_src_arr(build, nir_op_vec16, srcs);
}
static inline nir_def *
nir_vec2(nir_builder *build, nir_def *src0, nir_def *src1)
{
   return nir_build_alu2(build, nir_op_vec2, src0, src1);
}
static inline nir_def *
nir_vec3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   return nir_build_alu3(build, nir_op_vec3, src0, src1, src2);
}
static inline nir_def *
nir_vec4(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   return nir_build_alu4(build, nir_op_vec4, src0, src1, src2, src3);
}
static inline nir_def *
nir_vec5(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4)
{
   nir_def *srcs[5] = {src0, src1, src2, src3, src4};
   return nir_build_alu_src_arr(build, nir_op_vec5, srcs);
}
static inline nir_def *
nir_vec8(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6, nir_def *src7)
{
   nir_def *srcs[8] = {src0, src1, src2, src3, src4, src5, src6, src7};
   return nir_build_alu_src_arr(build, nir_op_vec8, srcs);
}

struct _nir_addr_mode_is_indices {
   int _; /* exists to avoid empty initializers */
   nir_variable_mode memory_modes;
};
struct _nir_al2p_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned flags;
};
struct _nir_ald_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
   unsigned range;
   unsigned flags;
   enum gl_access_qualifier access;
};
struct _nir_ast_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
   unsigned range;
   unsigned flags;
};
struct _nir_atomic_add_gen_prim_count_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_atomic_add_xfb_prim_count_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_atomic_counter_add_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_and_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_comp_swap_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_exchange_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_inc_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_max_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_min_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_or_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_post_dec_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_pre_dec_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_read_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_atomic_counter_xor_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
};
struct _nir_barrier_indices {
   int _; /* exists to avoid empty initializers */
   mesa_scope execution_scope;
   mesa_scope memory_scope;
   nir_memory_semantics memory_semantics;
   nir_variable_mode memory_modes;
};
struct _nir_bindless_image_agx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
};
struct _nir_bindless_image_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_bindless_image_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_bindless_image_descriptor_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_fence_kk_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_format_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_fragment_mask_load_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_levels_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_bindless_image_load_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_order_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_samples_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_samples_identical_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_size_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_sparse_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_bindless_image_store_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type src_type;
};
struct _nir_bindless_image_store_block_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   bool explicit_coord;
};
struct _nir_bindless_image_store_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_image_texel_address_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_bindless_resource_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
};
struct _nir_bindless_sampler_agx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
};
struct _nir_blend2_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   nir_alu_type src_type;
   nir_alu_type dest_type;
};
struct _nir_blend_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   nir_alu_type src_type;
};
struct _nir_brcst_active_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned cluster_size;
};
struct _nir_btd_stack_push_intel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stack_size;
};
struct _nir_bvh_stack_rtn_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stack_size;
};
struct _nir_cmat_binary_op_indices {
   int _; /* exists to avoid empty initializers */
   nir_op alu_op;
   unsigned fp_math_ctrl;
};
struct _nir_cmat_convert_indices {
   int _; /* exists to avoid empty initializers */
   unsigned saturate;
   nir_cmat_signed cmat_signed_mask;
   unsigned fp_math_ctrl;
};
struct _nir_cmat_length_indices {
   int _; /* exists to avoid empty initializers */
   struct glsl_cmat_description cmat_desc;
};
struct _nir_cmat_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_matrix_layout matrix_layout;
};
struct _nir_cmat_load_shared_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned num_matrices;
   enum glsl_matrix_layout matrix_layout;
   int base;
};
struct _nir_cmat_muladd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned saturate;
   nir_cmat_signed cmat_signed_mask;
};
struct _nir_cmat_muladd_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned saturate;
   unsigned neg_lo_amd;
   unsigned neg_hi_amd;
   enum glsl_base_type src_base_type;
   enum glsl_base_type src_base_type2;
};
struct _nir_cmat_muladd_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_cmat_scalar_op_indices {
   int _; /* exists to avoid empty initializers */
   nir_op alu_op;
   unsigned fp_math_ctrl;
};
struct _nir_cmat_store_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_matrix_layout matrix_layout;
};
struct _nir_cmat_transpose_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_cmat_unary_op_indices {
   int _; /* exists to avoid empty initializers */
   nir_op alu_op;
   unsigned fp_math_ctrl;
};
struct _nir_convert_alu_types_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
   nir_alu_type dest_type;
   nir_rounding_mode rounding_mode;
   unsigned saturate;
   unsigned fp_math_ctrl;
};
struct _nir_convert_cmat_intel_indices {
   int _; /* exists to avoid empty initializers */
   struct glsl_cmat_description dst_cmat_desc;
   struct glsl_cmat_description src_cmat_desc;
};
struct _nir_copy_deref_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier dst_access;
   enum gl_access_qualifier src_access;
};
struct _nir_copy_global_to_uniform_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
   unsigned range;
};
struct _nir_copy_push_const_to_uniform_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_copy_ubo_to_uniform_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_ddx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_ddx_coarse_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_ddx_fine_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_ddy_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_ddy_coarse_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_ddy_fine_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fp_math_ctrl;
};
struct _nir_decl_reg_indices {
   int _; /* exists to avoid empty initializers */
   unsigned num_components;
   unsigned num_array_elems;
   unsigned bit_size;
   bool divergent;
};
struct _nir_deref_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_deref_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_deref_buffer_address_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_deref_buffer_array_length_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_deref_mode_is_indices {
   int _; /* exists to avoid empty initializers */
   nir_variable_mode memory_modes;
};
struct _nir_dma_st_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_dma_st_shregs_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_dpas_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_base_type dest_base_type;
   enum glsl_base_type src_base_type;
   unsigned saturate;
   unsigned systolic_depth;
   unsigned repeat_count;
};
struct _nir_dpp16_shift_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_dummy_load_store_pco_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_emit_primitive_poly_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_emit_vertex_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_emit_vertex_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_emit_vertex_with_counter_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_emitpix_pco_indices {
   int _; /* exists to avoid empty initializers */
   bool freep;
};
struct _nir_end_primitive_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_end_primitive_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_end_primitive_with_counter_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_exclusive_scan_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
};
struct _nir_exclusive_scan_clusters_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
};
struct _nir_export_agx_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_export_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned target;
   unsigned enabled_channels;
   unsigned flags;
};
struct _nir_export_dual_src_blend_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned enabled_channels;
};
struct _nir_export_row_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned target;
   unsigned enabled_channels;
   unsigned flags;
};
struct _nir_flush_tile_buffer_pco_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_frag_store_pco_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_fs_out_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_gds_atomic_add_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_get_ssbo_size_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_get_ubo_size_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_global_atomic_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_2x32_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_agx_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
   bool sign_extend;
};
struct _nir_global_atomic_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_pco_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_swap_2x32_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_swap_agx_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
   bool sign_extend;
};
struct _nir_global_atomic_swap_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_swap_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_global_atomic_swap_pco_indices {
   int _; /* exists to avoid empty initializers */
   nir_atomic_op atomic_op;
};
struct _nir_glslopt_precision_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_imadsp_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_image_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   nir_atomic_op atomic_op;
};
struct _nir_image_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   nir_atomic_op atomic_op;
};
struct _nir_image_deref_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_image_deref_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_image_deref_descriptor_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_fence_kk_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_format_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_fragment_mask_load_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_levels_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_image_deref_load_info_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_image_deref_load_param_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_image_deref_load_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_order_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_samples_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_samples_identical_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_size_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_sparse_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_image_deref_store_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type src_type;
};
struct _nir_image_deref_store_block_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   bool explicit_coord;
};
struct _nir_image_deref_store_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_deref_texel_address_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_descriptor_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_fence_kk_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_format_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_fragment_mask_load_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_heap_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_image_heap_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
};
struct _nir_image_heap_descriptor_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_fence_kk_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_format_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_fragment_mask_load_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_levels_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_image_heap_load_param_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_image_heap_load_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_order_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_samples_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_samples_identical_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_size_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_sparse_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_image_heap_store_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   nir_alu_type src_type;
};
struct _nir_image_heap_store_block_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   bool explicit_coord;
};
struct _nir_image_heap_store_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_heap_texel_address_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
};
struct _nir_image_levels_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   nir_alu_type dest_type;
};
struct _nir_image_load_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_order_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_samples_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_samples_identical_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_size_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_sparse_load_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   nir_alu_type dest_type;
};
struct _nir_image_store_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   nir_alu_type src_type;
};
struct _nir_image_store_block_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
   bool explicit_coord;
};
struct _nir_image_store_raw_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_image_texel_address_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned range_base;
};
struct _nir_inclusive_scan_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
};
struct _nir_inclusive_scan_clusters_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
};
struct _nir_initialize_node_payloads_indices {
   int _; /* exists to avoid empty initializers */
   mesa_scope execution_scope;
};
struct _nir_ipa_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned flags;
};
struct _nir_is_subgroup_invocation_lt_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_isberd_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
   unsigned range;
   unsigned flags;
   enum gl_access_qualifier access;
};
struct _nir_isbewr_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range_base;
   unsigned range;
   unsigned flags;
   enum gl_access_qualifier access;
};
struct _nir_launch_mesh_workgroups_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_ldc_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_ldcx_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_ldtram_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned flags;
};
struct _nir_lea_attr_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
   unsigned desc_set;
};
struct _nir_lea_tex_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
};
struct _nir_load_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   enum pipe_format format;
   bool sign_extend;
};
struct _nir_load_attribute_pan_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_barycentric_at_offset_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_at_offset_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_at_sample_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_centroid_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_coord_at_offset_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_coord_at_sample_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_coord_centroid_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_coord_pixel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_coord_sample_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_model_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_pixel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_barycentric_sample_indices {
   int _; /* exists to avoid empty initializers */
   unsigned interp_mode;
};
struct _nir_load_blend_descriptor_pan_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_blend_input_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   nir_alu_type dest_type;
};
struct _nir_load_buffer_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_variable_mode memory_modes;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   nir_alu_type dest_type;
};
struct _nir_load_buffer_ptr_deref_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_resource_type resource_type;
};
struct _nir_load_buffer_ptr_kk_indices {
   int _; /* exists to avoid empty initializers */
   unsigned binding;
};
struct _nir_load_clear_value_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   nir_alu_type dest_type;
};
struct _nir_load_coefficients_agx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
   struct nir_io_semantics io_semantics;
   unsigned interp_mode;
};
struct _nir_load_const_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_constant_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_constant_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   enum pipe_format format;
   bool sign_extend;
};
struct _nir_load_depth_texture_kk_indices {
   int _; /* exists to avoid empty initializers */
   enum glsl_sampler_dim image_dim;
   bool image_array;
};
struct _nir_load_deref_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_load_deref_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_load_deref_transpose_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_load_descriptor_set_agx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
};
struct _nir_load_exported_agx_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_frag_offset_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned range;
};
struct _nir_load_frag_size_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned range;
};
struct _nir_load_fs_coeffs_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_fs_input_interp_deltas_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_global_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_2x32_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_bounded_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_constant_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_constant_bounded_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_constant_offset_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_constant_uniform_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   int base;
};
struct _nir_load_global_cvt_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   enum gl_access_qualifier access;
};
struct _nir_load_global_etna_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_offset_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
   unsigned range;
};
struct _nir_load_global_tr_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_global_transpose_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_gmem_frag_offset_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned range;
};
struct _nir_load_gmem_frag_scale_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned range;
};
struct _nir_load_gs_vertex_offset_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_heap_descriptor_indices {
   int _; /* exists to avoid empty initializers */
   nir_resource_type resource_type;
};
struct _nir_load_hit_attrib_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_incoming_ray_payload_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_inline_data_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_load_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_input_vertex_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_interpolated_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_kernel_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_local_pixel_agx_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum pipe_format format;
};
struct _nir_load_mesh_view_indices_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
};
struct _nir_load_outgoing_ray_payload_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_param_indices {
   int _; /* exists to avoid empty initializers */
   unsigned param_idx;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_per_primitive_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_per_primitive_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_per_vertex_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_per_vertex_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_per_view_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_pixel_local_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   enum pipe_format format;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_preamble_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_preamble_class preamble_class;
};
struct _nir_load_primitive_location_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned driver_location;
};
struct _nir_load_push_constant_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_push_constant_zink_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
};
struct _nir_load_push_data_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   enum gl_access_qualifier access;
};
struct _nir_load_r600_per_vertex_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_raw_output_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
};
struct _nir_load_ray_object_to_world_indices {
   int _; /* exists to avoid empty initializers */
   unsigned column;
};
struct _nir_load_ray_payload_ptr_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_ray_triangle_vertex_positions_indices {
   int _; /* exists to avoid empty initializers */
   unsigned column;
};
struct _nir_load_ray_world_to_object_indices {
   int _; /* exists to avoid empty initializers */
   unsigned column;
};
struct _nir_load_reg_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   bool legacy_fabs;
   bool legacy_fneg;
};
struct _nir_load_reg_indirect_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   bool legacy_fabs;
   bool legacy_fneg;
};
struct _nir_load_reloc_const_intel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned param_idx;
   int base;
};
struct _nir_load_resource_heap_data_indices {
   int _; /* exists to avoid empty initializers */
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_resume_shader_address_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned call_idx;
};
struct _nir_load_return_param_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned call_idx;
   unsigned param_idx;
};
struct _nir_load_ring_gsvs_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_load_rt_conversion_pan_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_alu_type src_type;
};
struct _nir_load_scalar_arg_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned arg_upper_bound_u32_amd;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_scalar_arg_wg_div_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned arg_upper_bound_u32_amd;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_scratch_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_scratch_base_ptr_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_scratch_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_load_scratch_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shader_indirect_data_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shared_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shared2_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   uint8_t offset0;
   uint8_t offset1;
   bool st64;
};
struct _nir_load_shared_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shared_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shared_lock_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_shared_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned offset_shift_nv;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_shared_uniform_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_smp_meta_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   unsigned component;
};
struct _nir_load_smp_state_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   unsigned component;
   unsigned flags;
};
struct _nir_load_ssbo_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
};
struct _nir_load_ssbo_address_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_load_ssbo_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_ssbo_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_ssbo_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
};
struct _nir_load_ssbo_uniform_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   int base;
};
struct _nir_load_stack_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
   unsigned call_idx;
   unsigned value_id;
};
struct _nir_load_stat_query_address_poly_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_streamout_buffer_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_streamout_offset_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_sysval_agx_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   unsigned flags;
};
struct _nir_load_sysval_nv_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   bool divergent;
};
struct _nir_load_task_payload_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_tex_meta_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   unsigned component;
};
struct _nir_load_tex_pan_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
};
struct _nir_load_tex_state_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   unsigned component;
};
struct _nir_load_texture_handle_kk_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   enum glsl_sampler_dim image_dim;
   bool image_array;
   enum gl_access_qualifier access;
   unsigned flags;
};
struct _nir_load_tile_coord_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
};
struct _nir_load_tile_image_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
   enum gl_access_qualifier access;
};
struct _nir_load_tile_pan_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_tile_res_pan_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_tiled_offset_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned component;
   unsigned flags;
};
struct _nir_load_tlb_color_brcm_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
};
struct _nir_load_ttmp_register_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned arg_upper_bound_u32_amd;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_ttmp_register_wg_div_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned arg_upper_bound_u32_amd;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_typed_buffer_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_variable_mode memory_modes;
   enum gl_access_qualifier access;
   enum pipe_format format;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_uav_ir3_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_load_ubo_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned range_base;
   unsigned range;
};
struct _nir_load_ubo_uniform_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   int base;
   unsigned range;
};
struct _nir_load_ubo_vec4_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   unsigned component;
};
struct _nir_load_uniform_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   unsigned range;
   nir_alu_type dest_type;
};
struct _nir_load_urb_lsc_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   nir_variable_mode memory_modes;
};
struct _nir_load_urb_vec4_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   nir_variable_mode memory_modes;
};
struct _nir_load_user_clip_plane_indices {
   int _; /* exists to avoid empty initializers */
   unsigned ucp_id;
};
struct _nir_load_uvs_index_agx_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
};
struct _nir_load_var_buf_flat_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_var_buf_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_var_flat_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_var_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_load_var_special_pan_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_load_vector_arg_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned arg_upper_bound_u32_amd;
   unsigned arg_num_lsb_zero;
};
struct _nir_load_vulkan_descriptor_indices {
   int _; /* exists to avoid empty initializers */
   nir_descriptor_type desc_type;
};
struct _nir_load_xfb_address_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_load_xfb_size_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_masked_swizzle_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned swizzle_mask;
   bool fetch_inactive;
};
struct _nir_memcpy_deref_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier dst_access;
   enum gl_access_qualifier src_access;
};
struct _nir_mutex_pco_indices {
   int _; /* exists to avoid empty initializers */
   unsigned mutex_id_pco;
   unsigned mutex_op_pco;
};
struct _nir_nop_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_ordered_xfb_counter_add_gfx11_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
};
struct _nir_pack_pco_indices {
   int _; /* exists to avoid empty initializers */
   enum pipe_format format;
};
struct _nir_parallel_copy_indices {
   int _; /* exists to avoid empty initializers */
   bool src_is_reg;
   bool dst_is_reg;
};
struct _nir_pck_prog_pco_indices {
   int _; /* exists to avoid empty initializers */
   bool scale;
   bool roundzero;
};
struct _nir_printf_indices {
   int _; /* exists to avoid empty initializers */
   unsigned fmt_idx;
};
struct _nir_quad_swizzle_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned swizzle_mask;
   bool fetch_inactive;
};
struct _nir_reduce_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
   unsigned cluster_size;
};
struct _nir_reduce_clusters_ir3_indices {
   int _; /* exists to avoid empty initializers */
   unsigned reduction_op;
};
struct _nir_resource_intel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   nir_resource_data_intel resource_access_intel;
   unsigned resource_block_intel;
};
struct _nir_rotate_indices {
   int _; /* exists to avoid empty initializers */
   unsigned cluster_size;
};
struct _nir_rq_load_indices {
   int _; /* exists to avoid empty initializers */
   nir_ray_query_value ray_query_value;
   bool committed;
   unsigned column;
};
struct _nir_rt_execute_callable_indices {
   int _; /* exists to avoid empty initializers */
   unsigned call_idx;
   unsigned stack_size;
};
struct _nir_rt_resume_indices {
   int _; /* exists to avoid empty initializers */
   unsigned call_idx;
   unsigned stack_size;
};
struct _nir_rt_trace_ray_indices {
   int _; /* exists to avoid empty initializers */
   unsigned call_idx;
   unsigned stack_size;
};
struct _nir_select_vertex_poly_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_sendmsg_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_set_vertex_and_primitive_count_indices {
   int _; /* exists to avoid empty initializers */
   unsigned stream_id;
};
struct _nir_shader_clock_indices {
   int _; /* exists to avoid empty initializers */
   mesa_scope memory_scope;
};
struct _nir_shared_append_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_shared_atomic_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_shared_atomic_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
   unsigned offset_shift_nv;
};
struct _nir_shared_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_shared_atomic_swap_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
   unsigned offset_shift_nv;
};
struct _nir_shared_consume_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_sleep_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_smp_coeffs_pco_indices {
   int _; /* exists to avoid empty initializers */
   uint16_t smp_flags_pco;
   unsigned range;
   enum gl_access_qualifier access;
};
struct _nir_smp_pco_indices {
   int _; /* exists to avoid empty initializers */
   uint16_t smp_flags_pco;
   unsigned range;
   enum gl_access_qualifier access;
};
struct _nir_smp_raw_pco_indices {
   int _; /* exists to avoid empty initializers */
   uint16_t smp_flags_pco;
   unsigned range;
   unsigned enabled_channels;
   enum gl_access_qualifier access;
};
struct _nir_smp_write_pco_indices {
   int _; /* exists to avoid empty initializers */
   uint16_t smp_flags_pco;
   unsigned range;
   enum gl_access_qualifier access;
};
struct _nir_ssbo_atomic_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
   unsigned offset_shift;
};
struct _nir_ssbo_atomic_ir3_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
   unsigned offset_shift;
};
struct _nir_ssbo_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
   unsigned offset_shift;
};
struct _nir_ssbo_atomic_swap_ir3_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_atomic_op atomic_op;
   unsigned offset_shift;
};
struct _nir_ssbo_descriptor_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_store_agx_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   enum pipe_format format;
   bool sign_extend;
};
struct _nir_store_buffer_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   nir_variable_mode memory_modes;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_clip_distance_kk_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_combined_output_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   unsigned component;
   nir_alu_type src_type;
   nir_alu_type dest_type;
};
struct _nir_store_const_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_deref_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
};
struct _nir_store_deref_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
};
struct _nir_store_global_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_2x32_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned write_mask;
};
struct _nir_store_global_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_cvt_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type src_type;
   enum gl_access_qualifier access;
};
struct _nir_store_global_etna_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_global_offset_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
};
struct _nir_store_global_psiz_pan_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
};
struct _nir_store_hit_attrib_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_incoming_ray_payload_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_local_pixel_agx_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   enum pipe_format format;
   bool explicit_coord;
};
struct _nir_store_local_shared_r600_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
};
struct _nir_store_outgoing_ray_payload_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned write_mask;
   unsigned component;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
   struct nir_io_xfb io_xfb;
};
struct _nir_store_param_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned param_idx;
};
struct _nir_store_per_primitive_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned write_mask;
   unsigned component;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_store_per_primitive_payload_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
};
struct _nir_store_per_vertex_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned write_mask;
   unsigned component;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_store_per_view_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned write_mask;
   unsigned component;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_store_pixel_local_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned range;
   unsigned write_mask;
   unsigned component;
   enum pipe_format format;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_store_preamble_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_preamble_class preamble_class;
};
struct _nir_store_preamble_dynamic_indices {
   int _; /* exists to avoid empty initializers */
   nir_preamble_class preamble_class;
};
struct _nir_store_raw_output_pan_indices {
   int _; /* exists to avoid empty initializers */
   struct nir_io_semantics io_semantics;
   int base;
};
struct _nir_store_reg_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   bool legacy_fsat;
};
struct _nir_store_reg_indirect_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   bool legacy_fsat;
};
struct _nir_store_render_target_intel_indices {
   int _; /* exists to avoid empty initializers */
   unsigned target;
};
struct _nir_store_scalar_arg_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_scratch_indices {
   int _; /* exists to avoid empty initializers */
   unsigned align_mul;
   unsigned align_offset;
   unsigned write_mask;
};
struct _nir_store_scratch_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_shared_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned write_mask;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_shared2_amd_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   uint8_t offset0;
   uint8_t offset1;
   bool st64;
};
struct _nir_store_shared_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_shared_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_shared_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned offset_shift_nv;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_shared_unlock_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_ssbo_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
};
struct _nir_store_ssbo_block_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_ssbo_intel_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   int base;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_ssbo_ir3_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned write_mask;
   enum gl_access_qualifier access;
   unsigned align_mul;
   unsigned align_offset;
   unsigned offset_shift;
};
struct _nir_store_stack_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned align_mul;
   unsigned align_offset;
   unsigned write_mask;
   unsigned call_idx;
   unsigned value_id;
};
struct _nir_store_task_payload_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   enum gl_access_qualifier access;
   unsigned write_mask;
   unsigned align_mul;
   unsigned align_offset;
};
struct _nir_store_tile_pan_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   nir_alu_type src_type;
   struct nir_io_semantics io_semantics;
};
struct _nir_store_tlb_sample_color_v3d_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   unsigned component;
   nir_alu_type src_type;
};
struct _nir_store_urb_lsc_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_urb_vec4_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_vector_arg_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_store_zs_agx_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_strict_wqm_coord_amd_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_subfm_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_suclamp_nv_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};
struct _nir_suldga_nv_indices {
   int _; /* exists to avoid empty initializers */
   enum pipe_format format;
   enum gl_access_qualifier access;
   unsigned flags;
};
struct _nir_sustga_nv_indices {
   int _; /* exists to avoid empty initializers */
   enum gl_access_qualifier access;
   unsigned flags;
};
struct _nir_task_payload_atomic_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_task_payload_atomic_swap_indices {
   int _; /* exists to avoid empty initializers */
   int base;
   nir_atomic_op atomic_op;
};
struct _nir_test_fs_config_intel_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_texc0_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   unsigned flags;
};
struct _nir_texc1_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   unsigned flags;
};
struct _nir_texc2_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   unsigned flags;
};
struct _nir_texs_2d_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   unsigned flags;
};
struct _nir_texs_cube_pan_indices {
   int _; /* exists to avoid empty initializers */
   nir_alu_type dest_type;
   unsigned flags;
};
struct _nir_trace_ray_intel_indices {
   int _; /* exists to avoid empty initializers */
   bool synchronous;
};
struct _nir_unit_test_divergent_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_unit_test_output_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_unit_test_uniform_input_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_unpack_pco_indices {
   int _; /* exists to avoid empty initializers */
   enum pipe_format format;
};
struct _nir_vild_nv_indices {
   int _; /* exists to avoid empty initializers */
   int base;
};
struct _nir_vulkan_resource_index_indices {
   int _; /* exists to avoid empty initializers */
   unsigned desc_set;
   unsigned binding;
   nir_descriptor_type desc_type;
   nir_resource_type resource_type;
};
struct _nir_vulkan_resource_reindex_indices {
   int _; /* exists to avoid empty initializers */
   nir_descriptor_type desc_type;
};
struct _nir_xfb_counter_sub_gfx11_amd_indices {
   int _; /* exists to avoid empty initializers */
   unsigned write_mask;
};
struct _nir_zs_emit_pan_indices {
   int _; /* exists to avoid empty initializers */
   unsigned flags;
};



static inline nir_intrinsic_instr *
_nir_build_abort(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_abort);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_accept_ray_intersection(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_accept_ray_intersection);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_addr_mode_is(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_addr_mode_is_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_addr_mode_is);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_al2p_nv(nir_builder *build, nir_def *src0, struct _nir_al2p_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_al2p_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ald_nv(nir_builder *build, unsigned num_components, nir_def *src0, nir_def *src1, struct _nir_ald_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ald_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_flags(intrin, indices.flags);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_alpha_to_coverage(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_alpha_to_coverage);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_alphatst_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_alphatst_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_as_uniform(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_as_uniform);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_ast_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_ast_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ast_nv);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_atest_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atest_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_atomic_add_gen_prim_count_amd(nir_builder *build, nir_def *src0, struct _nir_atomic_add_gen_prim_count_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_add_gen_prim_count_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_atomic_add_gs_emit_prim_count_amd(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_add_gs_emit_prim_count_amd);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_atomic_add_shader_invocation_count_amd(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_add_shader_invocation_count_amd);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_atomic_add_xfb_prim_count_amd(nir_builder *build, nir_def *src0, struct _nir_atomic_add_xfb_prim_count_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_add_xfb_prim_count_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_atomic_counter_add(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_add_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_add);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_add_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_add_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_and(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_and_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_and);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_and_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_and_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_comp_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_atomic_counter_comp_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_comp_swap);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_comp_swap_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_comp_swap_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_exchange(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_exchange_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_exchange);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_exchange_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_exchange_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_inc(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_atomic_counter_inc_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_inc);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_inc_deref(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_inc_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_max(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_max_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_max);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_max_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_max_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_min(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_min_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_min);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_min_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_min_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_or(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_or_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_or);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_or_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_or_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_post_dec(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_atomic_counter_post_dec_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_post_dec);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_post_dec_deref(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_post_dec_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_pre_dec(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_atomic_counter_pre_dec_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_pre_dec);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_pre_dec_deref(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_pre_dec_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_read(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_atomic_counter_read_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_read);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_read_deref(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_read_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_xor(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_atomic_counter_xor_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_xor);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_atomic_counter_xor_deref(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_atomic_counter_xor_deref);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_bit_count_exclusive(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_bit_count_exclusive);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_bit_count_inclusive(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_bit_count_inclusive);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_bit_count_reduce(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_bit_count_reduce);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_bitfield_extract(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_bitfield_extract);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_find_lsb(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_find_lsb);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_find_msb(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_find_msb);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ballot_relaxed(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ballot_relaxed);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bar_break_nv(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bar_break_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bar_set_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bar_set_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_bar_sync_nv(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bar_sync_nv);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_barrier(nir_builder *build, struct _nir_barrier_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_barrier);

   nir_intrinsic_set_execution_scope(intrin, indices.execution_scope);
   nir_intrinsic_set_memory_scope(intrin, indices.memory_scope);
   nir_intrinsic_set_memory_semantics(intrin, indices.memory_semantics);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_begin_invocation_interlock(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_begin_invocation_interlock);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_bindgen_return(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindgen_return);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_bindless_image_agx(nir_builder *build, nir_def *src0, struct _nir_bindless_image_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_bindless_image_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_atomic);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_bindless_image_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_atomic_swap);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_descriptor_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_bindless_image_descriptor_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_descriptor_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_bindless_image_fence_kk(nir_builder *build, nir_def *src0, struct _nir_bindless_image_fence_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_fence_kk);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_bindless_image_format(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_bindless_image_format_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_format);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_fragment_mask_load_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_bindless_image_fragment_mask_load_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_fragment_mask_load_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_levels(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_bindless_image_levels_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_levels);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_bindless_image_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_load_raw_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_bindless_image_load_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_load_raw_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_order(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_bindless_image_order_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_order);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_samples(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_bindless_image_samples_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_samples);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_samples_identical(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_bindless_image_samples_identical_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_samples_identical);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_size(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_bindless_image_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_size);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_image_sparse_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_bindless_image_sparse_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_sparse_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_bindless_image_store(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_bindless_image_store_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_store);

   intrin->num_components = src3->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_bindless_image_store_block_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_bindless_image_store_block_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_store_block_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_explicit_coord(intrin, indices.explicit_coord);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_bindless_image_store_raw_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_bindless_image_store_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_store_raw_intel);

   intrin->num_components = src2->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_bindless_image_texel_address(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_bindless_image_texel_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_image_texel_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_resource_ir3(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_bindless_resource_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_resource_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bindless_sampler_agx(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_bindless_sampler_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bindless_sampler_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_blend2_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_blend2_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_blend2_pan);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_blend_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_blend_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_blend_pan);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_blend_return_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_blend_return_pan);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_brcst_active_ir3(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_brcst_active_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_brcst_active_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_cluster_size(intrin, indices.cluster_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_btd_retire_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_btd_retire_intel);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_btd_spawn_intel(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_btd_spawn_intel);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_btd_stack_push_intel(nir_builder *build, struct _nir_btd_stack_push_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_btd_stack_push_intel);

   nir_intrinsic_set_stack_size(intrin, indices.stack_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_bvh64_intersect_ray_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bvh64_intersect_ray_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bvh8_intersect_ray_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bvh8_intersect_ray_amd);

      nir_def_init(&intrin->instr, &intrin->def, 16, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   intrin->src[6] = nir_src_for_ssa(src6);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_bvh_stack_rtn_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_bvh_stack_rtn_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_bvh_stack_rtn_amd);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_stack_size(intrin, indices.stack_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_binary_op(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_binary_op_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_binary_op);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_alu_op(intrin, indices.alu_op);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_bitcast(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_bitcast);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_construct(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_construct);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_convert(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_cmat_convert_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_convert);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_saturate(intrin, indices.saturate);
   nir_intrinsic_set_cmat_signed_mask(intrin, indices.cmat_signed_mask);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_copy(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_copy);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_cmat_extract(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_extract);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_insert(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_insert);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_cmat_length(nir_builder *build, struct _nir_cmat_length_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_length);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_cmat_desc(intrin, indices.cmat_desc);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_load(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_load);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_matrix_layout(intrin, indices.matrix_layout);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_cmat_load_shared_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_cmat_load_shared_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_load_shared_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_num_matrices(intrin, indices.num_matrices);
   nir_intrinsic_set_matrix_layout(intrin, indices.matrix_layout);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_cmat_mov_transpose_nv(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_mov_transpose_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, 16);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_muladd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_cmat_muladd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_muladd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_saturate(intrin, indices.saturate);
   nir_intrinsic_set_cmat_signed_mask(intrin, indices.cmat_signed_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_cmat_muladd_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_muladd_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_muladd_amd);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src2->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_saturate(intrin, indices.saturate);
   nir_intrinsic_set_neg_lo_amd(intrin, indices.neg_lo_amd);
   nir_intrinsic_set_neg_hi_amd(intrin, indices.neg_hi_amd);
   nir_intrinsic_set_src_base_type(intrin, indices.src_base_type);
   nir_intrinsic_set_src_base_type2(intrin, indices.src_base_type2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_cmat_muladd_nv(nir_builder *build, unsigned num_components, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_muladd_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_muladd_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src2->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_scalar_op(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_scalar_op_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_scalar_op);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_alu_op(intrin, indices.alu_op);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_store(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_cmat_store_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_store);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_matrix_layout(intrin, indices.matrix_layout);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_transpose(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_cmat_transpose_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_transpose);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_cmat_unary_op(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_cmat_unary_op_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cmat_unary_op);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_alu_op(intrin, indices.alu_op);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_convert_alu_types(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_convert_alu_types_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_convert_alu_types);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_rounding_mode(intrin, indices.rounding_mode);
   nir_intrinsic_set_saturate(intrin, indices.saturate);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_convert_cmat_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_convert_cmat_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_convert_cmat_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_dst_cmat_desc(intrin, indices.dst_cmat_desc);
   nir_intrinsic_set_src_cmat_desc(intrin, indices.src_cmat_desc);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_copy_deref(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_copy_deref_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_copy_deref);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_dst_access(intrin, indices.dst_access);
   nir_intrinsic_set_src_access(intrin, indices.src_access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_copy_fs_outputs_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_copy_fs_outputs_nv);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_copy_global_to_uniform_ir3(nir_builder *build, nir_def *src0, struct _nir_copy_global_to_uniform_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_copy_global_to_uniform_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_copy_push_const_to_uniform_ir3(nir_builder *build, nir_def *src0, struct _nir_copy_push_const_to_uniform_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_copy_push_const_to_uniform_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_copy_ubo_to_uniform_ir3(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_copy_ubo_to_uniform_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_copy_ubo_to_uniform_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_cube_ssel_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cube_ssel_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_cube_tsel_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cube_tsel_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_cubeface_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_cubeface_pan);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddx(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddx);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddx_coarse(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddx_coarse_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddx_coarse);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddx_fine(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddx_fine_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddx_fine);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddy(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddy_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddy);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddy_coarse(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddy_coarse_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddy_coarse);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ddy_fine(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_ddy_fine_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ddy_fine);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fp_math_ctrl(intrin, indices.fp_math_ctrl);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_debug_break(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_debug_break);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_decl_reg(nir_builder *build, unsigned bit_size, struct _nir_decl_reg_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_decl_reg);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   nir_intrinsic_set_num_components(intrin, indices.num_components);
   nir_intrinsic_set_num_array_elems(intrin, indices.num_array_elems);
   nir_intrinsic_set_bit_size(intrin, indices.bit_size);
   nir_intrinsic_set_divergent(intrin, indices.divergent);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_demote(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_demote);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_demote_if(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_demote_if);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_demote_samples(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_demote_samples);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_deref_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_deref_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_atomic);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_deref_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_atomic_swap);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_buffer_address(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_deref_buffer_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_buffer_address);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_buffer_array_length(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_deref_buffer_array_length_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_buffer_array_length);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_implicit_array_length(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_implicit_array_length);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_mode_is(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_deref_mode_is_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_mode_is);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_deref_texture_src(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_deref_texture_src);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_dma_flush_pco(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_flush_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_dma_ld_pco(nir_builder *build, unsigned num_components, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_ld_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_dma_ld_shregs_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_ld_shregs_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_dma_st_pco(nir_builder *build, nir_def *src0, struct _nir_dma_st_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_st_pco);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_dma_st_shregs_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_dma_st_shregs_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_st_shregs_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_dma_st_tiled_pco(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dma_st_tiled_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_doorbell_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_doorbell_agx);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_dpas_intel(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_dpas_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dpas_intel);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_dest_base_type(intrin, indices.dest_base_type);
   nir_intrinsic_set_src_base_type(intrin, indices.src_base_type);
   nir_intrinsic_set_saturate(intrin, indices.saturate);
   nir_intrinsic_set_systolic_depth(intrin, indices.systolic_depth);
   nir_intrinsic_set_repeat_count(intrin, indices.repeat_count);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_dpp16_shift_amd(nir_builder *build, nir_def *src0, struct _nir_dpp16_shift_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dpp16_shift_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_dummy_load_store_pco(nir_builder *build, struct _nir_dummy_load_store_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_dummy_load_store_pco);

   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_elect(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_elect);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_elect_any_ir3(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_elect_any_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_emit_primitive_poly(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_emit_primitive_poly_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_emit_primitive_poly);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_emit_vertex(nir_builder *build, struct _nir_emit_vertex_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_emit_vertex);

   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_emit_vertex_nv(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_emit_vertex_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_emit_vertex_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_emit_vertex_with_counter(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_emit_vertex_with_counter_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_emit_vertex_with_counter);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_emitpix_pco(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_emitpix_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_emitpix_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_freep(intrin, indices.freep);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_end_invocation_interlock(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_end_invocation_interlock);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_end_primitive(nir_builder *build, struct _nir_end_primitive_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_end_primitive);

   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_end_primitive_nv(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_end_primitive_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_end_primitive_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_end_primitive_with_counter(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_end_primitive_with_counter_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_end_primitive_with_counter);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_enqueue_node_payloads(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_enqueue_node_payloads);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_exclusive_scan(nir_builder *build, nir_def *src0, struct _nir_exclusive_scan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_exclusive_scan);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_exclusive_scan_clusters_ir3(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_exclusive_scan_clusters_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_exclusive_scan_clusters_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_execute_callable(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_execute_callable);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_execute_closest_hit_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_execute_closest_hit_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   intrin->src[6] = nir_src_for_ssa(src6);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_execute_miss_amd(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_execute_miss_amd);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_export_agx(nir_builder *build, nir_def *src0, struct _nir_export_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_export_agx);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_export_amd(nir_builder *build, nir_def *src0, struct _nir_export_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_export_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_target(intrin, indices.target);
   nir_intrinsic_set_enabled_channels(intrin, indices.enabled_channels);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_export_dual_src_blend_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_export_dual_src_blend_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_export_dual_src_blend_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_enabled_channels(intrin, indices.enabled_channels);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_export_row_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_export_row_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_export_row_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_target(intrin, indices.target);
   nir_intrinsic_set_enabled_channels(intrin, indices.enabled_channels);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_fence_helper_exit_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_fence_helper_exit_agx);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_fence_mem_to_tex_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_fence_mem_to_tex_agx);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_fence_pbe_to_tex_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_fence_pbe_to_tex_agx);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_fence_pbe_to_tex_pixel_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_fence_pbe_to_tex_pixel_agx);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_final_primitive_nv(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_final_primitive_nv);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_finalize_incoming_node_payload(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_finalize_incoming_node_payload);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_first_invocation(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_first_invocation);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_flush_tile_buffer_pco(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_flush_tile_buffer_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_flush_tile_buffer_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_frag_store_pco(nir_builder *build, nir_def *src0, struct _nir_frag_store_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_frag_store_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_fs_out_nv(nir_builder *build, nir_def *src0, struct _nir_fs_out_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_fs_out_nv);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_gds_atomic_add_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_gds_atomic_add_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_gds_atomic_add_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_get_ssbo_size(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_get_ssbo_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_get_ssbo_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_get_ubo_size(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_get_ubo_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_get_ubo_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_global_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_2x32(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_global_atomic_2x32_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_2x32);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_agx(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_agx);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_sign_extend(intrin, indices.sign_extend);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_amd);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_nv(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_nv);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_global_atomic_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap_2x32(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_swap_2x32_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap_2x32);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap_agx(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_global_atomic_swap_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap_agx);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_sign_extend(intrin, indices.sign_extend);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_global_atomic_swap_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap_amd);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap_nv(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_global_atomic_swap_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap_nv);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_global_atomic_swap_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_global_atomic_swap_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_global_atomic_swap_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_glslopt_precision(nir_builder *build, nir_def *src0, struct _nir_glslopt_precision_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_glslopt_precision);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_ignore_ray_intersection(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ignore_ray_intersection);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_imadsp_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_imadsp_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_imadsp_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_atomic);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_atomic_swap);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_deref_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_atomic);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_deref_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_atomic_swap);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_descriptor_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_image_deref_descriptor_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_descriptor_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_deref_fence_kk(nir_builder *build, nir_def *src0, struct _nir_image_deref_fence_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_fence_kk);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_deref_format(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_deref_format_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_format);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_fragment_mask_load_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_image_deref_fragment_mask_load_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_fragment_mask_load_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_levels(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_deref_levels_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_levels);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_deref_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_load_info_nv(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_image_deref_load_info_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_load_info_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_load_param_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_image_deref_load_param_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_load_param_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_load_raw_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_deref_load_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_load_raw_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_order(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_deref_order_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_order);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_samples(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_deref_samples_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_samples);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_samples_identical(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_deref_samples_identical_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_samples_identical);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_size(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_deref_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_size);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_deref_sparse_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_deref_sparse_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_sparse_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_deref_store(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_deref_store_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_store);

   intrin->num_components = src3->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_deref_store_block_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_deref_store_block_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_store_block_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_explicit_coord(intrin, indices.explicit_coord);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_deref_store_raw_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_deref_store_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_store_raw_intel);

   intrin->num_components = src2->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_deref_texel_address(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_deref_texel_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_deref_texel_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_descriptor_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_image_descriptor_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_descriptor_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_fence_kk(nir_builder *build, nir_def *src0, struct _nir_image_fence_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_fence_kk);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_format(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_format_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_format);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_fragment_mask_load_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_image_fragment_mask_load_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_fragment_mask_load_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_heap_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_atomic);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_heap_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_atomic_swap);

   intrin->num_components = src3->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_descriptor_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_image_heap_descriptor_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_descriptor_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_heap_fence_kk(nir_builder *build, nir_def *src0, struct _nir_image_heap_fence_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_fence_kk);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_heap_format(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_heap_format_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_format);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_fragment_mask_load_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_image_heap_fragment_mask_load_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_fragment_mask_load_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_levels(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_heap_levels_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_levels);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_heap_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_load_param_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_image_heap_load_param_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_load_param_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_load_raw_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_heap_load_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_load_raw_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_order(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_heap_order_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_order);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_samples(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_heap_samples_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_samples);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_samples_identical(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_heap_samples_identical_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_samples_identical);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_size(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_heap_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_size);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_heap_sparse_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_heap_sparse_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_sparse_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_heap_store(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_heap_store_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_store);

   intrin->num_components = src3->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_heap_store_block_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_heap_store_block_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_store_block_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_explicit_coord(intrin, indices.explicit_coord);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_heap_store_raw_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_heap_store_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_store_raw_intel);

   intrin->num_components = src2->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_heap_texel_address(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_heap_texel_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_heap_texel_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_levels(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_levels_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_levels);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_load_raw_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_load_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_load_raw_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_order(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_order_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_order);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_samples(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_image_samples_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_samples);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_samples_identical(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_samples_identical_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_samples_identical);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_size(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_image_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_size);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_image_sparse_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_image_sparse_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_sparse_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_image_store(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_image_store_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_store);

   intrin->num_components = src3->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_store_block_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_store_block_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_store_block_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_explicit_coord(intrin, indices.explicit_coord);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_image_store_raw_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_store_raw_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_store_raw_intel);

   intrin->num_components = src2->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_image_texel_address(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_image_texel_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_image_texel_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_range_base(intrin, indices.range_base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_inclusive_scan(nir_builder *build, nir_def *src0, struct _nir_inclusive_scan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_inclusive_scan);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_inclusive_scan_clusters_ir3(nir_builder *build, nir_def *src0, struct _nir_inclusive_scan_clusters_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_inclusive_scan_clusters_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_initialize_node_payloads(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_initialize_node_payloads_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_initialize_node_payloads);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_execution_scope(intrin, indices.execution_scope);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_interp_deref_at_centroid(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_interp_deref_at_centroid);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_interp_deref_at_offset(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_interp_deref_at_offset);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_interp_deref_at_sample(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_interp_deref_at_sample);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_interp_deref_at_vertex(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_interp_deref_at_vertex);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_inverse_ballot(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_inverse_ballot);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ipa_nv(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_ipa_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ipa_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_is_helper_invocation(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_is_helper_invocation);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_is_null_descriptor(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_is_null_descriptor);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_is_sparse_resident_zink(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_is_sparse_resident_zink);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_is_sparse_texels_resident(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_is_sparse_texels_resident);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_is_subgroup_invocation_lt_amd(nir_builder *build, nir_def *src0, struct _nir_is_subgroup_invocation_lt_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_is_subgroup_invocation_lt_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_isberd_nv(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_isberd_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_isberd_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_flags(intrin, indices.flags);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_isbewr_nv(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_isbewr_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_isbewr_nv);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_flags(intrin, indices.flags);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_isp_feedback_pco(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_isp_feedback_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_lane_permute_16_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_lane_permute_16_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_last_invocation(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_last_invocation);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_launch_mesh_workgroups(nir_builder *build, nir_def *src0, struct _nir_launch_mesh_workgroups_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_launch_mesh_workgroups);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_launch_mesh_workgroups_with_payload_deref(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_launch_mesh_workgroups_with_payload_deref);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_ldc_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_ldc_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ldc_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ldcx_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_ldcx_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ldcx_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ldtram_nv(nir_builder *build, struct _nir_ldtram_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ldtram_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_lea_attr_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_lea_attr_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_lea_attr_pan);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_lea_buf_pan(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_lea_buf_pan);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_lea_tex_pan(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_lea_tex_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_lea_tex_pan);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_aa_line_width(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_aa_line_width);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_abort_buffer_address(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_abort_buffer_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_abort_buffer_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_abort_buffer_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_accel_struct_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_accel_struct_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_active_samples_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_active_samples_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_active_subgroup_count_agx(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_active_subgroup_count_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_active_subgroup_invocation_agx(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_active_subgroup_invocation_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_agx(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_sign_extend(intrin, indices.sign_extend);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_alpha_reference_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_alpha_reference_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_alpha_to_coverage_enable_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_alpha_to_coverage_enable_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_amplification_id_kk(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_amplification_id_kk);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_api_sample_mask_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_api_sample_mask_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_attrib_clamp_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_attrib_clamp_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_attribute_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_attribute_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_attribute_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_attribute_payload_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_attribute_payload_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_back_face_agx(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_back_face_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_at_offset(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_barycentric_at_offset_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_at_offset);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_at_offset_nv(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_barycentric_at_offset_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_at_offset_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_at_sample(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_barycentric_at_sample_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_at_sample);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_centroid(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_centroid_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_centroid);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_coord_at_offset(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_barycentric_coord_at_offset_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_coord_at_offset);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_coord_at_sample(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_barycentric_coord_at_sample_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_coord_at_sample);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_coord_centroid(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_coord_centroid_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_coord_centroid);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_coord_pixel(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_coord_pixel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_coord_pixel);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_coord_sample(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_coord_sample_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_coord_sample);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_model(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_model_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_model);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_optimize_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_optimize_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_pixel(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_pixel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_pixel);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_barycentric_sample(nir_builder *build, unsigned bit_size, struct _nir_load_barycentric_sample_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_barycentric_sample);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_base_global_invocation_id(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_base_global_invocation_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_base_instance(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_base_instance);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_base_vertex(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_base_vertex);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_base_workgroup_id(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_base_workgroup_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_a_float(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_a_float);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_aaaa8888_unorm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_aaaa8888_unorm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_b_float(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_b_float);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_g_float(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_g_float);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_r_float(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_r_float);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_rgba(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_rgba);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_const_color_rgba8888_unorm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_const_color_rgba8888_unorm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_descriptor_pan(nir_builder *build, struct _nir_load_blend_descriptor_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_descriptor_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_blend_input_pan(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_blend_input_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_blend_input_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_btd_global_arg_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_btd_global_arg_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_btd_local_arg_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_btd_local_arg_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_btd_resume_sbt_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_btd_resume_sbt_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_btd_shader_type_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_btd_shader_type_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_btd_stack_id_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_btd_stack_id_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_buffer_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_load_buffer_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_buffer_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_buffer_ptr_deref(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_buffer_ptr_deref_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_buffer_ptr_deref);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_resource_type(intrin, indices.resource_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_buffer_ptr_kk(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_buffer_ptr_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_buffer_ptr_kk);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_binding(intrin, indices.binding);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_call_return_address_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_call_return_address_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_callable_sbt_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_callable_sbt_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_callable_sbt_stride_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_callable_sbt_stride_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_clamp_vertex_color_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_clamp_vertex_color_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_clear_value_pan(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_clear_value_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_clear_value_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_clip_half_line_width_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_clip_half_line_width_amd);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_clip_z_coeff(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_clip_z_coeff);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cluster_num_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cluster_num_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_coalesced_input_count(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_coalesced_input_count);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_coeff_pco(nir_builder *build, unsigned num_components, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_coeff_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_coefficients_agx(nir_builder *build, nir_def *src0, struct _nir_load_coefficients_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_coefficients_agx);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_interp_mode(intrin, indices.interp_mode);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_color0_amd(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_color0_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_color1_amd(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_color1_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_const_buf_base_addr_lvp(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_const_buf_base_addr_lvp);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_const_ir3(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_const_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_const_ir3);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_constant(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_constant_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_constant);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_constant_agx(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_constant_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_constant_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_sign_extend(intrin, indices.sign_extend);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_constant_base_ptr(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_constant_base_ptr);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_core_count_arm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_core_count_arm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_core_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_core_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_core_max_id_arm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_core_max_id_arm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_coverage_mask_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_coverage_mask_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_any_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_any_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_back_face_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_back_face_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_ccw_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_ccw_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_front_face_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_front_face_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_line_viewport_xy_scale_and_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_line_viewport_xy_scale_and_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_mask(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_mask);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_mask_and_flags_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_mask_and_flags_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_small_line_precision_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_small_line_precision_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_small_lines_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_small_lines_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_small_triangle_precision_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_small_triangle_precision_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_small_triangles_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_small_triangles_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cull_triangle_viewport_xy_scale_and_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cull_triangle_viewport_xy_scale_and_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_cumulative_coverage_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_cumulative_coverage_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_debug_log_desc_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_debug_log_desc_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_depth_never_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_depth_never_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_depth_texture_kk(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_depth_texture_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_depth_texture_kk);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_deref(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_deref_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_deref);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_deref_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_deref_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_deref_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_deref_transpose_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_deref_transpose_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_deref_transpose_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_descriptor_set_agx(nir_builder *build, struct _nir_load_descriptor_set_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_descriptor_set_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_draw_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_draw_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_esgs_vertex_stride_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_esgs_vertex_stride_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_exported_agx(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_exported_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_exported_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_face_ccw_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_face_ccw_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fb_layers_v3d(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fb_layers_v3d);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fb_render_area_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fb_render_area_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fbfetch_image_desc_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fbfetch_image_desc_amd);

      nir_def_init(&intrin->instr, &intrin->def, 8, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fbfetch_image_fmask_desc_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fbfetch_image_fmask_desc_amd);

      nir_def_init(&intrin->instr, &intrin->def, 8, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fep_w_v3d(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fep_w_v3d);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_first_vertex(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_first_vertex);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fixed_point_size_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fixed_point_size_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_flat_mask(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_flat_mask);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_force_vrs_rates_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_force_vrs_rates_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_gmem_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_gmem_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_unscaled_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_unscaled_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_w(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_w);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_w_rcp(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_w_rcp);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_xy(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_xy);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_coord_z(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_coord_z);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_invocation_count(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_invocation_count);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_offset_ir3(nir_builder *build, nir_def *src0, struct _nir_load_frag_offset_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_offset_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_shading_rate(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_shading_rate);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_shading_rate_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_shading_rate_intel);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_size);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_frag_size_ir3(nir_builder *build, nir_def *src0, struct _nir_load_frag_size_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_frag_size_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_from_texture_handle_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_from_texture_handle_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_front_face(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_front_face);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_front_face_fsign(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_front_face_fsign);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_front_face_op_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_front_face_op_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_coeffs_pco(nir_builder *build, struct _nir_load_fs_coeffs_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_coeffs_pco);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_config_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_config_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_input_interp_deltas(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_fs_input_interp_deltas_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_input_interp_deltas);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_meta_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_meta_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_start_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_start_intel);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_z_c0_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_z_c0_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fs_z_c_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fs_z_c_intel);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_fully_covered(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_fully_covered);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_geometry_param_buffer_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_geometry_param_buffer_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_2x32(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_2x32_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_2x32);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_base_ptr(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_base_ptr);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_bounded(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_global_bounded_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_bounded);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_constant(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_constant_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_constant);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_constant_bounded(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_global_constant_bounded_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_constant_bounded);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_constant_offset(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_constant_offset_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_constant_offset);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_constant_uniform_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_constant_uniform_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_constant_uniform_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_cvt_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_cvt_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_cvt_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_etna(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_etna_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_etna);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_invocation_id(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_invocation_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_invocation_index(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_invocation_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_global_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_offset(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_offset_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_offset);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_size(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_size);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_tr_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_global_tr_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_tr_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_global_transpose_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_global_transpose_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_global_transpose_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_gmem_frag_offset_ir3(nir_builder *build, nir_def *src0, struct _nir_load_gmem_frag_offset_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_gmem_frag_offset_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_gmem_frag_scale_ir3(nir_builder *build, nir_def *src0, struct _nir_load_gmem_frag_scale_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_gmem_frag_scale_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_gs_header_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_gs_header_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_gs_vertex_offset_amd(nir_builder *build, struct _nir_load_gs_vertex_offset_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_gs_vertex_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_gs_wave_id_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_gs_wave_id_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_heap_descriptor(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_heap_descriptor_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_heap_descriptor);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_resource_type(intrin, indices.resource_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_helper_arg_hi_agx(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_helper_arg_hi_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_helper_arg_lo_agx(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_helper_arg_lo_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_helper_invocation(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_helper_invocation);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_helper_op_id_agx(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_helper_op_id_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_hit_attrib_amd(nir_builder *build, struct _nir_load_hit_attrib_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_hit_attrib_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_hs_out_patch_data_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_hs_out_patch_data_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_hs_patch_stride_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_hs_patch_stride_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_idvs_output_buf_index_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_idvs_output_buf_index_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_incoming_ray_payload_amd(nir_builder *build, struct _nir_load_incoming_ray_payload_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_incoming_ray_payload_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_index_size_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_index_size_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_indirect_address_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_indirect_address_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_initial_edgeflags_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_initial_edgeflags_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_inline_data_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_inline_data_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_inline_data_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input_attachment_conv_pan(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input_attachment_conv_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input_attachment_coord(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input_attachment_coord);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input_attachment_target_pan(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input_attachment_target_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input_topology_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input_topology_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_input_vertex(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_input_vertex_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_input_vertex);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_instance_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_instance_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_instance_num_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_instance_num_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_interpolated_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_interpolated_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_interpolated_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_intersection_opaque_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_intersection_opaque_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_invocation_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_invocation_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_is_first_fan_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_is_first_fan_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_is_indexed_draw(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_is_indexed_draw);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_kernel_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_kernel_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_kernel_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_layer_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_layer_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_lds_ngg_gs_out_vertex_base_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_lds_ngg_gs_out_vertex_base_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_leaf_opaque_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_leaf_opaque_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_leaf_procedural_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_leaf_procedural_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_line_coord(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_line_coord);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_line_width(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_line_width);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_local_invocation_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_local_invocation_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_local_invocation_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_local_invocation_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_local_invocation_index_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_local_invocation_index_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_local_pixel_agx(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_local_pixel_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_local_pixel_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_format(intrin, indices.format);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_local_shared_r600(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_local_shared_r600);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_lshs_vertex_stride_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_lshs_vertex_stride_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_max_polygon_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_max_polygon_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_merged_wave_info_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_merged_wave_info_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_mesh_view_count(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_mesh_view_count);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_mesh_view_indices(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_mesh_view_indices_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_mesh_view_indices);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_msaa_rate_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_msaa_rate_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_multisampled_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_multisampled_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_noperspective_varyings_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_noperspective_varyings_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_num_subgroups(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_num_subgroups);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_num_vertices(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_num_vertices);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_num_vertices_per_primitive_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_num_vertices_per_primitive_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_num_workgroups(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_num_workgroups);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ordered_id_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ordered_id_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_outgoing_ray_payload_amd(nir_builder *build, struct _nir_load_outgoing_ray_payload_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_outgoing_ray_payload_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_output(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_output);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_packed_passthrough_primitive_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_packed_passthrough_primitive_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_packed_sample_location_pco(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_packed_sample_location_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_param(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_param_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_param);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_param_idx(intrin, indices.param_idx);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_patch_vertices_in(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_patch_vertices_in);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_draw_ptr_kk(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_draw_ptr_kk);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_primitive_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_per_primitive_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_primitive_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_primitive_output(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_per_primitive_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_primitive_output);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_vertex_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_per_vertex_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_vertex_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_vertex_output(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_per_vertex_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_vertex_output);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_per_view_output(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_per_view_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_per_view_output);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_persp_center_rhw_ir3(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_persp_center_rhw_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_pipeline_stat_query_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_pipeline_stat_query_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_pixel_coord(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_pixel_coord);

      nir_def_init(&intrin->instr, &intrin->def, 2, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_pixel_coord_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_pixel_coord_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_pixel_local(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_pixel_local_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_pixel_local);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_point_coord(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_point_coord);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_point_coord_maybe_flipped(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_point_coord_maybe_flipped);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_poly_line_smooth_enabled(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_poly_line_smooth_enabled);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_polygon_stipple_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_polygon_stipple_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_polygon_stipple_buffer_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_polygon_stipple_buffer_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_preamble(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_preamble_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_preamble);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_preamble_class(intrin, indices.preamble_class);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_prim_gen_query_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_prim_gen_query_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_prim_xfb_query_enabled_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_prim_xfb_query_enabled_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_primitive_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_primitive_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_primitive_id_modulo_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_primitive_id_modulo_r600);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_primitive_id_raw_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_primitive_id_raw_r600);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_primitive_location_ir3(nir_builder *build, struct _nir_load_primitive_location_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_primitive_location_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_driver_location(intrin, indices.driver_location);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_printf_buffer_address(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_printf_buffer_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_printf_buffer_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_printf_buffer_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_provoking_last(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_provoking_last);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_provoking_vtx_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_provoking_vtx_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_provoking_vtx_in_prim_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_provoking_vtx_in_prim_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ps_iter_mask_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ps_iter_mask_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_push_constant(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_push_constant_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_push_constant);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_push_constant_zink(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_push_constant_zink_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_push_constant_zink);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_push_data_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_push_data_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_push_data_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_r600_per_vertex_input(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_r600_per_vertex_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_r600_per_vertex_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rasterization_primitive_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rasterization_primitive_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rasterization_samples_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rasterization_samples_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rasterization_stream(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rasterization_stream);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_raw_output_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_raw_output_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_raw_output_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_raw_vertex_id_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_raw_vertex_id_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_raw_vertex_offset_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_raw_vertex_offset_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_base_mem_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_base_mem_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_flags(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_flags);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_geometry_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_geometry_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_hit_kind(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_hit_kind);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_hit_sbt_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_hit_sbt_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_hit_sbt_stride_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_hit_sbt_stride_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_hw_stack_size_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_hw_stack_size_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_instance_custom_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_instance_custom_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_launch_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_launch_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_launch_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_launch_size);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_miss_sbt_addr_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_miss_sbt_addr_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_miss_sbt_stride_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_miss_sbt_stride_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_num_dss_rt_stacks_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_num_dss_rt_stacks_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_object_direction(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_object_direction);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_object_origin(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_object_origin);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_object_to_world(nir_builder *build, struct _nir_load_ray_object_to_world_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_object_to_world);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   nir_intrinsic_set_column(intrin, indices.column);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_payload_ptr_amd(nir_builder *build, unsigned bit_size, struct _nir_load_ray_payload_ptr_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_payload_ptr_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_query_global_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_query_global_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_sw_stack_size_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_sw_stack_size_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_t_max(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_t_max);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_t_min(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_t_min);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_tracing_stack_base_lvp(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_tracing_stack_base_lvp);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_triangle_vertex_positions(nir_builder *build, struct _nir_load_ray_triangle_vertex_positions_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_triangle_vertex_positions);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   nir_intrinsic_set_column(intrin, indices.column);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_world_direction(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_world_direction);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_world_origin(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_world_origin);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ray_world_to_object(nir_builder *build, struct _nir_load_ray_world_to_object_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ray_world_to_object);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);
   nir_intrinsic_set_column(intrin, indices.column);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_reg(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_reg_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_reg);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_legacy_fabs(intrin, indices.legacy_fabs);
   nir_intrinsic_set_legacy_fneg(intrin, indices.legacy_fneg);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_reg_indirect(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_reg_indirect_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_reg_indirect);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_legacy_fabs(intrin, indices.legacy_fabs);
   nir_intrinsic_set_legacy_fneg(intrin, indices.legacy_fneg);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rel_patch_id_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rel_patch_id_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_reloc_const_intel(nir_builder *build, struct _nir_load_reloc_const_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_reloc_const_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_param_idx(intrin, indices.param_idx);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_resource_heap_data(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_resource_heap_data_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_resource_heap_data);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_resume_shader_address_amd(nir_builder *build, struct _nir_load_resume_shader_address_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_resume_shader_address_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_return_param_amd(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_return_param_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_return_param_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_param_idx(intrin, indices.param_idx);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_attr_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_attr_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_attr_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_attr_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_es2gs_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_es2gs_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_esgs_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_esgs_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_gs2vs_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_gs2vs_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_gsvs_amd(nir_builder *build, struct _nir_load_ring_gsvs_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_gsvs_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_mesh_scratch_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_mesh_scratch_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_mesh_scratch_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_mesh_scratch_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_task_draw_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_task_draw_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_task_payload_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_task_payload_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_tess_factors_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_tess_factors_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_tess_factors_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_tess_factors_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_tess_offchip_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_tess_offchip_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ring_tess_offchip_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ring_tess_offchip_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ro_sink_address_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ro_sink_address_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_root_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_root_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_arg_scratch_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_arg_scratch_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_conversion_pan(nir_builder *build, struct _nir_load_rt_conversion_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_conversion_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_descriptors_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_descriptors_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_dynamic_descriptors_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_dynamic_descriptors_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_heap_resource_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_heap_resource_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_heap_sampler_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_heap_sampler_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_rt_push_constants_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_rt_push_constants_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_mask(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_mask);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_mask_in(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_mask_in);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_pos(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_pos);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_pos_from_id(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_pos_from_id);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_pos_or_center(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_pos_or_center);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_positions_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_positions_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_positions_amd(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_positions_amd);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sample_positions_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sample_positions_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sampler_handle_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sampler_handle_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sampler_handle_kk(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sampler_handle_kk);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sampler_lod_parameters(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sampler_lod_parameters);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_samples_log2_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_samples_log2_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_savmsk_vm_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_savmsk_vm_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sbt_base_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sbt_base_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sbt_offset_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sbt_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sbt_stride_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sbt_stride_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scalar_arg_amd(nir_builder *build, unsigned num_components, struct _nir_load_scalar_arg_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scalar_arg_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_arg_upper_bound_u32_amd(intrin, indices.arg_upper_bound_u32_amd);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scalar_arg_wg_div_amd(nir_builder *build, unsigned num_components, struct _nir_load_scalar_arg_wg_div_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scalar_arg_wg_div_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_arg_upper_bound_u32_amd(intrin, indices.arg_upper_bound_u32_amd);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scratch(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_scratch_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scratch);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scratch_base_ptr(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_scratch_base_ptr_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scratch_base_ptr);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scratch_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_scratch_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scratch_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_scratch_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_scratch_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_scratch_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_call_data_offset_lvp(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_call_data_offset_lvp);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_indirect_data_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_shader_indirect_data_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_indirect_data_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_output_pan(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_output_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_part_tests_zs_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_part_tests_zs_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shader_record_ptr(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shader_record_ptr);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_shared_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared2_amd(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_shared2_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared2_amd);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_offset0(intrin, indices.offset0);
   nir_intrinsic_set_offset1(intrin, indices.offset1);
   nir_intrinsic_set_st64(intrin, indices.st64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_base_ptr(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_base_ptr);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_shared_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_ir3(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_shared_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_ir3);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_lock_nv(nir_builder *build, unsigned bit_size, nir_def *src0, struct _nir_load_shared_lock_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_lock_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_nv(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_shared_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_offset_shift_nv(intrin, indices.offset_shift_nv);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_reg_alloc_size_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_reg_alloc_size_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_shared_uniform_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_shared_uniform_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_shared_uniform_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_simd_width_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_simd_width_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_slot_num_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_slot_num_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sm_count_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sm_count_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sm_id_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sm_id_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_smp_meta_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_load_smp_meta_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_smp_meta_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_smp_state_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_load_smp_state_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_smp_state_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ssbo_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo_address(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ssbo_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo_address);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ssbo_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ssbo_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo_ir3(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_ssbo_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo_ir3);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ssbo_uniform_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ssbo_uniform_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ssbo_uniform_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_stack(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_stack_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_stack);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_value_id(intrin, indices.value_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_stat_query_address_poly(nir_builder *build, struct _nir_load_stat_query_address_poly_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_stat_query_address_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_streamout_buffer_amd(nir_builder *build, struct _nir_load_streamout_buffer_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_streamout_buffer_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_streamout_config_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_streamout_config_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_streamout_offset_amd(nir_builder *build, struct _nir_load_streamout_offset_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_streamout_offset_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_streamout_write_index_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_streamout_write_index_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_eq_mask(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_eq_mask);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_ge_mask(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_ge_mask);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_gt_mask(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_gt_mask);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_id_shift_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_id_shift_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_invocation(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_invocation);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_le_mask(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_le_mask);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_lt_mask(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_lt_mask);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_subgroup_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_subgroup_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sysval_agx(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_load_sysval_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sysval_agx);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_sysval_nv(nir_builder *build, unsigned bit_size, struct _nir_load_sysval_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_sysval_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_divergent(intrin, indices.divergent);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_task_payload(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_task_payload_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_task_payload);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_task_ring_entry_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_task_ring_entry_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_header_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_header_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_in_param_base_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_in_param_base_r600);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_mem_attrib_stride(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_mem_attrib_stride);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_num_patches_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_num_patches_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_out_param_base_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_out_param_base_r600);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_primitive_mode_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_primitive_mode_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_rel_patch_id_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_rel_patch_id_r600);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_tess_factor_base_r600(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_tess_factor_base_r600);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tcs_tess_levels_to_tes_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tcs_tess_levels_to_tes_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_config_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_config_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_coord(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_coord);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_coord_xy(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_coord_xy);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_factor_base_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_factor_base_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_level_inner(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_level_inner);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_level_inner_default(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_level_inner_default);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_level_outer(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_level_outer);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_level_outer_default(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_level_outer_default);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_param_base_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_param_base_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_param_buffer_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_param_buffer_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tess_rel_patch_id_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tess_rel_patch_id_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tex_meta_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_load_tex_meta_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tex_meta_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tex_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_tex_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tex_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tex_sprite_mask_agx(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tex_sprite_mask_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tex_state_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_load_tex_state_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tex_state_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_texture_handle_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_texture_handle_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_texture_handle_kk(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_texture_handle_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_texture_handle_kk);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_image_dim(intrin, indices.image_dim);
   nir_intrinsic_set_image_array(intrin, indices.image_array);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_texture_scale(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_texture_scale);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_texture_size_etna(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_texture_size_etna);

      nir_def_init(&intrin->instr, &intrin->def, 3, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tile_coord_pco(nir_builder *build, unsigned num_components, struct _nir_load_tile_coord_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tile_coord_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tile_image(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_tile_image_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tile_image);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tile_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_tile_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tile_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tile_res_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_load_tile_res_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tile_res_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tiled_offset_pco(nir_builder *build, struct _nir_load_tiled_offset_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tiled_offset_pco);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_tlb_color_brcm(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_tlb_color_brcm_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_tlb_color_brcm);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_topology_id_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_topology_id_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ttmp_register_amd(nir_builder *build, struct _nir_load_ttmp_register_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ttmp_register_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_arg_upper_bound_u32_amd(intrin, indices.arg_upper_bound_u32_amd);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ttmp_register_wg_div_amd(nir_builder *build, struct _nir_load_ttmp_register_wg_div_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ttmp_register_wg_div_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_arg_upper_bound_u32_amd(intrin, indices.arg_upper_bound_u32_amd);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_typed_buffer_amd(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_load_typed_buffer_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_typed_buffer_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_uav_ir3(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_uav_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_uav_ir3);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ubo(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ubo_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ubo);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_range_base(intrin, indices.range_base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ubo_uniform_block_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ubo_uniform_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ubo_uniform_block_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = intrin->def.bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_ubo_vec4(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_ubo_vec4_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_ubo_vec4);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_uniform(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_uniform_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_uniform);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_urb_input_handle_indexed_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_urb_input_handle_indexed_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_urb_input_handle_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_urb_input_handle_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_urb_lsc_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_urb_lsc_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_urb_lsc_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_urb_output_handle_intel(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_urb_output_handle_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_urb_vec4_intel(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_urb_vec4_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_urb_vec4_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_use_float_frag_coord_xy_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_use_float_frag_coord_xy_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_use_quad_pos_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_use_quad_pos_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_use_sample_mask_in_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_use_sample_mask_in_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_user_clip_plane(nir_builder *build, struct _nir_load_user_clip_plane_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_user_clip_plane);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);
   nir_intrinsic_set_ucp_id(intrin, indices.ucp_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_user_data_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_user_data_amd);

      nir_def_init(&intrin->instr, &intrin->def, 8, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_uvs_index_agx(nir_builder *build, struct _nir_load_uvs_index_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_uvs_index_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 16);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_var_buf_flat_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_var_buf_flat_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_var_buf_flat_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_var_buf_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_var_buf_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_var_buf_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_var_flat_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_var_flat_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_var_flat_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_var_pan(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_load_var_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_var_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.dest_type)
      indices.dest_type = (nir_alu_type)(nir_type_float | bit_size);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_var_special_pan(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_load_var_special_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_var_special_pan);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vbo_base_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vbo_base_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vbo_stride_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vbo_stride_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vector_arg_amd(nir_builder *build, unsigned num_components, struct _nir_load_vector_arg_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vector_arg_amd);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_arg_upper_bound_u32_amd(intrin, indices.arg_upper_bound_u32_amd);
   nir_intrinsic_set_arg_num_lsb_zero(intrin, indices.arg_num_lsb_zero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vertex_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vertex_id);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vertex_id_zero_base(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vertex_id_zero_base);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vertex_param_buffer_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vertex_param_buffer_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_view_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_view_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_offset(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_offset);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_scale(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_scale);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_x_offset(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_x_offset);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_x_scale(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_x_scale);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_y_offset(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_y_offset);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_y_scale(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_y_scale);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_z_offset(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_z_offset);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_viewport_z_scale(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_viewport_z_scale);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vs_outputs_poly(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vs_outputs_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vs_primitive_stride_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vs_primitive_stride_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vs_vertex_stride_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vs_vertex_stride_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vtxin_pco(nir_builder *build, unsigned num_components, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vtxin_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_vulkan_descriptor(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_load_vulkan_descriptor_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_vulkan_descriptor);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_type(intrin, indices.desc_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_warp_id_arm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_warp_id_arm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_warp_id_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_warp_id_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_warp_max_id_arm(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_warp_max_id_arm);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_warps_per_sm_nv(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_warps_per_sm_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_work_dim(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_work_dim);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_workgroup_id(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_workgroup_id);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_workgroup_index(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_workgroup_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_workgroup_num_input_primitives_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_workgroup_num_input_primitives_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_workgroup_num_input_vertices_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_workgroup_num_input_vertices_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_workgroup_size(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_workgroup_size);

      nir_def_init(&intrin->instr, &intrin->def, 3, 32);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_xfb_address(nir_builder *build, unsigned bit_size, struct _nir_load_xfb_address_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_xfb_address);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_xfb_index_buffer(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_xfb_index_buffer);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_xfb_size(nir_builder *build, struct _nir_load_xfb_size_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_xfb_size);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_load_xfb_state_address_gfx12_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_load_xfb_state_address_gfx12_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_masked_swizzle_amd(nir_builder *build, nir_def *src0, struct _nir_masked_swizzle_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_masked_swizzle_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_swizzle_mask(intrin, indices.swizzle_mask);
   nir_intrinsic_set_fetch_inactive(intrin, indices.fetch_inactive);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_match_any_nv(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_match_any_nv);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_mbcnt_amd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_mbcnt_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_memcpy_deref(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_memcpy_deref_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_memcpy_deref);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_dst_access(intrin, indices.dst_access);
   nir_intrinsic_set_src_access(intrin, indices.src_access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_mutex_pco(nir_builder *build, struct _nir_mutex_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_mutex_pco);

   nir_intrinsic_set_mutex_id_pco(intrin, indices.mutex_id_pco);
   nir_intrinsic_set_mutex_op_pco(intrin, indices.mutex_op_pco);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_nop(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_nop);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_nop_amd(nir_builder *build, struct _nir_nop_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_nop_amd);

   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_optimization_barrier_sgpr_amd(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_optimization_barrier_sgpr_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_optimization_barrier_vgpr_amd(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_optimization_barrier_vgpr_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ordered_add_loop_gfx12_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ordered_add_loop_gfx12_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ordered_xfb_counter_add_gfx11_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_ordered_xfb_counter_add_gfx11_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ordered_xfb_counter_add_gfx11_amd);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_overwrite_tes_arguments_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_overwrite_tes_arguments_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_overwrite_vs_arguments_amd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_overwrite_vs_arguments_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_pack_pco(nir_builder *build, nir_def *src0, struct _nir_pack_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_pack_pco);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_format(intrin, indices.format);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_parallel_copy(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_parallel_copy_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_parallel_copy);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_src_is_reg(intrin, indices.src_is_reg);
   nir_intrinsic_set_dst_is_reg(intrin, indices.dst_is_reg);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_pck_prog_pco(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_pck_prog_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_pck_prog_pco);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_scale(intrin, indices.scale);
   nir_intrinsic_set_roundzero(intrin, indices.roundzero);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_pin_cx_handle_nv(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_pin_cx_handle_nv);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_preamble_end_ir3(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_preamble_end_ir3);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_preamble_start_ir3(nir_builder *build, unsigned bit_size)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_preamble_start_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_prefetch_sam_ir3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_prefetch_sam_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_prefetch_tex_ir3(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_prefetch_tex_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_prefetch_ubo_ir3(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_prefetch_ubo_ir3);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_printf(nir_builder *build, nir_def *src0, struct _nir_printf_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_printf);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_fmt_idx(intrin, indices.fmt_idx);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_printf_abort(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_printf_abort);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_quad_ballot_agx(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_ballot_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_broadcast(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_broadcast);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_swap_diagonal(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_swap_diagonal);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_swap_horizontal(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_swap_horizontal);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_swap_vertical(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_swap_vertical);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_swizzle_amd(nir_builder *build, nir_def *src0, struct _nir_quad_swizzle_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_swizzle_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_swizzle_mask(intrin, indices.swizzle_mask);
   nir_intrinsic_set_fetch_inactive(intrin, indices.fetch_inactive);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_vote_all(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_vote_all);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_quad_vote_any(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_quad_vote_any);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_r600_indirect_vertex_at_index(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_r600_indirect_vertex_at_index);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ray_intersection_ir3(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ray_intersection_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 5, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_read_first_invocation(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_read_first_invocation);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_read_getlast_ir3(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_read_getlast_ir3);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_read_invocation(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_read_invocation);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_read_invocation_cond_ir3(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_read_invocation_cond_ir3);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_reduce(nir_builder *build, nir_def *src0, struct _nir_reduce_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_reduce);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);
   nir_intrinsic_set_cluster_size(intrin, indices.cluster_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_reduce_clusters_ir3(nir_builder *build, nir_def *src0, struct _nir_reduce_clusters_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_reduce_clusters_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_reduction_op(intrin, indices.reduction_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_report_ray_intersection(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_report_ray_intersection);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_resbase_ir3(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_resbase_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 2, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_resource_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_resource_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_resource_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_resource_access_intel(intrin, indices.resource_access_intel);
   nir_intrinsic_set_resource_block_intel(intrin, indices.resource_block_intel);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ro_to_rw_poly(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ro_to_rw_poly);

      nir_def_init(&intrin->instr, &intrin->def, 1, 64);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_rotate(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_rotate_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rotate);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_cluster_size(intrin, indices.cluster_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_rq_confirm_intersection(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_confirm_intersection);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rq_generate_intersection(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_generate_intersection);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rq_initialize(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6, nir_def *src7)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_initialize);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   intrin->src[6] = nir_src_for_ssa(src6);
   intrin->src[7] = nir_src_for_ssa(src7);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_rq_load(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_rq_load_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_load);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_ray_query_value(intrin, indices.ray_query_value);
   nir_intrinsic_set_committed(intrin, indices.committed);
   nir_intrinsic_set_column(intrin, indices.column);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_rq_proceed(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_proceed);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_rq_terminate(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rq_terminate);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rt_execute_callable(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_rt_execute_callable_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rt_execute_callable);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_stack_size(intrin, indices.stack_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rt_resume(nir_builder *build, struct _nir_rt_resume_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rt_resume);

   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_stack_size(intrin, indices.stack_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rt_return_amd(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rt_return_amd);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_rt_trace_ray(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6, nir_def *src7, nir_def *src8, nir_def *src9, nir_def *src10, struct _nir_rt_trace_ray_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_rt_trace_ray);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   intrin->src[6] = nir_src_for_ssa(src6);
   intrin->src[7] = nir_src_for_ssa(src7);
   intrin->src[8] = nir_src_for_ssa(src8);
   intrin->src[9] = nir_src_for_ssa(src9);
   intrin->src[10] = nir_src_for_ssa(src10);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_stack_size(intrin, indices.stack_size);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_sample_mask_agx(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sample_mask_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_select_vertex_poly(nir_builder *build, nir_def *src0, struct _nir_select_vertex_poly_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_select_vertex_poly);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_sendmsg_amd(nir_builder *build, nir_def *src0, struct _nir_sendmsg_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sendmsg_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_set_next_call_pc_amd(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_set_next_call_pc_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_set_vertex_and_primitive_count(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_set_vertex_and_primitive_count_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_set_vertex_and_primitive_count);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_stream_id(intrin, indices.stream_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_shader_clock(nir_builder *build, struct _nir_shader_clock_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shader_clock);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   nir_intrinsic_set_memory_scope(intrin, indices.memory_scope);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_append_amd(nir_builder *build, struct _nir_shared_append_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_append_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_shared_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_atomic);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_atomic_nv(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_shared_atomic_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_atomic_nv);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift_nv(intrin, indices.offset_shift_nv);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_shared_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_atomic_swap);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_atomic_swap_nv(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_shared_atomic_swap_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_atomic_swap_nv);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift_nv(intrin, indices.offset_shift_nv);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shared_consume_amd(nir_builder *build, struct _nir_shared_consume_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shared_consume_amd);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_down(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_down);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_down_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_down_intel);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_down_uniform_ir3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_down_uniform_ir3);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_intel(nir_builder *build, unsigned num_components, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_intel);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_up(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_up);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_up_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_up_intel);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_up_uniform_ir3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_up_uniform_ir3);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_xor(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_xor);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_shuffle_xor_uniform_ir3(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_shuffle_xor_uniform_ir3);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_sleep_amd(nir_builder *build, struct _nir_sleep_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sleep_amd);

   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_smp_coeffs_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_smp_coeffs_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_smp_coeffs_pco);

      nir_def_init(&intrin->instr, &intrin->def, 16, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_smp_flags_pco(intrin, indices.smp_flags_pco);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_smp_pco(nir_builder *build, unsigned num_components, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_smp_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_smp_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_smp_flags_pco(intrin, indices.smp_flags_pco);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_smp_raw_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_smp_raw_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_smp_raw_pco);

      nir_def_init(&intrin->instr, &intrin->def, 16, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_smp_flags_pco(intrin, indices.smp_flags_pco);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_enabled_channels(intrin, indices.enabled_channels);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_smp_write_pco(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_smp_write_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_smp_write_pco);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_smp_flags_pco(intrin, indices.smp_flags_pco);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_sparse_residency_code_and(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sparse_residency_code_and);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_ssa_bar_nv(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssa_bar_nv);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_ssbo_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_ssbo_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssbo_atomic);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ssbo_atomic_ir3(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_ssbo_atomic_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssbo_atomic_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ssbo_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_ssbo_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssbo_atomic_swap);

   intrin->num_components = src2->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ssbo_atomic_swap_ir3(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_ssbo_atomic_swap_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssbo_atomic_swap_ir3);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_ssbo_descriptor_amd(nir_builder *build, nir_def *src0, struct _nir_ssbo_descriptor_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_ssbo_descriptor_amd);

      nir_def_init(&intrin->instr, &intrin->def, 4, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_stack_map_agx(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_stack_map_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_stack_unmap_agx(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_stack_unmap_agx);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_store_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_agx);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_sign_extend(intrin, indices.sign_extend);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_buffer_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_store_buffer_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_buffer_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_memory_modes(intrin, indices.memory_modes);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_clip_distance_kk(nir_builder *build, nir_def *src0, struct _nir_store_clip_distance_kk_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_clip_distance_kk);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_combined_output_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_store_combined_output_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_combined_output_pan);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_const_ir3(nir_builder *build, nir_def *src0, struct _nir_store_const_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_const_ir3);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_deref(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_deref_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_deref);

   intrin->num_components = src1->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_deref_block_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_deref_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_deref_block_intel);

   intrin->num_components = src1->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_global_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_2x32(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_global_2x32_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_2x32);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_global_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_block_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_global_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_block_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_cvt_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_global_cvt_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_cvt_pan);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_etna(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_global_etna_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_etna);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_global_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_global_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_nv);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_offset(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_global_offset_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_offset);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_global_psiz_pan(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_global_psiz_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_global_psiz_pan);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_hit_attrib_amd(nir_builder *build, nir_def *src0, struct _nir_store_hit_attrib_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_hit_attrib_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_incoming_ray_payload_amd(nir_builder *build, nir_def *src0, struct _nir_store_incoming_ray_payload_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_incoming_ray_payload_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_local_pixel_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_local_pixel_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_local_pixel_agx);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_explicit_coord(intrin, indices.explicit_coord);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_local_shared_r600(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_local_shared_r600_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_local_shared_r600);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_outgoing_ray_payload_amd(nir_builder *build, nir_def *src0, struct _nir_store_outgoing_ray_payload_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_outgoing_ray_payload_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_output(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_output);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_io_xfb(intrin, indices.io_xfb);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_param_amd(nir_builder *build, nir_def *src0, struct _nir_store_param_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_param_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_param_idx(intrin, indices.param_idx);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_per_primitive_output(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_per_primitive_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_per_primitive_output);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_per_primitive_payload_intel(nir_builder *build, nir_def *src0, struct _nir_store_per_primitive_payload_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_per_primitive_payload_intel);

   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_per_vertex_output(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_per_vertex_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_per_vertex_output);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_per_view_output(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_per_view_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_per_view_output);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_pixel_local(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_pixel_local_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_pixel_local);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_range(intrin, indices.range);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_preamble(nir_builder *build, nir_def *src0, struct _nir_store_preamble_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_preamble);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_preamble_class(intrin, indices.preamble_class);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_preamble_dynamic(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_preamble_dynamic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_preamble_dynamic);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_preamble_class(intrin, indices.preamble_class);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_raw_output_pan(nir_builder *build, nir_def *src0, struct _nir_store_raw_output_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_raw_output_pan);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_reg(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_reg_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_reg);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_legacy_fsat(intrin, indices.legacy_fsat);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_reg_indirect(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_reg_indirect_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_reg_indirect);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_legacy_fsat(intrin, indices.legacy_fsat);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_render_target_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, struct _nir_store_render_target_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_render_target_intel);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   nir_intrinsic_set_target(intrin, indices.target);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_scalar_arg_amd(nir_builder *build, nir_def *src0, struct _nir_store_scalar_arg_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_scalar_arg_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_scratch(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_scratch_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_scratch);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_scratch_intel(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_scratch_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_scratch_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_scratch_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_scratch_nv);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_shared(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_shared_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_shared2_amd(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_shared2_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared2_amd);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_offset0(intrin, indices.offset0);
   nir_intrinsic_set_offset1(intrin, indices.offset1);
   nir_intrinsic_set_st64(intrin, indices.st64);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_shared_block_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_shared_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared_block_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_shared_ir3(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_shared_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared_ir3);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_shared_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_shared_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared_nv);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_offset_shift_nv(intrin, indices.offset_shift_nv);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_store_shared_unlock_nv(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_store_shared_unlock_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_shared_unlock_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_store_ssbo(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_ssbo_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_ssbo);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_ssbo_block_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_ssbo_block_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_ssbo_block_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_ssbo_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_ssbo_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_ssbo_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_ssbo_ir3(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_store_ssbo_ir3_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_ssbo_ir3);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_offset_shift(intrin, indices.offset_shift);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_stack(nir_builder *build, nir_def *src0, struct _nir_store_stack_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_stack);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_call_idx(intrin, indices.call_idx);
   nir_intrinsic_set_value_id(intrin, indices.value_id);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_task_payload(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_task_payload_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_task_payload);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   if (!indices.align_mul)
      indices.align_mul = src0->bit_size / 8u;
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);
   nir_intrinsic_set_align_mul(intrin, indices.align_mul);
   nir_intrinsic_set_align_offset(intrin, indices.align_offset);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_tf_r600(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_tf_r600);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_tile_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_store_tile_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_tile_pan);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   if (!indices.io_semantics.num_slots)
      indices.io_semantics.num_slots = 1;
   if (!indices.src_type)
      indices.src_type = (nir_alu_type)(nir_type_float | src0->bit_size);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_src_type(intrin, indices.src_type);
   nir_intrinsic_set_io_semantics(intrin, indices.io_semantics);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_tlb_sample_color_v3d(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_tlb_sample_color_v3d_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_tlb_sample_color_v3d);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_component(intrin, indices.component);
   nir_intrinsic_set_src_type(intrin, indices.src_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_urb_lsc_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_store_urb_lsc_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_urb_lsc_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_urb_vec4_intel(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_store_urb_vec4_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_urb_vec4_intel);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_uvs_agx(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_uvs_agx);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_vector_arg_amd(nir_builder *build, nir_def *src0, struct _nir_store_vector_arg_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_vector_arg_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_store_zs_agx(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_store_zs_agx_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_store_zs_agx);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_strict_wqm_coord_amd(nir_builder *build, nir_def *src0, struct _nir_strict_wqm_coord_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_strict_wqm_coord_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_subfm_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_subfm_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_subfm_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_suclamp_nv(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_suclamp_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_suclamp_nv);

      nir_def_init(&intrin->instr, &intrin->def, 2, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_sueau_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sueau_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_suldga_nv(nir_builder *build, unsigned num_components, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_suldga_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_suldga_nv);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_format(intrin, indices.format);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_sustga_nv(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_sustga_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_sustga_nv);

   intrin->num_components = src3->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_access(intrin, indices.access);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_task_payload_atomic(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_task_payload_atomic_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_task_payload_atomic);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_task_payload_atomic_swap(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_task_payload_atomic_swap_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_task_payload_atomic_swap);

   intrin->num_components = src1->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_base(intrin, indices.base);
   nir_intrinsic_set_atomic_op(intrin, indices.atomic_op);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_terminate(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_terminate);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_terminate_if(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_terminate_if);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_terminate_ray(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_terminate_ray);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_test_fs_config_intel(nir_builder *build, unsigned bit_size, struct _nir_test_fs_config_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_test_fs_config_intel);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_texc0_pan(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_texc0_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_texc0_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_texc1_pan(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, struct _nir_texc1_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_texc1_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_texc2_pan(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, struct _nir_texc2_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_texc2_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_texs_2d_pan(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_texs_2d_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_texs_2d_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_texs_cube_pan(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_texs_cube_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_texs_cube_pan);

      nir_def_init(&intrin->instr, &intrin->def, 4, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_dest_type(intrin, indices.dest_type);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_trace_ray(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, nir_def *src3, nir_def *src4, nir_def *src5, nir_def *src6, nir_def *src7, nir_def *src8, nir_def *src9, nir_def *src10)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_trace_ray);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   intrin->src[3] = nir_src_for_ssa(src3);
   intrin->src[4] = nir_src_for_ssa(src4);
   intrin->src[5] = nir_src_for_ssa(src5);
   intrin->src[6] = nir_src_for_ssa(src6);
   intrin->src[7] = nir_src_for_ssa(src7);
   intrin->src[8] = nir_src_for_ssa(src8);
   intrin->src[9] = nir_src_for_ssa(src9);
   intrin->src[10] = nir_src_for_ssa(src10);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_trace_ray_intel(nir_builder *build, nir_def *src0, nir_def *src1, struct _nir_trace_ray_intel_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_trace_ray_intel);

   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_synchronous(intrin, indices.synchronous);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_unit_test_assert_eq(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unit_test_assert_eq);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_unit_test_divergent_input(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_unit_test_divergent_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unit_test_divergent_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_unit_test_output(nir_builder *build, nir_def *src0, struct _nir_unit_test_output_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unit_test_output);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_unit_test_uniform_input(nir_builder *build, unsigned num_components, unsigned bit_size, struct _nir_unit_test_uniform_input_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unit_test_uniform_input);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_unpack_pco(nir_builder *build, unsigned num_components, nir_def *src0, struct _nir_unpack_pco_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unpack_pco);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_format(intrin, indices.format);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_unpin_cx_handle_nv(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_unpin_cx_handle_nv);

   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_use(nir_builder *build, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_use);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_intrinsic_instr *
_nir_build_uvsw_write_pco(nir_builder *build, nir_def *src0, nir_def *src1)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_uvsw_write_pco);

   intrin->num_components = src1->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_vild_nv(nir_builder *build, nir_def *src0, struct _nir_vild_nv_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vild_nv);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_base(intrin, indices.base);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vote_all(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vote_all);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vote_any(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vote_any);

      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vote_feq(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vote_feq);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vote_ieq(nir_builder *build, unsigned bit_size, nir_def *src0)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vote_ieq);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, 1, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vulkan_resource_index(nir_builder *build, unsigned num_components, unsigned bit_size, nir_def *src0, struct _nir_vulkan_resource_index_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vulkan_resource_index);

   intrin->num_components = (uint8_t)num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   nir_intrinsic_set_desc_set(intrin, indices.desc_set);
   nir_intrinsic_set_binding(intrin, indices.binding);
   nir_intrinsic_set_desc_type(intrin, indices.desc_type);
   nir_intrinsic_set_resource_type(intrin, indices.resource_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_def *
_nir_build_vulkan_resource_reindex(nir_builder *build, unsigned bit_size, nir_def *src0, nir_def *src1, struct _nir_vulkan_resource_reindex_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_vulkan_resource_reindex);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   nir_intrinsic_set_desc_type(intrin, indices.desc_type);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_wop_pco(nir_builder *build)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_wop_pco);


   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_write_invocation_amd(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_write_invocation_amd);

   intrin->num_components = src0->num_components;
      nir_def_init(&intrin->instr, &intrin->def, intrin->num_components, src0->bit_size);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}
static inline nir_intrinsic_instr *
_nir_build_xfb_counter_sub_gfx11_amd(nir_builder *build, nir_def *src0, struct _nir_xfb_counter_sub_gfx11_amd_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_xfb_counter_sub_gfx11_amd);

   intrin->num_components = src0->num_components;
   intrin->src[0] = nir_src_for_ssa(src0);
   if (!indices.write_mask)
      indices.write_mask = BITFIELD_MASK(intrin->num_components);
   nir_intrinsic_set_write_mask(intrin, indices.write_mask);

   nir_builder_instr_insert(build, &intrin->instr);
   return intrin;
}
static inline nir_def *
_nir_build_zs_emit_pan(nir_builder *build, nir_def *src0, nir_def *src1, nir_def *src2, struct _nir_zs_emit_pan_indices indices)
{
   nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(
      build->shader, nir_intrinsic_zs_emit_pan);

      nir_def_init(&intrin->instr, &intrin->def, 1, 32);
   intrin->src[0] = nir_src_for_ssa(src0);
   intrin->src[1] = nir_src_for_ssa(src1);
   intrin->src[2] = nir_src_for_ssa(src2);
   nir_intrinsic_set_flags(intrin, indices.flags);

   nir_builder_instr_insert(build, &intrin->instr);
   return &intrin->def;
}

#define nir_abort _nir_build_abort
#define nir_accept_ray_intersection _nir_build_accept_ray_intersection
#ifdef __cplusplus
#define nir_addr_mode_is(build, bit_size, src0, ...) \
_nir_build_addr_mode_is(build, bit_size, src0, _nir_addr_mode_is_indices{0, __VA_ARGS__})
#else
#define nir_addr_mode_is(build, bit_size, src0, ...) \
_nir_build_addr_mode_is(build, bit_size, src0, (struct _nir_addr_mode_is_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_al2p_nv(build, src0, ...) \
_nir_build_al2p_nv(build, src0, _nir_al2p_nv_indices{0, __VA_ARGS__})
#else
#define nir_al2p_nv(build, src0, ...) \
_nir_build_al2p_nv(build, src0, (struct _nir_al2p_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ald_nv(build, num_components, src0, src1, ...) \
_nir_build_ald_nv(build, num_components, src0, src1, _nir_ald_nv_indices{0, __VA_ARGS__})
#else
#define nir_ald_nv(build, num_components, src0, src1, ...) \
_nir_build_ald_nv(build, num_components, src0, src1, (struct _nir_ald_nv_indices){0, __VA_ARGS__})
#endif
#define nir_alpha_to_coverage _nir_build_alpha_to_coverage
#define nir_alphatst_pco _nir_build_alphatst_pco
#define nir_as_uniform _nir_build_as_uniform
#ifdef __cplusplus
#define nir_ast_nv(build, src0, src1, src2, ...) \
_nir_build_ast_nv(build, src0, src1, src2, _nir_ast_nv_indices{0, __VA_ARGS__})
#else
#define nir_ast_nv(build, src0, src1, src2, ...) \
_nir_build_ast_nv(build, src0, src1, src2, (struct _nir_ast_nv_indices){0, __VA_ARGS__})
#endif
#define nir_atest_pan _nir_build_atest_pan
#ifdef __cplusplus
#define nir_atomic_add_gen_prim_count_amd(build, src0, ...) \
_nir_build_atomic_add_gen_prim_count_amd(build, src0, _nir_atomic_add_gen_prim_count_amd_indices{0, __VA_ARGS__})
#else
#define nir_atomic_add_gen_prim_count_amd(build, src0, ...) \
_nir_build_atomic_add_gen_prim_count_amd(build, src0, (struct _nir_atomic_add_gen_prim_count_amd_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_add_gs_emit_prim_count_amd _nir_build_atomic_add_gs_emit_prim_count_amd
#define nir_atomic_add_shader_invocation_count_amd _nir_build_atomic_add_shader_invocation_count_amd
#ifdef __cplusplus
#define nir_atomic_add_xfb_prim_count_amd(build, src0, ...) \
_nir_build_atomic_add_xfb_prim_count_amd(build, src0, _nir_atomic_add_xfb_prim_count_amd_indices{0, __VA_ARGS__})
#else
#define nir_atomic_add_xfb_prim_count_amd(build, src0, ...) \
_nir_build_atomic_add_xfb_prim_count_amd(build, src0, (struct _nir_atomic_add_xfb_prim_count_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_atomic_counter_add(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_add(build, bit_size, src0, src1, _nir_atomic_counter_add_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_add(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_add(build, bit_size, src0, src1, (struct _nir_atomic_counter_add_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_add_deref _nir_build_atomic_counter_add_deref
#ifdef __cplusplus
#define nir_atomic_counter_and(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_and(build, bit_size, src0, src1, _nir_atomic_counter_and_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_and(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_and(build, bit_size, src0, src1, (struct _nir_atomic_counter_and_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_and_deref _nir_build_atomic_counter_and_deref
#ifdef __cplusplus
#define nir_atomic_counter_comp_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_atomic_counter_comp_swap(build, bit_size, src0, src1, src2, _nir_atomic_counter_comp_swap_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_comp_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_atomic_counter_comp_swap(build, bit_size, src0, src1, src2, (struct _nir_atomic_counter_comp_swap_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_comp_swap_deref _nir_build_atomic_counter_comp_swap_deref
#ifdef __cplusplus
#define nir_atomic_counter_exchange(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_exchange(build, bit_size, src0, src1, _nir_atomic_counter_exchange_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_exchange(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_exchange(build, bit_size, src0, src1, (struct _nir_atomic_counter_exchange_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_exchange_deref _nir_build_atomic_counter_exchange_deref
#ifdef __cplusplus
#define nir_atomic_counter_inc(build, bit_size, src0, ...) \
_nir_build_atomic_counter_inc(build, bit_size, src0, _nir_atomic_counter_inc_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_inc(build, bit_size, src0, ...) \
_nir_build_atomic_counter_inc(build, bit_size, src0, (struct _nir_atomic_counter_inc_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_inc_deref _nir_build_atomic_counter_inc_deref
#ifdef __cplusplus
#define nir_atomic_counter_max(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_max(build, bit_size, src0, src1, _nir_atomic_counter_max_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_max(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_max(build, bit_size, src0, src1, (struct _nir_atomic_counter_max_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_max_deref _nir_build_atomic_counter_max_deref
#ifdef __cplusplus
#define nir_atomic_counter_min(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_min(build, bit_size, src0, src1, _nir_atomic_counter_min_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_min(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_min(build, bit_size, src0, src1, (struct _nir_atomic_counter_min_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_min_deref _nir_build_atomic_counter_min_deref
#ifdef __cplusplus
#define nir_atomic_counter_or(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_or(build, bit_size, src0, src1, _nir_atomic_counter_or_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_or(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_or(build, bit_size, src0, src1, (struct _nir_atomic_counter_or_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_or_deref _nir_build_atomic_counter_or_deref
#ifdef __cplusplus
#define nir_atomic_counter_post_dec(build, bit_size, src0, ...) \
_nir_build_atomic_counter_post_dec(build, bit_size, src0, _nir_atomic_counter_post_dec_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_post_dec(build, bit_size, src0, ...) \
_nir_build_atomic_counter_post_dec(build, bit_size, src0, (struct _nir_atomic_counter_post_dec_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_post_dec_deref _nir_build_atomic_counter_post_dec_deref
#ifdef __cplusplus
#define nir_atomic_counter_pre_dec(build, bit_size, src0, ...) \
_nir_build_atomic_counter_pre_dec(build, bit_size, src0, _nir_atomic_counter_pre_dec_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_pre_dec(build, bit_size, src0, ...) \
_nir_build_atomic_counter_pre_dec(build, bit_size, src0, (struct _nir_atomic_counter_pre_dec_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_pre_dec_deref _nir_build_atomic_counter_pre_dec_deref
#ifdef __cplusplus
#define nir_atomic_counter_read(build, bit_size, src0, ...) \
_nir_build_atomic_counter_read(build, bit_size, src0, _nir_atomic_counter_read_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_read(build, bit_size, src0, ...) \
_nir_build_atomic_counter_read(build, bit_size, src0, (struct _nir_atomic_counter_read_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_read_deref _nir_build_atomic_counter_read_deref
#ifdef __cplusplus
#define nir_atomic_counter_xor(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_xor(build, bit_size, src0, src1, _nir_atomic_counter_xor_indices{0, __VA_ARGS__})
#else
#define nir_atomic_counter_xor(build, bit_size, src0, src1, ...) \
_nir_build_atomic_counter_xor(build, bit_size, src0, src1, (struct _nir_atomic_counter_xor_indices){0, __VA_ARGS__})
#endif
#define nir_atomic_counter_xor_deref _nir_build_atomic_counter_xor_deref
#define nir_ballot _nir_build_ballot
#define nir_ballot_bit_count_exclusive _nir_build_ballot_bit_count_exclusive
#define nir_ballot_bit_count_inclusive _nir_build_ballot_bit_count_inclusive
#define nir_ballot_bit_count_reduce _nir_build_ballot_bit_count_reduce
#define nir_ballot_bitfield_extract _nir_build_ballot_bitfield_extract
#define nir_ballot_find_lsb _nir_build_ballot_find_lsb
#define nir_ballot_find_msb _nir_build_ballot_find_msb
#define nir_ballot_relaxed _nir_build_ballot_relaxed
#define nir_bar_break_nv _nir_build_bar_break_nv
#define nir_bar_set_nv _nir_build_bar_set_nv
#define nir_bar_sync_nv _nir_build_bar_sync_nv
#ifdef __cplusplus
#define nir_barrier(build, ...) \
_nir_build_barrier(build, _nir_barrier_indices{0, __VA_ARGS__})
#else
#define nir_barrier(build, ...) \
_nir_build_barrier(build, (struct _nir_barrier_indices){0, __VA_ARGS__})
#endif
#define nir_begin_invocation_interlock _nir_build_begin_invocation_interlock
#define nir_bindgen_return _nir_build_bindgen_return
#ifdef __cplusplus
#define nir_bindless_image_agx(build, src0, ...) \
_nir_build_bindless_image_agx(build, src0, _nir_bindless_image_agx_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_agx(build, src0, ...) \
_nir_build_bindless_image_agx(build, src0, (struct _nir_bindless_image_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_atomic(build, bit_size, src0, src1, src2, src3, _nir_bindless_image_atomic_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_atomic(build, bit_size, src0, src1, src2, src3, (struct _nir_bindless_image_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_bindless_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, _nir_bindless_image_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_bindless_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_bindless_image_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_bindless_image_descriptor_amd(build, num_components, bit_size, src0, _nir_bindless_image_descriptor_amd_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_bindless_image_descriptor_amd(build, num_components, bit_size, src0, (struct _nir_bindless_image_descriptor_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_fence_kk(build, src0, ...) \
_nir_build_bindless_image_fence_kk(build, src0, _nir_bindless_image_fence_kk_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_fence_kk(build, src0, ...) \
_nir_build_bindless_image_fence_kk(build, src0, (struct _nir_bindless_image_fence_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_format(build, bit_size, src0, ...) \
_nir_build_bindless_image_format(build, bit_size, src0, _nir_bindless_image_format_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_format(build, bit_size, src0, ...) \
_nir_build_bindless_image_format(build, bit_size, src0, (struct _nir_bindless_image_format_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_bindless_image_fragment_mask_load_amd(build, src0, src1, _nir_bindless_image_fragment_mask_load_amd_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_bindless_image_fragment_mask_load_amd(build, src0, src1, (struct _nir_bindless_image_fragment_mask_load_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_levels(build, bit_size, src0, ...) \
_nir_build_bindless_image_levels(build, bit_size, src0, _nir_bindless_image_levels_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_levels(build, bit_size, src0, ...) \
_nir_build_bindless_image_levels(build, bit_size, src0, (struct _nir_bindless_image_levels_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_bindless_image_load_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_bindless_image_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_bindless_image_load_raw_intel(build, num_components, bit_size, src0, src1, _nir_bindless_image_load_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_bindless_image_load_raw_intel(build, num_components, bit_size, src0, src1, (struct _nir_bindless_image_load_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_order(build, bit_size, src0, ...) \
_nir_build_bindless_image_order(build, bit_size, src0, _nir_bindless_image_order_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_order(build, bit_size, src0, ...) \
_nir_build_bindless_image_order(build, bit_size, src0, (struct _nir_bindless_image_order_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_samples(build, bit_size, src0, ...) \
_nir_build_bindless_image_samples(build, bit_size, src0, _nir_bindless_image_samples_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_samples(build, bit_size, src0, ...) \
_nir_build_bindless_image_samples(build, bit_size, src0, (struct _nir_bindless_image_samples_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_bindless_image_samples_identical(build, bit_size, src0, src1, _nir_bindless_image_samples_identical_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_bindless_image_samples_identical(build, bit_size, src0, src1, (struct _nir_bindless_image_samples_identical_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_bindless_image_size(build, num_components, bit_size, src0, src1, _nir_bindless_image_size_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_bindless_image_size(build, num_components, bit_size, src0, src1, (struct _nir_bindless_image_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_bindless_image_sparse_load_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_bindless_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_bindless_image_sparse_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_bindless_image_store(build, src0, src1, src2, src3, src4, _nir_bindless_image_store_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_bindless_image_store(build, src0, src1, src2, src3, src4, (struct _nir_bindless_image_store_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_bindless_image_store_block_agx(build, src0, src1, src2, _nir_bindless_image_store_block_agx_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_bindless_image_store_block_agx(build, src0, src1, src2, (struct _nir_bindless_image_store_block_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_bindless_image_store_raw_intel(build, src0, src1, src2, _nir_bindless_image_store_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_bindless_image_store_raw_intel(build, src0, src1, src2, (struct _nir_bindless_image_store_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_image_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_bindless_image_texel_address(build, bit_size, src0, src1, src2, _nir_bindless_image_texel_address_indices{0, __VA_ARGS__})
#else
#define nir_bindless_image_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_bindless_image_texel_address(build, bit_size, src0, src1, src2, (struct _nir_bindless_image_texel_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_resource_ir3(build, bit_size, src0, ...) \
_nir_build_bindless_resource_ir3(build, bit_size, src0, _nir_bindless_resource_ir3_indices{0, __VA_ARGS__})
#else
#define nir_bindless_resource_ir3(build, bit_size, src0, ...) \
_nir_build_bindless_resource_ir3(build, bit_size, src0, (struct _nir_bindless_resource_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_bindless_sampler_agx(build, src0, src1, ...) \
_nir_build_bindless_sampler_agx(build, src0, src1, _nir_bindless_sampler_agx_indices{0, __VA_ARGS__})
#else
#define nir_bindless_sampler_agx(build, src0, src1, ...) \
_nir_build_bindless_sampler_agx(build, src0, src1, (struct _nir_bindless_sampler_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_blend2_pan(build, src0, src1, src2, src3, ...) \
_nir_build_blend2_pan(build, src0, src1, src2, src3, _nir_blend2_pan_indices{0, __VA_ARGS__})
#else
#define nir_blend2_pan(build, src0, src1, src2, src3, ...) \
_nir_build_blend2_pan(build, src0, src1, src2, src3, (struct _nir_blend2_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_blend_pan(build, src0, src1, src2, ...) \
_nir_build_blend_pan(build, src0, src1, src2, _nir_blend_pan_indices{0, __VA_ARGS__})
#else
#define nir_blend_pan(build, src0, src1, src2, ...) \
_nir_build_blend_pan(build, src0, src1, src2, (struct _nir_blend_pan_indices){0, __VA_ARGS__})
#endif
#define nir_blend_return_pan _nir_build_blend_return_pan
#ifdef __cplusplus
#define nir_brcst_active_ir3(build, src0, src1, ...) \
_nir_build_brcst_active_ir3(build, src0, src1, _nir_brcst_active_ir3_indices{0, __VA_ARGS__})
#else
#define nir_brcst_active_ir3(build, src0, src1, ...) \
_nir_build_brcst_active_ir3(build, src0, src1, (struct _nir_brcst_active_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_btd_retire_intel _nir_build_btd_retire_intel
#define nir_btd_spawn_intel _nir_build_btd_spawn_intel
#ifdef __cplusplus
#define nir_btd_stack_push_intel(build, ...) \
_nir_build_btd_stack_push_intel(build, _nir_btd_stack_push_intel_indices{0, __VA_ARGS__})
#else
#define nir_btd_stack_push_intel(build, ...) \
_nir_build_btd_stack_push_intel(build, (struct _nir_btd_stack_push_intel_indices){0, __VA_ARGS__})
#endif
#define nir_bvh64_intersect_ray_amd _nir_build_bvh64_intersect_ray_amd
#define nir_bvh8_intersect_ray_amd _nir_build_bvh8_intersect_ray_amd
#ifdef __cplusplus
#define nir_bvh_stack_rtn_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_bvh_stack_rtn_amd(build, bit_size, src0, src1, src2, _nir_bvh_stack_rtn_amd_indices{0, __VA_ARGS__})
#else
#define nir_bvh_stack_rtn_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_bvh_stack_rtn_amd(build, bit_size, src0, src1, src2, (struct _nir_bvh_stack_rtn_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_binary_op(build, src0, src1, src2, ...) \
_nir_build_cmat_binary_op(build, src0, src1, src2, _nir_cmat_binary_op_indices{0, __VA_ARGS__})
#else
#define nir_cmat_binary_op(build, src0, src1, src2, ...) \
_nir_build_cmat_binary_op(build, src0, src1, src2, (struct _nir_cmat_binary_op_indices){0, __VA_ARGS__})
#endif
#define nir_cmat_bitcast _nir_build_cmat_bitcast
#define nir_cmat_construct _nir_build_cmat_construct
#ifdef __cplusplus
#define nir_cmat_convert(build, src0, src1, ...) \
_nir_build_cmat_convert(build, src0, src1, _nir_cmat_convert_indices{0, __VA_ARGS__})
#else
#define nir_cmat_convert(build, src0, src1, ...) \
_nir_build_cmat_convert(build, src0, src1, (struct _nir_cmat_convert_indices){0, __VA_ARGS__})
#endif
#define nir_cmat_copy _nir_build_cmat_copy
#define nir_cmat_extract _nir_build_cmat_extract
#define nir_cmat_insert _nir_build_cmat_insert
#ifdef __cplusplus
#define nir_cmat_length(build, ...) \
_nir_build_cmat_length(build, _nir_cmat_length_indices{0, __VA_ARGS__})
#else
#define nir_cmat_length(build, ...) \
_nir_build_cmat_length(build, (struct _nir_cmat_length_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_load(build, src0, src1, src2, ...) \
_nir_build_cmat_load(build, src0, src1, src2, _nir_cmat_load_indices{0, __VA_ARGS__})
#else
#define nir_cmat_load(build, src0, src1, src2, ...) \
_nir_build_cmat_load(build, src0, src1, src2, (struct _nir_cmat_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_load_shared_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_cmat_load_shared_nv(build, num_components, bit_size, src0, src1, _nir_cmat_load_shared_nv_indices{0, __VA_ARGS__})
#else
#define nir_cmat_load_shared_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_cmat_load_shared_nv(build, num_components, bit_size, src0, src1, (struct _nir_cmat_load_shared_nv_indices){0, __VA_ARGS__})
#endif
#define nir_cmat_mov_transpose_nv _nir_build_cmat_mov_transpose_nv
#ifdef __cplusplus
#define nir_cmat_muladd(build, src0, src1, src2, src3, ...) \
_nir_build_cmat_muladd(build, src0, src1, src2, src3, _nir_cmat_muladd_indices{0, __VA_ARGS__})
#else
#define nir_cmat_muladd(build, src0, src1, src2, src3, ...) \
_nir_build_cmat_muladd(build, src0, src1, src2, src3, (struct _nir_cmat_muladd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_muladd_amd(build, src0, src1, src2, ...) \
_nir_build_cmat_muladd_amd(build, src0, src1, src2, _nir_cmat_muladd_amd_indices{0, __VA_ARGS__})
#else
#define nir_cmat_muladd_amd(build, src0, src1, src2, ...) \
_nir_build_cmat_muladd_amd(build, src0, src1, src2, (struct _nir_cmat_muladd_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_muladd_nv(build, num_components, src0, src1, src2, ...) \
_nir_build_cmat_muladd_nv(build, num_components, src0, src1, src2, _nir_cmat_muladd_nv_indices{0, __VA_ARGS__})
#else
#define nir_cmat_muladd_nv(build, num_components, src0, src1, src2, ...) \
_nir_build_cmat_muladd_nv(build, num_components, src0, src1, src2, (struct _nir_cmat_muladd_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_scalar_op(build, src0, src1, src2, ...) \
_nir_build_cmat_scalar_op(build, src0, src1, src2, _nir_cmat_scalar_op_indices{0, __VA_ARGS__})
#else
#define nir_cmat_scalar_op(build, src0, src1, src2, ...) \
_nir_build_cmat_scalar_op(build, src0, src1, src2, (struct _nir_cmat_scalar_op_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_store(build, src0, src1, src2, ...) \
_nir_build_cmat_store(build, src0, src1, src2, _nir_cmat_store_indices{0, __VA_ARGS__})
#else
#define nir_cmat_store(build, src0, src1, src2, ...) \
_nir_build_cmat_store(build, src0, src1, src2, (struct _nir_cmat_store_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_transpose(build, src0, src1, ...) \
_nir_build_cmat_transpose(build, src0, src1, _nir_cmat_transpose_indices{0, __VA_ARGS__})
#else
#define nir_cmat_transpose(build, src0, src1, ...) \
_nir_build_cmat_transpose(build, src0, src1, (struct _nir_cmat_transpose_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_cmat_unary_op(build, src0, src1, ...) \
_nir_build_cmat_unary_op(build, src0, src1, _nir_cmat_unary_op_indices{0, __VA_ARGS__})
#else
#define nir_cmat_unary_op(build, src0, src1, ...) \
_nir_build_cmat_unary_op(build, src0, src1, (struct _nir_cmat_unary_op_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_convert_alu_types(build, bit_size, src0, ...) \
_nir_build_convert_alu_types(build, bit_size, src0, _nir_convert_alu_types_indices{0, __VA_ARGS__})
#else
#define nir_convert_alu_types(build, bit_size, src0, ...) \
_nir_build_convert_alu_types(build, bit_size, src0, (struct _nir_convert_alu_types_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_convert_cmat_intel(build, num_components, bit_size, src0, ...) \
_nir_build_convert_cmat_intel(build, num_components, bit_size, src0, _nir_convert_cmat_intel_indices{0, __VA_ARGS__})
#else
#define nir_convert_cmat_intel(build, num_components, bit_size, src0, ...) \
_nir_build_convert_cmat_intel(build, num_components, bit_size, src0, (struct _nir_convert_cmat_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_copy_deref(build, src0, src1, ...) \
_nir_build_copy_deref(build, src0, src1, _nir_copy_deref_indices{0, __VA_ARGS__})
#else
#define nir_build_copy_deref(build, src0, src1, ...) \
_nir_build_copy_deref(build, src0, src1, (struct _nir_copy_deref_indices){0, __VA_ARGS__})
#endif
#define nir_copy_deref nir_build_copy_deref
#define nir_copy_fs_outputs_nv _nir_build_copy_fs_outputs_nv
#ifdef __cplusplus
#define nir_copy_global_to_uniform_ir3(build, src0, ...) \
_nir_build_copy_global_to_uniform_ir3(build, src0, _nir_copy_global_to_uniform_ir3_indices{0, __VA_ARGS__})
#else
#define nir_copy_global_to_uniform_ir3(build, src0, ...) \
_nir_build_copy_global_to_uniform_ir3(build, src0, (struct _nir_copy_global_to_uniform_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_copy_push_const_to_uniform_ir3(build, src0, ...) \
_nir_build_copy_push_const_to_uniform_ir3(build, src0, _nir_copy_push_const_to_uniform_ir3_indices{0, __VA_ARGS__})
#else
#define nir_copy_push_const_to_uniform_ir3(build, src0, ...) \
_nir_build_copy_push_const_to_uniform_ir3(build, src0, (struct _nir_copy_push_const_to_uniform_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_copy_ubo_to_uniform_ir3(build, src0, src1, ...) \
_nir_build_copy_ubo_to_uniform_ir3(build, src0, src1, _nir_copy_ubo_to_uniform_ir3_indices{0, __VA_ARGS__})
#else
#define nir_copy_ubo_to_uniform_ir3(build, src0, src1, ...) \
_nir_build_copy_ubo_to_uniform_ir3(build, src0, src1, (struct _nir_copy_ubo_to_uniform_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_cube_ssel_pan _nir_build_cube_ssel_pan
#define nir_cube_tsel_pan _nir_build_cube_tsel_pan
#define nir_cubeface_pan _nir_build_cubeface_pan
#ifdef __cplusplus
#define nir_ddx(build, bit_size, src0, ...) \
_nir_build_ddx(build, bit_size, src0, _nir_ddx_indices{0, __VA_ARGS__})
#else
#define nir_ddx(build, bit_size, src0, ...) \
_nir_build_ddx(build, bit_size, src0, (struct _nir_ddx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ddx_coarse(build, bit_size, src0, ...) \
_nir_build_ddx_coarse(build, bit_size, src0, _nir_ddx_coarse_indices{0, __VA_ARGS__})
#else
#define nir_ddx_coarse(build, bit_size, src0, ...) \
_nir_build_ddx_coarse(build, bit_size, src0, (struct _nir_ddx_coarse_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ddx_fine(build, bit_size, src0, ...) \
_nir_build_ddx_fine(build, bit_size, src0, _nir_ddx_fine_indices{0, __VA_ARGS__})
#else
#define nir_ddx_fine(build, bit_size, src0, ...) \
_nir_build_ddx_fine(build, bit_size, src0, (struct _nir_ddx_fine_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ddy(build, bit_size, src0, ...) \
_nir_build_ddy(build, bit_size, src0, _nir_ddy_indices{0, __VA_ARGS__})
#else
#define nir_ddy(build, bit_size, src0, ...) \
_nir_build_ddy(build, bit_size, src0, (struct _nir_ddy_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ddy_coarse(build, bit_size, src0, ...) \
_nir_build_ddy_coarse(build, bit_size, src0, _nir_ddy_coarse_indices{0, __VA_ARGS__})
#else
#define nir_ddy_coarse(build, bit_size, src0, ...) \
_nir_build_ddy_coarse(build, bit_size, src0, (struct _nir_ddy_coarse_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ddy_fine(build, bit_size, src0, ...) \
_nir_build_ddy_fine(build, bit_size, src0, _nir_ddy_fine_indices{0, __VA_ARGS__})
#else
#define nir_ddy_fine(build, bit_size, src0, ...) \
_nir_build_ddy_fine(build, bit_size, src0, (struct _nir_ddy_fine_indices){0, __VA_ARGS__})
#endif
#define nir_debug_break _nir_build_debug_break
#ifdef __cplusplus
#define nir_decl_reg(build, bit_size, ...) \
_nir_build_decl_reg(build, bit_size, _nir_decl_reg_indices{0, __VA_ARGS__})
#else
#define nir_decl_reg(build, bit_size, ...) \
_nir_build_decl_reg(build, bit_size, (struct _nir_decl_reg_indices){0, __VA_ARGS__})
#endif
#define nir_demote _nir_build_demote
#define nir_demote_if _nir_build_demote_if
#define nir_demote_samples _nir_build_demote_samples
#ifdef __cplusplus
#define nir_deref_atomic(build, bit_size, src0, src1, ...) \
_nir_build_deref_atomic(build, bit_size, src0, src1, _nir_deref_atomic_indices{0, __VA_ARGS__})
#else
#define nir_deref_atomic(build, bit_size, src0, src1, ...) \
_nir_build_deref_atomic(build, bit_size, src0, src1, (struct _nir_deref_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_deref_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_deref_atomic_swap(build, bit_size, src0, src1, src2, _nir_deref_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_deref_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_deref_atomic_swap(build, bit_size, src0, src1, src2, (struct _nir_deref_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_deref_buffer_address(build, num_components, bit_size, src0, ...) \
_nir_build_deref_buffer_address(build, num_components, bit_size, src0, _nir_deref_buffer_address_indices{0, __VA_ARGS__})
#else
#define nir_deref_buffer_address(build, num_components, bit_size, src0, ...) \
_nir_build_deref_buffer_address(build, num_components, bit_size, src0, (struct _nir_deref_buffer_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_deref_buffer_array_length(build, bit_size, src0, ...) \
_nir_build_deref_buffer_array_length(build, bit_size, src0, _nir_deref_buffer_array_length_indices{0, __VA_ARGS__})
#else
#define nir_deref_buffer_array_length(build, bit_size, src0, ...) \
_nir_build_deref_buffer_array_length(build, bit_size, src0, (struct _nir_deref_buffer_array_length_indices){0, __VA_ARGS__})
#endif
#define nir_deref_implicit_array_length _nir_build_deref_implicit_array_length
#ifdef __cplusplus
#define nir_build_deref_mode_is(build, bit_size, src0, ...) \
_nir_build_deref_mode_is(build, bit_size, src0, _nir_deref_mode_is_indices{0, __VA_ARGS__})
#else
#define nir_build_deref_mode_is(build, bit_size, src0, ...) \
_nir_build_deref_mode_is(build, bit_size, src0, (struct _nir_deref_mode_is_indices){0, __VA_ARGS__})
#endif
#define nir_deref_mode_is nir_build_deref_mode_is
#define nir_deref_texture_src _nir_build_deref_texture_src
#define nir_dma_flush_pco _nir_build_dma_flush_pco
#define nir_dma_ld_pco _nir_build_dma_ld_pco
#define nir_dma_ld_shregs_pco _nir_build_dma_ld_shregs_pco
#ifdef __cplusplus
#define nir_dma_st_pco(build, src0, ...) \
_nir_build_dma_st_pco(build, src0, _nir_dma_st_pco_indices{0, __VA_ARGS__})
#else
#define nir_dma_st_pco(build, src0, ...) \
_nir_build_dma_st_pco(build, src0, (struct _nir_dma_st_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_dma_st_shregs_pco(build, src0, src1, src2, ...) \
_nir_build_dma_st_shregs_pco(build, src0, src1, src2, _nir_dma_st_shregs_pco_indices{0, __VA_ARGS__})
#else
#define nir_dma_st_shregs_pco(build, src0, src1, src2, ...) \
_nir_build_dma_st_shregs_pco(build, src0, src1, src2, (struct _nir_dma_st_shregs_pco_indices){0, __VA_ARGS__})
#endif
#define nir_dma_st_tiled_pco _nir_build_dma_st_tiled_pco
#define nir_doorbell_agx _nir_build_doorbell_agx
#ifdef __cplusplus
#define nir_dpas_intel(build, bit_size, src0, src1, src2, ...) \
_nir_build_dpas_intel(build, bit_size, src0, src1, src2, _nir_dpas_intel_indices{0, __VA_ARGS__})
#else
#define nir_dpas_intel(build, bit_size, src0, src1, src2, ...) \
_nir_build_dpas_intel(build, bit_size, src0, src1, src2, (struct _nir_dpas_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_dpp16_shift_amd(build, src0, ...) \
_nir_build_dpp16_shift_amd(build, src0, _nir_dpp16_shift_amd_indices{0, __VA_ARGS__})
#else
#define nir_dpp16_shift_amd(build, src0, ...) \
_nir_build_dpp16_shift_amd(build, src0, (struct _nir_dpp16_shift_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_dummy_load_store_pco(build, ...) \
_nir_build_dummy_load_store_pco(build, _nir_dummy_load_store_pco_indices{0, __VA_ARGS__})
#else
#define nir_dummy_load_store_pco(build, ...) \
_nir_build_dummy_load_store_pco(build, (struct _nir_dummy_load_store_pco_indices){0, __VA_ARGS__})
#endif
#define nir_elect _nir_build_elect
#define nir_elect_any_ir3 _nir_build_elect_any_ir3
#ifdef __cplusplus
#define nir_emit_primitive_poly(build, src0, src1, src2, src3, ...) \
_nir_build_emit_primitive_poly(build, src0, src1, src2, src3, _nir_emit_primitive_poly_indices{0, __VA_ARGS__})
#else
#define nir_emit_primitive_poly(build, src0, src1, src2, src3, ...) \
_nir_build_emit_primitive_poly(build, src0, src1, src2, src3, (struct _nir_emit_primitive_poly_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_emit_vertex(build, ...) \
_nir_build_emit_vertex(build, _nir_emit_vertex_indices{0, __VA_ARGS__})
#else
#define nir_emit_vertex(build, ...) \
_nir_build_emit_vertex(build, (struct _nir_emit_vertex_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_emit_vertex_nv(build, bit_size, src0, ...) \
_nir_build_emit_vertex_nv(build, bit_size, src0, _nir_emit_vertex_nv_indices{0, __VA_ARGS__})
#else
#define nir_emit_vertex_nv(build, bit_size, src0, ...) \
_nir_build_emit_vertex_nv(build, bit_size, src0, (struct _nir_emit_vertex_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_emit_vertex_with_counter(build, src0, src1, ...) \
_nir_build_emit_vertex_with_counter(build, src0, src1, _nir_emit_vertex_with_counter_indices{0, __VA_ARGS__})
#else
#define nir_emit_vertex_with_counter(build, src0, src1, ...) \
_nir_build_emit_vertex_with_counter(build, src0, src1, (struct _nir_emit_vertex_with_counter_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_emitpix_pco(build, src0, src1, ...) \
_nir_build_emitpix_pco(build, src0, src1, _nir_emitpix_pco_indices{0, __VA_ARGS__})
#else
#define nir_emitpix_pco(build, src0, src1, ...) \
_nir_build_emitpix_pco(build, src0, src1, (struct _nir_emitpix_pco_indices){0, __VA_ARGS__})
#endif
#define nir_end_invocation_interlock _nir_build_end_invocation_interlock
#ifdef __cplusplus
#define nir_end_primitive(build, ...) \
_nir_build_end_primitive(build, _nir_end_primitive_indices{0, __VA_ARGS__})
#else
#define nir_end_primitive(build, ...) \
_nir_build_end_primitive(build, (struct _nir_end_primitive_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_end_primitive_nv(build, bit_size, src0, ...) \
_nir_build_end_primitive_nv(build, bit_size, src0, _nir_end_primitive_nv_indices{0, __VA_ARGS__})
#else
#define nir_end_primitive_nv(build, bit_size, src0, ...) \
_nir_build_end_primitive_nv(build, bit_size, src0, (struct _nir_end_primitive_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_end_primitive_with_counter(build, src0, src1, ...) \
_nir_build_end_primitive_with_counter(build, src0, src1, _nir_end_primitive_with_counter_indices{0, __VA_ARGS__})
#else
#define nir_end_primitive_with_counter(build, src0, src1, ...) \
_nir_build_end_primitive_with_counter(build, src0, src1, (struct _nir_end_primitive_with_counter_indices){0, __VA_ARGS__})
#endif
#define nir_enqueue_node_payloads _nir_build_enqueue_node_payloads
#ifdef __cplusplus
#define nir_exclusive_scan(build, src0, ...) \
_nir_build_exclusive_scan(build, src0, _nir_exclusive_scan_indices{0, __VA_ARGS__})
#else
#define nir_exclusive_scan(build, src0, ...) \
_nir_build_exclusive_scan(build, src0, (struct _nir_exclusive_scan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_exclusive_scan_clusters_ir3(build, src0, src1, ...) \
_nir_build_exclusive_scan_clusters_ir3(build, src0, src1, _nir_exclusive_scan_clusters_ir3_indices{0, __VA_ARGS__})
#else
#define nir_exclusive_scan_clusters_ir3(build, src0, src1, ...) \
_nir_build_exclusive_scan_clusters_ir3(build, src0, src1, (struct _nir_exclusive_scan_clusters_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_execute_callable _nir_build_execute_callable
#define nir_execute_closest_hit_amd _nir_build_execute_closest_hit_amd
#define nir_execute_miss_amd _nir_build_execute_miss_amd
#ifdef __cplusplus
#define nir_export_agx(build, src0, ...) \
_nir_build_export_agx(build, src0, _nir_export_agx_indices{0, __VA_ARGS__})
#else
#define nir_export_agx(build, src0, ...) \
_nir_build_export_agx(build, src0, (struct _nir_export_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_export_amd(build, src0, ...) \
_nir_build_export_amd(build, src0, _nir_export_amd_indices{0, __VA_ARGS__})
#else
#define nir_export_amd(build, src0, ...) \
_nir_build_export_amd(build, src0, (struct _nir_export_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_export_dual_src_blend_amd(build, src0, src1, ...) \
_nir_build_export_dual_src_blend_amd(build, src0, src1, _nir_export_dual_src_blend_amd_indices{0, __VA_ARGS__})
#else
#define nir_export_dual_src_blend_amd(build, src0, src1, ...) \
_nir_build_export_dual_src_blend_amd(build, src0, src1, (struct _nir_export_dual_src_blend_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_export_row_amd(build, src0, src1, ...) \
_nir_build_export_row_amd(build, src0, src1, _nir_export_row_amd_indices{0, __VA_ARGS__})
#else
#define nir_export_row_amd(build, src0, src1, ...) \
_nir_build_export_row_amd(build, src0, src1, (struct _nir_export_row_amd_indices){0, __VA_ARGS__})
#endif
#define nir_fence_helper_exit_agx _nir_build_fence_helper_exit_agx
#define nir_fence_mem_to_tex_agx _nir_build_fence_mem_to_tex_agx
#define nir_fence_pbe_to_tex_agx _nir_build_fence_pbe_to_tex_agx
#define nir_fence_pbe_to_tex_pixel_agx _nir_build_fence_pbe_to_tex_pixel_agx
#define nir_final_primitive_nv _nir_build_final_primitive_nv
#define nir_finalize_incoming_node_payload _nir_build_finalize_incoming_node_payload
#define nir_first_invocation _nir_build_first_invocation
#ifdef __cplusplus
#define nir_flush_tile_buffer_pco(build, src0, src1, ...) \
_nir_build_flush_tile_buffer_pco(build, src0, src1, _nir_flush_tile_buffer_pco_indices{0, __VA_ARGS__})
#else
#define nir_flush_tile_buffer_pco(build, src0, src1, ...) \
_nir_build_flush_tile_buffer_pco(build, src0, src1, (struct _nir_flush_tile_buffer_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_frag_store_pco(build, src0, ...) \
_nir_build_frag_store_pco(build, src0, _nir_frag_store_pco_indices{0, __VA_ARGS__})
#else
#define nir_frag_store_pco(build, src0, ...) \
_nir_build_frag_store_pco(build, src0, (struct _nir_frag_store_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_fs_out_nv(build, src0, ...) \
_nir_build_fs_out_nv(build, src0, _nir_fs_out_nv_indices{0, __VA_ARGS__})
#else
#define nir_fs_out_nv(build, src0, ...) \
_nir_build_fs_out_nv(build, src0, (struct _nir_fs_out_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_gds_atomic_add_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_gds_atomic_add_amd(build, bit_size, src0, src1, src2, _nir_gds_atomic_add_amd_indices{0, __VA_ARGS__})
#else
#define nir_gds_atomic_add_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_gds_atomic_add_amd(build, bit_size, src0, src1, src2, (struct _nir_gds_atomic_add_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_get_ssbo_size(build, bit_size, src0, ...) \
_nir_build_get_ssbo_size(build, bit_size, src0, _nir_get_ssbo_size_indices{0, __VA_ARGS__})
#else
#define nir_get_ssbo_size(build, bit_size, src0, ...) \
_nir_build_get_ssbo_size(build, bit_size, src0, (struct _nir_get_ssbo_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_get_ubo_size(build, bit_size, src0, ...) \
_nir_build_get_ubo_size(build, bit_size, src0, _nir_get_ubo_size_indices{0, __VA_ARGS__})
#else
#define nir_get_ubo_size(build, bit_size, src0, ...) \
_nir_build_get_ubo_size(build, bit_size, src0, (struct _nir_get_ubo_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic(build, bit_size, src0, src1, ...) \
_nir_build_global_atomic(build, bit_size, src0, src1, _nir_global_atomic_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic(build, bit_size, src0, src1, ...) \
_nir_build_global_atomic(build, bit_size, src0, src1, (struct _nir_global_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_2x32(build, bit_size, src0, src1, ...) \
_nir_build_global_atomic_2x32(build, bit_size, src0, src1, _nir_global_atomic_2x32_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_2x32(build, bit_size, src0, src1, ...) \
_nir_build_global_atomic_2x32(build, bit_size, src0, src1, (struct _nir_global_atomic_2x32_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_agx(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_agx(build, bit_size, src0, src1, src2, _nir_global_atomic_agx_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_agx(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_agx(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_amd(build, bit_size, src0, src1, src2, _nir_global_atomic_amd_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_amd(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_amd(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_nv(build, bit_size, src0, src1, src2, _nir_global_atomic_nv_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_nv(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_pco(build, num_components, src0, ...) \
_nir_build_global_atomic_pco(build, num_components, src0, _nir_global_atomic_pco_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_pco(build, num_components, src0, ...) \
_nir_build_global_atomic_pco(build, num_components, src0, (struct _nir_global_atomic_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap(build, bit_size, src0, src1, src2, _nir_global_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap_2x32(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap_2x32(build, bit_size, src0, src1, src2, _nir_global_atomic_swap_2x32_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap_2x32(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap_2x32(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_swap_2x32_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap_agx(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_global_atomic_swap_agx(build, bit_size, src0, src1, src2, src3, _nir_global_atomic_swap_agx_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap_agx(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_global_atomic_swap_agx(build, bit_size, src0, src1, src2, src3, (struct _nir_global_atomic_swap_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap_amd(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_global_atomic_swap_amd(build, bit_size, src0, src1, src2, src3, _nir_global_atomic_swap_amd_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap_amd(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_global_atomic_swap_amd(build, bit_size, src0, src1, src2, src3, (struct _nir_global_atomic_swap_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap_nv(build, bit_size, src0, src1, src2, _nir_global_atomic_swap_nv_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_global_atomic_swap_nv(build, bit_size, src0, src1, src2, (struct _nir_global_atomic_swap_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_global_atomic_swap_pco(build, num_components, src0, ...) \
_nir_build_global_atomic_swap_pco(build, num_components, src0, _nir_global_atomic_swap_pco_indices{0, __VA_ARGS__})
#else
#define nir_global_atomic_swap_pco(build, num_components, src0, ...) \
_nir_build_global_atomic_swap_pco(build, num_components, src0, (struct _nir_global_atomic_swap_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_glslopt_precision(build, src0, ...) \
_nir_build_glslopt_precision(build, src0, _nir_glslopt_precision_indices{0, __VA_ARGS__})
#else
#define nir_glslopt_precision(build, src0, ...) \
_nir_build_glslopt_precision(build, src0, (struct _nir_glslopt_precision_indices){0, __VA_ARGS__})
#endif
#define nir_ignore_ray_intersection _nir_build_ignore_ray_intersection
#ifdef __cplusplus
#define nir_imadsp_nv(build, src0, src1, src2, ...) \
_nir_build_imadsp_nv(build, src0, src1, src2, _nir_imadsp_nv_indices{0, __VA_ARGS__})
#else
#define nir_imadsp_nv(build, src0, src1, src2, ...) \
_nir_build_imadsp_nv(build, src0, src1, src2, (struct _nir_imadsp_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_atomic(build, bit_size, src0, src1, src2, src3, _nir_image_atomic_indices{0, __VA_ARGS__})
#else
#define nir_image_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_atomic(build, bit_size, src0, src1, src2, src3, (struct _nir_image_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, _nir_image_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_image_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_atomic(build, bit_size, src0, src1, src2, src3, _nir_image_deref_atomic_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_atomic(build, bit_size, src0, src1, src2, src3, (struct _nir_image_deref_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_deref_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, _nir_image_deref_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_deref_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_image_deref_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_deref_descriptor_amd(build, num_components, bit_size, src0, _nir_image_deref_descriptor_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_deref_descriptor_amd(build, num_components, bit_size, src0, (struct _nir_image_deref_descriptor_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_fence_kk(build, src0, ...) \
_nir_build_image_deref_fence_kk(build, src0, _nir_image_deref_fence_kk_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_fence_kk(build, src0, ...) \
_nir_build_image_deref_fence_kk(build, src0, (struct _nir_image_deref_fence_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_format(build, bit_size, src0, ...) \
_nir_build_image_deref_format(build, bit_size, src0, _nir_image_deref_format_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_format(build, bit_size, src0, ...) \
_nir_build_image_deref_format(build, bit_size, src0, (struct _nir_image_deref_format_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_deref_fragment_mask_load_amd(build, src0, src1, _nir_image_deref_fragment_mask_load_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_deref_fragment_mask_load_amd(build, src0, src1, (struct _nir_image_deref_fragment_mask_load_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_levels(build, bit_size, src0, ...) \
_nir_build_image_deref_levels(build, bit_size, src0, _nir_image_deref_levels_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_levels(build, bit_size, src0, ...) \
_nir_build_image_deref_levels(build, bit_size, src0, (struct _nir_image_deref_levels_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_deref_load_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_deref_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_load_info_nv(build, num_components, src0, ...) \
_nir_build_image_deref_load_info_nv(build, num_components, src0, _nir_image_deref_load_info_nv_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_load_info_nv(build, num_components, src0, ...) \
_nir_build_image_deref_load_info_nv(build, num_components, src0, (struct _nir_image_deref_load_info_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_load_param_intel(build, num_components, bit_size, src0, ...) \
_nir_build_image_deref_load_param_intel(build, num_components, bit_size, src0, _nir_image_deref_load_param_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_load_param_intel(build, num_components, bit_size, src0, ...) \
_nir_build_image_deref_load_param_intel(build, num_components, bit_size, src0, (struct _nir_image_deref_load_param_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_deref_load_raw_intel(build, num_components, bit_size, src0, src1, _nir_image_deref_load_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_deref_load_raw_intel(build, num_components, bit_size, src0, src1, (struct _nir_image_deref_load_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_order(build, bit_size, src0, ...) \
_nir_build_image_deref_order(build, bit_size, src0, _nir_image_deref_order_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_order(build, bit_size, src0, ...) \
_nir_build_image_deref_order(build, bit_size, src0, (struct _nir_image_deref_order_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_samples(build, bit_size, src0, ...) \
_nir_build_image_deref_samples(build, bit_size, src0, _nir_image_deref_samples_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_samples(build, bit_size, src0, ...) \
_nir_build_image_deref_samples(build, bit_size, src0, (struct _nir_image_deref_samples_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_deref_samples_identical(build, bit_size, src0, src1, _nir_image_deref_samples_identical_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_deref_samples_identical(build, bit_size, src0, src1, (struct _nir_image_deref_samples_identical_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_deref_size(build, num_components, bit_size, src0, src1, _nir_image_deref_size_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_deref_size(build, num_components, bit_size, src0, src1, (struct _nir_image_deref_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_deref_sparse_load_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_deref_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_deref_sparse_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_deref_store(build, src0, src1, src2, src3, src4, _nir_image_deref_store_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_deref_store(build, src0, src1, src2, src3, src4, (struct _nir_image_deref_store_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_deref_store_block_agx(build, src0, src1, src2, _nir_image_deref_store_block_agx_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_deref_store_block_agx(build, src0, src1, src2, (struct _nir_image_deref_store_block_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_deref_store_raw_intel(build, src0, src1, src2, _nir_image_deref_store_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_deref_store_raw_intel(build, src0, src1, src2, (struct _nir_image_deref_store_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_deref_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_deref_texel_address(build, bit_size, src0, src1, src2, _nir_image_deref_texel_address_indices{0, __VA_ARGS__})
#else
#define nir_image_deref_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_deref_texel_address(build, bit_size, src0, src1, src2, (struct _nir_image_deref_texel_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_descriptor_amd(build, num_components, bit_size, src0, _nir_image_descriptor_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_descriptor_amd(build, num_components, bit_size, src0, (struct _nir_image_descriptor_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_fence_kk(build, src0, ...) \
_nir_build_image_fence_kk(build, src0, _nir_image_fence_kk_indices{0, __VA_ARGS__})
#else
#define nir_image_fence_kk(build, src0, ...) \
_nir_build_image_fence_kk(build, src0, (struct _nir_image_fence_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_format(build, bit_size, src0, ...) \
_nir_build_image_format(build, bit_size, src0, _nir_image_format_indices{0, __VA_ARGS__})
#else
#define nir_image_format(build, bit_size, src0, ...) \
_nir_build_image_format(build, bit_size, src0, (struct _nir_image_format_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_fragment_mask_load_amd(build, src0, src1, _nir_image_fragment_mask_load_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_fragment_mask_load_amd(build, src0, src1, (struct _nir_image_fragment_mask_load_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_atomic(build, bit_size, src0, src1, src2, src3, _nir_image_heap_atomic_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_atomic(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_atomic(build, bit_size, src0, src1, src2, src3, (struct _nir_image_heap_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_heap_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, _nir_image_heap_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_image_heap_atomic_swap(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_image_heap_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_heap_descriptor_amd(build, num_components, bit_size, src0, _nir_image_heap_descriptor_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_descriptor_amd(build, num_components, bit_size, src0, ...) \
_nir_build_image_heap_descriptor_amd(build, num_components, bit_size, src0, (struct _nir_image_heap_descriptor_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_fence_kk(build, src0, ...) \
_nir_build_image_heap_fence_kk(build, src0, _nir_image_heap_fence_kk_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_fence_kk(build, src0, ...) \
_nir_build_image_heap_fence_kk(build, src0, (struct _nir_image_heap_fence_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_format(build, bit_size, src0, ...) \
_nir_build_image_heap_format(build, bit_size, src0, _nir_image_heap_format_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_format(build, bit_size, src0, ...) \
_nir_build_image_heap_format(build, bit_size, src0, (struct _nir_image_heap_format_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_heap_fragment_mask_load_amd(build, src0, src1, _nir_image_heap_fragment_mask_load_amd_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_fragment_mask_load_amd(build, src0, src1, ...) \
_nir_build_image_heap_fragment_mask_load_amd(build, src0, src1, (struct _nir_image_heap_fragment_mask_load_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_levels(build, bit_size, src0, ...) \
_nir_build_image_heap_levels(build, bit_size, src0, _nir_image_heap_levels_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_levels(build, bit_size, src0, ...) \
_nir_build_image_heap_levels(build, bit_size, src0, (struct _nir_image_heap_levels_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_heap_load_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_heap_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_load_param_intel(build, num_components, bit_size, src0, ...) \
_nir_build_image_heap_load_param_intel(build, num_components, bit_size, src0, _nir_image_heap_load_param_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_load_param_intel(build, num_components, bit_size, src0, ...) \
_nir_build_image_heap_load_param_intel(build, num_components, bit_size, src0, (struct _nir_image_heap_load_param_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_heap_load_raw_intel(build, num_components, bit_size, src0, src1, _nir_image_heap_load_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_heap_load_raw_intel(build, num_components, bit_size, src0, src1, (struct _nir_image_heap_load_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_order(build, bit_size, src0, ...) \
_nir_build_image_heap_order(build, bit_size, src0, _nir_image_heap_order_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_order(build, bit_size, src0, ...) \
_nir_build_image_heap_order(build, bit_size, src0, (struct _nir_image_heap_order_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_samples(build, bit_size, src0, ...) \
_nir_build_image_heap_samples(build, bit_size, src0, _nir_image_heap_samples_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_samples(build, bit_size, src0, ...) \
_nir_build_image_heap_samples(build, bit_size, src0, (struct _nir_image_heap_samples_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_heap_samples_identical(build, bit_size, src0, src1, _nir_image_heap_samples_identical_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_heap_samples_identical(build, bit_size, src0, src1, (struct _nir_image_heap_samples_identical_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_heap_size(build, num_components, bit_size, src0, src1, _nir_image_heap_size_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_heap_size(build, num_components, bit_size, src0, src1, (struct _nir_image_heap_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_heap_sparse_load_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_heap_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_heap_sparse_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_heap_store(build, src0, src1, src2, src3, src4, _nir_image_heap_store_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_heap_store(build, src0, src1, src2, src3, src4, (struct _nir_image_heap_store_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_heap_store_block_agx(build, src0, src1, src2, _nir_image_heap_store_block_agx_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_heap_store_block_agx(build, src0, src1, src2, (struct _nir_image_heap_store_block_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_heap_store_raw_intel(build, src0, src1, src2, _nir_image_heap_store_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_heap_store_raw_intel(build, src0, src1, src2, (struct _nir_image_heap_store_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_heap_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_heap_texel_address(build, bit_size, src0, src1, src2, _nir_image_heap_texel_address_indices{0, __VA_ARGS__})
#else
#define nir_image_heap_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_heap_texel_address(build, bit_size, src0, src1, src2, (struct _nir_image_heap_texel_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_levels(build, bit_size, src0, ...) \
_nir_build_image_levels(build, bit_size, src0, _nir_image_levels_indices{0, __VA_ARGS__})
#else
#define nir_image_levels(build, bit_size, src0, ...) \
_nir_build_image_levels(build, bit_size, src0, (struct _nir_image_levels_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_load_indices{0, __VA_ARGS__})
#else
#define nir_image_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_load_raw_intel(build, num_components, bit_size, src0, src1, _nir_image_load_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_load_raw_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_load_raw_intel(build, num_components, bit_size, src0, src1, (struct _nir_image_load_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_order(build, bit_size, src0, ...) \
_nir_build_image_order(build, bit_size, src0, _nir_image_order_indices{0, __VA_ARGS__})
#else
#define nir_image_order(build, bit_size, src0, ...) \
_nir_build_image_order(build, bit_size, src0, (struct _nir_image_order_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_samples(build, bit_size, src0, ...) \
_nir_build_image_samples(build, bit_size, src0, _nir_image_samples_indices{0, __VA_ARGS__})
#else
#define nir_image_samples(build, bit_size, src0, ...) \
_nir_build_image_samples(build, bit_size, src0, (struct _nir_image_samples_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_samples_identical(build, bit_size, src0, src1, _nir_image_samples_identical_indices{0, __VA_ARGS__})
#else
#define nir_image_samples_identical(build, bit_size, src0, src1, ...) \
_nir_build_image_samples_identical(build, bit_size, src0, src1, (struct _nir_image_samples_identical_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_size(build, num_components, bit_size, src0, src1, _nir_image_size_indices{0, __VA_ARGS__})
#else
#define nir_image_size(build, num_components, bit_size, src0, src1, ...) \
_nir_build_image_size(build, num_components, bit_size, src0, src1, (struct _nir_image_size_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, _nir_image_sparse_load_indices{0, __VA_ARGS__})
#else
#define nir_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_image_sparse_load(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_image_sparse_load_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_store(build, src0, src1, src2, src3, src4, _nir_image_store_indices{0, __VA_ARGS__})
#else
#define nir_image_store(build, src0, src1, src2, src3, src4, ...) \
_nir_build_image_store(build, src0, src1, src2, src3, src4, (struct _nir_image_store_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_store_block_agx(build, src0, src1, src2, _nir_image_store_block_agx_indices{0, __VA_ARGS__})
#else
#define nir_image_store_block_agx(build, src0, src1, src2, ...) \
_nir_build_image_store_block_agx(build, src0, src1, src2, (struct _nir_image_store_block_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_store_raw_intel(build, src0, src1, src2, _nir_image_store_raw_intel_indices{0, __VA_ARGS__})
#else
#define nir_image_store_raw_intel(build, src0, src1, src2, ...) \
_nir_build_image_store_raw_intel(build, src0, src1, src2, (struct _nir_image_store_raw_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_image_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_texel_address(build, bit_size, src0, src1, src2, _nir_image_texel_address_indices{0, __VA_ARGS__})
#else
#define nir_image_texel_address(build, bit_size, src0, src1, src2, ...) \
_nir_build_image_texel_address(build, bit_size, src0, src1, src2, (struct _nir_image_texel_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_inclusive_scan(build, src0, ...) \
_nir_build_inclusive_scan(build, src0, _nir_inclusive_scan_indices{0, __VA_ARGS__})
#else
#define nir_inclusive_scan(build, src0, ...) \
_nir_build_inclusive_scan(build, src0, (struct _nir_inclusive_scan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_inclusive_scan_clusters_ir3(build, src0, ...) \
_nir_build_inclusive_scan_clusters_ir3(build, src0, _nir_inclusive_scan_clusters_ir3_indices{0, __VA_ARGS__})
#else
#define nir_inclusive_scan_clusters_ir3(build, src0, ...) \
_nir_build_inclusive_scan_clusters_ir3(build, src0, (struct _nir_inclusive_scan_clusters_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_initialize_node_payloads(build, src0, src1, src2, ...) \
_nir_build_initialize_node_payloads(build, src0, src1, src2, _nir_initialize_node_payloads_indices{0, __VA_ARGS__})
#else
#define nir_initialize_node_payloads(build, src0, src1, src2, ...) \
_nir_build_initialize_node_payloads(build, src0, src1, src2, (struct _nir_initialize_node_payloads_indices){0, __VA_ARGS__})
#endif
#define nir_interp_deref_at_centroid _nir_build_interp_deref_at_centroid
#define nir_interp_deref_at_offset _nir_build_interp_deref_at_offset
#define nir_interp_deref_at_sample _nir_build_interp_deref_at_sample
#define nir_interp_deref_at_vertex _nir_build_interp_deref_at_vertex
#define nir_inverse_ballot _nir_build_inverse_ballot
#ifdef __cplusplus
#define nir_ipa_nv(build, src0, src1, ...) \
_nir_build_ipa_nv(build, src0, src1, _nir_ipa_nv_indices{0, __VA_ARGS__})
#else
#define nir_ipa_nv(build, src0, src1, ...) \
_nir_build_ipa_nv(build, src0, src1, (struct _nir_ipa_nv_indices){0, __VA_ARGS__})
#endif
#define nir_is_helper_invocation _nir_build_is_helper_invocation
#define nir_is_null_descriptor _nir_build_is_null_descriptor
#define nir_is_sparse_resident_zink _nir_build_is_sparse_resident_zink
#define nir_is_sparse_texels_resident _nir_build_is_sparse_texels_resident
#ifdef __cplusplus
#define nir_is_subgroup_invocation_lt_amd(build, src0, ...) \
_nir_build_is_subgroup_invocation_lt_amd(build, src0, _nir_is_subgroup_invocation_lt_amd_indices{0, __VA_ARGS__})
#else
#define nir_is_subgroup_invocation_lt_amd(build, src0, ...) \
_nir_build_is_subgroup_invocation_lt_amd(build, src0, (struct _nir_is_subgroup_invocation_lt_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_isberd_nv(build, bit_size, src0, ...) \
_nir_build_isberd_nv(build, bit_size, src0, _nir_isberd_nv_indices{0, __VA_ARGS__})
#else
#define nir_isberd_nv(build, bit_size, src0, ...) \
_nir_build_isberd_nv(build, bit_size, src0, (struct _nir_isberd_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_isbewr_nv(build, src0, src1, ...) \
_nir_build_isbewr_nv(build, src0, src1, _nir_isbewr_nv_indices{0, __VA_ARGS__})
#else
#define nir_isbewr_nv(build, src0, src1, ...) \
_nir_build_isbewr_nv(build, src0, src1, (struct _nir_isbewr_nv_indices){0, __VA_ARGS__})
#endif
#define nir_isp_feedback_pco _nir_build_isp_feedback_pco
#define nir_lane_permute_16_amd _nir_build_lane_permute_16_amd
#define nir_last_invocation _nir_build_last_invocation
#ifdef __cplusplus
#define nir_launch_mesh_workgroups(build, src0, ...) \
_nir_build_launch_mesh_workgroups(build, src0, _nir_launch_mesh_workgroups_indices{0, __VA_ARGS__})
#else
#define nir_launch_mesh_workgroups(build, src0, ...) \
_nir_build_launch_mesh_workgroups(build, src0, (struct _nir_launch_mesh_workgroups_indices){0, __VA_ARGS__})
#endif
#define nir_launch_mesh_workgroups_with_payload_deref _nir_build_launch_mesh_workgroups_with_payload_deref
#ifdef __cplusplus
#define nir_ldc_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_ldc_nv(build, num_components, bit_size, src0, src1, _nir_ldc_nv_indices{0, __VA_ARGS__})
#else
#define nir_ldc_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_ldc_nv(build, num_components, bit_size, src0, src1, (struct _nir_ldc_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ldcx_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_ldcx_nv(build, num_components, bit_size, src0, src1, _nir_ldcx_nv_indices{0, __VA_ARGS__})
#else
#define nir_ldcx_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_ldcx_nv(build, num_components, bit_size, src0, src1, (struct _nir_ldcx_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ldtram_nv(build, ...) \
_nir_build_ldtram_nv(build, _nir_ldtram_nv_indices{0, __VA_ARGS__})
#else
#define nir_ldtram_nv(build, ...) \
_nir_build_ldtram_nv(build, (struct _nir_ldtram_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_lea_attr_pan(build, src0, src1, src2, ...) \
_nir_build_lea_attr_pan(build, src0, src1, src2, _nir_lea_attr_pan_indices{0, __VA_ARGS__})
#else
#define nir_lea_attr_pan(build, src0, src1, src2, ...) \
_nir_build_lea_attr_pan(build, src0, src1, src2, (struct _nir_lea_attr_pan_indices){0, __VA_ARGS__})
#endif
#define nir_lea_buf_pan _nir_build_lea_buf_pan
#ifdef __cplusplus
#define nir_lea_tex_pan(build, src0, src1, ...) \
_nir_build_lea_tex_pan(build, src0, src1, _nir_lea_tex_pan_indices{0, __VA_ARGS__})
#else
#define nir_lea_tex_pan(build, src0, src1, ...) \
_nir_build_lea_tex_pan(build, src0, src1, (struct _nir_lea_tex_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_aa_line_width _nir_build_load_aa_line_width
#define nir_load_abort_buffer_address _nir_build_load_abort_buffer_address
#define nir_load_abort_buffer_size _nir_build_load_abort_buffer_size
#define nir_load_accel_struct_amd _nir_build_load_accel_struct_amd
#define nir_load_active_samples_agx _nir_build_load_active_samples_agx
#define nir_load_active_subgroup_count_agx _nir_build_load_active_subgroup_count_agx
#define nir_load_active_subgroup_invocation_agx _nir_build_load_active_subgroup_invocation_agx
#ifdef __cplusplus
#define nir_load_agx(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_agx(build, num_components, bit_size, src0, src1, _nir_load_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_agx(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_agx(build, num_components, bit_size, src0, src1, (struct _nir_load_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_alpha_reference_amd _nir_build_load_alpha_reference_amd
#define nir_load_alpha_to_coverage_enable_ir3 _nir_build_load_alpha_to_coverage_enable_ir3
#define nir_load_amplification_id_kk _nir_build_load_amplification_id_kk
#define nir_load_api_sample_mask_agx _nir_build_load_api_sample_mask_agx
#define nir_load_attrib_clamp_agx _nir_build_load_attrib_clamp_agx
#ifdef __cplusplus
#define nir_load_attribute_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_attribute_pan(build, num_components, bit_size, src0, src1, src2, _nir_load_attribute_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_attribute_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_attribute_pan(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_attribute_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_attribute_payload_intel _nir_build_load_attribute_payload_intel
#define nir_load_back_face_agx _nir_build_load_back_face_agx
#ifdef __cplusplus
#define nir_load_barycentric_at_offset(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_offset(build, bit_size, src0, _nir_load_barycentric_at_offset_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_at_offset(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_offset(build, bit_size, src0, (struct _nir_load_barycentric_at_offset_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_at_offset_nv(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_offset_nv(build, bit_size, src0, _nir_load_barycentric_at_offset_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_at_offset_nv(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_offset_nv(build, bit_size, src0, (struct _nir_load_barycentric_at_offset_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_at_sample(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_sample(build, bit_size, src0, _nir_load_barycentric_at_sample_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_at_sample(build, bit_size, src0, ...) \
_nir_build_load_barycentric_at_sample(build, bit_size, src0, (struct _nir_load_barycentric_at_sample_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_centroid(build, bit_size, ...) \
_nir_build_load_barycentric_centroid(build, bit_size, _nir_load_barycentric_centroid_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_centroid(build, bit_size, ...) \
_nir_build_load_barycentric_centroid(build, bit_size, (struct _nir_load_barycentric_centroid_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_coord_at_offset(build, bit_size, src0, ...) \
_nir_build_load_barycentric_coord_at_offset(build, bit_size, src0, _nir_load_barycentric_coord_at_offset_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_coord_at_offset(build, bit_size, src0, ...) \
_nir_build_load_barycentric_coord_at_offset(build, bit_size, src0, (struct _nir_load_barycentric_coord_at_offset_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_coord_at_sample(build, bit_size, src0, ...) \
_nir_build_load_barycentric_coord_at_sample(build, bit_size, src0, _nir_load_barycentric_coord_at_sample_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_coord_at_sample(build, bit_size, src0, ...) \
_nir_build_load_barycentric_coord_at_sample(build, bit_size, src0, (struct _nir_load_barycentric_coord_at_sample_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_coord_centroid(build, bit_size, ...) \
_nir_build_load_barycentric_coord_centroid(build, bit_size, _nir_load_barycentric_coord_centroid_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_coord_centroid(build, bit_size, ...) \
_nir_build_load_barycentric_coord_centroid(build, bit_size, (struct _nir_load_barycentric_coord_centroid_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_coord_pixel(build, bit_size, ...) \
_nir_build_load_barycentric_coord_pixel(build, bit_size, _nir_load_barycentric_coord_pixel_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_coord_pixel(build, bit_size, ...) \
_nir_build_load_barycentric_coord_pixel(build, bit_size, (struct _nir_load_barycentric_coord_pixel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_coord_sample(build, bit_size, ...) \
_nir_build_load_barycentric_coord_sample(build, bit_size, _nir_load_barycentric_coord_sample_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_coord_sample(build, bit_size, ...) \
_nir_build_load_barycentric_coord_sample(build, bit_size, (struct _nir_load_barycentric_coord_sample_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_model(build, bit_size, ...) \
_nir_build_load_barycentric_model(build, bit_size, _nir_load_barycentric_model_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_model(build, bit_size, ...) \
_nir_build_load_barycentric_model(build, bit_size, (struct _nir_load_barycentric_model_indices){0, __VA_ARGS__})
#endif
#define nir_load_barycentric_optimize_amd _nir_build_load_barycentric_optimize_amd
#ifdef __cplusplus
#define nir_load_barycentric_pixel(build, bit_size, ...) \
_nir_build_load_barycentric_pixel(build, bit_size, _nir_load_barycentric_pixel_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_pixel(build, bit_size, ...) \
_nir_build_load_barycentric_pixel(build, bit_size, (struct _nir_load_barycentric_pixel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_barycentric_sample(build, bit_size, ...) \
_nir_build_load_barycentric_sample(build, bit_size, _nir_load_barycentric_sample_indices{0, __VA_ARGS__})
#else
#define nir_load_barycentric_sample(build, bit_size, ...) \
_nir_build_load_barycentric_sample(build, bit_size, (struct _nir_load_barycentric_sample_indices){0, __VA_ARGS__})
#endif
#define nir_load_base_global_invocation_id _nir_build_load_base_global_invocation_id
#define nir_load_base_instance _nir_build_load_base_instance
#define nir_load_base_vertex _nir_build_load_base_vertex
#define nir_load_base_workgroup_id _nir_build_load_base_workgroup_id
#define nir_load_blend_const_color_a_float _nir_build_load_blend_const_color_a_float
#define nir_load_blend_const_color_aaaa8888_unorm _nir_build_load_blend_const_color_aaaa8888_unorm
#define nir_load_blend_const_color_b_float _nir_build_load_blend_const_color_b_float
#define nir_load_blend_const_color_g_float _nir_build_load_blend_const_color_g_float
#define nir_load_blend_const_color_r_float _nir_build_load_blend_const_color_r_float
#define nir_load_blend_const_color_rgba _nir_build_load_blend_const_color_rgba
#define nir_load_blend_const_color_rgba8888_unorm _nir_build_load_blend_const_color_rgba8888_unorm
#ifdef __cplusplus
#define nir_load_blend_descriptor_pan(build, ...) \
_nir_build_load_blend_descriptor_pan(build, _nir_load_blend_descriptor_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_blend_descriptor_pan(build, ...) \
_nir_build_load_blend_descriptor_pan(build, (struct _nir_load_blend_descriptor_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_blend_input_pan(build, num_components, bit_size, ...) \
_nir_build_load_blend_input_pan(build, num_components, bit_size, _nir_load_blend_input_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_blend_input_pan(build, num_components, bit_size, ...) \
_nir_build_load_blend_input_pan(build, num_components, bit_size, (struct _nir_load_blend_input_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_btd_global_arg_addr_intel _nir_build_load_btd_global_arg_addr_intel
#define nir_load_btd_local_arg_addr_intel _nir_build_load_btd_local_arg_addr_intel
#define nir_load_btd_resume_sbt_addr_intel _nir_build_load_btd_resume_sbt_addr_intel
#define nir_load_btd_shader_type_intel _nir_build_load_btd_shader_type_intel
#define nir_load_btd_stack_id_intel _nir_build_load_btd_stack_id_intel
#ifdef __cplusplus
#define nir_load_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_load_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, _nir_load_buffer_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_load_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_load_buffer_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_buffer_ptr_deref(build, num_components, bit_size, src0, ...) \
_nir_build_load_buffer_ptr_deref(build, num_components, bit_size, src0, _nir_load_buffer_ptr_deref_indices{0, __VA_ARGS__})
#else
#define nir_load_buffer_ptr_deref(build, num_components, bit_size, src0, ...) \
_nir_build_load_buffer_ptr_deref(build, num_components, bit_size, src0, (struct _nir_load_buffer_ptr_deref_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_buffer_ptr_kk(build, num_components, bit_size, ...) \
_nir_build_load_buffer_ptr_kk(build, num_components, bit_size, _nir_load_buffer_ptr_kk_indices{0, __VA_ARGS__})
#else
#define nir_load_buffer_ptr_kk(build, num_components, bit_size, ...) \
_nir_build_load_buffer_ptr_kk(build, num_components, bit_size, (struct _nir_load_buffer_ptr_kk_indices){0, __VA_ARGS__})
#endif
#define nir_load_call_return_address_amd _nir_build_load_call_return_address_amd
#define nir_load_callable_sbt_addr_intel _nir_build_load_callable_sbt_addr_intel
#define nir_load_callable_sbt_stride_intel _nir_build_load_callable_sbt_stride_intel
#define nir_load_clamp_vertex_color_amd _nir_build_load_clamp_vertex_color_amd
#ifdef __cplusplus
#define nir_load_clear_value_pan(build, num_components, bit_size, ...) \
_nir_build_load_clear_value_pan(build, num_components, bit_size, _nir_load_clear_value_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_clear_value_pan(build, num_components, bit_size, ...) \
_nir_build_load_clear_value_pan(build, num_components, bit_size, (struct _nir_load_clear_value_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_clip_half_line_width_amd _nir_build_load_clip_half_line_width_amd
#define nir_load_clip_z_coeff _nir_build_load_clip_z_coeff
#define nir_load_cluster_num_pco _nir_build_load_cluster_num_pco
#define nir_load_coalesced_input_count _nir_build_load_coalesced_input_count
#define nir_load_coeff_pco _nir_build_load_coeff_pco
#ifdef __cplusplus
#define nir_load_coefficients_agx(build, src0, ...) \
_nir_build_load_coefficients_agx(build, src0, _nir_load_coefficients_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_coefficients_agx(build, src0, ...) \
_nir_build_load_coefficients_agx(build, src0, (struct _nir_load_coefficients_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_color0_amd _nir_build_load_color0_amd
#define nir_load_color1_amd _nir_build_load_color1_amd
#define nir_load_const_buf_base_addr_lvp _nir_build_load_const_buf_base_addr_lvp
#ifdef __cplusplus
#define nir_load_const_ir3(build, num_components, bit_size, src0, ...) \
_nir_build_load_const_ir3(build, num_components, bit_size, src0, _nir_load_const_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_const_ir3(build, num_components, bit_size, src0, ...) \
_nir_build_load_const_ir3(build, num_components, bit_size, src0, (struct _nir_load_const_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_constant(build, num_components, bit_size, src0, _nir_load_constant_indices{0, __VA_ARGS__})
#else
#define nir_load_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_constant(build, num_components, bit_size, src0, (struct _nir_load_constant_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_constant_agx(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_constant_agx(build, num_components, bit_size, src0, src1, _nir_load_constant_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_constant_agx(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_constant_agx(build, num_components, bit_size, src0, src1, (struct _nir_load_constant_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_constant_base_ptr _nir_build_load_constant_base_ptr
#define nir_load_core_count_arm _nir_build_load_core_count_arm
#define nir_load_core_id _nir_build_load_core_id
#define nir_load_core_max_id_arm _nir_build_load_core_max_id_arm
#define nir_load_coverage_mask_intel _nir_build_load_coverage_mask_intel
#define nir_load_cull_any_enabled_amd _nir_build_load_cull_any_enabled_amd
#define nir_load_cull_back_face_enabled_amd _nir_build_load_cull_back_face_enabled_amd
#define nir_load_cull_ccw_amd _nir_build_load_cull_ccw_amd
#define nir_load_cull_front_face_enabled_amd _nir_build_load_cull_front_face_enabled_amd
#define nir_load_cull_line_viewport_xy_scale_and_offset_amd _nir_build_load_cull_line_viewport_xy_scale_and_offset_amd
#define nir_load_cull_mask _nir_build_load_cull_mask
#define nir_load_cull_mask_and_flags_amd _nir_build_load_cull_mask_and_flags_amd
#define nir_load_cull_small_line_precision_amd _nir_build_load_cull_small_line_precision_amd
#define nir_load_cull_small_lines_enabled_amd _nir_build_load_cull_small_lines_enabled_amd
#define nir_load_cull_small_triangle_precision_amd _nir_build_load_cull_small_triangle_precision_amd
#define nir_load_cull_small_triangles_enabled_amd _nir_build_load_cull_small_triangles_enabled_amd
#define nir_load_cull_triangle_viewport_xy_scale_and_offset_amd _nir_build_load_cull_triangle_viewport_xy_scale_and_offset_amd
#define nir_load_cumulative_coverage_pan _nir_build_load_cumulative_coverage_pan
#define nir_load_debug_log_desc_amd _nir_build_load_debug_log_desc_amd
#define nir_load_depth_never_agx _nir_build_load_depth_never_agx
#ifdef __cplusplus
#define nir_load_depth_texture_kk(build, num_components, bit_size, src0, ...) \
_nir_build_load_depth_texture_kk(build, num_components, bit_size, src0, _nir_load_depth_texture_kk_indices{0, __VA_ARGS__})
#else
#define nir_load_depth_texture_kk(build, num_components, bit_size, src0, ...) \
_nir_build_load_depth_texture_kk(build, num_components, bit_size, src0, (struct _nir_load_depth_texture_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_load_deref(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref(build, num_components, bit_size, src0, _nir_load_deref_indices{0, __VA_ARGS__})
#else
#define nir_build_load_deref(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref(build, num_components, bit_size, src0, (struct _nir_load_deref_indices){0, __VA_ARGS__})
#endif
#define nir_load_deref nir_build_load_deref
#ifdef __cplusplus
#define nir_load_deref_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref_block_intel(build, num_components, bit_size, src0, _nir_load_deref_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_deref_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref_block_intel(build, num_components, bit_size, src0, (struct _nir_load_deref_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_deref_transpose_amd(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref_transpose_amd(build, num_components, bit_size, src0, _nir_load_deref_transpose_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_deref_transpose_amd(build, num_components, bit_size, src0, ...) \
_nir_build_load_deref_transpose_amd(build, num_components, bit_size, src0, (struct _nir_load_deref_transpose_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_descriptor_set_agx(build, ...) \
_nir_build_load_descriptor_set_agx(build, _nir_load_descriptor_set_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_descriptor_set_agx(build, ...) \
_nir_build_load_descriptor_set_agx(build, (struct _nir_load_descriptor_set_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_draw_id _nir_build_load_draw_id
#define nir_load_esgs_vertex_stride_amd _nir_build_load_esgs_vertex_stride_amd
#ifdef __cplusplus
#define nir_load_exported_agx(build, num_components, bit_size, ...) \
_nir_build_load_exported_agx(build, num_components, bit_size, _nir_load_exported_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_exported_agx(build, num_components, bit_size, ...) \
_nir_build_load_exported_agx(build, num_components, bit_size, (struct _nir_load_exported_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_face_ccw_pco _nir_build_load_face_ccw_pco
#define nir_load_fb_layers_v3d _nir_build_load_fb_layers_v3d
#define nir_load_fb_render_area_pan _nir_build_load_fb_render_area_pan
#define nir_load_fbfetch_image_desc_amd _nir_build_load_fbfetch_image_desc_amd
#define nir_load_fbfetch_image_fmask_desc_amd _nir_build_load_fbfetch_image_fmask_desc_amd
#define nir_load_fep_w_v3d _nir_build_load_fep_w_v3d
#define nir_load_first_vertex _nir_build_load_first_vertex
#define nir_load_fixed_point_size_agx _nir_build_load_fixed_point_size_agx
#define nir_load_flat_mask _nir_build_load_flat_mask
#define nir_load_force_vrs_rates_amd _nir_build_load_force_vrs_rates_amd
#define nir_load_frag_coord _nir_build_load_frag_coord
#define nir_load_frag_coord_gmem_ir3 _nir_build_load_frag_coord_gmem_ir3
#define nir_load_frag_coord_unscaled_ir3 _nir_build_load_frag_coord_unscaled_ir3
#define nir_load_frag_coord_w _nir_build_load_frag_coord_w
#define nir_load_frag_coord_w_rcp _nir_build_load_frag_coord_w_rcp
#define nir_load_frag_coord_xy _nir_build_load_frag_coord_xy
#define nir_load_frag_coord_z _nir_build_load_frag_coord_z
#define nir_load_frag_invocation_count _nir_build_load_frag_invocation_count
#ifdef __cplusplus
#define nir_load_frag_offset_ir3(build, src0, ...) \
_nir_build_load_frag_offset_ir3(build, src0, _nir_load_frag_offset_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_frag_offset_ir3(build, src0, ...) \
_nir_build_load_frag_offset_ir3(build, src0, (struct _nir_load_frag_offset_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_load_frag_shading_rate _nir_build_load_frag_shading_rate
#define nir_load_frag_shading_rate_intel _nir_build_load_frag_shading_rate_intel
#define nir_load_frag_size _nir_build_load_frag_size
#ifdef __cplusplus
#define nir_load_frag_size_ir3(build, src0, ...) \
_nir_build_load_frag_size_ir3(build, src0, _nir_load_frag_size_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_frag_size_ir3(build, src0, ...) \
_nir_build_load_frag_size_ir3(build, src0, (struct _nir_load_frag_size_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_load_from_texture_handle_agx _nir_build_load_from_texture_handle_agx
#define nir_load_front_face _nir_build_load_front_face
#define nir_load_front_face_fsign _nir_build_load_front_face_fsign
#define nir_load_front_face_op_pco _nir_build_load_front_face_op_pco
#ifdef __cplusplus
#define nir_load_fs_coeffs_pco(build, ...) \
_nir_build_load_fs_coeffs_pco(build, _nir_load_fs_coeffs_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_fs_coeffs_pco(build, ...) \
_nir_build_load_fs_coeffs_pco(build, (struct _nir_load_fs_coeffs_pco_indices){0, __VA_ARGS__})
#endif
#define nir_load_fs_config_intel _nir_build_load_fs_config_intel
#ifdef __cplusplus
#define nir_load_fs_input_interp_deltas(build, bit_size, src0, ...) \
_nir_build_load_fs_input_interp_deltas(build, bit_size, src0, _nir_load_fs_input_interp_deltas_indices{0, __VA_ARGS__})
#else
#define nir_load_fs_input_interp_deltas(build, bit_size, src0, ...) \
_nir_build_load_fs_input_interp_deltas(build, bit_size, src0, (struct _nir_load_fs_input_interp_deltas_indices){0, __VA_ARGS__})
#endif
#define nir_load_fs_meta_pco _nir_build_load_fs_meta_pco
#define nir_load_fs_start_intel _nir_build_load_fs_start_intel
#define nir_load_fs_z_c0_intel _nir_build_load_fs_z_c0_intel
#define nir_load_fs_z_c_intel _nir_build_load_fs_z_c_intel
#define nir_load_fully_covered _nir_build_load_fully_covered
#define nir_load_geometry_param_buffer_poly _nir_build_load_geometry_param_buffer_poly
#ifdef __cplusplus
#define nir_build_load_global(build, num_components, bit_size, src0, ...) \
_nir_build_load_global(build, num_components, bit_size, src0, _nir_load_global_indices{0, __VA_ARGS__})
#else
#define nir_build_load_global(build, num_components, bit_size, src0, ...) \
_nir_build_load_global(build, num_components, bit_size, src0, (struct _nir_load_global_indices){0, __VA_ARGS__})
#endif
#define nir_load_global nir_build_load_global
#ifdef __cplusplus
#define nir_load_global_2x32(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_2x32(build, num_components, bit_size, src0, _nir_load_global_2x32_indices{0, __VA_ARGS__})
#else
#define nir_load_global_2x32(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_2x32(build, num_components, bit_size, src0, (struct _nir_load_global_2x32_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_amd(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_amd(build, num_components, bit_size, src0, src1, _nir_load_global_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_global_amd(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_amd(build, num_components, bit_size, src0, src1, (struct _nir_load_global_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_global_base_ptr _nir_build_load_global_base_ptr
#ifdef __cplusplus
#define nir_load_global_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_block_intel(build, num_components, bit_size, src0, _nir_load_global_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_global_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_block_intel(build, num_components, bit_size, src0, (struct _nir_load_global_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_bounded(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_bounded(build, num_components, bit_size, src0, src1, src2, _nir_load_global_bounded_indices{0, __VA_ARGS__})
#else
#define nir_load_global_bounded(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_bounded(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_global_bounded_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_load_global_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_constant(build, num_components, bit_size, src0, _nir_load_global_constant_indices{0, __VA_ARGS__})
#else
#define nir_build_load_global_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_constant(build, num_components, bit_size, src0, (struct _nir_load_global_constant_indices){0, __VA_ARGS__})
#endif
#define nir_load_global_constant nir_build_load_global_constant
#ifdef __cplusplus
#define nir_load_global_constant_bounded(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_constant_bounded(build, num_components, bit_size, src0, src1, src2, _nir_load_global_constant_bounded_indices{0, __VA_ARGS__})
#else
#define nir_load_global_constant_bounded(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_constant_bounded(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_global_constant_bounded_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_constant_offset(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_constant_offset(build, num_components, bit_size, src0, src1, _nir_load_global_constant_offset_indices{0, __VA_ARGS__})
#else
#define nir_load_global_constant_offset(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_constant_offset(build, num_components, bit_size, src0, src1, (struct _nir_load_global_constant_offset_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_constant_uniform_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_constant_uniform_block_intel(build, num_components, bit_size, src0, _nir_load_global_constant_uniform_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_global_constant_uniform_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_constant_uniform_block_intel(build, num_components, bit_size, src0, (struct _nir_load_global_constant_uniform_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_cvt_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_cvt_pan(build, num_components, bit_size, src0, src1, _nir_load_global_cvt_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_global_cvt_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_cvt_pan(build, num_components, bit_size, src0, src1, (struct _nir_load_global_cvt_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_etna(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_etna(build, num_components, bit_size, src0, src1, _nir_load_global_etna_indices{0, __VA_ARGS__})
#else
#define nir_load_global_etna(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_etna(build, num_components, bit_size, src0, src1, (struct _nir_load_global_etna_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_intel(build, num_components, bit_size, src0, _nir_load_global_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_global_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_intel(build, num_components, bit_size, src0, (struct _nir_load_global_intel_indices){0, __VA_ARGS__})
#endif
#define nir_load_global_invocation_id _nir_build_load_global_invocation_id
#define nir_load_global_invocation_index _nir_build_load_global_invocation_index
#ifdef __cplusplus
#define nir_load_global_nv(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_nv(build, num_components, bit_size, src0, src1, src2, _nir_load_global_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_global_nv(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_global_nv(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_global_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_offset(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_offset(build, num_components, bit_size, src0, src1, _nir_load_global_offset_indices{0, __VA_ARGS__})
#else
#define nir_load_global_offset(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_offset(build, num_components, bit_size, src0, src1, (struct _nir_load_global_offset_indices){0, __VA_ARGS__})
#endif
#define nir_load_global_size _nir_build_load_global_size
#ifdef __cplusplus
#define nir_load_global_tr_amd(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_tr_amd(build, num_components, bit_size, src0, src1, _nir_load_global_tr_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_global_tr_amd(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_global_tr_amd(build, num_components, bit_size, src0, src1, (struct _nir_load_global_tr_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_global_transpose_amd(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_transpose_amd(build, num_components, bit_size, src0, _nir_load_global_transpose_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_global_transpose_amd(build, num_components, bit_size, src0, ...) \
_nir_build_load_global_transpose_amd(build, num_components, bit_size, src0, (struct _nir_load_global_transpose_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_gmem_frag_offset_ir3(build, src0, ...) \
_nir_build_load_gmem_frag_offset_ir3(build, src0, _nir_load_gmem_frag_offset_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_gmem_frag_offset_ir3(build, src0, ...) \
_nir_build_load_gmem_frag_offset_ir3(build, src0, (struct _nir_load_gmem_frag_offset_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_gmem_frag_scale_ir3(build, src0, ...) \
_nir_build_load_gmem_frag_scale_ir3(build, src0, _nir_load_gmem_frag_scale_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_gmem_frag_scale_ir3(build, src0, ...) \
_nir_build_load_gmem_frag_scale_ir3(build, src0, (struct _nir_load_gmem_frag_scale_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_load_gs_header_ir3 _nir_build_load_gs_header_ir3
#ifdef __cplusplus
#define nir_load_gs_vertex_offset_amd(build, ...) \
_nir_build_load_gs_vertex_offset_amd(build, _nir_load_gs_vertex_offset_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_gs_vertex_offset_amd(build, ...) \
_nir_build_load_gs_vertex_offset_amd(build, (struct _nir_load_gs_vertex_offset_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_gs_wave_id_amd _nir_build_load_gs_wave_id_amd
#ifdef __cplusplus
#define nir_load_heap_descriptor(build, num_components, bit_size, src0, ...) \
_nir_build_load_heap_descriptor(build, num_components, bit_size, src0, _nir_load_heap_descriptor_indices{0, __VA_ARGS__})
#else
#define nir_load_heap_descriptor(build, num_components, bit_size, src0, ...) \
_nir_build_load_heap_descriptor(build, num_components, bit_size, src0, (struct _nir_load_heap_descriptor_indices){0, __VA_ARGS__})
#endif
#define nir_load_helper_arg_hi_agx _nir_build_load_helper_arg_hi_agx
#define nir_load_helper_arg_lo_agx _nir_build_load_helper_arg_lo_agx
#define nir_load_helper_invocation _nir_build_load_helper_invocation
#define nir_load_helper_op_id_agx _nir_build_load_helper_op_id_agx
#ifdef __cplusplus
#define nir_load_hit_attrib_amd(build, ...) \
_nir_build_load_hit_attrib_amd(build, _nir_load_hit_attrib_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_hit_attrib_amd(build, ...) \
_nir_build_load_hit_attrib_amd(build, (struct _nir_load_hit_attrib_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_hs_out_patch_data_offset_amd _nir_build_load_hs_out_patch_data_offset_amd
#define nir_load_hs_patch_stride_ir3 _nir_build_load_hs_patch_stride_ir3
#define nir_load_idvs_output_buf_index_pan _nir_build_load_idvs_output_buf_index_pan
#ifdef __cplusplus
#define nir_load_incoming_ray_payload_amd(build, ...) \
_nir_build_load_incoming_ray_payload_amd(build, _nir_load_incoming_ray_payload_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_incoming_ray_payload_amd(build, ...) \
_nir_build_load_incoming_ray_payload_amd(build, (struct _nir_load_incoming_ray_payload_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_index_size_poly _nir_build_load_index_size_poly
#define nir_load_indirect_address_intel _nir_build_load_indirect_address_intel
#define nir_load_initial_edgeflags_amd _nir_build_load_initial_edgeflags_amd
#ifdef __cplusplus
#define nir_load_inline_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_inline_data_intel(build, num_components, bit_size, src0, _nir_load_inline_data_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_inline_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_inline_data_intel(build, num_components, bit_size, src0, (struct _nir_load_inline_data_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_input(build, num_components, bit_size, src0, _nir_load_input_indices{0, __VA_ARGS__})
#else
#define nir_load_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_input(build, num_components, bit_size, src0, (struct _nir_load_input_indices){0, __VA_ARGS__})
#endif
#define nir_load_input_attachment_conv_pan _nir_build_load_input_attachment_conv_pan
#define nir_load_input_attachment_coord _nir_build_load_input_attachment_coord
#define nir_load_input_attachment_target_pan _nir_build_load_input_attachment_target_pan
#define nir_load_input_topology_poly _nir_build_load_input_topology_poly
#ifdef __cplusplus
#define nir_load_input_vertex(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_input_vertex(build, num_components, bit_size, src0, src1, _nir_load_input_vertex_indices{0, __VA_ARGS__})
#else
#define nir_load_input_vertex(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_input_vertex(build, num_components, bit_size, src0, src1, (struct _nir_load_input_vertex_indices){0, __VA_ARGS__})
#endif
#define nir_load_instance_id _nir_build_load_instance_id
#define nir_load_instance_num_pco _nir_build_load_instance_num_pco
#ifdef __cplusplus
#define nir_load_interpolated_input(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_interpolated_input(build, num_components, bit_size, src0, src1, _nir_load_interpolated_input_indices{0, __VA_ARGS__})
#else
#define nir_load_interpolated_input(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_interpolated_input(build, num_components, bit_size, src0, src1, (struct _nir_load_interpolated_input_indices){0, __VA_ARGS__})
#endif
#define nir_load_intersection_opaque_amd _nir_build_load_intersection_opaque_amd
#define nir_load_invocation_id _nir_build_load_invocation_id
#define nir_load_is_first_fan_agx _nir_build_load_is_first_fan_agx
#define nir_load_is_indexed_draw _nir_build_load_is_indexed_draw
#ifdef __cplusplus
#define nir_load_kernel_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_kernel_input(build, num_components, bit_size, src0, _nir_load_kernel_input_indices{0, __VA_ARGS__})
#else
#define nir_load_kernel_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_kernel_input(build, num_components, bit_size, src0, (struct _nir_load_kernel_input_indices){0, __VA_ARGS__})
#endif
#define nir_load_layer_id _nir_build_load_layer_id
#define nir_load_lds_ngg_gs_out_vertex_base_amd _nir_build_load_lds_ngg_gs_out_vertex_base_amd
#define nir_load_leaf_opaque_intel _nir_build_load_leaf_opaque_intel
#define nir_load_leaf_procedural_intel _nir_build_load_leaf_procedural_intel
#define nir_load_line_coord _nir_build_load_line_coord
#define nir_load_line_width _nir_build_load_line_width
#define nir_load_local_invocation_id _nir_build_load_local_invocation_id
#define nir_load_local_invocation_index _nir_build_load_local_invocation_index
#define nir_load_local_invocation_index_intel _nir_build_load_local_invocation_index_intel
#ifdef __cplusplus
#define nir_load_local_pixel_agx(build, num_components, bit_size, src0, ...) \
_nir_build_load_local_pixel_agx(build, num_components, bit_size, src0, _nir_load_local_pixel_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_local_pixel_agx(build, num_components, bit_size, src0, ...) \
_nir_build_load_local_pixel_agx(build, num_components, bit_size, src0, (struct _nir_load_local_pixel_agx_indices){0, __VA_ARGS__})
#endif
#define nir_load_local_shared_r600 _nir_build_load_local_shared_r600
#define nir_load_lshs_vertex_stride_amd _nir_build_load_lshs_vertex_stride_amd
#define nir_load_max_polygon_intel _nir_build_load_max_polygon_intel
#define nir_load_merged_wave_info_amd _nir_build_load_merged_wave_info_amd
#define nir_load_mesh_view_count _nir_build_load_mesh_view_count
#ifdef __cplusplus
#define nir_load_mesh_view_indices(build, num_components, bit_size, src0, ...) \
_nir_build_load_mesh_view_indices(build, num_components, bit_size, src0, _nir_load_mesh_view_indices_indices{0, __VA_ARGS__})
#else
#define nir_load_mesh_view_indices(build, num_components, bit_size, src0, ...) \
_nir_build_load_mesh_view_indices(build, num_components, bit_size, src0, (struct _nir_load_mesh_view_indices_indices){0, __VA_ARGS__})
#endif
#define nir_load_msaa_rate_intel _nir_build_load_msaa_rate_intel
#define nir_load_multisampled_pan _nir_build_load_multisampled_pan
#define nir_load_noperspective_varyings_pan _nir_build_load_noperspective_varyings_pan
#define nir_load_num_subgroups _nir_build_load_num_subgroups
#define nir_load_num_vertices _nir_build_load_num_vertices
#define nir_load_num_vertices_per_primitive_amd _nir_build_load_num_vertices_per_primitive_amd
#define nir_load_num_workgroups _nir_build_load_num_workgroups
#define nir_load_ordered_id_amd _nir_build_load_ordered_id_amd
#ifdef __cplusplus
#define nir_load_outgoing_ray_payload_amd(build, ...) \
_nir_build_load_outgoing_ray_payload_amd(build, _nir_load_outgoing_ray_payload_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_outgoing_ray_payload_amd(build, ...) \
_nir_build_load_outgoing_ray_payload_amd(build, (struct _nir_load_outgoing_ray_payload_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_output(build, num_components, bit_size, src0, ...) \
_nir_build_load_output(build, num_components, bit_size, src0, _nir_load_output_indices{0, __VA_ARGS__})
#else
#define nir_load_output(build, num_components, bit_size, src0, ...) \
_nir_build_load_output(build, num_components, bit_size, src0, (struct _nir_load_output_indices){0, __VA_ARGS__})
#endif
#define nir_load_packed_passthrough_primitive_amd _nir_build_load_packed_passthrough_primitive_amd
#define nir_load_packed_sample_location_pco _nir_build_load_packed_sample_location_pco
#ifdef __cplusplus
#define nir_build_load_param(build, num_components, bit_size, ...) \
_nir_build_load_param(build, num_components, bit_size, _nir_load_param_indices{0, __VA_ARGS__})
#else
#define nir_build_load_param(build, num_components, bit_size, ...) \
_nir_build_load_param(build, num_components, bit_size, (struct _nir_load_param_indices){0, __VA_ARGS__})
#endif
#define nir_load_param nir_build_load_param
#define nir_load_patch_vertices_in _nir_build_load_patch_vertices_in
#define nir_load_per_draw_ptr_kk _nir_build_load_per_draw_ptr_kk
#ifdef __cplusplus
#define nir_load_per_primitive_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_per_primitive_input(build, num_components, bit_size, src0, _nir_load_per_primitive_input_indices{0, __VA_ARGS__})
#else
#define nir_load_per_primitive_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_per_primitive_input(build, num_components, bit_size, src0, (struct _nir_load_per_primitive_input_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_per_primitive_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_primitive_output(build, num_components, bit_size, src0, src1, _nir_load_per_primitive_output_indices{0, __VA_ARGS__})
#else
#define nir_load_per_primitive_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_primitive_output(build, num_components, bit_size, src0, src1, (struct _nir_load_per_primitive_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_per_vertex_input(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_vertex_input(build, num_components, bit_size, src0, src1, _nir_load_per_vertex_input_indices{0, __VA_ARGS__})
#else
#define nir_load_per_vertex_input(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_vertex_input(build, num_components, bit_size, src0, src1, (struct _nir_load_per_vertex_input_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_per_vertex_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_vertex_output(build, num_components, bit_size, src0, src1, _nir_load_per_vertex_output_indices{0, __VA_ARGS__})
#else
#define nir_load_per_vertex_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_vertex_output(build, num_components, bit_size, src0, src1, (struct _nir_load_per_vertex_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_per_view_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_view_output(build, num_components, bit_size, src0, src1, _nir_load_per_view_output_indices{0, __VA_ARGS__})
#else
#define nir_load_per_view_output(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_per_view_output(build, num_components, bit_size, src0, src1, (struct _nir_load_per_view_output_indices){0, __VA_ARGS__})
#endif
#define nir_load_persp_center_rhw_ir3 _nir_build_load_persp_center_rhw_ir3
#define nir_load_pipeline_stat_query_enabled_amd _nir_build_load_pipeline_stat_query_enabled_amd
#define nir_load_pixel_coord _nir_build_load_pixel_coord
#define nir_load_pixel_coord_intel _nir_build_load_pixel_coord_intel
#ifdef __cplusplus
#define nir_load_pixel_local(build, num_components, bit_size, src0, ...) \
_nir_build_load_pixel_local(build, num_components, bit_size, src0, _nir_load_pixel_local_indices{0, __VA_ARGS__})
#else
#define nir_load_pixel_local(build, num_components, bit_size, src0, ...) \
_nir_build_load_pixel_local(build, num_components, bit_size, src0, (struct _nir_load_pixel_local_indices){0, __VA_ARGS__})
#endif
#define nir_load_point_coord _nir_build_load_point_coord
#define nir_load_point_coord_maybe_flipped _nir_build_load_point_coord_maybe_flipped
#define nir_load_poly_line_smooth_enabled _nir_build_load_poly_line_smooth_enabled
#define nir_load_polygon_stipple_agx _nir_build_load_polygon_stipple_agx
#define nir_load_polygon_stipple_buffer_amd _nir_build_load_polygon_stipple_buffer_amd
#ifdef __cplusplus
#define nir_load_preamble(build, num_components, bit_size, ...) \
_nir_build_load_preamble(build, num_components, bit_size, _nir_load_preamble_indices{0, __VA_ARGS__})
#else
#define nir_load_preamble(build, num_components, bit_size, ...) \
_nir_build_load_preamble(build, num_components, bit_size, (struct _nir_load_preamble_indices){0, __VA_ARGS__})
#endif
#define nir_load_prim_gen_query_enabled_amd _nir_build_load_prim_gen_query_enabled_amd
#define nir_load_prim_xfb_query_enabled_amd _nir_build_load_prim_xfb_query_enabled_amd
#define nir_load_primitive_id _nir_build_load_primitive_id
#define nir_load_primitive_id_modulo_r600 _nir_build_load_primitive_id_modulo_r600
#define nir_load_primitive_id_raw_r600 _nir_build_load_primitive_id_raw_r600
#ifdef __cplusplus
#define nir_load_primitive_location_ir3(build, ...) \
_nir_build_load_primitive_location_ir3(build, _nir_load_primitive_location_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_primitive_location_ir3(build, ...) \
_nir_build_load_primitive_location_ir3(build, (struct _nir_load_primitive_location_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_load_printf_buffer_address _nir_build_load_printf_buffer_address
#define nir_load_printf_buffer_size _nir_build_load_printf_buffer_size
#define nir_load_provoking_last _nir_build_load_provoking_last
#define nir_load_provoking_vtx_amd _nir_build_load_provoking_vtx_amd
#define nir_load_provoking_vtx_in_prim_amd _nir_build_load_provoking_vtx_in_prim_amd
#define nir_load_ps_iter_mask_amd _nir_build_load_ps_iter_mask_amd
#ifdef __cplusplus
#define nir_load_push_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_constant(build, num_components, bit_size, src0, _nir_load_push_constant_indices{0, __VA_ARGS__})
#else
#define nir_load_push_constant(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_constant(build, num_components, bit_size, src0, (struct _nir_load_push_constant_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_push_constant_zink(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_constant_zink(build, num_components, bit_size, src0, _nir_load_push_constant_zink_indices{0, __VA_ARGS__})
#else
#define nir_load_push_constant_zink(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_constant_zink(build, num_components, bit_size, src0, (struct _nir_load_push_constant_zink_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_push_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_data_intel(build, num_components, bit_size, src0, _nir_load_push_data_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_push_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_push_data_intel(build, num_components, bit_size, src0, (struct _nir_load_push_data_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_r600_per_vertex_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_r600_per_vertex_input(build, num_components, bit_size, src0, _nir_load_r600_per_vertex_input_indices{0, __VA_ARGS__})
#else
#define nir_load_r600_per_vertex_input(build, num_components, bit_size, src0, ...) \
_nir_build_load_r600_per_vertex_input(build, num_components, bit_size, src0, (struct _nir_load_r600_per_vertex_input_indices){0, __VA_ARGS__})
#endif
#define nir_load_rasterization_primitive_amd _nir_build_load_rasterization_primitive_amd
#define nir_load_rasterization_samples_amd _nir_build_load_rasterization_samples_amd
#define nir_load_rasterization_stream _nir_build_load_rasterization_stream
#ifdef __cplusplus
#define nir_load_raw_output_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_raw_output_pan(build, num_components, bit_size, src0, _nir_load_raw_output_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_raw_output_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_raw_output_pan(build, num_components, bit_size, src0, (struct _nir_load_raw_output_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_raw_vertex_id_pan _nir_build_load_raw_vertex_id_pan
#define nir_load_raw_vertex_offset_pan _nir_build_load_raw_vertex_offset_pan
#define nir_load_ray_base_mem_addr_intel _nir_build_load_ray_base_mem_addr_intel
#define nir_load_ray_flags _nir_build_load_ray_flags
#define nir_load_ray_geometry_index _nir_build_load_ray_geometry_index
#define nir_load_ray_hit_kind _nir_build_load_ray_hit_kind
#define nir_load_ray_hit_sbt_addr_intel _nir_build_load_ray_hit_sbt_addr_intel
#define nir_load_ray_hit_sbt_stride_intel _nir_build_load_ray_hit_sbt_stride_intel
#define nir_load_ray_hw_stack_size_intel _nir_build_load_ray_hw_stack_size_intel
#define nir_load_ray_instance_custom_index _nir_build_load_ray_instance_custom_index
#define nir_load_ray_launch_id _nir_build_load_ray_launch_id
#define nir_load_ray_launch_size _nir_build_load_ray_launch_size
#define nir_load_ray_miss_sbt_addr_intel _nir_build_load_ray_miss_sbt_addr_intel
#define nir_load_ray_miss_sbt_stride_intel _nir_build_load_ray_miss_sbt_stride_intel
#define nir_load_ray_num_dss_rt_stacks_intel _nir_build_load_ray_num_dss_rt_stacks_intel
#define nir_load_ray_object_direction _nir_build_load_ray_object_direction
#define nir_load_ray_object_origin _nir_build_load_ray_object_origin
#ifdef __cplusplus
#define nir_load_ray_object_to_world(build, ...) \
_nir_build_load_ray_object_to_world(build, _nir_load_ray_object_to_world_indices{0, __VA_ARGS__})
#else
#define nir_load_ray_object_to_world(build, ...) \
_nir_build_load_ray_object_to_world(build, (struct _nir_load_ray_object_to_world_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ray_payload_ptr_amd(build, bit_size, ...) \
_nir_build_load_ray_payload_ptr_amd(build, bit_size, _nir_load_ray_payload_ptr_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_ray_payload_ptr_amd(build, bit_size, ...) \
_nir_build_load_ray_payload_ptr_amd(build, bit_size, (struct _nir_load_ray_payload_ptr_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_ray_query_global_intel _nir_build_load_ray_query_global_intel
#define nir_load_ray_sw_stack_size_intel _nir_build_load_ray_sw_stack_size_intel
#define nir_load_ray_t_max _nir_build_load_ray_t_max
#define nir_load_ray_t_min _nir_build_load_ray_t_min
#define nir_load_ray_tracing_stack_base_lvp _nir_build_load_ray_tracing_stack_base_lvp
#ifdef __cplusplus
#define nir_load_ray_triangle_vertex_positions(build, ...) \
_nir_build_load_ray_triangle_vertex_positions(build, _nir_load_ray_triangle_vertex_positions_indices{0, __VA_ARGS__})
#else
#define nir_load_ray_triangle_vertex_positions(build, ...) \
_nir_build_load_ray_triangle_vertex_positions(build, (struct _nir_load_ray_triangle_vertex_positions_indices){0, __VA_ARGS__})
#endif
#define nir_load_ray_world_direction _nir_build_load_ray_world_direction
#define nir_load_ray_world_origin _nir_build_load_ray_world_origin
#ifdef __cplusplus
#define nir_load_ray_world_to_object(build, ...) \
_nir_build_load_ray_world_to_object(build, _nir_load_ray_world_to_object_indices{0, __VA_ARGS__})
#else
#define nir_load_ray_world_to_object(build, ...) \
_nir_build_load_ray_world_to_object(build, (struct _nir_load_ray_world_to_object_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_load_reg(build, num_components, bit_size, src0, ...) \
_nir_build_load_reg(build, num_components, bit_size, src0, _nir_load_reg_indices{0, __VA_ARGS__})
#else
#define nir_build_load_reg(build, num_components, bit_size, src0, ...) \
_nir_build_load_reg(build, num_components, bit_size, src0, (struct _nir_load_reg_indices){0, __VA_ARGS__})
#endif
#define nir_load_reg nir_build_load_reg
#ifdef __cplusplus
#define nir_load_reg_indirect(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_reg_indirect(build, num_components, bit_size, src0, src1, _nir_load_reg_indirect_indices{0, __VA_ARGS__})
#else
#define nir_load_reg_indirect(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_reg_indirect(build, num_components, bit_size, src0, src1, (struct _nir_load_reg_indirect_indices){0, __VA_ARGS__})
#endif
#define nir_load_rel_patch_id_ir3 _nir_build_load_rel_patch_id_ir3
#ifdef __cplusplus
#define nir_load_reloc_const_intel(build, ...) \
_nir_build_load_reloc_const_intel(build, _nir_load_reloc_const_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_reloc_const_intel(build, ...) \
_nir_build_load_reloc_const_intel(build, (struct _nir_load_reloc_const_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_resource_heap_data(build, num_components, bit_size, src0, ...) \
_nir_build_load_resource_heap_data(build, num_components, bit_size, src0, _nir_load_resource_heap_data_indices{0, __VA_ARGS__})
#else
#define nir_load_resource_heap_data(build, num_components, bit_size, src0, ...) \
_nir_build_load_resource_heap_data(build, num_components, bit_size, src0, (struct _nir_load_resource_heap_data_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_resume_shader_address_amd(build, ...) \
_nir_build_load_resume_shader_address_amd(build, _nir_load_resume_shader_address_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_resume_shader_address_amd(build, ...) \
_nir_build_load_resume_shader_address_amd(build, (struct _nir_load_resume_shader_address_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_return_param_amd(build, num_components, bit_size, ...) \
_nir_build_load_return_param_amd(build, num_components, bit_size, _nir_load_return_param_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_return_param_amd(build, num_components, bit_size, ...) \
_nir_build_load_return_param_amd(build, num_components, bit_size, (struct _nir_load_return_param_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_ring_attr_amd _nir_build_load_ring_attr_amd
#define nir_load_ring_attr_offset_amd _nir_build_load_ring_attr_offset_amd
#define nir_load_ring_es2gs_offset_amd _nir_build_load_ring_es2gs_offset_amd
#define nir_load_ring_esgs_amd _nir_build_load_ring_esgs_amd
#define nir_load_ring_gs2vs_offset_amd _nir_build_load_ring_gs2vs_offset_amd
#ifdef __cplusplus
#define nir_load_ring_gsvs_amd(build, ...) \
_nir_build_load_ring_gsvs_amd(build, _nir_load_ring_gsvs_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_ring_gsvs_amd(build, ...) \
_nir_build_load_ring_gsvs_amd(build, (struct _nir_load_ring_gsvs_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_ring_mesh_scratch_amd _nir_build_load_ring_mesh_scratch_amd
#define nir_load_ring_mesh_scratch_offset_amd _nir_build_load_ring_mesh_scratch_offset_amd
#define nir_load_ring_task_draw_amd _nir_build_load_ring_task_draw_amd
#define nir_load_ring_task_payload_amd _nir_build_load_ring_task_payload_amd
#define nir_load_ring_tess_factors_amd _nir_build_load_ring_tess_factors_amd
#define nir_load_ring_tess_factors_offset_amd _nir_build_load_ring_tess_factors_offset_amd
#define nir_load_ring_tess_offchip_amd _nir_build_load_ring_tess_offchip_amd
#define nir_load_ring_tess_offchip_offset_amd _nir_build_load_ring_tess_offchip_offset_amd
#define nir_load_ro_sink_address_poly _nir_build_load_ro_sink_address_poly
#define nir_load_root_agx _nir_build_load_root_agx
#define nir_load_rt_arg_scratch_offset_amd _nir_build_load_rt_arg_scratch_offset_amd
#ifdef __cplusplus
#define nir_load_rt_conversion_pan(build, ...) \
_nir_build_load_rt_conversion_pan(build, _nir_load_rt_conversion_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_rt_conversion_pan(build, ...) \
_nir_build_load_rt_conversion_pan(build, (struct _nir_load_rt_conversion_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_rt_descriptors_amd _nir_build_load_rt_descriptors_amd
#define nir_load_rt_dynamic_descriptors_amd _nir_build_load_rt_dynamic_descriptors_amd
#define nir_load_rt_heap_resource_amd _nir_build_load_rt_heap_resource_amd
#define nir_load_rt_heap_sampler_amd _nir_build_load_rt_heap_sampler_amd
#define nir_load_rt_push_constants_amd _nir_build_load_rt_push_constants_amd
#define nir_load_sample_id _nir_build_load_sample_id
#define nir_load_sample_mask _nir_build_load_sample_mask
#define nir_load_sample_mask_in _nir_build_load_sample_mask_in
#define nir_load_sample_pos _nir_build_load_sample_pos
#define nir_load_sample_pos_from_id _nir_build_load_sample_pos_from_id
#define nir_load_sample_pos_or_center _nir_build_load_sample_pos_or_center
#define nir_load_sample_positions_agx _nir_build_load_sample_positions_agx
#define nir_load_sample_positions_amd _nir_build_load_sample_positions_amd
#define nir_load_sample_positions_pan _nir_build_load_sample_positions_pan
#define nir_load_sampler_handle_agx _nir_build_load_sampler_handle_agx
#define nir_load_sampler_handle_kk _nir_build_load_sampler_handle_kk
#define nir_load_sampler_lod_parameters _nir_build_load_sampler_lod_parameters
#define nir_load_samples_log2_agx _nir_build_load_samples_log2_agx
#define nir_load_savmsk_vm_pco _nir_build_load_savmsk_vm_pco
#define nir_load_sbt_base_amd _nir_build_load_sbt_base_amd
#define nir_load_sbt_offset_amd _nir_build_load_sbt_offset_amd
#define nir_load_sbt_stride_amd _nir_build_load_sbt_stride_amd
#ifdef __cplusplus
#define nir_load_scalar_arg_amd(build, num_components, ...) \
_nir_build_load_scalar_arg_amd(build, num_components, _nir_load_scalar_arg_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_scalar_arg_amd(build, num_components, ...) \
_nir_build_load_scalar_arg_amd(build, num_components, (struct _nir_load_scalar_arg_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_scalar_arg_wg_div_amd(build, num_components, ...) \
_nir_build_load_scalar_arg_wg_div_amd(build, num_components, _nir_load_scalar_arg_wg_div_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_scalar_arg_wg_div_amd(build, num_components, ...) \
_nir_build_load_scalar_arg_wg_div_amd(build, num_components, (struct _nir_load_scalar_arg_wg_div_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_scratch(build, num_components, bit_size, src0, ...) \
_nir_build_load_scratch(build, num_components, bit_size, src0, _nir_load_scratch_indices{0, __VA_ARGS__})
#else
#define nir_load_scratch(build, num_components, bit_size, src0, ...) \
_nir_build_load_scratch(build, num_components, bit_size, src0, (struct _nir_load_scratch_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_scratch_base_ptr(build, num_components, bit_size, ...) \
_nir_build_load_scratch_base_ptr(build, num_components, bit_size, _nir_load_scratch_base_ptr_indices{0, __VA_ARGS__})
#else
#define nir_load_scratch_base_ptr(build, num_components, bit_size, ...) \
_nir_build_load_scratch_base_ptr(build, num_components, bit_size, (struct _nir_load_scratch_base_ptr_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_scratch_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_scratch_intel(build, num_components, bit_size, src0, _nir_load_scratch_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_scratch_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_scratch_intel(build, num_components, bit_size, src0, (struct _nir_load_scratch_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_scratch_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_scratch_nv(build, num_components, bit_size, src0, src1, _nir_load_scratch_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_scratch_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_scratch_nv(build, num_components, bit_size, src0, src1, (struct _nir_load_scratch_nv_indices){0, __VA_ARGS__})
#endif
#define nir_load_shader_call_data_offset_lvp _nir_build_load_shader_call_data_offset_lvp
#define nir_load_shader_index _nir_build_load_shader_index
#ifdef __cplusplus
#define nir_load_shader_indirect_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shader_indirect_data_intel(build, num_components, bit_size, src0, _nir_load_shader_indirect_data_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_shader_indirect_data_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shader_indirect_data_intel(build, num_components, bit_size, src0, (struct _nir_load_shader_indirect_data_intel_indices){0, __VA_ARGS__})
#endif
#define nir_load_shader_output_pan _nir_build_load_shader_output_pan
#define nir_load_shader_part_tests_zs_agx _nir_build_load_shader_part_tests_zs_agx
#define nir_load_shader_record_ptr _nir_build_load_shader_record_ptr
#ifdef __cplusplus
#define nir_load_shared(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared(build, num_components, bit_size, src0, _nir_load_shared_indices{0, __VA_ARGS__})
#else
#define nir_load_shared(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared(build, num_components, bit_size, src0, (struct _nir_load_shared_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_shared2_amd(build, bit_size, src0, ...) \
_nir_build_load_shared2_amd(build, bit_size, src0, _nir_load_shared2_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_shared2_amd(build, bit_size, src0, ...) \
_nir_build_load_shared2_amd(build, bit_size, src0, (struct _nir_load_shared2_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_shared_base_ptr _nir_build_load_shared_base_ptr
#ifdef __cplusplus
#define nir_load_shared_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_block_intel(build, num_components, bit_size, src0, _nir_load_shared_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_shared_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_block_intel(build, num_components, bit_size, src0, (struct _nir_load_shared_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_shared_ir3(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_ir3(build, num_components, bit_size, src0, _nir_load_shared_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_shared_ir3(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_ir3(build, num_components, bit_size, src0, (struct _nir_load_shared_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_shared_lock_nv(build, bit_size, src0, ...) \
_nir_build_load_shared_lock_nv(build, bit_size, src0, _nir_load_shared_lock_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_shared_lock_nv(build, bit_size, src0, ...) \
_nir_build_load_shared_lock_nv(build, bit_size, src0, (struct _nir_load_shared_lock_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_shared_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_shared_nv(build, num_components, bit_size, src0, src1, _nir_load_shared_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_shared_nv(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_shared_nv(build, num_components, bit_size, src0, src1, (struct _nir_load_shared_nv_indices){0, __VA_ARGS__})
#endif
#define nir_load_shared_reg_alloc_size_pco _nir_build_load_shared_reg_alloc_size_pco
#ifdef __cplusplus
#define nir_load_shared_uniform_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_uniform_block_intel(build, num_components, bit_size, src0, _nir_load_shared_uniform_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_shared_uniform_block_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_shared_uniform_block_intel(build, num_components, bit_size, src0, (struct _nir_load_shared_uniform_block_intel_indices){0, __VA_ARGS__})
#endif
#define nir_load_simd_width_intel _nir_build_load_simd_width_intel
#define nir_load_slot_num_pco _nir_build_load_slot_num_pco
#define nir_load_sm_count_nv _nir_build_load_sm_count_nv
#define nir_load_sm_id_nv _nir_build_load_sm_id_nv
#ifdef __cplusplus
#define nir_load_smp_meta_pco(build, num_components, src0, ...) \
_nir_build_load_smp_meta_pco(build, num_components, src0, _nir_load_smp_meta_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_smp_meta_pco(build, num_components, src0, ...) \
_nir_build_load_smp_meta_pco(build, num_components, src0, (struct _nir_load_smp_meta_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_smp_state_pco(build, num_components, src0, ...) \
_nir_build_load_smp_state_pco(build, num_components, src0, _nir_load_smp_state_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_smp_state_pco(build, num_components, src0, ...) \
_nir_build_load_smp_state_pco(build, num_components, src0, (struct _nir_load_smp_state_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo(build, num_components, bit_size, src0, src1, _nir_load_ssbo_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo(build, num_components, bit_size, src0, src1, (struct _nir_load_ssbo_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo_address(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_address(build, num_components, bit_size, src0, src1, _nir_load_ssbo_address_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo_address(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_address(build, num_components, bit_size, src0, src1, (struct _nir_load_ssbo_address_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_block_intel(build, num_components, bit_size, src0, src1, _nir_load_ssbo_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_block_intel(build, num_components, bit_size, src0, src1, (struct _nir_load_ssbo_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_intel(build, num_components, bit_size, src0, src1, _nir_load_ssbo_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_intel(build, num_components, bit_size, src0, src1, (struct _nir_load_ssbo_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo_ir3(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_ssbo_ir3(build, num_components, bit_size, src0, src1, src2, _nir_load_ssbo_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo_ir3(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_ssbo_ir3(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_ssbo_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ssbo_uniform_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_uniform_block_intel(build, num_components, bit_size, src0, src1, _nir_load_ssbo_uniform_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_ssbo_uniform_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ssbo_uniform_block_intel(build, num_components, bit_size, src0, src1, (struct _nir_load_ssbo_uniform_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_stack(build, num_components, bit_size, ...) \
_nir_build_load_stack(build, num_components, bit_size, _nir_load_stack_indices{0, __VA_ARGS__})
#else
#define nir_load_stack(build, num_components, bit_size, ...) \
_nir_build_load_stack(build, num_components, bit_size, (struct _nir_load_stack_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_stat_query_address_poly(build, ...) \
_nir_build_load_stat_query_address_poly(build, _nir_load_stat_query_address_poly_indices{0, __VA_ARGS__})
#else
#define nir_load_stat_query_address_poly(build, ...) \
_nir_build_load_stat_query_address_poly(build, (struct _nir_load_stat_query_address_poly_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_streamout_buffer_amd(build, ...) \
_nir_build_load_streamout_buffer_amd(build, _nir_load_streamout_buffer_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_streamout_buffer_amd(build, ...) \
_nir_build_load_streamout_buffer_amd(build, (struct _nir_load_streamout_buffer_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_streamout_config_amd _nir_build_load_streamout_config_amd
#ifdef __cplusplus
#define nir_load_streamout_offset_amd(build, ...) \
_nir_build_load_streamout_offset_amd(build, _nir_load_streamout_offset_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_streamout_offset_amd(build, ...) \
_nir_build_load_streamout_offset_amd(build, (struct _nir_load_streamout_offset_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_streamout_write_index_amd _nir_build_load_streamout_write_index_amd
#define nir_load_subgroup_eq_mask _nir_build_load_subgroup_eq_mask
#define nir_load_subgroup_ge_mask _nir_build_load_subgroup_ge_mask
#define nir_load_subgroup_gt_mask _nir_build_load_subgroup_gt_mask
#define nir_load_subgroup_id _nir_build_load_subgroup_id
#define nir_load_subgroup_id_shift_ir3 _nir_build_load_subgroup_id_shift_ir3
#define nir_load_subgroup_invocation _nir_build_load_subgroup_invocation
#define nir_load_subgroup_le_mask _nir_build_load_subgroup_le_mask
#define nir_load_subgroup_lt_mask _nir_build_load_subgroup_lt_mask
#define nir_load_subgroup_size _nir_build_load_subgroup_size
#ifdef __cplusplus
#define nir_load_sysval_agx(build, num_components, bit_size, ...) \
_nir_build_load_sysval_agx(build, num_components, bit_size, _nir_load_sysval_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_sysval_agx(build, num_components, bit_size, ...) \
_nir_build_load_sysval_agx(build, num_components, bit_size, (struct _nir_load_sysval_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_sysval_nv(build, bit_size, ...) \
_nir_build_load_sysval_nv(build, bit_size, _nir_load_sysval_nv_indices{0, __VA_ARGS__})
#else
#define nir_load_sysval_nv(build, bit_size, ...) \
_nir_build_load_sysval_nv(build, bit_size, (struct _nir_load_sysval_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_task_payload(build, num_components, bit_size, src0, ...) \
_nir_build_load_task_payload(build, num_components, bit_size, src0, _nir_load_task_payload_indices{0, __VA_ARGS__})
#else
#define nir_load_task_payload(build, num_components, bit_size, src0, ...) \
_nir_build_load_task_payload(build, num_components, bit_size, src0, (struct _nir_load_task_payload_indices){0, __VA_ARGS__})
#endif
#define nir_load_task_ring_entry_amd _nir_build_load_task_ring_entry_amd
#define nir_load_tcs_header_ir3 _nir_build_load_tcs_header_ir3
#define nir_load_tcs_in_param_base_r600 _nir_build_load_tcs_in_param_base_r600
#define nir_load_tcs_mem_attrib_stride _nir_build_load_tcs_mem_attrib_stride
#define nir_load_tcs_num_patches_amd _nir_build_load_tcs_num_patches_amd
#define nir_load_tcs_out_param_base_r600 _nir_build_load_tcs_out_param_base_r600
#define nir_load_tcs_primitive_mode_amd _nir_build_load_tcs_primitive_mode_amd
#define nir_load_tcs_rel_patch_id_r600 _nir_build_load_tcs_rel_patch_id_r600
#define nir_load_tcs_tess_factor_base_r600 _nir_build_load_tcs_tess_factor_base_r600
#define nir_load_tcs_tess_levels_to_tes_amd _nir_build_load_tcs_tess_levels_to_tes_amd
#define nir_load_tess_config_intel _nir_build_load_tess_config_intel
#define nir_load_tess_coord _nir_build_load_tess_coord
#define nir_load_tess_coord_xy _nir_build_load_tess_coord_xy
#define nir_load_tess_factor_base_ir3 _nir_build_load_tess_factor_base_ir3
#define nir_load_tess_level_inner _nir_build_load_tess_level_inner
#define nir_load_tess_level_inner_default _nir_build_load_tess_level_inner_default
#define nir_load_tess_level_outer _nir_build_load_tess_level_outer
#define nir_load_tess_level_outer_default _nir_build_load_tess_level_outer_default
#define nir_load_tess_param_base_ir3 _nir_build_load_tess_param_base_ir3
#define nir_load_tess_param_buffer_poly _nir_build_load_tess_param_buffer_poly
#define nir_load_tess_rel_patch_id_amd _nir_build_load_tess_rel_patch_id_amd
#ifdef __cplusplus
#define nir_load_tex_meta_pco(build, num_components, src0, ...) \
_nir_build_load_tex_meta_pco(build, num_components, src0, _nir_load_tex_meta_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_tex_meta_pco(build, num_components, src0, ...) \
_nir_build_load_tex_meta_pco(build, num_components, src0, (struct _nir_load_tex_meta_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tex_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_tex_pan(build, num_components, bit_size, src0, src1, _nir_load_tex_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_tex_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_tex_pan(build, num_components, bit_size, src0, src1, (struct _nir_load_tex_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_tex_sprite_mask_agx _nir_build_load_tex_sprite_mask_agx
#ifdef __cplusplus
#define nir_load_tex_state_pco(build, num_components, src0, ...) \
_nir_build_load_tex_state_pco(build, num_components, src0, _nir_load_tex_state_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_tex_state_pco(build, num_components, src0, ...) \
_nir_build_load_tex_state_pco(build, num_components, src0, (struct _nir_load_tex_state_pco_indices){0, __VA_ARGS__})
#endif
#define nir_load_texture_handle_agx _nir_build_load_texture_handle_agx
#ifdef __cplusplus
#define nir_load_texture_handle_kk(build, num_components, bit_size, src0, ...) \
_nir_build_load_texture_handle_kk(build, num_components, bit_size, src0, _nir_load_texture_handle_kk_indices{0, __VA_ARGS__})
#else
#define nir_load_texture_handle_kk(build, num_components, bit_size, src0, ...) \
_nir_build_load_texture_handle_kk(build, num_components, bit_size, src0, (struct _nir_load_texture_handle_kk_indices){0, __VA_ARGS__})
#endif
#define nir_load_texture_scale _nir_build_load_texture_scale
#define nir_load_texture_size_etna _nir_build_load_texture_size_etna
#ifdef __cplusplus
#define nir_load_tile_coord_pco(build, num_components, ...) \
_nir_build_load_tile_coord_pco(build, num_components, _nir_load_tile_coord_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_tile_coord_pco(build, num_components, ...) \
_nir_build_load_tile_coord_pco(build, num_components, (struct _nir_load_tile_coord_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tile_image(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_tile_image(build, num_components, bit_size, src0, src1, _nir_load_tile_image_indices{0, __VA_ARGS__})
#else
#define nir_load_tile_image(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_tile_image(build, num_components, bit_size, src0, src1, (struct _nir_load_tile_image_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tile_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_tile_pan(build, num_components, bit_size, src0, src1, src2, _nir_load_tile_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_tile_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_tile_pan(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_tile_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tile_res_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_tile_res_pan(build, num_components, bit_size, src0, src1, src2, _nir_load_tile_res_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_tile_res_pan(build, num_components, bit_size, src0, src1, src2, ...) \
_nir_build_load_tile_res_pan(build, num_components, bit_size, src0, src1, src2, (struct _nir_load_tile_res_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tiled_offset_pco(build, ...) \
_nir_build_load_tiled_offset_pco(build, _nir_load_tiled_offset_pco_indices{0, __VA_ARGS__})
#else
#define nir_load_tiled_offset_pco(build, ...) \
_nir_build_load_tiled_offset_pco(build, (struct _nir_load_tiled_offset_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_tlb_color_brcm(build, num_components, bit_size, src0, ...) \
_nir_build_load_tlb_color_brcm(build, num_components, bit_size, src0, _nir_load_tlb_color_brcm_indices{0, __VA_ARGS__})
#else
#define nir_load_tlb_color_brcm(build, num_components, bit_size, src0, ...) \
_nir_build_load_tlb_color_brcm(build, num_components, bit_size, src0, (struct _nir_load_tlb_color_brcm_indices){0, __VA_ARGS__})
#endif
#define nir_load_topology_id_intel _nir_build_load_topology_id_intel
#ifdef __cplusplus
#define nir_load_ttmp_register_amd(build, ...) \
_nir_build_load_ttmp_register_amd(build, _nir_load_ttmp_register_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_ttmp_register_amd(build, ...) \
_nir_build_load_ttmp_register_amd(build, (struct _nir_load_ttmp_register_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ttmp_register_wg_div_amd(build, ...) \
_nir_build_load_ttmp_register_wg_div_amd(build, _nir_load_ttmp_register_wg_div_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_ttmp_register_wg_div_amd(build, ...) \
_nir_build_load_ttmp_register_wg_div_amd(build, (struct _nir_load_ttmp_register_wg_div_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_typed_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_load_typed_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, _nir_load_typed_buffer_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_typed_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, ...) \
_nir_build_load_typed_buffer_amd(build, num_components, bit_size, src0, src1, src2, src3, (struct _nir_load_typed_buffer_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_uav_ir3(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_uav_ir3(build, num_components, bit_size, src0, src1, _nir_load_uav_ir3_indices{0, __VA_ARGS__})
#else
#define nir_load_uav_ir3(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_uav_ir3(build, num_components, bit_size, src0, src1, (struct _nir_load_uav_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ubo(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo(build, num_components, bit_size, src0, src1, _nir_load_ubo_indices{0, __VA_ARGS__})
#else
#define nir_load_ubo(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo(build, num_components, bit_size, src0, src1, (struct _nir_load_ubo_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ubo_uniform_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo_uniform_block_intel(build, num_components, bit_size, src0, src1, _nir_load_ubo_uniform_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_ubo_uniform_block_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo_uniform_block_intel(build, num_components, bit_size, src0, src1, (struct _nir_load_ubo_uniform_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_ubo_vec4(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo_vec4(build, num_components, bit_size, src0, src1, _nir_load_ubo_vec4_indices{0, __VA_ARGS__})
#else
#define nir_load_ubo_vec4(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_ubo_vec4(build, num_components, bit_size, src0, src1, (struct _nir_load_ubo_vec4_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_uniform(build, num_components, bit_size, src0, ...) \
_nir_build_load_uniform(build, num_components, bit_size, src0, _nir_load_uniform_indices{0, __VA_ARGS__})
#else
#define nir_load_uniform(build, num_components, bit_size, src0, ...) \
_nir_build_load_uniform(build, num_components, bit_size, src0, (struct _nir_load_uniform_indices){0, __VA_ARGS__})
#endif
#define nir_load_urb_input_handle_indexed_intel _nir_build_load_urb_input_handle_indexed_intel
#define nir_load_urb_input_handle_intel _nir_build_load_urb_input_handle_intel
#ifdef __cplusplus
#define nir_load_urb_lsc_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_urb_lsc_intel(build, num_components, bit_size, src0, _nir_load_urb_lsc_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_urb_lsc_intel(build, num_components, bit_size, src0, ...) \
_nir_build_load_urb_lsc_intel(build, num_components, bit_size, src0, (struct _nir_load_urb_lsc_intel_indices){0, __VA_ARGS__})
#endif
#define nir_load_urb_output_handle_intel _nir_build_load_urb_output_handle_intel
#ifdef __cplusplus
#define nir_load_urb_vec4_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_urb_vec4_intel(build, num_components, bit_size, src0, src1, _nir_load_urb_vec4_intel_indices{0, __VA_ARGS__})
#else
#define nir_load_urb_vec4_intel(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_urb_vec4_intel(build, num_components, bit_size, src0, src1, (struct _nir_load_urb_vec4_intel_indices){0, __VA_ARGS__})
#endif
#define nir_load_use_float_frag_coord_xy_amd _nir_build_load_use_float_frag_coord_xy_amd
#define nir_load_use_quad_pos_amd _nir_build_load_use_quad_pos_amd
#define nir_load_use_sample_mask_in_amd _nir_build_load_use_sample_mask_in_amd
#ifdef __cplusplus
#define nir_load_user_clip_plane(build, ...) \
_nir_build_load_user_clip_plane(build, _nir_load_user_clip_plane_indices{0, __VA_ARGS__})
#else
#define nir_load_user_clip_plane(build, ...) \
_nir_build_load_user_clip_plane(build, (struct _nir_load_user_clip_plane_indices){0, __VA_ARGS__})
#endif
#define nir_load_user_data_amd _nir_build_load_user_data_amd
#ifdef __cplusplus
#define nir_load_uvs_index_agx(build, ...) \
_nir_build_load_uvs_index_agx(build, _nir_load_uvs_index_agx_indices{0, __VA_ARGS__})
#else
#define nir_load_uvs_index_agx(build, ...) \
_nir_build_load_uvs_index_agx(build, (struct _nir_load_uvs_index_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_var_buf_flat_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_var_buf_flat_pan(build, num_components, bit_size, src0, _nir_load_var_buf_flat_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_var_buf_flat_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_var_buf_flat_pan(build, num_components, bit_size, src0, (struct _nir_load_var_buf_flat_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_var_buf_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_var_buf_pan(build, num_components, bit_size, src0, src1, _nir_load_var_buf_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_var_buf_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_var_buf_pan(build, num_components, bit_size, src0, src1, (struct _nir_load_var_buf_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_var_flat_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_var_flat_pan(build, num_components, bit_size, src0, _nir_load_var_flat_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_var_flat_pan(build, num_components, bit_size, src0, ...) \
_nir_build_load_var_flat_pan(build, num_components, bit_size, src0, (struct _nir_load_var_flat_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_var_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_var_pan(build, num_components, bit_size, src0, src1, _nir_load_var_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_var_pan(build, num_components, bit_size, src0, src1, ...) \
_nir_build_load_var_pan(build, num_components, bit_size, src0, src1, (struct _nir_load_var_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_load_var_special_pan(build, num_components, src0, ...) \
_nir_build_load_var_special_pan(build, num_components, src0, _nir_load_var_special_pan_indices{0, __VA_ARGS__})
#else
#define nir_load_var_special_pan(build, num_components, src0, ...) \
_nir_build_load_var_special_pan(build, num_components, src0, (struct _nir_load_var_special_pan_indices){0, __VA_ARGS__})
#endif
#define nir_load_vbo_base_agx _nir_build_load_vbo_base_agx
#define nir_load_vbo_stride_agx _nir_build_load_vbo_stride_agx
#ifdef __cplusplus
#define nir_load_vector_arg_amd(build, num_components, ...) \
_nir_build_load_vector_arg_amd(build, num_components, _nir_load_vector_arg_amd_indices{0, __VA_ARGS__})
#else
#define nir_load_vector_arg_amd(build, num_components, ...) \
_nir_build_load_vector_arg_amd(build, num_components, (struct _nir_load_vector_arg_amd_indices){0, __VA_ARGS__})
#endif
#define nir_load_vertex_id _nir_build_load_vertex_id
#define nir_load_vertex_id_zero_base _nir_build_load_vertex_id_zero_base
#define nir_load_vertex_param_buffer_poly _nir_build_load_vertex_param_buffer_poly
#define nir_load_view_index _nir_build_load_view_index
#define nir_load_viewport_offset _nir_build_load_viewport_offset
#define nir_load_viewport_scale _nir_build_load_viewport_scale
#define nir_load_viewport_x_offset _nir_build_load_viewport_x_offset
#define nir_load_viewport_x_scale _nir_build_load_viewport_x_scale
#define nir_load_viewport_y_offset _nir_build_load_viewport_y_offset
#define nir_load_viewport_y_scale _nir_build_load_viewport_y_scale
#define nir_load_viewport_z_offset _nir_build_load_viewport_z_offset
#define nir_load_viewport_z_scale _nir_build_load_viewport_z_scale
#define nir_load_vs_outputs_poly _nir_build_load_vs_outputs_poly
#define nir_load_vs_primitive_stride_ir3 _nir_build_load_vs_primitive_stride_ir3
#define nir_load_vs_vertex_stride_ir3 _nir_build_load_vs_vertex_stride_ir3
#define nir_load_vtxin_pco _nir_build_load_vtxin_pco
#ifdef __cplusplus
#define nir_load_vulkan_descriptor(build, num_components, bit_size, src0, ...) \
_nir_build_load_vulkan_descriptor(build, num_components, bit_size, src0, _nir_load_vulkan_descriptor_indices{0, __VA_ARGS__})
#else
#define nir_load_vulkan_descriptor(build, num_components, bit_size, src0, ...) \
_nir_build_load_vulkan_descriptor(build, num_components, bit_size, src0, (struct _nir_load_vulkan_descriptor_indices){0, __VA_ARGS__})
#endif
#define nir_load_warp_id_arm _nir_build_load_warp_id_arm
#define nir_load_warp_id_nv _nir_build_load_warp_id_nv
#define nir_load_warp_max_id_arm _nir_build_load_warp_max_id_arm
#define nir_load_warps_per_sm_nv _nir_build_load_warps_per_sm_nv
#define nir_load_work_dim _nir_build_load_work_dim
#define nir_load_workgroup_id _nir_build_load_workgroup_id
#define nir_load_workgroup_index _nir_build_load_workgroup_index
#define nir_load_workgroup_num_input_primitives_amd _nir_build_load_workgroup_num_input_primitives_amd
#define nir_load_workgroup_num_input_vertices_amd _nir_build_load_workgroup_num_input_vertices_amd
#define nir_load_workgroup_size _nir_build_load_workgroup_size
#ifdef __cplusplus
#define nir_load_xfb_address(build, bit_size, ...) \
_nir_build_load_xfb_address(build, bit_size, _nir_load_xfb_address_indices{0, __VA_ARGS__})
#else
#define nir_load_xfb_address(build, bit_size, ...) \
_nir_build_load_xfb_address(build, bit_size, (struct _nir_load_xfb_address_indices){0, __VA_ARGS__})
#endif
#define nir_load_xfb_index_buffer _nir_build_load_xfb_index_buffer
#ifdef __cplusplus
#define nir_load_xfb_size(build, ...) \
_nir_build_load_xfb_size(build, _nir_load_xfb_size_indices{0, __VA_ARGS__})
#else
#define nir_load_xfb_size(build, ...) \
_nir_build_load_xfb_size(build, (struct _nir_load_xfb_size_indices){0, __VA_ARGS__})
#endif
#define nir_load_xfb_state_address_gfx12_amd _nir_build_load_xfb_state_address_gfx12_amd
#ifdef __cplusplus
#define nir_masked_swizzle_amd(build, src0, ...) \
_nir_build_masked_swizzle_amd(build, src0, _nir_masked_swizzle_amd_indices{0, __VA_ARGS__})
#else
#define nir_masked_swizzle_amd(build, src0, ...) \
_nir_build_masked_swizzle_amd(build, src0, (struct _nir_masked_swizzle_amd_indices){0, __VA_ARGS__})
#endif
#define nir_match_any_nv _nir_build_match_any_nv
#define nir_mbcnt_amd _nir_build_mbcnt_amd
#ifdef __cplusplus
#define nir_build_memcpy_deref(build, src0, src1, src2, ...) \
_nir_build_memcpy_deref(build, src0, src1, src2, _nir_memcpy_deref_indices{0, __VA_ARGS__})
#else
#define nir_build_memcpy_deref(build, src0, src1, src2, ...) \
_nir_build_memcpy_deref(build, src0, src1, src2, (struct _nir_memcpy_deref_indices){0, __VA_ARGS__})
#endif
#define nir_memcpy_deref nir_build_memcpy_deref
#ifdef __cplusplus
#define nir_mutex_pco(build, ...) \
_nir_build_mutex_pco(build, _nir_mutex_pco_indices{0, __VA_ARGS__})
#else
#define nir_mutex_pco(build, ...) \
_nir_build_mutex_pco(build, (struct _nir_mutex_pco_indices){0, __VA_ARGS__})
#endif
#define nir_nop _nir_build_nop
#ifdef __cplusplus
#define nir_nop_amd(build, ...) \
_nir_build_nop_amd(build, _nir_nop_amd_indices{0, __VA_ARGS__})
#else
#define nir_nop_amd(build, ...) \
_nir_build_nop_amd(build, (struct _nir_nop_amd_indices){0, __VA_ARGS__})
#endif
#define nir_optimization_barrier_sgpr_amd _nir_build_optimization_barrier_sgpr_amd
#define nir_optimization_barrier_vgpr_amd _nir_build_optimization_barrier_vgpr_amd
#define nir_ordered_add_loop_gfx12_amd _nir_build_ordered_add_loop_gfx12_amd
#ifdef __cplusplus
#define nir_ordered_xfb_counter_add_gfx11_amd(build, src0, src1, ...) \
_nir_build_ordered_xfb_counter_add_gfx11_amd(build, src0, src1, _nir_ordered_xfb_counter_add_gfx11_amd_indices{0, __VA_ARGS__})
#else
#define nir_ordered_xfb_counter_add_gfx11_amd(build, src0, src1, ...) \
_nir_build_ordered_xfb_counter_add_gfx11_amd(build, src0, src1, (struct _nir_ordered_xfb_counter_add_gfx11_amd_indices){0, __VA_ARGS__})
#endif
#define nir_overwrite_tes_arguments_amd _nir_build_overwrite_tes_arguments_amd
#define nir_overwrite_vs_arguments_amd _nir_build_overwrite_vs_arguments_amd
#ifdef __cplusplus
#define nir_pack_pco(build, src0, ...) \
_nir_build_pack_pco(build, src0, _nir_pack_pco_indices{0, __VA_ARGS__})
#else
#define nir_pack_pco(build, src0, ...) \
_nir_build_pack_pco(build, src0, (struct _nir_pack_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_parallel_copy(build, num_components, bit_size, src0, src1, ...) \
_nir_build_parallel_copy(build, num_components, bit_size, src0, src1, _nir_parallel_copy_indices{0, __VA_ARGS__})
#else
#define nir_parallel_copy(build, num_components, bit_size, src0, src1, ...) \
_nir_build_parallel_copy(build, num_components, bit_size, src0, src1, (struct _nir_parallel_copy_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_pck_prog_pco(build, src0, src1, ...) \
_nir_build_pck_prog_pco(build, src0, src1, _nir_pck_prog_pco_indices{0, __VA_ARGS__})
#else
#define nir_pck_prog_pco(build, src0, src1, ...) \
_nir_build_pck_prog_pco(build, src0, src1, (struct _nir_pck_prog_pco_indices){0, __VA_ARGS__})
#endif
#define nir_pin_cx_handle_nv _nir_build_pin_cx_handle_nv
#define nir_preamble_end_ir3 _nir_build_preamble_end_ir3
#define nir_preamble_start_ir3 _nir_build_preamble_start_ir3
#define nir_prefetch_sam_ir3 _nir_build_prefetch_sam_ir3
#define nir_prefetch_tex_ir3 _nir_build_prefetch_tex_ir3
#define nir_prefetch_ubo_ir3 _nir_build_prefetch_ubo_ir3
#ifdef __cplusplus
#define nir_printf(build, src0, ...) \
_nir_build_printf(build, src0, _nir_printf_indices{0, __VA_ARGS__})
#else
#define nir_printf(build, src0, ...) \
_nir_build_printf(build, src0, (struct _nir_printf_indices){0, __VA_ARGS__})
#endif
#define nir_printf_abort _nir_build_printf_abort
#define nir_quad_ballot_agx _nir_build_quad_ballot_agx
#define nir_quad_broadcast _nir_build_quad_broadcast
#define nir_quad_swap_diagonal _nir_build_quad_swap_diagonal
#define nir_quad_swap_horizontal _nir_build_quad_swap_horizontal
#define nir_quad_swap_vertical _nir_build_quad_swap_vertical
#ifdef __cplusplus
#define nir_quad_swizzle_amd(build, src0, ...) \
_nir_build_quad_swizzle_amd(build, src0, _nir_quad_swizzle_amd_indices{0, __VA_ARGS__})
#else
#define nir_quad_swizzle_amd(build, src0, ...) \
_nir_build_quad_swizzle_amd(build, src0, (struct _nir_quad_swizzle_amd_indices){0, __VA_ARGS__})
#endif
#define nir_quad_vote_all _nir_build_quad_vote_all
#define nir_quad_vote_any _nir_build_quad_vote_any
#define nir_r600_indirect_vertex_at_index _nir_build_r600_indirect_vertex_at_index
#define nir_ray_intersection_ir3 _nir_build_ray_intersection_ir3
#define nir_read_first_invocation _nir_build_read_first_invocation
#define nir_read_getlast_ir3 _nir_build_read_getlast_ir3
#define nir_read_invocation _nir_build_read_invocation
#define nir_read_invocation_cond_ir3 _nir_build_read_invocation_cond_ir3
#ifdef __cplusplus
#define nir_reduce(build, src0, ...) \
_nir_build_reduce(build, src0, _nir_reduce_indices{0, __VA_ARGS__})
#else
#define nir_reduce(build, src0, ...) \
_nir_build_reduce(build, src0, (struct _nir_reduce_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_reduce_clusters_ir3(build, src0, ...) \
_nir_build_reduce_clusters_ir3(build, src0, _nir_reduce_clusters_ir3_indices{0, __VA_ARGS__})
#else
#define nir_reduce_clusters_ir3(build, src0, ...) \
_nir_build_reduce_clusters_ir3(build, src0, (struct _nir_reduce_clusters_ir3_indices){0, __VA_ARGS__})
#endif
#define nir_report_ray_intersection _nir_build_report_ray_intersection
#define nir_resbase_ir3 _nir_build_resbase_ir3
#ifdef __cplusplus
#define nir_resource_intel(build, src0, src1, src2, src3, ...) \
_nir_build_resource_intel(build, src0, src1, src2, src3, _nir_resource_intel_indices{0, __VA_ARGS__})
#else
#define nir_resource_intel(build, src0, src1, src2, src3, ...) \
_nir_build_resource_intel(build, src0, src1, src2, src3, (struct _nir_resource_intel_indices){0, __VA_ARGS__})
#endif
#define nir_ro_to_rw_poly _nir_build_ro_to_rw_poly
#ifdef __cplusplus
#define nir_rotate(build, src0, src1, ...) \
_nir_build_rotate(build, src0, src1, _nir_rotate_indices{0, __VA_ARGS__})
#else
#define nir_rotate(build, src0, src1, ...) \
_nir_build_rotate(build, src0, src1, (struct _nir_rotate_indices){0, __VA_ARGS__})
#endif
#define nir_rq_confirm_intersection _nir_build_rq_confirm_intersection
#define nir_rq_generate_intersection _nir_build_rq_generate_intersection
#define nir_rq_initialize _nir_build_rq_initialize
#ifdef __cplusplus
#define nir_rq_load(build, num_components, bit_size, src0, ...) \
_nir_build_rq_load(build, num_components, bit_size, src0, _nir_rq_load_indices{0, __VA_ARGS__})
#else
#define nir_rq_load(build, num_components, bit_size, src0, ...) \
_nir_build_rq_load(build, num_components, bit_size, src0, (struct _nir_rq_load_indices){0, __VA_ARGS__})
#endif
#define nir_rq_proceed _nir_build_rq_proceed
#define nir_rq_terminate _nir_build_rq_terminate
#ifdef __cplusplus
#define nir_rt_execute_callable(build, src0, src1, ...) \
_nir_build_rt_execute_callable(build, src0, src1, _nir_rt_execute_callable_indices{0, __VA_ARGS__})
#else
#define nir_rt_execute_callable(build, src0, src1, ...) \
_nir_build_rt_execute_callable(build, src0, src1, (struct _nir_rt_execute_callable_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_rt_resume(build, ...) \
_nir_build_rt_resume(build, _nir_rt_resume_indices{0, __VA_ARGS__})
#else
#define nir_rt_resume(build, ...) \
_nir_build_rt_resume(build, (struct _nir_rt_resume_indices){0, __VA_ARGS__})
#endif
#define nir_rt_return_amd _nir_build_rt_return_amd
#ifdef __cplusplus
#define nir_rt_trace_ray(build, src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10, ...) \
_nir_build_rt_trace_ray(build, src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10, _nir_rt_trace_ray_indices{0, __VA_ARGS__})
#else
#define nir_rt_trace_ray(build, src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10, ...) \
_nir_build_rt_trace_ray(build, src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10, (struct _nir_rt_trace_ray_indices){0, __VA_ARGS__})
#endif
#define nir_sample_mask_agx _nir_build_sample_mask_agx
#ifdef __cplusplus
#define nir_select_vertex_poly(build, src0, ...) \
_nir_build_select_vertex_poly(build, src0, _nir_select_vertex_poly_indices{0, __VA_ARGS__})
#else
#define nir_select_vertex_poly(build, src0, ...) \
_nir_build_select_vertex_poly(build, src0, (struct _nir_select_vertex_poly_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_sendmsg_amd(build, src0, ...) \
_nir_build_sendmsg_amd(build, src0, _nir_sendmsg_amd_indices{0, __VA_ARGS__})
#else
#define nir_sendmsg_amd(build, src0, ...) \
_nir_build_sendmsg_amd(build, src0, (struct _nir_sendmsg_amd_indices){0, __VA_ARGS__})
#endif
#define nir_set_next_call_pc_amd _nir_build_set_next_call_pc_amd
#ifdef __cplusplus
#define nir_set_vertex_and_primitive_count(build, src0, src1, src2, ...) \
_nir_build_set_vertex_and_primitive_count(build, src0, src1, src2, _nir_set_vertex_and_primitive_count_indices{0, __VA_ARGS__})
#else
#define nir_set_vertex_and_primitive_count(build, src0, src1, src2, ...) \
_nir_build_set_vertex_and_primitive_count(build, src0, src1, src2, (struct _nir_set_vertex_and_primitive_count_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shader_clock(build, ...) \
_nir_build_shader_clock(build, _nir_shader_clock_indices{0, __VA_ARGS__})
#else
#define nir_shader_clock(build, ...) \
_nir_build_shader_clock(build, (struct _nir_shader_clock_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_append_amd(build, ...) \
_nir_build_shared_append_amd(build, _nir_shared_append_amd_indices{0, __VA_ARGS__})
#else
#define nir_shared_append_amd(build, ...) \
_nir_build_shared_append_amd(build, (struct _nir_shared_append_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_atomic(build, bit_size, src0, src1, ...) \
_nir_build_shared_atomic(build, bit_size, src0, src1, _nir_shared_atomic_indices{0, __VA_ARGS__})
#else
#define nir_shared_atomic(build, bit_size, src0, src1, ...) \
_nir_build_shared_atomic(build, bit_size, src0, src1, (struct _nir_shared_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_atomic_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_nv(build, bit_size, src0, src1, src2, _nir_shared_atomic_nv_indices{0, __VA_ARGS__})
#else
#define nir_shared_atomic_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_nv(build, bit_size, src0, src1, src2, (struct _nir_shared_atomic_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_swap(build, bit_size, src0, src1, src2, _nir_shared_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_shared_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_swap(build, bit_size, src0, src1, src2, (struct _nir_shared_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_atomic_swap_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_swap_nv(build, bit_size, src0, src1, src2, _nir_shared_atomic_swap_nv_indices{0, __VA_ARGS__})
#else
#define nir_shared_atomic_swap_nv(build, bit_size, src0, src1, src2, ...) \
_nir_build_shared_atomic_swap_nv(build, bit_size, src0, src1, src2, (struct _nir_shared_atomic_swap_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_shared_consume_amd(build, ...) \
_nir_build_shared_consume_amd(build, _nir_shared_consume_amd_indices{0, __VA_ARGS__})
#else
#define nir_shared_consume_amd(build, ...) \
_nir_build_shared_consume_amd(build, (struct _nir_shared_consume_amd_indices){0, __VA_ARGS__})
#endif
#define nir_shuffle _nir_build_shuffle
#define nir_shuffle_down _nir_build_shuffle_down
#define nir_shuffle_down_intel _nir_build_shuffle_down_intel
#define nir_shuffle_down_uniform_ir3 _nir_build_shuffle_down_uniform_ir3
#define nir_shuffle_intel _nir_build_shuffle_intel
#define nir_shuffle_up _nir_build_shuffle_up
#define nir_shuffle_up_intel _nir_build_shuffle_up_intel
#define nir_shuffle_up_uniform_ir3 _nir_build_shuffle_up_uniform_ir3
#define nir_shuffle_xor _nir_build_shuffle_xor
#define nir_shuffle_xor_uniform_ir3 _nir_build_shuffle_xor_uniform_ir3
#ifdef __cplusplus
#define nir_sleep_amd(build, ...) \
_nir_build_sleep_amd(build, _nir_sleep_amd_indices{0, __VA_ARGS__})
#else
#define nir_sleep_amd(build, ...) \
_nir_build_sleep_amd(build, (struct _nir_sleep_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_smp_coeffs_pco(build, src0, src1, src2, ...) \
_nir_build_smp_coeffs_pco(build, src0, src1, src2, _nir_smp_coeffs_pco_indices{0, __VA_ARGS__})
#else
#define nir_smp_coeffs_pco(build, src0, src1, src2, ...) \
_nir_build_smp_coeffs_pco(build, src0, src1, src2, (struct _nir_smp_coeffs_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_smp_pco(build, num_components, src0, src1, src2, ...) \
_nir_build_smp_pco(build, num_components, src0, src1, src2, _nir_smp_pco_indices{0, __VA_ARGS__})
#else
#define nir_smp_pco(build, num_components, src0, src1, src2, ...) \
_nir_build_smp_pco(build, num_components, src0, src1, src2, (struct _nir_smp_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_smp_raw_pco(build, src0, src1, src2, ...) \
_nir_build_smp_raw_pco(build, src0, src1, src2, _nir_smp_raw_pco_indices{0, __VA_ARGS__})
#else
#define nir_smp_raw_pco(build, src0, src1, src2, ...) \
_nir_build_smp_raw_pco(build, src0, src1, src2, (struct _nir_smp_raw_pco_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_smp_write_pco(build, src0, src1, src2, ...) \
_nir_build_smp_write_pco(build, src0, src1, src2, _nir_smp_write_pco_indices{0, __VA_ARGS__})
#else
#define nir_smp_write_pco(build, src0, src1, src2, ...) \
_nir_build_smp_write_pco(build, src0, src1, src2, (struct _nir_smp_write_pco_indices){0, __VA_ARGS__})
#endif
#define nir_sparse_residency_code_and _nir_build_sparse_residency_code_and
#define nir_ssa_bar_nv _nir_build_ssa_bar_nv
#ifdef __cplusplus
#define nir_ssbo_atomic(build, bit_size, src0, src1, src2, ...) \
_nir_build_ssbo_atomic(build, bit_size, src0, src1, src2, _nir_ssbo_atomic_indices{0, __VA_ARGS__})
#else
#define nir_ssbo_atomic(build, bit_size, src0, src1, src2, ...) \
_nir_build_ssbo_atomic(build, bit_size, src0, src1, src2, (struct _nir_ssbo_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ssbo_atomic_ir3(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_ssbo_atomic_ir3(build, bit_size, src0, src1, src2, src3, _nir_ssbo_atomic_ir3_indices{0, __VA_ARGS__})
#else
#define nir_ssbo_atomic_ir3(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_ssbo_atomic_ir3(build, bit_size, src0, src1, src2, src3, (struct _nir_ssbo_atomic_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ssbo_atomic_swap(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_ssbo_atomic_swap(build, bit_size, src0, src1, src2, src3, _nir_ssbo_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_ssbo_atomic_swap(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_ssbo_atomic_swap(build, bit_size, src0, src1, src2, src3, (struct _nir_ssbo_atomic_swap_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ssbo_atomic_swap_ir3(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_ssbo_atomic_swap_ir3(build, bit_size, src0, src1, src2, src3, src4, _nir_ssbo_atomic_swap_ir3_indices{0, __VA_ARGS__})
#else
#define nir_ssbo_atomic_swap_ir3(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_ssbo_atomic_swap_ir3(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_ssbo_atomic_swap_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_ssbo_descriptor_amd(build, src0, ...) \
_nir_build_ssbo_descriptor_amd(build, src0, _nir_ssbo_descriptor_amd_indices{0, __VA_ARGS__})
#else
#define nir_ssbo_descriptor_amd(build, src0, ...) \
_nir_build_ssbo_descriptor_amd(build, src0, (struct _nir_ssbo_descriptor_amd_indices){0, __VA_ARGS__})
#endif
#define nir_stack_map_agx _nir_build_stack_map_agx
#define nir_stack_unmap_agx _nir_build_stack_unmap_agx
#ifdef __cplusplus
#define nir_store_agx(build, src0, src1, src2, ...) \
_nir_build_store_agx(build, src0, src1, src2, _nir_store_agx_indices{0, __VA_ARGS__})
#else
#define nir_store_agx(build, src0, src1, src2, ...) \
_nir_build_store_agx(build, src0, src1, src2, (struct _nir_store_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_buffer_amd(build, src0, src1, src2, src3, src4, ...) \
_nir_build_store_buffer_amd(build, src0, src1, src2, src3, src4, _nir_store_buffer_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_buffer_amd(build, src0, src1, src2, src3, src4, ...) \
_nir_build_store_buffer_amd(build, src0, src1, src2, src3, src4, (struct _nir_store_buffer_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_clip_distance_kk(build, src0, ...) \
_nir_build_store_clip_distance_kk(build, src0, _nir_store_clip_distance_kk_indices{0, __VA_ARGS__})
#else
#define nir_store_clip_distance_kk(build, src0, ...) \
_nir_build_store_clip_distance_kk(build, src0, (struct _nir_store_clip_distance_kk_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_combined_output_pan(build, src0, src1, src2, src3, src4, ...) \
_nir_build_store_combined_output_pan(build, src0, src1, src2, src3, src4, _nir_store_combined_output_pan_indices{0, __VA_ARGS__})
#else
#define nir_store_combined_output_pan(build, src0, src1, src2, src3, src4, ...) \
_nir_build_store_combined_output_pan(build, src0, src1, src2, src3, src4, (struct _nir_store_combined_output_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_const_ir3(build, src0, ...) \
_nir_build_store_const_ir3(build, src0, _nir_store_const_ir3_indices{0, __VA_ARGS__})
#else
#define nir_store_const_ir3(build, src0, ...) \
_nir_build_store_const_ir3(build, src0, (struct _nir_store_const_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_store_deref(build, src0, src1, ...) \
_nir_build_store_deref(build, src0, src1, _nir_store_deref_indices{0, __VA_ARGS__})
#else
#define nir_build_store_deref(build, src0, src1, ...) \
_nir_build_store_deref(build, src0, src1, (struct _nir_store_deref_indices){0, __VA_ARGS__})
#endif
#define nir_store_deref nir_build_store_deref
#ifdef __cplusplus
#define nir_store_deref_block_intel(build, src0, src1, ...) \
_nir_build_store_deref_block_intel(build, src0, src1, _nir_store_deref_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_deref_block_intel(build, src0, src1, ...) \
_nir_build_store_deref_block_intel(build, src0, src1, (struct _nir_store_deref_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_store_global(build, src0, src1, ...) \
_nir_build_store_global(build, src0, src1, _nir_store_global_indices{0, __VA_ARGS__})
#else
#define nir_build_store_global(build, src0, src1, ...) \
_nir_build_store_global(build, src0, src1, (struct _nir_store_global_indices){0, __VA_ARGS__})
#endif
#define nir_store_global nir_build_store_global
#ifdef __cplusplus
#define nir_store_global_2x32(build, src0, src1, ...) \
_nir_build_store_global_2x32(build, src0, src1, _nir_store_global_2x32_indices{0, __VA_ARGS__})
#else
#define nir_store_global_2x32(build, src0, src1, ...) \
_nir_build_store_global_2x32(build, src0, src1, (struct _nir_store_global_2x32_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_amd(build, src0, src1, src2, ...) \
_nir_build_store_global_amd(build, src0, src1, src2, _nir_store_global_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_global_amd(build, src0, src1, src2, ...) \
_nir_build_store_global_amd(build, src0, src1, src2, (struct _nir_store_global_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_block_intel(build, src0, src1, ...) \
_nir_build_store_global_block_intel(build, src0, src1, _nir_store_global_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_global_block_intel(build, src0, src1, ...) \
_nir_build_store_global_block_intel(build, src0, src1, (struct _nir_store_global_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_cvt_pan(build, src0, src1, src2, ...) \
_nir_build_store_global_cvt_pan(build, src0, src1, src2, _nir_store_global_cvt_pan_indices{0, __VA_ARGS__})
#else
#define nir_store_global_cvt_pan(build, src0, src1, src2, ...) \
_nir_build_store_global_cvt_pan(build, src0, src1, src2, (struct _nir_store_global_cvt_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_etna(build, src0, src1, src2, ...) \
_nir_build_store_global_etna(build, src0, src1, src2, _nir_store_global_etna_indices{0, __VA_ARGS__})
#else
#define nir_store_global_etna(build, src0, src1, src2, ...) \
_nir_build_store_global_etna(build, src0, src1, src2, (struct _nir_store_global_etna_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_intel(build, src0, src1, ...) \
_nir_build_store_global_intel(build, src0, src1, _nir_store_global_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_global_intel(build, src0, src1, ...) \
_nir_build_store_global_intel(build, src0, src1, (struct _nir_store_global_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_nv(build, src0, src1, src2, ...) \
_nir_build_store_global_nv(build, src0, src1, src2, _nir_store_global_nv_indices{0, __VA_ARGS__})
#else
#define nir_store_global_nv(build, src0, src1, src2, ...) \
_nir_build_store_global_nv(build, src0, src1, src2, (struct _nir_store_global_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_offset(build, src0, src1, src2, ...) \
_nir_build_store_global_offset(build, src0, src1, src2, _nir_store_global_offset_indices{0, __VA_ARGS__})
#else
#define nir_store_global_offset(build, src0, src1, src2, ...) \
_nir_build_store_global_offset(build, src0, src1, src2, (struct _nir_store_global_offset_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_global_psiz_pan(build, src0, src1, ...) \
_nir_build_store_global_psiz_pan(build, src0, src1, _nir_store_global_psiz_pan_indices{0, __VA_ARGS__})
#else
#define nir_store_global_psiz_pan(build, src0, src1, ...) \
_nir_build_store_global_psiz_pan(build, src0, src1, (struct _nir_store_global_psiz_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_hit_attrib_amd(build, src0, ...) \
_nir_build_store_hit_attrib_amd(build, src0, _nir_store_hit_attrib_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_hit_attrib_amd(build, src0, ...) \
_nir_build_store_hit_attrib_amd(build, src0, (struct _nir_store_hit_attrib_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_incoming_ray_payload_amd(build, src0, ...) \
_nir_build_store_incoming_ray_payload_amd(build, src0, _nir_store_incoming_ray_payload_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_incoming_ray_payload_amd(build, src0, ...) \
_nir_build_store_incoming_ray_payload_amd(build, src0, (struct _nir_store_incoming_ray_payload_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_local_pixel_agx(build, src0, src1, src2, ...) \
_nir_build_store_local_pixel_agx(build, src0, src1, src2, _nir_store_local_pixel_agx_indices{0, __VA_ARGS__})
#else
#define nir_store_local_pixel_agx(build, src0, src1, src2, ...) \
_nir_build_store_local_pixel_agx(build, src0, src1, src2, (struct _nir_store_local_pixel_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_local_shared_r600(build, src0, src1, ...) \
_nir_build_store_local_shared_r600(build, src0, src1, _nir_store_local_shared_r600_indices{0, __VA_ARGS__})
#else
#define nir_store_local_shared_r600(build, src0, src1, ...) \
_nir_build_store_local_shared_r600(build, src0, src1, (struct _nir_store_local_shared_r600_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_outgoing_ray_payload_amd(build, src0, ...) \
_nir_build_store_outgoing_ray_payload_amd(build, src0, _nir_store_outgoing_ray_payload_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_outgoing_ray_payload_amd(build, src0, ...) \
_nir_build_store_outgoing_ray_payload_amd(build, src0, (struct _nir_store_outgoing_ray_payload_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_output(build, src0, src1, ...) \
_nir_build_store_output(build, src0, src1, _nir_store_output_indices{0, __VA_ARGS__})
#else
#define nir_store_output(build, src0, src1, ...) \
_nir_build_store_output(build, src0, src1, (struct _nir_store_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_param_amd(build, src0, ...) \
_nir_build_store_param_amd(build, src0, _nir_store_param_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_param_amd(build, src0, ...) \
_nir_build_store_param_amd(build, src0, (struct _nir_store_param_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_per_primitive_output(build, src0, src1, src2, ...) \
_nir_build_store_per_primitive_output(build, src0, src1, src2, _nir_store_per_primitive_output_indices{0, __VA_ARGS__})
#else
#define nir_store_per_primitive_output(build, src0, src1, src2, ...) \
_nir_build_store_per_primitive_output(build, src0, src1, src2, (struct _nir_store_per_primitive_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_per_primitive_payload_intel(build, src0, ...) \
_nir_build_store_per_primitive_payload_intel(build, src0, _nir_store_per_primitive_payload_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_per_primitive_payload_intel(build, src0, ...) \
_nir_build_store_per_primitive_payload_intel(build, src0, (struct _nir_store_per_primitive_payload_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_per_vertex_output(build, src0, src1, src2, ...) \
_nir_build_store_per_vertex_output(build, src0, src1, src2, _nir_store_per_vertex_output_indices{0, __VA_ARGS__})
#else
#define nir_store_per_vertex_output(build, src0, src1, src2, ...) \
_nir_build_store_per_vertex_output(build, src0, src1, src2, (struct _nir_store_per_vertex_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_per_view_output(build, src0, src1, src2, ...) \
_nir_build_store_per_view_output(build, src0, src1, src2, _nir_store_per_view_output_indices{0, __VA_ARGS__})
#else
#define nir_store_per_view_output(build, src0, src1, src2, ...) \
_nir_build_store_per_view_output(build, src0, src1, src2, (struct _nir_store_per_view_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_pixel_local(build, src0, src1, ...) \
_nir_build_store_pixel_local(build, src0, src1, _nir_store_pixel_local_indices{0, __VA_ARGS__})
#else
#define nir_store_pixel_local(build, src0, src1, ...) \
_nir_build_store_pixel_local(build, src0, src1, (struct _nir_store_pixel_local_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_preamble(build, src0, ...) \
_nir_build_store_preamble(build, src0, _nir_store_preamble_indices{0, __VA_ARGS__})
#else
#define nir_store_preamble(build, src0, ...) \
_nir_build_store_preamble(build, src0, (struct _nir_store_preamble_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_preamble_dynamic(build, src0, src1, ...) \
_nir_build_store_preamble_dynamic(build, src0, src1, _nir_store_preamble_dynamic_indices{0, __VA_ARGS__})
#else
#define nir_store_preamble_dynamic(build, src0, src1, ...) \
_nir_build_store_preamble_dynamic(build, src0, src1, (struct _nir_store_preamble_dynamic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_raw_output_pan(build, src0, ...) \
_nir_build_store_raw_output_pan(build, src0, _nir_store_raw_output_pan_indices{0, __VA_ARGS__})
#else
#define nir_store_raw_output_pan(build, src0, ...) \
_nir_build_store_raw_output_pan(build, src0, (struct _nir_store_raw_output_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_build_store_reg(build, src0, src1, ...) \
_nir_build_store_reg(build, src0, src1, _nir_store_reg_indices{0, __VA_ARGS__})
#else
#define nir_build_store_reg(build, src0, src1, ...) \
_nir_build_store_reg(build, src0, src1, (struct _nir_store_reg_indices){0, __VA_ARGS__})
#endif
#define nir_store_reg nir_build_store_reg
#ifdef __cplusplus
#define nir_store_reg_indirect(build, src0, src1, src2, ...) \
_nir_build_store_reg_indirect(build, src0, src1, src2, _nir_store_reg_indirect_indices{0, __VA_ARGS__})
#else
#define nir_store_reg_indirect(build, src0, src1, src2, ...) \
_nir_build_store_reg_indirect(build, src0, src1, src2, (struct _nir_store_reg_indirect_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_render_target_intel(build, src0, src1, src2, src3, src4, src5, ...) \
_nir_build_store_render_target_intel(build, src0, src1, src2, src3, src4, src5, _nir_store_render_target_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_render_target_intel(build, src0, src1, src2, src3, src4, src5, ...) \
_nir_build_store_render_target_intel(build, src0, src1, src2, src3, src4, src5, (struct _nir_store_render_target_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_scalar_arg_amd(build, src0, ...) \
_nir_build_store_scalar_arg_amd(build, src0, _nir_store_scalar_arg_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_scalar_arg_amd(build, src0, ...) \
_nir_build_store_scalar_arg_amd(build, src0, (struct _nir_store_scalar_arg_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_scratch(build, src0, src1, ...) \
_nir_build_store_scratch(build, src0, src1, _nir_store_scratch_indices{0, __VA_ARGS__})
#else
#define nir_store_scratch(build, src0, src1, ...) \
_nir_build_store_scratch(build, src0, src1, (struct _nir_store_scratch_indices){0, __VA_ARGS__})
#endif
#define nir_store_scratch_intel _nir_build_store_scratch_intel
#ifdef __cplusplus
#define nir_store_scratch_nv(build, src0, src1, src2, ...) \
_nir_build_store_scratch_nv(build, src0, src1, src2, _nir_store_scratch_nv_indices{0, __VA_ARGS__})
#else
#define nir_store_scratch_nv(build, src0, src1, src2, ...) \
_nir_build_store_scratch_nv(build, src0, src1, src2, (struct _nir_store_scratch_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared(build, src0, src1, ...) \
_nir_build_store_shared(build, src0, src1, _nir_store_shared_indices{0, __VA_ARGS__})
#else
#define nir_store_shared(build, src0, src1, ...) \
_nir_build_store_shared(build, src0, src1, (struct _nir_store_shared_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared2_amd(build, src0, src1, ...) \
_nir_build_store_shared2_amd(build, src0, src1, _nir_store_shared2_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_shared2_amd(build, src0, src1, ...) \
_nir_build_store_shared2_amd(build, src0, src1, (struct _nir_store_shared2_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared_block_intel(build, src0, src1, ...) \
_nir_build_store_shared_block_intel(build, src0, src1, _nir_store_shared_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_shared_block_intel(build, src0, src1, ...) \
_nir_build_store_shared_block_intel(build, src0, src1, (struct _nir_store_shared_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared_ir3(build, src0, src1, ...) \
_nir_build_store_shared_ir3(build, src0, src1, _nir_store_shared_ir3_indices{0, __VA_ARGS__})
#else
#define nir_store_shared_ir3(build, src0, src1, ...) \
_nir_build_store_shared_ir3(build, src0, src1, (struct _nir_store_shared_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared_nv(build, src0, src1, src2, ...) \
_nir_build_store_shared_nv(build, src0, src1, src2, _nir_store_shared_nv_indices{0, __VA_ARGS__})
#else
#define nir_store_shared_nv(build, src0, src1, src2, ...) \
_nir_build_store_shared_nv(build, src0, src1, src2, (struct _nir_store_shared_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_shared_unlock_nv(build, bit_size, src0, src1, ...) \
_nir_build_store_shared_unlock_nv(build, bit_size, src0, src1, _nir_store_shared_unlock_nv_indices{0, __VA_ARGS__})
#else
#define nir_store_shared_unlock_nv(build, bit_size, src0, src1, ...) \
_nir_build_store_shared_unlock_nv(build, bit_size, src0, src1, (struct _nir_store_shared_unlock_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_ssbo(build, src0, src1, src2, ...) \
_nir_build_store_ssbo(build, src0, src1, src2, _nir_store_ssbo_indices{0, __VA_ARGS__})
#else
#define nir_store_ssbo(build, src0, src1, src2, ...) \
_nir_build_store_ssbo(build, src0, src1, src2, (struct _nir_store_ssbo_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_ssbo_block_intel(build, src0, src1, src2, ...) \
_nir_build_store_ssbo_block_intel(build, src0, src1, src2, _nir_store_ssbo_block_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_ssbo_block_intel(build, src0, src1, src2, ...) \
_nir_build_store_ssbo_block_intel(build, src0, src1, src2, (struct _nir_store_ssbo_block_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_ssbo_intel(build, src0, src1, src2, ...) \
_nir_build_store_ssbo_intel(build, src0, src1, src2, _nir_store_ssbo_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_ssbo_intel(build, src0, src1, src2, ...) \
_nir_build_store_ssbo_intel(build, src0, src1, src2, (struct _nir_store_ssbo_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_ssbo_ir3(build, src0, src1, src2, src3, ...) \
_nir_build_store_ssbo_ir3(build, src0, src1, src2, src3, _nir_store_ssbo_ir3_indices{0, __VA_ARGS__})
#else
#define nir_store_ssbo_ir3(build, src0, src1, src2, src3, ...) \
_nir_build_store_ssbo_ir3(build, src0, src1, src2, src3, (struct _nir_store_ssbo_ir3_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_stack(build, src0, ...) \
_nir_build_store_stack(build, src0, _nir_store_stack_indices{0, __VA_ARGS__})
#else
#define nir_store_stack(build, src0, ...) \
_nir_build_store_stack(build, src0, (struct _nir_store_stack_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_task_payload(build, src0, src1, ...) \
_nir_build_store_task_payload(build, src0, src1, _nir_store_task_payload_indices{0, __VA_ARGS__})
#else
#define nir_store_task_payload(build, src0, src1, ...) \
_nir_build_store_task_payload(build, src0, src1, (struct _nir_store_task_payload_indices){0, __VA_ARGS__})
#endif
#define nir_store_tf_r600 _nir_build_store_tf_r600
#ifdef __cplusplus
#define nir_store_tile_pan(build, src0, src1, src2, src3, ...) \
_nir_build_store_tile_pan(build, src0, src1, src2, src3, _nir_store_tile_pan_indices{0, __VA_ARGS__})
#else
#define nir_store_tile_pan(build, src0, src1, src2, src3, ...) \
_nir_build_store_tile_pan(build, src0, src1, src2, src3, (struct _nir_store_tile_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_tlb_sample_color_v3d(build, src0, src1, ...) \
_nir_build_store_tlb_sample_color_v3d(build, src0, src1, _nir_store_tlb_sample_color_v3d_indices{0, __VA_ARGS__})
#else
#define nir_store_tlb_sample_color_v3d(build, src0, src1, ...) \
_nir_build_store_tlb_sample_color_v3d(build, src0, src1, (struct _nir_store_tlb_sample_color_v3d_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_urb_lsc_intel(build, src0, src1, ...) \
_nir_build_store_urb_lsc_intel(build, src0, src1, _nir_store_urb_lsc_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_urb_lsc_intel(build, src0, src1, ...) \
_nir_build_store_urb_lsc_intel(build, src0, src1, (struct _nir_store_urb_lsc_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_urb_vec4_intel(build, src0, src1, src2, src3, ...) \
_nir_build_store_urb_vec4_intel(build, src0, src1, src2, src3, _nir_store_urb_vec4_intel_indices{0, __VA_ARGS__})
#else
#define nir_store_urb_vec4_intel(build, src0, src1, src2, src3, ...) \
_nir_build_store_urb_vec4_intel(build, src0, src1, src2, src3, (struct _nir_store_urb_vec4_intel_indices){0, __VA_ARGS__})
#endif
#define nir_store_uvs_agx _nir_build_store_uvs_agx
#ifdef __cplusplus
#define nir_store_vector_arg_amd(build, src0, ...) \
_nir_build_store_vector_arg_amd(build, src0, _nir_store_vector_arg_amd_indices{0, __VA_ARGS__})
#else
#define nir_store_vector_arg_amd(build, src0, ...) \
_nir_build_store_vector_arg_amd(build, src0, (struct _nir_store_vector_arg_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_store_zs_agx(build, src0, src1, src2, ...) \
_nir_build_store_zs_agx(build, src0, src1, src2, _nir_store_zs_agx_indices{0, __VA_ARGS__})
#else
#define nir_store_zs_agx(build, src0, src1, src2, ...) \
_nir_build_store_zs_agx(build, src0, src1, src2, (struct _nir_store_zs_agx_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_strict_wqm_coord_amd(build, src0, ...) \
_nir_build_strict_wqm_coord_amd(build, src0, _nir_strict_wqm_coord_amd_indices{0, __VA_ARGS__})
#else
#define nir_strict_wqm_coord_amd(build, src0, ...) \
_nir_build_strict_wqm_coord_amd(build, src0, (struct _nir_strict_wqm_coord_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_subfm_nv(build, src0, src1, src2, ...) \
_nir_build_subfm_nv(build, src0, src1, src2, _nir_subfm_nv_indices{0, __VA_ARGS__})
#else
#define nir_subfm_nv(build, src0, src1, src2, ...) \
_nir_build_subfm_nv(build, src0, src1, src2, (struct _nir_subfm_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_suclamp_nv(build, src0, src1, ...) \
_nir_build_suclamp_nv(build, src0, src1, _nir_suclamp_nv_indices{0, __VA_ARGS__})
#else
#define nir_suclamp_nv(build, src0, src1, ...) \
_nir_build_suclamp_nv(build, src0, src1, (struct _nir_suclamp_nv_indices){0, __VA_ARGS__})
#endif
#define nir_sueau_nv _nir_build_sueau_nv
#ifdef __cplusplus
#define nir_suldga_nv(build, num_components, src0, src1, src2, ...) \
_nir_build_suldga_nv(build, num_components, src0, src1, src2, _nir_suldga_nv_indices{0, __VA_ARGS__})
#else
#define nir_suldga_nv(build, num_components, src0, src1, src2, ...) \
_nir_build_suldga_nv(build, num_components, src0, src1, src2, (struct _nir_suldga_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_sustga_nv(build, src0, src1, src2, src3, ...) \
_nir_build_sustga_nv(build, src0, src1, src2, src3, _nir_sustga_nv_indices{0, __VA_ARGS__})
#else
#define nir_sustga_nv(build, src0, src1, src2, src3, ...) \
_nir_build_sustga_nv(build, src0, src1, src2, src3, (struct _nir_sustga_nv_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_task_payload_atomic(build, bit_size, src0, src1, ...) \
_nir_build_task_payload_atomic(build, bit_size, src0, src1, _nir_task_payload_atomic_indices{0, __VA_ARGS__})
#else
#define nir_task_payload_atomic(build, bit_size, src0, src1, ...) \
_nir_build_task_payload_atomic(build, bit_size, src0, src1, (struct _nir_task_payload_atomic_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_task_payload_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_task_payload_atomic_swap(build, bit_size, src0, src1, src2, _nir_task_payload_atomic_swap_indices{0, __VA_ARGS__})
#else
#define nir_task_payload_atomic_swap(build, bit_size, src0, src1, src2, ...) \
_nir_build_task_payload_atomic_swap(build, bit_size, src0, src1, src2, (struct _nir_task_payload_atomic_swap_indices){0, __VA_ARGS__})
#endif
#define nir_terminate _nir_build_terminate
#define nir_terminate_if _nir_build_terminate_if
#define nir_terminate_ray _nir_build_terminate_ray
#ifdef __cplusplus
#define nir_test_fs_config_intel(build, bit_size, ...) \
_nir_build_test_fs_config_intel(build, bit_size, _nir_test_fs_config_intel_indices{0, __VA_ARGS__})
#else
#define nir_test_fs_config_intel(build, bit_size, ...) \
_nir_build_test_fs_config_intel(build, bit_size, (struct _nir_test_fs_config_intel_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_texc0_pan(build, bit_size, src0, src1, src2, ...) \
_nir_build_texc0_pan(build, bit_size, src0, src1, src2, _nir_texc0_pan_indices{0, __VA_ARGS__})
#else
#define nir_texc0_pan(build, bit_size, src0, src1, src2, ...) \
_nir_build_texc0_pan(build, bit_size, src0, src1, src2, (struct _nir_texc0_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_texc1_pan(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_texc1_pan(build, bit_size, src0, src1, src2, src3, _nir_texc1_pan_indices{0, __VA_ARGS__})
#else
#define nir_texc1_pan(build, bit_size, src0, src1, src2, src3, ...) \
_nir_build_texc1_pan(build, bit_size, src0, src1, src2, src3, (struct _nir_texc1_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_texc2_pan(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_texc2_pan(build, bit_size, src0, src1, src2, src3, src4, _nir_texc2_pan_indices{0, __VA_ARGS__})
#else
#define nir_texc2_pan(build, bit_size, src0, src1, src2, src3, src4, ...) \
_nir_build_texc2_pan(build, bit_size, src0, src1, src2, src3, src4, (struct _nir_texc2_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_texs_2d_pan(build, bit_size, src0, src1, ...) \
_nir_build_texs_2d_pan(build, bit_size, src0, src1, _nir_texs_2d_pan_indices{0, __VA_ARGS__})
#else
#define nir_texs_2d_pan(build, bit_size, src0, src1, ...) \
_nir_build_texs_2d_pan(build, bit_size, src0, src1, (struct _nir_texs_2d_pan_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_texs_cube_pan(build, bit_size, src0, src1, src2, ...) \
_nir_build_texs_cube_pan(build, bit_size, src0, src1, src2, _nir_texs_cube_pan_indices{0, __VA_ARGS__})
#else
#define nir_texs_cube_pan(build, bit_size, src0, src1, src2, ...) \
_nir_build_texs_cube_pan(build, bit_size, src0, src1, src2, (struct _nir_texs_cube_pan_indices){0, __VA_ARGS__})
#endif
#define nir_trace_ray _nir_build_trace_ray
#ifdef __cplusplus
#define nir_trace_ray_intel(build, src0, src1, ...) \
_nir_build_trace_ray_intel(build, src0, src1, _nir_trace_ray_intel_indices{0, __VA_ARGS__})
#else
#define nir_trace_ray_intel(build, src0, src1, ...) \
_nir_build_trace_ray_intel(build, src0, src1, (struct _nir_trace_ray_intel_indices){0, __VA_ARGS__})
#endif
#define nir_unit_test_assert_eq _nir_build_unit_test_assert_eq
#ifdef __cplusplus
#define nir_unit_test_divergent_input(build, num_components, bit_size, ...) \
_nir_build_unit_test_divergent_input(build, num_components, bit_size, _nir_unit_test_divergent_input_indices{0, __VA_ARGS__})
#else
#define nir_unit_test_divergent_input(build, num_components, bit_size, ...) \
_nir_build_unit_test_divergent_input(build, num_components, bit_size, (struct _nir_unit_test_divergent_input_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_unit_test_output(build, src0, ...) \
_nir_build_unit_test_output(build, src0, _nir_unit_test_output_indices{0, __VA_ARGS__})
#else
#define nir_unit_test_output(build, src0, ...) \
_nir_build_unit_test_output(build, src0, (struct _nir_unit_test_output_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_unit_test_uniform_input(build, num_components, bit_size, ...) \
_nir_build_unit_test_uniform_input(build, num_components, bit_size, _nir_unit_test_uniform_input_indices{0, __VA_ARGS__})
#else
#define nir_unit_test_uniform_input(build, num_components, bit_size, ...) \
_nir_build_unit_test_uniform_input(build, num_components, bit_size, (struct _nir_unit_test_uniform_input_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_unpack_pco(build, num_components, src0, ...) \
_nir_build_unpack_pco(build, num_components, src0, _nir_unpack_pco_indices{0, __VA_ARGS__})
#else
#define nir_unpack_pco(build, num_components, src0, ...) \
_nir_build_unpack_pco(build, num_components, src0, (struct _nir_unpack_pco_indices){0, __VA_ARGS__})
#endif
#define nir_unpin_cx_handle_nv _nir_build_unpin_cx_handle_nv
#define nir_use _nir_build_use
#define nir_uvsw_write_pco _nir_build_uvsw_write_pco
#ifdef __cplusplus
#define nir_vild_nv(build, src0, ...) \
_nir_build_vild_nv(build, src0, _nir_vild_nv_indices{0, __VA_ARGS__})
#else
#define nir_vild_nv(build, src0, ...) \
_nir_build_vild_nv(build, src0, (struct _nir_vild_nv_indices){0, __VA_ARGS__})
#endif
#define nir_vote_all _nir_build_vote_all
#define nir_vote_any _nir_build_vote_any
#define nir_vote_feq _nir_build_vote_feq
#define nir_vote_ieq _nir_build_vote_ieq
#ifdef __cplusplus
#define nir_vulkan_resource_index(build, num_components, bit_size, src0, ...) \
_nir_build_vulkan_resource_index(build, num_components, bit_size, src0, _nir_vulkan_resource_index_indices{0, __VA_ARGS__})
#else
#define nir_vulkan_resource_index(build, num_components, bit_size, src0, ...) \
_nir_build_vulkan_resource_index(build, num_components, bit_size, src0, (struct _nir_vulkan_resource_index_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_vulkan_resource_reindex(build, bit_size, src0, src1, ...) \
_nir_build_vulkan_resource_reindex(build, bit_size, src0, src1, _nir_vulkan_resource_reindex_indices{0, __VA_ARGS__})
#else
#define nir_vulkan_resource_reindex(build, bit_size, src0, src1, ...) \
_nir_build_vulkan_resource_reindex(build, bit_size, src0, src1, (struct _nir_vulkan_resource_reindex_indices){0, __VA_ARGS__})
#endif
#define nir_wop_pco _nir_build_wop_pco
#define nir_write_invocation_amd _nir_build_write_invocation_amd
#ifdef __cplusplus
#define nir_xfb_counter_sub_gfx11_amd(build, src0, ...) \
_nir_build_xfb_counter_sub_gfx11_amd(build, src0, _nir_xfb_counter_sub_gfx11_amd_indices{0, __VA_ARGS__})
#else
#define nir_xfb_counter_sub_gfx11_amd(build, src0, ...) \
_nir_build_xfb_counter_sub_gfx11_amd(build, src0, (struct _nir_xfb_counter_sub_gfx11_amd_indices){0, __VA_ARGS__})
#endif
#ifdef __cplusplus
#define nir_zs_emit_pan(build, src0, src1, src2, ...) \
_nir_build_zs_emit_pan(build, src0, src1, src2, _nir_zs_emit_pan_indices{0, __VA_ARGS__})
#else
#define nir_zs_emit_pan(build, src0, src1, src2, ...) \
_nir_build_zs_emit_pan(build, src0, src1, src2, (struct _nir_zs_emit_pan_indices){0, __VA_ARGS__})
#endif

static inline nir_def *
nir_flt_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_flt(build, src1, nir_imm_floatN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_fge_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_fge(build, src1, nir_imm_floatN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_feq_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_feq(build, src1, nir_imm_floatN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_fneu_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_fneu(build, src1, nir_imm_floatN_t(build, src2, src1->bit_size));
}

static inline nir_def *
nir_ilt_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ilt(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_ige_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ige(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_ieq_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ieq(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_ine_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ine(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_ult_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ult(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}
static inline nir_def *
nir_uge_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_uge(build, src1, nir_imm_intN_t(build, src2, src1->bit_size));
}

static inline nir_def *
nir_igt_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ilt(build, nir_imm_intN_t(build, src2, src1->bit_size), src1);
}

static inline nir_def *
nir_ile_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ige(build, nir_imm_intN_t(build, src2, src1->bit_size), src1);
}
static inline nir_def *
nir_ugt_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_ult(build, nir_imm_intN_t(build, src2, src1->bit_size), src1);
}

static inline nir_def *
nir_ule_imm(nir_builder *build, nir_def *src1, uint64_t src2)
{
   return nir_uge(build, nir_imm_intN_t(build, src2, src1->bit_size), src1);
}

#endif /* _NIR_BUILDER_OPCODES_ */
