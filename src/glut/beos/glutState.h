/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutState.h
 *
 *	DESCRIPTION:	the global state for GLUT
 *		(takes the place of glutint.h in the C version)
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <Application.h>
#include "glutWindow.h"
#include "glutMenu.h"

/***********************************************************
 *	CLASS:	GlutState
 *
 *	DESCRIPTION:	all the global state variables
 ***********************************************************/
struct GlutState {
	BApplication *display;
	char *programName;			// used in error messages
	int initX, initY;			// initial window position
	int initWidth, initHeight;	// initial window size
	unsigned int displayMode;	// initial display mode
	char *displayString;		// verbose display mode

	GlutWindow *currentWindow;	// current window
	GlutMenu *currentMenu;		// current menu
	
	GlutWindow **windowList;	// array of pointers to windows
	int windowListSize;			// size of window list
	
	GLUTidleCB idle;				// idle callback
	GLUTmenuStatusCB menuStatus;	// menu status callback
	int modifierKeys;				// only valid during keyboard callback
	
	bool debug;					// call glGetError
	bool swapHack;
	
	GlutState() {
		display = 0;
		programName = 0;
		initX = initY = -1;
		initWidth = initHeight = 300;
		displayMode = GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;
		displayString = 0;
		currentWindow = 0;
		currentMenu = 0;
		windowList = 0;
		windowListSize = 0;
		idle = 0;
		menuStatus = 0;
		modifierKeys = ~0;
		debug = swapHack = false;
	}
};

/***********************************************************
 *	Global variable (declared in glutInit.cpp)
 ***********************************************************/
extern GlutState gState;