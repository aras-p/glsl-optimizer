
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/glut.h>

static const char *filename = NULL;
static GLuint nr_steps = 4;
static GLuint prim = GL_TRIANGLES;
static GLfloat psz = 1.0;
static GLboolean pointsmooth = 0;
static GLboolean program_point_size = 0;

static GLuint fragShader;
static GLuint vertShader;
static GLuint program;

static void usage( char *name )
{
   fprintf( stderr, "usage: %s [ options ] shader_filename\n", name );
   fprintf( stderr, "\n" );
   fprintf( stderr, "options:\n" );
   fprintf( stderr, "    -f     flat shaded\n" );
   fprintf( stderr, "    -nNr  subdivision steps\n" );
}


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
      fprintf(stderr, "vp-tris: problem compiling shader:\n%s\n", log);
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
      fprintf(stderr, "vp-tris: Unable to open shader file %s\n", filename);
      exit(1);
   }

   n = fread(buffer, 1, max, f);
   printf("vp-tris: read %d bytes from shader file %s\n", n, filename);
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
      GLuint loc1f = glGetUniformLocationARB(program, "Offset1f");
      GLuint loc2f = glGetUniformLocationARB(program, "Offset2f");
      GLuint loc4f = glGetUniformLocationARB(program, "Offset4f");
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
      GLuint loc1f = glGetUniformLocationARB(program, "KernelValue1f");
      GLuint loc2f = glGetUniformLocationARB(program, "KernelValue2f");
      GLuint loc4f = glGetUniformLocationARB(program, "KernelValue4f");
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
   load_and_compile_shader(fragShader, fragShaderText);


   vertShader = glCreateShader(GL_VERTEX_SHADER);
   if (filename)
      read_shader(vertShader, filename);
   else
      load_and_compile_shader(vertShader, vertShaderText);

   program = glCreateProgram();
   glAttachShader(program, fragShader);
   glAttachShader(program, vertShader);
   glLinkProgram(program);
   check_link(program);
   glUseProgram(program);

   setup_uniforms();
}

static void args(int argc, char *argv[])
{
   GLint i;

   for (i = 1; i < argc; i++) {
      if (strncmp(argv[i], "-n", 2) == 0) {
	 nr_steps = atoi((argv[i]) + 2);
      }
      else if (strcmp(argv[i], "-f") == 0) {
	 glShadeModel(GL_FLAT);
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




union vert {
   struct {
      GLfloat color[3];
      GLfloat pos[3];
   } v;
   GLfloat f[6];
};

static void make_midpoint( union vert *out,
			   const union vert *v0,
			   const union vert *v1)
{
   int i;
   for (i = 0; i < 6; i++)
      out->f[i] = v0->f[i] + .5 * (v1->f[i] - v0->f[i]);
}

static void subdiv( union vert *v0,
		    union vert *v1,
		    union vert *v2,
		    GLuint depth )
{
   if (depth == 0) {
      glColor3fv(v0->v.color);
      glVertex3fv(v0->v.pos);
      glColor3fv(v1->v.color);
      glVertex3fv(v1->v.pos);
      glColor3fv(v2->v.color);
      glVertex3fv(v2->v.pos);
   }
   else {
      union vert m[3];

      make_midpoint(&m[0], v0, v1);
      make_midpoint(&m[1], v1, v2);
      make_midpoint(&m[2], v2, v0);

      subdiv(&m[0], &m[2], v0, depth-1);
      subdiv(&m[1], &m[0], v1, depth-1);
      subdiv(&m[2], &m[1], v2, depth-1);
      subdiv(&m[0], &m[1], &m[2], depth-1);
   }
}

static void enable( GLenum value, GLboolean flag )
{
   if (flag)
      glEnable(value);
   else
      glDisable(value);
}

/** Assignment */
#define ASSIGN_3V( V, V0, V1, V2 )  \
do {                                \
    V[0] = V0;                      \
    V[1] = V1;                      \
    V[2] = V2;                      \
} while(0)

static void Display( void )
{
   glClearColor(0.3, 0.3, 0.3, 1);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glPointSize(psz);

   glUseProgram(program);
   enable( GL_POINT_SMOOTH, pointsmooth );
   enable( GL_VERTEX_PROGRAM_POINT_SIZE_ARB, program_point_size );

   glBegin(prim);


   {
      union vert v[3];

      ASSIGN_3V(v[0].v.color, 0,0,1);
      ASSIGN_3V(v[0].v.pos,  0.9, -0.9, 0.0);
      ASSIGN_3V(v[1].v.color, 1,0,0);
      ASSIGN_3V(v[1].v.pos, 0.9, 0.9, 0.0);
      ASSIGN_3V(v[2].v.color, 0,1,0);
      ASSIGN_3V(v[2].v.pos, -0.9, 0, 0.0);

      subdiv(&v[0], &v[1], &v[2], nr_steps);
   }

   glEnd();


   glFlush();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   /*glTranslatef( 0.0, 0.0, -15.0 );*/
}


static void CleanUp(void)
{
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
}

static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case 'p':
      prim = GL_POINTS;
      break;
   case 't':
      prim = GL_TRIANGLES;
      break;
   case 's':
      psz += .5;
      break;
   case 'S':
      if (psz > .5)
         psz -= .5;
      break;
   case 'm':
      pointsmooth = !pointsmooth;
      break;
   case 'z':
      program_point_size = !program_point_size;
      break;
   case '+':
      nr_steps++;
      break; 
   case '-':
      if (nr_steps) 
         nr_steps--;
      break;
   case ' ':
      psz = 1.0;
      prim = GL_TRIANGLES;
      nr_steps = 4;
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}

int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 250, 250 );
   glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH );
   glutCreateWindow(argv[argc-1]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   args( argc, argv );
   prepare_shaders();
   glutMainLoop();
   return 0;
}
