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


GLUTmenuStatusCB _glut_menu_status_func = NULL;


void APIENTRY
glutMenuStateFunc (GLUTmenuStateCB func)
{
   _glut_menu_status_func = (GLUTmenuStatusCB)func;
}


void APIENTRY
glutMenuStatusFunc (GLUTmenuStatusCB func)
{
   _glut_menu_status_func = func;
}


int APIENTRY
glutCreateMenu (GLUTselectCB func)
{
   return 0;
}


void APIENTRY
glutDestroyMenu (int menu)
{
}


int APIENTRY
glutGetMenu (void)
{
   return 0;
}


void APIENTRY
glutSetMenu (int menu)
{
}


void APIENTRY
glutAddMenuEntry (const char *label, int value)
{
}


void APIENTRY
glutAddSubMenu (const char *label, int submenu)
{
}


void APIENTRY
glutChangeToMenuEntry (int item, const char *label, int value)
{
}


void APIENTRY
glutChangeToSubMenu (int item, const char *label, int submenu)
{
}


void APIENTRY
glutRemoveMenuItem (int item)
{
}


void APIENTRY
glutAttachMenu (int button)
{
}


void APIENTRY
glutDetachMenu (int button)
{
}


void APIENTRY
glutMenuDestroyFunc ( void (* callback)( void ) )
{
}


void * APIENTRY
glutGetMenuData (void)
{
   return NULL;
}


void APIENTRY
glutSetMenuData (void *data)
{
}
