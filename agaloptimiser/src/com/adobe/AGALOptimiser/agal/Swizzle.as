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
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.type.TypeUtils;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

import flash.utils.ByteArray;

use namespace nsinternal;

[ExcludeClass]
public final class Swizzle
{
    private var swizzle_      : Vector.<int>     = new Vector.<int>(4, true);
    private var usedChannels_ : Vector.<Boolean> = new Vector.<Boolean>(4, true);

    private static const channelNames_ : Array = ["x", "y", "z", "w"];

    public function Swizzle(s0 : int = 0,
                            s1 : int = 1,
                            s2 : int = 2,
                            s3 : int = 3)
    {
        Enforcer.checkRange((s0 >= 0) && (s0 <= 3) &&
                            (s1 >= 0) && (s1 <= 3) &&
                            (s2 >= 0) && (s2 <= 3) &&
                            (s3 >= 0) && (s3 <= 3),
                            ErrorMessages.SWIZZLE_RANGE_VIOLATION);

        usedChannels_[0] = usedChannels_[1] = usedChannels_[2] = usedChannels_[3] = false;

        swizzle_[0] = s0;
        swizzle_[1] = s1;
        swizzle_[2] = s2;
        swizzle_[3] = s3;

        usedChannels_[s0] = true;
        usedChannels_[s1] = true;
        usedChannels_[s2] = true;
        usedChannels_[s3] = true;
    }

    nsinternal function clone() : Swizzle
    {
        return new Swizzle(swizzle_[0],
                           swizzle_[1],
                           swizzle_[2],
                           swizzle_[3]);
    }


    nsinternal function createMinimizingAndReexpandingMaps(minimizer : Vector.<int>,
                                                              reexpander : Vector.<int>) : void
    {
        minimizer.length  = distinctChannelCount();
        reexpander.length = 4;

        var i : int;
        var j : int;

        for (i = 0, j = 0 ; i < 4 ; ++i)
            if (usedChannels_[i])
                minimizer[j++] = i;

        for (i = 0 ; i < 4 ; ++i)
        {
            for (j = 0 ; j < minimizer.length ; ++j)
            {
                if (swizzle_[i] == minimizer[j])
                {
                    reexpander[i] = j;
                    break;
                }
            }
        }
    }

    nsinternal function distinctChannelCount() : int
    {
        return ((usedChannels_[0] ? 1 : 0) +
                (usedChannels_[1] ? 1 : 0) +
                (usedChannels_[2] ? 1 : 0) +
                (usedChannels_[3] ? 1 : 0));
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        var binarySwizzle : int = 0;

        binarySwizzle |= swizzle_[3];
        binarySwizzle = binarySwizzle << 2;
        binarySwizzle |= swizzle_[2];
        binarySwizzle = binarySwizzle << 2;
        binarySwizzle |= swizzle_[1];
        binarySwizzle = binarySwizzle << 2;
        binarySwizzle |= swizzle_[0];

        sink.writeByte(binarySwizzle);
    }

    nsinternal function getChannel(channelIndex : int) : int
    {
        return swizzle_[channelIndex];
    }

    nsinternal function isSwizzled() : Boolean
    {
        return ((swizzle_[0] != 0) ||
                (swizzle_[1] != 1) ||
                (swizzle_[2] != 2) ||
                (swizzle_[3] != 3));
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        if (distinctChannelCount() == 1)
            s += channelNames_[swizzle_[0]];
        else
        {
            for (var i : int = 0 ; i < 4 ; ++i)
                s += channelNames_[swizzle_[i]];
        }

        return s;
    }

    nsinternal function setChannel(channelIndex : int,
                                      swizzleIndex : int) : void
    {
        swizzle_[channelIndex] = swizzleIndex;
    }

    nsinternal static function swizzleFromType(t : Type) : Swizzle
    {
        var channelCount : int = TypeUtils.dimCount(t);

        switch (channelCount)
        {
        case 1:  return new Swizzle(0, 0, 0, 0);
        case 2:  return new Swizzle(0, 1, 1, 1);
        case 3:  return new Swizzle(0, 1, 2, 2);
        default: return new Swizzle(); 
        }
    }

} // Swizzle
} // com.adobe.AGALOptimiser.agal
