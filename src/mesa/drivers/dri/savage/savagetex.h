/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef SAVAGETEX_INC
#define SAVAGETEX_INC

#include "mtypes.h"
#include "mm.h"

#include "savagecontext.h"
#include "savage_3d_reg.h"

#define VALID_SAVAGE_TEXTURE_OBJECT(tobj)  (tobj) 

#define SAVAGE_TEX_MAXLEVELS 11
#define MIN_TILE_CHUNK       8
#define MIPMAP_CHUNK         4



/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
typedef struct  {
    GLuint sWrapMode;
    GLuint tWrapMode;
    GLuint minFilter;
    GLuint magFilter;
    GLuint boarderColor;
    GLuint hwPhysAddress;
} savage_texture_parameter_t;

/** \brief Texture tiling information */
typedef struct savage_tileinfo_t {
    GLuint width, height;       /**< tile width and height */
    GLuint wInSub, hInSub;      /**< tile width and height in subtiles */
    GLuint subWidth, subHeight; /**< subtile width and height */
    GLuint tinyOffset[2];       /**< internal offsets size 1 and 2 images */
} savageTileInfo, *savageTileInfoPtr;

struct savage_texture_object_t {
   struct savage_texture_object_t *next, *prev;
   struct gl_texture_object *globj;
   GLuint age;   
   
   const savageTileInfo *tileInfo;
   GLuint texelBytes;
   GLuint totalSize;
   GLuint bound;
   GLuint heap;

   PMemBlock MemBlock;   
   char *BufAddr;
   
   GLuint min_level;
   GLuint max_level;
   GLuint dirty_images;

   struct { 
      const struct gl_texture_image *image;
      GLuint offset;		/* into BufAddr */
      GLuint height;
      GLuint internalFormat;
   } image[SAVAGE_TEX_MAXLEVELS];

   /* Support for multitexture.
    */
   GLuint current_unit;   
   savage_texture_parameter_t texParams;
};		

#define SAVAGE_NO_PALETTE        0x0
#define SAVAGE_USE_PALETTE       0x1
#define SAVAGE_UPDATE_PALETTE    0x2
#define SAVAGE_FALLBACK_PALETTE  0x4
#define __HWEnvCombineSingleUnitScale(imesa, flag0, flag1, TexBlendCtrl)
#define __HWParseTexEnvCombine(imesa, flag0, TexCtrl, TexBlendCtrl)


void savageUpdateTextureState( GLcontext *ctx );
void savageDDInitTextureFuncs( struct dd_function_table *functions );

void savageDestroyTexObj( savageContextPtr imesa, savageTextureObjectPtr t);
int savageUploadTexImages( savageContextPtr imesa, savageTextureObjectPtr t );

void savageResetGlobalLRU( savageContextPtr imesa , GLuint heap);
void savageTexturesGone( savageContextPtr imesa, GLuint heap, 
		       GLuint start, GLuint end, 
		       GLuint in_use ); 

void savagePrintLocalLRU( savageContextPtr imesa ,GLuint heap);
void savagePrintGlobalLRU( savageContextPtr imesa ,GLuint heap);

#endif
