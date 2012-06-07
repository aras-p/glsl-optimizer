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
 * Contains the name, semantics, format and map index of an 
 * input vertex value used in a source program.
 */
    
public final class VertexRegisterInfo
    extends RegisterInfo
{
    private var mapIndex_ : int;

    /**
     * @param name The name of the input vertex variable
     * @param semantics The semantics of the variable
     * @param format A string representation corresponding to the Stage3D format type 
     * @param mapIndex The number of the vertex attribute register this will be stored in
     * based on the type of value contained in the variable
     */
    
    /** @private */
    public function VertexRegisterInfo(name : String,
                                       semantics : Semantics,
                                       format : String,
                                       mapIndex : int )
    {
        super( name, semantics, format );
        mapIndex_ = mapIndex;
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
    nsinternal function clone() : VertexRegisterInfo
    {
        var result : VertexRegisterInfo = new VertexRegisterInfo( name,
                                       semantics,
                                       format,
                                       mapIndex_ );

        return result;
    }

    /**
     * The index into the vertex atribute register map where the data is stored.
     */
    public function get mapIndex() : int
    {
        return mapIndex_;
    }

    /** @private */
    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var s : String = SerializationUtils.indentString(indentLevel);

        s += Constants.VERTEX_REGISTER_INFO_TAG;
        s += globalID.serialize();

        return s;
    }

} // VertexRegisterInfo
} // com.adobe.AGALOptimiser

