
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * Copyright (C) 1995-2000  Brian Paul
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
 * This file includes all .h files needed for the GLU source code for
 * the purpose of precompiled headers.
 *
 * If the preprocessor symbol PCH is defined at compile time then each
 * of the .c files will #include "all.h" only, instead of a bunch of
 * individual .h files.
 */


#ifndef GLU_ALL_H
#define GLU_ALL_H


#ifndef PC_HEADER
This is an error.  all.h should be included only if PCH is defined.
#endif


#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/gl.h"
#include "GL/glu.h"
#include "gluP.h"
#include "nurbs.h"
#include "tess.h"


#endif /*GLU_ALL_H */
