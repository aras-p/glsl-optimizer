/* $Id: texdown.c,v 1.3 2000/03/29 18:02:52 brianp Exp $ */

/*
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * texdown
 *
 * Measure texture download speed.
 * Use keyboard to change texture size, format, datatype, scale/bias,
 * subimageload, etc.
 *
 * Brian Paul  28 January 2000
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static GLsizei MaxSize = 1024;
static GLsizei TexWidth = 256, TexHeight = 256, TexBorder = 0;
static GLboolean ScaleAndBias = GL_FALSE;
static GLboolean SubImage = GL_FALSE;
static GLdouble DownloadRate = 0.0;  /* texels/sec */

static GLuint Mode = 0;


#define NUM_FORMATS 4
struct FormatRec {
   GLenum Format;
   GLenum Type;
   GLenum IntFormat;
   GLint TexelSize;
};


static const struct FormatRec FormatTable[NUM_FORMATS] = {
  /* Format   Type                         IntFormat   TexelSize */
   { GL_RGB,  GL_UNSIGNED_BYTE,            GL_RGB,         3    },
   { GL_RGBA, GL_UNSIGNED_BYTE,            GL_RGBA,        4    },
   { GL_RGBA, GL_UNSIGNED_BYTE,            GL_RGB,         4    },
   { GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,     GL_RGB,         2    },
};
static GLint Format;


static int
BytesPerTexel(GLint format)
{
   return FormatTable[format].TexelSize;
}


static const char *
FormatStr(GLenum format)
{
   switch (format) {
      case GL_RGB:
         return "GL_RGB";
      case GL_RGBA:
         return "GL_RGBA";
      default:
         return "";
   }
}


static const char *
TypeStr(GLenum type)
{
   switch (type) {
      case GL_UNSIGNED_BYTE:
         return "GL_UNSIGNED_BYTE";
      case GL_UNSIGNED_SHORT:
         return "GL_UNSIGNED_SHORT";
      case GL_UNSIGNED_SHORT_5_6_5:
         return "GL_UNSIGNED_SHORT_5_6_5";
      case GL_UNSIGNED_SHORT_5_6_5_REV:
         return "GL_UNSIGNED_SHORT_5_6_5_REV";
      default:
         return "";
   }
}


static void
MeasureDownloadRate(void)
{
   const int w = TexWidth + 2 * TexBorder;
   const int h = TexHeight + 2 * TexBorder;
   const int bytes = w * h * BytesPerTexel(Format);
   GLubyte *texImage, *getImage;
   GLdouble t0, t1, time;
   int count;
   int i;

   texImage = (GLubyte *) malloc(bytes);
   getImage = (GLubyte *) malloc(bytes);
   if (!texImage || !getImage) {
      DownloadRate = 0.0;
      return;
   }

   for (i = 0; i < bytes; i++) {
      texImage[i] = i & 0xff;
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   if (ScaleAndBias) {
      glPixelTransferf(GL_RED_SCALE, 0.5);
      glPixelTransferf(GL_GREEN_SCALE, 0.5);
      glPixelTransferf(GL_BLUE_SCALE, 0.5);
      glPixelTransferf(GL_RED_BIAS, 0.5);
      glPixelTransferf(GL_GREEN_BIAS, 0.5);
      glPixelTransferf(GL_BLUE_BIAS, 0.5);
   }
   else {
      glPixelTransferf(GL_RED_SCALE, 1.0);
      glPixelTransferf(GL_GREEN_SCALE, 1.0);
      glPixelTransferf(GL_BLUE_SCALE, 1.0);
      glPixelTransferf(GL_RED_BIAS, 0.0);
      glPixelTransferf(GL_GREEN_BIAS, 0.0);
      glPixelTransferf(GL_BLUE_BIAS, 0.0);
   }

   count = 0;
   t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   do {
      if (SubImage && count > 0) {
         glTexSubImage2D(GL_TEXTURE_2D, 0, -TexBorder, -TexBorder, w, h,
                         FormatTable[Format].Format,
                         FormatTable[Format].Type, texImage);
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0,
                      FormatTable[Format].IntFormat, w, h, TexBorder,
                      FormatTable[Format].Format,
                      FormatTable[Format].Type, texImage);
      }

      if (count == 0) {
         /* draw a tiny polygon to force texture into texram */
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glEnable(GL_TEXTURE_2D);
         glBegin(GL_TRIANGLES);
         glTexCoord2f(0, 0);   glVertex2f(1, 1);
         glTexCoord2f(1, 0);   glVertex2f(3, 1);
         glTexCoord2f(0.5, 1);   glVertex2f(2, 3);
         glEnd();
         glDisable(GL_TEXTURE_2D);
      }

      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      time = t1 - t0;
      count++;
   } while (time < 3.0);

   printf("w*h=%d  count=%d  time=%f\n", w*h, count, time);
   DownloadRate = w * h * count / time;

#if 0
   if (!ScaleAndBias) {
      /* verify texture readback */
      glGetTexImage(GL_TEXTURE_2D, 0,
                    FormatTable[Format].Format,
                    FormatTable[Format].Type, getImage);
      for (i = 0; i < w * h; i++) {
         if (texImage[i] != getImage[i]) {
            printf("[%d] %d != %d\n", i, texImage[i], getImage[i]);
         }
      }
   }
#endif

   free(texImage);
   free(getImage);

   {
      GLint err = glGetError();
      if (err)
         printf("GL error %d\n", err);
   }
}


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
   const int w = TexWidth + 2 * TexBorder;
   const int h = TexHeight + 2 * TexBorder;
   char s[1000];

   glClear(GL_COLOR_BUFFER_BIT);

   glRasterPos2i(10, 80);
   sprintf(s, "Texture size[cursor]: %d x %d  Border[b]: %d", w, h, TexBorder);
   PrintString(s);

   glRasterPos2i(10, 65);
   sprintf(s, "Format[f]: %s  Type: %s  IntFormat: %s",
           FormatStr(FormatTable[Format].Format),
           TypeStr(  FormatTable[Format].Type),
           FormatStr(FormatTable[Format].IntFormat));
   PrintString(s);

   glRasterPos2i(10, 50);
   sprintf(s, "Pixel Scale&Bias[p]: %s   TexSubImage[s]: %s",
           ScaleAndBias ? "Yes" : "No",
           SubImage ? "Yes" : "No");
   PrintString(s);

   if (Mode == 0) {
      glRasterPos2i(200, 10);
      sprintf(s, "...Measuring...");
      PrintString(s);
      glutSwapBuffers();
      glutPostRedisplay();
      Mode++;
   }
   else if (Mode == 1) {
      MeasureDownloadRate();
      glutPostRedisplay();
      Mode++;
   }
   else {
      /* show results */
      glRasterPos2i(10, 10);
      sprintf(s, "Download rate: %g Mtexels/second  %g MB/second",
              DownloadRate / 1000000.0,
              DownloadRate * BytesPerTexel(Format) / 1000000.0);
      PrintString(s);
      {
         GLint r, g, b, a, l, i;
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &r);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &g);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &b);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &a);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_LUMINANCE_SIZE, &l);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTENSITY_SIZE, &i);
         sprintf(s, "TexelBits: R=%d G=%d B=%d A=%d L=%d I=%d", r, g, b, a, l, i);
         glRasterPos2i(10, 25);
         PrintString(s);
      }

      glutSwapBuffers();
   }
}


static void
Reshape(int width, int height)
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}



static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case ' ':
         Mode = 0;
         break;
      case 'b':
         /* toggle border */
         TexBorder = 1 - TexBorder;
         Mode = 0;
         break;
      case 'f':
         /* change format */
         Format = (Format + 1) % NUM_FORMATS;
         Mode = 0;
         break;
      case 'p':
         /* toggle border */
         ScaleAndBias = !ScaleAndBias;
         Mode = 0;
         break;
      case 's':
         SubImage = !SubImage;
         Mode = 0;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         if (TexHeight < MaxSize)
            TexHeight *= 2;
         break;
      case GLUT_KEY_DOWN:
         if (TexHeight > 1)
            TexHeight /= 2;
         break;
      case GLUT_KEY_LEFT:
         if (TexWidth > 1)
            TexWidth /= 2;
         break;
      case GLUT_KEY_RIGHT:
         if (TexWidth < MaxSize)
            TexWidth *= 2;
         break;
   }
   Mode = 0;
   glutPostRedisplay();
}


static void
Init(void)
{
   printf("GL_VENDOR = %s\n", (const char *) glGetString(GL_VENDOR));
   printf("GL_VERSION = %s\n", (const char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (const char *) glGetString(GL_RENDERER));
}


int
main(int argc, char *argv[])
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 600, 100 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
