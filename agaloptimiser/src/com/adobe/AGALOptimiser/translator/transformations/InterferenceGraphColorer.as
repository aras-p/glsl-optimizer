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

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.nsinternal;

use namespace nsinternal;

[ExcludeClass]
public class InterferenceGraphColorer
{
    private static var MAX_COLORS : int = 32;

    private var iGraph_          : InterferenceGraph              = null;
    private var colorOrderStack_ : Vector.<InterferenceGraphNode> = new Vector.<InterferenceGraphNode>();

    public function InterferenceGraphColorer()
    {
    }
    
    private function chooseRemovalTarget() : int
    {
        var leastIndex  : int = -1;
        var leastDegree : int = -1;

        for (var i : int = 0 ; i < iGraph_.nodes.length ; ++i)
        {
            var deg : int = 0;

            for each (var edge : InterferenceGraphEdge in iGraph_.nodes[i].edges)
                deg += edge.degree;

            if ((deg < leastDegree) || (leastIndex == -1))
            {
                leastDegree = deg;
                leastIndex = i;
            }
        }

        return leastIndex;
    }

    nsinternal function colorGraph(toColor : InterferenceGraph) : Boolean
    {
        Enforcer.checkNull(toColor);

        var allColored : Boolean = false;

        try 
        {
            iGraph_ = toColor;

            // first, rip the graph apart to get the right coloring order

            while (iGraph_.nodes.length > 0)
            {
                var toRemove : int = chooseRemovalTarget();

                colorOrderStack_.push(iGraph_.nodes[toRemove]);

                removeNodeAndEdges(toRemove);
                
                // trace(iGraph_.serialize());
            }

            // now put it back together in the right order, coloring as we go

            while (colorOrderStack_.length > 0)
            {
                var toAdd : InterferenceGraphNode = colorOrderStack_.pop();

                iGraph_.addNode(toAdd);

                colorNode(toAdd);
                
                // trace(iGraph_.serialize());
            }

            // now test the result

            allColored = true;

            for each (var node : InterferenceGraphNode in iGraph_.nodes)
            {
                if (node.color == -1)
                    allColored = false;
            }
        }
        finally
        {
            iGraph_ = null;
            colorOrderStack_.length = 0;
        }

        return allColored;
    }


    private function colorNode(node : InterferenceGraphNode) : void
    {
        for (var i : int = 0 ; i < MAX_COLORS ; ++i)
        {
            var used : Boolean = false;

            for each (var edge : InterferenceGraphEdge in node.edges)
            {
                if ((edge.source == node) && (edge.target.color == i))
                {
                    used = true;
                    break;
                }
                else if ((edge.target == node) && (edge.source.color == i))
                {
                    used = true;
                    break;
                }
            }

            if (!used)
            {
                node.color = i;
                break;
            }
        }
    }

    private function removeNodeAndEdges(toRemove : int) : void
    {
        var nodeToRemove : InterferenceGraphNode = iGraph_.nodes[toRemove];
        var lastIndex    : int                   = iGraph_.nodes.length - 1;

        for each (var edge : InterferenceGraphEdge in nodeToRemove)
        {
            if (edge.source == nodeToRemove)
                edge.target.removeEdgesTo(nodeToRemove);
            else
                edge.source.removeEdgesTo(nodeToRemove);
        }

        iGraph_.nodes[toRemove] = iGraph_.nodes[lastIndex];
        iGraph_.nodes.length = lastIndex;
    }

    
} // InterferenceGraphColorer
} // com.adobe.AGALOptimiser.translator.transformations