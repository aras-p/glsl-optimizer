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

package com.adobe.AGALOptimiser.utils {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.nsinternal;

import flash.utils.Dictionary;

use namespace nsinternal;

[ExcludeClass]
public class UniqueNameGenerator
{
    private var nameTable_  : Dictionary      = new Dictionary();
    private var keys_       : Vector.<String> = new Vector.<String>();
    private var prefix_     : String          = null;
    private var generation_ : int             = -1;

    public function UniqueNameGenerator()
    {

    }

    // there is no pair and no by-ref parameters of basic types, so we're using prefix_ and generation_ to
    // hold the results.
    private function breakNameIntoPrefixAndGeneration(nameWithOptionalTrailingDigits : String) : void
    {
        Enforcer.checkNull(nameWithOptionalTrailingDigits);

        var digits : RegExp = new RegExp(/\d+$/); // $ means it must be at the end of the string
        var index : int = nameWithOptionalTrailingDigits.search(digits);

        if (index != -1)
        {
            generation_ = int(nameWithOptionalTrailingDigits.substring(index));
            prefix_     = nameWithOptionalTrailingDigits.slice(0, index);
        }
        else
        {
            generation_ = -1;
            prefix_     = nameWithOptionalTrailingDigits;
        }
    }

    nsinternal function clone() : UniqueNameGenerator
    {
        var newGenerator : UniqueNameGenerator = new UniqueNameGenerator();

        // iterating the table gives the values, not the keys, so we need to maintain
        // a set of current keys in order to clone.

        for each (var key : String in keys_)
            newGenerator.nameTable_[key] = nameTable_[key];

        newGenerator.keys_ = newGenerator.keys_.concat(keys_);

        // there is no need clone prefix_ & generation_ as they are just temps used internally

        return newGenerator;
    }

    nsinternal function getUniqueName(nameRoot : String) : String
    {
        breakNameIntoPrefixAndGeneration(nameRoot);

        if (nameTable_[prefix_] == null)
        {
            // we have a new root, let's establish it in the table and return it as is
            // if the new root already contains a number like m0..add that too
            // generation_ contains that integer prefix
            if (generation_ != -1)
                prefix_ = prefix_ + generation_;
            nameTable_[prefix_] = -1;
            keys_.push(prefix_);
            return prefix_;
        }
        else
        {
            var newNumber : int = nameTable_[prefix_] = nameTable_[prefix_] + 1;
            return prefix_ + newNumber;
        }
    }

    // this is used for establishing a name in the wild in the table.  Once registered,
    // it will not be returned again when new getUniqueName requests are issued.
    nsinternal function registerNameIntoTable(nameWithOptionalTrailingDigits : String) : void
    {
        breakNameIntoPrefixAndGeneration(nameWithOptionalTrailingDigits);

        if (nameTable_[prefix_] == null)
        {
            nameTable_[prefix_] = generation_;
            keys_.push(prefix_);
        }
        else if (nameTable_[prefix_] < generation_)
            nameTable_[prefix_] = generation_;
    }

} // UniqueNameGenerator
} // com.adobe.AGALOptimiser.utils
