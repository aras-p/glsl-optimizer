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
import com.adobe.AGALOptimiser.type.Type;

use namespace nsinternal;

[ExcludeClass]
public class Value
{
    private var type_ : Type = undefined;
    private var id_   : int  = undefined;
    
    public function Value(id : int,
                          type : Type)
    {
        Enforcer.checkNull(type);

        id_       = id;
        type_     = type;
    }

    nsinternal function clone(context : CloningContext) : Value
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function get id() : int
    {
        return id_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        throw new Error(ErrorMessages.UNEXPECTED_BASE_CLASS_CALL);
        return null;
    }

    nsinternal function get type() : Type
    {
        return type_;
    }

} // Value
} // com.adobe.AGALOptimiser.agal