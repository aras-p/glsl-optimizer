/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutWindow.h
 *
 *	DESCRIPTION:	the GlutWindow class saves all events for
 *		handling by main thread
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <Window.h>
#include <GLView.h>

/***********************************************************
 *	CLASS:	GlutWindow
 *
 *  INHERITS FROM:  BGLView (NOT BWindow!)
 *
 *  DESCRIPTION:	all information needed for windows and
 *			subwindows (handled as similarly as possible)
 ***********************************************************/
class GlutWindow : public BGLView {
public:
	GlutWindow(GlutWindow *nparent, char *name, int x, int y, int width,
				int height, ulong options);
	
	void KeyDown(const char *bytes, int32 numBytes);
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	void FrameResized(float width, float height);
	void Draw(BRect updateRect);
	void Hide();
	void Show();
	void Pulse();		// needed since MouseUp() is broken
	void MouseCheck();	// check for button state changes
	void ErrorCallback(GLenum errorCode);

	static long MenuThread(void *menu);
	
	int num;			// window number returned to user
	int cursor;			// my cursor
#define GLUT_MAX_MENUS              3
	int menu[GLUT_MAX_MENUS];	// my popup menus
	int m_width, m_height;		// the last width and height reported to GLUT
	uint32 m_buttons;			// the last mouse button state

	/* Window relationship state. */
  GlutWindow *parent;   /* parent window */
  GlutWindow *children; /* first child window */
  GlutWindow *siblings; /* next sibling */

	// leave out buttons and dials callbacks that we don't support
  GLUTdisplayCB display;  /* redraw  */
  GLUTreshapeCB reshape;  /* resize  (width,height) */
  GLUTmouseCB mouse;    /* mouse  (button,state,x,y) */
  GLUTmotionCB motion;  /* motion  (x,y) */
  GLUTpassiveCB passive;  /* passive motion  (x,y) */
  GLUTentryCB entry;    /* window entry/exit  (state) */
  GLUTkeyboardCB keyboard;  /* keyboard  (ASCII,x,y) */
  GLUTvisibilityCB visibility;  /* visibility  */
  GLUTspecialCB special;  /* special key  */

	bool anyevents;		// were any events received?
	bool displayEvent;		// call display
	bool reshapeEvent;		// call reshape
	bool mouseEvent;		// call mouse
	bool motionEvent;		// call motion
	bool passiveEvent;		// call passive
	bool entryEvent;		// call entry
	bool keybEvent;			// call keyboard
	bool visEvent;			// call visibility
	bool specialEvent;		// call special
	bool statusEvent;		// menu status changed
	bool menuEvent;			// menu selected
	
	bool swapHack;			// faked out single buffering
	
	int button, mouseState; // for mouse callback
	int mouseX, mouseY; // for mouse callback
	int motionX, motionY; // for motion callback
	int passiveX, passiveY; // for passive callback
	int entryState;  // for entry callback
	unsigned char key;  // for keyboard callback
	int keyX, keyY;  // for keyboard callback
	int visState;  // for visibility callback
	int specialKey; // for special key callback
	int specialX, specialY; // for special callback
	int modifierKeys;	// modifier key state
	int menuStatus;		// for status callback
	int statusX, statusY;	// for status callback
	int menuNumber;		// for menu and status callbacks
	int menuValue;		// for menu callback
};

/***********************************************************
 *	CLASS:	GlutBWindow
 *
 *  INHERITS FROM:	BWindow
 *
 *	DESCRIPTION:  basically a BWindow that won't quit
 ***********************************************************/
class GlutBWindow : public BWindow {
public:
	GlutBWindow(BRect frame, char *name);
	bool QuitRequested();	// exits app
};