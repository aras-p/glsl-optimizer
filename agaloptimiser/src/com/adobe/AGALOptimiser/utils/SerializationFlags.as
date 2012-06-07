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

public final class SerializationFlags
{
    // bits 12..19 for AGAL hints
    nsinternal static const DISPLAY_AGAL_OPERATION_SERIAL_NUMBERS         : int = 0x00001000;
    nsinternal static const DISPLAY_AGAL_LIVE_RANGE_CROSS_REFERENCES      : int = 0x00002000;
    nsinternal static const DISPLAY_AGAL_DEFINITION_LINE_CROSS_REFERENCES : int = 0x00004000;
    nsinternal static const DISPLAY_AGAL_BLOCK_NUMBERS                    : int = 0x00008000;
    nsinternal static const DISPLAY_AGAL_CHANNELS                         : int = 0x00010000;
    nsinternal static const DISPLAY_AGAL_MINIASM_FORM                     : int = 0x00020000;
    nsinternal static const DISPLAY_AGAL_REGISTER_INFO                    : int = 0x00040000;
    nsinternal static const DISPLAY_AGAL_REGISTER_NUMBER                  : int = 0x00080000;

    // bits 20..31 currently open

    nsinternal static function isSet(flags : int,
                                        flagToTest : int) : Boolean
    {
        return ((flags & flagToTest) != 0);
    }

} // SerializationFlags
} // com.adobe.AGALOptimiser.utils
