/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef TGSI_BUILD_H
#define TGSI_BUILD_H


struct tgsi_token;


#if defined __cplusplus
extern "C" {
#endif


/*
 * header
 */

struct tgsi_header
tgsi_build_header( void );

struct tgsi_processor
tgsi_default_processor( void );

struct tgsi_processor
tgsi_build_processor(
   unsigned processor,
   struct tgsi_header *header );

/*
 * declaration
 */

struct tgsi_declaration
tgsi_default_declaration( void );

struct tgsi_declaration
tgsi_build_declaration(
   unsigned file,
   unsigned usage_mask,
   unsigned interpolate,
   unsigned dimension,
   unsigned semantic,
   unsigned centroid,
   unsigned invariant,
   unsigned cylindrical_wrap,
   struct tgsi_header *header );

struct tgsi_full_declaration
tgsi_default_full_declaration( void );

unsigned
tgsi_build_full_declaration(
   const struct tgsi_full_declaration *full_decl,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize );

struct tgsi_declaration_range
tgsi_default_declaration_range( void );

struct tgsi_declaration_range
tgsi_build_declaration_range(
   unsigned first,
   unsigned last,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header );

struct tgsi_declaration_dimension
tgsi_default_declaration_dimension(void);

struct tgsi_declaration_dimension
tgsi_build_declaration_dimension(unsigned index_2d,
                                 struct tgsi_declaration *declaration,
                                 struct tgsi_header *header);

struct tgsi_declaration_semantic
tgsi_default_declaration_semantic( void );

struct tgsi_declaration_semantic
tgsi_build_declaration_semantic(
   unsigned semantic_name,
   unsigned semantic_index,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header );

/*
 * immediate
 */

struct tgsi_immediate
tgsi_default_immediate( void );

struct tgsi_immediate
tgsi_build_immediate(
   struct tgsi_header *header );

struct tgsi_full_immediate
tgsi_default_full_immediate( void );

union tgsi_immediate_data
tgsi_build_immediate_float32(
   float value,
   struct tgsi_immediate *immediate,
   struct tgsi_header *header );

unsigned
tgsi_build_full_immediate(
   const struct tgsi_full_immediate *full_imm,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize );

/*
 * properties
 */

struct tgsi_property
tgsi_default_property( void );

struct tgsi_property
tgsi_build_property(
   unsigned property_name,
   struct tgsi_header *header );

struct tgsi_full_property
tgsi_default_full_property( void );

struct tgsi_property_data
tgsi_build_property_data(
   unsigned value,
   struct tgsi_property *property,
   struct tgsi_header *header );

unsigned
tgsi_build_full_property(
   const struct tgsi_full_property *full_prop,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize );

/*
 * instruction
 */

struct tgsi_instruction
tgsi_default_instruction( void );

struct tgsi_instruction
tgsi_build_instruction(
   unsigned opcode,
   unsigned saturate,
   unsigned predicate,
   unsigned num_dst_regs,
   unsigned num_src_regs,
   struct tgsi_header *header );

struct tgsi_full_instruction
tgsi_default_full_instruction( void );

unsigned
tgsi_build_full_instruction(
   const struct tgsi_full_instruction *full_inst,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   unsigned maxsize );

struct tgsi_instruction_predicate
tgsi_default_instruction_predicate(void);

struct tgsi_instruction_predicate
tgsi_build_instruction_predicate(int index,
                                 unsigned negate,
                                 unsigned swizzleX,
                                 unsigned swizzleY,
                                 unsigned swizzleZ,
                                 unsigned swizzleW,
                                 struct tgsi_instruction *instruction,
                                 struct tgsi_header *header);

struct tgsi_instruction_label
tgsi_default_instruction_label( void );

struct tgsi_instruction_label
tgsi_build_instruction_label(
   unsigned label,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_instruction_texture
tgsi_default_instruction_texture( void );

struct tgsi_instruction_texture
tgsi_build_instruction_texture(
   unsigned texture,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_src_register
tgsi_default_src_register( void );

struct tgsi_src_register
tgsi_build_src_register(
   unsigned file,
   unsigned swizzle_x,
   unsigned swizzle_y,
   unsigned swizzle_z,
   unsigned swizzle_w,
   unsigned negate,
   unsigned absolute,
   unsigned indirect,
   unsigned dimension,
   int index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_full_src_register
tgsi_default_full_src_register( void );


struct tgsi_dimension
tgsi_default_dimension( void );

struct tgsi_dimension
tgsi_build_dimension(
   unsigned indirect,
   unsigned index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_dst_register
tgsi_default_dst_register( void );

struct tgsi_dst_register
tgsi_build_dst_register(
   unsigned file,
   unsigned mask,
   unsigned indirect,
   int index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_full_dst_register
tgsi_default_full_dst_register( void );


#if defined __cplusplus
}
#endif

#endif /* TGSI_BUILD_H */
