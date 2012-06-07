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
public final class Opcode
{
    internal      static const LOWEST_OPCODE         : int = 0x00;

    nsinternal static const MOVE                  : int = 0x00;
    nsinternal static const ADD                   : int = 0x01;
    nsinternal static const SUB                   : int = 0x02;
    nsinternal static const MUL                   : int = 0x03;
    nsinternal static const DIV                   : int = 0x04;
    nsinternal static const RCP                   : int = 0x05;
    nsinternal static const MIN                   : int = 0x06;
    nsinternal static const MAX                   : int = 0x07;
    nsinternal static const FRACT                 : int = 0x08;
    nsinternal static const SQRT                  : int = 0x09;
    nsinternal static const RCPSQRT               : int = 0x0a;
    nsinternal static const POW                   : int = 0x0b;
    nsinternal static const LOG2                  : int = 0x0c;
    nsinternal static const EXP2                  : int = 0x0d;
    nsinternal static const NORMALIZE             : int = 0x0e;
    nsinternal static const SIN                   : int = 0x0f;
    nsinternal static const COS                   : int = 0x10;
    nsinternal static const CROSS                 : int = 0x11;
    nsinternal static const DOT3                  : int = 0x12;
    nsinternal static const DOT4                  : int = 0x13;
    nsinternal static const ABS                   : int = 0x14;
    nsinternal static const NEG                   : int = 0x15;
    nsinternal static const SATURATE              : int = 0x16;
    nsinternal static const MULV3M3               : int = 0x17;
    nsinternal static const MULV4M4               : int = 0x18;
    nsinternal static const MULV3M4               : int = 0x19;

    // 0x1a -> 0x26 are reserved for future flow control opcodes
    private       static const LOWEST_CONTROL_FLOW   : int = 0x1a;
    private       static const HIGHEST_CONTROL_FLOW  : int = 0x26;

    nsinternal static const KILL                  : int = 0x27;
    nsinternal static const SAMPLE                : int = 0x28;
    nsinternal static const GREATER_THAN_OR_EQUAL : int = 0x29;
    nsinternal static const LESS_THAN             : int = 0x2a;

    nsinternal static const EQUALS                : int = 0x2c;
    nsinternal static const NOT_EQUALS            : int = 0x2d;

    internal      static const HIGHEST_OPCODE        : int = 0x2d;

    internal static const codeMap_ : Array = new Array(
        MOVE,                  "mov",
        ADD,                   "add",
        SUB,                   "sub",
        MUL,                   "mul",
        DIV,                   "div",
        RCP,                   "rcp",
        MIN,                   "min",
        MAX,                   "max",
        FRACT,                 "frc",
        SQRT,                  "sqt",
        RCPSQRT,               "rsq",
        POW,                   "pow",
        LOG2,                  "log",
        EXP2,                  "exp",
        NORMALIZE,             "nrm",
        SIN,                   "sin",
        COS,                   "cos",
        CROSS,                 "crs",
        DOT3,                  "dp3",
        DOT4,                  "dp4",
        ABS,                   "abs",
        NEG,                   "neg",
        SATURATE,              "sat",
        MULV3M3,               "m33",
        MULV4M4,               "m44",
        MULV3M4,               "m34",
        KILL,                  "kil",
        SAMPLE,                "tex",
        LESS_THAN,             "slt",
        GREATER_THAN_OR_EQUAL, "sge",
        EQUALS,                "seq",
        NOT_EQUALS,            "sne"
        );

    private var opcode_ : int = undefined;

    public function Opcode(code : int)
    {
        Enforcer.check((code >= LOWEST_OPCODE) && (code <= HIGHEST_OPCODE), ErrorMessages.UNKNOWN_OPCODE_OR_MNEMONIC +  code);
        Enforcer.check(!((code >= LOWEST_CONTROL_FLOW) && (code <= HIGHEST_CONTROL_FLOW)), ErrorMessages.UNSUPPORTED_OPCODE_FOR_VERSION_1);

        opcode_ = code;
    }

    internal function clone() : Opcode
    {
        return new Opcode(opcode_);
    }

    nsinternal function get code() : int
    {
        return opcode_;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        Enforcer.checkNull(sink);
        sink.writeUnsignedInt(opcode_);
    }
    
    nsinternal function isConditional() : Boolean
    {
        return false;
    }
    
    nsinternal function isControlFlow() : Boolean 
    {
        return false;
    }

    nsinternal static function isKnownMnemonic(mnemonic : String) : Boolean
    {
        for (var i : int = 0 ; i < codeMap_.length ; i += 2)
            if (codeMap_[i + 1] == mnemonic)
                return true;

        return false;
    }
    
    nsinternal function isSubroutineRelated() : Boolean
    {
        return false;
    }

    nsinternal static function mnemonicToOpcode(mnemonic : String) : Opcode
    {
        // the map is opcode then mnemonic

        for (var i : int = 0 ; i < codeMap_.length ; i += 2)
            if (codeMap_[i + 1] == mnemonic)
                return new Opcode(codeMap_[i]);

        throw new Error(ErrorMessages.UNKNOWN_OPCODE_OR_MNEMONIC + " " + mnemonic);

        return null;
    }

    nsinternal static function opcodeToMnemonic(op : Opcode) : String
    {
        // probably want to do this better

        // the map is opcode then mnemonic
        for (var i : int = 0 ; i < codeMap_.length ; i += 2)
            if (codeMap_[i] == op.code)
                return codeMap_[i + 1];

        throw new Error(ErrorMessages.UNKNOWN_OPCODE_OR_MNEMONIC + " " + op);

        return null;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        return Opcode.opcodeToMnemonic(this);
    }

} // Opcode
} // com.adobe.AGALOptimiser.agal
