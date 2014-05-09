/*
 * Copyright 2011 Christoph Bumiller
 *           2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "codegen/nv50_ir_target_gm107.h"
#include "codegen/nv50_ir_lowering_gm107.h"

namespace nv50_ir {

Target *getTargetGM107(unsigned int chipset)
{
   return new TargetGM107(chipset);
}

// BULTINS / LIBRARY FUNCTIONS:

// lazyness -> will just hardcode everything for the time being

#include "lib/gm107.asm.h"

void
TargetGM107::getBuiltinCode(const uint32_t **code, uint32_t *size) const
{
   *code = (const uint32_t *)&gm107_builtin_code[0];
   *size = sizeof(gm107_builtin_code);
}

uint32_t
TargetGM107::getBuiltinOffset(int builtin) const
{
   assert(builtin < NVC0_BUILTIN_COUNT);
   return gm107_builtin_offsets[builtin];
}

bool
TargetGM107::isOpSupported(operation op, DataType ty) const
{
   switch (op) {
   case OP_MAD:
   case OP_FMA:
      if (ty != TYPE_F32)
         return false;
      break;
   case OP_SAD:
   case OP_POW:
   case OP_SQRT:
   case OP_DIV:
   case OP_MOD:
      return false;
   default:
      break;
   }

   return true;
}

bool
TargetGM107::runLegalizePass(Program *prog, CGStage stage) const
{
   if (stage == CG_STAGE_PRE_SSA) {
      GM107LoweringPass pass(prog);
      return pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_POST_RA) {
      NVC0LegalizePostRA pass(prog);
      return pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_SSA) {
      NVC0LegalizeSSA pass;
      return pass.run(prog, false, true);
   }
   return false;
}

CodeEmitter *
TargetGM107::getCodeEmitter(Program::Type type)
{
   return createCodeEmitterGM107(type);
}

} // namespace nv50_ir
