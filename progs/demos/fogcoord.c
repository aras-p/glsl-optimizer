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
#include <GL/glew.h>
#include <GL/glut.h>

#define DEPTH 5.0f

static GLfloat camz;

static GLint fogMode;
static GLboolean fogCoord;
static GLfloat fogDensity = 0.75;
static GLfloat fogStart = 1.0, fogEnd = DEPTH;
static GLfloat fogColor[4] = {0.6f, 0.3f, 0.0f, 1.0f};
static const char *ModeStr = NULL;
static GLboolean Arrays = GL_FALSE;
static GLboolean Texture = GL_TRUE;


static void
Reset(void)
{
   fogMode = 1;
   fogCoord = 1;
   fogDensity = 0.75;
   fogStart = 1.0;
   fogEnd = DEPTH;
   Arrays = GL_FALSE;
   Texture = GL_TRUE;
}


static void
glFogCoordf_ext (GLfloat f)
{
   if (fogCoord)
      glFogCoordfEXT(f);
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
PrintInfo(void)
{
   char s[100];

   glDisable(GL_FOG);
   glColor3f(0, 1, 1);

   sprintf(s, "Mode(m): %s  Start(s/S): %g  End(e/E): %g  Density(d/D): %g",
           ModeStr, fogStart, fogEnd, fogDensity);
   glWindowPos2iARB(5, 20);
   PrintString(s);

   sprintf(s, "Arrays(a): %s  glFogCoord(c): %s  EyeZ(z/z): %g",
           (Arrays ? "Yes" : "No"),
           (fogCoord ? "Yes" : "No"),
           camz);
   glWindowPos2iARB(5, 5);
   PrintString(s);
}


static int
SetFogMode(GLint fogMode)
{
   fogMode &= 3;
   switch (fogMode) {
   case 0:
      ModeStr = "Off";
      glDisable(GL_FOG);
      break;
   case 1:
      ModeStr = "GL_LINEAR";
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_LINEAR);
      glFogf(GL_FOG_START, fogStart);
      glFogf(GL_FOG_END, fogEnd);
      break;
   case 2:
      ModeStr = "GL_EXP";
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_EXP);
      glFogf(GL_FOG_DENSITY, fogDensity);
      break;
   case 3:
      ModeStr = "GL_EXP2";
      glEnable(GL_FOG);
      glFogi(GL_FOG_MODE, GL_EXP2);
      glFogf(GL_FOG_DENSITY, fogDensity);
      break;
   }
   return fogMode;
}


static GLboolean
SetFogCoord(GLboolean fogCoord)
{
   if (!GLEW_EXT_fog_coord) {
      return GL_FALSE;
   }

   if (fogCoord) {
      glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
   }
   else {
      glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
   }
   return fogCoord;
}


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
   {-1.0f,-1.0f,-DEPTH}, { 1.0f,-1.0f,-DEPTH}, { 1.0f, 1.0f,-DEPTH}, {-1.0f, 1.0f,-DEPTH},

   /* Floor */
   {-1.0f,-1.0f,-DEPTH}, { 1.0f,-1.0f,-DEPTH}, { 1.0f,-1.0f, 0.0}, {-1.0f,-1.0f, 0.0},

   /* Roof */
   {-1.0f, 1.0f,-DEPTH}, { 1.0f, 1.0f,-DEPTH}, { 1.0f, 1.0f, 0.0}, {-1.0f, 1.0f, 0.0},

   /* Right */
   { 1.0f,-1.0f, 0.0}, { 1.0f, 1.0f, 0.0}, { 1.0f, 1.0f,-DEPTH}, { 1.0f,-1.0f,-DEPTH},

   /* Left */
   {-1.0f,-1.0f, 0.0}, {-1.0f, 1.0f, 0.0}, {-1.0f, 1.0f,-DEPTH}, {-1.0f,-1.0f,-DEPTH}
};

static GLfloat texcoord_pointer[][2] = {
   /* Back */
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

   /* Floor */
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, DEPTH}, {0.0f, DEPTH},

   /* Roof */
   {1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, DEPTH}, {1.0f, DEPTH},

   /* Right */
   {0.0f, 1.0f}, {0.0f, 0.0f}, {DEPTH, 0.0f}, {DEPTH, 1.0f},

   /* Left */
   {0.0f, 0.0f}, {0.0f, 1.0f}, {DEPTH, 1.0f}, {DEPTH, 0.0f}
};

static GLfloat fogcoord_pointer[] = {
   /* Back */
   DEPTH, DEPTH, DEPTH, DEPTH,

   /* Floor */
   DEPTH, DEPTH, 0.0, 0.0,

   /* Roof */
   DEPTH, DEPTH, 0.0, 0.0,

   /* Right */
   0.0, 0.0, DEPTH, DEPTH,

   /* Left */
   0.0, 0.0, DEPTH, DEPTH
};


static void
Display( void )
{
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity ();
   
   glTranslatef(0.0f, 0.0f, -camz);

   SetFogMode(fogMode);

   glColor3f(1, 1, 1);

   if (Texture)
      glEnable(GL_TEXTURE_2D);

   if (Arrays) {
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glDrawElements(GL_QUADS, sizeof(vertex_index) / sizeof(vertex_index[0]),
                     GL_UNSIGNED_INT, vertex_index);
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   }
   else {
      /* Back */
      glBegin(GL_QUADS);
      glFogCoordf_ext(DEPTH); glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,-1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,-1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f,-DEPTH);
      glEnd();

      /* Floor */
      glBegin(GL_QUADS);
      glFogCoordf_ext(DEPTH); glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,-1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,-1.0f,-DEPTH);
      glFogCoordf_ext(0.0f); glTexCoord2f(1.0f,  DEPTH); glVertex3f( 1.0f,-1.0f,0.0);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f,  DEPTH); glVertex3f(-1.0f,-1.0f,0.0);
      glEnd();

      /* Roof */
      glBegin(GL_QUADS);
      glFogCoordf_ext(DEPTH); glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, 1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, 1.0f,-DEPTH);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, DEPTH); glVertex3f( 1.0f, 1.0f,0.0);
      glFogCoordf_ext(0.0f); glTexCoord2f(1.0f, DEPTH); glVertex3f(-1.0f, 1.0f,0.0);
      glEnd();

      /* Right */
      glBegin(GL_QUADS);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,-1.0f,0.0);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, 1.0f,0.0);
      glFogCoordf_ext(DEPTH); glTexCoord2f(DEPTH, 0.0f); glVertex3f( 1.0f, 1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(DEPTH, 1.0f); glVertex3f( 1.0f,-1.0f,-DEPTH);
      glEnd();

      /* Left */
      glBegin(GL_QUADS);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,-1.0f,0.0);
      glFogCoordf_ext(0.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f,0.0);
      glFogCoordf_ext(DEPTH); glTexCoord2f(DEPTH, 1.0f); glVertex3f(-1.0f, 1.0f,-DEPTH);
      glFogCoordf_ext(DEPTH); glTexCoord2f(DEPTH, 0.0f); glVertex3f(-1.0f,-1.0f,-DEPTH);
      glEnd();
   }

   glDisable(GL_TEXTURE_2D);

   PrintInfo();

   glutSwapBuffers();
}


static void
Reshape( int width, int height )
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1, 1, -1, 1, 1.0, 100);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Arrays = !Arrays;
         break;
      case 'f':
      case 'm':
         fogMode = SetFogMode(fogMode + 1);
         break;
      case 'D':
         fogDensity += 0.05;
         SetFogMode(fogMode);
         break;
      case 'd':
         if (fogDensity > 0.0) {
            fogDensity -= 0.05;
         }
         SetFogMode(fogMode);
         break;
      case 's':
         if (fogStart > 0.0) {
            fogStart -= 0.25;
         }
         SetFogMode(fogMode);
         break;
      case 'S':
         if (fogStart < 100.0) {
            fogStart += 0.25;
         }
         SetFogMode(fogMode);
         break;
      case 'e':
         if (fogEnd > 0.0) {
            fogEnd -= 0.25;
         }
         SetFogMode(fogMode);
         break;
      case 'E':
         if (fogEnd < 100.0) {
            fogEnd += 0.25;
         }
         SetFogMode(fogMode);
         break;
      case 'c':
	 fogCoord = SetFogCoord(fogCoord ^ GL_TRUE);
         break;
      case 't':
         Texture = !Texture;
         break;
      case 'z':
         camz -= 0.1;
         break;
      case 'Z':
         camz += 0.1;
         break;
      case 'r':
         Reset();
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   static const GLubyte teximage[2][2][4] = {
      { { 255, 255, 255, 255}, { 128, 128, 128, 255} },
      { { 128, 128, 128, 255}, { 255, 255, 255, 255} }
   };

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   if (!GLEW_EXT_fog_coord) {
      printf("GL_EXT_fog_coord not supported!\n");
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, teximage);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

   glFogfv(GL_FOG_COLOR, fogColor);
   glHint(GL_FOG_HINT, GL_NICEST);
   fogCoord = SetFogCoord(GL_TRUE); /* try to enable fog_coord */
   fogMode = SetFogMode(1);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, vertex_pointer);

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, 0, texcoord_pointer);

   if (GLEW_EXT_fog_coord) {
      glEnableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
      glFogCoordPointerEXT(GL_FLOAT, 0, fogcoord_pointer);
   }

   Reset();
}


int
main( int argc, char *argv[] )
{
   glutInitWindowSize( 600, 600 );
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
