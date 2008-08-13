#include "tgsitollvm.h"

#include "gallivm.h"
#include "gallivm_p.h"

#include "storage.h"
#include "instructions.h"
#include "storagesoa.h"
#include "instructionssoa.h"

#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_dump.h"


#include <llvm/Module.h>
#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/ModuleProvider.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/ParameterAttributes.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Utils/Cloning.h>


#include <sstream>
#include <fstream>
#include <iostream>

using namespace llvm;

static inline FunctionType *vertexShaderFunctionType()
{
   //Function takes three arguments,
   // the calling code has to make sure the types it will
   // pass are castable to the following:
   // [4 x <4 x float>] inputs,
   // [4 x <4 x float>] output,
   // [4 x [4 x float]] consts,
   // [4 x <4 x float>] temps

   std::vector<const Type*> funcArgs;
   VectorType *vectorType = VectorType::get(Type::FloatTy, 4);
   ArrayType *vectorArray = ArrayType::get(vectorType, 4);
   PointerType *vectorArrayPtr = PointerType::get(vectorArray, 0);

   ArrayType   *floatArray     = ArrayType::get(Type::FloatTy, 4);
   ArrayType   *constsArray    = ArrayType::get(floatArray, 4);
   PointerType *constsArrayPtr = PointerType::get(constsArray, 0);

   funcArgs.push_back(vectorArrayPtr);//inputs
   funcArgs.push_back(vectorArrayPtr);//output
   funcArgs.push_back(constsArrayPtr);//consts
   funcArgs.push_back(vectorArrayPtr);//temps

   FunctionType *functionType = FunctionType::get(
      /*Result=*/Type::VoidTy,
      /*Params=*/funcArgs,
      /*isVarArg=*/false);

   return functionType;
}

static inline void
add_interpolator(struct gallivm_ir *ir,
                 struct gallivm_interpolate *interp)
{
   ir->interpolators[ir->num_interp] = *interp;
   ++ir->num_interp;
}

static void
translate_declaration(struct gallivm_ir *prog,
                      llvm::Module *module,
                      Storage *storage,
                      struct tgsi_full_declaration *decl,
                      struct tgsi_full_declaration *fd)
{
   if (decl->Declaration.File == TGSI_FILE_INPUT) {
      unsigned first, last, mask;
      uint interp_method;

      first = decl->DeclarationRange.First;
      last = decl->DeclarationRange.Last;
      mask = decl->Declaration.UsageMask;

      /* Do not touch WPOS.xy */
      if (first == 0) {
         mask &= ~TGSI_WRITEMASK_XY;
         if (mask == TGSI_WRITEMASK_NONE) {
            first++;
            if (first > last) {
               return;
            }
         }
      }

      interp_method = decl->Declaration.Interpolate;

      if (mask == TGSI_WRITEMASK_XYZW) {
         unsigned i, j;

         for (i = first; i <= last; i++) {
            for (j = 0; j < NUM_CHANNELS; j++) {
               //interp( mach, i, j );
               struct gallivm_interpolate interp;
               interp.type = interp_method;
               interp.attrib = i;
               interp.chan = j;
               add_interpolator(prog, &interp);
            }
         }
      } else {
         unsigned i, j;
         for( j = 0; j < NUM_CHANNELS; j++ ) {
            if( mask & (1 << j) ) {
               for( i = first; i <= last; i++ ) {
                  struct gallivm_interpolate interp;
                  interp.type = interp_method;
                  interp.attrib = i;
                  interp.chan = j;
                  add_interpolator(prog, &interp);
               }
            }
         }
      }
   }
}

static void
translate_declarationir(struct gallivm_ir *,
                      llvm::Module *,
                      StorageSoa *storage,
                      struct tgsi_full_declaration *decl,
                      struct tgsi_full_declaration *)
{
   if (decl->Declaration.File == TGSI_FILE_ADDRESS) {
      int idx = decl->DeclarationRange.First;
      storage->addAddress(idx);
   }
}

static void
translate_immediate(Storage *storage,
                    struct tgsi_full_immediate *imm)
{
   float vec[4];
   int i;
   for (i = 0; i < imm->Immediate.Size - 1; ++i) {
      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         vec[i] = imm->u.ImmediateFloat32[i].Float;
         break;
      default:
         assert(0);
      }
   }
   storage->addImmediate(vec);
}


static void
translate_immediateir(StorageSoa *storage,
                      struct tgsi_full_immediate *imm)
{
   float vec[4];
   int i;
   for (i = 0; i < imm->Immediate.Size - 1; ++i) {
      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         vec[i] = imm->u.ImmediateFloat32[i].Float;
         break;
      default:
         assert(0);
      }
   }
   storage->addImmediate(vec);
}

static inline int
swizzleInt(struct tgsi_full_src_register *src)
{
   int swizzle = 0;
   int start = 1000;

   for (int k = 0; k < 4; ++k) {
      swizzle += tgsi_util_get_full_src_register_extswizzle(src, k) * start;
      start /= 10;
   }
   return swizzle;
}

static inline llvm::Value *
swizzleVector(llvm::Value *val, struct tgsi_full_src_register *src,
              Storage *storage)
{
   int swizzle = swizzleInt(src);

   if (gallivm_is_swizzle(swizzle)) {
      /*fprintf(stderr, "XXXXXXXX swizzle = %d\n", swizzle);*/
      val = storage->shuffleVector(val, swizzle);
   }
   return val;
}

static void
translate_instruction(llvm::Module *module,
                      Storage *storage,
                      Instructions *instr,
                      struct tgsi_full_instruction *inst,
                      struct tgsi_full_instruction *fi,
                      unsigned instno)
{
   llvm::Value *inputs[4];
   inputs[0] = 0;
   inputs[1] = 0;
   inputs[2] = 0;
   inputs[3] = 0;

   for (int i = 0; i < inst->Instruction.NumSrcRegs; ++i) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      llvm::Value *val = 0;
      llvm::Value *indIdx = 0;

      if (src->SrcRegister.Indirect) {
         indIdx = storage->addrElement(src->SrcRegisterInd.Index);
         indIdx = storage->extractIndex(indIdx);
      }
      if (src->SrcRegister.File == TGSI_FILE_CONSTANT) {
         val = storage->constElement(src->SrcRegister.Index, indIdx);
      } else if (src->SrcRegister.File == TGSI_FILE_INPUT) {
         val = storage->inputElement(src->SrcRegister.Index, indIdx);
      } else if (src->SrcRegister.File == TGSI_FILE_TEMPORARY) {
         val = storage->tempElement(src->SrcRegister.Index);
      } else if (src->SrcRegister.File == TGSI_FILE_OUTPUT) {
         val = storage->outputElement(src->SrcRegister.Index, indIdx);
      } else if (src->SrcRegister.File == TGSI_FILE_IMMEDIATE) {
         val = storage->immediateElement(src->SrcRegister.Index);
      } else {
         fprintf(stderr, "ERROR: not supported llvm source %d\n", src->SrcRegister.File);
         return;
      }

      inputs[i] = swizzleVector(val, src, storage);
   }

   /*if (inputs[0])
     instr->printVector(inputs[0]);
     if (inputs[1])
     instr->printVector(inputs[1]);*/
   llvm::Value *out = 0;
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL: {
      out = instr->arl(inputs[0]);
   }
      break;
   case TGSI_OPCODE_MOV: {
      out = inputs[0];
   }
      break;
   case TGSI_OPCODE_LIT: {
      out = instr->lit(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RCP: {
      out = instr->rcp(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RSQ: {
      out = instr->rsq(inputs[0]);
   }
      break;
   case TGSI_OPCODE_EXP:
      break;
   case TGSI_OPCODE_LOG:
      break;
   case TGSI_OPCODE_MUL: {
      out = instr->mul(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_ADD: {
      out = instr->add(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP3: {
      out = instr->dp3(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP4: {
      out = instr->dp4(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DST: {
      out = instr->dst(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MIN: {
      out = instr->min(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MAX: {
      out = instr->max(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SLT: {
      out = instr->slt(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SGE: {
      out = instr->sge(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MAD: {
      out = instr->madd(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_SUB: {
      out = instr->sub(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_LERP: {
      out = instr->lerp(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_CND:
      break;
   case TGSI_OPCODE_CND0:
      break;
   case TGSI_OPCODE_DOT2ADD:
      break;
   case TGSI_OPCODE_INDEX:
      break;
   case TGSI_OPCODE_NEGATE:
      break;
   case TGSI_OPCODE_FRAC: {
      out = instr->frc(inputs[0]);
   }
      break;
   case TGSI_OPCODE_CLAMP:
      break;
   case TGSI_OPCODE_FLOOR: {
      out = instr->floor(inputs[0]);
   }
      break;
   case TGSI_OPCODE_ROUND:
      break;
   case TGSI_OPCODE_EXPBASE2: {
      out = instr->ex2(inputs[0]);
   }
      break;
   case TGSI_OPCODE_LOGBASE2: {
      out = instr->lg2(inputs[0]);
   }
      break;
   case TGSI_OPCODE_POWER: {
      out = instr->pow(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_CROSSPRODUCT: {
      out = instr->cross(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MULTIPLYMATRIX:
      break;
   case TGSI_OPCODE_ABS: {
      out = instr->abs(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RCC:
      break;
   case TGSI_OPCODE_DPH: {
      out = instr->dph(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_COS: {
      out = instr->cos(inputs[0]);
   }
      break;
   case TGSI_OPCODE_DDX:
      break;
   case TGSI_OPCODE_DDY:
      break;
   case TGSI_OPCODE_KILP:
      break;
   case TGSI_OPCODE_PK2H:
      break;
   case TGSI_OPCODE_PK2US:
      break;
   case TGSI_OPCODE_PK4B:
      break;
   case TGSI_OPCODE_PK4UB:
      break;
   case TGSI_OPCODE_RFL:
      break;
   case TGSI_OPCODE_SEQ:
      break;
   case TGSI_OPCODE_SFL:
      break;
   case TGSI_OPCODE_SGT: {
      out = instr->sgt(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SIN: {
      out = instr->sin(inputs[0]);
   }
      break;
   case TGSI_OPCODE_SLE:
      break;
   case TGSI_OPCODE_SNE:
      break;
   case TGSI_OPCODE_STR:
      break;
   case TGSI_OPCODE_TEX:
      break;
   case TGSI_OPCODE_TXD:
      break;
   case TGSI_OPCODE_UP2H:
      break;
   case TGSI_OPCODE_UP2US:
      break;
   case TGSI_OPCODE_UP4B:
      break;
   case TGSI_OPCODE_UP4UB:
      break;
   case TGSI_OPCODE_X2D:
      break;
   case TGSI_OPCODE_ARA:
      break;
   case TGSI_OPCODE_ARR:
      break;
   case TGSI_OPCODE_BRA:
      break;
   case TGSI_OPCODE_CAL: {
      instr->cal(inst->InstructionExtLabel.Label, storage->inputPtr());
      return;
   }
      break;
   case TGSI_OPCODE_RET: {
      instr->end();
      return;
   }
      break;
   case TGSI_OPCODE_SSG:
      break;
   case TGSI_OPCODE_CMP: {
      out = instr->cmp(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_SCS: {
      out = instr->scs(inputs[0]);
   }
      break;
   case TGSI_OPCODE_TXB:
      break;
   case TGSI_OPCODE_NRM:
      break;
   case TGSI_OPCODE_DIV:
      break;
   case TGSI_OPCODE_DP2:
      break;
   case TGSI_OPCODE_TXL:
      break;
   case TGSI_OPCODE_BRK: {
      instr->brk();
      return;
   }
      break;
   case TGSI_OPCODE_IF: {
      instr->ifop(inputs[0]);
      storage->setCurrentBlock(instr->currentBlock());
      return;  //just update the state
   }
      break;
   case TGSI_OPCODE_LOOP:
      break;
   case TGSI_OPCODE_REP:
      break;
   case TGSI_OPCODE_ELSE: {
      instr->elseop();
      storage->setCurrentBlock(instr->currentBlock());
      return; //only state update
   }
      break;
   case TGSI_OPCODE_ENDIF: {
      instr->endif();
      storage->setCurrentBlock(instr->currentBlock());
      return; //just update the state
   }
      break;
   case TGSI_OPCODE_ENDLOOP:
      break;
   case TGSI_OPCODE_ENDREP:
      break;
   case TGSI_OPCODE_PUSHA:
      break;
   case TGSI_OPCODE_POPA:
      break;
   case TGSI_OPCODE_CEIL:
      break;
   case TGSI_OPCODE_I2F:
      break;
   case TGSI_OPCODE_NOT:
      break;
   case TGSI_OPCODE_TRUNC: {
      out = instr->trunc(inputs[0]);
   }
      break;
   case TGSI_OPCODE_SHL:
      break;
   case TGSI_OPCODE_SHR:
      break;
   case TGSI_OPCODE_AND:
      break;
   case TGSI_OPCODE_OR:
      break;
   case TGSI_OPCODE_MOD:
      break;
   case TGSI_OPCODE_XOR:
      break;
   case TGSI_OPCODE_SAD:
      break;
   case TGSI_OPCODE_TXF:
      break;
   case TGSI_OPCODE_TXQ:
      break;
   case TGSI_OPCODE_CONT:
      break;
   case TGSI_OPCODE_EMIT:
      break;
   case TGSI_OPCODE_ENDPRIM:
      break;
   case TGSI_OPCODE_BGNLOOP2: {
      instr->beginLoop();
      storage->setCurrentBlock(instr->currentBlock());
      return;
   }
      break;
   case TGSI_OPCODE_BGNSUB: {
      instr->bgnSub(instno);
      storage->setCurrentBlock(instr->currentBlock());
      storage->pushTemps();
      return;
   }
      break;
   case TGSI_OPCODE_ENDLOOP2: {
      instr->endLoop();
      storage->setCurrentBlock(instr->currentBlock());
      return;
   }
      break;
   case TGSI_OPCODE_ENDSUB: {
      instr->endSub();
      storage->setCurrentBlock(instr->currentBlock());
      storage->popArguments();
      storage->popTemps();
      return;
   }
      break;
   case TGSI_OPCODE_NOISE1:
      break;
   case TGSI_OPCODE_NOISE2:
      break;
   case TGSI_OPCODE_NOISE3:
      break;
   case TGSI_OPCODE_NOISE4:
      break;
   case TGSI_OPCODE_NOP:
      break;
   case TGSI_OPCODE_M4X3:
      break;
   case TGSI_OPCODE_M3X4:
      break;
   case TGSI_OPCODE_M3X3:
      break;
   case TGSI_OPCODE_M3X2:
      break;
   case TGSI_OPCODE_NRM4:
      break;
   case TGSI_OPCODE_CALLNZ:
      break;
   case TGSI_OPCODE_IFC:
      break;
   case TGSI_OPCODE_BREAKC:
      break;
   case TGSI_OPCODE_KIL: {
      out = instr->kil(inputs[0]);
      storage->setKilElement(out);
      return;
   }
      break;
   case TGSI_OPCODE_END:
      instr->end();
      return;
      break;
   default:
      fprintf(stderr, "ERROR: Unknown opcode %d\n",
              inst->Instruction.Opcode);
      assert(0);
      break;
   }

   if (!out) {
      fprintf(stderr, "ERROR: unsupported opcode %d\n",
              inst->Instruction.Opcode);
      assert(!"Unsupported opcode");
   }

   /* # not sure if we need this */
   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      /*TXT( "_SAT" );*/
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      /*TXT( "_SAT[-1,1]" );*/
      break;
   default:
      assert( 0 );
   }

   /* store results  */
   for (int i = 0; i < inst->Instruction.NumDstRegs; ++i) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      if (dst->DstRegister.File == TGSI_FILE_OUTPUT) {
         storage->setOutputElement(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else if (dst->DstRegister.File == TGSI_FILE_TEMPORARY) {
         storage->setTempElement(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else if (dst->DstRegister.File == TGSI_FILE_ADDRESS) {
         storage->setAddrElement(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else {
         fprintf(stderr, "ERROR: unsupported LLVM destination!");
         assert(!"wrong destination");
      }
   }
}


static void
translate_instructionir(llvm::Module *module,
                        StorageSoa *storage,
                        InstructionsSoa *instr,
                        struct tgsi_full_instruction *inst,
                        struct tgsi_full_instruction *fi,
                        unsigned instno)
{
   std::vector< std::vector<llvm::Value*> > inputs(inst->Instruction.NumSrcRegs);

   for (int i = 0; i < inst->Instruction.NumSrcRegs; ++i) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      std::vector<llvm::Value*> val;
      llvm::Value *indIdx = 0;
      int swizzle = swizzleInt(src);

      if (src->SrcRegister.Indirect) {
         indIdx = storage->addrElement(src->SrcRegisterInd.Index);
      }

      val = storage->load((enum tgsi_file_type)src->SrcRegister.File,
                          src->SrcRegister.Index, swizzle, indIdx);

      inputs[i] = val;
   }

   std::vector<llvm::Value*> out(4);
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL: {
      out = instr->arl(inputs[0]);
   }
      break;
   case TGSI_OPCODE_MOV: {
      out = inputs[0];
   }
      break;
   case TGSI_OPCODE_LIT: {
      out = instr->lit(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RCP: {
   }
      break;
   case TGSI_OPCODE_RSQ: {
      out = instr->rsq(inputs[0]);
   }
      break;
   case TGSI_OPCODE_EXP:
      break;
   case TGSI_OPCODE_LOG:
      break;
   case TGSI_OPCODE_MUL: {
      out = instr->mul(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_ADD: {
      out = instr->add(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP3: {
      out = instr->dp3(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP4: {
      out = instr->dp4(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DST: {
   }
      break;
   case TGSI_OPCODE_MIN: {
      out = instr->min(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MAX: {
      out = instr->max(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SLT: {
   }
      break;
   case TGSI_OPCODE_SGE: {
   }
      break;
   case TGSI_OPCODE_MAD: {
      out = instr->madd(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_SUB: {
      out = instr->sub(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_LERP: {
   }
      break;
   case TGSI_OPCODE_CND:
      break;
   case TGSI_OPCODE_CND0:
      break;
   case TGSI_OPCODE_DOT2ADD:
      break;
   case TGSI_OPCODE_INDEX:
      break;
   case TGSI_OPCODE_NEGATE:
      break;
   case TGSI_OPCODE_FRAC: {
   }
      break;
   case TGSI_OPCODE_CLAMP:
      break;
   case TGSI_OPCODE_FLOOR: {
   }
      break;
   case TGSI_OPCODE_ROUND:
      break;
   case TGSI_OPCODE_EXPBASE2: {
   }
      break;
   case TGSI_OPCODE_LOGBASE2: {
   }
      break;
   case TGSI_OPCODE_POWER: {
      out = instr->pow(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_CROSSPRODUCT: {
   }
      break;
   case TGSI_OPCODE_MULTIPLYMATRIX:
      break;
   case TGSI_OPCODE_ABS: {
      out = instr->abs(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RCC:
      break;
   case TGSI_OPCODE_DPH: {
   }
      break;
   case TGSI_OPCODE_COS: {
   }
      break;
   case TGSI_OPCODE_DDX:
      break;
   case TGSI_OPCODE_DDY:
      break;
   case TGSI_OPCODE_KILP:
      break;
   case TGSI_OPCODE_PK2H:
      break;
   case TGSI_OPCODE_PK2US:
      break;
   case TGSI_OPCODE_PK4B:
      break;
   case TGSI_OPCODE_PK4UB:
      break;
   case TGSI_OPCODE_RFL:
      break;
   case TGSI_OPCODE_SEQ:
      break;
   case TGSI_OPCODE_SFL:
      break;
   case TGSI_OPCODE_SGT: {
   }
      break;
   case TGSI_OPCODE_SIN: {
   }
      break;
   case TGSI_OPCODE_SLE:
      break;
   case TGSI_OPCODE_SNE:
      break;
   case TGSI_OPCODE_STR:
      break;
   case TGSI_OPCODE_TEX:
      break;
   case TGSI_OPCODE_TXD:
      break;
   case TGSI_OPCODE_UP2H:
      break;
   case TGSI_OPCODE_UP2US:
      break;
   case TGSI_OPCODE_UP4B:
      break;
   case TGSI_OPCODE_UP4UB:
      break;
   case TGSI_OPCODE_X2D:
      break;
   case TGSI_OPCODE_ARA:
      break;
   case TGSI_OPCODE_ARR:
      break;
   case TGSI_OPCODE_BRA:
      break;
   case TGSI_OPCODE_CAL: {
   }
      break;
   case TGSI_OPCODE_RET: {
   }
      break;
   case TGSI_OPCODE_SSG:
      break;
   case TGSI_OPCODE_CMP: {
   }
      break;
   case TGSI_OPCODE_SCS: {
   }
      break;
   case TGSI_OPCODE_TXB:
      break;
   case TGSI_OPCODE_NRM:
      break;
   case TGSI_OPCODE_DIV:
      break;
   case TGSI_OPCODE_DP2:
      break;
   case TGSI_OPCODE_TXL:
      break;
   case TGSI_OPCODE_BRK: {
   }
      break;
   case TGSI_OPCODE_IF: {
   }
      break;
   case TGSI_OPCODE_LOOP:
      break;
   case TGSI_OPCODE_REP:
      break;
   case TGSI_OPCODE_ELSE: {
   }
      break;
   case TGSI_OPCODE_ENDIF: {
   }
      break;
   case TGSI_OPCODE_ENDLOOP:
      break;
   case TGSI_OPCODE_ENDREP:
      break;
   case TGSI_OPCODE_PUSHA:
      break;
   case TGSI_OPCODE_POPA:
      break;
   case TGSI_OPCODE_CEIL:
      break;
   case TGSI_OPCODE_I2F:
      break;
   case TGSI_OPCODE_NOT:
      break;
   case TGSI_OPCODE_TRUNC: {
   }
      break;
   case TGSI_OPCODE_SHL:
      break;
   case TGSI_OPCODE_SHR:
      break;
   case TGSI_OPCODE_AND:
      break;
   case TGSI_OPCODE_OR:
      break;
   case TGSI_OPCODE_MOD:
      break;
   case TGSI_OPCODE_XOR:
      break;
   case TGSI_OPCODE_SAD:
      break;
   case TGSI_OPCODE_TXF:
      break;
   case TGSI_OPCODE_TXQ:
      break;
   case TGSI_OPCODE_CONT:
      break;
   case TGSI_OPCODE_EMIT:
      break;
   case TGSI_OPCODE_ENDPRIM:
      break;
   case TGSI_OPCODE_BGNLOOP2: {
   }
      break;
   case TGSI_OPCODE_BGNSUB: {
   }
      break;
   case TGSI_OPCODE_ENDLOOP2: {
   }
      break;
   case TGSI_OPCODE_ENDSUB: {
   }
      break;
   case TGSI_OPCODE_NOISE1:
      break;
   case TGSI_OPCODE_NOISE2:
      break;
   case TGSI_OPCODE_NOISE3:
      break;
   case TGSI_OPCODE_NOISE4:
      break;
   case TGSI_OPCODE_NOP:
      break;
   case TGSI_OPCODE_M4X3:
      break;
   case TGSI_OPCODE_M3X4:
      break;
   case TGSI_OPCODE_M3X3:
      break;
   case TGSI_OPCODE_M3X2:
      break;
   case TGSI_OPCODE_NRM4:
      break;
   case TGSI_OPCODE_CALLNZ:
      break;
   case TGSI_OPCODE_IFC:
      break;
   case TGSI_OPCODE_BREAKC:
      break;
   case TGSI_OPCODE_KIL: {
   }
      break;
   case TGSI_OPCODE_END:
      instr->end();
      return;
      break;
   default:
      fprintf(stderr, "ERROR: Unknown opcode %d\n",
              inst->Instruction.Opcode);
      assert(0);
      break;
   }

   if (!out[0]) {
      fprintf(stderr, "ERROR: unsupported opcode %d\n",
              inst->Instruction.Opcode);
      assert(!"Unsupported opcode");
   }

   /* store results  */
   for (int i = 0; i < inst->Instruction.NumDstRegs; ++i) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      storage->store((enum tgsi_file_type)dst->DstRegister.File,
                     dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
   }
}

llvm::Module *
tgsi_to_llvm(struct gallivm_ir *ir, const struct tgsi_token *tokens)
{
   llvm::Module *mod = new Module("shader");
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   unsigned instno = 0;
   Function* shader = mod->getFunction("execute_shader");
   std::ostringstream stream;
   if (ir->type == GALLIVM_VS) {
      stream << "vs_shader";
   } else {
      stream << "fs_shader";
   }
   stream << ir->id;
   std::string func_name = stream.str();
   shader->setName(func_name.c_str());

   Function::arg_iterator args = shader->arg_begin();
   Value *ptr_INPUT = args++;
   ptr_INPUT->setName("input");

   BasicBlock *label_entry = BasicBlock::Create("entry", shader, 0);

   tgsi_parse_init(&parse, tokens);

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();
   Storage storage(label_entry, ptr_INPUT);
   Instructions instr(mod, shader, label_entry, &storage);
   while(!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         translate_declaration(ir, mod, &storage,
                               &parse.FullToken.FullDeclaration,
                               &fd);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         translate_immediate(&storage,
                             &parse.FullToken.FullImmediate);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         translate_instruction(mod, &storage, &instr,
                               &parse.FullToken.FullInstruction,
                               &fi, instno);
         ++instno;
         break;

      default:
         assert(0);
      }
   }

   tgsi_parse_free(&parse);

   ir->num_consts = storage.numConsts();
   return mod;
}

llvm::Module * tgsi_to_llvmir(struct gallivm_ir *ir,
                              const struct tgsi_token *tokens)
{
   llvm::Module *mod = new Module("shader");
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   unsigned instno = 0;
   std::ostringstream stream;
   if (ir->type == GALLIVM_VS) {
      stream << "vs_shader";
   } else {
      stream << "fs_shader";
   }
   //stream << ir->id;
   std::string func_name = stream.str();
   Function *shader = llvm::cast<Function>(mod->getOrInsertFunction(
                                              func_name.c_str(),
                                              vertexShaderFunctionType()));

   Function::arg_iterator args = shader->arg_begin();
   Value *input = args++;
   input->setName("inputs");
   Value *output = args++;
   output->setName("outputs");
   Value *consts = args++;
   consts->setName("consts");
   Value *temps = args++;
   temps->setName("temps");

   BasicBlock *label_entry = BasicBlock::Create("entry", shader, 0);

   tgsi_parse_init(&parse, tokens);

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();

   StorageSoa storage(label_entry, input, output, consts, temps);
   InstructionsSoa instr(mod, shader, label_entry, &storage);

   while(!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         translate_declarationir(ir, mod, &storage,
                                 &parse.FullToken.FullDeclaration,
                                 &fd);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         translate_immediateir(&storage,
                             &parse.FullToken.FullImmediate);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         storage.declareImmediates();
         translate_instructionir(mod, &storage, &instr,
                                 &parse.FullToken.FullInstruction,
                                 &fi, instno);
         ++instno;
         break;

      default:
         assert(0);
      }
   }

   tgsi_parse_free(&parse);

   return mod;
}
