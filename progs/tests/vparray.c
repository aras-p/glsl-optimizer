/*
 * Test vertex arrays with GL_NV_vertex_program
 *
 * Based on a stripped-down version of the isosurf demo.
 * The vertex program is trivial: compute the resulting
 * RGB color as a linear function of vertex XYZ.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "GL/glew.h"
#include "GL/glut.h"

#define MAXVERTS 10000
static float data[MAXVERTS][6];
static GLint numverts;

static GLfloat xrot;
static GLfloat yrot;
static GLboolean useArrays = GL_TRUE;
static GLboolean useProgram = GL_TRUE;
static GLboolean useList = GL_FALSE;


static void read_surface( char *filename )
{
   FILE *f;

   f = fopen(filename,"r");
   if (!f) {
      printf("couldn't read %s\n", filename);
      exit(1);
   }

   numverts = 0;
   while (!feof(f) && numverts < MAXVERTS) {
      fscanf( f, "%f %f %f  %f %f %f",
	      &data[numverts][0], &data[numverts][1], &data[numverts][2],
	      &data[numverts][3], &data[numverts][4], &data[numverts][5] );
      numverts++;
   }
   numverts--;

   printf("%d vertices, %d triangles\n", numverts, numverts-2);
   printf("data = %p\n", (void *) data);
   fclose(f);
}




static void Display(void)
{
   if (useProgram)
      glEnable(GL_VERTEX_PROGRAM_NV);
   else
      glDisable(GL_VERTEX_PROGRAM_NV);

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
      glRotatef(xrot, 1, 0, 0);
      glRotatef(yrot, 0, 1, 0);
      glScalef(2, 2, 2);
      if (useArrays) {
         if (useProgram) {
            glVertexAttribPointerNV( 0, 3, GL_FLOAT, 6 * sizeof(GLfloat), data );
            glEnableClientState( GL_VERTEX_ATTRIB_ARRAY0_NV );
            glVertexAttribPointerNV( 2, 3, GL_FLOAT, 6 * sizeof(GLfloat), ((GLfloat *) data) + 3);
            glEnableClientState( GL_VERTEX_ATTRIB_ARRAY2_NV);
         }
         else {
            glVertexPointer( 3, GL_FLOAT, 6 * sizeof(GLfloat), data );
            glEnableClientState( GL_VERTEX_ARRAY );
            glNormalPointer( GL_FLOAT, 6 * sizeof(GLfloat), ((GLfloat *) data) + 3);
            glEnableClientState( GL_NORMAL_ARRAY );
         }

         if (useList) {
            /* dumb, but a good test */
            glNewList(1,GL_COMPILE);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, numverts);
            glEndList();
            glCallList(1);
         }
         else {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, numverts);
         }

         glDisableClientState( GL_VERTEX_ATTRIB_ARRAY0_NV );
         glDisableClientState( GL_VERTEX_ATTRIB_ARRAY2_NV);
         glDisableClientState( GL_VERTEX_ARRAY );
         glDisableClientState( GL_NORMAL_ARRAY );
      }
      else {
         int i;
         glBegin(GL_TRIANGLE_STRIP);
         for (i = 0; i < numverts; i++) {
            glNormal3fv( data[i] + 3 );
            glVertex3fv( data[i] + 0 );
         }
         glEnd();
      }
   glPopMatrix();

   if (glGetError())
      printf("Error!\n");

   glutSwapBuffers();
}


static void InitMaterials(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position0[] = {0.0, 0.0, 20.0, 0.0};
    static float position1[] = {0.0, 0.0, -20.0, 0.0};
    static float front_mat_shininess[] = {60.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    /*
    static float back_mat_shininess[] = {60.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    */
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_FALSE};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position0);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);
    glEnable(GL_LIGHT1);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, front_mat_diffuse);
    glEnable(GL_LIGHTING);
}


static void init_program(void)
{
   /*
    * c[0..3] = modelview matrix
    * c[4..7] = inverse modelview matrix
    * c[30] = color scale
    * c[31] = color bias
    */
   static const char prog[] = 
      "!!VP1.0\n"

      "# RGB is proportional to XYZ \n"

      "MUL R0, v[OPOS], c[30]; \n"
      "ADD o[COL0], R0, c[31]; \n"

      "# Continue with typical modelview/projection\n"
      "MOV R3, v[OPOS]; \n"
      "DP4   o[HPOS].x, c[0], R3 ;	# object x MVP -> clip\n"
      "DP4   o[HPOS].y, c[1], R3 ;\n"
      "DP4   o[HPOS].z, c[2], R3 ;\n"
      "DP4   o[HPOS].w, c[3], R3 ;\n"

      "END";

   static const GLfloat scale[4] = {2.0, 2.0, 2.0, 0.0};
   static const GLfloat bias[4] = {1.0, 1.0, 1.0, 0.0};

   if (!glutExtensionSupported("GL_NV_vertex_program")) {
      printf("Sorry, this program requires GL_NV_vertex_program");
      exit(1);
   }

   glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 1,
                   strlen(prog), (const GLubyte *) prog);
   assert(glIsProgramNV(1));
   glBindProgramNV(GL_VERTEX_PROGRAM_NV, 1);

   /* Load the program registers */
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
   glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 4, GL_MODELVIEW, GL_INVERSE_TRANSPOSE_NV);

   glProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, 30, scale);
   glProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, 31, bias);
}


static void init(void)
{
   xrot = 0;
   yrot = 0;
   glClearColor(0.0, 0.0, 1.0, 0.0);
   glEnable( GL_DEPTH_TEST );
   glEnable(GL_NORMALIZE);
   InitMaterials();
   read_surface( "../demos/isosurf.dat" );
   init_program();
}


static void Reshape(int width, int height)
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5, 25 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -15);
}



static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case 27:
      exit(0);
   case 'a':
      useArrays = !useArrays;
      printf("use arrays: %s\n", useArrays ? "yes" : "no");
      break;
   case 'l':
      useList = !useList;
      printf("use list: %s\n", useList ? "yes" : "no");
      break;
   case 'p':
      useProgram = !useProgram;
      printf("use program: %s\n", useProgram ? "yes" : "no");
      break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_LEFT:
      yrot -= 15.0;
      break;
   case GLUT_KEY_RIGHT:
      yrot += 15.0;
      break;
   case GLUT_KEY_UP:
      xrot += 15.0;
      break;
   case GLUT_KEY_DOWN:
      xrot -= 15.0;
      break;
   default:
      return;
   }
   glutPostRedisplay();
}



int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode( GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE );
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   if (glutCreateWindow("Isosurface") <= 0) {
      exit(0);
   }
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Display);

   init();

   glutMainLoop();
   return 0;
}
