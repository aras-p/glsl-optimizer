/*
** blendeq.c - Demonstrates the use of the blend_minmax, blend_subtract,
**    and blend_logic_op extensions using glBlendEquationEXT.
**
**    Over a two-color backround, draw rectangles using twelve blend
**    options.  The values are read back as UNSIGNED_BYTE and printed
**    in hex over each value.  These values are useful for logic
**    op comparisons when channels are 8 bits deep.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>


GLenum doubleBuffer;
static int dithering = 0;
static int doPrint = 1;
static int deltaY;
GLint windW, windH;

static void DrawString(const char *string)
{
    int i;

    for (i = 0; string[i]; i++)
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, string[i]);
}

static void Init(void)
{

    glDisable(GL_DITHER);
    glShadeModel(GL_FLAT);
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;

    glViewport(0, 0, (GLint)width, (GLint)height);
    deltaY = windH /16;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windW, 0, windH);
    glMatrixMode(GL_MODELVIEW);
}

static void Key(unsigned char key, int x, int y)
{

    switch (key) {
      case 27:
	exit(1);
      case 'd':
	dithering = !dithering;
	break;
      default:
	return;
    }

    glutPostRedisplay();
}

static void PrintColorStrings( void )
{
    GLubyte ubbuf[3];
    int i, xleft, xright;
    char colorString[18];

    xleft = 5 + windW/4;
    xright = 5 + windW/2;

    for (i = windH - deltaY + 4; i > 0; i-=deltaY) {
        glReadPixels(xleft, i+10, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, ubbuf);
        sprintf(colorString, "(0x%x, 0x%x, 0x%x)",
                ubbuf[0], ubbuf[1], ubbuf[2]);
        glRasterPos2f(xleft, i);
	DrawString(colorString);
        glReadPixels(xright, i+10, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, ubbuf);
        sprintf(colorString, "(0x%x, 0x%x, 0x%x)",
                ubbuf[0], ubbuf[1], ubbuf[2]);
        glRasterPos2f(xright, i);
	DrawString(colorString);
    }
}

static void Draw(void)
{
    int stringOffset = 5, stringx = 8;
    int x1, x2, xleft, xright;
    int i;

    (dithering) ? glEnable(GL_DITHER) : glDisable(GL_DITHER);
    glDisable(GL_BLEND);

    glClearColor(0.5, 0.6, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Draw background */
    glColor3f(0.1, 0.1, 1.0);
    glRectf(0.0, 0.0, windW/2, windH);

    /* Draw labels */
    glColor3f(0.8, 0.8, 0.0);
    i = windH - deltaY + stringOffset;
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("SOURCE");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("DEST");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("min");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("max");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("subtract");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("reverse_subtract");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("clear");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("set");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("copy");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("noop");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("and");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("invert");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("or");
    glRasterPos2f(stringx, i); i -= deltaY;
    DrawString("xor");


    i = windH - deltaY;
    x1 = windW/4;
    x2 = 3 * windW/4;
    xleft = 5 + windW/4;
    xright = 5 + windW/2;

    /* Draw foreground color for comparison */
    glColor3f(0.9, 0.2, 0.8);
    glRectf(x1, i, x2, i+deltaY);

    /* Leave one rectangle of background color */

    /* Begin test cases */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    i -= 2*deltaY;
    glBlendEquationEXT(GL_MIN_EXT);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_MAX_EXT);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_FUNC_SUBTRACT_EXT);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
    glRectf(x1, i, x2, i+deltaY);

    glBlendFunc(GL_ONE, GL_ZERO);
    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_CLEAR);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_SET);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_COPY);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_NOOP);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_AND);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_INVERT);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_OR);
    glRectf(x1, i, x2, i+deltaY);

    i -= deltaY;
    glBlendEquationEXT(GL_LOGIC_OP);
    glLogicOp(GL_XOR);
    glRectf(x1, i, x2, i+deltaY);
    glRectf(x1, i+10, x2, i+5);

  if (doPrint) {
      glDisable(GL_BLEND);
      glColor3f(1.0, 1.0, 1.0);
      PrintColorStrings();
  }
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
	    printf("%s (Bad option).\n", argv[i]);
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

int main(int argc, char **argv)
{
    GLenum type;
    char *s;
    char *extName1 = "GL_EXT_blend_logic_op";
    char *extName2 = "GL_EXT_blend_minmax";
    char *extName3 = "GL_EXT_blend_subtract";

    glutInit(&argc, argv);

    if (Args(argc, argv) == GL_FALSE) {
	exit(1);
    }

    glutInitWindowPosition(0, 0); glutInitWindowSize( 800, 400);

    type = GLUT_RGB;
    type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
    glutInitDisplayMode(type);

    if (glutCreateWindow("Blend Equation") == GL_FALSE) {
	exit(1);
    }

    /* Make sure blend_logic_op extension is there. */
    s = (char *) glGetString(GL_EXTENSIONS);
    if (!s)
	exit(1);
    if (strstr(s,extName1) == 0) {
	printf("Blend_logic_op extension is not present.\n");
	exit(1);
    }
    if (strstr(s,extName2) == 0) {
	printf("Blend_minmax extension is not present.\n");
	exit(1);
    }
    if (strstr(s,extName3) == 0) {
	printf("Blend_subtract extension is not present.\n");
	exit(1);
    }

    Init();

    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutDisplayFunc(Draw);
    glutMainLoop();
    return 0;
}
