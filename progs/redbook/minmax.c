/*
 * Copyright (c) 1993-2003, Silicon Graphics, Inc.
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation,
 * and that the name of Silicon Graphics, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS" AND
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION, LOSS OF
 * PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD
 * PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor clauses
 * in the FAR or the DOD or NASA FAR Supplement.  Unpublished - rights
 * reserved under the copyright laws of the United States.
 *
 * Contractor/manufacturer is:
 *	Silicon Graphics, Inc.
 *	1500 Crittenden Lane
 *	Mountain View, CA  94043
 *	United State of America
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/*
 *  minmax.c
 *  Determine the minimum and maximum values of a group of pixels.
 *  This demonstrates use of the glMinmax() call.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>


static GLubyte  *pixels;
static GLsizei   width, height;


static GLuint bswap(GLuint x)
{
   const GLuint ui = 1;
   const GLubyte *ubp = (const GLubyte *) &ui;
   if (*ubp == 1) {
      /* we're on little endiang so byteswap x */
      GLsizei y =  ((x >> 24)
                    | ((x >> 8) & 0xff00)
                    | ((x << 8) & 0xff0000)
                    | ((x << 24) & 0xff000000));
      return y;
   }
   else {
      return x;
   }
}


static GLubyte *
readImage( const char* filename, GLsizei* width, GLsizei *height )
{
    int       n;
    GLubyte*  pixels;

    FILE* infile = fopen( filename, "rb" );

    if ( !infile ) {
	fprintf( stderr, "Unable to open file '%s'\n", filename );
	return NULL;
    }

    fread( width, sizeof( GLsizei ), 1, infile );
    fread( height, sizeof( GLsizei ), 1, infile );

    *width = bswap(*width);
    *height = bswap(*height);

    n = 3 * (*width) * (*height);

    pixels = (GLubyte *) malloc( n * sizeof( GLubyte ));
    if ( !pixels ) {
	fprintf( stderr, "Unable to malloc() bytes for pixels\n" );
	return NULL;
    }

    fread( pixels, sizeof( GLubyte ), n, infile );
    
    fclose( infile );

    return pixels;
}

static void init(void)
{
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glClearColor(0.0, 0.0, 0.0, 0.0);

   glMinmax(GL_MINMAX, GL_RGB, GL_FALSE);
   glEnable(GL_MINMAX);
}

static void display(void)
{
   GLubyte  values[6];
   
   glClear(GL_COLOR_BUFFER_BIT);
   glRasterPos2i(1, 1);
   glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
   glFlush();
   
   glGetMinmax(GL_MINMAX, GL_TRUE, GL_RGB, GL_UNSIGNED_BYTE, values);
   printf(" Red   : min = %d   max = %d\n", values[0], values[3]);
   printf(" Green : min = %d   max = %d\n", values[1], values[4]);
   printf(" Blue  : min = %d   max = %d\n", values[2], values[5]);
}

static void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, 0, h, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
   }
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
   pixels = readImage("leeds.bin", &width, &height);    

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
   glutInitWindowSize(width, height);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   init();
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutDisplayFunc(display);
   glutMainLoop();
   return 0;
}
