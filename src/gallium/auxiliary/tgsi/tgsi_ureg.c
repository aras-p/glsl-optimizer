/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE, INC AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_sanity.h"
#include "util/u_memory.h"
#include "util/u_math.h"

union tgsi_any_token {
   struct tgsi_version version;
   struct tgsi_header header;
   struct tgsi_processor processor;
   struct tgsi_token token;
   struct tgsi_declaration decl;
   struct tgsi_declaration_range decl_range;
   struct tgsi_declaration_semantic decl_semantic;
   struct tgsi_immediate imm;
   union  tgsi_immediate_data imm_data;
   struct tgsi_instruction insn;
   struct tgsi_instruction_ext_label insn_ext_label;
   struct tgsi_instruction_ext_texture insn_ext_texture;
   struct tgsi_instruction_ext_predicate insn_ext_predicate;
   struct tgsi_src_register src;
   struct tgsi_src_register_ext_mod src_ext_mod;
   struct tgsi_dimension dim;
   struct tgsi_dst_register dst;
   struct tgsi_dst_register_ext_modulate dst_ext_mod;
   unsigned value;
};


struct ureg_tokens {
   union tgsi_any_token *tokens;
   unsigned size;
   unsigned order;
   unsigned count;
};

#define UREG_MAX_INPUT PIPE_MAX_ATTRIBS
#define UREG_MAX_OUTPUT PIPE_MAX_ATTRIBS
#define UREG_MAX_CONSTANT_RANGE 32
#define UREG_MAX_IMMEDIATE 32
#define UREG_MAX_TEMP 256
#define UREG_MAX_ADDR 2
#define UREG_MAX_PRED 1

#define DOMAIN_DECL 0
#define DOMAIN_INSN 1

struct ureg_program
{
   unsigned processor;
   struct pipe_context *pipe;

   struct {
      unsigned semantic_name;
      unsigned semantic_index;
      unsigned interp;
   } fs_input[UREG_MAX_INPUT];
   unsigned nr_fs_inputs;

   unsigned vs_inputs[UREG_MAX_INPUT/32];

   struct {
      unsigned semantic_name;
      unsigned semantic_index;
   } output[UREG_MAX_OUTPUT];
   unsigned nr_outputs;

   struct {
      float v[4];
      unsigned nr;
   } immediate[UREG_MAX_IMMEDIATE];
   unsigned nr_immediates;

   struct ureg_src sampler[PIPE_MAX_SAMPLERS];
   unsigned nr_samplers;

   unsigned temps_active[UREG_MAX_TEMP / 32];
   unsigned nr_temps;

   struct {
      unsigned first;
      unsigned last;
   } constant_range[UREG_MAX_CONSTANT_RANGE];
   unsigned nr_constant_ranges;

   unsigned nr_addrs;
   unsigned nr_preds;
   unsigned nr_instructions;

   struct ureg_tokens domain[2];
};

static union tgsi_any_token error_tokens[32];

static void tokens_error( struct ureg_tokens *tokens )
{
   if (tokens->tokens && tokens->tokens != error_tokens)
      FREE(tokens->tokens);

   tokens->tokens = error_tokens;
   tokens->size = Elements(error_tokens);
   tokens->count = 0;
}


static void tokens_expand( struct ureg_tokens *tokens,
                           unsigned count )
{
   unsigned old_size = tokens->size * sizeof(unsigned);

   if (tokens->tokens == error_tokens) {
      return;
   }

   while (tokens->count + count > tokens->size) {
      tokens->size = (1 << ++tokens->order);
   }

   tokens->tokens = REALLOC(tokens->tokens, 
                            old_size,
                            tokens->size * sizeof(unsigned));
   if (tokens->tokens == NULL) {
      tokens_error(tokens);
   }
}

static void set_bad( struct ureg_program *ureg )
{
   tokens_error(&ureg->domain[0]);
}



static union tgsi_any_token *get_tokens( struct ureg_program *ureg,
                                         unsigned domain,
                                         unsigned count )
{
   struct ureg_tokens *tokens = &ureg->domain[domain];
   union tgsi_any_token *result;

   if (tokens->count + count > tokens->size) 
      tokens_expand(tokens, count);

   result = &tokens->tokens[tokens->count];
   tokens->count += count;
   return result;
}


static union tgsi_any_token *retrieve_token( struct ureg_program *ureg,
                                            unsigned domain,
                                            unsigned nr )
{
   if (ureg->domain[domain].tokens == error_tokens)
      return &error_tokens[0];

   return &ureg->domain[domain].tokens[nr];
}



static INLINE struct ureg_dst
ureg_dst_register( unsigned file,
                   unsigned index )
{
   struct ureg_dst dst;

   dst.File      = file;
   dst.WriteMask = TGSI_WRITEMASK_XYZW;
   dst.Indirect  = 0;
   dst.IndirectIndex = 0;
   dst.IndirectSwizzle = 0;
   dst.Saturate  = 0;
   dst.Predicate = 0;
   dst.PredNegate = 0;
   dst.PredSwizzleX = TGSI_SWIZZLE_X;
   dst.PredSwizzleY = TGSI_SWIZZLE_Y;
   dst.PredSwizzleZ = TGSI_SWIZZLE_Z;
   dst.PredSwizzleW = TGSI_SWIZZLE_W;
   dst.Index     = index;

   return dst;
}

static INLINE struct ureg_src 
ureg_src_register( unsigned file,
                   unsigned index )
{
   struct ureg_src src;

   src.File     = file;
   src.SwizzleX = TGSI_SWIZZLE_X;
   src.SwizzleY = TGSI_SWIZZLE_Y;
   src.SwizzleZ = TGSI_SWIZZLE_Z;
   src.SwizzleW = TGSI_SWIZZLE_W;
   src.Pad      = 0;
   src.Indirect = 0;
   src.IndirectIndex = 0;
   src.IndirectSwizzle = 0;
   src.Absolute = 0;
   src.Index    = index;
   src.Negate   = 0;

   return src;
}




struct ureg_src 
ureg_DECL_fs_input( struct ureg_program *ureg,
                    unsigned name,
                    unsigned index,
                    unsigned interp_mode )
{
   unsigned i;

   for (i = 0; i < ureg->nr_fs_inputs; i++) {
      if (ureg->fs_input[i].semantic_name == name &&
          ureg->fs_input[i].semantic_index == index) 
         goto out;
   }

   if (ureg->nr_fs_inputs < UREG_MAX_INPUT) {
      ureg->fs_input[i].semantic_name = name;
      ureg->fs_input[i].semantic_index = index;
      ureg->fs_input[i].interp = interp_mode;
      ureg->nr_fs_inputs++;
   }
   else {
      set_bad( ureg );
   }

out:
   return ureg_src_register( TGSI_FILE_INPUT, i );
}


struct ureg_src 
ureg_DECL_vs_input( struct ureg_program *ureg,
                    unsigned index )
{
   assert(ureg->processor == TGSI_PROCESSOR_VERTEX);
   
   ureg->vs_inputs[index/32] |= 1 << (index % 32);
   return ureg_src_register( TGSI_FILE_INPUT, index );
}


struct ureg_dst 
ureg_DECL_output( struct ureg_program *ureg,
                  unsigned name,
                  unsigned index )
{
   unsigned i;

   for (i = 0; i < ureg->nr_outputs; i++) {
      if (ureg->output[i].semantic_name == name &&
          ureg->output[i].semantic_index == index) 
         goto out;
   }

   if (ureg->nr_outputs < UREG_MAX_OUTPUT) {
      ureg->output[i].semantic_name = name;
      ureg->output[i].semantic_index = index;
      ureg->nr_outputs++;
   }
   else {
      set_bad( ureg );
   }

out:
   return ureg_dst_register( TGSI_FILE_OUTPUT, i );
}


/* Returns a new constant register.  Keep track of which have been
 * referred to so that we can emit decls later.
 *
 * There is nothing in this code to bind this constant to any tracked
 * value or manage any constant_buffer contents -- that's the
 * resposibility of the calling code.
 */
struct ureg_src ureg_DECL_constant(struct ureg_program *ureg, 
                                   unsigned index )
{
   unsigned minconst = index, maxconst = index;
   unsigned i;

   /* Inside existing range?
    */
   for (i = 0; i < ureg->nr_constant_ranges; i++) {
      if (ureg->constant_range[i].first <= index &&
          ureg->constant_range[i].last >= index)
         goto out;
   }

   /* Extend existing range?
    */
   for (i = 0; i < ureg->nr_constant_ranges; i++) {
      if (ureg->constant_range[i].last == index - 1) {
         ureg->constant_range[i].last = index;
         goto out;
      }

      if (ureg->constant_range[i].first == index + 1) {
         ureg->constant_range[i].first = index;
         goto out;
      }

      minconst = MIN2(minconst, ureg->constant_range[i].first);
      maxconst = MAX2(maxconst, ureg->constant_range[i].last);
   }

   /* Create new range?
    */
   if (ureg->nr_constant_ranges < UREG_MAX_CONSTANT_RANGE) {
      i = ureg->nr_constant_ranges++;
      ureg->constant_range[i].first = index;
      ureg->constant_range[i].last = index;
   }

   /* Collapse all ranges down to one:
    */
   i = 0;
   ureg->constant_range[0].first = minconst;
   ureg->constant_range[0].last = maxconst;
   ureg->nr_constant_ranges = 1;

out:
   assert(i < ureg->nr_constant_ranges);
   assert(ureg->constant_range[i].first <= index);
   assert(ureg->constant_range[i].last >= index);
   return ureg_src_register( TGSI_FILE_CONSTANT, index );
}


/* Allocate a new temporary.  Temporaries greater than UREG_MAX_TEMP
 * are legal, but will not be released.
 */
struct ureg_dst ureg_DECL_temporary( struct ureg_program *ureg )
{
   unsigned i;

   for (i = 0; i < UREG_MAX_TEMP; i += 32) {
      int bit = ffs(~ureg->temps_active[i/32]);
      if (bit != 0) {
         i += bit - 1;
         goto out;
      }
   }

   /* No reusable temps, so allocate a new one:
    */
   i = ureg->nr_temps++;

out:
   if (i < UREG_MAX_TEMP)
      ureg->temps_active[i/32] |= 1 << (i % 32);

   if (i >= ureg->nr_temps)
      ureg->nr_temps = i + 1;

   return ureg_dst_register( TGSI_FILE_TEMPORARY, i );
}


void ureg_release_temporary( struct ureg_program *ureg,
                             struct ureg_dst tmp )
{
   if(tmp.File == TGSI_FILE_TEMPORARY)
      if (tmp.Index < UREG_MAX_TEMP)
         ureg->temps_active[tmp.Index/32] &= ~(1 << (tmp.Index % 32));
}


/* Allocate a new address register.
 */
struct ureg_dst ureg_DECL_address( struct ureg_program *ureg )
{
   if (ureg->nr_addrs < UREG_MAX_ADDR)
      return ureg_dst_register( TGSI_FILE_ADDRESS, ureg->nr_addrs++ );

   assert( 0 );
   return ureg_dst_register( TGSI_FILE_ADDRESS, 0 );
}

/* Allocate a new predicate register.
 */
struct ureg_dst
ureg_DECL_predicate(struct ureg_program *ureg)
{
   if (ureg->nr_preds < UREG_MAX_PRED) {
      return ureg_dst_register(TGSI_FILE_PREDICATE, ureg->nr_preds++);
   }

   assert(0);
   return ureg_dst_register(TGSI_FILE_PREDICATE, 0);
}

/* Allocate a new sampler.
 */
struct ureg_src ureg_DECL_sampler( struct ureg_program *ureg,
                                   unsigned nr )
{
   unsigned i;

   for (i = 0; i < ureg->nr_samplers; i++)
      if (ureg->sampler[i].Index == nr)
         return ureg->sampler[i];
   
   if (i < PIPE_MAX_SAMPLERS) {
      ureg->sampler[i] = ureg_src_register( TGSI_FILE_SAMPLER, nr );
      ureg->nr_samplers++;
      return ureg->sampler[i];
   }

   assert( 0 );
   return ureg->sampler[0];
}




static int match_or_expand_immediate( const float *v,
                                      unsigned nr,
                                      float *v2,
                                      unsigned *nr2,
                                      unsigned *swizzle )
{
   unsigned i, j;
   
   *swizzle = 0;

   for (i = 0; i < nr; i++) {
      boolean found = FALSE;

      for (j = 0; j < *nr2 && !found; j++) {
         if (v[i] == v2[j]) {
            *swizzle |= j << (i * 2);
            found = TRUE;
         }
      }

      if (!found) {
         if (*nr2 >= 4) 
            return FALSE;

         v2[*nr2] = v[i];
         *swizzle |= *nr2 << (i * 2);
         (*nr2)++;
      }
   }

   return TRUE;
}




struct ureg_src ureg_DECL_immediate( struct ureg_program *ureg, 
                                     const float *v,
                                     unsigned nr )
{
   unsigned i, j;
   unsigned swizzle;

   /* Could do a first pass where we examine all existing immediates
    * without expanding.
    */

   for (i = 0; i < ureg->nr_immediates; i++) {
      if (match_or_expand_immediate( v, 
                                     nr,
                                     ureg->immediate[i].v,
                                     &ureg->immediate[i].nr, 
                                     &swizzle ))
         goto out;
   }

   if (ureg->nr_immediates < UREG_MAX_IMMEDIATE) {
      i = ureg->nr_immediates++;
      if (match_or_expand_immediate( v,
                                     nr,
                                     ureg->immediate[i].v,
                                     &ureg->immediate[i].nr, 
                                     &swizzle ))
         goto out;
   }

   set_bad( ureg );

out:
   /* Make sure that all referenced elements are from this immediate.
    * Has the effect of making size-one immediates into scalars.
    */
   for (j = nr; j < 4; j++)
      swizzle |= (swizzle & 0x3) << (j * 2);

   return ureg_swizzle( ureg_src_register( TGSI_FILE_IMMEDIATE, i ),
                        (swizzle >> 0) & 0x3,
                        (swizzle >> 2) & 0x3,
                        (swizzle >> 4) & 0x3,
                        (swizzle >> 6) & 0x3);
}


void 
ureg_emit_src( struct ureg_program *ureg,
               struct ureg_src src )
{
   unsigned size = (1 + 
                    (src.Absolute ? 1 : 0) +
                    (src.Indirect ? 1 : 0));

   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_INSN, size );
   unsigned n = 0;

   assert(src.File != TGSI_FILE_NULL);
   assert(src.File != TGSI_FILE_OUTPUT);
   assert(src.File < TGSI_FILE_COUNT);
   
   out[n].value = 0;
   out[n].src.File = src.File;
   out[n].src.SwizzleX = src.SwizzleX;
   out[n].src.SwizzleY = src.SwizzleY;
   out[n].src.SwizzleZ = src.SwizzleZ;
   out[n].src.SwizzleW = src.SwizzleW;
   out[n].src.Index = src.Index;
   out[n].src.Negate = src.Negate;
   n++;
   
   if (src.Absolute) {
      out[0].src.Extended = 1;
      out[0].src.Negate = 0;
      out[n].value = 0;
      out[n].src_ext_mod.Type = TGSI_SRC_REGISTER_EXT_TYPE_MOD;
      out[n].src_ext_mod.Absolute = 1;
      out[n].src_ext_mod.Negate = src.Negate;
      n++;
   }

   if (src.Indirect) {
      out[0].src.Indirect = 1;
      out[n].value = 0;
      out[n].src.File = TGSI_FILE_ADDRESS;
      out[n].src.SwizzleX = src.IndirectSwizzle;
      out[n].src.SwizzleY = src.IndirectSwizzle;
      out[n].src.SwizzleZ = src.IndirectSwizzle;
      out[n].src.SwizzleW = src.IndirectSwizzle;
      out[n].src.Index = src.IndirectIndex;
      n++;
   }

   assert(n == size);
}


void 
ureg_emit_dst( struct ureg_program *ureg,
               struct ureg_dst dst )
{
   unsigned size = (1 + 
                    (dst.Indirect ? 1 : 0));

   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_INSN, size );
   unsigned n = 0;

   assert(dst.File != TGSI_FILE_NULL);
   assert(dst.File != TGSI_FILE_CONSTANT);
   assert(dst.File != TGSI_FILE_INPUT);
   assert(dst.File != TGSI_FILE_SAMPLER);
   assert(dst.File != TGSI_FILE_IMMEDIATE);
   assert(dst.File < TGSI_FILE_COUNT);
   
   out[n].value = 0;
   out[n].dst.File = dst.File;
   out[n].dst.WriteMask = dst.WriteMask;
   out[n].dst.Indirect = dst.Indirect;
   out[n].dst.Index = dst.Index;
   n++;
   
   if (dst.Indirect) {
      out[n].value = 0;
      out[n].src.File = TGSI_FILE_ADDRESS;
      out[n].src.SwizzleX = dst.IndirectSwizzle;
      out[n].src.SwizzleY = dst.IndirectSwizzle;
      out[n].src.SwizzleZ = dst.IndirectSwizzle;
      out[n].src.SwizzleW = dst.IndirectSwizzle;
      out[n].src.Index = dst.IndirectIndex;
      n++;
   }

   assert(n == size);
}


static void validate( unsigned opcode,
                      unsigned nr_dst,
                      unsigned nr_src )
{
#ifdef DEBUG
   const struct tgsi_opcode_info *info = tgsi_get_opcode_info( opcode );
   assert(info);
   if(info) {
      assert(nr_dst == info->num_dst);
      assert(nr_src == info->num_src);
   }
#endif
}

struct ureg_emit_insn_result
ureg_emit_insn(struct ureg_program *ureg,
               unsigned opcode,
               boolean saturate,
               boolean predicate,
               boolean pred_negate,
               unsigned pred_swizzle_x,
               unsigned pred_swizzle_y,
               unsigned pred_swizzle_z,
               unsigned pred_swizzle_w,
               unsigned num_dst,
               unsigned num_src )
{
   union tgsi_any_token *out;
   uint count = predicate ? 2 : 1;
   struct ureg_emit_insn_result result;

   validate( opcode, num_dst, num_src );
   
   out = get_tokens( ureg, DOMAIN_INSN, count );
   out[0].value = 0;
   out[0].insn.Type = TGSI_TOKEN_TYPE_INSTRUCTION;
   out[0].insn.NrTokens = 0;
   out[0].insn.Opcode = opcode;
   out[0].insn.Saturate = saturate;
   out[0].insn.NumDstRegs = num_dst;
   out[0].insn.NumSrcRegs = num_src;
   out[0].insn.Padding = 0;

   result.insn_token = ureg->domain[DOMAIN_INSN].count - count;

   if (predicate) {
      out[0].insn.Extended = 1;
      out[1].insn_ext_predicate = tgsi_default_instruction_ext_predicate();
      out[1].insn_ext_predicate.Negate = pred_negate;
      out[1].insn_ext_predicate.SwizzleX = pred_swizzle_x;
      out[1].insn_ext_predicate.SwizzleY = pred_swizzle_y;
      out[1].insn_ext_predicate.SwizzleZ = pred_swizzle_z;
      out[1].insn_ext_predicate.SwizzleW = pred_swizzle_w;

      result.extended_token = result.insn_token + 1;
   } else {
      out[0].insn.Extended = 0;

      result.extended_token = result.insn_token;
   }

   ureg->nr_instructions++;
   
   return result;
}


void
ureg_emit_label(struct ureg_program *ureg,
                unsigned extended_token,
                unsigned *label_token )
{
   union tgsi_any_token *out, *insn;

   if(!label_token)
      return;

   out = get_tokens( ureg, DOMAIN_INSN, 1 );
   insn = retrieve_token( ureg, DOMAIN_INSN, extended_token );

   insn->token.Extended = 1;

   out[0].value = 0;
   out[0].insn_ext_label.Type = TGSI_INSTRUCTION_EXT_TYPE_LABEL;
   
   *label_token = ureg->domain[DOMAIN_INSN].count - 1;
}

/* Will return a number which can be used in a label to point to the
 * next instruction to be emitted.
 */
unsigned
ureg_get_instruction_number( struct ureg_program *ureg )
{
   return ureg->nr_instructions;
}

/* Patch a given label (expressed as a token number) to point to a
 * given instruction (expressed as an instruction number).
 */
void
ureg_fixup_label(struct ureg_program *ureg,
                 unsigned label_token,
                 unsigned instruction_number )
{
   union tgsi_any_token *out = retrieve_token( ureg, DOMAIN_INSN, label_token );

   assert(out->insn_ext_label.Type == TGSI_INSTRUCTION_EXT_TYPE_LABEL);
   out->insn_ext_label.Label = instruction_number;
}


void
ureg_emit_texture(struct ureg_program *ureg,
                  unsigned extended_token,
                  unsigned target )
{
   union tgsi_any_token *out, *insn;

   out = get_tokens( ureg, DOMAIN_INSN, 1 );
   insn = retrieve_token( ureg, DOMAIN_INSN, extended_token );

   insn->token.Extended = 1;

   out[0].value = 0;
   out[0].insn_ext_texture.Type = TGSI_INSTRUCTION_EXT_TYPE_TEXTURE;
   out[0].insn_ext_texture.Texture = target;
}


void
ureg_fixup_insn_size(struct ureg_program *ureg,
                     unsigned insn )
{
   union tgsi_any_token *out = retrieve_token( ureg, DOMAIN_INSN, insn );

   assert(out->insn.Type == TGSI_TOKEN_TYPE_INSTRUCTION);
   out->insn.NrTokens = ureg->domain[DOMAIN_INSN].count - insn - 1;
}


void
ureg_insn(struct ureg_program *ureg,
          unsigned opcode,
          const struct ureg_dst *dst,
          unsigned nr_dst,
          const struct ureg_src *src,
          unsigned nr_src )
{
   struct ureg_emit_insn_result insn;
   unsigned i;
   boolean saturate;
   boolean predicate;
   boolean negate;
   unsigned swizzle[4];

   saturate = nr_dst ? dst[0].Saturate : FALSE;
   predicate = nr_dst ? dst[0].Predicate : FALSE;
   if (predicate) {
      negate = dst[0].PredNegate;
      swizzle[0] = dst[0].PredSwizzleX;
      swizzle[1] = dst[0].PredSwizzleY;
      swizzle[2] = dst[0].PredSwizzleZ;
      swizzle[3] = dst[0].PredSwizzleW;
   }

   insn = ureg_emit_insn(ureg,
                         opcode,
                         saturate,
                         predicate,
                         negate,
                         swizzle[0],
                         swizzle[1],
                         swizzle[2],
                         swizzle[3],
                         nr_dst,
                         nr_src);

   for (i = 0; i < nr_dst; i++)
      ureg_emit_dst( ureg, dst[i] );

   for (i = 0; i < nr_src; i++)
      ureg_emit_src( ureg, src[i] );

   ureg_fixup_insn_size( ureg, insn.insn_token );
}

void
ureg_tex_insn(struct ureg_program *ureg,
              unsigned opcode,
              const struct ureg_dst *dst,
              unsigned nr_dst,
              unsigned target,
              const struct ureg_src *src,
              unsigned nr_src )
{
   struct ureg_emit_insn_result insn;
   unsigned i;
   boolean saturate;
   boolean predicate;
   boolean negate;
   unsigned swizzle[4];

   saturate = nr_dst ? dst[0].Saturate : FALSE;
   predicate = nr_dst ? dst[0].Predicate : FALSE;
   if (predicate) {
      negate = dst[0].PredNegate;
      swizzle[0] = dst[0].PredSwizzleX;
      swizzle[1] = dst[0].PredSwizzleY;
      swizzle[2] = dst[0].PredSwizzleZ;
      swizzle[3] = dst[0].PredSwizzleW;
   }

   insn = ureg_emit_insn(ureg,
                         opcode,
                         saturate,
                         predicate,
                         negate,
                         swizzle[0],
                         swizzle[1],
                         swizzle[2],
                         swizzle[3],
                         nr_dst,
                         nr_src);

   ureg_emit_texture( ureg, insn.extended_token, target );

   for (i = 0; i < nr_dst; i++)
      ureg_emit_dst( ureg, dst[i] );

   for (i = 0; i < nr_src; i++)
      ureg_emit_src( ureg, src[i] );

   ureg_fixup_insn_size( ureg, insn.insn_token );
}


void
ureg_label_insn(struct ureg_program *ureg,
                unsigned opcode,
                const struct ureg_src *src,
                unsigned nr_src,
                unsigned *label_token )
{
   struct ureg_emit_insn_result insn;
   unsigned i;

   insn = ureg_emit_insn(ureg,
                         opcode,
                         FALSE,
                         FALSE,
                         FALSE,
                         TGSI_SWIZZLE_X,
                         TGSI_SWIZZLE_Y,
                         TGSI_SWIZZLE_Z,
                         TGSI_SWIZZLE_W,
                         0,
                         nr_src);

   ureg_emit_label( ureg, insn.extended_token, label_token );

   for (i = 0; i < nr_src; i++)
      ureg_emit_src( ureg, src[i] );

   ureg_fixup_insn_size( ureg, insn.insn_token );
}



static void emit_decl( struct ureg_program *ureg,
                       unsigned file,
                       unsigned index,
                       unsigned semantic_name,
                       unsigned semantic_index,
                       unsigned interp )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL, 3 );

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 3;
   out[0].decl.File = file;
   out[0].decl.UsageMask = TGSI_WRITEMASK_XYZW; /* FIXME! */
   out[0].decl.Interpolate = interp;
   out[0].decl.Semantic = 1;

   out[1].value = 0;
   out[1].decl_range.First = 
      out[1].decl_range.Last = index;

   out[2].value = 0;
   out[2].decl_semantic.SemanticName = semantic_name;
   out[2].decl_semantic.SemanticIndex = semantic_index;

}


static void emit_decl_range( struct ureg_program *ureg,
                             unsigned file,
                             unsigned first,
                             unsigned count )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL, 2 );

   out[0].value = 0;
   out[0].decl.Type = TGSI_TOKEN_TYPE_DECLARATION;
   out[0].decl.NrTokens = 2;
   out[0].decl.File = file;
   out[0].decl.UsageMask = 0xf;
   out[0].decl.Interpolate = TGSI_INTERPOLATE_CONSTANT;
   out[0].decl.Semantic = 0;

   out[1].value = 0;
   out[1].decl_range.First = first;
   out[1].decl_range.Last = first + count - 1;
}

static void emit_immediate( struct ureg_program *ureg,
                            const float *v )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL, 5 );

   out[0].value = 0;
   out[0].imm.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   out[0].imm.NrTokens = 5;
   out[0].imm.DataType = TGSI_IMM_FLOAT32;
   out[0].imm.Padding = 0;
   out[0].imm.Extended = 0;

   out[1].imm_data.Float = v[0];
   out[2].imm_data.Float = v[1];
   out[3].imm_data.Float = v[2];
   out[4].imm_data.Float = v[3];
}




static void emit_decls( struct ureg_program *ureg )
{
   unsigned i;

   if (ureg->processor == TGSI_PROCESSOR_VERTEX) {
      for (i = 0; i < UREG_MAX_INPUT; i++) {
         if (ureg->vs_inputs[i/32] & (1 << (i%32))) {
            emit_decl_range( ureg, TGSI_FILE_INPUT, i, 1 );
         }
      }
   }
   else {
      for (i = 0; i < ureg->nr_fs_inputs; i++) {
         emit_decl( ureg, 
                    TGSI_FILE_INPUT, 
                    i,
                    ureg->fs_input[i].semantic_name,
                    ureg->fs_input[i].semantic_index,
                    ureg->fs_input[i].interp );
      }
   }

   for (i = 0; i < ureg->nr_outputs; i++) {
      emit_decl( ureg, 
                 TGSI_FILE_OUTPUT, 
                 i,
                 ureg->output[i].semantic_name,
                 ureg->output[i].semantic_index,
                 TGSI_INTERPOLATE_CONSTANT );
   }

   for (i = 0; i < ureg->nr_samplers; i++) {
      emit_decl_range( ureg, 
                       TGSI_FILE_SAMPLER,
                       ureg->sampler[i].Index, 1 );
   }

   if (ureg->nr_constant_ranges) {
      for (i = 0; i < ureg->nr_constant_ranges; i++)
         emit_decl_range( ureg,
                          TGSI_FILE_CONSTANT,
                          ureg->constant_range[i].first, 
                          (ureg->constant_range[i].last + 1 -
                           ureg->constant_range[i].first) );
   }

   if (ureg->nr_temps) {
      emit_decl_range( ureg,
                       TGSI_FILE_TEMPORARY,
                       0, ureg->nr_temps );
   }

   if (ureg->nr_addrs) {
      emit_decl_range( ureg,
                       TGSI_FILE_ADDRESS,
                       0, ureg->nr_addrs );
   }

   if (ureg->nr_preds) {
      emit_decl_range(ureg,
                      TGSI_FILE_PREDICATE,
                      0,
                      ureg->nr_preds);
   }

   for (i = 0; i < ureg->nr_immediates; i++) {
      emit_immediate( ureg,
                      ureg->immediate[i].v );
   }
}

/* Append the instruction tokens onto the declarations to build a
 * contiguous stream suitable to send to the driver.
 */
static void copy_instructions( struct ureg_program *ureg )
{
   unsigned nr_tokens = ureg->domain[DOMAIN_INSN].count;
   union tgsi_any_token *out = get_tokens( ureg, 
                                           DOMAIN_DECL, 
                                           nr_tokens );

   memcpy(out, 
          ureg->domain[DOMAIN_INSN].tokens, 
          nr_tokens * sizeof out[0] );
}


static void
fixup_header_size(struct ureg_program *ureg)
{
   union tgsi_any_token *out = retrieve_token( ureg, DOMAIN_DECL, 1 );

   out->header.BodySize = ureg->domain[DOMAIN_DECL].count - 3;
}


static void
emit_header( struct ureg_program *ureg )
{
   union tgsi_any_token *out = get_tokens( ureg, DOMAIN_DECL, 3 );

   out[0].version.MajorVersion = 1;
   out[0].version.MinorVersion = 1;
   out[0].version.Padding = 0;

   out[1].header.HeaderSize = 2;
   out[1].header.BodySize = 0;

   out[2].processor.Processor = ureg->processor;
   out[2].processor.Padding = 0;
}


const struct tgsi_token *ureg_finalize( struct ureg_program *ureg )
{
   const struct tgsi_token *tokens;

   emit_header( ureg );
   emit_decls( ureg );
   copy_instructions( ureg );
   fixup_header_size( ureg );
   
   if (ureg->domain[0].tokens == error_tokens ||
       ureg->domain[1].tokens == error_tokens) {
      debug_printf("%s: error in generated shader\n", __FUNCTION__);
      assert(0);
      return NULL;
   }

   tokens = &ureg->domain[DOMAIN_DECL].tokens[0].token;

   if (0) {
      debug_printf("%s: emitted shader %d tokens:\n", __FUNCTION__, 
                   ureg->domain[DOMAIN_DECL].count);
      tgsi_dump( tokens, 0 );
   }

#if DEBUG
   if (tokens && !tgsi_sanity_check(tokens)) {
      debug_printf("tgsi_ureg.c, sanity check failed on generated tokens:\n");
      tgsi_dump(tokens, 0);
      assert(0);
   }
#endif

   
   return tokens;
}


void *ureg_create_shader( struct ureg_program *ureg,
                          struct pipe_context *pipe )
{
   struct pipe_shader_state state;

   state.tokens = ureg_finalize(ureg);
   if(!state.tokens)
      return NULL;

   if (ureg->processor == TGSI_PROCESSOR_VERTEX)
      return pipe->create_vs_state( pipe, &state );
   else
      return pipe->create_fs_state( pipe, &state );
}


const struct tgsi_token *ureg_get_tokens( struct ureg_program *ureg,
                                          unsigned *nr_tokens )
{
   const struct tgsi_token *tokens;

   ureg_finalize(ureg);

   tokens = &ureg->domain[DOMAIN_DECL].tokens[0].token;

   if (nr_tokens) 
      *nr_tokens = ureg->domain[DOMAIN_DECL].size;

   ureg->domain[DOMAIN_DECL].tokens = 0;
   ureg->domain[DOMAIN_DECL].size = 0;
   ureg->domain[DOMAIN_DECL].order = 0;
   ureg->domain[DOMAIN_DECL].count = 0;

   return tokens;
}


struct ureg_program *ureg_create( unsigned processor )
{
   struct ureg_program *ureg = CALLOC_STRUCT( ureg_program );
   if (ureg == NULL)
      return NULL;

   ureg->processor = processor;
   return ureg;
}


void ureg_destroy( struct ureg_program *ureg )
{
   unsigned i;

   for (i = 0; i < Elements(ureg->domain); i++) {
      if (ureg->domain[i].tokens && 
          ureg->domain[i].tokens != error_tokens)
         FREE(ureg->domain[i].tokens);
   }
   
   FREE(ureg);
}
