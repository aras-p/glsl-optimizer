/*
 * Mesa 3-D graphics library
 * Version:  3.4
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * DOS/DJGPP glut driver v1.6 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "glutint.h"


GLUTSShotCB g_sscb[MAX_SSHOT_CB];

GLUTidleCB g_idle_func = NULL;


void APIENTRY
glutDisplayFunc (GLUTdisplayCB func)
{
   g_curwin->display = func;
}


void APIENTRY
glutReshapeFunc (GLUTreshapeCB func)
{
   g_curwin->reshape = func;
}


void APIENTRY
glutKeyboardFunc (GLUTkeyboardCB func)
{
   g_curwin->keyboard = func;
}


void APIENTRY
glutMouseFunc (GLUTmouseCB func)
{
   g_curwin->mouse = func;
}


void APIENTRY
glutMotionFunc (GLUTmotionCB func)
{
   g_curwin->motion = func;
}


void APIENTRY
glutPassiveMotionFunc (GLUTpassiveCB func)
{
   g_curwin->passive = func;
}


void APIENTRY
glutEntryFunc (GLUTentryCB func)
{
   g_curwin->entry = func;
}


void APIENTRY
glutVisibilityFunc (GLUTvisibilityCB func)
{
   g_curwin->visibility = func;
}


void APIENTRY
glutWindowStatusFunc (GLUTwindowStatusCB func)
{
}


void APIENTRY
glutIdleFunc (GLUTidleCB func)
{
   g_idle_func = func;
}


void APIENTRY
glutTimerFunc (unsigned int millis, GLUTtimerCB func, int value)
{
   int i;

   if (millis > 0) {
      for (i = 0; i < MAX_SSHOT_CB; i++) {
         GLUTSShotCB *cb = &g_sscb[i];
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
   g_curwin->special = func;
}


void APIENTRY
glutSpaceballMotionFunc (GLUTspaceMotionCB func)
{
}


void APIENTRY
glutSpaceballRotateFunc (GLUTspaceRotateCB func)
{
}


void APIENTRY
glutSpaceballButtonFunc (GLUTspaceButtonCB func)
{
}


void APIENTRY
glutDialsFunc (GLUTdialsCB func)
{
}


void APIENTRY
glutButtonBoxFunc (GLUTbuttonBoxCB func)
{
}


void APIENTRY
glutTabletMotionFunc (GLUTtabletMotionCB func)
{
}


void APIENTRY
glutTabletButtonFunc (GLUTtabletButtonCB func)
{
}


void APIENTRY
glutJoystickFunc (GLUTjoystickCB func, int interval)
{
}
