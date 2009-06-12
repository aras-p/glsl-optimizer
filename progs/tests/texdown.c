
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


static GLsizei MaxSize = 2048;
static GLsizei TexWidth = 1024, TexHeight = 1024, TexBorder = 0;
static GLboolean ScaleAndBias = GL_FALSE;
static GLboolean SubImage = GL_FALSE;
static GLdouble DownloadRate = 0.0;  /* texels/sec */

static GLuint Mode = 0;


/* Try and avoid L2 cache effects by cycling through a small number of
 * textures.
 * 
 * At the initial size of 1024x1024x4 == 4mbyte, say 8 textures will
 * keep us out of most caches at 32mb total.
 *
 * This turns into a fairly interesting question of what exactly you
 * expect to be in cache in normal usage, and what you think should be
 * outside.  There's no rules for this, no reason to favour one usage
 * over another except what the application you care about happens to
 * resemble most closely.
 *
 * - Should the client texture image be in L2 cache?  Has it just been
 *   generated or read from disk?
 * - Does the application really use >1 texture, or is it constantly 
 *   updating one image in-place?
 *
 * Different answers will favour different texture upload mechanisms.
 * To upload an image that is purely outside of cache, a DMA-based
 * upload will probably win, whereas for small, in-cache textures,
 * copying looks good.
 */
#define NR_TEXOBJ 4
static GLuint TexObj[NR_TEXOBJ];


struct FormatRec {
   GLenum Format;
   GLenum Type;
   GLenum IntFormat;
   GLint TexelSize;
};


static const struct FormatRec FormatTable[] = {
  /* Format   Type                         IntFormat   TexelSize */
   { GL_BGRA, GL_UNSIGNED_BYTE,            GL_RGBA,        4    },
   { GL_RGB,  GL_UNSIGNED_BYTE,            GL_RGB,         3    },
   { GL_RGBA, GL_UNSIGNED_BYTE,            GL_RGBA,        4    },
   { GL_RGBA, GL_UNSIGNED_BYTE,            GL_RGB,         4    },
   { GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,     GL_RGB,         2    },
   { GL_LUMINANCE, GL_UNSIGNED_BYTE,       GL_LUMINANCE,   1    },
   { GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE_ALPHA,   2    },
   { GL_ALPHA, GL_UNSIGNED_BYTE,           GL_ALPHA,       1    },
};
static GLint Format;

#define NUM_FORMATS (sizeof(FormatTable)/sizeof(FormatTable[0]))

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
      case GL_BGRA:
         return "GL_BGRA";
      case GL_LUMINANCE:
         return "GL_LUMINANCE";
      case GL_LUMINANCE_ALPHA:
         return "GL_LUMINANCE_ALPHA";
      case GL_ALPHA:
         return "GL_ALPHA";
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

/* On x86, there is a performance cliff for memcpy to texture memory
 * for sources below 64 byte alignment.  We do our best with this in
 * the driver, but it is better if the images are correctly aligned to
 * start with:
 */
#define ALIGN (1<<12)

static unsigned long align(unsigned long value, unsigned long a)
{
   return (value + a - 1) & ~(a-1);
}

static void
MeasureDownloadRate(void)
{
   const int w = TexWidth + 2 * TexBorder;
   const int h = TexHeight + 2 * TexBorder;
   const int image_bytes = align(w * h * BytesPerTexel(Format), ALIGN);
   const int bytes = image_bytes * NR_TEXOBJ;
   GLubyte *orig_texImage, *orig_getImage;
   GLubyte *texImage, *getImage;
   GLdouble t0, t1, time;
   int count;
   int i;
   int offset = 0;
   GLdouble total = 0;		/* ints will tend to overflow */

   printf("allocating %d bytes for %d %dx%d images\n",
	  bytes, NR_TEXOBJ, w, h);

   orig_texImage = (GLubyte *) malloc(bytes + ALIGN);
   orig_getImage = (GLubyte *) malloc(image_bytes + ALIGN);
   if (!orig_texImage || !orig_getImage) {
      DownloadRate = 0.0;
      return;
   }

   printf("alloc %p %p\n", orig_texImage, orig_getImage);

   texImage = (GLubyte *)align((unsigned long)orig_texImage, ALIGN);
   getImage = (GLubyte *)align((unsigned long)orig_getImage, ALIGN);   

   for (i = 1; !(((unsigned long)texImage) & i); i<<=1)
      ;
   printf("texture image alignment: %d bytes (%p)\n", i, texImage);
      
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

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glEnable(GL_TEXTURE_2D);

   count = 0;
   t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   do {
      int img = count%NR_TEXOBJ;
      GLubyte *img_ptr = texImage + img * image_bytes;

      glBindTexture(GL_TEXTURE_2D, TexObj[img]);

      if (SubImage && count > 0) {
	 /* Only update a portion of the image each iteration.  This
	  * is presumably why you'd want to use texsubimage, otherwise
	  * you may as well just call teximage again.
	  *
	  * A bigger question is whether to use a pointer that moves
	  * with each call, ie does the incoming data come from L2
	  * cache under normal circumstances, or is it pulled from
	  * uncached memory?  
	  * 
	  * There's a good argument to say L2 cache, ie you'd expect
	  * the data to have been recently generated.  It's possible
	  * that it could have come from a file read, which may or may
	  * not have gone through the cpu.
	  */
         glTexSubImage2D(GL_TEXTURE_2D, 0, 
			 -TexBorder, 
			 -TexBorder + offset * h/8, 
			 w, 
			 h/8,
                         FormatTable[Format].Format,
                         FormatTable[Format].Type, 
#if 1
			 texImage /* likely in L2$ */
#else
			 img_ptr + offset * bytes/8 /* unlikely in L2$ */
#endif
	    );
	 offset += 1;
	 offset %= 8;
	 total += w * h / 8;
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0,
                      FormatTable[Format].IntFormat, w, h, TexBorder,
                      FormatTable[Format].Format,
                      FormatTable[Format].Type, 
		      img_ptr);
	 total += w*h;
      }

      /* draw a tiny polygon to force texture into texram */
      glBegin(GL_TRIANGLES);
      glTexCoord2f(0, 0);     glVertex2f(1, 1);
      glTexCoord2f(1, 0);     glVertex2f(3, 1);
      glTexCoord2f(0.5, 1);   glVertex2f(2, 3);
      glEnd();

      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      time = t1 - t0;
      count++;
   } while (time < 3.0);

   glDisable(GL_TEXTURE_2D);

   printf("total texels=%f  time=%f\n", total, time);
   DownloadRate = total / time;


   free(orig_texImage); 
   free(orig_getImage); 

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
      case 'F':
         /* change format */
         Format = (Format - 1) % NUM_FORMATS;
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
