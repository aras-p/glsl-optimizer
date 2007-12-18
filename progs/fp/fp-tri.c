
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <unistd.h>
#include <signal.h>

unsigned show_fps = 0;
unsigned int frame_cnt = 0;
void alarmhandler(int);
static const char *filename = NULL;

static void usage(char *name)
{
   fprintf(stderr, "usage: %s [ options ] shader_filename\n", name);
   fprintf(stderr, "\n" );
   fprintf(stderr, "options:\n");
   fprintf(stderr, "    -fps  show frames per second\n");
}

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

   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
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
	return;
    }

    glutPostRedisplay();
}

static void Display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glBegin(GL_TRIANGLES);
   glColor3f(0,0,1);
   glVertex3f( 0.9, -0.9, -30.0);
   glColor3f(1,0,0);
   glVertex3f( 0.9,  0.9, -30.0);
   glColor3f(0,1,0);
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
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   args(argc, argv);
   Init();
   if (show_fps) {
      signal(SIGALRM, alarmhandler);
      alarm(5);
   }
   glutMainLoop();
   return 0;
}
