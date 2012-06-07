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

use namespace nsinternal;

[ExcludeClass]
public final class TypeUtils
{
    static nsinternal function dimCount(t : Type) : int
    {
        if (t is ScalarType)
            return 1;
        else if (t is VectorType)
            return ((t as VectorType).dimension);
        else if (t is MatrixType)
            return ((t as MatrixType).rowDimension); // Assuming they are square matrices
        else if (t is ImageType)
            return ((t as ImageType).channelCount());
        else if (t is ArrayType)
            return (dimCount((t as ArrayType).elementType));
        else
            return 0;
    }

    static nsinternal function stage3dTypeStringToType(typeString : String) : Type
    {
        var scalarType : ScalarType = null;
        var dimension  : int        = 0;

        if (typeString.indexOf(Constants.STAGE3D_INT_TYPE_STRING_BASE) != -1)
        {
            scalarType = new ScalarType(ScalarType.INT_TYPE);
            dimension = int(typeString.slice(3));
        }
        else if (typeString.indexOf(Constants.STAGE3D_BOOL_TYPE_STRING_BASE) != -1)
        {
            scalarType = new ScalarType(ScalarType.BOOL_TYPE);
            dimension = int(typeString.slice(4));
        }
        else if (typeString.indexOf(Constants.STAGE3D_FLOAT_TYPE_STRING_BASE) != -1)
        {
            var remainder : String = typeString.slice(5);

            if (remainder.indexOf("X") != -1)
            {
                // it's a matrix.  Assume square.
                dimension = int(remainder.substring(0,1));

                return new MatrixType(dimension, dimension);
            }
            else
            {
                scalarType = new ScalarType(ScalarType.FLOAT_TYPE);
                dimension = int(remainder);
            }
        }
        else
            return null;

        if (dimension == 0) // the dimension slice above returned null, which became a 0, not a 1
            return scalarType;
        else
            return new VectorType(scalarType, dimension);
    }

} // TypeUtils
} // com.adobe.AGALOptimiser.type
