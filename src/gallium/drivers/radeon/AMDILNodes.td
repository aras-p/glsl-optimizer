//===- AMDILNodes.td - AMD IL nodes ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Conversion DAG Nodes
//===----------------------------------------------------------------------===//
// Double to Single conversion
def IL_d2f : SDNode<"AMDILISD::DP_TO_FP"   , SDTIL_DPToFPOp>;

def IL_inttoany: SDNode<"AMDILISD::INTTOANY", SDTIL_IntToAny>;
//===----------------------------------------------------------------------===//
// Flow Control DAG Nodes
//===----------------------------------------------------------------------===//
def IL_brcond      : SDNode<"AMDILISD::BRANCH_COND", SDTIL_BRCond, [SDNPHasChain]>;

//===----------------------------------------------------------------------===//
// Comparison DAG Nodes
//===----------------------------------------------------------------------===//
def IL_cmp       : SDNode<"AMDILISD::CMP", SDTIL_Cmp>;

//===----------------------------------------------------------------------===//
// Call/Return DAG Nodes
//===----------------------------------------------------------------------===//
def IL_callseq_start : SDNode<"ISD::CALLSEQ_START", SDTIL_CallSeqStart,
    [SDNPHasChain, SDNPOutGlue]>;
def IL_callseq_end   : SDNode<"ISD::CALLSEQ_END",   SDTIL_CallSeqEnd,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def IL_call      : SDNode<"AMDILISD::CALL", SDTIL_Call,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def IL_retflag       : SDNode<"AMDILISD::RET_FLAG", SDTNone,
    [SDNPHasChain, SDNPOptInGlue]>;

//===----------------------------------------------------------------------===//
// Arithmetic DAG Nodes
//===----------------------------------------------------------------------===//
// Address modification nodes
def IL_addaddrri : SDNode<"AMDILISD::ADDADDR", SDTIL_AddAddrri,
    [SDNPCommutative, SDNPAssociative]>;
def IL_addaddrir : SDNode<"AMDILISD::ADDADDR", SDTIL_AddAddrir,
    [SDNPCommutative, SDNPAssociative]>;

//===--------------------------------------------------------------------===//
// Instructions
//===--------------------------------------------------------------------===//
// Floating point math functions
def IL_cmov_logical : SDNode<"AMDILISD::CMOVLOG", SDTIL_GenTernaryOp>;
def IL_add       : SDNode<"AMDILISD::ADD"     , SDTIL_GenBinaryOp>;
def IL_cmov        : SDNode<"AMDILISD::CMOV"    , SDTIL_GenBinaryOp>;
def IL_or      : SDNode<"AMDILISD::OR"    ,SDTIL_GenBinaryOp>;
def IL_and      : SDNode<"AMDILISD::AND"    ,SDTIL_GenBinaryOp>;
def IL_xor          : SDNode<"AMDILISD::XOR", SDTIL_GenBinaryOp>;
def IL_not          : SDNode<"AMDILISD::NOT", SDTIL_GenUnaryOp>;
def IL_div_inf      : SDNode<"AMDILISD::DIV_INF", SDTIL_GenBinaryOp>;
def IL_mad          : SDNode<"AMDILISD::MAD", SDTIL_GenTernaryOp>;

//===----------------------------------------------------------------------===//
// Integer functions
//===----------------------------------------------------------------------===//
def IL_inegate     : SDNode<"AMDILISD::INEGATE" , SDTIntUnaryOp>;
def IL_umul        : SDNode<"AMDILISD::UMUL"    , SDTIntBinOp,
    [SDNPCommutative, SDNPAssociative]>;
def IL_mov        : SDNode<"AMDILISD::MOVE", SDTIL_GenUnaryOp>;
def IL_phimov      : SDNode<"AMDILISD::PHIMOVE", SDTIL_GenUnaryOp>;
def IL_bitconv     : SDNode<"AMDILISD::BITCONV", SDTIL_GenBitConv>;
def IL_ffb_hi      : SDNode<"AMDILISD::IFFB_HI", SDTIL_GenUnaryOp>;
def IL_ffb_lo      : SDNode<"AMDILISD::IFFB_LO", SDTIL_GenUnaryOp>;
def IL_smax        : SDNode<"AMDILISD::SMAX", SDTIL_GenBinaryOp>;

//===----------------------------------------------------------------------===//
// Double functions
//===----------------------------------------------------------------------===//
def IL_dcreate     : SDNode<"AMDILISD::DCREATE"   , SDTIL_DCreate>;
def IL_dcomphi     : SDNode<"AMDILISD::DCOMPHI"     , SDTIL_DComp>;
def IL_dcomplo     : SDNode<"AMDILISD::DCOMPLO"     , SDTIL_DComp>;
def IL_dcreate2     : SDNode<"AMDILISD::DCREATE2"   , SDTIL_DCreate2>;
def IL_dcomphi2     : SDNode<"AMDILISD::DCOMPHI2"     , SDTIL_DComp2>;
def IL_dcomplo2     : SDNode<"AMDILISD::DCOMPLO2"     , SDTIL_DComp2>;

//===----------------------------------------------------------------------===//
// Long functions
//===----------------------------------------------------------------------===//
def IL_lcreate     : SDNode<"AMDILISD::LCREATE"   , SDTIL_LCreate>;
def IL_lcreate2    : SDNode<"AMDILISD::LCREATE2"   , SDTIL_LCreate2>;
def IL_lcomphi     : SDNode<"AMDILISD::LCOMPHI"     , SDTIL_LComp>;
def IL_lcomphi2    : SDNode<"AMDILISD::LCOMPHI2"     , SDTIL_LComp2>;
def IL_lcomplo     : SDNode<"AMDILISD::LCOMPLO"     , SDTIL_LComp>;
def IL_lcomplo2    : SDNode<"AMDILISD::LCOMPLO2"     , SDTIL_LComp2>;

//===----------------------------------------------------------------------===//
// Vector functions
//===----------------------------------------------------------------------===//
def IL_vbuild     : SDNode<"AMDILISD::VBUILD", SDTIL_GenVecBuild,
    []>;
def IL_vextract   : SDNode<"AMDILISD::VEXTRACT", SDTIL_GenVecExtract,
    []>;
def IL_vinsert    : SDNode<"AMDILISD::VINSERT", SDTIL_GenVecInsert,
    []>;
def IL_vconcat    : SDNode<"AMDILISD::VCONCAT", SDTIL_GenVecConcat,
    []>;

//===----------------------------------------------------------------------===//
// AMDIL Image Custom SDNodes
//===----------------------------------------------------------------------===//
def image2d_read  : SDNode<"AMDILISD::IMAGE2D_READ", SDTIL_ImageRead,
    [SDNPHasChain, SDNPMayLoad]>;
def image2d_write : SDNode<"AMDILISD::IMAGE2D_WRITE", SDTIL_ImageWrite,
    [SDNPHasChain, SDNPMayStore]>;
def image2d_info0 : SDNode<"AMDILISD::IMAGE2D_INFO0", SDTIL_ImageInfo, []>;
def image2d_info1 : SDNode<"AMDILISD::IMAGE2D_INFO1", SDTIL_ImageInfo, []>;
def image3d_read  : SDNode<"AMDILISD::IMAGE3D_READ", SDTIL_ImageRead,
    [SDNPHasChain, SDNPMayLoad]>;
def image3d_write : SDNode<"AMDILISD::IMAGE3D_WRITE", SDTIL_ImageWrite3D,
    [SDNPHasChain, SDNPMayStore]>;
def image3d_info0 : SDNode<"AMDILISD::IMAGE3D_INFO0", SDTIL_ImageInfo, []>;
def image3d_info1 : SDNode<"AMDILISD::IMAGE3D_INFO1", SDTIL_ImageInfo, []>;

//===----------------------------------------------------------------------===//
// AMDIL Atomic Custom SDNodes
//===----------------------------------------------------------------------===//
//===-------------- 32 bit global atomics with return values --------------===//
def atom_g_add : SDNode<"AMDILISD::ATOM_G_ADD", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_and : SDNode<"AMDILISD::ATOM_G_AND", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_cmpxchg : SDNode<"AMDILISD::ATOM_G_CMPXCHG", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>; 
def atom_g_dec : SDNode<"AMDILISD::ATOM_G_DEC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_inc : SDNode<"AMDILISD::ATOM_G_INC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_max : SDNode<"AMDILISD::ATOM_G_MAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_umax : SDNode<"AMDILISD::ATOM_G_UMAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_min : SDNode<"AMDILISD::ATOM_G_MIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_umin : SDNode<"AMDILISD::ATOM_G_UMIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_or : SDNode<"AMDILISD::ATOM_G_OR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_sub : SDNode<"AMDILISD::ATOM_G_SUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_rsub : SDNode<"AMDILISD::ATOM_G_RSUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_xchg : SDNode<"AMDILISD::ATOM_G_XCHG", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_xor : SDNode<"AMDILISD::ATOM_G_XOR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

//===------------- 32 bit global atomics without return values ------------===//
def atom_g_add_noret : SDNode<"AMDILISD::ATOM_G_ADD_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_and_noret : SDNode<"AMDILISD::ATOM_G_AND_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_cmpxchg_noret : SDNode<"AMDILISD::ATOM_G_CMPXCHG_NORET",
    SDTIL_TriAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_cmp_noret : SDNode<"AMDILISD::ATOM_G_CMPXCHG_NORET",
    SDTIL_TriAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_dec_noret : SDNode<"AMDILISD::ATOM_G_DEC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_inc_noret : SDNode<"AMDILISD::ATOM_G_INC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_max_noret : SDNode<"AMDILISD::ATOM_G_MAX_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_umax_noret: SDNode<"AMDILISD::ATOM_G_UMAX_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_min_noret : SDNode<"AMDILISD::ATOM_G_MIN_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_umin_noret: SDNode<"AMDILISD::ATOM_G_UMIN_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_or_noret : SDNode<"AMDILISD::ATOM_G_OR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_sub_noret : SDNode<"AMDILISD::ATOM_G_SUB_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_rsub_noret : SDNode<"AMDILISD::ATOM_G_RSUB_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_g_xchg_noret: SDNode<"AMDILISD::ATOM_G_XCHG_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_g_xor_noret : SDNode<"AMDILISD::ATOM_G_XOR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

//===--------------- 32 bit local atomics with return values --------------===//
def atom_l_add : SDNode<"AMDILISD::ATOM_L_ADD", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_and : SDNode<"AMDILISD::ATOM_L_AND", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_cmpxchg : SDNode<"AMDILISD::ATOM_L_CMPXCHG", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_dec : SDNode<"AMDILISD::ATOM_L_DEC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_inc : SDNode<"AMDILISD::ATOM_L_INC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_max : SDNode<"AMDILISD::ATOM_L_MAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_umax : SDNode<"AMDILISD::ATOM_L_UMAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_min : SDNode<"AMDILISD::ATOM_L_MIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_umin : SDNode<"AMDILISD::ATOM_L_UMIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_or : SDNode<"AMDILISD::ATOM_L_OR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_mskor : SDNode<"AMDILISD::ATOM_L_MSKOR", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_sub : SDNode<"AMDILISD::ATOM_L_SUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_rsub : SDNode<"AMDILISD::ATOM_L_RSUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_xchg : SDNode<"AMDILISD::ATOM_L_XCHG", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_xor : SDNode<"AMDILISD::ATOM_L_XOR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;

//===-------------- 32 bit local atomics without return values ------------===//
def atom_l_add_noret : SDNode<"AMDILISD::ATOM_L_ADD_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_and_noret : SDNode<"AMDILISD::ATOM_L_AND_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_cmpxchg_noret : SDNode<"AMDILISD::ATOM_L_CMPXCHG_NORET",
    SDTIL_TriAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_dec_noret : SDNode<"AMDILISD::ATOM_L_DEC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_inc_noret : SDNode<"AMDILISD::ATOM_L_INC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_max_noret : SDNode<"AMDILISD::ATOM_L_MAX_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_umax_noret: SDNode<"AMDILISD::ATOM_L_UMAX_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_min_noret : SDNode<"AMDILISD::ATOM_L_MIN_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_umin_noret: SDNode<"AMDILISD::ATOM_L_UMIN_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_or_noret : SDNode<"AMDILISD::ATOM_L_OR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_mskor_noret : SDNode<"AMDILISD::ATOM_L_MSKOR_NORET",
    SDTIL_TriAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_sub_noret : SDNode<"AMDILISD::ATOM_L_SUB_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_rsub_noret : SDNode<"AMDILISD::ATOM_L_RSUB_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_l_xchg_noret: SDNode<"AMDILISD::ATOM_L_XCHG_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_l_xor_noret : SDNode<"AMDILISD::ATOM_L_XOR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

//===--------------- 32 bit local atomics with return values --------------===//
def atom_r_add : SDNode<"AMDILISD::ATOM_R_ADD", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_and : SDNode<"AMDILISD::ATOM_R_AND", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_cmpxchg : SDNode<"AMDILISD::ATOM_R_CMPXCHG", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_dec : SDNode<"AMDILISD::ATOM_R_DEC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_inc : SDNode<"AMDILISD::ATOM_R_INC", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_max : SDNode<"AMDILISD::ATOM_R_MAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_umax : SDNode<"AMDILISD::ATOM_R_UMAX", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_min : SDNode<"AMDILISD::ATOM_R_MIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_umin : SDNode<"AMDILISD::ATOM_R_UMIN", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_or : SDNode<"AMDILISD::ATOM_R_OR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_mskor : SDNode<"AMDILISD::ATOM_R_MSKOR", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_sub : SDNode<"AMDILISD::ATOM_R_SUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_rsub : SDNode<"AMDILISD::ATOM_R_RSUB", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_xchg : SDNode<"AMDILISD::ATOM_R_XCHG", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_xor : SDNode<"AMDILISD::ATOM_R_XOR", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;

//===-------------- 32 bit local atomics without return values ------------===//
def atom_r_add_noret : SDNode<"AMDILISD::ATOM_R_ADD_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_and_noret : SDNode<"AMDILISD::ATOM_R_AND_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_cmpxchg_noret : SDNode<"AMDILISD::ATOM_R_CMPXCHG_NORET",
    SDTIL_TriAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_dec_noret : SDNode<"AMDILISD::ATOM_R_DEC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_inc_noret : SDNode<"AMDILISD::ATOM_R_INC_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_max_noret : SDNode<"AMDILISD::ATOM_R_MAX_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_umax_noret: SDNode<"AMDILISD::ATOM_R_UMAX_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_min_noret : SDNode<"AMDILISD::ATOM_R_MIN_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_umin_noret: SDNode<"AMDILISD::ATOM_R_UMIN_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_or_noret : SDNode<"AMDILISD::ATOM_R_OR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_mskor_noret : SDNode<"AMDILISD::ATOM_R_MSKOR_NORET", SDTIL_TriAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_sub_noret : SDNode<"AMDILISD::ATOM_R_SUB_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_rsub_noret : SDNode<"AMDILISD::ATOM_R_RSUB_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;
def atom_r_xchg_noret: SDNode<"AMDILISD::ATOM_R_XCHG_NORET",
    SDTIL_BinAtom, [SDNPHasChain, SDNPMayLoad, SDNPMayStore, SDNPMemOperand]>;
def atom_r_xor_noret : SDNode<"AMDILISD::ATOM_R_XOR_NORET", SDTIL_BinAtom,
    [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

//===--------------- 32 bit atomic counter instructions -------------------===//
def append_alloc : SDNode<"AMDILISD::APPEND_ALLOC", SDTIL_Append,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore]>;
def append_consume : SDNode<"AMDILISD::APPEND_CONSUME", SDTIL_Append,
    [SDNPHasChain, SDNPMayLoad, SDNPMayStore]>;
def append_alloc_noret : SDNode<"AMDILISD::APPEND_ALLOC_NORET", SDTIL_Append,
    [SDNPHasChain, SDNPMayStore]>;
def append_consume_noret : SDNode<"AMDILISD::APPEND_CONSUME_NORET",
    SDTIL_Append, [SDNPHasChain, SDNPMayStore]>;
