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
/*
 *  xfont.c
 *  Draws some text in a bitmapped font.  Uses glBitmap() 
 *  and other pixel routines.  Also demonstrates use of 
 *  display lists.
 */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glaux.h"

GLuint base;

void makeRasterFont(void)
{
    XFontStruct *fontInfo;
    Font id;
    unsigned int first, last;
    Display *xdisplay;

    xdisplay = auxXDisplay ();
    fontInfo = XLoadQueryFont(xdisplay, 
	"-adobe-helvetica-medium-r-normal--17-120-100-100-p-88-iso8859-1");
    if (fontInfo == NULL) {
        printf ("no font found\n");
	exit (0);
    }

    id = fontInfo->fid;
    first = fontInfo->min_char_or_byte2;
    last = fontInfo->max_char_or_byte2;

    base = glGenLists((GLuint) last+1);
    if (base == 0) {
        printf ("out of display lists\n");
	exit (0);
    }
    glXUseXFont(id, first, last-first+1, base+first);
/*    *height = fontInfo->ascent + fontInfo->descent;
    *width = fontInfo->max_bounds.width;  */
}

void printString(char *s)
{
    glPushAttrib (GL_LIST_BIT);
    glListBase(base);
    glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *)s);
    glPopAttrib ();
}

void myinit (void) 
{
    makeRasterFont ();
    glShadeModel (GL_FLAT);    
}

void display(void)
{
    GLfloat white[3] = { 1.0, 1.0, 1.0 };
    int i, j;
    char teststring[33];

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3fv(white);
    for (i = 32; i < 127; i += 32) {
	glRasterPos2i(20, 200 - 18*(GLint) i/32);
	for (j = 0; j < 32; j++)
	    teststring[j] = (char) (i+j);
	teststring[32] = 0;
	printString(teststring);
    }
    glRasterPos2i(20, 100);
    printString("The quick brown fox jumps");
    glRasterPos2i(20, 82);
    printString("over a lazy dog.");
    glFlush ();
}

void myReshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho (0.0, (GLfloat) w, 0.0, (GLfloat) h, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB);
    auxInitPosition (0, 0, 500, 500);
    if (!auxInitWindow (argv[0]))
       auxQuit();
    auxReshapeFunc (myReshape);
    myinit ();
    auxMainLoop(display);
    return 0;
}




