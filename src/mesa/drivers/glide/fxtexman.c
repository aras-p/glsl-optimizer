/* -*- mode: C; tab-width:8;  -*-

             fxtexman.c - 3Dfx VooDoo texture memory functions
*/

/*
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
 *
 * See the file fxapi.c for more informations about authors
 *
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "fxdrv.h"

static tfxTMFreeNode *fxTMNewTMFreeNode(FxU32 start, FxU32 end)
{
  tfxTMFreeNode *tmn;

  if(!(tmn=malloc(sizeof(tfxTMFreeNode)))) {
    fprintf(stderr,"fx Driver: out of memory !\n");
    fxCloseHardware();
    exit(-1);
  }

  tmn->next=NULL;
  tmn->startAddress=start;
  tmn->endAddress=end;

  return tmn;
}

static void fxTMUInit(fxMesaContext fxMesa, int tmu)
{
  tfxTMFreeNode *tmn,*tmntmp;
  FxU32 start,end,blockstart,blockend;

  start=grTexMinAddress(tmu);
  end=grTexMaxAddress(tmu);

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
    free(tmfree);

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

      if(!(newtmalloc=malloc(sizeof(tfxTMAllocNode)))) {
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

void fxTMMoveInTM(fxMesaContext fxMesa, struct gl_texture_object *tObj, GLint where)
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
    texmemsize=(int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&(ti->info));
    ti->tmi.tm[where]=fxTMGetTMBlock(fxMesa,tObj,where,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    for(i=FX_largeLodValue(ti->info),l=ti->minLevel;i<=FX_smallLodValue(ti->info);i++,l++)
      grTexDownloadMipMapLevel(where,
			       ti->tmi.tm[where]->startAddress,FX_valueToLod(i),
			       FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			       ti->info.format,GR_MIPMAPLEVELMASK_BOTH,
			       ti->tmi.mipmapLevel[l].data);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    texmemsize=(int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD,&(ti->info));
    ti->tmi.tm[FX_TMU0]=fxTMGetTMBlock(fxMesa,tObj,FX_TMU0,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    texmemsize=(int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN,&(ti->info));
    ti->tmi.tm[FX_TMU1]=fxTMGetTMBlock(fxMesa,tObj,FX_TMU1,texmemsize);
    fxMesa->stats.memTexUpload+=texmemsize;

    for(i=FX_largeLodValue(ti->info),l=ti->minLevel;i<=FX_smallLodValue(ti->info);i++,l++) {
      grTexDownloadMipMapLevel(GR_TMU0,ti->tmi.tm[FX_TMU0]->startAddress,FX_valueToLod(i),
			       FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			       ti->info.format,GR_MIPMAPLEVELMASK_ODD,
			       ti->tmi.mipmapLevel[l].data);

      grTexDownloadMipMapLevel(GR_TMU1,ti->tmi.tm[FX_TMU1]->startAddress,FX_valueToLod(i),
			       FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			       ti->info.format,GR_MIPMAPLEVELMASK_EVEN,
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
    grTexDownloadMipMapLevel(tmu,
			     ti->tmi.tm[tmu]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
			     FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			     ti->info.format,GR_MIPMAPLEVELMASK_BOTH,
			     ti->tmi.mipmapLevel[level].data);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    grTexDownloadMipMapLevel(GR_TMU0,
			     ti->tmi.tm[GR_TMU0]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
			     FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
			     ti->info.format,GR_MIPMAPLEVELMASK_ODD,
			     ti->tmi.mipmapLevel[level].data);
    
    grTexDownloadMipMapLevel(GR_TMU1,
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
    grTexDownloadMipMapLevelPartial(tmu,
				    ti->tmi.tm[tmu]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
				    FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
				    ti->info.format,GR_MIPMAPLEVELMASK_BOTH,
				    data,
				    yoffset,yoffset+height-1);
    break;
  case FX_TMU_SPLIT: /* TO DO: alternate even/odd TMU0/TMU1 */
    grTexDownloadMipMapLevelPartial(GR_TMU0,
				    ti->tmi.tm[FX_TMU0]->startAddress,FX_valueToLod(FX_lodToValue(lodlevel)+level),
				    FX_largeLodLog2(ti->info),FX_aspectRatioLog2(ti->info),
				    ti->info.format,GR_MIPMAPLEVELMASK_ODD,
				    data,
				    yoffset,yoffset+height-1);

    grTexDownloadMipMapLevelPartial(GR_TMU1,
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
    free(tmalloc);

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
      free(tmfree->next);

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
      free(ti->tmi.mipmapLevel[i].data);

    (void)ti->tmi.mipmapLevel[i].data;
  }
}

void fxTMFreeAllFreeNode(tfxTMFreeNode *fn)
{
  if(!fn)
    return;

  if(fn->next)
    fxTMFreeAllFreeNode(fn->next);

  free(fn);
}

void fxTMFreeAllAllocNode(tfxTMAllocNode *an)
{
  if(!an)
    return;

  if(an->next)
    fxTMFreeAllAllocNode(an->next);

  free(an);
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


#else


/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_texman(void)
{
  return 0;
}

#endif  /* FX */
