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

package com.adobe.AGALOptimiser.agal {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

// REVIEW: once the if and call operations were removed from agal version 1, this class became
// fairly vacuous. Consider removing it and altering register allocation accordingly  

[ExcludeClass]
internal class BasicBlockAnalyzer
{
    private var nextBlockNum_ : int = 0;

    public function BasicBlockAnalyzer()
    {
    }

    // This function returns a vector of BasicBlocks, which decompose the program into the different regions
    // needed for consideration for register allocation.  
    nsinternal function determineBlockStructure(agalProgram : Program) : void
    {
        nextBlockNum_ = 0;

        Enforcer.checkNull(agalProgram);
        Enforcer.check((agalProgram.main != null) && (agalProgram.procedures.length == 1), 
                       ErrorMessages.AGAL_CONTROL_FLOW_RESTRICTION_V1);
        
        var mainEntry    : Procedure           = agalProgram.main;
        var currentBlock : BasicBlock          = new BasicBlock(nextBlockNum_++);
        var i            : int                 = 0;

        mainEntry.basicBlocks.length = 0;
        mainEntry.basicBlocks.push(currentBlock);

        while (i < mainEntry.operations.length)
        {
            currentBlock.operations.push(mainEntry.operations[i]);
            mainEntry.operations[i].block = currentBlock;
            ++i;
        }
    }

} // BasicBlockAnalyzer
} // com.adobe.AGALOptimiser.agal