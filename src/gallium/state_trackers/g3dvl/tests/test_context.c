#include <vl_context.h>
#include <xsp_winsys.h>

int main(int argc, char **argv)
{
	const unsigned int	video_width = 32, video_height = 32;
	
	Display			*display;
	struct pipe_context	*pipe;
	struct VL_CONTEXT	*ctx;
	
	display = XOpenDisplay(NULL);
	pipe = create_pipe_context(display);
	
	vlCreateContext(display, pipe, video_width, video_height, VL_FORMAT_YCBCR_420, &ctx);
	vlDestroyContext(ctx);
	
	XCloseDisplay(display);
	
	return 0;
}

