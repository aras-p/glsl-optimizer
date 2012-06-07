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

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

/**
 * This class contains the name, semantics and format of a 
 * parameter (program variable) used in the source program.
 */
[ExcludeClass]
public final class VariableInfo
{
    private var name_        : String        = undefined;
    private var format_      : String        = undefined;
    private var semantics_   : Semantics     = undefined;

    /**
     * Constructor to constuct a variableInfo object corresponding to the parameters used in the source.
     * @param name The name of the parameter used in the program.
     * @param format The format string representation of the type of the value in the parameter.
     * @param semantics The semantics of the parameter.
     */
    public function VariableInfo(name : String,
                                 format : String,
                                 semantics : Semantics)
    {
        Enforcer.checkNull(name);
        Enforcer.checkNull(format);

        name_ = name;
        format_ = format;
        semantics_ = semantics;
    }

    public function get format() : String
    {
        return format_;
    }

    public function get semantics() : Semantics
    {
        return semantics_;
    }

    public function get name() : String
    {
        return name_;
    }
} // VariableInfo
} // com.adobe.AGALOptimiser
