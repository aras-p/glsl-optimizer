/*
 * Test glMapBuffer() and glMapBufferRange()
 *
 * Fill a VBO with vertex data to draw several colored quads.
 * On each redraw, update the geometry for just one rect in the VBO.
 *
 * Brian Paul
 * 4 March 2009
 */


#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glew.h>
#include <GL/glut.h>

static GLuint Win;
static const GLuint NumRects = 10;
static GLuint BufferID;
static GLboolean Anim = GL_TRUE;
static GLboolean UseBufferRange = GL_FALSE;



static const float RectData[] = {
   /* vertex */    /* color */
    0, -1, 0,      1,  0,  0,
    1,  0, 0,      1,  1,  0,
    0,  1, 0,      0,  1,  1,
   -1,  0, 0,      1,  0,  1
};


/**
 * The buffer contains vertex positions (float[3]) and colors (float[3])
 * for 'NumRects' quads.
 * This function updates/rotates one quad in the buffer.
 */
static void
UpdateRect(int r, float angle)
{
   float *rect;
   int i;

   assert(r < NumRects);

   glBindBufferARB(GL_ARRAY_BUFFER_ARB, BufferID);
   if (UseBufferRange) {
      GLintptr offset = r * sizeof(RectData);
      GLsizeiptr length = sizeof(RectData);
      GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;
      float *buf = (float *) glMapBufferRange(GL_ARRAY_BUFFER_ARB,
                                              offset, length, access);
      rect = buf;
   }
   else {
      /* map whole buffer */
      float *buf = (float *)
         glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
      rect = buf + r * 24;
   }

   /* set rect verts/colors */
   memcpy(rect, RectData, sizeof(RectData));

   /* loop over four verts, updating vertices */
   for (i = 0; i < 4; i++) {
      float x = 0.2 * RectData[i*6+0];
      float y = 0.2 * RectData[i*6+1];
      float xpos = -2.5 + 0.5 * r;
      float ypos = 0.0;

      /* translate and rotate vert */
      rect[i * 6 + 0] = xpos + x * cos(angle) + y * sin(angle);
      rect[i * 6 + 1] = ypos + x * sin(angle) - y * cos(angle);
   }

   glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
}


static void
LoadBuffer(void)
{
   static int frame = 0;
   float angle = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   UpdateRect(frame % NumRects, angle);
   frame++;
}


static void
Draw(void)
{
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, BufferID);
   glVertexPointer(3, GL_FLOAT, 24, 0);
   glEnableClientState(GL_VERTEX_ARRAY);

   glColorPointer(3, GL_FLOAT, 24, (void*) 12);
   glEnableClientState(GL_COLOR_ARRAY);

   glDrawArrays(GL_QUADS, 0, NumRects * 4);

   if (0)
      glFinish();
}


static void
Display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);
   Draw();
   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-3.0, 3.0, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
Idle(void)
{
   LoadBuffer();
   glutPostRedisplay();
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   if (key == 'a') {
      Anim = !Anim;
      glutIdleFunc(Anim ? Idle : NULL);
   }
   else if (key == 's') {
      LoadBuffer();
   }
   else if (key == 27) {
      glutDestroyWindow(Win);
      exit(0);
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   GLuint BufferSize = NumRects * sizeof(RectData);
   float *buf;

   if (!glutExtensionSupported("GL_ARB_vertex_buffer_object")) {
      printf("GL_ARB_vertex_buffer_object not found!\n");
      exit(0);
   }

   UseBufferRange = glutExtensionSupported("GL_ARB_map_buffer_range");
   printf("Use GL_ARB_map_buffer_range: %c\n", "NY"[UseBufferRange]);

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* initially load buffer with zeros */
   buf = (float *) calloc(1, BufferSize);

   glGenBuffersARB(1, &BufferID);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, BufferID);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, BufferSize, buf, GL_DYNAMIC_DRAW_ARB);

   free(buf);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(800, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   glutIdleFunc(Anim ? Idle : NULL);
   Init();
   glutMainLoop();
   return 0;
}
