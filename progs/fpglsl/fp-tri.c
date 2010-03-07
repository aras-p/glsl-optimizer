
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

static GLuint fragShader;
static GLuint vertShader;
static GLuint program;


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




static void load_and_compile_shader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader(shader);

   glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "fp-tri: problem compiling shader:\n%s\n", log);
      exit(1);
   }
}

static void read_shader(GLuint shader, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   FILE *f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "fp-tri: Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("fp-tri: read %d bytes from shader file %s\n", n, filename);
   if (n > 0) {
      buffer[n] = 0;
      load_and_compile_shader(shader, buffer);
   }

   fclose(f);
   free(buffer);
}

static void check_link(GLuint prog)
{
   GLint stat;
   glGetProgramiv(prog, GL_LINK_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetProgramInfoLog(prog, 1000, &len, log);
      fprintf(stderr, "Linker error:\n%s\n", log);
   }
}

static void setup_uniforms()
{
   {
      GLint loc1f = glGetUniformLocationARB(program, "Offset1f");
      GLint loc2f = glGetUniformLocationARB(program, "Offset2f");
      GLint loc4f = glGetUniformLocationARB(program, "Offset4f");
      GLfloat vecKer[] =
         { 1.0, 0.0, 0.0,  1.0,
           0.0, 1.0, 0.0,  1.0,
           1.0, 0.0, 0.0,  1.0,
           0.0, 0.0, 0.0,  1.0
         };
      if (loc1f >= 0)
         glUniform1fv(loc1f, 16, vecKer);

      if (loc2f >= 0)
         glUniform2fv(loc2f, 8, vecKer);

      if (loc4f >= 0)
         glUniform4fv(loc4f, 4, vecKer);

   }

   {
      GLint loc1f = glGetUniformLocationARB(program, "KernelValue1f");
      GLint loc2f = glGetUniformLocationARB(program, "KernelValue2f");
      GLint loc4f = glGetUniformLocationARB(program, "KernelValue4f");
      GLfloat vecKer[] =
         { 1.0, 0.0, 0.0,  0.25,
           0.0, 1.0, 0.0,  0.25,
           0.0, 0.0, 1.0,  0.25,
           0.0, 0.0, 0.0,  0.25,
           0.5, 0.0, 0.0,  0.35,
           0.0, 0.5, 0.0,  0.35,
           0.0, 0.0, 0.5,  0.35,
           0.0, 0.0, 0.0,  0.35
         };
      if (loc1f >= 0)
         glUniform1fv(loc1f, 16, vecKer);

      if (loc2f >= 0)
         glUniform2fv(loc2f, 8, vecKer);

      if (loc4f >= 0)
         glUniform4fv(loc4f, 4, vecKer);
   }
}

static void prepare_shaders()
{
   static const char *fragShaderText =
      "void main() {\n"
      "    gl_FragColor = gl_Color;\n"
      "}\n";
   static const char *vertShaderText =
      "void main() {\n"
      "   gl_FrontColor = gl_Color;\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "}\n";
   fragShader = glCreateShader(GL_FRAGMENT_SHADER);
   if (filename)
      read_shader(fragShader, filename);
   else
      load_and_compile_shader(fragShader, fragShaderText);


   vertShader = glCreateShader(GL_VERTEX_SHADER);
   load_and_compile_shader(vertShader, vertShaderText);

   program = glCreateProgram();
   glAttachShader(program, fragShader);
   glAttachShader(program, vertShader);
   glLinkProgram(program);
   check_link(program);
   glUseProgram(program);

   setup_uniforms();
}

#define LEVELS 8
#define SIZE (1<<LEVELS)
static int TexWidth = SIZE, TexHeight = SIZE;


static void
ResetTextureLevel( int i )
{
   GLubyte tex2d[SIZE*SIZE][4];
      
   {
      GLint Width = TexWidth / (1 << i);
      GLint Height = TexHeight / (1 << i);
      GLint s, t;
         
      for (s = 0; s < Width; s++) {
         for (t = 0; t < Height; t++) {
            tex2d[t*Width+s][0] = ((s / 16) % 2) ? 0 : 255;
            tex2d[t*Width+s][1] = ((t / 16) % 2) ? 0 : 255;
            tex2d[t*Width+s][2] = 128;
            tex2d[t*Width+s][3] = 255;
         }
      }
         
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         
      glTexImage2D(GL_TEXTURE_2D, i, GL_RGB, Width, Height, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, tex2d);
   }
}


static void
ResetTexture( void )
{
   int i;
      
   for (i = 0; i <= LEVELS; i++)
   {
      ResetTextureLevel(i);
   }
}

static void Init( void )
{
   GLuint Texture;

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

   glGenTextures(1, &Texture);
   glActiveTextureARB(GL_TEXTURE0_ARB + 2);
   glBindTexture(GL_TEXTURE_2D, Texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   ResetTexture();

   glClearColor(.1, .3, .5, 0);
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
}





static void Reshape(int width, int height)
{

    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

static void CleanUp(void)
{
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
}

static void Key(unsigned char key, int x, int y)
{

   switch (key) {
   case 27:
      CleanUp();
      exit(1);
   default:
      break;
   }

   glutPostRedisplay();
}

static void Display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glUseProgram(program);
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
   glutCreateWindow(filename ? filename : "fp-tri");
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   prepare_shaders();
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
