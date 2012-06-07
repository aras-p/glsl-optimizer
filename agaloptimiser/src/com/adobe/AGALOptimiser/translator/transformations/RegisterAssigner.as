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

import com.adobe.AGALOptimiser.agal.DefinitionLine;
import com.adobe.AGALOptimiser.agal.DefinitionLineAnalyzer;
import com.adobe.AGALOptimiser.agal.DestinationRegister;
import com.adobe.AGALOptimiser.agal.Instruction;
import com.adobe.AGALOptimiser.agal.LiveRange;
import com.adobe.AGALOptimiser.agal.LiveRangeAnalyzer;
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.Procedure;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.Register;
import com.adobe.AGALOptimiser.agal.RegisterCategory;
import com.adobe.AGALOptimiser.agal.SampleOperation;
import com.adobe.AGALOptimiser.agal.SourceRegister;
import com.adobe.AGALOptimiser.agal.Swizzle;
import com.adobe.AGALOptimiser.agal.WriteMask;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.MatrixType;
import com.adobe.AGALOptimiser.type.ScalarType;
import com.adobe.AGALOptimiser.type.VectorType;
import com.adobe.AGALOptimiser.utils.SerializationFlags;

import flash.utils.Dictionary;

use namespace nsinternal;

public final class RegisterAssigner extends Transformation
{
    private var assignment_          : Dictionary                     = null;
    private var canonicalRegisters_  : Dictionary                     = null;
    private var nodesAssigned_       : Vector.<InterferenceGraphNode> = null;
    private var nodesUnassigned_     : Vector.<InterferenceGraphNode> = null;
    private var newTempRegisters_    : Vector.<Register>              = null;
    private var destVisited_         : Dictionary                     = null;
    private var srcVisited_          : Dictionary                     = null;
    private var agalProgram_         : Program                        = null;

    public function RegisterAssigner(p : Program)
    {
        super();
        assignRegisters(p);
    }

    private function  assignRegisters(p : Program) : void
    {
        Enforcer.check(p != null, ErrorMessages.TRANSFORMATION_SCOPE_ERROR);

        try
        {
            numChanges_ = 0;

            var interferences      : InterferenceGraph          = undefined;
            var successfulColoring : Boolean                    = undefined;
            var assignment         : Dictionary                 = undefined;
            var graphGenerator     : InterferenceGraphGenerator = new InterferenceGraphGenerator();
            var colorer            : InterferenceGraphColorer   = new InterferenceGraphColorer();

            agalProgram_ = p;
            
            p.populateLiveRangeData();

            p.setupBlockListsForProcedures();

            // ultimately will need to do this for mulitple procedures, but right now all we have is main
            interferences      = graphGenerator.buildInterferenceGraph(p.main.basicBlocks, p.liveRanges);
            successfulColoring = colorer.colorGraph(interferences);

            Enforcer.check(successfulColoring, ErrorMessages.INTERNAL_ERROR);

            canonicalRegisters_ = new Dictionary();
            newTempRegisters_   = new Vector.<Register>();
            destVisited_        = new Dictionary();
            srcVisited_         = new Dictionary();

            setupVisitationFlags();

            calculateRegisterAssignment(interferences);

            for each (var node : InterferenceGraphNode in interferences.nodes)
            {
                if (node.liveRange.register.category.code != RegisterCategory.TEMPORARY)
                    continue;

                var assignedRegister : Register = assignment_[node.liveRange];
                Enforcer.check(assignedRegister != null, ErrorMessages.INTERNAL_ERROR);
                Enforcer.check(node.liveRange.register.offsetInPhysicalRegister == 0, ErrorMessages.INTERNAL_ERROR);
                
                renameAndAdjust(node.liveRange.register.baseNumber,
                                assignedRegister);
            }

            updateFinalRegisterTypes();
            updateFinalRegisterList();

            p.renumberConstantBuffers();
            numChanges_ = 1;
        }
        catch (e : Error)
        {
            numChanges_ = 0;
            throw e;
        }
        finally
        {
            assignment_         = null;
            nodesAssigned_      = null;
            nodesUnassigned_    = null;
            newTempRegisters_   = null;
            canonicalRegisters_ = null;
            destVisited_        = null;
            srcVisited_         = null;
            agalProgram_        = null;
        }
    }

    private function calculateRegisterAssignment(interferences : InterferenceGraph) : void
    {
        assignment_ = new Dictionary();

        nodesAssigned_   = new Vector.<InterferenceGraphNode>(interferences.nodes.length); // reserve the space
        nodesUnassigned_ = new Vector.<InterferenceGraphNode>(interferences.nodes.length);
        
        nodesAssigned_.length = 0;
        nodesUnassigned_.length = 0;
        
        for (var i : int = 0 ; i < interferences.nodes.length ; ++i)
            if (interferences.nodes[i].liveRange.register.category.code == RegisterCategory.TEMPORARY)
                nodesUnassigned_.push(interferences.nodes[i]);
          
        
        simpleRegisterAssignment();
    }

    private function findOpening(node : InterferenceGraphNode) : Register
    {
        var newMatrixRegister      : Register = null;
        var sanityCheckRegCount    : int      = agalProgram_.temporaryRegisterCount;

        if (node.liveRange.register.type.isMatrix())
        {
            // this loop will terminate since the allocator can't take more than the original
            // number of registers in the program.  We'll throw an exception if it tries to 
            // go to a higher number than the number of temps in the original program.  
            
            for (var j : int = 0 ; true ; ++j)
            {
                var matrixStart : int = undefined;
                Enforcer.check(j < sanityCheckRegCount, ErrorMessages.INTERNAL_ERROR);
                matrixStart = findRegisterRangeForMatrix(node);

                if (matrixStart != -1)
                {             
                    newMatrixRegister = new Register(node.liveRange.register.category,
                                                     matrixStart,
                                                     node.liveRange.register.type,
                                                     node.liveRange.register.slotCount,
                                                     node.liveRange.register.semanticCrossReference,
                                                     0); //matrix registers currently have no offsets in physical registers
                    return newMatrixRegister;
                }
            }
        }    
        else
        {
            var newRegister      : Register = null;
            var sanityCheckCount : int      = agalProgram_.temporaryRegisterCount;

            // this loop will terminate since the allocator can't take more than the original
            // number of registers in the program.  We'll throw an exception if it tries to 
            // go to a higher number than the number of temps in the original program.  

            for (var i : int = 0 ;; ++i)
            {
                var offset : int = undefined;

                Enforcer.check(i < sanityCheckCount, ErrorMessages.INTERNAL_ERROR);

                offset = findOpeningInRegister(node, i);

                if (offset != -1)
                {
                    newRegister = new Register(node.liveRange.register.category,
                                               i,
                                               node.liveRange.register.type,
                                               node.liveRange.register.slotCount,
                                               node.liveRange.register.semanticCrossReference,
                                               offset);
                    return newRegister;
                }
            }
        }

        return null;  // this path isn't reachable
    }

    private function findOpeningInRegister(node : InterferenceGraphNode, 
                                           registerNumber : int) : int
    {
        var channelCount    : int       = node.liveRange.register.type.channelCount();
        var accumulatedMask : WriteMask = new WriteMask(false, false, false, false);

        for each (var edge : InterferenceGraphEdge in node.edges)
        {
            var target         : InterferenceGraphNode = (edge.source == node) ? edge.target : edge.source;
            var targetRegister : Register              = assignment_[target.liveRange];

            if ((targetRegister != null) && (targetRegister.baseNumber <= registerNumber) && (targetRegister.baseNumber + targetRegister.slotCount > registerNumber))
                accumulatedMask.accumulate(targetRegister.naturalMask());
        }

        for (var i : int = 0 ; i < (5 - channelCount) ; ++i)
            if (accumulatedMask.isSubrangeEmpty(i, channelCount))
                return i;

        return -1;
    }

    private function findRegisterRangeForMatrix(node : InterferenceGraphNode) : int
    {
        var absoluteMax : int              = agalProgram_.temporaryRegisterCount; 
        var touched     : Vector.<Boolean> = new Vector.<Boolean>(absoluteMax);

        for each (var edge : InterferenceGraphEdge in node.edges)
        {
            var target         : InterferenceGraphNode = (edge.source == node) ? edge.target : edge.source;
            var targetRegister : Register              = assignment_[target.liveRange];

            if (targetRegister != null) 
            {
                var targetChannels : int = targetRegister.type.channelCount();

                if (targetChannels <= 4)
                {
                    // REVIEW: 2x2 matrices we put in two registers
                    if (targetRegister.type.isMatrix())
                    {
                        touched[targetRegister.baseNumber] = true;
                        touched[targetRegister.baseNumber + 1] = true;
                    }
                    else
                        touched[targetRegister.baseNumber] = true;
                }
                else if (targetChannels == 9)
                {
                    touched[targetRegister.baseNumber]     = true;
                    touched[targetRegister.baseNumber + 1] = true;
                    touched[targetRegister.baseNumber + 2] = true;
                }
                else if (targetChannels == 16)
                {
                    touched[targetRegister.baseNumber]     = true;
                    touched[targetRegister.baseNumber + 1] = true;
                    touched[targetRegister.baseNumber + 2] = true;
                    touched[targetRegister.baseNumber + 3] = true;
                }
                else 
                    throw new Error(ErrorMessages.INTERNAL_ERROR);
            }
        }

        // ok, we know where we can't look, now lets look for some openings

        var registersNeeded : int = node.liveRange.register.slotCount;

        for (var i : int = 0 ; i < absoluteMax - registersNeeded ; ++i)
        {
            var rangeFound : Boolean = true;
            for (var j : int = i ; j < absoluteMax ; ++j)
            {
                if (touched[i])
                {
                    rangeFound = false;
                    break;
                }
            }

            if (rangeFound)
                return i;
        }

        return -1;
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_REGISTER_ASSIGNER;
    }

    private function renameAndAdjust(registerNumberToChange : int,
                                     assignedRegister : Register) : void
    {
        var actual : Register = null;

        var inspectOperation : Function = function(operation : Operation,
                                                   index : int,
                                                   vector : Vector.<Operation>) : void
        {
            if (operation.destination != null)
                renameAndAdjustDestinationRegister(operation.destination,
                                                   registerNumberToChange, 
                                                   assignedRegister,
                                                   actual);

            if (operation is Instruction)
            {
                var inst : Instruction = operation as Instruction;
                
                if (inst.operand0 != null)
                    renameAndAdjustSourceRegister(inst.operand0,
                                                  registerNumberToChange,
                                                  assignedRegister,
                                                  actual);

                if (inst.operand1 != null)
                    renameAndAdjustSourceRegister(inst.operand1,
                                                  registerNumberToChange,
                                                  assignedRegister,
                                                  actual);
            }
            else if (operation is SampleOperation)
            {
                var samp : SampleOperation = operation as SampleOperation;
                
                if (samp.operand0 != null)
                    renameAndAdjustSourceRegister(samp.operand0,
                                                  registerNumberToChange,
                                                  assignedRegister,
                                                  actual);
            }
            else
            {
                throw new Error(ErrorMessages.INTERNAL_ERROR);
            }
        }

        var inspectProcedure : Function = function(proc : Procedure,
                                                   index : int,
                                                   vector : Vector.<Procedure>) : void
        {
            proc.operations.forEach(inspectOperation);
        }

        for each (var reg : Register in newTempRegisters_)
            if (reg.baseNumber == assignedRegister.baseNumber)
                actual = reg;

        agalProgram_.procedures.forEach(inspectProcedure);
    }

    private function renameAndAdjustDestinationRegister(destination: DestinationRegister,
                                                        oldNumber : int,
                                                        assignedRegisterTemplate : Register,
                                                        actualAssignedRegister : Register) : void
    {
        if ((destVisited_[destination]) || (destination.register.baseNumber != oldNumber) || (destination.register.category.code != assignedRegisterTemplate.category.code))
            return;

        destVisited_[destination] = true;

        var newOffset    : int = assignedRegisterTemplate.offsetInPhysicalRegister;
        var channelCount : int = destination.register.type.channelCount();
        
        Enforcer.check((newOffset + destination.mask.channelCount()) <= 4, ErrorMessages.OUT_OF_RANGE);
        Enforcer.check(channelCount == assignedRegisterTemplate.type.channelCount(), ErrorMessages.CHANNEL_COUNT_MISMATCH);

        destination.register = actualAssignedRegister;

        destination.mask.shift(newOffset);
    }

    private function renameAndAdjustSourceRegister(source : SourceRegister,
                                                   oldNumber : int,
                                                   assignedRegisterTemplate : Register,
                                                   actualAssignedRegister : Register) : void
    {
        if (!source.isDirect)
        {
            if ((srcVisited_[source]) || (source.indirectRegister.baseNumber != oldNumber) || (source.indirectRegister.category.code != assignedRegisterTemplate.category.code))
                return; 
            
            srcVisited_[source] = true;
            
            Enforcer.check(source.indirectRegister.type.channelCount() == assignedRegisterTemplate.type.channelCount(), ErrorMessages.TYPE_MISMATCH);
            
            source.indirectRegister = actualAssignedRegister;
            
            var indirectSwizzleOffset : int = assignedRegisterTemplate.offsetInPhysicalRegister;
            
            source.indirectRegister.offsetInPhysicalRegister = indirectSwizzleOffset;
            source.indirectSwizzle = new Swizzle(source.indirectSwizzle.getChannel(0) + indirectSwizzleOffset,
                                                          source.indirectSwizzle.getChannel(1) + indirectSwizzleOffset,
                                                          source.indirectSwizzle.getChannel(2) + indirectSwizzleOffset,
                                                          source.indirectSwizzle.getChannel(3) + indirectSwizzleOffset);
        }
        else
        {
            if ((srcVisited_[source]) || (source.register.baseNumber != oldNumber) || (source.register.category.code != assignedRegisterTemplate.category.code))
                return;

            srcVisited_[source] = true;

            Enforcer.check(source.register.type.channelCount() == assignedRegisterTemplate.type.channelCount(), ErrorMessages.TYPE_MISMATCH);

            source.register = actualAssignedRegister;

            var swizzleOffset : int = assignedRegisterTemplate.offsetInPhysicalRegister;

            source.swizzle = new Swizzle(source.swizzle.getChannel(0) + swizzleOffset,
                                         source.swizzle.getChannel(1) + swizzleOffset,
                                         source.swizzle.getChannel(2) + swizzleOffset,
                                         source.swizzle.getChannel(3) + swizzleOffset);
        }
    }

    private function setupVisitationFlags() : void
    {
        var actual : Register = null;

        var inspectOperation : Function = function(operation : Operation,
                                                   index : int,
                                                   vector : Vector.<Operation>) : void
        {
            if (operation.destination != null)
                destVisited_[operation.destination] = false;

            if (operation is Instruction)
            {
                var inst : Instruction = operation as Instruction;
                
                if (inst.operand0 != null)
                    srcVisited_[inst.operand0] = false;

                if (inst.operand1 != null)
                    srcVisited_[inst.operand1] = false;
            }
            else if (operation is SampleOperation)
            {
                var samp : SampleOperation = operation as SampleOperation;
                
                if (samp.operand0 != null)
                    srcVisited_[samp.operand0] = false;
            }
            else 
                throw new Error(ErrorMessages.INTERNAL_ERROR);
        }

        var inspectProcedure : Function = function(proc : Procedure,
                                                   index : int,
                                                   vector : Vector.<Procedure>) : void
        {
            proc.operations.forEach(inspectOperation);
        }

        agalProgram_.procedures.forEach(inspectProcedure);
    }

    private function simpleRegisterAssignment() : void
    {
        while (nodesUnassigned_.length > 0)
        {
            var node        : InterferenceGraphNode = nodesUnassigned_.shift();
            var newRegister : Register              = findOpening(node);
  
            Enforcer.check(newRegister != null, ErrorMessages.INTERNAL_ERROR);

            assignment_[node.liveRange] = newRegister;

            if (canonicalRegisters_[newRegister.baseNumber] == null)
            {
                canonicalRegisters_[newRegister.baseNumber] = newRegister;
                newTempRegisters_.push(newRegister);
            }

            // what are we going to do if there is no room?

            nodesAssigned_.push(node);
        }
    }


    private function updateFinalRegisterList() : void
    {
        var finalRegisters : Vector.<Register> = new Vector.<Register>;
        var reg            : Register          = undefined;

        for each (reg in agalProgram_.registers)
            if (reg.category.code != RegisterCategory.TEMPORARY)
                finalRegisters.push(reg);

        finalRegisters = finalRegisters.concat(newTempRegisters_);

        agalProgram_.registers.length = 0;

        for each (reg in finalRegisters)
            agalProgram_.registers.push(reg);
    }

    private function updateFinalRegisterTypes() : void
    {
        var finalMasks : Dictionary = new Dictionary;

        var inspectOperation : Function = function(op : Operation,
                                                   index : int,
                                                   vector : Vector.<Operation>) : void
        {
            if (op.destination != null)
            {
                var mask : WriteMask = finalMasks[op.destination.register];

                if (mask == null)
                    finalMasks[op.destination.register] = op.destination.mask.clone();
                else
                    finalMasks[op.destination.register].accumulate(op.destination.mask);
            }
        }

        var inspectProcedure : Function = function(proc : Procedure,
                                                   index : int,
                                                   vector : Vector.<Procedure>) : void
        {
            proc.operations.forEach(inspectOperation);
        }

        agalProgram_.procedures.forEach(inspectProcedure);

        for each (var reg : Register in newTempRegisters_)
        {
            reg.offsetInPhysicalRegister = 0; // all the renaming is done, so things are normalized

            var channelCount : int = 0
            
            if (reg.slotCount == 1)
                channelCount = finalMasks[reg] ? finalMasks[reg].channelCount() : 4;
            else
                channelCount = reg.type.channelCount();

            if (channelCount == 1)
                reg.type = new ScalarType(ScalarType.FLOAT_TYPE);
            else if (channelCount <= 4) 
            {
                if (reg.slotCount > 1)
                    reg.type = new MatrixType(2, 2);
                else               
                    reg.type = new VectorType(new ScalarType(ScalarType.FLOAT_TYPE),
                                          channelCount);
            }
            else if (channelCount == 9)
                reg.type = new MatrixType(3, 3);
            else if (channelCount == 16)
                reg.type = new MatrixType(4, 4);
            else
                throw new Error("Unexpected type dimension");
        }
    }
    
} // RegisterAssigner
} // com.adobe.AGALOptimiser.translator.transformations