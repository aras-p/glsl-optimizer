/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "tpf.h"

// TODO: we should fix this to output the same syntax as fxc, if tpf_dump_ms_syntax is set

bool tpf_dump_ms_syntax = true;

std::ostream& operator <<(std::ostream& out, const tpf_op& op)
{
	if(op.neg)
		out << '-';
	if(op.abs)
		out << '|';
	if(op.file == TPF_FILE_IMMEDIATE32)
	{
		out << "l(";
		for(unsigned i = 0; i < op.comps; ++i)
		{
			if(i)
				out << ", ";
			out << op.imm_values[i].f32;
		}
		out << ")";
	}
	else if(op.file == TPF_FILE_IMMEDIATE64)
	{
		out << "d(";
		for(unsigned i = 0; i < op.comps; ++i)
		{
			if(i)
				out << ", ";
			out << op.imm_values[i].f64;
		}
		out << ")";
		return out;
	}
	else
	{
		bool naked = false;
		if(tpf_dump_ms_syntax)
		{
			switch(op.file)
			{
			case TPF_FILE_TEMP:
			case TPF_FILE_INPUT:
			case TPF_FILE_OUTPUT:
			case TPF_FILE_CONSTANT_BUFFER:
			case TPF_FILE_INDEXABLE_TEMP:
			case TPF_FILE_UNORDERED_ACCESS_VIEW:
			case TPF_FILE_THREAD_GROUP_SHARED_MEMORY:
				naked = true;
				break;
			default:
				naked = false;
				break;
			}
		}

		out << (tpf_dump_ms_syntax ? tpf_file_ms_names : tpf_file_names)[op.file];

		if(op.indices[0].reg.get())
			naked = false;

		for(unsigned i = 0; i < op.num_indices; ++i)
		{
			if(!naked || i)
				out << '[';
			if(op.indices[i].reg.get())
			{
				out << *op.indices[i].reg;
				if(op.indices[i].disp)
					out << '+' << op.indices[i].disp;
			}
			else
				out << op.indices[i].disp;
			if(!naked || i)
				out << ']';
		}
		if(op.comps)
		{
			switch(op.mode)
			{
			case TPF_OPERAND_MODE_MASK:
				out << (tpf_dump_ms_syntax ? '.' : '!');
				for(unsigned i = 0; i < op.comps; ++i)
				{
					if(op.mask & (1 << i))
						out << "xyzw"[i];
				}
				break;
			case TPF_OPERAND_MODE_SWIZZLE:
				out << '.';
				for(unsigned i = 0; i < op.comps; ++i)
					out << "xyzw"[op.swizzle[i]];
				break;
			case TPF_OPERAND_MODE_SCALAR:
				out << (tpf_dump_ms_syntax ? '.' : ':');
				out << "xyzw"[op.swizzle[0]];
				break;
			}
		}
	}
	if(op.abs)
		out << '|';
	return out;
}

std::ostream& operator <<(std::ostream& out, const tpf_dcl& dcl)
{
	out << tpf_opcode_names[dcl.opcode];
	switch(dcl.opcode)
	{
	case TPF_OPCODE_DCL_GLOBAL_FLAGS:
		if(dcl.dcl_global_flags.allow_refactoring)
			out << " refactoringAllowed";
		if(dcl.dcl_global_flags.early_depth_stencil)
			out << " forceEarlyDepthStencil";
		if(dcl.dcl_global_flags.fp64)
			out << " enableDoublePrecisionFloatOps";
		if(dcl.dcl_global_flags.enable_raw_and_structured_in_non_cs)
			out << " enableRawAndStructuredBuffers";
		break;
	case TPF_OPCODE_DCL_INPUT_PS:
	case TPF_OPCODE_DCL_INPUT_PS_SIV:
	case TPF_OPCODE_DCL_INPUT_PS_SGV:
		out << ' ' << tpf_interpolation_names[dcl.dcl_input_ps.interpolation];
		break;
	case TPF_OPCODE_DCL_TEMPS:
		out << ' ' << dcl.num;
		break;
	default:
		break;
	}
	if(dcl.op.get())
		out << ' ' << *dcl.op;
	switch(dcl.opcode)
	{
	case TPF_OPCODE_DCL_CONSTANT_BUFFER:
		out << ", " << (dcl.dcl_constant_buffer.dynamic ? "dynamicIndexed" : "immediateIndexed");
		break;
	case TPF_OPCODE_DCL_INPUT_SIV:
	case TPF_OPCODE_DCL_INPUT_SGV:
	case TPF_OPCODE_DCL_OUTPUT_SIV:
	case TPF_OPCODE_DCL_OUTPUT_SGV:
	case TPF_OPCODE_DCL_INPUT_PS_SIV:
	case TPF_OPCODE_DCL_INPUT_PS_SGV:
		out << ", " << tpf_sv_names[dcl.num];
		break;
	}

	return out;
}

std::ostream& operator <<(std::ostream& out, const tpf_insn& insn)
{
	out << tpf_opcode_names[insn.opcode];
	if(insn.insn.sat)
		out << "_sat";
	for(unsigned i = 0; i < insn.num_ops; ++i)
	{
		if(i)
			out << ',';
		out << ' ' << *insn.ops[i];
	}
	return out;
}

std::ostream& operator <<(std::ostream& out, const tpf_program& program)
{
	out << "pvghdc"[program.version.type] << "s_" << program.version.major << "_" << program.version.minor << "\n";
	for(unsigned i = 0; i < program.dcls.size(); ++i)
		out << *program.dcls[i] << "\n";

	for(unsigned i = 0; i < program.insns.size(); ++i)
		out << *program.insns[i] << "\n";
	return out;
}

void tpf_op::dump()
{
	std::cout << *this;
}

void tpf_insn::dump()
{
	std::cout << *this;
}

void tpf_dcl::dump()
{
	std::cout << *this;
}

void tpf_program::dump()
{
	std::cout << *this;
}
