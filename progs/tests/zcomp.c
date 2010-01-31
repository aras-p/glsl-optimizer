/**
 * Test Z compositing with glDrawPixels(GL_DEPTH_COMPONENT) and stencil test.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static int Win;
static GLfloat Xrot = 0, Yrot = 0, Zpos = 6;
static GLboolean Anim = GL_FALSE;

static int Width = 400, Height = 200;
static GLfloat *Zimg;
static GLubyte *Cimg;
static GLboolean showZ = 0;


static void
Idle(void)
{
   Xrot += 3.0;
   Yrot += 4.0;
   glutPostRedisplay();
}


/**
 * Draw first object, save color+Z images
 */
static void
DrawFirst(void)
{
   static const GLfloat red[4] = { 1.0, 0.0, 0.0, 0.0 };

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glTranslatef(-1, 0, 0);
   glRotatef(45 + Xrot, 1, 0, 0);

   glutSolidTorus(0.75, 2.0, 10, 20);

   glPopMatrix();

   glReadPixels(0, 0, Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, Zimg);
   glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Cimg);
}


/**
 * Draw second object.
 */
static void
DrawSecond(void)
{
   static const GLfloat blue[4] = { 0.0, 0.0, 1.0, 0.0 };

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   glPushMatrix();
   glTranslatef(+1, 0, 0);
   glRotatef(-45 + Xrot, 1, 0, 0);

   glutSolidTorus(0.75, 2.0, 10, 20);

   glPopMatrix();
}


/**
 * Composite first/saved image over second rendering.
 */
static void
Composite(void)
{
   glWindowPos2i(0, 0);

   /* Draw Z values, set stencil where Z test passes */
   glEnable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS, 1, ~0);
   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
   glColorMask(0,0,0,0);
   glDrawPixels(Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, Zimg);
   glColorMask(1,1,1,1);

   /* Draw color where stencil==1 */
   glStencilFunc(GL_EQUAL, 1, ~0);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
   glDisable(GL_DEPTH_TEST);
   glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Cimg);
   glEnable(GL_DEPTH_TEST);

   glDisable(GL_STENCIL_TEST);
}


static void
Draw(void)
{
   DrawFirst();
   DrawSecond();
   Composite();
   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   GLfloat ar = (float) width / height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   Width = width;
   Height = height;

   if (Zimg)
      free(Zimg);
   if (Cimg)
      free(Cimg);
   Zimg = (float *) malloc(width * height * 4);
   Cimg = (GLubyte *) malloc(width * height * 4);
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 1.0;
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'd':
         showZ = !showZ;
         break;
      case 'z':
         Zpos -= step;
         break;
      case 'Z':
         Zpos += step;
         break;
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   /* setup lighting, etc */
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
