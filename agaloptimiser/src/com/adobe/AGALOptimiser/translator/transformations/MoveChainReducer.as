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
import com.adobe.AGALOptimiser.agal.DestinationRegister;
import com.adobe.AGALOptimiser.agal.Instruction;
import com.adobe.AGALOptimiser.agal.Opcode;
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.Procedure;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.SourceRegister;
import com.adobe.AGALOptimiser.agal.Swizzle;
import com.adobe.AGALOptimiser.agal.WriteMask;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

// REVIEW: do we want to include restrictions on register categories of the moves?
//         do we want to worry about the two constant buffer issue?
//
//         I suspect the right thing to do is to put specific Stage3D happiness transforms into
//         the stream as a final step prior to register allocation / constant vectorization.

// This transformation takes a move chain and reduces it by 1 unit.  Iterations
// of this transformation are needed to remove all spurious moves.  It does the following
//
// mov B, A.s
// op B.s', C
//
// becomes
// 
// mov B, A.s     <-- if this only had one use, it is now dead and will dissapear in the next DCE round
// op A.s->s', C     
//
// At present, it does not reduce mov chains where the interior target has a incomplete
// write mask.  That essentially is changing the "type" of the register.

use namespace nsinternal;

[ExcludeClass]
public final class MoveChainReducer extends Transformation
{
    public function MoveChainReducer(p : Program)
    {
        super();
        transform(p);
    }

    // 0   => neither operand matches pattern
    // +1  => first operand matches pattern
    // +2  => second operand matches pattern
    private function calculateCandidateCode(inst : Instruction) : int
    {
        var candidateCode : int = 0;

        // basically, it will be a candidate if one of its operands is
        // defined solely by one instruction
        // that instruction is a simple move
        // that move writes into all the elements of the register used by the source

        // things without definition lines are things which are from out of context, 
        // e.g. a constant buffer reference.  Anything which cycles through a 
        // temporary will have a def line.

        if ((inst.operand0 != null) && 
            (inst.operand0.isDirect) &&
            (inst.operand0.slotUsed == 0) &&
            (inst.operand0.definitionLine != null) &&
            (inst.operand0.liveRange.definitionLines.length == 1) &&
            (inst.operand0.definitionLine.definitionSite.opcode.code == Opcode.MOVE) && 
            (isSwizzleMaskChainCompatible(inst.operand0, inst.operand0.definitionLine.definitionSite.destination)))
        {
            candidateCode += 1;
        }

        if ((inst.operand1 != null) && 
            (inst.operand1.isDirect) &&
            (inst.operand1.slotUsed == 0) &&
            (inst.operand1.definitionLine != null) && 
            (inst.operand1.liveRange.definitionLines.length == 1) &&
            (inst.operand1.definitionLine.definitionSite.opcode.code == Opcode.MOVE) && 
            (isSwizzleMaskChainCompatible(inst.operand1, inst.operand1.definitionLine.definitionSite.destination)))
        {
            candidateCode += 2;
        }

        return candidateCode;
    }

    // Consider the following two instructions (which may or may not be subsequent):
    // 
    //     mov B.w, A.s
    //     op  C.w', B.s', D.s''    // equally valid if B & D are reversed
    //
    // This function returns true if for each channel in s', it is written in w, i.e.
    // the write mask of the mov "covers" the source register swizzle.

    private function isSwizzleMaskChainCompatible(srcB : SourceRegister,
                                                  dstB : DestinationRegister) : Boolean
    {
        return (dstB.mask.getChannel(srcB.swizzle.getChannel(0)) &&
                dstB.mask.getChannel(srcB.swizzle.getChannel(1)) &&
                dstB.mask.getChannel(srcB.swizzle.getChannel(2)) &&
                dstB.mask.getChannel(srcB.swizzle.getChannel(3)));
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_MOVE_CHAIN_REDUCTION;
    }

    // This function takes the two swizzles on sources B & A and creates a new compound swizzle
    // the plausibility of this has already been established via isSwizzleMaskChainCompatible
    //
    // mov B, A.s
    // op C, B.s', ...
    //
    // s'' = s' <- s
    //
    // where s''[i] = s[s'[i]]
    //
    // for example, if s is wzxy and s' xyyz, then s'' becomes wzzx

    private function projectSwizzle(sB : Swizzle,
                                    sA : Swizzle) : Swizzle
    {
        return new Swizzle(sA.getChannel(sB.getChannel(0)),
                           sA.getChannel(sB.getChannel(1)),
                           sA.getChannel(sB.getChannel(2)),
                           sA.getChannel(sB.getChannel(3)));
    }

    private function transform(p : Program) : void
    {
        numChanges_ = 0;

        Enforcer.check(p != null, ErrorMessages.TRANSFORMATION_SCOPE_ERROR);

        try
        {
            p.procedures.forEach(transformProcedure);
        }
        catch (e : Error)
        {
            numChanges_ = 0;
            throw e;
        }
    }

    private function transformInstruction(inst : Instruction) : void
    {
        // trace(inst.serialize()); 
        
        var candidateCode : int         = calculateCandidateCode(inst);
        var movInst       : Instruction;

        // transformations of the two operands are independent.  They may be referencing the same
        // move or different ones.  The candidateCode tells us which of them (or both) 
        // to transform.  Once transformed, it is likely we've created dead code 
        // with the moves which can be removed via subsequent DCE.
		
		var newOperand:SourceRegister;

        if ((candidateCode == 1) || (candidateCode == 3))
        {
            // first operand is changed
            movInst = inst.operand0.definitionLine.definitionSite as Instruction;

			newOperand = transformSourceRegister(inst.operand0, movInst.operand0);
			
			// bail out if this would result in invalid AGAL
			if(newOperand.register.category.isConstant() && inst.operand1.register.category.isConstant()) {
				return
			}
				
			inst.operand0 = newOperand;
            ++numChanges_;
        }
        else if ((candidateCode == 2) || (candidateCode == 3))
        {
            // second operand is changed
            movInst = inst.operand1.definitionLine.definitionSite as Instruction;

            newOperand = transformSourceRegister(inst.operand1, movInst.operand0);
			
			// bail out if this would result in invalid AGAL
			if(newOperand.register.category.isConstant() && inst.operand1.register.category.isConstant()) {
				return
			}

			inst.operand1 = newOperand;
            ++numChanges_;
        }
    }

    private function transformOperation(op : Operation,
                                        index : int,
                                        operations : Vector.<Operation>) : void
    {
        if (op is Instruction)
            transformInstruction(op as Instruction);
    }

    private function transformProcedure(proc : Procedure,
                                        index : int,
                                        procedures : Vector.<Procedure>) : void
    {
        proc.operations.forEach(transformOperation);
    }

    // For a pattern like this...
    //
    // mov B, A.s
    // op C, B.s', ...
    //
    // the soure register B.s' is replaced with A.s'<-s, s' <- s being the projected swizzle

    private function transformSourceRegister(B : SourceRegister,
                                             A : SourceRegister) : SourceRegister
    {
        return new SourceRegister(A.register, 
                                  projectSwizzle(B.swizzle, A.swizzle),
                                  A.isDirect,
                                  A.slotUsed);
    }
} // MoveChainReduction
} // com.adobe.AGALOptimiser.translator.transformations