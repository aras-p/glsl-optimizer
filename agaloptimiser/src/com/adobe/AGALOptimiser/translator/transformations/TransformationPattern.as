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

// patterns relate one tree of instructions/values with a new tree of instructions/values.
// 
// Basically, it consists of two sequences of patterns.  On the source side, is tree of values/instructions
// stemming from a particular result calculation.  The last pattern in the list is the final destination
// from which the pattern matching is actually performed when instructions are inspected in the tree.  
// For example, the instruction stream containing....
//
//     rcp B.w, A.s         
//     rcp C.w, B.s            
//
// might match a pattern containing
//
//     rcp ft1, ft0.xyzz
//     rcp ft2.xyz, ft1
//
// the target pattern indicates how to alter the stream.  For the above optimization, the target might be 
// 
//     mov C.w, A.s
//
// The final instruction can be replaced at pattern substitution since we will only apply this on
// registers which are written once (and in some sense in SSA-ish form at least locally at the point of 
// a specific write).  As an aside, these peephole optimizations take place before register allocation.
//
// The instruction stream would become...
//
//     rcp ft1, ft0.xyzz         <-- unless this intermediate calculation was used independently, it is dead now and will be cleaned up
//     mov ft2.xyz, ft0.xyzz

use namespace nsinternal;

[ExcludeClass]
public final class TransformationPattern
{
    private var ruleName_      : String                       = undefined;
    private var sourcePattern_ : Vector.<InstructionTemplate> = new Vector.<InstructionTemplate>();
    private var targetPattern_ : Vector.<InstructionTemplate> = new Vector.<InstructionTemplate>();

    public function TransformationPattern(ruleName : String)
    {
        ruleName_ = ruleName;
    }

    internal function getDefinitionPattern(id : int) : InstructionTemplate
    {
        for each (var it : InstructionTemplate in sourcePattern_)
            if ((it.destination != null) && (it.destination.id == id))
                return it;

        return null;
    }

    internal function get name() : String
    {
        return ruleName_;
    }

    internal function get sourcePattern() : Vector.<InstructionTemplate>
    {
        return sourcePattern_;
    }

    internal function get targetPattern() : Vector.<InstructionTemplate>
    {
        return targetPattern_;
    }
    
} // TransformationPattern
} // com.adobe.AGALOptimiser.translator.transformations