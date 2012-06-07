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

import com.adobe.AGALOptimiser.agal.BasicBlock;
import com.adobe.AGALOptimiser.agal.Instruction;
import com.adobe.AGALOptimiser.agal.Opcode;
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.SampleOperation;
import com.adobe.AGALOptimiser.agal.SourceRegister;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.agal.DefinitionLine;
import com.adobe.AGALOptimiser.agal.LiveRange;

import flash.utils.Dictionary;

use namespace nsinternal;

[ExcludeClass]
public final class InterferenceGraphGenerator
{
    private var blocks_         : Vector.<BasicBlock> = null;
    private var liveOutSets_    : Array               = null;
    private var liveRanges_     : Vector.<LiveRange>  = null;
    private var iGraph_         : InterferenceGraph   = null;
    
    public function InterferenceGraphGenerator()
    {
    }

    private function addLiveRange(liveRange : LiveRange,
                                  liveRangeSet : Vector.<LiveRange>,
                                  allowDuplicate : Boolean = false) : void
    {
        if (allowDuplicate || (liveRangeSet.indexOf(liveRange) == -1))
            liveRangeSet.push(liveRange);
    }
    
    private function addLiveRangeV1(liveRange : LiveRange,
                                    liveRangesInPlay : Dictionary) : void
    {
        if (liveRangesInPlay[liveRange] == null)
            liveRangesInPlay[liveRange] = liveRange.definitionLines.slice(0); // , liveRange.definitionLines.length - 1);
    }
                                    

    nsinternal function buildInterferenceGraph(blocks : Vector.<BasicBlock>,
                                                  liveRanges : Vector.<LiveRange>) : InterferenceGraph
    {
        var iGraph : InterferenceGraph = null;

        try
        {
            blocks_         = blocks;
            liveRanges_     = liveRanges;
            iGraph_         = new InterferenceGraph();

            buildLiveOutSets();

            setupNodes();

            for each (var b : BasicBlock in blocks_)
                buildInterferenceGraphForBlockV1(b);
        }
        finally
        {
            iGraph          = iGraph_;

            blocks_         = null;
            liveOutSets_    = null;
            liveRanges_     = null;
            iGraph_         = null;
        }
        
        return iGraph;    
    }

    private function buildInterferenceGraphForBlock(b : BasicBlock) : void
    {
        var liveOut : Vector.<LiveRange> = liveOutSets_[b];
        var liveNow : Vector.<LiveRange> = liveOut.slice(0, liveOut.length - 1);

        for (var i : int = b.operations.length - 1 ; i >= 0 ; --i)
        {
            var op : Operation = b.operations[i];
            
            if ((op.destination == null) || (op.destination.liveRange == null))
                continue;

            for each (var liveRange : LiveRange in liveNow)
            {
                var A : InterferenceGraphNode = iGraph_.findNode(op.destination.liveRange);
                var B : InterferenceGraphNode = iGraph_.findNode(liveRange);

                if ((A != B) && (!A.isConnectedTo(B)))
                {
                    iGraph_.addEdge(new InterferenceGraphEdge(A, B));
                }
            }

            removeLiveRange(op.destination.liveRange, liveNow);

            for each (var operand : SourceRegister in op.simpleOperands())
                if (operand.liveRange != null)
                    addLiveRange(operand.liveRange, liveNow);
        }
    }
    
    // this works because we have only one basic block, though we might be 
    // able to extend this to a more general solution.  Because we have 
    // one blocks, the liveouts don't exist.
    private function buildInterferenceGraphForBlockV1(b : BasicBlock) : void
    {
        var liveNow  : Dictionary = new Dictionary();

        for (var i : int = b.operations.length - 1 ; i >= 0 ; --i)
        {
            var op : Operation = b.operations[i];

            if ((op.destination == null) || (op.destination.liveRange == null))
                continue;

            for (var key : Object in liveNow)
            {
                if (!(key is LiveRange))
                    continue;
                
                var liveRange : LiveRange = key as LiveRange;
               
                if (liveNow[liveRange].length > 0) // all the def sites haven't been removed for this key
                {
                    var A : InterferenceGraphNode = iGraph_.findNode(op.destination.liveRange);
                    var B : InterferenceGraphNode = iGraph_.findNode(liveRange);
                     
                    if ((A != B) && (!A.isConnectedTo(B)))
                    {
                        iGraph_.addEdge(new InterferenceGraphEdge(A, B));
                    }
                }
            }

            removeDefinitionSite(op.destination.liveRange, op, liveNow);

            for each (var operand : SourceRegister in op.simpleOperands())
            {
                if (operand.liveRange != null)
                    addLiveRangeV1(operand.liveRange, liveNow);
            }

        }
    }

    private function buildLiveOutSets() : void
    {
        liveOutSets_ = new Array();

        for each (var b : BasicBlock in blocks_)
            liveOutSets_[b] = new Vector.<LiveRange>();

        for each (var r : LiveRange in liveRanges_)
            buildLiveOutSetsForLiveRange(r);
    }

    private function buildLiveOutSetsForLiveRange(liveRange : LiveRange) : void
    {
        for each (var defLine : DefinitionLine in liveRange.definitionLines)
        {
            var liveOut : Vector.<LiveRange> = liveOutSets_[defLine.definitionSite.block];

            for each (var useSite : Operation in defLine.useSites)
            {
                if ((useSite.block != defLine.definitionSite.block) && (liveOut.indexOf(liveRange) == -1))
                    liveOut.push(liveRange);
            }
        }
    }
    
    private function removeDefinitionSite(liveRange : LiveRange,
                                          definitionSite : Operation,
                                          liveRangesInPlay : Dictionary) : void
    {
        var current   : Vector.<DefinitionLine> = liveRangesInPlay[liveRange];
        var remaining : Vector.<DefinitionLine> = new Vector.<DefinitionLine>();

        for each (var DL : DefinitionLine in current)
            if (DL.definitionSite != definitionSite)
                remaining.push(DL);

        liveRangesInPlay[liveRange] = remaining;
    }

    private function removeLiveRange(liveRange : LiveRange,
                                     liveRangeSet : Vector.<LiveRange>) : void
    {
        var i : int = liveRangeSet.indexOf(liveRange);

        if (i != -1)
        {
            if (i < (liveRangeSet.length - 1))
                liveRangeSet[i] = liveRangeSet[liveRangeSet.length - 1];

            liveRangeSet.length -= 1;
        }
    }
    
    private function setupNodes() : void
    {
        for each (var liveRange : LiveRange in liveRanges_)
            iGraph_.nodes.push(new InterferenceGraphNode(liveRange));
    }

    private function traceState(liveNow : Vector.<LiveRange>) : void
    {
        var s  : String    = null;
        var LR : LiveRange = null;

        s = "LiveNow:";

        for each (LR in liveNow)
        {
            s += " ";
            s += LR.id;
        }

        trace(s);
    }

    private function traceStateV1(liveNow : Dictionary) : void
    {
        var s  : String    = null;

        s = "LiveNow:";

        for (var key : Object in liveNow)
        {
            if (!(key is LiveRange))
                continue;
            
            var LR : LiveRange = key as LiveRange;
            
            s += " ";
            s += LR.id;

            s += ":";
            s += liveNow[LR].length;
        }

        trace(s);
    }

} // InterferenceGraphGenerator
} // com.adobe.AGALOptimiser.translator.transformations