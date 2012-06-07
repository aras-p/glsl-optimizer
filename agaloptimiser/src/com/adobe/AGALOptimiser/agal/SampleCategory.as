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
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public final class SampleCategory
{
    private       static const FIRST_SAMPLE_CATEGORY   : int = 1; // make sure it's the same as the lowest value below
    nsinternal static const SAMPLE_LINEAR           : int = 1;
    nsinternal static const SAMPLE_NEAREST          : int = 2;
    private       static const LAST_SAMPLE_CATEGORY    : int = 2; // make sure it's the same as the highest value above

    private var category_ : int = 0;

    public function SampleCategory(category : int)
    {
        Enforcer.checkEnumValue((category >= FIRST_SAMPLE_CATEGORY) && (category <= LAST_SAMPLE_CATEGORY));

        category_ = category;
    }

    nsinternal function get code() : int
    {
        return category_;
    }

    nsinternal function clone() : SampleCategory
    {
        return new SampleCategory(category_);
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        switch (category_)
        {
        case SAMPLE_LINEAR:  
            s += Constants.LINEAR_TAG; 
            break;

        case SAMPLE_NEAREST: 
            s += Constants.NEAREST_TAG; 
            break;

        default: 
            throw new Error(ErrorMessages.INTERNAL_ERROR);
        }

        return s;
    }

} // SampleCategory
} // com.adobe.AGALOptimiser.asm
