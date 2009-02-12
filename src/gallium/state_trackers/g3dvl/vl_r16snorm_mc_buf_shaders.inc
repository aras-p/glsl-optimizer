static int vlCreateVertexShaderIMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->i_vs = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderIMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 */
	for (i = 0; i < 3; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
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
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->i_fs = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFramePMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4		; Ref surface top field texcoords
	 * decl i5		; Ref surface bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; Ref macroblock texcoords
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o4, i0, i4	; Translate vertex pos by motion vec to form ref macroblock texcoords */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, 4);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->p_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFieldPMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; Ref macroblock top field texcoords
	 * decl i5              ; Ref macroblock bottom field texcoords
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Render target dimensions */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; Ref macroblock top field texcoords
	 * decl o5		; Ref macroblock bottom field texcoords
	 * decl o6		; Denormalized vertex pos
	 */
	for (i = 0; i < 7; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form top field macroblock texcoords
	 * add o5, i0, i5	; Translate vertex pos by motion vec to form bottom field macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o6, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 6, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->p_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFramePMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; Ref macroblock texcoords
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
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
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* tex2d t1, i3, s3		; Read texel from ref macroblock */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 3, TGSI_FILE_SAMPLER, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, t1		; Add ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->p_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFieldPMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; Ref macroblock top field texcoords
	 * decl i4			; Ref macroblock bottom field texcoords
	 * decl i5			; Denormalized vertex pos
	 */
	for (i = 0; i < 6; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constants 1/2 & 2 in .x, .y channels for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t4 */
	decl = vl_decl_temps(0, 4);
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
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i3, s3		; Read texel from ref macroblock top field
	 * tex2d t2, i4, s3		; Read texel from ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* XXX: Pos values off by 0.5? */
	/* sub t4, i5.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 5, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t4, c1.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t3, c1.y		; Multiply by 2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 1);
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
	mc->p_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFrameBMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; First ref macroblock top field texcoords
	 * decl i5              ; First ref macroblock bottom field texcoords (unused, packed in the same stream)
	 * decl i6		; Second ref macroblock top field texcoords
	 * decl i7		; Second ref macroblock bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; First ref macroblock texcoords
	 * decl o5		; Second ref macroblock texcoords
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form first ref macroblock texcoords
	 * add o5, i0, i6	; Translate vertex pos by motion vec to form second ref macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, (i + 2) * 2);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->b_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFieldBMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; First ref macroblock top field texcoords
	 * decl i5              ; First ref macroblock bottom field texcoords
	 * decl i6              ; Second ref macroblock top field texcoords
	 * decl i7              ; Second ref macroblock bottom field texcoords
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Render target dimensions */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; First ref macroblock top field texcoords
	 * decl o5		; First ref macroblock Bottom field texcoords
	 * decl o6		; Second ref macroblock top field texcoords
	 * decl o7		; Second ref macroblock Bottom field texcoords
	 * decl o8		; Denormalized vertex pos
	 */
	for (i = 0; i < 9; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form first top field macroblock texcoords
	 * add o5, i0, i5	; Translate vertex pos by motion vec to form first bottom field macroblock texcoords
	 * add o6, i0, i6	; Translate vertex pos by motion vec to form second top field macroblock texcoords
	 * add o7, i0, i7	; Translate vertex pos by motion vec to form second bottom field macroblock texcoords
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o8, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 8, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->b_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFrameBMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; First ref macroblock texcoords
	 * decl i4			; Second ref macroblock texcoords
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constant 1/2 in .x channel to use as weight to blend past and future texels
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t2 */
	decl = vl_decl_temps(0, 2);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for first ref surface texture
	 * decl s4			; Sampler for second ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i3, s3		; Read texel from first ref macroblock
	 * tex2d t2, i4, s4		; Read texel from second ref macroblock
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* lerp t1, c1.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
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
	mc->b_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFieldBMB
(
	struct vlR16SnormBufferedMC *mc
)
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

	assert(mc);

	pipe = mc->pipe;
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; First ref macroblock top field texcoords
	 * decl i4			; First ref macroblock bottom field texcoords
	 * decl i5			; Second ref macroblock top field texcoords
	 * decl i6			; Second ref macroblock bottom field texcoords
	 * decl i7			; Denormalized vertex pos
	 */
	for (i = 0; i < 8; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constants 1/2 & 2 in .x, .y channels to use as weight to blend past and future texels
	 *				; and for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t5 */
	decl = vl_decl_temps(0, 5);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for first ref surface texture
	 * decl s4			; Sampler for second ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* XXX: Pos values off by 0.5? */
	/* sub t4, i7.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 7, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t4, c1.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t3, c1.y		; Multiply by 2 */
	inst = vl_inst3( TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* sub t3, t4, t3		; Subtract from original Y to get Y % 2 */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i3, s3		; Read texel from past ref macroblock top field
	 * tex2d t2, i4, s3		; Read texel from past ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t4, i5, s4		; Read texel from future ref macroblock top field
	 * tex2d t5, i6, s4		; Read texel from future ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 4, TGSI_FILE_INPUT, i + 5, TGSI_FILE_SAMPLER, 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t2, t3, t4, t5		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 2, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 5);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* lerp t1, c1.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
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
	mc->b_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}
