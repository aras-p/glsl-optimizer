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

import com.adobe.AGALOptimiser.agal.LiveRange;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public final class InterferenceGraphNode
{
    private var liveRange_ : LiveRange                      = undefined;
    private var color_     : int                            = -1;
    private var edges_     : Vector.<InterferenceGraphEdge> = new Vector.<InterferenceGraphEdge>();
    
    public function InterferenceGraphNode(range : LiveRange)
    {
        Enforcer.checkNull(range);
        
        liveRange_ = range;
    }

    nsinternal function addEdge(edge : InterferenceGraphEdge) : void
    {
        Enforcer.checkNull(edge);

        if (edges_.indexOf(edge) == -1)
        {
            edges_.push(edge);
            
            // assume the nodes are sorted, now let's bubble the newly added edge into place

            for (var i : int = edges_.length - 1 ; i >= 1 ; --i)
            {
                var iWeight   : int = edges_[i].source.id + edges_[i].target.id;
                var im1Weight : int = edges_[i-1].source.id + edges_[i-1].target.id;

                if (iWeight < im1Weight)
                {
                    // swap
                    var temp : InterferenceGraphEdge = edges_[i];

                    edges_[i] = edges_[i-1];
                    edges_[i-1] = temp;
                }
                else
                    break;
            }
        }
    }

    nsinternal function get color() : int
    {
        return color_;
    }

    nsinternal function set color(value : int) : void
    {
        color_ = value;
    }

    nsinternal function get edges() : Vector.<InterferenceGraphEdge>
    {
        return edges_;
    }
    
    nsinternal function get id() : int
    {
        return liveRange_.id;
    }

    nsinternal function isConnectedTo(neighbor : InterferenceGraphNode) : Boolean
    {
        for each (var edge : InterferenceGraphEdge in edges_)
            if ((edge.source == neighbor) || (edge.target == neighbor))
                return true;

        return false;
    }

    nsinternal function get liveRange() : LiveRange
    {
        return liveRange_;
    }

    nsinternal function removeEdgesTo(target : InterferenceGraphNode) : void
    {
        var i : int = undefined;

        for (i = 0 ; i < edges_.length ; ++i)
            if ((edges_[i].source == target) || (edges_[i].target == target))
                break;

        if (i == edges_.length)
            throw new Error(ErrorMessages.INTERNAL_ERROR);

        edges_[i] = edges_[edges_.length - 1];

        edges_.length = edges_.length - 1;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);
        
        s += Constants.ID_TAG;
        s += "=";
        s += id;
        s += " ";
        s += Constants.COLOR_TAG;
        s += "=";
        s += color_;
        s += " ";
        s += Constants.EDGES_TAG;
        s += "=";
        s += edges_.length;
        s += " ";
        s += liveRange_.serialize();

        for each (var edge : InterferenceGraphEdge in edges_)
        {
            s += "\n";
            s += edge.serialize(indentLevel + 4);
        }

        s += "\n";

        return s;
    }
} // InterferenceGraphNode
} // com.adobe.AGALOptimiser.translator.transformations