#include <assert.h>
#include <error.h>
#include "testlib.h"

int main(int argc, char **argv)
{
	const unsigned int	width = 16, height = 16;
	const unsigned int	min_required_blocks = 1, min_required_macroblocks = 1;
	const unsigned int	mc_types[2] = {XVMC_MOCOMP | XVMC_MPEG_2, XVMC_IDCT | XVMC_MPEG_2};
	
	Display			*display;
	XvPortID		port_num;
	int			surface_type_id;
	unsigned int		is_overlay, intra_unsigned;
	int			colorkey;
	XvMCContext		context;
	XvMCSurface		surface;
	XvMCBlockArray		blocks = {0};
	XvMCMacroBlockArray	macroblocks = {0};
	
	display = XOpenDisplay(NULL);
	
	if (!GetPort
	(
		display,
		width,
		height,
		XVMC_CHROMA_FORMAT_420,
    		mc_types,
    		2,
    		&port_num,
    		&surface_type_id,
    		&is_overlay,
    		&intra_unsigned
	))
	{
		XCloseDisplay(display);
		error(1, 0, "Error, unable to find a good port.\n");
	}
	
	if (is_overlay)
	{
		Atom xv_colorkey = XInternAtom(display, "XV_COLORKEY", 0);
		XvGetPortAttribute(display, port_num, xv_colorkey, &colorkey);
	}
	
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, XVMC_DIRECT, &context) == Success);
	assert(XvMCCreateSurface(display, &context, &surface) == Success);
	
	/* Test NULL context */
	assert(XvMCCreateBlocks(display, NULL, 1, &blocks) == XvMCBadContext);
	/* Test 0 blocks */
	assert(XvMCCreateBlocks(display, &context, 0, &blocks) == BadValue);
	/* Test too many blocks */
	/*assert(XvMCCreateBlocks(display, &context, 16384, &blocks) == BadAlloc);*/
	
	/* Note: No XvMCBadBlock(s) error in spec */
	
	/* Test valid params */
	assert(XvMCCreateBlocks(display, &context, min_required_blocks, &blocks) == Success);
	/* Test context id assigned and correct */
	assert(blocks.context_id == context.context_id);
	/* Test number of blocks assigned and correct */
	assert(blocks.num_blocks == min_required_blocks);
	/* Test block pointer valid */
	assert(blocks.blocks != NULL);
	/* Test NULL context */
	assert(XvMCCreateMacroBlocks(display, NULL, 1, &macroblocks) == XvMCBadContext);
	/* Test 0 macroblocks */
	assert(XvMCCreateMacroBlocks(display, &context, 0, &macroblocks) == BadValue);
	/* Test too many macroblocks */
	/*assert(XvMCCreateMacroBlocks(display, &context, 16384, &macroblocks) == BadAlloc);*/

	/* Note: No XvMCBadMacroBlock(s) error in spec */

	/* Test valid params */
	assert(XvMCCreateMacroBlocks(display, &context, min_required_macroblocks, &macroblocks) == Success);
	/* Test context id assigned and correct */
	assert(macroblocks.context_id == context.context_id);
	/* Test macroblock pointer valid */
	assert(macroblocks.macro_blocks != NULL);
	/* Test valid params */
	assert(XvMCDestroyMacroBlocks(display, &macroblocks) == Success);
	/* Test valid params */
	assert(XvMCDestroyBlocks(display, &blocks) == Success);
	
	assert(XvMCDestroySurface(display, &surface) == Success);
	assert(XvMCDestroyContext(display, &context) == Success);
	
	XvUngrabPort(display, port_num, CurrentTime);
	XCloseDisplay(display);
	
	return 0;
}

