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

import com.adobe.AGALOptimiser.nsinternal;

import flash.utils.Dictionary;

use namespace nsinternal;

// This class is used during the testing of whether or not a rule applies to a 
// particular instruction tree.  If completed, then the rule is applicable.
// If the testing proves the instruction tree is not a match for the rule pattern,
// then the object is abandoned.
//
// It's done this way since the act of testing is tantamount to determining the 
// binding.  

[ExcludeClass]
internal final class TemplateBindings
{
    public var rule              : TransformationPattern = null;
    public var boundSources      : Dictionary            = new Dictionary();
    public var boundInstructions : Dictionary            = new Dictionary();
    public var boundDestinations : Dictionary            = new Dictionary();
    public var boundTypes        : Dictionary            = new Dictionary();

    public function TemplateBindings()
    {
    }

} // TemplateBindings
} // com.adobe.AGALOptimiser.translator.transformations