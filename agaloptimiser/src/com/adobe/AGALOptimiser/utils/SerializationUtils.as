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

package com.adobe.AGALOptimiser.utils {

import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public final class SerializationUtils
{
    private static var indentStrings_ : Array = new Array();

    nsinternal static function indentString(amt : int = 0) : String
    {
        if (amt < indentStrings_.length)
            return indentStrings_[amt];
        else
        {
            // prime it if necessary
            if (indentStrings_.length == 0)
                indentStrings_.push(new String(""));

            var biggest : String = indentStrings_[indentStrings_.length - 1];
            var biggestIndex : int = indentStrings_.length;

            for (; biggestIndex <= amt ; ++biggestIndex)
            {
                biggest = biggest.concat(" ");
                indentStrings_.push(biggest);
            }

            return biggest;
        }
    }

} // SerializationUtils
} // com.adobe.AGALOptimiser.utils
