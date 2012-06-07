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

import com.adobe.AGALOptimiser.agal.SimpleValue;
import com.adobe.AGALOptimiser.agal.Register;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.utils.SerializationUtils;
import com.adobe.AGALOptimiser.type.MatrixType;

use namespace nsinternal;
use namespace nsdebug;

/**
 * Contains the name, semantics, format and registers of a 
 * parameter used in a source program.
 */

public final class ParameterRegisterInfo
    extends RegisterInfo
{
    private var register_ : Register;

    /** @private */
    public function ParameterRegisterInfo(
        register : Register)
    {
        Enforcer.checkNull(register);

        // REVIEW: should we just override the class above this?
        super(register.name, 
              register.semantics,
              register.type.getStage3DTypeFormatString());

        register_ = register;
    }

    /** @private */
    nsinternal function get register() : Register
    {
        return register_;
    }

    /**
     * The id of this parameter - name, semantics and format
     */
    public function get globalID() : GlobalID
    {
        var result : GlobalID = new GlobalID( 
            register_.name, 
            register_.semantics,
            register_.type.getStage3DTypeFormatString());

        return result;
    }

    /**
     * The register elements that contain the data for this parameter.
     */
    public function get elementGroup() : RegisterElementGroup
    {
        var matrixDim  : int       = 0;
        var matrixType : MatrixType = register.type as MatrixType;

        if( matrixType )
        {
            matrixDim = matrixType.colDimension;
        }
        
        var re : RegisterElement = new RegisterElement(
            register.baseNumber,
            register.offsetInPhysicalRegister );

        var res : Vector.<RegisterElement> = new Vector.<RegisterElement>();

        var s : String = register.serialize();

        for( var i : int = 0; i < register.type.channelCount(); ++i )
        {
            res.push( re );

            re = re.add( 1 );
            
            if( matrixDim == 3 && i % 3 == 2 )
            {
                re = re.add( 1 );
            }
        }

        return new RegisterElementGroup(
            res );
    }
    
    /** @private */
    nsdebug function serialize(
        indentLevel : int = 0,
        flags : int = 0) : String
    {
        var indentString  : String   = SerializationUtils.indentString(indentLevel);
        var result     : String   = indentString;

        result += Constants.PARAMETER_TAG;
        result += globalID.serialize();
        result += " ";
        result += elementGroup.serialize();
        
        return result;
    }
    
} // ParameterRegisterInfo
} // com.adobe.AGALOptimiser

