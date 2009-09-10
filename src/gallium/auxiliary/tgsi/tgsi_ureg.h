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

#ifndef TGSI_UREG_H
#define TGSI_UREG_H

#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"

#ifdef __cplusplus
extern "C" {
#endif
   
struct ureg_program;

/* Almost a tgsi_src_register, but we need to pull in the Absolute
 * flag from the _ext token.  Indirect flag always implies ADDR[0].
 */
struct ureg_src
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned SwizzleX    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleY    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleZ    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleW    : 2;  /* TGSI_SWIZZLE_ */
   unsigned Pad         : 1;  /* BOOL */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Absolute    : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Negate      : 1;  /* BOOL */
   int      IndirectIndex   : 16; /* SINT */
   int      IndirectSwizzle : 2;  /* TGSI_SWIZZLE_ */
};

/* Very similar to a tgsi_dst_register, removing unsupported fields
 * and adding a Saturate flag.  It's easier to push saturate into the
 * destination register than to try and create a _SAT varient of each
 * instruction function.
 */
struct ureg_dst
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned WriteMask   : 4;  /* TGSI_WRITEMASK_ */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Saturate    : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Pad1        : 5;
   unsigned Pad2        : 1;  /* BOOL */
   int      IndirectIndex   : 16; /* SINT */
   int      IndirectSwizzle : 2;  /* TGSI_SWIZZLE_ */
};

struct pipe_context;

struct ureg_program *
ureg_create( unsigned processor );

const struct tgsi_token *
ureg_finalize( struct ureg_program * );

void *
ureg_create_shader( struct ureg_program *,
                    struct pipe_context *pipe );

void 
ureg_destroy( struct ureg_program * );


/***********************************************************************
 * Convenience routine:
 */
static INLINE void *
ureg_create_shader_and_destroy( struct ureg_program *p,
                                struct pipe_context *pipe )
{
   void *result = ureg_create_shader( p, pipe );
   ureg_destroy( p );
   return result;
}



/***********************************************************************
 * Build shader declarations:
 */

struct ureg_src
ureg_DECL_fs_input( struct ureg_program *,
                    unsigned semantic_name,
                    unsigned semantic_index,
                    unsigned interp_mode );

struct ureg_src
ureg_DECL_vs_input( struct ureg_program *,
                    unsigned semantic_name,
                    unsigned semantic_index );

struct ureg_dst
ureg_DECL_output( struct ureg_program *,
                  unsigned semantic_name,
                  unsigned semantic_index );

struct ureg_src
ureg_DECL_immediate( struct ureg_program *,
                     const float *v,
                     unsigned nr );

struct ureg_src
ureg_DECL_constant( struct ureg_program * );

struct ureg_dst
ureg_DECL_temporary( struct ureg_program * );

void 
ureg_release_temporary( struct ureg_program *ureg,
                        struct ureg_dst tmp );

struct ureg_dst
ureg_DECL_address( struct ureg_program * );

/* Supply an index to the sampler declaration as this is the hook to
 * the external pipe_sampler state.  Users of this function probably
 * don't want just any sampler, but a specific one which they've set
 * up state for in the context.
 */
struct ureg_src
ureg_DECL_sampler( struct ureg_program *,
                   unsigned index );


static INLINE struct ureg_src
ureg_imm4f( struct ureg_program *ureg,
                       float a, float b,
                       float c, float d)
{
   float v[4];
   v[0] = a;
   v[1] = b;
   v[2] = c;
   v[3] = d;
   return ureg_DECL_immediate( ureg, v, 4 );
}

static INLINE struct ureg_src
ureg_imm3f( struct ureg_program *ureg,
                       float a, float b,
                       float c)
{
   float v[3];
   v[0] = a;
   v[1] = b;
   v[2] = c;
   return ureg_DECL_immediate( ureg, v, 3 );
}

static INLINE struct ureg_src
ureg_imm2f( struct ureg_program *ureg,
                       float a, float b)
{
   float v[2];
   v[0] = a;
   v[1] = b;
   return ureg_DECL_immediate( ureg, v, 2 );
}

static INLINE struct ureg_src
ureg_imm1f( struct ureg_program *ureg,
                       float a)
{
   float v[1];
   v[0] = a;
   return ureg_DECL_immediate( ureg, v, 1 );
}

/***********************************************************************
 * Functions for patching up labels
 */


/* Will return a number which can be used in a label to point to the
 * next instruction to be emitted.
 */
unsigned
ureg_get_instruction_number( struct ureg_program *ureg );


/* Patch a given label (expressed as a token number) to point to a
 * given instruction (expressed as an instruction number).
 *
 * Labels are obtained from instruction emitters, eg ureg_CAL().
 * Instruction numbers are obtained from ureg_get_instruction_number(),
 * above.
 */
void
ureg_fixup_label(struct ureg_program *ureg,
                 unsigned label_token,
                 unsigned instruction_number );


/* Generic instruction emitter.  Use if you need to pass the opcode as
 * a parameter, rather than using the emit_OP() varients below.
 */
void
ureg_insn(struct ureg_program *ureg,
          unsigned opcode,
          const struct ureg_dst *dst,
          unsigned nr_dst,
          const struct ureg_src *src,
          unsigned nr_src );


/***********************************************************************
 * Internal instruction helpers, don't call these directly:
 */

unsigned
ureg_emit_insn(struct ureg_program *ureg,
               unsigned opcode,
               boolean saturate,
               unsigned num_dst,
               unsigned num_src );

void
ureg_emit_label(struct ureg_program *ureg,
                unsigned insn_token,
                unsigned *label_token );

void
ureg_emit_texture(struct ureg_program *ureg,
                  unsigned insn_token,
                  unsigned target );

void 
ureg_emit_dst( struct ureg_program *ureg,
               struct ureg_dst dst );

void 
ureg_emit_src( struct ureg_program *ureg,
               struct ureg_src src );

void
ureg_fixup_insn_size(struct ureg_program *ureg,
                     unsigned insn );


#define OP00( op )                                              \
static INLINE void ureg_##op( struct ureg_program *ureg )       \
{                                                               \
   unsigned opcode = TGSI_OPCODE_##op;                          \
   unsigned insn = ureg_emit_insn( ureg, opcode, FALSE, 0, 0 ); \
   ureg_fixup_insn_size( ureg, insn );                          \
}

#define OP01( op )                                              \
static INLINE void ureg_##op( struct ureg_program *ureg,        \
                              struct ureg_src src )             \
{                                                               \
   unsigned opcode = TGSI_OPCODE_##op;                          \
   unsigned insn = ureg_emit_insn( ureg, opcode, FALSE, 0, 1 ); \
   ureg_emit_src( ureg, src );                                  \
   ureg_fixup_insn_size( ureg, insn );                          \
}

#define OP00_LBL( op )                                          \
static INLINE void ureg_##op( struct ureg_program *ureg,        \
                              unsigned *label_token )           \
{                                                               \
   unsigned opcode = TGSI_OPCODE_##op;                          \
   unsigned insn = ureg_emit_insn( ureg, opcode, FALSE, 0, 0 ); \
   ureg_emit_label( ureg, insn, label_token );                  \
   ureg_fixup_insn_size( ureg, insn );                          \
}

#define OP01_LBL( op )                                          \
static INLINE void ureg_##op( struct ureg_program *ureg,        \
                              struct ureg_src src,              \
                              unsigned *label_token )          \
{                                                               \
   unsigned opcode = TGSI_OPCODE_##op;                          \
   unsigned insn = ureg_emit_insn( ureg, opcode, FALSE, 0, 1 ); \
   ureg_emit_label( ureg, insn, label_token );                  \
   ureg_emit_src( ureg, src );                                  \
   ureg_fixup_insn_size( ureg, insn );                          \
}

#define OP10( op )                                                      \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst )                     \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 0 );  \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_fixup_insn_size( ureg, insn );                                  \
}


#define OP11( op )                                                      \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src )                     \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 1 );  \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src );                                          \
   ureg_fixup_insn_size( ureg, insn );                                  \
}

#define OP12( op )                                                      \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src0,                     \
                              struct ureg_src src1 )                    \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 2 );  \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_fixup_insn_size( ureg, insn );                                  \
}

#define OP12_TEX( op )                                                  \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              unsigned target,                          \
                              struct ureg_src src0,                     \
                              struct ureg_src src1 )                    \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 2 );  \
   ureg_emit_texture( ureg, insn, target );                             \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_fixup_insn_size( ureg, insn );                                  \
}

#define OP13( op )                                                      \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              struct ureg_src src0,                     \
                              struct ureg_src src1,                     \
                              struct ureg_src src2 )                    \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 3 );  \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_emit_src( ureg, src2 );                                         \
   ureg_fixup_insn_size( ureg, insn );                                  \
}

#define OP14_TEX( op )                                                  \
static INLINE void ureg_##op( struct ureg_program *ureg,                \
                              struct ureg_dst dst,                      \
                              unsigned target,                          \
                              struct ureg_src src0,                     \
                              struct ureg_src src1,                     \
                              struct ureg_src src2,                     \
                              struct ureg_src src3 )                    \
{                                                                       \
   unsigned opcode = TGSI_OPCODE_##op;                                  \
   unsigned insn = ureg_emit_insn( ureg, opcode, dst.Saturate, 1, 4 );  \
   ureg_emit_texture( ureg, insn, target );                             \
   ureg_emit_dst( ureg, dst );                                          \
   ureg_emit_src( ureg, src0 );                                         \
   ureg_emit_src( ureg, src1 );                                         \
   ureg_emit_src( ureg, src2 );                                         \
   ureg_emit_src( ureg, src3 );                                         \
   ureg_fixup_insn_size( ureg, insn );                                  \
}


/* Use a template include to generate a correctly-typed ureg_OP()
 * function for each TGSI opcode:
 */
#include "tgsi_opcode_tmp.h"


/***********************************************************************
 * Inline helpers for manipulating register structs:
 */
static INLINE struct ureg_src 
ureg_negate( struct ureg_src reg )
{
   assert(reg.File != TGSI_FILE_NULL);
   reg.Negate ^= 1;
   return reg;
}

static INLINE struct ureg_src
ureg_abs( struct ureg_src reg )
{
   assert(reg.File != TGSI_FILE_NULL);
   reg.Absolute = 1;
   reg.Negate = 0;
   return reg;
}

static INLINE struct ureg_src 
ureg_swizzle( struct ureg_src reg, 
              int x, int y, int z, int w )
{
   unsigned swz = ( (reg.SwizzleX << 0) |
                    (reg.SwizzleY << 2) |
                    (reg.SwizzleZ << 4) |
                    (reg.SwizzleW << 6));

   assert(reg.File != TGSI_FILE_NULL);
   assert(x < 4);
   assert(y < 4);
   assert(z < 4);
   assert(w < 4);

   reg.SwizzleX = (swz >> (x*2)) & 0x3;
   reg.SwizzleY = (swz >> (y*2)) & 0x3;
   reg.SwizzleZ = (swz >> (z*2)) & 0x3;
   reg.SwizzleW = (swz >> (w*2)) & 0x3;
   return reg;
}

static INLINE struct ureg_src
ureg_scalar( struct ureg_src reg, int x )
{
   return ureg_swizzle(reg, x, x, x, x);
}

static INLINE struct ureg_dst 
ureg_writemask( struct ureg_dst reg,
                unsigned writemask )
{
   assert(reg.File != TGSI_FILE_NULL);
   reg.WriteMask &= writemask;
   return reg;
}

static INLINE struct ureg_dst 
ureg_saturate( struct ureg_dst reg )
{
   assert(reg.File != TGSI_FILE_NULL);
   reg.Saturate = 1;
   return reg;
}

static INLINE struct ureg_dst 
ureg_dst_indirect( struct ureg_dst reg, struct ureg_src addr )
{
   assert(reg.File != TGSI_FILE_NULL);
   assert(addr.File == TGSI_FILE_ADDRESS);
   reg.Indirect = 1;
   reg.IndirectIndex = addr.Index;
   reg.IndirectSwizzle = addr.SwizzleX;
   return reg;
}

static INLINE struct ureg_src 
ureg_src_indirect( struct ureg_src reg, struct ureg_src addr )
{
   assert(reg.File != TGSI_FILE_NULL);
   assert(addr.File == TGSI_FILE_ADDRESS);
   reg.Indirect = 1;
   reg.IndirectIndex = addr.Index;
   reg.IndirectSwizzle = addr.SwizzleX;
   return reg;
}

static INLINE struct ureg_dst
ureg_dst( struct ureg_src src )
{
   struct ureg_dst dst;

   dst.File      = src.File;
   dst.WriteMask = TGSI_WRITEMASK_XYZW;
   dst.Indirect  = src.Indirect;
   dst.IndirectIndex = src.IndirectIndex;
   dst.IndirectSwizzle = src.IndirectSwizzle;
   dst.Saturate  = 0;
   dst.Index     = src.Index;
   dst.Pad1      = 0;
   dst.Pad2      = 0;

   return dst;
}

static INLINE struct ureg_src
ureg_src( struct ureg_dst dst )
{
   struct ureg_src src;

   src.File      = dst.File;
   src.SwizzleX  = TGSI_SWIZZLE_X;
   src.SwizzleY  = TGSI_SWIZZLE_Y;
   src.SwizzleZ  = TGSI_SWIZZLE_Z;
   src.SwizzleW  = TGSI_SWIZZLE_W;
   src.Pad       = 0;
   src.Indirect  = dst.Indirect;
   src.IndirectIndex = dst.IndirectIndex;
   src.IndirectSwizzle = dst.IndirectSwizzle;
   src.Absolute  = 0;
   src.Index     = dst.Index;
   src.Negate    = 0;

   return src;
}



static INLINE struct ureg_dst
ureg_dst_undef( void )
{
   struct ureg_dst dst;

   dst.File      = TGSI_FILE_NULL;
   dst.WriteMask = 0;
   dst.Indirect  = 0;
   dst.IndirectIndex = 0;
   dst.IndirectSwizzle = 0;
   dst.Saturate  = 0;
   dst.Index     = 0;
   dst.Pad1      = 0;
   dst.Pad2      = 0;

   return dst;
}

static INLINE struct ureg_src
ureg_src_undef( void )
{
   struct ureg_src src;

   src.File      = TGSI_FILE_NULL;
   src.SwizzleX  = 0;
   src.SwizzleY  = 0;
   src.SwizzleZ  = 0;
   src.SwizzleW  = 0;
   src.Pad       = 0;
   src.Indirect  = 0;
   src.IndirectIndex = 0;
   src.IndirectSwizzle = 0;
   src.Absolute  = 0;
   src.Index     = 0;
   src.Negate    = 0;
   
   return src;
}

static INLINE boolean
ureg_src_is_undef( struct ureg_src src )
{
   return src.File == TGSI_FILE_NULL;
}

static INLINE boolean
ureg_dst_is_undef( struct ureg_dst dst )
{
   return dst.File == TGSI_FILE_NULL;
}


#ifdef __cplusplus
}
#endif

#endif
