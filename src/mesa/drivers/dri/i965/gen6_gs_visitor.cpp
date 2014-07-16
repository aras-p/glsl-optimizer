/*
 * Copyright © 2014 Intel Corporation
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
 *
 * This code is based on original work by Ilia Mirkin.
 */

/**
 * \file gen6_gs_visitor.cpp
 *
 * Gen6 geometry shader implementation
 */

#include "gen6_gs_visitor.h"

namespace brw {

void
gen6_gs_visitor::emit_prolog()
{
   vec4_gs_visitor::emit_prolog();

   /* Gen6 geometry shaders require to allocate an initial VUE handle via
    * FF_SYNC message, however the documentation remarks that only one thread
    * can write to the URB simultaneously and the FF_SYNC message provides the
    * synchronization mechanism for this, so using this message effectively
    * stalls the thread until it is its turn to write to the URB. Because of
    * this, the best way to implement geometry shader algorithms in gen6 is to
    * execute the algorithm before the FF_SYNC message to maximize parallelism.
    *
    * To achieve this we buffer the geometry shader outputs for each emitted
    * vertex in vertex_output during operation. Then, when we have processed
    * the last vertex (that is, at thread end time), we send the FF_SYNC
    * message to allocate the initial VUE handle and write all buffered vertex
    * data to the URB in one go.
    *
    * For each emitted vertex, vertex_output will hold vue_map.num_slots
    * data items plus one additional item to hold required flags
    * (PrimType, PrimStart, PrimEnd, as expected by the URB_WRITE message)
    * which come right after the data items for that vertex. Vertex data and
    * flags for the next vertex come right after the data items and flags for
    * the previous vertex.
    */
   this->current_annotation = "gen6 prolog";
   this->vertex_output = src_reg(this,
                                 glsl_type::uint_type,
                                 (prog_data->vue_map.num_slots + 1) *
                                 c->gp->program.VerticesOut);
   this->vertex_output_offset = src_reg(this, glsl_type::uint_type);
   emit(MOV(dst_reg(this->vertex_output_offset), src_reg(0u)));

   /* MRF 1 will be the header for all messages (FF_SYNC and URB_WRITES),
    * so initialize it once to R0.
    */
   vec4_instruction *inst = emit(MOV(dst_reg(MRF, 1),
                                     retype(brw_vec8_grf(0, 0),
                                            BRW_REGISTER_TYPE_UD)));
   inst->force_writemask_all = true;

   /* This will be used as a temporary to store writeback data of FF_SYNC
    * and URB_WRITE messages.
    */
   this->temp = src_reg(this, glsl_type::uint_type);
}

void
gen6_gs_visitor::visit(ir_emit_vertex *)
{
   this->current_annotation = "gen6 emit vertex";
   /* Honor max_vertex layout indication in geometry shader by ignoring any
    * vertices coming after c->gp->program.VerticesOut.
    */
   unsigned num_output_vertices = c->gp->program.VerticesOut;
   emit(CMP(dst_null_d(), this->vertex_count, src_reg(num_output_vertices),
            BRW_CONDITIONAL_L));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      /* Buffer all output slots for this vertex in vertex_output */
      for (int slot = 0; slot < prog_data->vue_map.num_slots; ++slot) {
         /* We will handle PSIZ for each vertex at thread end time since it
          * is not computed by the GS algorithm and requires specific handling.
          */
         int varying = prog_data->vue_map.slot_to_varying[slot];
         if (varying != VARYING_SLOT_PSIZ) {
            dst_reg dst(this->vertex_output);
            dst.reladdr = ralloc(mem_ctx, src_reg);
            memcpy(dst.reladdr, &this->vertex_output_offset, sizeof(src_reg));
            emit_urb_slot(dst, varying);
         }
         emit(ADD(dst_reg(this->vertex_output_offset),
                  this->vertex_output_offset, 1u));
      }

      /* Now buffer flags for this vertex (we only support point output
       * for now).
       */
      dst_reg dst(this->vertex_output);
      dst.reladdr = ralloc(mem_ctx, src_reg);
      memcpy(dst.reladdr, &this->vertex_output_offset, sizeof(src_reg));
      /* If we are outputting points, then every vertex has PrimStart and
       * PrimEnd set.
       */
      if (c->gp->program.OutputType == GL_POINTS) {
         emit(MOV(dst, (_3DPRIM_POINTLIST << URB_WRITE_PRIM_TYPE_SHIFT) |
                  URB_WRITE_PRIM_START | URB_WRITE_PRIM_END));
      }
      emit(ADD(dst_reg(this->vertex_output_offset),
               this->vertex_output_offset, 1u));

      /* Update vertex count */
      emit(ADD(dst_reg(this->vertex_count), this->vertex_count, 1u));
   }
   emit(BRW_OPCODE_ENDIF);
}

void
gen6_gs_visitor::visit(ir_end_primitive *)
{
   this->current_annotation = "gen6 end primitive";
   /* Calling EndPrimitive() is optional for point output. In this case we set
    * the PrimEnd flag when we process EmitVertex().
    */
   if (c->gp->program.OutputType == GL_POINTS)
      return;
}

void
gen6_gs_visitor::emit_urb_write_header(int mrf)
{
   this->current_annotation = "gen6 urb header";
   /* Compute offset of the flags for the current vertex in vertex_output and
    * write them in dw2 of the message header.
    *
    * Notice that by the time that emit_thread_end() calls here
    * vertex_output_offset should point to the first data item of the current
    * vertex in vertex_output, thus we only need to add the number of output
    * slots per vertex to that offset to obtain the flags data offset.
    */
   src_reg flags_offset(this, glsl_type::uint_type);
   emit(ADD(dst_reg(flags_offset),
            this->vertex_output_offset, src_reg(prog_data->vue_map.num_slots)));

   src_reg flags_data(this->vertex_output);
   flags_data.reladdr = ralloc(mem_ctx, src_reg);
   memcpy(flags_data.reladdr, &flags_offset, sizeof(src_reg));

   emit(GS_OPCODE_SET_DWORD_2, dst_reg(MRF, mrf), flags_data);
}

void
gen6_gs_visitor::emit_urb_write_opcode(bool complete, src_reg vertex,
                                       int base_mrf, int mlen, int urb_offset)
{
   vec4_instruction *inst = NULL;

   /* If the vertex is not complete we don't have to do anything special */
   if (!complete) {
      inst = emit(GS_OPCODE_URB_WRITE);
      inst->urb_write_flags = BRW_URB_WRITE_NO_FLAGS;
      inst->base_mrf = base_mrf;
      inst->mlen = mlen;
      inst->offset = urb_offset;
      return;
   }

   /* Otherwise, if this is not the last vertex we are going to write,
    * we have to request a new VUE handle for the next vertex.
    *
    * Notice that the vertex parameter has been pre-incremented in
    * emit_thread_end() to make this comparison easier.
    */
   emit(CMP(dst_null_d(), vertex, this->vertex_count, BRW_CONDITIONAL_L));
   emit(IF(BRW_PREDICATE_NORMAL));
   {
      inst = emit(GS_OPCODE_URB_WRITE_ALLOCATE);
      inst->urb_write_flags = BRW_URB_WRITE_COMPLETE;
      inst->base_mrf = base_mrf;
      inst->mlen = mlen;
      inst->offset = urb_offset;
      inst->dst = dst_reg(MRF, base_mrf);
      inst->src[0] = this->temp;
   }
   emit(BRW_OPCODE_ELSE);
   {
      inst = emit(GS_OPCODE_URB_WRITE);
      inst->urb_write_flags = BRW_URB_WRITE_COMPLETE;
      inst->base_mrf = base_mrf;
      inst->mlen = mlen;
      inst->offset = urb_offset;
   }
   emit(BRW_OPCODE_ENDIF);
}

void
gen6_gs_visitor::emit_thread_end()
{
   /* Here we have to:
    * 1) Emit an FF_SYNC messsage to obtain an initial VUE handle.
    * 2) Loop over all buffered vertex data and write it to corresponding
    *    URB entries.
    * 3) Allocate new VUE handles for all vertices other than the first.
    * 4) Send a final EOT message.
    */

   /* MRF 0 is reserved for the debugger, so start with message header
    * in MRF 1.
    */
   int base_mrf = 1;

   /* In the process of generating our URB write message contents, we
    * may need to unspill a register or load from an array.  Those
    * reads would use MRFs 14-15.
    */
   int max_usable_mrf = 13;

   /* Issue the FF_SYNC message and obtain the initial VUE handle. */
   this->current_annotation = "gen6 thread end: ff_sync";
   vec4_instruction *inst =
      emit(GS_OPCODE_FF_SYNC, dst_reg(this->temp), this->vertex_count);
   inst->base_mrf = base_mrf;

   /* Loop over all buffered vertices and emit URB write messages */
   this->current_annotation = "gen6 thread end: urb writes init";
   src_reg vertex(this, glsl_type::uint_type);
   emit(MOV(dst_reg(vertex), 0u));
   emit(MOV(dst_reg(this->vertex_output_offset), 0u));

   this->current_annotation = "gen6 thread end: urb writes";
   emit(BRW_OPCODE_DO);
   {
      emit(CMP(dst_null_d(), vertex, this->vertex_count, BRW_CONDITIONAL_GE));
      inst = emit(BRW_OPCODE_BREAK);
      inst->predicate = BRW_PREDICATE_NORMAL;

      /* First we prepare the message header */
      emit_urb_write_header(base_mrf);

      /* Then add vertex data to the message in interleaved fashion */
      int slot = 0;
      bool complete = false;
      do {
         int mrf = base_mrf + 1;

         /* URB offset is in URB row increments, and each of our MRFs is half
          * of one of those, since we're doing interleaved writes.
          */
         int urb_offset = slot / 2;

         for (; slot < prog_data->vue_map.num_slots; ++slot) {
            int varying = prog_data->vue_map.slot_to_varying[slot];
            current_annotation = output_reg_annotation[varying];

            /* Compute offset of this slot for the current vertex
             * in vertex_output
             */
            src_reg data(this->vertex_output);
            data.reladdr = ralloc(mem_ctx, src_reg);
            memcpy(data.reladdr, &this->vertex_output_offset, sizeof(src_reg));

            if (varying == VARYING_SLOT_PSIZ) {
               /* We did not buffer PSIZ, emit it directly here */
               emit_urb_slot(dst_reg(MRF, mrf), varying);
            } else {
               /* Copy this slot to the appropriate message register */
               dst_reg reg = dst_reg(MRF, mrf);
               reg.type = output_reg[varying].type;
               data.type = reg.type;
               vec4_instruction *inst = emit(MOV(reg, data));
               inst->force_writemask_all = true;
            }

            mrf++;
            emit(ADD(dst_reg(this->vertex_output_offset),
                     this->vertex_output_offset, 1u));

            /* If this was max_usable_mrf, we can't fit anything more into this
             * URB WRITE.
             */
            if (mrf > max_usable_mrf) {
               slot++;
               break;
            }
         }

         complete = slot >= prog_data->vue_map.num_slots;

         /* When we emit the URB_WRITE below we need to do different things
          * depending on whether this is the last vertex we are going to
          * write. That means that we will need to check if
          * vertex >= vertex_count - 1. However, by increasing vertex early
          * we transform that comparison into vertex >= vertex_count, which
          * is more convenient.
          */
         if (complete)
            emit(ADD(dst_reg(vertex), vertex, 1u));

         /* URB data written (does not include the message header reg) must
          * be a multiple of 256 bits, or 2 VS registers.  See vol5c.5,
          * section 5.4.3.2.2: URB_INTERLEAVED.
          */
         int mlen = mrf - base_mrf;
         if ((mlen % 2) != 1)
            mlen++;
         emit_urb_write_opcode(complete, vertex, base_mrf, mlen, urb_offset);
      } while (!complete);

      /* Skip over the flags data item so that vertex_output_offset points to
       * the first data item of the next vertex, so that we can start writing
       * the next vertex.
       */
       emit(ADD(dst_reg(this->vertex_output_offset),
                this->vertex_output_offset, 1u));
   }
   emit(BRW_OPCODE_WHILE);

   /* Finally, emit EOT message.
    *
    * In gen6 it looks like we have to set the complete flag too, otherwise
    * the GPU hangs.
    */
   this->current_annotation = "gen6 thread end: EOT";
   inst = emit(GS_OPCODE_THREAD_END);
   inst->urb_write_flags = BRW_URB_WRITE_COMPLETE;
   inst->base_mrf = base_mrf;
   inst->mlen = 1;
}

} /* namespace brw */
