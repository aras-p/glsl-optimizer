/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include "vl_mpeg12_mc_renderer.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_memory.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include "vl_shader_build.h"

#define DEFAULT_BUF_ALIGNMENT 1
#define MACROBLOCK_WIDTH 16
#define MACROBLOCK_HEIGHT 16
#define BLOCK_WIDTH 8
#define BLOCK_HEIGHT 8
#define ZERO_BLOCK_NIL -1.0f
#define ZERO_BLOCK_IS_NIL(zb) ((zb).x < 0.0f)

struct vertex2f
{
   float x, y;
};

struct vertex4f
{
   float x, y, z, w;
};

struct vertex_shader_consts
{
   struct vertex4f denorm;
};

struct fragment_shader_consts
{
   struct vertex4f multiplier;
   struct vertex4f div;
};

/*
 * Muliplier renormalizes block samples from 16 bits to 12 bits.
 * Divider is used when calculating Y % 2 for choosing top or bottom
 * field for P or B macroblocks.
 * TODO: Use immediates.
 */
static const struct fragment_shader_consts fs_consts = {
   {32767.0f / 255.0f, 32767.0f / 255.0f, 32767.0f / 255.0f, 0.0f},
   {0.5f, 2.0f, 0.0f, 0.0f}
};

struct vert_stream_0
{
   struct vertex2f pos;
   struct vertex2f luma_tc;
   struct vertex2f cb_tc;
   struct vertex2f cr_tc;
};

enum MACROBLOCK_TYPE
{
   MACROBLOCK_TYPE_INTRA,
   MACROBLOCK_TYPE_FWD_FRAME_PRED,
   MACROBLOCK_TYPE_FWD_FIELD_PRED,
   MACROBLOCK_TYPE_BKWD_FRAME_PRED,
   MACROBLOCK_TYPE_BKWD_FIELD_PRED,
   MACROBLOCK_TYPE_BI_FRAME_PRED,
   MACROBLOCK_TYPE_BI_FIELD_PRED,

   NUM_MACROBLOCK_TYPES
};

static void
create_intra_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 50;

   struct pipe_shader_state vs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

   ti = 2;

   /*
    * decl i0              ; Vertex pos
    * decl i1              ; Luma texcoords
    * decl i2              ; Chroma Cb texcoords
    * decl i3              ; Chroma Cr texcoords
    */
   for (i = 0; i < 4; i++) {
      decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * decl o0              ; Vertex pos
    * decl o1              ; Luma texcoords
    * decl o2              ; Chroma Cb texcoords
    * decl o3              ; Chroma Cr texcoords
    */
   for (i = 0; i < 4; i++) {
      decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * mov o0, i0           ; Move input vertex pos to output
    * mov o1, i1           ; Move input luma texcoords to output
    * mov o2, i2           ; Move input chroma Cb texcoords to output
    * mov o3, i3           ; Move input chroma Cr texcoords to output
    */
   for (i = 0; i < 4; ++i) {
      inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   vs.tokens = tokens;
   r->i_vs = r->pipe->create_vs_state(r->pipe, &vs);
   free(tokens);
}

static void
create_intra_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 100;

   struct pipe_shader_state fs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

   ti = 2;

   /*
    * decl i0                      ; Luma texcoords
    * decl i1                      ; Chroma Cb texcoords
    * decl i2                      ; Chroma Cr texcoords
    */
   for (i = 0; i < 3; ++i) {
      decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /* decl c0                      ; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
   decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl o0                      ; Fragment color */
   decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl t0, t1 */
   decl = vl_decl_temps(0, 1);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * decl s0                      ; Sampler for luma texture
    * decl s1                      ; Sampler for chroma Cb texture
    * decl s2                      ; Sampler for chroma Cr texture
    */
   for (i = 0; i < 3; ++i) {
      decl = vl_decl_samplers(i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * tex2d t1, i0, s0             ; Read texel from luma texture
    * mov t0.x, t1.x               ; Move luma sample into .x component
    * tex2d t1, i1, s1             ; Read texel from chroma Cb texture
    * mov t0.y, t1.x               ; Move Cb sample into .y component
    * tex2d t1, i2, s2             ; Read texel from chroma Cr texture
    * mov t0.z, t1.x               ; Move Cr sample into .z component
    */
   for (i = 0; i < 3; ++i) {
      inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

      inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
      inst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_X;
      inst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X << i;
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* mul o0, t0, c0               ; Rescale texel to correct range */
   inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   fs.tokens = tokens;
   r->i_fs = r->pipe->create_fs_state(r->pipe, &fs);
   free(tokens);
}

static void
create_frame_pred_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 100;

   struct pipe_shader_state vs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

   ti = 2;

   /*
    * decl i0              ; Vertex pos
    * decl i1              ; Luma texcoords
    * decl i2              ; Chroma Cb texcoords
    * decl i3              ; Chroma Cr texcoords
    * decl i4              ; Ref surface top field texcoords
    * decl i5              ; Ref surface bottom field texcoords (unused, packed in the same stream)
    */
   for (i = 0; i < 6; i++) {
      decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * decl o0              ; Vertex pos
    * decl o1              ; Luma texcoords
    * decl o2              ; Chroma Cb texcoords
    * decl o3              ; Chroma Cr texcoords
    * decl o4              ; Ref macroblock texcoords
    */
   for (i = 0; i < 5; i++) {
      decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * mov o0, i0           ; Move input vertex pos to output
    * mov o1, i1           ; Move input luma texcoords to output
    * mov o2, i2           ; Move input chroma Cb texcoords to output
    * mov o3, i3           ; Move input chroma Cr texcoords to output
    */
   for (i = 0; i < 4; ++i) {
        inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
        ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* add o4, i0, i4       ; Translate vertex pos by motion vec to form ref macroblock texcoords */
   inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, 4);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   vs.tokens = tokens;
   r->p_vs[0] = r->pipe->create_vs_state(r->pipe, &vs);
   free(tokens);
}

#if 0
static void
create_field_pred_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   assert(false);
}
#endif

static void
create_frame_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 100;

   struct pipe_shader_state fs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

   ti = 2;

   /*
    * decl i0                      ; Luma texcoords
    * decl i1                      ; Chroma Cb texcoords
    * decl i2                      ; Chroma Cr texcoords
    * decl i3                      ; Ref macroblock texcoords
    */
   for (i = 0; i < 4; ++i) {
      decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /* decl c0                      ; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
   decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl o0                      ; Fragment color */
   decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl t0, t1 */
   decl = vl_decl_temps(0, 1);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * decl s0                      ; Sampler for luma texture
    * decl s1                      ; Sampler for chroma Cb texture
    * decl s2                      ; Sampler for chroma Cr texture
    * decl s3                      ; Sampler for ref surface texture
    */
   for (i = 0; i < 4; ++i) {
      decl = vl_decl_samplers(i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * tex2d t1, i0, s0             ; Read texel from luma texture
    * mov t0.x, t1.x               ; Move luma sample into .x component
    * tex2d t1, i1, s1             ; Read texel from chroma Cb texture
    * mov t0.y, t1.x               ; Move Cb sample into .y component
    * tex2d t1, i2, s2             ; Read texel from chroma Cr texture
    * mov t0.z, t1.x               ; Move Cr sample into .z component
    */
   for (i = 0; i < 3; ++i) {
      inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

      inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
      inst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_X;
      inst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X << i;
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* mul t0, t0, c0               ; Rescale texel to correct range */
   inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* tex2d t1, i3, s3             ; Read texel from ref macroblock */
   inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 3, TGSI_FILE_SAMPLER, 3);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* add o0, t0, t1               ; Add ref and differential to form final output */
   inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   fs.tokens = tokens;
   r->p_fs[0] = r->pipe->create_fs_state(r->pipe, &fs);
   free(tokens);
}

#if 0
static void
create_field_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   assert(false);
}
#endif

static void
create_frame_bi_pred_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 100;

   struct pipe_shader_state vs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

   ti = 2;

   /*
    * decl i0              ; Vertex pos
    * decl i1              ; Luma texcoords
    * decl i2              ; Chroma Cb texcoords
    * decl i3              ; Chroma Cr texcoords
    * decl i4              ; First ref macroblock top field texcoords
    * decl i5              ; First ref macroblock bottom field texcoords (unused, packed in the same stream)
    * decl i6              ; Second ref macroblock top field texcoords
    * decl i7              ; Second ref macroblock bottom field texcoords (unused, packed in the same stream)
    */
   for (i = 0; i < 8; i++) {
      decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * decl o0              ; Vertex pos
    * decl o1              ; Luma texcoords
    * decl o2              ; Chroma Cb texcoords
    * decl o3              ; Chroma Cr texcoords
    * decl o4              ; First ref macroblock texcoords
    * decl o5              ; Second ref macroblock texcoords
    */
   for (i = 0; i < 6; i++) {
      decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * mov o0, i0           ; Move input vertex pos to output
    * mov o1, i1           ; Move input luma texcoords to output
    * mov o2, i2           ; Move input chroma Cb texcoords to output
    * mov o3, i3           ; Move input chroma Cr texcoords to output
    */
   for (i = 0; i < 4; ++i) {
      inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * add o4, i0, i4       ; Translate vertex pos by motion vec to form first ref macroblock texcoords
    * add o5, i0, i6       ; Translate vertex pos by motion vec to form second ref macroblock texcoords
    */
   for (i = 0; i < 2; ++i) {
      inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, (i + 2) * 2);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   vs.tokens = tokens;
   r->b_vs[0] = r->pipe->create_vs_state(r->pipe, &vs);
   free(tokens);
}

#if 0
static void
create_field_bi_pred_vert_shader(struct vl_mpeg12_mc_renderer *r)
{
   assert(false);
}
#endif

static void
create_frame_bi_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   const unsigned max_tokens = 100;

   struct pipe_shader_state fs;
   struct tgsi_token *tokens;
   struct tgsi_header *header;

   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;

   unsigned ti;

   unsigned i;

   assert(r);

   tokens = (struct tgsi_token *) malloc(max_tokens * sizeof(struct tgsi_token));
   header = (struct tgsi_header *) &tokens[0];
   *header = tgsi_build_header();
   *(struct tgsi_processor *) &tokens[1] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

   ti = 2;

   /*
    * decl i0                      ; Luma texcoords
    * decl i1                      ; Chroma Cb texcoords
    * decl i2                      ; Chroma Cr texcoords
    * decl i3                      ; First ref macroblock texcoords
    * decl i4                      ; Second ref macroblock texcoords
    */
   for (i = 0; i < 5; ++i) {
      decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * decl c0                      ; Scaling factor, rescales 16-bit snorm to 9-bit snorm
    * decl c1                      ; Constant 1/2 in .x channel to use as weight to blend past and future texels
    */
   decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl o0                      ; Fragment color */
   decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /* decl t0-t2 */
   decl = vl_decl_temps(0, 2);
   ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

   /*
    * decl s0                      ; Sampler for luma texture
    * decl s1                      ; Sampler for chroma Cb texture
    * decl s2                      ; Sampler for chroma Cr texture
    * decl s3                      ; Sampler for first ref surface texture
    * decl s4                      ; Sampler for second ref surface texture
    */
   for (i = 0; i < 5; ++i) {
      decl = vl_decl_samplers(i, i);
      ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
   }

   /*
    * tex2d t1, i0, s0             ; Read texel from luma texture
    * mov t0.x, t1.x               ; Move luma sample into .x component
    * tex2d t1, i1, s1             ; Read texel from chroma Cb texture
    * mov t0.y, t1.x               ; Move Cb sample into .y component
    * tex2d t1, i2, s2             ; Read texel from chroma Cr texture
    * mov t0.z, t1.x               ; Move Cr sample into .z component
    */
   for (i = 0; i < 3; ++i) {
      inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

      inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
      inst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
      inst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_X;
      inst.Dst[0].Register.WriteMask = TGSI_WRITEMASK_X << i;
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* mul t0, t0, c0               ; Rescale texel to correct range */
   inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /*
    * tex2d t1, i3, s3             ; Read texel from first ref macroblock
    * tex2d t2, i4, s4             ; Read texel from second ref macroblock
    */
   for (i = 0; i < 2; ++i) {
      inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, i + 3);
      ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
   }

   /* lerp t1, c1.x, t1, t2        ; Blend past and future texels */
   inst = vl_inst4(TGSI_OPCODE_LRP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
   inst.Src[0].Register.SwizzleX = TGSI_SWIZZLE_X;
   inst.Src[0].Register.SwizzleY = TGSI_SWIZZLE_X;
   inst.Src[0].Register.SwizzleZ = TGSI_SWIZZLE_X;
   inst.Src[0].Register.SwizzleW = TGSI_SWIZZLE_X;
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* add o0, t0, t1               ; Add past/future ref and differential to form final output */
   inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   /* end */
   inst = vl_end();
   ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

   assert(ti <= max_tokens);

   fs.tokens = tokens;
   r->b_fs[0] = r->pipe->create_fs_state(r->pipe, &fs);
   free(tokens);
}

#if 0
static void
create_field_bi_pred_frag_shader(struct vl_mpeg12_mc_renderer *r)
{
   assert(false);
}
#endif

static void
xfer_buffers_map(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 3; ++i) {
      r->tex_transfer[i] = r->pipe->screen->get_tex_transfer
      (
         r->pipe->screen, r->textures.all[i],
         0, 0, 0, PIPE_TRANSFER_WRITE, 0, 0,
         r->textures.all[i]->width0, r->textures.all[i]->height0
      );

      r->texels[i] = r->pipe->screen->transfer_map(r->pipe->screen, r->tex_transfer[i]);
   }
}

static void
xfer_buffers_unmap(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 3; ++i) {
      r->pipe->screen->transfer_unmap(r->pipe->screen, r->tex_transfer[i]);
      r->pipe->screen->tex_transfer_destroy(r->tex_transfer[i]);
   }
}

static bool
init_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_sampler_state sampler;
   unsigned filters[5];
   unsigned i;

   assert(r);

   r->viewport.scale[0] = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   r->viewport.scale[1] = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   r->viewport.scale[2] = 1;
   r->viewport.scale[3] = 1;
   r->viewport.translate[0] = 0;
   r->viewport.translate[1] = 0;
   r->viewport.translate[2] = 0;
   r->viewport.translate[3] = 0;

   r->scissor.maxx = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   r->scissor.maxy = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;

   r->fb_state.width = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   r->fb_state.height = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   r->fb_state.nr_cbufs = 1;
   r->fb_state.zsbuf = NULL;

   /* Luma filter */
   filters[0] = PIPE_TEX_FILTER_NEAREST;
   /* Chroma filters */
   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_444 ||
       r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
      filters[1] = PIPE_TEX_FILTER_NEAREST;
      filters[2] = PIPE_TEX_FILTER_NEAREST;
   }
   else {
      filters[1] = PIPE_TEX_FILTER_LINEAR;
      filters[2] = PIPE_TEX_FILTER_LINEAR;
   }
   /* Fwd, bkwd ref filters */
   filters[3] = PIPE_TEX_FILTER_LINEAR;
   filters[4] = PIPE_TEX_FILTER_LINEAR;

   for (i = 0; i < 5; ++i) {
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_img_filter = filters[i];
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = filters[i];
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      /*sampler.shadow_ambient = ; */
      /*sampler.lod_bias = ; */
      sampler.min_lod = 0;
      /*sampler.max_lod = ; */
      /*sampler.border_color[i] = ; */
      /*sampler.max_anisotropy = ; */
      r->samplers.all[i] = r->pipe->create_sampler_state(r->pipe, &sampler);
   }

   return true;
}

static void
cleanup_pipe_state(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   for (i = 0; i < 5; ++i)
      r->pipe->delete_sampler_state(r->pipe, r->samplers.all[i]);
}

static bool
init_shaders(struct vl_mpeg12_mc_renderer *r)
{
   assert(r);

   create_intra_vert_shader(r);
   create_intra_frag_shader(r);
   create_frame_pred_vert_shader(r);
   create_frame_pred_frag_shader(r);
   create_frame_bi_pred_vert_shader(r);
   create_frame_bi_pred_frag_shader(r);

   return true;
}

static void
cleanup_shaders(struct vl_mpeg12_mc_renderer *r)
{
   assert(r);

   r->pipe->delete_vs_state(r->pipe, r->i_vs);
   r->pipe->delete_fs_state(r->pipe, r->i_fs);
   r->pipe->delete_vs_state(r->pipe, r->p_vs[0]);
   r->pipe->delete_fs_state(r->pipe, r->p_fs[0]);
   r->pipe->delete_vs_state(r->pipe, r->b_vs[0]);
   r->pipe->delete_fs_state(r->pipe, r->b_fs[0]);
}

static bool
init_buffers(struct vl_mpeg12_mc_renderer *r)
{
   struct pipe_texture template;

   const unsigned mbw =
      align(r->picture_width, MACROBLOCK_WIDTH) / MACROBLOCK_WIDTH;
   const unsigned mbh =
      align(r->picture_height, MACROBLOCK_HEIGHT) / MACROBLOCK_HEIGHT;

   unsigned i;

   assert(r);

   r->macroblocks_per_batch =
      mbw * (r->bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE ? mbh : 1);
   r->num_macroblocks = 0;
   r->macroblock_buf = MALLOC(r->macroblocks_per_batch * sizeof(struct pipe_mpeg12_macroblock));

   memset(&template, 0, sizeof(struct pipe_texture));
   template.target = PIPE_TEXTURE_2D;
   /* TODO: Accomodate HW that can't do this and also for cases when this isn't precise enough */
   template.format = PIPE_FORMAT_R16_SNORM;
   template.last_level = 0;
   template.width0 = r->pot_buffers ?
      util_next_power_of_two(r->picture_width) : r->picture_width;
   template.height0 = r->pot_buffers ?
      util_next_power_of_two(r->picture_height) : r->picture_height;
   template.depth0 = 1;
   template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER | PIPE_TEXTURE_USAGE_DYNAMIC;

   r->textures.individual.y = r->pipe->screen->texture_create(r->pipe->screen, &template);

   if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      template.width0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_width / 2) :
         r->picture_width / 2;
      template.height0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_height / 2) :
         r->picture_height / 2;
   }
   else if (r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422)
      template.height0 = r->pot_buffers ?
         util_next_power_of_two(r->picture_height / 2) :
         r->picture_height / 2;

   r->textures.individual.cb =
      r->pipe->screen->texture_create(r->pipe->screen, &template);
   r->textures.individual.cr =
      r->pipe->screen->texture_create(r->pipe->screen, &template);

   r->vertex_bufs.individual.ycbcr.stride = sizeof(struct vertex2f) * 4;
   r->vertex_bufs.individual.ycbcr.max_index = 24 * r->macroblocks_per_batch - 1;
   r->vertex_bufs.individual.ycbcr.buffer_offset = 0;
   r->vertex_bufs.individual.ycbcr.buffer = pipe_buffer_create
   (
      r->pipe->screen,
      DEFAULT_BUF_ALIGNMENT,
      PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_DISCARD,
      sizeof(struct vertex2f) * 4 * 24 * r->macroblocks_per_batch
   );

   for (i = 1; i < 3; ++i) {
      r->vertex_bufs.all[i].stride = sizeof(struct vertex2f) * 2;
      r->vertex_bufs.all[i].max_index = 24 * r->macroblocks_per_batch - 1;
      r->vertex_bufs.all[i].buffer_offset = 0;
      r->vertex_bufs.all[i].buffer = pipe_buffer_create
      (
         r->pipe->screen,
         DEFAULT_BUF_ALIGNMENT,
         PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_DISCARD,
         sizeof(struct vertex2f) * 2 * 24 * r->macroblocks_per_batch
      );
   }

   /* Position element */
   r->vertex_elems[0].src_offset = 0;
   r->vertex_elems[0].instance_divisor = 0;
   r->vertex_elems[0].vertex_buffer_index = 0;
   r->vertex_elems[0].nr_components = 2;
   r->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Luma, texcoord element */
   r->vertex_elems[1].src_offset = sizeof(struct vertex2f);
   r->vertex_elems[1].instance_divisor = 0;
   r->vertex_elems[1].vertex_buffer_index = 0;
   r->vertex_elems[1].nr_components = 2;
   r->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Chroma Cr texcoord element */
   r->vertex_elems[2].src_offset = sizeof(struct vertex2f) * 2;
   r->vertex_elems[2].instance_divisor = 0;
   r->vertex_elems[2].vertex_buffer_index = 0;
   r->vertex_elems[2].nr_components = 2;
   r->vertex_elems[2].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Chroma Cb texcoord element */
   r->vertex_elems[3].src_offset = sizeof(struct vertex2f) * 3;
   r->vertex_elems[3].instance_divisor = 0;
   r->vertex_elems[3].vertex_buffer_index = 0;
   r->vertex_elems[3].nr_components = 2;
   r->vertex_elems[3].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* First ref surface top field texcoord element */
   r->vertex_elems[4].src_offset = 0;
   r->vertex_elems[4].instance_divisor = 0;
   r->vertex_elems[4].vertex_buffer_index = 1;
   r->vertex_elems[4].nr_components = 2;
   r->vertex_elems[4].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* First ref surface bottom field texcoord element */
   r->vertex_elems[5].src_offset = sizeof(struct vertex2f);
   r->vertex_elems[5].instance_divisor = 0;
   r->vertex_elems[5].vertex_buffer_index = 1;
   r->vertex_elems[5].nr_components = 2;
   r->vertex_elems[5].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface top field texcoord element */
   r->vertex_elems[6].src_offset = 0;
   r->vertex_elems[6].instance_divisor = 0;
   r->vertex_elems[6].vertex_buffer_index = 2;
   r->vertex_elems[6].nr_components = 2;
   r->vertex_elems[6].src_format = PIPE_FORMAT_R32G32_FLOAT;

   /* Second ref surface bottom field texcoord element */
   r->vertex_elems[7].src_offset = sizeof(struct vertex2f);
   r->vertex_elems[7].instance_divisor = 0;
   r->vertex_elems[7].vertex_buffer_index = 2;
   r->vertex_elems[7].nr_components = 2;
   r->vertex_elems[7].src_format = PIPE_FORMAT_R32G32_FLOAT;

   r->vs_const_buf = pipe_buffer_create
   (
      r->pipe->screen,
      DEFAULT_BUF_ALIGNMENT,
      PIPE_BUFFER_USAGE_CONSTANT | PIPE_BUFFER_USAGE_DISCARD,
      sizeof(struct vertex_shader_consts)
   );

   r->fs_const_buf = pipe_buffer_create
   (
      r->pipe->screen,
      DEFAULT_BUF_ALIGNMENT,
      PIPE_BUFFER_USAGE_CONSTANT, sizeof(struct fragment_shader_consts)
   );

   memcpy
   (
      pipe_buffer_map(r->pipe->screen, r->fs_const_buf, PIPE_BUFFER_USAGE_CPU_WRITE),
      &fs_consts, sizeof(struct fragment_shader_consts)
   );

   pipe_buffer_unmap(r->pipe->screen, r->fs_const_buf);

   return true;
}

static void
cleanup_buffers(struct vl_mpeg12_mc_renderer *r)
{
   unsigned i;

   assert(r);

   pipe_buffer_reference(&r->vs_const_buf, NULL);
   pipe_buffer_reference(&r->fs_const_buf, NULL);

   for (i = 0; i < 3; ++i)
      pipe_buffer_reference(&r->vertex_bufs.all[i].buffer, NULL);

   for (i = 0; i < 3; ++i)
      pipe_texture_reference(&r->textures.all[i], NULL);

   FREE(r->macroblock_buf);
}

static enum MACROBLOCK_TYPE
get_macroblock_type(struct pipe_mpeg12_macroblock *mb)
{
   assert(mb);

   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
         return MACROBLOCK_TYPE_INTRA;
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_FWD_FRAME_PRED : MACROBLOCK_TYPE_FWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_BKWD_FRAME_PRED : MACROBLOCK_TYPE_BKWD_FIELD_PRED;
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
         return mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME ?
            MACROBLOCK_TYPE_BI_FRAME_PRED : MACROBLOCK_TYPE_BI_FIELD_PRED;
      default:
         assert(0);
   }

   /* Unreachable */
   return -1;
}

/* XXX: One of these days this will have to be killed with fire */
#define SET_BLOCK(vb, cbp, mbx, mby, unitx, unity, ofsx, ofsy, hx, hy, lm, cbm, crm, use_zb, zb)				\
	do {															\
	(vb)[0].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[0].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[1].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[1].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
	(vb)[2].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[3].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[4].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[4].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
	(vb)[5].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
																\
	if (!use_zb || (cbp) & (lm))												\
	{															\
		(vb)[0].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].luma_tc.x = (zb)[0].x;		(vb)[0].luma_tc.y = (zb)[0].y;						\
		(vb)[1].luma_tc.x = (zb)[0].x;		(vb)[1].luma_tc.y = (zb)[0].y + (hy);					\
		(vb)[2].luma_tc.x = (zb)[0].x + (hx);	(vb)[2].luma_tc.y = (zb)[0].y;						\
		(vb)[3].luma_tc.x = (zb)[0].x + (hx);	(vb)[3].luma_tc.y = (zb)[0].y;						\
		(vb)[4].luma_tc.x = (zb)[0].x;		(vb)[4].luma_tc.y = (zb)[0].y + (hy);					\
		(vb)[5].luma_tc.x = (zb)[0].x + (hx);	(vb)[5].luma_tc.y = (zb)[0].y + (hy);					\
	}															\
																\
	if (!use_zb || (cbp) & (cbm))												\
	{															\
		(vb)[0].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].cb_tc.x = (zb)[1].x;		(vb)[0].cb_tc.y = (zb)[1].y;						\
		(vb)[1].cb_tc.x = (zb)[1].x;		(vb)[1].cb_tc.y = (zb)[1].y + (hy);					\
		(vb)[2].cb_tc.x = (zb)[1].x + (hx);	(vb)[2].cb_tc.y = (zb)[1].y;						\
		(vb)[3].cb_tc.x = (zb)[1].x + (hx);	(vb)[3].cb_tc.y = (zb)[1].y;						\
		(vb)[4].cb_tc.x = (zb)[1].x;		(vb)[4].cb_tc.y = (zb)[1].y + (hy);					\
		(vb)[5].cb_tc.x = (zb)[1].x + (hx);	(vb)[5].cb_tc.y = (zb)[1].y + (hy);					\
	}															\
																\
	if (!use_zb || (cbp) & (crm))												\
	{															\
		(vb)[0].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].cr_tc.x = (zb)[2].x;		(vb)[0].cr_tc.y = (zb)[2].y;						\
		(vb)[1].cr_tc.x = (zb)[2].x;		(vb)[1].cr_tc.y = (zb)[2].y + (hy);					\
		(vb)[2].cr_tc.x = (zb)[2].x + (hx);	(vb)[2].cr_tc.y = (zb)[2].y;						\
		(vb)[3].cr_tc.x = (zb)[2].x + (hx);	(vb)[3].cr_tc.y = (zb)[2].y;						\
		(vb)[4].cr_tc.x = (zb)[2].x;		(vb)[4].cr_tc.y = (zb)[2].y + (hy);					\
		(vb)[5].cr_tc.x = (zb)[2].x + (hx);	(vb)[5].cr_tc.y = (zb)[2].y + (hy);					\
	}															\
	} while (0)

static void
gen_macroblock_verts(struct vl_mpeg12_mc_renderer *r,
                     struct pipe_mpeg12_macroblock *mb, unsigned pos,
                     struct vert_stream_0 *ycbcr_vb, struct vertex2f **ref_vb)
{
   struct vertex2f mo_vec[2];

   unsigned i;

   assert(r);
   assert(mb);
   assert(ycbcr_vb);
   assert(pos < r->macroblocks_per_batch);

   mo_vec[1].x = 0;
   mo_vec[1].y = 0;

   switch (mb->mb_type) {
      case PIPE_MPEG12_MACROBLOCK_TYPE_BI:
      {
         struct vertex2f *vb;

         assert(ref_vb && ref_vb[1]);

         vb = ref_vb[1] + pos * 2 * 24;

         mo_vec[0].x = mb->pmv[0][1][0] * 0.5f * r->surface_tex_inv_size.x;
         mo_vec[0].y = mb->pmv[0][1][1] * 0.5f * r->surface_tex_inv_size.y;

         if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
            }
         }
         else {
            mo_vec[1].x = mb->pmv[1][1][0] * 0.5f * r->surface_tex_inv_size.x;
            mo_vec[1].y = mb->pmv[1][1][1] * 0.5f * r->surface_tex_inv_size.y;

            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
               vb[i + 1].x = mo_vec[1].x;
               vb[i + 1].y = mo_vec[1].y;
            }
         }

         /* fall-through */
      }
      case PIPE_MPEG12_MACROBLOCK_TYPE_FWD:
      case PIPE_MPEG12_MACROBLOCK_TYPE_BKWD:
      {
         struct vertex2f *vb;

         assert(ref_vb && ref_vb[0]);

         vb = ref_vb[0] + pos * 2 * 24;

         if (mb->mb_type == PIPE_MPEG12_MACROBLOCK_TYPE_BKWD) {
             mo_vec[0].x = mb->pmv[0][1][0] * 0.5f * r->surface_tex_inv_size.x;
             mo_vec[0].y = mb->pmv[0][1][1] * 0.5f * r->surface_tex_inv_size.y;

             if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FIELD) {
                mo_vec[1].x = mb->pmv[1][1][0] * 0.5f * r->surface_tex_inv_size.x;
                mo_vec[1].y = mb->pmv[1][1][1] * 0.5f * r->surface_tex_inv_size.y;
             }
         }
         else {
            mo_vec[0].x = mb->pmv[0][0][0] * 0.5f * r->surface_tex_inv_size.x;
            mo_vec[0].y = mb->pmv[0][0][1] * 0.5f * r->surface_tex_inv_size.y;

            if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FIELD) {
               mo_vec[1].x = mb->pmv[1][0][0] * 0.5f * r->surface_tex_inv_size.x;
               mo_vec[1].y = mb->pmv[1][0][1] * 0.5f * r->surface_tex_inv_size.y;
            }
         }

         if (mb->mb_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
            }
         }
         else {
            for (i = 0; i < 24 * 2; i += 2) {
               vb[i].x = mo_vec[0].x;
               vb[i].y = mo_vec[0].y;
               vb[i + 1].x = mo_vec[1].x;
               vb[i + 1].y = mo_vec[1].y;
            }
         }

         /* fall-through */
      }
      case PIPE_MPEG12_MACROBLOCK_TYPE_INTRA:
      {
         const struct vertex2f unit =
         {
            r->surface_tex_inv_size.x * MACROBLOCK_WIDTH,
            r->surface_tex_inv_size.y * MACROBLOCK_HEIGHT
         };
         const struct vertex2f half =
         {
            r->surface_tex_inv_size.x * (MACROBLOCK_WIDTH / 2),
            r->surface_tex_inv_size.y * (MACROBLOCK_HEIGHT / 2)
         };
         const bool use_zb = r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE;

         struct vert_stream_0 *vb = ycbcr_vb + pos * 24;

         SET_BLOCK(vb, mb->cbp, mb->mbx, mb->mby,
                   unit.x, unit.y, 0, 0, half.x, half.y,
                   32, 2, 1, use_zb, r->zero_block);

         SET_BLOCK(vb + 6, mb->cbp, mb->mbx, mb->mby,
                   unit.x, unit.y, half.x, 0, half.x, half.y,
                   16, 2, 1, use_zb, r->zero_block);

         SET_BLOCK(vb + 12, mb->cbp, mb->mbx, mb->mby,
                   unit.x, unit.y, 0, half.y, half.x, half.y,
                   8, 2, 1, use_zb, r->zero_block);

         SET_BLOCK(vb + 18, mb->cbp, mb->mbx, mb->mby,
                   unit.x, unit.y, half.x, half.y, half.x, half.y,
                   4, 2, 1, use_zb, r->zero_block);

         break;
      }
      default:
         assert(0);
   }
}

static void
gen_macroblock_stream(struct vl_mpeg12_mc_renderer *r,
                      unsigned *num_macroblocks)
{
   unsigned offset[NUM_MACROBLOCK_TYPES];
   struct vert_stream_0 *ycbcr_vb;
   struct vertex2f *ref_vb[2];
   unsigned i;

   assert(r);
   assert(num_macroblocks);

   for (i = 0; i < r->num_macroblocks; ++i) {
      enum MACROBLOCK_TYPE mb_type = get_macroblock_type(&r->macroblock_buf[i]);
      ++num_macroblocks[mb_type];
   }

   offset[0] = 0;

   for (i = 1; i < NUM_MACROBLOCK_TYPES; ++i)
      offset[i] = offset[i - 1] + num_macroblocks[i - 1];

   ycbcr_vb = (struct vert_stream_0 *)pipe_buffer_map
   (
      r->pipe->screen,
      r->vertex_bufs.individual.ycbcr.buffer,
      PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
   );

   for (i = 0; i < 2; ++i)
      ref_vb[i] = (struct vertex2f *)pipe_buffer_map
      (
         r->pipe->screen,
         r->vertex_bufs.individual.ref[i].buffer,
         PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
      );

   for (i = 0; i < r->num_macroblocks; ++i) {
      enum MACROBLOCK_TYPE mb_type = get_macroblock_type(&r->macroblock_buf[i]);

      gen_macroblock_verts(r, &r->macroblock_buf[i], offset[mb_type],
                           ycbcr_vb, ref_vb);

      ++offset[mb_type];
   }

   pipe_buffer_unmap(r->pipe->screen, r->vertex_bufs.individual.ycbcr.buffer);
   for (i = 0; i < 2; ++i)
      pipe_buffer_unmap(r->pipe->screen, r->vertex_bufs.individual.ref[i].buffer);
}

static void
flush(struct vl_mpeg12_mc_renderer *r)
{
   unsigned num_macroblocks[NUM_MACROBLOCK_TYPES] = { 0 };
   unsigned vb_start = 0;
   struct vertex_shader_consts *vs_consts;
   unsigned i;

   assert(r);
   assert(r->num_macroblocks == r->macroblocks_per_batch);

   gen_macroblock_stream(r, num_macroblocks);

   r->fb_state.cbufs[0] = r->pipe->screen->get_tex_surface
   (
      r->pipe->screen, r->surface,
      0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE
   );

   r->pipe->set_framebuffer_state(r->pipe, &r->fb_state);
   r->pipe->set_viewport_state(r->pipe, &r->viewport);
   r->pipe->set_scissor_state(r->pipe, &r->scissor);

   vs_consts = pipe_buffer_map
   (
      r->pipe->screen, r->vs_const_buf,
      PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
   );

   vs_consts->denorm.x = r->surface->width0;
   vs_consts->denorm.y = r->surface->height0;

   pipe_buffer_unmap(r->pipe->screen, r->vs_const_buf);

   r->pipe->set_constant_buffer(r->pipe, PIPE_SHADER_VERTEX, 0,
                                r->vs_const_buf);
   r->pipe->set_constant_buffer(r->pipe, PIPE_SHADER_FRAGMENT, 0,
                                r->fs_const_buf);

   if (num_macroblocks[MACROBLOCK_TYPE_INTRA] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 1, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 4, r->vertex_elems);
      r->pipe->set_fragment_sampler_textures(r->pipe, 3, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 3, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->i_vs);
      r->pipe->bind_fs_state(r->pipe, r->i_fs);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_INTRA] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_INTRA] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 6, r->vertex_elems);
      r->textures.individual.ref[0] = r->past;
      r->pipe->set_fragment_sampler_textures(r->pipe, 4, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[0]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_FWD_FRAME_PRED] * 24;
   }

   if (false /*num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] > 0 */ ) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 6, r->vertex_elems);
      r->textures.individual.ref[0] = r->past;
      r->pipe->set_fragment_sampler_textures(r->pipe, 4, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[1]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_FWD_FIELD_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 6, r->vertex_elems);
      r->textures.individual.ref[0] = r->future;
      r->pipe->set_fragment_sampler_textures(r->pipe, 4, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[0]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BKWD_FRAME_PRED] * 24;
   }

   if (false /*num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] > 0 */ ) {
      r->pipe->set_vertex_buffers(r->pipe, 2, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 6, r->vertex_elems);
      r->textures.individual.ref[0] = r->future;
      r->pipe->set_fragment_sampler_textures(r->pipe, 4, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 4, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->p_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->p_fs[1]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BKWD_FIELD_PRED] * 24;
   }

   if (num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] > 0) {
      r->pipe->set_vertex_buffers(r->pipe, 3, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 8, r->vertex_elems);
      r->textures.individual.ref[0] = r->past;
      r->textures.individual.ref[1] = r->future;
      r->pipe->set_fragment_sampler_textures(r->pipe, 5, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 5, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->b_vs[0]);
      r->pipe->bind_fs_state(r->pipe, r->b_fs[0]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BI_FRAME_PRED] * 24;
   }

   if (false /*num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] > 0 */ ) {
      r->pipe->set_vertex_buffers(r->pipe, 3, r->vertex_bufs.all);
      r->pipe->set_vertex_elements(r->pipe, 8, r->vertex_elems);
      r->textures.individual.ref[0] = r->past;
      r->textures.individual.ref[1] = r->future;
      r->pipe->set_fragment_sampler_textures(r->pipe, 5, r->textures.all);
      r->pipe->bind_fragment_sampler_states(r->pipe, 5, r->samplers.all);
      r->pipe->bind_vs_state(r->pipe, r->b_vs[1]);
      r->pipe->bind_fs_state(r->pipe, r->b_fs[1]);

      r->pipe->draw_arrays(r->pipe, PIPE_PRIM_TRIANGLES, vb_start,
                           num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] * 24);
      vb_start += num_macroblocks[MACROBLOCK_TYPE_BI_FIELD_PRED] * 24;
   }

   r->pipe->flush(r->pipe, PIPE_FLUSH_RENDER_CACHE, r->fence);
   pipe_surface_reference(&r->fb_state.cbufs[0], NULL);

   if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE)
      for (i = 0; i < 3; ++i)
         r->zero_block[i].x = ZERO_BLOCK_NIL;

   r->num_macroblocks = 0;
}

static void
grab_frame_coded_block(short *src, short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(src);
   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memcpy(dst + y * dst_pitch, src + y * BLOCK_WIDTH, BLOCK_WIDTH * 2);
}

static void
grab_field_coded_block(short *src, short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(src);
   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memcpy(dst + y * dst_pitch * 2, src + y * BLOCK_WIDTH, BLOCK_WIDTH * 2);
}

static void
fill_zero_block(short *dst, unsigned dst_pitch)
{
   unsigned y;

   assert(dst);

   for (y = 0; y < BLOCK_HEIGHT; ++y)
      memset(dst + y * dst_pitch, 0, BLOCK_WIDTH * 2);
}

static void
grab_blocks(struct vl_mpeg12_mc_renderer *r, unsigned mbx, unsigned mby,
            enum pipe_mpeg12_dct_type dct_type, unsigned cbp, short *blocks)
{
   unsigned tex_pitch;
   short *texels;
   unsigned tb = 0, sb = 0;
   unsigned mbpx = mbx * MACROBLOCK_WIDTH, mbpy = mby * MACROBLOCK_HEIGHT;
   unsigned x, y;

   assert(r);
   assert(blocks);

   tex_pitch = r->tex_transfer[0]->stride / util_format_get_blocksize(r->tex_transfer[0]->texture->format);
   texels = r->texels[0] + mbpy * tex_pitch + mbpx;

   for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, ++tb) {
         if ((cbp >> (5 - tb)) & 1) {
            if (dct_type == PIPE_MPEG12_DCT_TYPE_FRAME) {
               grab_frame_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT,
                                      texels + y * tex_pitch * BLOCK_WIDTH +
                                      x * BLOCK_WIDTH, tex_pitch);
            }
            else {
               grab_field_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT,
                                      texels + y * tex_pitch + x * BLOCK_WIDTH,
                                      tex_pitch);
            }

            ++sb;
         }
         else if (r->eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE) {
            if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ALL ||
                ZERO_BLOCK_IS_NIL(r->zero_block[0])) {
               fill_zero_block(texels + y * tex_pitch * BLOCK_WIDTH + x * BLOCK_WIDTH, tex_pitch);
               if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
                  r->zero_block[0].x = (mbpx + x * 8) * r->surface_tex_inv_size.x;
                  r->zero_block[0].y = (mbpy + y * 8) * r->surface_tex_inv_size.y;
               }
            }
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(r->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   mbpx /= 2;
   mbpy /= 2;

   for (tb = 0; tb < 2; ++tb) {
      tex_pitch = r->tex_transfer[tb + 1]->stride / util_format_get_blocksize(r->tex_transfer[tb + 1]->texture->format);
      texels = r->texels[tb + 1] + mbpy * tex_pitch + mbpx;

      if ((cbp >> (1 - tb)) & 1) {
         grab_frame_coded_block(blocks + sb * BLOCK_WIDTH * BLOCK_HEIGHT, texels, tex_pitch);
         ++sb;
      }
      else if (r->eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE) {
         if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ALL ||
             ZERO_BLOCK_IS_NIL(r->zero_block[tb + 1])) {
            fill_zero_block(texels, tex_pitch);
            if (r->eb_handling == VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE) {
               r->zero_block[tb + 1].x = (mbpx << 1) * r->surface_tex_inv_size.x;
               r->zero_block[tb + 1].y = (mbpy << 1) * r->surface_tex_inv_size.y;
            }
         }
      }
   }
}

static void
grab_macroblock(struct vl_mpeg12_mc_renderer *r,
                struct pipe_mpeg12_macroblock *mb)
{
   assert(r);
   assert(mb);
   assert(r->num_macroblocks < r->macroblocks_per_batch);

   memcpy(&r->macroblock_buf[r->num_macroblocks], mb,
          sizeof(struct pipe_mpeg12_macroblock));

   grab_blocks(r, mb->mbx, mb->mby, mb->dct_type, mb->cbp, mb->blocks);

   ++r->num_macroblocks;
}

bool
vl_mpeg12_mc_renderer_init(struct vl_mpeg12_mc_renderer *renderer,
                           struct pipe_context *pipe,
                           unsigned picture_width,
                           unsigned picture_height,
                           enum pipe_video_chroma_format chroma_format,
                           enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode,
                           enum VL_MPEG12_MC_RENDERER_EMPTY_BLOCK eb_handling,
                           bool pot_buffers)
{
   unsigned i;

   assert(renderer);
   assert(pipe);
   /* TODO: Implement other policies */
   assert(bufmode == VL_MPEG12_MC_RENDERER_BUFFER_PICTURE);
   /* TODO: Implement this */
   /* XXX: XFER_ALL sampling issue at block edges when using bilinear filtering */
   assert(eb_handling != VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE);
   /* TODO: Non-pot buffers untested, probably doesn't work without changes to texcoord generation, vert shader, etc */
   assert(pot_buffers);

   memset(renderer, 0, sizeof(struct vl_mpeg12_mc_renderer));

   renderer->pipe = pipe;
   renderer->picture_width = picture_width;
   renderer->picture_height = picture_height;
   renderer->chroma_format = chroma_format;
   renderer->bufmode = bufmode;
   renderer->eb_handling = eb_handling;
   renderer->pot_buffers = pot_buffers;

   if (!init_pipe_state(renderer))
      return false;
   if (!init_shaders(renderer)) {
      cleanup_pipe_state(renderer);
      return false;
   }
   if (!init_buffers(renderer)) {
      cleanup_shaders(renderer);
      cleanup_pipe_state(renderer);
      return false;
   }

   renderer->surface = NULL;
   renderer->past = NULL;
   renderer->future = NULL;
   for (i = 0; i < 3; ++i)
      renderer->zero_block[i].x = ZERO_BLOCK_NIL;
   renderer->num_macroblocks = 0;

   xfer_buffers_map(renderer);

   return true;
}

void
vl_mpeg12_mc_renderer_cleanup(struct vl_mpeg12_mc_renderer *renderer)
{
   assert(renderer);

   xfer_buffers_unmap(renderer);

   cleanup_pipe_state(renderer);
   cleanup_shaders(renderer);
   cleanup_buffers(renderer);
}

void
vl_mpeg12_mc_renderer_render_macroblocks(struct vl_mpeg12_mc_renderer
                                         *renderer,
                                         struct pipe_texture *surface,
                                         struct pipe_texture *past,
                                         struct pipe_texture *future,
                                         unsigned num_macroblocks,
                                         struct pipe_mpeg12_macroblock
                                         *mpeg12_macroblocks,
                                         struct pipe_fence_handle **fence)
{
   bool new_surface = false;

   assert(renderer);
   assert(surface);
   assert(num_macroblocks);
   assert(mpeg12_macroblocks);

   if (renderer->surface) {
      if (surface != renderer->surface) {
         if (renderer->num_macroblocks > 0) {
            xfer_buffers_unmap(renderer);
            flush(renderer);
         }
         
         new_surface = true;
      }

      /* If the surface we're rendering hasn't changed the ref frames shouldn't change. */
      assert(surface != renderer->surface || renderer->past == past);
      assert(surface != renderer->surface || renderer->future == future);
   }
   else
      new_surface = true;

   if (new_surface) {
      renderer->surface = surface;
      renderer->past = past;
      renderer->future = future;
      renderer->fence = fence;
      renderer->surface_tex_inv_size.x = 1.0f / surface->width0;
      renderer->surface_tex_inv_size.y = 1.0f / surface->height0;
   }

   while (num_macroblocks) {
      unsigned left_in_batch = renderer->macroblocks_per_batch - renderer->num_macroblocks;
      unsigned num_to_submit = MIN2(num_macroblocks, left_in_batch);
      unsigned i;

      for (i = 0; i < num_to_submit; ++i) {
         assert(mpeg12_macroblocks[i].base.codec == PIPE_VIDEO_CODEC_MPEG12);
         grab_macroblock(renderer, &mpeg12_macroblocks[i]);
      }

      num_macroblocks -= num_to_submit;

      if (renderer->num_macroblocks == renderer->macroblocks_per_batch) {
         xfer_buffers_unmap(renderer);
         flush(renderer);
         xfer_buffers_map(renderer);
         /* Next time we get this surface it may have new ref frames */
         renderer->surface = NULL;
      }
   }
}
