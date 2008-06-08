#include <assert.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMC.h>
#include <vl_context.h>

/*
 * XvMC defines 64 element blocks (8x8 elements).
 * Elements are 8 bits when they represent color values,
 * 9 bits when they reprecent DCT coefficients, we
 * store them in 2 bytes in either case. DCT coefficients
 * can be signed or unsigned, at our option.
 */
#define BLOCK_SIZE (64 * 2)

Status XvMCCreateBlocks(Display *display, XvMCContext *context, unsigned int num_blocks, XvMCBlockArray *blocks)
{
	struct vl_context *vl_ctx;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	if (num_blocks == 0)
		return BadValue;
	
	assert(blocks);
	
	vl_ctx = context->privData;
	assert(display == vl_ctx->display);

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
	struct vl_context *vl_ctx;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	if (num_blocks == 0)
		return BadValue;
	
	assert(blocks);
	
	vl_ctx = context->privData;
	assert(display == vl_ctx->display);
	
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

