

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include "readtex.h"
#include "skybox.h"


static int
load(GLenum target, const char *filename,
     GLboolean flipTB, GLboolean flipLR)
{
   GLint w, h;
   GLenum format;
   GLubyte *img = LoadRGBImage( filename, &w, &h, &format );
   if (!img) {
      printf("Error: couldn't load texture image %s\n", filename);
      return 0;
   }
   assert(format == GL_RGB);

   printf("Load cube face 0x%x: %s %d x %d\n", target, filename, w, h);

   /* <sigh> the way the texture cube mapping works, we have to flip
    * images to make things look right.
    */
   if (flipTB) {
      const int stride = 3 * w;
      GLubyte temp[3*1024];
      int i;
      for (i = 0; i < h / 2; i++) {
         memcpy(temp, img + i * stride, stride);
         memcpy(img + i * stride, img + (h - i - 1) * stride, stride);
         memcpy(img + (h - i - 1) * stride, temp, stride);
      }
   }
   if (flipLR) {
      const int stride = 3 * w;
      GLubyte temp[3];
      GLubyte *row;
      int i, j;
      for (i = 0; i < h; i++) {
         row = img + i * stride;
         for (j = 0; j < w / 2; j++) {
            int k = w - j - 1;
            temp[0] = row[j*3+0];
            temp[1] = row[j*3+1];
            temp[2] = row[j*3+2];
            row[j*3+0] = row[k*3+0];
            row[j*3+1] = row[k*3+1];
            row[j*3+2] = row[k*3+2];
            row[k*3+0] = temp[0];
            row[k*3+1] = temp[1];
            row[k*3+2] = temp[2];
         }
      }
   }

   gluBuild2DMipmaps(target, GL_RGB, w, h, format, GL_UNSIGNED_BYTE, img);
   free(img);
   return 1;
}


GLuint
LoadSkyBoxCubeTexture(const char *filePosX,
                      const char *fileNegX,
                      const char *filePosY,
                      const char *fileNegY,
                      const char *filePosZ,
                      const char *fileNegZ)
{
   GLuint tex;

   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                   GL_LINEAR_MIPMAP_NEAREST);

   if (!load(GL_TEXTURE_CUBE_MAP_POSITIVE_X, filePosX, GL_TRUE, GL_TRUE))
      return 0;
   if (!load(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, fileNegX, GL_TRUE, GL_TRUE))
      return 0;
   if (!load(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, filePosY, GL_TRUE, GL_TRUE))
      return 0;
   if (!load(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, fileNegY, GL_TRUE, GL_TRUE))
      return 0;
   if (!load(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, filePosZ, GL_TRUE, GL_TRUE))
      return 0;
   if (!load(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, fileNegZ, GL_TRUE, GL_TRUE))
      return 0;

   return tex;
}


#define eps1 0.99
#define br   20.0  /* box radius */

void
DrawSkyBoxCubeTexture(GLuint tex)
{
   struct vertex {
      float x, y, z, s, t, r;
   };

   static const struct vertex verts[24] = {
      /* +X side */
      { br, -br, -br,  1.0, -eps1, -eps1 },
      { br, -br,  br,  1.0, -eps1,  eps1 },
      { br,  br,  br,  1.0,  eps1,  eps1 },
      { br,  br, -br,  1.0,  eps1, -eps1 },

      /* -X side */
      { -br,  br, -br,  -1.0,  eps1, -eps1 },
      { -br,  br,  br,  -1.0,  eps1,  eps1 },
      { -br, -br,  br,  -1.0, -eps1,  eps1 },
      { -br, -br, -br,  -1.0, -eps1, -eps1 },

      /* +Y side */
      {  br,  br, -br,   eps1, 1.0, -eps1 },
      {  br,  br,  br,   eps1, 1.0,  eps1 },
      { -br,  br,  br,  -eps1, 1.0,  eps1 },
      { -br,  br, -br,  -eps1, 1.0, -eps1 },

      /* -Y side */
      { -br, -br, -br,  -eps1, -1.0, -eps1 },
      { -br, -br,  br,  -eps1, -1.0,  eps1 },
      {  br, -br,  br,   eps1, -1.0,  eps1 },
      {  br, -br, -br,   eps1, -1.0, -eps1 },

      /* +Z side */
      {  br, -br, br,   eps1, -eps1, 1.0 },
      { -br, -br, br,  -eps1, -eps1, 1.0 },
      { -br,  br, br,  -eps1,  eps1, 1.0 },
      {  br,  br, br,   eps1,  eps1, 1.0 },

      /* -Z side */
      {  br,  br, -br,   eps1,  eps1, -1.0 },
      { -br,  br, -br,  -eps1,  eps1, -1.0 },
      { -br, -br, -br,  -eps1, -eps1, -1.0 },
      {  br, -br, -br,   eps1, -eps1, -1.0 },
   };

   static GLuint vbo = 0;

   if (!vbo ) {
      glGenBuffersARB(1, &vbo);
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
      glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts,
                      GL_STATIC_DRAW_ARB);
   }
   else {
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);
   }

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glVertexPointer(3, GL_FLOAT, sizeof(struct vertex),
                   (void *) offsetof(struct vertex, x));
   glTexCoordPointer(3, GL_FLOAT, sizeof(struct vertex),
                     (void *) offsetof(struct vertex, s));
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
   glEnable(GL_TEXTURE_CUBE_MAP);

   glDisable(GL_BLEND);

   glDrawArrays(GL_QUADS, 0, 24);

   glDisable(GL_TEXTURE_CUBE_MAP);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

