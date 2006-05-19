/*
 * Test packed pixel formats for textures.
 * Brian Paul
 * 12 May 2004
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>


struct pixel_format {
   const char *name;
   GLenum format;
   GLenum type;
   GLint bytes;
   GLuint redTexel, greenTexel;
};

static const struct pixel_format Formats[] = {

   { "GL_RGBA/GL_UNSIGNED_INT_8_8_8_8",
     GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 4, 0xff000000, 0x00ff0000 },
   { "GL_RGBA/GL_UNSIGNED_INT_8_8_8_8_REV",
     GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, 0x000000ff, 0x0000ff00 },
   { "GL_RGBA/GL_UNSIGNED_INT_10_10_10_2",
     GL_RGBA, GL_UNSIGNED_INT_10_10_10_2, 4, 0xffc00000, 0x3ff000 },
   { "GL_RGBA/GL_UNSIGNED_INT_2_10_10_10_REV",
     GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, 4, 0x3ff, 0xffc00 },
   { "GL_RGBA/GL_UNSIGNED_SHORT_4_4_4_4",
     GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 2, 0xf000, 0x0f00 },
   { "GL_RGBA/GL_UNSIGNED_SHORT_4_4_4_4_REV",
     GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, 0x000f, 0x00f0 },
   { "GL_RGBA/GL_UNSIGNED_SHORT_5_5_5_1",
     GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 2, 0xf800, 0x7c0 },
   { "GL_RGBA/GL_UNSIGNED_SHORT_1_5_5_5_REV",
     GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, 0x1f, 0x3e0 },

   { "GL_BGRA/GL_UNSIGNED_INT_8_8_8_8",
     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 4, 0x0000ff00, 0x00ff0000 },
   { "GL_BGRA/GL_UNSIGNED_INT_8_8_8_8_REV",
     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, 0x00ff0000, 0x0000ff00 },
   { "GL_BGRA/GL_UNSIGNED_SHORT_4_4_4_4",
     GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, 2, 0x00f0, 0x0f00 },
   { "GL_BGRA/GL_UNSIGNED_SHORT_4_4_4_4_REV",
     GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, 0x0f00, 0x00f0 },
   { "GL_BGRA/GL_UNSIGNED_SHORT_5_5_5_1",
     GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1, 2, 0x3e, 0x7c0 },
   { "GL_BGRA/GL_UNSIGNED_SHORT_1_5_5_5_REV",
     GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, 0x7c00, 0x3e0 },

   { "GL_ABGR_EXT/GL_UNSIGNED_INT_8_8_8_8",
     GL_ABGR_EXT, GL_UNSIGNED_INT_8_8_8_8, 4, 0x000000ff, 0x0000ff00 },
   { "GL_ABGR_EXT/GL_UNSIGNED_INT_8_8_8_8_REV",
     GL_ABGR_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, 4, 0xff000000, 0x00ff0000 },
   { "GL_ABGR_EXT/GL_UNSIGNED_SHORT_4_4_4_4",
     GL_ABGR_EXT, GL_UNSIGNED_SHORT_4_4_4_4, 2, 0x000f, 0x00f0 },
   { "GL_ABGR_EXT/GL_UNSIGNED_SHORT_4_4_4_4_REV",
     GL_ABGR_EXT, GL_UNSIGNED_SHORT_4_4_4_4_REV, 2, 0xf000, 0x0f00 },
   { "GL_ABGR_EXT/GL_UNSIGNED_SHORT_5_5_5_1",
     GL_ABGR_EXT, GL_UNSIGNED_SHORT_5_5_5_1, 2, 0x1, 0x3e },
   { "GL_ABGR_EXT/GL_UNSIGNED_SHORT_1_5_5_5_REV",
     GL_ABGR_EXT, GL_UNSIGNED_SHORT_1_5_5_5_REV, 2, 0x8000, 0x7c00 },

   { "GL_RGB/GL_UNSIGNED_SHORT_5_6_5",
     GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 2, 0xf800, 0x7e0 },
   { "GL_RGB/GL_UNSIGNED_SHORT_5_6_5_REV",
     GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, 2, 0x1f, 0x7e0 },
   { "GL_RGB/GL_UNSIGNED_BYTE_3_3_2",
     GL_RGB, GL_UNSIGNED_BYTE_3_3_2, 1, 0xe0, 0x1c },
   { "GL_RGB/GL_UNSIGNED_BYTE_2_3_3_REV",
     GL_RGB, GL_UNSIGNED_BYTE_2_3_3_REV, 1, 0x7, 0x38 },

   { NULL, 0, 0, 0, 0, 0 }
};


struct name_format {
   const char *name;
   GLenum format;
};

static const struct name_format IntFormats[] = {
   { "GL_RGBA", GL_RGBA },
   { "GL_RGBA2", GL_RGBA2 },
   { "GL_RGBA4", GL_RGBA4 },
   { "GL_RGB5_A1", GL_RGB5_A1 },
   { "GL_RGBA8", GL_RGBA8 },
   { "GL_RGBA12", GL_RGBA12 },
   { "GL_RGBA16", GL_RGBA16 },
   { "GL_RGB10_A2", GL_RGB10_A2 },

   { "GL_RGB", GL_RGB },
   { "GL_R3_G3_B2", GL_R3_G3_B2 },
   { "GL_RGB4", GL_RGB4 },
   { "GL_RGB5", GL_RGB5 },
   { "GL_RGB8", GL_RGB8 },
   { "GL_RGB10", GL_RGB10 },
   { "GL_RGB12", GL_RGB12 },
   { "GL_RGB16", GL_RGB16 },

};

#define NUM_INT_FORMATS (sizeof(IntFormats) / sizeof(IntFormats[0]))
static GLuint CurFormat = 0;

static GLboolean Test3D = GL_FALSE;



static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
MakeTexture(const struct pixel_format *format, GLenum intFormat, GLboolean swap)
{
   GLubyte texBuffer[1000];
   int i;

   glPixelStorei(GL_UNPACK_SWAP_BYTES, swap);

   if (format->bytes == 1) {
      for (i = 0; i < 8; i++) {
         texBuffer[i] = format->redTexel;
      }
      for (i = 8; i < 16; i++) {
         texBuffer[i] = format->greenTexel;
      }
   }
   else if (format->bytes == 2) {
      GLushort *us = (GLushort *) texBuffer;
      for (i = 0; i < 8; i++) {
         us[i] = format->redTexel;
      }
      for (i = 8; i < 16; i++) {
         us[i] = format->greenTexel;
      }
      if (swap) {
         for (i = 0; i < 16; i++)
            us[i] = (us[i] << 8) | (us[i] >> 8);
      }
   }
   else if (format->bytes == 4) {
      GLuint *ui = (GLuint *) texBuffer;
      for (i = 0; i < 8; i++) {
         ui[i] = format->redTexel;
      }
      for (i = 8; i < 16; i++) {
         ui[i] = format->greenTexel;
      }
      if (swap) {
         for (i = 0; i < 16; i++) {
            GLuint b = ui[i];
            ui[i] =  (b >> 24)
                  | ((b >> 8) & 0xff00)
                  | ((b << 8) & 0xff0000)
                  | ((b << 24) & 0xff000000);
         }
      }
   }
   else {
      abort();
   }

   if (Test3D) {
      /* 4 x 4 x 4 texture, undefined data */
      glTexImage3D(GL_TEXTURE_3D, 0, intFormat, 4, 4, 4, 0,
                   format->format, format->type, NULL);
      /* fill in Z=1 and Z=2 slices with the real texture data */
      glTexSubImage3D(GL_TEXTURE_3D, 0,
                      0, 0, 1,  /* offset */
                      4, 4, 1,  /* size */
                      format->format, format->type, texBuffer);
      glTexSubImage3D(GL_TEXTURE_3D, 0,
                      0, 0, 2,  /* offset */
                      4, 4, 1,  /* size */
                      format->format, format->type, texBuffer);
   }
   else {
      glTexImage2D(GL_TEXTURE_2D, 0, intFormat, 4, 4, 0,
                   format->format, format->type, texBuffer);
   }

   if (glGetError()) {
      printf("GL Error for %s\n", format->name);
      memset(texBuffer, 255, 1000);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, texBuffer);
   }
}



static void
Draw(void)
{
   char s[1000];
   int w = 350, h = 20;
   int i, swap;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   for (swap = 0; swap < 2; swap++) {
     for (i = 0; Formats[i].name; i++) {
        glPushMatrix();
        glTranslatef(swap * (w + 2), i * (h + 2), 0);

        MakeTexture(Formats + i, IntFormats[CurFormat].format, swap);

        if (Test3D)
           glEnable(GL_TEXTURE_3D);
        else
           glEnable(GL_TEXTURE_2D);
        glBegin(GL_POLYGON);
        glTexCoord3f(0, 0, 0.5);  glVertex2f(0, 0);
        glTexCoord3f(1, 0, 0.5);  glVertex2f(w, 0);
        glTexCoord3f(1, 1, 0.5);  glVertex2f(w, h);
        glTexCoord3f(0, 1, 0.5);  glVertex2f(0, h);
        glEnd();

        if (Test3D)
           glDisable(GL_TEXTURE_3D);
        else
           glDisable(GL_TEXTURE_2D);
        glColor3f(0, 0, 0);
        glRasterPos2i(8, 6);
        PrintString(Formats[i].name);

        glPopMatrix();
     }
   }

   glPushMatrix();
   glTranslatef(2, i * (h + 2), 0);
   glColor3f(1, 1, 1);
   glRasterPos2i(8, 6);
   PrintString("Normal");
   glRasterPos2i(w + 2, 6);
   PrintString("Byte Swapped");
   glPopMatrix();

   glPushMatrix();
   glTranslatef(2, (i + 1) * (h + 2), 0);
   glRasterPos2i(8, 6);
   sprintf(s, "Internal Texture Format [f/F]: %s (%d of %d)",
           IntFormats[CurFormat].name, CurFormat + 1, NUM_INT_FORMATS);
   PrintString(s);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(2, (i + 2) * (h + 2), 0);
   glRasterPos2i(8, 6);
   if (Test3D)
      PrintString("Target [2/3]: GL_TEXTURE_3D");
   else
      PrintString("Target [2/3]: GL_TEXTURE_2D");
   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'F':
         if (CurFormat == 0)
            CurFormat = NUM_INT_FORMATS - 1;
         else
            CurFormat--;
         break;
      case 'f':
         CurFormat++;
         if (CurFormat == NUM_INT_FORMATS)
            CurFormat = 0;
         break;
      case '2':
         Test3D = GL_FALSE;
         break;
      case '3':
         Test3D = GL_TRUE;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(700, 800);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
