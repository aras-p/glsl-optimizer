/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutOverlay.cpp
 *
 *	DESCRIPTION:	we don't support overlays, so this code is
 *		really simple
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"

void glutEstablishOverlay() {
	__glutFatalError("BeOS lacks overlay support.");
}

void glutUseLayer(GLenum layer) {
	// ignore
}

void glutRemoveOverlay() {
	// ignore
}

void glutPostOverlayRedisplay() {
	// ignore
}

void glutShowOverlay() {
	// ignore
}

void glutHideOverlay() {
	// ignore
}