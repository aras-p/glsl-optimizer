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
 *  convolution.c
 *  Use various 2D convolutions filters to find edges in an image.
 *  
 */
#include <GL/glew.h>
#include <GL/glut.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


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
    size_t    num_read;

    FILE* infile = fopen( filename, "rb" );

    if ( !infile ) {
	fprintf( stderr, "Unable to open file '%s'\n", filename );
        exit(1);
    }

    num_read = fread( width, sizeof( GLsizei ), 1, infile );
    assert(num_read == 1);
    num_read = fread( height, sizeof( GLsizei ), 1, infile );
    assert(num_read == 1);

    *width = bswap(*width);
    *height = bswap(*height);

    assert(*width > 0);
    assert(*height > 0);

    n = 3 * (*width) * (*height);

    pixels = (GLubyte *) malloc( n * sizeof( GLubyte ));
    if ( !pixels ) {
	fprintf( stderr, "Unable to malloc() bytes for pixels\n" );
	fclose( infile );
	return NULL;
    }

    num_read = fread( pixels, sizeof( GLubyte ), n, infile );
    assert(num_read == n);
    
    fclose( infile );

    return pixels;
}


GLubyte  *pixels;
GLsizei   width, height;

GLfloat  horizontal[3][3] = {
    { 0, -1, 0 },
    { 0,  1, 0 },
    { 0,  0, 0 }
};

GLfloat  vertical[3][3] = {
    {  0, 0, 0 },
    { -1, 1, 0 },
    {  0, 0, 0 }
};

GLfloat  laplacian[3][3] = {
    { -0.125, -0.125, -0.125 },
    { -0.125,  1.0  , -0.125 },
    { -0.125, -0.125, -0.125 },
};


static void init(void)
{
   if (!glutExtensionSupported("GL_ARB_imaging")) {
      fprintf(stderr, "Sorry, this program requires GL_ARB_imaging.\n");
      exit(1);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glClearColor(0.0, 0.0, 0.0, 0.0);

   printf("Using the horizontal filter\n");
   glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_LUMINANCE,
			 3, 3, GL_LUMINANCE, GL_FLOAT, horizontal);
   glEnable(GL_CONVOLUTION_2D);
}

static void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);
   glRasterPos2i( 1, 1);
   glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
   glFlush();
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
       case 'h' :
	   printf("Using a horizontal filter\n");
	   glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_LUMINANCE, 3, 3,
				  GL_LUMINANCE, GL_FLOAT, horizontal);
	   break;

       case 'v' : 
	   printf("Using the vertical filter\n");
	   glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_LUMINANCE, 3, 3,
				  GL_LUMINANCE, GL_FLOAT, vertical);
	   break;

       case 'l' :
	   printf("Using the laplacian filter\n");
	   glConvolutionFilter2D(GL_CONVOLUTION_2D, GL_LUMINANCE, 3, 3,
				  GL_LUMINANCE, GL_FLOAT, laplacian);
	   break;

       case 27:
         exit(0);
   }
   glutPostRedisplay();
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
   pixels = readImage("leeds.bin", &width, &height);

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
   glutInitWindowSize(width, height);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   glewInit();
   init();
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutDisplayFunc(display);
   glutMainLoop();
   return 0;
}
