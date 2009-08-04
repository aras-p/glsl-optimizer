/**
 * Test glGetTexImage()
 * Brian Paul
 * 9 June 2009
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Win;


static void
TestGetTexImage(GLboolean npot)
{
   GLuint iter;
   GLubyte *data = (GLubyte *) malloc(1024 * 1024 * 4);
   GLubyte *data2 = (GLubyte *) malloc(1024 * 1024 * 4);

   glEnable(GL_TEXTURE_2D);

   printf("glTexImage2D + glGetTexImage:\n");

   for (iter = 0; iter < 8; iter++) {
      GLint p = (iter % 8) + 3;
      GLint w = npot ? (p * 20) : (1 << p);
      GLint h = npot ? (p * 10) : (1 << p);
      GLuint i;
      GLint level = 0;

      printf("  Testing %d x %d tex image\n", w, h);

      /* fill data */
      for (i = 0; i < w * h * 4; i++) {
         data[i] = i & 0xff;
         data2[i] = 0;
      }

      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, data);

      glBegin(GL_POINTS);
      glVertex2f(0, 0);
      glEnd();

      /* get */
      glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, data2);

      /* compare */
      for (i = 0; i < w * h * 4; i++) {
         if (data2[i] != data[i]) {
            printf("glTexImage + glGetTexImage failure!\n");
            printf("Expected value %d, found %d\n", data[i], data2[i]);
            abort();
         }
      }
   }

   printf("Passed\n");
   glDisable(GL_TEXTURE_2D);
   free(data);
   free(data2);
}


static GLboolean
ColorsEqual(const GLubyte ref[4], const GLubyte act[4])
{
   if (abs((int) ref[0] - (int) act[0]) > 1 ||
       abs((int) ref[1] - (int) act[1]) > 1 ||
       abs((int) ref[2] - (int) act[2]) > 1 ||
       abs((int) ref[3] - (int) act[3]) > 1) {
      printf("expected %d %d %d %d\n", ref[0], ref[1], ref[2], ref[3]);
      printf("found    %d %d %d %d\n", act[0], act[1], act[2], act[3]);
      return GL_FALSE;
   }
   return GL_TRUE;
}


static void
TestGetTexImageRTT(GLboolean npot)
{
   GLuint iter;

   printf("Render to texture + glGetTexImage:\n");

   for (iter = 0; iter < 8; iter++) {

      GLuint fb, tex;
      GLint w, h;
      GLint level = 0;

      if (npot) {
         w = 200 + iter * 40;
         h = 200 + iter * 12;
      }
      else {
         w = 4 << iter;
         h = 4 << iter;
      }

      glGenTextures(1, &tex);
      glGenFramebuffersEXT(1, &fb);

      glBindTexture(GL_TEXTURE_2D, tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, NULL);

      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                GL_TEXTURE_2D, tex, level);

      glViewport(0, 0, w, h);

      printf("  Testing %d x %d tex image\n", w, h);
      {
         static const GLubyte blue[4] = {0, 0, 255, 255};
         GLubyte color[4];
         GLubyte *data2 = (GLubyte *) malloc(w * h * 4);
         GLuint i;

         /* random clear color */
         for (i = 0; i < 4; i++) {
            color[i] = rand() % 256;
         }

         glClearColor(color[0] / 255.0,
                      color[1] / 255.0,
                      color[2] / 255.0,
                      color[3] / 255.0);

         glClear(GL_COLOR_BUFFER_BIT);

         /* draw polygon over top half, in blue */
         glColor4ubv(blue);
         glRectf(0, 0.5, 1.0, 1.0);

         /* get */
         glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, data2);

         /* compare */
         for (i = 0; i < w * h; i += 4) {
            if (i < w * h / 2) {
               /* lower half */
               if (!ColorsEqual(color, data2 + i * 4)) {
                  printf("Render to texture failure (expected clear color)!\n");
                  abort();
               }
            }
            else {
               /* upper half */
               if (!ColorsEqual(blue, data2 + i * 4)) {
                  printf("Render to texture failure (expected blue)!\n");
                  abort();
               }
            }
         }

         free(data2);
      }

      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      glDeleteFramebuffersEXT(1, &fb);
      glDeleteTextures(1, &tex);

   }

   printf("Passed\n");
}




static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   TestGetTexImage(GL_FALSE);
   if (glutExtensionSupported("GL_ARB_texture_non_power_of_two"))
      TestGetTexImage(GL_TRUE);

   if (glutExtensionSupported("GL_EXT_framebuffer_object") ||
       glutExtensionSupported("GL_ARB_framebuffer_object")) {
      TestGetTexImageRTT(GL_FALSE);
      if (glutExtensionSupported("GL_ARB_texture_non_power_of_two"))
         TestGetTexImageRTT(GL_TRUE);
   }

   glutDestroyWindow(Win);
   exit(0);

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 1, 0, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, 0.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
