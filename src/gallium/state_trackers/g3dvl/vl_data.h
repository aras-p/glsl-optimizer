#ifndef vl_data_h
#define vl_data_h

#include "vl_types.h"

/* TODO: Needs to be rolled into the appropriate stage */

extern const struct vlVertex2f macroblock_verts[24];
extern const struct vlVertex2f macroblock_luma_texcoords[24];
extern const struct vlVertex2f *macroblock_chroma_420_texcoords;
extern const struct vlVertex2f *macroblock_chroma_422_texcoords;
extern const struct vlVertex2f *macroblock_chroma_444_texcoords;

extern const struct vlVertex2f surface_verts[4];
extern const struct vlVertex2f *surface_texcoords;

/*
extern const struct VL_MC_FS_CONSTS vl_mc_fs_consts;

extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_identity;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601_full;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709;
extern const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709_full;
*/

#endif
