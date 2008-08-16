#include <assert.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMC.h>
#include <vl_display.h>
#include <vl_screen.h>
#include <vl_context.h>

#define BLOCK_SIZE (64 * 2)

Status XvMCCreateBlocks(Display *display, XvMCContext *context, unsigned int num_blocks, XvMCBlockArray *blocks)
{
	struct vlContext *vl_ctx;

	assert(display);

	if (!context)
		return XvMCBadContext;
	if (num_blocks == 0)
		return BadValue;

	assert(blocks);

	vl_ctx = context->privData;
	assert(display == vlGetNativeDisplay(vlGetDisplay(vlContextGetScreen(vl_ctx))));

	blocks->context_id = context->context_id;
	blocks->num_blocks = num_blocks;
	blocks->blocks = malloc(BLOCK_SIZE * num_blocks);
	/* Since we don't have a VL type for blocks, set privData to the display so we can catch mismatches */
	blocks->privData = display;

	return Success;
}

Status XvMCDestroyBlocks(Display *display, XvMCBlockArray *blocks)
{
	assert(display);
	assert(blocks);
	assert(display == blocks->privData);
	free(blocks->blocks);

	return Success;
}

Status XvMCCreateMacroBlocks(Display *display, XvMCContext *context, unsigned int num_blocks, XvMCMacroBlockArray *blocks)
{
	struct vlContext *vl_ctx;

	assert(display);

	if (!context)
		return XvMCBadContext;
	if (num_blocks == 0)
		return BadValue;

	assert(blocks);

	vl_ctx = context->privData;
	assert(display == vlGetNativeDisplay(vlGetDisplay(vlContextGetScreen(vl_ctx))));

	blocks->context_id = context->context_id;
	blocks->num_blocks = num_blocks;
	blocks->macro_blocks = malloc(sizeof(XvMCMacroBlock) * num_blocks);
	/* Since we don't have a VL type for blocks, set privData to the display so we can catch mismatches */
	blocks->privData = display;

	return Success;
}

Status XvMCDestroyMacroBlocks(Display *display, XvMCMacroBlockArray *blocks)
{
	assert(display);
	assert(blocks);
	assert(display == blocks->privData);
	free(blocks->macro_blocks);

	return Success;
}
