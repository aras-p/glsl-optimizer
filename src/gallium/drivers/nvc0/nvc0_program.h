
#ifndef __NVC0_PROGRAM_H__
#define __NVC0_PROGRAM_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

#define NVC0_CAP_MAX_PROGRAM_TEMPS 64

#define NVC0_SHADER_HEADER_SIZE (20 * 4)

struct nvc0_program {
   struct pipe_shader_state pipe;

   ubyte type;
   boolean translated;
   ubyte max_gpr;

   uint32_t *code;
   unsigned code_base;
   unsigned code_size;
   unsigned parm_size;

   uint32_t hdr[20]; /* TODO: move this into code to save space */

   uint32_t flags[2];

   struct {
      uint8_t edgeflag;
      uint8_t num_ucps;
      uint8_t out_pos[PIPE_MAX_SHADER_OUTPUTS];
   } vp;
   struct {
      uint8_t early_z;
      uint8_t in_pos[PIPE_MAX_SHADER_INPUTS];
   } fp;

   void *relocs;
   unsigned num_relocs;

   struct nouveau_resource *res;
};

/* first 2 bits are written into the program header, for each input */
#define NVC0_INTERP_FLAT          (1 << 0)
#define NVC0_INTERP_PERSPECTIVE   (2 << 0)
#define NVC0_INTERP_LINEAR        (3 << 0)
#define NVC0_INTERP_CENTROID      (1 << 2)

/* analyze TGSI and see which TEMP[] are used as subroutine inputs/outputs */
struct nvc0_subroutine {
   unsigned id;
   unsigned first_insn;
   uint32_t argv[NVC0_CAP_MAX_PROGRAM_TEMPS][4];
   uint32_t retv[NVC0_CAP_MAX_PROGRAM_TEMPS][4];
};

struct nvc0_translation_info {
   struct nvc0_program *prog;
   struct tgsi_full_instruction *insns;
   unsigned num_insns;
   ubyte input_file;
   ubyte output_file;
   ubyte fp_depth_output;
   uint16_t input_loc[PIPE_MAX_SHADER_INPUTS][4];
   uint16_t output_loc[PIPE_MAX_SHADER_OUTPUTS][4];
   uint16_t sysval_loc[TGSI_SEMANTIC_COUNT];
   boolean sysval_in[TGSI_SEMANTIC_COUNT];
   int input_access[PIPE_MAX_SHADER_INPUTS][4];
   int output_access[PIPE_MAX_SHADER_OUTPUTS][4];
   ubyte interp_mode[PIPE_MAX_SHADER_INPUTS];
   boolean indirect_inputs;
   boolean indirect_outputs;
   boolean require_stores;
   boolean global_stores;
   uint32_t *immd32;
   ubyte *immd32_ty;
   unsigned immd32_nr;
   unsigned temp128_nr;
   ubyte edgeflag_out;
   struct nvc0_subroutine *subr;
   unsigned num_subrs;
   boolean append_ucp;
   struct tgsi_shader_info scan;
};

int nvc0_generate_code(struct nvc0_translation_info *);

void nvc0_relocate_program(struct nvc0_program *,
                           uint32_t code_base, uint32_t data_base);

#endif
