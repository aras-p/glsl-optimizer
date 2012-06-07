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
import com.adobe.AGALOptimiser.Semantics;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.utils.SerializationUtils;
import com.adobe.AGALOptimiser.VertexRegisterInfo;


use namespace nsinternal;

[ExcludeClass]
public class VertexValue extends Value
{
    private var vertexRegisterInfo_ : VertexRegisterInfo = undefined;

    public function VertexValue(id : int,
                                mapIndex : int,
                                name : String,
                                type : Type,
                                semantics : Semantics)
    {
        super(id, type);

        vertexRegisterInfo_ = new VertexRegisterInfo(name,
                                                     semantics,
                                                     type.getStage3DTypeFormatString(),
                                                     mapIndex);
    }

    override nsinternal function clone(context : CloningContext) : Value
    {
        if (context.vertexBuffers[this] != null)
            return context.vertexBuffers[this];
        else
        {
            var cloned : VertexValue = new VertexValue(id, 
                                                       vertexRegisterInfo_.mapIndex,
                                                       vertexRegisterInfo_.name,
                                                       type,
                                                       vertexRegisterInfo_.semantics);

            context.vertexBuffers[this] = cloned;
            
            return cloned;
        }
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.VERTEX_TAG;
        s += "(";
        s += vertexRegisterInfo_.name;
        s += " ";
        s += vertexRegisterInfo_.mapIndex;

        if (vertexRegisterInfo_.semantics != null)
        {
            s += " ";
            s += vertexRegisterInfo_.semantics.serialize(0, flags);
        }

        s += ")";

        return s;
    }
    
    nsinternal function get vertexRegisterInfo() : VertexRegisterInfo
    {
        return vertexRegisterInfo_;
    }

} // VertexValue
} // com.adobe.AGALOptimiser.agal
