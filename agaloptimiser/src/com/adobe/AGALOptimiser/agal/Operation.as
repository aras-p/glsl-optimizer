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

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

import flash.utils.ByteArray;

use namespace nsinternal;

[ExcludeClass]
public class Operation
{
    private var opcode_      : Opcode              = undefined;
    private var destination_ : DestinationRegister = undefined;
    private var id_          : int                 = -1;
    private var block_       : BasicBlock          = null;
    private var index_       : int                 = -1;

    public function Operation(value : Opcode,
                              destination : DestinationRegister)
    {
        Enforcer.checkNull(value);

        opcode_      = value;
        destination_ = destination;
    }

    nsinternal function get block() : BasicBlock
    {
        return block_;
    }

    nsinternal function set block(value : BasicBlock) : void
    {
        block_ = value;
    }

    internal function clone(context : CloningContext) : Operation
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function get destination() : DestinationRegister
    {
        return destination_;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        opcode_.generateBinaryBlob(sink);
    }

    nsinternal function get id() : int
    {
        return id_;
    }

    nsinternal function set id(value : int) : void
    {
        id_ = value;
    }

    nsinternal function get location() : int
    {
        return index_;
    }

    nsinternal function set location(value : int) : void
    {
        index_ = value;
    }

    nsinternal function get opcode() : Opcode
    {
        return opcode_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function setDefinitionLineForDestination(defLine : DefinitionLine) : void
    {
        Enforcer.check(destination_ != null, ErrorMessages.INTERNAL_ERROR);

        destination_.definitionLine = defLine;
    }

    nsinternal function setLiveRangeForDestination(liveRange : LiveRange) : void
    {
        Enforcer.check(destination_ != null, ErrorMessages.INTERNAL_ERROR);
        Enforcer.check(liveRange.register == destination_.register, ErrorMessages.INTERNAL_ERROR);

        destination_.liveRange = liveRange;
    }

    nsinternal function setLiveRangeForSourceRegisters(liveRange : LiveRange,
                                                          register : Register) : void
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
    }

    nsinternal function simpleOperands(excludeSources : Boolean = false) : Vector.<SourceRegister>
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

} // Operation
} // com.adobe.AGALOptimiser.agal
