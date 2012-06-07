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
public final class ProgramCategory
{
    private       static const FIRST_CATEGORY : int = 1;
    nsinternal static const FRAGMENT       : int = 1;
    nsinternal static const VERTEX         : int = 2;
    private       static const LAST_CATEGORY  : int = 2;

    private var category_ : int = undefined;

    public function ProgramCategory(cat : int)
    {
        Enforcer.checkEnumValue((cat >= FIRST_CATEGORY) && (cat <= LAST_CATEGORY));

        category_ = cat;
    }

    nsinternal function get code() : int
    {
        return category_;
    }

    internal function clone() : ProgramCategory
    {
        return new ProgramCategory(category_);
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        switch (category_)
        {
        case FRAGMENT:
            sink.writeByte(0x01);
            break;
        case VERTEX:
            sink.writeByte(0x00);
            break;
        default:
            throw new Error(ErrorMessages.ENUM_VALUE_INVALID);
        }
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        switch (category_)
        {
        case FRAGMENT: return Constants.FRAGMENT_PROGRAM_TAG;
        case VERTEX:   return Constants.VERTEX_PROGRAM_TAG;

        default:
            throw new Error(ErrorMessages.ENUM_VALUE_INVALID);
        }

        return null;
    }

} // ProgramCategory
} // com.adobe.AGALOptimiser.agal
