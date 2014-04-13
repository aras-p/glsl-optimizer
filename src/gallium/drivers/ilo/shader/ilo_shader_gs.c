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

#include "tgsi/tgsi_dump.h"
#include "toy_compiler.h"
#include "toy_tgsi.h"
#include "toy_legalize.h"
#include "toy_optimize.h"
#include "toy_helpers.h"
#include "ilo_shader_internal.h"

/* XXX Below is proof-of-concept code.  Skip this file! */

/*
 * TODO
 * - primitive id is in r0.1.  FS receives PID as a flat attribute.
 * - set VUE header m0.1 for layered rendering
 */
struct gs_compile_context {
   struct ilo_shader *shader;
   const struct ilo_shader_variant *variant;
   const struct pipe_stream_output_info *so_info;

   struct toy_compiler tc;
   struct toy_tgsi tgsi;
   int output_map[PIPE_MAX_SHADER_OUTPUTS];

   bool write_so;
   bool write_vue;

   int in_vue_size;
   int in_vue_count;

   int out_vue_size;
   int out_vue_min_count;

   bool is_static;

   struct {
      struct toy_src header;
      struct toy_src svbi;
      struct toy_src vues[6];
   } payload;

   struct {
      struct toy_dst urb_write_header;
      bool prim_start;
      bool prim_end;
      int prim_type;

      struct toy_dst tmp;

      /* buffered tgsi_outs */
      struct toy_dst buffers[3];
      int buffer_needed, buffer_cur;

      struct toy_dst so_written;
      struct toy_dst so_index;

      struct toy_src tgsi_outs[PIPE_MAX_SHADER_OUTPUTS];
   } vars;

   struct {
      struct toy_dst total_vertices;
      struct toy_dst total_prims;

      struct toy_dst num_vertices;
      struct toy_dst num_vertices_in_prim;
   } dynamic_data;

   struct {
      int total_vertices;
      int total_prims;
      /* this limits the max vertice count to be 256 */
      uint32_t last_vertex[8];

      int num_vertices;
      int num_vertices_in_prim;
   } static_data;

   int first_free_grf;
   int last_free_grf;
   int first_free_mrf;
   int last_free_mrf;
};

static void
gs_COPY8(struct toy_compiler *tc, struct toy_dst dst, struct toy_src src)
{
   struct toy_inst *inst;

   inst = tc_MOV(tc, dst, src);
   inst->exec_size = GEN6_EXECSIZE_8;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
}

static void
gs_COPY4(struct toy_compiler *tc,
         struct toy_dst dst, int dst_ch,
         struct toy_src src, int src_ch)
{
   struct toy_inst *inst;

   inst = tc_MOV(tc,
         tdst_offset(dst, 0, dst_ch),
         tsrc_offset(src, 0, src_ch));
   inst->exec_size = GEN6_EXECSIZE_4;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
}

static void
gs_COPY1(struct toy_compiler *tc,
         struct toy_dst dst, int dst_ch,
         struct toy_src src, int src_ch)
{
   struct toy_inst *inst;

   inst = tc_MOV(tc,
         tdst_offset(dst, 0, dst_ch),
         tsrc_rect(tsrc_offset(src, 0, src_ch), TOY_RECT_010));
   inst->exec_size = GEN6_EXECSIZE_1;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;
}

static void
gs_init_vars(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_dst dst;

   /* init URB_WRITE header */
   dst = gcc->vars.urb_write_header;

   gs_COPY8(tc, dst, gcc->payload.header);

   gcc->vars.prim_start = true;
   gcc->vars.prim_end = false;
   switch (gcc->out_vue_min_count) {
   case 1:
      gcc->vars.prim_type = GEN6_3DPRIM_POINTLIST;
      break;
   case 2:
      gcc->vars.prim_type = GEN6_3DPRIM_LINESTRIP;
      break;
   case 3:
      gcc->vars.prim_type = GEN6_3DPRIM_TRISTRIP;
      break;
   }

   if (gcc->write_so)
      tc_MOV(tc, gcc->vars.so_written, tsrc_imm_d(0));
}

static void
gs_save_output(struct gs_compile_context *gcc, const struct toy_src *outs)
{
   struct toy_compiler *tc = &gcc->tc;
   const struct toy_dst buf = gcc->vars.buffers[gcc->vars.buffer_cur];
   int i;

   for (i = 0; i < gcc->shader->out.count; i++)
      tc_MOV(tc, tdst_offset(buf, i, 0), outs[i]);

   /* advance the cursor */
   gcc->vars.buffer_cur++;
   gcc->vars.buffer_cur %= gcc->vars.buffer_needed;
}

static void
gs_write_so(struct gs_compile_context *gcc,
            struct toy_dst dst,
            struct toy_src index, struct toy_src out,
            bool send_write_commit_message,
            int binding_table_index)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_dst mrf_header;
   struct toy_src desc;

   mrf_header = tdst_d(tdst(TOY_FILE_MRF, gcc->first_free_mrf, 0));

   /* m0.5: destination index */
   gs_COPY1(tc, mrf_header, 5, index, 0);

   /* m0.0 - m0.3: RGBA */
   gs_COPY4(tc, mrf_header, 0, tsrc_type(out, mrf_header.type), 0);

   desc = tsrc_imm_mdesc_data_port(tc, false,
         1, send_write_commit_message,
         true, send_write_commit_message,
         GEN6_MSG_DP_SVB_WRITE, 0,
         binding_table_index);

   tc_SEND(tc, dst, tsrc_from(mrf_header), desc,
         GEN6_SFID_DP_RC);
}

static void
gs_write_vue(struct gs_compile_context *gcc,
             struct toy_dst dst, struct toy_src msg_header,
             const struct toy_src *outs, int num_outs,
             bool eot)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_dst mrf_header;
   struct toy_src desc;
   int sent = 0;

   mrf_header = tdst_d(tdst(TOY_FILE_MRF, gcc->first_free_mrf, 0));
   gs_COPY8(tc, mrf_header, msg_header);

   while (sent < num_outs) {
      int mrf = gcc->first_free_mrf + 1;
      const int mrf_avail = gcc->last_free_mrf - mrf + 1;
      int msg_len, num_entries, i;
      bool complete;

      num_entries = (num_outs - sent + 1) / 2;
      complete = true;
      if (num_entries > mrf_avail) {
         num_entries = mrf_avail;
         complete = false;
      }

      for (i = 0; i < num_entries; i++) {
         gs_COPY4(tc, tdst(TOY_FILE_MRF, mrf + i / 2, 0), 0,
               outs[sent + 2 * i], 0);
         if (sent + i * 2 + 1 < gcc->shader->out.count) {
            gs_COPY4(tc, tdst(TOY_FILE_MRF, mrf + i / 2, 0), 4,
                  outs[sent + 2 * i + 1], 0);
         }
         mrf++;
      }

      /* do not forget the header */
      msg_len = num_entries + 1;

      if (complete) {
         desc = tsrc_imm_mdesc_urb(tc,
               eot, msg_len, !eot, true, true, !eot,
               false, sent, 0);
      }
      else {
         desc = tsrc_imm_mdesc_urb(tc,
               false, msg_len, 0, false, true, false,
               false, sent, 0);
      }

      tc_add2(tc, TOY_OPCODE_URB_WRITE,
            (complete) ? dst : tdst_null(), tsrc_from(mrf_header), desc);

      sent += num_entries * 2;
   }
}

static void
gs_ff_sync(struct gs_compile_context *gcc, struct toy_dst dst,
           struct toy_src num_prims)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_dst mrf_header =
      tdst_d(tdst(TOY_FILE_MRF, gcc->first_free_mrf, 0));
   struct toy_src desc;
   bool allocate;

   gs_COPY8(tc, mrf_header, gcc->payload.header);

   /* set NumSOVertsToWrite and NumSOPrimsNeeded */
   if (gcc->write_so) {
      if (num_prims.file == TOY_FILE_IMM) {
         const uint32_t v =
            (num_prims.val32 * gcc->in_vue_count) << 16 | num_prims.val32;

         gs_COPY1(tc, mrf_header, 0, tsrc_imm_d(v), 0);
      }
      else {
         struct toy_dst m0_0 = tdst_d(gcc->vars.tmp);

         tc_MUL(tc, m0_0, num_prims, tsrc_imm_d(gcc->in_vue_count << 16));
         tc_OR(tc, m0_0, tsrc_from(m0_0), num_prims);

         gs_COPY1(tc, mrf_header, 0, tsrc_from(m0_0), 0);
      }
   }

   /* set NumGSPrimsGenerated */
   if (gcc->write_vue)
      gs_COPY1(tc, mrf_header, 1, num_prims, 0);

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 173:
    *
    *     "Programming Note: If the GS stage is enabled, software must always
    *      allocate at least one GS URB Entry. This is true even if the GS
    *      thread never needs to output vertices to the pipeline, e.g., when
    *      only performing stream output. This is an artifact of the need to
    *      pass the GS thread an initial destination URB handle."
    */
   allocate = true;
   desc = tsrc_imm_mdesc_urb(tc, false, 1, 1,
         false, false, allocate,
         false, 0, 1);

   tc_SEND(tc, dst, tsrc_from(mrf_header), desc, GEN6_SFID_URB);
}

static void
gs_discard(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_dst mrf_header;
   struct toy_src desc;

   mrf_header = tdst_d(tdst(TOY_FILE_MRF, gcc->first_free_mrf, 0));

   gs_COPY8(tc, mrf_header, tsrc_from(gcc->vars.urb_write_header));

   desc = tsrc_imm_mdesc_urb(tc,
         true, 1, 0, true, false, false,
         false, 0, 0);

   tc_add2(tc, TOY_OPCODE_URB_WRITE,
         tdst_null(), tsrc_from(mrf_header), desc);
}

static void
gs_lower_opcode_endprim(struct gs_compile_context *gcc, struct toy_inst *inst)
{
   /* if has control flow, set PrimEnd on the last vertex and URB_WRITE */
}

static void
gs_lower_opcode_emit_vue_dynamic(struct gs_compile_context *gcc)
{
   /* TODO similar to the static version */

   /*
    * When SO is enabled and the inputs are lines or triangles, vertices are
    * always buffered.  we can defer the emission of the current vertex until
    * the next EMIT or ENDPRIM.  Or, we can emit two URB_WRITEs with the later
    * patching the former.
    */
}

static void
gs_lower_opcode_emit_so_dynamic(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;

   tc_IF(tc, tdst_null(),
         tsrc_from(gcc->dynamic_data.num_vertices_in_prim),
         tsrc_imm_d(gcc->out_vue_min_count),
         GEN6_COND_GE);

   {
      tc_ADD(tc, gcc->vars.tmp, tsrc_from(gcc->vars.so_index), tsrc_imm_d(0x03020100));

      /* TODO same as static version */
   }

   tc_ENDIF(tc);

   tc_ADD(tc, gcc->vars.so_index,
         tsrc_from(gcc->vars.so_index), tsrc_imm_d(gcc->out_vue_min_count));
}

static void
gs_lower_opcode_emit_vue_static(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_inst *inst2;
   bool eot;

   eot = (gcc->static_data.num_vertices == gcc->static_data.total_vertices);

   gcc->vars.prim_end =
      ((gcc->static_data.last_vertex[(gcc->static_data.num_vertices - 1) / 32] &
        1 << ((gcc->static_data.num_vertices - 1) % 32)) != 0);

   if (eot && gcc->write_so) {
      inst2 = tc_OR(tc, tdst_offset(gcc->vars.urb_write_header, 0, 2),
            tsrc_from(gcc->vars.so_written),
            tsrc_imm_d(gcc->vars.prim_type << 2 |
                       gcc->vars.prim_start << 1 |
                       gcc->vars.prim_end));
      inst2->exec_size = GEN6_EXECSIZE_1;
      inst2->src[0] = tsrc_rect(inst2->src[0], TOY_RECT_010);
      inst2->src[1] = tsrc_rect(inst2->src[1], TOY_RECT_010);
   }
   else {
      gs_COPY1(tc, gcc->vars.urb_write_header, 2,
            tsrc_imm_d(gcc->vars.prim_type << 2 |
                       gcc->vars.prim_start << 1 |
                       gcc->vars.prim_end), 0);
   }

   gs_write_vue(gcc, tdst_d(gcc->vars.tmp),
         tsrc_from(gcc->vars.urb_write_header),
         gcc->vars.tgsi_outs,
         gcc->shader->out.count, eot);

   if (!eot) {
      gs_COPY1(tc, gcc->vars.urb_write_header, 0,
            tsrc_from(tdst_d(gcc->vars.tmp)), 0);
   }

   gcc->vars.prim_start = gcc->vars.prim_end;
   gcc->vars.prim_end = false;
}

static void
gs_lower_opcode_emit_so_static(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_inst *inst;
   int i, j;

   if (gcc->static_data.num_vertices_in_prim < gcc->out_vue_min_count)
      return;

   inst = tc_MOV(tc, tdst_w(gcc->vars.tmp), tsrc_imm_v(0x03020100));
   inst->exec_size = GEN6_EXECSIZE_8;
   inst->mask_ctrl = GEN6_MASKCTRL_NOMASK;

   tc_ADD(tc, tdst_d(gcc->vars.tmp), tsrc_from(tdst_d(gcc->vars.tmp)),
         tsrc_rect(tsrc_from(gcc->vars.so_index), TOY_RECT_010));

   tc_IF(tc, tdst_null(),
         tsrc_rect(tsrc_offset(tsrc_from(tdst_d(gcc->vars.tmp)), 0, gcc->out_vue_min_count - 1), TOY_RECT_010),
         tsrc_rect(tsrc_offset(gcc->payload.svbi, 0, 4), TOY_RECT_010),
         GEN6_COND_LE);
   {
      for (i = 0; i < gcc->out_vue_min_count; i++) {
         for (j = 0; j < gcc->so_info->num_outputs; j++) {
            const int idx = gcc->so_info->output[j].register_index;
            struct toy_src index, out;
            int binding_table_index;
            bool write_commit;

            index = tsrc_d(tsrc_offset(tsrc_from(gcc->vars.tmp), 0, i));

            if (i == gcc->out_vue_min_count - 1) {
               out = gcc->vars.tgsi_outs[idx];
            }
            else {
               /* gcc->vars.buffer_cur also points to the first vertex */
               const int buf =
                  (gcc->vars.buffer_cur + i) % gcc->vars.buffer_needed;

               out = tsrc_offset(tsrc_from(gcc->vars.buffers[buf]), idx, 0);
            }

            out = tsrc_offset(out, 0, gcc->so_info->output[j].start_component);

            /*
             * From the Sandy Bridge PRM, volume 4 part 2, page 19:
             *
             *     "The Kernel must do a write commit on the last write to DAP
             *      prior to a URB_WRITE with End of Thread."
             */
            write_commit =
               (gcc->static_data.num_vertices == gcc->static_data.total_vertices &&
                i == gcc->out_vue_min_count - 1 &&
                j == gcc->so_info->num_outputs - 1);


            binding_table_index = ILO_GS_SO_SURFACE(j);

            gs_write_so(gcc, gcc->vars.tmp, index,
                  out, write_commit, binding_table_index);

            /*
             * From the Sandy Bridge PRM, volume 4 part 1, page 168:
             *
             *     "The write commit does not modify the destination register, but
             *      merely clears the dependency associated with the destination
             *      register. Thus, a simple "mov" instruction using the register as a
             *      source is sufficient to wait for the write commit to occur."
             */
            if (write_commit)
               tc_MOV(tc, gcc->vars.tmp, tsrc_from(gcc->vars.tmp));
         }
      }

      /* SONumPrimsWritten occupies the higher word of m0.2 of URB_WRITE */
      tc_ADD(tc, gcc->vars.so_written,
            tsrc_from(gcc->vars.so_written), tsrc_imm_d(1 << 16));
      tc_ADD(tc, gcc->vars.so_index,
            tsrc_from(gcc->vars.so_index), tsrc_imm_d(gcc->out_vue_min_count));
   }
   tc_ENDIF(tc);
}

static void
gs_lower_opcode_emit_static(struct gs_compile_context *gcc,
                            struct toy_inst *inst)
{
   gcc->static_data.num_vertices++;
   gcc->static_data.num_vertices_in_prim++;

   if (gcc->write_so) {
      gs_lower_opcode_emit_so_static(gcc);

      if (gcc->out_vue_min_count > 1 &&
          gcc->static_data.num_vertices != gcc->static_data.total_vertices)
         gs_save_output(gcc, gcc->vars.tgsi_outs);
   }

   if (gcc->write_vue)
      gs_lower_opcode_emit_vue_static(gcc);
}

static void
gs_lower_opcode_emit_dynamic(struct gs_compile_context *gcc,
                             struct toy_inst *inst)
{
   struct toy_compiler *tc = &gcc->tc;

   tc_ADD(tc, gcc->dynamic_data.num_vertices,
         tsrc_from(gcc->dynamic_data.num_vertices), tsrc_imm_d(1));
   tc_ADD(tc, gcc->dynamic_data.num_vertices_in_prim,
         tsrc_from(gcc->dynamic_data.num_vertices_in_prim), tsrc_imm_d(1));

   if (gcc->write_so) {
      gs_lower_opcode_emit_so_dynamic(gcc);

      if (gcc->out_vue_min_count > 1)
         gs_save_output(gcc, gcc->vars.tgsi_outs);
   }

   if (gcc->write_vue)
      gs_lower_opcode_emit_vue_dynamic(gcc);
}

static void
gs_lower_opcode_emit(struct gs_compile_context *gcc, struct toy_inst *inst)
{
   if (gcc->is_static)
      gs_lower_opcode_emit_static(gcc, inst);
   else
      gs_lower_opcode_emit_dynamic(gcc, inst);
}

static void
gs_lower_opcode_tgsi_in(struct gs_compile_context *gcc,
                        struct toy_dst dst, int dim, int idx)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_src attr;
   int slot, reg = -1, subreg;

   slot = toy_tgsi_find_input(&gcc->tgsi, idx);
   if (slot >= 0) {
      int i;

      for (i = 0; i < gcc->variant->u.gs.num_inputs; i++) {
         if (gcc->variant->u.gs.semantic_names[i] ==
               gcc->tgsi.inputs[slot].semantic_name &&
               gcc->variant->u.gs.semantic_indices[i] ==
               gcc->tgsi.inputs[slot].semantic_index) {
            reg = i / 2;
            subreg = (i % 2) * 4;
            break;
         }
      }
   }

   if (reg < 0) {
      tc_MOV(tc, dst, tsrc_imm_f(0.0f));
      return;
   }

   /* fix vertex ordering for GEN6_3DPRIM_TRISTRIP_REVERSE */
   if (gcc->in_vue_count == 3 && dim < 2) {
      struct toy_inst *inst;

      /* get PrimType */
      inst = tc_AND(tc, tdst_d(gcc->vars.tmp),
            tsrc_offset(gcc->payload.header, 0, 2), tsrc_imm_d(0x1f));
      inst->exec_size = GEN6_EXECSIZE_1;
      inst->src[0] = tsrc_rect(inst->src[0], TOY_RECT_010);
      inst->src[1] = tsrc_rect(inst->src[1], TOY_RECT_010);

      inst = tc_CMP(tc, tdst_null(), tsrc_from(tdst_d(gcc->vars.tmp)),
            tsrc_imm_d(GEN6_3DPRIM_TRISTRIP_REVERSE), GEN6_COND_NZ);
      inst->src[0] = tsrc_rect(inst->src[0], TOY_RECT_010);

      attr = tsrc_offset(gcc->payload.vues[dim], reg, subreg);
      inst = tc_MOV(tc, dst, attr);
      inst->pred_ctrl = GEN6_PREDCTRL_NORMAL;

      /* swap IN[0] and IN[1] for GEN6_3DPRIM_TRISTRIP_REVERSE */
      dim = !dim;

      attr = tsrc_offset(gcc->payload.vues[dim], reg, subreg);
      inst = tc_MOV(tc, dst, attr);
      inst->pred_ctrl = GEN6_PREDCTRL_NORMAL;
      inst->pred_inv = true;
   }
   else {
      attr = tsrc_offset(gcc->payload.vues[dim], reg, subreg);
      tc_MOV(tc, dst, attr);
   }


}

static void
gs_lower_opcode_tgsi_imm(struct gs_compile_context *gcc,
                         struct toy_dst dst, int idx)
{
   const uint32_t *imm;
   int ch;

   imm = toy_tgsi_get_imm(&gcc->tgsi, idx, NULL);

   for (ch = 0; ch < 4; ch++) {
      struct toy_inst *inst;

      /* raw moves */
      inst = tc_MOV(&gcc->tc,
            tdst_writemask(tdst_ud(dst), 1 << ch),
            tsrc_imm_ud(imm[ch]));
      inst->access_mode = GEN6_ALIGN_16;
   }
}

static void
gs_lower_opcode_tgsi_direct(struct gs_compile_context *gcc,
                            struct toy_inst *inst)
{
   struct toy_compiler *tc = &gcc->tc;
   int dim, idx;

   assert(inst->src[0].file == TOY_FILE_IMM);
   dim = inst->src[0].val32;

   assert(inst->src[1].file == TOY_FILE_IMM);
   idx = inst->src[1].val32;

   switch (inst->opcode) {
   case TOY_OPCODE_TGSI_IN:
      gs_lower_opcode_tgsi_in(gcc, inst->dst, dim, idx);
      /* fetch all dimensions */
      if (dim == 0) {
         int i;

         for (i = 1; i < gcc->in_vue_count; i++) {
            const int vrf = toy_tgsi_get_vrf(&gcc->tgsi, TGSI_FILE_INPUT, i, idx);
            struct toy_dst dst;

            if (vrf < 0)
               continue;

            dst = tdst(TOY_FILE_VRF, vrf, 0);
            gs_lower_opcode_tgsi_in(gcc, dst, i, idx);
         }
      }
      break;
   case TOY_OPCODE_TGSI_IMM:
      assert(!dim);
      gs_lower_opcode_tgsi_imm(gcc, inst->dst, idx);
      break;
   case TOY_OPCODE_TGSI_CONST:
   case TOY_OPCODE_TGSI_SV:
   default:
      tc_fail(tc, "unhandled TGSI fetch");
      break;
   }

   tc_discard_inst(tc, inst);
}

static void
gs_lower_virtual_opcodes(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct toy_inst *inst;

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case TOY_OPCODE_TGSI_IN:
      case TOY_OPCODE_TGSI_CONST:
      case TOY_OPCODE_TGSI_SV:
      case TOY_OPCODE_TGSI_IMM:
         gs_lower_opcode_tgsi_direct(gcc, inst);
         break;
      case TOY_OPCODE_TGSI_INDIRECT_FETCH:
      case TOY_OPCODE_TGSI_INDIRECT_STORE:
         /* TODO similar to VS */
         tc_fail(tc, "no indirection support");
         tc_discard_inst(tc, inst);
         break;
      case TOY_OPCODE_TGSI_TEX:
      case TOY_OPCODE_TGSI_TXB:
      case TOY_OPCODE_TGSI_TXD:
      case TOY_OPCODE_TGSI_TXL:
      case TOY_OPCODE_TGSI_TXP:
      case TOY_OPCODE_TGSI_TXF:
      case TOY_OPCODE_TGSI_TXQ:
      case TOY_OPCODE_TGSI_TXQ_LZ:
      case TOY_OPCODE_TGSI_TEX2:
      case TOY_OPCODE_TGSI_TXB2:
      case TOY_OPCODE_TGSI_TXL2:
      case TOY_OPCODE_TGSI_SAMPLE:
      case TOY_OPCODE_TGSI_SAMPLE_I:
      case TOY_OPCODE_TGSI_SAMPLE_I_MS:
      case TOY_OPCODE_TGSI_SAMPLE_B:
      case TOY_OPCODE_TGSI_SAMPLE_C:
      case TOY_OPCODE_TGSI_SAMPLE_C_LZ:
      case TOY_OPCODE_TGSI_SAMPLE_D:
      case TOY_OPCODE_TGSI_SAMPLE_L:
      case TOY_OPCODE_TGSI_GATHER4:
      case TOY_OPCODE_TGSI_SVIEWINFO:
      case TOY_OPCODE_TGSI_SAMPLE_POS:
      case TOY_OPCODE_TGSI_SAMPLE_INFO:
         /* TODO similar to VS */
         tc_fail(tc, "no sampling support");
         tc_discard_inst(tc, inst);
         break;
      case TOY_OPCODE_EMIT:
         gs_lower_opcode_emit(gcc, inst);
         tc_discard_inst(tc, inst);
         break;
      case TOY_OPCODE_ENDPRIM:
         gs_lower_opcode_endprim(gcc, inst);
         tc_discard_inst(tc, inst);
         break;
      default:
         break;
      }
   }

   tc_head(tc);
   while ((inst = tc_next(tc)) != NULL) {
      switch (inst->opcode) {
      case TOY_OPCODE_INV:
      case TOY_OPCODE_LOG:
      case TOY_OPCODE_EXP:
      case TOY_OPCODE_SQRT:
      case TOY_OPCODE_RSQ:
      case TOY_OPCODE_SIN:
      case TOY_OPCODE_COS:
      case TOY_OPCODE_FDIV:
      case TOY_OPCODE_POW:
      case TOY_OPCODE_INT_DIV_QUOTIENT:
      case TOY_OPCODE_INT_DIV_REMAINDER:
         toy_compiler_lower_math(tc, inst);
         break;
      case TOY_OPCODE_URB_WRITE:
         toy_compiler_lower_to_send(tc, inst, false, GEN6_SFID_URB);
         break;
      default:
         if (inst->opcode > 127)
            tc_fail(tc, "unhandled virtual opcode");
         break;
      }
   }
}

/**
 * Get the number of (tessellated) primitives generated by this shader.
 * Return false if that is unknown until runtime.
 */
static void
get_num_prims_static(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   const struct toy_inst *inst;
   int num_vertices_in_prim = 0, if_depth = 0, do_depth = 0;
   bool is_static = true;

   tc_head(tc);
   while ((inst = tc_next_no_skip(tc)) != NULL) {
      switch (inst->opcode) {
      case GEN6_OPCODE_IF:
         if_depth++;
         break;
      case GEN6_OPCODE_ENDIF:
         if_depth--;
         break;
      case TOY_OPCODE_DO:
         do_depth++;
         break;
      case GEN6_OPCODE_WHILE:
         do_depth--;
         break;
      case TOY_OPCODE_EMIT:
         if (if_depth || do_depth) {
            is_static = false;
         }
         else {
            gcc->static_data.total_vertices++;

            num_vertices_in_prim++;
            if (num_vertices_in_prim >= gcc->out_vue_min_count)
               gcc->static_data.total_prims++;
         }
         break;
      case TOY_OPCODE_ENDPRIM:
         if (if_depth || do_depth) {
            is_static = false;
         }
         else {
            const int vertidx = gcc->static_data.total_vertices - 1;
            const int idx = vertidx / 32;
            const int subidx = vertidx % 32;

            gcc->static_data.last_vertex[idx] |= 1 << subidx;
            num_vertices_in_prim = 0;
         }
         break;
      default:
         break;
      }

      if (!is_static)
         break;
   }

   gcc->is_static = is_static;
}

/**
 * Compile the shader.
 */
static bool
gs_compile(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct ilo_shader *sh = gcc->shader;

   get_num_prims_static(gcc);

   if (gcc->is_static) {
      tc_head(tc);

      gs_init_vars(gcc);
      gs_ff_sync(gcc, tdst_d(gcc->vars.tmp), tsrc_imm_d(gcc->static_data.total_prims));
      gs_COPY1(tc, gcc->vars.urb_write_header, 0, tsrc_from(tdst_d(gcc->vars.tmp)), 0);
      if (gcc->write_so)
         gs_COPY4(tc, gcc->vars.so_index, 0, tsrc_from(tdst_d(gcc->vars.tmp)), 1);

      tc_tail(tc);
   }
   else {
      tc_fail(tc, "no control flow support");
      return false;
   }

   if (!gcc->write_vue)
      gs_discard(gcc);

   gs_lower_virtual_opcodes(gcc);
   toy_compiler_legalize_for_ra(tc);
   toy_compiler_optimize(tc);
   toy_compiler_allocate_registers(tc,
         gcc->first_free_grf,
         gcc->last_free_grf,
         1);
   toy_compiler_legalize_for_asm(tc);

   if (tc->fail) {
      ilo_err("failed to legalize GS instructions: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_GS) {
      ilo_printf("legalized instructions:\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   sh->kernel = toy_compiler_assemble(tc, &sh->kernel_size);
   if (!sh->kernel)
      return false;

   if (ilo_debug & ILO_DEBUG_GS) {
      ilo_printf("disassembly:\n");
      toy_compiler_disassemble(tc, sh->kernel, sh->kernel_size);
      ilo_printf("\n");
   }

   return true;
}

static bool
gs_compile_passthrough(struct gs_compile_context *gcc)
{
   struct toy_compiler *tc = &gcc->tc;
   struct ilo_shader *sh = gcc->shader;

   gcc->is_static = true;
   gcc->static_data.total_vertices = gcc->in_vue_count;
   gcc->static_data.total_prims = 1;
   gcc->static_data.last_vertex[0] = 1 << (gcc->in_vue_count - 1);

   gs_init_vars(gcc);
   gs_ff_sync(gcc, tdst_d(gcc->vars.tmp), tsrc_imm_d(gcc->static_data.total_prims));
   gs_COPY1(tc, gcc->vars.urb_write_header, 0, tsrc_from(tdst_d(gcc->vars.tmp)), 0);
   if (gcc->write_so)
      gs_COPY4(tc, gcc->vars.so_index, 0, tsrc_from(tdst_d(gcc->vars.tmp)), 1);

   {
      int vert, attr;

      for (vert = 0; vert < gcc->out_vue_min_count; vert++) {
         for (attr = 0; attr < gcc->shader->out.count; attr++) {
            tc_MOV(tc, tdst_from(gcc->vars.tgsi_outs[attr]),
                  tsrc_offset(gcc->payload.vues[vert], attr / 2, (attr % 2) * 4));
         }

         gs_lower_opcode_emit(gcc, NULL);
      }

      gs_lower_opcode_endprim(gcc, NULL);
   }

   if (!gcc->write_vue)
      gs_discard(gcc);

   gs_lower_virtual_opcodes(gcc);

   toy_compiler_legalize_for_ra(tc);
   toy_compiler_optimize(tc);
   toy_compiler_allocate_registers(tc,
         gcc->first_free_grf,
         gcc->last_free_grf,
         1);

   toy_compiler_legalize_for_asm(tc);

   if (tc->fail) {
      ilo_err("failed to translate GS TGSI tokens: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_GS) {
      int i;

      ilo_printf("VUE count %d, VUE size %d\n",
            gcc->in_vue_count, gcc->in_vue_size);
      ilo_printf("%srasterizer discard\n",
            (gcc->variant->u.gs.rasterizer_discard) ? "" : "no ");

      for (i = 0; i < gcc->so_info->num_outputs; i++) {
         ilo_printf("SO[%d] = OUT[%d]\n", i,
               gcc->so_info->output[i].register_index);
      }

      ilo_printf("legalized instructions:\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   sh->kernel = toy_compiler_assemble(tc, &sh->kernel_size);
   if (!sh->kernel) {
      ilo_err("failed to compile GS: %s\n", tc->reason);
      return false;
   }

   if (ilo_debug & ILO_DEBUG_GS) {
      ilo_printf("disassembly:\n");
      toy_compiler_disassemble(tc, sh->kernel, sh->kernel_size);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Translate the TGSI tokens.
 */
static bool
gs_setup_tgsi(struct toy_compiler *tc, const struct tgsi_token *tokens,
              struct toy_tgsi *tgsi)
{
   if (ilo_debug & ILO_DEBUG_GS) {
      ilo_printf("dumping geometry shader\n");
      ilo_printf("\n");

      tgsi_dump(tokens, 0);
      ilo_printf("\n");
   }

   toy_compiler_translate_tgsi(tc, tokens, true, tgsi);
   if (tc->fail)
      return false;

   if (ilo_debug & ILO_DEBUG_GS) {
      ilo_printf("TGSI translator:\n");
      toy_tgsi_dump(tgsi);
      ilo_printf("\n");
      toy_compiler_dump(tc);
      ilo_printf("\n");
   }

   return true;
}

/**
 * Set up shader inputs for fixed-function units.
 */
static void
gs_setup_shader_in(struct ilo_shader *sh,
                   const struct ilo_shader_variant *variant)
{
   int i;

   for (i = 0; i < variant->u.gs.num_inputs; i++) {
      sh->in.semantic_names[i] = variant->u.gs.semantic_names[i];
      sh->in.semantic_indices[i] = variant->u.gs.semantic_indices[i];
      sh->in.interp[i] = TGSI_INTERPOLATE_CONSTANT;
      sh->in.centroid[i] = false;
   }

   sh->in.count = variant->u.gs.num_inputs;

   sh->in.has_pos = false;
   sh->in.has_linear_interp = false;
   sh->in.barycentric_interpolation_mode = 0;
}

/**
 * Set up shader outputs for fixed-function units.
 *
 * XXX share the code with VS
 */
static void
gs_setup_shader_out(struct ilo_shader *sh, const struct toy_tgsi *tgsi,
                    bool output_clipdist, int *output_map)
{
   int psize_slot = -1, pos_slot = -1;
   int clipdist_slot[2] = { -1, -1 };
   int color_slot[4] = { -1, -1, -1, -1 };
   int num_outs, i;

   /* find out the slots of outputs that need special care */
   for (i = 0; i < tgsi->num_outputs; i++) {
      switch (tgsi->outputs[i].semantic_name) {
      case TGSI_SEMANTIC_PSIZE:
         psize_slot = i;
         break;
      case TGSI_SEMANTIC_POSITION:
         pos_slot = i;
         break;
      case TGSI_SEMANTIC_CLIPDIST:
         if (tgsi->outputs[i].semantic_index)
            clipdist_slot[1] = i;
         else
            clipdist_slot[0] = i;
         break;
      case TGSI_SEMANTIC_COLOR:
         if (tgsi->outputs[i].semantic_index)
            color_slot[2] = i;
         else
            color_slot[0] = i;
         break;
      case TGSI_SEMANTIC_BCOLOR:
         if (tgsi->outputs[i].semantic_index)
            color_slot[3] = i;
         else
            color_slot[1] = i;
         break;
      default:
         break;
      }
   }

   /* the first two VUEs are always PSIZE and POSITION */
   num_outs = 2;
   output_map[0] = psize_slot;
   output_map[1] = pos_slot;

   sh->out.register_indices[0] =
      (psize_slot >= 0) ? tgsi->outputs[psize_slot].index : -1;
   sh->out.semantic_names[0] = TGSI_SEMANTIC_PSIZE;
   sh->out.semantic_indices[0] = 0;

   sh->out.register_indices[1] =
      (pos_slot >= 0) ? tgsi->outputs[pos_slot].index : -1;
   sh->out.semantic_names[1] = TGSI_SEMANTIC_POSITION;
   sh->out.semantic_indices[1] = 0;

   sh->out.has_pos = true;

   /* followed by optional clip distances */
   if (output_clipdist) {
      sh->out.register_indices[num_outs] =
         (clipdist_slot[0] >= 0) ? tgsi->outputs[clipdist_slot[0]].index : -1;
      sh->out.semantic_names[num_outs] = TGSI_SEMANTIC_CLIPDIST;
      sh->out.semantic_indices[num_outs] = 0;
      output_map[num_outs++] = clipdist_slot[0];

      sh->out.register_indices[num_outs] =
         (clipdist_slot[1] >= 0) ? tgsi->outputs[clipdist_slot[1]].index : -1;
      sh->out.semantic_names[num_outs] = TGSI_SEMANTIC_CLIPDIST;
      sh->out.semantic_indices[num_outs] = 1;
      output_map[num_outs++] = clipdist_slot[1];
   }

   /*
    * make BCOLOR follow COLOR so that we can make use of
    * ATTRIBUTE_SWIZZLE_INPUTATTR_FACING in 3DSTATE_SF
    */
   for (i = 0; i < 4; i++) {
      const int slot = color_slot[i];

      if (slot < 0)
         continue;

      sh->out.register_indices[num_outs] = tgsi->outputs[slot].index;
      sh->out.semantic_names[num_outs] = tgsi->outputs[slot].semantic_name;
      sh->out.semantic_indices[num_outs] = tgsi->outputs[slot].semantic_index;

      output_map[num_outs++] = slot;
   }

   /* add the rest of the outputs */
   for (i = 0; i < tgsi->num_outputs; i++) {
      switch (tgsi->outputs[i].semantic_name) {
      case TGSI_SEMANTIC_PSIZE:
      case TGSI_SEMANTIC_POSITION:
      case TGSI_SEMANTIC_CLIPDIST:
      case TGSI_SEMANTIC_COLOR:
      case TGSI_SEMANTIC_BCOLOR:
         break;
      default:
         sh->out.register_indices[num_outs] = tgsi->outputs[i].index;
         sh->out.semantic_names[num_outs] = tgsi->outputs[i].semantic_name;
         sh->out.semantic_indices[num_outs] = tgsi->outputs[i].semantic_index;
         output_map[num_outs++] = i;
         break;
      }
   }

   sh->out.count = num_outs;
}

static void
gs_setup_vars(struct gs_compile_context *gcc)
{
   int grf = gcc->first_free_grf;
   int i;

   gcc->vars.urb_write_header = tdst_d(tdst(TOY_FILE_GRF, grf, 0));
   grf++;

   gcc->vars.tmp = tdst(TOY_FILE_GRF, grf, 0);
   grf++;

   if (gcc->write_so) {
      gcc->vars.buffer_needed = gcc->out_vue_min_count - 1;
      for (i = 0; i < gcc->vars.buffer_needed; i++) {
         gcc->vars.buffers[i] = tdst(TOY_FILE_GRF, grf, 0);
         grf += gcc->shader->out.count;
      }

      gcc->vars.so_written = tdst_d(tdst(TOY_FILE_GRF, grf, 0));
      grf++;

      gcc->vars.so_index = tdst_d(tdst(TOY_FILE_GRF, grf, 0));
      grf++;
   }

   gcc->first_free_grf = grf;

   if (!gcc->tgsi.reg_mapping) {
      for (i = 0; i < gcc->shader->out.count; i++)
         gcc->vars.tgsi_outs[i] = tsrc(TOY_FILE_GRF, grf++, 0);

      gcc->first_free_grf = grf;
      return;
   }

   for (i = 0; i < gcc->shader->out.count; i++) {
      const int slot = gcc->output_map[i];
      const int vrf = (slot >= 0) ? toy_tgsi_get_vrf(&gcc->tgsi,
            TGSI_FILE_OUTPUT, 0, gcc->tgsi.outputs[slot].index) : -1;

      if (vrf >= 0)
         gcc->vars.tgsi_outs[i] = tsrc(TOY_FILE_VRF, vrf, 0);
      else
         gcc->vars.tgsi_outs[i] = (i == 0) ? tsrc_imm_d(0) : tsrc_imm_f(0.0f);
   }
}

static void
gs_setup_payload(struct gs_compile_context *gcc)
{
   int grf, i;

   grf = 0;

   /* r0: payload header */
   gcc->payload.header = tsrc_d(tsrc(TOY_FILE_GRF, grf, 0));
   grf++;

   /* r1: SVBI */
   if (gcc->write_so) {
      gcc->payload.svbi = tsrc_ud(tsrc(TOY_FILE_GRF, grf, 0));
      grf++;
   }

   /* URB data */
   gcc->shader->in.start_grf = grf;

   /* no pull constants */

   /* VUEs */
   for (i = 0; i < gcc->in_vue_count; i++) {
      gcc->payload.vues[i] = tsrc(TOY_FILE_GRF, grf, 0);
      grf += gcc->in_vue_size;
   }

   gcc->first_free_grf = grf;
   gcc->last_free_grf = 127;
}

/**
 * Set up GS compile context.  This includes translating the TGSI tokens.
 */
static bool
gs_setup(struct gs_compile_context *gcc,
         const struct ilo_shader_state *state,
         const struct ilo_shader_variant *variant,
         int num_verts)
{
   memset(gcc, 0, sizeof(*gcc));

   gcc->shader = CALLOC_STRUCT(ilo_shader);
   if (!gcc->shader)
      return false;

   gcc->variant = variant;
   gcc->so_info = &state->info.stream_output;

   toy_compiler_init(&gcc->tc, state->info.dev);

   gcc->write_so = (state->info.stream_output.num_outputs > 0);
   gcc->write_vue = !gcc->variant->u.gs.rasterizer_discard;

   gcc->tc.templ.access_mode = GEN6_ALIGN_16;
   gcc->tc.templ.exec_size = GEN6_EXECSIZE_4;
   gcc->tc.rect_linear_width = 4;

   if (state->info.tokens) {
      if (!gs_setup_tgsi(&gcc->tc, state->info.tokens, &gcc->tgsi)) {
         toy_compiler_cleanup(&gcc->tc);
         FREE(gcc->shader);
         return false;
      }

      switch (gcc->tgsi.props.gs_input_prim) {
      case PIPE_PRIM_POINTS:
         gcc->in_vue_count = 1;
         break;
      case PIPE_PRIM_LINES:
         gcc->in_vue_count = 2;
         gcc->shader->in.discard_adj = true;
         break;
      case PIPE_PRIM_TRIANGLES:
         gcc->in_vue_count = 3;
         gcc->shader->in.discard_adj = true;
         break;
      case PIPE_PRIM_LINES_ADJACENCY:
         gcc->in_vue_count = 4;
         break;
      case PIPE_PRIM_TRIANGLES_ADJACENCY:
         gcc->in_vue_count = 6;
         break;
      default:
         tc_fail(&gcc->tc, "unsupported GS input type");
         gcc->in_vue_count = 0;
         break;
      }

      switch (gcc->tgsi.props.gs_output_prim) {
      case PIPE_PRIM_POINTS:
         gcc->out_vue_min_count = 1;
         break;
      case PIPE_PRIM_LINE_STRIP:
         gcc->out_vue_min_count = 2;
         break;
      case PIPE_PRIM_TRIANGLE_STRIP:
         gcc->out_vue_min_count = 3;
         break;
      default:
         tc_fail(&gcc->tc, "unsupported GS output type");
         gcc->out_vue_min_count = 0;
         break;
      }
   }
   else {
      int i;

      gcc->in_vue_count = num_verts;
      gcc->out_vue_min_count = num_verts;

      gcc->tgsi.num_outputs = gcc->variant->u.gs.num_inputs;
      for (i = 0; i < gcc->variant->u.gs.num_inputs; i++) {
         gcc->tgsi.outputs[i].semantic_name =
            gcc->variant->u.gs.semantic_names[i];
         gcc->tgsi.outputs[i].semantic_index =
            gcc->variant->u.gs.semantic_indices[i];
      }
   }

   gcc->tc.templ.access_mode = GEN6_ALIGN_1;

   gs_setup_shader_in(gcc->shader, gcc->variant);
   gs_setup_shader_out(gcc->shader, &gcc->tgsi, false, gcc->output_map);

   gcc->in_vue_size = (gcc->shader->in.count + 1) / 2;

   gcc->out_vue_size = (gcc->shader->out.count + 1) / 2;

   gs_setup_payload(gcc);
   gs_setup_vars(gcc);

   /* m0 is reserved for system routines */
   gcc->first_free_mrf = 1;
   gcc->last_free_mrf = 15;

   return true;
}

/**
 * Compile the geometry shader.
 */
struct ilo_shader *
ilo_shader_compile_gs(const struct ilo_shader_state *state,
                      const struct ilo_shader_variant *variant)
{
   struct gs_compile_context gcc;

   if (!gs_setup(&gcc, state, variant, 0))
      return NULL;

   if (!gs_compile(&gcc)) {
      FREE(gcc.shader);
      gcc.shader = NULL;
   }

   toy_tgsi_cleanup(&gcc.tgsi);
   toy_compiler_cleanup(&gcc.tc);

   return gcc.shader;;
}

static bool
append_gs_to_vs(struct ilo_shader *vs, struct ilo_shader *gs, int num_verts)
{
   void *combined;
   int gs_offset;

   if (!gs)
      return false;

   /* kernels must be aligned to 64-byte */
   gs_offset = align(vs->kernel_size, 64);
   combined = REALLOC(vs->kernel, vs->kernel_size,
         gs_offset + gs->kernel_size);
   if (!combined)
      return false;

   memcpy(combined + gs_offset, gs->kernel, gs->kernel_size);

   vs->kernel = combined;
   vs->kernel_size = gs_offset + gs->kernel_size;

   vs->stream_output = true;
   vs->gs_offsets[num_verts - 1] = gs_offset;
   vs->gs_start_grf = gs->in.start_grf;

   ilo_shader_destroy_kernel(gs);

   return true;
}

bool
ilo_shader_compile_gs_passthrough(const struct ilo_shader_state *vs_state,
                                  const struct ilo_shader_variant *vs_variant,
                                  const int *so_mapping,
                                  struct ilo_shader *vs)
{
   struct gs_compile_context gcc;
   struct ilo_shader_state state;
   struct ilo_shader_variant variant;
   const int num_verts = 3;
   int i;

   /* init GS state and variant */
   state = *vs_state;
   state.info.tokens = NULL;
   for (i = 0; i < state.info.stream_output.num_outputs; i++) {
      const int reg = state.info.stream_output.output[i].register_index;

      state.info.stream_output.output[i].register_index = so_mapping[reg];
   }

   variant = *vs_variant;
   variant.u.gs.rasterizer_discard = vs_variant->u.vs.rasterizer_discard;
   variant.u.gs.num_inputs = vs->out.count;
   for (i = 0; i < vs->out.count; i++) {
      variant.u.gs.semantic_names[i] =
         vs->out.semantic_names[i];
      variant.u.gs.semantic_indices[i] =
         vs->out.semantic_indices[i];
   }

   if (!gs_setup(&gcc, &state, &variant, num_verts))
      return false;

   if (!gs_compile_passthrough(&gcc)) {
      FREE(gcc.shader);
      gcc.shader = NULL;
   }

   /* no need to call toy_tgsi_cleanup() */
   toy_compiler_cleanup(&gcc.tc);

   return append_gs_to_vs(vs, gcc.shader, num_verts);
}
