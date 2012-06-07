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

import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

/**
 * Contains the name, semantics and format of a value passed into a source program.
 */
    
public final class GlobalID
{
    private var name_ : String;
    private var semantics_ : Semantics;
    private var format_ : String;

    /**
     * @param name 
     * @param semantics
     * @param format A string representation corresponding to the Stage3D format type 
     */
    
    public function GlobalID(
        name : String,
        semantics : Semantics,
        format : String)
    {
        name_ = name;
        semantics_ = semantics;
        format_ = format;
    }

    /**
    * The name of the global value
    */
    public function get name() : String
    {
        return name_;
    }
    
    /**
    * Any semantics associated with the global value
    */
    public function get semantics() : Semantics
    {
        return semantics_;
    }

    /**
    * The exact values returned from here are yet to be decided.
    */
    public function get format() : String
    {
        return format_;
    }

    /** @private */
    nsinternal function clone() : GlobalID
    {
        return new GlobalID( 
            name,
            semantics,
            format);
    }

    /** @private */
    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += "(";
        s += name;
        s += ", ";
        s += format;

        if (semantics != null)
        {
            s += ", ";
            s += semantics.serialize();
        }

        s += ")";
        return s;
    }

} // GlobalID
} // com.adobe.AGALOptimiser


