/* $Id: ggimesa.h,v 1.3 2000/02/09 19:03:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
 * Copyright (C) 1998  Uwe Maurer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef GGIMESA_H
#define GGIMESA_H


#define GGIMESA_MAJOR_VERSION 3
#define GGIMESA_MINOR_VERSION 3


#ifdef __cplusplus
extern "C" {
#endif

#include "GL/gl.h"


typedef struct ggi_mesa_context *GGIMesaContext;

#include <ggi/ggi.h>

extern GGIMesaContext GGIMesaCreateContext(void);

extern void GGIMesaDestroyContext(GGIMesaContext ctx);

extern void GGIMesaMakeCurrent(GGIMesaContext ctx);

extern GGIMesaContext GGIMesaGetCurrentContext(void);

extern void GGIMesaSwapBuffers(void);

extern int GGIMesaSetVisual(GGIMesaContext ctx, ggi_visual_t vis,
			    GLboolean rgb_flag, GLboolean db_flag);

#ifdef __cplusplus
}
#endif

#endif
