
/* uglalldemos.c - WindML/Mesa example program */

/* Copyright (C) 2001 by Wind River Systems, Inc */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * The MIT License
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
 * THE AUTHORS OR COPYRIGHT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
modification history
--------------------
02a,29aug01,sra  WindML mode added
01a,17jul01,sra  written
*/

/*
DESCRIPTION
Show all the UGL/Mesa demos
*/

#include <stdio.h>
#include <vxWorks.h>
#include <taskLib.h>
#include <ugl/ugl.h>
#include <ugl/uglinput.h>
#include <ugl/uglevent.h>
#include <ugl/uglfont.h>

#define BLACK 0
#define RED   1

struct _colorStruct
    {
    UGL_RGB rgbColor;
    UGL_COLOR uglColor;
    }
colorTable[] =
    {
    { UGL_MAKE_RGB(0, 0, 0), 0},
    { UGL_MAKE_RGB(255, 0, 0), 0},
    };

void windMLPoint (UGL_BOOL windMLMode);
void windMLLine (UGL_BOOL windMLMode);
void windMLFlip (UGL_BOOL windMLMode);
void windMLCube (UGL_BOOL windMLMode);
void windMLBounce (UGL_BOOL windMLMode);
void windMLGears (UGL_BOOL windMLMode);
void windMLIcoTorus (UGL_BOOL windMLMode);
void windMLOlympic (UGL_BOOL windMLMode);
void windMLTexCube (UGL_BOOL windMLMode);
void windMLTexCyl (UGL_BOOL windMLMode);
void windMLTeapot (UGL_BOOL windMLMode);
void windMLStencil (UGL_BOOL windMLMode);
void windMLDrawPix (UGL_BOOL windMLMode);
void windMLAccum (UGL_BOOL windMLMode);
void windMLAllDemos (void);

void uglalldemos (void)
    {
    taskSpawn("tAllDemos", 210, VX_FP_TASK, 200000,
	      (FUNCPTR)windMLAllDemos, 0,1,2,3,4,5,6,7,8,9);
    }

void windMLAllDemos(void)
    {
    UGL_BOOL windMLFlag = UGL_FALSE;
    UGL_FB_INFO fbInfo;
    UGL_EVENT event;
    UGL_EVENT_SERVICE_ID eventServiceId;
    UGL_EVENT_Q_ID qId;
    UGL_INPUT_EVENT * pInputEvent;
    UGL_INPUT_DEVICE_ID keyboardDevId;
    UGL_DEVICE_ID devId;
    UGL_GC_ID gc;
    UGL_FONT_ID fontId;
    UGL_FONT_DEF fontDef;
    UGL_FONT_DRIVER_ID fontDrvId;
    UGL_ORD textOrigin = UGL_FONT_TEXT_UPPER_LEFT;
    int displayHeight, displayWidth;
    int textWidth, textHeight;
    static UGL_CHAR * message =
	"Do you want to use WindML exclusively ? (y/n) ";
    
    uglInitialize();

    uglDriverFind (UGL_DISPLAY_TYPE, 0, (UGL_UINT32 *)&devId);
    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);
    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);
    qId = uglEventQCreate (eventServiceId, 100);
    
    gc = uglGcCreate(devId);

    uglDriverFind (UGL_FONT_ENGINE_TYPE, 0, (UGL_UINT32 *)&fontDrvId);
    uglFontDriverInfo(fontDrvId, UGL_FONT_TEXT_ORIGIN, &textOrigin);

    uglFontFindString(fontDrvId, "familyName=Helvetica; pixelSize = 18",
		      &fontDef);

    if ((fontId = uglFontCreate(fontDrvId, &fontDef)) == UGL_NULL)
        {
 	printf("Font not found. Exiting.\n");
	return;       
        }

    uglInfo(devId, UGL_FB_INFO_REQ, &fbInfo);
    displayWidth = fbInfo.width;
    displayHeight = fbInfo.height;

    uglColorAlloc (devId, &colorTable[BLACK].rgbColor, UGL_NULL, 
                   &colorTable[BLACK].uglColor, 1);
    uglColorAlloc(devId, &colorTable[RED].rgbColor, UGL_NULL,
		  &colorTable[RED].uglColor, 1);
    
    uglBackgroundColorSet(gc, colorTable[BLACK].uglColor);
    uglForegroundColorSet(gc, colorTable[RED].uglColor);
    uglFontSet(gc, fontId);
    uglTextSizeGet(fontId, &textWidth, &textHeight, -1, message);
    uglTextDraw(gc, (displayWidth - textWidth) / 2, 
		(displayHeight - textHeight) / 2  - textHeight, -1, message);
/*    flushQ();
 */   
    if (uglEventGet (qId, &event, sizeof (event), UGL_WAIT_FOREVER)
	!= UGL_STATUS_Q_EMPTY)
	{
	pInputEvent = (UGL_INPUT_EVENT *)&event;
	    
	if (pInputEvent->header.type == UGL_EVENT_TYPE_KEYBOARD &&
	    pInputEvent->modifiers & UGL_KEYBOARD_KEYDOWN)
	    {
	    switch(pInputEvent->type.keyboard.key)
		{
		case 'Y':
		case 'y':
		    windMLFlag = UGL_TRUE;
		    break;
		default:
		    windMLFlag = UGL_FALSE;
		}
	    }
	}
    
    uglFontDestroy (fontId);
    uglGcDestroy (gc);
    uglEventQDestroy (eventServiceId, qId);
    uglDeinitialize();
		   
    windMLPoint(windMLFlag);

    windMLLine(windMLFlag);
    
    windMLFlip(windMLFlag);

    windMLCube(windMLFlag);

    windMLBounce(windMLFlag);

    windMLGears(windMLFlag);

    windMLIcoTorus(windMLFlag);

    windMLOlympic(windMLFlag);

    windMLTexCube(windMLFlag);
    
    windMLTexCyl(windMLFlag);

    windMLTeapot(windMLFlag);

    windMLStencil(windMLFlag);

    windMLDrawPix(windMLFlag);

    windMLAccum(windMLFlag);

    return;
    }
