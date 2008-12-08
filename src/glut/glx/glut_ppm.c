/*
 * PPM file output
 * Brian Paul
 * 8 Dec 2008
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "glutint.h"


static void
write_ppm_file(const char *filename, const GLubyte *buffer,
               int width, int height)
{
   const int binary = 1;
   FILE *f = fopen( filename, "w" );
   if (f) {
      const GLubyte *ptr = buffer;
      int i, x, y;
      if (binary) {
         fprintf(f,"P6\n");
         fprintf(f,"# ppm-file created by GLUT\n");
         fprintf(f,"%i %i\n", width, height);
         fprintf(f,"255\n");
         fclose(f);
         f = fopen( filename, "ab" );  /* reopen in binary append mode */
         for (y = height - 1; y >= 0; y--) {
            for (x = 0; x < width; x++) {
               i = (y * width + x) * 4;
               fputc(ptr[i], f);   /* write red */
               fputc(ptr[i+1], f); /* write green */
               fputc(ptr[i+2], f); /* write blue */
            }
         }
      }
      else {
         /*ASCII*/
         int counter = 0;
         fprintf(f,"P3\n");
         fprintf(f,"# ascii ppm file created by GLUT\n");
         fprintf(f,"%i %i\n", width, height);
         fprintf(f,"255\n");
         for (y = height - 1; y >= 0; y--) {
            for (x = 0; x < width; x++) {
               i = (y * width + x) * 4;
               fprintf(f, " %3d %3d %3d", ptr[i], ptr[i+1], ptr[i+2]);
               counter++;
               if (counter % 5 == 0)
                  fprintf(f, "\n");
            }
         }
      }
      fclose(f);
   }
}


/**
 * Called from SwapBuffers if the GLUT_PPM_FILE env var is set.
 */
void __glutWritePPMFile(void)
{
   int w = glutGet(GLUT_WINDOW_WIDTH);
   int h = glutGet(GLUT_WINDOW_HEIGHT);
   GLubyte *buf;

   assert(__glutPPMFile);

   buf = (GLubyte *) malloc(w * h * 4);
   if (buf) {
      /* XXX save/restore pixel packing */
      glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
      write_ppm_file(__glutPPMFile, buf, w, h);
      free(buf);
   }

   __glutPPMFile = NULL; /* only write one file */
}
