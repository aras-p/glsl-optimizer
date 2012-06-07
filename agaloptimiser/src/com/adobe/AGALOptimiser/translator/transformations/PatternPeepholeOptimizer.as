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

import com.adobe.AGALOptimiser.agal.DestinationRegister;
import com.adobe.AGALOptimiser.agal.Instruction;
import com.adobe.AGALOptimiser.agal.Opcode;
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.Procedure;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.SampleOperation;
import com.adobe.AGALOptimiser.agal.SourceRegister;
import com.adobe.AGALOptimiser.agal.Swizzle;
import com.adobe.AGALOptimiser.agal.WriteMask;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.Type;

use namespace nsinternal;

[ExcludeClass]
public class PatternPeepholeOptimizer extends Transformation
{
    protected static var rules_         : Vector.<TransformationPattern> = null;
    private          var newOperations_ : Vector.<Operation>             = null;
    private          var bindings_      : TemplateBindings               = null;
    
    public function PatternPeepholeOptimizer(p : Program)
    {
        super();

        transform(p);
    }

    private function applyRule(bindings : TemplateBindings) : void
    {
        for (var i : int = 0 ; i < bindings.rule.targetPattern.length ; ++i)
            applyRuleToOperation(bindings.rule.targetPattern[i], bindings);
    }

    private function applyRuleToOperation(instPattern : InstructionTemplate,
                                          bindings : TemplateBindings) : void
    {
        var s          : SourceRegister      = null;
        var src0       : SourceRegister      = null;
        var src1       : SourceRegister      = null;
        var dst        : DestinationRegister = null;
        var bound      : Instruction         = null;

        // first make the source registers
        
        if (instPattern.operand0 != null)
        {
            s = bindings.boundSources[instPattern.operand0.id];
            Enforcer.check(s != null, ErrorMessages.INTERNAL_ERROR);

            src0 = applySwizzleMaskChainToSource(s, instPattern.operand0, bindings);
        }

        if (instPattern.operand1 != null)
        {
            s = bindings.boundSources[instPattern.operand1.id];
            Enforcer.check(s != null, ErrorMessages.INTERNAL_ERROR);

            src1 = applySwizzleMaskChainToSource(s, instPattern.operand1, bindings);
        }
        
        // make the destination // destinations are reused if they use numbers bound to 
        // a pattern.  In general, the last instruction in the pattern will reuse the destination from
        // the original.  That's the thing that disconnects the dead code.

        if (instPattern.destination != null)
        {
            dst = bindings.boundDestinations[instPattern.destination.id];

            if (dst == null)
            {
                // it's a new internal that we're adding to the mix, so let's replace it.
            }
        }

        // put together the instruction
        
        // put it into the stream

        newOperations_.push(new Instruction(instPattern.opcode,
                                            dst,
                                            src0,
                                            src1));
    }

    private function applySwizzleMaskChainToSource(sourceRegister : SourceRegister,
                                                   template : SourceRegisterTemplate,
                                                   bindings : TemplateBindings) : SourceRegister
    {
        var finalSwizzle : Swizzle;

        if (template.maskAndSwizzleChain != null)
        {
            finalSwizzle = bindings.boundSources[template.maskAndSwizzleChain[0]].swizzle.clone();

            var onDestination : Boolean = true;        

            // trace("swizzle is: " + finalSwizzle.serialize());

            for (var i : int = 1 ; i < template.maskAndSwizzleChain.length ; ++i)
            {
                if (onDestination)
                    projectSwizzleThroughMask(finalSwizzle,
                                              template.maskAndSwizzleChain[i],
                                              bindings);
                else
                    projectSwizzleThroughSwizzle(finalSwizzle,
                                                 template.maskAndSwizzleChain[i],
                                                 bindings);

                // trace("swizzle is: " + finalSwizzle.serialize());

                onDestination = !onDestination;
            }
        }
        else
            finalSwizzle = sourceRegister.swizzle.clone();

        return new SourceRegister(sourceRegister.register, 
                                  finalSwizzle);
    }

    private function bindToDestination(inst : Instruction,
                                       destTemplate : DestinationRegisterTemplate) : Boolean
    {
        var boundType : Type = null;

        // we're still in a write-once regime for optimization
        if (bindings_.boundDestinations[destTemplate.id] != null)
            return false;

        if (destTemplate.typeId != -1)
        {
            boundType = bindings_.boundTypes[destTemplate.typeId];

            if (boundType != null)
            { 
                if (!boundType.equals(inst.destination.register.type))
                    return false;
            }
            else
                bindings_.boundTypes[destTemplate.typeId] = inst.destination.register.type;
        }

        if ((destTemplate.allowableTypes != null) && (!destTemplate.allowableTypes.isInSet(inst.destination.register.type)))
            return false;
            
        bindings_.boundDestinations[destTemplate.id] = inst.destination;
        
        return true;
    }
    
    private function bindToOperation(op : Operation,
                                     instTemplate : InstructionTemplate,
                                     rule : TransformationPattern) : Boolean
    {
        var inst : Instruction = op as Instruction;

        if (inst == null)
            return false;

        if (inst.opcode.code != instTemplate.opcode.code)
            return false;

        // check destination register template

        if (inst.destination != null) 
        {
            if (instTemplate.destination == null)
                return false;

            if (!bindToDestination(inst, instTemplate.destination))
                return false;
        }
        else if (instTemplate.destination != null)
            return false;

        // now check sources.  This may result in a recursion

        if (inst.operand0 != null)
        {
            if (instTemplate.operand0 == null)
                return false;

            if (!bindToSource(inst.operand0, inst, instTemplate.operand0, rule))
                return false;
        }
        else if (instTemplate.operand0 != null)
            return false;

        if (inst.operand1 != null)
        {
            if (instTemplate.operand1 == null)
                return false;

            if (!bindToSource(inst.operand1, inst, instTemplate.operand1, rule))
                return false;
        }
        else if (instTemplate.operand1 != null)
            return false;

        // if we get through all the tests, it's a match

        return true;
    }
                                     

    private function bindToRule(op : Operation,
                                rule : TransformationPattern) : TemplateBindings
    {
        var toReturn : TemplateBindings = null;

        bindings_ = new TemplateBindings();
        bindings_.rule = rule;
        
        // we begin with the last one and recur
        if (bindToOperation(op, rule.sourcePattern[rule.sourcePattern.length - 1], rule))
            toReturn = bindings_;

        bindings_ = null;
        return toReturn;
    }

    private function bindToSource(src : SourceRegister,
                                  inst : Instruction,
                                  srcTemplate : SourceRegisterTemplate, 
                                  rule : TransformationPattern) : Boolean
    {
        var defInst     : Instruction;
        var defTemplate : InstructionTemplate;

        if (bindings_.boundSources[srcTemplate.id] != null)
            return true;
        
        if ((src.liveRange == null) || (src.liveRange.definitionLines.length != 1))
            return false;

        defInst = src.definitionLine.definitionSite as Instruction;

        if (defInst == null)
            return false;

        defTemplate = rule.getDefinitionPattern(srcTemplate.id);

        if (defTemplate != null)
            if (!bindToOperation(defInst, defTemplate, rule))
                return false;
        
        bindings_.boundSources[srcTemplate.id] = src;
        
        return true;
    }

    private function cleanState() : void
    {
        newOperations_ = null;
        bindings_      = null;
    }
    
    private function findApplicableRule(op : Operation) : TemplateBindings
    {
        for each (var rule : TransformationPattern in rules_)
        {
            // trace("trying \"" + rule.name + "\" against: " + op.serialize());

            var binding : TemplateBindings = bindToRule(op, rule);
            
            if (binding != null)
            {
                // trace("match found!");
                return binding;
            }
        }
        
        return null;
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_PATTERN_PEEPHOLE_OPTIMIZER;
    }

    private function projectSwizzleThroughMask(swizzle : Swizzle,
                                               destinationId : int,
                                               bindings : TemplateBindings) : void
    {
        var dst : DestinationRegister = bindings.boundDestinations[destinationId];

        Enforcer.check(dst != null, ErrorMessages.INTERNAL_ERROR);

        var firstWritten : int = dst.mask.firstWrittenChannel();

        for (var i : int = 0 ; i < 4 ; ++i)
        {
            if (!dst.mask.getChannel(i))
                swizzle.setChannel(i, firstWritten);
        }
    }

    private function projectSwizzleThroughSwizzle(swizzle: Swizzle,
                                                  sourceId : int,
                                                  bindings : TemplateBindings) : void
    {
        var src : SourceRegister = bindings.boundSources[sourceId];

        Enforcer.check(src != null, ErrorMessages.INTERNAL_ERROR);

        for (var i : int = 0 ; i < 4 ; ++i)
            swizzle.setChannel(i, src.swizzle.getChannel(swizzle.getChannel(i)));
    }
                                               
    private function transform(p : Program) : void
    {
        Enforcer.check(p != null, ErrorMessages.TRANSFORMATION_SCOPE_ERROR);

        try
        {
            numChanges_ = 0;
            p.procedures.forEach(transformProcedure);
        }
        catch (e : Error)
        {
            numChanges_ = 0;
            throw e;
        }
        finally
        {
            cleanState();
        }
    }

    private function transformOperation(op : Operation,
                                        index : int,
                                        operations : Vector.<Operation>) : void
    {
        var bindings : TemplateBindings = findApplicableRule(op);

        if (bindings != null)
            applyRule(bindings);
        else
            newOperations_.push(op);
    }

    private function transformProcedure(proc : Procedure,
                                        index : int,
                                        procedures : Vector.<Procedure>) : void
    {
        newOperations_ = new Vector.<Operation>();

        proc.operations.forEach(transformOperation);

        proc.operations.length = 0;
        for each (var op : Operation in newOperations_)
            proc.operations.push(op);        
    }
    
} // PatternPeepholeOptimzer
} // com.adobe.AGALOptimiser.translator.transformations