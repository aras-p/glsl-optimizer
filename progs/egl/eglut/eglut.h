#ifndef EGLUT_H
#define EGLUT_H

/* used by eglutInitAPIMask */
enum {
   EGLUT_OPENGL_BIT     = 0x1,
   EGLUT_OPENGL_ES1_BIT = 0x2,
   EGLUT_OPENGL_ES2_BIT = 0x4,
   EGLUT_OPENVG_BIT     = 0x8
};

/* used by EGLUTspecialCB */
enum {
   /* function keys */
   EGLUT_KEY_F1,
   EGLUT_KEY_F2,
   EGLUT_KEY_F3,
   EGLUT_KEY_F4,
   EGLUT_KEY_F5,
   EGLUT_KEY_F6,
   EGLUT_KEY_F7,
   EGLUT_KEY_F8,
   EGLUT_KEY_F9,
   EGLUT_KEY_F10,
   EGLUT_KEY_F11,
   EGLUT_KEY_F12,

   /* directional keys */
   EGLUT_KEY_LEFT,
   EGLUT_KEY_UP,
   EGLUT_KEY_RIGHT,
   EGLUT_KEY_DOWN,
};

/* used by eglutGet */
enum {
   EGLUT_ELAPSED_TIME
};

typedef void (*EGLUTidleCB)(void);
typedef void (*EGLUTreshapeCB)(int, int);
typedef void (*EGLUTdisplayCB)(void);
typedef void (*EGLUTkeyboardCB)(unsigned char);
typedef void (*EGLUTspecialCB)(int);

void eglutInitAPIMask(int mask);
void eglutInitWindowSize(int width, int height);
void eglutInit(int argc, char **argv);

int eglutGet(int state);

void eglutIdleFunc(EGLUTidleCB func);
void eglutPostRedisplay(void);

void eglutMainLoop(void);

int eglutCreateWindow(const char *title);
void eglutDestroyWindow(int win);

int eglutGetWindowWidth(void);
int eglutGetWindowHeight(void);

void eglutDisplayFunc(EGLUTdisplayCB func);
void eglutReshapeFunc(EGLUTreshapeCB func);
void eglutKeyboardFunc(EGLUTkeyboardCB func);
void eglutSpecialFunc(EGLUTspecialCB func);

#endif /* EGLUT_H */
