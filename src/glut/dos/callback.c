/*
 * DOS/DJGPP Mesa Utility Toolkit
 * Version:  1.0
 *
 * Copyright (C) 2005  Daniel Borca   All Rights Reserved.
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
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

 
#include "internal.h"


GLUTSShotCB _glut_timer_cb[MAX_TIMER_CB];

GLUTidleCB _glut_idle_func = NULL;


void APIENTRY
glutDisplayFunc (GLUTdisplayCB func)
{
   _glut_current->display = func;
}


void APIENTRY
glutReshapeFunc (GLUTreshapeCB func)
{
   _glut_current->reshape = func;
}


void APIENTRY
glutKeyboardFunc (GLUTkeyboardCB func)
{
   _glut_current->keyboard = func;
}


void APIENTRY
glutMouseFunc (GLUTmouseCB func)
{
   _glut_current->mouse = func;
}


void APIENTRY
glutMotionFunc (GLUTmotionCB func)
{
   _glut_current->motion = func;
}


void APIENTRY
glutPassiveMotionFunc (GLUTpassiveCB func)
{
   _glut_current->passive = func;
}


void APIENTRY
glutEntryFunc (GLUTentryCB func)
{
   _glut_current->entry = func;
}


void APIENTRY
glutVisibilityFunc (GLUTvisibilityCB func)
{
   _glut_current->visibility = func;
}


void APIENTRY
glutWindowStatusFunc (GLUTwindowStatusCB func)
{
   _glut_current->windowStatus = func;
}


void APIENTRY
glutIdleFunc (GLUTidleCB func)
{
   _glut_idle_func = func;
}


void APIENTRY
glutTimerFunc (unsigned int millis, GLUTtimerCB func, int value)
{
   int i;

   if (millis > 0) {
      for (i = 0; i < MAX_TIMER_CB; i++) {
         GLUTSShotCB *cb = &_glut_timer_cb[i];
         if (cb->func == NULL) {
            cb->value = value;
            cb->func = func;
            cb->time = glutGet(GLUT_ELAPSED_TIME) + millis;
            break;
         }
      }
   }
}


void APIENTRY
glutSpecialFunc (GLUTspecialCB func)
{
   _glut_current->special = func;
}


void APIENTRY
glutSpaceballMotionFunc (GLUTspaceMotionCB func)
{
   _glut_current->spaceMotion = func;
}


void APIENTRY
glutSpaceballRotateFunc (GLUTspaceRotateCB func)
{
   _glut_current->spaceRotate = func;
}


void APIENTRY
glutSpaceballButtonFunc (GLUTspaceButtonCB func)
{
   _glut_current->spaceButton = func;
}


void APIENTRY
glutDialsFunc (GLUTdialsCB func)
{
   _glut_current->dials = func;
}


void APIENTRY
glutButtonBoxFunc (GLUTbuttonBoxCB func)
{
   _glut_current->buttonBox = func;
}


void APIENTRY
glutTabletMotionFunc (GLUTtabletMotionCB func)
{
   _glut_current->tabletMotion = func;
}


void APIENTRY
glutTabletButtonFunc (GLUTtabletButtonCB func)
{
   _glut_current->tabletButton = func;
}


void APIENTRY
glutJoystickFunc (GLUTjoystickCB func, int interval)
{
   _glut_current->joystick = func;
}


void APIENTRY
glutKeyboardUpFunc (GLUTkeyboardCB func)
{
   _glut_current->keyboardUp = func;
}


void APIENTRY
glutSpecialUpFunc (GLUTspecialCB func)
{
   _glut_current->specialUp = func;
}


void APIENTRY
glutMouseWheelFunc (GLUTmouseWheelCB func)
{
   _glut_current->mouseWheel = func;
}
