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

#include <stdio.h>

#include "glheader.h"
#include "buffers.h"
#include "context.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"
#include "dd.h"

#include "mm.h"
#include "via_context.h"
#include "via_state.h"
#include "via_tex.h"
#include "via_tris.h"
#include "via_ioctl.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"


static GLuint ROP[16] = {
    HC_HROP_BLACK,    /* GL_CLEAR           0                      	*/
    HC_HROP_DPa,      /* GL_AND             s & d                  	*/
    HC_HROP_PDna,     /* GL_AND_REVERSE     s & ~d  			*/
    HC_HROP_P,        /* GL_COPY            s                       	*/
    HC_HROP_DPna,     /* GL_AND_INVERTED    ~s & d                      */
    HC_HROP_D,        /* GL_NOOP            d  		                */
    HC_HROP_DPx,      /* GL_XOR             s ^ d                       */
    HC_HROP_DPo,      /* GL_OR              s | d                       */
    HC_HROP_DPon,     /* GL_NOR             ~(s | d)                    */
    HC_HROP_DPxn,     /* GL_EQUIV           ~(s ^ d)                    */
    HC_HROP_Dn,       /* GL_INVERT          ~d                       	*/
    HC_HROP_PDno,     /* GL_OR_REVERSE      s | ~d                      */
    HC_HROP_Pn,       /* GL_COPY_INVERTED   ~s                       	*/
    HC_HROP_DPno,     /* GL_OR_INVERTED     ~s | d                      */
    HC_HROP_DPan,     /* GL_NAND            ~(s & d)                    */
    HC_HROP_WHITE     /* GL_SET             1                       	*/
};



void viaEmitState(viaContextPtr vmesa)
{
   GLcontext *ctx = vmesa->glCtx;
   GLuint i = 0;
   GLuint j = 0;
   RING_VARS;

   if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);

   viaCheckDma(vmesa, 0x110);
    
   BEGIN_RING(5);
   OUT_RING( HC_HEADER2 );
   OUT_RING( (HC_ParaType_NotTex << 16) );
   OUT_RING( ((HC_SubA_HEnable << 24) | vmesa->regEnable) );
   OUT_RING( ((HC_SubA_HFBBMSKL << 24) | vmesa->regHFBBMSKL) );    
   OUT_RING( ((HC_SubA_HROP << 24) | vmesa->regHROP) );        
   ADVANCE_RING();
    
   if (vmesa->have_hw_stencil) {
      GLuint pitch, format, offset;
	
      format = HC_HZWBFM_24;	    	
      offset = vmesa->depth.offset;
      pitch = vmesa->depth.pitch;
	
      BEGIN_RING(6);
      OUT_RING( ((HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF)) );
      OUT_RING( ((HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24)) );	
      OUT_RING( ((HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK | 
	         format | pitch) );            
      OUT_RING( ((HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD) );
      OUT_RING( ((HC_SubA_HSTREF << 24) | vmesa->regHSTREF) );
      OUT_RING( ((HC_SubA_HSTMD << 24) | vmesa->regHSTMD) );
      ADVANCE_RING();
   }
   else if (vmesa->hasDepth) {
      GLuint pitch, format, offset;
	
      if (vmesa->depthBits == 16) {
	 /* We haven't support 16bit depth yet */
	 format = HC_HZWBFM_16;
	 /*format = HC_HZWBFM_32;*/
	 if (VIA_DEBUG) fprintf(stderr, "z format = 16\n");
      }	    
      else {
	 format = HC_HZWBFM_32;
	 if (VIA_DEBUG) fprintf(stderr, "z format = 32\n");
      }
	    
	    
      offset = vmesa->depth.offset;
      pitch = vmesa->depth.pitch;
	
      BEGIN_RING(4);
      OUT_RING( ((HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF)) );
      OUT_RING( ((HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24)) );	
      OUT_RING( ((HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK | 
	         format | pitch) );            
      OUT_RING( ((HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD) );
      ADVANCE_RING();
   }
    
   if (ctx->Color.AlphaEnabled) {
      BEGIN_RING(1);
      OUT_RING( ((HC_SubA_HATMD << 24) | vmesa->regHATMD) );
      ADVANCE_RING();
      i++;
   }   

   if (ctx->Color.BlendEnabled) {
      BEGIN_RING(11);
      OUT_RING( ((HC_SubA_HABLCsat << 24) | vmesa->regHABLCsat) );
      OUT_RING( ((HC_SubA_HABLCop  << 24) | vmesa->regHABLCop) ); 
      OUT_RING( ((HC_SubA_HABLAsat << 24) | vmesa->regHABLAsat) );        
      OUT_RING( ((HC_SubA_HABLAop  << 24) | vmesa->regHABLAop) ); 
      OUT_RING( ((HC_SubA_HABLRCa  << 24) | vmesa->regHABLRCa) ); 
      OUT_RING( ((HC_SubA_HABLRFCa << 24) | vmesa->regHABLRFCa) );        
      OUT_RING( ((HC_SubA_HABLRCbias << 24) | vmesa->regHABLRCbias) );    
      OUT_RING( ((HC_SubA_HABLRCb  << 24) | vmesa->regHABLRCb) ); 
      OUT_RING( ((HC_SubA_HABLRFCb << 24) | vmesa->regHABLRFCb) );        
      OUT_RING( ((HC_SubA_HABLRAa  << 24) | vmesa->regHABLRAa) ); 
      OUT_RING( ((HC_SubA_HABLRAb  << 24) | vmesa->regHABLRAb) ); 
      ADVANCE_RING();
   }
    
   if (ctx->Fog.Enabled) {
      BEGIN_RING(3);
      OUT_RING( ((HC_SubA_HFogLF << 24) | vmesa->regHFogLF) );        
      OUT_RING( ((HC_SubA_HFogCL << 24) | vmesa->regHFogCL) );            
      OUT_RING( ((HC_SubA_HFogCH << 24) | vmesa->regHFogCH) );            
      ADVANCE_RING();
   }
    
   if (0 && ctx->Line.StippleFlag) {
      BEGIN_RING(2);
      OUT_RING( ((HC_SubA_HLP << 24) | ctx->Line.StipplePattern) );           
      OUT_RING( ((HC_SubA_HLPRF << 24) | ctx->Line.StippleFactor) );                  
      ADVANCE_RING();
   }
   else {
      BEGIN_RING(2);
      OUT_RING( ((HC_SubA_HLP << 24) | 0xFFFF) );         
      OUT_RING( ((HC_SubA_HLPRF << 24) | 0x1) );              
      ADVANCE_RING();
   }

   BEGIN_RING(1);
   OUT_RING( ((HC_SubA_HPixGC << 24) | 0x0) );             
   ADVANCE_RING();
    
   QWORD_PAD_RING();


   if (ctx->Texture._EnabledUnits) {
    
      struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
      struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];

      {
	 GLuint nDummyValue = 0;

	 BEGIN_RING( 8 );
	 OUT_RING( HC_HEADER2 );
	 OUT_RING( (HC_ParaType_Tex << 16) | (HC_SubType_TexGeneral << 24) );

	 if (ctx->Texture._EnabledUnits > 1) {
	    if (VIA_DEBUG) fprintf(stderr, "multi texture\n");
	    nDummyValue = (HC_SubA_HTXSMD << 24) | (1 << 3);
                
	    /* Clear cache flag never set:
	     */
	    if (0) {
	       OUT_RING( nDummyValue | HC_HTXCHCLR_MASK );
	       OUT_RING( nDummyValue );
	    }
	    else {
	       OUT_RING( nDummyValue );
	       OUT_RING( nDummyValue );
	    }
	 }
	 else {
	    if (VIA_DEBUG) fprintf(stderr, "single texture\n");
	    nDummyValue = (HC_SubA_HTXSMD << 24) | 0;
                
	    if (0) {
	       OUT_RING( nDummyValue | HC_HTXCHCLR_MASK );
	       OUT_RING( nDummyValue );
	    }
	    else {
	       OUT_RING( nDummyValue );
	       OUT_RING( nDummyValue );
	    }
	 }
	 OUT_RING( HC_HEADER2 );
	 OUT_RING( HC_ParaType_NotTex << 16 );
	 OUT_RING( (HC_SubA_HEnable << 24) | vmesa->regEnable );
	 OUT_RING( (HC_SubA_HEnable << 24) | vmesa->regEnable );
	 ADVANCE_RING();
      }

      if (texUnit0->Enabled) {
	 struct gl_texture_object *texObj = texUnit0->_Current;
	 viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
	 GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	 if (VIA_DEBUG) {
	    fprintf(stderr, "texture0 enabled\n");
	    fprintf(stderr, "texture level %d\n", t->actualLevel);
	 }		
	 if (numLevels == 8) {
	    BEGIN_RING(27);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexWidthLog2[1] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexHeightLog2[1] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseH[1] );
	    OUT_RING( t->regTexBaseH[2] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[1].baseL );
	    OUT_RING( t->regTexBaseAndPitch[1].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[2].baseL );
	    OUT_RING( t->regTexBaseAndPitch[2].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[3].baseL );
	    OUT_RING( t->regTexBaseAndPitch[3].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[4].baseL );
	    OUT_RING( t->regTexBaseAndPitch[4].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[5].baseL );
	    OUT_RING( t->regTexBaseAndPitch[5].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[6].baseL );
	    OUT_RING( t->regTexBaseAndPitch[6].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[7].baseL );
	    OUT_RING( t->regTexBaseAndPitch[7].pitchLog2 );
	    ADVANCE_RING();
	 }
	 else if (numLevels > 1) {

	    BEGIN_RING(12 + numLevels * 2);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
		
	    if (numLevels > 6) {
	       OUT_RING( t->regTexWidthLog2[1] );
	       OUT_RING( t->regTexHeightLog2[1] );
	    }
                
	    OUT_RING( t->regTexBaseH[0] );
		
	    if (numLevels > 3) {
	       OUT_RING( t->regTexBaseH[1] );
	    }
	    if (numLevels > 6) {
	       OUT_RING( t->regTexBaseH[2] );
	    }
	    if (numLevels > 9)  {
	       OUT_RING( t->regTexBaseH[3] );
	    }

	    for (j = 0; j < numLevels; j++) {
	       OUT_RING( t->regTexBaseAndPitch[j].baseL );
	       OUT_RING( t->regTexBaseAndPitch[j].pitchLog2 );
	    }

	    ADVANCE_RING_VARIABLE();
	 }
	 else {

	    BEGIN_RING(9);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (0 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    ADVANCE_RING();
	 }

	 BEGIN_RING(12);
	 OUT_RING( (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB_0 );
	 OUT_RING( (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD_0 );
	 OUT_RING( (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat_0 );
	 OUT_RING( (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop_0 );
	 OUT_RING( (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog_0 );
	 OUT_RING( (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRCa << 24) | vmesa->regHTXnTBLRCa_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRCc << 24) | vmesa->regHTXnTBLRCc_0 );
	 OUT_RING( (HC_SubA_HTXnTBLRCbias << 24) | vmesa->regHTXnTBLRCbias_0 );
	 ADVANCE_RING();

	 if (t->regTexFM == HC_HTXnFM_Index8) {
	    struct gl_color_table *table = &texObj->Palette;
	    GLfloat *tableF = (GLfloat *)table->Table;

	    BEGIN_RING(2 + table->Size);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Palette << 16) | (0 << 24) );
	    for (j = 0; j < table->Size; j++) 
	       OUT_RING( tableF[j] );
	    ADVANCE_RING();
	       
	 }

	 QWORD_PAD_RING();
      }
	
      if (texUnit1->Enabled) {
	 struct gl_texture_object *texObj = texUnit1->_Current;
	 viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
	 GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	 if (VIA_DEBUG) {
	    fprintf(stderr, "texture1 enabled\n");
	    fprintf(stderr, "texture level %d\n", t->actualLevel);
	 }		
	 if (numLevels == 8) {
	    BEGIN_RING(27);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (1 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexWidthLog2[1] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexHeightLog2[1] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseH[1] );
	    OUT_RING( t->regTexBaseH[2] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[1].baseL );
	    OUT_RING( t->regTexBaseAndPitch[1].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[2].baseL );
	    OUT_RING( t->regTexBaseAndPitch[2].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[3].baseL );
	    OUT_RING( t->regTexBaseAndPitch[3].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[4].baseL );
	    OUT_RING( t->regTexBaseAndPitch[4].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[5].baseL );
	    OUT_RING( t->regTexBaseAndPitch[5].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[6].baseL );
	    OUT_RING( t->regTexBaseAndPitch[6].pitchLog2 );
	    OUT_RING( t->regTexBaseAndPitch[7].baseL );
	    OUT_RING( t->regTexBaseAndPitch[7].pitchLog2 );
	    ADVANCE_RING();
	 }
	 else if (numLevels > 1) {
	    BEGIN_RING(12 + numLevels * 2);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (1 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
		
	    if (numLevels > 6) {
	       OUT_RING( t->regTexWidthLog2[1] );
	       OUT_RING( t->regTexHeightLog2[1] );
	       i += 2;
	    }
                
	    OUT_RING( t->regTexBaseH[0] );
		
	    if (numLevels > 3) { 
	       OUT_RING( t->regTexBaseH[1] );
	    }
	    if (numLevels > 6) {
	       OUT_RING( t->regTexBaseH[2] );
	    }
	    if (numLevels > 9)  {
	       OUT_RING( t->regTexBaseH[3] );
	    }
		
	    for (j = 0; j < numLevels; j++) {
	       OUT_RING( t->regTexBaseAndPitch[j].baseL );
	       OUT_RING( t->regTexBaseAndPitch[j].pitchLog2 );
	    }
	    ADVANCE_RING_VARIABLE();
	 }
	 else {
	    BEGIN_RING(9);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Tex << 16) |  (1 << 24) );
	    OUT_RING( t->regTexFM );
	    OUT_RING( (HC_SubA_HTXnL0OS << 24) |
	       ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel );
	    OUT_RING( t->regTexWidthLog2[0] );
	    OUT_RING( t->regTexHeightLog2[0] );
	    OUT_RING( t->regTexBaseH[0] );
	    OUT_RING( t->regTexBaseAndPitch[0].baseL );
	    OUT_RING( t->regTexBaseAndPitch[0].pitchLog2 );
	    ADVANCE_RING();
	 }

	 BEGIN_RING(9);
	 OUT_RING( (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB_1 );
	 OUT_RING( (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD_1 );
	 OUT_RING( (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat_1 );
	 OUT_RING( (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop_1 );
	 OUT_RING( (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog_1 );
	 OUT_RING( (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat_1 );
	 OUT_RING( (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb_1 );
	 OUT_RING( (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa_1 );
	 OUT_RING( (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog_1 );
	 ADVANCE_RING();

	 if (t->regTexFM == HC_HTXnFM_Index8) {
	    struct gl_color_table *table = &texObj->Palette;
	    GLfloat *tableF = (GLfloat *)table->Table;

	    BEGIN_RING(2 + table->Size);
	    OUT_RING( HC_HEADER2 );
	    OUT_RING( (HC_ParaType_Palette << 16) | (1 << 24) );
	    for (j = 0; j < table->Size; j++) {
	       OUT_RING( tableF[j] );
	    }
	    ADVANCE_RING();
	 }

	 QWORD_PAD_RING();
      }
   }
    
    
   if (ctx->Polygon.StippleFlag) {
      GLuint *stipple = &ctx->PolygonStipple[0];
        
      BEGIN_RING(38);
      OUT_RING( HC_HEADER2 );             
      OUT_RING( ((HC_ParaType_Palette << 16) | (HC_SubType_Stipple << 24)) );
      OUT_RING( stipple[31] );            
      OUT_RING( stipple[30] );            
      OUT_RING( stipple[29] );            
      OUT_RING( stipple[28] );            
      OUT_RING( stipple[27] );            
      OUT_RING( stipple[26] );            
      OUT_RING( stipple[25] );            
      OUT_RING( stipple[24] );            
      OUT_RING( stipple[23] );            
      OUT_RING( stipple[22] );            
      OUT_RING( stipple[21] );            
      OUT_RING( stipple[20] );            
      OUT_RING( stipple[19] );            
      OUT_RING( stipple[18] );            
      OUT_RING( stipple[17] );            
      OUT_RING( stipple[16] );            
      OUT_RING( stipple[15] );            
      OUT_RING( stipple[14] );            
      OUT_RING( stipple[13] );            
      OUT_RING( stipple[12] );            
      OUT_RING( stipple[11] );            
      OUT_RING( stipple[10] );            
      OUT_RING( stipple[9] );             
      OUT_RING( stipple[8] );             
      OUT_RING( stipple[7] );             
      OUT_RING( stipple[6] );             
      OUT_RING( stipple[5] );             
      OUT_RING( stipple[4] );             
      OUT_RING( stipple[3] );             
      OUT_RING( stipple[2] );             
      OUT_RING( stipple[1] );             
      OUT_RING( stipple[0] );             
      OUT_RING( HC_HEADER2 );                     
      OUT_RING( (HC_ParaType_NotTex << 16) );
      OUT_RING( ((HC_SubA_HSPXYOS << 24) | (0x20 - (vmesa->driDrawable->h & 0x1F))) );
      OUT_RING( ((HC_SubA_HSPXYOS << 24) | (0x20 - (vmesa->driDrawable->h & 0x1F))) );
      ADVANCE_RING();
   }
   
   if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);

   vmesa->newEmitState = 0;
}


static __inline__ GLuint viaPackColor(GLuint format,
                                      GLubyte r, GLubyte g,
                                      GLubyte b, GLubyte a)
{
    switch (format) {
    case 0x10:
        return PACK_COLOR_565(r, g, b);
    case 0x20:
        return PACK_COLOR_8888(a, r, g, b);        
    default:
        if (VIA_DEBUG) fprintf(stderr, "unknown format %d\n", (int)format);
        return PACK_COLOR_8888(a, r, g, b);
   }
}

static void viaBlendEquationSeparate(GLcontext *ctx, GLenum rgbMode, GLenum aMode)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);

    /* GL_EXT_blend_equation_separate not supported */
    ASSERT(rgbMode == aMode);

    /* Can only do GL_ADD equation in hardware */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_BLEND_EQ, rgbMode != GL_FUNC_ADD_EXT);

    /* BlendEquation sets ColorLogicOpEnabled in an unexpected
     * manner.
     */
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_LOGICOP,
             (ctx->Color.ColorLogicOpEnabled &&
              ctx->Color.LogicOp != GL_COPY));
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

static void viaBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLboolean fallback = GL_FALSE;
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);

    switch (ctx->Color.BlendSrcRGB) {
    case GL_SRC_ALPHA_SATURATE:  
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        break;
    }

    switch (ctx->Color.BlendDstRGB) {
    case GL_CONSTANT_COLOR:
    case GL_ONE_MINUS_CONSTANT_COLOR:
    case GL_CONSTANT_ALPHA:
    case GL_ONE_MINUS_CONSTANT_ALPHA:
        fallback = GL_TRUE;
        break;
    default:
        break;
    }

    FALLBACK(vmesa, VIA_FALLBACK_BLEND_FUNC, fallback);
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

/* Shouldn't be called as the extension is disabled.
 */
static void viaBlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB,
                                 GLenum dfactorRGB, GLenum sfactorA,
                                 GLenum dfactorA)
{
    if (dfactorRGB != dfactorA || sfactorRGB != sfactorA) {
        _mesa_error(ctx, GL_INVALID_OPERATION, "glBlendEquation (disabled)");
    }

    viaBlendFunc(ctx, sfactorRGB, dfactorRGB);
}




/* =============================================================
 * Hardware clipping
 */
static void viaScissor(GLcontext *ctx, GLint x, GLint y,
                       GLsizei w, GLsizei h)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (!vmesa->driDrawable)
       return;

    if (VIA_DEBUG)
       fprintf(stderr, "%s %d,%d %dx%d, drawH %d\n", __FUNCTION__, x,y,w,h, vmesa->driDrawable->h);

    if (ctx->Scissor.Enabled) {
        VIA_FLUSH_DMA(vmesa); /* don't pipeline cliprect changes */
    }

    vmesa->scissorRect.x1 = x;
    vmesa->scissorRect.y1 = vmesa->driDrawable->h - (y + h);
    vmesa->scissorRect.x2 = x + w;
    vmesa->scissorRect.y2 = vmesa->driDrawable->h - y;
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
}

static void viaEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   viaContextPtr vmesa = VIA_CONTEXT(ctx);

   switch (cap) {
   case GL_SCISSOR_TEST:
      VIA_FLUSH_DMA(vmesa);
      break;
   default:
      break;
   }
}



/* Fallback to swrast for select and feedback.
 */
static void viaRenderMode(GLcontext *ctx, GLenum mode)
{
    FALLBACK(VIA_CONTEXT(ctx), VIA_FALLBACK_RENDERMODE, (mode != GL_RENDER));
}


static void viaDrawBuffer(GLcontext *ctx, GLenum mode)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (mode == GL_FRONT) {
        VIA_FLUSH_DMA(vmesa);
	vmesa->drawBuffer = vmesa->readBuffer = &vmesa->front;
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
        return;
    }
    else if (mode == GL_BACK) {
        VIA_FLUSH_DMA(vmesa);
	vmesa->drawBuffer = vmesa->readBuffer = &vmesa->back;
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_FALSE);
        return;
    }
    else {
        FALLBACK(vmesa, VIA_FALLBACK_DRAW_BUFFER, GL_TRUE);
        return;
    }

    viaXMesaWindowMoved(vmesa);

   /* We want to update the s/w rast state too so that r200SetBuffer()
    * gets called.
    */
   _swrast_DrawBuffer(ctx, mode);


    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);    
}

static void viaClearColor(GLcontext *ctx, const GLfloat color[4])
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLubyte pcolor[4];
    CLAMPED_FLOAT_TO_UBYTE(pcolor[0], color[0]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[1], color[1]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[2], color[2]);
    CLAMPED_FLOAT_TO_UBYTE(pcolor[3], color[3]);
    vmesa->ClearColor = viaPackColor(vmesa->viaScreen->bitsPerPixel,
                                     pcolor[0], pcolor[1],
                                     pcolor[2], pcolor[3]);
	
}

#define WRITEMASK_ALPHA_SHIFT 31
#define WRITEMASK_RED_SHIFT   30
#define WRITEMASK_GREEN_SHIFT 29
#define WRITEMASK_BLUE_SHIFT  28

static void viaColorMask(GLcontext *ctx,
			 GLboolean r, GLboolean g,
			 GLboolean b, GLboolean a)
{
   viaContextPtr vmesa = VIA_CONTEXT( ctx );

   if (VIA_DEBUG)
      fprintf(stderr, "%s r(%d) g(%d) b(%d) a(%d)\n", __FUNCTION__, r, g, b, a);

   vmesa->ClearMask = (((!r) << WRITEMASK_RED_SHIFT) |
		       ((!g) << WRITEMASK_GREEN_SHIFT) |
		       ((!b) << WRITEMASK_BLUE_SHIFT) |
		       ((!a) << WRITEMASK_ALPHA_SHIFT));
}


/* =============================================================
 */


/* Using drawXoff like this is incorrect outside of locked regions.
 * This hardware just isn't capable of private back buffers without
 * glitches and/or a hefty locking scheme.
 */
void viaCalcViewport(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    const GLfloat *v = ctx->Viewport._WindowMap.m;
    GLfloat *m = vmesa->ViewportMatrix.m;
    
    /* See also via_translate_vertex.
     */
    m[MAT_SX] =   v[MAT_SX];
    m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X + vmesa->drawXoff;
    m[MAT_SY] = - v[MAT_SY];
    m[MAT_TY] = - v[MAT_TY] + vmesa->driDrawable->h + SUBPIXEL_Y;
    m[MAT_SZ] =   v[MAT_SZ] * (1.0 / vmesa->depth_max);
    m[MAT_TZ] =   v[MAT_TZ] * (1.0 / vmesa->depth_max);
}

static void viaViewport(GLcontext *ctx,
                        GLint x, GLint y,
                        GLsizei width, GLsizei height)
{
   /* update size of Mesa/software ancillary buffers */
   _mesa_ResizeBuffersMESA();
    viaCalcViewport(ctx);
}

static void viaDepthRange(GLcontext *ctx,
                          GLclampd nearval, GLclampd farval)
{
    viaCalcViewport(ctx);
}


void viaInitState(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    vmesa->regCmdA = HC_ACMD_HCmdA;
    vmesa->regCmdB = HC_ACMD_HCmdB;
    vmesa->regEnable = HC_HenCW_MASK;

   /* Mesa should do this for us:
    */

   ctx->Driver.BlendEquationSeparate( ctx, 
				      ctx->Color.BlendEquationRGB,
				      ctx->Color.BlendEquationA);

   ctx->Driver.BlendFuncSeparate( ctx,
				  ctx->Color.BlendSrcRGB,
				  ctx->Color.BlendDstRGB,
				  ctx->Color.BlendSrcA,
				  ctx->Color.BlendDstA);

   ctx->Driver.Scissor( ctx, ctx->Scissor.X, ctx->Scissor.Y,
			ctx->Scissor.Width, ctx->Scissor.Height );

   ctx->Driver.DrawBuffer( ctx, ctx->Color.DrawBuffer[0] );
}

/**
 * Convert S and T texture coordinate wrap modes to hardware bits.
 */
static u_int32_t
get_wrap_mode( GLenum sWrap, GLenum tWrap )
{
    u_int32_t v = 0;


    switch( sWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Srepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Sclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Smirror;
	break;
    }

    switch( tWrap ) {
    case GL_REPEAT:
	v |= HC_HTXnMPMD_Trepeat;
	break;
    case GL_CLAMP:
    case GL_CLAMP_TO_EDGE:
	v |= HC_HTXnMPMD_Tclamp;
	break;
    case GL_MIRRORED_REPEAT:
	v |= HC_HTXnMPMD_Tmirror;
	break;
    }
    
    return v;
}


static void viaChooseTextureState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
    struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];
    /*=* John Sheng [2003.7.18] texture combine *=*/

    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (texUnit0->_ReallyEnabled || texUnit1->_ReallyEnabled) {
	if (VIA_DEBUG) {
	    fprintf(stderr, "Texture._ReallyEnabled - in\n");    
	    fprintf(stderr, "texUnit0->_ReallyEnabled = %x\n",texUnit0->_ReallyEnabled);
	}

	if (VIA_DEBUG) {
            struct gl_texture_object *texObj0 = texUnit0->_Current;
            struct gl_texture_object *texObj1 = texUnit1->_Current;

	    fprintf(stderr, "env mode: 0x%04x / 0x%04x\n", texUnit0->EnvMode, texUnit1->EnvMode);

	    if ( (texObj0 != NULL) && (texObj0->Image[0][0] != NULL) )
	      fprintf(stderr, "format 0: 0x%04x\n", texObj0->Image[0][0]->Format);
		    
	    if ( (texObj1 != NULL) && (texObj1->Image[0][0] != NULL) )
	      fprintf(stderr, "format 1: 0x%04x\n", texObj1->Image[0][0]->Format);
	}


        if (texUnit0->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit0->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];

	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->_ReallyEnabled\n");    
            if (texImage->Border) {
                FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, GL_TRUE);
                return;
            }

            vmesa->regEnable |= HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK;
   
            switch (texObj->MinFilter) {
            case GL_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                break;
            case GL_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                break;
            case GL_NEAREST_MIPMAP_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Nearest;
                break;
            case GL_LINEAR_MIPMAP_NEAREST:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Nearest;
                break;
            case GL_NEAREST_MIPMAP_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Linear;
                break;
            case GL_LINEAR_MIPMAP_LINEAR:
                vmesa->regHTXnTB_0 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_0 |= HC_HTXnFLDs_Linear;
                break;
            default:
                break;
            }

    	    switch (texObj->MagFilter) {
	    case GL_LINEAR:
                vmesa->regHTXnTB_0 |= HC_HTXnFLSe_Linear |
                                      HC_HTXnFLTe_Linear;
		break;
	    case GL_NEAREST:
                vmesa->regHTXnTB_0 |= HC_HTXnFLSe_Nearest |
                                      HC_HTXnFLTe_Nearest;
		break;
	    default:
 	       break;
            }

	    vmesa->regHTXnMPMD_0 &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD_0 |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    if (VIA_DEBUG) fprintf(stderr, "texUnit0->EnvMode %x\n",texUnit0->EnvMode);    

	    viaTexCombineState( vmesa, texUnit0->_CurrentCombine, 0 );
        }

        if (texUnit1->_ReallyEnabled) {
            struct gl_texture_object *texObj = texUnit1->_Current;
            struct gl_texture_image *texImage = texObj->Image[0][0];

            if (texImage->Border) {
                FALLBACK(vmesa, VIA_FALLBACK_TEXTURE, GL_TRUE);
                return;
            }

            switch (texObj->MinFilter) {
            case GL_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                break;
            case GL_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                break;
            case GL_NEAREST_MIPMAP_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Nearest;
                break ;
            case GL_LINEAR_MIPMAP_NEAREST:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Nearest;
                break ;
            case GL_NEAREST_MIPMAP_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Nearest |
                                     HC_HTXnFLTs_Nearest;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Linear;
                break ;
            case GL_LINEAR_MIPMAP_LINEAR:
                vmesa->regHTXnTB_1 = HC_HTXnFLSs_Linear |
                                     HC_HTXnFLTs_Linear;
                vmesa->regHTXnTB_1 |= HC_HTXnFLDs_Linear;
                break ;
            default:
                break;
            }

	    switch(texObj->MagFilter) {
	    case GL_NEAREST:
	       vmesa->regHTXnTB_1 |= HC_HTXnFLSs_Nearest |
		  HC_HTXnFLTs_Nearest;
	       break;
	    case GL_LINEAR:
	       vmesa->regHTXnTB_1 |= HC_HTXnFLSs_Linear |
		  HC_HTXnFLTs_Linear;
	       break;
	    default:
	       break;
	    }
	    
	    vmesa->regHTXnMPMD_1 &= ~(HC_HTXnMPMD_SMASK | HC_HTXnMPMD_TMASK);
	    vmesa->regHTXnMPMD_1 |= get_wrap_mode( texObj->WrapS,
						   texObj->WrapT );

	    viaTexCombineState( vmesa, texUnit1->_CurrentCombine, 1 );
        }
	
	if (VIA_DEBUG) {
	    fprintf( stderr, "Csat_0 / Cop_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLCsat_0, vmesa->regHTXnTBLCop_0 );
	    fprintf( stderr, "Asat_0        = 0x%08x\n",
		     vmesa->regHTXnTBLAsat_0 );
	    fprintf( stderr, "RCb_0 / RAa_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLRCb_0, vmesa->regHTXnTBLRAa_0 );
	    fprintf( stderr, "RCa_0 / RCc_0 = 0x%08x / 0x%08x\n",
		     vmesa->regHTXnTBLRCa_0, vmesa->regHTXnTBLRCc_0 );
	    fprintf( stderr, "RCbias_0      = 0x%08x\n",
		     vmesa->regHTXnTBLRCbias_0 );
	}
    }
    else {
        vmesa->regEnable &= (~(HC_HenTXMP_MASK | HC_HenTXCH_MASK | HC_HenTXPP_MASK));
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    
}

static void viaChooseColorState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLenum s = ctx->Color.BlendSrcRGB;
    GLenum d = ctx->Color.BlendDstRGB;

    /* The HW's blending equation is:
     * (Ca * FCa + Cbias + Cb * FCb) << Cshift
     */
     if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    

    if (ctx->Color.BlendEnabled) {
        vmesa->regEnable |= HC_HenABL_MASK;
        /* Ca  -- always from source color.
         */
        vmesa->regHABLCsat = HC_HABLCsat_MASK | HC_HABLCa_OPC |
                             HC_HABLCa_Csrc;
        /* Aa  -- always from source alpha.
         */
        vmesa->regHABLAsat = HC_HABLAsat_MASK | HC_HABLAa_OPA |
                             HC_HABLAa_Asrc;
        /* FCa -- depend on following condition.
         * FAa -- depend on following condition.
         */
        switch (s) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_HABLRCa;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
            vmesa->regHABLRFCa = 0x0;
            vmesa->regHABLRAa = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Csrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Cdst;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Asrc;
            vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_OPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_InvOPC | HC_HABLFCa_Adst;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_Adst;
                }
            }
            break;
        case GL_SRC_ALPHA_SATURATE:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (f, f, f, 1), f = min(As, 1 - Ad) = min(As, 1 - 1) = 0
                     * So (f, f, f, 1) = (0, 0, 0, 1)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_HABLRCa;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
                else {
                    /* (f, f, f, 1), f = min(As, 1 - Ad)
                     */
                    vmesa->regHABLCsat |= HC_HABLFCa_OPC | HC_HABLFCa_mimAsrcInvAdst;
                    vmesa->regHABLAsat |= HC_HABLFAa_InvOPA | HC_HABLFAa_HABLFRA;
                    vmesa->regHABLRFCa = 0x0;
                    vmesa->regHABLRAa = 0x0;
                }
            }
            break;
        }

        /* Op is add.
         */

        /* bias is 0.
         */
        vmesa->regHABLCsat |= HC_HABLCbias_HABLRCbias;
        vmesa->regHABLAsat |= HC_HABLAbias_HABLRAbias;

        /* Cb  -- always from destination color.
         */
        vmesa->regHABLCop = HC_HABLCb_OPC | HC_HABLCb_Cdst;
        /* Ab  -- always from destination alpha.
         */
        vmesa->regHABLAop = HC_HABLAb_OPA | HC_HABLAb_Adst;
        /* FCb -- depend on following condition.
         */
        switch (d) {
        case GL_ZERO:
            /* (0, 0, 0, 0)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_ONE:
            /* (1, 1, 1, 1)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        case GL_SRC_COLOR:
            /* (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            /* (1, 1, 1, 1) - (Rs, Gs, Bs, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Csrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_COLOR:
            /* (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            /* (1, 1, 1, 1) - (Rd, Gd, Bd, Ad)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Cdst;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
            break;
        case GL_SRC_ALPHA:
            /* (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Asrc;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            /* (1, 1, 1, 1) - (As, As, As, As)
             */
            vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Asrc;
            vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Asrc;
            break;
        case GL_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_Adst;
                }
            }
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            {
                if (vmesa->viaScreen->bitsPerPixel == 16) {
                    /* (1, 1, 1, 1) - (1, 1, 1, 1) = (0, 0, 0, 0)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
                    vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
                    vmesa->regHABLRFCb = 0x0;
                    vmesa->regHABLRAb = 0x0;
                }
                else {
                    /* (1, 1, 1, 1) - (Ad, Ad, Ad, Ad)
                     */
                    vmesa->regHABLCop |= HC_HABLFCb_InvOPC | HC_HABLFCb_Adst;
                    vmesa->regHABLAop |= HC_HABLFAb_InvOPA | HC_HABLFAb_Adst;
                }
            }
            break;
        default:
            vmesa->regHABLCop |= HC_HABLFCb_OPC | HC_HABLFCb_HABLRCb;
            vmesa->regHABLAop |= HC_HABLFAb_OPA | HC_HABLFAb_HABLFRA;
            vmesa->regHABLRFCb = 0x0;
            vmesa->regHABLRAb = 0x0;
            break;
        }

        if (vmesa->viaScreen->bitsPerPixel <= 16)
            vmesa->regEnable &= ~HC_HenDT_MASK;

    }
    else {
        vmesa->regEnable &= (~HC_HenABL_MASK);
    }

    if (ctx->Color.AlphaEnabled) {
        vmesa->regEnable |= HC_HenAT_MASK;
        vmesa->regHATMD = (((GLchan)ctx->Color.AlphaRef) & 0xFF) |
            ((ctx->Color.AlphaFunc - GL_NEVER) << 8);
    }
    else {
        vmesa->regEnable &= (~HC_HenAT_MASK);
    }

    if (ctx->Color.DitherFlag && (vmesa->viaScreen->bitsPerPixel < 32)) {
        if (ctx->Color.BlendEnabled) {
            vmesa->regEnable &= ~HC_HenDT_MASK;
        }
        else {
            vmesa->regEnable |= HC_HenDT_MASK;
        }
    }

    if (ctx->Color.ColorLogicOpEnabled) 
        vmesa->regHROP = ROP[ctx->Color.LogicOp & 0xF];
    else
        vmesa->regHROP = HC_HROP_P;

    vmesa->regHFBBMSKL = (*(GLuint *)&ctx->Color.ColorMask[0]) & 0xFFFFFF;
    vmesa->regHROP |= ctx->Color.ColorMask[3];

    if (ctx->Color.ColorMask[3])
        vmesa->regEnable |= HC_HenAW_MASK;
    else
        vmesa->regEnable &= ~HC_HenAW_MASK;

    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static void viaChooseFogState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Fog.Enabled) {
        GLubyte r, g, b, a;

        vmesa->regEnable |= HC_HenFOG_MASK;

        /* Use fog equation 0 (OpenGL's default) & local fog.
         */
        vmesa->regHFogLF = 0x0;

        r = (GLubyte)(ctx->Fog.Color[0] * 255.0F);
        g = (GLubyte)(ctx->Fog.Color[1] * 255.0F);
        b = (GLubyte)(ctx->Fog.Color[2] * 255.0F);
        a = (GLubyte)(ctx->Fog.Color[3] * 255.0F);
        vmesa->regHFogCL = (r << 16) | (g << 8) | b;
        vmesa->regHFogCH = a;
    }
    else {
        vmesa->regEnable &= ~HC_HenFOG_MASK;
    }
}

static void viaChooseDepthState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (ctx->Depth.Test) {
        vmesa->regEnable |= HC_HenZT_MASK;
        if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);
	vmesa->regHZWTMD = (ctx->Depth.Func - GL_NEVER) << 16;
	
    }
    else {
        vmesa->regEnable &= ~HC_HenZT_MASK;
        
        /*=* [DBG] racer : can't display cars in car selection menu *=*/
	/*if (ctx->Depth.Mask)
            vmesa->regEnable |= HC_HenZW_MASK;
        else
            vmesa->regEnable &= (~HC_HenZW_MASK);*/
	vmesa->regEnable &= (~HC_HenZW_MASK);
    }
}

static void viaChooseLightState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Light.ShadeModel == GL_SMOOTH) {
        vmesa->regCmdA |= HC_HShading_Gouraud;
    }
    else {
        vmesa->regCmdA &= ~HC_HShading_Gouraud;
    }
}

static void viaChooseLineState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    if (ctx->Line.SmoothFlag) {
        vmesa->regEnable |= HC_HenAA_MASK;
    }
    else {
        if (!ctx->Polygon.SmoothFlag) {
            vmesa->regEnable &= ~HC_HenAA_MASK;
        }
    }

    if (0 && ctx->Line.StippleFlag) {
        vmesa->regEnable |= HC_HenLP_MASK;
        vmesa->regHLP = ctx->Line.StipplePattern;
        vmesa->regHLPRF = ctx->Line.StippleFactor;
    }
    else {
        vmesa->regEnable &= ~HC_HenLP_MASK;
    }
}

static void viaChoosePolygonState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    /* KW: FIXME: this should be in viaRasterPrimitive (somehow)
     */
    if (ctx->Polygon.SmoothFlag) {
        vmesa->regEnable |= HC_HenAA_MASK;
    }
    else {
        if (!ctx->Line.SmoothFlag) {
            vmesa->regEnable &= ~HC_HenAA_MASK;
        }
    }

    if (ctx->Polygon.StippleFlag) {
        vmesa->regEnable |= HC_HenSP_MASK;
    }
    else {
        vmesa->regEnable &= ~HC_HenSP_MASK;
    }

    if (ctx->Polygon.CullFlag) {
        vmesa->regEnable |= HC_HenFBCull_MASK;
    }
    else {
        vmesa->regEnable &= ~HC_HenFBCull_MASK;
    }
}

static void viaChooseStencilState(GLcontext *ctx) 
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    
    if (ctx->Stencil.Enabled) {
        GLuint temp;

        vmesa->regEnable |= HC_HenST_MASK;
        temp = (ctx->Stencil.Ref[0] & 0xFF) << HC_HSTREF_SHIFT;
        temp |= 0xFF << HC_HSTOPMSK_SHIFT;
        temp |= (ctx->Stencil.ValueMask[0] & 0xFF);
        vmesa->regHSTREF = temp;

        temp = (ctx->Stencil.Function[0] - GL_NEVER) << 16;

        switch (ctx->Stencil.FailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSF_DECR;
            break;
        }

        switch (ctx->Stencil.ZFailFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZF_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZF_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZF_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZF_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZF_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZF_DECR;
            break;
        }

        switch (ctx->Stencil.ZPassFunc[0]) {
        case GL_KEEP:
            temp |= HC_HSTOPSPZP_KEEP;
            break;
        case GL_ZERO:
            temp |= HC_HSTOPSPZP_ZERO;
            break;
        case GL_REPLACE:
            temp |= HC_HSTOPSPZP_REPLACE;
            break;
        case GL_INVERT:
            temp |= HC_HSTOPSPZP_INVERT;
            break;
        case GL_INCR:
            temp |= HC_HSTOPSPZP_INCR;
            break;
        case GL_DECR:
            temp |= HC_HSTOPSPZP_DECR;
            break;
        }
        vmesa->regHSTMD = temp;
    }
    else {
        vmesa->regEnable &= ~HC_HenST_MASK;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}



static void viaChooseTriangle(GLcontext *ctx) 
{       
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) {
       fprintf(stderr, "%s - in\n", __FUNCTION__);        
       fprintf(stderr, "GL_CULL_FACE = %x\n", GL_CULL_FACE);    
       fprintf(stderr, "ctx->Polygon.CullFlag = %x\n", ctx->Polygon.CullFlag);       
       fprintf(stderr, "GL_FRONT = %x\n", GL_FRONT);    
       fprintf(stderr, "ctx->Polygon.CullFaceMode = %x\n", ctx->Polygon.CullFaceMode);    
       fprintf(stderr, "GL_CCW = %x\n", GL_CCW);    
       fprintf(stderr, "ctx->Polygon.FrontFace = %x\n", ctx->Polygon.FrontFace);    
    }
    if (ctx->Polygon.CullFlag == GL_TRUE) {
        switch (ctx->Polygon.CullFaceMode) {
        case GL_FRONT:
            if (ctx->Polygon.FrontFace == GL_CCW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_BACK:
            if (ctx->Polygon.FrontFace == GL_CW)
                vmesa->regCmdB |= HC_HBFace_MASK;
            else
                vmesa->regCmdB &= ~HC_HBFace_MASK;
            break;
        case GL_FRONT_AND_BACK:
            return;
        }
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaValidateState( GLcontext *ctx )
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    
    if (vmesa->newState & _NEW_TEXTURE) {
        viaChooseTextureState(ctx);
	viaUpdateTextureState(ctx); /* May modify vmesa->Fallback */
    }

    if (vmesa->newState & _NEW_COLOR)
        viaChooseColorState(ctx);

    if (vmesa->newState & _NEW_DEPTH)
        viaChooseDepthState(ctx);

    if (vmesa->newState & _NEW_FOG)
        viaChooseFogState(ctx);

    if (vmesa->newState & _NEW_LIGHT)
        viaChooseLightState(ctx);

    if (vmesa->newState & _NEW_LINE)
        viaChooseLineState(ctx);

    if (vmesa->newState & (_NEW_POLYGON | _NEW_POLYGONSTIPPLE)) {
        viaChoosePolygonState(ctx);
	viaChooseTriangle(ctx);
    }

    if (vmesa->newState & _NEW_STENCIL)
        viaChooseStencilState(ctx);
    
    if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
        vmesa->regEnable |= HC_HenCS_MASK;
    else
        vmesa->regEnable &= ~HC_HenCS_MASK;

    vmesa->newEmitState |= vmesa->newState;
    vmesa->newState = 0;
}

static void viaInvalidateState(GLcontext *ctx, GLuint newState)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    VIA_FINISH_PRIM( vmesa );
    vmesa->newState |= newState;

    _swrast_InvalidateState(ctx, newState);
    _swsetup_InvalidateState(ctx, newState);
    _ac_InvalidateState(ctx, newState);
    _tnl_InvalidateState(ctx, newState);
}

void viaInitStateFuncs(GLcontext *ctx)
{
    /* Callbacks for internal Mesa events.
     */
    ctx->Driver.UpdateState = viaInvalidateState;

    /* API callbacks
     */
    ctx->Driver.BlendEquationSeparate = viaBlendEquationSeparate;
    ctx->Driver.BlendFuncSeparate = viaBlendFuncSeparate;
    ctx->Driver.ClearColor = viaClearColor;
    ctx->Driver.ColorMask = viaColorMask;
    ctx->Driver.DrawBuffer = viaDrawBuffer;
    ctx->Driver.RenderMode = viaRenderMode;
    ctx->Driver.Scissor = viaScissor;
    ctx->Driver.DepthRange = viaDepthRange;
    ctx->Driver.Viewport = viaViewport;
    ctx->Driver.Enable = viaEnable;

    /* Pixel path fallbacks.
     */
    ctx->Driver.Accum = _swrast_Accum;
    ctx->Driver.Bitmap = _swrast_Bitmap;
    ctx->Driver.CopyPixels = _swrast_CopyPixels;
    ctx->Driver.DrawPixels = _swrast_DrawPixels;
    ctx->Driver.ReadPixels = _swrast_ReadPixels;
    ctx->Driver.ResizeBuffers = viaReAllocateBuffers;

    /* Swrast hooks for imaging extensions:
     */
    ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
    ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
    ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
    ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
