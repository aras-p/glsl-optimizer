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
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

// A definition line is the site of a value definition for a particular virtual register together
// with all the uses sites that can possibly be seen by that definition independent for all possible
// paths through the execution graph at runtime.  A definition in the compiler nomenclature, 
// in this context, is the assignment into a target register.  That assignment
// has a certain potential impact on the running program.
[ExcludeClass]
public class DefinitionLine
{
    private var defSite_  : Operation           = undefined;
    private var useSites_ : Vector.<Operation>  = new Vector.<Operation>();
    
    public function DefinitionLine(defSite : Operation)
    {
        Enforcer.checkNull(defSite);
        
        defSite_ = defSite;
    }

    nsinternal function get definitionSite() : Operation
    {
        return defSite_;
    }

    nsinternal function get definitionSiteLocation() : int
    {
        return defSite_.location;
    }

    nsinternal function get id() : int
    {
        return defSite_.id;
    }

    nsinternal function get mask() : WriteMask
    {
        return defSite_.destination.mask;
    }

    nsinternal function get register() : Register
    {
        return defSite_.destination.register;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.DEFINITION_LINE_TAG;
        s += "(";
        s += Constants.ID_TAG;
        s += "=";
        s += defSite_.id;
        s += " ";
        s += Constants.REGISTER_TAG;
        s += "=";
        s += defSite_.destination.serialize();
        s += " ";
        s += Constants.USED_BY_TAG
        s += "(";
        
        var first : Boolean = true;

        for each (var useSite : Operation in useSites_)
        {
            if (!first)
                s += ", ";
            else
                first = false;

            s += useSite.id;
        }

        s += "))";

        return s;
    }

    nsinternal function get slotUsed() : int
    {
        return defSite_.destination.slotUsed;
    }

    nsinternal function get useSites() : Vector.<Operation>
    {
        return useSites_;
    }

    nsinternal function useSitesLocationRange() : LocationRange
    {
        if (useSites_.length == 0)
            return new LocationRange(-1, -1);
        else
        {
            var start : int = useSites_[0].location;
            var end   : int = start;

            for (var i : int = 1 ; i < useSites_.length ; ++i)
            {
                var loc : int = useSites_[i].location;

                start = (start < loc) ? start : loc;
                end   = (loc < end)   ? loc   : end;
            }

            return new LocationRange(start, end);
        }
    }

} // DefinitionLine
} // com.adobe.AGALOptimiser.translator.transformation