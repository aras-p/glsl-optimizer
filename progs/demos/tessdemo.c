/* $Id: tessdemo.c,v 1.8 2000/07/11 14:11:58 brianp Exp $ */

/*
 * A demo of the GLU polygon tesselation functions written by Bogdan Sikorski.
 */


#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POINTS 200
#define MAX_CONTOURS 50

static int menu;
typedef enum
{ QUIT, TESSELATE, CLEAR }
menu_entries;

typedef enum
{ DEFINE, TESSELATED }
mode_type;

struct
{
   GLint p[MAX_POINTS][2];
   GLuint point_cnt;
}
contours[MAX_CONTOURS];

static GLuint contour_cnt;
static GLsizei width, height;
static mode_type mode;

struct
{
   GLsizei no;
   GLfloat color[3];
   GLint p[3][2];
   GLclampf p_color[3][3];
}
triangle;


static void GLCALLBACK
my_error(GLenum err)
{
   int len, i;
   char const *str;

   glColor3f(0.9, 0.9, 0.9);
   glRasterPos2i(5, 5);
   str = (const char *) gluErrorString(err);
   len = strlen(str);
   for (i = 0; i < len; i++)
      glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);
}


static void GLCALLBACK
begin_callback(GLenum mode)
{
   triangle.no = 0;
}


static void GLCALLBACK
edge_callback(GLenum flag)
{
   if (flag == GL_TRUE) {
      triangle.color[0] = 1.0;
      triangle.color[1] = 1.0;
      triangle.color[2] = 0.5;
   }
   else {
      triangle.color[0] = 1.0;
      triangle.color[1] = 0.0;
      triangle.color[2] = 0.0;
   }
}


static void GLCALLBACK
end_callback()
{
   glBegin(GL_LINES);
   glColor3f(triangle.p_color[0][0], triangle.p_color[0][1],
	     triangle.p_color[0][2]);
   glVertex2i(triangle.p[0][0], triangle.p[0][1]);
   glVertex2i(triangle.p[1][0], triangle.p[1][1]);
   glColor3f(triangle.p_color[1][0], triangle.p_color[1][1],
	     triangle.p_color[1][2]);
   glVertex2i(triangle.p[1][0], triangle.p[1][1]);
   glVertex2i(triangle.p[2][0], triangle.p[2][1]);
   glColor3f(triangle.p_color[2][0], triangle.p_color[2][1],
	     triangle.p_color[2][2]);
   glVertex2i(triangle.p[2][0], triangle.p[2][1]);
   glVertex2i(triangle.p[0][0], triangle.p[0][1]);
   glEnd();
}


static void GLCALLBACK
vertex_callback(void *data)
{
   GLsizei no;
   GLint *p;

   p = (GLint *) data;
   no = triangle.no;
   triangle.p[no][0] = p[0];
   triangle.p[no][1] = p[1];
   triangle.p_color[no][0] = triangle.color[0];
   triangle.p_color[no][1] = triangle.color[1];
   triangle.p_color[no][2] = triangle.color[2];
   ++(triangle.no);
}


static void
set_screen_wh(GLsizei w, GLsizei h)
{
   width = w;
   height = h;
}


static void
tesse(void)
{
   GLUtriangulatorObj *tobj;
   GLdouble data[3];
   GLuint i, j, point_cnt;

   tobj = gluNewTess();
   if (tobj != NULL) {
      glClear(GL_COLOR_BUFFER_BIT);
      glColor3f(0.7, 0.7, 0.0);
      gluTessCallback(tobj, GLU_BEGIN, glBegin);
      gluTessCallback(tobj, GLU_END, glEnd);
      gluTessCallback(tobj, GLU_ERROR, my_error);
      gluTessCallback(tobj, GLU_VERTEX, glVertex2iv);
      gluBeginPolygon(tobj);
      for (j = 0; j <= contour_cnt; j++) {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour(tobj, GLU_UNKNOWN);
	 for (i = 0; i < point_cnt; i++) {
	    data[0] = (GLdouble) (contours[j].p[i][0]);
	    data[1] = (GLdouble) (contours[j].p[i][1]);
	    data[2] = 0.0;
	    gluTessVertex(tobj, data, contours[j].p[i]);
	 }
      }
      gluEndPolygon(tobj);
      glLineWidth(2.0);
      gluTessCallback(tobj, GLU_BEGIN, begin_callback);
      gluTessCallback(tobj, GLU_END, end_callback);
      gluTessCallback(tobj, GLU_VERTEX, vertex_callback);
      gluTessCallback(tobj, GLU_EDGE_FLAG, edge_callback);
      gluBeginPolygon(tobj);
      for (j = 0; j <= contour_cnt; j++) {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour(tobj, GLU_UNKNOWN);
	 for (i = 0; i < point_cnt; i++) {
	    data[0] = (GLdouble) (contours[j].p[i][0]);
	    data[1] = (GLdouble) (contours[j].p[i][1]);
	    data[2] = 0.0;
	    gluTessVertex(tobj, data, contours[j].p[i]);
	 }
      }
      gluEndPolygon(tobj);
      gluDeleteTess(tobj);
      glutMouseFunc(NULL);
      glColor3f(1.0, 1.0, 0.0);
      glLineWidth(1.0);
      mode = TESSELATED;
   }
}


static void
left_down(int x1, int y1)
{
   GLint P[2];
   GLuint point_cnt;

   /* translate GLUT into GL coordinates */
   P[0] = x1;
   P[1] = height - y1;
   point_cnt = contours[contour_cnt].point_cnt;
   contours[contour_cnt].p[point_cnt][0] = P[0];
   contours[contour_cnt].p[point_cnt][1] = P[1];
   glBegin(GL_LINES);
   if (point_cnt) {
      glVertex2iv(contours[contour_cnt].p[point_cnt - 1]);
      glVertex2iv(P);
   }
   else {
      glVertex2iv(P);
      glVertex2iv(P);
   }
   glEnd();
   glFinish();
   ++(contours[contour_cnt].point_cnt);
}


static void
middle_down(int x1, int y1)
{
   GLuint point_cnt;

   point_cnt = contours[contour_cnt].point_cnt;
   if (point_cnt > 2) {
      glBegin(GL_LINES);
      glVertex2iv(contours[contour_cnt].p[0]);
      glVertex2iv(contours[contour_cnt].p[point_cnt - 1]);
      contours[contour_cnt].p[point_cnt][0] = -1;
      glEnd();
      glFinish();
      contour_cnt++;
      contours[contour_cnt].point_cnt = 0;
   }
}


static void
mouse_clicked(int button, int state, int x, int y)
{
   x -= x % 10;
   y -= y % 10;
   switch (button) {
   case GLUT_LEFT_BUTTON:
      if (state == GLUT_DOWN)
	 left_down(x, y);
      break;
   case GLUT_MIDDLE_BUTTON:
      if (state == GLUT_DOWN)
	 middle_down(x, y);
      break;
   }
}


static void
display(void)
{
   GLuint i, j;
   GLuint point_cnt;

   glClear(GL_COLOR_BUFFER_BIT);
   switch (mode) {
   case DEFINE:
      /* draw grid */
      glColor3f(0.6, 0.5, 0.5);
      glBegin(GL_LINES);
      for (i = 0; i < width; i += 10)
	 for (j = 0; j < height; j += 10) {
	    glVertex2i(0, j);
	    glVertex2i(width, j);
	    glVertex2i(i, height);
	    glVertex2i(i, 0);
	 }
      glEnd();
      glColor3f(1.0, 1.0, 0.0);
      for (i = 0; i <= contour_cnt; i++) {
	 point_cnt = contours[i].point_cnt;
	 glBegin(GL_LINES);
	 switch (point_cnt) {
	 case 0:
	    break;
	 case 1:
	    glVertex2iv(contours[i].p[0]);
	    glVertex2iv(contours[i].p[0]);
	    break;
	 case 2:
	    glVertex2iv(contours[i].p[0]);
	    glVertex2iv(contours[i].p[1]);
	    break;
	 default:
	    --point_cnt;
	    for (j = 0; j < point_cnt; j++) {
	       glVertex2iv(contours[i].p[j]);
	       glVertex2iv(contours[i].p[j + 1]);
	    }
	    if (contours[i].p[j + 1][0] == -1) {
	       glVertex2iv(contours[i].p[0]);
	       glVertex2iv(contours[i].p[j]);
	    }
	    break;
	 }
	 glEnd();
      }
      glFinish();
      break;
   case TESSELATED:
      /* draw lines */
      tesse();
      break;
   }

   glColor3f(1.0, 1.0, 0.0);
}


static void
clear(void)
{
   contour_cnt = 0;
   contours[0].point_cnt = 0;
   glutMouseFunc(mouse_clicked);
   mode = DEFINE;
   display();
}


static void
quit(void)
{
   exit(0);
}


static void
menu_selected(int entry)
{
   switch (entry) {
   case CLEAR:
      clear();
      break;
   case TESSELATE:
      tesse();
      break;
   case QUIT:
      quit();
      break;
   }
}


static void
key_pressed(unsigned char key, int x, int y)
{
   switch (key) {
   case 't':
   case 'T':
      tesse();
      glFinish();
      break;
   case 'q':
   case 'Q':
      quit();
      break;
   case 'c':
   case 'C':
      clear();
      break;
   }
}


static void
myinit(void)
{
/*  clear background to gray	*/
   glClearColor(0.4, 0.4, 0.4, 0.0);
   glShadeModel(GL_FLAT);

   menu = glutCreateMenu(menu_selected);
   glutAddMenuEntry("clear", CLEAR);
   glutAddMenuEntry("tesselate", TESSELATE);
   glutAddMenuEntry("quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);
   glutMouseFunc(mouse_clicked);
   glutKeyboardFunc(key_pressed);
   contour_cnt = 0;
   glPolygonMode(GL_FRONT, GL_FILL);
   mode = DEFINE;
}


static void
reshape(GLsizei w, GLsizei h)
{
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0.0, (GLdouble) w, 0.0, (GLdouble) h, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   set_screen_wh(w, h);
}


static void
usage(void)
{
   printf("Use left mouse button to place vertices.\n");
   printf("Press middle mouse button when done.\n");
   printf("Select tesselate from the pop-up menu.\n");
}


/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int
main(int argc, char **argv)
{
   usage();
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
   glutInitWindowSize(400, 400);
   glutCreateWindow(argv[0]);
   myinit();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutMainLoop();
   return 0;
}
