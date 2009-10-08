/*
 * Test minification vs. magnification filtering.
 * Draw two quads with different filtering modes:
 *
 *    +--------------------------+  +--------------------------+
 *    |   MagFilter = GL_LINEAR  |  |  MagFilter = GL_LINEAR   |
 *    |   MinFilter = GL_LINEAR  |  |  MinFilter = GL_NEAREST  |
 *    +--------------------------+  +--------------------------+
 *
 * They should look different when the quad is smaller than the level 0
 * texture size (when minifying).
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLint Width = 1000, Height = 500;


static GLint TexWidth = 256, TexHeight = 256;
static GLfloat Zpos = 5;
static GLboolean MipMap = 0*GL_TRUE;
static GLboolean LinearFilter = GL_TRUE;


static void
redraw(void)
{
   GLfloat w = 1.0;
   GLfloat h = 1.0;

   glClear( GL_COLOR_BUFFER_BIT );

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glPushMatrix();
   glTranslatef(-1.5, 0, -Zpos);
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex2f(-w, -h);
      glTexCoord2f(1, 0);  glVertex2f( w, -h);
      glTexCoord2f(1, 1);  glVertex2f( w,  h);
      glTexCoord2f(0, 1);  glVertex2f(-w,  h);
      glEnd();
   glPopMatrix();

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   glPushMatrix();
   glTranslatef(1.5, 0, -Zpos);
      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex2f(-w, -h);
      glTexCoord2f(1, 0);  glVertex2f( w, -h);
      glTexCoord2f(1, 1);  glVertex2f( w,  h);
      glTexCoord2f(0, 1);  glVertex2f(-w,  h);
      glEnd();
   glPopMatrix();

   glutSwapBuffers();
}


static void
init(void)
{
   GLubyte color[10][4] = {
      { 0, 0, 0, 0 },
      { 1, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 1, 0, 1, 0 },
      { 1, 1, 0, 0 },
      { 1, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 0, 1, 0 }
   };
   GLubyte *texImage;

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("Left quad should be linear filtered and right should be nearest filtered.\n");
   printf("Press z/Z to change quad distance.\n");

   texImage = (GLubyte*) malloc(4 * TexWidth * TexHeight * sizeof(GLubyte));
   assert(texImage);

   {
      GLint level = 0;
      GLint w = TexWidth, h = TexHeight;
      while (1) {
         int i, j;

         for (i = 0; i < h; i++) {
            for (j = 0;j < w; j++) {
               if (w==1 || h==1 || (((i / 2) ^ (j / 2)) & 1)) {
                  /*if (j < i) {*/
                  texImage[(i*w+j) * 4 + 0] = 255;
                  texImage[(i*w+j) * 4 + 1] = 255;
                  texImage[(i*w+j) * 4 + 2] = 255;
                  texImage[(i*w+j) * 4 + 3] = 255;
               }
               else {
                  texImage[(i*w+j) * 4 + 0] = color[level][0] * 255;
                  texImage[(i*w+j) * 4 + 1] = color[level][1] * 255;
                  texImage[(i*w+j) * 4 + 2] = color[level][2] * 255;
                  texImage[(i*w+j) * 4 + 3] = color[level][3] * 255;
               }
            }
         }

         glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA8, w, h, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, texImage);

         printf("Texture level %d: %d x %d\n", level, w, h);
         if (!MipMap)
            break;

         if (w == 1 && h == 1)
            break;
         if (w > 1)
            w /= 2;
         if (h > 1)
            h /= 2;
         level++;
      }
   }

   free(texImage);

   glClearColor(0.25, 0.25, 0.25, 1.0);
   glEnable(GL_TEXTURE_2D);

   glViewport(0, 0, Width, Height);
}



static void
Reshape(int width, int height)
{
   float ar = (float) width /height;
   Width = width;
   Height = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -1.0, 1.0, 5.0, 2500.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 'z':
      Zpos--;
      break;
   case 'Z':
      Zpos++;
      break;
   case 'f':
      LinearFilter = !LinearFilter;
      break;
   case 27:
      exit(0);
      break;
   }
   glutPostRedisplay();
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(redraw);
   init();
   glutMainLoop();
   return 0;
}
