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
public final class SampleSourceRegister
{
    private var registerInfo_     : Register       = undefined;
    private var sampleCategory_   : SampleOptions = undefined;

    public function SampleSourceRegister(reg : Register, 
                                         category : SampleOptions)
    {
        Enforcer.checkNull(reg);
        Enforcer.checkNull(category);

        Enforcer.check(reg.category.code == RegisterCategory.SAMPLER, ErrorMessages.REGISTER_SAMPLER_RESTRICTION);

        registerInfo_    = reg;
        sampleCategory_  = category;
    }

    internal function clone(context : CloningContext) : SampleSourceRegister
    {
        return new SampleSourceRegister(context.registerMap[registerInfo_],
                                        sampleCategory_.clone());
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        sink.writeShort(registerInfo_.baseNumber);

        sink.writeShort(0);

        sink.writeUnsignedInt( sampleCategory_.code | registerInfo_.category.code ); 
    }

    nsinternal function isScalar() : Boolean
    {
        return true; // for now, all source registers can act as scalars or vectors
    }

    nsinternal function isVector() : Boolean
    {
        return  true; // for now, all source registers can act as scalars or vectors
    }

    nsinternal function get register() : Register
    {
        return registerInfo_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += registerInfo_.serialize(indentLevel, flags | SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER);
        s += " ";
        s += sampleCategory_.serialize();

        return s;
    }

} // SampleSourceRegister
} // com.adobe.AGALOptimiser.agal
