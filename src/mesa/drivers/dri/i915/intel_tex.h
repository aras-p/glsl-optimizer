/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 **************************************************************************/

#ifndef INTELTEX_INC
#define INTELTEX_INC

#include "mtypes.h"
#include "intel_context.h"
#include "texmem.h"


void intelInitTextureFuncs( struct dd_function_table *functions );

void intelDestroyTexObj( intelContextPtr intel, intelTextureObjectPtr t );
int intelUploadTexImages( intelContextPtr intel, intelTextureObjectPtr t,
			  GLuint face );

#endif
