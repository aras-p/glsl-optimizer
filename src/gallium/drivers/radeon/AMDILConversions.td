//==- AMDILConversions.td - Type conversion tablegen patterns -*-tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
def actos_i16:Pat < (i16 (anyext GPRI8:$src)),
(IL_ASSHORT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def uctos_i16:Pat < (i16 (zext GPRI8:$src)),
(IL_ASSHORT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def sctos_i16:Pat < (i16 (sext GPRI8:$src)),
(IL_ASSHORT_i32
 (SHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def actoi_i32:Pat < (i32 (anyext GPRI8:$src)),
(IL_ASINT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def uctoi_i32:Pat < (i32 (zext GPRI8:$src)),
(IL_ASINT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def sctoi_i32:Pat < (i32 (sext GPRI8:$src)),
(IL_ASINT_i32
 (SHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))) >;


def actol_i64:Pat < (i64 (anyext GPRI8:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24)),
 (LOADCONST_i32 0)) >;


def uctol_i64:Pat < (i64 (zext GPRI8:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24)),
 (LOADCONST_i32 0)) >;


def sctol_i64:Pat < (i64 (sext GPRI8:$src)),
(LCREATE
 (SHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24)),
 (SHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 31))) >;


def astoi_i32:Pat < (i32 (anyext GPRI16:$src)),
(IL_ASINT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16))) >;


def ustoi_i32:Pat < (i32 (zext GPRI16:$src)),
(IL_ASINT_i32
 (USHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16))) >;


def sstoi_i32:Pat < (i32 (sext GPRI16:$src)),
(IL_ASINT_i32
 (SHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16))) >;


def astol_i64:Pat < (i64 (anyext GPRI16:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16)),
 (LOADCONST_i32 0)) >;


def ustol_i64:Pat < (i64 (zext GPRI16:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16)),
 (LOADCONST_i32 0)) >;


def sstol_i64:Pat < (i64 (sext GPRI16:$src)),
(LCREATE
 (SHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16)),
 (SHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 31))) >;


def aitol_i64:Pat < (i64 (anyext GPRI32:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i32 GPRI32:$src),
   (LOADCONST_i32 0)),
  (LOADCONST_i32 0)),
 (LOADCONST_i32 0)) >;


def uitol_i64:Pat < (i64 (zext GPRI32:$src)),
(LCREATE
 (USHR_i32
  (SHL_i32
(IL_ASINT_i32 GPRI32:$src),
   (LOADCONST_i32 0)),
  (LOADCONST_i32 0)),
 (LOADCONST_i32 0)) >;


def sitol_i64:Pat < (i64 (sext GPRI32:$src)),
(LCREATE
 (SHR_i32
  (SHL_i32
(IL_ASINT_i32 GPRI32:$src),
   (LOADCONST_i32 0)),
  (LOADCONST_i32 0)),
 (SHR_i32
  (SHL_i32
(IL_ASINT_i32 GPRI32:$src),
   (LOADCONST_i32 0)),
  (LOADCONST_i32 31))) >;



def sctof_f32:Pat < (f32 (sint_to_fp GPRI8:$src)),
(f32
 (ITOF
  (SHR_i32
   (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
    (LOADCONST_i32 24)),
   (LOADCONST_i32 24)))) >;


def uctof_f32:Pat < (f32 (uint_to_fp GPRI8:$src)),
(f32
 (UTOF
  (USHR_i32
   (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
    (LOADCONST_i32 24)),
   (LOADCONST_i32 24)))) >;


def sctod_f64:Pat < (f64 (sint_to_fp GPRI8:$src)),
(f64 (FTOD
      (ITOF
       (SHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))))) >;


def uctod_f64:Pat < (f64 (uint_to_fp GPRI8:$src)),
(f64 (FTOD
      (UTOF
       (USHR_i32
  (SHL_i32
(IL_ASINT_i8 GPRI8:$src),
   (LOADCONST_i32 24)),
  (LOADCONST_i32 24))))) >;

def sstof_f32:Pat < (f32 (sint_to_fp GPRI16:$src)),
(f32
 (ITOF
  (SHR_i32
   (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
    (LOADCONST_i32 16)),
   (LOADCONST_i32 16)))) >;


def ustof_f32:Pat < (f32 (uint_to_fp GPRI16:$src)),
(f32
 (UTOF
  (USHR_i32
   (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
    (LOADCONST_i32 16)),
   (LOADCONST_i32 16)))) >;

def sstod_f64:Pat < (f64 (sint_to_fp GPRI16:$src)),
(f64 (FTOD
      (ITOF
       (SHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16))))) >;


def ustod_f64:Pat < (f64 (uint_to_fp GPRI16:$src)),
(f64 (FTOD
      (UTOF
       (USHR_i32
  (SHL_i32
(IL_ASINT_i16 GPRI16:$src),
   (LOADCONST_i32 16)),
  (LOADCONST_i32 16))))) >;


def actos_v2i16:Pat < (v2i16 (anyext GPRV2I8:$src)),
(IL_ASV2SHORT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def uctos_v2i16:Pat < (v2i16 (zext GPRV2I8:$src)),
(IL_ASV2SHORT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def sctos_v2i16:Pat < (v2i16 (sext GPRV2I8:$src)),
(IL_ASV2SHORT_v2i32
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def actoi_v2i32:Pat < (v2i32 (anyext GPRV2I8:$src)),
(IL_ASV2INT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def uctoi_v2i32:Pat < (v2i32 (zext GPRV2I8:$src)),
(IL_ASV2INT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def sctoi_v2i32:Pat < (v2i32 (sext GPRV2I8:$src)),
(IL_ASV2INT_v2i32
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24)))) >;


def actol_v2i64:Pat < (v2i64 (anyext GPRV2I8:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def uctol_v2i64:Pat < (v2i64 (zext GPRV2I8:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def sctol_v2i64:Pat < (v2i64 (sext GPRV2I8:$src)),
(LCREATE_v2i64
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 24))),
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
   (VCREATE_v2i32 (LOADCONST_i32 24))),
  (VCREATE_v2i32 (LOADCONST_i32 31)))) >;


def astoi_v2i32:Pat < (v2i32 (anyext GPRV2I16:$src)),
(IL_ASV2INT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16)))) >;


def ustoi_v2i32:Pat < (v2i32 (zext GPRV2I16:$src)),
(IL_ASV2INT_v2i32
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16)))) >;


def sstoi_v2i32:Pat < (v2i32 (sext GPRV2I16:$src)),
(IL_ASV2INT_v2i32
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16)))) >;


def astol_v2i64:Pat < (v2i64 (anyext GPRV2I16:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def ustol_v2i64:Pat < (v2i64 (zext GPRV2I16:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def sstol_v2i64:Pat < (v2i64 (sext GPRV2I16:$src)),
(LCREATE_v2i64
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 16))),
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
   (VCREATE_v2i32 (LOADCONST_i32 16))),
  (VCREATE_v2i32 (LOADCONST_i32 31)))) >;


def aitol_v2i64:Pat < (v2i64 (anyext GPRV2I32:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i32 GPRV2I32:$src),
   (VCREATE_v2i32 (LOADCONST_i32 0))),
  (VCREATE_v2i32 (LOADCONST_i32 0))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def uitol_v2i64:Pat < (v2i64 (zext GPRV2I32:$src)),
(LCREATE_v2i64
 (USHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i32 GPRV2I32:$src),
   (VCREATE_v2i32 (LOADCONST_i32 0))),
  (VCREATE_v2i32 (LOADCONST_i32 0))),
 (VCREATE_v2i32 (LOADCONST_i32 0))) >;


def sitol_v2i64:Pat < (v2i64 (sext GPRV2I32:$src)),
(LCREATE_v2i64
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i32 GPRV2I32:$src),
   (VCREATE_v2i32 (LOADCONST_i32 0))),
  (VCREATE_v2i32 (LOADCONST_i32 0))),
 (SHRVEC_v2i32
  (SHLVEC_v2i32
(IL_ASV2INT_v2i32 GPRV2I32:$src),
   (VCREATE_v2i32 (LOADCONST_i32 0))),
  (VCREATE_v2i32 (LOADCONST_i32 31)))) >;



def sctof_v2f32:Pat < (v2f32 (sint_to_fp GPRV2I8:$src)),
(v2f32
 (ITOF_v2f32
  (SHRVEC_v2i32
   (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
    (VCREATE_v2i32 (LOADCONST_i32 24))),
   (VCREATE_v2i32 (LOADCONST_i32 24))))) >;


def uctof_v2f32:Pat < (v2f32 (uint_to_fp GPRV2I8:$src)),
(v2f32
 (UTOF_v2f32
  (USHRVEC_v2i32
   (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
    (VCREATE_v2i32 (LOADCONST_i32 24))),
   (VCREATE_v2i32 (LOADCONST_i32 24))))) >;


def sctod_v2f64:Pat < (v2f64 (sint_to_fp GPRV2I8:$src)),
(v2f64 
 (VINSERT_v2f64
 (VCREATE_v2f64 
 (FTOD
  (VEXTRACT_v2f32
  (ITOF_v2f32
   (SHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
     (VCREATE_v2i32 (LOADCONST_i32 24))),
    (VCREATE_v2i32 (LOADCONST_i32 24)))),
  1)
 )),
 (FTOD
  (VEXTRACT_v2f32
  (ITOF_v2f32
   (SHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
     (VCREATE_v2i32 (LOADCONST_i32 24))),
    (VCREATE_v2i32 (LOADCONST_i32 24)))),
  2)
  ), 1, 256)
 ) >;

def uctod_v2f64:Pat < (v2f64 (uint_to_fp GPRV2I8:$src)),
(v2f64 
 (VINSERT_v2f64
 (VCREATE_v2f64 
 (FTOD
  (VEXTRACT_v2f32
  (UTOF_v2f32
   (USHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
     (VCREATE_v2i32 (LOADCONST_i32 24))),
    (VCREATE_v2i32 (LOADCONST_i32 24)))),
  1)
 )),
 (FTOD
  (VEXTRACT_v2f32
  (UTOF_v2f32
   (USHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i8 GPRV2I8:$src),
     (VCREATE_v2i32 (LOADCONST_i32 24))),
    (VCREATE_v2i32 (LOADCONST_i32 24)))),
  2)
  ), 1, 256)
 ) >;

def sstof_v2f32:Pat < (v2f32 (sint_to_fp GPRV2I16:$src)),
(v2f32
 (ITOF_v2f32
  (SHRVEC_v2i32
   (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
    (VCREATE_v2i32 (LOADCONST_i32 16))),
   (VCREATE_v2i32 (LOADCONST_i32 16))))) >;


def ustof_v2f32:Pat < (v2f32 (uint_to_fp GPRV2I16:$src)),
(v2f32
 (UTOF_v2f32
  (USHRVEC_v2i32
   (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
    (VCREATE_v2i32 (LOADCONST_i32 16))),
   (VCREATE_v2i32 (LOADCONST_i32 16))))) >;


def sstod_v2f64:Pat < (v2f64 (sint_to_fp GPRV2I16:$src)),
(v2f64 
 (VINSERT_v2f64
 (VCREATE_v2f64 
 (FTOD
  (VEXTRACT_v2f32
  (ITOF_v2f32
   (SHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
     (VCREATE_v2i32 (LOADCONST_i32 16))),
    (VCREATE_v2i32 (LOADCONST_i32 16)))),
  1)
 )),
 (FTOD
  (VEXTRACT_v2f32
  (ITOF_v2f32
   (SHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
     (VCREATE_v2i32 (LOADCONST_i32 16))),
    (VCREATE_v2i32 (LOADCONST_i32 16)))),
  2)
  ), 1, 256)
 ) >;

def ustod_v2f64:Pat < (v2f64 (uint_to_fp GPRV2I16:$src)),
(v2f64 
 (VINSERT_v2f64
 (VCREATE_v2f64 
 (FTOD
  (VEXTRACT_v2f32
  (UTOF_v2f32
   (USHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
     (VCREATE_v2i32 (LOADCONST_i32 16))),
    (VCREATE_v2i32 (LOADCONST_i32 16)))),
  1)
 )),
 (FTOD
  (VEXTRACT_v2f32
  (UTOF_v2f32
   (USHRVEC_v2i32
    (SHLVEC_v2i32
(IL_ASV2INT_v2i16 GPRV2I16:$src),
     (VCREATE_v2i32 (LOADCONST_i32 16))),
    (VCREATE_v2i32 (LOADCONST_i32 16)))),
  2)
  ), 1, 256)
 ) >;


def actos_v4i16:Pat < (v4i16 (anyext GPRV4I8:$src)),
(IL_ASV4SHORT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def uctos_v4i16:Pat < (v4i16 (zext GPRV4I8:$src)),
(IL_ASV4SHORT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def sctos_v4i16:Pat < (v4i16 (sext GPRV4I8:$src)),
(IL_ASV4SHORT_v4i32
 (SHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def actoi_v4i32:Pat < (v4i32 (anyext GPRV4I8:$src)),
(IL_ASV4INT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def uctoi_v4i32:Pat < (v4i32 (zext GPRV4I8:$src)),
(IL_ASV4INT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def sctoi_v4i32:Pat < (v4i32 (sext GPRV4I8:$src)),
(IL_ASV4INT_v4i32
 (SHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
   (VCREATE_v4i32 (LOADCONST_i32 24))),
  (VCREATE_v4i32 (LOADCONST_i32 24)))) >;


def astoi_v4i32:Pat < (v4i32 (anyext GPRV4I16:$src)),
(IL_ASV4INT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i16 GPRV4I16:$src),
   (VCREATE_v4i32 (LOADCONST_i32 16))),
  (VCREATE_v4i32 (LOADCONST_i32 16)))) >;


def ustoi_v4i32:Pat < (v4i32 (zext GPRV4I16:$src)),
(IL_ASV4INT_v4i32
 (USHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i16 GPRV4I16:$src),
   (VCREATE_v4i32 (LOADCONST_i32 16))),
  (VCREATE_v4i32 (LOADCONST_i32 16)))) >;


def sstoi_v4i32:Pat < (v4i32 (sext GPRV4I16:$src)),
(IL_ASV4INT_v4i32
 (SHRVEC_v4i32
  (SHLVEC_v4i32
(IL_ASV4INT_v4i16 GPRV4I16:$src),
   (VCREATE_v4i32 (LOADCONST_i32 16))),
  (VCREATE_v4i32 (LOADCONST_i32 16)))) >;



def sctof_v4f32:Pat < (v4f32 (sint_to_fp GPRV4I8:$src)),
(v4f32
 (ITOF_v4f32
  (SHRVEC_v4i32
   (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
    (VCREATE_v4i32 (LOADCONST_i32 24))),
   (VCREATE_v4i32 (LOADCONST_i32 24))))) >;


def uctof_v4f32:Pat < (v4f32 (uint_to_fp GPRV4I8:$src)),
(v4f32
 (UTOF_v4f32
  (USHRVEC_v4i32
   (SHLVEC_v4i32
(IL_ASV4INT_v4i8 GPRV4I8:$src),
    (VCREATE_v4i32 (LOADCONST_i32 24))),
   (VCREATE_v4i32 (LOADCONST_i32 24))))) >;


def sstof_v4f32:Pat < (v4f32 (sint_to_fp GPRV4I16:$src)),
(v4f32
 (ITOF_v4f32
  (SHRVEC_v4i32
   (SHLVEC_v4i32
(IL_ASV4INT_v4i16 GPRV4I16:$src),
    (VCREATE_v4i32 (LOADCONST_i32 16))),
   (VCREATE_v4i32 (LOADCONST_i32 16))))) >;


def ustof_v4f32:Pat < (v4f32 (uint_to_fp GPRV4I16:$src)),
(v4f32
 (UTOF_v4f32
  (USHRVEC_v4i32
   (SHLVEC_v4i32
(IL_ASV4INT_v4i16 GPRV4I16:$src),
    (VCREATE_v4i32 (LOADCONST_i32 16))),
   (VCREATE_v4i32 (LOADCONST_i32 16))))) >;

