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

#define WIDTH	640
#define HEIGHT	480
#define GRAPHTYPE_RGB	GT_16BIT
#define GRAPHTYPE_INDEX	GT_8BIT

/*************************************************************************/

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "GL/ggimesa.h"

#include <ggi/ggi.h>
#include <ggi/gii.h>

char *__glutProgramName = "GGI";

static ggi_visual_t __glut_vis;

static GGIMesaContext __glut_ctx;

static int __glut_width = WIDTH;
static int __glut_height = HEIGHT;
static ggi_graphtype __glut_gt_rgb = GRAPHTYPE_RGB;
static ggi_graphtype __glut_gt_index = GRAPHTYPE_INDEX;
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

int glut_ggi_debug = GL_FALSE;

void glut_ggiPRINT(char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "GGIGLUT: ");
	vfprintf(stderr, format, args);
	va_end(args);
}

void glut_ggiDEBUG(char *format, ...)
{
	va_list args;
	if (glut_ggi_debug)
	{
		va_start(args, format);
		fprintf(stderr, "GGIGLUT: ");
		vfprintf(stderr, format, args);
		va_end(args);
	}
	
}

void glutInit(int *argc, char **argv)
{
	ggi_graphtype gt;
	int i, k;
	char *s;

	#define REMOVE  {for (k=i;k<*argc-1;k++) argv[k]=argv[k+1];  \
				(*argc)--; i--; }

	if (__glut_init) return;
	
	s = getenv("GGIGLUT_DEBUG");
	glut_ggi_debug = (s && atoi(s));

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
					glut_ggiDEBUG("\"%s\" bits per pixel?\n",
						      argv[i+1]);

					exit(1);
				}
				__glut_gt_rgb = __glut_gt_index = gt;
				REMOVE;
				REMOVE;
			} 
			else
			if (strcmp(argv[i], "-size") == 0 && (i + 2) < (*argc))
			{
				__glut_width=atoi(argv[i+1]);
				__glut_height=atoi(argv[i+2]);
				REMOVE;
				REMOVE;
				REMOVE;
			}
		}
	}
	
	if (ggiInit()<0)
	{
		ggiPanic("ggiInit() failed!\n");
	}
	__glut_init=GL_TRUE;

	#undef REMOVE
}

void glutInitWindowPosition(int x, int y)
{
}

void glutInitWindowSize(int x, int y)
{
}

void glutFullScreen(void)
{
}

void glutInitDisplayMode(unsigned int mode)
{
	__glut_mode = mode;
}

void glutInitDisplayString(const char *string)
{
	glut_ggiPRINT("glutInitDisplayString: %s\n", string);
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

	if (!__glut_init) 
	  glutInit(NULL, NULL);

	glut_ggiPRINT("GGIGLUT: %s\n", title);

	rgb = !(__glut_mode & GLUT_INDEX);
	frames = (__glut_mode & GLUT_DOUBLE) ? 2 : 1;
	gt = (rgb) ? __glut_gt_rgb : __glut_gt_index;
	
	__glut_ctx = GGIMesaCreateContext();
	
	if (__glut_ctx == NULL) 
	  ggiPanic("Can't create mesa-context\n");

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
	
	ggiGetMode(__glut_vis, &mode);

	if (GGIMesaSetVisual(__glut_ctx, __glut_vis, rgb, frames > 1) < 0) 
	{
		ggiPanic("GGIMesaSetVisual failed!\n");
	}
	
	GGIMesaMakeCurrent(__glut_ctx);
	
	if (__glut_reshape) 
	  __glut_reshape(__glut_width, __glut_height);

	return GL_TRUE;
}

void glutReshapeFunc(void (*func)(int, int))
{
  __glut_reshape = func;
  if (__glut_vis && __glut_reshape) 
    __glut_reshape(__glut_width, __glut_height);
}

void glutKeyboardFunc(void (*keyboard)(unsigned char key, int x, int y))
{
  __glut_keyboard = keyboard;
}

int glutGetModifiers(void)
{
  return __glut_mod_keys;
}

void glutEntryFunc(void (*func)(int state))
{
}

void glutVisibilitFunc(void (*func)(int state))
{
}

void glutTimerFunc(unsigned int millis, void (*func)(int value), int value)
{
}

void glutMenuStateFunc(void (*func)(int state))
{
}

int glutGet(GLenum type)
{
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
		glut_ggiDEBUG("glutGet: unknown type %i\n", type);
	}
	return 0;
}

void glutSpecialFunc(void (*special)(int key, int x, int y))
{
  __glut_special=special;
}

void glutDisplayFunc(void (*disp)(void))
{
  __glut_display=disp;
}

void glutSetColor(int index, GLfloat red, GLfloat green, GLfloat blue)
{
  ggi_color c;
  GLfloat max;
  
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
  __glut_redisplay = GL_TRUE;
}

void glutPostWindowRedisplay(int win)
{
  __glut_redisplay = GL_TRUE;
}

void glutSwapBuffers(void)
{
  GGIMesaSwapBuffers();
}

void glutIdleFunc(void (*idle)(void))
{
  __glut_idle = idle;
}

static void keyboard(ggi_event *ev)
{
	int sym;
	int modifer = 0, key = 0;
  
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
  int oldx=mousex;
  int oldy=mousey;
  
  mousex=ev->pmove.x;
  mousey=ev->pmove.y;
  
  if (mousex<0) mousex=0;
  if (mousey<0) mousey=0;
  if (mousex>=__glut_width) mousex=__glut_width-1;
  if (mousey>=__glut_height) mousey=__glut_height-1;
  
  if (mousex!=oldx || mousey!=oldy) mouse_moved=GL_TRUE;
}

static void mouse(ggi_event *ev)
{
  int oldx=mousex;
  int oldy=mousey;
  
  mousex+=ev->pmove.x>>1;
  mousey+=ev->pmove.y>>1;
  
  if (mousex<0) mousex=0;
  if (mousey<0) mousey=0;
  if (mousex>=__glut_width) mousex=__glut_width-1;
  if (mousey>=__glut_height) mousey=__glut_height-1;
  
  if (mousex!=oldx || mousey!=oldy) mouse_moved=GL_TRUE;
  
}

/* FIXME: Prototypes belong in headers, not here! [JMT] */
static void showmenu(void);
static int clickmenu(void);
static void updatemouse(void);
static void drawmouse(void);

static void mousemove(void)
{
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
  int btn[4]={0,GLUT_LEFT_BUTTON,GLUT_RIGHT_BUTTON,GLUT_MIDDLE_BUTTON};
  mousemove();
  
  if (ev->pbutton.button <= 3) { /* GGI can have >3 buttons ! */ 
    char state = ev->any.type == evPtrButtonPress ?GLUT_DOWN:GLUT_UP;
    if (__glut_menuactive) {
      if (state==GLUT_UP && clickmenu()) {
	glutPostRedisplay();
	__glut_menuactive=GL_FALSE;
      }
    } else
      if (btn[ev->pbutton.button]==__glut_menubutton) {
	__glut_menuactive=GL_TRUE;
	activemenu=mainmenu;
	showmenu();
      } else 
	if (__glut_mouse) {
	  __glut_mouse(btn[ev->pbutton.button],state,mousex,mousey);
	}
    if (state==GLUT_DOWN) {
      mouse_down|=(1<<(ev->pbutton.button-1));
    }
    else mouse_down&=~(1<<(ev->pbutton.button-1));
  }
}

void glutMainLoop(void)
{
	ggi_event ev;
	ggi_event_mask evmask = (emKeyPress | emKeyRepeat | emPtrMove | emPtrButton);
  
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
  ggi_color col={0xffff,0xffff,0xffff};
  
  ggiSetGCForeground(__glut_vis,ggiMapColor(__glut_vis,&col));
  ggiSetOrigin(__glut_vis,0,0);
  
  for (y=i=0;i<activemenu->num_entries;i++,y+=8) {
    ggiPuts(__glut_vis,0,y,activemenu->label[i]);
  }
  drawmouse();
}

static int clickmenu(void)
{
  int i;
  int w,h;
  
  i=mousey/8;
  
  if (i>=activemenu->num_entries) return GL_TRUE;
  if (mousex>=8*strlen(activemenu->label[i])) return GL_TRUE;
  
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
  ggiPutBox(__glut_vis,oldx,oldy,16,16,buffer);
  drawmouse();
}

static void drawmouse()
{
  int x,y;
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
  m=malloc(sizeof(menu_t));
  memset(m,0,sizeof(menu_t));
  curmenu=m;
  curmenu->func=func;
  return (int)curmenu;	
}

static void addEntry(const char *label,int value,menu_t *submenu)
{
  int i=curmenu->num_entries;	
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
  addEntry(label,value,NULL);
}

void glutAddSubMenu(const char *label,int submenu)
{
  char text[100];
  
  if (!curmenu) return;
  strncpy(text,label,98);
  text[98]=0;
  text[strlen(text)+1]=0;
  text[strlen(text)]='>';
  
  addEntry(text,0,(menu_t *) submenu);
}

void glutAttachMenu(int button)
{
  mainmenu=curmenu;
  __glut_menubutton=button;
}

void glutDetachMenu(int button)
{
}

void glutVisibilityFunc(void (*func)(int state))
{
  __glut_visibility=func;
}

void glutMouseFunc(void (*mouse)(int, int, int, int))
{
  __glut_mouse=mouse;	
}

void glutMotionFunc(void (*motion)(int,int))
{
  __glut_motion=motion;
}

void glutPassiveMotionFunc(void (*motion)(int,int))
{
  __glut_passive_motion=motion;
}

void glutSetWindowTitle(const char *title)
{
}

void glutSetIconTitle(const char *title)
{
}

void glutChangeToMenuEntry(int item,const char *label,int value)
{
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
  
  for (i=0;i<m->num_entries;i++) {
    free(m->label[i]);
  }
  free(m);
}

int glutCreateSubWindow(int win,int x,int y,int w,int h)
{
  return 0;
}

void glutDestroyWindow(int win)
{
}

int glutGetWindow(void)
{
  return 0;
}

void glutSetWindow(int win)
{
}

void glutPositionWindow(int x,int y)
{
}

void glutReshapeWindow(int x,int y)
{
}

void glutPushWindow(void)
{
}

void glutPopWindow(void)
{
}

void glutIconifyWindow(void)
{
}

void glutShowWindow()
{
}

void glutHideWindow()
{
}

void glutSetCursor(int cursor)
{
}

void glutWarpPointer(int x,int y)
{
}

void glutEstablishOverlay(void)
{
}

void glutRemoveOverlay(void)
{
}

void glutUseLayer(GLenum layer)
{
}

int glutLayerGet(GLenum type)
{
	return 0;
}

void glutPostOverlayRedisplay(void)
{
}

void glutPostWindowOverlayRedisplay(int w)
{
}

void glutShowOverlay(void)
{
}

void glutHideOverlay(void)
{
}

int glutGetMenu(void)
{
	return 0;
}

void glutSetMenu(int menu)
{
}

void glutRemoveMenuItem(int item)
{
}

void glutSpaceBallMotionFunc(void (*func)(int key,int x,int y))
{
}

void glutSpaceBallRotateFunc(void (*func)(int x,int y,int z))
{
}

void glutSpaceBallButtonFunc(void (*func)(int button,int state))
{
}

void glutCopyColormap(int win)
{
}

int glutDeviceGet(GLenum param)
{
	return 0;
}
