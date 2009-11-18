
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.c"


#define TEXTURE_FILE "../images/girl.rgb"



static void Init( void )
{
   static const char *modulate2D =
      "!!ARBfp1.0\n"
      "TEX result.color, fragment.color, texture[0], 2D; \n"
      "END"
      ;
   GLuint modulateProg;
   GLuint Texture;

   if (!GLEW_ARB_fragment_program) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Setup the fragment program */
   glGenProgramsARB(1, &modulateProg);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, modulateProg);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                        strlen(modulate2D), (const GLubyte *)modulate2D);

   printf("glGetError = 0x%x\n", (int) glGetError());
   printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
          (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   assert(glIsProgramARB(modulateProg));

   glEnable(GL_FRAGMENT_PROGRAM_ARB);

   /* Load texture */
   glGenTextures(1, &Texture);
   glBindTexture(GL_TEXTURE_2D, Texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image file %s\n", TEXTURE_FILE);
      exit(1);
   }
   /* XXX this enable shouldn't really be needed!!! */
   glEnable(GL_TEXTURE_2D);

   glClearColor(.3, .3, .3, 0);
}

static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
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
   glColor3f(0,0,1); 
/*    glTexCoord2f(1, 0); */
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(1,0,0); 
/*    glTexCoord2f(1, 1); */
   glVertex3f( 0.9,  0.9, -30.0);
   glColor3f(0,1,0); 
/*    glTexCoord2f(0, .5); */
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glFlush();
}


int main(int argc, char **argv)
{
    GLenum type;

    glutInit(&argc, argv);



    glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

    type = GLUT_RGB;
    type |= GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("First Tri") == GL_FALSE) {
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
