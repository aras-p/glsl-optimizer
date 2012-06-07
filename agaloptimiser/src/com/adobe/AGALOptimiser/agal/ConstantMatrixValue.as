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
import com.adobe.AGALOptimiser.type.MatrixType;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

import flash.geom.Matrix;

use namespace nsinternal;

[ExcludeClass]
public class ConstantMatrixValue extends ConstantValue
{
    private var values_     : Vector.<Number> = null;
    private var matrixType_ : MatrixType      = null;
    
    public function ConstantMatrixValue(id : int,
                                        type : MatrixType,
                                        values : Vector.<Number> = null)
    {
        super(id, type);
        Enforcer.checkNull(type);

        Enforcer.check((values == null) || (values.length == type.channelCount()), 
                       ErrorMessages.UNEXPECTED_NUMBER_OF_INITIALIZERS);

        matrixType_ = type;

        values_ = new Vector.<Number>(type.channelCount(), true);

        for (var i : int = 0 ; i < values_.length ; ++i)
            values_[i] = (values != null) ? values[i] : 0.0;
    }

    override nsinternal function clone(context : CloningContext) : Value
    {
        if (context.constants[this] != null)
            return context.constants[this];
        else
        {
            var cloned : ConstantMatrixValue = new ConstantMatrixValue(id,  matrixType_.clone() as MatrixType, values_);

            context.constants[this] = cloned;

            return cloned;
        }
    }

    nsinternal function getChannel(channelIndex : int) : Number
    {
        Enforcer.checkRange(channelIndex < type.channelCount());

        return values_[channelIndex];
    }

    override nsinternal function getChannelAsFloat(index : int) : Number
    {
        Enforcer.checkRange(index < type.channelCount());

        return values_[index];
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.CONST_TAG;
        s += "(";

        if (type.isScalar())
            s += values_[0];
        else
        {
            for (var i : int = 0 ; i < type.channelCount() ; ++i)
            {
                if (i > 0)
                    s += ", ";
                s += values_[i];
            }
        }

        s += ")";

        return s;
    }

    nsinternal function setChannel(channelIndex : int,
                                      value : Number) : void
    {
        Enforcer.checkRange(channelIndex < type.channelCount());

        values_[channelIndex] = value;
    }
} // ConstantMatrixValue
} // com.adobe.AGALOptimiser.agal