/**
 * Draw colored quads.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#define NUM_QUADS 20


static int Win;
static GLfloat Xrot = 40, Yrot = 0, Zrot = 0;
static GLboolean Anim = GL_TRUE;
static GLuint Vbuffer = 0;

#if 1
#else
static GLfloat buf[NUM_QUADS * 6 * 4];
#endif

static GLboolean doSwapBuffers = GL_TRUE;

static GLint Frames = 0, T0 = 0;


static void
Idle(void)
{
   Xrot += 3.0;
   Yrot += 4.0;
   Zrot += 2.0;
   glutPostRedisplay();
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   glDrawArrays(GL_QUADS, 0, NUM_QUADS*4);

   glPopMatrix();

   if (doSwapBuffers)
      glutSwapBuffers();
   /*
   else
      glFinish();
   */

   {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      Frames++;
      if (t - T0 >= 5000) {
         GLfloat seconds = (t - T0) / 1000.0;
         GLfloat fps = Frames / seconds;
         printf("%d frames in %6.3f seconds = %6.3f FPS\n",
                Frames, seconds, fps);
         T0 = t;
         Frames = 0;
      }
   }
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -8.0);
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case 's':
         doSwapBuffers = !doSwapBuffers;
         break;
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'z':
         Zrot -= step;
         break;
      case 'Z':
         Zrot += step;
         break;
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void
quad(float x, float y, float z, float *v)
{
   int k = 0;

   /* color */
   v[k++] = x * 0.5 + 0.5;
   v[k++] = y * 0.5 + 0.5;
   v[k++] = z * 0.5 + 0.5;
   /* vert */
   v[k++] = x;
   v[k++] = y;
   v[k++] = z;

   /* color */
   v[k++] = -x * 0.5 + 0.5;
   v[k++] = -y * 0.5 + 0.5;
   v[k++] = z * 0.5 + 0.5;
   /* vert */
   v[k++] = -x;
   v[k++] = -y;
   v[k++] = z;

   /* color */
   v[k++] = -x * 0.5 + 0.5;
   v[k++] = -y * 0.5 + 0.5;
   v[k++] = -z * 0.5 + 0.5;
   /* vert */
   v[k++] = -x;
   v[k++] = -y;
   v[k++] = -z;

   /* color */
   v[k++] = x * 0.5 + 0.5;
   v[k++] = y * 0.5 + 0.5;
   v[k++] = -z * 0.5 + 0.5;
   /* vert */
   v[k++] = x;
   v[k++] = y;
   v[k++] = -z;
}

static void
gen_quads(GLfloat *buf)
{
   float *v = buf;
   float r = 1.0;
   int i;

   for (i = 0; i < NUM_QUADS; i++) {
      float angle = i / (float) NUM_QUADS * M_PI;
      float x = r * cos(angle);
      float y = r * sin(angle);
      float z = 1.10;
      quad(x, y, z, v);
      v += 24;
   }

   if (0) {
      float *p = buf;
      for (i = 0; i < NUM_QUADS * 4 * 2; i++) {
         printf("%d: %f %f %f\n", i, p[0], p[1], p[2]);
         p += 3;
      }
   }
}


static void
Init(void)
{
   int bytes = NUM_QUADS * 4 * 2 * 3 * sizeof(float);
   GLfloat *f;

#if 1
   glGenBuffers(1, &Vbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, Vbuffer);
   glBufferData(GL_ARRAY_BUFFER_ARB, bytes, NULL, GL_STATIC_DRAW_ARB);
   f = (float *) glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
   gen_quads(f);
   glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
   glColorPointer(3, GL_FLOAT, 6*sizeof(float), (void *) 0);
   glVertexPointer(3, GL_FLOAT, 6*sizeof(float), (void *) 12);
#else
   f = buf;
   gen_quads(f);
   glColorPointer(3, GL_FLOAT, 6*sizeof(float), buf);
   glVertexPointer(3, GL_FLOAT, 6*sizeof(float), buf + 3);
#endif

   glEnableClientState(GL_COLOR_ARRAY);
   glEnableClientState(GL_VERTEX_ARRAY);

   glEnable(GL_DEPTH_TEST);

   glClearColor(0.5, 0.5, 0.5, 0.0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(600, 600);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
