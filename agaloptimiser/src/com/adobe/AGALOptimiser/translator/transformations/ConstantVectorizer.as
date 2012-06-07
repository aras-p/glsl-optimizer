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

package com.adobe.AGALOptimiser.translator.transformations {

import com.adobe.AGALOptimiser.agal.ConstantValue;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.Register;
import com.adobe.AGALOptimiser.agal.RegisterCategory;
import com.adobe.AGALOptimiser.agal.SimpleValue;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.ScalarType;
import com.adobe.AGALOptimiser.type.TypeUtils;
import com.adobe.AGALOptimiser.type.VectorType;

use namespace nsinternal;

[ExcludeClass]
public class ConstantVectorizer extends Transformation
{
    private var numericalConstantRegisters_ : Vector.<Register> = new Vector.<Register>();
    private var programConstantRegisters_   : Vector.<Register> = new Vector.<Register>();
    private var otherRegisters_             : Vector.<Register> = new Vector.<Register>();
    
    public function ConstantVectorizer(p : Program)
    { 
        super();

        transform(p);
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_CONSTANT_VECTORIZER;
    }

    private function packRegisters(constantRegisters : Vector.<Register>) : void
    {
        var nextRegIndexToPack : int = 0;
        var registerIndexToPack     : int = 0;
        
        for (var currentRegisterIndex : int = 0; currentRegisterIndex < constantRegisters.length; currentRegisterIndex++)
        {
            var vectorType : VectorType = null;
            var scalarType : ScalarType = null;
            var regDim     : int       = 0;
            
            regDim = constantRegisters[currentRegisterIndex].type.channelCount();  
            // if 3 channel - dont do anything
            if (regDim == 3 && nextRegIndexToPack == 0)
                nextRegIndexToPack = currentRegisterIndex; 
            // if 2 channel - pack with other two channel registers
            // Logical registers will have same base number but different offsets
            else if (regDim == 2)
            {
                for( registerIndexToPack = currentRegisterIndex + 1 ; registerIndexToPack < constantRegisters.length; registerIndexToPack++)
                {                   
                    regDim = constantRegisters[registerIndexToPack].type.channelCount();
                    
                    if (regDim == 2 && constantRegisters[currentRegisterIndex].channelsPacked == 2)
                    {                                                                
                        ++numChanges_;
                        constantRegisters[registerIndexToPack].baseNumber = constantRegisters[currentRegisterIndex].baseNumber;
                        constantRegisters[registerIndexToPack].offsetInPhysicalRegister = constantRegisters[currentRegisterIndex].channelsPacked;
                        constantRegisters[currentRegisterIndex].channelsPacked += 2;
                        constantRegisters[registerIndexToPack].channelsPacked = constantRegisters[currentRegisterIndex].channelsPacked;
                        break;
                    }
                }
            }
            // If 1 channel , look first for 3 , then 2 , then other 1 channel registers
            else if (regDim == 1)
            {
                for( registerIndexToPack = nextRegIndexToPack ; registerIndexToPack < constantRegisters.length; registerIndexToPack++)
                {
                    regDim = constantRegisters[registerIndexToPack].type.channelCount();
                    
                    if (regDim == 3 && constantRegisters[registerIndexToPack].channelsPacked < 4)
                    {
                        packChannelsIntoRegister(constantRegisters, registerIndexToPack, currentRegisterIndex);
                        // if a register is packed, change the next register index to start packing
                        nextRegIndexToPack = (constantRegisters[registerIndexToPack].channelsPacked == 4)? registerIndexToPack + 1  : nextRegIndexToPack;
                        break;
                    }
                    else if (regDim == 2 && constantRegisters[registerIndexToPack].channelsPacked < 4)
                    {
                        packChannelsIntoRegister(constantRegisters, registerIndexToPack, currentRegisterIndex);
                        // if a register is packed, reset the next register index to start packing
                        //Here we increment by 2 to account for a two 2 element registers appended with a 1 element register
                        nextRegIndexToPack = (constantRegisters[registerIndexToPack].channelsPacked == 4)? registerIndexToPack + 2 : nextRegIndexToPack;
                        break;
                    }
                    else if (regDim == 1 && constantRegisters[registerIndexToPack].channelsPacked < 4 && registerIndexToPack < currentRegisterIndex )
                    {
                        packChannelsIntoRegister(constantRegisters, registerIndexToPack, currentRegisterIndex);
                        // if a register is packed, reset the next register index to start packing
                        //Here we increment by 3 to account for a two 2 element registers appended with a 1 element register
                        nextRegIndexToPack = (constantRegisters[registerIndexToPack].channelsPacked == 4)? registerIndexToPack + 3 : nextRegIndexToPack;
                        break;
                    }
                }// end_for_finish packing_current_register
            }//end_packing_regDim 1
        }//end_for_pack_all_registers
        
    }
    
    private function packChannelsIntoRegister( constantRegisters : Vector.<Register>, registerIndexToPack : int, currentRegisterIndex : int) : void
    {
        constantRegisters[currentRegisterIndex].baseNumber = constantRegisters[registerIndexToPack].baseNumber;
        constantRegisters[currentRegisterIndex].offsetInPhysicalRegister = constantRegisters[registerIndexToPack].channelsPacked;
        constantRegisters[registerIndexToPack].channelsPacked += 1;
        constantRegisters[currentRegisterIndex].channelsPacked = constantRegisters[registerIndexToPack].channelsPacked;     
        ++numChanges_;
    }

    private function transform(p : Program) : void
    {
        try
        {
            var newRegisters : Vector.<Register> = vectorize(p.registers);

            p.registers.length = 0;
            for each (var newReg : Register in newRegisters)
                p.registers.push(newReg);

            p.renumberConstantBuffers(); 
            p.swizzleOnSourceRegisters();
        }
        catch (e : Error)
        {
            numChanges_ = 0;
            throw e;
        }
        finally
        {
            numericalConstantRegisters_.length = 0;
            programConstantRegisters_.length   = 0;
            otherRegisters_.length             = 0;
        }
    }
    
    private function vectorize(registers : Vector.<Register>) : Vector.<Register>
    {
        
        // consider matrix type, vector type, scalar type
        // If matrix , check 4x4 ,3x3 - do nothing for now
        // If vector check 3 - do nothing
        // if vector is 2 - try to pack with another 2 channel
        // if scalar , first try to pack in 3, then 2 , then with another scalar
                
        var r : Register = null;
        
        // Separate the registers into programConst, numericalConst, otherRegisters
        for each (r in registers)
        {
            if (r.category.code == RegisterCategory.CONSTANT)
            {
                var constantInfo : ConstantValue = r.valueInfo as ConstantValue;
                var paramInfo    : SimpleValue   = r.valueInfo as SimpleValue;
                
                if ((constantInfo == null) && ((paramInfo == null) || (paramInfo.category.code != RegisterCategory.CONSTANT)))
                    throw new Error(ErrorMessages.EXTERNAL_VALUES_NEED_INFO);

                if (r.type.isMatrix() || r.type.isArray())
                    otherRegisters_.push(r);                   
                else
                {
                    if (r.valueInfo is ConstantValue)
                        numericalConstantRegisters_.push(r);
                    else 
                        programConstantRegisters_.push(r);
                }
            }
            else
                otherRegisters_.push(r);                   
        }
       
        // Sort the registers based on the dimension in descending order
        var constantCategorySort : Function = function(a : Register,
                                                       b : Register) : Number
        {
            Enforcer.checkType(a.type.isVector() || a.type.isScalarFloat(), ErrorMessages.SORT_ON_VECTOR_FLOAT_ONLY);
            Enforcer.checkType(b.type.isVector() || b.type.isScalarFloat(), ErrorMessages.SORT_ON_VECTOR_FLOAT_ONLY);
            
            var vectorType : VectorType = null;
            var scalarType : ScalarType = null;
            var dimA       : int        = 0;
            var dimB       : int        = 0;
            
            dimA = a.type.channelCount();
            dimB = b.type.channelCount();
                   
            if (dimA < dimB)
                return 1;
            else if (dimA > dimB)
                return -1;
            else
                return 0;
        }
        
        programConstantRegisters_.sort(constantCategorySort);
        numericalConstantRegisters_.sort(constantCategorySort);
       
        // Pack the program and numerical constants separately
        packRegisters(programConstantRegisters_);
        packRegisters(numericalConstantRegisters_);
         
        // Push the packed registers into the registerSet to be returned
        var registerSet : Vector.<Register> = new Vector.<Register>
        for each (r in programConstantRegisters_)
            registerSet.push(r);

        for each (r in numericalConstantRegisters_)
            registerSet.push(r);

        for each (r in otherRegisters_)
            registerSet.push(r);

        return registerSet;
    }
        
} // ConstantVectorizer
} // com.adobe.AGALOptimiser.translator.transformations
