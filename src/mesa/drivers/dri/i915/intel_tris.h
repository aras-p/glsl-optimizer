/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 **************************************************************************/

#ifndef INTELTRIS_INC
#define INTELTRIS_INC

#include "mtypes.h"

#define _INTEL_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |		\
			       _DD_NEW_TRI_UNFILLED |		\
			       _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			       _DD_NEW_TRI_OFFSET |		\
			       _DD_NEW_TRI_STIPPLE |		\
			       _NEW_PROGRAM |		\
			       _NEW_POLYGONSTIPPLE)

extern void intelInitTriFuncs( GLcontext *ctx );

extern void intelPrintRenderState( const char *msg, GLuint state );
extern void intelChooseRenderState( GLcontext *ctx );

#endif
