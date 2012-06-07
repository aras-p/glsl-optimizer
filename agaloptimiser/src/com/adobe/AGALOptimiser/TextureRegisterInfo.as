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

package com.adobe.AGALOptimiser {

import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

use namespace nsinternal;

/**
 * Contains the name, semantics, format and sampler unit number of a 
 * texture used in a source program.
 */
public final class TextureRegisterInfo
    extends RegisterInfo
{
    private var sampler_      : int;
    
    /**
     * Constructor to construct a TextureInfo object
     * @param name The name of the texture variable.
     * @param semantics The semantics of the texture variable.
     * @param registerNumber The number of the sampler unit the texture will be stored in.
     */    

    /** @private */
    public function TextureRegisterInfo(
        name : String,
        semantics : Semantics,
        format : String,
        sampler : int)
    {
        super( name, semantics, format );
        sampler_ = sampler;
    }

    /** @private */
    nsinternal function clone() : TextureRegisterInfo
    {
        return new TextureRegisterInfo( name,
                                        semantics,
                                        format,
                                        sampler_ );
    }

    /**
     * The number of the sampler unit the texture is stored in.
     */
    public function get sampler() : int
    {
        return sampler_;
    }

    /**
     * The id of this parameter - name, semantics and format
     */
    public function get globalID() : GlobalID
    {
        var result : GlobalID = new GlobalID( 
            name, 
            semantics,
            format);

        return result;
    }

    /** @private */
    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.TEXTURE_REGISTER_INFO_TAG;
        s += globalID.serialize();
        s += " ";
        s += sampler_;
        
        return s;
    }

} // TextureRegisterInfo
} // com.adobe.AGALOptimiser
