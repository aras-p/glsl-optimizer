/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Measure glTex[Sub]Image2D() and glGetTexImage() rate
 *
 * Brian Paul
 * 16 Sep 2009
 */

#include "glmain.h"
#include "common.h"


int WinWidth = 100, WinHeight = 100;

static GLuint VBO;
static GLuint TexObj = 0;
static GLubyte *TexImage = NULL;
static GLsizei TexSize;
static GLenum TexIntFormat, TexSrcFormat, TexSrcType;

static const GLboolean DrawPoint = GL_TRUE;
static const GLboolean TexSubImage4 = GL_FALSE;

enum {
   MODE_CREATE_TEXIMAGE,
   MODE_TEXIMAGE,
   MODE_TEXSUBIMAGE,
   MODE_GETTEXIMAGE,
   MODE_COUNT
};

static const char *mode_name[MODE_COUNT] = 
{
   "Create_TexImage",
   "TexImage",
   "TexSubImage",
   "GetTexImage"
};



struct vertex
{
   GLfloat x, y, s, t;
};

static const struct vertex vertices[1] = {
   { 0.0, 0.0, 0.5, 0.5 },
};

#define VOFFSET(F) ((void *) offsetof(struct vertex, F))


/** Called from test harness/main */
void
PerfInit(void)
{
   /* setup VBO w/ vertex data */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                   sizeof(vertices), vertices, GL_STATIC_DRAW_ARB);
   glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(x));
   glTexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(s));
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   /* texture */
   glGenTextures(1, &TexObj);
   glBindTexture(GL_TEXTURE_2D, TexObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glEnable(GL_TEXTURE_2D);
}




static void
CreateUploadTexImage2D(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      if (TexObj)
         glDeleteTextures(1, &TexObj);

      glGenTextures(1, &TexObj);
      glBindTexture(GL_TEXTURE_2D, TexObj);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexImage2D(GL_TEXTURE_2D, 0, TexIntFormat,
                   TexSize, TexSize, 0,
                   TexSrcFormat, TexSrcType, TexImage);

      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
   }
   glFinish();
}


static void
UploadTexImage2D(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      /* XXX is this equivalent to a glTexSubImage call since we're
       * always specifying the same image size?  That case isn't optimized
       * in Mesa but may be optimized in other drivers.  Note sure how
       * much difference that might make.
       */
      glTexImage2D(GL_TEXTURE_2D, 0, TexIntFormat,
                   TexSize, TexSize, 0,
                   TexSrcFormat, TexSrcType, TexImage);
      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
   }
   glFinish();
}


static void
UploadTexSubImage2D(unsigned count)
{
   unsigned i;
   for (i = 0; i < count; i++) {
      if (TexSubImage4) {
         GLsizei halfSize = (TexSize == 1) ? 1 : TexSize / 2;
         GLsizei halfPos = TexSize - halfSize;
         /* do glTexSubImage2D in four pieces */
         /* lower-left */
         glPixelStorei(GL_UNPACK_ROW_LENGTH, TexSize);
         glTexSubImage2D(GL_TEXTURE_2D, 0,
                         0, 0, halfSize, halfSize,
                         TexSrcFormat, TexSrcType, TexImage);
         /* lower-right */
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, halfPos);
         glTexSubImage2D(GL_TEXTURE_2D, 0,
                         halfPos, 0, halfSize, halfSize,
                         TexSrcFormat, TexSrcType, TexImage);
         /* upper-left */
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
         glPixelStorei(GL_UNPACK_SKIP_ROWS, halfPos);
         glTexSubImage2D(GL_TEXTURE_2D, 0,
                         0, halfPos, halfSize, halfSize,
                         TexSrcFormat, TexSrcType, TexImage);
         /* upper-right */
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, halfPos);
         glPixelStorei(GL_UNPACK_SKIP_ROWS, halfPos);
         glTexSubImage2D(GL_TEXTURE_2D, 0,
                         halfPos, halfPos, halfSize, halfSize,
                         TexSrcFormat, TexSrcType, TexImage);
         /* reset the unpacking state */
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
         glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      }
      else {
         /* replace whole texture image at once */
         glTexSubImage2D(GL_TEXTURE_2D, 0,
                         0, 0, TexSize, TexSize,
                         TexSrcFormat, TexSrcType, TexImage);
      }
      if (DrawPoint)
         glDrawArrays(GL_POINTS, 0, 1);
   }
   glFinish();
}


static void
GetTexImage2D(unsigned count)
{
   unsigned i;
   GLubyte *buf = (GLubyte *) malloc(TexSize * TexSize * 4);
   for (i = 0; i < count; i++) {
      glGetTexImage(GL_TEXTURE_2D, 0,
                    TexSrcFormat, TexSrcType, buf);
   }
   glFinish();
   free(buf);
}


/* XXX any other formats to measure? */
static const struct {
   GLenum format, type;
   GLenum internal_format;
   const char *name;
   GLuint texel_size;
   GLboolean full_test;
} SrcFormats[] = {
   { GL_RGBA, GL_UNSIGNED_BYTE,       GL_RGBA, "RGBA/ubyte", 4,   GL_TRUE },
   { GL_RGB, GL_UNSIGNED_BYTE,        GL_RGB, "RGB/ubyte", 3,     GL_FALSE },
   { GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB, "RGB/565", 2,       GL_FALSE },
   { GL_BGRA, GL_UNSIGNED_BYTE,       GL_RGBA, "BGRA/ubyte", 4,   GL_FALSE },
   { GL_LUMINANCE, GL_UNSIGNED_BYTE,  GL_LUMINANCE, "L/ubyte", 1, GL_FALSE },
   { 0, 0, 0, NULL, 0, 0 }
};


/** Called from test harness/main */
void
PerfNextRound(void)
{
}


/** Called from test harness/main */
void
PerfDraw(void)
{
   GLint maxSize;
   double rate;
   GLint fmt, mode;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

   /* loop over source data formats */
   for (fmt = 0; SrcFormats[fmt].format; fmt++) {
      TexIntFormat = SrcFormats[fmt].internal_format;
      TexSrcFormat = SrcFormats[fmt].format;
      TexSrcType = SrcFormats[fmt].type;

      /* loop over glTexImage, glTexSubImage */
      for (mode = 0; mode < MODE_COUNT; mode++) {
         GLuint minsz, maxsz;

         if (SrcFormats[fmt].full_test) {
            minsz = 16;
            maxsz = 4096;
         }
         else {
            minsz = maxsz = 256;
            if (mode == MODE_CREATE_TEXIMAGE)
               continue;
         }

         /* loop over a defined range of texture sizes, test only the
          * ones which are legal for this driver.
          */
         for (TexSize = minsz; TexSize <= maxsz; TexSize *= 4) {
            double mbPerSec;

            if (TexSize <= maxSize) {
               GLint bytesPerImage;

               bytesPerImage = TexSize * TexSize * SrcFormats[fmt].texel_size;
               TexImage = malloc(bytesPerImage);

               switch (mode) {
               case MODE_TEXIMAGE:
                  rate = PerfMeasureRate(UploadTexImage2D);
                  break;

               case MODE_CREATE_TEXIMAGE:
                  rate = PerfMeasureRate(CreateUploadTexImage2D);
                  break;
                  
               case MODE_TEXSUBIMAGE:
                  /* create initial, empty texture */
                  glTexImage2D(GL_TEXTURE_2D, 0, TexIntFormat,
                               TexSize, TexSize, 0,
                               TexSrcFormat, TexSrcType, NULL);
                  rate = PerfMeasureRate(UploadTexSubImage2D);
                  break;

               case MODE_GETTEXIMAGE:
                  glTexImage2D(GL_TEXTURE_2D, 0, TexIntFormat,
                               TexSize, TexSize, 0,
                               TexSrcFormat, TexSrcType, TexImage);
                  rate = PerfMeasureRate(GetTexImage2D);
                  break;

               default:
                  exit(1);
               }

               mbPerSec = rate * bytesPerImage / (1024.0 * 1024.0);
               free(TexImage);


               {
                  unsigned err;
                  err = glGetError();
                  if (err) {
                     perf_printf("non-zero glGetError() %d\n", err);
                     exit(1);
                  }
               }  

            }
            else {
               rate = 0;
               mbPerSec = 0;
            }

            perf_printf("  %s(%s %d x %d): "
                        "%.1f images/sec, %.1f MB/sec\n",
                        mode_name[mode],
                        SrcFormats[fmt].name, TexSize, TexSize, rate, mbPerSec);
         }

         if (SrcFormats[fmt].full_test) 
            perf_printf("\n");
      }
   }

   exit(0);
}
