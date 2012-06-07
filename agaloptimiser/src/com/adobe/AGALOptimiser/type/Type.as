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

import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public class Type
{
    public function Type()
    {
    }

    nsinternal function channelCount() : int
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return 0;
    }

    nsinternal function clone() : Type
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function equals(t : Type) : Boolean
    {
        return false;
    }

    nsinternal function getStage3DTypeFormatString() : String
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function isArray() : Boolean
    {
        return false;
    }
    
    nsinternal function isImage() : Boolean
    {
        return false;
    }

    nsinternal function isMatrix() : Boolean
    {
        return false;
    }

    nsinternal function isScalar() : Boolean
    {
        return false;
    }

    nsinternal function isScalarBool() : Boolean
    {
        return false;
    }

    nsinternal function isScalarFloat() : Boolean
    {
        return false;
    }

    nsinternal function isScalarSampleOption() : Boolean
    {
        return false;
    }

    nsinternal function isScalarInt() : Boolean
    {
        return false;
    }

    nsinternal function isString() : Boolean
    {
        return false;
    }
    
    nsinternal function toGlslString() : String
    {
        return null;
    }

    nsinternal function isVector() : Boolean
    {
        return false;
    }

    nsinternal function isVectorBool() : Boolean
    {
        return false;
    }

    nsinternal function isVectorFloat() : Boolean
    {
        return false;
    }

    nsinternal function isVectorInt() : Boolean
    {
        return false;
    }

    nsinternal function isVoid() : Boolean
    {
        return false;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

} // Type
} // com.adobe.AGALOptimiser.type
