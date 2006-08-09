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
	BRW_ATTRIB_SIX = 6,
	BRW_ATTRIB_SEVEN = 7,
	BRW_ATTRIB_TEX0 = 8,
	BRW_ATTRIB_TEX1 = 9,
	BRW_ATTRIB_TEX2 = 10,
	BRW_ATTRIB_TEX3 = 11,
	BRW_ATTRIB_TEX4 = 12,
	BRW_ATTRIB_TEX5 = 13,
	BRW_ATTRIB_TEX6 = 14,
	BRW_ATTRIB_TEX7 = 15,
	BRW_ATTRIB_MAT_FRONT_AMBIENT = 16,
	BRW_ATTRIB_MAT_BACK_AMBIENT = 17,
	BRW_ATTRIB_MAT_FRONT_DIFFUSE = 18,
	BRW_ATTRIB_MAT_BACK_DIFFUSE = 19,
	BRW_ATTRIB_MAT_FRONT_SPECULAR = 20,
	BRW_ATTRIB_MAT_BACK_SPECULAR = 21,
	BRW_ATTRIB_MAT_FRONT_EMISSION = 22,
	BRW_ATTRIB_MAT_BACK_EMISSION = 23,
	BRW_ATTRIB_MAT_FRONT_SHININESS = 24,
	BRW_ATTRIB_MAT_BACK_SHININESS = 25,
	BRW_ATTRIB_MAT_FRONT_INDEXES = 26,
	BRW_ATTRIB_MAT_BACK_INDEXES = 27, 
	BRW_ATTRIB_INDEX = 28,        
	BRW_ATTRIB_EDGEFLAG = 29,     

	BRW_ATTRIB_MAX = 30
} ;

#define BRW_ATTRIB_FIRST_MATERIAL BRW_ATTRIB_MAT_FRONT_AMBIENT

/* Will probably have to revise this scheme fairly shortly, eg. by
 * compacting all the MAT flags down to one bit, or by using two
 * dwords to store the flags.
 */
#define BRW_BIT_POS                 (1<<0)
#define BRW_BIT_WEIGHT              (1<<1)
#define BRW_BIT_NORMAL              (1<<2)
#define BRW_BIT_COLOR0              (1<<3)
#define BRW_BIT_COLOR1              (1<<4)
#define BRW_BIT_FOG                 (1<<5)
#define BRW_BIT_SIX                 (1<<6)
#define BRW_BIT_SEVEN               (1<<7)
#define BRW_BIT_TEX0                (1<<8)
#define BRW_BIT_TEX1                (1<<9)
#define BRW_BIT_TEX2                (1<<10)
#define BRW_BIT_TEX3                (1<<11)
#define BRW_BIT_TEX4                (1<<12)
#define BRW_BIT_TEX5                (1<<13)
#define BRW_BIT_TEX6                (1<<14)
#define BRW_BIT_TEX7                (1<<15)
#define BRW_BIT_MAT_FRONT_AMBIENT   (1<<16)
#define BRW_BIT_MAT_BACK_AMBIENT    (1<<17)
#define BRW_BIT_MAT_FRONT_DIFFUSE   (1<<18)
#define BRW_BIT_MAT_BACK_DIFFUSE    (1<<19)
#define BRW_BIT_MAT_FRONT_SPECULAR  (1<<20)
#define BRW_BIT_MAT_BACK_SPECULAR   (1<<21)
#define BRW_BIT_MAT_FRONT_EMISSION  (1<<22)
#define BRW_BIT_MAT_BACK_EMISSION   (1<<23)
#define BRW_BIT_MAT_FRONT_SHININESS (1<<24)
#define BRW_BIT_MAT_BACK_SHININESS  (1<<25)
#define BRW_BIT_MAT_FRONT_INDEXES   (1<<26)
#define BRW_BIT_MAT_BACK_INDEXES    (1<<27)
#define BRW_BIT_INDEX               (1<<28)
#define BRW_BIT_EDGEFLAG            (1<<29)


#define BRW_MAX_COPIED_VERTS 3

#endif
