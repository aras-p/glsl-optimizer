/*
 * Test mipmap generation and lod bias.
 *
 * Brian Paul
 * 17 March 2008
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glext.h>

#include "readtex.h"

#define TEXTURE_FILE "../images/arch.rgb"

static int TexWidth = 256, TexHeight = 256;
static int WinWidth = 1044, WinHeight = 900;
static GLfloat Bias = 0.0;
static GLboolean ScaleQuads = GL_FALSE;
static GLboolean Linear = GL_FALSE;
static GLint Win = 0;



static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
Display(void)
{
   int x, y, bias;
   char str[100];
   int texWidth = TexWidth, texHeight = TexHeight;

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, WinWidth, 0, WinHeight, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   glColor3f(1,1,1);

   if (Linear) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   }
   else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   }

   y = WinHeight - 300;
   x = 4;

   for (bias = -1; bias < 11; bias++) {

      if (ScaleQuads) {
         if (bias > 0) {
            if (texWidth == 1 && texHeight == 1)
               break;
            texWidth = TexWidth >> bias;
            texHeight = TexHeight >> bias;
            if (texWidth < 1)
               texWidth = 1;
            if (texHeight < 1)
               texHeight = 1;
         }
         glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, 0.0);
      }
      else {
         glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, bias);
      }

      glRasterPos2f(x, y + TexHeight + 5);
      if (ScaleQuads)
         sprintf(str, "Texture Level %d: %d x %d",
                 (bias < 0 ? 0 : bias),
                 texWidth, texHeight);
      else
         sprintf(str, "Texture LOD Bias = %d", bias);
      PrintString(str);

      glPushMatrix();
      glTranslatef(x, y, 0);

      glEnable(GL_TEXTURE_2D);

      glBegin(GL_POLYGON);
      glTexCoord2f(0, 0);  glVertex2f(0, 0);
      glTexCoord2f(1, 0);  glVertex2f(texWidth, 0);
      glTexCoord2f(1, 1);  glVertex2f(texWidth, texHeight);
      glTexCoord2f(0, 1);  glVertex2f(0, texHeight);
      glEnd();

      glPopMatrix();

      glDisable(GL_TEXTURE_2D);

      x += TexWidth + 4;
      if (x >= WinWidth) {
         x = 4;
         y -= 300;
      }
   }

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Bias -= 10;
         break;
      case 'B':
         Bias += 10;
         break;
      case 'l':
         Linear = !Linear;
         break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         Bias = 100.0 * (key - '0');
         break;
      case 's':
         ScaleQuads = !ScaleQuads;
         break;
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
   GLfloat maxBias;

   if (!glutExtensionSupported("GL_EXT_texture_lod_bias")) {
      printf("Sorry, GL_EXT_texture_lod_bias not supported by this renderer.\n");
      exit(1);
   }

   if (!glutExtensionSupported("GL_SGIS_generate_mipmap")) {
      printf("Sorry, GL_SGIS_generate_mipmap not supported by this renderer.\n");
      exit(1);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if (1) {
      /* test auto mipmap generation */
      GLint width, height, i;
      GLenum format;
      GLubyte *image = LoadRGBImage(TEXTURE_FILE, &width, &height, &format);
      if (!image) {
         printf("Error: could not load texture image %s\n", TEXTURE_FILE);
         exit(1);
      }
      /* resize to TexWidth x TexHeight */
      if (width != TexWidth || height != TexHeight) {
         GLubyte *newImage = malloc(TexWidth * TexHeight * 4);
         gluScaleImage(format, width, height, GL_UNSIGNED_BYTE, image,
                       TexWidth, TexHeight, GL_UNSIGNED_BYTE, newImage);
         free(image);
         image = newImage;
      }
      printf("Using GL_SGIS_generate_mipmap\n");
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
      glTexImage2D(GL_TEXTURE_2D, 0, format, TexWidth, TexHeight, 0,
                   format, GL_UNSIGNED_BYTE, image);
      free(image);

      /* make sure mipmap was really generated correctly */
      width = TexWidth;
      height = TexHeight;
      for (i = 0; i < 9; i++) {
         GLint w, h;
         glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_WIDTH, &w);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_HEIGHT, &h);
         printf("Level %d size: %d x %d\n", i, w, h);
         assert(w == width);
         assert(h == height);
         width /= 2;
         height /= 2;
      }
   }
   else {
      if (LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
         printf("Using gluBuildMipmaps()\n");
      }
      else {
         printf("Error: could not load texture image %s\n", TEXTURE_FILE);
         exit(1);
      }
   }


   /* mipmapping required for this extension */
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &maxBias);

   printf("GL_RENDERER: %s\n", (char*) glGetString(GL_RENDERER));
   printf("LOD bias range: [%g, %g]\n", -maxBias, maxBias);

   printf("Press 's' to toggle quad scaling\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
   glutMainLoop();
   return 0;
}
