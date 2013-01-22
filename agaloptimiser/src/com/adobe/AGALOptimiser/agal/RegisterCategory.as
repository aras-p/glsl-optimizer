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

public final class RegisterCategory
{
    internal      static const FIRST_CATEGORY  : int = 0;

    nsinternal static const VERTEXATTRIBUTE : int = 0;
    nsinternal static const CONSTANT        : int = 1;
    nsinternal static const TEMPORARY       : int = 2;
    nsinternal static const OUTPUT          : int = 3;
    nsinternal static const VARYING         : int = 4;
    nsinternal static const SAMPLER         : int = 5;

    internal      static const LAST_CATEGORY   : int = 5;

    private var category_        : int             = undefined;
    private var programCategory_ : ProgramCategory = undefined;

    public function RegisterCategory(type : int,
                                     programType : ProgramCategory)
    {
        Enforcer.checkNull(programType);
        Enforcer.checkEnumValue((type >= FIRST_CATEGORY) && (type <= LAST_CATEGORY));

        category_ = type;
        programCategory_ = new ProgramCategory(programType.code);

        if (programCategory_.code == ProgramCategory.FRAGMENT)
            Enforcer.check(type != VERTEXATTRIBUTE, ErrorMessages.REGISTER_RESTRICTION_VIOLATION);

        if (programCategory_.code == ProgramCategory.VERTEX)
            Enforcer.check(type != SAMPLER, ErrorMessages.REGISTER_RESTRICTION_VIOLATION);
    }

    internal function clone() : RegisterCategory
    {
        return new RegisterCategory(category_,
                                    programCategory_.clone());
    }

    nsinternal function get code() : int
    {
        return category_;
    }

    nsinternal function generateBinaryBlob(sink : ByteArray) : void
    {
        Enforcer.checkNull(sink);

        sink.writeByte(category_);
    }

    nsinternal function isSink() : Boolean
    {
        return ((category_ == OUTPUT) || 
                ((category_ == VARYING) && (programCategory_.code == ProgramCategory.VERTEX)));
    }

    nsinternal function isSource() : Boolean
    {
        return ((category_ == CONSTANT) ||
                (category_ == SAMPLER) ||
                (category_ == VERTEXATTRIBUTE) ||
                ((category_ == VARYING) && (programCategory_.code == ProgramCategory.FRAGMENT)))
    }

    nsinternal function get programCategory() : ProgramCategory
    {
        return programCategory_;
    }
	
	nsinternal function isConstant():Boolean{
		return code == CONSTANT;
	}

    nsinternal static function registerCategoryToString(rt : RegisterCategory,
                                                           flags : int) : String
    {
        if (rt.programCategory.code == ProgramCategory.FRAGMENT)
        {
            switch (rt.code)
            {
            case CONSTANT:         return "fc";
            case TEMPORARY:        return "ft";
            case OUTPUT:           
                if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
                    return "oc";
                else
                    return "fo";

            case VARYING:
                if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
                    return "v";
                else
                    return "i";
                
            case SAMPLER:          return "fs";

            default:
                throw new Error(ErrorMessages.INTERNAL_ERROR);
            }
        }
        else // vertex
        {
            switch (rt.code)
            {
            case CONSTANT:         return "vc";
            case TEMPORARY:        return "vt";
            case OUTPUT:           
                if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
                    return "op";
                else
                    return "vo";

            case VERTEXATTRIBUTE:  return "va";
            case VARYING:
                if (SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
                    return "v";
                else
                    return "i";

            default:
                throw new Error(ErrorMessages.INTERNAL_ERROR);
            }
        }

        return null;
    }

} // RegisterCategory
} // com.adobe.AGALOptimiser.agal
