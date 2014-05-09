#include "codegen/nv50_ir_lowering_nvc0.h"

namespace nv50_ir {

class GM107LoweringPass : public NVC0LoweringPass
{
public:
   GM107LoweringPass(Program *p) : NVC0LoweringPass(p) {}
private:
   virtual bool visit(Instruction *);

   virtual bool handleManualTXD(TexInstruction *);
   bool handleDFDX(Instruction *);
   bool handlePFETCH(Instruction *);
   bool handlePOPCNT(Instruction *);
};

} // namespace nv50_ir
