/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutint.h
 *
 *	DESCRIPTION:	This is the new glut internal header file.
 *		most of the things formerly stored here are now in
 *		their own header files as C++ classes.
 ***********************************************************/

#include <SupportDefs.h>

/***********************************************************
 *	GLUT function types
 ***********************************************************/
typedef void (*GLUTdisplayCB) (void);
typedef void (*GLUTreshapeCB) (int, int);
typedef void (*GLUTkeyboardCB) (unsigned char, int, int);
typedef void (*GLUTmouseCB) (int, int, int, int);
typedef void (*GLUTmotionCB) (int, int);
typedef void (*GLUTpassiveCB) (int, int);
typedef void (*GLUTentryCB) (int);
typedef void (*GLUTvisibilityCB) (int);
typedef void (*GLUTwindowStatusCB) (int);
typedef void (*GLUTidleCB) (void);
typedef void (*GLUTtimerCB) (int);
typedef void (*GLUTmenuStateCB) (int);  /* DEPRICATED. 
                                                   */
typedef void (*GLUTmenuStatusCB) (int, int, int);
typedef void (*GLUTselectCB) (int);
typedef void (*GLUTspecialCB) (int, int, int);
typedef void (*GLUTspaceMotionCB) (int, int, int);
typedef void (*GLUTspaceRotateCB) (int, int, int);
typedef void (*GLUTspaceButtonCB) (int, int);
typedef void (*GLUTdialsCB) (int, int);
typedef void (*GLUTbuttonBoxCB) (int, int);
typedef void (*GLUTtabletMotionCB) (int, int);
typedef void (*GLUTtabletButtonCB) (int, int, int, int);

/***********************************************************
 *	Prototypes for glutInit.cpp
 ***********************************************************/
void __glutInitTime(bigtime_t *beginning);
void __glutInit();

/***********************************************************
 *	Prototypes for glut_util.c
 ***********************************************************/
void __glutWarning(char *format,...);
void __glutFatalError(char *format,...);
void __glutFatalUsage(char *format,...);

/***********************************************************
 *	Prototypes for glutMenu.cpp
 ***********************************************************/
class GlutMenu;		// avoid including glutMenu.h
GlutMenu *__glutGetMenuByNum(int menunum);
 
/***********************************************************
 *	Prototypes for glutWindow.cpp
 ***********************************************************/
int __glutConvertDisplayMode(unsigned long *options);
void __glutDefaultReshape(int width, int height);
class GlutWindow;	// avoid including glutWindow.h in every source file
void __glutSetWindow(GlutWindow * window);

/***********************************************************
 *	Prototypes for glutDstr.cpp
 ***********************************************************/
int __glutConvertDisplayModeFromString(unsigned long *options);

/***********************************************************
 *	Prototypes for glutCursor.cpp
 ***********************************************************/
void __glutSetCursor(int cursor);
