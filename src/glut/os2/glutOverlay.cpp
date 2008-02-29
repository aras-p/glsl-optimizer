/***********************************************************
 *     Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:      glutOverlay.cpp
 *
 *     DESCRIPTION:    we don't support overlays, so this code is
 *             really simple
 ***********************************************************/

/***********************************************************
 *     Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"
#include "glutbitmap.h"
#include "glutstroke.h"

GLUTAPI void GLUTAPIENTRY
glutEstablishOverlay(void)
{
       __glutFatalError("OS2PM lacks overlay support.");
}

GLUTAPI void GLUTAPIENTRY
glutUseLayer(GLenum layer) {
       // ignore
}

GLUTAPI void GLUTAPIENTRY
glutRemoveOverlay(void) {
       // ignore
}

GLUTAPI void GLUTAPIENTRY
glutPostOverlayRedisplay(void) {
       // ignore
}

GLUTAPI void GLUTAPIENTRY
glutShowOverlay(void) {
       // ignore
}

GLUTAPI void GLUTAPIENTRY glutHideOverlay(void)
{
       // ignore
}

int GLUTAPIENTRY
glutLayerGet(GLenum param)
{
       // ignore
}

/***********************************************************
 *     Unsupported callbacks
 ***********************************************************/
GLUTAPI void GLUTAPIENTRY
glutOverlayDisplayFunc(GLUTdisplayCB displayFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutSpaceballMotionFunc(GLUTspaceMotionCB spaceMotionFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutSpaceballRotateFunc(GLUTspaceRotateCB spaceRotateFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutSpaceballButtonFunc(GLUTspaceButtonCB spaceButtonFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutButtonBoxFunc(GLUTbuttonBoxCB buttonBoxFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutDialsFunc(GLUTdialsCB dialsFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutTabletMotionFunc(GLUTtabletMotionCB tabletMotionFunc)
{
}

GLUTAPI void GLUTAPIENTRY
glutTabletButtonFunc(GLUTtabletButtonCB tabletButtonFunc)
{
}
GLUTAPI void GLUTAPIENTRY
glutPostWindowOverlayRedisplay(int win)
{ //
}

void GLUTAPIENTRY
glutInitDisplayString(const char *string)
{ //
}
void GLUTAPIENTRY
glutJoystickFunc(GLUTjoystickCB joystickFunc, int pollInterval)
{ //
}

void GLUTAPIENTRY
glutForceJoystickFunc(void)
{ //
}


int  GLUTAPIENTRY
glutBitmapWidth(GLUTbitmapFont font, int c)
{  return 0;
}
int  GLUTAPIENTRY
glutBitmapLength(GLUTbitmapFont font, const unsigned char *string)
{ //
  return 0;
}
int GLUTAPIENTRY
glutStrokeWidth(GLUTstrokeFont font, int c)
{  return 0;
}
int GLUTAPIENTRY
glutStrokeLength(GLUTstrokeFont font, const unsigned char *string)
{ return 0;
}
