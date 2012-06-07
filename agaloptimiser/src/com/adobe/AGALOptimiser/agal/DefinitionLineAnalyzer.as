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

[ExcludeClass]
public final class DefinitionLineAnalyzer
{
    private var nextId_        : int                 = 0;
    private var visitedBlocks_ : Vector.<BasicBlock> = new Vector.<BasicBlock>();

    public function DefinitionLineAnalyzer()
    {
    }

    private function analyzeBlockOperations(block : BasicBlock,
                                            blocks : Vector.<BasicBlock>,
                                            defLiveRanges : Vector.<DefinitionLine>) : void
    {
        for (var i : int = 0 ; i < block.operations.length ; ++i)
        {
            block.operations[i].id = nextId_++;

            var dest : DestinationRegister = block.operations[i].destination;

            if (dest != null) // allow outputs
            {
                var def : DefinitionLine = new DefinitionLine(block.operations[i]);

                block.operations[i].setDefinitionLineForDestination(def); 
                
                visitedBlocks_.length = 0;

                // trace("tracing for " + block.operations[i].serialize());

                traceDefinition(def, block, i + 1);  

                defLiveRanges.push(def);
            }
        }
    }

    private function checkUses(def : DefinitionLine, 
                               op : Operation, 
                               currentBlock : BasicBlock) : void
    {
        var simpleOperands : Vector.<SourceRegister> = op.simpleOperands();
        var alreadyHooked  : Boolean                 = false;

        for each (var operand : SourceRegister in simpleOperands)
        {
            // We don't check the slotCount on the registers and the value - even if it
            // just a matrix column lives for the duration of use of all registers in the matrix.
            // This account for cases where an opcode like m44,m33 has a matrix represented by just the
            // first physical register of the matrix but has to account for the liveness of all physical
            // registers making up the matrix. 
            if ((def.register == operand.register))
            {
                if (!alreadyHooked)
                {
                    alreadyHooked = true;
                    def.useSites.push(op);
                }
                
                operand.definitionLine = def;
                
                // trace("found direct use " + op.serialize());
            }
            else if (!(operand.isDirect)) 
            {
                if (def.register == operand.indirectRegister)  
                {
                    if (!alreadyHooked)
                    {
                        alreadyHooked = true;
                        def.useSites.push(op);
                    }
                
                    operand.definitionLine = def;
                    
                    // trace("found indirect use " + op.serialize());
                }
            }
        }

        // the instruction starts a new definition line, but if the register
        // being written into is the same as the one we're looking at, then we 
        // call it a use if it already hasn't been seen in the operand list.
        //
        // we see this construction in this form:
        // mov r.x, a.x
        // mov r.y, b.x
        // mov r.z, ...
        //
        // These are all essentially one instruction which is creating r and we 
        // shouldn't break them apart during code reorganization.

        if ((!alreadyHooked) && 
            (op.destination != null) &&
            (op.destination.slotUsed == def.slotUsed) &&
            (op.destination.register == def.register))
        { 
            def.useSites.push(op);
        }
    }

    nsinternal function determineDefinitionLines(blocks : Vector.<BasicBlock>) : Vector.<DefinitionLine>
    {
        nextId_ = 0;

        Enforcer.checkNull(blocks);

        var determinedRanges : Vector.<DefinitionLine> = new Vector.<DefinitionLine>();

        for each (var b : BasicBlock in blocks)
            analyzeBlockOperations(b, blocks, determinedRanges);

        return determinedRanges;
    }

    private function traceDefinition(def : DefinitionLine, 
                                     currentBlock : BasicBlock,
                                     startLocationInBlock : int) : void
    {
        if (visitedBlocks_.indexOf(currentBlock) != -1)
        {
            // trace("stopping-- block seen");
            return;
        }

        visitedBlocks_.push(currentBlock);

        for (var i : int = startLocationInBlock ; i < currentBlock.operations.length ; ++i)
        {
            // trace("looking at " + currentBlock.operations[i].serialize());
            checkUses(def, currentBlock.operations[i], currentBlock);

            // if we're redefining the virtual register from the definition site and the region
            // of the register that we are redefining covers all the channels in the original
            // definition, then we are done tracing down this definition along this path
            if ((currentBlock.operations[i].destination != null) && // null destination means it cannot by definition redefine anything
                (def.register == currentBlock.operations[i].destination.register) &&
                (def.slotUsed == currentBlock.operations[i].destination.slotUsed) && 
                (currentBlock.operations[i].destination.mask.covers(def.mask)))
            {
                // trace("stopping-- redefinition");
                return;
            }
        }

        /*
        if (currentBlock.outlets.length == 0)
            trace("stopping-- end");
        */

        for each (var b : BasicBlock in currentBlock.outlets)
            traceDefinition(def, b, 0);
    }

} // DefinitionLineAnalyzer
} // com.adobe.AGALOptimiser.translator.transformations