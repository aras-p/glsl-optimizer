
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <signal.h>
#endif

#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.c"


#define TEXTURE_FILE "../images/bw.rgb"

unsigned show_fps = 0;
unsigned int frame_cnt = 0;
void alarmhandler(int);
static const char *filename = NULL;

static void usage(char *name)
{
   fprintf(stderr, "usage: %s [ options ] shader_filename\n", name);
#ifndef WIN32
   fprintf(stderr, "\n" );
   fprintf(stderr, "options:\n");
   fprintf(stderr, "    -fps  show frames per second\n");
#endif
}

#ifndef WIN32
void alarmhandler (int sig)
{
   if (sig == SIGALRM) {
      printf("%d frames in 5.0 seconds = %.3f FPS\n", frame_cnt,
             frame_cnt / 5.0);

      frame_cnt = 0;
   }
   signal(SIGALRM, alarmhandler);
   alarm(5);
}
#endif

static void args(int argc, char *argv[])
{
   GLint i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-fps") == 0) {
         show_fps = 1;
      }
      else if (i == argc - 1) {
	 filename = argv[i];
      }
      else {
	 usage(argv[0]);
	 exit(1);
      }
   }

   if (!filename) {
      usage(argv[0]);
      exit(1);
   }
}

static void Init( void )
{
   GLuint Texture;
   GLint errno;
   GLuint prognum;
   char buf[4096];
   GLuint sz;
   FILE *f;

   if ((f = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "Couldn't open %s\n", filename);
      exit(1);
   }

   sz = fread(buf, 1, sizeof(buf), f);
   if (!feof(f)) {
      fprintf(stderr, "file too long\n");
      exit(1);
   }
   fprintf(stderr, "%.*s\n", sz, buf);

   if (!GLEW_ARB_fragment_program) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   /* Setup the fragment program */
   glGenProgramsARB(1, &prognum);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, prognum);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                      sz, (const GLubyte *)buf);

   errno = glGetError();
   printf("glGetError = 0x%x\n", errno);
   if (errno != GL_NO_ERROR) {
      GLint errorpos;

      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
      printf("errorpos: %d\n", errorpos);
      printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
             (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
   }
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


   glGenTextures(1, &Texture);
   glActiveTextureARB(GL_TEXTURE0_ARB + 1);
   glBindTexture(GL_TEXTURE_2D, Texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   {
      GLubyte data[32][32];
      int width = 32;
      int height = 32;
      int i;
      int j;

      for (i = 0; i < 32; i++)
         for (j = 0; j < 32; j++)
	 {
	    /**
	     ** +-----------+
	     ** |     W     |
	     ** |  +-----+  |
	     ** |  |     |  |
	     ** |  |  B  |  |
	     ** |  |     |  |
	     ** |  +-----+  |
	     ** |           |
	     ** +-----------+
	     **/
	    int i2 = i - height / 2;
	    int j2 = j - width / 2;
	    int h8 = height / 8;
	    int w8 = width / 8;
	    if ( -h8 <= i2 && i2 <= h8 && -w8 <= j2 && j2 <= w8 ) {
	       data[i][j] = 0x00;
	    } else if ( -2 * h8 <= i2 && i2 <= 2 * h8 && -2 * w8 <= j2 && j2 <= 2 * w8 ) {
	       data[i][j] = 0x55;
	    } else if ( -3 * h8 <= i2 && i2 <= 3 * h8 && -3 * w8 <= j2 && j2 <= 3 * w8 ) {
	       data[i][j] = 0xaa;
	    } else {
	       data[i][j] = 0xff;
	    }
	 }

      glTexImage2D( GL_TEXTURE_2D, 0,
                    GL_ALPHA8,
                    32, 32, 0,
                    GL_ALPHA, GL_UNSIGNED_BYTE, data );
   }


   glClearColor(.1, .3, .5, 0);
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
	return;
    }

    glutPostRedisplay();
}

static void Display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, 1.0, 1.0, 0.0, 0.0);
   glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, 0.0, 0.0, 1.0, 1.0);
   glBegin(GL_TRIANGLES);

   glColor3f(0,0,1);
   glTexCoord3f(1,1,0);
   glVertex3f( 0.9, -0.9, -30.0);

   glColor3f(1,0,0);
   glTexCoord3f(1,-1,0);
   glVertex3f( 0.9,  0.9, -30.0);

   glColor3f(0,1,0);
   glTexCoord3f(-1,0,0);
   glVertex3f(-0.9,  0.0, -30.0);
   glEnd();

   glFlush();
   if (show_fps) {
      ++frame_cnt;
      glutPostRedisplay();
   }
}


int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(250, 250);
   glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH);
   args(argc, argv);
   glutCreateWindow(filename);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
#ifndef WIN32
   if (show_fps) {
      signal(SIGALRM, alarmhandler);
      alarm(5);
   }
#endif
   glutMainLoop();
   return 0;
}
