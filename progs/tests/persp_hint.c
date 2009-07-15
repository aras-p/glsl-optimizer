/*
 * Test the GL_PERSPECTIVE_CORRECTION_HINT setting and its effect on
 * color interpolation.
 *
 * Press 'i' to toggle between GL_NICEST/GL_FASTEST/GL_DONT_CARE.
 *
 * Depending on the driver, the hint may make a difference, or not.
 */


#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>


static GLenum PerspHint = GL_DONT_CARE;


static void
init(void)
{
   GLubyte image[256][256][4];
   GLuint i, j;
   for (i = 0; i < 256; i++) {
      for (j = 0; j < 256; j++) {
         image[i][j][0] = j;
         image[i][j][1] = j;
         image[i][j][2] = j;
         image[i][j][3] = 255;
      }
   }
   glTexImage2D(GL_TEXTURE_2D, 0, 4, 256, 256, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
}


static void
display(void)
{
   switch (PerspHint) {
   case GL_NICEST:
      printf("hint = GL_NICEST\n");
      break;
   case GL_FASTEST:
      printf("hint = GL_FASTEST\n");
      break;
   case GL_DONT_CARE:
      printf("hint = GL_DONT_CARE\n");
      break;
   default:
      ;
   }

   glClear(GL_COLOR_BUFFER_BIT);

#if 1
   glBegin(GL_QUADS);
   /* exercise perspective interpolation */
   glColor3f(0.0, 0.0, 0.0); glVertex3f(-1.0, -1.0, -1.0);
   glColor3f(0.0, 0.0, 0.0); glVertex3f(-1.0, 1.0, -1.0);
   glColor3f(0.0, 1.0, 0.0); glVertex3f( 7.0, 1.0, -7.0);
   glColor3f(0.0, 1.0, 0.0); glVertex3f( 7.0, -1.0, -7.0);

   /* stripe of linear interpolation */
   glColor3f(0.0, 0.0, 0.0); glVertex3f(-1.0, -0.1, -1.001);
   glColor3f(0.0, 0.0, 0.0); glVertex3f(-1.0,  0.1, -1.001);
   glColor3f(0.0, 1.0, 0.0); glVertex3f( 1.0,  0.1, -1.001);
   glColor3f(0.0, 1.0, 0.0); glVertex3f( 1.0, -0.1, -1.001);
   glEnd();
#else
   glEnable(GL_TEXTURE_2D);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, 0.0, -1.0);
   glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, 1.0, -1.0);
   glTexCoord2f(1.0, 1.0); glVertex3f( 5.0, 1.0, -7.0);
   glTexCoord2f(1.0, 0.0); glVertex3f( 5.0, 0.0, -7.0);

   glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, -1.001);
   glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, 0.0, -1.001);
   glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, 0.0, -1.001);
   glTexCoord2f(1.0, 0.0); glVertex3f( 1.0, -1.0, -1.001);
   glEnd();
#endif

   glFlush();
}


static void
reshape(int w, int h)
{
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(90.0, (GLfloat) w / h, 1.0, 300.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
key(unsigned char k, int x, int y)
{
   switch (k) {
   case 27:  /* Escape */
      exit(0);
      break;
   case 'i':
      if (PerspHint == GL_FASTEST)
         PerspHint = GL_NICEST;
      else if (PerspHint == GL_NICEST)
         PerspHint = GL_DONT_CARE;
      else
         PerspHint = GL_FASTEST;
      glHint(GL_PERSPECTIVE_CORRECTION_HINT, PerspHint);
      break;
   default:
      return;
   }
   glutPostRedisplay();
}


int
main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize (500, 500);
    glutCreateWindow (argv[0]);
    glutReshapeFunc (reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);

    printf("Main quad: perspective projection\n");
    printf("Middle stripe: linear interpolation\n");
    printf("Press 'i' to toggle interpolation hint\n");
    init();

    glutMainLoop();
    return 0;
}
