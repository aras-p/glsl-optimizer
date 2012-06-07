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

import com.adobe.AGALOptimiser.GlobalID;
import com.adobe.AGALOptimiser.ParameterRegisterInfo;
import com.adobe.AGALOptimiser.RegisterElement;
import com.adobe.AGALOptimiser.TextureRegisterInfo;
import com.adobe.AGALOptimiser.VertexRegisterInfo;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsdebug;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.type.TypeUtils;
import com.adobe.AGALOptimiser.utils.SerializationFlags;
import com.adobe.AGALOptimiser.utils.SerializationUtils;

import flash.utils.ByteArray;
import flash.utils.Dictionary;

use namespace nsinternal;
use namespace nsdebug;

public final class Program
{
    private var category_        : ProgramCategory         = undefined;
    private var version_         : int                     = undefined;
    private var registers_       : Vector.<Register>       = new Vector.<Register>();
    private var procedures_      : Vector.<Procedure>      = new Vector.<Procedure>();
    private var main_            : Procedure               = null;
    private var parameters_      : Vector.<SimpleValue>    = new Vector.<SimpleValue>();
    private var constants_       : Vector.<ConstantValue>  = new Vector.<ConstantValue>();
    private var outputs_         : Vector.<SimpleValue>    = new Vector.<SimpleValue>();
    private var vertexBuffers_   : Vector.<VertexValue>    = new Vector.<VertexValue>();
    private var textures_        : Vector.<TextureValue>   = new Vector.<TextureValue>();
    private var interpolateds_   : Vector.<SimpleValue>    = new Vector.<SimpleValue>();
    private var definitionLines_ : Vector.<DefinitionLine> = null;
    private var liveRanges_      : Vector.<LiveRange>      = null;
	
	
	nsinternal static const   REGISTER                : int = 0;
	nsinternal static const   TEMPORARY               : int = 1;
	nsinternal static const   MEMBER                  : int = 2;
	nsinternal static const   CONST                   : int = 3;
	nsinternal static const   GLOBAL_PARAMETER        : int = 4;
	nsinternal static const   GLOBAL_DEPENDENT        : int = 5;
	nsinternal static const   GLOBAL_INTERNAL         : int = 6;
	nsinternal static const   GLOBAL_INPUT_IMAGE      : int = 7;
	nsinternal static const   GLOBAL_OUTPUT_PIXEL     : int = 8;
	nsinternal static const   GLOBAL_OUTPUT_FRAGMENT  : int = 9;
	nsinternal static const   GLOBAL_INPUT_VERTEX     : int = 10;
	nsinternal static const   GLOBAL_INTERPOLATED     : int = 11;
	nsinternal static const   GLOBAL_OUTPUT_VERTEX    : int = 12;
	nsinternal static const   GLOBAL_OUTPUT_CLIPCOORD : int = 13;
	nsinternal static const   ARGUMENT_IN             : int = 14;
	nsinternal static const   ARGUMENT_OUT            : int = 15;
	nsinternal static const   ARGUMENT_INOUT          : int = 16;
	nsinternal static const   ARGUMENT_CONST          : int = 17;

    public function Program(category : ProgramCategory,
                            version : int = 1)
    {
        Enforcer.checkNull(category);
        Enforcer.check(version == 1, ErrorMessages.INVALID_VALUE);

        category_ = category;
        version_ = version;
    }

    nsinternal function get category() : ProgramCategory
    {
        return category_;
    }

    nsinternal function get constants() : Vector.<ConstantValue> 
    {
        return constants_;
    }

    nsinternal function clone() : Program
    {
        var context : CloningContext = new CloningContext();
        
        var clonedProgram : Program = new Program(category_.clone(),
                                                  version_);

        context.program = clonedProgram;

        for each (var param : SimpleValue in parameters_)
            clonedProgram.parameters.push(param.clone(context) as SimpleValue);

        context.cloningAnOutput = true;
        for each (var output : SimpleValue in outputs_)
            clonedProgram.outputs.push(output.clone(context) as SimpleValue);
        context.cloningAnOutput = false;

        for each (var constVal : ConstantValue in constants_)
            clonedProgram.constants.push(constVal.clone(context) as ConstantValue);

        for each (var vertVal : VertexValue in vertexBuffers_)
            clonedProgram.vertexBuffers.push(vertVal.clone(context) as VertexValue);

        for each (var texVal : TextureValue in textures_)
            clonedProgram.textures.push(texVal.clone(context) as TextureValue);

        context.cloningAnInterpolated = true;
        for each (var interpolated : SimpleValue in interpolateds_)
            clonedProgram.interpolateds.push(interpolated.clone(context) as SimpleValue);
        context.cloningAnInterpolated = false;

        for each (var reg : Register in registers_)
            clonedProgram.registers.push(reg.clone(context));

        for each (var proc : Procedure in procedures_)
        {
            var clonedProc : Procedure  = proc.clone(context);

            clonedProgram.procedures.push(clonedProc);
            
            if (proc == main_)
                clonedProgram.main = clonedProc;
        }

        return clonedProgram;
    }

    nsinternal function createNewRegister(category : RegisterCategory,
                                             type : Type,
                                             slotCount : int) : Register
    {
        var baseNumber : int = 0;

        for (var i : int = 0 ; i < registers_.length ; ++i)
        {
            if (category.code != registers_[i].category.code)
                continue;

            var top : int = registers_[i].baseNumber + registers_[i].slotCount - 1;

            baseNumber = (baseNumber >= top) ? baseNumber : top;
        }

        var newRegister : Register = new Register(category,
                                                  baseNumber + 1,
                                                  type,
                                                  slotCount);

        registers_.push(newRegister);
        
        return newRegister;
    }

    nsinternal function get definitionLines() : Vector.<DefinitionLine>
    {
        return definitionLines_;
    }

    public function generateBinaryBlob(sink : ByteArray) : void
    {
        Enforcer.checkNull(sink);

        sink.length = 0; // clear the sink

        sink.writeByte(0xa0);
        sink.writeInt(version_);
        sink.writeByte(0xa1);

        category_.generateBinaryBlob(sink);

        if (main_ != null)
            main_.generateBinaryBlob(sink);

        for each (var p : Procedure in procedures_)
            if (p != main_)
                p.generateBinaryBlob(sink);
    }
    
    private function getRegNumber(reg: Register, numConst : int, constantRegisterNumbers : Dictionary) : int
    {
        if (reg.offsetInPhysicalRegister == 0 && numConst > -1)
            numConst = numConst + 1;
        // This is for the first register in the set
        if (numConst < 0)
            numConst = 0;
        reg.baseNumber = numConst;
        constantRegisterNumbers[reg] = numConst;
        numConst = numConst + reg.slotCount - 1;
        return numConst;
    }

    nsinternal function get interpolateds() : Vector.<SimpleValue>
    {
        return interpolateds_;
    }

    nsinternal function get liveRanges() : Vector.<LiveRange>
    {
        return liveRanges_;
    }
    
    nsinternal function get main() : Procedure
    {
        return main_;
    }

    nsinternal function set main(main : Procedure) : void
    {
        Enforcer.checkNull(main);

        main_ = main;
    }

    nsinternal function get outputs() : Vector.<SimpleValue>
    {
        return outputs_;
    }

    nsinternal function populateLiveRangeData() : void
    { 
        var lineAnalyzer : DefinitionLineAnalyzer   = new DefinitionLineAnalyzer();
        var liveAnalyzer : LiveRangeAnalyzer        = new LiveRangeAnalyzer();

        setupBlockListsForProcedures();

        definitionLines_ = lineAnalyzer.determineDefinitionLines(main.basicBlocks);
        liveRanges_      = liveAnalyzer.determineLiveRanges(definitionLines_);

        for each (var proc : Procedure in procedures_)
            proc.populateLocationRangeData();
    }

    nsinternal function get parameters() : Vector.<SimpleValue>
    {
        return parameters_;
    }

    nsinternal function get procedures() : Vector.<Procedure>
    {
        return procedures_;
    }

    nsinternal function get registers() : Vector.<Register>
    {
        return registers_;
    }

    public function renumberAndRecoverTemps() : void
    {
        var nextNumber   : int               = 0;
        var usedTemps    : Vector.<Register> = new Vector.<Register>();
        var newRegisters : Vector.<Register> = new Vector.<Register>();
        
        var inspectOperation : Function = function(operation : Operation,
                                                   index : int,
                                                   operations : Vector.<Operation>) : void
        {
            if ((operation.destination != null) && (operation.destination.register.category.code == RegisterCategory.TEMPORARY))
            {
                if (usedTemps.indexOf(operation.destination.register) == -1)
                {
                    usedTemps.push(operation.destination.register);

                    operation.destination.register.baseNumber = nextNumber;
                    nextNumber += operation.destination.register.slotCount;
                }
            }
        }
 
        var inspectProcedure : Function = function(proc : Procedure,
                                                   index : int,
                                                   procedures : Vector.<Procedure>) : void
        {
            proc.operations.forEach(inspectOperation);
        }

        var cullRegisters : Function = function(reg : Register,
                                                index : int,
                                                registers : Vector.<Register>) : void
        {
            if ((reg.category.code == RegisterCategory.TEMPORARY) && (usedTemps.indexOf(reg) == -1))
                return;

            newRegisters.push(reg);
        }

        procedures_.forEach(inspectProcedure);                                    
        registers_.forEach(cullRegisters);

        registers_ = newRegisters;

        sortRegisters();
    }

    public function renumberConstantBuffers() : void
    {
        var constantRegisterNumbers : Dictionary        = new Dictionary();
        var constantRegisters       : Vector.<Register> = new Vector.<Register>();
        var numConst                : int               = -1;
        var reg                     : Register          = undefined;
       
        for each (reg in registers_)
        {
            if (reg.category.code == RegisterCategory.CONSTANT)
                constantRegisters.push(reg);
        }
        constantRegisters.sort(Register.compareRegisters);
       
        //First number the numerical constant registers starting from 0
        for each (reg in constantRegisters)
        {
            if (reg.valueInfo is ConstantValue)
                numConst = getRegNumber(reg, numConst, constantRegisterNumbers);          
        }
        
        //Continue with the program constant registers
        for each (reg in constantRegisters)
        {
            if (reg.valueInfo is SimpleValue)
                numConst = getRegNumber(reg, numConst, constantRegisterNumbers);
        }
        
        for each (reg in registers_)
        {
            if (reg.category.code != RegisterCategory.CONSTANT)
                continue; // nothing to do
            else
            {
                reg.baseNumber = constantRegisterNumbers[reg];
            }          
        }

        sortRegisters();
    }

    nsinternal function serialize(indentLevel : int = 0,
                                     flags : int = 0) : String
    {
        var progString : String = SerializationUtils.indentString(indentLevel);

        if (!SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
        {
            progString += category_.serialize();
            progString += " ";
            progString += Constants.VERSION_TAG;
            progString += "(";
            progString += version_;
            progString += ")\n";
            
            indentLevel += 4;

            var tagString  : String = SerializationUtils.indentString(indentLevel);

            var i : int = undefined;
        
            progString += tagString;
            progString += Constants.REGISTERS_TAG;
            progString += "\n";
            
            for (i = 0 ; i < registers_.length ; ++i)
            {
                progString += registers_[i].serialize(indentLevel + 4, SerializationFlags.DISPLAY_AGAL_CHANNELS | SerializationFlags.DISPLAY_AGAL_REGISTER_INFO |
                        SerializationFlags.DISPLAY_AGAL_REGISTER_NUMBER);
                progString += "\n";
            }
            
            progString += tagString;
            progString += Constants.END_REGISTERS_TAG;
            progString += "\n";
        }

        if (main_ != null)
            progString += main_.serialize(indentLevel, flags);

        for (i = 0 ; i < procedures_.length ; ++i)
            if (procedures_[i] != main_)
                progString += procedures_[i].serialize(indentLevel, flags);

        if (!SerializationFlags.isSet(flags, SerializationFlags.DISPLAY_AGAL_MINIASM_FORM))
        {
            progString += Constants.END_TAG_PREFIX;
            progString += category_.serialize();
            progString += "\n";
            
            indentLevel -= 4;
        }

        return progString;
    }

    nsinternal function setupBlockListsForProcedures() : void
    {
        var analyzer : BasicBlockAnalyzer = new BasicBlockAnalyzer();

        analyzer.determineBlockStructure(this);
    }

    nsinternal function sortRegisters() : void
    {
        registers_ = registers_.sort(Register.compareRegisters);
    }
    
    private function adjustSwizzleForOffset(register : Register, operand : SourceRegister, startOffset : int) : void
    {
        // the temp registers have already had their swizzles adjusted in the register allocator
        if (register.category.code != RegisterCategory.TEMPORARY)
        {
            operand.swizzle = new Swizzle(operand.swizzle.getChannel(0) + startOffset,
                operand.swizzle.getChannel(1) + startOffset,
                operand.swizzle.getChannel(2) + startOffset,
                operand.swizzle.getChannel(3) + startOffset);
        }
        
    }
    private function swizzleFromMask(mask : WriteMask, operand : SourceRegister) : void
    {       
        var register            : Register = null;
        var startOffset         : int      = 0;
        var indirectRegister    : Register = null;
        var indirectStartOffset : int      = 0;
        var j                   : int      = 0;
        var k                   : int      = 0;
        var swiz                : Array    = new Array([0,0,0,0]);
        
        register = operand.register;
        startOffset = register.offsetInPhysicalRegister;
          
        if(register.type.isScalar())
        {
            adjustSwizzleForOffset(register, operand, startOffset);   
        }
        // for non scalar registers - adjust swizzle based on the write mask.
        else
        {
            if (mask.channelCount() != 1) // does not include ops like dp3,dp4
            {
                if (register.category.code != RegisterCategory.TEMPORARY)
                {
                    for (j = 0; j < 4; j++)
                    {
                        if (mask.getChannel(j))
                        {
                            swiz[j] = operand.swizzle.getChannel(k) + startOffset;
                            k++;
                        }
                        else
                            swiz[j] = operand.swizzle.getChannel(j) + startOffset;
                    }
                    operand.swizzle = new Swizzle(swiz[0], swiz[1], swiz[2], swiz[3]);
                }
                // if it is a temp register - swizzle already adjusted in the allocator
                else
                {
                    for (j = 0; j < 4; j++)
                    {
                        if (mask.getChannel(j))
                        {
                            swiz[j] = operand.swizzle.getChannel(k);
                            k++;
                        }
                        else
                            swiz[j] = operand.swizzle.getChannel(j);
                    }
                    operand.swizzle = new Swizzle(swiz[0], swiz[1], swiz[2], swiz[3]);
                }
               
            }//end_if_channelcount != 1
            // if the write mask is a single channel, source is multiple channels like dp3,dp4
            // swizzle doesnt need to take the mask into account.
            else
            {
                // if it is a temp register - swizzle already adjusted in the allocator
                adjustSwizzleForOffset(register, operand, startOffset);
            }
        }//end_else_non_scalar_registers
            
        if (operand.indirectRegister)
        {
            indirectRegister = operand.indirectRegister;
            indirectStartOffset = indirectRegister.offsetInPhysicalRegister;
            adjustSwizzleForOffset(indirectRegister, operand, indirectStartOffset);   
        }                
    }
    
    nsinternal function swizzleOnSourceRegisters() : void
    {
        for (var i : int = 0 ; i < procedures_[0].operations.length ; ++i)
        {
            var opAsInst      : Instruction = procedures_[0].operations[i] as Instruction;
            var operand0      : SourceRegister = null;
            var operand1      : SourceRegister = null;
            var dest          : DestinationRegister = null;           
            var mask          : WriteMask = null;
            var isSameOperand : Boolean = false
            
            if (opAsInst != null)
            {
                operand0 = opAsInst.operand0;
                operand1 = opAsInst.operand1;
                dest     = opAsInst.destination;
            }
            if (dest != null)
                mask = dest.mask;
                    
            if (operand0 != null && dest!= null)
            {
                if (operand1 != null && (operand0.swizzle == operand1.swizzle))
                    isSameOperand = true;
                swizzleFromMask(mask, operand0);
            }
            if (operand1 != null && dest!= null && !isSameOperand)
            {
                swizzleFromMask(mask, operand1);                
            }
        }
    }

    nsinternal function get temporaryRegisterCount() : int
    {
        var count : int = 0;

        for each (var r : Register in registers_)
        {
            // this must account for matrix registers as well
            if (r.category.code == RegisterCategory.TEMPORARY)
                count = count + r.slotCount;
        }

        return count;
    }

    nsinternal function get textures() : Vector.<TextureValue>
    {
        return textures_;
    }

    nsinternal function get vertexBuffers() : Vector.<VertexValue>
    {
        return vertexBuffers_;
    }
    
} // Program
} // com.adobe.AGALOptimiser.agal

