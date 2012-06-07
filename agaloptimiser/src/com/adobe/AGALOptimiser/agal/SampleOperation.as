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
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

import flash.utils.ByteArray;

use namespace nsinternal;

// This class used to be called Sample, but that was conflicting with the Sample opcodes used in agal and asm, hence it was renamed.
[ExcludeClass]
public class SampleOperation extends Operation
{
    private var operand0_    : SourceRegister       = undefined;
    private var operand1_    : SampleSourceRegister = undefined;

    public function SampleOperation(destination : DestinationRegister,
                                    operand0 : SourceRegister,
                                    operand1 : SampleSourceRegister)
    {
        super(new Opcode(Opcode.SAMPLE), destination);

        Enforcer.checkNull(destination);
        Enforcer.checkNull(operand0);
        Enforcer.checkNull(operand1);

        operand0_    = operand0;
        operand1_    = operand1;

        Enforcer.check(isValid(), ErrorMessages.AGAL_INSTRUCTION_MALFORMED);
    }

    override internal function clone(context : CloningContext) : Operation
    {
        return new SampleOperation((destination != null) ? destination.clone(context) : null,
                                   (operand0_ != null) ? operand0_.clone(context) : null,
                                   (operand1_ != null) ? operand1_.clone(context) : null);
    }

    override nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        opcode.generateBinaryBlob(sink);

        destination.generateBinaryBlob(sink);
        operand0_.generateBinaryBlob(sink);
        operand1_.generateBinaryBlob(sink);
    }

    nsinternal function isValid() : Boolean
    {
        if (opcode.code == Opcode.SAMPLE)
            return ((destination != null) && (operand0_ != null) && (operand0_.isVector()) && (operand1_ != null) && (operand1_.isVector()));
        else
            return false;
    }

    nsinternal function get operand0() : SourceRegister
    {
        return operand0_;
    }

    nsinternal function get operand1() : SampleSourceRegister
    {
        return operand1_;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var instString : String = "";

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_BLOCK_NUMBERS))
        {
            instString += "B";
            instString += (block != null) ? block.blockNumber : "?";
            instString += "-";
            instString += (block != null) ? block.outlets.length : "?";
        }

        instString += SerializationUtils.indentString(indentLevel);

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_OPERATION_SERIAL_NUMBERS))
        {
            instString += id;
            instString += ": ";
        }

        instString += opcode.serialize();
        instString += " ";

        instString += destination.serialize(0, flags);
        instString += ", ";

        instString += operand0_.serialize(0, flags);
        instString += ", ";

        instString += operand1_.serialize(0, flags);

        return instString;
    }

    override nsinternal function setLiveRangeForSourceRegisters(liveRange : LiveRange,
                                                                   register : Register) : void
    {
        if ((operand0_ != null) && (operand0_.register == register))
            operand0_.liveRange = liveRange;

        // operand1_ is a texture sampler, so doesn't get a live range
    }

    override nsinternal function simpleOperands(excludeSources : Boolean = false) : Vector.<SourceRegister>
    {
        var toReturn : Vector.<SourceRegister> = new Vector.<SourceRegister>();

        if ((operand0_ != null) && ((!excludeSources) || (!operand0_.register.category.isSource())))
            toReturn.push(operand0_);

        return toReturn;
    }

} // Sample
} // com.adobe.AGALOptimiser.agal
