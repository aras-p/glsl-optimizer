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
import com.adobe.AGALOptimiser.utils.SerializationFlags;

import flash.utils.ByteArray;

use namespace nsinternal;

[ExcludeClass]
public final class DestinationRegister
{
    private var registerInfo_ : Register       = undefined;
    private var mask_         : WriteMask      = undefined;
    private var slotUsed_     : int            = undefined;
    private var liveRange_    : LiveRange      = null;
    private var defLine_      : DefinitionLine = null;

    public function DestinationRegister(reg : Register,
                                        mask : WriteMask,
                                        slotUsed : int = 0)
    {
        Enforcer.checkNull(reg);

        registerInfo_ = reg;

        if (mask != null)
            mask_ = mask.clone();
        else
            mask_ = new WriteMask();

        slotUsed_ = slotUsed;
    }

    nsinternal function channelCount() : int
    {
        return ((mask_.getChannel(0) ? 1 : 0) +
                (mask_.getChannel(1) ? 1 : 0) +
                (mask_.getChannel(2) ? 1 : 0) +
                (mask_.getChannel(3) ? 1 : 0));
    }

    internal function clone(context : CloningContext) : DestinationRegister
    {
        return new DestinationRegister(context.registerMap[registerInfo_],
                                       mask_.clone(),
                                       slotUsed_);
    }

    nsinternal function get definitionLine() : DefinitionLine
    {
        return defLine_;
    }

    nsinternal function set definitionLine(value : DefinitionLine) : void
    {
        defLine_ = value;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        var numHigh : int = int(registerInfo_.baseNumber + slotUsed_);
        var numLow  : int = int(registerInfo_.baseNumber + slotUsed_);

        numHigh &= 0x0000FF00;
        numHigh = numHigh >> 8;

        numLow &= 0x000000FF;

        sink.writeByte(numLow);
        sink.writeByte(numHigh);
        mask_.generateBinaryBlob(sink);
        registerInfo_.category.generateBinaryBlob(sink);
    }

    nsinternal function isWrittenCompletely() : Boolean
    {
        return (mask_.channelCount() == 4);
    }

    nsinternal function get liveRange() : LiveRange
    {
        return liveRange_;
    }

    nsinternal function set liveRange(value : LiveRange) : void
    {
        liveRange_ = value;
    }

    nsinternal function get mask() : WriteMask
    {
        return mask_;
    }

    nsinternal function get register() : Register
    {
        return registerInfo_;
    }

    nsinternal function set register(value : Register) : void
    {
        Enforcer.checkNull(value);

        registerInfo_ = value;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        registerInfo_.slotUsedBySrcOrDest = slotUsed_;
        var base : String = registerInfo_.serialize(indentLevel, flags | SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER);
        if (!isWrittenCompletely())
        {
            base += ".";
            base += mask_.serialize();
        }

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_LIVE_RANGE_CROSS_REFERENCES))
        {
            base += "(LR";
            base += (liveRange_ != null) ? liveRange_.id : "?";
            base += ")";
        }

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_DEFINITION_LINE_CROSS_REFERENCES))
        {
            base += "(DL";
            base += (defLine_ != null) ? defLine_.id : "?";
            base += ")";
        }

        //This variable stores the current slot in use.
        // Reset it to 0 , since we share the registerInfo object for the slots
        // of a logical register
        // It is set at the beginning of this method
        registerInfo_.slotUsedBySrcOrDest = 0;
        return base;
    }

    nsinternal function get slotUsed() : int
    {
        return slotUsed_;
    }

} // DestinationRegister
} // com.adobe.AGALOptimiser.agal
