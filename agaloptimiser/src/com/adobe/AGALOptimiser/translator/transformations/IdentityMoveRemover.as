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

import com.adobe.AGALOptimiser.agal.Instruction;
import com.adobe.AGALOptimiser.agal.Opcode;
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.Procedure;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

// this is a simple transformation.  It looks for identity moves and removes
// them.  An identity move is one defined as having a move from a register onto itself
// where the mask and the sizzle "match".  A match is defined as any combination
// which results in no change.  For example, these qualify:
//
//     mov ft0, ft0
//     mov ft0.xy, ft0
//     mov ft0.xyz, ft0.xyzz
//     mov ft0.zw, ft0.zzzw
//
// these do not
// 
//     mov ft0, ft0.xyzz    << w will be altered
//     mov ft0.y, ft0.xxzw  << because y becomes x, which is non-identity

use namespace nsinternal;

[ExcludeClass]
public class IdentityMoveRemover extends Transformation
{
    private var newOperations_ : Vector.<Operation> = null;

    public function IdentityMoveRemover(p : Program)
    {
        super();
        transform(p);
    }

    private function cleanState() : void
    {
        newOperations_ = null;
    }

    private function isCandidate(inst : Instruction) : Boolean
    {
        if ((inst == null) || (inst.opcode.code != Opcode.MOVE))
            return false;

        if ((inst.destination.register.baseNumber != inst.operand0.register.baseNumber) ||
            (inst.destination.register.category.code != inst.operand0.register.category.code) ||
            (inst.destination.slotUsed != inst.operand0.slotUsed) ||
            (!inst.operand0.isDirect))
            return false;

        // ok, we're looking at a simple move, now it's up to whether or not the swizzles
        // map appropriately
        
        var isCandidate : Boolean = true;

        isCandidate = isCandidate && ((!inst.destination.mask.getChannel(0)) || (inst.operand0.swizzle.getChannel(0) == 0));
        isCandidate = isCandidate && ((!inst.destination.mask.getChannel(1)) || (inst.operand0.swizzle.getChannel(1) == 1));
        isCandidate = isCandidate && ((!inst.destination.mask.getChannel(2)) || (inst.operand0.swizzle.getChannel(2) == 2));
        isCandidate = isCandidate && ((!inst.destination.mask.getChannel(3)) || (inst.operand0.swizzle.getChannel(3) == 3));

        return isCandidate;
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_IDENTITY_MOVE_REMOVER;
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
        finally
        {
            cleanState();
        }
    }

    private function transformOperation(op : Operation,
                                        index : int,
                                        operations : Vector.<Operation>) : void
    {
        if (!isCandidate(op as Instruction))
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

    
    
} // IdentityMoveRemover
} // com.adobe.AGALOptimiser.translator.transformations