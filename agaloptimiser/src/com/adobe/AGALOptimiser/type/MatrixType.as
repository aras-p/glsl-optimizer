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
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

public class MatrixType extends Type
{
    use namespace nsinternal;

    private var colDim_ : int = undefined;
    private var rowDim_ : int = undefined;

    public function MatrixType(colDimension : int,
                               rowDimension : int)
    {
        super();

        if ((colDimension != rowDimension) || (colDimension < 2) || (colDimension > 4))
            throw new Error(ErrorMessages.SQUARE_MATRICES_ONLY);

        colDim_ = colDimension;
        rowDim_ = rowDimension;
    }

    override nsinternal function channelCount() : int
    {
        return colDim_ * rowDim_;
    }

    override nsinternal function clone() : Type
    {
        return new MatrixType(colDim_, rowDim_);
    }

    nsinternal function get colDimension() : int
    {
        return colDim_;
    }

    override nsinternal function equals(t : Type) : Boolean
    {
        return ((t != null) &&
                (t is MatrixType) &&
                (colDim_ == (t as MatrixType).colDimension) &&
                (rowDim_ == (t as MatrixType).rowDimension));
    }

    override nsinternal function getStage3DTypeFormatString() : String
    {
        return (Constants.STAGE3D_FLOAT_TYPE_STRING_BASE + colDim_ + "x" + rowDim_);
    }

    override nsinternal function isMatrix() : Boolean
    {
        return true;
    }

    nsinternal function get rowDimension() : int
    {
        return rowDim_;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.FLOAT_TYPE_STRING;
        s += "-";
        s += colDim_;
        s += "-";
        s += rowDim_;

        return s;
    }

} // MatrixType
} // com.adobe.AGALOptimiser.type
