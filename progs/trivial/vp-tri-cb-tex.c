/* Test glGenProgramsNV(), glIsProgramNV(), glLoadProgramNV() */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>



GLenum doubleBuffer;

static void Init(void)
{
   GLint errno;
   GLuint prognum;
   
   static const char *prog1 =
      "!!ARBvp1.0\n"
      "PARAM Emission = state.material.emission; \n"
      "PARAM Ambient = state.material.ambient; \n"
      "PARAM Diffuse = state.material.diffuse; \n"
      "PARAM Specular = state.material.specular; \n"
      "DP4  result.position.x, Ambient, vertex.position;\n"
      "DP4  result.position.y, Diffuse, vertex.position;\n"
      "DP4  result.position.z, Specular, vertex.position;\n"
      "DP4  result.position.w, Emission, vertex.position;\n"
      "MOV  result.texcoord[0], vertex.texcoord[0];\n"
      "END\n";

   const float Ambient[4] = { 0.0, 1.0, 0.0, 0.0 };
   const float Diffuse[4] = { 1.0, 0.0, 0.0, 0.0 };
   const float Specular[4] = { 0.0, 0.0, 1.0, 0.0 };
   const float Emission[4] = { 0.0, 0.0, 0.0, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Ambient);
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Diffuse);
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
   glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Emission);


   glGenProgramsARB(1, &prognum);

   glBindProgramARB(GL_VERTEX_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
		      strlen(prog1), (const GLubyte *) prog1);

   assert(glIsProgramARB(prognum));
   errno = glGetError();
   printf("glGetError = %d\n", errno);
   if (errno != GL_NO_ERROR)
   {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("%s\n", (char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }

   glEnable(GL_VERTEX_PROGRAM_NV);

#define SIZE 32
   {
      GLubyte tex2d[SIZE][SIZE][3];
      GLint s, t;

      for (s = 0; s < SIZE; s++) {
	 for (t = 0; t < SIZE; t++) {
#if 0
	    tex2d[t][s][0] = (s < SIZE/2) ? 0 : 255;
	    tex2d[t][s][1] = (t < SIZE/2) ? 0 : 255;
	    tex2d[t][s][2] = 0;
#else
	    tex2d[t][s][0] = s*255/(SIZE-1);
	    tex2d[t][s][1] = t*255/(SIZE-1);
	    tex2d[t][s][2] = 0;
#endif
	 }
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      glTexImage2D(GL_TEXTURE_2D, 0, 3, SIZE, SIZE, 0,
                   GL_RGB, GL_UNSIGNED_BYTE, tex2d);

      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   }

}


static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
/*     glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0); */
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      default:
	break;
    }

    glutPostRedisplay();
}

static void Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT); 

   glBegin(GL_TRIANGLES);
   glTexCoord2f(1,-1); 
   glVertex3f( 0.9, -0.9, -0.0);
   glTexCoord2f(1,1); 
   glVertex3f( 0.9,  0.9, -0.0);
   glTexCoord2f(-1,0); 
   glVertex3f(-0.9,  0.0, -0.0);
   glEnd();

   glFlush();

   if (doubleBuffer) {
      glutSwapBuffers();
   }
}

static GLenum Args(int argc, char **argv)
{
    GLint i;

    doubleBuffer = GL_FALSE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-sb") == 0) {
	    doubleBuffer = GL_FALSE;
	} else if (strcmp(argv[i], "-db") == 0) {
	    doubleBuffer = GL_TRUE;
	} else {
	    fprintf(stderr, "%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB | GLUT_ALPHA;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow(*argv) == GL_FALSE) {
	exit(1);
    }

    glewInit();

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
	return 0;
}
