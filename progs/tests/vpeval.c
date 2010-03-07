/*
 * Vertex program evaluators test.
 * Based on book/bezmesh.c
 *
 * Brian Paul
 * 22 June 2002
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>


/*
 * Transform position by modelview/projection.
 * Square incoming color.
 */
static const char prog[] = 
"!!VP1.0\n"

"# Typical modelview/projection\n"
"DP4   o[HPOS].x, c[0], v[OPOS] ;	# object x MVP -> clip\n"
"DP4   o[HPOS].y, c[1], v[OPOS] ;\n"
"DP4   o[HPOS].z, c[2], v[OPOS] ;\n"
"DP4   o[HPOS].w, c[3], v[OPOS] ;\n"

"MOV   R0, v[COL0];\n       # square the color\n"
"MUL   R0, R0, R0;\n"
"MOV   o[COL0], R0;\n       # store output color\n"

"END";


static int program = 1;


GLfloat ctrlpoints[4][4][4] =
{
    {
        {-1.5, -1.5, 4.0, 1.0},
        {-0.5, -1.5, 2.0, 1.0},
        {0.5, -1.5, -1.0, 1.0},
        {1.5, -1.5, 2.0, 1.0}},
    {
        {-1.5, -0.5, 1.0, 1.0},
        {-0.5, -0.5, 3.0, 1.0},
        {0.5, -0.5, 0.0, 1.0},
        {1.5, -0.5, -1.0, 1.0}},
    {
        {-1.5, 0.5, 4.0, 1.0},
        {-0.5, 0.5, 0.0, 1.0},
        {0.5, 0.5, 3.0, 1.0},
        {1.5, 0.5, 4.0, 1.0}},
    {
        {-1.5, 1.5, -2.0, 1.0},
        {-0.5, 1.5, -2.0, 1.0},
        {0.5, 1.5, 0.0, 1.0},
        {1.5, 1.5, -1.0, 1.0}}
};

/*
 * +-------------+
 * |green        |yellow
 * |             |
 * |             |
 * |black        |red
 * +-------------+
 */
GLfloat colorPoints[4][4][4] =
{
    {
        {0.0, 0.0, 0.0, 1.0},
        {0.3, 0.0, 0.0, 1.0},
        {0.6, 0.0, 0.0, 1.0},
        {1.0, 0.0, 0.0, 1.0}},
    {
        {0.0, 0.3, 0.0, 1.0},
        {0.3, 0.3, 0.0, 1.0},
        {0.6, 0.3, 0.0, 1.0},
        {1.0, 0.3, 0.0, 1.0}},
    {
        {0.0, 0.6, 0.0, 1.0},
        {0.3, 0.6, 0.0, 1.0},
        {0.6, 0.6, 0.0, 1.0},
        {1.0, 0.6, 0.0, 1.0}},
    {
        {0.0, 1.0, 0.0, 1.0},
        {0.3, 1.0, 0.0, 1.0},
        {0.6, 1.0, 0.0, 1.0},
        {1.0, 1.0, 0.0, 1.0}}
};


static void
initlights(void)
{
#if 0 /* no lighting for now */
    GLfloat ambient[] = {0.2, 0.2, 0.2, 1.0};
    GLfloat position[] = {0.0, 0.0, 2.0, 1.0};
    GLfloat mat_diffuse[] = {0.6, 0.6, 0.6, 1.0};
    GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat mat_shininess[] = {50.0};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
#endif
}

static void
display(void)
{
   glClearColor(.3, .3, .3, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
#if 1
    glRotatef(85.0, 1.0, 1.0, 1.0);
#endif
    glEvalMesh2(GL_FILL, 0, 8, 0, 8);
    glPopMatrix();
    glFlush();
}

static void
myinit(int argc, char *argv[])
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    initlights();       /* for lighted version only */

    glMapGrid2f(8, 0.0, 1.0, 8, 0.0, 1.0);

    if (argc > 1)
       program = 0;

    printf("Using vertex program attribs? %s\n", program ? "yes" : "no");

    if (program && !glutExtensionSupported("GL_NV_vertex_program")) {
       printf("Sorry, this requires GL_NV_vertex_program\n");
       exit(1);
    }

    if (!program) {
        glMap2f(GL_MAP2_VERTEX_4,
                0.0, 1.0, 4, 4,
                0.0, 1.0, 16, 4, &ctrlpoints[0][0][0]);
        glMap2f(GL_MAP2_COLOR_4,
                0.0, 1.0, 4, 4,
                0.0, 1.0, 16, 4, &colorPoints[0][0][0]);
        glEnable(GL_MAP2_VERTEX_4);
        glEnable(GL_MAP2_COLOR_4);
        /*
        glEnable(GL_AUTO_NORMAL);
        glEnable(GL_NORMALIZE);
        */
    }
    else {
        glMap2f(GL_MAP2_VERTEX_ATTRIB0_4_NV,
                0.0, 1.0, 4, 4,
                0.0, 1.0, 16, 4, &ctrlpoints[0][0][0]);
        glMap2f(GL_MAP2_VERTEX_ATTRIB3_4_NV,
                0.0, 1.0, 4, 4,
                0.0, 1.0, 16, 4, &colorPoints[0][0][0]);
        glEnable(GL_MAP2_VERTEX_ATTRIB0_4_NV);
        glEnable(GL_MAP2_VERTEX_ATTRIB3_4_NV);

        /*
        glEnable(GL_AUTO_NORMAL);
        glEnable(GL_NORMALIZE);
        */

        /* vertex program init */
        glLoadProgramNV(GL_VERTEX_PROGRAM_NV, 1,
                        strlen(prog), (const GLubyte *) prog);
        assert(glIsProgramNV(1));
        glBindProgramNV(GL_VERTEX_PROGRAM_NV, 1);

        /* track matrices */
        glTrackMatrixNV(GL_VERTEX_PROGRAM_NV, 0, GL_MODELVIEW_PROJECTION_NV, GL_IDENTITY_NV);
        glEnable(GL_VERTEX_PROGRAM_NV);
    }
}

static void
myReshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (w <= h)
        glOrtho(-4.0, 4.0, -4.0 * (GLfloat) h / (GLfloat) w,
            4.0 * (GLfloat) h / (GLfloat) w, -4.0, 4.0);
    else
        glOrtho(-4.0 * (GLfloat) w / (GLfloat) h,
            4.0 * (GLfloat) w / (GLfloat) h, -4.0, 4.0, -4.0, 4.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void
key(unsigned char k, int x, int y)
{
  switch (k) {
  case 27:  /* Escape */
    exit(0);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

int
main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(argv[0]);
    glewInit();
    myinit(argc, argv);
    glutReshapeFunc(myReshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
