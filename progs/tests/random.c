/**
 * Random rendering, to check for crashes, hangs, etc.
 *
 * Brian Paul
 * 21 June 2007
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Win;
static GLboolean Anim = GL_TRUE;
static int Width = 200, Height = 200;
static int DB = 0;
static int MinVertexCount = 0, MaxVertexCount = 1000;
static int Count = 0;

struct vertex
{
   int type;
   float v[4];
};

static int BufferSize = 10000;
static struct vertex *Vbuffer = NULL;
static int Vcount, Vprim;

enum {
   BEGIN,
   END,
   VERTEX2,
   VERTEX3,
   VERTEX4,
   COLOR3,
   COLOR4,
   TEX2,
   TEX3,
   TEX4,
   SECCOLOR3,
   NORMAL3
};



/**
 * This can be called from within gdb after a crash:
 * (gdb) call ReportState()
 */
static void
ReportState(void)
{
   static const struct {
      GLenum token;
      char *str;
      GLenum type;
   } state [] = {
      { GL_ALPHA_TEST, "GL_ALPHA_TEST", GL_INT },
      { GL_BLEND, "GL_BLEND", GL_INT },
      { GL_CLIP_PLANE0, "GL_CLIP_PLANE0", GL_INT },
      { GL_DEPTH_TEST, "GL_DEPTH_TEST", GL_INT },
      { GL_LIGHTING, "GL_LIGHTING", GL_INT },
      { GL_LINE_WIDTH, "GL_LINE_WIDTH", GL_FLOAT },
      { GL_POINT_SIZE, "GL_POINT_SIZE", GL_FLOAT },
      { GL_SHADE_MODEL, "GL_SHADE_MODEL", GL_INT },
      { GL_SCISSOR_TEST, "GL_SCISSOR_TEST", GL_INT },
      { 0, NULL, 0 }
   };

   GLint i;

   for (i = 0; state[i].token; i++) {
      if (state[i].type == GL_INT) {
         GLint v;
         glGetIntegerv(state[i].token, &v);
         printf("%s = %d\n", state[i].str, v);
      }
      else {
         GLfloat v;
         glGetFloatv(state[i].token, &v);
         printf("%s = %f\n", state[i].str, v);
      }
   }
}

static void
PrintVertex(const char *f, const struct vertex *v, int sz)
{
   int i;
   printf("%s(", f);
   for (i = 0; i < sz; i++) {
      printf("%g%s", v->v[i], (i == sz-1) ? "" : ", ");
   }
   printf(");\n");
}

/**
 * This can be called from within gdb after a crash:
 * (gdb) call ReportState()
 */
static void
LastPrim(void)
{
   int i;
   for (i = 0; i < Vcount; i++) {
      switch (Vbuffer[i].type) {
      case BEGIN:
         printf("glBegin(%d);\n", (int) Vbuffer[i].v[0]);
         break;
      case END:
         printf("glEnd();\n");
         break;
      case VERTEX2:
         PrintVertex("glVertex2f", Vbuffer + i, 2);
         break;
      case VERTEX3:
         PrintVertex("glVertex3f", Vbuffer + i, 3);
         break;
      case VERTEX4:
         PrintVertex("glVertex4f", Vbuffer + i, 4);
         break;
      case COLOR3:
         PrintVertex("glColor3f", Vbuffer + i, 3);
         break;
      case COLOR4:
         PrintVertex("glColor4f", Vbuffer + i, 4);
         break;
      case TEX2:
         PrintVertex("glTexCoord2f", Vbuffer + i, 2);
         break;
      case TEX3:
         PrintVertex("glTexCoord3f", Vbuffer + i, 3);
         break;
      case TEX4:
         PrintVertex("glTexCoord4f", Vbuffer + i, 4);
         break;
      case SECCOLOR3:
         PrintVertex("glSecondaryColor3f", Vbuffer + i, 3);
         break;
      case NORMAL3:
         PrintVertex("glNormal3f", Vbuffer + i, 3);
         break;
      default:
         abort();
      }
   }
}


static int
RandomInt(int max)
{
   if (max == 0)
      return 0;
   return rand() % max;
}

static float
RandomFloat(float min, float max)
{
   int k = rand() % 10000;
   float x = min + (max - min) * k / 10000.0;
   return x;
}

/*
 * Return true if random number in [0,1] is <= percentile.
 */
static GLboolean
RandomChoice(float percentile)
{
   return RandomFloat(0.0, 1.0) <= percentile;
}

static void
RandomStateChange(void)
{
   int k = RandomInt(19);
   switch (k) {
   case 0:
      glEnable(GL_BLEND);
      break;
   case 1:
      glDisable(GL_BLEND);
      break;
   case 2:
      glEnable(GL_ALPHA_TEST);
      break;
   case 3:
      glEnable(GL_ALPHA_TEST);
      break;
   case 4:
      glEnable(GL_DEPTH_TEST);
      break;
   case 5:
      glEnable(GL_DEPTH_TEST);
      break;
   case 6:
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      break;
   case 7:
      glPointSize(10.0);
      break;
   case 8:
      glPointSize(1.0);
      break;
   case 9:
      glLineWidth(10.0);
      break;
   case 10:
      glLineWidth(1.0);
      break;
   case 11:
      glEnable(GL_LIGHTING);
      break;
   case 12:
      glDisable(GL_LIGHTING);
      break;
   case 13:
      glEnable(GL_SCISSOR_TEST);
      break;
   case 14:
      glDisable(GL_SCISSOR_TEST);
      break;
   case 15:
      glEnable(GL_CLIP_PLANE0);
      break;
   case 16:
      glDisable(GL_CLIP_PLANE0);
      break;
   case 17:
      glShadeModel(GL_FLAT);
      break;
   case 18:
      glShadeModel(GL_SMOOTH);
      break;
   }
}


static void
RandomPrimitive(void)
{
   int i;
   int len = MinVertexCount + RandomInt(MaxVertexCount - MinVertexCount);

   Vprim = RandomInt(10);

   glBegin(Vprim);
   Vbuffer[Vcount].type = BEGIN;
   Vbuffer[Vcount].v[0] = Vprim;
   Vcount++;

   for (i = 0; i < len; i++) {
      int k = RandomInt(9);
      Vbuffer[Vcount].v[0] = RandomFloat(-3, 3);
      Vbuffer[Vcount].v[1] = RandomFloat(-3, 3);
      Vbuffer[Vcount].v[2] = RandomFloat(-3, 3);
      Vbuffer[Vcount].v[3] = RandomFloat(-3, 3);
      switch (k) {
      case 0:
         glVertex2fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = VERTEX2;
         break;
      case 1:
         glVertex3fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = VERTEX3;
         break;
      case 2:
         glVertex4fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = VERTEX4;
         break;
      case 3:
         glColor3fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = COLOR3;
         break;
      case 4:
         glColor4fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = COLOR4;
         break;
      case 5:
         glTexCoord2fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = TEX2;
         break;
      case 6:
         glTexCoord3fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = TEX3;
         break;
      case 7:
         glTexCoord4fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = TEX4;
         break;
      case 8:
         glSecondaryColor3fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = SECCOLOR3;
         break;
      case 9:
         glNormal3fv(Vbuffer[Vcount].v);
         Vbuffer[Vcount].type = NORMAL3;
         break;
      default:
         abort();
      }
      Vcount++;

      if (Vcount >= BufferSize - 2) {
         /* reset */
         Vcount = 0;
      }
   }

   Vbuffer[Vcount++].type = END;

   glEnd();
}


static void
RandomDraw(void)
{
   int i;
   GLboolean dlist = RandomChoice(0.1);
   if (dlist)
      glNewList(1, GL_COMPILE);
   for (i = 0; i < 3; i++) {
      RandomStateChange();
   }
   RandomPrimitive();

   if (dlist) {
      glEndList();
      glCallList(1);
   }
}


static void
Idle(void)
{
   glutPostRedisplay();
}


static void
Draw(void)
{
#if 1
   RandomDraw();
   Count++;
#else
   /* cut & paste temp code here */
#endif

   assert(glGetError() == 0);

   if (DB)
      glutSwapBuffers();
   else
      glFinish();
}


static void
Reshape(int width, int height)
{
   Width = width;
   Height = height;
   glViewport(0, 0, width, height);
   glScissor(20, 20, Width-40, Height-40);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   static const GLdouble plane[4] = {1, 1, 0, 0};
   glDrawBuffer(GL_FRONT);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_LIGHT0);
   glClipPlane(GL_CLIP_PLANE0, plane);

   Vbuffer = (struct vertex *)
      malloc(BufferSize * sizeof(struct vertex));

   /* silence warnings */
   (void) ReportState;
   (void) LastPrim;
}


static void
ParseArgs(int argc, char *argv[])
{
   int i;
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-s") == 0) {
         int j = atoi(argv[i + 1]);
         printf("Random seed value: %d\n", j);
         srand(j);
         i++;
      }
      else if (strcmp(argv[i], "-a") == 0) {
         i++;
         MinVertexCount = atoi(argv[i]);
      }
      else if (strcmp(argv[i], "-b") == 0) {
         i++;
         MaxVertexCount = atoi(argv[i]);
      }
   }
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   ParseArgs(argc, argv);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
