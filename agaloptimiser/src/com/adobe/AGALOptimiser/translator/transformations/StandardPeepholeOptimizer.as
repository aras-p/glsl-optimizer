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

import com.adobe.AGALOptimiser.agal.Opcode;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.type.TypeSet;

import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public class StandardPeepholeOptimizer extends PatternPeepholeOptimizer
{
    public function StandardPeepholeOptimizer(p : Program)
    {
        if (rules_ == null)
            setupRules();

        super(p);
    }
    
    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_STANDARD_PEEPHOLE_OPTIMIZER;
    }

    private static function setupRules() : void
    {
        var S0 : SourceRegisterTemplate;
        var S1 : SourceRegisterTemplate;
        var D0 : DestinationRegisterTemplate;
        var D1 : DestinationRegisterTemplate;
        var D2 : DestinationRegisterTemplate;
        var DT : DestinationRegisterTemplate;
        var TP : TransformationPattern;
        var ST : SourceRegisterTemplate;

        // rcp D1.w1, S0.s0
        // rcp D2.w2, S1.s1     ->      mov D2.w2, S0.w(1<-0)

        S0 = new SourceRegisterTemplate(0);
        S1 = new SourceRegisterTemplate(1);

        D1 = new DestinationRegisterTemplate(1, TypeSet.floats(), 0);
        D2 = new DestinationRegisterTemplate(2, TypeSet.floats(), 0);
        
        TP = new TransformationPattern("y=rcp(rcp(x))");

        TP.sourcePattern.push(new InstructionTemplate(D1, new Opcode(Opcode.RCP), S0, null));
        TP.sourcePattern.push(new InstructionTemplate(D2, new Opcode(Opcode.RCP), S1, null));
        
        ST = new SourceRegisterTemplate(0, Vector.<int>([1, 1, 0]));
        DT = new DestinationRegisterTemplate(2, TypeSet.floats(), 0, Vector.<int>([2, 1, 1, 0]));

        TP.targetPattern.push(new InstructionTemplate(DT, new Opcode(Opcode.MOVE), ST, null));

        rules_ = new Vector.<TransformationPattern>();

        rules_.push(TP);
    }
    
} // StandardPeepholeOptimizer
} // com.adobe.AGALOptimiser.translator.transformations