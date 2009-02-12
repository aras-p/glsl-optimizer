/* Test fragment shader */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLuint fragShader;
static GLuint vertShader;
static GLuint program;
static GLint win = 0;
static GLfloat xpos = 0, ypos = 0;


static void
Redisplay(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glTranslatef(xpos, ypos, 0);

   glBegin(GL_TRIANGLES);
   glColor3f(1, 0, 0);
   glVertex2f(-0.9, -0.9);
   glColor3f(0, 1, 0);
   glVertex2f( 0.9, -0.9);
   glColor3f(0, 0, 1);
   glVertex2f( 0,  0.9);
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
CleanUp(void)
{
   glDeleteShader(fragShader);
   glDeleteShader(vertShader);
   glDeleteProgram(program);
   glutDestroyWindow(win);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;

   switch(key) {
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 0.1;

  (void) x;
  (void) y;

   switch(key) {
   case GLUT_KEY_UP:
      ypos += step;
      break;
   case GLUT_KEY_DOWN:
      ypos -= step;
      break;
   case GLUT_KEY_LEFT:
      xpos -= step;
      break;
   case GLUT_KEY_RIGHT:
      xpos += step;
      break;
   }
   glutPostRedisplay();
}


static void
LoadAndCompileShader(GLuint shader, const char *text)
{
   GLint stat;

   glShaderSource(shader, 1, (const GLchar **) &text, NULL);

   glCompileShader(shader);

   glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      glGetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "fslight: problem compiling shader:\n%s\n", log);
      exit(1);
   }
}


static void
CheckLink(GLuint prog)
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


static void
Init(void)
{
   /* fragment color is a function of fragment position: */
   static const char *fragShaderText =
      "void main() {\n"
      "   gl_FragColor = gl_FragCoord * vec4(0.005); \n"
      "   //gl_FragColor = gl_Color; \n"
      "   //gl_FragColor = vec4(1, 0, 0.5, 0); \n"
      "}\n";
#if 0
   static const char *vertShaderText =
      "varying vec3 normal;\n"
      "void main() {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   normal = gl_NormalMatrix * gl_Normal;\n"
      "}\n";
#endif
   const char *version;

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] != '2' || version[1] != '.') {
      printf("This program requires OpenGL 2.x, found %s\n", version);
      exit(1);
   }

   fragShader = glCreateShader(GL_FRAGMENT_SHADER);
   LoadAndCompileShader(fragShader, fragShaderText);

#if 0
   vertShader = glCreateShader(GL_VERTEX_SHADER);
   LoadAndCompileShader(vertShader, vertShaderText);
#endif

   program = glCreateProgram();
   glAttachShader(program, fragShader);
#if 0
   glAttachShader(program, vertShader);
#endif
   glLinkProgram(program);
   CheckLink(program);
   glUseProgram(program);

   assert(glGetError() == 0);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition( 0, 0);
   glutInitWindowSize(200, 200);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Redisplay);
   Init();
   glutMainLoop();
   return 0;
}


