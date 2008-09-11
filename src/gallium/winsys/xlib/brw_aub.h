/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#ifndef BRW_AUB_H
#define BRW_AUB_H

/* We set up this region, buffers may be allocated here:
 */
#define AUB_BUF_START (4096*4)
#define AUB_BUF_SIZE  (8*1024*1024)

struct intel_context;
struct pipe_surface;

struct brw_aubfile *brw_aubfile_create( void );

void brw_aub_destroy( struct brw_aubfile *aubfile );

void brw_aub_gtt_data( struct brw_aubfile *aubfile,
		       unsigned offset,
		       const void *data,
		       unsigned sz,
		       unsigned type,
		       unsigned state_type );

void brw_aub_gtt_cmds( struct brw_aubfile *aubfile,
		       unsigned offset,
		       const void *data,
		       unsigned sz );

void brw_aub_dump_bmp( struct brw_aubfile *aubfile,
		       struct pipe_surface *surface,
		       unsigned gtt_offset );


enum data_write_type {
   DW_NOTYPE,
   DW_BATCH_BUFFER,
   DW_BIN_BUFFER,
   DW_BIN_POINTER_LIST,
   DW_SLOW_STATE_BUFFER,
   DW_VERTEX_BUFFER,
   DW_2D_MAP,
   DW_CUBE_MAP,
   DW_INDIRECT_STATE_BUFFER,
   DW_VOLUME_MAP,
   DW_1D_MAP,
   DW_CONSTANT_BUFFER,
   DW_CONSTANT_URB_ENTRY,
   DW_INDEX_BUFFER,
   DW_GENERAL_STATE,
   DW_SURFACE_STATE,
   DW_MEDIA_OBJECT_INDIRECT_DATA,
   DW_MAX_TYPE
};

enum data_write_general_state_type {
   DWGS_NOTYPE,
   DWGS_VERTEX_SHADER_STATE,
   DWGS_GEOMETRY_SHADER_STATE ,
   DWGS_CLIPPER_STATE,
   DWGS_STRIPS_FANS_STATE,
   DWGS_WINDOWER_IZ_STATE,
   DWGS_COLOR_CALC_STATE,
   DWGS_CLIPPER_VIEWPORT_STATE,	/* was 0x7 */
   DWGS_STRIPS_FANS_VIEWPORT_STATE,
   DWGS_COLOR_CALC_VIEWPORT_STATE, /* was 0x9 */
   DWGS_SAMPLER_STATE,
   DWGS_KERNEL_INSTRUCTIONS,
   DWGS_SCRATCH_SPACE,
   DWGS_SAMPLER_DEFAULT_COLOR,
   DWGS_INTERFACE_DESCRIPTOR,
   DWGS_VLD_STATE,
   DWGS_VFE_STATE,
   DWGS_MAX_TYPE
};

enum data_write_surface_state_type {
   DWSS_NOTYPE,
   DWSS_BINDING_TABLE_STATE,
   DWSS_SURFACE_STATE,
   DWSS_MAX_TYPE
};


#endif
