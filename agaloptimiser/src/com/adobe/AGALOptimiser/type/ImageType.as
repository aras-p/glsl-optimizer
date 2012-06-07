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

package com.adobe.AGALOptimiser.type {

import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public class ImageType extends Type
{
    private var channelCount_ : int = undefined;

    public function ImageType(channelCount : int)
    {
        super();
        if ((channelCount < 1) || (channelCount > 4))
            throw new Error(ErrorMessages.INVALID_CHANNEL_COUNT);

        channelCount_ = channelCount;
    }

    override nsinternal function channelCount() : int
    {
        return channelCount_;
    }

    override nsinternal function clone() : Type
    {
        return new ImageType(channelCount_);
    }

    override nsinternal function equals(t : Type) : Boolean
    {
        return ((t != null) &&
                (t.isImage()));
    }

    override nsinternal function isImage() : Boolean
    {
        return true;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);
        return (s + Constants.IMAGE_TYPE_STRING + "-" + channelCount_);
    }

    override nsinternal function getStage3DTypeFormatString() : String
    {
        return (Constants.IMAGE_TYPE_STRING+ channelCount_);
    }

} // ImageType
} // com.adobe.AGALOptimiser.type
