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

#ifndef BRW_ATTRIB_H
#define BRW_ATTRIB_H


/*
 * Note: The first attributes match the VERT_ATTRIB_* definitions
 * in mtypes.h.  However, the tnl module has additional attributes
 * for materials, color indexes, edge flags, etc.
 */
/* Although it's nice to use these as bit indexes in a DWORD flag, we
 * could manage without if necessary.  Another limit currently is the
 * number of bits allocated for these numbers in places like vertex
 * program instruction formats and register layouts.
 */
enum {
	BRW_ATTRIB_POS = 0,
	BRW_ATTRIB_WEIGHT = 1,
	BRW_ATTRIB_NORMAL = 2,
	BRW_ATTRIB_COLOR0 = 3,
	BRW_ATTRIB_COLOR1 = 4,
	BRW_ATTRIB_FOG = 5,
	BRW_ATTRIB_INDEX = 6,        
	BRW_ATTRIB_EDGEFLAG = 7,     
	BRW_ATTRIB_TEX0 = 8,
	BRW_ATTRIB_TEX1 = 9,
	BRW_ATTRIB_TEX2 = 10,
	BRW_ATTRIB_TEX3 = 11,
	BRW_ATTRIB_TEX4 = 12,
	BRW_ATTRIB_TEX5 = 13,
	BRW_ATTRIB_TEX6 = 14,
	BRW_ATTRIB_TEX7 = 15,

	BRW_ATTRIB_GENERIC0 = 16, /* Not used? */
	BRW_ATTRIB_GENERIC1 = 17,
	BRW_ATTRIB_GENERIC2 = 18,
	BRW_ATTRIB_GENERIC3 = 19,
	BRW_ATTRIB_GENERIC4 = 20,
	BRW_ATTRIB_GENERIC5 = 21,
	BRW_ATTRIB_GENERIC6 = 22,
	BRW_ATTRIB_GENERIC7 = 23,
	BRW_ATTRIB_GENERIC8 = 24,
	BRW_ATTRIB_GENERIC9 = 25,
	BRW_ATTRIB_GENERIC10 = 26,
	BRW_ATTRIB_GENERIC11 = 27,
	BRW_ATTRIB_GENERIC12 = 28,
	BRW_ATTRIB_GENERIC13 = 29,
	BRW_ATTRIB_GENERIC14 = 30,
	BRW_ATTRIB_GENERIC15 = 31,

	BRW_ATTRIB_MAT_FRONT_AMBIENT = 32, 
	BRW_ATTRIB_MAT_BACK_AMBIENT = 33,
	BRW_ATTRIB_MAT_FRONT_DIFFUSE = 34,
	BRW_ATTRIB_MAT_BACK_DIFFUSE = 35,
	BRW_ATTRIB_MAT_FRONT_SPECULAR = 36,
	BRW_ATTRIB_MAT_BACK_SPECULAR = 37,
	BRW_ATTRIB_MAT_FRONT_EMISSION = 38,
	BRW_ATTRIB_MAT_BACK_EMISSION = 39,
	BRW_ATTRIB_MAT_FRONT_SHININESS = 40,
	BRW_ATTRIB_MAT_BACK_SHININESS = 41,
	BRW_ATTRIB_MAT_FRONT_INDEXES = 42,
	BRW_ATTRIB_MAT_BACK_INDEXES = 43, 

	BRW_ATTRIB_MAX = 44
} ;

#define BRW_ATTRIB_FIRST_MATERIAL BRW_ATTRIB_MAT_FRONT_AMBIENT
#define BRW_ATTRIB_LAST_MATERIAL BRW_ATTRIB_MAT_BACK_INDEXES

#define BRW_MAX_COPIED_VERTS 3


static inline GLuint64EXT brw_translate_inputs( GLboolean vp_enabled,
						GLuint mesa_inputs )
{
   GLuint64EXT inputs = mesa_inputs;
   if (vp_enabled)
      return inputs;
   else 
      return (inputs & 0xffff) | ((inputs & 0xffff0000) << 16);
}


#endif
