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
import com.adobe.AGALOptimiser.Semantics;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public class SimpleValue extends Value
{
    private var name_      : String           = undefined;
    private var semantics_ : Semantics        = undefined;
    private var category_  : RegisterCategory = undefined;
    
    public function SimpleValue(id : int,
                                name : String,
                                type : Type,
                                semantics : Semantics,
                                category : RegisterCategory)
    {
        Enforcer.checkNull(name);
        Enforcer.checkNull(category);
        // semantics may be null

        super(id, type);

        Enforcer.check((category.code == RegisterCategory.VARYING) ||
                       (category.code == RegisterCategory.CONSTANT) ||
                       (category.code == RegisterCategory.OUTPUT), ErrorMessages.SIMPLE_VALUE_CATEGORIES);

        name_      = name;
        semantics_ = semantics;
        category_  = category.clone(); // we want this immutable
    }

    nsinternal function get category() : RegisterCategory
    {
        return category_;
    }

    override nsinternal function clone(context : CloningContext) : Value
    {
        if (context.cloningAnOutput && (context.outputs[this] != null))
            return context.outputs[this];
        else if (context.cloningAnInterpolated && (context.interpolateds[this] != null))
            return context.interpolateds[this];
        else if (context.parameters[this] != null)
            return context.parameters[this];
        else
        {
            var cloned       : SimpleValue = undefined;
            var newSemantics : Semantics      = null;
            
            if (semantics_ != null)
                newSemantics = semantics_.clone();
            
            cloned = new SimpleValue(id,  
                                     name_, 
                                     this.type.clone(),
                                     newSemantics,
                                     category_); // category is going to get cloned in the constructor

            if (context.cloningAnOutput)
                context.outputs[this] = cloned;
            else if (context.cloningAnInterpolated)
                context.interpolateds[this] = cloned;
            else
                context.parameters[this] = cloned;
            
            return cloned;
        }
    }

    nsinternal function get name() : String
    {
        return name_;
    }

    nsinternal function get semantics() : Semantics
    {
        return semantics_;
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        if (category_.code == RegisterCategory.CONSTANT)
            s += Constants.PARAMETER_TAG;
        else if (category_.code == RegisterCategory.OUTPUT)
            s += Constants.OUTPUT_TAG;
        else
            s += Constants.INTERPOLATED_TAG;
        
        s += "(";
        s += name_;

        if (semantics_ != null)
        {
            s += " ";
            s += semantics_.serialize(0, flags);
        }

        s += ")";

        return s;
    }

} // ParameterValue
} // com.adobe.AGALOptimiser.agal
