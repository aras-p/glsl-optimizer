/*
 * Use GL_ARB_fragment_shader and GL_ARB_vertex_shader to implement
 * simple per-pixel lighting.
 *
 * Michal Krol
 * 14 February 2006
 *
 * Based on the original demo by:
 * Brian Paul
 * 17 April 2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

static GLfloat diffuse[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
static GLfloat specular[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
static GLfloat lightPos[4] = { 0.0f, 10.0f, 20.0f, 1.0f };
static GLfloat delta = 1.0f;

static GLhandleARB fragShader;
static GLhandleARB vertShader;
static GLhandleARB program;

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

static void Redisplay (void)
{
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (pixelLight)
	{
		glUseProgramObjectARB (program);
		/* XXX source from uniform lightPos */
		glTexCoord4fv (lightPos);
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
		"void main () {\n"

		/* XXX source from uniform lightPos */
		"	vec4 lightPos;\n"
		"	lightPos = gl_TexCoord[1];\n"

		/* XXX source from uniform diffuse */
		"	vec4 diffuse;\n"
		"	diffuse.xy = vec2 (0.5);\n"
		"	diffuse.zw = vec2 (1.0);\n"

		/* XXX source from uniform specular */
		"	vec4 specular;\n"
		"	specular.xyz = vec3 (0.8);\n"
		"	specular.w = 1.0;\n"

		"	// Compute normalized light direction\n"
		"	vec4 lightDir;\n"
		"	lightDir = lightPos / length (lightPos);\n"
		"	// Compute normalized normal\n"
		"	vec4 normal;\n"
		"	normal = gl_TexCoord[0] / length (gl_TexCoord[0]);\n"
		"	// Compute dot product of light direction and normal vector\n"
		"	float dotProd;\n"
		"	dotProd = clamp (dot (lightDir.xyz, normal.xyz), 0.0, 1.0);\n"
		"	// Compute diffuse and specular contributions\n"
		"	gl_FragColor = diffuse * dotProd + specular * pow (dotProd, 20.0);\n"
		"}\n"
	;
	static const char *vertShaderText =
		"void main () {\n"
		"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
		"	gl_TexCoord[0].xyz = gl_NormalMatrix * gl_Normal;\n"
		"	gl_TexCoord[0].w = 1.0;\n"

		/* XXX source from uniform lightPos */
		"	gl_TexCoord[1] = gl_MultiTexCoord0;\n"
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

	glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) glutGetProcAddress ("glCreateShaderObjectARB");
	glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) glutGetProcAddress ("glShaderSourceARB");
	glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) glutGetProcAddress ("glCompileShaderARB");
	glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) glutGetProcAddress ("glCreateProgramObjectARB");
	glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) glutGetProcAddress ("glAttachObjectARB");
	glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) glutGetProcAddress ("glLinkProgramARB");
	glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) glutGetProcAddress ("glUseProgramObjectARB");

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

