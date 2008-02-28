/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                                 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include "d3dText.h"

/*=============================================================================

       1     
     ------
    |      | 
  6 |      | 2
    |  7   |
     ------  
    |      |
  5 |      | 3
    |      |
     ------ 
       4 

	    TL_0		TR_0
TLL		TL_1		TR_1	TRR

MLL_0   ML_0		MR_0	MRR_0
MLL_1   ML_1		MR_1	MRR_1

BLL		BL_0		BR_0	BRR
        BL_1		BR_1

=============================================================================*/

#define TLL		0
#define TRR		1
#define TL_0	2
#define TL_1	3
#define TR_0	4
#define TR_1	5

#define	MLL_0	6
#define	MLL_1	7
#define	MRR_0	8
#define	MRR_1	9

#define	ML_0	10
#define	ML_1	11
#define	MR_0	12
#define	MR_1	13

#define	BL_0	14
#define	BL_1	15
#define	BR_0	16
#define	BR_1	17
#define	BLL		18
#define	BRR		19

#define	BIT1	0x00000001
#define	BIT2	0x00000002
#define	BIT3	0x00000004
#define	BIT4	0x00000008
#define	BIT5	0x00000010
#define	BIT6	0x00000020
#define	BIT7	0x00000040

#define TOP		BIT4
#define MIDDLE		BIT7
#define BOTTOM		BIT1
#define TLEFT 		BIT5
#define BLEFT   	BIT6
#define LEFT		(TLEFT|BLEFT)
#define TRIGHT		BIT3
#define BRIGHT		BIT2
#define RIGHT		(TRIGHT|BRIGHT)
#define ALL		0xFFFFFFFF

/*===========================================================================*/
/*  This is the static array that will map the ASCII value of the character  */
/* being draw to the bit mask that will be scan converted to the LED display.*/
/*===========================================================================*/
DWORD	textBitMasks[] = 
{
  0xFFFFFFFF, // 000
  0xFFFFFFFF, // 001
  0xFFFFFFFF, // 002
  0xFFFFFFFF, // 003  
  0xFFFFFFFF, // 004
  0xFFFFFFFF, // 005
  0xFFFFFFFF, // 006
  0xFFFFFFFF, // 007  
  0xFFFFFFFF, // 008
  0xFFFFFFFF, // 009
  0xFFFFFFFF, // 010
  0xFFFFFFFF, // 011
  0xFFFFFFFF, // 012
  0xFFFFFFFF, // 013
  0xFFFFFFFF, // 014
  0xFFFFFFFF, // 015
  0xFFFFFFFF, // 016
  0xFFFFFFFF, // 017
  0xFFFFFFFF, // 018
  0xFFFFFFFF, // 019
  0xFFFFFFFF, // 020
  0xFFFFFFFF, // 021
  0xFFFFFFFF, // 022
  0xFFFFFFFF, // 023 
  0xFFFFFFFF, // 024
  0xFFFFFFFF, // 025
  0xFFFFFFFF, // 026
  0xFFFFFFFF, // 027
  0xFFFFFFFF, // 028
  0xFFFFFFFF, // 029
  0xFFFFFFFF, // 030
  0XFFFFFFFF, // 031 
  0x00000000, // 032 'SPC'
  0xFFFFFFFF, // 033
  0xFFFFFFFF, // 034
  0xFFFFFFFF, // 035
  0xFFFFFFFF, // 036
  0xFFFFFFFF, // 037
  0xFFFFFFFF, // 038
  0xFFFFFFFF, // 039
  0xFFFFFFFF, // 040
  0xFFFFFFFF, // 041
  0xFFFFFFFF, // 042
  0xFFFFFFFF, // 043
  0xFFFFFFFF, // 044
  0xFFFFFFFF, // 045
  0xFFFFFFFF, // 046
  0xFFFFFFFF, // 047 
  (ALL &~ MIDDLE), 			// 048	'0'
  (RIGHT),     				// 049	'1'
  (ALL &~ TLEFT &~ BRIGHT), // 050	'2'
  (ALL &~ LEFT), 			// 051	'3'
  (TLEFT | MIDDLE | RIGHT), // 052	'4'
  (ALL &~ TRIGHT &~ BLEFT), // 053	'5'
  (ALL &~ TRIGHT), 			// 054	'6'
  (TOP | RIGHT), 			// 055	'7'
  (ALL), 					// 056	'8'
  (ALL &~ BOTTOM &~ BLEFT), // 057	'9'
  0xFFFFFFFF, // 058
  0xFFFFFFFF, // 059
  0xFFFFFFFF, // 060
  0XFFFFFFFF, // 061
  0xFFFFFFFF, // 062
  0xFFFFFFFF, // 063
  0xFFFFFFFF, // 064
  (ALL &~ BOTTOM),			// 065	'A'
  (ALL), 					// 066	'B'
  (TOP | LEFT | BOTTOM), 	// 067	'C'
  (ALL &~ MIDDLE), 			// 068	'D'
  (ALL &~ RIGHT), 			// 069	'E'
  (LEFT | TOP | MIDDLE), 	// 070	'F'
  0x00000000,  				// 071	'G'
  (ALL &~ TOP &~ BOTTOM), 	// 072	'H'
  (RIGHT), 					// 073	'I'
  (RIGHT | BOTTOM),			// 074	'J'
  0x00000000,  				// 075	'K'
  (LEFT | BOTTOM),			// 076	'L'
  0x00000000, 				// 088	'M'
  0x00000000, 				// 089	'N'
  (ALL &~ MIDDLE),  		// 090	'O'
  (ALL &~ BRIGHT &~ BOTTOM),// 091	'P'
  0x00000000, 				// 092	'Q'
  0x00000000,				// 093	'R'
  (ALL &~ TRIGHT &~ BLEFT),	// 094	'S'
  0X00000000, 				// 095	'T'
  (LEFT | RIGHT | BOTTOM),  // 096	'U'
  0x00000000, 				// 097	'V'
  0x00000000, 				// 098	'W'
  0x00000000, 				// 099	'X'
  0x00000000,  				// 1000	'Z'
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 100 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 104 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 108 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 112 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 116 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,  // 120 
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF   // 124 
};

#define	CT	1.0f
#define	CX	7.0f
#define	CY	13.0f
#define CM	((CY-(CT*3.0f))/2.0f)

float lCoords[][2] = 
{
  /* Top outsides. */
  { 0,  		(CY-CT) },		
  { CX, 		(CY-CT) },

  /* Top Line. */
  { CT, 		CY },
  { CT, 		(CY-CT) },
  { (CX-CT), 	CY },
  { (CX-CT), 	(CY-CT) },

  /* Middle outsides. */
  { 0.0f,		(CT+CM+CT) },
  { 0.0f,		(CT+CM) },
  { CX,			(CT+CM+CT) },
  { CX,			(CT+CM) },

  /* Middle Line. */
  { CT,			(CT+CM+CT) },	
  { CT,			(CT+CM)	},	
  { (CX-CT),	(CT+CM+CT) },
  { (CX-CT),	(CT+CM) },

  /* Bottom line. */
  { CT,			CT },
  { CT,			0.0f },
  { (CX-CT),	CT },
  { (CX-CT),	0.0f },

  /* Bottom outsides. */
  { 0.0f,		CT},
  { CX,			CT }
};

static int	ConvertCharacter( char *c, int cIndex,  PD3DFONTMETRICS pfntMetrics );

D3DTLVERTEX TextVertices[MAX_VERTICES];
/*===========================================================================*/
/*  When we attach I will zero out the whole D3D vertex buffer I'm using for */
/* the text.  This way I don't need to set all the redundant values.  I also */
/* set all the oow values to 1 as I will be doing direct rendering.          */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
extern "C" BOOL InitD3DText( void )
{
  int	index;

  /* Set the D3D Vertex Buffer up once so we don't do redundant changes. */
  memset( &TextVertices[0], 0, sizeof(TextVertices) );
  for( index = 0; index < MAX_VERTICES; index++ )
	TextVertices[index].rhw	= D3DVAL( 1.0 );

  return TRUE;
}
/*===========================================================================*/
/*  This function takes a single character and draw it using the supplied    */
/* fontmetrics structure.                                                    */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void d3dTextDrawString( char *pszString, int x, int y, PD3DFONTMETRICS pfntMetrics )
{
  int	cIndex,
	     nIndex,
	     index;
  float	cWidth = CX,
	     cHeight = CY;

  /*  Find the max width/height of a character and add the spacing so */
  /* that we can use this value to calculate the x,y of the character.*/
  cWidth  = (cWidth  * pfntMetrics->fntXScale)  + pfntMetrics->fntXSpacing;
  cHeight = (cHeight * pfntMetrics->fntYScale) + pfntMetrics->fntYSpacing;

  /* Walk the string.  This must be NULL terminated. */
  for( cIndex = 0, nIndex = 0; *pszString; pszString++, cIndex = nIndex, x++ )
  {
	/* Convert the character and get the index into the text vertex buffer. */
	nIndex = ConvertCharacter( &pszString[0], cIndex, pfntMetrics );
	if ( (nIndex - cIndex) > 2 )
    {
	  /* Modify the text vertex buffer based on the fntMetrics structure. */
	  for( index = cIndex; index < nIndex; index++ )
	  {
		/* Scale the character. */
		TextVertices[index].sx	*= pfntMetrics->fntXScale;
		TextVertices[index].sy   *= pfntMetrics->fntYScale;
		
		/* Move the character. */
		TextVertices[index].sx	+= (cWidth*x);
		TextVertices[index].sy   += (cHeight*y);

		/* Set the color. */
		TextVertices[index].color = pfntMetrics->dwColor;
	  }
	}
  }

  if ( nIndex < 3 )
	return;

  /* Set the states that slim things down. */
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, 			D3DCULL_NONE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, 			D3DFILL_SOLID );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE , 		FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, 	FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, 	FALSE );

  /* Blast them baby... */
  pfntMetrics->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
										   D3DFVF_TLVERTEX,
										   (LPVOID)&TextVertices[0],
										   nIndex, 
										   (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
}
/*===========================================================================*/
/*  This function takes a single character and draw it directly to the screen*/
/* unsing the supplied fntMetrics structure.  The character will be drawn at */
/* the supplied x,y.  The x,y position is relative to the top left and uses  */
/* the spacing in the fntMetrics structure.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
extern "C" void d3dTextDrawCharacter( char *c, int x, int y, PD3DFONTMETRICS pfntMetrics )
{
  int	cIndex = 0,
	    index;
  float	cWidth = CX,
	    cHeight = CY;

  /* Convert the character and get the index into the text vertex buffer. */
  cIndex = ConvertCharacter( c, 0, pfntMetrics );
  if ( cIndex < 3 )
	return;

  /*  Find the max width/height of a character and add the spacing so */
  /* that we can use this value to calculate the x,y of the character.*/
  cWidth  = (cWidth  * pfntMetrics->fntXScale)  + pfntMetrics->fntXSpacing;
  cHeight = (cHeight * pfntMetrics->fntYScale) + pfntMetrics->fntYSpacing;

  /* Modify the text vertex buffer based on the fntMetrics structure. */
  for( index = 0; index < cIndex; index++ )
  {
	/* Scale the character. */
	TextVertices[index].sx	*= pfntMetrics->fntXScale;
	TextVertices[index].sy   *= pfntMetrics->fntYScale;

	/* Move the character. */
	TextVertices[index].sx	+= (cWidth*x);
	TextVertices[index].sy   += (cHeight*y);

	/* Set the color. */
	TextVertices[index].color = pfntMetrics->dwColor;
  }
 

  /* Set the states that slim things down. */
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, 			D3DCULL_NONE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, 			D3DFILL_SOLID );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ZWRITEENABLE , 		FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, 	FALSE );
  pfntMetrics->lpD3DDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, 	FALSE );

  /* Blast them baby... */
  pfntMetrics->lpD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST,
										   D3DFVF_TLVERTEX,
										   (LPVOID)&TextVertices[0],
										   cIndex, 
										   (D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT) );
}
/*===========================================================================*/
/*  This function takes a single character and draw it using the supplied    */
/* fontmetrics structure.                                                    */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static int	ConvertCharacter( char *c, int cIndex,  PD3DFONTMETRICS pfntMetrics )
{
  DWORD	asciiChar = (int)(*c);

  /* Handle the TOP line. */
  if ( textBitMasks[asciiChar] & BIT1 )
  { 
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TL_0][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TL_0][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_0][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TR_0][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TR_1][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TR_1][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TL_1][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TL_1][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TL_0][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TL_0][1] );
  }

  /* Handle the TOP/BOTTOM RIGHT lines. */
  //  if ( textBitMasks[index] & (BIT2|BIT3) )
  if ( 1 == 0 )
  {
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TR_1][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TRR][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TRR][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[BRR][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[BRR][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[BRR][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[BRR][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[BR_0][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[BR_0][1] );
    TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
    TextVertices[cIndex++].sy= D3DVAL( lCoords[TR_1][1] );
  }
  else 
  {
	if ( textBitMasks[asciiChar] & BIT2 )
	{
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TR_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TRR][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TRR][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MRR_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MRR_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MRR_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MRR_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MR_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TR_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TR_1][1] );
	}
	if ( textBitMasks[asciiChar] & BIT3 )
	{
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MR_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MRR_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MRR_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BRR][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BRR][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BRR][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BRR][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BR_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BR_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MR_1][1] );
	}
  }

  /* Handle the TOP/BOTTOM LEFT lines. */
  //  if ( textBitMasks[asciiChar] & (BIT5|BIT6) )
  if ( 1 == 0 )
  {
	TextVertices[cIndex].sx	= D3DVAL( lCoords[TLL][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[TLL][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[TL_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[TL_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BL_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BL_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BLL][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BLL][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[TLL][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[TLL][1] );
  }
  else 
  {
	if ( textBitMasks[asciiChar] & BIT5 )
	{
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MLL_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MLL_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[ML_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BL_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BL_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[BLL][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[BLL][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MLL_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MLL_1][1] );
	}
	if ( textBitMasks[asciiChar] & BIT6 )
	{
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TLL][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TLL][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TL_1][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TL_1][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[ML_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[ML_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[MLL_0][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[MLL_0][1] );
	  TextVertices[cIndex].sx	= D3DVAL( lCoords[TLL][0] );
	  TextVertices[cIndex++].sy	= D3DVAL( lCoords[TLL][1] );
	}
  }

  /* Handle the MIDDLE line. */
  if ( textBitMasks[asciiChar] & BIT7 )
  {
	TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[ML_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[MR_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[MR_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[MR_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[MR_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[ML_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[ML_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[ML_0][1] );
  }

  /* Handle the BOTTOM line. */
  if ( textBitMasks[asciiChar] & BIT4 )
  {
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BL_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BR_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BR_0][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BR_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BR_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BR_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BR_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_1][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BL_1][1] );
	TextVertices[cIndex].sx	= D3DVAL( lCoords[BL_0][0] );
	TextVertices[cIndex++].sy= D3DVAL( lCoords[BL_0][1] );
  }
  
  return cIndex;
}

#undef CM
#undef CY
#undef CX
#undef CT

#undef TLL
#undef TRR
#undef TL_0
#undef TL_1
#undef TR_0
#undef TR_1

#undef	MLL_0
#undef	MLL_1
#undef	MRR_0
#undef	MRR_1

#undef	ML_0
#undef	ML_1
#undef	MR_0
#undef	MR_1

#undef	BL_0
#undef	BL_1
#undef	BR_0
#undef	BR_1
#undef	BLL
#undef	BRR

#undef	BIT1
#undef	BIT2
#undef	BIT3
#undef	BIT4
#undef	BIT5
#undef	BIT6
#undef	BIT7

#undef TOP
#undef MIDDLE
#undef BOTTOM
#undef TLEFT
#undef BLEFT
#undef LEFT
#undef TRIGHT
#undef BRIGHT
#undef RIGHT
#undef ALL
