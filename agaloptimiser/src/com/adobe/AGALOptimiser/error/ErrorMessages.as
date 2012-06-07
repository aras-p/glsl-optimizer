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

package com.adobe.AGALOptimiser.error {

import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public final class ErrorMessages
{
    nsinternal static const AGAL_CONTROL_FLOW_RESTRICTION_V1       : String = "AGAL version one cannot support direct translation of control flow or function calls.  If it cannot be removed by the welder, it cannot be translated."; 
    nsinternal static const AGAL_INSTRUCTION_MALFORMED             : String = "An AGAL instruction does not adhere to required restrictions";
    nsinternal static const AGAL_PROCEDURE_RESTRICTION_V1          : String = "AGAL version one only supports procedures without arguments and cannot support subordinate functions."
    nsinternal static const AGAL_TYPE_RESTRICTION_V1               : String = "AGAL version one must have all values cast to floats prior to translation.  The welder typically handles this transformation.";
    nsinternal static const ALLOCATOR_OUT_OF_REGISTERS             : String = "Register allocator has run out of registers.";
    nsinternal static const ALLOWABLE_SLOT_RANGE                   : String = "Only 1 to 4 slots permitted per register";
    nsinternal static const BAD_FORMAT                             : String = "Bad format";
    nsinternal static const BLOCK_OR_BLOCK_NAME_MISSING            : String = "Block or block name must be specified";
    nsinternal static const BLOCK_TARGET_NOT_RESOLVED              : String = "Block target not resolved";
    nsinternal static const CHANNEL_COUNT_MISMATCH                 : String = "Channel counts on registers mismatched";
    nsinternal static const CONFLICTING_INFORMATION                : String = "Conflicting information supplied";
    nsinternal static const DUPLICATED_DATA                        : String = "Duplicated data supplied";
    nsinternal static const ENUM_VALUE_INVALID                     : String = "Invalid enumerated value";
    nsinternal static const EMPTY_FUNCTION_BODY                    : String = "Body of a function is empty.  Did you intend a function declaration?";
    nsinternal static const EXPECTED_TOKEN_NOT_SEEN                : String = "Expected token not seen";
    nsinternal static const EXTERNAL_VALUES_NEED_INFO              : String = "Externally visible values must have sufficient information supplied";
    nsinternal static const FEATURE_NOT_IMPLEMENTED                : String = "Feature not implemented";
    nsinternal static const FEATURE_NOT_SUPPORTED                  : String = "Feature not supported";
    nsinternal static const FINAL_BLOCK_NOT_BASIC                  : String = "Final block must be a basic block";
    nsinternal static const FIRST_BLOCK_NOT_BASIC                  : String = "First block must be a basic block";
    nsinternal static const FRAGMENT_MODULE_FORM_VIOLATION         : String = "At present, only fragment shaders with evaluateFragment are permitted";
    nsinternal static const GLOBAL_STATE_CHANGES                   : String = "Cannot flatten some control flow when they contain global state changes";
    nsinternal static const INCORRECT_MODULE_TYPE                  : String = "Incorrect module type";
    nsinternal static const INCORRECT_TYPE_OBJECT                  : String = "Incorrect type ofjbect supplied";
    nsinternal static const INLINING_FUNCTION_INCORRECT            : String = "Incorrect function specified for inlining";
    nsinternal static const INSUFFICIENT_DATA                      : String = "Insufficient data";
    nsinternal static const INTERNAL_ERROR                         : String = "Internal error";
    nsinternal static const INVALID_CHANNEL_COUNT                  : String = "Invalid channel count";
    nsinternal static const INVALID_NAME                           : String = "Invalid name";
    nsinternal static const INVALID_OPERAND                        : String = "Invalid operand";
    nsinternal static const INVALID_OPTION                         : String = "Invalid option";
    nsinternal static const INVALID_REGISTER                       : String = "Invalid register";
    nsinternal static const INVALID_TYPE                           : String = "Invalid type";
    nsinternal static const INVALID_VALUE                          : String = "Invalid value";
    nsinternal static const LAST_INSTRUCTION_IN_TEST               : String = "The last instruction in a test block must be a branch or jump";
    nsinternal static const LINK_FAILURE                           : String = "Function declaration not resolved during welder link phase";
    nsinternal static const LOOP_BLOCK_RESTRICTION_VIOLATION       : String = "Loop block restriction violated.  Likely an interation block supplied for non-for loop";
    nsinternal static const MISSING_INFORMATION                    : String = "Necessary information not supplied";
    nsinternal static const OBJECT_NOT_PROPERLY_CONSTRUCTED        : String = "Object not properly constructed";
    nsinternal static const ONE_OUTPUT_AGAL_INST_REQUIRED          : String = "Must have at least one output agal instruction";
    nsinternal static const OUT_OF_RANGE                           : String = "Out of range";
    nsinternal static const OUTPUT_REGISTER_UNIQUENESS             : String = "Only one output register permitted and its number must be 0";
    nsinternal static const PARAMETER_NOT_FOUND                    : String = "Parameter not found";
    nsinternal static const PARAMETER_TYPE_RESTRICTION_VIOLATED    : String = "Parameters may be scalars, vectors, matrices, and arrays";
    nsinternal static const POSITION_PROGRAM_FAILS_STRUCTURE_CHECK : String = "Vertex position program fails structure check. Likely problems: no output clip coordinate, multiple output clip coordinates, or interpolateds which are inconsistent between fragment and vertex stages."
    nsinternal static const REGISTER_CATEGORY_UNDETERMINED         : String = "Cannot determine register category for this type/qualifier/program combination";
    nsinternal static const REGISTER_MISSING                       : String = "An expected register was not found in the current register set";
    nsinternal static const REGISTER_RESTRICTION_VIOLATION         : String = "Register constructed or set with unacceptable setting combination"; 
    nsinternal static const REGISTER_SAMPLER_RESTRICTION           : String = "Sampler register required";
    nsinternal static const REPETITION_COUNT_UNDEFINED             : String = "Repetition count undefiend for repeat-until-converged transformations";
    nsinternal static const REPETITION_COUNT_UNSPECIFIED           : String = "Pattern must be specified and repetition count must be > 0 or repeat-until-converged";
    nsinternal static const SEMANTIC_INDEX_RESTRICTION             : String = "Index must be -1 for no index or 0 or more for indicated index";
    nsinternal static const SIMPLE_VALUE_CATEGORIES                : String = "SimpleValue objects can only have registers of category varying, program constant, or numerical constant";
    nsinternal static const SORT_ON_VECTOR_FLOAT_ONLY              : String = "Currently sorts on vector/float registers only";
    nsinternal static const SPECIFIED_VALUE_MISSING                : String = "Value supplied is not present";
    nsinternal static const SQUARE_MATRICES_ONLY                   : String = "Only 2x2, 3x3, & 4x4  matrices supported at present";
    nsinternal static const SWIZZLE_LENGTH_RESTRICTION             : String = "Only 2/3/4 channels can be specified as swizzle operand";
    nsinternal static const SWIZZLE_RANGE_VIOLATION                : String = "Swizzles must be in range 0..3";
    nsinternal static const SWIZZLE_STRING_TYPE_MISMATCH           : String = "Swizzle string has more or less unique channels then type demands";
    nsinternal static const TEMPLATE_VALUE_HAS_NO_BINDING          : String = "A template value has no binding";
    nsinternal static const TRANSLATOR_CONTROL_FLOW                : String = "This version does not support explicit translation of control flow elements.  A control flow element failed to expand during the welding phase.";
    nsinternal static const TEST_BLOCK_WRONG_BLOCK_TYPE            : String = "A test block must be a basic block or a selection block";
    nsinternal static const TOKEN_STREAM_CORRUPT                   : String = "Token stream corrupted, overlowed, or underflowed";
    nsinternal static const TOO_MANY_OPERANDS                      : String = "Too many operands supplied";
    nsinternal static const TRANSFORMATION_SCOPE_ERROR             : String = "Transformation scope improperly set for this transformation";
    nsinternal static const TYPE_MISMATCH                          : String = "Type mismatch";
    nsinternal static const UNAVOIDABLE_DISCARD_DISCOVERED         : String = "There is an unavoidable discard in this filter";
    nsinternal static const UNEXPECTED_DATA_LENGTH                 : String = "Amount of data is not expected size or divisible by required amount";
    nsinternal static const UNEXPECTED_BASE_CLASS_CALL             : String = "Unexpected call to base class function";
    nsinternal static const UNEXPECTED_MUTABILITY_QUALIFIER        : String = "Unexpected mutability qualifier";
    nsinternal static const UNEXPECTED_NULL_ARGUMENT               : String = "Unexpected null argument";
    nsinternal static const UNEXPECTED_NUMBER_OF_INITIALIZERS      : String = "Incorrect number of initializer values supplied";
    nsinternal static const UNEXPECTED_QUALIFIER                   : String = "Unexpected qualifier";
    nsinternal static const UNEXPECTED_SEMANTIC_INFORMATION        : String = "Unexepcted semantics information";
    nsinternal static const UNKNOWN_IDENTIFIER                     : String = "Unknown identifier";
    nsinternal static const UNKNOWN_INSTRUCTION                    : String = "Unknown instruction or instruction form in translation step";
    nsinternal static const UNKNOWN_OPCODE_OR_MNEMONIC             : String = "Unknown opcode / opcode mnemonic";
    nsinternal static const UNKNOWN_SEMANTICS                      : String = "Indicated semantics unknown in current program constant register set";
    nsinternal static const UNSUPPORTED_OPCODE_FOR_VERSION_1       : String = "Indicated opcode is reserved for future use, but is currently unsupported";
    nsinternal static const UNSUPPORTED_SAMPLING_FLAGS             : String = "Invalid texture sampling flags";
    nsinternal static const UNSUPPORTED_TYPE                       : String = "Unsupported type";
    nsinternal static const VALUE_USED_PRIOR_TO_DEFINITION         : String = "Value used prior to definition";
    nsinternal static const VERSION_NUMBER_UNDEFINED               : String = "Version number undefined on unversioned identifiers";
    nsinternal static const VERSION_NUMBER_UNSPECIFIED             : String = "Version number unspecified on identifier not declared unversioned";
    nsinternal static const VERTEX_ASSIGNMENT_MISSING              : String = "Vertex regsiter assignment missing";
    nsinternal static const VERTEX_MODULE_FORM_VIOLATION           : String = "At present, only vertex modules with evaluateVertex are supported";
    nsinternal static const WRITE_MASK_FORMAT_ERROR                : String = "Write masks are 1 to 4 channels in increasing order";

} // ErrorMessages
} // com.adobe.AGALOptimiser.error
