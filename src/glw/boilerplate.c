/*

                                BOILERPLATE

   To get started with mixed model programming with Motif and OpenGL, this
   boilerplate `application' might help get you started.

   This program honors two environment variables:

      SETVISUAL <id>    Makes the application use the indicated visual,
                        instead of the one chosen by glxChooseVisual.

      SAMEVISUAL        Make the application use the same visual for the
                        GUI as for the 3D GL Widget.

   The basic idea is to minimize colormap `flash' on systems with only one
   hardware colormap, especially when focus shifts between several
   of the application's windows, e.g. the about box.

   If you have suggestions for improvements, please mail to:


      Jeroen van der Zijp  <jvz@cyberia.cfdrc.com>


   Feel free to turn this into a useful program!!

*/


/*

    This code is hereby placed under GNU GENERAL PUBLIC LICENSE.
    Copyright (C) 1996 Jeroen van der Zijp <jvz@cyberia.cfdrc.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/* Include the kitchen sink */
#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <Xm/BulletinB.h>
#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/MwmUtil.h>

/* Now some good stuff */
#include <GLwMDrawA.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>


/* Stuff */
Display      *display;
Visual       *gui_visual;
Colormap      gui_colormap;
Colormap      gl_colormap;
XVisualInfo  *gl_visualinfo;
XtAppContext  app_context;
Widget        toplevel;
Widget        mainwindow;
Widget        menubar;
Widget        mainform;
Widget        mainframe;
Widget        glwidget;
Widget        button;
Widget        separator;
GLXContext    glx_context;

#ifndef __cplusplus
#define c_class class
#endif

/* Requested attributes; fix as you see fit */
static int glxConfig[]={
  GLX_RGBA,
  GLX_DOUBLEBUFFER,
  GLX_DEPTH_SIZE,   16,
  GLX_RED_SIZE,     1,
  GLX_GREEN_SIZE,   1,
  GLX_BLUE_SIZE,    1,
  None
  };


/* Forwards */
static void exposeCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs);
static void initCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs);
static void resizeCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs);
static void inputCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs);
static void byeCB(Widget w,XtPointer client_data,XtPointer call_data);
static void aboutCB(Widget w,XtPointer client_data,XtPointer call_data);
static char* showvisualclass(int cls);



/* Sample application */
int main(int argc, char *argv[]){
  char *thevisual;
  XVisualInfo vi;
  int nvisinfos,visid,n;
  Arg args[30];
  Widget pane,cascade,but;

  /*
  ** Initialize toolkit
  ** We do *not* use XtAppInitialize as we want to figure visual and
  ** colormap BEFORE we create the top level shell!!!
  */
  XtToolkitInitialize();

  /* Make application context */
  app_context=XtCreateApplicationContext();

  /* Try open display */
  display=XtOpenDisplay(app_context,NULL,"boilerPlate","BoilerPlate",NULL,0,&argc,argv);

  /* Report failure */
  if(!display){
    fprintf(stderr,"Unable to open the specified display.\n");
    fprintf(stderr,"Set your `DISPLAY' environment variable properly or\n");
    fprintf(stderr,"use the `xhost' command to authorize access to the display.\n");
    exit(1);
    }

  /* Check for extension; for Mesa, this is always cool */
  if(!glXQueryExtension(display,NULL,NULL)){
    fprintf(stderr,"The specified display does not support the OpenGL extension\n");
    exit(1);
    }

  /* Init with default visual and colormap */
  gui_visual=DefaultVisual(display,0);
  gui_colormap=DefaultColormap(display,0);
  gl_colormap=DefaultColormap(display,0);

  /* User insists on a specific visual */
  if((thevisual=getenv("SETVISUAL"))!=NULL){
    if(sscanf(thevisual,"%x",&visid)==1){
      vi.visualid=visid;
      gl_visualinfo=XGetVisualInfo(display,VisualIDMask,&vi,&nvisinfos);
      }
    else{
      fprintf(stderr,"Please set the `SETVISUAL' variable in hexadecimal\n");
      fprintf(stderr,"Use one of the Visual ID's reported by `xdpyinfo'\n");
      exit(1);
      }
    }

  /* Find visual the regular way */
  else{
    gl_visualinfo=glXChooseVisual(display,DefaultScreen(display),glxConfig);
    }

  /* Make sure we have a visual */
  if(!gl_visualinfo){
    fprintf(stderr,"Unable to obtain visual for graphics\n");
    exit(1);
    }

  /* Show what visual is being used */
  fprintf(stderr,"Using the following visual:\n");
  fprintf(stderr,"  visualid: %lx\n",gl_visualinfo->visualid);
  fprintf(stderr,"     depth: %d\n",gl_visualinfo->depth);
  fprintf(stderr,"    screen: %d\n",gl_visualinfo->screen);
  fprintf(stderr,"  bits/rgb: %d\n",gl_visualinfo->bits_per_rgb);
  fprintf(stderr,"     class: %s\n",showvisualclass(gl_visualinfo->c_class));

  /*
  ** If not using default visual, we need a colormap for this visual.
  ** Yes, the GL widget can allocate one itself, but we want to make
  ** sure the GUI and the 3D have the same one (if hardware does not
  ** allow more than one simultaneously).
  ** This prevents nasty flashing when the window with the 3D widget
  ** looses the focus.
  */
  if(gl_visualinfo->visual!=DefaultVisual(display,0)){
    fprintf(stderr,"Making another colormap\n");
    gl_colormap=XCreateColormap(display,
                                RootWindow(display,0),
                                gl_visualinfo->visual,
                                AllocNone);
    if(!gl_colormap){
      fprintf(stderr,"Unable to create private colormap\n");
      exit(1);
      }
    }

  /*
  ** Use common visual for GUI and GL?
  ** Maybe you can invoke some hardware interrogation function and
  ** see if more than one hardware map is supported.  For the purpose
  ** of this demo, we'll use an environment variable instead.
  */
  if(getenv("SAMEVISUAL")!=NULL){
    gui_visual=gl_visualinfo->visual;
    gui_colormap=gl_colormap;
    }

  fprintf(stderr,"GUI uses visual: %lx\n",XVisualIDFromVisual(gui_visual));

  /* Create application shell, finally */
  n=0;
  XtSetArg(args[n],XmNvisual,gui_visual); n++;  	/* Plug in that visual */
  XtSetArg(args[n],XmNcolormap,gui_colormap); n++;	/* And that colormap */
  toplevel=XtAppCreateShell("boilerPlate","BoilerPlate",
                            applicationShellWidgetClass,display,args,n);


  /* Main window */
  n=0;
  mainwindow=XmCreateMainWindow(toplevel,"window",args,n);
  XtManageChild(mainwindow);

  /* Make a menu */
  n=0;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  menubar=XmCreateMenuBar(mainwindow,"menubar",args,2);
  XtManageChild(menubar);

  n=0;
  pane=XmCreatePulldownMenu(menubar,"pane",args,n);
  n=0;
  but=XmCreatePushButton(pane,"Open",args,n);
  XtManageChild(but);
  but=XmCreatePushButton(pane,"Save",args,n);
  XtManageChild(but);
  but=XmCreatePushButton(pane,"Save As",args,n);
  XtManageChild(but);
  but=XmCreatePushButton(pane,"Quit",args,n);
  XtAddCallback(but,XmNactivateCallback,byeCB,(XtPointer)NULL);
  XtManageChild(but);
  XtSetArg(args[0],XmNsubMenuId,pane);
  cascade=XmCreateCascadeButton(menubar,"File",args,1);
  XtManageChild(cascade);

  n=0;
  pane=XmCreatePulldownMenu(menubar,"pane",args,n);
  n=0;
  but=XmCreatePushButton(pane,"About",args,n);
  XtAddCallback(but,XmNactivateCallback,aboutCB,(XtPointer)NULL);
  XtManageChild(but);
  XtSetArg(args[0],XmNsubMenuId,pane);
  cascade=XmCreateCascadeButton(menubar,"Help",args,1);
  XtManageChild(cascade);
  XtVaSetValues(menubar,XmNmenuHelpWidget,cascade,NULL);

  /* Main window form */
  n=0;
  XtSetArg(args[n],XmNmarginWidth,5); n++;
  XtSetArg(args[n],XmNmarginHeight,5); n++;
  mainform=XmCreateForm(mainwindow,"mainForm",args,n);
  XtManageChild(mainform);

  /* Some nice button */
  n=0;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
  button=XmCreatePushButton(mainform,"Bye",args,n);
  XtAddCallback(button,XmNactivateCallback,byeCB,(XtPointer)NULL);
  XtManageChild(button);

  n=0;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNbottomWidget,button); n++;
  XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
  separator=XmCreateSeparator(mainform,"separator",args,n);
  XtManageChild(separator);

  /* Main window frame */
  n = 0;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNbottomWidget,separator); n++;
  XtSetArg(args[n],XmNshadowType,XmSHADOW_IN); n++;
  mainframe = XmCreateFrame(mainform,"mainFrame",args,n);
  XtManageChild(mainframe);

  /* GL drawing area */
  n = 0;
  XtSetArg(args[n],XmNcolormap,gl_colormap); n++;
  XtSetArg(args[n],GLwNvisualInfo,gl_visualinfo); n++;
  XtSetArg(args[n],GLwNinstallColormap,True); n++;
  XtSetArg(args[n],XmNtraversalOn,True); n++;
  XtSetArg(args[n],XmNwidth,400); n++;
  XtSetArg(args[n],XmNheight,300); n++;
  glwidget = GLwCreateMDrawingArea(mainframe,"glWidget",args,n);
  XtAddCallback(glwidget,GLwNexposeCallback,(XtCallbackProc)exposeCB,(XtPointer)NULL);
  XtAddCallback(glwidget,GLwNresizeCallback,(XtCallbackProc)resizeCB,(XtPointer)NULL);
  XtAddCallback(glwidget,GLwNginitCallback,(XtCallbackProc)initCB,(XtPointer)NULL);
  XtAddCallback(glwidget,GLwNinputCallback,(XtCallbackProc)inputCB,(XtPointer)NULL);
  XtManageChild(glwidget);

  /* Set into main window */
  XmMainWindowSetAreas(mainwindow,menubar,NULL,NULL,NULL,mainform);
  XtRealizeWidget(toplevel);

  /* Loop until were done */
  XtAppMainLoop(app_context);
  return 0;
  }


/* Show visual class */
static char* showvisualclass(int cls){
  if(cls==TrueColor) return "TrueColor";
  if(cls==DirectColor) return "DirectColor";
  if(cls==PseudoColor) return "PseudoColor";
  if(cls==StaticColor) return "StaticColor";
  if(cls==GrayScale) return "GrayScale";
  if(cls==StaticGray) return "StaticGray";
  return "Unknown";
  }


static void exposeCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs){
  GLwDrawingAreaMakeCurrent(glwidget,glx_context);

  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);

  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,0.0,1.0);

  glMatrixMode(GL_MODELVIEW);

  glLoadIdentity();

  glColor3f(1.0,0.0,0.0);
  glBegin(GL_LINE_STRIP);
  glVertex3f(-1.0,-1.0,0.0);
  glVertex3f( 1.0, 1.0,0.0);
  glEnd();
  glXSwapBuffers(display,XtWindow(glwidget));
  }


/* Initialize widget */
static void initCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs){

  /* First, create context. We prefer direct rendering */
  glx_context=glXCreateContext(display,gl_visualinfo,0,TRUE);
  if(!glx_context){
    fprintf(stderr,"Unable to create gl context\n");
    exit(1);
    }

  /* Make it current */
  GLwDrawingAreaMakeCurrent(glwidget,glx_context);

  /* Set a viewport */
  glViewport(0,0,cbs->width,cbs->height);

  /* You might want to do a lot more here ... */
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glClearColor(1.0,1.0,1.0,1.0);
  glClearDepth(1.0);


  }


/* Widget changed size, so adjust txform */
static void resizeCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs){
  GLwDrawingAreaMakeCurrent(glwidget,glx_context);
  glViewport(0,0,cbs->width,cbs->height);

  /* blablabla */
  }


/* Boilerplate event handling */
static void inputCB(Widget w,XtPointer client_data,GLwDrawingAreaCallbackStruct *cbs){
  switch(cbs->event->type){
    case ButtonPress:
      switch(cbs->event->xbutton.button){
        case Button1: fprintf(stderr,"Pressed 1\n"); break;
        case Button2: fprintf(stderr,"Pressed 2\n"); break;
        case Button3: fprintf(stderr,"Pressed 3\n"); break;
        }
      break;
    case ButtonRelease:
      switch(cbs->event->xbutton.button){
        case Button1: fprintf(stderr,"Released 1\n"); break;
        case Button2: fprintf(stderr,"Released 2\n"); break;
        case Button3: fprintf(stderr,"Released 3\n"); break;
        }
      break;
    case MotionNotify:
      fprintf(stderr,"Moved mouse to (%d %d)\n",
              cbs->event->xbutton.x,
              cbs->event->xbutton.y);
      break;
    }
  }


/* Hasta la vista, baby */
static void byeCB(Widget w,XtPointer client_data,XtPointer call_data){
  exit(0);
  }


/* Pop informative panel */
static void aboutCB(Widget w,XtPointer client_data,XtPointer call_data){
  Arg args[10];
  XmString str;
  Widget box;
  str=XmStringCreateLtoR("Boilerplate Mixed Model Programming Example\n\n   (C) 1996 Jeroen van der Zijp \n\n    jvz@cyberia.cfdrc.com",XmSTRING_DEFAULT_CHARSET);
  XtSetArg(args[0],XmNnoResize,True);
  XtSetArg(args[1],XmNautoUnmanage,True);
  XtSetArg(args[2],XmNmessageString,str);
  XtSetArg(args[3],XmNdefaultPosition,False);
  box=XmCreateInformationDialog(toplevel,"About Boilerplate",args,4);
  XtManageChild(box);
  XtUnmanageChild(XmMessageBoxGetChild(box,XmDIALOG_HELP_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(box,XmDIALOG_CANCEL_BUTTON));
  XmStringFree(str);
  }
