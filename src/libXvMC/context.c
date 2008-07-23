#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>
#include <vl_context.h>
#include <vl_winsys.h>

static Status Validate(Display *display, XvPortID port, int surface_type_id, unsigned int width, unsigned int height, int flags, int *chroma_format)
{
	unsigned int	found_port = 0;
	unsigned int	found_surface = 0;
	XvAdaptorInfo	*adaptor_info;
	unsigned int	num_adaptors;
	int		num_types;
	unsigned int	max_width, max_height;
	Status		ret;
	unsigned int	i, j, k;
	
	assert(display && chroma_format);
	
	ret = XvQueryAdaptors(display, XDefaultRootWindow(display), &num_adaptors, &adaptor_info);
	if (ret != Success)
		return ret;
	
	/* Scan through all adaptors looking for this port and surface */
	for (i = 0; i < num_adaptors && !found_port; ++i)
	{
		/* Scan through all ports of this adaptor looking for our port */
		for (j = 0; j < adaptor_info[i].num_ports && !found_port; ++j)
		{
			/* If this is our port, scan through all its surfaces looking for our surface */
			if (adaptor_info[i].base_id + j == port)
			{
				XvMCSurfaceInfo *surface_info;
				
				found_port = 1;
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
	
	if (!found_port)
		return XvBadPort;
	if (!found_surface)
		return BadMatch;
	if (width > max_width || height > max_height)
		return BadValue;
	if (flags != XVMC_DIRECT && flags != 0)
		return BadValue;
	
	return Success;
}

static enum VL_FORMAT FormatToVL(int xvmc_format)
{
	enum VL_FORMAT vl_format;
	
	switch (xvmc_format)
	{
		case XVMC_CHROMA_FORMAT_420:
		{
			vl_format = VL_FORMAT_YCBCR_420;
			break;
		}
		case XVMC_CHROMA_FORMAT_422:
		{
			vl_format = VL_FORMAT_YCBCR_422;
			break;
		}
		case XVMC_CHROMA_FORMAT_444:
		{
			vl_format = VL_FORMAT_YCBCR_444;
			break;
		}
		default:
			assert(0);
	}
	
	return vl_format;
}

Status XvMCCreateContext(Display *display, XvPortID port, int surface_type_id, int width, int height, int flags, XvMCContext *context)
{
	int			chroma_format;
	Status			ret;
	struct VL_CONTEXT	*vl_ctx;
	struct pipe_context	*pipe;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	
	ret = Validate(display, port, surface_type_id, width, height, flags, &chroma_format);
	if (ret != Success)
		return ret;
	
	pipe = create_pipe_context(display, XDefaultScreen(display));
	
	assert(pipe);
	
	vlCreateContext(display, pipe, width, height, FormatToVL(chroma_format), &vl_ctx);
	
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
	struct VL_CONTEXT	*vl_ctx;
	struct pipe_context	*pipe;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	
	vl_ctx = context->privData;
	
	assert(display == vl_ctx->display);
	
	pipe = vl_ctx->pipe;
	vlDestroyContext(vl_ctx);
	destroy_pipe_context(pipe);
	
	return Success;
}

/* XXX: The following are here temporarily, need to be implemented in the DDX driver */
/* TODO: Figure out which of these need to be in DDX, which are better off in DDX, which can stay */

Bool XvMCQueryExtension(Display *display, int *event_base, int *err_base)
{
	*event_base = 0;
	*err_base = 0;
	
	return True;
}

Status XvMCQueryVersion(Display *display, int *major, int *minor)
{
	*major = 1;
	*minor = 0;
	
	return Success;
}

XvMCSurfaceInfo* XvMCListSurfaceTypes(Display *display, XvPortID port, int *num)
{
	XvMCSurfaceInfo	*surface_info = calloc(1, sizeof(XvMCSurfaceInfo));
	
	*num = 1;
	
	surface_info->chroma_format = XVMC_CHROMA_FORMAT_420;
	surface_info->max_width = 2048;
	surface_info->max_height = 2048;
	surface_info->mc_type = XVMC_IDCT | XVMC_MPEG_2;
	surface_info->surface_type_id = 123; /* FIXME: XAllocID(display)*/;
	surface_info->flags |= XVMC_INTRA_UNSIGNED;
	
	return surface_info;
}

XvImageFormatValues* XvMCListSubpictureTypes(Display* display, XvPortID port, int surface_type_id, int *count_return)
{
	/* TODO */
	*count_return = 0;
	
	return NULL;
}

