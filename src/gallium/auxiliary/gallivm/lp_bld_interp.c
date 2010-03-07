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
#include "tgsi/tgsi_parse.h"
#include "lp_bld_debug.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_swizzle.h"
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
   LLVMBuilderRef builder = bld->base.builder;
   unsigned attrib;
   unsigned chan;

   for(attrib = 0; attrib < bld->num_attribs; ++attrib) {
      unsigned mask = bld->mask[attrib];
      unsigned mode = bld->mode[attrib];
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(mask & (1 << chan)) {
            LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), attrib*NUM_CHANNELS + chan, 0);
            LLVMValueRef a0 = NULL;
            LLVMValueRef dadx = NULL;
            LLVMValueRef dady = NULL;

            switch( mode ) {
            case TGSI_INTERPOLATE_PERSPECTIVE:
               /* fall-through */

            case TGSI_INTERPOLATE_LINEAR:
               dadx = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dadx_ptr, &index, 1, ""), "");
               dady = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dady_ptr, &index, 1, ""), "");
               dadx = lp_build_broadcast_scalar(&bld->base, dadx);
               dady = lp_build_broadcast_scalar(&bld->base, dady);
               attrib_name(dadx, attrib, chan, ".dadx");
               attrib_name(dady, attrib, chan, ".dady");
               /* fall-through */

            case TGSI_INTERPOLATE_CONSTANT:
               a0 = LLVMBuildLoad(builder, LLVMBuildGEP(builder, a0_ptr, &index, 1, ""), "");
               a0 = lp_build_broadcast_scalar(&bld->base, a0);
               attrib_name(a0, attrib, chan, ".a0");
               break;

            default:
               assert(0);
               break;
            }

            bld->a0  [attrib][chan] = a0;
            bld->dadx[attrib][chan] = dadx;
            bld->dady[attrib][chan] = dady;
         }
      }
   }
}


/**
 * Emit LLVM code to compute the fragment shader input attribute values.
 * For example, for a color input, we'll compute red, green, blue and alpha
 * values for the four pixels in a quad.
 * Recall that we're operating on 4-element vectors so each arithmetic
 * operation is operating on the four pixels in a quad.
 */
static void
attribs_init(struct lp_build_interp_soa_context *bld)
{
   LLVMValueRef x = bld->pos[0];
   LLVMValueRef y = bld->pos[1];
   LLVMValueRef oow = NULL;
   unsigned attrib;
   unsigned chan;

   for(attrib = 0; attrib < bld->num_attribs; ++attrib) {
      unsigned mask = bld->mask[attrib];
      unsigned mode = bld->mode[attrib];
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(mask & (1 << chan)) {
            LLVMValueRef a0   = bld->a0  [attrib][chan];
            LLVMValueRef dadx = bld->dadx[attrib][chan];
            LLVMValueRef dady = bld->dady[attrib][chan];
            LLVMValueRef res;

            res = a0;

            if (mode != TGSI_INTERPOLATE_CONSTANT) {
               /* res = res + x * dadx */
               res = lp_build_add(&bld->base, res, lp_build_mul(&bld->base, x, dadx));
               /* res = res + y * dady */
               res = lp_build_add(&bld->base, res, lp_build_mul(&bld->base, y, dady));
            }

            /* Keep the value of the attribue before perspective divide for faster updates */
            bld->attribs_pre[attrib][chan] = res;

            if (mode == TGSI_INTERPOLATE_PERSPECTIVE) {
               LLVMValueRef w = bld->pos[3];
               assert(attrib != 0);
               if(!oow)
                  oow = lp_build_rcp(&bld->base, w);
               res = lp_build_mul(&bld->base, res, oow);
            }

            attrib_name(res, attrib, chan, "");

            bld->attribs[attrib][chan] = res;
         }
      }
   }
}


/**
 * Increment the shader input attribute values.
 * This is called when we move from one quad to the next.
 */
static void
attribs_update(struct lp_build_interp_soa_context *bld, int quad_index)
{
   LLVMValueRef oow = NULL;
   unsigned attrib;
   unsigned chan;

   assert(quad_index < 4);

   for(attrib = 0; attrib < bld->num_attribs; ++attrib) {
      unsigned mask = bld->mask[attrib];
      unsigned mode = bld->mode[attrib];

      if (mode != TGSI_INTERPOLATE_CONSTANT) {
         for(chan = 0; chan < NUM_CHANNELS; ++chan) {
            if(mask & (1 << chan)) {
               LLVMValueRef dadx = bld->dadx[attrib][chan];
               LLVMValueRef dady = bld->dady[attrib][chan];
               LLVMValueRef res;

               res = bld->attribs_pre[attrib][chan];

               if (quad_index == 1 || quad_index == 3) {
                  /* top-right or bottom-right quad */
                  /* build res = res + dadx + dadx */
                  res = lp_build_add(&bld->base, res, dadx);
                  res = lp_build_add(&bld->base, res, dadx);
               }

               if (quad_index == 2 || quad_index == 3) {
                  /* bottom-left or bottom-right quad */
                  /* build res = res + dady + dady */
                  res = lp_build_add(&bld->base, res, dady);
                  res = lp_build_add(&bld->base, res, dady);
               }

               //XXX bld->attribs_pre[attrib][chan] = res;

               if (mode == TGSI_INTERPOLATE_PERSPECTIVE) {
                  LLVMValueRef w = bld->pos[3];
                  assert(attrib != 0);
                  if(!oow)
                     oow = lp_build_rcp(&bld->base, w);
                  res = lp_build_mul(&bld->base, res, oow);
               }

               attrib_name(res, attrib, chan, "");

               bld->attribs[attrib][chan] = res;
            }
         }
      }
   }
}


/**
 * Generate the position vectors.
 *
 * Parameter x0, y0 are the integer values with the quad upper left coordinates.
 */
static void
pos_init(struct lp_build_interp_soa_context *bld,
         LLVMValueRef x0,
         LLVMValueRef y0)
{
   lp_build_name(x0, "pos.x");
   lp_build_name(y0, "pos.y");

   bld->attribs[0][0] = x0;
   bld->attribs[0][1] = y0;
}


/**
 * Update quad position values when moving to the next quad.
 */
static void
pos_update(struct lp_build_interp_soa_context *bld, int quad_index)
{
   LLVMValueRef x = bld->attribs[0][0];
   LLVMValueRef y = bld->attribs[0][1];
   const int xstep = 2, ystep = 2;

   if (quad_index == 1 || quad_index == 3) {
      /* top-right or bottom-right quad in block */
      /* build x += xstep */
      x = lp_build_add(&bld->base, x,
                       lp_build_const_scalar(bld->base.type, xstep));
   }

   if (quad_index == 2) {
      /* bottom-left quad in block */
      /* build y += ystep */
      y = lp_build_add(&bld->base, y,
                       lp_build_const_scalar(bld->base.type, ystep));
      /* build x -= xstep */
      x = lp_build_sub(&bld->base, x,
                       lp_build_const_scalar(bld->base.type, xstep));
   }

   lp_build_name(x, "pos.x");
   lp_build_name(y, "pos.y");

   bld->attribs[0][0] = x;
   bld->attribs[0][1] = y;
}


/**
 * Initialize fragment shader input attribute info.
 */
void
lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,
                         const struct tgsi_token *tokens,
                         boolean flatshade,
                         LLVMBuilderRef builder,
                         struct lp_type type,
                         LLVMValueRef a0_ptr,
                         LLVMValueRef dadx_ptr,
                         LLVMValueRef dady_ptr,
                         LLVMValueRef x0,
                         LLVMValueRef y0)
{
   struct tgsi_parse_context parse;
   struct tgsi_full_declaration *decl;

   memset(bld, 0, sizeof *bld);

   lp_build_context_init(&bld->base, builder, type);

   /* For convenience */
   bld->pos = bld->attribs[0];
   bld->inputs = (const LLVMValueRef (*)[NUM_CHANNELS]) bld->attribs[1];

   /* Position */
   bld->num_attribs = 1;
   bld->mask[0] = TGSI_WRITEMASK_ZW;
   bld->mode[0] = TGSI_INTERPOLATE_LINEAR;

   /* Inputs */
   tgsi_parse_init( &parse, tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         decl = &parse.FullToken.FullDeclaration;
         if( decl->Declaration.File == TGSI_FILE_INPUT ) {
            unsigned first, last, mask;
            unsigned attrib;

            first = decl->Range.First;
            last = decl->Range.Last;
            mask = decl->Declaration.UsageMask;

            for( attrib = first; attrib <= last; ++attrib ) {
               bld->mask[1 + attrib] = mask;

               /* XXX: have mesa set INTERP_CONSTANT in the fragment
                * shader.
                */
               if (decl->Semantic.Name == TGSI_SEMANTIC_COLOR &&
                   flatshade)
                  bld->mode[1 + attrib] = TGSI_INTERPOLATE_CONSTANT;
               else
                  bld->mode[1 + attrib] = decl->Declaration.Interpolate;
            }

            bld->num_attribs = MAX2(bld->num_attribs, 1 + last + 1);
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
      case TGSI_TOKEN_TYPE_IMMEDIATE:
      case TGSI_TOKEN_TYPE_PROPERTY:
         break;

      default:
         assert( 0 );
      }
   }
   tgsi_parse_free( &parse );

   coeffs_init(bld, a0_ptr, dadx_ptr, dady_ptr);

   pos_init(bld, x0, y0);

   attribs_init(bld);
}


/**
 * Advance the position and inputs to the given quad within the block.
 */
void
lp_build_interp_soa_update(struct lp_build_interp_soa_context *bld,
                           int quad_index)
{
   assert(quad_index < 4);

   pos_update(bld, quad_index);

   attribs_update(bld, quad_index);
}
