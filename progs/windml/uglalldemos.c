
/* uglalldemos.c - WindML/Mesa example program */

/* Copyright (C) 2001 by Wind River Systems, Inc */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * The MIT License
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
 * THE AUTHORS OR COPYRIGHT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
modification history
--------------------
01a,17jul01,sra  written
*/

/*
DESCRIPTION
Show all the UGL/Mesa demos
*/

#include <vxWorks.h>
#include <taskLib.h>

void windMLPoint (void);
void windMLLine (void);
void windMLFlip (void);
void windMLCube (void);
void windMLBounce (void);
void windMLGears (void);
void windMLIcoTorus (void);
void windMLOlympic (void);
void windMLTexCube (void);
void windMLTexCyl (void);
void windMLTeapot (void);
void windMLStencil (void);
void windMLDrawPix (void);
void windMLAccum (void);
void windMLAllDemos (void);

void uglalldemos (void)
    {
    taskSpawn("tAllDemos", 210, VX_FP_TASK, 200000,
	      (FUNCPTR)windMLAllDemos, 0,1,2,3,4,5,6,7,8,9);
    }

void windMLAllDemos(void)
    {

    windMLPoint();

    windMLLine();
    
    windMLFlip();

    windMLCube();

    windMLBounce();

    windMLGears();

    windMLIcoTorus();

    windMLOlympic();

    windMLTexCube();
    
    windMLTexCyl();

    windMLTeapot();

    windMLStencil();

    windMLDrawPix();

    windMLAccum();

    return;
    }
