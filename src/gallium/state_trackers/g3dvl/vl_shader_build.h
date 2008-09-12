#ifndef vl_shader_build_h
#define vl_shader_build_h

#include <pipe/p_shader_tokens.h>

struct tgsi_full_declaration vl_decl_input(unsigned int name, unsigned int index, unsigned int first, unsigned int last);
struct tgsi_full_declaration vl_decl_interpolated_input
(
	unsigned int name,
	unsigned int index,
	unsigned int first,
	unsigned int last,
	int interpolation
);
struct tgsi_full_declaration vl_decl_constants(unsigned int name, unsigned int index, unsigned int first, unsigned int last);
struct tgsi_full_declaration vl_decl_output(unsigned int name, unsigned int index, unsigned int first, unsigned int last);
struct tgsi_full_declaration vl_decl_temps(unsigned int first, unsigned int last);
struct tgsi_full_declaration vl_decl_samplers(unsigned int first, unsigned int last);
struct tgsi_full_instruction vl_inst2
(
	int opcode,
	enum tgsi_file_type dst_file,
	unsigned int dst_index,
	enum tgsi_file_type src_file,
	unsigned int src_index
);
struct tgsi_full_instruction vl_inst3
(
	int opcode,
	enum tgsi_file_type dst_file,
	unsigned int dst_index,
	enum tgsi_file_type src1_file,
	unsigned int src1_index,
	enum tgsi_file_type src2_file,
	unsigned int src2_index
);
struct tgsi_full_instruction vl_tex
(
	int tex,
	enum tgsi_file_type dst_file,
	unsigned int dst_index,
	enum tgsi_file_type src1_file,
	unsigned int src1_index,
	enum tgsi_file_type src2_file,
	unsigned int src2_index
);
struct tgsi_full_instruction vl_inst4
(
	int opcode,
	enum tgsi_file_type dst_file,
	unsigned int dst_index,
	enum tgsi_file_type src1_file,
	unsigned int src1_index,
	enum tgsi_file_type src2_file,
	unsigned int src2_index,
	enum tgsi_file_type src3_file,
	unsigned int src3_index
);
struct tgsi_full_instruction vl_end(void);

#endif
