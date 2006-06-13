/*----------------------------- 
 * stex3d.c GL example of the mesa 3d-texture extention to simulate procedural
 *            texturing, it uses a perlin noise and turbulence functions.
 * 
 * Author:   Daniel Barrero
 *           barrero@irit.fr
 *           dbarrero@pegasus.uniandes.edu.co
 *
 * Converted to GLUT by brianp on 1/1/98
 * Massive clean-up on 2002/10/23 by brianp
 *
 *      
 * cc stex3d.c -o stex3d -lglut -lMesaGLU -lMesaGL -lX11 -lXext -lm 
 *
 *---------------------------- */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>


#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

#define NOISE_TEXTURE 1
#define GRADIENT_TEXTURE 2

#define TORUS 1
#define SPHERE 2

static int tex_width=64, tex_height=64, tex_depth=64;
static float angx=0, angy=0, angz=0;
static int texgen = 2, animate = 1, smooth = 1, wireframe = 0;
static int CurTexture = NOISE_TEXTURE, CurObject = TORUS;


static void
BuildTorus(void)
{
   GLint i, j;
   float theta1, phi1, theta2, phi2, rings, sides;
   float v0[03], v1[3], v2[3], v3[3];
   float t0[03], t1[3], t2[3], t3[3];
   float n0[3], n1[3], n2[3], n3[3];
   float innerRadius = 0.25;
   float outerRadius = 0.5;
   float scalFac;

   rings = 16;
   sides = 12;
   scalFac = 1 / (outerRadius * 2);

   glNewList(TORUS, GL_COMPILE);
   for (i = 0; i < rings; i++) {
      theta1 = (float) i *2.0 * M_PI / rings;
      theta2 = (float) (i + 1) * 2.0 * M_PI / rings;
      for (j = 0; j < sides; j++) {
	 phi1 = (float) j *2.0 * M_PI / sides;
	 phi2 = (float) (j + 1) * 2.0 * M_PI / sides;

	 v0[0] = cos(theta1) * (outerRadius + innerRadius * cos(phi1));
	 v0[1] = -sin(theta1) * (outerRadius + innerRadius * cos(phi1));
	 v0[2] = innerRadius * sin(phi1);

	 v1[0] = cos(theta2) * (outerRadius + innerRadius * cos(phi1));
	 v1[1] = -sin(theta2) * (outerRadius + innerRadius * cos(phi1));
	 v1[2] = innerRadius * sin(phi1);
	 v2[0] = cos(theta2) * (outerRadius + innerRadius * cos(phi2));
	 v2[1] = -sin(theta2) * (outerRadius + innerRadius * cos(phi2));
	 v2[2] = innerRadius * sin(phi2);

	 v3[0] = cos(theta1) * (outerRadius + innerRadius * cos(phi2));
	 v3[1] = -sin(theta1) * (outerRadius + innerRadius * cos(phi2));
	 v3[2] = innerRadius * sin(phi2);

	 n0[0] = cos(theta1) * (cos(phi1));
	 n0[1] = -sin(theta1) * (cos(phi1));
	 n0[2] = sin(phi1);

	 n1[0] = cos(theta2) * (cos(phi1));
	 n1[1] = -sin(theta2) * (cos(phi1));
	 n1[2] = sin(phi1);

	 n2[0] = cos(theta2) * (cos(phi2));
	 n2[1] = -sin(theta2) * (cos(phi2));
	 n2[2] = sin(phi2);

	 n3[0] = cos(theta1) * (cos(phi2));
	 n3[1] = -sin(theta1) * (cos(phi2));
	 n3[2] = sin(phi2);

	 t0[0] = v0[0] * scalFac + 0.5;
	 t0[1] = v0[1] * scalFac + 0.5;
	 t0[2] = v0[2] * scalFac + 0.5;

	 t1[0] = v1[0] * scalFac + 0.5;
	 t1[1] = v1[1] * scalFac + 0.5;
	 t1[2] = v1[2] * scalFac + 0.5;

	 t2[0] = v2[0] * scalFac + 0.5;
	 t2[1] = v2[1] * scalFac + 0.5;
	 t2[2] = v2[2] * scalFac + 0.5;

	 t3[0] = v3[0] * scalFac + 0.5;
	 t3[1] = v3[1] * scalFac + 0.5;
	 t3[2] = v3[2] * scalFac + 0.5;

	 glBegin(GL_POLYGON);
	 glNormal3fv(n3);
	 glTexCoord3fv(t3);
	 glVertex3fv(v3);
	 glNormal3fv(n2);
	 glTexCoord3fv(t2);
	 glVertex3fv(v2);
	 glNormal3fv(n1);
	 glTexCoord3fv(t1);
	 glVertex3fv(v1);
	 glNormal3fv(n0);
	 glTexCoord3fv(t0);
	 glVertex3fv(v0);
	 glEnd();
      }
   }
   glEndList();
}


/*--------------------------------------------------------------------
 noise function over R3 - implemented by a pseudorandom tricubic spline 
              EXCERPTED FROM SIGGRAPH 92, COURSE 23
                        PROCEDURAL MODELING
                             Ken Perlin
                           New York University
----------------------------------------------------------------------*/


#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
#define B 128
static int p[B + B + 2];
static float g[B + B + 2][3];
#define setup(i,b0,b1,r0,r1) \
        t = vec[i] + 10000.; \
        b0 = ((int)t) & (B-1); \
        b1 = (b0+1) & (B-1); \
        r0 = t - (int)t; \
        r1 = r0 - 1.;

static float
noise3(float vec[3])
{
   int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
   float rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v;
   register int i, j;

   setup(0, bx0, bx1, rx0, rx1);
   setup(1, by0, by1, ry0, ry1);
   setup(2, bz0, bz1, rz0, rz1);

   i = p[bx0];
   j = p[bx1];

   b00 = p[i + by0];
   b10 = p[j + by0];
   b01 = p[i + by1];
   b11 = p[j + by1];

#define at(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )
#define surve(t) ( t * t * (3. - 2. * t) )
#define lerp(t, a, b) ( a + t * (b - a) )

   sx = surve(rx0);
   sy = surve(ry0);
   sz = surve(rz0);

   q = g[b00 + bz0];
   u = at(rx0, ry0, rz0);
   q = g[b10 + bz0];
   v = at(rx1, ry0, rz0);
   a = lerp(sx, u, v);

   q = g[b01 + bz0];
   u = at(rx0, ry1, rz0);
   q = g[b11 + bz0];
   v = at(rx1, ry1, rz0);
   b = lerp(sx, u, v);

   c = lerp(sy, a, b);		/* interpolate in y at lo x */

   q = g[b00 + bz1];
   u = at(rx0, ry0, rz1);
   q = g[b10 + bz1];
   v = at(rx1, ry0, rz1);
   a = lerp(sx, u, v);

   q = g[b01 + bz1];
   u = at(rx0, ry1, rz1);
   q = g[b11 + bz1];
   v = at(rx1, ry1, rz1);
   b = lerp(sx, u, v);

   d = lerp(sy, a, b);		/* interpolate in y at hi x */

   return 1.5 * lerp(sz, c, d);	/* interpolate in z */
}

static void
initNoise(void)
{
   /*long random(); */
   int i, j, k;
   float v[3], s;

   /* Create an array of random gradient vectors uniformly on the unit sphere */
   /*srandom(1); */
   srand(1);
   for (i = 0; i < B; i++) {
      do {			/* Choose uniformly in a cube */
	 for (j = 0; j < 3; j++)
	    v[j] = (float) ((rand() % (B + B)) - B) / B;
	 s = DOT(v, v);
      } while (s > 1.0);	/* If not in sphere try again */
      s = sqrt(s);
      for (j = 0; j < 3; j++)	/* Else normalize */
	 g[i][j] = v[j] / s;
   }

   /* Create a pseudorandom permutation of [1..B] */
   for (i = 0; i < B; i++)
      p[i] = i;
   for (i = B; i > 0; i -= 2) {
      k = p[i];
      p[i] = p[j = rand() % B];
      p[j] = k;
   }

   /* Extend g and p arrays to allow for faster indexing */
   for (i = 0; i < B + 2; i++) {
      p[B + i] = p[i];
      for (j = 0; j < 3; j++)
	 g[B + i][j] = g[i][j];
   }
}


static float
turbulence(float point[3], float lofreq, float hifreq)
{
   float freq, t, p[3];

   p[0] = point[0] + 123.456;
   p[1] = point[1];
   p[2] = point[2];

   t = 0;
   for (freq = lofreq; freq < hifreq; freq *= 2.) {
      t += fabs(noise3(p)) / freq;
      p[0] *= 2.;
      p[1] *= 2.;
      p[2] *= 2.;
   }
   return t - 0.3;		/* readjust to make mean value = 0.0 */
}


static void
create3Dtexture(void)
{
   unsigned char *voxels = NULL;
   int i, j, k;
   unsigned char *vp;
   float vec[3];
   int tmp;

   printf("creating 3d textures...\n");
   voxels =
      (unsigned char *)
      malloc((size_t) (4 * tex_width * tex_height * tex_depth));
   vp = voxels;
   for (i = 0; i < tex_width; i++) {
      vec[0] = i;
      for (j = 0; j < tex_height; j++) {
	 vec[1] = j;
	 for (k = 0; k < tex_depth; k++) {
	    vec[2] = k;
	    tmp = (sin(k * i * j + turbulence(vec, 0.01, 1)) + 1) * 127.5;
	    *vp++ = 0;
	    *vp++ = 0;
	    *vp++ = tmp;
	    *vp++ = tmp + 128;
	 }
      }
   }

   printf("setting up 3d texture...\n");

   glBindTexture(GL_TEXTURE_3D, NOISE_TEXTURE);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA,
		tex_width, tex_height, tex_depth,
		0, GL_RGBA, GL_UNSIGNED_BYTE, voxels);

   free(voxels);

   printf("finished setting up 3d texture image.\n");
}


static void
printHelp(void)
{
   printf("\nUsage: stex3d  <cmd line options>\n");
   printf(" cmd line options:\n");
   printf("      -wxxx   Width of the texture (Default=64)\n");
   printf("      -hxxx   Height of the texture (Default=64)\n");
   printf("      -dxxx   Depth of the texture (Default=64)\n");
   printf(" Keyboard Options:\n");
   printf("    up/down   rotate around X\n");
   printf("  left/right  rotate around Y\n");
   printf("      z/Z     rotate around Z\n");
   printf("       a      toggle animation\n");
   printf("       s      toggle smooth shading\n");
   printf("       t      toggle texgen mode\n");
   printf("       o      toggle object: torus/sphere\n");
   printf("       i      toggle texture image: noise/gradient\n");
}


static GLenum
parseCmdLine(int argc, char **argv)
{
   GLint i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-help") == 0) {
	 printHelp();
	 return GL_FALSE;
      }
      else if (strstr(argv[i], "-w") != NULL) {
	 tex_width = atoi((argv[i]) + 2);
      }
      else if (strstr(argv[i], "-h") != NULL) {
	 tex_height = atoi((argv[i]) + 2);
      }
      else if (strstr(argv[i], "-d") != NULL) {
	 tex_depth = atoi((argv[i]) + 2);
      }
      else {
	 printf("%s (Bad option).\n", argv[i]);
	 printHelp();
	 return GL_FALSE;
      }
   }
   if (tex_width == 0 || tex_height == 0 || tex_depth == 0) {
      printf("%s (Bad option).\n", "size parameters can't be 0");
      printHelp();
      return GL_FALSE;
   }
   return GL_TRUE;
}


static void
drawScene(void)
{
   static const GLfloat sPlane[4] = { 0.5, 0, 0, -.5 };
   static const GLfloat tPlane[4] = { 0, 0.5, 0, -.5 };
   static const GLfloat rPlane[4] = { 0, 0, 0.5, -.5 };

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glPushMatrix();
   if (texgen == 2) {
      glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
      glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
      glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
   }

   glRotatef(angx, 1.0, 0.0, 0.0);
   glRotatef(angy, 0.0, 1.0, 0.0);
   glRotatef(angz, 0.0, 0.0, 1.0);

   if (texgen == 1) {
      glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
      glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
      glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
   }

   if (texgen) {
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_R);
   }
   else {
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
      glDisable(GL_TEXTURE_GEN_R);
   }

   glCallList(CurObject);
   glPopMatrix();

   glutSwapBuffers();
}


static void
resize(int w, int h)
{
   float ar = (float) w / (float) h;
   float ax = 0.6 * ar;
   float ay = 0.6;
   glViewport(0, 0, (GLint) w, (GLint) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-ax, ax, -ay, ay, 2, 20);
   /*glOrtho(-2, 2, -2, 2, -10, 10);*/
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -4);
}


static void
Idle(void)
{
   float t = glutGet(GLUT_ELAPSED_TIME);
   angx = 0.01 * t;
   angy = 0.03 * t;
   angz += 0;
   glutPostRedisplay();
}


static void
SpecialKey(int k, int x, int y)
{
  switch (k) {
  case GLUT_KEY_UP:
    angx += 5.0;
    break;
  case GLUT_KEY_DOWN:
    angx -= 5.0;
    break;
  case GLUT_KEY_LEFT:
    angy += 5.0;
    break;
  case GLUT_KEY_RIGHT:
    angy -= 5.0;
    break;
  default:
    return;
  }
  glutPostRedisplay();
}


static void
KeyHandler(unsigned char key, int x, int y)
{
   static const char *mode[] = {
      "glTexCoord3f (no texgen)",
      "texgen fixed to object coords",
      "texgen fixed to eye coords"
   };
   (void) x;
   (void) y;
   switch (key) {
   case 27:
   case 'q':
   case 'Q':			/* quit game. */
      exit(0);
      break;
   case 'z':
      angz += 10;
      break;
   case 'Z':
      angz -= 10;
      break;
   case 's':
      smooth = !smooth;
      if (smooth)
         glShadeModel(GL_SMOOTH);
      else
         glShadeModel(GL_FLAT);
      break;
   case 't':
      texgen++;
      if (texgen > 2)
         texgen = 0;
      printf("Texgen: %s\n", mode[texgen]);
      break;
   case 'o':
      if (CurObject == TORUS)
         CurObject = SPHERE;
      else
         CurObject = TORUS;
      break;
   case 'i':
      if (CurTexture == NOISE_TEXTURE)
         CurTexture = GRADIENT_TEXTURE;
      else
         CurTexture = NOISE_TEXTURE;
      glBindTexture(GL_TEXTURE_3D, CurTexture);
      break;
   case 'a':
      animate = !animate;
      if (animate)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'w':
      wireframe = !wireframe;
      if (wireframe)
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   default:
      break;
   }
   glutPostRedisplay();
}


static void
create3Dgradient(void)
{
   unsigned char *v;
   int i, j, k;
   unsigned char *voxels = NULL;

   voxels = (unsigned char *) malloc(4 * tex_width * tex_height * tex_depth);
   v = voxels;

   for (i = 0; i < tex_depth; i++) {
      for (j = 0; j < tex_height; j++) {
	 for (k = 0; k < tex_width; k++) {
	    GLint r = (255 * i) / (tex_depth - 1);
	    GLint g = (255 * j) / (tex_height - 1);
	    GLint b = (255 * k) / (tex_width - 1);
	    *v++ = r;
	    *v++ = g;
	    *v++ = b;
	    *v++ = 255;
	 }
      }
   }


   glBindTexture(GL_TEXTURE_3D, GRADIENT_TEXTURE);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA,
		tex_width, tex_height, tex_depth,
		0, GL_RGBA, GL_UNSIGNED_BYTE, voxels);

   free(voxels);
}



static void
init(void)
{
   static const GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
   static const GLfloat mat_shininess[] = { 25.0 };
   static const GLfloat gray[] = { 0.6, 0.6, 0.6, 0.0 };
   static const GLfloat white[] = { 1.0, 1.0, 1.0, 0.0 };
   static const GLfloat light_position[] = { 0.0, 1.0, 1.0, 0.0 };

   int max;

   /* see if we have OpenGL 1.2 or later, for 3D texturing */
   {
      const char *version = (const char *) glGetString(GL_VERSION);
      if (strncmp(version, "1.0", 3) == 0 || strncmp(version, "1.1", 3) == 0) {
	 printf("Sorry, OpenGL 1.2 or later is required\n");
	 exit(1);
      }
   }
   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max);
   printf("GL_MAX_3D_TEXTURE_SIZE: %d\n", max);
   printf("Current 3D texture size: %d x %d x %d\n",
          tex_width, tex_height, tex_depth);

   /* init light */
   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   glLightfv(GL_LIGHT1, GL_POSITION, light_position);
   glLightfv(GL_LIGHT1, GL_AMBIENT, gray);
   glLightfv(GL_LIGHT1, GL_DIFFUSE, white);
   glLightfv(GL_LIGHT1, GL_SPECULAR, white);
   glColorMaterial(GL_FRONT, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT1);

   glClearColor(.5, .5, .5, 0);

   {
      GLUquadricObj *q;
      q = gluNewQuadric();
      gluQuadricTexture( q, GL_TRUE );
      glNewList(SPHERE, GL_COMPILE);
      gluSphere( q, 0.95, 30, 15 );
      glEndList();
      gluDeleteQuadric(q);
   }

   BuildTorus();


   create3Dgradient();

   initNoise();
   create3Dtexture();

   glEnable(GL_TEXTURE_3D);

   /*
      glBlendFunc(GL_SRC_COLOR, GL_SRC_ALPHA);
      glEnable(GL_BLEND); 
    */
   glEnable(GL_DEPTH_TEST);

   glColor3f(0.6, 0.7, 0.8);
}


int
main(int argc, char **argv)
{
   glutInit(&argc, argv);

   if (parseCmdLine(argc, argv) == GL_FALSE) {
      exit(0);
   }

   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

   if (glutCreateWindow("stex3d") <= 0) {
      exit(0);
   }

   init();

   printHelp();

   glutReshapeFunc(resize);
   glutKeyboardFunc(KeyHandler);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(drawScene);
   if (animate)
      glutIdleFunc(Idle);
   glutMainLoop();
   return 0;
}

