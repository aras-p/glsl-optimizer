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

import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public class StringType extends Type
{
    public function StringType()
    {
        super();
    }

    override nsinternal function clone() : Type
    {
        return new StringType();
    }

    override nsinternal function isString() : Boolean
    {
        return true;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        return SerializationUtils.indentString(indentLevel) + Constants.STRING_TYPE_STRING;
    }
} // StringType
} // com.adobe.AGALOptimiser.type
