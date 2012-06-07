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

package com.adobe.AGALOptimiser.translator.transformations {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.TypeSet;

use namespace nsinternal;

[ExcludeClass]
public final class DestinationRegisterTemplate
{
    private var id_                  : int          = undefined;
    private var allowableTypes_      : TypeSet      = undefined;
    private var typeId_              : int          = undefined;
    private var maskAndSwizzleChain_ : Vector.<int> = undefined;
    
    public function DestinationRegisterTemplate(id : int,
                                                allowableTypes : TypeSet = null,
                                                typeId : int = -1,
                                                maskAndSwizzleChain : Vector.<int> = null)
    {
        Enforcer.check(id >= 0, ErrorMessages.OUT_OF_RANGE);
        
        id_                  = id;
        allowableTypes_      = allowableTypes;
        typeId_              = typeId;
        maskAndSwizzleChain_ = maskAndSwizzleChain;
    }

    nsinternal function get allowableTypes() : TypeSet
    {
        return allowableTypes_;
    }

    nsinternal function get id() : int
    {
        return id_;
    }

    nsinternal function get maskAndSwizzleChain() : Vector.<int>
    {
        return maskAndSwizzleChain_;
    }

    nsinternal function get typeId() : int
    {
        return typeId_;
    }
    
} // DestinationRegisterTemplate
} // com.adobe.AGALOptimiser.translator.transformations