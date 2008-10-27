#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>
#include <pipe/p_context.h>
#include <vl_display.h>
#include <vl_screen.h>
#include <vl_context.h>
#include <vl_winsys.h>

static Status Validate
(
	Display *display,
	XvPortID port,
	int surface_type_id,
	unsigned int width,
	unsigned int height,
	int flags,
	int *found_port,
	int *chroma_format,
	int *mc_type
)
{
	unsigned int	found_surface = 0;
	XvAdaptorInfo	*adaptor_info;
	unsigned int	num_adaptors;
	int		num_types;
	unsigned int	max_width, max_height;
	Status		ret;
	unsigned int	i, j, k;

	assert(display && chroma_format);

	*found_port = 0;

	ret = XvQueryAdaptors(display, XDefaultRootWindow(display), &num_adaptors, &adaptor_info);
	if (ret != Success)
		return ret;

	/* Scan through all adaptors looking for this port and surface */
	for (i = 0; i < num_adaptors && !*found_port; ++i)
	{
		/* Scan through all ports of this adaptor looking for our port */
		for (j = 0; j < adaptor_info[i].num_ports && !*found_port; ++j)
		{
			/* If this is our port, scan through all its surfaces looking for our surface */
			if (adaptor_info[i].base_id + j == port)
			{
				XvMCSurfaceInfo *surface_info;

				*found_port = 1;
				surface_info = XvMCListSurfaceTypes(display, adaptor_info[i].base_id, &num_types);

				if (surface_info)
				{
					for (k = 0; k < num_types && !found_surface; ++k)
					{
						if (surface_info[k].surface_type_id == surface_type_id)
						{
							found_surface = 1;
							max_width = surface_info[k].max_width;
							max_height = surface_info[k].max_height;
							*chroma_format = surface_info[k].chroma_format;
							*mc_type = surface_info[k].mc_type;
						}
					}

					XFree(surface_info);
				}
				else
				{
					XvFreeAdaptorInfo(adaptor_info);
					return BadAlloc;
				}
			}
		}
	}

	XvFreeAdaptorInfo(adaptor_info);

	if (!*found_port)
		return XvBadPort;
	if (!found_surface)
		return BadMatch;
	if (width > max_width || height > max_height)
		return BadValue;
	if (flags != XVMC_DIRECT && flags != 0)
		return BadValue;

	return Success;
}

static enum vlProfile ProfileToVL(int xvmc_profile)
{
	if (xvmc_profile & XVMC_MPEG_1)
		assert(0);
	else if (xvmc_profile & XVMC_MPEG_2)
		return vlProfileMpeg2Main;
	else if (xvmc_profile & XVMC_H263)
		assert(0);
	else if (xvmc_profile & XVMC_MPEG_4)
		assert(0);
	else
		assert(0);

	return -1;
}

static enum vlEntryPoint EntryToVL(int xvmc_entry)
{
	return xvmc_entry & XVMC_IDCT ? vlEntryPointIDCT : vlEntryPointMC;
}

static enum vlFormat FormatToVL(int xvmc_format)
{
	switch (xvmc_format)
	{
		case XVMC_CHROMA_FORMAT_420:
			return vlFormatYCbCr420;
		case XVMC_CHROMA_FORMAT_422:
			return vlFormatYCbCr422;
		case XVMC_CHROMA_FORMAT_444:
			return vlFormatYCbCr444;
		default:
			assert(0);
	}

	return -1;
}

Status XvMCCreateContext(Display *display, XvPortID port, int surface_type_id, int width, int height, int flags, XvMCContext *context)
{
	int			found_port;
	int			chroma_format;
	int			mc_type;
	Status			ret;
	struct vlDisplay	*vl_dpy;
	struct vlScreen		*vl_scrn;
	struct vlContext	*vl_ctx;
	struct pipe_context	*pipe;

	assert(display);

	if (!context)
		return XvMCBadContext;

	ret = Validate(display, port, surface_type_id, width, height, flags, &found_port, &chroma_format, &mc_type);

	/* XXX: Success and XvBadPort have the same value */
	if (ret != Success || !found_port)
		return ret;

	/* XXX: Assumes default screen, should check which screen port is on */
	pipe = create_pipe_context(display, XDefaultScreen(display));

	assert(pipe);

	vlCreateDisplay(display, &vl_dpy);
	vlCreateScreen(vl_dpy, XDefaultScreen(display), pipe->screen, &vl_scrn);
	vlCreateContext
	(
		vl_scrn,
		pipe,
		width,
		height,
		FormatToVL(chroma_format),
		ProfileToVL(mc_type),
		EntryToVL(mc_type),
		&vl_ctx
	);

	context->context_id = XAllocID(display);
	context->surface_type_id = surface_type_id;
	context->width = width;
	context->height = height;
	context->flags = flags;
	context->port = port;
	context->privData = vl_ctx;

	return Success;
}

Status XvMCDestroyContext(Display *display, XvMCContext *context)
{
	struct vlContext	*vl_ctx;
	struct pipe_context	*pipe;

	assert(display);

	if (!context)
		return XvMCBadContext;

	vl_ctx = context->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlContextGetScreen(vl_ctx))));

	pipe = vlGetPipeContext(vl_ctx);
	vlDestroyContext(vl_ctx);
	destroy_pipe_context(pipe);

	return Success;
}
