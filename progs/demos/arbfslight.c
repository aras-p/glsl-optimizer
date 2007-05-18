/*
 * Use GL_ARB_fragment_shader and GL_ARB_vertex_shader to implement
 * simple per-pixel lighting.
 *
 * Michal Krol
 * 20 February 2006
 *
 * Based on the original demo by:
 * Brian Paul
 * 17 April 2003
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>

#ifdef WIN32
#define GETPROCADDRESS wglGetProcAddress
#else
#define GETPROCADDRESS glutGetProcAddress
#endif

static GLfloat diffuse[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
static GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
static GLfloat lightPos[4] = { 0.0f, 10.0f, 20.0f, 1.0f };
static GLfloat delta = 1.0f;

static GLhandleARB fragShader;
static GLhandleARB vertShader;
static GLhandleARB program;

static GLint uLightPos;
static GLint uDiffuse;
static GLint uSpecular;

static GLboolean anim = GL_TRUE;
static GLboolean wire = GL_FALSE;
static GLboolean pixelLight = GL_TRUE;

static GLint t0 = 0;
static GLint frames = 0;

static GLfloat xRot = 0.0f, yRot = 0.0f;

static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = NULL;
static PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
static PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
static PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = NULL;
static PFNGLATTACHOBJECTARBPROC glAttachObjectARB = NULL;
static PFNGLLINKPROGRAMARBPROC glLinkProgramARB = NULL;
static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = NULL;
static PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
static PFNGLUNIFORM3FVARBPROC glUniform3fvARB = NULL;
static PFNGLUNIFORM3FVARBPROC glUniform4fvARB = NULL;

static void normalize (GLfloat *dst, const GLfloat *src)
{
   GLfloat len = sqrt (src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);
   dst[0] = src[0] / len;
   dst[1] = src[1] / len;
   dst[2] = src[2] / len;
}

static void Redisplay (void)
{
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (pixelLight)
	{
      GLfloat vec[3];

		glUseProgramObjectARB (program);
      normalize (vec, lightPos);
      glUniform3fvARB (uLightPos, 1, vec);
		glDisable(GL_LIGHTING);
	}
	else
	{
		glUseProgramObjectARB (0);
		glLightfv (GL_LIGHT0, GL_POSITION, lightPos);
		glEnable(GL_LIGHTING);
	}

	glPushMatrix ();
	glRotatef (xRot, 1.0f, 0.0f, 0.0f);
	glRotatef (yRot, 0.0f, 1.0f, 0.0f);
	glutSolidSphere (2.0, 10, 5);
	glPopMatrix ();

	glutSwapBuffers();
	frames++;

	if (anim)
	{
		GLint t = glutGet (GLUT_ELAPSED_TIME);
		if (t - t0 >= 5000)
		{
			GLfloat seconds = (GLfloat) (t - t0) / 1000.0f;
			GLfloat fps = frames / seconds;
			printf ("%d frames in %6.3f seconds = %6.3f FPS\n", frames, seconds, fps);
			t0 = t;
			frames = 0;
		}
	}
}

static void Idle (void)
{
	lightPos[0] += delta;
	if (lightPos[0] > 25.0f || lightPos[0] < -25.0f)
		delta = -delta;
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
	case ' ':
	case 'a':
		anim = !anim;
		if (anim)
			glutIdleFunc (Idle);
		else
			glutIdleFunc (NULL);
		break;
	case 'x':
		lightPos[0] -= 1.0f;
		break;
	case 'X':
		lightPos[0] += 1.0f;
		break;
	case 'w':
		wire = !wire;
		if (wire)
			glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'p':
		pixelLight = !pixelLight;
		if (pixelLight)
			printf ("Per-pixel lighting\n");
		else
			printf ("Conventional lighting\n");
		break;
	case 27:
		exit(0);
		break;
	}
	glutPostRedisplay ();
}

static void SpecialKey (int key, int x, int y)
{
	const GLfloat step = 3.0f;

	(void) x;
	(void) y;

	switch (key)
	{
	case GLUT_KEY_UP:
		xRot -= step;
		break;
	case GLUT_KEY_DOWN:
		xRot += step;
		break;
	case GLUT_KEY_LEFT:
		yRot -= step;
		break;
	case GLUT_KEY_RIGHT:
		yRot += step;
		break;
	}
	glutPostRedisplay ();
}

static void Init (void)
{
   static const char *fragShaderText =
      "uniform vec3 lightPos;\n"
      "uniform vec4 diffuse;\n"
      "uniform vec4 specular;\n"
      "varying vec3 normal;\n"
      "void main () {\n"
      "   // Compute dot product of light direction and normal vector\n"
      "   float dotProd = max (dot (lightPos, normalize (normal)), 0.0);\n"
      "   // Compute diffuse and specular contributions\n"
      "   gl_FragColor = diffuse * dotProd + specular * pow (dotProd, 20.0);\n"
      "}\n"
   ;
   static const char *vertShaderText =
      "varying vec3 normal;\n"
      "void main () {\n"
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
      "   normal = gl_NormalMatrix * gl_Normal;\n"
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

	glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) GETPROCADDRESS ("glCreateShaderObjectARB");
	glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) GETPROCADDRESS ("glShaderSourceARB");
	glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) GETPROCADDRESS ("glCompileShaderARB");
	glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) GETPROCADDRESS ("glCreateProgramObjectARB");
	glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) GETPROCADDRESS ("glAttachObjectARB");
	glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) GETPROCADDRESS ("glLinkProgramARB");
	glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) GETPROCADDRESS ("glUseProgramObjectARB");
	glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) GETPROCADDRESS ("glGetUniformLocationARB");
   glUniform3fvARB = (PFNGLUNIFORM3FVARBPROC) GETPROCADDRESS ("glUniform3fvARB");
   glUniform4fvARB = (PFNGLUNIFORM3FVARBPROC) GETPROCADDRESS ("glUniform4fvARB");

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

	uLightPos = glGetUniformLocationARB (program, "lightPos");
	uDiffuse = glGetUniformLocationARB (program, "diffuse");
	uSpecular = glGetUniformLocationARB (program, "specular");

   glUniform4fvARB (uDiffuse, 1, diffuse);
   glUniform4fvARB (uSpecular, 1, specular);

	glClearColor (0.3f, 0.3f, 0.3f, 0.0f);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_LIGHT0);
	glEnable (GL_LIGHTING);
	glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);

	printf ("GL_RENDERER = %s\n", (const char *) glGetString (GL_RENDERER));
	printf ("Press p to toggle between per-pixel and per-vertex lighting\n");
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
	glutSpecialFunc (SpecialKey);
	glutDisplayFunc (Redisplay);
	if (anim)
		glutIdleFunc (Idle);
	Init ();
	glutMainLoop ();
	return 0;
}

