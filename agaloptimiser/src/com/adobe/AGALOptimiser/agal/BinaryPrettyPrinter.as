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
public class BinaryPrettyPrinter
{
    public function BinaryPrettyPrinter()
    {
    }

    private function dec2hex(d : int) : String
    {
        var c : Array = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                         'a', 'b', 'c', 'd', 'e', 'f'];
        if (d > 255)
            d = 255;

        var l : int = d / 16;
        var r : int = d % 16;

        var toReturn : String = c[l] + c[r];

        return toReturn;
    }

    private function extractAndPrintBytes(binaryBlob : ByteArray,
                                          numBytes : int) : String
    {
        var stringRep : String = null;

        for (var i : int = 0 ; i < numBytes ; ++i)
        {
            var b : int = binaryBlob.readByte();
            b &= 0x000000FF;

            if (stringRep == null)
                stringRep = dec2hex(b);
            else
                stringRep += dec2hex(b);
        }

        return stringRep;
    }

    public function prettyPrint(binaryBlob : ByteArray) : String
    {
        var stringRep : String = null;

        // header is 5 bytes.
        // shader type is 2 bytes
        // each instruction is 24 bytes

        Enforcer.check(binaryBlob.length > 7, ErrorMessages.INSUFFICIENT_DATA);
        Enforcer.check(((binaryBlob.length - 7) % 24) == 0, ErrorMessages.UNEXPECTED_DATA_LENGTH);

        try
        {
            var currentPosition : int = binaryBlob.position;
            var numInstructions : int = (binaryBlob.length - 7) / 24;
            var b               : int;

            binaryBlob.position = 0;

            // first print out header
            stringRep = extractAndPrintBytes(binaryBlob, 1);
            stringRep += " ";
            stringRep += extractAndPrintBytes(binaryBlob, 4);
            stringRep += "\n";

            // next the shader type
            stringRep += extractAndPrintBytes(binaryBlob, 1);
            stringRep += " ";
            stringRep += extractAndPrintBytes(binaryBlob, 1);
            stringRep += "\n";

            for (var i : int = 0 ; i < numInstructions ; ++i)
            {
                // for now we only handle non-sample instructions

                stringRep += extractAndPrintBytes(binaryBlob, 4);
                stringRep += " ";
                stringRep += extractAndPrintBytes(binaryBlob, 4);
                stringRep += " ";
                stringRep += extractAndPrintBytes(binaryBlob, 8);
                stringRep += " ";
                stringRep += extractAndPrintBytes(binaryBlob, 8);
                stringRep += "\n";
            }
        }
        finally
        {
            binaryBlob.position = currentPosition;
        }

        return stringRep;
    }

} // BinaryPrettyPrinter
} // com.adobe.AGALOptimiser.agal
