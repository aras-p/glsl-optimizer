/* **************************************************************************
 * ggiglut.c
 * **************************************************************************
 * 
 * Copyright (C) 1998 Uwe Maurer - uwe_maurer@t-online.de 
 * Copyright (C) 1999 James Simmons
 * Copyright (C) 1999 Jon Taylor
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
 * 
 * **************************************************************************
 * To-do:
 * - Make everything use the portable ggi_* types
 * 
 */

#define BUILDING_GGIGLUT

#define WIDTH	640
#define HEIGHT	480
#define GRAPHTYPE_RGB	GT_16BIT
#define GRAPHTYPE_INDEX	GT_8BIT

/*************************************************************************/

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "GL/ggimesa.h"
#include "debug.h"

#include <ggi/ggi.h>
#include <ggi/gii.h>

int _ggiglutDebugSync = 0;
uint32 _ggiglutDebugState = 0;

char *__glutProgramName = "GGI";

static ggi_visual_t __glut_vis;

static ggi_mesa_context_t __glut_ctx;

//static int __glut_width = WIDTH;
//static int __glut_height = HEIGHT;
//static ggi_graphtype __glut_gt_rgb = GRAPHTYPE_RGB;
//static ggi_graphtype __glut_gt_index = GRAPHTYPE_INDEX;
static int __glut_width = GGI_AUTO;
static int __glut_height = GGI_AUTO;
static ggi_graphtype __glut_gt_rgb = GT_AUTO;
static ggi_graphtype __glut_gt_index = GT_8BIT;
static int __glut_init = GL_FALSE;

static int mousex = WIDTH / 2;
static int mousey = HEIGHT / 2;
static int mouse_moved = GL_FALSE;
static int mouse_down = GL_FALSE;
static int mouse_showcursor = GL_FALSE;

static void (*__glut_reshape)(int, int);
static void (*__glut_display)(void);
static void (*__glut_idle)(void);
static void (*__glut_keyboard)(unsigned char, int, int);
static void (*__glut_special)(int, int, int);
static void (*__glut_mouse)(int, int, int, int);
static void (*__glut_motion)(int, int);
static void (*__glut_passive_motion)(int, int);
static void (*__glut_visibility)(int);

static unsigned int __glut_mode = GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;

static int __glut_mod_keys = 0;

static int __glut_redisplay = GL_FALSE;

/* Menu */
static int __glut_menubutton = -1;
static int __glut_menuactive = GL_FALSE;

#define MAX_ENTRIES	64

typedef struct menu_s
{
	char *label[MAX_ENTRIES];
	int value[MAX_ENTRIES];
	struct menu_s * submenu[MAX_ENTRIES];
	void (*func)(int);		
	int max_strlen;
	int num_entries;
} menu_t;

static menu_t *mainmenu;
static menu_t *curmenu;
static menu_t *activemenu;

void glutInit(int *argc, char **argv)
{
	ggi_graphtype gt;
	int i, k;
	char *str;
	
	GGIGLUTDPRINT_CORE("glutInit() called\n");

	#define REMOVE  {for (k=i;k<*argc-1;k++) argv[k]=argv[k+1];  \
				(*argc)--; i--; }

	if (__glut_init) return;
	
        str = getenv("GGIGLUT_DEBUG");
	if (str != NULL) {
		_ggiglutDebugState = atoi(str);
		fprintf(stderr, "Debugging=%d\n", _ggiglutDebugState);
		GGIGLUTDPRINT_CORE("Debugging=%d\n", _ggiglutDebugState);
	}
	
	str = getenv("GGIGLUT_DEBUGSYNC");
	if (str != NULL) {
		_ggiglutDebugSync = 1;
	}

	if (argc && argv)
	{

		for (i = 1; i < *argc; i++)
		{
			if (strcmp(argv[i], "-mouse") == 0)
			{
				mouse_showcursor = GL_TRUE;	
				REMOVE;
			}
			else
			if (strcmp(argv[i], "-bpp") == 0 && (i + 1) < (*argc))
			{
				switch(atoi(argv[i + 1]))
				{
					case 4: gt = GT_4BIT; break;
					case 8: gt = GT_8BIT; break;
					case 15: gt = GT_15BIT; break;
					case 16: gt = GT_16BIT; break;
					case 24: gt = GT_24BIT; break;
					case 32: gt = GT_32BIT; break;
					default:
					ggiPanic("\"%s\" bits per pixel?\n", argv[i+1]);
				}
				__glut_gt_rgb = __glut_gt_index = gt;
				REMOVE;
				REMOVE;
			} 
			else
			if (strcmp(argv[i], "-size") == 0 && (i + 2) < (*argc))
			{
				__glut_width = atoi(argv[i + 1]);
				__glut_height = atoi(argv[i + 2]);
				REMOVE;
				REMOVE;
				REMOVE;
			}
		}
	}
	
	if (ggiInit() < 0)
	{
		ggiPanic("ggiInit() failed!\n");
	}
	__glut_init = GL_TRUE;

#undef REMOVE
}

void glutInitWindowPosition(int x, int y)
{
	GGIGLUTDPRINT_CORE("glutInitWindowPosition() called\n");
}

void glutInitWindowSize(int x, int y)
{
	GGIGLUTDPRINT_CORE("glutInitWindowsSize() called\n");
}

void glutFullScreen(void)
{
	GGIGLUTDPRINT_CORE("glutFullScreen() called\n");
}

void glutInitDisplayMode(unsigned int mode)
{
	GGIGLUTDPRINT_CORE("glutInitDisplayMode() called\n");
	
	__glut_mode = mode;
}

void glutInitDisplayString(const char *string)
{
	GGIGLUTDPRINT_CORE("glutInitDisplayString(%s) called\n", string);
}

int glutCreateWindow(const char *title)
{
	ggi_graphtype gt;
	ggi_mode mode = 
	{
		1,
		{ GGI_AUTO, GGI_AUTO },
		{ GGI_AUTO, GGI_AUTO },
		{ 0, 0 },
		GT_AUTO,
		{ GGI_AUTO, GGI_AUTO }
	};
	int frames;
	int rgb;
	int err;

	GGIGLUTDPRINT_CORE("glutCreateWindow() called\n");
	
	if (!__glut_init) 
	  glutInit(NULL, NULL);

	GGIGLUTDPRINT("GGIGLUT: %s\n", title);

	rgb = !(__glut_mode & GLUT_INDEX);
	frames = (__glut_mode & GLUT_DOUBLE) ? 2 : 1;
	
	gt = (rgb) ? __glut_gt_rgb : __glut_gt_index;
	
	__glut_vis = ggiOpen(NULL);
	if (__glut_vis == NULL) 
	{
	  ggiPanic("ggiOpen() failed!\n");
	  /* return GL_FALSE; */
	}
	
	ggiSetFlags(__glut_vis, GGIFLAG_ASYNC);
	
	ggiCheckMode(__glut_vis, &mode);
	
	err = ggiSetMode(__glut_vis, &mode);
	if (err < 0) 
	{
	  ggiPanic("Can't set graphic mode!\n");
	  /* return GL_FALSE; */
	}
	
	if (ggiMesaExtendVisual(__glut_vis, GL_FALSE, GL_FALSE, 
	                        16, 0, 0, 0, 0, 0, 1) < 0) 
	{
		ggiPanic("GGIMesaSetVisual failed!\n");
	}
	
	__glut_ctx = ggiMesaCreateContext(__glut_vis);
	
	if (__glut_ctx == NULL) 
	  ggiPanic("Can't create mesa-context\n");

	ggiGetMode(__glut_vis, &mode);

	
	__glut_width = mode.visible.x;
	__glut_height = mode.visible.y;
	
	mousex = mode.visible.x / 2;
	mousey = mode.visible.y / 2;
	
	ggiMesaMakeCurrent(__glut_ctx, __glut_vis);
	
	if (__glut_reshape) 
	  __glut_reshape(__glut_width, __glut_height);
	
	return GL_TRUE;
}

void glutReshapeFunc(void (*func)(int, int))
{
	GGIGLUTDPRINT_CORE("glutReshapeFunc() called\n");
	
	__glut_reshape = func;
	if (__glut_vis && __glut_reshape) 
	  __glut_reshape(__glut_width, __glut_height);
}

void glutKeyboardFunc(void (*keyboard)(unsigned char key, int x, int y))
{
	GGIGLUTDPRINT_CORE("glutKeyBoardFunc() called\n");

	__glut_keyboard = keyboard;
}

int glutGetModifiers(void)
{
	GGIGLUTDPRINT_CORE("glutGetModifiers() called\n");

	return __glut_mod_keys;
}

void glutEntryFunc(void (*func)(int state))
{
	GGIGLUTDPRINT_CORE("glutEntryFunc() called\n");
}

void glutVisibilitFunc(void (*func)(int state))
{
	GGIGLUTDPRINT_CORE("glutVisibilityFunc() called\n");
}

void glutTimerFunc(unsigned int millis, void (*func)(int value), int value)
{
	GGIGLUTDPRINT_CORE("glutTimerFunc() called\n");
}

void glutMenuStateFunc(void (*func)(int state))
{
	GGIGLUTDPRINT_CORE("glutMenuStateFunc() called\n");
}

int glutGet(GLenum type)
{
	GGIGLUTDPRINT_CORE("glutGet() called\n");
	
	switch(type)
	{
		case GLUT_WINDOW_X:	
		return 0;
		case GLUT_WINDOW_Y:
		return 0;
		case GLUT_WINDOW_WIDTH:
		return __glut_width;
		case GLUT_WINDOW_HEIGHT:
		return __glut_height;
		case GLUT_MENU_NUM_ITEMS:
		if (curmenu) 
		  return curmenu->num_entries;
		else 
		  return 0;
		default:
		GGIGLUTDPRINT("glutGet: unknown type %i\n", type);
	}
	return 0;
}

void glutSpecialFunc(void (*special)(int key, int x, int y))
{
	GGIGLUTDPRINT_CORE("glutSpecialFunc() called\n");
	
	__glut_special=special;
}

void glutDisplayFunc(void (*disp)(void))
{
	GGIGLUTDPRINT_CORE("glutDisplayFunc() called\n");
	__glut_display=disp;
}

void glutSetColor(int index, GLfloat red, GLfloat green, GLfloat blue)
{
	ggi_color c;
	GLfloat max;
  
	GGIGLUTDPRINT_CORE("glutSetColor() called\n");
	
	if (red > 1.0) red = 1.0;
	if (red < 0.0) red = 0.0;
	if (green > 1.0) green = 1.0;
	if (green < 0.0) green = 0.0;
	if (blue > 1.0) blue = 1.0;
	if (blue < 0.0) blue = 0.0;
  
	max = (float)((1 << GGI_COLOR_PRECISION) - 1);
	
	c.r = (int)(max * red);
	c.g = (int)(max * green);
	c.b = (int)(max * blue);
	ggiSetPalette(__glut_vis, index, 1, &c);
}

void glutPostRedisplay(void)
{
	GGIGLUTDPRINT_CORE("glutPostRedisplay() called\n");

	__glut_redisplay = GL_TRUE;
}

void glutPostWindowRedisplay(int win)
{
	GGIGLUTDPRINT_CORE("glutPostWindowRedisplay() called\n");
	
	__glut_redisplay = GL_TRUE;
}

void glutSwapBuffers(void)
{
	GGIGLUTDPRINT_CORE("glutSwapBuffers() called\n");

	ggiMesaSwapBuffers();
}

void glutIdleFunc(void (*idle)(void))
{
	GGIGLUTDPRINT_CORE("glutIdleFunc() called\n");
	
	__glut_idle = idle;
}

static void keyboard(ggi_event *ev)
{
	int sym;
	int modifer = 0, key = 0;
  
	GGIGLUTDPRINT_CORE("keyboard() called\n");
	
	sym = ev->key.sym;
  
	modifer = ev->key.modifiers;
	if (modifer == GII_KM_SHIFT)
	  __glut_mod_keys |= GLUT_ACTIVE_SHIFT;
	if (modifer == GII_KM_CTRL)
	  __glut_mod_keys |= GLUT_ACTIVE_CTRL;
	if (modifer == GII_KM_ALT)
	  __glut_mod_keys |= GLUT_ACTIVE_ALT;

	/* if (__glut_special && key) __glut_special(GII_KTYP(key),0,0); */
  
	if (__glut_keyboard) 
//	  __glut_keyboard(GII_UNICODE(sym), 0, 0);
	  __glut_keyboard(sym, 0, 0);
}

static void mouseabs(ggi_event *ev)
{
	int oldx = mousex;
	int oldy = mousey;
  
	mousex = ev->pmove.x;
	mousey = ev->pmove.y;
  
	if (mousex < 0) mousex = 0;
	if (mousey < 0) mousey = 0;
	if (mousex >= __glut_width) mousex = __glut_width - 1;
	if (mousey >= __glut_height) mousey = __glut_height - 1;
	
	if (mousex != oldx || mousey != oldy) mouse_moved = GL_TRUE;
}

static void mouse(ggi_event *ev)
{
	int oldx = mousex;
	int oldy = mousey;
  
	GGIGLUTDPRINT_CORE("mouse() called\n");
	
	mousex += ev->pmove.x >> 1;
	mousey += ev->pmove.y >> 1;
	
	if (mousex < 0) mousex = 0;
	if (mousey < 0) mousey = 0;
	if (mousex >= __glut_width) mousex = __glut_width - 1;
	if (mousey >= __glut_height) mousey = __glut_height - 1;
	
	if (mousex != oldx || mousey != oldy) mouse_moved = GL_TRUE;
  
}

static void showmenu(void);
static int clickmenu(void);
static void updatemouse(void);
static void drawmouse(void);

static void mousemove(void)
{
	GGIGLUTDPRINT_CORE("mousemove() called\n");

	if (mouse_moved) {
		if (__glut_motion && mouse_down) {
			__glut_motion(mousex,mousey);
		}
		
		if (__glut_passive_motion && (!mouse_down)) {
			__glut_passive_motion(mousex,mousey);
		}
		
		if (__glut_menuactive) updatemouse();
		mouse_moved=GL_FALSE;
	}
}

static void button(ggi_event *ev)
{
	int i;
	int btn[4] = {
		0,
		GLUT_LEFT_BUTTON,
		GLUT_RIGHT_BUTTON,
		GLUT_MIDDLE_BUTTON
	};
	
	GGIGLUTDPRINT_CORE("button() called\n");
	
	mousemove();
	
	if (ev->pbutton.button <= 3) { /* GGI can have >3 buttons ! */ 
		char state = ev->any.type == evPtrButtonPress ? GLUT_DOWN : GLUT_UP;
		if (__glut_menuactive) {
			if (state == GLUT_UP && clickmenu()) {
				glutPostRedisplay();
				__glut_menuactive = GL_FALSE;
			}
		} else
		  if (btn[ev->pbutton.button] == __glut_menubutton) {
			  __glut_menuactive = GL_TRUE;
			  activemenu = mainmenu;
			  showmenu();
		  } else 
		  if (__glut_mouse) {
			  __glut_mouse(btn[ev->pbutton.button], state, mousex, mousey);
		  }
		if (state == GLUT_DOWN) {
			mouse_down |= (1 << (ev->pbutton.button - 1));
		}
		else mouse_down &= ~( 1 << (ev->pbutton.button - 1));
	}
}

void glutMainLoop(void)
{
	ggi_event ev;
	ggi_event_mask evmask = (emKeyPress | emKeyRepeat | emPtrMove | emPtrButton);

	GGIGLUTDPRINT_CORE("glutMainLoop() called\n");

	ggiSetEventMask(__glut_vis, evmask);

	glutPostRedisplay();
	
	if (__glut_visibility) 
	  __glut_visibility(GLUT_VISIBLE);
	
	while (1) 
	{
		if (!__glut_menuactive) 
		{
			if (__glut_idle) 
			  __glut_idle();
			if (__glut_redisplay && __glut_display) 
			{
				__glut_redisplay = GL_FALSE;
				__glut_display();
			}
		}
		
		while (1) 
		{
			struct timeval t = {0, 0};
			
			if (ggiEventPoll(__glut_vis, evmask, &t) == 0) 
			  break;
			
			ggiEventRead(__glut_vis, &ev, evmask);
      
			switch (ev.any.type) 
			{
				case evKeyPress:
				case evKeyRepeat:
				keyboard(&ev);
				break;
				case evPtrAbsolute:
				mouseabs(&ev);
				break;
				case evPtrRelative:
				mouse(&ev);
				break;
				case evPtrButtonPress:
				case evPtrButtonRelease:
				button(&ev);
				break;
			}
		}
		mousemove();
	}
}

static void showmenu()
{
	int y,i;
	ggi_color col = { 0xffff, 0xffff, 0xffff };
	
	GGIGLUTDPRINT_CORE("showmenu() called\n");
	
	ggiSetGCForeground(__glut_vis,ggiMapColor(__glut_vis,&col));
	ggiSetOrigin(__glut_vis,0,0);
	
	for (y = i = 0; i < activemenu->num_entries; i++, y += 8) {
		ggiPuts(__glut_vis, 0, y, activemenu->label[i]);
	}
	drawmouse();
}

static int clickmenu(void)
{
	int i;
	int w, h;
  
	GGIGLUTDPRINT_CORE("clickmenu() called\n");
	
	i = mousey / 8;
  
	if (i >= activemenu->num_entries) return GL_TRUE;
	if (mousex >= 8 * strlen(activemenu->label[i])) return GL_TRUE;
  
	if (activemenu->submenu[i]) {
		ggi_color col={0,0,0};
		ggiSetGCForeground(__glut_vis,ggiMapColor(__glut_vis,&col));
		h=activemenu->num_entries*8;
		w=activemenu->max_strlen*8;
		ggiDrawBox(__glut_vis,0,0,w,h);
		activemenu=activemenu->submenu[i];
		showmenu();
		return GL_FALSE;
	}
	curmenu=activemenu;
	activemenu->func(activemenu->value[i]);
	return GL_TRUE;
}

static int oldx=-1;
static int oldy=-1;
static char buffer[16*16*4];

static void updatemouse()
{
	GGIGLUTDPRINT_CORE("updatemouse() called\n");

	ggiPutBox(__glut_vis,oldx,oldy,16,16,buffer);
	drawmouse();
}

static void drawmouse()
{
	int x,y;
	
	GGIGLUTDPRINT_CORE("drawmouse() called\n");
	
	x=mousex-8;
	if (x<0) x=0;
	y=mousey-8;
	if (y<0) y=0;
	ggiGetBox(__glut_vis,x,y,16,16,buffer);
	ggiDrawLine(__glut_vis,mousex-2,mousey,mousex+2,mousey);
	ggiDrawLine(__glut_vis,mousex,mousey-2,mousex,mousey+2);
	oldx=x;
	oldy=y;
}

int glutCreateMenu(void(*func)(int))
{
	menu_t *m;
	
	GGIGLUTDPRINT_CORE("glutCreateMenu() called\n");
	
	m=malloc(sizeof(menu_t));
	memset(m,0,sizeof(menu_t));
	curmenu=m;
	curmenu->func=func;
	return (int)curmenu;
}

static void addEntry(const char *label,int value,menu_t *submenu)
{
	int i=curmenu->num_entries;
	
	GGIGLUTDPRINT_CORE("addEntry() called\n");
	
	/* printf("%i %i %s %p\n",i,value,label,submenu); */
	if (i<MAX_ENTRIES) {
		curmenu->label[i]=strdup(label);
		curmenu->value[i]=value;
		curmenu->submenu[i]=submenu;
    
		if (strlen(label)>curmenu->max_strlen)
		  curmenu->max_strlen=strlen(label);
		curmenu->num_entries++;
	}
}

void glutAddMenuEntry(const char *label,int value)
{
	GGIGLUTDPRINT_CORE("glutAddMenuEntry() called\n");

	addEntry(label,value,NULL);
}

void glutAddSubMenu(const char *label,int submenu)
{
	char text[100];
	
	GGIGLUTDPRINT_CORE("glutAddSubMenu() called\n");
	
	if (!curmenu) return;
	strncpy(text,label,98);
	text[98]=0;
	text[strlen(text)+1]=0;
	text[strlen(text)]='>';
	
	addEntry(text,0,(menu_t *) submenu);
}

void glutAttachMenu(int button)
{
	GGIGLUTDPRINT_CORE("glutAttachMenu() called\n");

	mainmenu=curmenu;
	__glut_menubutton=button;
}

void glutDetachMenu(int button)
{
	GGIGLUTDPRINT_CORE("glutDetachMenu() called\n");
}

void glutVisibilityFunc(void (*func)(int state))
{
	GGIGLUTDPRINT_CORE("glutVisibilityFunc() called\n");

	__glut_visibility=func;
}

void glutMouseFunc(void (*mouse)(int, int, int, int))
{
	GGIGLUTDPRINT_CORE("glutMouseFunc() called\n");
	
	__glut_mouse=mouse;	
}

void glutMotionFunc(void (*motion)(int,int))
{
	GGIGLUTDPRINT_CORE("glutMotionFunc() called\n");
	
	__glut_motion=motion;
}

void glutPassiveMotionFunc(void (*motion)(int,int))
{
	GGIGLUTDPRINT_CORE("glutPassiveMotionFunc() called\n");
	
	__glut_passive_motion=motion;
}

void glutSetWindowTitle(const char *title)
{
	GGIGLUTDPRINT_CORE("glutSetWindowTitle() called\n");
}

void glutSetIconTitle(const char *title)
{
	GGIGLUTDPRINT_CORE("glutSetIconTitle() called\n");
}

void glutChangeToMenuEntry(int item,const char *label,int value)
{
	GGIGLUTDPRINT_CORE("glutChangeToMenuEntry() called\n");

	if (item>0 && item<=curmenu->num_entries) {
		item--;
		free(curmenu->label[item]);
		curmenu->label[item]=strdup(label);
		curmenu->value[item]=value;
		curmenu->submenu[item]=NULL;
	}
}
void glutChangeToSubMenu(int item,const char *label,int submenu)
{
	GGIGLUTDPRINT_CORE("glutChengeToSubMenu() called\n");
	
	if (item>0 && item<=curmenu->num_entries) {
		item--;
		free(curmenu->label[item]);
		curmenu->label[item]=strdup(label);
		curmenu->value[item]=0;
		curmenu->submenu[item]=(menu_t *)submenu;
	}
}

void glutDestroyMenu(int menu)
{
	menu_t *m=(menu_t *)menu;
	int i;
	
	GGIGLUTDPRINT_CORE("glutDestroyMenu() called\n");
	
	for (i=0;i<m->num_entries;i++) {
		free(m->label[i]);
	}
	free(m);
}

int glutCreateSubWindow(int win,int x,int y,int w,int h)
{
	GGIGLUTDPRINT_CORE("glutCreateSubWindow() called\n");

	return 0;
}

void glutDestroyWindow(int win)
{
	GGIGLUTDPRINT_CORE("glutDestroyWindow() called\n");
}

int glutGetWindow(void)
{
	GGIGLUTDPRINT_CORE("glutGetWindow() called\n");
	
	return 0;
}

void glutSetWindow(int win)
{
	GGIGLUTDPRINT_CORE("glutSetWindow() called\n");
}

void glutPositionWindow(int x,int y)
{
	GGIGLUTDPRINT_CORE("glutPositionWindow() called\n");
}

void glutReshapeWindow(int x,int y)
{
	GGIGLUTDPRINT_CORE("glutReshapeWindow() called\n");
}

void glutPushWindow(void)
{
	GGIGLUTDPRINT_CORE("glutPushWindow() called\n");
}

void glutPopWindow(void)
{
	GGIGLUTDPRINT_CORE("glutPopWindow() called\n");
}

void glutIconifyWindow(void)
{
	GGIGLUTDPRINT_CORE("glutIconifyWindow() called\n");
}

void glutShowWindow()
{
	GGIGLUTDPRINT_CORE("glutShowWindow() called\n");
}

void glutHideWindow()
{
	GGIGLUTDPRINT_CORE("glutHideWindow() called\n");
}

void glutSetCursor(int cursor)
{
	GGIGLUTDPRINT_CORE("glutSetCursor() called\n");
}

void glutWarpPointer(int x,int y)
{
	GGIGLUTDPRINT_CORE("glutWarpPointer() called\n");
}

void glutEstablishOverlay(void)
{
	GGIGLUTDPRINT_CORE("glutEstablishOverlay() called\n");
}

void glutRemoveOverlay(void)
{
	GGIGLUTDPRINT_CORE("glutRemoveOverlay() called\n");
}

void glutUseLayer(GLenum layer)
{
	GGIGLUTDPRINT_CORE("glutUseLayer() called\n");
}

int glutLayerGet(GLenum type)
{
	GGIGLUTDPRINT_CORE("glutLayerGet() called\n");
	return 0;
}

void glutPostOverlayRedisplay(void)
{
	GGIGLUTDPRINT_CORE("glutPostOverlayRedisplay() called\n");
}

void glutPostWindowOverlayRedisplay(int w)
{
	GGIGLUTDPRINT_CORE("glutPostWindowOverlayRedisplay() called\n");
}

void glutShowOverlay(void)
{
	GGIGLUTDPRINT_CORE("glutShowOverlay() called\n");
}

void glutHideOverlay(void)
{
	GGIGLUTDPRINT_CORE("glutHideOverlay() called\n");
}

int glutGetMenu(void)
{
	GGIGLUTDPRINT_CORE("glutGetMenu() called\n");
	return 0;
}

void glutSetMenu(int menu)
{
	GGIGLUTDPRINT_CORE("glutSetMenu() called\n");
}

void glutRemoveMenuItem(int item)
{
	GGIGLUTDPRINT_CORE("glutRemoveMenuItem() called\n");
}

void glutSpaceBallMotionFunc(void (*func)(int key,int x,int y))
{
	GGIGLUTDPRINT_CORE("glutSpaceBallMotionFunc() called\n");
}

void glutSpaceBallRotateFunc(void (*func)(int x,int y,int z))
{
	GGIGLUTDPRINT_CORE("glutSpaceBallRotateFunc() called\n");
}

void glutSpaceBallButtonFunc(void (*func)(int button,int state))
{
	GGIGLUTDPRINT_CORE("glutSpaceBallButtonFunc() called\n");
}

void glutCopyColormap(int win)
{
	GGIGLUTDPRINT_CORE("glutCopyColormap() called\n");
}

int glutDeviceGet(GLenum param)
{
	GGIGLUTDPRINT_CORE("glutDeviceGet() called\n");
	
	return 0;
}
