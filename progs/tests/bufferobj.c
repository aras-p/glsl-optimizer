/*
 * Test GL_ARB_vertex_buffer_object
 *
 * Brian Paul
 * 16 Sep 2003
 */


#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define NUM_OBJECTS 10

struct object
{
   GLuint BufferID;
   GLuint ElementsBufferID;
   GLuint NumVerts;
   GLuint VertexOffset;
   GLuint ColorOffset;
   GLuint NumElements;
};

static struct object Objects[NUM_OBJECTS];
static GLuint NumObjects;

static GLuint Win;

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLboolean Anim = GL_TRUE;


static void CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


static void DrawObject( const struct object *obj )
{
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, obj->BufferID);
   glVertexPointer(3, GL_FLOAT, 0, (void *) obj->VertexOffset);
   glEnable(GL_VERTEX_ARRAY);

   /* test push/pop attrib */
   /* XXX this leads to a segfault with NVIDIA's 53.36 driver */
#if 0
   if (1)
   {
      glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
      /*glVertexPointer(3, GL_FLOAT, 0, (void *) (obj->VertexOffset + 10000));*/
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, 999999);
      glPopClientAttrib();
   }
#endif
   glColorPointer(3, GL_FLOAT, 0, (void *) obj->ColorOffset);
   glEnable(GL_COLOR_ARRAY);

   if (obj->NumElements > 0) {
      /* indexed arrays */
      glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, obj->ElementsBufferID);
      glDrawElements(GL_LINE_LOOP, obj->NumElements, GL_UNSIGNED_INT, NULL);
   }
   else {
      /* non-indexed arrays */
      glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
      glDrawArrays(GL_LINE_LOOP, 0, obj->NumVerts);
   }
}


static void Idle( void )
{
   Zrot = 0.05 * glutGet(GLUT_ELAPSED_TIME);
   glutPostRedisplay();
}


static void Display( void )
{
   int i;

   glClear( GL_COLOR_BUFFER_BIT );

   for (i = 0; i < NumObjects; i++) {
      float x = 5.0 * ((float) i / (NumObjects-1) - 0.5);
      glPushMatrix();
      glTranslatef(x, 0, 0);
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Zrot, 0, 0, 1);

      DrawObject(Objects + i);

      glPopMatrix();
   }

   CheckError(__LINE__);
   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   float ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}


static void FreeBuffers(void)
{
   int i;
   for (i = 0; i < NUM_OBJECTS; i++)
      glDeleteBuffersARB(1, &Objects[i].BufferID);
}


static void Key( unsigned char key, int x, int y )
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
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
         FreeBuffers();
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
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



static void MakeObject1(struct object *obj)
{
   GLfloat *v, *c;
   void *p;
   int i;
   GLubyte buffer[500];

   for (i = 0; i < 500; i++)
      buffer[i] = i & 0xff;

   obj->BufferID = 0;
   glGenBuffersARB(1, &obj->BufferID);
   assert(obj->BufferID != 0);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, obj->BufferID);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, 500, buffer, GL_STATIC_DRAW_ARB);

   for (i = 0; i < 500; i++)
      buffer[i] = 0;

   glGetBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 500, buffer);

   for (i = 0; i < 500; i++)
      assert(buffer[i] == (i & 0xff));

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAPPED_ARB, &i);
   assert(!i);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_USAGE_ARB, &i);

   v = (GLfloat *) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

   /* do some sanity tests */
   glGetBufferPointervARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAP_POINTER_ARB, &p);
   assert(p == v);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &i);
   assert(i == 500);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_USAGE_ARB, &i);
   assert(i == GL_STATIC_DRAW_ARB);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_ACCESS_ARB, &i);
   assert(i == GL_WRITE_ONLY_ARB);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAPPED_ARB, &i);
   assert(i);

   /* Make rectangle */
   v[0] = -1;  v[1] = -1;  v[2] = 0;
   v[3] =  1;  v[4] = -1;  v[5] = 0;
   v[6] =  1;  v[7] =  1;  v[8] = 0;
   v[9] = -1;  v[10] = 1;  v[11] = 0;
   c = v + 12;
   c[0] = 1;  c[1] = 0;  c[2] = 0;
   c[3] = 1;  c[4] = 0;  c[5] = 0;
   c[6] = 1;  c[7] = 0;  c[8] = 1;
   c[9] = 1;  c[10] = 0;  c[11] = 1;
   obj->NumVerts = 4;
   obj->VertexOffset = 0;
   obj->ColorOffset = 3 * sizeof(GLfloat) * obj->NumVerts;
   obj->NumElements = 0;

   glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

   glGetBufferPointervARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAP_POINTER_ARB, &p);
   assert(!p);

   glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_MAPPED_ARB, &i);
   assert(!i);
}


static void MakeObject2(struct object *obj)
{
   GLfloat *v, *c;

   glGenBuffersARB(1, &obj->BufferID);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, obj->BufferID);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, 1000, NULL, GL_STATIC_DRAW_ARB);
   v = (GLfloat *) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

   /* Make triangle */
   v[0] = -1;  v[1] = -1;  v[2] = 0;
   v[3] =  1;  v[4] = -1;  v[5] = 0;
   v[6] =  0;  v[7] =  1;  v[8] = 0;
   c = v + 9;
   c[0] = 0;  c[1] = 1;  c[2] = 0;
   c[3] = 0;  c[4] = 1;  c[5] = 0;
   c[6] = 1;  c[7] = 1;  c[8] = 0;
   obj->NumVerts = 3;
   obj->VertexOffset = 0;
   obj->ColorOffset = 3 * sizeof(GLfloat) * obj->NumVerts;
   obj->NumElements = 0;

   glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
}


static void MakeObject3(struct object *obj)
{
   GLfloat vertexData[1000];
   GLfloat *v, *c;
   GLuint *i;
   int bytes;

   /* Make rectangle */
   v = vertexData;
   v[0] = -1;  v[1] = -0.5;  v[2] = 0;
   v[3] =  1;  v[4] = -0.5;  v[5] = 0;
   v[6] =  1;  v[7] =  0.5;  v[8] = 0;
   v[9] = -1;  v[10] = 0.5;  v[11] = 0;
   c = vertexData + 12;
   c[0] = 0;  c[1] = 0;  c[2] = 1;
   c[3] = 0;  c[4] = 0;  c[5] = 1;
   c[6] = 0;  c[7] = 1;  c[8] = 1;
   c[9] = 0;  c[10] = 1;  c[11] = 1;
   obj->NumVerts = 4;
   obj->VertexOffset = 0;
   obj->ColorOffset = 3 * sizeof(GLfloat) * obj->NumVerts;

   bytes = obj->NumVerts * (3 + 3) * sizeof(GLfloat);

   /* Don't use glMap/UnmapBuffer for this object */
   glGenBuffersARB(1, &obj->BufferID);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, obj->BufferID);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB, bytes, vertexData, GL_STATIC_DRAW_ARB);

   /* Setup a buffer of indices to test the ELEMENTS path */
   glGenBuffersARB(1, &obj->ElementsBufferID);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, obj->ElementsBufferID);
   glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 100, NULL, GL_STATIC_DRAW_ARB);
   i = (GLuint *) glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);
   i[0] = 0;
   i[1] = 1;
   i[2] = 2;
   i[3] = 3;
   glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
   obj->NumElements = 4;
}



static void Init( void )
{
   if (!glutExtensionSupported("GL_ARB_vertex_buffer_object")) {
      printf("GL_ARB_vertex_buffer_object not found!\n");
      exit(0);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Test buffer object deletion */
   if (1) {
      static GLubyte data[1000];
      GLuint id = 999;
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, id);
      glBufferDataARB(GL_ARRAY_BUFFER_ARB, 1000, data, GL_STATIC_DRAW_ARB);
      glVertexPointer(3, GL_FLOAT, 0, (void *) 0);
      glDeleteBuffersARB(1, &id);
      assert(!glIsBufferARB(id));
      glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
      glVertexPointer(3, GL_FLOAT, 0, (void *) 0);
      assert(!glIsBufferARB(id));
   }

   MakeObject1(Objects + 0);
   MakeObject2(Objects + 1);
   MakeObject3(Objects + 2);
   NumObjects = 3;
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 600, 300 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
