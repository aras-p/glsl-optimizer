/*
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * Test that glXGetProcAddress works.
 */

#define GLX_GLXEXT_PROTOTYPES

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


typedef void (*generic_func)();

#define EQUAL(X, Y)  (fabs((X) - (Y)) < 0.001)

/* This macro simplifies the task of querying an extension function
 * pointer and checking to see whether it resolved.
 */
#define DECLARE_GLFUNC_PTR(name,type) \
   type name = (type) glXGetProcAddressARB((const GLubyte *) "gl" #name)

/********************************************************************
 * Generic helper functions used by the test functions.
 */

static void CheckGLError(int line, const char *file, const char *function)
{
    int errorCode;
    glFinish();
    errorCode  = glGetError();
    if (errorCode == GL_NO_ERROR) return;
    while (errorCode != GL_NO_ERROR) {
	fprintf(stderr, "OpenGL error 0x%x (%s) at line %d of file %s in function %s()\n",
	    errorCode,
	    errorCode == GL_INVALID_VALUE? "GL_INVALID_VALUE":
	    errorCode == GL_INVALID_ENUM? "GL_INVALID_ENUM":
	    errorCode == GL_INVALID_OPERATION? "GL_INVALID_OPERATION":
	    errorCode == GL_STACK_OVERFLOW? "GL_STACK_OVERFLOW":
	    errorCode == GL_STACK_UNDERFLOW? "GL_STACK_UNDERFLOW":
	    errorCode == GL_OUT_OF_MEMORY? "GL_OUT_OF_MEMORY":
	    "unknown",
	    line, file, function);
	errorCode = glGetError();
    }
    fflush(stderr);
}

static GLboolean 
compare_bytes(const char *errorLabel, GLuint expectedSize, 
   const GLubyte *expectedData, GLuint actualSize, const GLubyte *actualData)
{
   int i;

   if (expectedSize == actualSize &&
      memcmp(expectedData, actualData, actualSize) == 0) {
      /* All is well */
      return GL_TRUE;
   }

   /* Trouble; we don't match.  Print out why. */
   fprintf(stderr, "%s: actual data is not as expected\n", errorLabel);
   for (i = 0; i <= 1; i++) {
      const GLubyte *ptr;
      int size;
      char *label;
      int j;

      switch(i) {
         case 0:
            label = "expected";
            size = expectedSize;
            ptr = expectedData;
            break;
         case 1:
            label = "  actual";
            size = actualSize;
            ptr = actualData;
            break;
      }
      
      fprintf(stderr, "    %s: size %d: {", label, size);
      for (j = 0; j < size; j++) {
         fprintf(stderr, "%s0x%02x", j > 0 ? ", " : "", ptr[j]);
      }
      fprintf(stderr, "}\n");
   }

   /* We fail if the data is unexpected. */
   return GL_FALSE;
}


static GLboolean 
compare_ints(const char *errorLabel, GLuint expectedSize, 
   const GLint *expectedData, GLuint actualSize, const GLint *actualData)
{
   int i;

   if (expectedSize == actualSize &&
      memcmp(expectedData, actualData, actualSize*sizeof(*expectedData)) == 0) {
      /* All is well */
      return GL_TRUE;
   }

   /* Trouble; we don't match.  Print out why. */
   fprintf(stderr, "%s: actual data is not as expected\n", errorLabel);
   for (i = 0; i <= 1; i++) {
      const GLint *ptr;
      int size;
      char *label;
      int j;

      switch(i) {
         case 0:
            label = "expected";
            size = expectedSize;
            ptr = expectedData;
            break;
         case 1:
            label = "  actual";
            size = actualSize;
            ptr = actualData;
            break;
      }
      
      fprintf(stderr, "    %s: size %d: {", label, size);
      for (j = 0; j < size; j++) {
         fprintf(stderr, "%s%d", j > 0 ? ", " : "", ptr[j]);
      }
      fprintf(stderr, "}\n");
   }

   /* We fail if the data is unexpected. */
   return GL_FALSE;
}

#define MAX_CONVERTED_VALUES 4
static GLboolean 
compare_shorts_to_ints(const char *errorLabel, GLuint expectedSize, 
   const GLshort *expectedData, GLuint actualSize, const GLint *actualData)
{
   int i;
   GLint convertedValues[MAX_CONVERTED_VALUES];

   if (expectedSize > MAX_CONVERTED_VALUES) {
      fprintf(stderr, "%s: too much data [need %d values, have %d values]\n",
         errorLabel, expectedSize, MAX_CONVERTED_VALUES);
      return GL_FALSE;
   }

   for (i = 0; i < expectedSize; i++) {
      convertedValues[i] = (GLint) expectedData[i];
   }

   return compare_ints(errorLabel, expectedSize, convertedValues, 
      actualSize, actualData);
}

static GLboolean 
compare_floats(const char *errorLabel, GLuint expectedSize, 
   const GLfloat *expectedData, GLuint actualSize, const GLfloat *actualData)
{
   int i;

   if (expectedSize == actualSize &&
      memcmp(expectedData, actualData, actualSize*sizeof(*expectedData)) == 0) {
      /* All is well */
      return GL_TRUE;
   }

   /* Trouble; we don't match.  Print out why. */
   fprintf(stderr, "%s: actual data is not as expected\n", errorLabel);
   for (i = 0; i <= 1; i++) {
      const GLfloat *ptr;
      int size;
      char *label;
      int j;

      switch(i) {
         case 0:
            label = "expected";
            size = expectedSize;
            ptr = expectedData;
            break;
         case 1:
            label = "  actual";
            size = actualSize;
            ptr = actualData;
            break;
      }
      
      fprintf(stderr, "    %s: size %d: {", label, size);
      for (j = 0; j < size; j++) {
         fprintf(stderr, "%s%f", j > 0 ? ", " : "", ptr[j]);
      }
      fprintf(stderr, "}\n");
   }

   /* We fail if the data is unexpected. */
   return GL_FALSE;
}

static GLboolean 
compare_doubles(const char *errorLabel, GLuint expectedSize, 
   const GLdouble *expectedData, GLuint actualSize, const GLdouble *actualData)
{
   int i;

   if (expectedSize == actualSize || 
      memcmp(expectedData, actualData, actualSize*sizeof(*expectedData)) == 0) {
      /* All is well */
      return GL_TRUE;
   }

   /* Trouble; we don't match.  Print out why. */
   fprintf(stderr, "%s: actual data is not as expected\n", errorLabel);
   for (i = 0; i <= 1; i++) {
      const GLdouble *ptr;
      int size;
      char *label;
      int j;

      switch(i) {
         case 0:
            label = "expected";
            size = expectedSize;
            ptr = expectedData;
            break;
         case 1:
            label = "  actual";
            size = actualSize;
            ptr = actualData;
            break;
      }
      
      fprintf(stderr, "    %s: size %d: {", label, size);
      for (j = 0; j < size; j++) {
         fprintf(stderr, "%s%f", j > 0 ? ", " : "", ptr[j]);
      }
      fprintf(stderr, "}\n");
   }

   /* We fail if the data is unexpected. */
   return GL_FALSE;
}

/********************************************************************
 * Functions to assist with GL_ARB_texture_compressiong testing
 */

static GLboolean
check_texture_format_supported(GLenum format)
{
   GLint numFormats;
   GLint *formats;
   register int i;

   glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, &numFormats);
   formats = malloc(numFormats * sizeof(GLint));
   if (formats == NULL) {
      fprintf(stderr, "check_texture_format_supported: could not allocate memory for %d GLints\n", 
         numFormats);
      return GL_FALSE;
   }
   
   memset(formats, 0, numFormats * sizeof(GLint));
   glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS_ARB, formats);

   for (i = 0; i < numFormats; i++) {
      if (formats[i] == format) {
         free(formats);
         return GL_TRUE;
      }
   }

   /* We didn't find the format we were looking for.  Give an error. */
#define FORMAT_NAME(x) (\
   x == GL_COMPRESSED_RGB_FXT1_3DFX ? "GL_COMPRESSED_RGB_FXT1_3DFX" : \
   x == GL_COMPRESSED_RGBA_FXT1_3DFX ? "GL_COMPRESSED_RGBA_FXT1_3DFX" : \
   x == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ? "GL_COMPRESSED_RGB_S3TC_DXT1_EXT" : \
   x == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT" : \
   x == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ? "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT" : \
   x == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ? "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT" : \
   x == GL_RGB_S3TC ? "GL_RGB_S3TC" : \
   x == GL_RGB4_S3TC ? "GL_RGB4_S3TC" : \
   x == GL_RGBA_S3TC ? "GL_RGBA_S3TC" : \
   x == GL_RGBA4_S3TC ? "GL_RGBA4_S3TC" : \
   "unknown")
   fprintf(stderr, "check_texture_format_supported: unsupported format 0x%04x [%s]\n",
      format, FORMAT_NAME(format));
   fprintf(stderr, "supported formats:");
   for (i = 0; i < numFormats; i++) {
      fprintf(stderr, " 0x%04x [%s]", formats[i], FORMAT_NAME(formats[i]));
   }
   fprintf(stderr, "\n");
   return GL_FALSE;
}

/* This helper function compresses an RGBA texture and compares it
 * against the expected compressed data.  It returns GL_TRUE if all
 * went as expected, or GL_FALSE in the case of error.
 */
static GLboolean
check_texture_compression(const char *message, GLenum dimension,
   GLint width, GLint height, GLint depth, const GLubyte *texture, 
   int expectedCompressedSize, const GLubyte *expectedCompressedData)
{
   /* These are the data we query about the texture. */
   GLint isCompressed;
   GLenum compressedFormat;
   GLint compressedSize;
   GLubyte *compressedData;

   /* We need this function pointer to operate. */
   DECLARE_GLFUNC_PTR(GetCompressedTexImageARB, PFNGLGETCOMPRESSEDTEXIMAGEARBPROC);
   if (GetCompressedTexImageARB == NULL) {
      fprintf(stderr, 
         "%s: could not query GetCompressedTexImageARB function pointer\n",
         message);
      return GL_FALSE;
   }

   /* Verify that we actually have the GL_COMPRESSED_RGBA_S3TC_DXT3_EXT format available. */
   if (!check_texture_format_supported(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)) {
      return GL_FALSE;
   }

   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   /* Set up the base image, requesting that the GL library compress it. */
   switch(dimension) {
      case GL_TEXTURE_1D:
         glTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            width, 0, 
            GL_RGBA, GL_UNSIGNED_BYTE, texture);
         break;
      case GL_TEXTURE_2D:
         glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            width, height, 0, 
            GL_RGBA, GL_UNSIGNED_BYTE, texture);
         break;
      case GL_TEXTURE_3D:
         glTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            width, height, depth, 0, 
            GL_RGBA, GL_UNSIGNED_BYTE, texture);
         break;
      default:
         fprintf(stderr, "%s: unknown dimension 0x%04x.\n", message, dimension);
         return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Make sure the texture is compressed, and pull it out if it is. */
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_COMPRESSED_ARB, 
      &isCompressed);
   if (!isCompressed) {
      fprintf(stderr, "%s: could not compress GL_COMPRESSED_RGBA_S3TC_DXT3_EXT texture\n",
         message);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_INTERNAL_FORMAT, 
      (GLint *)&compressedFormat);
   if (compressedFormat != GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
      fprintf(stderr, "%s: got internal format 0x%04x, expected GL_COMPRESSED_RGBA_S3TC_DXT3_EXT [0x%04x]\n",
         __FUNCTION__, compressedFormat, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, &compressedSize);
   compressedData = malloc(compressedSize);
   if (compressedData == NULL) {
      fprintf(stderr, "%s: could not malloc %d bytes for compressed texture\n",
         message, compressedSize);
      return GL_FALSE;
   }
   memset(compressedData, 0, compressedSize);
   (*GetCompressedTexImageARB)(dimension, 0, compressedData);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Compare it to the expected compressed data. The compare_bytes()
    * call will print out diagnostics in the case of failure.
    */
   if (!compare_bytes(message, 
      expectedCompressedSize, expectedCompressedData,
      compressedSize, compressedData)) {

      free(compressedData);
      return GL_FALSE;
   }

   /* All done.  Free our allocated data and return success. */
   free(compressedData);
   return GL_TRUE;
}

/* We'll use one function to exercise 1D, 2D, and 3D textures. */

/* The test function for compressed 3D texture images requires several
 * different function pointers that have to be queried.  This function
 * gets all the function pointers it needs itself, and so is suitable for 
 * use to test any and all of the incorporated functions.
 */

static GLboolean
exercise_CompressedTextures(GLenum dimension)
{
   /* Set up a basic (uncompressed) texture.  We're doing a blue/yellow
    * checkerboard.  The 8x4/32-pixel board is well-suited to S3TC
    * compression, which works on 4x4 blocks of pixels.
    */
#define B 0,0,255,255
#define Y 255,255,0,255
#define TEXTURE_WIDTH 16 
#define TEXTURE_HEIGHT 4
#define TEXTURE_DEPTH 1
   static GLubyte texture[TEXTURE_WIDTH*TEXTURE_HEIGHT*TEXTURE_DEPTH*4] = {
      B, B, Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, Y, Y,
      B, B, Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, Y, Y,
      Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, 
      Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, Y, Y, B, B, 
   };
#undef B
#undef Y
   GLubyte uncompressedTexture[TEXTURE_WIDTH*TEXTURE_HEIGHT*TEXTURE_DEPTH*4];

   /* We'll use this as a texture subimage. */
#define R 255,0,0,255
#define G 0,255,0,255
#define SUBTEXTURE_WIDTH 4
#define SUBTEXTURE_HEIGHT 4
#define SUBTEXTURE_DEPTH 1
   static GLubyte subtexture[SUBTEXTURE_WIDTH*SUBTEXTURE_HEIGHT*SUBTEXTURE_DEPTH*4] = {
      G, G, R, R,
      G, G, R, R,
      R, R, G, G,
      R, R, G, G,
   };
#undef R
#undef G

   /* These are the expected compressed textures.  (In the case of
    * a failed comparison, the test program will print out the
    * actual compressed data in a format that can be directly used
    * here, if desired.)  The brave of heart can calculate the compression 
    * themselves based on the formulae described at:
    *   http://en.wikipedia.org/wiki/S3_Texture_Compression
    * In a nutshell, each group of 16 bytes encodes a 4x4 texture block.
    * The first eight bytes of each group are 4-bit alpha values
    * for each of the 16 pixels in the texture block.
    * The next four bytes in each group are LSB-first RGB565 colors; the
    * first two bytes are identified as the color C0, and the next two
    * are the color C1.  (Two more colors C2 and C3 will be calculated 
    * from these, but do not appear in the compression data.)  The
    * last 4 bytes of the group are sixteen 2-bit indices that, for
    * each of the 16 pixels in the texture block, select one of the
    * colors C0, C1, C2, or C3.
    *
    * For example, our blue/yellow checkerboard is made up of
    * four identical 4x4 blocks.  Each of those blocks will
    * be encoded as: eight bytes of 0xff (16 alpha values, each 0xf),
    * C0 as the RGB565 color yellow (0xffe0), encoded LSB-first;
    * C1 as the RGB565 color blue (0x001f), encoded LSB-first;
    * and 4 bytes of 16 2-bit color indices reflecting the
    * choice of color for each of the 16 pixels:
    *     00, 00, 01, 01, = 0x05
    *     00, 00, 01, 01, = 0x05
    *     01, 01, 00, 00, = 0x50
    *     01, 01, 00, 00, = 0x50
    */
   static GLubyte compressedTexture[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50
   };

   /* The similar calculations for the 4x4 subtexture are left
    * as an exercise for the reader.
    */
   static GLubyte compressedSubTexture[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00, 0xf8, 0xe0, 0x07, 0x05, 0x05, 0x50, 0x50,
   };

   /* The combined texture replaces the initial blue/yellow
    * block with the green/red block.  (I'd wanted to do
    * the more interesting exercise of putting the
    * green/red block in the middle of the blue/yellow
    * texture, which is a non-trivial replacement, but
    * the attempt produces GL_INVALID_OPERATION, showing
    * that you can only replace whole blocks of 
    * subimages with S3TC.)  The combined texture looks
    * like:
    *      G G R R  B B Y Y  B B Y Y  B B Y Y
    *      G G R R  B B Y Y  B B Y Y  B B Y Y
    *      R R G G  Y Y B B  Y Y B B  Y Y B B 
    *      R R G G  Y Y B B  Y Y B B  Y Y B B 
    * which encodes just like the green/red block followed
    * by 3 copies of the yellow/blue block.
    */
   static GLubyte compressedCombinedTexture[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00, 0xf8, 0xe0, 0x07, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xe0, 0xff, 0x1f, 0x00, 0x05, 0x05, 0x50, 0x50
   };

   /* These are the data we query about the texture. */
   GLint queryIsCompressed;
   GLenum queryCompressedFormat;
   GLint queryCompressedSize;
   GLubyte queryCompressedData[sizeof(compressedTexture)];

   /* Query the function pointers we need.  We actually won't need most
    * of these (the "dimension" parameter dictates whether we're testing
    * 1D, 2D, or 3D textures), but we'll have them all ready just in case.
    */
   DECLARE_GLFUNC_PTR(GetCompressedTexImageARB, PFNGLGETCOMPRESSEDTEXIMAGEARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexImage3DARB, PFNGLCOMPRESSEDTEXIMAGE3DARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexSubImage3DARB, PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexImage2DARB, PFNGLCOMPRESSEDTEXIMAGE2DARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexSubImage2DARB, PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexImage1DARB, PFNGLCOMPRESSEDTEXIMAGE1DARBPROC);
   DECLARE_GLFUNC_PTR(CompressedTexSubImage1DARB, PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC);

   /* If the necessary functions are missing, we can't continue */
   if (GetCompressedTexImageARB == NULL) {
      fprintf(stderr, "%s: GetCompressedTexImageARB function is missing\n",
         __FUNCTION__);
      return GL_FALSE;
   }
   switch (dimension) {
      case GL_TEXTURE_1D:
         if (CompressedTexImage1DARB == NULL || CompressedTexSubImage1DARB == NULL) {
            fprintf(stderr, "%s: 1D compressed texture functions are missing\n",
               __FUNCTION__);
            return GL_FALSE;
         };
         break;
      case GL_TEXTURE_2D:
         if (CompressedTexImage2DARB == NULL || CompressedTexSubImage2DARB == NULL) {
            fprintf(stderr, "%s: 2D compressed texture functions are missing\n",
               __FUNCTION__);
            return GL_FALSE;
         };
         break;
      case GL_TEXTURE_3D:
         if (CompressedTexImage3DARB == NULL || CompressedTexSubImage3DARB == NULL) {
            fprintf(stderr, "%s: 3D compressed texture functions are missing\n",
               __FUNCTION__);
            return GL_FALSE;
         };
         break;
      default:
         fprintf(stderr, "%s: unknown texture dimension 0x%04x passed.\n",
            __FUNCTION__, dimension);
         return GL_FALSE;
   }
   
   /* Check the compression of our base texture image. */
   if (!check_texture_compression("texture compression", dimension,
         TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH, texture,
         sizeof(compressedTexture), compressedTexture)) {

      /* Something's wrong with texture compression.  The function
       * above will have printed an appropriate error.
       */
      return GL_FALSE;
   }

   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Do the same for our texture subimage */
   if (!check_texture_compression("subtexture compression", dimension,
         SUBTEXTURE_WIDTH, SUBTEXTURE_HEIGHT, SUBTEXTURE_DEPTH, subtexture,
         sizeof(compressedSubTexture), compressedSubTexture)) {

      /* Something's wrong with texture compression.  The function
       * above will have printed an appropriate error.
       */
      return GL_FALSE;
   }

   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Send the base compressed texture down to the hardware. */
   switch(dimension) {
      case GL_TEXTURE_3D:
         (*CompressedTexImage3DARB)(GL_TEXTURE_3D, 0, 
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH, 0, 
            sizeof(compressedTexture), compressedTexture);
         break;

      case GL_TEXTURE_2D:
         (*CompressedTexImage2DARB)(GL_TEXTURE_2D, 0, 
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, 
            sizeof(compressedTexture), compressedTexture);
         break;

      case GL_TEXTURE_1D:
         (*CompressedTexImage1DARB)(GL_TEXTURE_1D, 0, 
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            TEXTURE_WIDTH, 0, 
            sizeof(compressedTexture), compressedTexture);
         break;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* For grins, query it to make sure it is as expected. */
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_COMPRESSED_ARB, 
      &queryIsCompressed);
   if (!queryIsCompressed) {
      fprintf(stderr, "%s: compressed texture did not come back as compressed\n",
         __FUNCTION__);
      return GL_FALSE;
   }
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_INTERNAL_FORMAT, 
      (GLint *)&queryCompressedFormat);
   if (queryCompressedFormat != GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
      fprintf(stderr, "%s: got internal format 0x%04x, expected GL_COMPRESSED_RGBA_S3TC_DXT3_EXT [0x%04x]\n",
         __FUNCTION__, queryCompressedFormat, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
      return GL_FALSE;
   }
   glGetTexLevelParameteriv(dimension, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB, 
      &queryCompressedSize);
   if (queryCompressedSize != sizeof(compressedTexture)) {
      fprintf(stderr, "%s: compressed 3D texture changed size: expected %lu, actual %d\n",
         __FUNCTION__, (unsigned long) sizeof(compressedTexture), queryCompressedSize);
      return GL_FALSE;
   }
   (*GetCompressedTexImageARB)(dimension, 0, queryCompressedData);
   if (!compare_bytes(
      "exercise_CompressedTextures:doublechecking compressed texture",
      sizeof(compressedTexture), compressedTexture,
      queryCompressedSize, queryCompressedData)) {
      return GL_FALSE;
   }

   /* Now apply the texture subimage.  The current implementation of
    * S3TC requires that subimages be only applied to whole blocks.
    */
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   switch(dimension) {
      case GL_TEXTURE_3D:
         (*CompressedTexSubImage3DARB)(GL_TEXTURE_3D, 0, 
            0, 0, 0, /* offsets */
            SUBTEXTURE_WIDTH, SUBTEXTURE_HEIGHT, SUBTEXTURE_DEPTH,
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            sizeof(compressedSubTexture), compressedSubTexture);
         break;
      case GL_TEXTURE_2D:
         (*CompressedTexSubImage2DARB)(GL_TEXTURE_2D, 0, 
            0, 0, /* offsets */
            SUBTEXTURE_WIDTH, SUBTEXTURE_HEIGHT,
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            sizeof(compressedSubTexture), compressedSubTexture);
         break;
      case GL_TEXTURE_1D:
         (*CompressedTexSubImage2DARB)(GL_TEXTURE_2D, 0, 
            0, 0, /* offsets */
            SUBTEXTURE_WIDTH, SUBTEXTURE_HEIGHT,
            GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 
            sizeof(compressedSubTexture), compressedSubTexture);
         break;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query the compressed texture back now, and see that it
    * is as expected.
    */
   (*GetCompressedTexImageARB)(dimension, 0, queryCompressedData);
   if (!compare_bytes("exercise_CompressedTextures:combined texture",
      sizeof(compressedCombinedTexture), compressedCombinedTexture,
      queryCompressedSize, queryCompressedData)) {
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Just for the exercise, uncompress the texture and pull it out. 
    * We don't check it because the compression is lossy, so it won't
    * compare exactly to the source texture; we just 
    * want to exercise the code paths that convert it.
    */
   glGetTexImage(dimension, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncompressedTexture);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* If we survived this far, we pass. */
   return GL_TRUE;
}

/**************************************************************************
 * Functions to assist with GL_EXT_framebuffer_object and
 * GL_EXT_framebuffer_blit testing.
 */

#define FB_STATUS_NAME(x) (\
   x == GL_FRAMEBUFFER_COMPLETE_EXT ? "GL_FRAMEBUFFER_COMPLETE_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT" : \
   x == GL_FRAMEBUFFER_UNSUPPORTED_EXT ? "GL_FRAMEBUFFER_UNSUPPORTED_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT" : \
   x == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT ? "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT" : \
   "unknown")

static GLboolean
exercise_framebuffer(void)
{
   GLuint framebufferID = 0;
   GLuint renderbufferID = 0;
   
   /* Dimensions of the framebuffer and renderbuffers are arbitrary.
    * Since they won't be shown on-screen, we can use whatever we want.
    */
   const GLint Width = 100;
   const GLint Height = 100;

   /* Every function we use will be referenced through function pointers.
    * This will allow this test program to run on OpenGL implementations
    * that *don't* implement these extensions (though the implementation
    * used to compile them must have up-to-date header files).
    */
   DECLARE_GLFUNC_PTR(GenFramebuffersEXT, PFNGLGENFRAMEBUFFERSEXTPROC);
   DECLARE_GLFUNC_PTR(IsFramebufferEXT, PFNGLISFRAMEBUFFEREXTPROC);
   DECLARE_GLFUNC_PTR(DeleteFramebuffersEXT, PFNGLDELETEFRAMEBUFFERSEXTPROC);
   DECLARE_GLFUNC_PTR(BindFramebufferEXT, PFNGLBINDFRAMEBUFFEREXTPROC);
   DECLARE_GLFUNC_PTR(GenRenderbuffersEXT, PFNGLGENRENDERBUFFERSEXTPROC);
   DECLARE_GLFUNC_PTR(IsRenderbufferEXT, PFNGLISRENDERBUFFEREXTPROC);
   DECLARE_GLFUNC_PTR(DeleteRenderbuffersEXT, PFNGLDELETERENDERBUFFERSEXTPROC);
   DECLARE_GLFUNC_PTR(BindRenderbufferEXT, PFNGLBINDRENDERBUFFEREXTPROC);
   DECLARE_GLFUNC_PTR(FramebufferRenderbufferEXT, PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC);
   DECLARE_GLFUNC_PTR(RenderbufferStorageEXT, PFNGLRENDERBUFFERSTORAGEEXTPROC);
   DECLARE_GLFUNC_PTR(CheckFramebufferStatusEXT, PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC);

   /* The BlitFramebuffer function comes from a different extension.
    * It's possible for an implementation to implement all the above,
    * but not BlitFramebuffer; so it's okay if this one comes back
    * NULL, as we can still test the rest.
    */
   DECLARE_GLFUNC_PTR(BlitFramebufferEXT, PFNGLBLITFRAMEBUFFEREXTPROC);

   /* We cannot test unless we have all the function pointers. */
   if (
      GenFramebuffersEXT == NULL ||
      IsFramebufferEXT == NULL || 
      DeleteFramebuffersEXT == NULL ||
      BindFramebufferEXT == NULL ||
      GenRenderbuffersEXT == NULL ||
      IsRenderbufferEXT == NULL ||
      DeleteRenderbuffersEXT == NULL ||
      BindRenderbufferEXT == NULL ||
      FramebufferRenderbufferEXT == NULL ||
      RenderbufferStorageEXT == NULL ||
      CheckFramebufferStatusEXT == NULL
   ) {
      fprintf(stderr, "%s: could not locate all framebuffer functions\n",
         __FUNCTION__);
      return GL_FALSE;
   }

   /* Generate a framebuffer for us to play with. */
   (*GenFramebuffersEXT)(1, &framebufferID);
   if (framebufferID == 0) {
      fprintf(stderr, "%s: failed to generate a frame buffer ID.\n",
         __FUNCTION__);
      return GL_FALSE;
   }
   /* The generated name is not a framebuffer object until bound. */
   (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, framebufferID);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   if (!(*IsFramebufferEXT)(framebufferID)) {
      fprintf(stderr, "%s: generated a frame buffer ID 0x%x that wasn't a framebuffer\n",
         __FUNCTION__, framebufferID);
      (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
      (*DeleteFramebuffersEXT)(1, &framebufferID);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   {
      GLint queriedFramebufferID;
      glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &queriedFramebufferID);
      if (queriedFramebufferID != framebufferID) {
         fprintf(stderr, "%s: bound frame buffer 0x%x, but queried 0x%x\n",
            __FUNCTION__, framebufferID, queriedFramebufferID);
         (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
         (*DeleteFramebuffersEXT)(1, &framebufferID);
         return GL_FALSE;
      }
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Create a color buffer to attach to the frame buffer object, so
    * we can actually operate on it.  We go through the same basic checks
    * with the renderbuffer that we do with the framebuffer.
    */
   (*GenRenderbuffersEXT)(1, &renderbufferID);
   if (renderbufferID == 0) {
      fprintf(stderr, "%s: could not generate a renderbuffer ID\n",
         __FUNCTION__);
      (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
      (*DeleteFramebuffersEXT)(1, &framebufferID);
      return GL_FALSE;
   }
   (*BindRenderbufferEXT)(GL_RENDERBUFFER_EXT, renderbufferID);
   if (!(*IsRenderbufferEXT)(renderbufferID)) {
      fprintf(stderr, "%s: generated renderbuffer 0x%x is not a renderbuffer\n",
         __FUNCTION__, renderbufferID);
      (*BindRenderbufferEXT)(GL_RENDERBUFFER_EXT, 0);
      (*DeleteRenderbuffersEXT)(1, &renderbufferID);
      (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
      (*DeleteFramebuffersEXT)(1, &framebufferID);
      return GL_FALSE;
   }
   {
      GLint queriedRenderbufferID = 0;
      glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &queriedRenderbufferID);
      if (renderbufferID != queriedRenderbufferID) {
         fprintf(stderr, "%s: bound renderbuffer 0x%x, but got 0x%x\n",
            __FUNCTION__, renderbufferID, queriedRenderbufferID);
         (*BindRenderbufferEXT)(GL_RENDERBUFFER_EXT, 0);
         (*DeleteRenderbuffersEXT)(1, &renderbufferID);
         (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
         (*DeleteFramebuffersEXT)(1, &framebufferID);
         return GL_FALSE;
      }
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Add the renderbuffer as a color attachment to the current
    * framebuffer (which is our generated framebuffer).
    */
   (*FramebufferRenderbufferEXT)(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
      GL_RENDERBUFFER_EXT, renderbufferID);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* The renderbuffer will need some dimensions and storage space. */
   (*RenderbufferStorageEXT)(GL_RENDERBUFFER_EXT, GL_RGB, Width, Height);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* That should be everything we need.  If we set up to draw and to
    * read from our color attachment, we should be "framebuffer complete",
    * meaning the framebuffer is ready to go.
    */
   glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);
   {
      GLenum status = (*CheckFramebufferStatusEXT)(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         fprintf(stderr, "%s: framebuffer not complete; status = %s [0x%x]\n",
            __FUNCTION__, FB_STATUS_NAME(status), status);
         glReadBuffer(0);
         glDrawBuffer(0);
         (*BindRenderbufferEXT)(GL_RENDERBUFFER_EXT, 0);
         (*DeleteRenderbuffersEXT)(1, &renderbufferID);
         (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
         (*DeleteFramebuffersEXT)(1, &framebufferID);
         return GL_FALSE;
      }
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Define the contents of the frame buffer */
   glClearColor(0.5, 0.5, 0.5, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* If the GL_EXT_framebuffer_blit is supported, attempt a framebuffer
    * blit from (5,5)-(10,10) to (90,90)-(95,95).  This is *not* an
    * error if framebuffer_blit is *not* supported (as we can still
    * effectively test the other functions).
    */
   if (BlitFramebufferEXT != NULL) {
      (*BlitFramebufferEXT)(5, 5, 10, 10, 90, 90, 95, 95,
         GL_COLOR_BUFFER_BIT, GL_NEAREST);
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* We could now test to see whether the framebuffer had the desired
    * contents.  As this is just a touch test, we'll leave that for now.
    * Clean up and go home.
    */
   glReadBuffer(0);
   glDrawBuffer(0);
   (*BindRenderbufferEXT)(GL_RENDERBUFFER_EXT, 0);
   (*DeleteRenderbuffersEXT)(1, &renderbufferID);
   (*BindFramebufferEXT)(GL_FRAMEBUFFER_EXT, 0);
   (*DeleteFramebuffersEXT)(1, &framebufferID);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   return GL_TRUE;
}

/**************************************************************************
 * Functions to assist with GL_ARB_shader_objects testing.
 */

static void
print_info_log(const char *message, GLhandleARB object)
{
   DECLARE_GLFUNC_PTR(GetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC);
   DECLARE_GLFUNC_PTR(GetInfoLogARB, PFNGLGETINFOLOGARBPROC);
   int logLength, queryLength;
   char *log;

   if (GetObjectParameterivARB == NULL) {
      fprintf(stderr, "%s: could not get GetObjectParameterivARB address\n",
         message);
      return;
   }
   if (GetInfoLogARB == NULL) {
      fprintf(stderr, "%s: could not get GetInfoLogARB address\n",
         message);
      return;
   }

   (*GetObjectParameterivARB)(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, 
      &logLength);
   if (logLength == 0) {
      fprintf(stderr, "%s: info log length is 0\n", message);
      return;
   }
   log = malloc(logLength);
   if (log == NULL) {
      fprintf(stderr, "%s: could not malloc %d bytes for info log\n",
         message, logLength);
   }
   else {
      (*GetInfoLogARB)(object, logLength, &queryLength, log);
      fprintf(stderr, "%s: info log says '%s'\n",
         message, log);
   }
   free(log);
}

static GLboolean
exercise_uniform_start(const char *fragmentShaderText, const char *uniformName,
   GLhandleARB *returnProgram, GLint *returnUniformLocation)
{
   DECLARE_GLFUNC_PTR(CreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC);
   DECLARE_GLFUNC_PTR(ShaderSourceARB, PFNGLSHADERSOURCEARBPROC);
   DECLARE_GLFUNC_PTR(CompileShaderARB, PFNGLCOMPILESHADERARBPROC);
   DECLARE_GLFUNC_PTR(CreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC);
   DECLARE_GLFUNC_PTR(AttachObjectARB, PFNGLATTACHOBJECTARBPROC);
   DECLARE_GLFUNC_PTR(LinkProgramARB, PFNGLLINKPROGRAMARBPROC);
   DECLARE_GLFUNC_PTR(UseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
   DECLARE_GLFUNC_PTR(ValidateProgramARB, PFNGLVALIDATEPROGRAMARBPROC);
   DECLARE_GLFUNC_PTR(GetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC);
   DECLARE_GLFUNC_PTR(DeleteObjectARB, PFNGLDELETEOBJECTARBPROC);
   DECLARE_GLFUNC_PTR(GetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC);
   GLhandleARB fs, program;
   GLint uniformLocation;
   GLint shaderCompiled, programValidated;

   if (CreateShaderObjectARB == NULL ||
       ShaderSourceARB == NULL ||
       CompileShaderARB == NULL ||
       CreateProgramObjectARB == NULL ||
       AttachObjectARB == NULL ||
       LinkProgramARB == NULL ||
       UseProgramObjectARB == NULL ||
       ValidateProgramARB == NULL ||
       GetUniformLocationARB == NULL ||
       DeleteObjectARB == NULL ||
       GetObjectParameterivARB == NULL ||
       0) {
      return GL_FALSE;
   }

   /* Create the trivial fragment shader and program.  For safety
    * we'll check to make sure they compile and link correctly.
    */
   fs = (*CreateShaderObjectARB)(GL_FRAGMENT_SHADER_ARB);
   (*ShaderSourceARB)(fs, 1, &fragmentShaderText, NULL);
   (*CompileShaderARB)(fs);
   (*GetObjectParameterivARB)(fs, GL_OBJECT_COMPILE_STATUS_ARB,
      &shaderCompiled);
   if (!shaderCompiled) {
      print_info_log("shader did not compile", fs);
      (*DeleteObjectARB)(fs);
      CheckGLError(__LINE__, __FILE__, __FUNCTION__);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   program = (*CreateProgramObjectARB)();
   (*AttachObjectARB)(program, fs);
   (*LinkProgramARB)(program);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Make sure we're going to run successfully */
   (*ValidateProgramARB)(program);
   (*GetObjectParameterivARB)(program, GL_OBJECT_VALIDATE_STATUS_ARB, 
      &programValidated);
   if (!programValidated) {; 
      print_info_log("program did not validate", program);
      (*DeleteObjectARB)(program);
      (*DeleteObjectARB)(fs);
      CheckGLError(__LINE__, __FILE__, __FUNCTION__);
      return GL_FALSE;
   }

   /* Put the program in place.  We're not allowed to assign to uniform
    * variables used by the program until the program is put into use.
    */
   (*UseProgramObjectARB)(program);

   /* Once the shader is in place, we're free to delete it; this
    * won't affect the copy that's part of the program.
    */
   (*DeleteObjectARB)(fs);

   /* Find the location index of the uniform variable we declared;
    * the caller will ned that to set the value.
    */
   uniformLocation = (*GetUniformLocationARB)(program, uniformName);
   if (uniformLocation == -1) {
      fprintf(stderr, "%s: could not determine uniform location\n",
         __FUNCTION__);
      (*DeleteObjectARB)(program);
      CheckGLError(__LINE__, __FILE__, __FUNCTION__);
      return GL_FALSE;
   }

   /* All done with what we're supposed to do - return the program
    * handle and the uniform location to the caller.
    */
   *returnProgram = program;
   *returnUniformLocation = uniformLocation;
   return GL_TRUE;
}

static void
exercise_uniform_end(GLhandleARB program)
{
   DECLARE_GLFUNC_PTR(UseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
   DECLARE_GLFUNC_PTR(DeleteObjectARB, PFNGLDELETEOBJECTARBPROC);
   if (UseProgramObjectARB == NULL || DeleteObjectARB == NULL) {
      return;
   }

   /* Turn off our program by setting the special value 0, and
    * then delete the program object.
    */
   (*UseProgramObjectARB)(0);
   (*DeleteObjectARB)(program);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
}

/**************************************************************************
 * Exercises for fences
 */
static GLboolean
exercise_fences(void)
{
   DECLARE_GLFUNC_PTR(DeleteFencesNV, PFNGLDELETEFENCESNVPROC);
   DECLARE_GLFUNC_PTR(FinishFenceNV, PFNGLFINISHFENCENVPROC);
   DECLARE_GLFUNC_PTR(GenFencesNV, PFNGLGENFENCESNVPROC);
   DECLARE_GLFUNC_PTR(GetFenceivNV, PFNGLGETFENCEIVNVPROC);
   DECLARE_GLFUNC_PTR(IsFenceNV, PFNGLISFENCENVPROC);
   DECLARE_GLFUNC_PTR(SetFenceNV, PFNGLSETFENCENVPROC);
   DECLARE_GLFUNC_PTR(TestFenceNV, PFNGLTESTFENCENVPROC);
   GLuint fence;
   GLint fenceStatus, fenceCondition;
   int count;

   /* Make sure we have all the function pointers we need. */
   if (GenFencesNV == NULL ||
      SetFenceNV == NULL ||
      IsFenceNV == NULL ||
      GetFenceivNV == NULL ||
      TestFenceNV == NULL ||
      FinishFenceNV == NULL ||
      DeleteFencesNV == NULL) {
      fprintf(stderr, "%s: don't have all the fence functions\n",
         __FUNCTION__);
      return GL_FALSE;
   }

   /* Create and set a simple fence. */
   (*GenFencesNV)(1, &fence);
   (*SetFenceNV)(fence, GL_ALL_COMPLETED_NV);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Make sure it reads as a fence. */
   if (!(*IsFenceNV)(fence)) {
      fprintf(stderr, "%s: set fence is not a fence\n", __FUNCTION__);
      (*DeleteFencesNV)(1, &fence);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Try to read back its current status and condition. */
   (*GetFenceivNV)(fence, GL_FENCE_CONDITION_NV, &fenceCondition);
   if (fenceCondition != GL_ALL_COMPLETED_NV) {
      fprintf(stderr, "%s: expected fence condition 0x%x, got 0x%x\n",
         __FUNCTION__, GL_ALL_COMPLETED_NV, fenceCondition);
      (*DeleteFencesNV)(1, &fence);
      return GL_FALSE;
   }
   (*GetFenceivNV)(fence, GL_FENCE_STATUS_NV, &fenceStatus);
   if (fenceStatus != GL_TRUE && fenceStatus != GL_FALSE) {
      fprintf(stderr,"%s: fence status should be GL_TRUE or GL_FALSE, got 0x%x\n",
         __FUNCTION__, fenceStatus);
      (*DeleteFencesNV)(1, &fence);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Set the fence again, query its status, and wait for it to finish
    * two different ways: once by looping on TestFence(), and a 
    * second time by a simple call to FinishFence();
    */
   (*SetFenceNV)(fence, GL_ALL_COMPLETED_NV);
   glFlush();
   count = 1;
   while (!(*TestFenceNV)(fence)) {
      count++;
      if (count == 0) {
         break;
      }
   }
   if (count == 0) {
      fprintf(stderr, "%s: fence never returned true\n", __FUNCTION__);
      (*DeleteFencesNV)(1, &fence);
      return GL_FALSE;
   }
   (*SetFenceNV)(fence, GL_ALL_COMPLETED_NV);
   (*FinishFenceNV)(fence);
   if ((*TestFenceNV)(fence) != GL_TRUE) {
      fprintf(stderr, "%s: finished fence does not have status GL_TRUE\n",
         __FUNCTION__);
      (*DeleteFencesNV)(1, &fence);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* All done.  Delete the fence and return. */
   (*DeleteFencesNV)(1, &fence);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   return GL_TRUE;
}

/**************************************************************************
 * Exercises for buffer objects
 */
enum Map_Buffer_Usage{ Use_Map_Buffer, Use_Map_Buffer_Range};
static GLboolean
exercise_buffer_objects(enum Map_Buffer_Usage usage)
{
#define BUFFER_DATA_SIZE 1024
   GLuint bufferID;
   GLint bufferMapped;
   static GLubyte data[BUFFER_DATA_SIZE] = {0};
   float *dataPtr = NULL;
   const char *extensions = (const char *) glGetString(GL_EXTENSIONS);

   /* Get the function pointers we need.  These are from
    * GL_ARB_vertex_buffer_object and are required in all
    * cases.
    */
   DECLARE_GLFUNC_PTR(GenBuffersARB, PFNGLGENBUFFERSARBPROC);
   DECLARE_GLFUNC_PTR(BindBufferARB, PFNGLBINDBUFFERARBPROC);
   DECLARE_GLFUNC_PTR(BufferDataARB, PFNGLBUFFERDATAARBPROC);
   DECLARE_GLFUNC_PTR(MapBufferARB, PFNGLMAPBUFFERARBPROC);
   DECLARE_GLFUNC_PTR(UnmapBufferARB, PFNGLUNMAPBUFFERARBPROC);
   DECLARE_GLFUNC_PTR(DeleteBuffersARB, PFNGLDELETEBUFFERSARBPROC);
   DECLARE_GLFUNC_PTR(GetBufferParameterivARB, PFNGLGETBUFFERPARAMETERIVARBPROC);

   /* These are from GL_ARB_map_buffer_range, and are optional
    * unless we're given Use_Map_Buffer_Range.  Note that they do *not*
    * have the standard "ARB" suffixes; this is because the extension
    * was introduced *after* a superset was standardized in OpenGL 3.0.
    * (The extension really only exists to allow the functionality on
    * devices that cannot implement a full OpenGL 3.0 driver.)
    */
   DECLARE_GLFUNC_PTR(FlushMappedBufferRange, PFNGLFLUSHMAPPEDBUFFERRANGEPROC);
   DECLARE_GLFUNC_PTR(MapBufferRange, PFNGLMAPBUFFERRANGEPROC);
   
   /* This is from APPLE_flush_buffer_range, and is optional even if
    * we're given Use_Map_Buffer_Range.  Test it before using it.
    */
   DECLARE_GLFUNC_PTR(BufferParameteriAPPLE, PFNGLBUFFERPARAMETERIAPPLEPROC);
   if (!strstr("GL_APPLE_flush_buffer_range", extensions)) {
      BufferParameteriAPPLE = NULL;
   }

   /* Make sure we have all the function pointers we need. */
   if (GenBuffersARB == NULL ||
      BindBufferARB == NULL ||
      BufferDataARB == NULL ||
      MapBufferARB == NULL ||
      UnmapBufferARB == NULL ||
      DeleteBuffersARB == NULL ||
      GetBufferParameterivARB == NULL) {
      fprintf(stderr, "%s: missing basic MapBuffer functions\n", __FUNCTION__);
      return GL_FALSE;
   }
   if (usage == Use_Map_Buffer_Range) {
      if (FlushMappedBufferRange == NULL || MapBufferRange == NULL) {
         fprintf(stderr, "%s: missing MapBufferRange functions\n", __FUNCTION__);
         return GL_FALSE;
      }
   }

   /* Create and define a buffer */
   (*GenBuffersARB)(1, &bufferID);
   (*BindBufferARB)(GL_ARRAY_BUFFER_ARB, bufferID);
   (*BufferDataARB)(GL_ARRAY_BUFFER_ARB, BUFFER_DATA_SIZE, data, 
      GL_DYNAMIC_DRAW_ARB);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* If we're using MapBufferRange, and if the BufferParameteriAPPLE
    * function is present, use it before mapping.  This particular
    * use is a no-op, intended just to exercise the entry point.
    */
   if (usage == Use_Map_Buffer_Range && BufferParameteriAPPLE != NULL) {
      (*BufferParameteriAPPLE)(GL_ARRAY_BUFFER_ARB, 
         GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_TRUE);
   }

   /* Map it, and make sure it's mapped. */
   switch(usage) {
      case Use_Map_Buffer:
         dataPtr = (float *) (*MapBufferARB)(
            GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
         break;
      case Use_Map_Buffer_Range:
         dataPtr = (float *)(*MapBufferRange)(GL_ARRAY_BUFFER_ARB,
            4, 16, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
         break;
   }
   if (dataPtr == NULL) {
      fprintf(stderr, "%s: %s returned NULL\n", __FUNCTION__,
         usage == Use_Map_Buffer ? "MapBuffer" : "MapBufferRange");
      (*BindBufferARB)(GL_ARRAY_BUFFER_ARB, 0);
      (*DeleteBuffersARB)(1, &bufferID);
      return GL_FALSE;
   }
   (*GetBufferParameterivARB)(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAPPED_ARB, 
      &bufferMapped);
   if (!bufferMapped) {
      fprintf(stderr, "%s: buffer should be mapped but isn't\n", __FUNCTION__);
      (*BindBufferARB)(GL_ARRAY_BUFFER_ARB, 0);
      (*DeleteBuffersARB)(1, &bufferID);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Write something to it, just to make sure we don't segfault. */
   *dataPtr = 1.5;

   /* Unmap to show we're finished with the buffer.  Note that if we're
    * using MapBufferRange, we first have to flush the range we modified.
    */
   if (usage == Use_Map_Buffer_Range) {
      (*FlushMappedBufferRange)(GL_ARRAY_BUFFER_ARB, 0, 16);
   }
   if (!(*UnmapBufferARB)(GL_ARRAY_BUFFER_ARB)) {
      fprintf(stderr, "%s: UnmapBuffer failed\n", __FUNCTION__);
      (*BindBufferARB)(GL_ARRAY_BUFFER_ARB, 0);
      (*DeleteBuffersARB)(1, &bufferID);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* All done. */
   (*BindBufferARB)(GL_ARRAY_BUFFER_ARB, 0);
   (*DeleteBuffersARB)(1, &bufferID);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);
   return GL_TRUE;

#undef BUFFER_DATA_SIZE
}

/**************************************************************************
 * Exercises for occlusion query
 */
static GLboolean
exercise_occlusion_query(void)
{
   GLuint queryObject;
   GLint queryReady;
   GLuint querySampleCount;
   GLint queryCurrent;
   GLint queryCounterBits;

   /* Get the function pointers we need.  These are from
    * GL_ARB_vertex_buffer_object and are required in all
    * cases.
    */
   DECLARE_GLFUNC_PTR(GenQueriesARB, PFNGLGENQUERIESARBPROC);
   DECLARE_GLFUNC_PTR(BeginQueryARB, PFNGLBEGINQUERYARBPROC);
   DECLARE_GLFUNC_PTR(GetQueryivARB, PFNGLGETQUERYIVARBPROC);
   DECLARE_GLFUNC_PTR(EndQueryARB, PFNGLENDQUERYARBPROC);
   DECLARE_GLFUNC_PTR(IsQueryARB, PFNGLISQUERYARBPROC);
   DECLARE_GLFUNC_PTR(GetQueryObjectivARB, PFNGLGETQUERYOBJECTIVARBPROC);
   DECLARE_GLFUNC_PTR(GetQueryObjectuivARB, PFNGLGETQUERYOBJECTUIVARBPROC);
   DECLARE_GLFUNC_PTR(DeleteQueriesARB, PFNGLDELETEQUERIESARBPROC);

   /* Make sure we have all the function pointers we need. */
   if (GenQueriesARB == NULL ||
      BeginQueryARB == NULL ||
      GetQueryivARB == NULL ||
      EndQueryARB == NULL ||
      IsQueryARB == NULL ||
      GetQueryObjectivARB == NULL ||
      GetQueryObjectuivARB == NULL ||
      DeleteQueriesARB == NULL) {
      fprintf(stderr, "%s: don't have all the Query functions\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* Create a query object, and start a query. */
   (*GenQueriesARB)(1, &queryObject);
   (*BeginQueryARB)(GL_SAMPLES_PASSED_ARB, queryObject);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* While we're in the query, check the functions that are supposed
    * to return which query we're in and how many bits of resolution
    * we get.
    */
   (*GetQueryivARB)(GL_SAMPLES_PASSED_ARB, GL_CURRENT_QUERY_ARB, &queryCurrent);
   if (queryCurrent != queryObject) {
      fprintf(stderr, "%s: current query 0x%x != set query 0x%x\n",
         __FUNCTION__, queryCurrent, queryObject);
      (*EndQueryARB)(GL_SAMPLES_PASSED_ARB);
      (*DeleteQueriesARB)(1, &queryObject);
      return GL_FALSE;
   }
   (*GetQueryivARB)(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, 
      &queryCounterBits);
   if (queryCounterBits < 1) {
      fprintf(stderr, "%s: query counter bits is too small (%d)\n",
         __FUNCTION__, queryCounterBits);
      (*EndQueryARB)(GL_SAMPLES_PASSED_ARB);
      (*DeleteQueriesARB)(1, &queryObject);
      return GL_FALSE;
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Finish up the query.  Since we didn't draw anything, the result
    * should be 0 passed samples.
    */
   (*EndQueryARB)(GL_SAMPLES_PASSED_ARB);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Routine existence test */
   if (!(*IsQueryARB)(queryObject)) {
      fprintf(stderr, "%s: query object 0x%x fails existence test\n",
         __FUNCTION__, queryObject);
      (*DeleteQueriesARB)(1, &queryObject);
      return GL_FALSE;
   }

   /* Loop until the query is ready, then get back the result.  We use
    * the signed query for the boolean value of whether the result is
    * available, but the unsigned query to actually pull the result;
    * this is just to test both entrypoints, but in a real query you may
    * need the extra bit of resolution.
    */
   queryReady = GL_FALSE;
   do {
      (*GetQueryObjectivARB)(queryObject, GL_QUERY_RESULT_AVAILABLE_ARB, 
         &queryReady);
   } while (!queryReady);
   (*GetQueryObjectuivARB)(queryObject, GL_QUERY_RESULT_ARB, &querySampleCount);
   (*DeleteQueriesARB)(1, &queryObject);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* If sample count isn't 0, something's funny. */
   if (querySampleCount > 0) {
      fprintf(stderr, "%s: expected query result of 0, got %ud\n",
         __FUNCTION__, querySampleCount);
      return GL_FALSE;
   }

   /* Here, all is well. */
   return GL_TRUE;
}

/**************************************************************************
 * The following functions are used to check that the named OpenGL function
 * actually does what it's supposed to do.
 * The naming of these functions is significant.  The getprocaddress.py script
 * scans this file and extracts these function names.
 */

static GLboolean
test_WeightPointerARB(generic_func func)
{
   /* Assume we have at least 2 vertex units (or this extension makes
    * no sense), and establish a set of 2-element vector weights.
    * We use floats that can be represented exactly in binary
    * floating point formats so we can compare correctly later.
    * We also make sure the 0th entry matches the default weights,
    * so we can restore the default easily.
    */
#define USE_VERTEX_UNITS 2
#define USE_WEIGHT_INDEX 3
   static GLfloat weights[] = {
      1.0,   0.0,
      0.875, 0.125,
      0.75,  0.25,
      0.625, 0.375,
      0.5,   0.5,
      0.375, 0.625,
      0.25,  0.75,
      0.125, 0.875, 
      0.0,   1.0,
   };
   GLint numVertexUnits;
   GLfloat *currentWeights;
   int i;
   int errorCount = 0;

   PFNGLWEIGHTPOINTERARBPROC WeightPointerARB = (PFNGLWEIGHTPOINTERARBPROC) func;

   /* Make sure we have at least two vertex units */
   glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &numVertexUnits);
   if (numVertexUnits < USE_VERTEX_UNITS) {
      fprintf(stderr, "%s: need %d vertex units, got %d\n", 
         __FUNCTION__, USE_VERTEX_UNITS, numVertexUnits);
      return GL_FALSE;
   }
   
   /* Make sure we allocate enough room to query all the current weights */
   currentWeights = (GLfloat *)malloc(numVertexUnits * sizeof(GLfloat));
   if (currentWeights == NULL) {
      fprintf(stderr, "%s: couldn't allocate room for %d floats\n",
         __FUNCTION__, numVertexUnits);
      return GL_FALSE;
   }

   /* Set up the pointer, enable the state, and try to send down a
    * weight vector (we'll arbitrarily send index 2).
    */
   (*WeightPointerARB)(USE_VERTEX_UNITS, GL_FLOAT, 0, weights);
   glEnableClientState(GL_WEIGHT_ARRAY_ARB);
   glArrayElement(USE_WEIGHT_INDEX);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Verify that it changed the current state. */
   glGetFloatv(GL_CURRENT_WEIGHT_ARB, currentWeights);
   for (i = 0; i < numVertexUnits; i++) {
      if (i < USE_VERTEX_UNITS) {
         /* This is one of the units we explicitly set. */
         if (currentWeights[i] != weights[USE_VERTEX_UNITS*USE_WEIGHT_INDEX + i]) {
            fprintf(stderr, "%s: current weight at index %d is %f, should be %f\n",
               __FUNCTION__, i, currentWeights[i], 
               weights[USE_VERTEX_UNITS*USE_WEIGHT_INDEX + i]);
            errorCount++;
         }
      }
      else {
         /* All other weights should be 0. */
         if (currentWeights[i] != 0.0) {
            fprintf(stderr, "%s: current weight at index %d is %f, should be %f\n",
               __FUNCTION__, i, 0.0,
               weights[USE_VERTEX_UNITS*USE_WEIGHT_INDEX + i]);
            errorCount++;
         }
      }
   }
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Restore the old state.  We know the default set of weights is in
    * index 0.
    */
   glArrayElement(0);
   glDisableClientState(GL_WEIGHT_ARRAY_ARB);
   (*WeightPointerARB)(0, GL_FLOAT, 0, NULL);
   free(currentWeights);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* We're fine if we didn't get any mismatches. */
   if (errorCount == 0) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

/* Wrappers on the exercise_occlusion_query function */
static GLboolean
test_GenQueriesARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_BeginQueryARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_GetQueryivARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_EndQueryARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_IsQueryARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_GetQueryObjectivARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_GetQueryObjectuivARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}
static GLboolean
test_DeleteQueriesARB(generic_func func)
{
   (void) func;
   return exercise_occlusion_query();
}

/* Wrappers on the exercise_buffer_objects() function */
static GLboolean
test_GenBuffersARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_BindBufferARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_BufferDataARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_MapBufferARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_UnmapBufferARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_DeleteBuffersARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_GetBufferParameterivARB(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer);
}
static GLboolean
test_FlushMappedBufferRange(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer_Range);
}
static GLboolean
test_MapBufferRange(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer_Range);
}
static GLboolean
test_BufferParameteriAPPLE(generic_func func)
{
   (void) func;
   return exercise_buffer_objects(Use_Map_Buffer_Range);
}

/* Wrappers on the exercise_framebuffer() function */
static GLboolean
test_BindFramebufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_BindRenderbufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_CheckFramebufferStatusEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_DeleteFramebuffersEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_DeleteRenderbuffersEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_FramebufferRenderbufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_GenFramebuffersEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_GenRenderbuffersEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_IsFramebufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_IsRenderbufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_RenderbufferStorageEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}
static GLboolean
test_BlitFramebufferEXT(generic_func func)
{
   (void) func;
   return exercise_framebuffer();
}

/* These are wrappers on the exercise_CompressedTextures function. 
 * Unfortunately, we cannot test the 1D counterparts, because the
 * texture compressions available all support 2D and higher only.
 */
static GLboolean
test_CompressedTexImage2DARB(generic_func func)
{
   (void) func;
   return exercise_CompressedTextures(GL_TEXTURE_2D);
}
static GLboolean
test_CompressedTexSubImage2DARB(generic_func func)
{
   (void) func;
   return exercise_CompressedTextures(GL_TEXTURE_2D);
}
static GLboolean
test_CompressedTexImage3DARB(generic_func func)
{
   (void) func;
   /*return exercise_CompressedTextures(GL_TEXTURE_3D);*/
   return GL_TRUE;
}
static GLboolean
test_CompressedTexSubImage3DARB(generic_func func)
{
   (void) func;
   /*return exercise_CompressedTextures(GL_TEXTURE_3D);*/
   return GL_TRUE;
}
static GLboolean
test_GetCompressedTexImageARB(generic_func func)
{
   (void) func;
   /*return exercise_CompressedTextures(GL_TEXTURE_3D);*/
   return GL_TRUE;
}

/* Wrappers on exercise_fences(). */
static GLboolean
test_DeleteFencesNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_GenFencesNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_SetFenceNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_TestFenceNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_FinishFenceNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_GetFenceivNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}
static GLboolean
test_IsFenceNV(generic_func func)
{
   (void) func;
   return exercise_fences();
}

/* A bunch of glUniform*() tests */
static GLboolean
test_Uniform1iv(generic_func func)
{
   PFNGLUNIFORM1IVARBPROC Uniform1ivARB = (PFNGLUNIFORM1IVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform int uniformColor;" 
      "void main() {gl_FragColor.r = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[1] = {1};
   GLint queriedUniform[1];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * so we must set it using integer versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform1ivARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 1, uniform, 1, queriedUniform);
}

static GLboolean
test_Uniform1i(generic_func func)
{
   PFNGLUNIFORM1IARBPROC Uniform1iARB = (PFNGLUNIFORM1IARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform int uniformColor;"
      "void main() {gl_FragColor.r = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[1] = {1};
   GLint queriedUniform[4];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * so we must set it using integer versions
    * of the Uniform* functions.
    */
   (*Uniform1iARB)(uniformLocation, uniform[0]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 1, uniform, 1, queriedUniform);
}

static GLboolean
test_Uniform1fv(generic_func func)
{
   PFNGLUNIFORM1FVARBPROC Uniform1fvARB = (PFNGLUNIFORM1FVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform float uniformColor;"
      "void main() {gl_FragColor.r = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[1] = {1.1};
   GLfloat queriedUniform[1];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * so we must set it using float versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform1fvARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 1, uniform, 1, queriedUniform);
}

static GLboolean
test_Uniform1f(generic_func func)
{
   PFNGLUNIFORM1FARBPROC Uniform1fARB = (PFNGLUNIFORM1FARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform float uniformColor;"
      "void main() {gl_FragColor.r = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[1] = {1.1};
   GLfloat queriedUniform[1];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * so we must set it using float versions
    * of the Uniform* functions.
    */
   (*Uniform1fARB)(uniformLocation, uniform[0]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 1, uniform, 1, queriedUniform);
}

static GLboolean
test_Uniform2iv(generic_func func)
{
   PFNGLUNIFORM2IVARBPROC Uniform2ivARB = (PFNGLUNIFORM2IVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec2 uniformColor;" 
      "void main() {gl_FragColor.rg = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[2] = {1,2};
   GLint queriedUniform[2];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector 2 (ivec2), so we must set it using integer versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform2ivARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 2, uniform, 2, queriedUniform);
}

static GLboolean
test_Uniform2i(generic_func func)
{
   PFNGLUNIFORM2IARBPROC Uniform2iARB = (PFNGLUNIFORM2IARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec2 uniformColor;"
      "void main() {gl_FragColor.rg = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[2] = {1,2};
   GLint queriedUniform[4];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector 2 (ivec2), so we must set it using integer versions
    * of the Uniform* functions.
    */
   (*Uniform2iARB)(uniformLocation, uniform[0], uniform[1]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 2, uniform, 2, queriedUniform);
}

static GLboolean
test_Uniform2fv(generic_func func)
{
   PFNGLUNIFORM2FVARBPROC Uniform2fvARB = (PFNGLUNIFORM2FVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec2 uniformColor;"
      "void main() {gl_FragColor.rg = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[2] = {1.1,2.2};
   GLfloat queriedUniform[2];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * vector 2 (vec2), so we must set it using float versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform2fvARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 2, uniform, 2, queriedUniform);
}

static GLboolean
test_Uniform2f(generic_func func)
{
   PFNGLUNIFORM2FARBPROC Uniform2fARB = (PFNGLUNIFORM2FARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec2 uniformColor;"
      "void main() {gl_FragColor.rg = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[2] = {1.1,2.2};
   GLfloat queriedUniform[2];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * vector 2 (vec2), so we must set it using float versions
    * of the Uniform* functions.
    */
   (*Uniform2fARB)(uniformLocation, uniform[0], uniform[1]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 2, uniform, 2, queriedUniform);
}

static GLboolean
test_Uniform3iv(generic_func func)
{
   PFNGLUNIFORM3IVARBPROC Uniform3ivARB = (PFNGLUNIFORM3IVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec3 uniformColor;" 
      "void main() {gl_FragColor.rgb = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[3] = {1,2,3};
   GLint queriedUniform[3];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector 3 (ivec3), so we must set it using integer versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform3ivARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 3, uniform, 3, queriedUniform);
}

static GLboolean
test_Uniform3i(generic_func func)
{
   PFNGLUNIFORM3IARBPROC Uniform3iARB = (PFNGLUNIFORM3IARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec3 uniformColor;"
      "void main() {gl_FragColor.rgb = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[3] = {1,2,3};
   GLint queriedUniform[4];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector 3 (ivec3), so we must set it using integer versions
    * of the Uniform* functions.
    */
   (*Uniform3iARB)(uniformLocation, uniform[0], uniform[1], uniform[2]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 3, uniform, 3, queriedUniform);
}

static GLboolean
test_Uniform3fv(generic_func func)
{
   PFNGLUNIFORM3FVARBPROC Uniform3fvARB = (PFNGLUNIFORM3FVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec3 uniformColor;"
      "void main() {gl_FragColor.rgb = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[3] = {1.1,2.2,3.3};
   GLfloat queriedUniform[3];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * vector 3 (vec3), so we must set it using float versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform3fvARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 3, uniform, 3, queriedUniform);
}

static GLboolean
test_Uniform3f(generic_func func)
{
   PFNGLUNIFORM3FARBPROC Uniform3fARB = (PFNGLUNIFORM3FARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec3 uniformColor;"
      "void main() {gl_FragColor.rgb = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[3] = {1.1,2.2,3.3};
   GLfloat queriedUniform[3];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * vector 3 (vec3), so we must set it using float versions
    * of the Uniform* functions.
    */
   (*Uniform3fARB)(uniformLocation, uniform[0], uniform[1], uniform[2]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 3, uniform, 3, queriedUniform);
}

static GLboolean
test_Uniform4iv(generic_func func)
{
   PFNGLUNIFORM4IVARBPROC Uniform4ivARB = (PFNGLUNIFORM4IVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec4 uniformColor; void main() {gl_FragColor = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[4] = {1,2,3,4};
   GLint queriedUniform[4];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector (ivec4), so we must set it using integer versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform4ivARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 4, uniform, 4, queriedUniform);
}

static GLboolean
test_Uniform4i(generic_func func)
{
   PFNGLUNIFORM4IARBPROC Uniform4iARB = (PFNGLUNIFORM4IARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformivARB, PFNGLGETUNIFORMIVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform ivec4 uniformColor; void main() {gl_FragColor = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLint uniform[4] = {1,2,3,4};
   GLint queriedUniform[4];

   if (GetUniformivARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector (ivec4), so we must set it using integer versions
    * of the Uniform* functions.
    */
   (*Uniform4iARB)(uniformLocation, uniform[0], uniform[1], uniform[2],
      uniform[3]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformivARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_ints(__FUNCTION__, 4, uniform, 4, queriedUniform);
}

static GLboolean
test_Uniform4fv(generic_func func)
{
   PFNGLUNIFORM4FVARBPROC Uniform4fvARB = (PFNGLUNIFORM4FVARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec4 uniformColor; void main() {gl_FragColor = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[4] = {1.1,2.2,3.3,4.4};
   GLfloat queriedUniform[4];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is a float
    * vector (vec4), so we must set it using float versions
    * of the Uniform* functions.  The "1" means we're setting
    * one vector's worth of information.
    */
   (*Uniform4fvARB)(uniformLocation, 1, uniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 4, uniform, 4, queriedUniform);
}

static GLboolean
test_Uniform4f(generic_func func)
{
   PFNGLUNIFORM4FARBPROC Uniform4fARB = (PFNGLUNIFORM4FARBPROC) func;
   DECLARE_GLFUNC_PTR(GetUniformfvARB, PFNGLGETUNIFORMFVARBPROC);

   /* This is a trivial fragment shader that sets the color of the
    * fragment to the uniform value passed in.
    */
   static const char *fragmentShaderText = 
      "uniform vec4 uniformColor; void main() {gl_FragColor = uniformColor;}";
   static const char *uniformName = "uniformColor";

   GLhandleARB program;
   GLint uniformLocation;
   const GLfloat uniform[4] = {1.1,2.2,3.3,4.4};
   GLfloat queriedUniform[4];

   if (GetUniformfvARB == NULL) {
      return GL_FALSE;
   }

   /* Call a helper function to compile up the shader and give
    * us back the validated program and uniform location.
    * If it fails, something's wrong and we can't continue.
    */
   if (!exercise_uniform_start(fragmentShaderText, uniformName, 
      &program, &uniformLocation)) {
      return GL_FALSE;
   }

   /* Set the value of the program uniform.  Note that you must
    * use a compatible type.  Our uniform above is an integer
    * vector (ivec4), so we must set it using integer versions
    * of the Uniform* functions.
    */
   (*Uniform4fARB)(uniformLocation, uniform[0], uniform[1], uniform[2],
      uniform[3]);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Query it back */
   (*GetUniformfvARB)(program, uniformLocation, queriedUniform);
   CheckGLError(__LINE__, __FILE__, __FUNCTION__);

   /* Clean up before we check to see whether it came back unscathed */
   exercise_uniform_end(program);

   /* Now check to see whether the uniform came back as expected.  This
    * will return GL_TRUE if all is well, or GL_FALSE if the comparison failed.
    */
   return compare_floats(__FUNCTION__, 4, uniform, 4, queriedUniform);
}

static GLboolean
test_ActiveTextureARB(generic_func func)
{
   PFNGLACTIVETEXTUREARBPROC activeTexture = (PFNGLACTIVETEXTUREARBPROC) func;
   GLint t;
   GLboolean pass;
   (*activeTexture)(GL_TEXTURE1_ARB);
   glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &t);
   pass = (t == GL_TEXTURE1_ARB);
   (*activeTexture)(GL_TEXTURE0_ARB);  /* restore default */
   return pass;
}


static GLboolean
test_SecondaryColor3fEXT(generic_func func)
{
   PFNGLSECONDARYCOLOR3FEXTPROC secColor3f = (PFNGLSECONDARYCOLOR3FEXTPROC) func;
   GLfloat color[4];
   GLboolean pass;
   (*secColor3f)(1.0, 1.0, 0.0);
   glGetFloatv(GL_CURRENT_SECONDARY_COLOR_EXT, color);
   pass = (color[0] == 1.0 && color[1] == 1.0 && color[2] == 0.0);
   (*secColor3f)(0.0, 0.0, 0.0);  /* restore default */
   return pass;
}


static GLboolean
test_ActiveStencilFaceEXT(generic_func func)
{
   PFNGLACTIVESTENCILFACEEXTPROC activeFace = (PFNGLACTIVESTENCILFACEEXTPROC) func;
   GLint face;
   GLboolean pass;
   (*activeFace)(GL_BACK);
   glGetIntegerv(GL_ACTIVE_STENCIL_FACE_EXT, &face);
   pass = (face == GL_BACK);
   (*activeFace)(GL_FRONT);  /* restore default */
   return pass;
}


static GLboolean
test_VertexAttrib1fvARB(generic_func func)
{
   PFNGLVERTEXATTRIB1FVARBPROC vertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLfloat v[1] = {25.0};
   const GLfloat def[1] = {0};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib1fvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 25.0 && res[1] == 0.0 && res[2] == 0.0 && res[3] == 1.0);
   (*vertexAttrib1fvARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib1dvARB(generic_func func)
{
   PFNGLVERTEXATTRIB1DVARBPROC vertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC) func;
   PFNGLGETVERTEXATTRIBDVARBPROC getVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvARB");

   const GLdouble v[1] = {25.0};
   const GLdouble def[1] = {0};
   GLdouble res[4];
   GLboolean pass;
   (*vertexAttrib1dvARB)(6, v);
   (*getVertexAttribdvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 25.0 && res[1] == 0.0 && res[2] == 0.0 && res[3] == 1.0);
   (*vertexAttrib1dvARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib1svARB(generic_func func)
{
   PFNGLVERTEXATTRIB1SVARBPROC vertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC) func;
   PFNGLGETVERTEXATTRIBIVARBPROC getVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivARB");

   const GLshort v[1] = {25.0};
   const GLshort def[1] = {0};
   GLint res[4];
   GLboolean pass;
   (*vertexAttrib1svARB)(6, v);
   (*getVertexAttribivARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 25 && res[1] == 0 && res[2] == 0 && res[3] == 1);
   (*vertexAttrib1svARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib4NubvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUBVARBPROC vertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NubvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 1.0 && res[1] == 0.0 && res[2] == 1.0 && res[3] == 0.0);
   (*vertexAttrib4NubvARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4NuivARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUIVARBPROC vertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLuint v[4] = {0xffffffff, 0, 0xffffffff, 0};
   const GLuint def[4] = {0, 0, 0, 0xffffffff};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NuivARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 0.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NuivARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4ivARB(generic_func func)
{
   PFNGLVERTEXATTRIB4IVARBPROC vertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLint v[4] = {1, 2, -3, 4};
   const GLint def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4ivARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 2.0) && EQUAL(res[2], -3.0) && EQUAL(res[3], 4.0));
   (*vertexAttrib4ivARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4NsvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NSVARBPROC vertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLshort v[4] = {0, 32767, 32767, 0};
   const GLshort def[4] = {0, 0, 0, 32767};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NsvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 0.0) && EQUAL(res[1], 1.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NsvARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib4NusvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUSVARBPROC vertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLushort v[4] = {0xffff, 0, 0xffff, 0};
   const GLushort def[4] = {0, 0, 0, 0xffff};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NusvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 0.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NusvARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib1sNV(generic_func func)
{
   PFNGLVERTEXATTRIB1SNVPROC vertexAttrib1sNV = (PFNGLVERTEXATTRIB1SNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 0, 0, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib1sNV)(6, v[0]);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib1sNV)(6, def[0]);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib1fNV(generic_func func)
{
   PFNGLVERTEXATTRIB1FNVPROC vertexAttrib1fNV = (PFNGLVERTEXATTRIB1FNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib1fNV)(6, v[0]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib1fNV)(6, def[0]);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib1dNV(generic_func func)
{
   PFNGLVERTEXATTRIB1DNVPROC vertexAttrib1dNV = (PFNGLVERTEXATTRIB1DNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib1dNV)(6, v[0]);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib1dNV)(6, def[0]);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2sNV(generic_func func)
{
   PFNGLVERTEXATTRIB2SNVPROC vertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 0, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib2sNV)(6, v[0], v[1]);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2sNV)(6, def[0], def[1]);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2fNV(generic_func func)
{
   PFNGLVERTEXATTRIB2FNVPROC vertexAttrib2fNV = (PFNGLVERTEXATTRIB2FNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib2fNV)(6, v[0], v[1]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2fNV)(6, def[0], def[1]);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2dNV(generic_func func)
{
   PFNGLVERTEXATTRIB2DNVPROC vertexAttrib2dNV = (PFNGLVERTEXATTRIB2DNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib2dNV)(6, v[0], v[1]);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2dNV)(6, def[0], def[1]);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3sNV(generic_func func)
{
   PFNGLVERTEXATTRIB3SNVPROC vertexAttrib3sNV = (PFNGLVERTEXATTRIB3SNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib3sNV)(6, v[0], v[1], v[2]);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3sNV)(6, def[0], def[1], def[2]);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3fNV(generic_func func)
{
   PFNGLVERTEXATTRIB3FNVPROC vertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib3fNV)(6, v[0], v[1], v[2]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3fNV)(6, def[0], def[1], def[2]);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3dNV(generic_func func)
{
   PFNGLVERTEXATTRIB3DNVPROC vertexAttrib3dNV = (PFNGLVERTEXATTRIB3DNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib3dNV)(6, v[0], v[1], v[2]);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3dNV)(6, def[0], def[1], def[2]);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4sNV(generic_func func)
{
   PFNGLVERTEXATTRIB4SNVPROC vertexAttrib4sNV = (PFNGLVERTEXATTRIB4SNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 5};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib4sNV)(6, v[0], v[1], v[2], v[3]);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4sNV)(6, def[0], def[1], def[2], def[3]);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4fNV(generic_func func)
{
   PFNGLVERTEXATTRIB4FNVPROC vertexAttrib4fNV = (PFNGLVERTEXATTRIB4FNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib4fNV)(6, v[0], v[1], v[2], v[3]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4fNV)(6, def[0], def[1], def[2], def[3]);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4dNV(generic_func func)
{
   PFNGLVERTEXATTRIB4DNVPROC vertexAttrib4dNV = (PFNGLVERTEXATTRIB4DNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib4dNV)(6, v[0], v[1], v[2], v[3]);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4dNV)(6, def[0], def[1], def[2], def[3]);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4ubNV(generic_func func)
{
   PFNGLVERTEXATTRIB4UBNVPROC vertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   /* There's no byte-value query; so we use the float-value query.
    * Bytes are interpreted as steps between 0 and 1, so the
    * expected float values will be 0.0 for byte value 0 and 1.0 for
    * byte value 255.
    */
   GLfloat expectedResults[4] = {1.0, 0.0, 1.0, 0.0};
   (*vertexAttrib4ubNV)(6, v[0], v[1], v[2], v[3]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4ubNV)(6, def[0], def[1], def[2], def[3]);
   return compare_floats(__FUNCTION__, 4, expectedResults, 4, res);
}

static GLboolean
test_VertexAttrib1fvNV(generic_func func)
{
   PFNGLVERTEXATTRIB1FVNVPROC vertexAttrib1fvNV = (PFNGLVERTEXATTRIB1FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib1fvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib1fvNV)(6, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib1dvNV(generic_func func)
{
   PFNGLVERTEXATTRIB1DVNVPROC vertexAttrib1dvNV = (PFNGLVERTEXATTRIB1DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib1dvNV)(6, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib1dvNV)(6, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2svNV(generic_func func)
{
   PFNGLVERTEXATTRIB2SVNVPROC vertexAttrib2svNV = (PFNGLVERTEXATTRIB2SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 0, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib2svNV)(6, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2svNV)(6, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2fvNV(generic_func func)
{
   PFNGLVERTEXATTRIB2FVNVPROC vertexAttrib2fvNV = (PFNGLVERTEXATTRIB2FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib2fvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2fvNV)(6, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib2dvNV(generic_func func)
{
   PFNGLVERTEXATTRIB2DVNVPROC vertexAttrib2dvNV = (PFNGLVERTEXATTRIB2DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib2dvNV)(6, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib2dvNV)(6, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3svNV(generic_func func)
{
   PFNGLVERTEXATTRIB3SVNVPROC vertexAttrib3svNV = (PFNGLVERTEXATTRIB3SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib3svNV)(6, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3svNV)(6, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3fvNV(generic_func func)
{
   PFNGLVERTEXATTRIB3FVNVPROC vertexAttrib3fvNV = (PFNGLVERTEXATTRIB3FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib3fvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3fvNV)(6, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib3dvNV(generic_func func)
{
   PFNGLVERTEXATTRIB3DVNVPROC vertexAttrib3dvNV = (PFNGLVERTEXATTRIB3DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib3dvNV)(6, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib3dvNV)(6, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4svNV(generic_func func)
{
   PFNGLVERTEXATTRIB4SVNVPROC vertexAttrib4svNV = (PFNGLVERTEXATTRIB4SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 5};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttrib4svNV)(6, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4svNV)(6, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4fvNV(generic_func func)
{
   PFNGLVERTEXATTRIB4FVNVPROC vertexAttrib4fvNV = (PFNGLVERTEXATTRIB4FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttrib4fvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4fvNV)(6, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4dvNV(generic_func func)
{
   PFNGLVERTEXATTRIB4DVNVPROC vertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttrib4dvNV)(6, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4dvNV)(6, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttrib4ubvNV(generic_func func)
{
   PFNGLVERTEXATTRIB4UBVNVPROC vertexAttrib4ubvNV = (PFNGLVERTEXATTRIB4UBVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   /* There's no byte-value query; so we use the float-value query.
    * Bytes are interpreted as steps between 0 and 1, so the
    * expected float values will be 0.0 for byte value 0 and 1.0 for
    * byte value 255.
    */
   GLfloat expectedResults[4] = {1.0, 0.0, 1.0, 0.0};
   (*vertexAttrib4ubvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttrib4ubvNV)(6, def);
   return compare_floats(__FUNCTION__, 4, expectedResults, 4, res);
}

static GLboolean
test_VertexAttribs1fvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS1FVNVPROC vertexAttribs1fvNV = (PFNGLVERTEXATTRIBS1FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttribs1fvNV)(6, 1, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs1fvNV)(6, 1, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs1dvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS1DVNVPROC vertexAttribs1dvNV = (PFNGLVERTEXATTRIBS1DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 0.0, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttribs1dvNV)(6, 1, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs1dvNV)(6, 1, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs2svNV(generic_func func)
{
   PFNGLVERTEXATTRIBS2SVNVPROC vertexAttribs2svNV = (PFNGLVERTEXATTRIBS2SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 0, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttribs2svNV)(6, 1, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs2svNV)(6, 1, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs2fvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS2FVNVPROC vertexAttribs2fvNV = (PFNGLVERTEXATTRIBS2FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttribs2fvNV)(6, 1, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs2fvNV)(6, 1, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs2dvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS2DVNVPROC vertexAttribs2dvNV = (PFNGLVERTEXATTRIBS2DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 0.0, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttribs2dvNV)(6, 1, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs2dvNV)(6, 1, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs3svNV(generic_func func)
{
   PFNGLVERTEXATTRIBS3SVNVPROC vertexAttribs3svNV = (PFNGLVERTEXATTRIBS3SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 1};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttribs3svNV)(6, 1, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs3svNV)(6, 1, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs3fvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS3FVNVPROC vertexAttribs3fvNV = (PFNGLVERTEXATTRIBS3FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttribs3fvNV)(6, 1, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs3fvNV)(6, 1, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs3dvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS3DVNVPROC vertexAttribs3dvNV = (PFNGLVERTEXATTRIBS3DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 1.0};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttribs3dvNV)(6, 1, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs3dvNV)(6, 1, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs4svNV(generic_func func)
{
   PFNGLVERTEXATTRIBS4SVNVPROC vertexAttribs4svNV = (PFNGLVERTEXATTRIBS4SVNVPROC) func;
   PFNGLGETVERTEXATTRIBIVNVPROC getVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribivNV");

   const GLshort v[4] = {2, 4, 7, 5};
   const GLshort def[4] = {0, 0, 0, 1};
   GLint res[4];
   (*vertexAttribs4svNV)(6, 1, v);
   (*getVertexAttribivNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs4svNV)(6, 1, def);
   return compare_shorts_to_ints(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs4fvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS4FVNVPROC vertexAttribs4fvNV = (PFNGLVERTEXATTRIBS4FVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLfloat def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   (*vertexAttribs4fvNV)(6, 1, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs4fvNV)(6, 1, def);
   return compare_floats(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs4dvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS4DVNVPROC vertexAttribs4dvNV = (PFNGLVERTEXATTRIBS4DVNVPROC) func;
   PFNGLGETVERTEXATTRIBDVNVPROC getVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribdvNV");

   const GLdouble v[4] = {2.5, 4.25, 7.125, 5.0625};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLdouble res[4];
   (*vertexAttribs4dvNV)(6, 1, v);
   (*getVertexAttribdvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs4dvNV)(6, 1, def);
   return compare_doubles(__FUNCTION__, 4, v, 4, res);
}

static GLboolean
test_VertexAttribs4ubvNV(generic_func func)
{
   PFNGLVERTEXATTRIBS4UBVNVPROC vertexAttribs4ubvNV = (PFNGLVERTEXATTRIBS4UBVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   /* There's no byte-value query; so we use the float-value query.
    * Bytes are interpreted as steps between 0 and 1, so the
    * expected float values will be 0.0 for byte value 0 and 1.0 for
    * byte value 255.
    */
   GLfloat expectedResults[4] = {1.0, 0.0, 1.0, 0.0};
   (*vertexAttribs4ubvNV)(6, 1, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   (*vertexAttribs4ubvNV)(6, 1, def);
   return compare_floats(__FUNCTION__, 4, expectedResults, 4, res);
}

static GLboolean
test_StencilFuncSeparateATI(generic_func func)
{
#ifdef GL_ATI_separate_stencil
   PFNGLSTENCILFUNCSEPARATEATIPROC stencilFuncSeparateATI = (PFNGLSTENCILFUNCSEPARATEATIPROC) func;
   GLint frontFunc, backFunc;
   GLint frontRef, backRef;
   GLint frontMask, backMask;
   (*stencilFuncSeparateATI)(GL_LESS, GL_GREATER, 2, 0xa);
   glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
   glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
   glGetIntegerv(GL_STENCIL_REF, &frontRef);
   glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
   glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
   if (frontFunc != GL_LESS ||
       backFunc != GL_GREATER ||
       frontRef != 2 ||
       backRef != 2 ||
       frontMask != 0xa ||
       backMask != 0xa)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilFuncSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILFUNCSEPARATEPROC stencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) func;
   GLint frontFunc, backFunc;
   GLint frontRef, backRef;
   GLint frontMask, backMask;
   (*stencilFuncSeparate)(GL_BACK, GL_GREATER, 2, 0xa);
   glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
   glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
   glGetIntegerv(GL_STENCIL_REF, &frontRef);
   glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
   glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
   if (frontFunc != GL_ALWAYS ||
       backFunc != GL_GREATER ||
       frontRef != 0 ||
       backRef != 2 ||
       frontMask == 0xa || /* might be 0xff or ~0 */
       backMask != 0xa)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilOpSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILOPSEPARATEPROC stencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) func;
   GLint frontFail, backFail;
   GLint frontZFail, backZFail;
   GLint frontZPass, backZPass;
   (*stencilOpSeparate)(GL_BACK, GL_INCR, GL_DECR, GL_INVERT);
   glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
   glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
   glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontZFail);
   glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backZFail);
   glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontZPass);
   glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backZPass);
   if (frontFail != GL_KEEP ||
       backFail != GL_INCR ||
       frontZFail != GL_KEEP ||
       backZFail != GL_DECR ||
       frontZPass != GL_KEEP ||
       backZPass != GL_INVERT)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilMaskSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILMASKSEPARATEPROC stencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) func;
   GLint frontMask, backMask;
   (*stencilMaskSeparate)(GL_BACK, 0x1b);
   glGetIntegerv(GL_STENCIL_WRITEMASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &backMask);
   if (frontMask == 0x1b ||
       backMask != 0x1b)
      return GL_FALSE;
#endif
   return GL_TRUE;
}


/*
 * The following file is auto-generated with Python.
 */
#include "getproclist.h"



static int
extension_supported(const char *haystack, const char *needle)
{
   const char *p = strstr(haystack, needle);
   if (p) {
      /* found string, make sure next char is space or zero */
      const int len = strlen(needle);
      if (p[len] == ' ' || p[len] == 0)
         return 1;
      else
         return 0;
   }
   else
      return 0;
}


/* Run all the known extension function tests, if the extension is supported.
 * Return a count of how many failed.
 */
static int
check_functions( const char *extensions )
{
   struct name_test_pair *entry;
   int failures = 0, passes = 0, untested = 0;
   int totalFail = 0, totalPass = 0, totalUntested = 0, totalUnsupported = 0;
   int doTests = 0;
   const char *version = (const char *) glGetString(GL_VERSION);

   /* The functions list will have "real" entries (consisting of
    * a GL function name and a pointer to an exercise function for
    * that GL function), and "group" entries (indicated as
    * such by having a "-" as the first character of the name).
    * "Group" names always start with the "-" character, and can
    * be numeric (e.g. "-1.0", "-2.1"), indicating that a particular
    * OpenGL version is required for the following functions; or can be
    * an extension name (e.g. "-GL_ARB_multitexture") that means
    * that the named extension is required for the following functions.
    */
   for (entry = functions; entry->name; entry++) {
      /* Check if this is a group indicator */
      if (entry->name[0] == '-') {
         /* A group indicator; check if it's an OpenGL version group */
         if (entry->name[1] == '1') {
            /* check GL version 1.x */
            if (version[0] == '1' &&
                version[1] == '.' &&
                version[2] >= entry->name[3])
               doTests = 1;
            else
               doTests = 0;
         }
         else if (entry->name[1] == '2') {
            if (version[0] == '2' &&
                version[1] == '.' &&
                version[2] >= entry->name[3])
               doTests = 1;
            else
               doTests = 0;
         }
         else {
            /* check if the named extension is available */
            doTests = extension_supported(extensions, entry->name+1);
         }

         /* doTests is now set if we're starting an OpenGL version
          * group, and the running OpenGL version is at least the
          * version required; or if we're starting an OpenGL extension
          * group, and the extension is supported.
          */
         if (doTests)
            printf("Testing %s functions\n", entry->name + 1);

         /* Each time we hit a title function, reset the function
          * counts.
          */
         failures = 0;
         passes = 0;
         untested = 0;
      }
      else if (doTests) {
         /* Here, we know we're trying to exercise a function for
          * a supported extension.  See whether we have a test for
          * it, and try to run it.
          */
         generic_func funcPtr = (generic_func) glXGetProcAddressARB((const GLubyte *) entry->name);
         if (funcPtr) {
            if (entry->test) {
               GLboolean b;
               printf("   Validating %s:", entry->name);
               b = (*entry->test)(funcPtr);
               if (b) {
                  printf(" Pass\n");
                  passes++;
                  totalPass++;
               }
               else {
                  printf(" FAIL!!!\n");
                  failures++;
                  totalFail++;
               }
            }
            else {
               untested++;
               totalUntested++;
            }
         }
         else {
            printf("   glXGetProcAddress(%s) failed!\n", entry->name);
            failures++;
            totalFail++;
         }
      }
      else {
         /* Here, we have a function that belongs to a group that
          * is known to be unsupported.
          */
         totalUnsupported++;
      }

      /* Make sure a poor test case doesn't leave any lingering
       * OpenGL errors.
       */
      CheckGLError(__LINE__, __FILE__, __FUNCTION__);

      if (doTests && (!(entry+1)->name || (entry+1)->name[0] == '-')) {
         if (failures > 0) {
            printf("   %d failed.\n", failures);
         }
         if (passes > 0) {
            printf("   %d passed.\n", passes);
         }
         if (untested > 0) {
            printf("   %d untested.\n", untested);
         }
      }
   }

   printf("-----------------------------\n");
   printf("Total: %d pass  %d fail  %d untested  %d unsupported  %d total\n", 
      totalPass, totalFail, totalUntested, totalUnsupported,
      totalPass + totalFail + totalUntested + totalUnsupported);

   return totalFail;
}


/* Return an error code */
#define ERROR_NONE 0
#define ERROR_NO_VISUAL 1
#define ERROR_NO_CONTEXT 2
#define ERROR_NO_MAKECURRENT 3
#define ERROR_FAILED 4

static int
print_screen_info(Display *dpy, int scrnum, Bool allowDirect)
{
   Window win;
   int attribSingle[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      None };
   int attribDouble[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None };

   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   GLXContext ctx;
   XVisualInfo *visinfo;
   int width = 100, height = 100;
   int failures;

   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attribSingle);
   if (!visinfo) {
      visinfo = glXChooseVisual(dpy, scrnum, attribDouble);
      if (!visinfo) {
         fprintf(stderr, "Error: couldn't find RGB GLX visual\n");
         return ERROR_NO_VISUAL;
      }
   }

   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   win = XCreateWindow(dpy, root, 0, 0, width, height,
		       0, visinfo->depth, InputOutput,
		       visinfo->visual, mask, &attr);

   ctx = glXCreateContext( dpy, visinfo, NULL, allowDirect );
   if (!ctx) {
      fprintf(stderr, "Error: glXCreateContext failed\n");
      XDestroyWindow(dpy, win);
      return ERROR_NO_CONTEXT;
   }

   if (!glXMakeCurrent(dpy, win, ctx)) {
      fprintf(stderr, "Error: glXMakeCurrent failed\n");
      glXDestroyContext(dpy, ctx);
      XDestroyWindow(dpy, win);
      return ERROR_NO_MAKECURRENT;
   }

   failures = check_functions( (const char *) glGetString(GL_EXTENSIONS) );
   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);

   return (failures == 0 ? ERROR_NONE : ERROR_FAILED);
}

int
main(int argc, char *argv[])
{
   char *displayName = NULL;
   Display *dpy;
   int returnCode;

   dpy = XOpenDisplay(displayName);
   if (!dpy) {
      fprintf(stderr, "Error: unable to open display %s\n", displayName);
      return -1;
   }

   returnCode = print_screen_info(dpy, 0, GL_TRUE);

   XCloseDisplay(dpy);

   return returnCode;
}
