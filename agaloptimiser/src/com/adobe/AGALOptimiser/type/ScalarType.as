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
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

public class ScalarType extends Type
{
    nsinternal static const BOOL_TYPE  : int = 0;
    nsinternal static const FLOAT_TYPE : int = 1;
    nsinternal static const INT_TYPE   : int = 2;
    nsinternal static const SAMPLE_OPTION_TYPE   : int = 3;

    private var scalarType_ : int = undefined;

    public function ScalarType(scalarType : int)
    {
        super();

        Enforcer.checkEnumValue((scalarType == BOOL_TYPE) || (scalarType == FLOAT_TYPE) || (scalarType == INT_TYPE) || (scalarType == SAMPLE_OPTION_TYPE) );

        scalarType_ = scalarType;
    }

    override nsinternal function channelCount() : int
    {
        return 1;
    }

    override nsinternal function clone() : Type
    {
        return new ScalarType(scalarType_);
    }

    override nsinternal function equals(t : Type) : Boolean
    {
        return ((t != null) &&
                (t.isScalar()) &&
                (scalarType_ == ((t as ScalarType).scalarType_)));
    }

    override nsinternal function getStage3DTypeFormatString() : String
    {
        switch (scalarType_)
        {
        case BOOL_TYPE:  return (Constants.STAGE3D_BOOL_TYPE_STRING_BASE + "1");
        case FLOAT_TYPE: return (Constants.STAGE3D_FLOAT_TYPE_STRING_BASE + "1");
        case INT_TYPE:   return (Constants.STAGE3D_INT_TYPE_STRING_BASE + "1");

        default:
            throw new Error(ErrorMessages.INTERNAL_ERROR);
            return null;
        }
    }

    override nsinternal function isScalar() : Boolean
    {
        return true;
    }

    override nsinternal function isScalarBool() : Boolean
    {
        return (scalarType_ == BOOL_TYPE);
    }

    override nsinternal function isScalarFloat() : Boolean
    {
        return (scalarType_ == FLOAT_TYPE);
    }

    override nsinternal function isScalarInt() : Boolean
    {
        return (scalarType_ == INT_TYPE);
    }

    override nsinternal function isScalarSampleOption() : Boolean
    {
        return (scalarType_ == SAMPLE_OPTION_TYPE);
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        switch (scalarType_)
        {
        case BOOL_TYPE   : return (s + Constants.BOOL_TYPE_STRING);
        case FLOAT_TYPE  : return (s + Constants.FLOAT_TYPE_STRING);
        case INT_TYPE    : return (s + Constants.INT_TYPE_STRING);
        case SAMPLE_OPTION_TYPE    : return (s + Constants.SAMPLE_OPTION_TYPE_STRING);

        default:
            throw new Error(ErrorMessages.INTERNAL_ERROR);
            return null;
        }
    }
    
} // ScalarType
} // com.adobe.AGALOptimiser.type
