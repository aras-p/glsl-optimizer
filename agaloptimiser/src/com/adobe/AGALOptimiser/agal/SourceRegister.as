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
import com.adobe.AGALOptimiser.utils.SerializationFlags;

import flash.utils.ByteArray;

use namespace nsinternal;

[ExcludeClass]
public final class SourceRegister
{
    private var registerInfo_         : Register         = undefined;
    private var indirectRegisterInfo_ : Register         = undefined;
    private var indexOffset_          : int              = undefined;
    private var indirectSwizzle_      : Swizzle          = undefined;
    private var isDirect_             : Boolean          = undefined;
    private var swizzle_              : Swizzle          = undefined;
    private var liveRange_            : LiveRange        = null;
    private var defLine_              : DefinitionLine   = null;

    // This variable is for cases where we use only a single slot of a logical register
    // ex: a column of a matrix
    private var slotUsed_             : int              = undefined;
    
    public function SourceRegister(reg : Register,
                                   swizzle : Swizzle = null,
                                   isDirect : Boolean = true,
                                   slotUsed : int = 0,
                                   indirectRegister : Register = null,
                                   indirectSwizzle : Swizzle = null,
                                   indexOffset : int = 0)

    {
        Enforcer.checkNull(reg);

        registerInfo_ = reg;
        if (indirectRegister)
        {
            indirectRegisterInfo_ = indirectRegister;
            indexOffset_ = indexOffset;
        }
        
        if (swizzle != null)
            swizzle_ = swizzle.clone();
        else
            swizzle_ = new Swizzle();
        
        if (indirectSwizzle != null)
            indirectSwizzle_ = indirectSwizzle;
       
        isDirect_ = isDirect;
        slotUsed_ = slotUsed;
    }

    internal function clone(context : CloningContext) : SourceRegister
    {
        return new SourceRegister(context.registerMap[registerInfo_],
                                  swizzle_.clone(),
                                  isDirect_,
                                  slotUsed_,
                                  indirectRegisterInfo_,
                                  indirectSwizzle_,
                                  indexOffset_);
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
        if (isDirect_)
        {
            var numHigh : int = int(registerInfo_.baseNumber + slotUsed_);
            var numLow  : int = int(registerInfo_.baseNumber + slotUsed_);

            numHigh &= 0x0000FF00;
            numHigh = numHigh >> 8;

            numLow &= 0x000000FF;
            sink.writeByte(numLow);
            sink.writeByte(numHigh);
            sink.writeByte(0x00);
            swizzle_.generateBinaryBlob(sink);
            registerInfo_.category.generateBinaryBlob(sink);
            sink.writeByte(0x00);
            sink.writeByte(0x00);

            sink.writeByte(0x00);
        }
        else
        {
            var regNumHigh : int = int(indirectRegisterInfo_.baseNumber);
            var regNumLow  : int = int(indirectRegisterInfo_.baseNumber);
            
            regNumHigh &= 0x0000FF00;
            regNumHigh = regNumHigh >> 8;
            
            regNumLow &= 0x000000FF;
            sink.writeByte(regNumLow);
            sink.writeByte(regNumHigh);
            // To account for the start value of the register containing the array or matrix
            indexOffset_ += registerInfo_.baseNumber;
            // To account for the offset within a matrix
            indexOffset_ += slotUsed_;
            sink.writeByte(indexOffset_);
            swizzle_.generateBinaryBlob(sink);
            registerInfo_.category.generateBinaryBlob(sink);
            indirectRegisterInfo_.category.generateBinaryBlob(sink);
            var indirectComponent : int = 0;
            indirectComponent = getComponentFromIndirectSwizzle();
            
            sink.writeByte(indirectComponent);
            // The first bit of the source register is to be set to one - BigEndian style of writing the byte.
            sink.writeByte(0x80);
        }
    }
    // The indirect register typically holds a single index value
    // We use the indirect swizzle to obtain the register component that 
    // holds this index value
    private function getComponentFromIndirectSwizzle() : int
    {
        var indirectComp : int = 0;
        if (indirectSwizzle_.getChannel(0) == 0)
            indirectComp = 0;
        else if (indirectSwizzle_.getChannel(1) == 1)
            indirectComp = 1;
        else if (indirectSwizzle_.getChannel(2) == 2)
            indirectComp = 2;
        else if (indirectSwizzle_.getChannel(3) == 3)
            indirectComp = 3;
        return indirectComp;
    }

    nsinternal function get indexOffset() : int
    {
        return indexOffset_;
    }

    nsinternal function set indexOffset(value : int) : void
    {
        indexOffset_ = value;
    }
    
    nsinternal function get indirectRegister() : Register
    {
        return indirectRegisterInfo_;
    }
    
    nsinternal function set indirectRegister(value : Register) : void
    {
        indirectRegisterInfo_ = value;
    }
    
    nsinternal function get indirectSwizzle() : Swizzle
    {
        return indirectSwizzle_;
    }
    
    nsinternal function set indirectSwizzle(value : Swizzle) : void
    {
        Enforcer.checkNull(value);
        
        indirectSwizzle_ = value;
    }
    
    nsinternal function get isDirect() : Boolean
    {
        return isDirect_;
    }

    nsinternal function isMatrix33() : Boolean
    {
        return true; // need to lock this one down, but for now just make it true
    }

    nsinternal function isMatrix44() : Boolean
    {
        return true; // need to lock this one down, but for now just make it true
    }

    nsinternal function isScalar() : Boolean
    {
        return true; // for now, all source registers can act as scalars or vectors
    }

    nsinternal function isVector() : Boolean
    {
        return true; // for now, all source registers can act as scalars or vectors
    }

    nsinternal function get liveRange() : LiveRange
    {
        return liveRange_;
    }

    nsinternal function set liveRange(value : LiveRange) : void
    {
        liveRange_ = value;
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
        if (!indirectRegisterInfo_)
            flags = flags | SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER;
        var base : String = registerInfo_.serialize(indentLevel, flags);
        
        if (indirectRegisterInfo_)
        {
            var indirectRegister : String = indirectRegisterInfo_.serialize(indentLevel, flags | SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER);
            base += "[" + indirectRegister 
            if (indirectSwizzle_.isSwizzled())
            {
                base += ".";
                base += indirectSwizzle_.serialize();                
            }
            if (!SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
            {
                if(registerInfo_.type.isMatrix() || registerInfo_.type.isArray())
                {
                    base +=  " + " + (indexOffset_ - slotUsed_) + "]";
                    base += "[" + slotUsed_ + "]";   
                }
            
                else
                    base +=  " + " + indexOffset_ + "]";
            }
            else // display miniasm form
            {
                base +=  "+" + indexOffset_ + "]";
            }
        }
        
        
        if (swizzle_.isSwizzled())
        {
            base += ".";

            base += swizzle_.serialize();

        }

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_LIVE_RANGE_CROSS_REFERENCES))
        {
            base += "(LR";
            base += (liveRange_ != null) ? liveRange_.id : "?";
            base += ")";
        }

        //This variable stores the current slot in use.
        // Reset it to 0 , since we share the registerInfo object for the slots
        // of a logical register
        //It is set at the beginning of this method.
        registerInfo_.slotUsedBySrcOrDest = 0;
        
        return base;
    }
    
    nsinternal function get slotUsed() : int
    {
        return slotUsed_;
    }

    nsinternal function get swizzle() : Swizzle
    {
        return swizzle_;
    }

    nsinternal function set swizzle(value : Swizzle) : void
    {
        Enforcer.checkNull(value);

        swizzle_ = value.clone();
    }

} // SourceRegister
} // com.adobe.AGALOptimiser.agal
