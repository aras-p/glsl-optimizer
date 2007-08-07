#if !defined TGSI_BUILD_H
#define TGSI_BUILD_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

/*
 * version
 */

struct tgsi_version
tgsi_build_version( void );

/*
 * header
 */

struct tgsi_header
tgsi_build_header( void );

struct tgsi_processor
tgsi_default_processor( void );

struct tgsi_processor
tgsi_build_processor(
   GLuint processor,
   struct tgsi_header *header );

/*
 * declaration
 */

struct tgsi_declaration
tgsi_default_declaration( void );

struct tgsi_declaration
tgsi_build_declaration(
   GLuint file,
   GLuint declare,
   GLuint interpolate,
   struct tgsi_header *header );

struct tgsi_full_declaration
tgsi_default_full_declaration( void );

GLuint
tgsi_build_full_declaration(
   const struct tgsi_full_declaration *full_decl,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   GLuint maxsize );

struct tgsi_declaration_range
tgsi_build_declaration_range(
   GLuint first,
   GLuint last,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header );

struct tgsi_declaration_mask
tgsi_build_declaration_mask(
   GLuint mask,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header );

struct tgsi_declaration_interpolation
tgsi_default_declaration_interpolation( void );

struct tgsi_declaration_interpolation
tgsi_build_declaration_interpolation(
   GLuint interpolate,
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

struct tgsi_immediate_float32
tgsi_build_immediate_float32(
   GLfloat value,
   struct tgsi_immediate *immediate,
   struct tgsi_header *header );

GLuint
tgsi_build_full_immediate(
   const struct tgsi_full_immediate *full_imm,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   GLuint maxsize );

/*
 * instruction
 */

struct tgsi_instruction
tgsi_default_instruction( void );

struct tgsi_instruction
tgsi_build_instruction(
   GLuint opcode,
   GLuint saturate,
   GLuint num_dst_regs,
   GLuint num_src_regs,
   struct tgsi_header *header );

struct tgsi_full_instruction
tgsi_default_full_instruction( void );

GLuint
tgsi_build_full_instruction(
   const struct tgsi_full_instruction *full_inst,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   GLuint maxsize );

struct tgsi_instruction_ext_nv
tgsi_default_instruction_ext_nv( void );

GLuint
tgsi_compare_instruction_ext_nv(
   struct tgsi_instruction_ext_nv a,
   struct tgsi_instruction_ext_nv b );

struct tgsi_instruction_ext_nv
tgsi_build_instruction_ext_nv(
   GLuint precision,
   GLuint cond_dst_index,
   GLuint cond_flow_index,
   GLuint cond_mask,
   GLuint cond_swizzle_x,
   GLuint cond_swizzle_y,
   GLuint cond_swizzle_z,
   GLuint cond_swizzle_w,
   GLuint cond_dst_update,
   GLuint cond_flow_update,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_instruction_ext_label
tgsi_default_instruction_ext_label( void );

GLuint
tgsi_compare_instruction_ext_label(
   struct tgsi_instruction_ext_label a,
   struct tgsi_instruction_ext_label b );

struct tgsi_instruction_ext_label
tgsi_build_instruction_ext_label(
   GLuint label,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_instruction_ext_texture
tgsi_default_instruction_ext_texture( void );

GLuint
tgsi_compare_instruction_ext_texture(
   struct tgsi_instruction_ext_texture a,
   struct tgsi_instruction_ext_texture b );

struct tgsi_instruction_ext_texture
tgsi_build_instruction_ext_texture(
   GLuint texture,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_src_register
tgsi_default_src_register( void );

struct tgsi_src_register
tgsi_build_src_register(
   GLuint file,
   GLuint swizzle_x,
   GLuint swizzle_y,
   GLuint swizzle_z,
   GLuint swizzle_w,
   GLuint negate,
   GLuint indirect,
   GLuint dimension,
   GLint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_full_src_register
tgsi_default_full_src_register( void );

struct tgsi_src_register_ext_swz
tgsi_default_src_register_ext_swz( void );

GLuint
tgsi_compare_src_register_ext_swz(
   struct tgsi_src_register_ext_swz a,
   struct tgsi_src_register_ext_swz b );

struct tgsi_src_register_ext_swz
tgsi_build_src_register_ext_swz(
   GLuint ext_swizzle_x,
   GLuint ext_swizzle_y,
   GLuint ext_swizzle_z,
   GLuint ext_swizzle_w,
   GLuint negate_x,
   GLuint negate_y,
   GLuint negate_z,
   GLuint negate_w,
   GLuint ext_divide,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_src_register_ext_mod
tgsi_default_src_register_ext_mod( void );

GLuint
tgsi_compare_src_register_ext_mod(
   struct tgsi_src_register_ext_mod a,
   struct tgsi_src_register_ext_mod b );

struct tgsi_src_register_ext_mod
tgsi_build_src_register_ext_mod(
   GLuint complement,
   GLuint bias,
   GLuint scale_2x,
   GLuint absolute,
   GLuint negate,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_dimension
tgsi_default_dimension( void );

struct tgsi_dimension
tgsi_build_dimension(
   GLuint indirect,
   GLuint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_dst_register
tgsi_default_dst_register( void );

struct tgsi_dst_register
tgsi_build_dst_register(
   GLuint file,
   GLuint mask,
   GLint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_full_dst_register
tgsi_default_full_dst_register( void );

struct tgsi_dst_register_ext_concode
tgsi_default_dst_register_ext_concode( void );

GLuint
tgsi_compare_dst_register_ext_concode(
   struct tgsi_dst_register_ext_concode a,
   struct tgsi_dst_register_ext_concode b );

struct tgsi_dst_register_ext_concode
tgsi_build_dst_register_ext_concode(
   GLuint cc,
   GLuint swizzle_x,
   GLuint swizzle_y,
   GLuint swizzle_z,
   GLuint swizzle_w,
   GLint index,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

struct tgsi_dst_register_ext_modulate
tgsi_default_dst_register_ext_modulate( void );

GLuint
tgsi_compare_dst_register_ext_modulate(
   struct tgsi_dst_register_ext_modulate a,
   struct tgsi_dst_register_ext_modulate b );

struct tgsi_dst_register_ext_modulate
tgsi_build_dst_register_ext_modulate(
   GLuint modulate,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_BUILD_H

