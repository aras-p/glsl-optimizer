/* $Id: isosurf.c,v 1.5 2000/03/30 17:58:56 keithw Exp $ */

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

/*
 * $Log: isosurf.c,v $
 * Revision 1.5  2000/03/30 17:58:56  keithw
 * Added stipple mode
 *
 * Revision 1.4  1999/10/21 16:39:06  brianp
 * added -info command line option
 *
 * Revision 1.3  1999/09/08 22:14:31  brianp
 * minor changes. always call compactify_arrays()
 *
 * Revision 1.2  1999/09/03 14:56:40  keithw
 * Fog, displaylist and zoom operations
 *
 * Revision 3.4  1999/04/24 01:10:47  keithw
 * clip planes, materials
 *
 * Revision 3.3  1999/03/31 19:42:14  keithw
 * support for cva
 *
 * Revision 3.1  1998/11/01 20:30:20  brianp
 * added benchmark feature (b key)
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "GL/glut.h"

#include "../util/readtex.c"   /* I know, this is a hack.  KW: me too. */
#define TEXTURE_FILE "../images/reflect.rgb"

#define LIT                 0x1
#define UNLIT               0x2
#define TEXTURE             0x4
#define NO_TEXTURE          0x8
#define REFLECT             0x10
#define NO_REFLECT          0x20
#define POINT_FILTER        0x40
#define LINEAR_FILTER       0x80
#define GLVERTEX           0x100
#define DRAW_ARRAYS         0x200 /* or draw_elts, if compiled */
#define ARRAY_ELT           0x400
#define COMPILED            0x800
#define IMMEDIATE           0x1000
#define SHADE_SMOOTH        0x2000
#define SHADE_FLAT          0x4000
#define TRIANGLES           0x8000
#define STRIPS              0x10000
#define USER_CLIP           0x20000
#define NO_USER_CLIP        0x40000
#define MATERIALS           0x80000
#define NO_MATERIALS        0x100000
#define FOG                 0x200000
#define NO_FOG              0x400000
#define QUIT                0x800000
#define DISPLAYLIST         0x1000000
#define GLINFO              0x2000000
#define STIPPLE             0x4000000
#define NO_STIPPLE          0x8000000

#define LIGHT_MASK  (LIT|UNLIT)
#define TEXTURE_MASK (TEXTURE|NO_TEXTURE)
#define REFLECT_MASK (REFLECT|NO_REFLECT)
#define FILTER_MASK (POINT_FILTER|LINEAR_FILTER)
#define RENDER_STYLE_MASK (GLVERTEX|DRAW_ARRAYS|ARRAY_ELT)
#define COMPILED_MASK (COMPILED|IMMEDIATE|DISPLAYLIST)
#define MATERIAL_MASK (MATERIALS|NO_MATERIALS)
#define PRIMITIVE_MASK (TRIANGLES|STRIPS)
#define CLIP_MASK (USER_CLIP|NO_USER_CLIP)
#define SHADE_MASK (SHADE_SMOOTH|SHADE_FLAT)
#define FOG_MASK (FOG|NO_FOG)
#define STIPPLE_MASK (STIPPLE|NO_STIPPLE)

#define MAXVERTS 10000
static float data[MAXVERTS][6];
static float compressed_data[MAXVERTS][6];
static GLuint indices[MAXVERTS];
static GLuint tri_indices[MAXVERTS*3];
static GLfloat col[100][4];
static GLint numverts, num_tri_verts, numuniq;

static GLfloat xrot;
static GLfloat yrot;
static GLfloat dist = -6;
static GLint state, allowed = ~0;
static GLboolean doubleBuffer = GL_TRUE;
static GLdouble plane[4] = {1.0, 0.0, -1.0, 0.0};
static GLuint surf1;

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

/* forward decl */
int BuildList( int mode );


static void read_surface( char *filename )
{
   FILE *f;

   f = fopen(filename,"r");
   if (!f) {
      printf("couldn't read %s\n", filename);
      exit(1);
   }

   numverts = 0;
   while (!feof(f) && numverts<MAXVERTS) {
      fscanf( f, "%f %f %f  %f %f %f",
	      &data[numverts][0], &data[numverts][1], &data[numverts][2],
	      &data[numverts][3], &data[numverts][4], &data[numverts][5] );
      numverts++;
   }
   numverts--;

   printf("%d vertices, %d triangles\n", numverts, numverts-2);
   fclose(f);
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

static float myrand( float max )
{
   return max*rand()/(RAND_MAX+1.0);
}


static void make_tri_indices( void )
{
   unsigned int *v = tri_indices;
   unsigned int parity = 0;
   unsigned int i, j;

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
}

#define MIN(x,y) (x < y) ? x : y

static void draw_surface( int with_state )
{
   GLuint i, j;

   switch (with_state & (COMPILED_MASK|RENDER_STYLE_MASK|PRIMITIVE_MASK)) {
#ifdef GL_EXT_vertex_array

   case (COMPILED|DRAW_ARRAYS|STRIPS):
      glDrawElements( GL_TRIANGLE_STRIP, numverts, GL_UNSIGNED_INT, indices );
      break;


   case (COMPILED|ARRAY_ELT|STRIPS):
      glBegin( GL_TRIANGLE_STRIP );
      for (i = 0 ; i < numverts ; i++) 
	 glArrayElement( indices[i] );      
      glEnd();
      break;

   case (COMPILED|DRAW_ARRAYS|TRIANGLES):
   case (IMMEDIATE|DRAW_ARRAYS|TRIANGLES):
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

      /* Uses the original arrays (including duplicate elements):
       */
   case (IMMEDIATE|DRAW_ARRAYS|STRIPS):
      glDrawArraysEXT( GL_TRIANGLE_STRIP, 0, numverts );
      break;

      /* Uses the original arrays (including duplicate elements):
       */
   case (IMMEDIATE|ARRAY_ELT|STRIPS):
      glBegin( GL_TRIANGLE_STRIP );
      for (i = 0 ; i < numverts ; i++)
	 glArrayElement( i );
      glEnd();
      break;

   case (IMMEDIATE|ARRAY_ELT|TRIANGLES):
   case (COMPILED|ARRAY_ELT|TRIANGLES):
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

   case (IMMEDIATE|GLVERTEX|TRIANGLES):
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

   case (DISPLAYLIST|GLVERTEX|STRIPS):
      if (!surf1)
	 surf1 = BuildList( GL_COMPILE_AND_EXECUTE );
      else
	 glCallList(surf1);
      break;

#endif

      /* Uses the original arrays (including duplicate elements):
       */
   default:
      glBegin( GL_TRIANGLE_STRIP );
      for (i=0;i<numverts;i++) {
         glNormal3fv( &data[i][3] );
         glVertex3fv( &data[i][0] );
      }
      glEnd();
   }
}



static void Display(void)
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    draw_surface( state );
    glFlush();
    if (doubleBuffer) glutSwapBuffers();    
}

int BuildList( int mode )
{
   int rv = glGenLists(1);
   glNewList(rv, mode );
   draw_surface( IMMEDIATE|GLVERTEX|STRIPS );
   glEndList();
   return rv;
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
#define CHANGED(o,n,mask) ((n&mask) && \
                           (n&mask) != (o&mask) ? UPDATE(o,n,mask) : 0)

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
      if (m & LINEAR_FILTER) {
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      } else {
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
   }

   if (CHANGED(state, m, LIGHT_MASK)) {
      if (m & LIT)
	 glEnable(GL_LIGHTING);
      else
	 glDisable(GL_LIGHTING);
   }

   if (CHANGED(state, m, SHADE_MASK)) {
      if (m & SHADE_SMOOTH)
	 glShadeModel(GL_SMOOTH);
      else
	 glShadeModel(GL_FLAT);
   }


   if (CHANGED(state, m, TEXTURE_MASK)) {
      if (m & TEXTURE) 
	 glEnable(GL_TEXTURE_2D);
      else
	 glDisable(GL_TEXTURE_2D);
   }

   if (CHANGED(state, m, REFLECT_MASK)) {
      if (m & REFLECT) {
	 glEnable(GL_TEXTURE_GEN_S);
	 glEnable(GL_TEXTURE_GEN_T);
      } else {
	 glDisable(GL_TEXTURE_GEN_S);
	 glDisable(GL_TEXTURE_GEN_T);
      }
   }

   if (CHANGED(state, m, CLIP_MASK)) {
      if (m & USER_CLIP) {
	 glEnable(GL_CLIP_PLANE0);
      } else {
	 glDisable(GL_CLIP_PLANE0);
      }
   }

   if (CHANGED(state, m, FOG_MASK)) {
      if (m & FOG) 
      {
	 glEnable(GL_FOG);
	 printf("FOG enable\n");
      } 
      else 
      {
	 glDisable(GL_FOG);
	 printf("FOG disable\n");
      }
   }

   if (CHANGED(state, m, STIPPLE_MASK)) {
      if (m & STIPPLE) 
      {
	 glEnable(GL_POLYGON_STIPPLE);
	 printf("STIPPLE enable\n");
      } 
      else 
      {
	 glDisable(GL_POLYGON_STIPPLE);
	 printf("STIPPLE disable\n");
      }
   }

#ifdef GL_EXT_vertex_array
   if (CHANGED(state, m, (COMPILED_MASK|RENDER_STYLE_MASK|PRIMITIVE_MASK))) 
   {
      if ((m & (COMPILED_MASK|PRIMITIVE_MASK)) == (IMMEDIATE|STRIPS))
      {
	 glVertexPointerEXT( 3, GL_FLOAT, sizeof(data[0]), numverts, data );
	 glNormalPointerEXT( GL_FLOAT, sizeof(data[0]), numverts, &data[0][3]);
      }
      else 
      {
	 glVertexPointerEXT( 3, GL_FLOAT, sizeof(data[0]), numuniq, 
			     compressed_data );
	 glNormalPointerEXT( GL_FLOAT, sizeof(data[0]), numuniq, 
			     &compressed_data[0][3]);
      }
#ifdef GL_EXT_compiled_vertex_array
      if (allowed & COMPILED) {
	 if (m & COMPILED) {
	    glLockArraysEXT( 0, numuniq );
	 } else {
	    glUnlockArraysEXT();
	 }
      }
#endif
   }
#endif

   if (m & (RENDER_STYLE_MASK|PRIMITIVE_MASK)) {
      UPDATE(state, m, (RENDER_STYLE_MASK|PRIMITIVE_MASK));
   }
   
   if (m & MATERIAL_MASK) {
      UPDATE(state, m, MATERIAL_MASK);
   }

   glutPostRedisplay();
}



static void Init(int argc, char *argv[])
{
   GLfloat fogColor[4] = {0.5,1.0,0.5,1.0};

   glClearColor(0.0, 0.0, 1.0, 0.0);
   glEnable( GL_DEPTH_TEST );
   glEnable( GL_VERTEX_ARRAY_EXT );
   glEnable( GL_NORMAL_ARRAY_EXT );

   InitMaterials();

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5, 25 );

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glClipPlane(GL_CLIP_PLANE0, plane); 

   set_matrix();

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

   if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }

   /* Green fog is easy to see */
   glFogi(GL_FOG_MODE,GL_EXP2);
   glFogfv(GL_FOG_COLOR,fogColor);
   glFogf(GL_FOG_DENSITY,0.15);
   glHint(GL_FOG_HINT,GL_DONT_CARE);


   compactify_arrays();
   make_tri_indices();

   surf1 = BuildList( GL_COMPILE );

   ModeMenu(SHADE_SMOOTH|
	    LIT|
	    NO_TEXTURE|
	    NO_REFLECT|
	    POINT_FILTER|
	    IMMEDIATE|
	    NO_USER_CLIP|
	    NO_MATERIALS|
	    NO_FOG|
	    NO_STIPPLE|
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
      ModeMenu((state ^ LIGHT_MASK) & LIGHT_MASK);
      break;
   case 'm':
      ModeMenu((state ^ MATERIAL_MASK) & MATERIAL_MASK);
      break;
   case 'c':
      ModeMenu((state ^ CLIP_MASK) & CLIP_MASK);
      break;
   case 'v':
      if (allowed&COMPILED)
	 ModeMenu(COMPILED|DRAW_ARRAYS|TRIANGLES);
      break;
   case 'V':
      ModeMenu(IMMEDIATE|GLVERTEX|STRIPS);
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
      allowed &= ~(COMPILED|DRAW_ARRAYS|ARRAY_ELT);
   }
   else if (!strstr( extensions, "GL_EXT_compiled_vertex_array" )) 
   {
      printf("Compiled vertex arrays not supported by this renderer\n");
      allowed &= ~COMPILED;
   }

   Init(argc, argv);
   ModeMenu(arg_mode);
   
   glutCreateMenu(ModeMenu);
   glutAddMenuEntry("GL info",               GLINFO);   
   glutAddMenuEntry("", 0);   
   glutAddMenuEntry("Lit",                   LIT|NO_TEXTURE|NO_REFLECT);
   glutAddMenuEntry("Unlit",                 UNLIT|NO_TEXTURE|NO_REFLECT);
/*    glutAddMenuEntry("Textured", TEXTURE); */
   glutAddMenuEntry("Reflect",               TEXTURE|REFLECT);
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
   glutAddMenuEntry("Point Filtered",        POINT_FILTER);
   glutAddMenuEntry("Linear Filtered",       LINEAR_FILTER);
   glutAddMenuEntry("", 0);   
   glutAddMenuEntry("glVertex (STRIPS)",    IMMEDIATE|GLVERTEX|STRIPS);
   glutAddMenuEntry("glVertex (TRIANGLES)", IMMEDIATE|GLVERTEX|TRIANGLES);
   glutAddMenuEntry("", 0);   
   glutAddMenuEntry("glVertex display list (STRIPS)", 
		    DISPLAYLIST|GLVERTEX|STRIPS);
   glutAddMenuEntry("", 0);   
   if (allowed & DRAW_ARRAYS) {
      glutAddMenuEntry("DrawArrays (STRIPS)",
		       IMMEDIATE|DRAW_ARRAYS|STRIPS);
      glutAddMenuEntry("ArrayElement (STRIPS)",  
		       IMMEDIATE|ARRAY_ELT|STRIPS);
      glutAddMenuEntry("DrawElements (TRIANGLES)", 
		       IMMEDIATE|DRAW_ARRAYS|TRIANGLES);
      glutAddMenuEntry("ArrayElement (TRIANGLES)", 
		       IMMEDIATE|ARRAY_ELT|TRIANGLES);
      glutAddMenuEntry("", 0);   

   }
   if (allowed & COMPILED) {
      glutAddMenuEntry("Compiled DrawElements (TRIANGLES)", 
		       COMPILED|DRAW_ARRAYS|TRIANGLES);
      glutAddMenuEntry("Compiled DrawElements (STRIPS)", 
		       COMPILED|DRAW_ARRAYS|STRIPS);
      glutAddMenuEntry("Compiled ArrayElement (TRIANGLES)", 
		       COMPILED|ARRAY_ELT|TRIANGLES);   
      glutAddMenuEntry("Compiled ArrayElement (STRIPS)", 
		       COMPILED|ARRAY_ELT|STRIPS);   
      glutAddMenuEntry("", 0);   
   }
   glutAddMenuEntry("Quit",                  QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Display);

   glutMainLoop();
   return 0;
}
