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


/**
 * Stores information about the value that needs to be supplied in a register for the program to run.
 */
public class RegisterInfo
{
    private var format_    : String;
    private var name_      : String;
    private var semantics_ : Semantics;

    /** @private */
    public function RegisterInfo(
        name : String,
        semantics : Semantics,
        format : String)
    {
        name_ = name;
        semantics_ = semantics;
        format_ = format;
    }

    /**
    * The exact values returned from here are yet to be decided.
    */
    public function get format() : String
    {
        return format_;
    }

    /**
    * The name of the value
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
} // RegisterInfo
} // com.adobe.AGALOptimiser

