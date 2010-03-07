/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#ifndef CELL_STATE_H
#define CELL_STATE_H


#define CELL_NEW_VIEWPORT      0x1
#define CELL_NEW_RASTERIZER    0x2
#define CELL_NEW_FS            0x4
#define CELL_NEW_BLEND         0x8
#define CELL_NEW_CLIP          0x10
#define CELL_NEW_SCISSOR       0x20
#define CELL_NEW_STIPPLE       0x40
#define CELL_NEW_FRAMEBUFFER   0x80
#define CELL_NEW_ALPHA_TEST    0x100
#define CELL_NEW_DEPTH_STENCIL 0x200
#define CELL_NEW_SAMPLER       0x400
#define CELL_NEW_TEXTURE       0x800
#define CELL_NEW_VERTEX        0x1000
#define CELL_NEW_VS            0x2000
#define CELL_NEW_VS_CONSTANTS  0x4000
#define CELL_NEW_FS_CONSTANTS  0x8000
#define CELL_NEW_VERTEX_INFO   0x10000


extern void
cell_update_derived( struct cell_context *cell );


extern void
cell_init_shader_functions(struct cell_context *cell);


extern void
cell_init_vertex_functions(struct cell_context *cell);


#endif /* CELL_STATE_H */

