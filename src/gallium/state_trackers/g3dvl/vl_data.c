#include "vl_data.h"

/*
 * Represents 8 triangles (4 quads, 1 per block) in noormalized coords
 * that render a macroblock.
 * Need to be scaled to cover mbW*mbH macroblock pixels and translated into
 * position on target surface.
 */
const struct vlVertex2f macroblock_verts[24] =
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
const struct vlVertex2f macroblock_luma_texcoords[24] =
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
const struct vlVertex2f *macroblock_chroma_420_texcoords = macroblock_verts;

/*
 * Represents texcoords for the above for rendering 2 chroma blocks arranged
 * in a bW*(bH*2) texture. First chroma block located at 0,0->bW,bH; second at
 * 0,bH->bW,2bH. We can render this with 0,0->1,1 mapping.
 * Straight forward 0,0->1,1 mapping so we can reuse MB pos vectors.
 */
const struct vlVertex2f *macroblock_chroma_422_texcoords = macroblock_verts;

/*
 * Represents texcoords for the above for rendering 4 chroma blocks.
 * Same case as 4 luma blocks.
 */
const struct vlVertex2f *macroblock_chroma_444_texcoords = macroblock_luma_texcoords;

/*
 * Used when rendering P and B macroblocks, multiplier is applied to the A channel,
 * which is then added to the L channel, then the bias is subtracted from that to
 * get back the differential. The differential is then added to the samples from the
 * reference surface(s).
 */
#if 0
const struct VL_MC_FS_CONSTS vl_mc_fs_consts =
{
	{32767.0f / 255.0f, 32767.0f / 255.0f, 32767.0f / 255.0f, 0.0f},
	{0.5f, 2.0f, 0.0f, 0.0f}
};
#endif
