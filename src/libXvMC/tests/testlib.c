#include "testlib.h"
#include <stdio.h>

/*
void test(int pred, const char *pred_string, const char *doc_string, const char *file, unsigned int line)
{
	fputs(doc_string, stderr);
	if (!pred)
		fprintf(stderr, " FAIL!\n\t\"%s\" at %s:%u\n", pred_string, file, line);
	else
		fputs(" PASS!\n", stderr);
}
*/

int GetPort
(
	Display *display,
	unsigned int width,
	unsigned int height,
	unsigned int chroma_format,
	const unsigned int *mc_types,
	unsigned int num_mc_types,
	XvPortID *port_id,
	int *surface_type_id,
	unsigned int *is_overlay,
	unsigned int *intra_unsigned
)
{
	unsigned int	found_port = 0;
	XvAdaptorInfo	*adaptor_info;
	unsigned int	num_adaptors;
	int		num_types;
	int		ev_base, err_base;
	unsigned int	i, j, k, l;
	
	if (!XvMCQueryExtension(display, &ev_base, &err_base))
		return 0;
	if (XvQueryAdaptors(display, XDefaultRootWindow(display), &num_adaptors, &adaptor_info) != Success)
		return 0;
	
	for (i = 0; i < num_adaptors && !found_port; ++i)
	{
		if (adaptor_info[i].type & XvImageMask)
		{
			XvMCSurfaceInfo *surface_info = XvMCListSurfaceTypes(display, adaptor_info[i].base_id, &num_types);
			
			if (surface_info)
			{
				for (j = 0; j < num_types && !found_port; ++j)
				{
					if
					(
						surface_info[j].chroma_format == chroma_format &&
						surface_info[j].max_width >= width &&
						surface_info[j].max_height >= height
					)
					{
						for (k = 0; k < num_mc_types && !found_port; ++k)
						{
							if (surface_info[j].mc_type == mc_types[k])
							{
								for (l = 0; l < adaptor_info[i].num_ports && !found_port; ++l)
								{
									if (XvGrabPort(display, adaptor_info[i].base_id + l, CurrentTime) == Success)
									{
										*port_id = adaptor_info[i].base_id + l;
										*surface_type_id = surface_info[j].surface_type_id;
										*is_overlay = surface_info[j].flags & XVMC_OVERLAID_SURFACE;
										*intra_unsigned = surface_info[j].flags & XVMC_INTRA_UNSIGNED;
										found_port = 1;
									}
								}
							}
						}
					}
				}
				
				XFree(surface_info);
			}
		}
	}
	
	XvFreeAdaptorInfo(adaptor_info);
	
	return found_port;
}

