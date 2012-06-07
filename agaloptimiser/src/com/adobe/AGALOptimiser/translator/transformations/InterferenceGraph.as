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
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;
import com.adobe.AGALOptimiser.agal.LiveRange;

use namespace nsinternal;

[ExcludeClass]
public final class InterferenceGraph
{
    private var nodes_ : Vector.<InterferenceGraphNode> = new Vector.<InterferenceGraphNode>();

    public function InterferenceGraph()
    {
    }

    nsinternal function addNode(node : InterferenceGraphNode) : void
    {
        Enforcer.checkNull(node);

        if (nodes_.indexOf(node) != -1)
            return;

        // first step, add the node
        nodes_.push(node);

        for each (var edge : InterferenceGraphEdge in node.edges)
        {
            if (edge.source == node)
                edge.target.addEdge(edge);
            else
            {
                Enforcer.check(edge.target == node, ErrorMessages.INTERNAL_ERROR);
                edge.source.addEdge(edge);
            }
        }
    }

    nsinternal function addEdge(edge : InterferenceGraphEdge) : void
    {
        Enforcer.checkNull(edge);

        edge.source.addEdge(edge);
        edge.target.addEdge(edge);
    }

    nsinternal function areConnected(A : InterferenceGraphNode,
                                        B : InterferenceGraphNode) : Boolean
    {
        return A.isConnectedTo(B);
    }

    nsinternal function findNode(range : LiveRange) : InterferenceGraphNode
    {
        for each (var node : InterferenceGraphNode in nodes_)
            if (node.id == range.id)
                return node;
            
        return null;
    }
    
    nsinternal function get nodes() : Vector.<InterferenceGraphNode>
    {
        return nodes_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var indentString : String = SerializationUtils.indentString(indentLevel);
        var s            : String = indentString;

        s += Constants.INTERFERENCE_GRAPH_TAG;
        s += ":";

        for each (var node : InterferenceGraphNode in nodes_)
        {
            s += "\n";
            s += node.serialize(indentLevel + 4);
        }

        return s;
    }
} // InterferenceGraph
} // com.adobe.AGALOptimiser.translator.transformations