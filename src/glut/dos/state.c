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
 * DOS/DJGPP glut driver v0.2 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "GL/glut.h"
#include "internal.h"


#define FREQUENCY 100


static int timer_installed;
static volatile int ticks;

static void ticks_timer (void *p)
{
 (void)p;
 ticks++;
} ENDOFUNC(ticks_timer)



int APIENTRY glutGet (GLenum type)
{
 switch (type) {
        case GLUT_WINDOW_RGBA:
             return 1;
        case GLUT_ELAPSED_TIME:
             if (!timer_installed) {
                timer_installed = !timer_installed;
                LOCKDATA(ticks);
                LOCKFUNC(ticks_timer);
                pc_install_int(ticks_timer, NULL, FREQUENCY);
             }
             return ticks*1000/FREQUENCY;
        default:
             return 0;
 }
}


int APIENTRY glutDeviceGet (GLenum type)
{
 return 0;
}
