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
import com.adobe.AGALOptimiser.Semantics;
import com.adobe.AGALOptimiser.TextureRegisterInfo;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

[ExcludeClass]
public class TextureValue extends Value
{
    private var textureRegisterInfo_ : TextureRegisterInfo = undefined;

    public function TextureValue(id : int, 
                                 sampler : int,
                                 name : String,
                                 type : Type,
                                 semantics : Semantics)
    {
        super(id, type);

        textureRegisterInfo_ = new TextureRegisterInfo(name,
                                                       semantics,
                                                       type.getStage3DTypeFormatString(),
                                                       sampler);
    }

    override nsinternal function clone(context : CloningContext) : Value
    {
        if (context.textures[this] != null)
            return context.textures[this];
        else
        {
            var cloned : TextureValue = new TextureValue(id,
                                                         textureRegisterInfo_.sampler,
                                                         textureRegisterInfo_.name,
                                                         type,
                                                         textureRegisterInfo_.semantics);

            context.textures[this] = cloned;

            return cloned;
        }
    }

    override nsinternal function serialize(indentLevel : int = 0,
                                              flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.TEXTURE_TAG;
        s += "(";
        s += textureRegisterInfo_.name;
        s += " ";
        s += textureRegisterInfo_.sampler;

        if (textureRegisterInfo_.semantics != null)
        {
            s += " ";
            s += textureRegisterInfo_.semantics.serialize(0, flags);
        }

        s += ")";

        return s;
    }

    nsinternal function get textureRegisterInfo() : TextureRegisterInfo
    {
        return textureRegisterInfo_;
    }
    
} // TextureValue
} // com.adobe.AGALOptimiser.agal
