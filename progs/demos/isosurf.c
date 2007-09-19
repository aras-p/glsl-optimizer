
/*
 * Display an isosurface of 3-D wind speed volume.
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 * Brian Paul  This file in public domain.
 */


/* Keys:
 * =====
 *
 *   - Arrow keys to rotate
 *   - 's' toggles smooth shading
 *   - 'l' toggles lighting
 *   - 'f' toggles fog
 *   - 'I' and 'i' zoom in and out
 *   - 'c' toggles a user clip plane
 *   - 'm' toggles colorful materials in GL_TRIANGLES modes.
 *   - '+' and '-' move the user clip plane
 *
 * Other options are available via the popup menu.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#undef CLIP_MASK
#endif
#define GL_GLEXT_PROTOTYPES
#include "GL/glut.h"

#include "readtex.h"
#define TEXTURE_FILE "../images/reflect.rgb"

#define LIT		0x00000001
#define UNLIT		0x00000002
#define REFLECT		0x00000004
#define POINT_FILTER	0x00000008
#define LINEAR_FILTER	0x00000010
#define GLVERTEX	0x00000020
#define DRAW_ELTS	0x00000040 
#define DRAW_ARRAYS	0x00000080 
#define ARRAY_ELT	0x00000100
#define LOCKED	        0x00000200
#define UNLOCKED	0x00000400 
#define IMMEDIATE	0x00000800
#define DISPLAYLIST	0x00001000
#define SHADE_SMOOTH	0x00002000
#define SHADE_FLAT	0x00004000
#define TRIANGLES	0x00008000
#define STRIPS		0x00010000
#define POINTS		0x00020000
#define USER_CLIP	0x00040000
#define NO_USER_CLIP	0x00080000
#define MATERIALS	0x00100000
#define NO_MATERIALS	0x00200000
#define FOG		0x00400000
#define NO_FOG		0x00800000
#define QUIT		0x01000000
#define GLINFO		0x02000000
#define STIPPLE		0x04000000
#define NO_STIPPLE	0x08000000
#define POLYGON_FILL	0x10000000
#define POLYGON_LINE	0x20000000

#define LIGHT_MASK		(LIT|UNLIT|REFLECT)
#define FILTER_MASK		(POINT_FILTER|LINEAR_FILTER)
#define RENDER_STYLE_MASK	(GLVERTEX|DRAW_ARRAYS|DRAW_ELTS|ARRAY_ELT)
#define DLIST_MASK		(IMMEDIATE|DISPLAYLIST)
#define LOCK_MASK		(LOCKED|UNLOCKED)
#define MATERIAL_MASK		(MATERIALS|NO_MATERIALS)
#define PRIMITIVE_MASK		(TRIANGLES|STRIPS|POINTS)
#define CLIP_MASK		(USER_CLIP|NO_USER_CLIP)
#define SHADE_MASK		(SHADE_SMOOTH|SHADE_FLAT)
#define FOG_MASK		(FOG|NO_FOG)
#define STIPPLE_MASK		(STIPPLE|NO_STIPPLE)
#define POLYGON_MASK		(POLYGON_FILL|POLYGON_LINE)

#define MAXVERTS 10000
static GLint maxverts = MAXVERTS;
static float data[MAXVERTS][6];
static float compressed_data[MAXVERTS][6];
static float expanded_data[MAXVERTS*3][6];
static GLuint indices[MAXVERTS];
static GLuint tri_indices[MAXVERTS*3];
static GLuint strip_indices[MAXVERTS];
static GLfloat col[100][4];
static GLint numverts, num_tri_verts, numuniq;

static GLfloat xrot;
static GLfloat yrot;
static GLfloat dist;
static GLint state, allowed = ~0;
static GLboolean doubleBuffer = GL_TRUE;
static GLdouble plane[4];
static GLuint surf1, dlist_state;

static GLboolean PrintInfo = GL_FALSE;


static GLubyte halftone[] = {
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
   0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
   0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
   0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
   0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA,
   0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
   0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55};


static void read_surface( char *filename )
{
   FILE *f;

   f = fopen(filename,"r");
   if (!f) {
      printf("couldn't read %s\n", filename);
      exit(1);
   }

   numverts = 0;
   while (!feof(f) && numverts<maxverts) {
      fscanf( f, "%f %f %f  %f %f %f",
	      &data[numverts][0], &data[numverts][1], &data[numverts][2],
	      &data[numverts][3], &data[numverts][4], &data[numverts][5] );
      numverts++;
   }
   numverts--;

   printf("%d vertices, %d triangles\n", numverts, numverts-2);
   fclose(f);
}



static void print_flags( const char *msg, GLuint flags ) 
{
   fprintf(stderr, 
	   "%s (0x%x): %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg, flags,
	   (flags & GLVERTEX) ? "glVertex, " : "",
	   (flags & DRAW_ARRAYS) ? "glDrawArrays, " : "",
	   (flags & DRAW_ELTS) ? "glDrawElements, " : "",
	   (flags & ARRAY_ELT) ? "glArrayElement, " : "",
	   (flags & LOCKED) ? "locked arrays, " : "",
	   (flags & TRIANGLES) ? "GL_TRIANGLES, " : "",
	   (flags & STRIPS) ? "GL_TRIANGLE_STRIP, " : "",
	   (flags & POINTS) ? "GL_POINTS, " : "",
	   (flags & DISPLAYLIST) ? "as a displaylist, " : "",
	   (flags & LIT) ? "lit, " : "",
	   (flags & UNLIT) ? "unlit, " : "",
	   (flags & REFLECT) ? "reflect, " : "",
	   (flags & SHADE_FLAT) ? "flat-shaded, " : "",
	   (flags & USER_CLIP) ? "user_clip, " : "",
	   (flags & MATERIALS) ? "materials, " : "",
	   (flags & FOG) ? "fog, " : "",
	   (flags & STIPPLE) ? "stipple, " : "",
	   (flags & POLYGON_LINE) ? "polygon mode line, " : "");
}



struct data_idx {
   float *data;
   int idx;
   int uniq_idx;
};


#define COMPARE_FUNC( AXIS )                            \
static int compare_axis_##AXIS( const void *a, const void *b )	\
{							\
   float t = ( (*(struct data_idx *)a).data[AXIS] -	\
	       (*(struct data_idx *)b).data[AXIS] );	\
   							\
   if (t < 0) return -1;				\
   if (t > 0) return 1;					\
   return 0;						\
}

COMPARE_FUNC(0)
COMPARE_FUNC(1)
COMPARE_FUNC(2)
COMPARE_FUNC(3)
COMPARE_FUNC(4)
COMPARE_FUNC(5)
COMPARE_FUNC(6)

int (*(compare[7]))( const void *a, const void *b ) =
{
   compare_axis_0,
   compare_axis_1,
   compare_axis_2,
   compare_axis_3,
   compare_axis_4,
   compare_axis_5,
   compare_axis_6,
};


#define VEC_ELT(f, s, i)  (float *)(((char *)f) + s * i)

static int sort_axis( int axis,
		      int vec_size,
		      int vec_stride,
		      struct data_idx *indices,
		      int start,
		      int finish,
		      float *out,
		      int uniq,
		      const float fudge )
{
   int i;

   if (finish-start > 2)
   {
      qsort( indices+start, finish-start, sizeof(*indices), compare[axis] );
   }
   else if (indices[start].data[axis] > indices[start+1].data[axis])
   {
      struct data_idx tmp = indices[start];
      indices[start] = indices[start+1];
      indices[start+1] = tmp;
   }

   if (axis == vec_size-1) {
      for (i = start ; i < finish ; ) {
	 float max = indices[i].data[axis] + fudge;
	 float *dest = VEC_ELT(out, vec_stride, uniq);
	 int j;

	 for (j = 0 ; j < vec_size ; j++)
	    dest[j] = indices[i].data[j];

	 for ( ; i < finish && max >= indices[i].data[axis]; i++)
	    indices[i].uniq_idx = uniq;

	 uniq++;
      }
   } else {
      for (i = start ; i < finish ; ) {
	 int j = i + 1;
	 float max = indices[i].data[axis] + fudge;
	 while (j < finish && max >= indices[j].data[axis]) j++;
	 if (j == i+1) {
	    float *dest = VEC_ELT(out, vec_stride, uniq);
	    int k;

	    indices[i].uniq_idx = uniq;

	    for (k = 0 ; k < vec_size ; k++)
	       dest[k] = indices[i].data[k];

	    uniq++;
	 } else {
	    uniq = sort_axis( axis+1, vec_size, vec_stride,
			      indices, i, j, out, uniq, fudge );
	 }
	 i = j;
      }
   }

   return uniq;
}


static void extract_indices1( const struct data_idx *in, unsigned int *out,
			      int n )
{
   int i;
   for ( i = 0 ; i < n ; i++ ) {
      out[in[i].idx] = in[i].uniq_idx;
   }
}


static void compactify_arrays(void)
{
   int i;
   struct data_idx *ind;

   ind = (struct data_idx *) malloc( sizeof(struct data_idx) * numverts );

   for (i = 0 ; i < numverts ; i++) {
      ind[i].idx = i;
      ind[i].data = data[i];
   }

   numuniq = sort_axis(0,
		       sizeof(compressed_data[0])/sizeof(float),
		       sizeof(compressed_data[0]),
		       ind,
		       0,
		       numverts,
		       (float *)compressed_data,
		       0,
		       1e-6);

   printf("Nr unique vertex/normal pairs: %d\n", numuniq);

   extract_indices1( ind, indices, numverts );
   free( ind );
}

static void expand_arrays(void) 
{
   int i;
   int parity = 0;
   for (i = 2 ; i < numverts ; i++, parity ^= 1) {
      int v0 = i-2+parity;
      int v1 = i-1-parity;
      int v2 = i;
      memcpy( expanded_data[(i-2)*3+0], data[v0], sizeof(data[0]) );
      memcpy( expanded_data[(i-2)*3+1], data[v1], sizeof(data[0]) );
      memcpy( expanded_data[(i-2)*3+2], data[v2], sizeof(data[0]) );
   }
}

static float myrand( float max )
{
   return max*rand()/(RAND_MAX+1.0);
}


static void make_tri_indices( void )
{
   unsigned int *v = tri_indices;
   unsigned int parity = 0;
   int i, j;

   for (j=2;j<numverts;j++,parity^=1) {
      if (parity) {
	 *v++ = indices[j-1];
	 *v++ = indices[j-2];
	 *v++ = indices[j];
      } else {
	 *v++ = indices[j-2];
	 *v++ = indices[j-1];
	 *v++ = indices[j];
      }
   }

   num_tri_verts = v - tri_indices;
   printf("num_tri_verts: %d\n", num_tri_verts);

   for (i = j = 0 ; i < num_tri_verts ; i += 600, j++) {
      col[j][3] = 1;
      col[j][2] = myrand(1);
      col[j][1] = myrand(1);
      col[j][0] = myrand(1);
   }

   for (i = 0; i < numverts ; i++)
      strip_indices[i] = i;
}

#define MIN(x,y) (x < y) ? x : y

static void draw_surface( unsigned int with_state )
{
   GLint i, j;
   
   if (with_state & DISPLAYLIST) {
      if ((with_state & (RENDER_STYLE_MASK|PRIMITIVE_MASK|MATERIAL_MASK)) != 
	  dlist_state) {
	 /* 
	  */
	 fprintf(stderr, "rebuilding displaylist\n");

	 if (dlist_state)
	    glDeleteLists( surf1, 1 );

	 dlist_state = with_state & (RENDER_STYLE_MASK|PRIMITIVE_MASK|
				     MATERIAL_MASK);
	 surf1 = glGenLists(1);
	 glNewList(surf1, GL_COMPILE);
	 draw_surface( dlist_state );
	 glEndList();
      }

      glCallList( surf1 );
      return;
   }

   switch (with_state & (RENDER_STYLE_MASK|PRIMITIVE_MASK)) {
#ifdef GL_EXT_vertex_array

   case (DRAW_ELTS|TRIANGLES):
      if (with_state & MATERIALS) {
	 for (j = i = 0 ; i < num_tri_verts ; i += 600, j++) {
	    GLuint nr = MIN(num_tri_verts-i, 600);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, col[j]);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col[j]);
	    glDrawElements( GL_TRIANGLES, nr, GL_UNSIGNED_INT, tri_indices+i );
	 }
      } else {
	 glDrawElements( GL_TRIANGLES, num_tri_verts, GL_UNSIGNED_INT,
			 tri_indices );
      }
      break;

   case (DRAW_ARRAYS|TRIANGLES):
      glDrawArraysEXT( GL_TRIANGLES, 0, (numverts-2)*3 );
      break;

   case (ARRAY_ELT|TRIANGLES):
      if (with_state & MATERIALS) {
	 for (j = i = 0 ; i < num_tri_verts ; i += 600, j++) {
	    GLuint nr = MIN(num_tri_verts-i, 600);
	    GLuint k;
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, col[j]);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col[j]);
	    glBegin( GL_TRIANGLES );
	    for (k = 0 ; k < nr ; k++)
	       glArrayElement( tri_indices[i+k] );
	    glEnd();
	 }
      } else {
	 glBegin( GL_TRIANGLES );
	 for (i = 0 ; i < num_tri_verts ; i++)
	    glArrayElement( tri_indices[i] );

	 glEnd();
      }
      break;


      /* Uses the original arrays (including duplicate elements):
       */
   case (DRAW_ARRAYS|STRIPS):
      glDrawArraysEXT( GL_TRIANGLE_STRIP, 0, numverts );
      break;
   case (DRAW_ELTS|STRIPS):
      glDrawElements( GL_TRIANGLE_STRIP, numverts,
		      GL_UNSIGNED_INT, strip_indices );
      break;

      /* Uses the original arrays (including duplicate elements):
       */
   case (ARRAY_ELT|STRIPS):
      glBegin( GL_TRIANGLE_STRIP );
      for (i = 0 ; i < numverts ; i++)
	 glArrayElement( i );
      glEnd();
      break;

   case (DRAW_ARRAYS|POINTS):
      glDrawArraysEXT( GL_POINTS, 0, numuniq );
      break;
   case (DRAW_ELTS|POINTS):
      /* can use numuniq with strip_indices as strip_indices[i] == i.
       */
      glDrawElements( GL_POINTS, numuniq, 
		      GL_UNSIGNED_INT, strip_indices );
      break;
   case (ARRAY_ELT|POINTS):
      /* just emit each unique element once:
       */
      glBegin( GL_POINTS );
      for (i = 0 ; i < numuniq ; i++)
	 glArrayElement( i );
      glEnd();
      break;
#endif

   case (GLVERTEX|TRIANGLES):
      if (with_state & MATERIALS) {
	 for (j = i = 0 ; i < num_tri_verts ; i += 600, j++) {
	    GLuint nr = MIN(num_tri_verts-i, 600);
	    GLuint k;
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, col[j]);
	    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col[j]);
	    glBegin( GL_TRIANGLES );
	    for (k = 0 ; k < nr ; k++) {
	       glNormal3fv( &compressed_data[tri_indices[i+k]][3] );
	       glVertex3fv( &compressed_data[tri_indices[i+k]][0] );
	    }
	    glEnd();
	 }
      } else {
	 glBegin( GL_TRIANGLES );
	 for (i = 0 ; i < num_tri_verts ; i++) {
	    glNormal3fv( &compressed_data[tri_indices[i]][3] );
	    glVertex3fv( &compressed_data[tri_indices[i]][0] );
	 }
	 glEnd();
      }
      break;

   case (GLVERTEX|POINTS):
      /* Renders all points, but not in strip order...  Shouldn't be a
       * problem, but people may be confused as to why points are so
       * much faster in this demo...  And why cva doesn't help them...
       */
      glBegin( GL_POINTS );
      for ( i = 0 ; i < numuniq ; i++ ) {
         glNormal3fv( &compressed_data[i][3] );
         glVertex3fv( &compressed_data[i][0] );
      }
      glEnd();
      break;

   case (GLVERTEX|STRIPS):
      glBegin( GL_TRIANGLE_STRIP );
      for (i=0;i<numverts;i++) {
         glNormal3fv( &data[i][3] );
         glVertex3fv( &data[i][0] );
      }
      glEnd();
      break;

   default:
      fprintf(stderr, "unimplemented mode %x...\n", 
	      (with_state & (RENDER_STYLE_MASK|PRIMITIVE_MASK)));
      break;
   }
}



static void Display(void)
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    draw_surface( state );
    glFlush();
    if (doubleBuffer) glutSwapBuffers();
}


/* KW: only do this when necessary, so CVA can re-use results.
 */
static void set_matrix( void )
{
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, dist );
   glRotatef( yrot, 0.0, 1.0, 0.0 );
   glRotatef( xrot, 1.0, 0.0, 0.0 );
}

static void Benchmark( float xdiff, float ydiff )
{
   int startTime, endTime;
   int draws;
   double seconds, fps, triPerSecond;

   printf("Benchmarking...\n");

   draws = 0;
   startTime = glutGet(GLUT_ELAPSED_TIME);
   xrot = 0.0;
   do {
      xrot += xdiff;
      yrot += ydiff;
      set_matrix();
      Display();
      draws++;
      endTime = glutGet(GLUT_ELAPSED_TIME);
   } while (endTime - startTime < 5000);   /* 5 seconds */

   /* Results */
   seconds = (double) (endTime - startTime) / 1000.0;
   triPerSecond = (numverts - 2) * draws / seconds;
   fps = draws / seconds;
   printf("Result:  triangles/sec: %g  fps: %g\n", triPerSecond, fps);
}


static void InitMaterials(void)
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position0[] = {0.0, 0.0, 20.0, 0.0};
    static float position1[] = {0.0, 0.0, -20.0, 0.0};
    static float front_mat_shininess[] = {60.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    /*
    static float back_mat_shininess[] = {60.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    */
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_FALSE};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position0);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);
    glEnable(GL_LIGHT1);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, front_mat_diffuse);

    glPolygonStipple (halftone);
}



#define UPDATE(o,n,mask) (o&=~mask, o|=n&mask)
#define CHANGED(o,n,mask) ((n&mask) && (n&mask) != (o&mask) )

static void ModeMenu(int m)
{
   m &= allowed;

   if (!m) return;

   if (m==QUIT)
      exit(0);

   if (m==GLINFO) {
      printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_EXTENSIONS: %s\n", (char *) glGetString(GL_EXTENSIONS));
      printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
      return;
   }

   if (CHANGED(state, m, FILTER_MASK)) {
      UPDATE(state, m, FILTER_MASK);
      if (m & LINEAR_FILTER) {
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      } else {
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
   }

   if (CHANGED(state, m, LIGHT_MASK)) {
      UPDATE(state, m, LIGHT_MASK);
      if (m & LIT) {
	 glEnable(GL_LIGHTING);
	 glDisable(GL_TEXTURE_GEN_S);
	 glDisable(GL_TEXTURE_GEN_T);
	 glDisable(GL_TEXTURE_2D);
      }
      else if (m & UNLIT) {
	 glDisable(GL_LIGHTING);
	 glDisable(GL_TEXTURE_GEN_S);
	 glDisable(GL_TEXTURE_GEN_T);
	 glDisable(GL_TEXTURE_2D);
      }
      else if (m & REFLECT) {
	 glDisable(GL_LIGHTING);
	 glEnable(GL_TEXTURE_GEN_S);
	 glEnable(GL_TEXTURE_GEN_T);
	 glEnable(GL_TEXTURE_2D);
      }
   }

   if (CHANGED(state, m, SHADE_MASK)) {
      UPDATE(state, m, SHADE_MASK);
      if (m & SHADE_SMOOTH)
	 glShadeModel(GL_SMOOTH);
      else
	 glShadeModel(GL_FLAT);
   }


   if (CHANGED(state, m, CLIP_MASK)) {
      UPDATE(state, m, CLIP_MASK);
      if (m & USER_CLIP) {
	 glEnable(GL_CLIP_PLANE0);
      } else {
	 glDisable(GL_CLIP_PLANE0);
      }
   }

   if (CHANGED(state, m, FOG_MASK)) {
      UPDATE(state, m, FOG_MASK);
      if (m & FOG) {
	 glEnable(GL_FOG);
      }
      else {
	 glDisable(GL_FOG);
      }
   }

   if (CHANGED(state, m, STIPPLE_MASK)) {
      UPDATE(state, m, STIPPLE_MASK);
      if (m & STIPPLE) {
	 glEnable(GL_POLYGON_STIPPLE);
      }
      else {
	 glDisable(GL_POLYGON_STIPPLE);
      }
   }

   if (CHANGED(state, m, POLYGON_MASK)) {
      UPDATE(state, m, POLYGON_MASK);
      if (m & POLYGON_FILL) {
	 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
      else {
	 glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
   }

#ifdef GL_EXT_vertex_array
   if (CHANGED(state, m, (LOCK_MASK|RENDER_STYLE_MASK|PRIMITIVE_MASK)))
   {
      if (m & (PRIMITIVE_MASK)) {
	 UPDATE(state, m, (PRIMITIVE_MASK));
      }

      if (m & (RENDER_STYLE_MASK)) {
	 UPDATE(state, m, (RENDER_STYLE_MASK));
      }

      if (m & LOCK_MASK) {
	 UPDATE(state, m, (LOCK_MASK));
      }


      print_flags("primitive", state & PRIMITIVE_MASK);
      print_flags("render style", state & RENDER_STYLE_MASK);

      if ((state & PRIMITIVE_MASK) != STRIPS &&
	  ((state & RENDER_STYLE_MASK) == DRAW_ELTS ||
	   (state & RENDER_STYLE_MASK) == ARRAY_ELT || 
	   (state & PRIMITIVE_MASK) == POINTS))
      {
	 fprintf(stderr, "enabling small arrays\n");
	 /* Rendering any primitive with draw-element/array-element
	  *  --> Can't do strips here as ordering has been lost in
	  *  compaction process...
	  */
	 glVertexPointerEXT( 3, GL_FLOAT, sizeof(data[0]), numuniq,
			     compressed_data );
	 glNormalPointerEXT( GL_FLOAT, sizeof(data[0]), numuniq,
			     &compressed_data[0][3]);
#ifdef GL_EXT_compiled_vertex_array
	 if (allowed & LOCKED) {
	    if (state & LOCKED) {
	       glLockArraysEXT( 0, numuniq );
	    } else {
	       glUnlockArraysEXT();
	    }
	 }
#endif
      }
      else if ((state & PRIMITIVE_MASK) == TRIANGLES &&
	       (state & RENDER_STYLE_MASK) == DRAW_ARRAYS) {
	 fprintf(stderr, "enabling big arrays\n");
	 /* Only get here for TRIANGLES and drawarrays
	  */
	 glVertexPointerEXT( 3, GL_FLOAT, sizeof(data[0]), (numverts-2) * 3,
			     expanded_data );
	 glNormalPointerEXT( GL_FLOAT, sizeof(data[0]), (numverts-2) * 3,
			     &expanded_data[0][3]);

#ifdef GL_EXT_compiled_vertex_array
	 if (allowed & LOCKED) {
	    if (state & LOCKED) {
	       glLockArraysEXT( 0, (numverts-2)*3 );
	    } else {
	       glUnlockArraysEXT();
	    }
	 }
#endif
      }
      else {
	 fprintf(stderr, "enabling normal arrays\n");
	 glVertexPointerEXT( 3, GL_FLOAT, sizeof(data[0]), numverts, data );
	 glNormalPointerEXT( GL_FLOAT, sizeof(data[0]), numverts, &data[0][3]);
#ifdef GL_EXT_compiled_vertex_array
	 if (allowed & LOCKED) {
	    if (state & LOCKED) {
	       glLockArraysEXT( 0, numverts );
	    } else {
	       glUnlockArraysEXT();
	    }
	 }
#endif
      }

   }
#endif


   if (m & DLIST_MASK) {
      UPDATE(state, m, DLIST_MASK);
   }

   if (m & MATERIAL_MASK) {
      UPDATE(state, m, MATERIAL_MASK);
   }

   print_flags("new flags", state);

   glutPostRedisplay();
}



static void Init(int argc, char *argv[])
{
   GLfloat fogColor[4] = {0.5,1.0,0.5,1.0};

   xrot = 0;
   yrot = 0;
   dist = -6;
   plane[0] = 1.0;
   plane[1] = 0.0;
   plane[2] = -1.0;
   plane[3] = 0.0;

   glClearColor(0.0, 0.0, 1.0, 0.0);
   glEnable( GL_DEPTH_TEST );
   glEnable( GL_VERTEX_ARRAY_EXT );
   glEnable( GL_NORMAL_ARRAY_EXT );

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5, 25 );

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glClipPlane(GL_CLIP_PLANE0, plane);

   InitMaterials();

   set_matrix();

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);


   /* Green fog is easy to see */
   glFogi(GL_FOG_MODE,GL_EXP2);
   glFogfv(GL_FOG_COLOR,fogColor);
   glFogf(GL_FOG_DENSITY,0.15);
   glHint(GL_FOG_HINT,GL_DONT_CARE);

   {
      static int firsttime = 1;
      if (firsttime) {
	 firsttime = 0;
	 compactify_arrays();
	 expand_arrays();
	 make_tri_indices();

	 if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
	    printf("Error: couldn't load texture image\n");
	    exit(1);
	 }
      }
   }

   ModeMenu(SHADE_SMOOTH|
	    LIT|
	    POINT_FILTER|
	    NO_USER_CLIP|
	    NO_MATERIALS|
	    NO_FOG|
	    NO_STIPPLE|
	    IMMEDIATE|
	    STRIPS|
	    UNLOCKED|
	    GLVERTEX);

   if (PrintInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
}



static void Reshape(int width, int height)
{
    glViewport(0, 0, (GLint)width, (GLint)height);
}



static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case 27:
      exit(0);
   case 'f':
      ModeMenu((state ^ FOG_MASK) & FOG_MASK);
      break;
   case 's':
      ModeMenu((state ^ SHADE_MASK) & SHADE_MASK);
      break;
   case 't':
      ModeMenu((state ^ STIPPLE_MASK) & STIPPLE_MASK);
      break;
   case 'l':
      ModeMenu((state ^ LIGHT_MASK) & (LIT|UNLIT));
      break;
   case 'm':
      ModeMenu((state ^ MATERIAL_MASK) & MATERIAL_MASK);
      break;
   case 'c':
      ModeMenu((state ^ CLIP_MASK) & CLIP_MASK);
      break;
   case 'v':
      ModeMenu((LOCKED|IMMEDIATE|DRAW_ELTS|TRIANGLES) & allowed);
      break;
   case 'V':
      ModeMenu(UNLOCKED|IMMEDIATE|GLVERTEX|STRIPS);
      break;
   case 'b':
      Benchmark(5.0, 0);
      break;
   case 'B':
      Benchmark(0, 5.0);
      break;
   case 'i':
      dist += .25;
      set_matrix();
      glutPostRedisplay();
      break;
   case 'I':
      dist -= .25;
      set_matrix();
      glutPostRedisplay();
      break;
   case '-':
   case '_':
      plane[3] += 2.0;
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glClipPlane(GL_CLIP_PLANE0, plane);
      set_matrix();
      glutPostRedisplay();
      break;
   case '+':
   case '=':
      plane[3] -= 2.0;
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glClipPlane(GL_CLIP_PLANE0, plane);
      set_matrix();
      glutPostRedisplay();
      break;
   case ' ':
      Init(0,0);
      break;
   }
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_LEFT:
      yrot -= 15.0;
      break;
   case GLUT_KEY_RIGHT:
      yrot += 15.0;
      break;
   case GLUT_KEY_UP:
      xrot += 15.0;
      break;
   case GLUT_KEY_DOWN:
      xrot -= 15.0;
      break;
   default:
      return;
   }
   set_matrix();
   glutPostRedisplay();
}



static GLint Args(int argc, char **argv)
{
   GLint i;
   GLint mode = 0;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-sb") == 0) {
         doubleBuffer = GL_FALSE;
      }
      else if (strcmp(argv[i], "-db") == 0) {
         doubleBuffer = GL_TRUE;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         PrintInfo = GL_TRUE;
      }
      else if (strcmp(argv[i], "-10") == 0) {
         maxverts = 10;
      }
      else if (strcmp(argv[i], "-100") == 0) {
	 maxverts = 100;
      }
      else if (strcmp(argv[i], "-1000") == 0) {
	 maxverts = 1000;
      }
      else {
         printf("%s (Bad option).\n", argv[i]);
	 return QUIT;
      }
   }

   return mode;
}

int main(int argc, char **argv)
{
   GLenum type;
   char *extensions;

   GLuint arg_mode = Args(argc, argv);

   if (arg_mode & QUIT)
      exit(0);

   read_surface( "isosurf.dat" );

   glutInit( &argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);

   type = GLUT_DEPTH;
   type |= GLUT_RGB;
   type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
   glutInitDisplayMode(type);

   if (glutCreateWindow("Isosurface") <= 0) {
      exit(0);
   }

   /* Make sure server supports the vertex array extension */
   extensions = (char *) glGetString( GL_EXTENSIONS );

   if (!strstr( extensions, "GL_EXT_vertex_array" ))
   {
      printf("Vertex arrays not supported by this renderer\n");
      allowed &= ~(LOCKED|DRAW_ARRAYS|DRAW_ELTS|ARRAY_ELT);
   }
   else if (!strstr( extensions, "GL_EXT_compiled_vertex_array" ))
   {
      printf("Compiled vertex arrays not supported by this renderer\n");
      allowed &= ~LOCKED;
   }

   Init(argc, argv);
   ModeMenu(arg_mode);

   glutCreateMenu(ModeMenu);
   glutAddMenuEntry("GL info",               GLINFO);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Lit",                   LIT);
   glutAddMenuEntry("Unlit",                 UNLIT);
   glutAddMenuEntry("Reflect",               REFLECT);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Smooth",                SHADE_SMOOTH);
   glutAddMenuEntry("Flat",                  SHADE_FLAT);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Fog",                   FOG);
   glutAddMenuEntry("No Fog",                NO_FOG);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Stipple",               STIPPLE);
   glutAddMenuEntry("No Stipple",            NO_STIPPLE);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Polygon Mode Fill",     POLYGON_FILL);
   glutAddMenuEntry("Polygon Mode Line",     POLYGON_LINE);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Point Filtered",        POINT_FILTER);
   glutAddMenuEntry("Linear Filtered",       LINEAR_FILTER);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("GL_TRIANGLES",          TRIANGLES);
   glutAddMenuEntry("GL_TRIANGLE_STRIPS",    STRIPS);
   glutAddMenuEntry("GL_POINTS",             POINTS);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Displaylist",           DISPLAYLIST);
   glutAddMenuEntry("Immediate",             IMMEDIATE);
   glutAddMenuEntry("", 0);
   if (allowed & LOCKED) {
      glutAddMenuEntry("Locked Arrays (CVA)", LOCKED);
      glutAddMenuEntry("Unlocked Arrays",     UNLOCKED);
      glutAddMenuEntry("", 0);
   }
   glutAddMenuEntry("glVertex",               GLVERTEX);
   if (allowed & DRAW_ARRAYS) {
      glutAddMenuEntry("glDrawElements",      DRAW_ELTS);
      glutAddMenuEntry("glDrawArrays",	      DRAW_ARRAYS);
      glutAddMenuEntry("glArrayElement",      ARRAY_ELT);
   }
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Quit",                   QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Display);

   glutMainLoop();
   return 0;
}
