/*
 * EXT_fog_coord.
 *
 * Based on glutskel.c by Brian Paul
 * and NeHe's Volumetric fog tutorial!
 *
 * Daniel Borca
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#include "readtex.c" /* the compulsory hack  */

#define TEXTURE_FILE "../images/bw.rgb"

#define ARRAYS 0     /* use glDrawElements   */

#define VERBOSE 1    /* tell me what happens */

#define DEPTH 15.0f

#if !defined(GLAPIENTRYP)
#    define GLAPIENTRYP *
#endif

typedef void (GLAPIENTRYP GLFOGCOORDFEXTPROC) (GLfloat f);
typedef void (GLAPIENTRYP GLFOGCOORDPOINTEREXTPROC) (GLenum, GLsizei, const GLvoid *);

static GLFOGCOORDFEXTPROC glFogCoordf_ext;
#if ARRAYS
static GLFOGCOORDPOINTEREXTPROC glFogCoordPointer_ext;
#endif
static GLboolean have_fog_coord;

static GLfloat camz;
static GLuint texture[1];

static GLint fogMode;
static GLboolean fogCoord;
static GLfloat fogDensity = 0.75;
static GLfloat fogStart = 1.0, fogEnd = 40.0;
static GLfloat fogColor[4] = {0.6f, 0.3f, 0.0f, 1.0f};


static void APIENTRY glFogCoordf_nop (GLfloat f)
{
   (void)f;
}


static int BuildTexture (const char *filename, GLuint texid[])
{
   GLubyte *tex_data;
   GLenum tex_format;
   GLint tex_width, tex_height;

   tex_data = LoadRGBImage(filename, &tex_width, &tex_height, &tex_format);
   if (tex_data == NULL) {
      return -1;
   }

   {
      GLint tex_max;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tex_max);
      if ((tex_width > tex_max) || (tex_height > tex_max)) {
         return -1;
      }
   }

   glGenTextures(1, texid);
   
   glBindTexture(GL_TEXTURE_2D, texid[0]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, tex_format, tex_width, tex_height, 0,
                tex_format, GL_UNSIGNED_BYTE, tex_data);

   return 0;
}


static int SetFogMode (GLint fogMode)
{
   fogMode &= 3;
   switch (fogMode) {
   case 0:
      glDisable(GL_FOG);
#if VERBOSE
      printf("fog(disable)\n");
#endif
      break;
   case 1:
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_LINEAR);
      glFogf(GL_FOG_START, fogStart);
      glFogf(GL_FOG_END, fogEnd);
#if VERBOSE
      printf("fog(GL_LINEAR, %.2f, %.2f)\n", fogStart, fogEnd);
#endif
      break;
   case 2:
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_EXP);
      glFogf(GL_FOG_DENSITY, fogDensity);
#if VERBOSE
      printf("fog(GL_EXP, %.2f)\n", fogDensity);
#endif
      break;
   case 3:
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_EXP2);
      glFogf(GL_FOG_DENSITY, fogDensity);
#if VERBOSE
      printf("fog(GL_EXP2, %.2f)\n", fogDensity);
#endif
      break;
   }
   return fogMode;
}


static GLboolean SetFogCoord (GLboolean fogCoord)
{
   glFogCoordf_ext = glFogCoordf_nop;

   if (!have_fog_coord) {
#if VERBOSE
      printf("fog(GL_FRAGMENT_DEPTH_EXT)%s\n", fogCoord ? " EXT_fog_coord not available!" : "");
#endif
      return GL_FALSE;
   }

   if (fogCoord) {
      glFogCoordf_ext = (GLFOGCOORDFEXTPROC)glutGetProcAddress("glFogCoordfEXT");
      glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
#if VERBOSE
      printf("fog(GL_FOG_COORDINATE_EXT)\n");
#endif
   } else {
      glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
#if VERBOSE
      printf("fog(GL_FRAGMENT_DEPTH_EXT)\n");
#endif
   }
   return fogCoord;
}


#if ARRAYS
/* could reuse vertices */
static GLuint vertex_index[] = {
   /* Back */
   0, 1, 2, 3,

   /* Floor */
   4, 5, 6, 7,

   /* Roof */
   8, 9, 10, 11,

   /* Right */
   12, 13, 14, 15,

   /* Left */
   16, 17, 18, 19
};

static GLfloat vertex_pointer[][3] = {
   /* Back */
   {-2.5f,-2.5f,-DEPTH}, { 2.5f,-2.5f,-DEPTH}, { 2.5f, 2.5f,-DEPTH}, {-2.5f, 2.5f,-DEPTH},

   /* Floor */
   {-2.5f,-2.5f,-DEPTH}, { 2.5f,-2.5f,-DEPTH}, { 2.5f,-2.5f, DEPTH}, {-2.5f,-2.5f, DEPTH},

   /* Roof */
   {-2.5f, 2.5f,-DEPTH}, { 2.5f, 2.5f,-DEPTH}, { 2.5f, 2.5f, DEPTH}, {-2.5f, 2.5f, DEPTH},

   /* Right */
   { 2.5f,-2.5f, DEPTH}, { 2.5f, 2.5f, DEPTH}, { 2.5f, 2.5f,-DEPTH}, { 2.5f,-2.5f,-DEPTH},

   /* Left */
   {-2.5f,-2.5f, DEPTH}, {-2.5f, 2.5f, DEPTH}, {-2.5f, 2.5f,-DEPTH}, {-2.5f,-2.5f,-DEPTH}
};

static GLfloat texcoord_pointer[][2] = {
   /* Back */
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

   /* Floor */
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

   /* Roof */
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

   /* Right */
   {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},

   /* Left */
   {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}
};

static GLfloat fogcoord_pointer[][1] = {
   /* Back */
   {1.0f}, {1.0f}, {1.0f}, {1.0f},

   /* Floor */
   {1.0f}, {1.0f}, {0.0f}, {0.0f},

   /* Roof */
   {1.0f}, {1.0f}, {0.0f}, {0.0f},

   /* Right */
   {0.0f}, {0.0f}, {1.0f}, {1.0f},

   /* Left */
   {0.0f}, {0.0f}, {1.0f}, {1.0f}
};
#endif


static void Display( void )
{
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity ();
   
   glTranslatef(0.0f, 0.0f, camz);

#if ARRAYS
   glDrawElements(GL_QUADS, sizeof(vertex_index) / sizeof(vertex_index[0]), GL_UNSIGNED_INT, vertex_index);
#else
   /* Back */
   glBegin(GL_QUADS);
   glFogCoordf_ext(1.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f,-2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f( 2.5f,-2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f( 2.5f, 2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.5f, 2.5f,-DEPTH);
   glEnd();

   /* Floor */
   glBegin(GL_QUADS);
   glFogCoordf_ext(1.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f,-2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f( 2.5f,-2.5f,-DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f( 2.5f,-2.5f, DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.5f,-2.5f, DEPTH);
   glEnd();

   /* Roof */
   glBegin(GL_QUADS);
   glFogCoordf_ext(1.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f, 2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f( 2.5f, 2.5f,-DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f( 2.5f, 2.5f, DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.5f, 2.5f, DEPTH);
   glEnd();

   /* Right */
   glBegin(GL_QUADS);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f( 2.5f,-2.5f, DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f( 2.5f, 2.5f, DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f( 2.5f, 2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f( 2.5f,-2.5f,-DEPTH);
   glEnd();

   /* Left */
   glBegin(GL_QUADS);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f,-2.5f, DEPTH);
   glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.5f, 2.5f, DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-2.5f, 2.5f,-DEPTH);
   glFogCoordf_ext(1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-2.5f,-2.5f,-DEPTH);
   glEnd();
#endif

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0f, (GLfloat)(width)/(GLfloat)(height), 0.1f, 100.0f);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'f':
         fogMode = SetFogMode(fogMode + 1);
         break;
      case '+':
         if (fogDensity < 1.0) {
            fogDensity += 0.05;
         }
         SetFogMode(fogMode);
         break;
      case '-':
         if (fogDensity > 0.0) {
            fogDensity -= 0.05;
         }
         SetFogMode(fogMode);
         break;
      case 's':
         if (fogStart > 0.0) {
            fogStart -= 1.0;
         }
         SetFogMode(fogMode);
         break;
      case 'S':
         if (fogStart < fogEnd) {
            fogStart += 1.0;
         }
         SetFogMode(fogMode);
         break;
      case 'e':
         if (fogEnd > fogStart) {
            fogEnd -= 1.0;
         }
         SetFogMode(fogMode);
         break;
      case 'E':
         if (fogEnd < 100.0) {
            fogEnd += 1.0;
         }
         SetFogMode(fogMode);
         break;
      case 'c':
         fogCoord = SetFogCoord(fogCoord ^ GL_TRUE);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         if (camz < (DEPTH - 1.0)) {
            camz += 1.0f;
         }
         break;
      case GLUT_KEY_DOWN:
         if (camz > -19.0) {
            camz -= 1.0f;
         }
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   have_fog_coord = glutExtensionSupported("GL_EXT_fog_coord");

   if (BuildTexture(TEXTURE_FILE, texture) == -1) {
      exit(1);
   }

   glEnable(GL_TEXTURE_2D);
   glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
   glClearDepth(1.0f);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

   glFogfv(GL_FOG_COLOR, fogColor);
   glHint(GL_FOG_HINT, GL_NICEST);
   fogCoord = SetFogCoord(GL_TRUE); /* try to enable fog_coord */
   fogMode = SetFogMode(2);         /* GL_EXP */

   camz = -19.0f;

#if ARRAYS
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, vertex_pointer);

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, 0, texcoord_pointer);

   if (have_fog_coord) {
      glFogCoordPointer_ext = (GLFOGCOORDPOINTEREXTPROC)glutGetProcAddress("glFogCoordPointerEXT");
      glEnableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
      glFogCoordPointer_ext(GL_FLOAT, 0, fogcoord_pointer);
   }
#endif
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 640, 480 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
