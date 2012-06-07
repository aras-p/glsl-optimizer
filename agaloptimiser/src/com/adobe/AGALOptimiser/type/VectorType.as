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
public class VectorType extends Type
{
    private var dimension_  : int        = undefined;
    private var scalarType_ : ScalarType = undefined;

    public function VectorType(scalarType : ScalarType,
                               dimension : int)
    {
        super();

        Enforcer.checkNull(scalarType);
        Enforcer.check(dimension > 1, ErrorMessages.INVALID_CHANNEL_COUNT);

        scalarType_ = scalarType;
        dimension_ = dimension;
    }

    override nsinternal function channelCount() : int
    {
        return dimension_;
    }

    override nsinternal function clone() : Type
    {
        return new VectorType(scalarType_.clone() as ScalarType,
                              dimension_);
    }

    nsinternal function get dimension() : int
    {
        return dimension_;
    }

    override nsinternal function equals(t : Type) : Boolean
    {
        return ((t != null) &&
                (t is VectorType) &&
                (dimension_ == (t as VectorType).dimension_) &&
                (scalarType_.equals((t as VectorType).scalarType_)));
    }

    override nsinternal function getStage3DTypeFormatString() : String
    {
        if (scalarType_.isScalarBool())
            return (Constants.STAGE3D_BOOL_TYPE_STRING_BASE + dimension_);
        else if (scalarType_.isScalarFloat())
            return (Constants.STAGE3D_FLOAT_TYPE_STRING_BASE + dimension_);
        else if (scalarType_.isScalarInt())
            return (Constants.STAGE3D_INT_TYPE_STRING_BASE + dimension_);
        else
        {
            throw new Error(ErrorMessages.INTERNAL_ERROR);
            return null;
        }
    }

    override nsinternal function isVector() : Boolean
    {
        return true;
    }

    override nsinternal function isVectorBool() : Boolean
    {
        return scalarType_.isScalarBool();
    }

    override nsinternal function isVectorFloat() : Boolean
    {
        return scalarType_.isScalarFloat();
    }

    override nsinternal function isVectorInt() : Boolean
    {
        return scalarType_.isScalarInt();
    }

    nsinternal function get scalarType() : ScalarType
    {
        return scalarType_;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        return scalarType_.serialize(0, flags) + "-" + dimension_;
    }
} // VectorType
} // com.adobe.AGALOptimiser.type
