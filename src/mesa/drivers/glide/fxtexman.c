/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


/* fxtexman.c - 3Dfx VooDoo texture memory functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"

static tfxTMFreeNode *fxTMNewTMFreeNode(FxU32 start, FxU32 end)
{
  tfxTMFreeNode *tmn;

  if(!(tmn=MALLOC(sizeof(tfxTMFreeNode)))) {
    fprintf(stderr,"fx Driver: out of memory !\n");
    fxCloseHardware();
    exit(-1);
  }

  tmn->next=NULL;
  tmn->startAddress=start;
  tmn->endAddress=end;

  return tmn;
}

/* Notice this uses grTex{Min,Max}Address directly with FX_ because it
   is only used during initialization where the lock is already held. */
static void fxTMUInit(fxMesaContext fxMesa, int tmu)
{
  tfxTMFreeNode *tmn,*tmntmp;
  FxU32 start,end,blockstart,blockend;

  start=FX_grTexMinAddress(tmu);
  end=FX_grTexMaxAddress(tmu);

  if(fxMesa->verbose) {
    fprintf(stderr,"%s configuration:",(tmu==FX_TMU0) ? "TMU0" : "TMU1");
    fprintf(stderr,"  Lower texture memory address (%u)\n",(unsigned int)start);
    fprintf(stderr,"  Higher texture memory address (%u)\n",(unsigned int)end);
    fprintf(stderr,"  Splitting Texture memory in 2Mb blocks:\n");
  }

  fxMesa->freeTexMem[tmu]=end-start;
  fxMesa->tmFree[tmu]=NULL;
  fxMesa->tmAlloc[tmu]=NULL;

  blockstart=start;
  while(blockstart<=end) {
    if(blockstart+0x1fffff>end)
      blockend=end;
    else
      blockend=blockstart+0x1fffff;

    if(fxMesa->verbose)
      fprintf(stderr,"    %07u-%07u\n",(unsigned int)blockstart,(unsigned int)blockend);

    tmn=fxTMNewTMFreeNode(blockstart,blockend);

    if(fxMesa->tmFree[tmu]) {
      for(tmntmp=fxMesa->tmFree[tmu];tmntmp->next!=NULL;tmntmp=tmntmp->next){};
      tmntmp->next=tmn;
    } else
      fxMesa->tmFree[tmu]=tmn;

    blockstart+=0x1fffff+1;
  }
}

void fxTMInit(fxMesaContext fxMesa)
{
  fxTMUInit(fxMesa,FX_TMU0);

  if(fxMesa->haveTwoTMUs)
    fxTMUInit(fxMesa,FX_TMU1);

  fxMesa->texBindNumber=0;
}

static struct gl_texture_object *fxTMFindOldestTMBlock(fxMesaContext fxMesa,
						       tfxTMAllocNode *tmalloc,
						       GLuint texbindnumber)
{
  GLuint age,oldestage,lasttimeused;
  struct gl_texture_object *oldesttexobj;

  (void)fxMesa;
  oldesttexobj=tmalloc->tObj;
  oldestage=0;

  while(tmalloc) {
    lasttimeused=((tfxTexInfo *)(tmalloc->tObj->DriverData))->tmi.lastTimeUsed;

    if(lasttimeused>texbindnumber)
      age=texbindnumber+(UINT_MAX-lasttimeused+1); /* TO DO: check */
    else
      age=texbindnumber-lasttimeused;

    if(age>=oldestage) {
      oldestage=age;
      oldesttexobj=tmalloc->tObj;
    }

    tmalloc=tmalloc->next;
  }

  return oldesttexobj;
}

static GLboolean fxTMFreeOldTMBlock(fxMesaContext fxMesa, GLint tmu)
{
  struct gl_texture_object *oldesttexobj;

  if(!fxMesa->tmAlloc[tmu])
    return GL_FALSE;

  oldesttexobj=fxTMFindOldestTMBlock(fxMesa,fxMesa->tmAlloc[tmu],fxMesa->texBindNumber);

  fxTMMoveOutTM(fxMesa,oldesttexobj);

  return GL_TRUE;
}

static tfxTMFreeNode *fxTMExtractTMFreeBlock(tfxTMFreeNode *tmfree, int texmemsize,
					     GLboolean *success, FxU32 *startadr)
{
  int blocksize;

  /* TO DO: cut recursion */

  if(!tmfree) {
    *success=GL_FALSE;
    return NULL;
  }

  blocksize=(int)tmfree->endAddress-(int)tmfree->startAddress+1;

  if(blocksize==texmemsize) {
    tfxTMFreeNode *nexttmfree;

    *success=GL_TRUE;
    *startadr=tmfree->startAddress;

    nexttmfree=tmfree->next;
    FREE(tmfree);

    return nexttmfree;
  }

  if(blocksize>texmemsize) {
    *success=GL_TRUE;
    *startadr=tmfree->startAddress;

    tmfree->startAddress+=texmemsize;

    return tmfree;
  }

  tmfree->next=fxTMExtractTMFreeBlock(tmfree->next,texmemsize,success,startadr);

  return tmfree;
}

static tfxTMAllocNode *fxTMGetTMBlock(fxMesaContext fxMesa, struct gl_texture_object *tObj,
				      GLint tmu, int texmemsize)
{
  tfxTMFreeNode *newtmfree;
  tfxTMAllocNode *newtmalloc;
  GLboolean success;
  FxU32 startadr;

  for(;;) { /* TO DO: improve performaces */
    newtmfree=fxTMExtractTMFreeBlock(fxMesa->tmFree[tmu],texmemsize,&success,&startadr);

    if(success) {
      fxMesa->tmFree[tmu]=newtmfree;

      fxMesa->freeTexMem[tmu]-=texmemsize;

      if(!(newtmalloc=MALLOC(sizeof(tfxTMAllocNode)))) {
	fprintf(stderr,"fx Driver: out of memory !\n");
	fxCloseHardware();
	exit(-1);
      }
      
      newtmalloc->next=fxMesa->tmAlloc[tmu];
      newtmalloc->startAddress=startadr;
      newtmalloc->endAddress=startadr+texmemsize-1;
      newtmalloc->tObj=tObj;

      fxMesa->tmAlloc[tmu]=newtmalloc;

      return newtmalloc;
    }

    if(!fxTMFreeOldTMBlock(fxMesa,tmu)) {
      fprintf(stderr,"fx Driver: internal error in fxTMGetTMBlock()\n");
      fprintf(stderr,"           TMU: %d Size: %d\n",tmu,texmemsize);
    
      fxCloseHardware();
      exit(-1);
    }
  }
}

void fxTMMoveInTM_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj, GLint where)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  int i,l;
  int texmemsize;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxTMMoveInTM(%d)\n",tObj->Name);
  }

  fxMesa->stats.reqTexUpload++;

  if(!ti->validated) {
    fprintf(stderr,"fx Driver: internal error in fxTMMoveInTM() -> not validated\n");
    fxCloseHardware();
    exit(-1);
  }

  if(ti->tmi.isInTM)
    return;

  if (MESA_VERBOSE&(VERBOSE_DRIVER|VERBOSE_TEXTURE)) {
     fprintf(stderr,"fxmesa: downloading %x (%d) in texture memory in %d\n",(GLuint)tObj,tObj->Name,where);
  }

  ti->tmi.whichTMU=(FxU32)where;

  switch(where) {
  case FX_TMU0:
  case FX_TMU1:
    texmemsize=(int)FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_BOTH,
						      &(ti->info));
    ti->tmi.tm[where]=fxTMGetTMBlock(fxMesa,tObj,where,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    for(i=FX_largeLodValue(ti->info),l=ti->minLevel;i<=FX_smallLodValue(ti->info);i++,l++)
      FX_grTexDownloadMipMapLevel_NoLock(where,
					 ti->tmi.tm[where]->startAddress,
					 FX_valueToLod(i),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_BOTH,
					 ti->tmi.mipmapLevel[l].data);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    texmemsize=(int)FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_ODD,
						      &(ti->info));
    ti->tmi.tm[FX_TMU0]=fxTMGetTMBlock(fxMesa,tObj,FX_TMU0,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    texmemsize=(int)FX_grTexTextureMemRequired_NoLock(GR_MIPMAPLEVELMASK_EVEN,
						      &(ti->info));
    ti->tmi.tm[FX_TMU1]=fxTMGetTMBlock(fxMesa,tObj,FX_TMU1,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    for(i=FX_largeLodValue(ti->info),l=ti->minLevel;i<=FX_smallLodValue(ti->info);i++,l++) {
      FX_grTexDownloadMipMapLevel_NoLock(GR_TMU0,
					 ti->tmi.tm[FX_TMU0]->startAddress,
					 FX_valueToLod(i),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_ODD,
					 ti->tmi.mipmapLevel[l].data);

      FX_grTexDownloadMipMapLevel_NoLock(GR_TMU1,
					 ti->tmi.tm[FX_TMU1]->startAddress,
					 FX_valueToLod(i),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_EVEN,
					 ti->tmi.mipmapLevel[l].data);
    }
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxTMMoveInTM() -> wrong tmu (%d)\n",where);
    fxCloseHardware();
    exit(-1);
  }

  fxMesa->stats.texUpload++;

  ti->tmi.isInTM=GL_TRUE;
}

void fxTMMoveInTM(fxMesaContext fxMesa, struct gl_texture_object *tObj, GLint where) {
  BEGIN_BOARD_LOCK();
  fxTMMoveInTM_NoLock(fxMesa, tObj, where);
  END_BOARD_LOCK();
}

void fxTMReloadMipMapLevel(fxMesaContext fxMesa, struct gl_texture_object *tObj, GLint level)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  GrLOD_t lodlevel;
  GLint tmu;

  if(!ti->validated) {
    fprintf(stderr,"fx Driver: internal error in fxTMReloadMipMapLevel() -> not validated\n");
    fxCloseHardware();
    exit(-1);
  }

  tmu=(int)ti->tmi.whichTMU;
  fxTMMoveInTM(fxMesa,tObj,tmu);

  fxTexGetInfo(ti->tmi.mipmapLevel[0].width,ti->tmi.mipmapLevel[0].height,
	       &lodlevel,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

  switch(tmu) {
  case FX_TMU0:
  case FX_TMU1:
    FX_grTexDownloadMipMapLevel(tmu,
			     ti->tmi.tm[tmu]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
			     FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			     ti->info.format,GR_MIPMAPLEVELMASK_BOTH,
			     ti->tmi.mipmapLevel[level].data);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    FX_grTexDownloadMipMapLevel(GR_TMU0,
			     ti->tmi.tm[GR_TMU0]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
			     FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			     ti->info.format,GR_MIPMAPLEVELMASK_ODD,
			     ti->tmi.mipmapLevel[level].data);
    
    FX_grTexDownloadMipMapLevel(GR_TMU1,
			     ti->tmi.tm[GR_TMU1]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
			     FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			     ti->info.format,GR_MIPMAPLEVELMASK_EVEN,
			     ti->tmi.mipmapLevel[level].data);
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxTMReloadMipMapLevel() -> wrong tmu (%d)\n",tmu);
    fxCloseHardware();
    exit(-1);
  }
}

void fxTMReloadSubMipMapLevel(fxMesaContext fxMesa, struct gl_texture_object *tObj,
			      GLint level, GLint yoffset, GLint height)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  GrLOD_t lodlevel;
  unsigned short *data;
  GLint tmu;

  if(!ti->validated) {
    fprintf(stderr,"fx Driver: internal error in fxTMReloadSubMipMapLevel() -> not validated\n");
    fxCloseHardware();
    exit(-1);
  }

  tmu=(int)ti->tmi.whichTMU;
  fxTMMoveInTM(fxMesa,tObj,tmu);

  fxTexGetInfo(ti->tmi.mipmapLevel[0].width,ti->tmi.mipmapLevel[0].height,
	       &lodlevel,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

  if((ti->info.format==GR_TEXFMT_INTENSITY_8) ||
     (ti->info.format==GR_TEXFMT_P_8) ||
     (ti->info.format==GR_TEXFMT_ALPHA_8))
    data=ti->tmi.mipmapLevel[level].data+((yoffset*ti->tmi.mipmapLevel[level].width)>>1);
  else
    data=ti->tmi.mipmapLevel[level].data+yoffset*ti->tmi.mipmapLevel[level].width;

  switch(tmu) {
  case FX_TMU0:
  case FX_TMU1:
    FX_grTexDownloadMipMapLevelPartial(tmu,
				    ti->tmi.tm[tmu]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
				    FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
				    ti->info.format,GR_MIPMAPLEVELMASK_BOTH,
				    data,
				    yoffset,yoffset+height-1);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    FX_grTexDownloadMipMapLevelPartial(GR_TMU0,
				    ti->tmi.tm[FX_TMU0]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
				    FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
				    ti->info.format,GR_MIPMAPLEVELMASK_ODD,
				    data,
				    yoffset,yoffset+height-1);

    FX_grTexDownloadMipMapLevelPartial(GR_TMU1,
				    ti->tmi.tm[FX_TMU1]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
				    FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
				    ti->info.format,GR_MIPMAPLEVELMASK_EVEN,
				    data,
				    yoffset,yoffset+height-1);
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxTMReloadSubMipMapLevel() -> wrong tmu (%d)\n",tmu);
    fxCloseHardware();
    exit(-1);
  }
}

static tfxTMAllocNode *fxTMFreeTMAllocBlock(tfxTMAllocNode *tmalloc,
					    tfxTMAllocNode *tmunalloc)
{
  if(!tmalloc)
    return NULL;

  if(tmalloc==tmunalloc) {
    tfxTMAllocNode *newtmalloc;

    newtmalloc=tmalloc->next;
    FREE(tmalloc);

    return newtmalloc;
  }

  tmalloc->next=fxTMFreeTMAllocBlock(tmalloc->next,tmunalloc);

  return tmalloc;
}

static tfxTMFreeNode *fxTMAddTMFree(tfxTMFreeNode *tmfree, FxU32 startadr, FxU32 endadr)
{
  if(!tmfree)
    return fxTMNewTMFreeNode(startadr,endadr);

  if((endadr+1==tmfree->startAddress) && (tmfree->startAddress & 0x1fffff)) {
    tmfree->startAddress=startadr;

    return tmfree;
  }

  if((startadr-1==tmfree->endAddress) && (startadr & 0x1fffff)) {
    tmfree->endAddress=endadr;

    if((tmfree->next && (endadr+1==tmfree->next->startAddress) &&
        (tmfree->next->startAddress & 0x1fffff))) {
      tfxTMFreeNode *nexttmfree;

      tmfree->endAddress=tmfree->next->endAddress;

      nexttmfree=tmfree->next->next;
      FREE(tmfree->next);

      tmfree->next=nexttmfree;
    }


    return tmfree;
  }

  if(startadr<tmfree->startAddress) {
    tfxTMFreeNode *newtmfree;

    newtmfree=fxTMNewTMFreeNode(startadr,endadr);
    newtmfree->next=tmfree;

    return newtmfree;
  }

  tmfree->next=fxTMAddTMFree(tmfree->next,startadr,endadr);

  return tmfree;
}

static void fxTMFreeTMBlock(fxMesaContext fxMesa, GLint tmu, tfxTMAllocNode *tmalloc)
{
  FxU32 startadr,endadr;

  startadr=tmalloc->startAddress;
  endadr=tmalloc->endAddress;

  fxMesa->tmAlloc[tmu]=fxTMFreeTMAllocBlock(fxMesa->tmAlloc[tmu],tmalloc);

  fxMesa->tmFree[tmu]=fxTMAddTMFree(fxMesa->tmFree[tmu],startadr,endadr);

  fxMesa->freeTexMem[tmu]+=endadr-startadr+1;
}

void fxTMMoveOutTM(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxTMMoveOutTM(%x (%d))\n",(GLuint)tObj,tObj->Name);
  }

  if(!ti->tmi.isInTM)
    return;

  switch(ti->tmi.whichTMU) {
  case FX_TMU0:
  case FX_TMU1:
    fxTMFreeTMBlock(fxMesa,(int)ti->tmi.whichTMU,ti->tmi.tm[ti->tmi.whichTMU]);
    break;
  case FX_TMU_SPLIT:
    fxTMFreeTMBlock(fxMesa,FX_TMU0,ti->tmi.tm[FX_TMU0]);
    fxTMFreeTMBlock(fxMesa,FX_TMU1,ti->tmi.tm[FX_TMU1]);
    break;
  default:
    fprintf(stderr,"fx Driver: internal error in fxTMMoveOutTM()\n");
    fxCloseHardware();
    exit(-1);
  }

  ti->tmi.whichTMU=FX_TMU_NONE;
  ti->tmi.isInTM=GL_FALSE;
}

void fxTMFreeTexture(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  int i;

  fxTMMoveOutTM(fxMesa,tObj);

  for(i=0;i<MAX_TEXTURE_LEVELS;i++) {
    if(ti->tmi.mipmapLevel[i].used &&
       ti->tmi.mipmapLevel[i].translated)
      FREE(ti->tmi.mipmapLevel[i].data);

    (void)ti->tmi.mipmapLevel[i].data;
  }
}

void fxTMFreeAllFreeNode(tfxTMFreeNode *fn)
{
  if(!fn)
    return;

  if(fn->next)
    fxTMFreeAllFreeNode(fn->next);

  FREE(fn);
}

void fxTMFreeAllAllocNode(tfxTMAllocNode *an)
{
  if(!an)
    return;

  if(an->next)
    fxTMFreeAllAllocNode(an->next);

  FREE(an);
}

void fxTMClose(fxMesaContext fxMesa)
{
  fxTMFreeAllFreeNode(fxMesa->tmFree[FX_TMU0]);
  fxTMFreeAllAllocNode(fxMesa->tmAlloc[FX_TMU0]);
  fxMesa->tmFree[FX_TMU0] = NULL;
  fxMesa->tmAlloc[FX_TMU0] = NULL;
  if(fxMesa->haveTwoTMUs) {
    fxTMFreeAllFreeNode(fxMesa->tmFree[FX_TMU1]);
    fxTMFreeAllAllocNode(fxMesa->tmAlloc[FX_TMU1]);
    fxMesa->tmFree[FX_TMU1] = NULL;
    fxMesa->tmAlloc[FX_TMU1] = NULL;
  }
}

void fxTMRestore_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
  tfxTexInfo *ti=(tfxTexInfo *)tObj->DriverData;
  int i,l, where;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
     fprintf(stderr,"fxmesa: fxRestore(%d)\n",tObj->Name);
  }

  if (!ti->validated) {
    fprintf(stderr,"fxDriver: internal error in fxRestore -> not validated\n");
    fxCloseHardware();
    exit(-1);
  }

  where=ti->tmi.whichTMU;
  if (MESA_VERBOSE&(VERBOSE_DRIVER|VERBOSE_TEXTURE)) {
    fprintf(stderr,"fxmesa: reloading %x (%d) in texture memory in %d\n",(GLuint)tObj,tObj->Name,where);
  }

  switch(where) {
  case FX_TMU0:
  case FX_TMU1:
    for (i=FX_largeLodValue_NoLock(ti->info), l=ti->minLevel;
	 i<=FX_smallLodValue_NoLock(ti->info);
	 i++,l++)
      if (ti->tmi.mipmapLevel[l].data)
	FX_grTexDownloadMipMapLevel_NoLock(where,
					   ti->tmi.tm[where]->startAddress,
					   FX_valueToLod(i),
					   FX_largeLodLog2(ti->info),
					   FX_aspectRatioLog2(ti->info),
					   ti->info.format,
					   GR_MIPMAPLEVELMASK_BOTH,
					   ti->tmi.mipmapLevel[l].data);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    for (i=FX_largeLodValue_NoLock(ti->info),l=ti->minLevel;
	 i<=FX_smallLodValue_NoLock(ti->info);
	 i++,l++) {
      if (ti->tmi.mipmapLevel[l].data)
	FX_grTexDownloadMipMapLevel_NoLock(GR_TMU0,
					   ti->tmi.tm[FX_TMU0]->startAddress,
					   FX_valueToLod(i),
					   FX_largeLodLog2(ti->info),
					   FX_aspectRatioLog2(ti->info),
					   ti->info.format,
					   GR_MIPMAPLEVELMASK_ODD,
					   ti->tmi.mipmapLevel[l].data);
      if (ti->tmi.mipmapLevel[l].data)
	FX_grTexDownloadMipMapLevel_NoLock(GR_TMU1,
					   ti->tmi.tm[FX_TMU1]->startAddress,
					   FX_valueToLod(i),
					   FX_largeLodLog2(ti->info),
					   FX_aspectRatioLog2(ti->info),
					   ti->info.format,
					   GR_MIPMAPLEVELMASK_EVEN,
					   ti->tmi.mipmapLevel[l].data);
    }
    break;
  default:
    fprintf(stderr,"fxDriver: internal error in fxRestore -> bad tmu (%d)\n",
	    where);
    fxCloseHardware();
    exit(-1);
  }
}

void
fxTMRestoreTextures(fxMesaContext ctx) {
  tfxTexInfo *ti;
  struct gl_texture_object *tObj;
  int i;

  tObj=ctx->glCtx->Shared->TexObjectList;
  while (tObj) {
    ti=(tfxTexInfo*)tObj->DriverData;
    if (ti && ti->tmi.isInTM) {
      for (i=0; i<MAX_TEXTURE_UNITS; i++)
	if (ctx->glCtx->Texture.Unit[i].Current==tObj) {
	  /* Force the texture onto the board, as it could be in use */
	  fxTMRestore_NoLock(ctx, tObj);
	  break;
	}
      if (i==MAX_TEXTURE_UNITS) /* Mark the texture as off the board */
	fxTMMoveOutTM(ctx, tObj);
    }
    tObj=tObj->Next;
  }
  ctx->lastUnitsMode=0;
  fxSetupTexture_NoLock(ctx->glCtx);
}

#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_texman(void)
{
  return 0;
}

#endif  /* FX */
