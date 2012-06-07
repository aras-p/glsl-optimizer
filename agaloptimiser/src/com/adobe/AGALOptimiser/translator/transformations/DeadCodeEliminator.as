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
import com.adobe.AGALOptimiser.agal.Operation;
import com.adobe.AGALOptimiser.agal.Procedure;
import com.adobe.AGALOptimiser.agal.Program;
import com.adobe.AGALOptimiser.agal.RegisterCategory;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;

// this class removes dead code from the program.  It assumes that information 
// in the definition lines is accurate.  Basically, it will remove code that
//
//     1. Has a destination register with definition line
//     2. Has a destination register which is temporary
//     3. Has a definition line with 0 uses

use namespace nsinternal;

[ExcludeClass]
public final class DeadCodeEliminator extends Transformation
{
    public function DeadCodeEliminator(p : Program)
    {
        super();

        transform(p);
    }

    nsinternal static function get name() : String
    {
        return Constants.TRANSFORMATION_DEAD_CODE_ELIMINATION;
    }

    private function transform(p : Program) : void
    {
        numChanges_ = 0;

        Enforcer.check(p != null, ErrorMessages.TRANSFORMATION_SCOPE_ERROR);

        try
        {
            for each (var proc : Procedure in p.procedures)
                transformProcedure(proc);

            p.renumberAndRecoverTemps();
        }
        catch (e : Error)
        {
            numChanges_ = 0;
            throw e;
        }
    }

    private function transformProcedure(p : Procedure) : void
    {
        var newOps : Vector.<Operation> = new Vector.<Operation>();

        for each (var op : Operation in p.operations)
        {
            if ((op.destination != null) && (op.destination.definitionLine != null) && (op.destination.register.category.code == RegisterCategory.TEMPORARY) && (op.destination.definitionLine.useSites.length == 0))
            {
                ++numChanges_;
                continue;
            }

            newOps.push(op);
        }

        // now copy over the operations which made the cut
        p.operations.length = 0;

        for each (var newOp : Operation in newOps)
            p.operations.push(newOp);
    }
    
} // DeadCodeEliminator
} // com.adobe.AGALOptimiser.translator.transformations