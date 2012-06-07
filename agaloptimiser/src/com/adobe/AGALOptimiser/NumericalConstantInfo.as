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
 * Contains the numerical constants that must be set before running a program.
 */
public final class NumericalConstantInfo
{
    private var startRegister_ : int;
    private var values_ : Vector.<Number>;
    
    /** @private */
    public function NumericalConstantInfo(
        startRegister : int,
        values : Vector.<Number> )
    {
        startRegister_ = startRegister;
        values_ = values;
    }

    /**
     * The index of the first register containing numerical constants.
     */
    public function get startRegister() : int
    {
        return startRegister_;
    }

    /**
     * The constants themselves. These must be placed in contiguous elements starting with element 0 of the start register.
     */
    public function get values() : Vector.<Number>
    {
        return values_;
    }



} // NumericalConstantInfo
} // com.adobe.AGALOptimiser

