/* $Id: fxglidew.c,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

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
 */
 
#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "glide.h"
#include "fxglidew.h"
#include "fxdrv.h"

#include <stdlib.h>
#include <string.h>


FxI32 FX_grGetInteger(FxU32 pname)
{
#if !defined(FX_GLIDE3)
  switch (pname) 
  {
    case FX_FOG_TABLE_ENTRIES:
       return GR_FOG_TABLE_SIZE;
    case FX_GLIDE_STATE_SIZE:
       return sizeof(GrState);
    case FX_LFB_PIXEL_PIPE:
       return FXFALSE;
    case FX_PENDING_BUFFERSWAPS:
       return grBufferNumPending();
    default:
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
          fprintf(stderr,"Wrong parameter in FX_grGetInteger!\n");
          return -1;
       }
  }
#else
  FxU32 grname;
  FxI32 result;
  
  switch (pname)
  {
     case FX_FOG_TABLE_ENTRIES:
     case FX_GLIDE_STATE_SIZE:
     case FX_LFB_PIXEL_PIPE:
     case FX_PENDING_BUFFERSWAPS:
       grname = pname;
       break;
     default:
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
          fprintf(stderr,"Wrong parameter in FX_grGetInteger!\n");
          return -1;
       }
  }
  
  grGet(grname,4,&result);
  return result;
#endif
}



#if defined(FX_GLIDE3)

void FX_grGammaCorrectionValue(float val)
{
  (void)val;
/* ToDo */
}

void FX_grSstControl(int par)
{
  (void)par;
  /* ToDo */
}
int FX_getFogTableSize(void)
{
   int result;
   grGet(GR_FOG_TABLE_ENTRIES,sizeof(int),(void*)&result);
   return result; 
}

int FX_getGrStateSize(void)
{
   int result;
   grGet(GR_GLIDE_STATE_SIZE,sizeof(int),(void*)&result);
   
   return result;
   
}
int FX_grBufferNumPending()
{
   int result;
   grGet(GR_PENDING_BUFFERSWAPS,sizeof(int),(void*)&result);
   
   return result;
}

int FX_grSstScreenWidth()
{
   FxI32 result[4];
   
   grGet(GR_VIEWPORT,sizeof(FxI32)*4,result);
   
   return result[2];
}

int FX_grSstScreenHeight()
{
   FxI32 result[4];
   
   grGet(GR_VIEWPORT,sizeof(FxI32)*4,result);
   
   return result[3];
}

void FX_grGlideGetVersion(char *buf)
{
   strcpy(buf,grGetString(GR_VERSION));
}

void FX_grSstPerfStats(GrSstPerfStats_t *st)
{
  /* ToDo */
  st->pixelsIn = 0;
  st->chromaFail = 0;
  st->zFuncFail = 0;
  st->aFuncFail = 0;
  st->pixelsOut = 0;
}

void FX_grAADrawLine(GrVertex *a,GrVertex *b)
{
   /* ToDo */
   grDrawLine(a,b);
}
void FX_grAADrawPoint(GrVertex *a)
{
  grDrawPoint(a);
}

void FX_setupGrVertexLayout(void)
{
   grReset(GR_VERTEX_PARAMETER);
   
   grCoordinateSpace(GR_WINDOW_COORDS);
   grVertexLayout(GR_PARAM_XY,  	GR_VERTEX_X_OFFSET << 2, 	GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_RGB, 	GR_VERTEX_R_OFFSET << 2, 	GR_PARAM_ENABLE);
 /*  grVertexLayout(GR_PARAM_Z,   	GR_VERTEX_Z_OFFSET << 2, 	GR_PARAM_ENABLE); */
   grVertexLayout(GR_PARAM_A,   	GR_VERTEX_A_OFFSET << 2, 	GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_Q,		GR_VERTEX_OOW_OFFSET << 2,	GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_Z,           GR_VERTEX_OOZ_OFFSET << 2, 	GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_ST0, 	GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);	
   grVertexLayout(GR_PARAM_Q0,  	GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE); 
   grVertexLayout(GR_PARAM_ST1, 	GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);	
   grVertexLayout(GR_PARAM_Q1,  	GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);	
}

void FX_grHints(GrHint_t hintType, FxU32 hintMask)
{
   switch(hintType) {
      case GR_HINT_STWHINT:
      {
        if (hintMask & GR_STWHINT_W_DIFF_TMU0)
           grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2, 	GR_PARAM_ENABLE);
        else
           grVertexLayout(GR_PARAM_Q0,GR_VERTEX_OOW_TMU0_OFFSET << 2, 	GR_PARAM_DISABLE);
           
        if (hintMask & GR_STWHINT_ST_DIFF_TMU1)
            grVertexLayout(GR_PARAM_ST1,GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_ENABLE);
        else
            grVertexLayout(GR_PARAM_ST1,GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
        
        if (hintMask & GR_STWHINT_W_DIFF_TMU1)
            grVertexLayout(GR_PARAM_Q1,GR_VERTEX_OOW_TMU1_OFFSET << 2,	GR_PARAM_ENABLE);
        else
            grVertexLayout(GR_PARAM_Q1,GR_VERTEX_OOW_TMU1_OFFSET << 2,	GR_PARAM_DISABLE);
      	
      }
   }
}
int FX_grSstQueryHardware(GrHwConfiguration *config)
{
   int i,j;
   int numFB;
   grGet(GR_NUM_BOARDS,4,(void*)&(config->num_sst));
   if (config->num_sst == 0)
   	return 0;
   for (i = 0; i< config->num_sst; i++)
   {
      config->SSTs[i].type = GR_SSTTYPE_VOODOO;
      grSstSelect(i);
      grGet(GR_MEMORY_FB,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.fbRam));
      config->SSTs[i].sstBoard.VoodooConfig.fbRam/= 1024*1024;
      
      grGet(GR_NUM_TMU,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.nTexelfx));
   
      
      grGet(GR_NUM_FB,4,(void*)&numFB);
      if (numFB > 1)
         config->SSTs[i].sstBoard.VoodooConfig.sliDetect = FXTRUE;
      else
         config->SSTs[i].sstBoard.VoodooConfig.sliDetect = FXFALSE;
      for (j = 0; j < config->SSTs[i].sstBoard.VoodooConfig.nTexelfx; j++)
      {
      	 grGet(GR_MEMORY_TMU,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.tmuConfig[i].tmuRam));
      }
   }
   return 1;
}


#endif 
#else

/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_glidew(void)
{
  return 0;
}

#endif /* FX */
