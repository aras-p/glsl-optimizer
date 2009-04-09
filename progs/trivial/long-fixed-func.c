/**
 * Enable as much fixed-function vertex processing state as possible
 * to test fixed-function -> program code generation.
 */



#include <GL/glew.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>


static void
Reshape(int width, int height)
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode(GL_MODELVIEW);
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 

   glBegin(GL_TRIANGLES);
   glColor3f(.8,0,0); 
   glVertex3f(-0.9, -0.9, -30.0);
   glColor3f(0,.9,0); 
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(0,0,.7); 
   glVertex3f( 0.0,  0.9, -30.0);
   glEnd();

   glFlush();

   glutSwapBuffers();
}


static void
Init(void)
{
   GLubyte tex[16][16][4];
   GLfloat pos[4] = {5, 10, 3, 1.0};
   int i, j;

   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

   glClearColor(0.3, 0.1, 0.3, 0.0);

   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         if ((i+j) & 1) {
            tex[i][j][0] = 100;
            tex[i][j][1] = 100;
            tex[i][j][2] = 100;
            tex[i][j][3] = 255;
         }
         else {
            tex[i][j][0] = 200;
            tex[i][j][1] = 200;
            tex[i][j][2] = 200;
            tex[i][j][3] = 255;
         }
      }
   }


   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
   glFogi(GL_FOG_MODE, GL_LINEAR);
   glEnable(GL_FOG);

   glPointParameterfv(GL_DISTANCE_ATTENUATION_EXT, pos);

   for (i = 0; i < 8; i++) {
      GLuint texObj;

      glEnable(GL_LIGHT0 + i);
      glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 3.5);
      glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 30.);
      glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 3.);
      glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 3.);
      glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 3.);
      glLightfv(GL_LIGHT0 + i, GL_POSITION, pos);

      glActiveTexture(GL_TEXTURE0 + i);
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
      glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
      glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_R);
      glEnable(GL_TEXTURE_GEN_Q);
      glEnable(GL_TEXTURE_2D);

      glMatrixMode(GL_TEXTURE);
      glScalef(2.0, 1.0, 3.0);

      glGenTextures(1, &texObj);
      glBindTexture(GL_TEXTURE_2D, texObj);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   }

   glEnable(GL_LIGHTING);
   glActiveTexture(GL_TEXTURE0);
   glMatrixMode(GL_MODELVIEW);
}


static void
Key(unsigned char key, int x, int y)
{
   if (key == 27) {
      exit(0);
   }
   glutPostRedisplay();
}


int
main(int argc, char **argv)
{
    GLenum type = GLUT_RGB | GLUT_DOUBLE;

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize( 250, 250);
    glutInitDisplayMode(type);
    if (glutCreateWindow(*argv) == GL_FALSE) {
       exit(1);
    }
    glewInit();
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    Init();
    glutMainLoop();
    return 0;
}
