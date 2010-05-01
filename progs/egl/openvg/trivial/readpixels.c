#include "eglcommon.h"

#include <VG/openvg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

float red_color[4] = {1.0, 0.0, 0.0, 1.0};
float blue_color[4] = {0.0, 0.0, 1.0, 1.0};
VGint *data;

static void
init(void)
{
   data = malloc(sizeof(VGint)*2048*2048);
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
   vgLoadIdentity();
}

static void
draw(void)
{
   static const VGint red_pixel  = 255 << 24 | 255 << 16 | 0 << 8 |   0;
   static const VGint blue_pixel = 255 << 24 |   0 << 16 | 0 << 8 | 255;
   VGint i;

   vgSetfv(VG_CLEAR_COLOR, 4, red_color);
   vgClear(0, 0, window_width(), window_height());
   vgFlush();

   memset(data, 0, window_width() * window_height() * sizeof(VGint));

   vgReadPixels(data, window_width() * sizeof(VGint),
                VG_lARGB_8888,
                0, 0, window_width(), window_height());

   fprintf(stderr, "Red 0 = 0x%x and at 600 = 0x%x\n",
           data[0], data[600]);
   for (i = 0; i < window_width() * window_height(); ++i) {
      assert(data[i] == red_pixel);
   }

   vgSetfv(VG_CLEAR_COLOR, 4, blue_color);
   vgClear(50, 50, 50, 50);
   vgFlush();

   memset(data, 0, window_width() * window_height() * sizeof(VGint));

   vgReadPixels(data, 50 * sizeof(VGint),
                VG_lARGB_8888,
                50, 50, 50, 50);

   fprintf(stderr, "Blue 0 = 0x%x and at 100 = 0x%x\n",
           data[0], data[100]);
   for (i = 0; i < 50 * 50; ++i) {
      assert(data[i] == blue_pixel);
   }
}


int main(int argc, char **argv)
{
   int ret = run(argc, argv, init, reshape,
                 draw, 0);

   free(data);
   return ret;
}
