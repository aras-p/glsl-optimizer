#ifndef API_LOOPBACK_H
#define API_LOOPBACK_H

#include "glheader.h"

struct _glapi_table;

extern void _mesa_loopback_prefer_float( struct _glapi_table *dest, 
					 GLboolean prefer_float_colors );

extern void _mesa_loopback_init_api_table( struct _glapi_table *dest, 
					   GLboolean prefer_float_colors );

#endif
