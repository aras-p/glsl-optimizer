#include <assert.h>
#include <stdio.h>
#include <error.h>
#include "testlib.h"

int main(int argc, char **argv)
{
	const unsigned int	width = 32, height = 32;
	const unsigned int	mwidth = width / 16, mheight = height / 16;
	const unsigned int	num_macroblocks = mwidth * mheight;
	const unsigned int	num_blocks = num_macroblocks * 6;
	const unsigned int	mc_types[2] = {XVMC_MOCOMP | XVMC_MPEG_2, XVMC_IDCT | XVMC_MPEG_2};
	
	int			quit = 0;
	Display			*display;
	Window			root, window;
	Pixmap			framebuffer;
	XEvent			event;
	XvPortID		port_num;
	int			surface_type_id;
	unsigned int		is_overlay, intra_unsigned;
	int			colorkey;
	XvMCContext		context;
	XvMCSurface		surface;
	XvMCBlockArray		blocks;
	XvMCMacroBlockArray	macroblocks;
	unsigned int		b, x, y;
	
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
	
	root = XDefaultRootWindow(display);
	window = XCreateSimpleWindow(display, root, 0, 0, width, height, 0, 0, colorkey);
	framebuffer = XCreatePixmap(display, root, width, height, 24);
	
	XSelectInput(display, window, ExposureMask | KeyPressMask);
	XMapWindow(display, window);
	XSync(display, 0);
	
	assert(XvMCCreateContext(display, port_num, surface_type_id, width, height, XVMC_DIRECT, &context) == Success);
	assert(XvMCCreateSurface(display, &context, &surface) == Success);
	assert(XvMCCreateBlocks(display, &context, num_blocks, &blocks) == Success);
	assert(XvMCCreateMacroBlocks(display, &context, num_macroblocks, &macroblocks) == Success);
	
	for (b = 0; b < 6; ++b)
	{
		for (y = 0; y < 8; ++y)
		{
			for (x = 0; x < 8; ++x)
			{
				blocks.blocks[b * 64 + y * 8 + x] = 0xFFFF;
			}
		}
	}
	
	for (y = 0; y < mheight; ++y)
	{
		for (x = 0; x < mwidth; ++x)
		{
			macroblocks.macro_blocks[y * mwidth + x].x = x;
			macroblocks.macro_blocks[y * mwidth + x].y = y;
			macroblocks.macro_blocks[y * mwidth + x].index = (y * mwidth + x) * 6;
			macroblocks.macro_blocks[y * mwidth + x].macroblock_type = XVMC_MB_TYPE_INTRA;
			macroblocks.macro_blocks[y * mwidth + x].coded_block_pattern = 0x3F;
			macroblocks.macro_blocks[y * mwidth + x].dct_type = XVMC_DCT_TYPE_FRAME;
		}
	}
	
	/* Test NULL context */
	assert(XvMCRenderSurface(display, NULL, XVMC_FRAME_PICTURE, &surface, NULL, NULL, 0, 1, 0, &macroblocks, &blocks) == XvMCBadContext);
	/* Test NULL surface */
	assert(XvMCRenderSurface(display, &context, XVMC_FRAME_PICTURE, NULL, NULL, NULL, 0, 1, 0, &macroblocks, &blocks) == XvMCBadSurface);
	/* Test bad picture structure */
	assert(XvMCRenderSurface(display, &context, 0, &surface, NULL, NULL, 0, 1, 0, &macroblocks, &blocks) == BadValue);
	/* Test valid params */
	assert(XvMCRenderSurface(display, &context, XVMC_FRAME_PICTURE, &surface, NULL, NULL, 0, num_macroblocks, 0, &macroblocks, &blocks) == Success);
	
	/* Test NULL surface */
	assert(XvMCPutSurface(display, NULL, window, 0, 0, width, height, 0, 0, width, height, XVMC_FRAME_PICTURE) == XvMCBadSurface);
	/* Test bad window */
	/* X halts with a bad drawable for some reason, doesn't return BadDrawable as expected */
	/*assert(XvMCPutSurface(display, &surface, 0, 0, 0, width, height, 0, 0, width, height, XVMC_FRAME_PICTURE) == BadDrawable);*/
	/* Test valid params */
	assert(XvMCPutSurface(display, &surface, framebuffer, 0, 0, width, height, 0, 0, width, height, XVMC_FRAME_PICTURE) == Success);
	
	puts("Press any key to continue...");
	
	while (!quit)
	{
		XNextEvent(display, &event);
		switch (event.type)
		{
			case Expose:
			{
				XCopyArea
				(
					display,
					framebuffer,
					window,
					XDefaultGC(display, XDefaultScreen(display)),
					0,
					0,
					width,
					height,
					0,
					0
				);
				break;
			}
			case KeyPress:
			{
				quit = 1;
				break;
			}
		}
	}
	
	assert(XvMCDestroyBlocks(display, &blocks) == Success);
	assert(XvMCDestroyMacroBlocks(display, &macroblocks) == Success);
	assert(XvMCDestroySurface(display, &surface) == Success);	
	assert(XvMCDestroyContext(display, &context) == Success);
	
	XFreePixmap(display, framebuffer);
	XvUngrabPort(display, port_num, CurrentTime);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	
	return 0;
}

