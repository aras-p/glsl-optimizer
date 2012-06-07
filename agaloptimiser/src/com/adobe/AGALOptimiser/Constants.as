/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

package com.adobe.AGALOptimiser {

[ExcludeClass]
public class Constants
{
    use namespace nsinternal;

    nsinternal static const ALLOCATE_REGISTERS_OPTION               : String = "allocateRegisters";
    nsinternal static const END_NUMERICAL_CONSTANT_REGISTERS_TAG    : String = "end_numerical_constant_registers";
    nsinternal static const END_MATRIX_REGISTERS_TAG                : String = "end_matrix_registers";
    nsinternal static const END_PROGRAM_CONSTANT_REGISTERS_TAG      : String = "end_program_constant_registers";
    nsinternal static const END_REGISTER_MAP_TAG                    : String = "end_register_map";
    nsinternal static const END_TEXTURE_SAMPLER_REGISTERS_TAG       : String = "end_texture_sampler_units";
    nsinternal static const END_UNUSED_INPUT_VERTICES_TAG           : String = "end_unused_input_vertices";
    nsinternal static const END_UNUSED_PARAMETERS_TAG               : String = "end_unused_parameters";
    nsinternal static const END_UNUSED_TEXTURES_TAG                 : String = "end_unused_textures";
    nsinternal static const END_VERTEX_BUFFERS_TAG                  : String = "end_vertex_buffers";
    nsinternal static const MATRX_REGISTERS_TAG                     : String = "matrix_registers";
    nsinternal static const NUMERICAL_CONSTANT_REGISTERS_TAG        : String = "numerical_constant_registers";
    nsinternal static const PACK_REGISTERS_OPTION                   : String = "packRegisters";
    nsinternal static const PARAMETER_TAG                           : String = "parameter";
    nsinternal static const PERFORM_PEEPHOLE_OPTIMIZATION_OPTION    : String = "performPeepholeOptimization";
    nsinternal static const PROGRAM_CONSTANT_REGISTERS_TAG          : String = "program_constant_registers";
    nsinternal static const REGISTER_MAP_TAG                        : String = "register_map";
    nsinternal static const SEMANTICS_TAG                           : String = "semantics";
    nsinternal static const TEXTURE_REGISTER_INFO_TAG               : String = "texture_info";
    nsinternal static const TEXTURE_SAMPLER_UNITS_TAG               : String = "texture_sampler_units";
    nsinternal static const TEXTURE_SWIZZLE_MARSHALLING_OPTION      : String = "textureSwizzleMarshalling";
    nsinternal static const TWO_CONSTANT_OPERAND_MARSHALLING_OPTION : String = "twoConstantOperandMarshalling";
    nsinternal static const UNUSED_INPUT_VERTICES_TAG               : String = "unused_input_vertices";
    nsinternal static const UNUSED_PARAMETERS_TAG                   : String = "unused_parameters";
    nsinternal static const UNUSED_TEXTURES_TAG                     : String = "unused_textures";
    nsinternal static const VERTEX_BUFFERS_TAG                      : String = "vertex_buffers";
    nsinternal static const VERTEX_REGISTER_INFO_TAG                : String = "vertex_info";

} // Constants
} // com.adobe.AGALOptimiser
