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

import flash.utils.ByteArray;

use namespace nsinternal;

[ExcludeClass]
public final class WriteMask
{
    private var channels_ : Vector.<Boolean> = new Vector.<Boolean>(4, true);

    public function WriteMask(writex : Boolean = true,
                              writey : Boolean = true,
                              writez : Boolean = true,
                              writew : Boolean = true)
    {
        channels_[0] = writex;
        channels_[1] = writey;
        channels_[2] = writez;
        channels_[3] = writew;
    }

    nsinternal function accumulate(m : WriteMask) : void
    {
        channels_[0] = channels_[0] || m.getChannel(0);
        channels_[1] = channels_[1] || m.getChannel(1);
        channels_[2] = channels_[2] || m.getChannel(2);
        channels_[3] = channels_[3] || m.getChannel(3);
    }

    nsinternal function channelCount() : int
    {
        return ((channels_[0] ? 1 : 0) +
                (channels_[1] ? 1 : 0) +
                (channels_[2] ? 1 : 0) +
                (channels_[3] ? 1 : 0));
    }

    nsinternal function clone() : WriteMask
    {
        return new WriteMask(channels_[0],
                             channels_[1],
                             channels_[2],
                             channels_[3]);
    }

    nsinternal function covers(m : WriteMask) : Boolean
    {
        // this mask will "cover" mask m if all the channels set in m are also set in this

        return ((!m.getChannel(0) || getChannel(0)) &&
                (!m.getChannel(1) || getChannel(1)) &&
                (!m.getChannel(2) || getChannel(2)) &&
                (!m.getChannel(3) || getChannel(3)));
    }

    nsinternal function firstWrittenChannel() : int
    {
        for (var i : int = 0 ; i < 4 ; ++i)
            if (channels_[i])
                return i;

        throw new Error(ErrorMessages.INTERNAL_ERROR);
        return -1;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        var binaryMask : int = 0x0;

        binaryMask |= (channels_[0] ? 0x1 : 0x0);
        binaryMask |= (channels_[1] ? 0x2 : 0x0);
        binaryMask |= (channels_[2] ? 0x4 : 0x0);
        binaryMask |= (channels_[3] ? 0x8 : 0x0);

        sink.writeByte(binaryMask);
    }

    nsinternal function getChannel(i : int) : Boolean
    {
        return channels_[i];
    }

    nsinternal function isSubrangeEmpty(startOffset : int,
                                           numChannels : int) : Boolean
    {
        for (var i : int = 0 ; i < numChannels ; ++i)
            if (channels_[i + startOffset])
                return false;

        return true;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        if (channels_[0]) 
            s += "x";
        if (channels_[1]) 
            s += "y";
        if (channels_[2]) 
            s += "z"
        if (channels_[3]) 
            s += "w";

        return s;
    }

    nsinternal function setChannel(i : int,
                                      value : Boolean) : void
    {
        channels_[i] = value;
    }

    nsinternal function shift(shiftAmount : int) : void
    {
        var i : int;

        if (shiftAmount > 0)
        {
            for (i = 3 ; i >= 0 ; --i)
                channels_[i] = ((i - shiftAmount) >= 0) ? channels_[i - shiftAmount] : false;
        }
        else if (shiftAmount < 0)
        {
            for (i = 0 ; i < 4 ; ++i)
                channels_[i] = ((i - shiftAmount) < 4) ? channels_[i - shiftAmount] : false;
        }
    }

} // WriteMask
} // com.adobe.AGALOptimiser.agal
