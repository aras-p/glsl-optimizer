#ifndef vl_data_h
#define vl_data_h

#include "vl_types.h"

extern const struct VL_VERTEX2F vl_mb_vertex_positions[24];
extern const struct VL_TEXCOORD2F vl_luma_texcoords[24];
extern const struct VL_TEXCOORD2F *vl_chroma_420_texcoords;
extern const struct VL_TEXCOORD2F *vl_chroma_422_texcoords;
extern const struct VL_TEXCOORD2F *vl_chroma_444_texcoords;

extern const struct VL_VERTEX2F vl_surface_vertex_positions[4];
extern const struct VL_TEXCOORD2F *vl_surface_texcoords;

extern const struct VL_MC_FS_CONSTS vl_mc_fs_consts;

extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_identity;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601_full;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709_full;

#endif

