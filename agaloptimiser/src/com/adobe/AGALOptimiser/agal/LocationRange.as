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
public final class LocationRange
{
    private var start_ : int = undefined;
    private var end_   : int = undefined;
    
    public function LocationRange(start : int,
                                  end : int)
    {
        Enforcer.check(start <= end, ErrorMessages.INTERNAL_ERROR);            
        
        start_ = start;
        end_ = end;
    }
    
    nsinternal function get end() : int
    {
        return end_;
    }
    
    nsinternal function merge(range : LocationRange) : LocationRange
    {
        return new LocationRange(((this.start < range.start) ? start_ : range.start),
                                 ((this.end   > range.end)   ? end_   : range.end));
    }
    
    nsinternal function get start() : int
    {
        return start_;
    }
    
} // LocationRange
} // com.adobe.AGALOptimiser.agal