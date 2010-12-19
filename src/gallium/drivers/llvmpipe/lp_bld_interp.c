/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * @file
 * Position and shader input interpolation.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "tgsi/tgsi_scan.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_swizzle.h"
#include "lp_bld_interp.h"


/*
 * The shader JIT function operates on blocks of quads.
 * Each block has 2x2 quads and each quad has 2x2 pixels.
 *
 * We iterate over the quads in order 0, 1, 2, 3:
 *
 * #################
 * #   |   #   |   #
 * #---0---#---1---#
 * #   |   #   |   #
 * #################
 * #   |   #   |   #
 * #---2---#---3---#
 * #   |   #   |   #
 * #################
 *
 * Within each quad, we have four pixels which are represented in SOA
 * order:
 *
 * #########
 * # 0 | 1 #
 * #---+---#
 * # 2 | 3 #
 * #########
 *
 * So the green channel (for example) of the four pixels is stored in
 * a single vector register: {g0, g1, g2, g3}.
 */


/**
 * Do one perspective divide per quad.
 *
 * For perspective interpolation, the final attribute value is given
 *
 *  a' = a/w = a * oow
 *
 * where
 *
 *  a = a0 + dadx*x + dady*y
 *  w = w0 + dwdx*x + dwdy*y
 *  oow = 1/w = 1/(w0 + dwdx*x + dwdy*y)
 *
 * Instead of computing the division per pixel, with this macro we compute the
 * division on the upper left pixel of each quad, and use a linear
 * approximation in the remaining pixels, given by:
 *
 *  da'dx = (dadx - dwdx*a)*oow
 *  da'dy = (dady - dwdy*a)*oow
 *
 * Ironically, this actually makes things slower -- probably because the
 * divide hardware unit is rarely used, whereas the multiply unit is typically
 * already saturated.
 */
#define PERSPECTIVE_DIVIDE_PER_QUAD 0


static const unsigned char quad_offset_x[4] = {0, 1, 0, 1};
static const unsigned char quad_offset_y[4] = {0, 0, 1, 1};


static void
attrib_name(LLVMValueRef val, unsigned attrib, unsigned chan, const char *suffix)
{
   if(attrib == 0)
      lp_build_name(val, "pos.%c%s", "xyzw"[chan], suffix);
   else
      lp_build_name(val, "input%u.%c%s", attrib - 1, "xyzw"[chan], suffix);
}


/**
 * Initialize the bld->a0, dadx, dady fields.  This involves fetching
 * those values from the arrays which are passed into the JIT function.
 */
static void
coeffs_init(struct lp_build_interp_soa_context *bld,
            LLVMValueRef a0_ptr,
            LLVMValueRef dadx_ptr,
            LLVMValueRef dady_ptr)
{
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef zero = LLVMConstNull(coeff_bld->elem_type);
   LLVMValueRef one = LLVMConstReal(coeff_bld->elem_type, 1.0);
   LLVMValueRef i0 = lp_build_const_int32(gallivm, 0);
   LLVMValueRef i1 = lp_build_const_int32(gallivm, 1);
   LLVMValueRef i2 = lp_build_const_int32(gallivm, 2);
   LLVMValueRef i3 = lp_build_const_int32(gallivm, 3);
   unsigned attrib;
   unsigned chan;

   /* TODO: Use more vector operations */

   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      const unsigned mask = bld->mask[attrib];
      const unsigned interp = bld->interp[attrib];
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         if (mask & (1 << chan)) {
            LLVMValueRef index = lp_build_const_int32(gallivm,
                                      attrib * NUM_CHANNELS + chan);
            LLVMValueRef a0 = zero;
            LLVMValueRef dadx = zero;
            LLVMValueRef dady = zero;
            LLVMValueRef dadxy = zero;
            LLVMValueRef dadq;
            LLVMValueRef dadq2;
            LLVMValueRef a;

            switch (interp) {
            case LP_INTERP_PERSPECTIVE:
               /* fall-through */

            case LP_INTERP_LINEAR:
               if (attrib == 0 && chan == 0) {
                  dadxy = dadx = one;
               }
               else if (attrib == 0 && chan == 1) {
                  dadxy = dady = one;
               }
               else {
                  dadx = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dadx_ptr, &index, 1, ""), "");
                  dady = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dady_ptr, &index, 1, ""), "");
                  dadxy = LLVMBuildFAdd(builder, dadx, dady, "");
                  attrib_name(dadx, attrib, chan, ".dadx");
                  attrib_name(dady, attrib, chan, ".dady");
                  attrib_name(dadxy, attrib, chan, ".dadxy");
               }
               /* fall-through */

            case LP_INTERP_CONSTANT:
            case LP_INTERP_FACING:
               a0 = LLVMBuildLoad(builder, LLVMBuildGEP(builder, a0_ptr, &index, 1, ""), "");
               attrib_name(a0, attrib, chan, ".a0");
               break;

            case LP_INTERP_POSITION:
               /* Nothing to do as the position coeffs are already setup in slot 0 */
               continue;

            default:
               assert(0);
               break;
            }

            /*
             * dadq = {0, dadx, dady, dadx + dady}
             */

            dadq = coeff_bld->undef;
            dadq = LLVMBuildInsertElement(builder, dadq, zero,  i0, "");
            dadq = LLVMBuildInsertElement(builder, dadq, dadx,  i1, "");
            dadq = LLVMBuildInsertElement(builder, dadq, dady,  i2, "");
            dadq = LLVMBuildInsertElement(builder, dadq, dadxy, i3, "");

            /*
             * dadq2 = 2 * dq
             */

            dadq2 = LLVMBuildFAdd(builder, dadq, dadq, "");

            /*
             * a = a0 + (x * dadx + y * dady)
             */

            if (attrib == 0 && chan == 0) {
               a = bld->x;
            }
            else if (attrib == 0 && chan == 1) {
               a = bld->y;
            }
            else {
               a = a0;
               if (interp != LP_INTERP_CONSTANT &&
                   interp != LP_INTERP_FACING) {
                  LLVMValueRef ax, ay, axy;
                  ax = LLVMBuildFMul(builder, bld->x, dadx, "");
                  ay = LLVMBuildFMul(builder, bld->y, dady, "");
                  axy = LLVMBuildFAdd(builder, ax, ay, "");
                  a = LLVMBuildFAdd(builder, a, axy, "");
               }
            }

            /*
             * a = {a, a, a, a}
             */

            a = lp_build_broadcast(gallivm, coeff_bld->vec_type, a);

            /*
             * Compute the attrib values on the upper-left corner of each quad.
             */

            a = LLVMBuildFAdd(builder, a, dadq2, "");

#if PERSPECTIVE_DIVIDE_PER_QUAD
            /*
             * a *= 1 / w
             */

            if (interp == LP_INTERP_PERSPECTIVE) {
               LLVMValueRef w = bld->a[0][3];
               assert(attrib != 0);
               assert(bld->mask[0] & TGSI_WRITEMASK_W);
               if (!bld->oow) {
                  bld->oow = lp_build_rcp(coeff_bld, w);
                  lp_build_name(bld->oow, "oow");
               }
               a = lp_build_mul(coeff_bld, a, bld->oow);
            }
#endif

            attrib_name(a, attrib, chan, ".a");
            attrib_name(dadq, attrib, chan, ".dadq");

            bld->a   [attrib][chan] = a;
            bld->dadq[attrib][chan] = dadq;
         }
      }
   }
}


/**
 * Increment the shader input attribute values.
 * This is called when we move from one quad to the next.
 */
static void
attribs_update(struct lp_build_interp_soa_context *bld,
               struct gallivm_state *gallivm,
               int quad_index,
               int start,
               int end)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   LLVMValueRef shuffle = lp_build_const_int_vec(gallivm, coeff_bld->type, quad_index);
   LLVMValueRef oow = NULL;
   unsigned attrib;
   unsigned chan;

   assert(quad_index < 4);

   for(attrib = start; attrib < end; ++attrib) {
      const unsigned mask = bld->mask[attrib];
      const unsigned interp = bld->interp[attrib];
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(mask & (1 << chan)) {
            LLVMValueRef a;
            if (interp == LP_INTERP_CONSTANT ||
                interp == LP_INTERP_FACING) {
               a = bld->a[attrib][chan];
            }
            else if (interp == LP_INTERP_POSITION) {
               assert(attrib > 0);
               a = bld->attribs[0][chan];
            }
            else {
               LLVMValueRef dadq;

               a = bld->a[attrib][chan];

               /*
                * Broadcast the attribute value for this quad into all elements
                */

               a = LLVMBuildShuffleVector(builder,
                                          a, coeff_bld->undef, shuffle, "");

               /*
                * Get the derivatives.
                */

               dadq = bld->dadq[attrib][chan];

#if PERSPECTIVE_DIVIDE_PER_QUAD
               if (interp == LP_INTERP_PERSPECTIVE) {
                  LLVMValueRef dwdq = bld->dadq[0][3];

                  if (oow == NULL) {
                     assert(bld->oow);
                     oow = LLVMBuildShuffleVector(coeff_bld->builder,
                                                  bld->oow, coeff_bld->undef,
                                                  shuffle, "");
                  }

                  dadq = lp_build_sub(coeff_bld,
                                      dadq,
                                      lp_build_mul(coeff_bld, a, dwdq));
                  dadq = lp_build_mul(coeff_bld, dadq, oow);
               }
#endif

               /*
                * Add the derivatives
                */

               a = lp_build_add(coeff_bld, a, dadq);

#if !PERSPECTIVE_DIVIDE_PER_QUAD
               if (interp == LP_INTERP_PERSPECTIVE) {
                  if (oow == NULL) {
                     LLVMValueRef w = bld->attribs[0][3];
                     assert(attrib != 0);
                     assert(bld->mask[0] & TGSI_WRITEMASK_W);
                     oow = lp_build_rcp(coeff_bld, w);
                  }
                  a = lp_build_mul(coeff_bld, a, oow);
               }
#endif

               if (attrib == 0 && chan == 2) {
                  /* FIXME: Depth values can exceed 1.0, due to the fact that
                   * setup interpolation coefficients refer to (0,0) which causes
                   * precision loss. So we must clamp to 1.0 here to avoid artifacts
                   */
                  a = lp_build_min(coeff_bld, a, coeff_bld->one);
               }

               attrib_name(a, attrib, chan, "");
            }
            bld->attribs[attrib][chan] = a;
         }
      }
   }
}


/**
 * Generate the position vectors.
 *
 * Parameter x0, y0 are the integer values with upper left coordinates.
 */
static void
pos_init(struct lp_build_interp_soa_context *bld,
         LLVMValueRef x0,
         LLVMValueRef y0)
{
   LLVMBuilderRef builder = bld->coeff_bld.gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;

   bld->x = LLVMBuildSIToFP(builder, x0, coeff_bld->elem_type, "");
   bld->y = LLVMBuildSIToFP(builder, y0, coeff_bld->elem_type, "");
}


/**
 * Initialize fragment shader input attribute info.
 */
void
lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,
                         struct gallivm_state *gallivm,
                         unsigned num_inputs,
                         const struct lp_shader_input *inputs,
                         LLVMBuilderRef builder,
                         struct lp_type type,
                         LLVMValueRef a0_ptr,
                         LLVMValueRef dadx_ptr,
                         LLVMValueRef dady_ptr,
                         LLVMValueRef x0,
                         LLVMValueRef y0)
{
   struct lp_type coeff_type;
   unsigned attrib;
   unsigned chan;

   memset(bld, 0, sizeof *bld);

   memset(&coeff_type, 0, sizeof coeff_type);
   coeff_type.floating = TRUE;
   coeff_type.sign = TRUE;
   coeff_type.width = 32;
   coeff_type.length = QUAD_SIZE;

   /* XXX: we don't support interpolating into any other types */
   assert(memcmp(&coeff_type, &type, sizeof coeff_type) == 0);

   lp_build_context_init(&bld->coeff_bld, gallivm, coeff_type);

   /* For convenience */
   bld->pos = bld->attribs[0];
   bld->inputs = (const LLVMValueRef (*)[NUM_CHANNELS]) bld->attribs[1];

   /* Position */
   bld->num_attribs = 1;
   bld->mask[0] = TGSI_WRITEMASK_XYZW;
   bld->interp[0] = LP_INTERP_LINEAR;

   /* Inputs */
   for (attrib = 0; attrib < num_inputs; ++attrib) {
      bld->mask[1 + attrib] = inputs[attrib].usage_mask;
      bld->interp[1 + attrib] = inputs[attrib].interp;
   }
   bld->num_attribs = 1 + num_inputs;

   /* Ensure all masked out input channels have a valid value */
   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         bld->attribs[attrib][chan] = bld->coeff_bld.undef;
      }
   }

   pos_init(bld, x0, y0);

   coeffs_init(bld, a0_ptr, dadx_ptr, dady_ptr);
}


/**
 * Advance the position and inputs to the given quad within the block.
 */
void
lp_build_interp_soa_update_inputs(struct lp_build_interp_soa_context *bld,
                                  struct gallivm_state *gallivm,
                                  int quad_index)
{
   assert(quad_index < 4);

   attribs_update(bld, gallivm, quad_index, 1, bld->num_attribs);
}

void
lp_build_interp_soa_update_pos(struct lp_build_interp_soa_context *bld,
                                  struct gallivm_state *gallivm,
                                  int quad_index)
{
   assert(quad_index < 4);

   attribs_update(bld, gallivm, quad_index, 0, 1);
}

