/*
 * Mesa 3-D graphics library
 * Version:  3.4
 * Copyright (C) 1995-1998  Brian Paul
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
 * DOS/DJGPP glut driver v1.0 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#include <sys/time.h>


#define TIMEDELTA(dest, src1, src2) {				\
   if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) {	\
      (dest).tv_usec += 1000000;				\
      (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1;	\
   } else {							\
      (dest).tv_sec = (src1).tv_sec - (src2).tv_sec;		\
   }								\
}

int APIENTRY glutGet (GLenum type)
{
   
   switch (type) {
   case GLUT_WINDOW_RGBA:
      return 1;
   case GLUT_ELAPSED_TIME: {
      static int inited = 0;
      static struct timeval elapsed, beginning, now;
      if (!inited) {
	 gettimeofday(&beginning, 0);
	 inited = 1;
      }
      gettimeofday(&now, 0);
      TIMEDELTA(elapsed, now, beginning);
      /* Return elapsed milliseconds. */
      return (int) ((elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000));
   }
   default:
      return 0;
   }
}


int APIENTRY glutDeviceGet (GLenum type)
{
 return 0;
}
