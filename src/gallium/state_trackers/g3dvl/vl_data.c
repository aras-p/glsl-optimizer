#include "vl_data.h"

/*
 * Represents 8 triangles (4 quads, 1 per block) in noormalized coords
 * that render a macroblock.
 * Need to be scaled to cover mbW*mbH macroblock pixels and translated into
 * position on target surface.
 */
const struct VL_VERTEX2F vl_mb_vertex_positions[24] =
{
	{0.0f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.0f},
	{0.5f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.5f},
	
	{0.5f, 0.0f}, {0.5f, 0.5f}, {1.0f, 0.0f},
	{1.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 0.5f},
	
	{0.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 0.5f},
	{0.5f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f},
	
	{0.5f, 0.5f}, {0.5f, 1.0f}, {1.0f, 0.5f},
	{1.0f, 0.5f}, {0.5f, 1.0f}, {1.0f, 1.0f}
};

/*
 * Represents texcoords for the above for rendering 4 luma blocks arranged
 * in a bW*(bH*4) texture. First luma block located at 0,0->bW,bH; second at
 * 0,bH->bW,2bH; third at 0,2bH->bW,3bH; fourth at 0,3bH->bW,4bH.
 */
const struct VL_TEXCOORD2F vl_luma_texcoords[24] =
{
	{0.0f, 0.0f}, {0.0f, 0.25f}, {1.0f, 0.0f},
	{1.0f, 0.0f}, {0.0f, 0.25f}, {1.0f, 0.25f},
	
	{0.0f, 0.25f}, {0.0f, 0.5f}, {1.0f, 0.25f},
	{1.0f, 0.25f}, {0.0f, 0.5f}, {1.0f, 0.5f},
	
	{0.0f, 0.5f}, {0.0f, 0.75f}, {1.0f, 0.5f},
	{1.0f, 0.5f}, {0.0f, 0.75f}, {1.0f, 0.75f},
	
	{0.0f, 0.75f}, {0.0f, 1.0f}, {1.0f, 0.75f},
	{1.0f, 0.75f}, {0.0f, 1.0f}, {1.0f, 1.0f}
};

/*
 * Represents texcoords for the above for rendering 1 chroma block.
 * Straight forward 0,0->1,1 mapping so we can reuse the MB pos vectors.
 */
const struct VL_TEXCOORD2F *vl_chroma_420_texcoords = (const struct VL_TEXCOORD2F*)vl_mb_vertex_positions;

/*
 * Represents texcoords for the above for rendering 2 chroma blocks arranged
 * in a bW*(bH*2) texture. First chroma block located at 0,0->bW,bH; second at
 * 0,bH->bW,2bH. We can render this with 0,0->1,1 mapping.
 * Straight forward 0,0->1,1 mapping so we can reuse MB pos vectors.
 */
const struct VL_TEXCOORD2F *vl_chroma_422_texcoords = (const struct VL_TEXCOORD2F*)vl_mb_vertex_positions;

/*
 * Represents texcoords for the above for rendering 4 chroma blocks.
 * Same case as 4 luma blocks.
 */
const struct VL_TEXCOORD2F *vl_chroma_444_texcoords = vl_luma_texcoords;

/*
 * Represents 2 triangles in a strip in normalized coords.
 * Used to render the surface onto the frame buffer.
 */
const struct VL_VERTEX2F vl_surface_vertex_positions[4] =
{
	{0.0f, 0.0f},
	{0.0f, 1.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f}
};

/*
 * Represents texcoords for the above. We can use the position values directly.
 */
const struct VL_TEXCOORD2F *vl_surface_texcoords = (const struct VL_TEXCOORD2F*)vl_surface_vertex_positions;

/*
 * Used when rendering P and B macroblocks, multiplier is applied to the A channel,
 * which is then added to the L channel, then the bias is subtracted from that to
 * get back the differential. The differential is then added to the samples from the
 * reference surface(s).
 */
const struct VL_MC_FS_CONSTS vl_mc_fs_consts =
{
	{256.0f, 256.0f, 256.0f, 0.0f},
	{256.0f / 255.0f, 256.0f / 255.0f, 256.0f / 255.0f, 0.0f}
};

/*
 * Identity color conversion constants, for debugging
 */
const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_identity =
{
	{
		0.0f, 0.0f, 0.0f, 0.0f
	},
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	}
};

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601 =
{
	{
		0.0f,		0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.0f,		0.0f,		1.371f,		0.0f,
		1.0f,		-0.336f,	-0.698f,	0.0f,
		1.0f,		1.732f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_601_full =
{
	{
		0.062745098f,	0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.164f,		0.0f,		1.596f,		0.0f,
		1.164f,		-0.391f,	-0.813f,	0.0f,
		1.164f,		2.018f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709 =
{
	{
		0.0f,		0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.0f,		0.0f,		1.540f,		0.0f,
		1.0f,		-0.183f,	-0.459f,	0.0f,
		1.0f,		1.816f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
const struct VL_CSC_FS_CONSTS vl_csc_fs_consts_709_full =
{
	{
		0.062745098f,	0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.164f,		0.0f,		1.793f,		0.0f,
		1.164f,		-0.213f,	-0.534f,	0.0f,
		1.164f,		2.115f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

