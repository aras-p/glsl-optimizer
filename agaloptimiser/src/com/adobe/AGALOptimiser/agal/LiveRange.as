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
import com.adobe.AGALOptimiser.utils.SerializationUtils;
import com.adobe.AGALOptimiser.utils.SerializationFlags;

use namespace nsinternal;

// A DefinitionLine is a mapping from a definition of a particular register to the set of all possible operations
// using that definition through all possible paths through the run-time execution graph.  More simply:
//
//     DL = def(r) -> use(def(r))*
//
// A LiveRange is the closure of all definition lines which define the same regsiter r and which share uses either directly
// or via another definition line with which a second unrelated use (or chain of unrelated uses) is shared.  
// Please see LiveRangeAnalyzer for a description of the algorithm used to create this object.
[ExcludeClass]
public class LiveRange
{
    private var defLines_        : Vector.<DefinitionLine> = new Vector.<DefinitionLine>();
    private var register_        : Register                = undefined;
    private var accumulatedMask_ : WriteMask               = new WriteMask(false, false, false, false);
    private var id_              : int                     = undefined;
    
    public function LiveRange(regInfo : Register,
                              id : int)
    {
        Enforcer.checkNull(regInfo);

        register_ = regInfo;
        id_ = id;
    }

    nsinternal function get accumulatedMask() : WriteMask
    {
        return accumulatedMask_;
    }

    nsinternal function addDefinitionLine(defLine : DefinitionLine) : void
    {
        Enforcer.check(defLine.definitionSite.destination.register == register_, ErrorMessages.INTERNAL_ERROR);

        defLine.definitionSite.setLiveRangeForDestination(this);

        for each (var op : Operation in defLine.useSites)
            op.setLiveRangeForSourceRegisters(this, defLine.register);

        accumulatedMask_.accumulate(defLine.mask);

        defLines_.push(defLine);
    }

    nsinternal function get definitionLines() : Vector.<DefinitionLine>
    {
        return defLines_;
    }

    nsinternal function definitionSitesLocationRange() : LocationRange
    {
        var start : int = defLines_[0].definitionSiteLocation;
        var end   : int = start;

        for (var i : int = 1 ; i < defLines_.length ; ++i)
        {
            var loc : int = defLines_[i].definitionSiteLocation;

            start = (start < loc) ? start : loc;
            end   = (loc < end)   ? loc   : end;
        }

        return new LocationRange(start, end);
    }

    nsinternal function get id() : int
    {
        return id_;
    }
    
    nsinternal function get register() : Register
    {
        return register_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var indentString : String = SerializationUtils.indentString(indentLevel);
        var s            : String = indentString;

        s += Constants.LIVE_RANGE_TAG;
        s += "(";
        s += Constants.ID_TAG;
        s += "=";
        s += id_;
        s += " ";
        s += Constants.REGISTER_TAG;
        s += "=";
        s += register_.serialize(indentLevel, flags | SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER);
        
        if (accumulatedMask_.channelCount() < 4)
        {
            s += ".";
            s += accumulatedMask_.serialize();
        }

        s += " ";
        s += Constants.DEFINITION_LINES_TAG;
        s += "(";

        var first : Boolean = false;

        for each (var dl : DefinitionLine in defLines_)
        {
            if (first)
                s += " ";
            else
                first = true;

            s += dl.serialize();
        }

        s += "))";

        return s;
    }

    nsinternal function useSitesLocationRange() : LocationRange
    {
        var lr : LocationRange = defLines_[0].useSitesLocationRange();

        for (var i :int = 1 ; i < defLines_.length ; ++i)
            lr = lr.merge(defLines_[i].useSitesLocationRange());

        return lr;
    }
    
} // LiveRange
} // com.adobe.AGALOptimiser.translator.transformations