/**
 * Test rubber-band selection box w/ logicops and blend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "readtex.c"

#define IMAGE_FILE "../images/arch.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

static int Win;
static int Width = 512, Height = 512;

struct rect
{
   int x0, y0, x1, y1;
};

static struct rect OldRect, NewRect;

static GLboolean ButtonDown = GL_FALSE;
static GLboolean LogicOp = 0*GL_TRUE;

static GLboolean RedrawBackground = GL_TRUE;

static const GLfloat red[4] = {1.0, 0.2, 0.2, 1.0};
static const GLfloat green[4] = {0.2, 1.0, 0.2, 1.0};
static const GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};


/*
 * Draw rubberband box in front buffer
 */
static void
DrawRect(const struct rect *r)
{
   glDrawBuffer(GL_FRONT);

   if (LogicOp) {
      glLogicOp(GL_XOR);
      glEnable(GL_COLOR_LOGIC_OP);
   }
   else {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glBlendEquation(GL_FUNC_SUBTRACT);
   }

   glColor3f(1, 1, 1);

   glLineWidth(3.0);

   glBegin(GL_LINE_LOOP);
   glVertex2i(r->x0, r->y0);
   glVertex2i(r->x1, r->y0);
   glVertex2i(r->x1, r->y1);
   glVertex2i(r->x0, r->y1);
   glEnd();

   glDisable(GL_COLOR_LOGIC_OP);
   glDisable(GL_BLEND);

   glDrawBuffer(GL_BACK);
}


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
DrawBackground(void)
{
   char s[100];

   sprintf(s, "[L/B] %s mode.   Use mouse to make selection box.",
               LogicOp ? "LogicOp" : "Blend");

   glClear(GL_COLOR_BUFFER_BIT);

   glWindowPos2i((Width - ImgWidth) / 2, (Height - ImgHeight) / 2);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);

   glWindowPos2i(10, 10);
   PrintString(s);

   glutSwapBuffers();
}


static void
Draw(void)
{
   if (RedrawBackground) {
      DrawBackground();
   }

   if (ButtonDown) {
      if (!RedrawBackground)
         DrawRect(&OldRect); /* erase old */

      DrawRect(&NewRect); /* draw new */

      OldRect = NewRect;
   }

   RedrawBackground = GL_FALSE;
}


static void
Reshape(int width, int height)
{
   Width = width;
   Height = height;

   glViewport(0, 0, width, height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, Width, Height, 0, -1, 1); /* Inverted Y! */

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   RedrawBackground = GL_TRUE;
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 'b':
   case 'B':
      LogicOp = GL_FALSE;
      break;
   case 'l':
   case 'L':
      LogicOp = GL_TRUE;
      break;
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }
   RedrawBackground = GL_TRUE;
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      break;
   case GLUT_KEY_DOWN:
      break;
   case GLUT_KEY_LEFT:
      break;
   case GLUT_KEY_RIGHT:
      break;
   }
   glutPostRedisplay();
}


static void
MouseMotion(int x, int y)
{
   if (ButtonDown) {
      NewRect.x1 = x;
      NewRect.y1 = y;
      glutPostRedisplay();
   }
}


static void
MouseButton(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
     ButtonDown = GL_TRUE;
     RedrawBackground = GL_TRUE;
     NewRect.x0 = NewRect.x1 = x;
     NewRect.y0 = NewRect.y1 = y;
     OldRect = NewRect;
  }
  else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
     ButtonDown = GL_FALSE;
  }
  glutPostRedisplay();
}


static void
Init(void)
{
   Image = LoadRGBImage(IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat);
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutMotionFunc(MouseMotion);
   glutMouseFunc(MouseButton);
   glutDisplayFunc(Draw);
   Init();
   glutPostRedisplay();
   glutMainLoop();
   return 0;
}
