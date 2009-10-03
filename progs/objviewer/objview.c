/*  
 * .obj file viewer based on "smooth" by Nate Robins, 1997
 *
 * Brian Paul
 * 1 Oct 2009
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "glm.h"
#include "readtex.h"
#include "skybox.h"
#include "trackball.h"


static char *Model_file = NULL;		/* name of the obect file */
static GLMmodel *Model;
static GLfloat Scale = 4.0;			/* scaling factor */
static GLboolean Performance = GL_FALSE;
static GLboolean Stats = GL_FALSE;
static GLboolean Animate = GL_TRUE;
static GLuint SkyboxTex;
static GLboolean Skybox = GL_TRUE;
static GLboolean Cull = GL_TRUE;
static GLboolean WireFrame = GL_FALSE;
static GLenum FrontFace = GL_CCW;
static GLfloat Yrot = 0.0;
static GLint WinWidth = 1024, WinHeight = 768;
static GLuint NumInstances = 1;



typedef struct
{
   float CurQuat[4];
   float Distance;
   /* When mouse is moving: */
   GLboolean Rotating, Translating;
   GLint StartX, StartY;
   float StartDistance;
} ViewInfo;

static ViewInfo View;

static void
InitViewInfo(ViewInfo *view)
{
   view->Rotating = GL_FALSE;
   view->Translating = GL_FALSE;
   view->StartX = view->StartY = 0;
   view->Distance = 12.0;
   view->StartDistance = 0.0;
   view->CurQuat[0] = 0.0;
   view->CurQuat[1] = 1.0;
   view->CurQuat[2] = 0.0;
   view->CurQuat[3] = 0.0;
}



/* text: general purpose text routine.  draws a string according to
 * format in a stroke font at x, y after scaling it by the scale
 * specified (scale is in window-space (lower-left origin) pixels).  
 *
 * x      - position in x (in window-space)
 * y      - position in y (in window-space)
 * scale  - scale in pixels
 * format - as in printf()
 */
static void 
text(GLuint x, GLuint y, GLfloat scale, char* format, ...)
{
  va_list args;
  char buffer[255], *p;
  GLfloat font_scale = 119.05 + 33.33;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glTranslatef(x, y, 0.0);

  glScalef(scale/font_scale, scale/font_scale, scale/font_scale);

  for(p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  
  glPopAttrib();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}


static float
ComputeFPS(void)
{
   static double t0 = -1.0;
   static int frames = 0;
   double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   static float fps = 0;

   frames++;

   if (t0 < 0.0) {
      t0 = t;
      fps = 0.0;
   }
   else if (t - t0 >= 4.0) {
      fps = (frames / (t - t0) + 0.5);
      t0 = t;
      frames = 0;
      return fps;
   }

   return 0.0;
}


static void
init_model(void)
{
   float objScale;

   /* read in the model */
   Model = glmReadOBJ(Model_file);
   objScale = glmUnitize(Model);
   glmFacetNormals(Model);
   if (Model->numnormals == 0) {
      GLfloat smoothing_angle = 90.0;
      printf("Generating normals.\n");
      glmVertexNormals(Model, smoothing_angle);
   }

   glmLoadTextures(Model);
   glmReIndex(Model);
   glmMakeVBOs(Model);
   if (0)
      glmPrint(Model);
}

static void
init_skybox(void)
{
   SkyboxTex = LoadSkyBoxCubeTexture("alpine_east.rgb",
                                     "alpine_west.rgb",
                                     "alpine_up.rgb",
                                     "alpine_down.rgb",
                                     "alpine_south.rgb",
                                     "alpine_north.rgb");
   glmSpecularTexture(Model, SkyboxTex);
}


static void
init_gfx(void)
{
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_NORMALIZE);
   glClearColor(0.3, 0.3, 0.9, 0.0);
}


static void
reshape(int width, int height)
{
   float ar = 0.5 * (float) width / (float) height;

   WinWidth = width;
   WinHeight = height;

   glViewport(0, 0, width, height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ar, ar, -0.5, 0.5, 1.0, 300.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -3.0);
}


static void
Idle(void)
{
   float q[4];
   trackball(q, 100, 0, 99.99, 0);
   add_quats(q, View.CurQuat, View.CurQuat);

   glutPostRedisplay();
}


static void
display(void)
{
   GLfloat rot[4][4];
   float fps;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
      glTranslatef(0.0, 0.0, -View.Distance);
      glRotatef(Yrot, 0, 1, 0);
      build_rotmatrix(rot, View.CurQuat);
      glMultMatrixf(&rot[0][0]);
      glScalef(Scale, Scale, Scale );

      glUseProgram(0);

      if (Skybox)
         DrawSkyBoxCubeTexture(SkyboxTex);

      if (WireFrame)
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      if (Cull)
         glEnable(GL_CULL_FACE);
      else
         glDisable(GL_CULL_FACE);

      if (NumInstances == 1) {
         glmDrawVBO(Model);
      }
      else {
         /* draw > 1 instance */
         float dr = 360.0 / NumInstances;
         float r;
         for (r = 0.0; r < 360.0; r += dr) {
            glPushMatrix();
            glRotatef(r, 0, 1, 0);
            glTranslatef(1.4, 0.0, 0.0);
            glmDrawVBO(Model);
            glPopMatrix();
         }
      }

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glDisable(GL_CULL_FACE);

   glPopMatrix();

   if (Stats) {
      glColor3f(1.0, 1.0, 1.0);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*1), 20, "%s", 
           Model->pathname);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*2), 20, "%d vertices", 
           Model->numvertices);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*3), 20, "%d triangles", 
           Model->numtriangles);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*4), 20, "%d normals", 
           Model->numnormals);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*5), 20, "%d texcoords", 
           Model->numtexcoords);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*6), 20, "%d groups", 
           Model->numgroups);
      text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*7), 20, "%d materials", 
           Model->nummaterials);
   }

   glutSwapBuffers();

   fps = ComputeFPS();
   if (fps)
      printf("%f FPS\n", fps);
}


static void
keyboard(unsigned char key, int x, int y)
{
   switch (key) {
   case 'h':
      printf("help\n\n");
      printf("a            -  Toggle animation\n");
      printf("d/D          -  Decrease/Incrase number of models\n");
      printf("w            -  Toggle wireframe/filled\n");
      printf("c            -  Toggle culling\n");
      printf("n            -  Toggle facet/smooth normal\n");
      printf("r            -  Reverse polygon winding\n");
      printf("p            -  Toggle performance indicator\n");
      printf("s            -  Toggle skybox\n");
      printf("z/Z          -  Scale model smaller/larger\n");
      printf("i            -  Show model info/stats\n");
      printf("q/escape     -  Quit\n\n");
      break;
   case 'a':
      Animate = !Animate;
      if (Animate)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'd':
      if (NumInstances > 1)
         NumInstances--;
      break;
   case 'D':
      NumInstances++;
      break;
   case 'i':
      Stats = !Stats;
      break;
   case 'p':
      Performance = !Performance;
      break;
   case 'w':
      WireFrame = !WireFrame;
      break;
   case 'c':
      Cull = !Cull;
      printf("Polygon culling: %d\n", Cull);
      break;
   case 'r':
      if (FrontFace == GL_CCW)
         FrontFace = GL_CW;
      else
         FrontFace = GL_CCW;
      glFrontFace(FrontFace);
      printf("Front face:: %s\n", FrontFace == GL_CCW ? "CCW" : "CW");
      break;
   case 's':
      Skybox = !Skybox;
      if (Skybox)
         glmSpecularTexture(Model, SkyboxTex);
      else
         glmSpecularTexture(Model, 0);
      break;
   case 'z':
      Scale *= 0.9;
      break;
   case 'Z':
      Scale *= 1.1;
      break;
   case 'q':
   case 27:
      exit(0);
      break;
   }

   glutPostRedisplay();
}


static void
menu(int item)
{
    keyboard((unsigned char)item, 0, 0);
}


/**
 * Handle mouse button.
 */
static void
Mouse(int button, int state, int x, int y)
{
   if (button == GLUT_LEFT_BUTTON) {
      if (state == GLUT_DOWN) {
         View.StartX = x;
         View.StartY = y;
         View.Rotating = GL_TRUE;
      }
      else if (state == GLUT_UP) {
         View.Rotating = GL_FALSE;
      }
   }
   else if (button == GLUT_MIDDLE_BUTTON) {
      if (state == GLUT_DOWN) {
         View.StartX = x;
         View.StartY = y;
         View.StartDistance = View.Distance;
         View.Translating = GL_TRUE;
      }
      else if (state == GLUT_UP) {
         View.Translating = GL_FALSE;
      }
   }
}


/**
 * Handle mouse motion
 */
static void
Motion(int x, int y)
{
   int i;
   if (View.Rotating) {
      float x0 = (2.0 * View.StartX - WinWidth) / WinWidth;
      float y0 = (WinHeight - 2.0 * View.StartY) / WinHeight;
      float x1 = (2.0 * x - WinWidth) / WinWidth;
      float y1 = (WinHeight - 2.0 * y) / WinHeight;
      float q[4];

      trackball(q, x0, y0, x1, y1);
      View.StartX = x;
      View.StartY = y;
      for (i = 0; i < 1; i++)
         add_quats(q, View.CurQuat, View.CurQuat);

      glutPostRedisplay();
   }
   else if (View.Translating) {
      float dz = 0.02 * (y - View.StartY);
      View.Distance = View.StartDistance + dz;
      glutPostRedisplay();
   }
}


static void
DoFeatureChecks(void)
{
   char *version = (char *) glGetString(GL_VERSION);
   if (version[0] == '1') {
      /* check for individual extensions */
      if (!glutExtensionSupported("GL_ARB_texture_cube_map")) {
         printf("Sorry, GL_ARB_texture_cube_map is required.\n");
         exit(1);
      }
      if (!glutExtensionSupported("GL_ARB_vertex_shader")) {
         printf("Sorry, GL_ARB_vertex_shader is required.\n");
         exit(1);
      }
      if (!glutExtensionSupported("GL_ARB_fragment_shader")) {
         printf("Sorry, GL_ARB_fragment_shader is required.\n");
         exit(1);
      }
      if (!glutExtensionSupported("GL_ARB_vertex_buffer_object")) {
         printf("Sorry, GL_ARB_vertex_buffer_object is required.\n");
         exit(1);
      }
   }
}


int
main(int argc, char** argv)
{
   glutInitWindowSize(WinWidth, WinHeight);
   glutInit(&argc, argv);

   if (argc > 1) {
      Model_file = argv[1];
   }
   if (!Model_file) {
      fprintf(stderr, "usage: objview file.obj\n");
      fprintf(stderr, "(using default bunny.obj)\n");
      Model_file = "bunny.obj";
   }

   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutCreateWindow("objview");

   glewInit();

   DoFeatureChecks();

   glutReshapeFunc(reshape);
   glutDisplayFunc(display);
   glutKeyboardFunc(keyboard);
   glutMouseFunc(Mouse);
   glutMotionFunc(Motion);
   if (Animate)
      glutIdleFunc(Idle);

   glutCreateMenu(menu);
   glutAddMenuEntry("[a] Toggle animate", 'a');
   glutAddMenuEntry("[d] Fewer models", 'd');
   glutAddMenuEntry("[D] More models", 'D');
   glutAddMenuEntry("[w] Toggle wireframe/filled", 'w');
   glutAddMenuEntry("[c] Toggle culling on/off", 'c');
   glutAddMenuEntry("[r] Reverse polygon winding", 'r');
   glutAddMenuEntry("[z] Scale model smaller", 'z');
   glutAddMenuEntry("[Z] Scale model larger", 'Z');
   glutAddMenuEntry("[p] Toggle performance indicator", 'p');
   glutAddMenuEntry("[i] Show model stats", 'i');
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("[q] Quit", 27);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   InitViewInfo(&View);

   init_model();
   init_skybox();
   init_gfx();

   glutMainLoop();

   return 0;
}
