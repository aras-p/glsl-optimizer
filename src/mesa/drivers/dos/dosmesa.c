/* $Id: dosmesa.c,v 1.2 2000/09/26 20:54:10 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.3
 * Copyright (C) 1995-1997  Brian Paul
 *
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
 */


/*
 * $Log: dosmesa.c,v $
 * Revision 1.2  2000/09/26 20:54:10  brianp
 * First batch of OpenGL SI related changes:
 * Renamed struct gl_context to struct __GLcontextRec.
 * Include glcore.h, setup GL imports/exports.
 * Replaced gl_ prefix with _mesa_ prefix in context.[ch] functions.
 * GLcontext's Visual field is no longer a pointer.
 *
 * Revision 1.1.1.1  1999/08/19 00:55:41  jtg
 * Imported sources
 *
 * Revision 1.2  1999/03/28 21:11:57  brianp
 * updated SetBuffer driver function
 *
 * Revision 1.1  1999/02/24 03:56:31  brianp
 * initial check-in
 *
 *
 * Revision 1.5  1997/06/19  22:00:00  Brian Paul
 * Removed all ColorShift stuff
 *
 * Revision 1.4  1997/06/03  19:00:00  Brian Paul
 * Replaced VB->Unclipped[] with VB->ClipMask[]
 *
 * Revision 1.3  1997/05/28  07:00:00  Phil Frisbie, Jr.
 * Now pass red/green/blue/alpha bits to gl_create_visual()
 * Fixed DJGPP mode 13 support.
 *
 * Revision 1.2  1996/12/08  16:13:45  Charlie
 * Added VESA support via Scitechs SVGA kit.
 *
 * Revision 1.1  1996/12/08  16:09:52  Charlie
 * Initial revision
 *
 */


/*
 * DOS VGA/VESA/MGL/Mesa interface.
 *
 */

/*
 *
 * TODO: (cw)
 *	 Improve the colour matcher for rgb non vesa modes, its pretty bad and incorrect
 *	 Keyboard interrupt.
 *  Comments and tidy up.
 *  Add support for VESA without SVGAKIT.
 *  DirectX Support.
 *  Better GLIDE Support.
 *  Clear up the #ifdef madness.
 */

#ifdef DOSVGA

#if defined(DOSVGA) && !defined(GLIDE) && defined(DJGPP) && !defined(UNIVBE) && !defined(MGL)

/* Should help cut down on the crazy #if`s */
#define MODE13 1

#else
#undef MODE13
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <dos.h>

#ifdef DJGPP
#include <go32.h>
#endif

#ifdef MGL
#include <mgraph.h>
#endif

#include "GL/DOSmesa.h"
#include "context.h"
#include "matrix.h"
#include "types.h"

#ifdef GLIDE

static void glideshutdown( void );
#include "vb.h"
#include "glide.h"

/* the glide modes available, set one */

//#define GLIDE_MODE GR_RESOLUTION_
#define GLIDE_MODE GR_RESOLUTION_800x600
//#define GLIDE_MODE GR_RESOLUTION_640x480

static GrVertex gr_vtx,gr_vtx1,gr_vtx2;

#endif

#ifdef UNIVBE
/* You get this file from Scitechs VESA development kit */
#include "svga.h"

/*
   Set these to the VESA mode you require, the first is for 256 colour modes,
   the other is High colour modes, they must both be the same XRes and YRes.
 */

#define VESA_256COLOUR_MODE 0x11c
#define VESA_HICOLOUR_MODE 0x11f

#endif

struct DOSmesa_context {
   GLcontext *gl_ctx;			/* the core Mesa context */
   GLvisual *gl_vis;				/* describes the color buffer */
   GLframebuffer *gl_buffer;	/* the ancillary buffers */
   GLuint index;					/* current color index */
   GLint red, green, blue;		/* current rgb color */
   GLint width, height;			/* size of color buffer */
   GLint depth;					/* bits per pixel (8,16,24 or 32) */
};

static DOSMesaContext DOSMesa = NULL;    /* the current context */

#ifdef UNIVBE
SV_devCtx *DC=NULL;
int useLinear = TRUE;
SV_modeInfo *mi=NULL;
unsigned long modeNumber = 0;

int activePage = 0;
int visualPage = 1;

#endif

#if defined(MODE13)

/* DOSVGA With no UniVBE support */

unsigned char *video_buffer;

#define VID_BUF(x,y) *(video_buffer+x+(y*320))

#if defined(__WATCOMC__) && defined(__386__) && defined(__DOS__)

void setupcopy(void);
void copyscr(void);

/* Watcom C specfic, screen copy and clear */

#pragma aux setupcopy = \
	".386P"\
	"push ebp" \
	"mov esi,video_buffer" \
	"mov edi,0xa0000" \
	"mov ecx,2000" \
	"xor ebp,ebp";

#pragma aux copyscr = \
	".386P" \
	"lop1: mov eax,[esi]"\
	"mov ebx,[esi+4]"\
	"mov edx,[esi+8]"\
	"mov [edi],eax"\
	"mov eax,[esi+12]"\
	"mov dword ptr [esi],ebp"\
	"mov dword ptr [esi+4],ebp"\
	"mov dword ptr [esi+8],ebp"\
	"mov dword ptr [esi+12],ebp"\
	"mov [edi+4],ebx"\
	"mov [edi+8],edx"\
	"mov [edi+12],eax"\
	"mov eax,[esi+16]"\
	"mov ebx,[esi+4+16]"\
	"mov edx,[esi+8+16]"\
	"mov [edi+16],eax"\
	"mov eax,[esi+12+16]"\
	"mov dword ptr [esi+16],ebp"\
	"mov dword ptr [esi+4+16],ebp"\
	"mov dword ptr [esi+8+16],ebp"\
	"mov dword ptr [esi+12+16],ebp"\
	"mov [edi+4+16],ebx"\
	"mov [edi+8+16],edx"\
	"mov [edi+12+16],eax"\
	"add esi,32"\
	"add edi,32"\
	"dec ecx"\
	"jnz lop1"\
	"pop ebp"\
	modify exact [edi esi eax ebx ecx] ;

#endif // WATCOM

#endif // MODE13

/*
 * Convert Mesa window Y coordinate to VGA screen Y coordinate:
 */
#define FLIP(Y)  (DOSMesa->height-(Y)-1)

unsigned short vga_cindex = 32768 ;

static points_func choose_points_function( void );
static line_func choose_line_function( void );
static triangle_func choose_polygon_function( void );
static void fast_tri(GLcontext *ctx,GLuint v0, GLuint v1, GLuint v2, GLuint pv );

static points_func choose_points_function( void )
{
	return NULL;
}

static line_func choose_line_function( void )
{
	return NULL;
}

static triangle_func choose_triangle_function( void )
{
	#if defined(MODE13)
		return	NULL;
	#endif

	#if defined(GLIDE)
		return fast_tri;
	#endif

	return	NULL;
}


#if defined(MODE13)

void setgfxmode(void);
void settextmode(void);

#ifndef DJGPP
#pragma aux setgfxmode = \
	"mov	ax,13h" \
	"int	10h" \
	modify [eax];

#pragma aux settextmode = \
	"mov	ax,3h" \
	"int	10h" \
	modify [eax];
#else
void setgfxmode(void)
{
	union REGS in_regs,out_regs;

	in_regs.x.ax = 0x13;
	int386(0x10,&in_regs,&out_regs);
}

void settextmode(void)
{
	union REGS in_regs,out_regs;

	in_regs.x.ax = 0x3;
	int386(0x10,&in_regs,&out_regs);
}

#endif

int set_video_mode(unsigned short x,unsigned short y,char mode)
{
	setgfxmode();
	return 1;	/* likelyhood of this failing is very small */
}

void restore_video_mode(void)
{
	settextmode();
}

int vga_getcolors(void)
{
	return vga_cindex;
}

int vga_getxdim(void)
{
	return 320;
}

int vga_getydim(void)
{
	return 200;
}

static unsigned short num_rgb_alloc = 1; /* start from 1, zero is black */

/* an unlikely colour */
static unsigned char last_r=1,last_g=255,last_b=99;

extern void set_onecolor(int index,int R,int G,int B);

static unsigned char rgbtable[64][64][64];

void vga_setrgbcolor(int r,int g,int b)
{

/*
 * make this into a translation table
 */

	DOSMesa->red = r;
	DOSMesa->green = g ;
	DOSMesa->blue = b ;

	r/=4; g/=4; b/=4;

	if( (r == last_r) && (g == last_g) && (b == last_b) ) return;

	last_r = r ;
	last_g = g ;
	last_b = b ;

	if(r+g+b == 0 ) {
		DOSMesa->index = 0 ;
		return ;
	}

	if( rgbtable[r][g][b] == 0 ) {
		/* not allocated yet */
		if(num_rgb_alloc<256) {
			DOSMesa->index = num_rgb_alloc;
			set_onecolor(num_rgb_alloc,r,g,b);
			rgbtable[r][g][b] = num_rgb_alloc;
			num_rgb_alloc++;
			return;

		} else {
			/* need to search for a close colour */
			{
				unsigned short pass ;

				for(pass=0;pass<64;pass++) {
					if(r-pass>0) {
						if( rgbtable[r-pass][g][b] !=0 ) {
							rgbtable[r][g][b] = rgbtable[r-pass][g][b];
							DOSMesa->index = rgbtable[r-pass][g][b];
							return;
						}
					}
					if(r+pass<64) {
						if( rgbtable[r+pass][g][b] !=0 ) {
							rgbtable[r][g][b] = rgbtable[r+pass][g][b];
							DOSMesa->index = rgbtable[r+pass][g][b];
							return;
						}
					}
				}
			}
			rgbtable[r][g][b] = rand()%255;
		}
	}
	DOSMesa->index = rgbtable[r][g][b];
}

#if defined(DJGPP)
static int dos_seg;
#endif

void vga_clear(void)
{

/* Check if we`re using watcom and DOS */
#if defined(__WATCOMC__) && defined(__386__) && defined(__DOS__)
	setupcopy();
	copyscr();
#else

#if defined (DJGPP)


	asm ("
		pusha
		pushw	%es

		movw	_dos_seg, %es

		movl	_video_buffer, %esi
		movl	$0xa0000, %edi

		movl	$64000, %ecx

		rep	; movsb

		popw	%es
		popa
	");

#else

	/* copy the RAM buffer to the Video memory */
	memcpy((unsigned char *)0xa0000,video_buffer, vga_getxdim()*vga_getydim() );

#endif //DJGPP

	/* clear the RAM buffer */
	memset(video_buffer,0, vga_getxdim()*vga_getydim() );

#endif //WATCOMC

}

#ifndef DEBUG
#define vga_drawpixel(x,y) { VID_BUF(x,y) = DOSMesa->index; }
#else
void vga_drawpixel(x,y)
{
	VID_BUF(x,y) = DOSMesa->index;
}
#endif

int vga_getpixel(int x,int y)
{
	return 1;
}

void vga_flip(void)
{

}

void vga_setcolor(int index)
{
	/* does something, what i`ve no idea */
	DOSMesa->index = index;

}

#endif

#if defined(UNIVBE)

/* UniVBE VESA support */

void set_video_mode(unsigned short x,unsigned short y,char mode)
{
	if( setup_vesa_mode(x,y,mode) == FALSE ) {
		fprintf(stderr,"VESA: Set mode failed\n");
		exit(1);
	}
}

/*
   This is problematic as we don`t know what resolution the user program
   wants when we reach here. This is why the 256 colour and HiColour modes
	should be the same resolution, perhaps i`ll make this an environment
	variable
 */


void check_mi(void)
{
	if(mi!=0) return;

	if(DC==NULL) {
		DC = SV_init( TRUE );
	}

	if(modeNumber == 0 ) {
		modeNumber = VESA_HICOLOUR_MODE;
	}

	SV_getModeInfo(modeNumber,mi);

	return;
}

int setup_vesa_mode(short height,short width,short depth)
{
	if(DC==NULL) {
		DC = SV_init( TRUE );
		/* how many bits per pixel */
		if( depth == 0)
			modeNumber = VESA_256COLOUR_MODE;
		else
			modeNumber = VESA_HICOLOUR_MODE;
	}

	/* Check if correct VESA Version is available */
	if( !DC || DC->VBEVersion < 0x0102) {
		fprintf(stderr,"Require a VESA VBE version 1.2 or higher\n");
		return FALSE;
	}

	/* Check for LFB Supprt */
	if(DC->VBEVersion < 0x0200 ) {
		useLinear = FALSE;
	} else {
		useLinear = TRUE ;
	}

 	/* Get desired mode info */
   if(!SV_getModeInfo( modeNumber, mi))
		return FALSE;

	/* Set VESA mode */
	if(!SV_setMode(modeNumber | svMultiBuffer, FALSE, TRUE, mi->NumberOfPages) )
		return FALSE;

	return TRUE;
}

void restore_video_mode(void)
{
	SV_restoreMode();
}

void vga_clear(void)
{
	SV_clear(0);
}

void vga_flip(void)
{
	activePage = 1-activePage;
	visualPage = 1-activePage;

	SV_setActivePage(activePage);

	/*
	   Change false to true if you`re getting flickering
	   even in double buffer mode, ( sets wait for Vertical retrace  )
	*/
	SV_setVisualPage(visualPage,false);
}

int vga_getcolors(void)
{
	check_mi();
	switch ( mi->BitsPerPixel ) {
		case 8:
			return 256;
		case 15:
		case 16:
			return 32768;
		default:
			return 64000;
	 }
}

int vga_getxdim(void)
{
	check_mi();
	return mi->XResolution;
}

int vga_getydim(void)
{
	check_mi();
	return mi->YResolution;
}

unsigned long current_color = 255;

void vga_setrgbcolor(int r,int g,int b)
{
	DOSMesa->red = r;
	DOSMesa->green = g ;
	DOSMesa->blue = b ;
	current_color = SV_rgbColor(r,g,b);
}

void vga_setcolor(int index)
{
	DOSMesa->index = index;
	current_color = index;
}

void vga_drawpixel(x,y)
{
	SV_putPixel(x,y,current_color);
}

/* TODO: */
int vga_getpixel(x,y)
{
/*	return (int)SV_getPixel(x,y); */
	fprintf(stderr,"vga_getpixel: Not implemented yet\n");
	return 1;
}

/* End of UNIVBE section */
#endif

/* Scitechs MegaGraphicsLibrary http://www.scitechsoft.com/ */

#if defined(MGL)

/* MGL support */
struct MI {
	unsigned short BitsPerPixel;
	unsigned long XResolution;
	unsigned long YResolution;
};

struct MI*mi;

static MGLDC*DC;
static int activePage = 0;
static int visualPage = 1;
static int modeNumber = 0;

void set_video_mode(unsigned short xres,unsigned short yres,char mode)
{
    int     i,driver = grDETECT,dmode = grDETECT;
    event_t evt;

	/* Start the MGL with only the SVGA 16m driver active */
	MGL_registerDriver(MGL_SVGA16NAME,SVGA16_driver);
	if (!MGL_init(&driver,&dmode,"..\\..\\"))
		MGL_fatalError(MGL_errorMsg(MGL_result()));
	if ((DC = MGL_createDisplayDC(false)) == NULL)
		MGL_fatalError(MGL_errorMsg(MGL_result()));
	MGL_makeCurrentDC(DC);
}

/*
   This is problematic as we don`t know what resolution the user program
   wants when we reach here. This is why the 256 colour and HiColour modes
	should be the same resolution, perhaps i`ll make this an environment
	variable
 */

#define MGL_HICOLOUR_MODE 0


void check_mi(void)
{
	if(mi!=0) return;

	if(DC==NULL) {
//		DC = SV_init( TRUE );
	}

	if(modeNumber == 0 ) {
		modeNumber = MGL_HICOLOUR_MODE;
	}

//	SV_getModeInfo(modeNumber,mi);

	return;
}

void restore_video_mode(void)
{
	MGL_exit();
}

void vga_clear(void)
{
	MGL_clearDevice();
}

void vga_flip(void)
{
	activePage = 1-activePage;
	visualPage = 1-activePage;

//	SV_setActivePage(activePage);

	/*
	   Change false to true if you`re getting flickering
	   even in double buffer mode, ( sets wait for Vertical retrace  )
	*/
//	SV_setVisualPage(visualPage,false);
}

int vga_getcolors(void)
{
	check_mi();
	switch ( mi->BitsPerPixel ) {
		case 8:
			return 256;
		case 15:
		case 16:
			return 32768;
		default:
			return 64000;
	 }
}

int vga_getxdim(void)
{
	check_mi();
	return mi->XResolution;
}

int vga_getydim(void)
{
	check_mi();
	return mi->YResolution;
}

unsigned long current_color = 255;

void vga_setrgbcolor(int r,int g,int b)
{
	DOSMesa->red = r;
	DOSMesa->green = g ;
	DOSMesa->blue = b ;
	current_color = MGL_rgbColor(DC,r,g,b);
}

void vga_setcolor(int index)
{
	DOSMesa->index = index;
	current_color = index;
}

void vga_drawpixel(x,y)
{
	MGL_pixelCoord(x,y);
}

/* TODO: */
int vga_getpixel(x,y)
{
/*	return (int)SV_getPixel(x,y); */
	fprintf(stderr,"vga_getpixel: Not implemented yet\n");
	return 1;
}

/* End of UNIVBE section */
#endif

#ifdef GLIDE

/* GLIDE support */

static GrHwConfiguration hwconfig;

void set_video_mode(unsigned short x,unsigned short y,char mode)
{
	grGlideInit();
	if( grSstQueryHardware( &hwconfig ) ) {
		grSstSelect( 0 ) ;
		if( !grSstOpen( GLIDE_MODE,
							GR_REFRESH_60Hz,
							GR_COLORFORMAT_ABGR,
							GR_ORIGIN_UPPER_LEFT,
							GR_SMOOTHING_ENABLE,
							2 ) ) {
			fprintf(stderr,"Detected 3DFX board, but couldn`t initialize!");
			exit(1);
		}

		grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST);

		grDisableAllEffects();
		atexit( glideshutdown );

//		guColorCombineFunction( GR_COLORCOMBINE_ITRGB );
//		grTexCombineFunction( GR_TMU0, GR_TEXTURECOMBINE_ZERO);
	}
}

void restore_video_mode(void)
{
}

static void glideshutdown( void )
{
	grGlideShutdown() ;
}

void vga_clear(void)
{
	grBufferSwap(0);
	grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST);
}

void vga_flip(void)
{
}

int vga_getcolors(void)
{
	return 32768;
}

int vga_getxdim(void)
{
#if GLIDE_MODE == GR_RESOLUTION_800x600
		return 800;
#else
		return 640;
#endif
}

int vga_getydim(void)
{
#if GLIDE_MODE == GR_RESOLUTION_800x600
		return 600;
#else
		return 480;
#endif
}

unsigned long current_color = 255;

void vga_setrgbcolor(int r,int g,int b)
{
	DOSMesa->red = r;
	DOSMesa->green = g ;
	DOSMesa->blue = b ;
}

void vga_setcolor(int index)
{
	DOSMesa->index = index;
}

void vga_drawpixel(x,y)
{

	gr_vtx.x = x;
	gr_vtx.y = y;
	gr_vtx.z = 0;
	gr_vtx.r	= DOSMesa->red;
	gr_vtx.g	= DOSMesa->green;
	gr_vtx.b	= DOSMesa->blue;

	grDrawPoint( &gr_vtx );
}

static void fast_tri(GLcontext *ctx,GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;

	gr_vtx.z = 0;
	gr_vtx1.z = 0;
	gr_vtx2.z = 0;

   if (VB->MonoColor) {
	  	gr_vtx.r = DOSMesa->red;
   	gr_vtx.g = DOSMesa->green;
   	gr_vtx.b = DOSMesa->blue;
   	gr_vtx1.r = DOSMesa->red;
   	gr_vtx1.g = DOSMesa->green;
   	gr_vtx1.b = DOSMesa->blue;
   	gr_vtx2.r = DOSMesa->red;
   	gr_vtx2.g = DOSMesa->green;
   	gr_vtx2.b = DOSMesa->blue;
   } else {
		if(ctx->Light.ShadeModel == GL_SMOOTH ) {
		  	gr_vtx.r  = FixedToInt( VB->Color[v0][0] );
		  	gr_vtx.g  = FixedToInt( VB->Color[v0][1] );
		  	gr_vtx.b  = FixedToInt( VB->Color[v0][2] );

		  	gr_vtx1.r = FixedToInt( VB->Color[v1][0] );
		  	gr_vtx1.g = FixedToInt( VB->Color[v1][1] );
		  	gr_vtx1.b = FixedToInt( VB->Color[v1][2] );

		  	gr_vtx2.r = FixedToInt( VB->Color[v2][0] );
		  	gr_vtx2.g = FixedToInt( VB->Color[v2][1] );
		  	gr_vtx2.b = FixedToInt( VB->Color[v2][2] );
		} else {
		  	gr_vtx.r  = VB->Color[pv][0];
		  	gr_vtx.g  = VB->Color[pv][1];
		  	gr_vtx.b  = VB->Color[pv][2];

		  	gr_vtx1.r = VB->Color[pv][0];
		  	gr_vtx1.g = VB->Color[pv][1];
		  	gr_vtx1.b = VB->Color[pv][2];

		  	gr_vtx2.r = VB->Color[pv][0];
		  	gr_vtx2.g = VB->Color[pv][1];
		  	gr_vtx2.b = VB->Color[pv][2];
		}
	}

   gr_vtx.x  =       (VB->Win[v0][0] );
   gr_vtx.y  = FLIP( (VB->Win[v0][1] ) );
   gr_vtx1.x =       (VB->Win[v1][0] );
   gr_vtx1.y = FLIP( (VB->Win[v1][1] ) );
   gr_vtx2.x =       (VB->Win[v2][0] );
   gr_vtx2.y = FLIP( (VB->Win[v2][1] ) );

	if(gr_vtx.x <0 || gr_vtx.x > 639 )
		return;
	if(gr_vtx1.x <0 || gr_vtx1.x > 639 )
		return;
	if(gr_vtx2.x <0 || gr_vtx2.x > 639 )
		return;

	if(gr_vtx.y <0 || gr_vtx.y > 479 )
		return;
	if(gr_vtx1.y <0 || gr_vtx1.y > 479 )
		return;
	if(gr_vtx2.y <0 || gr_vtx2.y > 479 )
		return;

	grDrawTriangle( &gr_vtx,&gr_vtx1,&gr_vtx2);
}

void fast_plot(GLcontext *ctx,GLuint first,GLuint last )
{
	struct vertex_buffer *VB = ctx->VB;
	register GLuint i;

	if(VB->MonoColor) {
		/* all same color */

		gr_vtx.r = DOSMesa->red;
		gr_vtx.g = DOSMesa->green;
		gr_vtx.b = DOSMesa->blue;

		for(i=first;i<last;i++) {
			if(VB->ClipMask[i]==0) {
				gr_vtx.x = VB->Win[i][0];
				gr_vtx.y = FLIP(VB->Win[i][1]);
			}
		}
	}
}

/* TODO: */
int vga_getpixel(x,y)
{
/*	return (int)SV_getPixel(x,y); */
	fprintf(stderr,"vga_getpixel: Not implemented yet\n");
	return 1;
}

/* End of GLIDE section */

#endif // GLIDE
/**********************************************************************/
/*****                 Miscellaneous functions                    *****/
/**********************************************************************/


static void get_buffer_size( GLcontext *ctx, GLuint *width, GLuint *height )
{
   *width = DOSMesa->width = vga_getxdim();
   *height = DOSMesa->height = vga_getydim();
}


/* Set current color index */
static void set_index( GLcontext *ctx, GLuint index )
{
   DOSMesa->index = index;
   /*vga_setcolor( index );*/
}


/* Set current drawing color */
static void set_color( GLcontext *ctx,
                       GLubyte red, GLubyte green,
                       GLubyte blue, GLubyte alpha )
{
   DOSMesa->red = red;
   DOSMesa->green = green;
   DOSMesa->blue = blue;
   vga_setrgbcolor( red, green, blue );
}


static void clear_index( GLcontext *ctx, GLuint index )
{
   /* TODO: Implements glClearIndex() */
}


static void clear_color( GLcontext *ctx,
                         GLubyte red, GLubyte green,
                         GLubyte blue, GLubyte alpha )
{
   /* TODO: Implements glClearColor() */
}


static void clear( GLcontext *ctx,
                   GLboolean all,
                   GLint x, GLint y, GLint width, GLint height )
{
   vga_clear();
}


static GLboolean set_buffer( GLcontext *ctx,
                             GLenum mode )
{
   /* TODO: implement double buffering and use this function to select */
   /* between front and back buffers. */
   if (buffer == GL_FRONT_LEFT)
      return GL_TRUE;
   else if (buffer == GL_BACK_LEFT)
      return GL_TRUE;
   else
      return GL_FALSE;
}




/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/


static void write_index_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y,
                              const GLuint index[],
                              const GLubyte mask[] )
{
   int i;
   y = FLIP(y);
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
#ifdef UNIVBE
		SV_putPixelFast(x,y,current_color);
#else
         vga_setcolor( index[i] );
         vga_drawpixel( x, y );
#endif
      }
   }

#ifdef UNIVBE
	SV_endPixel();
#endif

}



static void write_monoindex_span( GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLubyte mask[] )
{
   int i;
   y = FLIP(y);
   /* use current color index */
   vga_setcolor( DOSMesa->index );
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0;i<n;i++,x++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x,y,current_color);
#else
         vga_drawpixel( x, y );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}



static void write_color_span( GLcontext *ctx,
                              GLuint n, GLint x, GLint y,
                              const GLubyte red[], const GLubyte green[],
                              const GLubyte blue[], const GLubyte alpha[],
                              const GLubyte mask[] )
{
   int i;
   y=FLIP(y);
#ifdef UNIVBE
	SV_beginPixel();
#endif
   if (mask) {
      /* draw some pixels */
      for (i=0; i<n; i++, x++) {
         if (mask[i]) {
#ifdef UNIVBE
				SV_putPixelFast(x,y,SV_rgbColor(red[i], green[i], blue[i]) );
#else
            vga_setrgbcolor( red[i], green[i], blue[i] );
            vga_drawpixel( x, y );
#endif
         }
      }
   }
   else {
      /* draw all pixels */
      for (i=0; i<n; i++, x++) {
#ifdef UNIVBE
			SV_putPixelFast(x,y,SV_rgbColor(red[i], green[i], blue[i]) );
#else
         vga_setrgbcolor( red[i], green[i], blue[i] );
         vga_drawpixel( x, y );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}



static void write_monocolor_span( GLcontext *ctx,
                                  GLuint n, GLint x, GLint y,
                                  const GLubyte mask[])
{
   int i;
   y=FLIP(y);
#ifdef UNIVBE
	SV_beginPixel();
#endif
   /* use current rgb color */
   vga_setrgbcolor( DOSMesa->red, DOSMesa->green, DOSMesa->blue );
   for (i=0; i<n; i++, x++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x,y,current_color);
#else
         vga_drawpixel( x, y );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}



/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


static void read_index_span( GLcontext *ctx,
                             GLuint n, GLint x, GLint y, GLuint index[])
{
   int i;
   y = FLIP(y);
   for (i=0; i<n; i++,x++) {
      index[i] = vga_getpixel( x, y );
   }
}



static void read_color_span( GLcontext *ctx,
                             GLuint n, GLint x, GLint y,
                             GLubyte red[], GLubyte green[],
                             GLubyte blue[], GLubyte alpha[] )
{
   int i;
   for (i=0; i<n; i++, x++) {
      /* TODO */
   }
}



/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/


static void write_index_pixels( GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLuint index[], const GLubyte mask[] )
{
   int i;
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0; i<n; i++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x[i], FLIP(y[i]), index[i] );
#else
         vga_setcolor( index[i] );
         vga_drawpixel( x[i], FLIP(y[i]) );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}



static void write_monoindex_pixels( GLcontext *ctx,
                                    GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLubyte mask[] )
{
   int i;
   /* use current color index */
   vga_setcolor( DOSMesa->index );
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0; i<n; i++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x[i], FLIP(y[i]), DOSMesa->index);
#else
         vga_drawpixel( x[i], FLIP(y[i]) );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}



static void write_color_pixels( GLcontext *ctx,
                                GLuint n, const GLint x[], const GLint y[],
                                const GLubyte r[], const GLubyte g[],
                                const GLubyte b[], const GLubyte a[],
                                const GLubyte mask[] )
{
   int i;
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0; i<n; i++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x[i], FLIP(y[i]), SV_rgbColor(r[i], g[i], b[i]) );
#else
         vga_setrgbcolor( r[i], g[i], b[i] );
         vga_drawpixel( x[i], FLIP(y[i]) );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}

static void write_monocolor_pixels( GLcontext *ctx,
                                    GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLubyte mask[] )
{
   int i;
   /* use current rgb color */
   vga_setrgbcolor( DOSMesa->red, DOSMesa->green, DOSMesa->blue );
#ifdef UNIVBE
	SV_beginPixel();
#endif
   for (i=0; i<n; i++) {
      if (mask[i]) {
#ifdef UNIVBE
			SV_putPixelFast(x[i], FLIP(y[i]), current_color );
#else
         vga_drawpixel( x[i], FLIP(y[i]) );
#endif
      }
   }
#ifdef UNIVBE
	SV_endPixel();
#endif
}

/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

/* Read an array of color index pixels. */
static void read_index_pixels( GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLuint index[], const GLubyte mask[] )
{
   int i;
   for (i=0; i<n; i++) {
      index[i] = vga_getpixel( x[i], FLIP(y[i]) );
   }
}



static void read_color_pixels( GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLubyte red[], GLubyte green[],
                               GLubyte blue[], GLubyte alpha[],
                               const GLubyte mask[] )
{
   /* TODO */
}

static void DOSmesa_setup_DD_pointers( GLcontext *ctx )
{
   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */

   ctx->Driver.UpdateState = DOSmesa_setup_DD_pointers;

   ctx->Driver.ClearIndex = clear_index;
   ctx->Driver.ClearColor = clear_color;
   ctx->Driver.Clear = clear;

   ctx->Driver.Index = set_index;
   ctx->Driver.Color = set_color;

   ctx->Driver.SetBuffer = set_buffer;
   ctx->Driver.GetBufferSize = get_buffer_size;

   ctx->Driver.PointsFunc = choose_points_function();
   ctx->Driver.LineFunc = choose_line_function();
   ctx->Driver.TriangleFunc = choose_triangle_function();


   /* Pixel/span writing functions: */
   /* TODO: use different funcs for 8, 16, 32-bit depths */
   ctx->Driver.WriteColorSpan       = write_color_span;
   ctx->Driver.WriteMonocolorSpan   = write_monocolor_span;
   ctx->Driver.WriteColorPixels     = write_color_pixels;
   ctx->Driver.WriteMonocolorPixels = write_monocolor_pixels;
   ctx->Driver.WriteIndexSpan       = write_index_span;
   ctx->Driver.WriteMonoindexSpan   = write_monoindex_span;
   ctx->Driver.WriteIndexPixels     = write_index_pixels;
   ctx->Driver.WriteMonoindexPixels = write_monoindex_pixels;

   /* Pixel/span reading functions: */
   /* TODO: use different funcs for 8, 16, 32-bit depths */
   ctx->Driver.ReadIndexSpan = read_index_span;
   ctx->Driver.ReadColorSpan = read_color_span;
   ctx->Driver.ReadIndexPixels = read_index_pixels;
   ctx->Driver.ReadColorPixels = read_color_pixels;
}

/*
 * Create a new VGA/Mesa context and return a handle to it.
 */
DOSMesaContext DOSMesaCreateContext( void )
{
   DOSMesaContext ctx;
   GLboolean rgb_flag;
   GLfloat redscale, greenscale, bluescale, alphascale;
   GLboolean db_flag = GL_FALSE;
   GLboolean alpha_flag = GL_FALSE;
   int colors;
   GLint index_bits;
   GLint redbits, greenbits, bluebits, alphabits;

#if !defined(UNIVBE) && !defined(GLIDE) && !defined(MGL)
	video_buffer = (unsigned char *) malloc( vga_getxdim() * vga_getydim() );

	memset(video_buffer,0, vga_getxdim() * vga_getydim() );

	memset(rgbtable,0,sizeof( rgbtable ) );
#endif

#if defined( DJGPP ) && !defined(UNIVBE) && !defined(GLIDE)
	dos_seg = _go32_conventional_mem_selector();
#endif

   /* determine if we're in RGB or color index mode */
   colors = vga_getcolors();
   if (colors==32768) {
      rgb_flag = GL_TRUE;
      redscale = greenscale = bluescale = alphascale = 255.0;
      redbits = greenbits = bluebits = 8;
      alphabits = 0;
      index_bits = 0;
   }
   else if (colors==256) {
      rgb_flag = GL_FALSE;
      redscale = greenscale = bluescale = alphascale = 0.0;
      redbits = greenbits = bluebits = alphabits = 0;
      index_bits = 8;
   }
   else {
		restore_video_mode();
      fprintf(stderr,"[%d] >16 bit color not implemented yet!\n",colors);
      return NULL;
   }

   ctx = (DOSMesaContext) calloc( 1, sizeof(struct DOSmesa_context) );
   if (!ctx) {
      return NULL;
   }

   ctx->gl_vis = _mesa_create_visual( rgb_flag,
                                      db_flag,
                                      GL_FALSE, /* stereo */
                                      redbits,
                                      greenbits,
                                      bluebits,
                                      alphabits,
                                      index_bits,
                                      16,   /* depth_size */
                                      8,    /* stencil_size */
                                      16, 16, 16, 16,  /* accum_size */
                                      1 );

   ctx->gl_ctx = _mesa_create_context( ctx->gl_vis,
                                    NULL,  /* share list context */
                                    (void *) ctx
                                  );

   ctx->gl_buffer = _mesa_create_framebuffer( ctx->gl_vis );

   ctx->index = 1;
   ctx->red = ctx->green = ctx->blue = 255;

   ctx->width = ctx->height = 0;  /* temporary until first "make-current" */

   return ctx;
}

/*
 * Destroy the given VGA/Mesa context.
 */
void DOSMesaDestroyContext( DOSMesaContext ctx )
{
   if (ctx) {
      _mesa_destroy_visual( ctx->gl_vis );
      _mesa_destroy_context( ctx->gl_ctx );
      _mesa_destroy_framebuffer( ctx->gl_buffer );
      free( ctx );
      if (ctx==DOSMesa) {
         DOSMesa = NULL;
      }
   }
}



/*
 * Make the specified VGA/Mesa context the current one.
 */
void DOSMesaMakeCurrent( DOSMesaContext ctx )
{
   DOSMesa = ctx;
   _mesa_make_current( ctx->gl_ctx, ctx->gl_buffer );
   DOSmesa_setup_DD_pointers( ctx->gl_ctx );

   if (ctx->width==0 || ctx->height==0) {
      /* setup initial viewport */
      ctx->width = vga_getxdim();
      ctx->height = vga_getydim();
      gl_Viewport( ctx->gl_ctx, 0, 0, ctx->width, ctx->height );
   }
}



/*
 * Return a handle to the current VGA/Mesa context.
 */
DOSMesaContext DOSMesaGetCurrentContext( void )
{
   return DOSMesa;
}


/*
 * Swap front/back buffers for current context if double buffered.
 */
void DOSMesaSwapBuffers( void )
{
#if !defined(UNIVBE)
/* Assume double buffering is available if in UNIVBE,
   if it isn`t its taken care of anyway */
//   if (DOSMesa->gl_vis->DBflag)
#endif
 	{
      vga_flip();
   }
}


#else

/*
 * Need this to provide at least one external definition when DOS is
 * not defined on the compiler command line.
 */

int gl_DOS_dummy_function(void)
{
   return 0;
}

#endif  /*DOS*/

