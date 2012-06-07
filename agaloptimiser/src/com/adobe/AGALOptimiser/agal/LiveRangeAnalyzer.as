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

import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

// The LiveRangeAnalyzer calculates the closure of DefinitionLine definition site to use site
// mapping for a particular definition site.

// We use a worklist reduction strategy to construct the live ranges from the definition
// lines.  Take a worklist of definition lines.  Until that worklist is empty, we do the following:
//
// 1. Remove a definition line from the worklist and seed a live range with it
// 2. Place all the use sites of the definition line on a list of use sites to trace 
//    and create a list of visited uses that is initially empty
// 3. Until that list is exhausted, do the following:
//     a. Look for definition lines still on the defintion line worklist. For any that
//        define the same virtual register and which share a register that is either on 
//        the list of visited use sites or the use sites to trace list, do the following:
//        i.    Add the definition line to the live range under construction
//        ii.   All the use sites on the definition line which are not in the visited or
//              to trace lists are added to the to trace list
//        iii.  Remove the definition line from the worklist.  It has been consumed.

// While step 3.a.ii adds to the worklist, it is followed by a removal of the 
// definition line whch gave rise to all those use sites, so results in an overall
// reduction in the work remaining.  

[ExcludeClass]
public final class LiveRangeAnalyzer
{
    private var defLinesWorkList_ : Vector.<DefinitionLine> = new Vector.<DefinitionLine>();
    private var visitedUses_      : Vector.<Operation>      = new Vector.<Operation>();
    private var usesLeftToTrace_  : Vector.<Operation>      = new Vector.<Operation>();
    private var nextId_           : int                     = 0;

    public function LiveRangeAnalyzer()
    {
    }

    private function addDefLineToLiveRange(defLineIndex : int,
                                           liveRange : LiveRange) : void
    {
        liveRange.addDefinitionLine(defLinesWorkList_[defLineIndex]);

        for each (var useSite : Operation in defLinesWorkList_[defLineIndex].useSites)
            if (usesLeftToTrace_.indexOf(useSite) == -1)
                usesLeftToTrace_.push(useSite);

        if (defLineIndex != (defLinesWorkList_.length - 1))
            defLinesWorkList_[defLineIndex] = defLinesWorkList_[defLinesWorkList_.length - 1];

        defLinesWorkList_.length = defLinesWorkList_.length - 1;
    }

    private function clearState() : void
    {
        defLinesWorkList_.length = 0;
        visitedUses_.length = 0;
        usesLeftToTrace_.length = 0;
    }

    nsinternal function determineLiveRanges(definitionLines : Vector.<DefinitionLine>) : Vector.<LiveRange>
    {
        var liveRanges : Vector.<LiveRange>  = new Vector.<LiveRange>();
        
        try
        {
            var defLine : DefinitionLine = undefined;
            
            nextId_ = 0;
            
            // setup the list of remaining definition lines which need to be considered

            for each (defLine in definitionLines)
                defLinesWorkList_.push(defLine);

            while (defLinesWorkList_.length > 0)
            {
                var DL : DefinitionLine = defLinesWorkList_.shift();
                var LR : LiveRange      = new LiveRange(DL.definitionSite.destination.register,
                                                        nextId_++);

                LR.addDefinitionLine(DL);

                visitedUses_.length = 0;
                usesLeftToTrace_.length = 0;

                for each (var op : Operation in DL.useSites)
                    usesLeftToTrace_.push(op);

                while (usesLeftToTrace_.length > 0)
                {
                    var useToTrace : Operation = usesLeftToTrace_.shift();

                    if (visitedUses_.indexOf(useToTrace) == -1)
                        visitedUses_.push(useToTrace);

                    traceUse(useToTrace, LR)
                }

                liveRanges.push(LR);
            }
        }
        finally
        {
            clearState();
        }

        return liveRanges;
    }

    private function traceUse(useToTrace : Operation,
                              liveRange : LiveRange) : void
    {
        for (var i : int = 0 ; i < defLinesWorkList_.length ; ++i)
        {
            if (defLinesWorkList_[i].register == liveRange.register)
            {
                for each (var useSite : Operation in defLinesWorkList_[i].useSites)
                {
                    if ((visitedUses_.indexOf(useSite) != -1) || (usesLeftToTrace_.indexOf(useSite) != -1))
                    {
                        addDefLineToLiveRange(i, liveRange);
                        return;
                    }
                }
            }
        }
    }

} // LiveRangeAnalyzer        
} // com.adobe.AGALOptimiser.translator.transformations