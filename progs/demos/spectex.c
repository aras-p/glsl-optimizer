
/*
 * GLUT demonstration of texturing with specular highlights.
 *
 * When drawing a lit, textured surface one usually wants the specular
 * highlight to override the texture colors.  However, OpenGL applies
 * texturing after lighting so the specular highlight is modulated by
 * the texture.
 *
 * The solution here shown here is a two-pass algorithm:
 *  1. Draw the textured surface without specular lighting.
 *  2. Enable blending to add the next pass:
 *  3. Redraw the surface with a matte white material and only the
 *     specular components of light sources enabled.
 *
 * Brian Paul  February 1997
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static GLUquadricObj *Quadric;
static GLuint Sphere;
static GLfloat LightPos[4] = {10.0, 10.0, 10.0, 1.0};
static GLfloat Delta = 20.0;
static GLint Mode = 4;

/*static GLfloat Blue[4] = {0.0, 0.0, 1.0, 1.0};*/
/*static GLfloat Gray[4] = {0.5, 0.5, 0.5, 1.0};*/
static GLfloat Black[4] = {0.0, 0.0, 0.0, 1.0};
static GLfloat White[4] = {1.0, 1.0, 1.0, 1.0};

static GLboolean smooth = 1;

static void
Idle(void)
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;
   LightPos[0] += Delta * dt;
   if (LightPos[0]>15.0 || LightPos[0]<-15.0)
      Delta = -Delta;

   glutPostRedisplay();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glLightfv(GL_LIGHT0, GL_POSITION, LightPos);

   glPushMatrix();
   glRotatef(90.0, 1.0, 0.0, 0.0);

   if (Mode==0) {
      /* Typical method: diffuse + specular + texture */
      glEnable(GL_TEXTURE_2D);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, White);  /* enable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, White);  /* enable specular */
#ifdef GL_VERSION_1_2
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
#endif
      glCallList(Sphere);
   }
   else if (Mode==1) {
      /* just specular highlight */
      glDisable(GL_TEXTURE_2D);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, Black);  /* disable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, White);  /* enable specular */
#ifdef GL_VERSION_1_2
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
#endif
      glCallList(Sphere);
   }
   else if (Mode==2) {
      /* diffuse textured */
      glEnable(GL_TEXTURE_2D);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, White);  /* enable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, Black);  /* disable specular */
#ifdef GL_VERSION_1_2
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
#endif
      glCallList(Sphere);
   }
   else if (Mode==3) {
      /* 2-pass: diffuse textured then add specular highlight*/
      glEnable(GL_TEXTURE_2D);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, White);  /* enable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, Black);  /* disable specular */
#ifdef GL_VERSION_1_2
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
#endif
      glCallList(Sphere);
      /* specular highlight */
      glDepthFunc(GL_EQUAL);  /* redraw same pixels */
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);  /* add */
      glLightfv(GL_LIGHT0, GL_DIFFUSE, Black);  /* disable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, White);  /* enable specular */
      glCallList(Sphere);
      glDepthFunc(GL_LESS);
      glDisable(GL_BLEND);
   }
   else if (Mode==4) {
      /* OpenGL 1.2's separate diffuse and specular color */
      glEnable(GL_TEXTURE_2D);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, White);  /* enable diffuse */
      glLightfv(GL_LIGHT0, GL_SPECULAR, White);  /* enable specular */
#ifdef GL_VERSION_1_2
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif
      glCallList(Sphere);
   }

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -12.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case 27:
      exit(0);
      break;
   case 's':
      smooth = !smooth;
      if (smooth)
         glShadeModel(GL_SMOOTH);
      else
         glShadeModel(GL_FLAT);
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
         break;
      case GLUT_KEY_DOWN:
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   int i, j;
   GLubyte texImage[64][64][3];

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Black);

   glShadeModel(GL_SMOOTH);

   glMaterialfv(GL_FRONT, GL_DIFFUSE, White);
   glMaterialfv(GL_FRONT, GL_SPECULAR, White);
   glMaterialf(GL_FRONT, GL_SHININESS, 20.0);

   /* Actually, these are set again later */
   glLightfv(GL_LIGHT0, GL_DIFFUSE, White);
   glLightfv(GL_LIGHT0, GL_SPECULAR, White);

   Quadric = gluNewQuadric();
   gluQuadricTexture( Quadric, GL_TRUE );

   Sphere= glGenLists(1);
   glNewList( Sphere, GL_COMPILE );
   gluSphere( Quadric, 1.0, 24, 24 );
   glEndList();

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   for (i=0;i<64;i++) {
      for (j=0;j<64;j++) {
         int k = ((i>>3)&1) ^ ((j>>3)&1);
         texImage[i][j][0] = 255*k;
         texImage[i][j][1] = 255*(1-k);
         texImage[i][j][2] = 0;
      }
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage2D( GL_TEXTURE_2D,
                 0,
                 3,
                 64, 64,
                 0,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 texImage );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glEnable(GL_TEXTURE_2D);

   glBlendFunc(GL_ONE, GL_ONE);
}


static void ModeMenu(int entry)
{
   if (entry==99)
      exit(0);
   Mode = entry;
}


int main( int argc, char *argv[] )
{

   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 300, 300 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

   glutCreateWindow( "spectex" );

   Init();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutCreateMenu( ModeMenu );
   glutAddMenuEntry("1-pass lighting + texturing", 0);
   glutAddMenuEntry("specular lighting", 1);
   glutAddMenuEntry("diffuse lighting + texturing", 2);
   glutAddMenuEntry("2-pass lighting + texturing", 3);
#ifdef GL_VERSION_1_2
   glutAddMenuEntry("OpenGL 1.2 separate specular", 4);
#endif
   glutAddMenuEntry("Quit", 99);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
