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
 * DOS/DJGPP glut driver v1.5 for Mesa
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include "glutint.h"



typedef struct {
        void (*func) (int); /* function to call */
        int value;          /* value to pass to callback */
        int ttl;            /* time to live (blank shots) */
        int fid;            /* func-id as returned from PCHW */
} GLUTSShotCB;

GLUTidleCB g_idle_func = NULL;



static void g_single_shot_callback (void *opaque)
{
 GLUTSShotCB *cb = (GLUTSShotCB *)opaque;
 if (!--cb->ttl) {
    cb->func(cb->value);
    pc_remove_int(cb->fid);
    /* We won't be needing this slot anymore, so free it. This operation
     * must be the last thing, and must be atomic, to mutex `glutTimerFunc'
     */
    cb->func = NULL;
 }
} ENDOFUNC(g_single_shot_callback)



void APIENTRY glutDisplayFunc (GLUTdisplayCB func)
{
 g_curwin->display = func;
}



void APIENTRY glutReshapeFunc (GLUTreshapeCB func)
{
 g_curwin->reshape = func;
}



void APIENTRY glutKeyboardFunc (GLUTkeyboardCB func)
{
 g_curwin->keyboard = func;
}



void APIENTRY glutMouseFunc (GLUTmouseCB func)
{
 g_curwin->mouse = func;
}



void APIENTRY glutMotionFunc (GLUTmotionCB func)
{
 g_curwin->motion = func;
}



void APIENTRY glutPassiveMotionFunc (GLUTpassiveCB func)
{
 g_curwin->passive = func;
}



void APIENTRY glutEntryFunc (GLUTentryCB func)
{
 g_curwin->entry = func;
}



void APIENTRY glutVisibilityFunc (GLUTvisibilityCB func)
{
 g_curwin->visibility = func;
}



void APIENTRY glutWindowStatusFunc (GLUTwindowStatusCB func)
{
}



void APIENTRY glutIdleFunc (GLUTidleCB func)
{
 g_idle_func = func;
}



void APIENTRY glutTimerFunc (unsigned int millis, GLUTtimerCB func, int value)
{
 static GLUTSShotCB g_sscb[MAX_SSHOT_CB];
 static GLboolean virgin = GL_TRUE;

 int i;
 int ttl;
 unsigned int freq;

 if (virgin) {
    virgin = GL_FALSE;
    LOCKDATA(g_sscb);
    LOCKFUNC(g_single_shot_callback);
    /* we should lock the callee also... */
 }

 if (millis > 0) {
    if (millis > 50) {
       /* don't go beyond 20Hz */
       freq = 200;
       ttl = millis * freq / 1000;
    } else {
       freq = 1000 / millis;
       ttl = 1;
    }
    for (i = 0; i < MAX_SSHOT_CB; i++) {
        if (g_sscb[i].func == NULL) {
           /* We will be needing this slot, so alloc it. This operation
            * must be the first thing, and must be atomic, to mutex callbacks!
            */
           g_sscb[i].func = func;
           g_sscb[i].value = value;
           g_sscb[i].ttl = ttl;
           /* There is a very small gap here: `pc_install_int' enables
            * interrupts just before returning FID value (which will be
            * used inside callback). The critical gap is 1 millisecond
            * - which I'm sure we won't overrun...
            */
           g_sscb[i].fid = pc_install_int((PFUNC)func, &g_sscb[i], freq);
           if (g_sscb[i].fid < 0) {
              /* Interrupt could not be set! Release the slot back */
              g_sscb[i].func = NULL;
           }
           break;
        }
    }
 }
}



void APIENTRY glutSpecialFunc (GLUTspecialCB func)
{
 g_curwin->special = func;
}



void APIENTRY glutSpaceballMotionFunc (GLUTspaceMotionCB func)
{
}



void APIENTRY glutSpaceballRotateFunc (GLUTspaceRotateCB func)
{
}



void APIENTRY glutSpaceballButtonFunc (GLUTspaceButtonCB func)
{
}



void APIENTRY glutDialsFunc (GLUTdialsCB func)
{
}



void APIENTRY glutButtonBoxFunc (GLUTbuttonBoxCB func)
{
}



void APIENTRY glutTabletMotionFunc (GLUTtabletMotionCB func)
{
}



void APIENTRY glutTabletButtonFunc (GLUTtabletButtonCB func)
{
}



void APIENTRY glutJoystickFunc (GLUTjoystickCB func, int interval)
{
}
