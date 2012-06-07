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
public class Procedure
{
    private var name_                 : String              = undefined;
    private var operations_           : Vector.<Operation>  = new Vector.<Operation>();
    private var returnValueRegister_  : Register            = null;
    private var outputArguments_      : Vector.<Register>   = new Vector.<Register>();
    private var inputArguments_       : Vector.<Register>   = new Vector.<Register>();
    private var blocks_               : Vector.<BasicBlock> = new Vector.<BasicBlock>();

    public function Procedure(name : String)
    {
        Enforcer.check(isNameAcceptable(name), ErrorMessages.INVALID_NAME);

        name_ = name;
    }

    nsinternal function get basicBlocks() : Vector.<BasicBlock>
    {
        return blocks_;
    }

    internal function clone(context : CloningContext) : Procedure
    {
        var clonedProcedure : Procedure = new Procedure(name_);

        for each (var inputArg : Register in inputArguments_)
            clonedProcedure.inputArguments.push(context.registerMap[inputArg]);

        for each (var outputArg : Register in outputArguments_)
            clonedProcedure.outputArguments.push(context.registerMap[outputArg]);

        for each (var op : Operation in operations_)
            clonedProcedure.operations.push(op.clone(context));

        if (returnValueRegister_)
            clonedProcedure.returnValueRegister = context.registerMap[returnValueRegister_];

        // need to add globals and constants

        context.procedures[this] = clonedProcedure;

        return clonedProcedure;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        for each (var operation : Operation in operations_)
            operation.generateBinaryBlob(sink);
    }

    nsinternal function get inputArguments() : Vector.<Register>
    {
        return inputArguments_;
    }

    private function isNameAcceptable(name : String) : Boolean
    {
        return true;  // need to add the rules
    }

    nsinternal function get name() : String
    {
        return name_;
    }

    nsinternal function get operations() : Vector.<Operation>
    {
        return operations_;
    }

    nsinternal function get outputArguments() : Vector.<Register>
    {
        return outputArguments_;
    }

    nsinternal function populateLocationRangeData() : void
    {
        // all DefinitionLine & LiveRange data piggybacks off the 
        // instruction index (for now) so setting instruction indices is sufficient.

        for (var i : int = 0 ; i < operations_.length ; ++i)
            operations_[i].location = i;
    }

    nsinternal function get returnValueRegister() : Register
    {
        return returnValueRegister_;
    }

    nsinternal function set returnValueRegister(r : Register) : void
    {
        returnValueRegister_ = r;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        if (!SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
        {
            s += Constants.PROCEDURE_TAG;
            s += " ";
            s += name_;
            s += "\n";
            
            indentLevel += 4;
            
            if (inputArguments_.length > 0)
                throw new Error(ErrorMessages.AGAL_PROCEDURE_RESTRICTION_V1);
            
            if ((returnValueRegister_ != null) || (outputArguments_.length > 0))
            {
                s += SerializationUtils.indentString(indentLevel);
                s += Constants.OUTPUT_TAG;
                s += "\n";
                
                indentLevel += 4;
                
                if (returnValueRegister_ != null)
                {
                    s += returnValueRegister_.serialize(indentLevel);
                    s += "\n";
                }
                
                for (var iOutputArgument : int = 0 ; iOutputArgument < outputArguments_.length ; ++iOutputArgument)
                {
                    s += outputArguments_[iOutputArgument].serialize(indentLevel);
                    s += "\n";
                }
                
                indentLevel -=4;
                
                s += SerializationUtils.indentString(indentLevel);
                s += Constants.END_OUTPUTS_TAG;
                s += "\n";
            }
            
            s += SerializationUtils.indentString(indentLevel);
            s += Constants.OPERATIONS_TAG;
            s += "\n";
            
            indentLevel += 4;
        }

        for each (var op : Operation in operations_)
        {
            s += op.serialize(indentLevel, flags);
            s += "\n";
        }

        if (!SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
        {
            indentLevel -= 4;
            
            s += SerializationUtils.indentString(indentLevel);
            s += Constants.END_OPERATIONS_TAG;
            s += "\n";
            
            indentLevel -= 4;
            
            s += SerializationUtils.indentString(indentLevel);
            s += Constants.END_PROCEDURE_TAG;
            s += "\n";
        }

        return s;
    }

} // Procedure
} // com.adobe.AGALOptimiser.agal
