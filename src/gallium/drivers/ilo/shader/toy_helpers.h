/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef TOY_HELPERS_H
#define TOY_HELPERS_H

#include "toy_compiler.h"

/**
 * Transpose a dst operand.
 *
 * Instead of processing a single vertex with each of its attributes in one
 * register, such as
 *
 *   r0 = [x0, y0, z0, w0]
 *
 * we want to process four vertices at a time
 *
 *   r0 = [x0, y0, z0, w0]
 *   r1 = [x1, y1, z1, w1]
 *   r2 = [x2, y2, z2, w2]
 *   r3 = [x3, y3, z3, w3]
 *
 * but with the attribute data "transposed"
 *
 *   r0 = [x0, x1, x2, x3]
 *   r1 = [y0, y1, y2, y3]
 *   r2 = [z0, z1, z2, z3]
 *   r3 = [w0, w1, w2, w3]
 *
 * This is also known as the SoA form.
 */
static inline void
tdst_transpose(struct toy_dst dst, struct toy_dst *trans)
{
   int i;

   switch (dst.file) {
   case TOY_FILE_VRF:
      assert(!dst.indirect);
      for (i = 0; i < 4; i++) {
         if (dst.writemask & (1 << i)) {
            trans[i] = tdst_offset(dst, i, 0);
            trans[i].writemask = TOY_WRITEMASK_XYZW;
         }
         else {
            trans[i] = tdst_null();
         }
      }
      break;
   case TOY_FILE_ARF:
      assert(tdst_is_null(dst));
      for (i = 0; i < 4; i++)
         trans[i] = dst;
      break;
   case TOY_FILE_GRF:
   case TOY_FILE_MRF:
   case TOY_FILE_IMM:
   default:
      assert(!"unexpected file in dst transposition");
      for (i = 0; i < 4; i++)
         trans[i] = tdst_null();
      break;
   }
}

/**
 * Transpose a src operand.
 */
static inline void
tsrc_transpose(struct toy_src src, struct toy_src *trans)
{
   const enum toy_swizzle swizzle[4] = {
      src.swizzle_x, src.swizzle_y,
      src.swizzle_z, src.swizzle_w,
   };
   int i;

   switch (src.file) {
   case TOY_FILE_VRF:
      assert(!src.indirect);
      for (i = 0; i < 4; i++) {
         trans[i] = tsrc_offset(src, swizzle[i], 0);
         trans[i].swizzle_x = TOY_SWIZZLE_X;
         trans[i].swizzle_y = TOY_SWIZZLE_Y;
         trans[i].swizzle_z = TOY_SWIZZLE_Z;
         trans[i].swizzle_w = TOY_SWIZZLE_W;
      }
      break;
   case TOY_FILE_ARF:
      assert(tsrc_is_null(src));
      /* fall through */
   case TOY_FILE_IMM:
      for (i = 0; i < 4; i++)
         trans[i] = src;
      break;
   case TOY_FILE_GRF:
   case TOY_FILE_MRF:
   default:
      assert(!"unexpected file in src transposition");
      for (i = 0; i < 4; i++)
         trans[i] = tsrc_null();
      break;
   }
}

static inline struct toy_src
tsrc_imm_mdesc(const struct toy_compiler *tc,
               bool eot,
               unsigned message_length,
               unsigned response_length,
               bool header_present,
               uint32_t function_control)
{
   uint32_t desc;

   assert(message_length >= 1 && message_length <= 15);
   assert(response_length >= 0 && response_length <= 16);
   assert(function_control < 1 << 19);

   desc = eot << 31 |
          message_length << 25 |
          response_length << 20 |
          header_present << 19 |
          function_control;

   return tsrc_imm_ud(desc);
}

static inline struct toy_src
tsrc_imm_mdesc_sampler(const struct toy_compiler *tc,
                       unsigned message_length,
                       unsigned response_length,
                       bool header_present,
                       unsigned simd_mode,
                       unsigned message_type,
                       unsigned sampler_index,
                       unsigned binding_table_index)
{
   const bool eot = false;
   uint32_t ctrl;

   assert(simd_mode < 4);
   assert(sampler_index < 16);
   assert(binding_table_index < 256);

   if (ilo_dev_gen(tc->dev) >= ILO_GEN(7)) {
      ctrl = simd_mode << 17 |
             message_type << 12 |
             sampler_index << 8 |
             binding_table_index;
   }
   else {
      ctrl = simd_mode << 16 |
             message_type << 12 |
             sampler_index << 8 |
             binding_table_index;
   }

   return tsrc_imm_mdesc(tc, eot, message_length,
         response_length, header_present, ctrl);
}

static inline struct toy_src
tsrc_imm_mdesc_data_port(const struct toy_compiler *tc,
                         bool eot,
                         unsigned message_length,
                         unsigned response_length,
                         bool header_present,
                         bool send_write_commit_message,
                         unsigned message_type,
                         unsigned message_specific_control,
                         unsigned binding_table_index)
{
   uint32_t ctrl;

   if (ilo_dev_gen(tc->dev) >= ILO_GEN(7)) {
      assert(!send_write_commit_message);
      assert((message_specific_control & 0x3f00) == message_specific_control);

      ctrl = message_type << 14 |
             (message_specific_control & 0x3f00) |
             binding_table_index;
   }
   else {
      assert(!send_write_commit_message ||
             message_type == GEN6_MSG_DP_SVB_WRITE);
      assert((message_specific_control & 0x1f00) == message_specific_control);

      ctrl = send_write_commit_message << 17 |
             message_type << 13 |
             (message_specific_control & 0x1f00) |
             binding_table_index;
   }

   return tsrc_imm_mdesc(tc, eot, message_length,
         response_length, header_present, ctrl);
}

static inline struct toy_src
tsrc_imm_mdesc_data_port_scratch(const struct toy_compiler *tc,
                                 unsigned message_length,
                                 unsigned response_length,
                                 bool write_type,
                                 bool dword_mode,
                                 bool invalidate_after_read,
                                 int num_registers,
                                 int hword_offset)
{
   const bool eot = false;
   const bool header_present = true;
   uint32_t ctrl;

   assert(ilo_dev_gen(tc->dev) >= ILO_GEN(7));
   assert(num_registers == 1 || num_registers == 2 || num_registers == 4);

   ctrl = 1 << 18 |
          write_type << 17 |
          dword_mode << 16 |
          invalidate_after_read << 15 |
          (num_registers - 1) << 12 |
          hword_offset;

   return tsrc_imm_mdesc(tc, eot, message_length,
         response_length, header_present, ctrl);
}

static inline struct toy_src
tsrc_imm_mdesc_urb(const struct toy_compiler *tc,
                   bool eot,
                   unsigned message_length,
                   unsigned response_length,
                   bool complete,
                   bool used,
                   bool allocate,
                   unsigned swizzle_control,
                   unsigned global_offset,
                   unsigned urb_opcode)
{
   const bool header_present = true;
   uint32_t ctrl;

   if (ilo_dev_gen(tc->dev) >= ILO_GEN(7)) {
      const bool per_slot_offset = false;

      ctrl = per_slot_offset << 16 |
             complete << 15 |
             swizzle_control << 14 |
             global_offset << 3 |
             urb_opcode;
   }
   else {
      ctrl = complete << 15 |
             used << 14 |
             allocate << 13 |
             swizzle_control << 10 |
             global_offset << 4 |
             urb_opcode;
   }

   return tsrc_imm_mdesc(tc, eot, message_length,
         response_length, header_present, ctrl);
}

#endif /* TOY_HELPERS_H */
