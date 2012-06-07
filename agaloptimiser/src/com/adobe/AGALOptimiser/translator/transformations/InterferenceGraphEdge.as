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
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public final class InterferenceGraphEdge
{
    private var source_ : InterferenceGraphNode = undefined;
    private var target_ : InterferenceGraphNode = undefined;
    private var degree_ : int                   = 0;
    
    public function InterferenceGraphEdge(source : InterferenceGraphNode,
                                          target : InterferenceGraphNode)
    {
        Enforcer.checkNull(source);
        Enforcer.checkNull(target);

        // order can come randomly due to use of Dictionary in construction (memory 
        // layout dependent ordering), so let's normalize things.

        if (source.id < target.id)
        {
            source_ = source;
            target_ = target;
        }
        else
        {
            source_ = target;
            target_ = source;
        }

        var sourceChannelCount : int = source.liveRange.accumulatedMask.channelCount();
        var targetChannelCount : int = target.liveRange.accumulatedMask.channelCount();

        degree_ = sourceChannelCount > targetChannelCount ? sourceChannelCount : targetChannelCount;
    }

    nsinternal function get degree() : int
    {
        return degree_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += source_.id;
        s += " <-> ";
        s += target_.id;
        s += " ";
        s += Constants.DEGREE_TAG;
        s += "=";
        s += degree_;

        return s;
    }

    nsinternal function get source() : InterferenceGraphNode
    {
        return source_;
    }

    nsinternal function get target() : InterferenceGraphNode
    {
        return target_;
    }
} // InterferenceGraphEdge
} // com.adobe.AGALOptimiser.translator.transformations