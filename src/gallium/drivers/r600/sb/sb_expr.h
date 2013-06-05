/*
 * Copyright 2013 Vadim Girlin <vadimgirlin@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Vadim Girlin
 */

#ifndef SB_EXPR_H_
#define SB_EXPR_H_

namespace r600_sb {

inline float float_clamp(float v) {
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

value* get_select_value_for_em(shader &sh, value *em);

void convert_predset_to_set(shader &sh, alu_node *a);
unsigned invert_setcc_condition(unsigned cc, bool &swap_args);
unsigned get_setcc_op(unsigned cc, unsigned cmp_type, bool int_dst);
unsigned get_predsetcc_op(unsigned cc, unsigned cmp_type);
unsigned get_killcc_op(unsigned cc, unsigned cmp_type);
unsigned get_cndcc_op(unsigned cc, unsigned cmp_type);

void convert_to_mov(alu_node &n, value *src,
                    bool neg = false, bool abs = false);

class expr_handler {

   shader &sh;
   value_table &vt;

public:

   expr_handler(shader &sh);

   bool equal(value *l, value *r);
   bool defs_equal(value *l, value *r);
   bool args_equal(const vvec &l, const vvec &r);
   bool ops_equal(const alu_node *l, const alu_node *r);
   bool ivars_equal(value *l, value *r);

   value* get_const(const literal &l);

   bool try_fold(value *v);
   bool try_fold(node *n);

   bool fold(node &n);
   bool fold(container_node &n);
   bool fold(alu_node &n);
   bool fold(fetch_node &n);
   bool fold(cf_node &n);

   bool fold_setcc(alu_node &n);

   bool fold_alu_op1(alu_node &n);
   bool fold_alu_op2(alu_node &n);
   bool fold_alu_op3(alu_node &n);

   bool fold_mul_add(alu_node *n);
   bool eval_const_op(unsigned op, literal &r, literal cv0, literal cv1);
   bool fold_assoc(alu_node *n);

   static void apply_alu_src_mod(const bc_alu &bc, unsigned src, literal &v);
   static void apply_alu_dst_mod(const bc_alu &bc, literal &v);

   void assign_source(value *dst, value *src);

   static bool evaluate_condition(unsigned alu_cnd_flags, literal s1,
                                  literal s2);
};

} // namespace r600_sb

#endif /* SB_EXPR_H_ */
