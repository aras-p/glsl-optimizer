
/* Copyright (c) Mark J. Kilgard, 1994, 1997.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdio.h>
#include "glutint.h"

GLint __glutFPS = 0;
GLint __glutSwapCount = 0;
GLint __glutSwapTime = 0;

/* CENTRY */
void GLUTAPIENTRY
glutSwapBuffers(void)
{
  GLUTwindow *window = __glutCurrentWindow;

  if (__glutPPMFile) {
     __glutWritePPMFile();
  }

  if (window->renderWin == window->win) {
    if (__glutCurrentWindow->treatAsSingle) {
      /* Pretend the double buffered window is single buffered,
         so treat glutSwapBuffers as a no-op. */
      return;
    }
  } else {
    if (__glutCurrentWindow->overlay->treatAsSingle) {
      /* Pretend the double buffered overlay is single
         buffered, so treat glutSwapBuffers as a no-op. */
      return;
    }
  }

  /* For the MESA_SWAP_HACK. */
  window->usedSwapBuffers = 1;

  SWAP_BUFFERS_LAYER(__glutCurrentWindow);

  /* I considered putting the window being swapped on the
     GLUT_FINISH_WORK work list because you could call
     glutSwapBuffers from an idle callback which doesn't call
     __glutSetWindow which normally adds indirect rendering
     windows to the GLUT_FINISH_WORK work list.  Not being put
     on the list could lead to the buffering up of multiple
     redisplays and buffer swaps and hamper interactivity.  I
     consider this an application bug due to not using
     glutPostRedisplay to trigger redraws.  If
     glutPostRedisplay were used, __glutSetWindow would be
     called and a glFinish to throttle buffering would occur. */

  if (__glutFPS) {
     GLint t = glutGet(GLUT_ELAPSED_TIME);
     __glutSwapCount++;
     if (__glutSwapTime == 0)
        __glutSwapTime = t;
     else if (t - __glutSwapTime > __glutFPS) {
        float time = 0.001 * (t - __glutSwapTime);
        float fps = (float) __glutSwapCount / time;
        fprintf(stderr, "GLUT: %d frames in %.2f seconds = %.2f FPS\n",
                __glutSwapCount, time, fps);
        __glutSwapTime = t;
        __glutSwapCount = 0;
     }
  }
}
/* ENDCENTRY */
