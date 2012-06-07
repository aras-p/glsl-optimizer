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
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public class ConstantBooleanValue extends ConstantValue
{
    private var values_ : Vector.<Boolean> = new Vector.<Boolean>(4, true);

    public function ConstantBooleanValue(id : int,
                                         type : Type,
                                         values : Vector.<Boolean> = null)
    {
        super(id, type);

        var i            : int;
        var channelCount : int = type.channelCount();

        Enforcer.check((values == null) || (values.length == channelCount), 
                       ErrorMessages.UNEXPECTED_NUMBER_OF_INITIALIZERS);

        values_ = new Vector.<Boolean>(channelCount);

        if (values != null)
            for (i = 0 ; i < channelCount ; ++i)
                values_[i] = values[i];
        else
            for (i = 0 ; i < channelCount ; ++i)
                values_[i] = false;
    }

    override nsinternal function clone(context : CloningContext) : Value
    {
        if (context.constants[this] != null)
            return context.constants[this];
        else
        {
            var cloned : ConstantBooleanValue = new ConstantBooleanValue(id, type.clone(), values_);
            
            context.constants[this] = cloned;

            return cloned;
        }
    }

    nsinternal function getChannel(channelIndex : int) : Boolean
    {
        Enforcer.checkRange(channelIndex < type.channelCount());

        return values_[channelIndex];
    }

    override nsinternal function getChannelAsFloat(index : int) : Number
    {
        Enforcer.checkRange(index < type.channelCount());

        return (values_[index] ? 1.0 : 0.0);
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.CONST_TAG;
        s += "(";

        if (type.isScalar())
            s += (values_[0] ? Constants.TRUE_TAG : Constants.FALSE_TAG);
        else
        {
            for (var i : int = 0 ; i < type.channelCount() ; ++i)
            {
                if (i > 0)
                    s += ", ";
                s += (values_[i] ? Constants.TRUE_TAG : Constants.FALSE_TAG);
            }
        }

        s += ")";

        return s;
    }

    nsinternal function setChannel(channelIndex : int,
                                      value : Boolean) : void
    {
        Enforcer.checkRange(channelIndex < type.channelCount());

        values_[channelIndex] = value;
    }

} // ConstantBooleanValue
} // com.adobe.AGALOptimiser.agal