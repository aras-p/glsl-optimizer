
/*
 * Test textured lines.
 *
 * Brian Paul
 * September 2000
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/girl.rgb"

static GLboolean Antialias = GL_FALSE;
static GLboolean Animate = GL_FALSE;
static GLint Texture = 1;
static GLboolean Stipple = GL_FALSE;
static GLfloat LineWidth = 1.0;

static GLfloat Xrot = -60.0, Yrot = 0.0, Zrot = 0.0;
static GLfloat DYrot = 1.0;
static GLboolean Points = GL_FALSE;
static GLfloat Scale = 1.0;

static void Idle( void )
{
   if (Animate) {
      Zrot += DYrot;
      glutPostRedisplay();
   }
}


static void Display( void )
{
   GLfloat x, y, s, t;

   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1.0, 0.0, 0.0);
   glRotatef(Yrot, 0.0, 1.0, 0.0);
   glRotatef(Zrot, 0.0, 0.0, 1.0);
   glScalef(Scale, Scale, Scale);

   if (Texture)
      glColor3f(1, 1, 1);

   if (Points) {
      glBegin(GL_POINTS);
      for (t = 0.0; t <= 1.0; t += 0.025) {
         for (s = 0.0; s <= 1.0; s += 0.025) {
            x = s * 2.0 - 1.0;
            y = t * 2.0 - 1.0;
            if (!Texture)
               glColor3f(1, 0, 1);
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, t, s);
            glTexCoord2f(s, t);
            glVertex2f(x, y);
         }
      }
      glEnd();
   }
   else {
      glBegin(GL_LINES);
      for (t = 0.0; t <= 1.0; t += 0.025) {
         x = t * 2.0 - 1.0;
         if (!Texture)
            glColor3f(1, 0, 1);
         glTexCoord2f(t, 0.0);
         glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, t);
         glVertex2f(x, -1.0);
         if (!Texture)
            glColor3f(0, 1, 0);
         glTexCoord2f(t, 1.0);
         glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, t);
         glVertex2f(x, 1.0);
      }
      glEnd();
   }

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   GLfloat ar = (float) width / height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, 10.0, 100.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -12.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Antialias = !Antialias;
         if (Antialias) {
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         }
         else {
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POINT_SMOOTH);
            glDisable(GL_BLEND);
         }
         break;
      case 't':
         Texture++;
         if (Texture > 2)
            Texture = 0;
         if (Texture == 0) {
            glActiveTextureARB(GL_TEXTURE0_ARB);
            glDisable(GL_TEXTURE_2D);
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_2D);
         }
         else if (Texture == 1) {
            glActiveTextureARB(GL_TEXTURE0_ARB);
            glEnable(GL_TEXTURE_2D);
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_2D);
         }
         else {
            glActiveTextureARB(GL_TEXTURE0_ARB);
            glEnable(GL_TEXTURE_2D);
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glEnable(GL_TEXTURE_2D);
         }
         break;
      case 'w':
         LineWidth -= 0.25;
         if (LineWidth < 0.25)
            LineWidth = 0.25;
         glLineWidth(LineWidth);
         glPointSize(LineWidth);
         break;
      case 'W':
         LineWidth += 0.25;
         if (LineWidth > 8.0)
            LineWidth = 8.0;
         glLineWidth(LineWidth);
         glPointSize(LineWidth);
         break;
      case 'p':
         Points = !Points;
         break;
      case 's':
         Stipple = !Stipple;
         if (Stipple)
            glEnable(GL_LINE_STIPPLE);
         else
            glDisable(GL_LINE_STIPPLE);
         break;
      case ' ':
         Animate = !Animate;
         if (Animate)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 27:
         exit(0);
         break;
   }
   printf("LineWidth, PointSize = %f\n", LineWidth);
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   float step = 3.0;
   (void) x;
   (void) y;

   switch (key) {
      case GLUT_KEY_UP:
         Xrot += step;
         break;
      case GLUT_KEY_DOWN:
         Xrot -= step;
         break;
      case GLUT_KEY_LEFT:
         Yrot += step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot -= step;
         break;
   }
   glutPostRedisplay();
}


static void Init( int argc, char *argv[] )
{
   GLfloat r[2];
   GLuint u;

   for (u = 0; u < 2; u++) {
      glActiveTextureARB(GL_TEXTURE0_ARB + u);
      glBindTexture(GL_TEXTURE_2D, 10+u);
      if (u == 0)
         glEnable(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      if (u == 0)
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      else
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
         printf("Error: couldn't load texture image\n");
         exit(1);
      }
   }

   glLineStipple(1, 0xff);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, r);
   printf("Non-smooth point size range: %g .. %g\n", r[0], r[1]);
   glGetFloatv(GL_POINT_SIZE_RANGE, r);
   printf("Smoothed point size range: %g .. %g\n", r[0], r[1]);
   glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, r);
   printf("Non-smooth line width range: %g .. %g\n", r[0], r[1]);
   glGetFloatv(GL_LINE_WIDTH_RANGE, r);
   printf("Smoothed line width range: %g .. %g\n", r[0], r[1]);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition(0, 0);
   glutInitWindowSize( 400, 300 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   glutCreateWindow(argv[0] );
   glewInit();

   Init(argc, argv);

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   if (Animate)
      glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
