/* $Id: texline.c,v 1.1 2000/09/30 18:48:33 brianp Exp $ */

/*
 * Test textured lines.
 *
 * Brian Paul
 * September 2000
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/girl.rgb"

static GLboolean Antialias = GL_FALSE;
static GLboolean Animate = GL_TRUE;
static GLboolean Texture = GL_TRUE;
static GLfloat LineWidth = 1.0;

static GLfloat Xrot = -60.0, Yrot = 0.0, Zrot = 0.0;
static GLfloat DYrot = 1.0;


static void Idle( void )
{
   if (Animate) {
      Zrot += DYrot;
      glutPostRedisplay();
   }
}


static void Display( void )
{
   GLfloat x, t;

   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1.0, 0.0, 0.0);
   glRotatef(Yrot, 0.0, 1.0, 0.0);
   glRotatef(Zrot, 0.0, 0.0, 1.0);

   glColor3f(1, 1, 1);
   glBegin(GL_LINES);
   for (t = 0.0; t <= 1.0; t += 0.0125) {
      x = t * 2.0 - 1.0;
      glTexCoord2f(t, 0.0);  glVertex2f(x, -1.0);
      glTexCoord2f(t, 1.0);  glVertex2f(x, 1.0);
   }
   glEnd();

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
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         }
         else {
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_BLEND);
         }
         break;
      case 't':
         Texture = !Texture;
         if (Texture)
            glEnable(GL_TEXTURE_2D);
         else
            glDisable(GL_TEXTURE_2D);
         break;
      case 'w':
         LineWidth -= 0.25;
         if (LineWidth < 0.25)
            LineWidth = 0.25;
         glLineWidth(LineWidth);
         break;
      case 'W':
         LineWidth += 0.25;
         if (LineWidth > 8.0)
            LineWidth = 8.0;
         glLineWidth(LineWidth);
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
   printf("Width %f\n", LineWidth);
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
   glEnable(GL_TEXTURE_2D);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 400, 300 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   glutCreateWindow(argv[0] );

   Init(argc, argv);

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
