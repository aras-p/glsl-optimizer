/*
 * Copyright 2010 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NV50_PROG_H__
#define __NV50_PROG_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

#define NV50_CAP_MAX_PROGRAM_TEMPS 64

struct nv50_varying {
   uint8_t id; /* tgsi index */
   uint8_t hw; /* hw index, nv50 wants flat FP inputs last */

   unsigned mask   : 4;
   unsigned linear : 1;
   unsigned pad    : 3;

   ubyte sn; /* semantic name */
   ubyte si; /* semantic index */
};

struct nv50_program {
   struct pipe_shader_state pipe;

   ubyte type;
   boolean translated;
   boolean uses_lmem;

   uint32_t *code;
   unsigned code_size;
   unsigned code_base;
   uint32_t *immd;
   unsigned immd_size;
   unsigned parm_size; /* size limit of uniform buffer */

   ubyte max_gpr; /* REG_ALLOC_TEMP */
   ubyte max_out; /* REG_ALLOC_RESULT or FP_RESULT_COUNT */

   ubyte in_nr;
   ubyte out_nr;
   struct nv50_varying in[16];
   struct nv50_varying out[16];

   struct {
      uint32_t attrs[3]; /* VP_ATTR_EN_0,1 and VP_GP_BUILTIN_ATTR_EN */
      ubyte psiz;
      ubyte bfc[2];
      ubyte edgeflag;
      ubyte clpd;
      ubyte clpd_nr;
   } vp;

   struct {
      uint32_t flags[2]; /* 0x19a8, 196c */
      uint32_t interp; /* 0x1988 */
      uint32_t colors; /* 0x1904 */
   } fp;

   struct {
      ubyte primid; /* primitive id output register */
      uint8_t vert_count;
      uint8_t prim_type; /* point, line strip or tri strip */
   } gp;

   /* relocation records */
   void *fixups;
   unsigned num_fixups;

   struct nouveau_heap *mem;
};

#define NV50_INTERP_LINEAR   (1 << 0)
#define NV50_INTERP_FLAT     (1 << 1)
#define NV50_INTERP_CENTROID (1 << 2)

/* analyze TGSI and see which TEMP[] are used as subroutine inputs/outputs */
struct nv50_subroutine {
   unsigned id;
   unsigned pos;
   /* function inputs and outputs */
   uint32_t argv[NV50_CAP_MAX_PROGRAM_TEMPS][4];
   uint32_t retv[NV50_CAP_MAX_PROGRAM_TEMPS][4];
};

struct nv50_translation_info {
   struct nv50_program *p;
   unsigned inst_nr;
   struct tgsi_full_instruction *insns;
   ubyte input_file;
   ubyte output_file;
   ubyte input_map[PIPE_MAX_SHADER_INPUTS][4];
   ubyte output_map[PIPE_MAX_SHADER_OUTPUTS][4];
   ubyte sysval_map[TGSI_SEMANTIC_COUNT];
   ubyte interp_mode[PIPE_MAX_SHADER_INPUTS];
   int input_access[PIPE_MAX_SHADER_INPUTS][4];
   int output_access[PIPE_MAX_SHADER_OUTPUTS][4];
   boolean indirect_inputs;
   boolean indirect_outputs;
   boolean store_to_memory;
   struct tgsi_shader_info scan;
   uint32_t *immd32;
   unsigned immd32_nr;
   ubyte *immd32_ty;
   ubyte edgeflag_out;
   struct nv50_subroutine *subr;
   unsigned subr_nr;
};

int nv50_generate_code(struct nv50_translation_info *ti);

void nv50_relocate_program(struct nv50_program *p,
                           uint32_t code_base, uint32_t data_base);

boolean nv50_program_tx(struct nv50_program *p);

#endif /* __NV50_PROG_H__ */
