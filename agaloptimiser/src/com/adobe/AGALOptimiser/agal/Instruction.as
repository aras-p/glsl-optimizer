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

[ExcludeClass]
public class Instruction extends Operation
{
    private var operands_ : Vector.<SourceRegister> = new Vector.<SourceRegister>(2, true);

    public function Instruction(opcodeValue : Opcode,
                                destination : DestinationRegister = null,
                                operand0 : SourceRegister = null,
                                operand1 : SourceRegister = null)
    {
        super(opcodeValue, destination);

        Enforcer.checkNull(opcodeValue);

        operands_[0]    = operand0;
        operands_[1]    = operand1;

        Enforcer.check(isValid(), ErrorMessages.AGAL_INSTRUCTION_MALFORMED);
    }

    override internal function clone(context : CloningContext) : Operation
    {
        return new Instruction(opcode.clone(),
                               (destination != null) ? destination.clone(context) : null,
                               (operands_[0] != null) ? operands_[0].clone(context) : null,
                               (operands_[1] != null) ? operands_[1].clone(context) : null);
    }

    static nsinternal function expectedSourceCount(opcode : Opcode) : int
    {
        switch (opcode.code)
        {
        case Opcode.MOVE:
        case Opcode.RCP:
        case Opcode.SQRT:
		case Opcode.FRACT:
        case Opcode.RCPSQRT:
        case Opcode.LOG2:
        case Opcode.EXP2:
        case Opcode.NORMALIZE:
        case Opcode.SIN:
        case Opcode.COS:
        case Opcode.ABS:
        case Opcode.NEG:
        case Opcode.SATURATE:
        case Opcode.KILL:

            return 1;

        default:

            return 2;
        }
    }

    override nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        opcode.generateBinaryBlob(sink);

        if (destination != null)
            destination.generateBinaryBlob(sink);
        else
            sink.writeUnsignedInt(0x00000000);

        if (operands_[0] != null)
            operands_[0].generateBinaryBlob(sink);
        else
        {
            sink.writeUnsignedInt(0x00000000);
            sink.writeUnsignedInt(0x00000000);
        }

        if (operands_[1] != null)
            operands_[1].generateBinaryBlob(sink);
        else
        {
            sink.writeUnsignedInt(0x00000000);
            sink.writeUnsignedInt(0x00000000);
        }
    }

    static nsinternal function isDestinationRequired(opcode : Opcode) : Boolean
    {
        return (opcode.code != Opcode.KILL);
    }

    nsinternal function isValid() : Boolean
    {
        switch (opcode.code)
        {
        // different kinds of destinations, e.g. can't set to constant register?
        case Opcode.MOVE:
        case Opcode.RCP:
        case Opcode.FRACT:
        case Opcode.SQRT:
        case Opcode.RCPSQRT:
        case Opcode.LOG2:
        case Opcode.EXP2:
        case Opcode.NORMALIZE:
        case Opcode.SIN:
        case Opcode.COS:
        case Opcode.ABS:
        case Opcode.NEG:
        case Opcode.SATURATE:

            return ((destination != null) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] == null));

        case Opcode.ADD:
        case Opcode.SUB:
        case Opcode.MUL:
        case Opcode.DIV:
        case Opcode.MIN:
        case Opcode.MAX:
        case Opcode.POW:

            return ((destination != null) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] != null) && (operands_[1].isVector()));

        case Opcode.CROSS:

            return ((destination != null) && (destination.channelCount() == 3) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] != null) && (operands_[1].isVector()));

        case Opcode.DOT3:
        case Opcode.DOT4:

            return ((destination != null) && (destination.channelCount() == 1) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] != null) && (operands_[1].isVector()));

        case Opcode.MULV3M3:

            return ((destination != null) && (destination.channelCount() <= 3) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[0] != null) && (operands_[1].isMatrix33()));

        case Opcode.MULV4M4:

            return ((destination != null) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] != null) && (operands_[1].isMatrix44()));

        case Opcode.MULV3M4:

            return ((destination != null) && (destination.channelCount() <= 3) && (operands_[0] != null) && (operands_[0].isVector()) && (operands_[1] != null) && (operands_[1].isMatrix44()));

        case Opcode.KILL:

            return ((destination == null) && (operands_[0] != null) && (operands_[0].isScalar()) && (operands_[1] == null));

        case Opcode.LESS_THAN:
        case Opcode.GREATER_THAN_OR_EQUAL:
        case Opcode.EQUALS:
        case Opcode.NOT_EQUALS:

            return ((destination != null) && (operands_[0] != null) && (operands_[1] != null));

        default:

            // sample is not an acceptable opcodes for Instruction

            return false;
        }
    }

    nsinternal function get operand0() : SourceRegister
    {
        return operands_[0];
    }

    nsinternal function set operand0(value : SourceRegister) : void
    {
        operands_[0] = value;
    }

    nsinternal function get operand1() : SourceRegister
    {
        return operands_[1];
    }

    nsinternal function set operand1(value : SourceRegister) : void
    {
        operands_[1] = value;
    }

    nsinternal function get operands() : Vector.<SourceRegister>
    {
        return operands_;
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

        var needComma : Boolean = false;

        if (destination != null)
        {
            instString += destination.serialize(0, flags);
            needComma = true;
        }

        if (operands_[0] != null)
        {
            if (needComma)
                instString += ", ";
            instString += operands_[0].serialize(0, flags);
            needComma = true;
        }

        if (operands_[1] != null)
        {
            if (needComma)
                instString += ", ";
            instString += operands_[1].serialize(0, flags);
        }

        return instString;
    }

    override nsinternal function setLiveRangeForSourceRegisters(liveRange : LiveRange,
                                                                   register : Register) : void
    {
        if ((operands_[0] != null) && (operands_[0].register == register))
            operands_[0].liveRange = liveRange;
        if ((operands_[1] != null) && (operands_[1].register == register))
            operands_[1].liveRange = liveRange;
        if ((operands_[0] != null) && !(operands_[0].isDirect))
        {
            if (operands_[0].indirectRegister == register)
                operands_[0].liveRange = liveRange;
        }
        if ((operands_[1] != null) && !(operands_[1].isDirect))
        {
            if (operands_[1].indirectRegister == register)
                operands_[1].liveRange = liveRange;
        }
    }

    override nsinternal function simpleOperands(excludeSources : Boolean = false) : Vector.<SourceRegister>
    {
        var toReturn : Vector.<SourceRegister> = new Vector.<SourceRegister>();

        if ((operands_[0] != null) && ((!excludeSources) || (!operands_[0].register.category.isSource())))
            toReturn.push(operands_[0]);
        if ((operands_[1] != null) && ((!excludeSources) || (!operands_[1].register.category.isSource())))
            toReturn.push(operands_[1]);

        return toReturn;
    }

} // Instruction
} // com.adobe.AGALOptimiser.agal
