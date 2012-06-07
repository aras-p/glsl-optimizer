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
use namespace nsdebug;

/**
 * Identifies several register elements that are being used to store a single global value.
 */
public class RegisterElementGroup
{
    private var contiguous_ : Boolean;
    private var elements_ : Vector.<RegisterElement>;
    
    /**
     * @param elements The elements making up this group.
     */    
    public function RegisterElementGroup(
        elements : Vector.<RegisterElement> )
    {
        contiguous_ = true;
        elements_ = elements;

        for( var i : int = 0; i < elements_.length - 1; ++i )
        {
            if( ! RegisterElement.contiguous( elements_[ i ], elements_[ i + 1 ] ) )
            {
                contiguous_ = false;
                break;
            }
        }
    }

    /**
     * true iff the elements are contiguous.
     */
    public function get contiguous() : Boolean
    {
        return contiguous_;
    }
    
    /**
     * The elements in the group.
     */
    public function get elements() : Vector.<RegisterElement>
    {
        return elements_;
    }

    /** @private */
    nsdebug function serialize(
        indentLevel : int = 0,
        flags : int = 0) : String
    {
        var indentString  : String   = SerializationUtils.indentString(indentLevel);
        var result     : String   = indentString;

        result += elements.length;
        if( ! contiguous )
            result += " not";
        result += " contiguous elements"
        result += " : ";
        
        for each( var re : RegisterElement in elements )
        {
            result += re.serialize();
            result += " ";
        }
        
        return result;
    }

} // RegisterElementGroup
} // com.adobe.AGALOptimiser



