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

import com.adobe.AGALOptimiser.Semantics;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.ArrayType;
import com.adobe.AGALOptimiser.type.MatrixType;
import com.adobe.AGALOptimiser.type.ScalarType;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.type.TypeUtils;
import com.adobe.AGALOptimiser.type.VectorType;
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

public class Register
{
    private var category_                : RegisterCategory       = undefined;
    private var baseNumber_              : int                    = undefined;
    private var type_                    : Type                   = undefined;
    private var slotCount_               : int                    = undefined;
    private var offsetInPhysical_        : int                    = undefined;
    private var slotUsedBySrcOrDest_     : int                    = 0;          // used by serialize() to write out the current slot used in the register
    private var semanticsCrossReference_ : Semantics              = null;
    private var channelsPacked_          : int                    = 0;
    private var valueInfo_               : Value                  = null;
  
    public function Register(category : RegisterCategory,
                             baseNumber : int,
                             type : Type = null,
                             slotCount : int = 1,
                             semanticsCrossReference : Semantics = null,
                             offsetInPhysical : int = 0,
                             isDirect : Boolean = true ) // Right now, isDirect is useful in the sourceRegister, but in the Register obj
                                                         // it is used for only serialization purposes
    {
        Enforcer.checkNull(category);
        if (!type.isArray())
            Enforcer.check((slotCount > 0) &&(slotCount <= 4),
                          ErrorMessages.ALLOWABLE_SLOT_RANGE);
        
        category_ = new RegisterCategory(category.code, category.programCategory);

        Enforcer.check((category.code != RegisterCategory.OUTPUT) || (baseNumber == 0), 
                       ErrorMessages.OUTPUT_REGISTER_UNIQUENESS);

        baseNumber_ = baseNumber;
        type_       = type;

        if (type != null)
            channelsPacked_ = TypeUtils.dimCount(type);

        slotCount_               = slotCount;
        semanticsCrossReference_ = semanticsCrossReference;
        offsetInPhysical_        = offsetInPhysical;
    }

    nsinternal function get baseNumber() : int
    {
        return baseNumber_;
    }
    
    nsinternal function set baseNumber(value : int) : void
    {
        baseNumber_ = value;
    }
    
    nsinternal function get category() : RegisterCategory
    {
        return category_;
    }

    nsinternal function get channelsPacked() : int
    {
        return channelsPacked_;
    }
    
    nsinternal function set channelsPacked(value : int) : void
    {
        channelsPacked_ = value;
    }

    internal function clone(context : CloningContext) : Register
    {
        var clonedRegister : Register = new Register(category_.clone(),
                                                     baseNumber_,
                                                     (type_ != null) ? type_.clone() : null,
                                                     slotCount_,
                                                     (semanticsCrossReference_ != null) ? semanticsCrossReference_.clone() : null,
                                                     offsetInPhysical_);
                                                     

        context.registerMap[this] = clonedRegister;
        
        if (valueInfo_ != null)
            clonedRegister.valueInfo = valueInfo_.clone(context);
        
        return clonedRegister;
    }

    internal static function compareRegisters(a : Register,
                                              b : Register) : Number
    {
        if (a.category.code < b.category.code)
            return -1;
        else if (a.category.code > b.category.code)
            return 1;
        else
        {
            if (a.baseNumber < b.baseNumber)
                return -1;
            else if (a.baseNumber > b.baseNumber)
                return 1;
            else
                return a.offsetInPhysicalRegister - b.offsetInPhysicalRegister;
        }
    }

    private function finalChannelIndex() : int
    {
       
        var lastOpenChannel : int = offsetInPhysical_ + type_.channelCount();

        // adjust for matrices and arrays
        if (slotCount_ > 1)
        {
            Enforcer.check(offsetInPhysical_ == 0, ErrorMessages.INTERNAL_ERROR);

            if (type.isMatrix())
            {
                if ((type_ as MatrixType).rowDimension == 4)
                    lastOpenChannel = 3;
                else if ((type_ as MatrixType).rowDimension == 3)
                    lastOpenChannel = 2;
                else
                    lastOpenChannel = 3;
            }
            else if (type_.isArray())
            {
                if ((type as ArrayType).elementType.isMatrix())
                {
                    var matrix : MatrixType = (type as ArrayType).elementType as MatrixType;
                    // Change this based on how we plan to implement 2x2 matrices
                    lastOpenChannel = matrix.rowDimension;
                }            
                else
                    lastOpenChannel = (type as ArrayType).elementType.channelCount();
            }          
        }
        return lastOpenChannel;
    }
 
    nsinternal function isMaskCompatible(mask : WriteMask) : Boolean
    {
        var lastOpenChannel : int = finalChannelIndex();

        for (var i : int = 0 ; i < 4 ; ++i)
            if (mask.getChannel(i) && ((i < offsetInPhysical_) || (i > lastOpenChannel)))
                return false;

        return true;
    }

    nsinternal function isSwizzleCompatible(swizzle : Swizzle) : Boolean
    {
        var lastOpenChannel : int = finalChannelIndex();

        for (var i : int = 0 ; i < 4 ; ++i)
            if ((swizzle.getChannel(i) < offsetInPhysical_) || (swizzle.getChannel(i) > lastOpenChannel))
                return false;

        return true;
    }
       
    nsinternal function get name() : String
    {
        if (valueInfo_ is SimpleValue)
            return ((valueInfo_ as SimpleValue).name);
        else if (valueInfo_ is TextureValue)
            return ((valueInfo_ as TextureValue).textureRegisterInfo.name);
        else if (valueInfo_ is VertexValue)
            return ((valueInfo_ as VertexValue).vertexRegisterInfo.name);
        else
            return null;
    }

    nsinternal function naturalMask() : WriteMask
    {
        var channelCount           : int = TypeUtils.dimCount(type_);
        // For 2x2 matrix - reserve the z component - as some operations on the
        // matrix columns are done as dp3 with the third component being zero
        // for 3x3 matrix - reserve the w component. as m33 opcode requires its swizzle
        // to be xyzw.
        if (type_.isMatrix())
        {
            if((type_ as MatrixType).rowDimension <= 3)
                channelCount += 1;
        }
        var offsetPlusChannelCount : int = channelCount + offsetInPhysical_;
       
        return new WriteMask((offsetInPhysical_ <= 0) && (0 < offsetPlusChannelCount),
                             (offsetInPhysical_ <= 1) && (1 < offsetPlusChannelCount),
                             (offsetInPhysical_ <= 2) && (2 < offsetPlusChannelCount),
                             (offsetInPhysical_ <= 3) && (3 < offsetPlusChannelCount));
    }

    nsinternal function naturalSwizzle() : Swizzle
    {
        return Swizzle.swizzleFromType(type_);
    }

    nsinternal function get offsetInPhysicalRegister() : int
    {
        return offsetInPhysical_;
    }

    nsinternal function set offsetInPhysicalRegister(value : int) : void
    {
        offsetInPhysical_ = value;
    }

    nsinternal function get semanticCrossReference() : Semantics
    {
        return semanticsCrossReference_;
    }

    nsinternal function get semantics() : Semantics
    {
        if (valueInfo_ is SimpleValue)
            return ((valueInfo_ as SimpleValue).semantics);
        else if (valueInfo_ is TextureValue)
            return ((valueInfo_ as TextureValue).textureRegisterInfo.semantics);
        else if (valueInfo_ is VertexValue)
            return ((valueInfo_ as VertexValue).vertexRegisterInfo.semantics);
        else
            return null;
    }

    nsinternal function set slotUsedBySrcOrDest(slotUsed : int) : void
    {
        slotUsedBySrcOrDest_ = slotUsed;
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);
        var r : String = "";
        var i : int;

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_REGISTER_INFO))
        {
            r += type.serialize();
            r += " ";
        }

        r += RegisterCategory.registerCategoryToString(category_, flags);

        if (category_.code != RegisterCategory.OUTPUT)
        {
            if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER))
                r += (baseNumber_ + slotUsedBySrcOrDest_);            
        }
       
        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_CHANNELS))
        {
            var channelCount : int  = type.channelCount();

            if (channelCount < 4)
            {
                var channelNames : Array = ['x', 'y', 'z', 'w'];
                r += ".";
                for (i = offsetInPhysical_ ; i <= (offsetInPhysical_ + channelCount - 1); ++i)
                    r += channelNames[i];
            }
        }

        s += r;

        if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_REGISTER_INFO))
        {
            var skipAhead : int = 20 - r.length;

            if (skipAhead > 0)
                s += SerializationUtils.indentString(skipAhead);

            if (valueInfo_ != null)
            {
                s += " ";
                s += valueInfo_.serialize();
            }
        }
        return s;
    }

    nsinternal function get slotCount() : int
    {
        return slotCount_;
    }

    nsinternal function get type() : Type
    {
        return type_;
    }
    
    nsinternal function set type(value : Type) : void
    {
        Enforcer.checkNull(value);

        type_ = value;
    }

    nsinternal function get valueInfo() : Value
    {
        return valueInfo_;
    }

    nsinternal function set valueInfo(value : Value) : void
    {
        valueInfo_ = value;
    }

} // Register
} // com.adobe.AGALOptimiser.agal
