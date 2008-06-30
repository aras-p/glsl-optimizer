#include "vl_context.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_winsys.h>
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <pipe/p_shader_tokens.h>
#include <tgsi/util/tgsi_parse.h>
#include <tgsi/util/tgsi_build.h>
#include "vl_shader_build.h"
#include "vl_data.h"

static int vlInitIDCT(struct VL_CONTEXT *context)
{
	assert(context);
	
	
	
	return 0;
}

static int vlDestroyIDCT(struct VL_CONTEXT *context)
{
	assert(context);
	
	
	
	return 0;
}

static int vlCreateVertexShaderIMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 50;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.mc.i_vs = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderIMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 50;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 */
	for (i = 0; i < 2; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 */
	for (i = 0; i < 3; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header,max_tokens - ti);
	}
	
	/*
	 * tex2d o0.x, i0, s0		; Read texel from luma texture into .x channel
	 * tex2d o0.y, i1, s1		; Read texel from chroma Cb texture into .y channel
	 * tex2d o0.z, i1, s2		; Read texel from chroma Cr texture into .z channel
	*/
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_OUTPUT, 0, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.mc.i_fs = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

static int vlCreateVertexShaderFramePMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Unused
	 * decl c3		; Translation vector to move ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 3);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Ref macroblock texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o3, t0, c3	; Translate rect into position on ref macroblock */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.mc.p_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateVertexShaderFieldPMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration
		(
			&decl,
			&tokens[ti],
			header,
			max_tokens - ti
		);
	}
	
	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Denorm coefficients
	 * decl c3		; Translation vector to move top field ref macroblock texcoords into position
	 * decl c4		; Translation vector to move bottom field ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 4);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Top field ref macroblock texcoords
	 * decl o4		; Bottom field ref macroblock texcoords
	 * decl o5		; Denormalized vertex pos
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_output((i == 0 || i == 5) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add t1, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mov o0, t1		; Move vertex pos to output */
	inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	mov o1, i1		; Move input luma texcoords to output
	mov o2, i2		; Move input chroma texcoords to output
	*/
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o3, t0, c3	; Translate top field rect into position on ref macroblock
	   add o4, t0, c4	; Translate bottom field rect into position on ref macroblock */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul o5, t1, c2	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 5, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.mc.p_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderFramePMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 */
	for (i = 0; i < 3; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0			; Multiplier to shift 9th bit of differential into place
	 * decl c1			; Bias to get differential back to a signed value
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for ref surface texture
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * tex2d t0.xw, i0, s0		; Read texel from luma texture into .x and .w channels
	 * mov t1.x, t0.w		; Move 9th bit from .w channel to .x
	 * tex2d t0.yw, i1, s1		; Read texel from chroma Cb texture into .y and .w channels
	 * mov t1.y, t0.w		; Move 9th bit from .w channel to .y
	 * tex2d t0.zw, i1, s2		; Read texel from chroma Cr texture into .z and .w channels
	 * mov t1.z, t0.w		; Move 9th bit from .w channel to .z
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		inst.FullDstRegisters[0].DstRegister.WriteMask = (TGSI_WRITEMASK_X << i) | TGSI_WRITEMASK_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
		
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t1, t1, c0		; Muliply 9th bit by multiplier to shift it into place */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add t0, t0, t1		; Add luma and chroma low and high parts to get normalized unsigned 9-bit values */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t0, t0, c1		; Subtract bias to get back signed values */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* tex2d t1, i2, s3		; Read texel from ref macroblock */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 2, TGSI_FILE_SAMPLER, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add o0, t0, t1		; Add ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.mc.p_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderFieldPMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 200;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s3
	 * decl i4			; Denormalized vertex pos
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0			; Multiplier to shift 9th bit of differential into place
	 * decl c1			; Bias to get differential back to a signed value
	 * decl c2			; Constants 1/2 & 2 in .x, .y channels for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 2);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for ref surface texture
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * tex2d t0.xw, i0, s0		; Read texel from luma texture into .x and .w channels
	 * mov t1.x, t0.w		; Move 9th bit from .w channel to .x
	 * tex2d t0.yw, i1, s1		; Read texel from chroma Cb texture into .y and .w channels
	 * mov t1.y, t0.w		; Move 9th bit from .w channel to .y
	 * tex2d t0.zw, i1, s2		; Read texel from chroma Cr texture into .z and .w channels
	 * mov t1.z, t0.w		; Move 9th bit from .w channel to .z
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		inst.FullDstRegisters[0].DstRegister.WriteMask = (TGSI_WRITEMASK_X << i) | TGSI_WRITEMASK_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
		
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t1, t1, c0		; Muliply 9th bit by multiplier to shift it into place */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add t0, t0, t1		; Add luma and chroma low and high parts to get normalized unsigned 9-bit values */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t0, t0, c1		; Subtract bias to get back signed values */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* tex2d t1, i2, s3		; Read texel from ref macroblock top field
	   tex2d t2, i3, s3		; Read texel from ref macroblock bottom field */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* XXX: Pos values off by 0.5? */
	/* sub t4, i4.y, c2.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 4, TGSI_FILE_INPUT, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mul t3, t4, c2.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 2);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mul t3, t3, c2.y		; Multiply by 2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 2);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t3, t4, t3		; Subtract from original Y to get Y % 2 */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add o0, t0, t1		; Add ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.mc.p_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

static int vlCreateVertexShaderFrameBMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Unused
	 * decl c3		; Translation vector to move past ref macroblock texcoords into position
	 * decl c4		; Unused
	 * decl c5		; Translation vector to move future ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 5);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Past ref macroblock texcoords
	 * decl o4		; Future ref macroblock texcoords
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* add o3, t0, c3	; Translate rect into position on past ref macroblock
	   add o4, t0, c5	; Translate rect into position on future ref macroblock */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i * 2 + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.mc.b_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateVertexShaderFieldBMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;	
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Denorm coefficients
	 * decl c3		; Translation vector to move top field past ref macroblock texcoords into position
	 * decl c4		; Translation vector to move bottom field past ref macroblock texcoords into position
	 * decl c5		; Translation vector to move top field future ref macroblock texcoords into position
	 * decl c6		; Translation vector to move bottom field future ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 6);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Top field past ref macroblock texcoords
	 * decl o4		; Bottom field past ref macroblock texcoords
	 * decl o5		; Top field future ref macroblock texcoords
	 * decl o6		; Bottom field future ref macroblock texcoords
	 * decl o7		; Denormalized vertex pos
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_output((i == 0 || i == 7) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add t1, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mov o0, t1		; Move vertex pos to output */
	inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o3, t0, c3	; Translate top field rect into position on past ref macroblock
	 * add o4, t0, c4	; Translate bottom field rect into position on past ref macroblock
	 * add o5, t0, c5	; Translate top field rect into position on future ref macroblock
	 * add o6, t0, c6	; Translate bottom field rect into position on future ref macroblock
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul o7, t1, c2	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 7, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.mc.b_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderFrameBMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s4
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0			; Multiplier to shift 9th bit of differential into place
	 * decl c1			; Bias to get differential back to a signed value
	 * decl c2			; Constant 1/2 in .x channel to use as weight to blend past and future texels
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 2);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for past ref surface texture
	 * decl s4			; Sampler for future ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * tex2d t0.xw, i0, s0		; Read texel from luma texture into .x and .w channels
	 * mov t1.x, t0.w		; Move 9th bit from .w channel to .x
	 * tex2d t0.yw, i1, s1		; Read texel from chroma Cb texture into .y and .w channels
	 * mov t1.y, t0.w		; Move 9th bit from .w channel to .y
	 * tex2d t0.zw, i1, s2		; Read texel from chroma Cr texture into .z and .w channels
	 * mov t1.z, t0.w		; Move 9th bit from .w channel to .z
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		inst.FullDstRegisters[0].DstRegister.WriteMask = (TGSI_WRITEMASK_X << i) | TGSI_WRITEMASK_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
		
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t1, t1, c0		; Muliply 9th bit by multiplier to shift it into place */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add t0, t0, t1		; Add luma and chroma low and high parts to get normalized unsigned 9-bit values */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t0, t0, c1		; Subtract bias to get back signed values */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * tex2d t1, i2, s3		; Read texel from past ref macroblock
	 * tex2d t2, i3, s4		; Read texel from future ref macroblock
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* lerp t1, c2.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add o0, t0, t1		; Add past/future ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.mc.b_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderFieldBMC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 200;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s3
	 * decl i4			; Texcoords for s4
	 * decl i5			; Texcoords for s4
	 * decl i6			; Denormalized vertex pos
	 */
	for (i = 0; i < 7; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * decl c0			; Multiplier to shift 9th bit of differential into place
	 * decl c1			; Bias to get differential back to a signed value
	 * decl c2			; Constants 1/2 & 2 in .x, .y channels to use as weight to blend past and future texels
	 *				; and for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 2);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for past ref surface texture
	 * decl s4			; Sampler for future ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}
	
	/*
	 * tex2d t0.xw, i0, s0		; Read texel from luma texture into .x and .w channels
	 * mov t1.x, t0.w		; Move 9th bit from .w channel to .x
	 * tex2d t0.yw, i1, s1		; Read texel from chroma Cb texture into .y and .w channels
	 * mov t1.y, t0.w		; Move 9th bit from .w channel to .y
	 * tex2d t0.zw, i1, s2		; Read texel from chroma Cr texture into .z and .w channels
	 * mov t1.z, t0.w		; Move 9th bit from .w channel to .z
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		inst.FullDstRegisters[0].DstRegister.WriteMask = (TGSI_WRITEMASK_X << i) | TGSI_WRITEMASK_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
		
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* mul t1, t1, c0		; Muliply 9th bit by multiplier to shift it into place */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add t0, t0, t1		; Add luma and chroma low and high parts to get normalized unsigned 9-bit values */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t0, t0, c1		; Subtract bias to get back signed values */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* XXX: Pos values off by 0.5? */
	/* sub t4, i6.y, c2.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 6, TGSI_FILE_CONSTANT, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mul t3, t4, c2.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 2);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* mul t3, t3, c2.y		; Multiply by 2 */
	inst = vl_inst3( TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 2);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t3, t4, t3		; Subtract from original Y to get Y % 2 */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * tex2d t1, i2, s3		; Read texel from past ref macroblock top field
	 * tex2d t2, i3, s3		; Read texel from past ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * tex2d t4, i4, s4		; Read texel from future ref macroblock top field
	 * tex2d t5, i5, s4		; Read texel from future ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 4, TGSI_FILE_INPUT, i + 4, TGSI_FILE_SAMPLER, 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}
	
	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t2, t3, t4, t5		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 2, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 5);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* lerp t1, c2.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* add o0, t0, t1		; Add past/future ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.mc.b_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

int vlCreateDataBufsMC(struct VL_CONTEXT *context)
{
	struct pipe_context	*pipe;
	unsigned int		i;
	
	assert(context);
	
	pipe = context->pipe;
	
	/* Create our vertex buffer and vertex buffer element */
	context->states.mc.vertex_bufs[0].pitch = sizeof(struct VL_VERTEX2F);
	context->states.mc.vertex_bufs[0].max_index = 23;
	context->states.mc.vertex_bufs[0].buffer_offset = 0;
	context->states.mc.vertex_bufs[0].buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct VL_VERTEX2F) * 24
	);
	
	context->states.mc.vertex_buf_elems[0].src_offset = 0;
	context->states.mc.vertex_buf_elems[0].vertex_buffer_index = 0;
	context->states.mc.vertex_buf_elems[0].nr_components = 2;
	context->states.mc.vertex_buf_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
	
	/* Create our texcoord buffers and texcoord buffer elements */
	/* TODO: Should be able to use 1 texcoord buf for chroma textures, 1 buf for ref surfaces */
	for (i = 1; i < 3; ++i)
	{
		context->states.mc.vertex_bufs[i].pitch = sizeof(struct VL_TEXCOORD2F);
		context->states.mc.vertex_bufs[i].max_index = 23;
		context->states.mc.vertex_bufs[i].buffer_offset = 0;
		context->states.mc.vertex_bufs[i].buffer = pipe->winsys->buffer_create
		(
			pipe->winsys,
			1,
			PIPE_BUFFER_USAGE_VERTEX,
			sizeof(struct VL_TEXCOORD2F) * 24
		);
	
		context->states.mc.vertex_buf_elems[i].src_offset = 0;
		context->states.mc.vertex_buf_elems[i].vertex_buffer_index = i;
		context->states.mc.vertex_buf_elems[i].nr_components = 2;
		context->states.mc.vertex_buf_elems[i].src_format = PIPE_FORMAT_R32G32_FLOAT;
	}
	
	/* Fill buffers */
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.mc.vertex_bufs[0].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		vl_chroma_420_texcoords,
		sizeof(struct VL_VERTEX2F) * 24
	);
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.mc.vertex_bufs[1].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		vl_luma_texcoords,
		sizeof(struct VL_TEXCOORD2F) * 24
	);
	/* TODO: Accomodate 422, 444 */
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.mc.vertex_bufs[2].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		vl_chroma_420_texcoords,
		sizeof(struct VL_TEXCOORD2F) * 24
	);
	
	for (i = 0; i < 3; ++i)
		pipe->winsys->buffer_unmap(pipe->winsys, context->states.mc.vertex_bufs[i].buffer);
	
	/* Create our constant buffer */
	context->states.mc.vs_const_buf.size = sizeof(struct VL_MC_VS_CONSTS);
	context->states.mc.vs_const_buf.buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		context->states.mc.vs_const_buf.size
	);
	
	context->states.mc.fs_const_buf.size = sizeof(struct VL_MC_FS_CONSTS);
	context->states.mc.fs_const_buf.buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		context->states.mc.fs_const_buf.size
	);
	
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.mc.fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		&vl_mc_fs_consts,
		sizeof(struct VL_MC_FS_CONSTS)
	);
	
	pipe->winsys->buffer_unmap(pipe->winsys, context->states.mc.fs_const_buf.buffer);
	
	return 0;
}

static int vlInitMC(struct VL_CONTEXT *context)
{	
	struct pipe_context		*pipe;
	struct pipe_sampler_state	sampler;
	struct pipe_texture		template;
	unsigned int			filters[5];
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	
	context->states.mc.viewport.scale[0] = context->video_width;
	context->states.mc.viewport.scale[1] = context->video_height;
	context->states.mc.viewport.scale[2] = 1;
	context->states.mc.viewport.scale[3] = 1;
	context->states.mc.viewport.translate[0] = 0;
	context->states.mc.viewport.translate[1] = 0;
	context->states.mc.viewport.translate[2] = 0;
	context->states.mc.viewport.translate[3] = 0;
	
	context->states.mc.render_target.width = context->video_width;
	context->states.mc.render_target.height = context->video_height;
	context->states.mc.render_target.num_cbufs = 1;
	/* FB for MC stage is a VL_SURFACE, set in vlSetRenderSurface() */
	context->states.mc.render_target.zsbuf = NULL;
	
	filters[0] = PIPE_TEX_FILTER_NEAREST;
	filters[1] = context->video_format == VL_FORMAT_YCBCR_444 ? PIPE_TEX_FILTER_NEAREST : PIPE_TEX_FILTER_LINEAR;
	filters[2] = context->video_format == VL_FORMAT_YCBCR_444 ? PIPE_TEX_FILTER_NEAREST : PIPE_TEX_FILTER_LINEAR;
	filters[3] = PIPE_TEX_FILTER_LINEAR;
	filters[4] = PIPE_TEX_FILTER_LINEAR;
	
	for (i = 0; i < 5; ++i)
	{
		sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.min_img_filter = filters[i];
		sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
		sampler.mag_img_filter = filters[i];
		sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
		sampler.compare_func = PIPE_FUNC_ALWAYS;
		sampler.normalized_coords = 1;
		/*sampler.prefilter = ;*/
		/*sampler.shadow_ambient = ;*/
		/*sampler.lod_bias = ;*/
		sampler.min_lod = 0;
		/*sampler.max_lod = ;*/
		/*sampler.border_color[i] = ;*/
		/*sampler.max_anisotropy = ;*/
		context->states.mc.samplers[i] = pipe->create_sampler_state(pipe, &sampler);
	}
	
	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_A8L8_UNORM;
	template.last_level = 0;
	template.width[0] = 8;
	template.height[0] = 8 * 4;
	template.depth[0] = 1;
	template.compressed = 0;
	template.cpp = 2;
	
	context->states.mc.textures[0] = pipe->screen->texture_create(pipe->screen, &template);
	
	if (context->video_format == VL_FORMAT_YCBCR_420)
		template.height[0] = 8;
	else if (context->video_format == VL_FORMAT_YCBCR_422)
		template.height[0] = 8 * 2;
	else if (context->video_format == VL_FORMAT_YCBCR_444)
		template.height[0] = 8 * 4;
	else
		assert(0);
		
	context->states.mc.textures[1] = pipe->screen->texture_create(pipe->screen, &template);
	context->states.mc.textures[2] = pipe->screen->texture_create(pipe->screen, &template);
	
	/* textures[3] & textures[4] are assigned from VL_SURFACEs for P and B macroblocks at render time */
	
	vlCreateVertexShaderIMC(context);
	vlCreateFragmentShaderIMC(context);
	vlCreateVertexShaderFramePMC(context);
	vlCreateVertexShaderFieldPMC(context);
	vlCreateFragmentShaderFramePMC(context);
	vlCreateFragmentShaderFieldPMC(context);
	vlCreateVertexShaderFrameBMC(context);
	vlCreateVertexShaderFieldBMC(context);
	vlCreateFragmentShaderFrameBMC(context);
	vlCreateFragmentShaderFieldBMC(context);
	vlCreateDataBufsMC(context);
	
	return 0;
}

static int vlDestroyMC(struct VL_CONTEXT *context)
{
	unsigned int i;
	
	assert(context);
	
	for (i = 0; i < 5; ++i)
		context->pipe->delete_sampler_state(context->pipe, context->states.mc.samplers[i]);
	
	for (i = 0; i < 3; ++i)
		context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.mc.vertex_bufs[i].buffer);
	
	/* Textures 3 & 4 are not created directly, no need to release them here */
	for (i = 0; i < 3; ++i)
		pipe_texture_release(&context->states.mc.textures[i]);
	
	context->pipe->delete_vs_state(context->pipe, context->states.mc.i_vs);
	context->pipe->delete_fs_state(context->pipe, context->states.mc.i_fs);
	
	for (i = 0; i < 2; ++i)
	{
		context->pipe->delete_vs_state(context->pipe, context->states.mc.p_vs[i]);
		context->pipe->delete_fs_state(context->pipe, context->states.mc.p_fs[i]);
		context->pipe->delete_vs_state(context->pipe, context->states.mc.b_vs[i]);
		context->pipe->delete_fs_state(context->pipe, context->states.mc.b_fs[i]);
	}
	
	context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.mc.vs_const_buf.buffer);
	context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.mc.fs_const_buf.buffer);
	
	return 0;
}

static int vlCreateVertexShaderCSC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 50;
	
	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;
	
	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Vertex texcoords
	 */
	for (i = 0; i < 2; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Vertex texcoords
	 */
	for (i = 0; i < 2; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move pos in to pos out
	 * mov o1, i1		; Move input texcoords to output
	 */
	for (i = 0; i < 2; i++)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	vs.tokens = tokens;
	context->states.csc.vertex_shader = pipe->create_vs_state(pipe, &vs);
	free(tokens);
	
	return 0;
}

static int vlCreateFragmentShaderCSC(struct VL_CONTEXT *context)
{
	const unsigned int		max_tokens = 50;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;
	
	unsigned int			ti;
	unsigned int			i;
	
	assert(context);
	
	pipe = context->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/* decl i0		; Texcoords for s0 */
	decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, 1, 0, 0, TGSI_INTERPOLATE_LINEAR);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * decl c0		; Bias vector for CSC
	 * decl c1-c4		; CSC matrix c1-c4
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 4);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0		; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/* decl s0		; Sampler for tex containing picture to display */
	decl = vl_decl_samplers(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	
	/* tex2d t0, i0, s0	; Read src pixel */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/* sub t0, t0, c0	; Subtract bias vector from pixel */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	
	/*
	 * dp4 o0.x, t0, c1	; Multiply pixel by the color conversion matrix
	 * dp4 o0.y, t0, c2
	 * dp4 o0.z, t0, c3
	 * dp4 o0.w, t0, c4	; XXX: Don't need 4th coefficient
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_DP4, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 1);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	context->states.csc.fragment_shader = pipe->create_fs_state(pipe, &fs);
	free(tokens);
	
	return 0;
}

static int vlCreateDataBufsCSC(struct VL_CONTEXT *context)
{
	struct pipe_context *pipe;
	
	assert(context);
	
	pipe = context->pipe;
	
	/*
	Create our vertex buffer and vertex buffer element
	VB contains 4 vertices that render a quad covering the entire window
	to display a rendered surface
	Quad is rendered as a tri strip
	*/
	context->states.csc.vertex_bufs[0].pitch = sizeof(struct VL_VERTEX2F);
	context->states.csc.vertex_bufs[0].max_index = 3;
	context->states.csc.vertex_bufs[0].buffer_offset = 0;
	context->states.csc.vertex_bufs[0].buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct VL_VERTEX2F) * 4
	);
	
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.csc.vertex_bufs[0].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		vl_surface_vertex_positions,
		sizeof(struct VL_VERTEX2F) * 4
	);
	
	pipe->winsys->buffer_unmap(pipe->winsys, context->states.csc.vertex_bufs[0].buffer);
	
	context->states.csc.vertex_buf_elems[0].src_offset = 0;
	context->states.csc.vertex_buf_elems[0].vertex_buffer_index = 0;
	context->states.csc.vertex_buf_elems[0].nr_components = 2;
	context->states.csc.vertex_buf_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
	
	/*
	Create our texcoord buffer and texcoord buffer element
	Texcoord buffer contains the TCs for mapping the rendered surface to the 4 vertices
	*/
	context->states.csc.vertex_bufs[1].pitch = sizeof(struct VL_TEXCOORD2F);
	context->states.csc.vertex_bufs[1].max_index = 3;
	context->states.csc.vertex_bufs[1].buffer_offset = 0;
	context->states.csc.vertex_bufs[1].buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct VL_TEXCOORD2F) * 4
	);
	
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.csc.vertex_bufs[1].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		vl_surface_texcoords,
		sizeof(struct VL_TEXCOORD2F) * 4
	);
	
	pipe->winsys->buffer_unmap(pipe->winsys, context->states.csc.vertex_bufs[1].buffer);
	
	context->states.csc.vertex_buf_elems[1].src_offset = 0;
	context->states.csc.vertex_buf_elems[1].vertex_buffer_index = 1;
	context->states.csc.vertex_buf_elems[1].nr_components = 2;
	context->states.csc.vertex_buf_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;
	
	/*
	Create our fragment shader's constant buffer
	Const buffer contains the color conversion matrix and bias vectors
	*/
	context->states.csc.fs_const_buf.size = sizeof(struct VL_CSC_FS_CONSTS);
	context->states.csc.fs_const_buf.buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		context->states.csc.fs_const_buf.size
	);
	
	/*
	TODO: Refactor this into a seperate function,
	allow changing the CSC matrix at runtime to switch between regular & full versions
	*/
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, context->states.csc.fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		&vl_csc_fs_consts_601,
		sizeof(struct VL_CSC_FS_CONSTS)
	);
	
	pipe->winsys->buffer_unmap(pipe->winsys, context->states.csc.fs_const_buf.buffer);
	
	return 0;
}

static int vlInitCSC(struct VL_CONTEXT *context)
{	
	struct pipe_context		*pipe;
	struct pipe_sampler_state	sampler;
	
	assert(context);
	
	pipe = context->pipe;
	
	/* Delay creating the FB until vlPutSurface() so we know window size */
	context->states.csc.framebuffer.num_cbufs = 1;
	context->states.csc.framebuffer.cbufs[0] = NULL;
	context->states.csc.framebuffer.zsbuf = NULL;

	sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
	sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
	sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
	sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
	sampler.compare_func = PIPE_FUNC_ALWAYS;
	sampler.normalized_coords = 1;
	/*sampler.prefilter = ;*/
	/*sampler.shadow_ambient = ;*/
	/*sampler.lod_bias = ;*/
	/*sampler.min_lod = ;*/
	/*sampler.max_lod = ;*/
	/*sampler.border_color[i] = ;*/
	/*sampler.max_anisotropy = ;*/
	context->states.csc.sampler = pipe->create_sampler_state(pipe, &sampler);
	
	vlCreateVertexShaderCSC(context);
	vlCreateFragmentShaderCSC(context);
	vlCreateDataBufsCSC(context);
	
	return 0;
}

static int vlDestroyCSC(struct VL_CONTEXT *context)
{
	assert(context);
	
	/*
	Since we create the final FB when we display our first surface,
	it may not be created if vlPutSurface() is never called
	*/
	if (context->states.csc.framebuffer.cbufs[0])
		context->pipe->winsys->surface_release(context->pipe->winsys, &context->states.csc.framebuffer.cbufs[0]);
	context->pipe->delete_sampler_state(context->pipe, context->states.csc.sampler);
	context->pipe->delete_vs_state(context->pipe, context->states.csc.vertex_shader);
	context->pipe->delete_fs_state(context->pipe, context->states.csc.fragment_shader);
	context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.csc.vertex_bufs[0].buffer);
	context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.csc.vertex_bufs[1].buffer);
	context->pipe->winsys->buffer_destroy(context->pipe->winsys, context->states.csc.fs_const_buf.buffer);
	
	return 0;
}

static int vlInitCommon(struct VL_CONTEXT *context)
{
	struct pipe_context			*pipe;
	struct pipe_rasterizer_state		rast;
	struct pipe_blend_state			blend;
	struct pipe_depth_stencil_alpha_state	dsa;
	unsigned int				i;
	
	assert(context);
	
	pipe = context->pipe;
	
	rast.flatshade = 1;
	rast.flatshade_first = 0;
	rast.light_twoside = 0;
	rast.front_winding = PIPE_WINDING_CCW;
	rast.cull_mode = PIPE_WINDING_CW;
	rast.fill_cw = PIPE_POLYGON_MODE_FILL;
	rast.fill_ccw = PIPE_POLYGON_MODE_FILL;
	rast.offset_cw = 0;
	rast.offset_ccw = 0;
	rast.scissor = 0;
	rast.poly_smooth = 0;
	rast.poly_stipple_enable = 0;
	rast.point_sprite = 0;
	rast.point_size_per_vertex = 0;
	rast.multisample = 0;
	rast.line_smooth = 0;
	rast.line_stipple_enable = 0;
	rast.line_stipple_factor = 0;
	rast.line_stipple_pattern = 0;
	rast.line_last_pixel = 0;
	/* Don't need clipping, but viewport mapping done here */
	rast.bypass_clipping = 0;
	rast.bypass_vs = 0;
	rast.origin_lower_left = 0;
	rast.line_width = 1;
	rast.point_smooth = 0;
	rast.point_size = 1;
	rast.offset_units = 1;
	rast.offset_scale = 1;
	/*rast.sprite_coord_mode[i] = ;*/
	context->states.common.raster = pipe->create_rasterizer_state(pipe, &rast);
	pipe->bind_rasterizer_state(pipe, context->states.common.raster);
	
	blend.blend_enable = 0;
	blend.rgb_func = PIPE_BLEND_ADD;
	blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
	blend.rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
	blend.alpha_func = PIPE_BLEND_ADD;
	blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
	blend.alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
	blend.logicop_enable = 0;
	blend.logicop_func = PIPE_LOGICOP_CLEAR;
	/* Needed to allow color writes to FB, even if blending disabled */
	blend.colormask = PIPE_MASK_RGBA;
	blend.dither = 0;
	context->states.common.blend = pipe->create_blend_state(pipe, &blend);
	pipe->bind_blend_state(pipe, context->states.common.blend);
	
	dsa.depth.enabled = 0;
	dsa.depth.writemask = 0;
	dsa.depth.func = PIPE_FUNC_ALWAYS;
	dsa.depth.occlusion_count = 0;
	for (i = 0; i < 2; ++i)
	{
		dsa.stencil[i].enabled = 0;
		dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
		dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].ref_value = 0;
		dsa.stencil[i].value_mask = 0;
		dsa.stencil[i].write_mask = 0;
	}
	dsa.alpha.enabled = 0;
	dsa.alpha.func = PIPE_FUNC_ALWAYS;
	dsa.alpha.ref = 0;
	context->states.common.dsa = pipe->create_depth_stencil_alpha_state(pipe, &dsa);
	pipe->bind_depth_stencil_alpha_state(pipe, context->states.common.dsa);
	
	return 0;
}

static int vlDestroyCommon(struct VL_CONTEXT *context)
{
	assert(context);
	
	context->pipe->delete_blend_state(context->pipe, context->states.common.blend);
	context->pipe->delete_rasterizer_state(context->pipe, context->states.common.raster);
	context->pipe->delete_depth_stencil_alpha_state(context->pipe, context->states.common.dsa);
	
	return 0;
}

static int vlInit(struct VL_CONTEXT *context)
{
	assert(context);
	
	vlInitCommon(context);
	vlInitCSC(context);
	vlInitMC(context);
	vlInitIDCT(context);
	
	return 0;
}

static int vlDestroy(struct VL_CONTEXT *context)
{
	assert(context);
	
	/* XXX: Must unbind shaders before we can delete them for some reason */
	context->pipe->bind_vs_state(context->pipe, NULL);
	context->pipe->bind_fs_state(context->pipe, NULL);
	
	vlDestroyCommon(context);
	vlDestroyCSC(context);
	vlDestroyMC(context);
	vlDestroyIDCT(context);
	
	return 0;
}

int vlCreateContext
(
	Display *display,
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum VL_FORMAT video_format,
	struct VL_CONTEXT **context
)
{
	struct VL_CONTEXT *ctx;
	
	assert(display);
	assert(pipe);
	assert(context);
	
	ctx = calloc(1, sizeof(struct VL_CONTEXT));
	
	ctx->display = display;
	ctx->pipe = pipe;
	ctx->video_width = video_width;
	ctx->video_height = video_height;
	ctx->video_format = video_format;
	
	vlInit(ctx);
	
	/* Since we only change states in vlPutSurface() we need to start in render mode */
	vlBeginRender(ctx);
	
	*context = ctx;
	
	return 0;
}

int vlDestroyContext(struct VL_CONTEXT *context)
{
	assert(context);
	
	vlDestroy(context);
	
	context->pipe->destroy(context->pipe);
	
	free(context);
	
	return 0;
}

int vlBeginRender(struct VL_CONTEXT *context)
{
	struct pipe_context	*pipe;
	
	assert(context);
	
	pipe = context->pipe;
	
	/* Frame buffer set in vlRender*Macroblock() */
	/* Shaders, samplers, textures set in vlRender*Macroblock() */
	pipe->set_vertex_buffers(pipe, 3, context->states.mc.vertex_bufs);
	pipe->set_vertex_elements(pipe, 3, context->states.mc.vertex_buf_elems);
	pipe->set_viewport_state(pipe, &context->states.mc.viewport);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, &context->states.mc.vs_const_buf);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &context->states.mc.fs_const_buf);
	
	return 0;
}

int vlEndRender(struct VL_CONTEXT *context)
{
	struct pipe_context *pipe;
	
	assert(context);
	
	pipe = context->pipe;
	
	pipe->set_framebuffer_state(pipe, &context->states.csc.framebuffer);
	pipe->set_viewport_state(pipe, &context->states.csc.viewport);
	pipe->bind_sampler_states(pipe, 1, (void**)&context->states.csc.sampler);
	/* Source texture set in vlPutSurface() */
	pipe->bind_vs_state(pipe, context->states.csc.vertex_shader);
	pipe->bind_fs_state(pipe, context->states.csc.fragment_shader);
	pipe->set_vertex_buffers(pipe, 2, context->states.csc.vertex_bufs);
	pipe->set_vertex_elements(pipe, 2, context->states.csc.vertex_buf_elems);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &context->states.csc.fs_const_buf);
	
	return 0;
}

