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
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public final class BasicBlock
{
    private var inlets_     : Vector.<BasicBlock> = new Vector.<BasicBlock>();
    private var outlets_    : Vector.<BasicBlock> = new Vector.<BasicBlock>();
    private var operations_ : Vector.<Operation>  = new Vector.<Operation>();
    private var blockNum_   : int                 = undefined;

    public function BasicBlock(blockNum : int)
    {
        blockNum_ = blockNum;
    }

    nsinternal function get blockNumber() : int
    {
        return blockNum_;
    }

    nsinternal function get inlets() : Vector.<BasicBlock>
    {
        return inlets_;
    }

    nsinternal function get operations() : Vector.<Operation>
    {
        return operations_;
    }

    nsinternal function get outlets() : Vector.<BasicBlock>
    {
        return outlets_;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var indentString0  : String     = SerializationUtils.indentString(indentLevel);
        var indentString1  : String     = SerializationUtils.indentString(indentLevel  + 4);
        var composedString : String     = indentString0;
        var b              : BasicBlock = undefined;
        var op             : Operation  = undefined;

        composedString += Constants.BASIC_BLOCK_TAG;
        composedString += " ";
        composedString += blockNum_;
        composedString += "\n";
        composedString += indentString1;
        composedString += Constants.ENTERED_FROM_TAG;
        composedString += ":";
        
        for each (b in inlets_)
        {
            composedString += " ";
            composedString += b.blockNumber;
        }

        composedString += "\n";

        for each (op in operations_)
            composedString += op.serialize(indentLevel + 4, flags);

        composedString += indentString1;
        composedString += Constants.EXITS_TO_TAG;
        composedString += ":"

        for each (b in outlets_)
        {
            composedString += " ";
            composedString += b.blockNumber;
        }

        composedString += "\n";

        composedString += indentString0;
        composedString += Constants.END_BASIC_BLOCK_TAG;
        composedString += " ";
        
        return composedString;
    }

} // BasicBlock
} // com.adobe.AGALOptimiser.agal