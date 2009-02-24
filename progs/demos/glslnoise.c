/*
 * GLSL noise demo.
 *
 * Michal Krol
 * 20 February 2006
 *
 * Based on the original demo by:
 * Stefan Gustavson (stegu@itn.liu.se) 2004, 2005
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>

#ifdef WIN32
#define GETPROCADDRESS(F) wglGetProcAddress(F)
#else
#define GETPROCADDRESS(F) glutGetProcAddress(F)
#endif

static GLhandleARB fragShader;
static GLhandleARB vertShader;
static GLhandleARB program;

static GLint uTime;

static GLint t0 = 0;
static GLint frames = 0;

static GLfloat u_time = 0.0f;

static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = NULL;
static PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
static PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
static PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = NULL;
static PFNGLATTACHOBJECTARBPROC glAttachObjectARB = NULL;
static PFNGLLINKPROGRAMARBPROC glLinkProgramARB = NULL;
static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = NULL;
static PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
static PFNGLUNIFORM1FARBPROC glUniform1fARB = NULL;

static void Redisplay (void)
{
   GLint t;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1fARB (uTime, 0.5f * u_time);

	glPushMatrix ();
	glutSolidSphere (2.0, 20, 10);
	glPopMatrix ();

	glutSwapBuffers();
   frames++;

   t = glutGet (GLUT_ELAPSED_TIME);
   if (t - t0 >= 5000) {
      GLfloat seconds = (GLfloat) (t - t0) / 1000.0f;
      GLfloat fps = frames / seconds;
      printf ("%d frames in %6.3f seconds = %6.3f FPS\n", frames, seconds, fps);
      fflush(stdout);
      t0 = t;
      frames = 0;
   }
}

static void Idle (void)
{
	u_time += 0.1f;
	glutPostRedisplay ();
}

static void Reshape (int width, int height)
{
	glViewport (0, 0, width, height);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0f, 0.0f, -15.0f);
}

static void Key (unsigned char key, int x, int y)
{
	(void) x;
	(void) y;

	switch (key)
	{
	case 27:
		exit(0);
		break;
	}
	glutPostRedisplay ();
}

static void Init (void)
{
   static const char *fragShaderText =
      "uniform float time;\n"
      "varying vec3 position;\n"
      "void main () {\n"
      "   gl_FragColor = vec4 (vec3 (0.5 + 0.5 * noise1 (vec4 (position, time))), 1.0);\n"
      "}\n"
   ;
   static const char *vertShaderText =
      "varying vec3 position;\n"
      "void main () {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   position = 4.0 * gl_Vertex.xyz;\n"
      "}\n"
   ;

	if (!glutExtensionSupported ("GL_ARB_fragment_shader"))
	{
		printf ("Sorry, this demo requires GL_ARB_fragment_shader\n");
		exit(1);
	}
	if (!glutExtensionSupported ("GL_ARB_shader_objects"))
	{
		printf ("Sorry, this demo requires GL_ARB_shader_objects\n");
		exit(1);
	}
	if (!glutExtensionSupported ("GL_ARB_shading_language_100"))
	{
		printf ("Sorry, this demo requires GL_ARB_shading_language_100\n");
		exit(1);
	}
	if (!glutExtensionSupported ("GL_ARB_vertex_shader"))
	{
		printf ("Sorry, this demo requires GL_ARB_vertex_shader\n");
		exit(1);
	}

	glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)
		GETPROCADDRESS("glCreateShaderObjectARB");
 	glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)
		GETPROCADDRESS("glShaderSourceARB");
 	glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)
		GETPROCADDRESS("glCompileShaderARB");
 	glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)
		GETPROCADDRESS("glCreateProgramObjectARB");
 	glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)
		GETPROCADDRESS("glAttachObjectARB");
 	glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)
		GETPROCADDRESS ("glLinkProgramARB");
 	glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)
		GETPROCADDRESS("glUseProgramObjectARB");          

	glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)
		GETPROCADDRESS("glGetUniformLocationARB");
	glUniform1fARB = (PFNGLUNIFORM1FARBPROC)
		GETPROCADDRESS("glUniform1fARB");

	fragShader = glCreateShaderObjectARB (GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB (fragShader, 1, &fragShaderText, NULL);
	glCompileShaderARB (fragShader);

	vertShader = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
	glShaderSourceARB (vertShader, 1, &vertShaderText, NULL);
	glCompileShaderARB (vertShader);

	program = glCreateProgramObjectARB ();
	glAttachObjectARB (program, fragShader);
	glAttachObjectARB (program, vertShader);
	glLinkProgramARB (program);
	glUseProgramObjectARB (program);

	uTime = glGetUniformLocationARB (program, "time");

	glClearColor (0.0f, 0.1f, 0.3f, 1.0f);
	glEnable (GL_CULL_FACE);
	glEnable (GL_DEPTH_TEST);

	printf ("GL_RENDERER = %s\n", (const char *) glGetString (GL_RENDERER));
}

int main (int argc, char *argv[])
{
	glutInit (&argc, argv);
	glutInitWindowPosition ( 0, 0);
	glutInitWindowSize (200, 200);
	glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow (argv[0]);
	glutReshapeFunc (Reshape);
	glutKeyboardFunc (Key);
	glutDisplayFunc (Redisplay);
	glutIdleFunc (Idle);
	Init ();
	glutMainLoop ();
	return 0;
}

