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

package com.adobe.AGALOptimiser {

import com.adobe.AGALOptimiser.error.*;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

/**
 * Identifies a single element in a register. Each register contains four elements.
 */
public class RegisterElement
{
    private var registerIndex_    : int;
    private var elementIndex_      : int;
    
    /**
     * Combine a register index and an element index to refer to a single element. Using the default values leads to an invalid RegisterElement.
     */    
    public function RegisterElement(
        registerIndex : int = -1,
        elementIndex : int = -1)
    {
        registerIndex_ = registerIndex;
        elementIndex_ = elementIndex;

        if( elementIndex_ == -1 && registerIndex_ != -1 )
            elementIndex_ = 0;
    }

    /**
     * true iff the RegisterElement refers to an actual element in a register. 
     * The default values for registerIndex and elementIndex produce an invalid RegisterElement.
     */
    public function isValid() : Boolean
    {
        return registerIndex_ != -1 && elementIndex_ != -1;
    }

    /**
     * The register index.
     */
    public function get registerIndex() : int
    {
        Enforcer.check( isValid(), ErrorMessages.INVALID_REGISTER );

        return registerIndex_;
    }

    /**
     * The element index in the range 0 - 3.
     */
    public function get elementIndex() : int
    {
        Enforcer.check( isValid(), ErrorMessages.INVALID_REGISTER );

        return elementIndex_;
    }

    /** @private */
    nsinternal function add( i : int ) : RegisterElement
    {
        var newRegisterIndex : int = registerIndex_;
        var newElementIndex : int = elementIndex_ + i;

        newRegisterIndex += newElementIndex / 4;
        newElementIndex = newElementIndex % 4;
        
        return new RegisterElement(
            newRegisterIndex,
            newElementIndex );
    }

    /** @private */
    nsdebug function serialize(
        indentLevel : int = 0,
        flags : int = 0) : String
    {
        var indentString  : String   = SerializationUtils.indentString(indentLevel);
        var result     : String   = indentString;

        if( isValid() )
        {
            result += registerIndex_ + "[" + elementIndex_ + "]";
        }
        else
        {
            result += "invalid register";
        }
        
        return result;
    }

    /** @private */
    nsinternal static function contiguous( re0 : RegisterElement, re1 : RegisterElement ) : Boolean
    {        
        return ( ( re0.registerIndex == re1.registerIndex ) && ( re0.elementIndex + 1 == re1.elementIndex ) ) ||
            ( ( re0.registerIndex + 1 == re1.registerIndex ) && ( re0.elementIndex == 3 ) && ( re1.elementIndex == 0 ) );
    }
    
    
} // RegisterElement
} // com.adobe.AGALOptimiser


