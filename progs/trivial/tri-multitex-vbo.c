/*
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the name of
 * Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF
 * ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

#define NR_VERTS 4

struct {
   GLfloat position[NR_VERTS][4];
   GLubyte color[NR_VERTS][4];
   GLfloat tex0[NR_VERTS][2];
   GLfloat tex1[NR_VERTS][2];
} verts = {

   { {  0.9, -0.9, 0.0, 1.0 },
     {  0.9,  0.9, 0.0, 1.0 },
     { -0.9,  0.9, 0.0, 1.0 },
     { -0.9, -0.9, 0.0, 1.0 } },

   { { 0x00, 0x00, 0xff, 0x00 },
     { 0x00, 0xff, 0x00, 0x00 },
     { 0xff, 0x00, 0x00, 0x00 },
     { 0xff, 0xff, 0xff, 0x00 }
   },

   { {  1, -1 },
     {  1,  1 },
     { -1,  1 },
     { -1, -1 } },

   { { 3, 0 },
     { 0, 3 },
     { -3, 0 },
     {  0, -3} },

};

GLuint indices[] = { 0, 1, 2, 3 };

GLuint arrayObj, elementObj;


GLenum doubleBuffer;


#define Offset(ptr, member) (void *)((const char *)&((ptr)->member) - (const char *)(ptr))

static void Init(void)
{
   GLuint texObj[2];

   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

    glClearColor(0.0, 0.0, 1.0, 0.0);

   glGenTextures(2, texObj);

#define SIZE 32
   {
      GLubyte tex2d[SIZE][SIZE][3];
      GLint s, t;

      for (s = 0; s < SIZE; s++) {
	 for (t = 0; t < SIZE; t++) {
	    tex2d[t][s][0] = s*255/(SIZE-1);
	    tex2d[t][s][1] = t*255/(SIZE-1);
	    tex2d[t][s][2] = 0;
	 }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glBindTexture(GL_TEXTURE_2D, texObj[0]);


      glTexImage2D(GL_TEXTURE_2D, 0, 3, SIZE, SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, tex2d);

      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   }

   {
      GLubyte tex2d[SIZE][SIZE][3];
      GLint s, t;

      for (s = 0; s < SIZE; s++) {
	 for (t = 0; t < SIZE; t++) {
            GLboolean on = ((s/4) ^ (t/4)) & 1;
	    tex2d[t][s][0] = on ? 128 : 0;
	    tex2d[t][s][1] = on ? 128 : 0;
	    tex2d[t][s][2] = on ? 128 : 0;
	 }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glBindTexture(GL_TEXTURE_2D, texObj[1]);

      glTexImage2D(GL_TEXTURE_2D, 0, 3, SIZE, SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, tex2d);

      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   }

   glActiveTextureARB( GL_TEXTURE0_ARB );


   {

      glGenBuffersARB(1, &arrayObj);
      glGenBuffersARB(1, &elementObj);
   
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, arrayObj);
      glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, elementObj);

      glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), &verts, GL_STATIC_DRAW_ARB);
      glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(indices), indices, GL_STATIC_DRAW_ARB);

      glEnableClientState( GL_VERTEX_ARRAY );
      glVertexPointer( 4, GL_FLOAT, 0, Offset(&verts, position) );

      glEnableClientState( GL_COLOR_ARRAY );
      glColorPointer( 4, GL_UNSIGNED_BYTE, 0, Offset(&verts, color) );

      glClientActiveTextureARB( GL_TEXTURE0_ARB );
      glEnableClientState( GL_TEXTURE_COORD_ARRAY );
      glTexCoordPointer( 2, GL_FLOAT, 0, Offset(&verts, tex0) );

      glClientActiveTextureARB( GL_TEXTURE1_ARB );
      glEnableClientState( GL_TEXTURE_COORD_ARRAY );
      glTexCoordPointer( 2, GL_FLOAT, 0, Offset(&verts, tex1) );

      glClientActiveTextureARB( GL_TEXTURE0_ARB );
   }
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
/*     glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0); */
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      default:
	break;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 

   glDrawElements( GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL );

   glFlush();

   if (doubleBuffer) {
      glutSwapBuffers();
   }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else {
	    fprintf(stderr, "%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB | GLUT_ALPHA;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow(*argv) == GL_FALSE) {
	exit(1);
    }

    glewInit();
    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
