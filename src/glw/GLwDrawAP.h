/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#ifndef _GLwDrawAP_h
#define _GLwDrawAP_h


/* MOTIF */
#ifdef __GLX_MOTIF
#include "GLwMDrawA.h"
#else
#include "GLwDrawA.h"
#endif

typedef struct _GLwDrawingAreaClassPart {
  caddr_t extension;
  } GLwDrawingAreaClassPart;


#ifdef __GLX_MOTIF
typedef struct _GLwMDrawingAreaClassRec {
  CoreClassPart               core_class;
  XmPrimitiveClassPart        primitive_class;
  GLwDrawingAreaClassPart     glwDrawingArea_class;
  } GLwMDrawingAreaClassRec;


GLAPI GLwMDrawingAreaClassRec glwMDrawingAreaClassRec;


/* XT */
#else 

typedef struct _GLwDrawingAreaClassRec {
  CoreClassPart               core_class;
  GLwDrawingAreaClassPart     glwDrawingArea_class;
  } GLwDrawingAreaClassRec;

GLAPI GLwDrawingAreaClassRec glwDrawingAreaClassRec;


#endif 



typedef struct {
  /* resources */
  int *                attribList;
  XVisualInfo *        visualInfo;
  Boolean              myList;                /* TRUE if we malloced the attribList*/
  Boolean              myVisual;        /* TRUE if we created the visualInfo*/
  Boolean              installColormap;
  Boolean              allocateBackground;
  Boolean              allocateOtherColors;
  Boolean              installBackground;
  XtCallbackList       ginitCallback;
  XtCallbackList       resizeCallback;
  XtCallbackList       exposeCallback;
  XtCallbackList       inputCallback;
  /* specific attributes; add as we get new attributes */
  int                  bufferSize;
  int                  level;
  Boolean              rgba;
  Boolean              doublebuffer;
  Boolean              stereo;
  int                  auxBuffers;
  int                  redSize;
  int                  greenSize;
  int                  blueSize;
  int                  alphaSize;
  int                  depthSize;
  int                  stencilSize;
  int                  accumRedSize;
  int                  accumGreenSize;
  int                  accumBlueSize;
  int                  accumAlphaSize;
  } GLwDrawingAreaPart;

#ifdef __GLX_MOTIF

typedef struct _GLwMDrawingAreaRec {
  CorePart             core;
  XmPrimitivePart      primitive;
  GLwDrawingAreaPart   glwDrawingArea;
  } GLwMDrawingAreaRec;

#else 

typedef struct _GLwDrawingAreaRec {
  CorePart             core;
  GLwDrawingAreaPart   glwDrawingArea;
  } GLwDrawingAreaRec;

#endif 

#endif
