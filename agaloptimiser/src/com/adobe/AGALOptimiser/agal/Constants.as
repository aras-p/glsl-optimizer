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

package com.adobe.AGALOptimiser.agal {

import com.adobe.AGALOptimiser.nsinternal;

[ExcludeClass]
internal final class Constants
{
    nsinternal static const BASIC_BLOCK_TAG             : String = "basic_block";
    nsinternal static const CONST_TAG                   : String = "const";
    nsinternal static const DEFINITION_LINE_TAG         : String = "definitionLine";
    nsinternal static const DEFINITION_LINES_TAG        : String = "lines";
    nsinternal static const END_BASIC_BLOCK_TAG         : String = "end_basic_block";
    nsinternal static const END_OPERATIONS_TAG          : String = "end_operations";
    nsinternal static const END_OUTPUTS_TAG             : String = "end_outputs";
    nsinternal static const END_TAG_PREFIX              : String = "end_";
    nsinternal static const END_PROCEDURE_TAG           : String = "end_procedure";
    nsinternal static const END_REGISTERS_TAG           : String = "end_registers";
    nsinternal static const ENTERED_FROM_TAG            : String = "enteredFrom";
    nsinternal static const EVALUATE_FRAGMENT_TAG       : String = "evaluateFragment";
    nsinternal static const EVALUATE_VERTEX_TAG         : String = "evaluateVertex";
    nsinternal static const EXITS_TO_TAG                : String = "exitsTo";
    nsinternal static const FALSE_TAG                   : String = "false";
    nsinternal static const FRAGMENT_PROGRAM_TAG        : String = "fragment_program";
    nsinternal static const ID_TAG                      : String = "id";
    nsinternal static const INTERPOLATED_TAG            : String = "interpolated";
    nsinternal static const LINEAR_TAG                  : String = "linear";
    nsinternal static const LIVE_RANGE_TAG              : String = "liveRange";
    nsinternal static const NEAREST_TAG                 : String = "nearest";
    nsinternal static const OPERATIONS_TAG              : String = "operations";
    nsinternal static const OUTPUT_TAG                  : String = "output";
    nsinternal static const OUTPUTS_TAG                 : String = "outputs";
    nsinternal static const PARAMETER_TAG               : String = "parameter";
    nsinternal static const PROCEDURE_TAG               : String = "procedure";
    nsinternal static const REGISTER_TAG                : String = "reg";
    nsinternal static const REGISTERS_TAG               : String = "registers";
    nsinternal static const SAMPLE_DIMENSION_ASM_2D_TAG : String = "2D";
    nsinternal static const SAMPLE_DIMENSION_ASM_3D_TAG : String = "3D";
    nsinternal static const SAMPLE_DIMENSION_2D_TAG     : String = "2d";
    nsinternal static const SAMPLE_DIMENSION_3D_TAG     : String = "3d";
    nsinternal static const SAMPLE_DIMENSION_CUBE_TAG   : String = "cube";
    nsinternal static const SAMPLE_FILTER_LINEAR_TAG    : String = "linear";
    nsinternal static const SAMPLE_FILTER_NEAREST_TAG   : String = "nearest";
    nsinternal static const SAMPLE_MIP_DISABLE_TAG      : String = "mipdisable";
    nsinternal static const SAMPLE_MIP_LINEAR_TAG       : String = "miplinear";
    nsinternal static const SAMPLE_MIP_NEAREST_TAG      : String = "mipnearest";
    nsinternal static const SAMPLE_WRAPPING_CLAMP_TAG   : String = "clamp";
    nsinternal static const SAMPLE_WRAPPING_REPEAT_TAG  : String = "repeat";
	nsinternal static const SAMPLE_WRAPPING_WRAP_TAG   : String = "wrap";
    nsinternal static const TEXTURE_TAG                 : String = "texture";
    nsinternal static const TRUE_TAG                    : String = "true";
    nsinternal static const USED_BY_TAG                 : String = "usedBy";
    nsinternal static const VERSION_TAG                 : String = "version";
    nsinternal static const VERTEX_TAG                  : String = "vertex";
    nsinternal static const VERTEX_PROGRAM_TAG          : String = "vertex_program";

} // Constants
} // com.adobe.AGALOptimiser.agal
