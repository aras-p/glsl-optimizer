/*
 * Test floating point textures.
 * No actual rendering, yet.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


/* XXX - temporary */
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float 1
#define GL_TEXTURE_RED_TYPE_ARB             0x9000
#define GL_TEXTURE_GREEN_TYPE_ARB           0x9001
#define GL_TEXTURE_BLUE_TYPE_ARB            0x9002
#define GL_TEXTURE_ALPHA_TYPE_ARB           0x9003
#define GL_TEXTURE_LUMINANCE_TYPE_ARB       0x9004
#define GL_TEXTURE_INTENSITY_TYPE_ARB       0x9005
#define GL_TEXTURE_DEPTH_TYPE_ARB           0x9006
#define GL_UNSIGNED_NORMALIZED_ARB          0x9007
#define GL_RGBA32F_ARB                      0x8814
#define GL_RGB32F_ARB                       0x8815
#define GL_ALPHA32F_ARB                     0x8816
#define GL_INTENSITY32F_ARB                 0x8817
#define GL_LUMINANCE32F_ARB                 0x8818
#define GL_LUMINANCE_ALPHA32F_ARB           0x8819
#define GL_RGBA16F_ARB                      0x881A
#define GL_RGB16F_ARB                       0x881B
#define GL_ALPHA16F_ARB                     0x881C
#define GL_INTENSITY16F_ARB                 0x881D
#define GL_LUMINANCE16F_ARB                 0x881E
#define GL_LUMINANCE_ALPHA16F_ARB           0x881F
#endif


static GLboolean
CheckError( int line )
{
   GLenum error = glGetError();
   if (error) {
      char *err = (char *) gluErrorString( error );
      fprintf( stderr, "GL Error: %s at line %d\n", err, line );
      return GL_TRUE;
   }
   return GL_FALSE;
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();

   glutSolidCube(2.0);

   glPopMatrix();

   glutSwapBuffers();
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
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}



static void
Init(void)
{
   GLfloat tex[16][16][4];
   GLfloat tex2[16][16][4];
   GLint i, j, t;

   if (!glutExtensionSupported("GL_MESAX_texture_float")) {
      printf("Sorry, this test requires GL_MESAX_texture_float\n");
      exit(1);
   }

   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         GLfloat s = i / 15.0;
         tex[i][j][0] = s;
         tex[i][j][1] = 2.0 * s;
         tex[i][j][2] = -3.0 * s;
         tex[i][j][3] = 4.0 * s;
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, 16, 16, 0, GL_RGBA,
                GL_FLOAT, tex);
   CheckError(__LINE__);

   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_TYPE_ARB, &t);
   assert(t == GL_FLOAT);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_TYPE_ARB, &t);
   assert(t == GL_FLOAT);

   CheckError(__LINE__);

   /* read back the texture and make sure values are correct */
   glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, tex2);
   CheckError(__LINE__);
   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         if (tex[i][j][0] != tex2[i][j][0] ||
             tex[i][j][1] != tex2[i][j][1] ||
             tex[i][j][2] != tex2[i][j][2] ||
             tex[i][j][3] != tex2[i][j][3]) {
            printf("tex[%d][%d] %g %g %g %g != tex2[%d][%d] %g %g %g %g\n",
                   i, j,
                   tex[i][j][0], tex[i][j][1], tex[i][j][2], tex[i][j][3],
                   i, j,
                   tex2[i][j][0], tex2[i][j][1], tex2[i][j][2], tex2[i][j][3]);
         }
      }
   }


}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
