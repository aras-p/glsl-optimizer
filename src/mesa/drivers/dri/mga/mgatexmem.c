/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatexmem.c,v 1.7 2002/10/30 12:51:36 alanh Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"
#include "mgaioctl.h"

/*#include "mem.h" */
#include "simple_list.h"

static void
mgaSwapOutTexObj(mgaContextPtr mmesa, mgaTextureObjectPtr t)
{
   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (t->age > mmesa->dirtyAge)
	 mmesa->dirtyAge = t->age;
   }

   t->dirty_images = ~0;
   move_to_tail(&(mmesa->SwappedOut), t);
}

static void
mgaPrintLocalLRU( mgaContextPtr mmesa, int heap )
{
   mgaTextureObjectPtr t;
   int sz = 1 << (mmesa->mgaScreen->logTextureGranularity[heap]);

   fprintf(stderr, "\nLocal LRU, heap %d:\n", heap);

   foreach( t, &(mmesa->TexObjList[heap]) ) {
      if (!t->tObj)
	 fprintf(stderr, "Placeholder %d at %x sz %x\n",
		 t->MemBlock->ofs / sz,
		 t->MemBlock->ofs,
		 t->MemBlock->size);
      else
	 fprintf(stderr, "Texture (bound %d) at %x sz %x\n",
		 t->bound,
		 t->MemBlock->ofs,
		 t->MemBlock->size);
   }

   fprintf(stderr, "\n\n");
}

static void
mgaPrintGlobalLRU( mgaContextPtr mmesa, int heap )
{
   int i, j;
   drmTextureRegion *list = mmesa->sarea->texList[heap];

   fprintf(stderr, "\nGlobal LRU, heap %d list %p:\n", heap, list);

   for (i = 0, j = MGA_NR_TEX_REGIONS ; i < MGA_NR_TEX_REGIONS ; i++) {
      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
	      j, list[j].age, list[j].next, list[j].prev);
      j = list[j].next;
      if (j == MGA_NR_TEX_REGIONS) break;
   }

   if (j != MGA_NR_TEX_REGIONS) {
      fprintf(stderr, "Loop detected in global LRU\n\n\n");
      for (i = 0 ; i < MGA_NR_TEX_REGIONS ; i++) {
	 fprintf(stderr, "list[%d] age %d next %d prev %d\n",
		 i, list[i].age, list[i].next, list[i].prev);
      }
   }

   fprintf(stderr, "\n\n");
}


static void mgaResetGlobalLRU( mgaContextPtr mmesa, GLuint heap )
{
   drmTextureRegion *list = mmesa->sarea->texList[heap];
   int sz = 1 << mmesa->mgaScreen->logTextureGranularity[heap];
   int i;

   mmesa->texAge[heap] = ++mmesa->sarea->texAge[heap];

   if (0) fprintf(stderr, "mgaResetGlobalLRU %d\n", (int)heap);

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (MGA_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.
    */
   for (i = 0 ; (i+1) * sz <= mmesa->mgaScreen->textureSize[heap] ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = mmesa->sarea->texAge[heap];
   }

   i--;
   list[0].prev = MGA_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = MGA_NR_TEX_REGIONS;
   list[MGA_NR_TEX_REGIONS].prev = i;
   list[MGA_NR_TEX_REGIONS].next = 0;

}


static void mgaUpdateTexLRU( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
   int i;
   int heap = t->heap;
   int logsz = mmesa->mgaScreen->logTextureGranularity[heap];
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   drmTextureRegion *list = mmesa->sarea->texList[heap];

   mmesa->texAge[heap] = ++mmesa->sarea->texAge[heap];

   if (!t->MemBlock) {
      fprintf(stderr, "no memblock\n\n");
      return;
   }

   /* Update our local LRU
    */
   move_to_head( &(mmesa->TexObjList[heap]), t );


   if (0)
      fprintf(stderr, "mgaUpdateTexLRU heap %d list %p\n", heap, list);


   /* Update the global LRU
    */
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = mmesa->texAge[heap];

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;

      /* insert_at_head(list, i)
       */
      list[i].prev = MGA_NR_TEX_REGIONS;
      list[i].next = list[MGA_NR_TEX_REGIONS].next;
      list[(unsigned)list[MGA_NR_TEX_REGIONS].next].prev = i;
      list[MGA_NR_TEX_REGIONS].next = i;
   }

   if (0) {
      mgaPrintGlobalLRU(mmesa, t->heap);
      mgaPrintLocalLRU(mmesa, t->heap);
   }
}

/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent
 * the other client's textures.
 */
static void mgaTexturesGone( mgaContextPtr mmesa,
			     GLuint heap,
			     GLuint offset,
			     GLuint size,
			     GLuint in_use )
{
   mgaTextureObjectPtr t, tmp;



   foreach_s ( t, tmp, &(mmesa->TexObjList[heap]) ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
	 continue;




      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
      if (t->bound)
	 mgaSwapOutTexObj( mmesa, t );
      else
	 mgaDestroyTexObj( mmesa, t );
   }


   if (in_use) {
      t = (mgaTextureObjectPtr) calloc(1, sizeof(*t));
      if (!t) return;

      t->heap = heap;
      t->MemBlock = mmAllocMem( mmesa->texHeap[heap], size, 0, offset);
      if (!t->MemBlock) {
	 fprintf(stderr, "Couldn't alloc placeholder sz %x ofs %x\n",
		 (int)size, (int)offset);
	 mmDumpMemInfo( mmesa->texHeap[heap]);
	 return;
      }
      insert_at_head( &(mmesa->TexObjList[heap]), t );
   }
}


void mgaAgeTextures( mgaContextPtr mmesa, int heap )
{
   MGASAREAPrivPtr sarea = mmesa->sarea;
   int sz = 1 << (mmesa->mgaScreen->logTextureGranularity[heap]);
   int idx, nr = 0;

   /* Have to go right round from the back to ensure stuff ends up
    * LRU in our local list...  Fix with a cursor pointer.
    */
   for (idx = sarea->texList[heap][MGA_NR_TEX_REGIONS].prev ;
	idx != MGA_NR_TEX_REGIONS && nr < MGA_NR_TEX_REGIONS ;
	idx = sarea->texList[heap][idx].prev, nr++)
   {
      /* If switching texturing schemes, then the SAREA might not
       * have been properly cleared, so we need to reset the
       * global texture LRU.
       */
      if ( idx * sz > mmesa->mgaScreen->textureSize[heap] ) {
         nr = MGA_NR_TEX_REGIONS;
         break;
      }

      if (sarea->texList[heap][idx].age > mmesa->texAge[heap]) {
        mgaTexturesGone(mmesa, heap, idx * sz, sz,
                        sarea->texList[heap][idx].in_use);
      }
   }

   if (nr == MGA_NR_TEX_REGIONS) {
      mgaTexturesGone(mmesa, heap, 0,
		      mmesa->mgaScreen->textureSize[heap], 0);
      mgaResetGlobalLRU( mmesa, heap );
   }


   if (0) {
      mgaPrintGlobalLRU( mmesa, heap );
      mgaPrintLocalLRU( mmesa, heap );
   }

   mmesa->texAge[heap] = sarea->texAge[heap];
   mmesa->dirty |= MGA_UPLOAD_TEX0IMAGE | MGA_UPLOAD_TEX1IMAGE;
}

/*
 * mgaUploadSubImageLocked
 *
 * Perform an iload based update of a resident buffer.  This is used for
 * both initial loading of the entire image, and texSubImage updates.
 *
 * Performed with the hardware lock held.
 */
void mgaUploadSubImageLocked( mgaContextPtr mmesa,
			      mgaTextureObjectPtr t,
			      int level,
			      int x, int y, int width, int height )
{
   int		x2;
   int		dwords;
   int		offset;
   struct gl_texture_image *image;
   int		texelBytes, texelsPerDword, texelMaccess, length;

   if ( level < 0 || level >= MGA_TEX_MAXLEVELS )
      return;

   image = t->tObj->Image[level];
   if ( !image ) return;


   if (image->Data == 0) {
      fprintf(stderr, "null texture image data tObj %p level %d\n",
	      t->tObj, level);
      return;
   }


   /* find the proper destination offset for this level */
   offset = (t->MemBlock->ofs +
	     t->offsets[level]);


   texelBytes = t->texelBytes;
   switch( texelBytes ) {
   case 1:
      texelsPerDword = 4;
      texelMaccess = 0;
      break;
   case 2:
      texelsPerDword = 2;
      texelMaccess = 1;
      break;
   case 4:
      texelsPerDword = 1;
      texelMaccess = 2;
      break;
   default:
      return;
   }


   /* We can't do a subimage update if pitch is < 32 texels due
    * to hardware XY addressing limits, so we will need to
    * linearly upload all modified rows.
    */
   if ( image->Width < 32 ) {
      x = 0;
      width = image->Width * height;
      height = 1;

      /* Assume that 1x1 textures aren't going to cause a
       * bus error if we read up to four texels from that
       * location:
       */
/*        if ( width < texelsPerDword ) { */
/*  	 width = texelsPerDword; */
/*        } */
   } else {
      /* pad the size out to dwords.  The image is a pointer
	 to the entire image, so we can safely reference
	 outside the x,y,width,height bounds if we need to */
      x2 = x + width;
      x2 = (x2 + (texelsPerDword-1)) & ~(texelsPerDword-1);
      x = (x + (texelsPerDword-1)) & ~(texelsPerDword-1);
      width = x2 - x;
   }

   /* we may not be able to upload the entire texture in one
      batch due to register limits or dma buffer limits.
      Recursively split it up. */
   while ( 1 ) {
      dwords = height * width / texelsPerDword;
      if ( dwords * 4 <= MGA_BUFFER_SIZE ) {
	 break;
      }

      mgaUploadSubImageLocked( mmesa, t, level, x, y,
			       width, height >> 1 );
      y += ( height >> 1 );
      height -= ( height >> 1 );
   }

   length = dwords * 4;

   /* Fill in the secondary buffer with properly converted texels
    * from the mesa buffer. */
   /* FIXME: the sync for direct copy reduces speed.. */
   if(t->heap == MGA_CARD_HEAP  ) {
      mgaGetILoadBufferLocked( mmesa );
      mgaConvertTexture( (GLuint *)mmesa->iload_buffer->address,
			 texelBytes, image, x, y, width, height );
      if(length < 64) length = 64;

      if (0)
	 fprintf(stderr, "TexelBytes : %d, offset: %d, length : %d\n",
		 texelBytes,
		 mmesa->mgaScreen->textureOffset[t->heap] +
		 offset +
		 y * width * 4/texelsPerDword,
		 length);

      mgaFireILoadLocked( mmesa,
			  mmesa->mgaScreen->textureOffset[t->heap] +
			  offset +
			  y * width * 4/texelsPerDword,
			  length);
   } else {
      /* This works, is slower for uploads to card space and needs
       * additional synchronization with the dma stream.
       */
       
      UPDATE_LOCK(mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT);
      mgaConvertTexture( (GLuint *)
			 (mmesa->mgaScreen->texVirtual[t->heap] +
			  offset +
			  y * width * 4/texelsPerDword),
			 texelBytes, image, x, y, width, height );
   }
}


static void mgaUploadTexLevel( mgaContextPtr mmesa,
			       mgaTextureObjectPtr t,
			       int l )
{
   mgaUploadSubImageLocked( mmesa,
			    t,
			    l,
			    0, 0,
			    t->tObj->Image[l]->Width,
			    t->tObj->Image[l]->Height);
}




#if 0
static void mgaMigrateTexture( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
	/* NOT DONE */
}
#endif


static int mgaChooseTexHeap( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
   int freeagp, freecard;
   int fitincard, fitinagp;
   int totalcard, totalagp;
   TMemBlock *b;

   totalcard = totalagp = fitincard = fitinagp = freeagp = freecard = 0;

   b = mmesa->texHeap[0];
   while(b)
   {
     totalcard += b->size;
     if(b->free) if(t->totalSize <= b->size)fitincard = 1;
     b = b->next;
   }

   b = mmesa->texHeap[1];
   while(b)
   {
     totalagp += b->size;
     if(b->free)  if(t->totalSize <= b->size)fitinagp = 1;
     b = b->next;
   }

   if(fitincard)return 0;
   if(fitinagp)return 1;

   if(totalcard && totalagp)
   {
     int ages;
     int ratio = (totalcard > totalagp) ? totalcard / totalagp : totalagp / totalcard;
     ages = mmesa->sarea->texAge[0] + mmesa->sarea->texAge[1];
     if( (ages % ratio) == 0)return totalcard > totalagp ? 1 : 0;
     else return totalcard > totalagp ? 0 : 1;
   }

   if(totalagp) return 1;
   return 0;
}


int mgaUploadTexImages( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
   int heap;
   int i;
   int ofs;

   heap = t->heap = mgaChooseTexHeap( mmesa, t );

   /* Do we need to eject LRU texture objects?
    */
   if (!t->MemBlock) {
      while (1)
      {
	 mgaTextureObjectPtr tmp = mmesa->TexObjList[heap].prev;

	 t->MemBlock = mmAllocMem( mmesa->texHeap[heap],
				   t->totalSize,
				   6, 0 );
	 if (t->MemBlock)
	    break;

	 if (mmesa->TexObjList[heap].prev->bound) {
	    fprintf(stderr, "Hit bound texture in upload\n");
	    return -1;
	 }

	 if (mmesa->TexObjList[heap].prev ==
	     &(mmesa->TexObjList[heap]))
	 {
	    fprintf(stderr, "Failed to upload texture, sz %d\n", t->totalSize);
	    mmDumpMemInfo( mmesa->texHeap[heap] );
	    return -1;
	 }

	 mgaDestroyTexObj( mmesa, tmp );
      }

      ofs = t->MemBlock->ofs
	 + mmesa->mgaScreen->textureOffset[heap]
	 ;

      t->setup.texorg  = ofs;
      t->setup.texorg1 = ofs + t->offsets[1];
      t->setup.texorg2 = ofs + t->offsets[2];
      t->setup.texorg3 = ofs + t->offsets[3];
      t->setup.texorg4 = ofs + t->offsets[4];

      mmesa->dirty |= MGA_UPLOAD_CONTEXT;
   }

   /* Let the world know we've used this memory recently.
    */
   mgaUpdateTexLRU( mmesa, t );


   if (MGA_DEBUG&DEBUG_VERBOSE_LRU)
      fprintf(stderr, "dispatch age: %d age freed memory: %d\n",
	      GET_DISPATCH_AGE(mmesa), mmesa->dirtyAge);

   if (mmesa->dirtyAge >= GET_DISPATCH_AGE(mmesa))
      mgaWaitAgeLocked( mmesa, mmesa->dirtyAge );

   if (t->dirty_images) {
      if (MGA_DEBUG&DEBUG_VERBOSE_LRU)
	 fprintf(stderr, "*");

      for (i = 0 ; i <= t->lastLevel ; i++)
	 if (t->dirty_images & (1<<i))
	    mgaUploadTexLevel( mmesa, t, i );
   }


   t->dirty_images = 0;
   return 0;
}
