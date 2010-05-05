#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "eglutint.h"

static struct eglut_state _eglut_state = {
   .api_mask = EGLUT_OPENGL_ES1_BIT,
   .window_width = 300,
   .window_height = 300,
   .verbose = 0,
   .num_windows = 0,
};

struct eglut_state *_eglut = &_eglut_state;

void
_eglutFatal(char *format, ...)
{
  va_list args;

  va_start(args, format);

  fprintf(stderr, "EGLUT: ");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);

  exit(1);
}

/* return current time (in milliseconds) */
int
_eglutNow(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void
_eglutDestroyWindow(struct eglut_window *win)
{
   if (_eglut->surface_type != EGL_PBUFFER_BIT &&
       _eglut->surface_type != EGL_SCREEN_BIT_MESA)
      eglDestroySurface(_eglut->dpy, win->surface);

   _eglutNativeFiniWindow(win);

   eglDestroyContext(_eglut->dpy, win->context);
}

static EGLConfig
_eglutChooseConfig(void)
{
   EGLConfig config;
   EGLint config_attribs[32];
   EGLint renderable_type, num_configs, i;

   i = 0;
   config_attribs[i++] = EGL_RED_SIZE;
   config_attribs[i++] = 1;
   config_attribs[i++] = EGL_GREEN_SIZE;
   config_attribs[i++] = 1;
   config_attribs[i++] = EGL_BLUE_SIZE;
   config_attribs[i++] = 1;
   config_attribs[i++] = EGL_DEPTH_SIZE;
   config_attribs[i++] = 1;

   config_attribs[i++] = EGL_SURFACE_TYPE;
   config_attribs[i++] = _eglut->surface_type;

   config_attribs[i++] = EGL_RENDERABLE_TYPE;
   renderable_type = 0x0;
   if (_eglut->api_mask & EGLUT_OPENGL_BIT)
      renderable_type |= EGL_OPENGL_BIT;
   if (_eglut->api_mask & EGLUT_OPENGL_ES1_BIT)
      renderable_type |= EGL_OPENGL_ES_BIT;
   if (_eglut->api_mask & EGLUT_OPENGL_ES2_BIT)
      renderable_type |= EGL_OPENGL_ES2_BIT;
   if (_eglut->api_mask & EGLUT_OPENVG_BIT)
      renderable_type |= EGL_OPENVG_BIT;
   config_attribs[i++] = renderable_type;

   config_attribs[i] = EGL_NONE;

   if (!eglChooseConfig(_eglut->dpy,
            config_attribs, &config, 1, &num_configs) || !num_configs)
      _eglutFatal("failed to choose a config");

   return config;
}

static struct eglut_window *
_eglutCreateWindow(const char *title, int x, int y, int w, int h)
{
   struct eglut_window *win;
   EGLint context_attribs[4];
   EGLint api, i;

   win = calloc(1, sizeof(*win));
   if (!win)
      _eglutFatal("failed to allocate window");

   win->config = _eglutChooseConfig();

   i = 0;
   context_attribs[i] = EGL_NONE;

   /* multiple APIs? */

   api = EGL_OPENGL_ES_API;
   if (_eglut->api_mask & EGLUT_OPENGL_BIT) {
      api = EGL_OPENGL_API;
   }
   else if (_eglut->api_mask & EGLUT_OPENVG_BIT) {
      api = EGL_OPENVG_API;
   }
   else if (_eglut->api_mask & EGLUT_OPENGL_ES2_BIT) {
      context_attribs[i++] = EGL_CONTEXT_CLIENT_VERSION;
      context_attribs[i++] = 2;
   }

   context_attribs[i] = EGL_NONE;

   eglBindAPI(api);
   win->context = eglCreateContext(_eglut->dpy,
         win->config, EGL_NO_CONTEXT, context_attribs);
   if (!win->context)
      _eglutFatal("failed to create context");

   _eglutNativeInitWindow(win, title, x, y, w, h);
   switch (_eglut->surface_type) {
   case EGL_WINDOW_BIT:
      win->surface = eglCreateWindowSurface(_eglut->dpy,
            win->config, win->native.u.window, NULL);
      break;
   case EGL_PIXMAP_BIT:
      win->surface = eglCreatePixmapSurface(_eglut->dpy,
            win->config, win->native.u.pixmap, NULL);
      break;
   case EGL_PBUFFER_BIT:
   case EGL_SCREEN_BIT_MESA:
      win->surface = win->native.u.surface;
      break;
   default:
      break;
   }
   if (win->surface == EGL_NO_SURFACE)
      _eglutFatal("failed to create surface");

   return win;
}

void
eglutInitAPIMask(int mask)
{
   _eglut->api_mask = mask;
}

void
eglutInitWindowSize(int width, int height)
{
   _eglut->window_width = width;
   _eglut->window_height = height;
}

void
eglutInit(int argc, char **argv)
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0)
         _eglut->display_name = argv[++i];
      else if (strcmp(argv[i], "-info") == 0) {
         _eglut->verbose = 1;
      }
   }

   _eglutNativeInitDisplay();
   _eglut->dpy = eglGetDisplay(_eglut->native_dpy);

   if (!eglInitialize(_eglut->dpy, &_eglut->major, &_eglut->minor))
      _eglutFatal("failed to initialize EGL display");

   _eglut->init_time = _eglutNow();

   printf("EGL_VERSION = %s\n", eglQueryString(_eglut->dpy, EGL_VERSION));
   if (_eglut->verbose) {
      printf("EGL_VENDOR = %s\n", eglQueryString(_eglut->dpy, EGL_VENDOR));
      printf("EGL_EXTENSIONS = %s\n",
            eglQueryString(_eglut->dpy, EGL_EXTENSIONS));
      printf("EGL_CLIENT_APIS = %s\n",
            eglQueryString(_eglut->dpy, EGL_CLIENT_APIS));
   }
}

int
eglutGet(int state)
{
   int val;

   switch (state) {
   case EGLUT_ELAPSED_TIME:
      val = _eglutNow() - _eglut->init_time;
      break;
   default:
      val = -1;
      break;
   }

   return val;
}

void
eglutIdleFunc(EGLUTidleCB func)
{
   _eglut->idle_cb = func;
}

void
eglutPostRedisplay(void)
{
   _eglut->redisplay = 1;
}

void
eglutMainLoop(void)
{
   struct eglut_window *win = _eglut->current;

   if (!win)
      _eglutFatal("no window is created\n");

   if (win->reshape_cb)
      win->reshape_cb(win->native.width, win->native.height);

   _eglutNativeEventLoop();
}

static void
_eglutFini(void)
{
   eglTerminate(_eglut->dpy);
   _eglutNativeFiniDisplay();
}

void
eglutDestroyWindow(int win)
{
   struct eglut_window *window = _eglut->current;

   if (window->index != win)
      return;

   /* XXX it causes some bug in st/egl KMS backend */
   if ( _eglut->surface_type != EGL_SCREEN_BIT_MESA)
      eglMakeCurrent(_eglut->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

   _eglutDestroyWindow(_eglut->current);
}

static void
_eglutDefaultKeyboard(unsigned char key)
{
   if (key == 27) {
      if (_eglut->current)
         eglutDestroyWindow(_eglut->current->index);
      _eglutFini();

      exit(0);
   }
}

int
eglutCreateWindow(const char *title)
{
   struct eglut_window *win;

   win = _eglutCreateWindow(title, 0, 0,
         _eglut->window_width, _eglut->window_height);

   win->index = _eglut->num_windows++;
   win->reshape_cb = NULL;
   win->display_cb = NULL;
   win->keyboard_cb = _eglutDefaultKeyboard;
   win->special_cb = NULL;

   if (!eglMakeCurrent(_eglut->dpy, win->surface, win->surface, win->context))
      _eglutFatal("failed to make window current");
   _eglut->current = win;

   return win->index;
}

int
eglutGetWindowWidth(void)
{
   struct eglut_window *win = _eglut->current;
   return win->native.width;
}

int
eglutGetWindowHeight(void)
{
   struct eglut_window *win = _eglut->current;
   return win->native.height;
}

void
eglutDisplayFunc(EGLUTdisplayCB func)
{
   struct eglut_window *win = _eglut->current;
   win->display_cb = func;

}

void
eglutReshapeFunc(EGLUTreshapeCB func)
{
   struct eglut_window *win = _eglut->current;
   win->reshape_cb = func;
}

void
eglutKeyboardFunc(EGLUTkeyboardCB func)
{
   struct eglut_window *win = _eglut->current;
   win->keyboard_cb = func;
}

void
eglutSpecialFunc(EGLUTspecialCB func)
{
   struct eglut_window *win = _eglut->current;
   win->special_cb = func;
}
