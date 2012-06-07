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

package com.adobe.AGALOptimiser.type {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public final class TypeSet
{
    private static var float1_  : TypeSet = null;
    private static var float2_  : TypeSet = null;
    private static var float3_  : TypeSet = null;
    private static var float4_  : TypeSet = null;
    private static var floats_  : TypeSet = null;

    private var allowableTypes_ : Vector.<Type> = new Vector.<Type>();

    public function TypeSet(... types)
    {
        for (var i : int = 0 ; i < types.length ; ++i)
        {
            Enforcer.checkNull(types[i]);

            if (types[i] is Type)
                allowableTypes_.push(types[i] as Type);
            else if (types[i] is TypeSet)
            {
                var ts : TypeSet = (types[i] as TypeSet);

                for (var j : int = 0 ; j < ts.allowableTypes_.length ; ++j)
                    allowableTypes_.push(ts.allowableTypes_[j]);
            }
            else
                throw new Error(ErrorMessages.INVALID_TYPE);
        }
    }

    nsinternal function isInSet(t : Type) : Boolean
    {
        for (var i : int = 0 ; i < allowableTypes_.length ; ++i)
            if (allowableTypes_[i].equals(t))
                return true;

        return false;
    }

    nsinternal static function float() : TypeSet
    {
        if (float1_ == null)
            float1_ = new TypeSet(new ScalarType(ScalarType.FLOAT_TYPE));

        return float1_;
    }

    nsinternal static function float2() : TypeSet
    {
        if (float2_ == null)
            float2_ = new TypeSet(new VectorType(new ScalarType(ScalarType.FLOAT_TYPE),
                                                 2));

        return float2_;
    }

    nsinternal static function float3() : TypeSet
    {
        if (float3_ == null)
            float3_ = new TypeSet(new VectorType(new ScalarType(ScalarType.FLOAT_TYPE),
                                                 3));

        return float3_;
    }

    nsinternal static function float4() : TypeSet
    {
        if (float4_ == null)
            float4_ = new TypeSet(new VectorType(new ScalarType(ScalarType.FLOAT_TYPE),
                                                 4));

        return float4_;
    }

    nsinternal static function floats() : TypeSet
    {
        if (floats_ == null)
            floats_ = new TypeSet(float(), float2(), float3(), float4());

        return floats_;
    }

} // TypeSet
} // com.adobe.AGALOptimiser.type
