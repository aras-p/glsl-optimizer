/* $Id: stex3d.c,v 1.4 2000/03/22 19:48:57 brianp Exp $ */

/*----------------------------- 
 * stex3d.c GL example of the mesa 3d-texture extention to simulate procedural
 *            texturing, it uses a perlin noise and turbulence functions.
 * 
 * Author:   Daniel Barrero
 *           barrero@irit.fr
 *           dbarrero@pegasus.uniandes.edu.co
 *
 * Converted to GLUT by brianp on 1/1/98
 *
 *      
 * cc stex3d.c -o stex3d -lglut -lMesaGLU -lMesaGL -lX11 -lXext -lm 
 *
 *---------------------------- */

/*
 * $Log: stex3d.c,v $
 * Revision 1.4  2000/03/22 19:48:57  brianp
 * converted from GL_EXT_texture3D to GL 1.2
 *
 * Revision 1.3  1999/12/16 08:54:22  brianp
 * added a cast to malloc call
 *
 * Revision 1.2  1999/09/17 12:27:01  brianp
 * silenced some warnings
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.1  1998/06/09 01:53:49  brianp
 * main() should return an int
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>
/* function declarations */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
 
void init(void),
     printHelp(void),
     create3Dtexture(void),
     setDefaults(void),
     drawScene(void),
     resize(int w, int h),
     buildFigure(void),
     initNoise(void);
float turbulence(float point[3], float lofreq, float hifreq);

void KeyHandler( unsigned char key, int x, int y );
GLenum parseCmdLine(int argc, char **argv);
float noise3(float vec[3]);
      
/* global variables */
GLenum rgb, doubleBuffer, directRender, windType; /* visualization state*/
float tex_width,tex_height,tex_depth;        /* texture volume dimensions  */
unsigned char *voxels;                       /* texture data ptr */
int angx,angy,angz;
GLuint figure;

/*function definitions */
int main(int argc, char **argv)
{

 if (parseCmdLine(argc, argv) == GL_FALSE) {
    exit(0);
 }

 glutInitWindowPosition(0, 0);
 glutInitWindowSize(400, 400);
 windType = (rgb) ? GLUT_RGB : GLUT_INDEX;
 windType |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
 windType |= GLUT_DEPTH;
 glutInitDisplayMode(windType);

 if (glutCreateWindow("stex3d") <= 0) {
    exit(0);
 }
 /* init all */
 init();

 glutReshapeFunc(resize);
 glutKeyboardFunc(KeyHandler);
 glutDisplayFunc(drawScene);
 glutMainLoop();
 return 0;
}

void init()
{ 
 /* init light */
 GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
 GLfloat mat_shininess[] = { 25.0 };
 GLfloat gray[] = { 0.6, 0.6, 0.6, 0.0 };
 GLfloat white[] = { 1.0, 1.0, 1.0, 0.0 };
 GLfloat light_position[] = { 0.0, 1.0, 1.0, 0.0 };

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

 /* create torus for texturing */
 figure=glGenLists(1); 
 buildFigure();
/* tkSolidTorus(figure,0.3,1.2);*/

 /* start the noise function variables */
 initNoise();
 
 /* see if we have OpenGL 1.2 or later, for 3D texturing */
 {
    const char *version = (const char *) glGetString(GL_VERSION);
    if (strncmp(version, "1.0", 3) == 0 ||
        strncmp(version, "1.1", 3) == 0) {
       printf("Sorry, OpenGL 1.2 or later is required\n");
       exit(1);
    }
 }
 
 /* if texture is supported then generate the texture */
 create3Dtexture(); 

 glEnable(GL_TEXTURE_3D); 
 /*
 glBlendFunc(GL_SRC_COLOR, GL_SRC_ALPHA);
 glEnable(GL_BLEND); 
 */
 glEnable(GL_DEPTH_TEST);

 glShadeModel(GL_FLAT);
 glColor3f(0.6,0.7,0.8);
}

void buildFigure(void)
{   GLint i, j;
    float theta1, phi1, theta2, phi2, rings, sides;
    float v0[03], v1[3], v2[3], v3[3];
    float t0[03], t1[3], t2[3], t3[3];
    float n0[3], n1[3], n2[3], n3[3];
    float innerRadius=0.4;
    float outerRadius=0.8;
    float scalFac;

    rings = 8;
    sides = 10;
    scalFac=1/(outerRadius*2);

    glNewList(figure, GL_COMPILE);
    for (i = 0; i < rings; i++) {
        theta1 = (float)i * 2.0 * M_PI / rings;
        theta2 = (float)(i + 1) * 2.0 * M_PI / rings;
        for (j = 0; j < sides; j++) {
            phi1 = (float)j * 2.0 * M_PI / sides;
            phi2 = (float)(j + 1) * 2.0 * M_PI / sides;

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

            t0[0] = v0[0]*scalFac + 0.5;
            t0[1] = v0[1]*scalFac + 0.5;
            t0[2] = v0[2]*scalFac + 0.5;

            t1[0] = v1[0]*scalFac + 0.5;
            t1[1] = v1[1]*scalFac + 0.5;
            t1[2] = v1[2]*scalFac + 0.5;

            t2[0] = v2[0]*scalFac + 0.5;
            t2[1] = v2[1]*scalFac + 0.5;
            t2[2] = v2[2]*scalFac + 0.5;

            t3[0] = v3[0]*scalFac + 0.5;
            t3[1] = v3[1]*scalFac + 0.5;
            t3[2] = v3[2]*scalFac + 0.5;

            glBegin(GL_POLYGON);
                glNormal3fv(n3); glTexCoord3fv(t3); glVertex3fv(v3);
                glNormal3fv(n2); glTexCoord3fv(t2); glVertex3fv(v2);
                glNormal3fv(n1); glTexCoord3fv(t1); glVertex3fv(v1);
                glNormal3fv(n0); glTexCoord3fv(t0); glVertex3fv(v0);
            glEnd();
        }
    }
    glEndList();
}

void create3Dtexture()
{
 int i,j,k;
 unsigned char *vp;
 float vec[3];
 int tmp;

 printf("creating 3d textures...\n");
 voxels = (unsigned char  *) malloc((size_t)(4*tex_width*tex_height*tex_depth));
 vp=voxels;
 for (i=0;i<tex_width;i++){
    vec[0]=i;
    for (j=0;j<tex_height;j++) {
      vec[1]=j;
       for (k=0;k<tex_depth;k++) {
           vec[2]=k;
           tmp=(sin(k*i*j+turbulence(vec,0.01,1))+1)*127.5; 
           *vp++=0;
           *vp++=0;
           *vp++=tmp; 
           *vp++=tmp+128; 
       }
     }
 }

 printf("setting up 3d texture...\n");
 glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
 glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
 glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
 glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_REPEAT);
 glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

 glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA,
              tex_width, tex_height, tex_depth,
              0, GL_RGBA, GL_UNSIGNED_BYTE, voxels);
 
 printf("finished setting up 3d texture image...\n");
}

void printHelp()
{
  printf("\nUsage: stex3d  <cmd line options>\n"); 
  printf(" cmd line options:\n");
  printf("      -help   print this help!\n");
  printf("      -rgb     RGBA mode. (Default)\n");
  printf("      -ci     Color index mode.\n");
  printf("      -sb     Single buffer mode. (Default)\n");
  printf("      -db     Double buffer mode. \n");
  printf("      -dr     Direct render mode.\n");
  printf("      -ir     Indirect render mode. (Default)\n");
  printf("      -wxxx   Width of the texture (Default=64)\n");
  printf("      -hxxx   Height of the texture (Default=64)\n");
  printf("      -dxxx   Depth of the texture (Default=64)\n");
  printf(" Keyboard Options:\n");
  printf("       1      Object Texture coordinates (Default)\n");
  printf("       2      Eye Texture coordinates \n");
  printf("       x      rotate around x clockwise\n");
  printf("       X      rotate around x counter clockwise\n");
  printf("       y      rotate around y clockwise\n");
  printf("       Y      rotate around y counter clockwise\n");
  printf("       z      rotate around z clockwise\n");
  printf("       Z      rotate around z counter clockwise\n");
  printf("       t      enable 3-D texuring (Default)\n");
  printf("       T      disable 3-D texuring\n");
  printf("       s      smooth shading \n");
  printf("       S      flat shading (Default)\n");
}

void setDefaults()
{
 /* visualization defaults */
  rgb = GL_TRUE;
  doubleBuffer = GL_FALSE;
  directRender = GL_TRUE;
  angx=130;
  angy=30;
  angz=0; 
 /* texture values */
 tex_width=64;
 tex_height=64;
 tex_depth=64;
}

GLenum parseCmdLine(int argc, char **argv)
{
  GLint i;
 
  setDefaults();

  for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-ci") == 0) {
          rgb = GL_FALSE;
      } else if (strcmp(argv[i], "-rgb") == 0) {
          rgb = GL_TRUE;
      } else if (strcmp(argv[i], "-sb") == 0) {
          doubleBuffer = GL_FALSE;
      } else if (strcmp(argv[i], "-db") == 0) {
          doubleBuffer = GL_TRUE;
      } else if (strcmp(argv[i], "-dr") == 0) {
          directRender = GL_TRUE;
      } else if (strcmp(argv[i], "-ir") == 0) {
          directRender = GL_FALSE;
      } else if (strstr(argv[i], "-w") == 0) {
          tex_width=atoi((argv[i])+2);
      } else if (strstr(argv[i], "-h") == 0) {
          tex_height=atoi((argv[i])+2);
      } else if (strstr(argv[i], "-d") == 0) {
          tex_depth=atoi((argv[i])+2);
      } else if (strcmp(argv[i], "-help") == 0) {
          printHelp();
          return GL_FALSE;
      } else {
          printf("%s (Bad option).\n", argv[i]);
          printHelp();
          return GL_FALSE;
      }
  }
 if(tex_width==0 || tex_height==0 || tex_depth==0) {
   printf("%s (Bad option).\n", "size parameters can't be 0");
   printHelp();
   return GL_FALSE;
 }
  return GL_TRUE;
}

void drawScene()
{
 /* clear background, z buffer etc */
 glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 glPushMatrix();
 glRotatef(angx,1.0,0.0,0.0);
 glRotatef(angy,0.0,1.0,0.0);
 glRotatef(angz,0.0,0.0,1.0);

 glCallList(figure);     
 glPopMatrix();
 glFlush();
 if(doubleBuffer)
    glutSwapBuffers();
 ;
}

void resize(int w, int h)
{ 
 glViewport(0, 0, (GLint)w, (GLint)h);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glOrtho(-2,2,-2,2,-5,10);
 glMatrixMode(GL_MODELVIEW);
 glLoadIdentity();
 glTranslatef(0,0,-5);
}

void cleanEverything(void)
{
/*  free(voxels); */
}


void KeyHandler( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch(key) {
      case 27:
      case 'q':
      case 'Q': /* quit game. */
         cleanEverything();
         exit(0);
         break;
      case 'x':
         angx+=10;
         break;
      case 'X': 
         angx-=10; 
         break;
      case 'y':
         angy+=10;
         break;
      case 'Y': 
         angy-=10; 
         break;
      case 'z':
         angz+=10;
         break;
      case 'Z': 
         angz-=10; 
         break;
      case 't':
         glEnable(GL_TEXTURE_3D); 
         break;
      case 'T':
         glDisable(GL_TEXTURE_3D);
         break;
      case 's':
         glShadeModel(GL_SMOOTH);
         break;
      case 'S':
         glShadeModel(GL_FLAT);
         break;
      case '1':
         glDisable(GL_TEXTURE_GEN_S);
         glDisable(GL_TEXTURE_GEN_T);
         glDisable(GL_TEXTURE_GEN_R);
         break;
      case '2':
         glEnable(GL_TEXTURE_GEN_S);
         glEnable(GL_TEXTURE_GEN_T);
         glEnable(GL_TEXTURE_GEN_R);
         break;
      default:
         break;
   }
   glutPostRedisplay();
}

/*--------------------------------------------------------------------
 noise function over R3 - implemented by a pseudorandom tricubic spline 
              EXCERPTED FROM SIGGRAPH 92, COURSE 23
                        PROCEDURAL MODELING
                             Ken Perlin
                           New York University
----------------------------------------------------------------------*/


#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
#define B 256
static int p[B + B + 2];
static float g[B + B + 2][3];
#define setup(i,b0,b1,r0,r1) \
        t = vec[i] + 10000.; \
        b0 = ((int)t) & (B-1); \
        b1 = (b0+1) & (B-1); \
        r0 = t - (int)t; \
        r1 = r0 - 1.;

float noise3(float vec[3])
{
        int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
        float rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v;
        register int i, j;

        setup(0, bx0,bx1, rx0,rx1);
        setup(1, by0,by1, ry0,ry1);
        setup(2, bz0,bz1, rz0,rz1);

        i = p[ bx0 ];
        j = p[ bx1 ];

        b00 = p[ i + by0 ];
        b10 = p[ j + by0 ];
        b01 = p[ i + by1 ];
        b11 = p[ j + by1 ];

#define at(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )
#define surve(t) ( t * t * (3. - 2. * t) )
#define lerp(t, a, b) ( a + t * (b - a) )

        sx = surve(rx0);
        sy = surve(ry0);
        sz = surve(rz0);

        q = g[ b00 + bz0 ] ; u = at(rx0,ry0,rz0);
        q = g[ b10 + bz0 ] ; v = at(rx1,ry0,rz0);
        a = lerp(sx, u, v);

        q = g[ b01 + bz0 ] ; u = at(rx0,ry1,rz0);
        q = g[ b11 + bz0 ] ; v = at(rx1,ry1,rz0);
        b = lerp(sx, u, v);

        c = lerp(sy, a, b);          /* interpolate in y at lo x */

        q = g[ b00 + bz1 ] ; u = at(rx0,ry0,rz1);
        q = g[ b10 + bz1 ] ; v = at(rx1,ry0,rz1);
        a = lerp(sx, u, v);

        q = g[ b01 + bz1 ] ; u = at(rx0,ry1,rz1);
        q = g[ b11 + bz1 ] ; v = at(rx1,ry1,rz1);
        b = lerp(sx, u, v);

        d = lerp(sy, a, b);          /* interpolate in y at hi x */

        return 1.5 * lerp(sz, c, d); /* interpolate in z */
}

void initNoise()
{
        /*long random();*/
        int i, j, k;
        float v[3], s;

/* Create an array of random gradient vectors uniformly on the unit sphere */
        /*srandom(1);*/
        srand(1);
        for (i = 0 ; i < B ; i++) {
                do {                            /* Choose uniformly in a cube */                        for (j=0 ; j<3 ; j++)
                                v[j] = (float)((rand() % (B + B)) - B) / B;
                        s = DOT(v,v);
                } while (s > 1.0);              /* If not in sphere try again */                s = sqrt(s);
                for (j = 0 ; j < 3 ; j++)       /* Else normalize */
                        g[i][j] = v[j] / s;
        }

/* Create a pseudorandom permutation of [1..B] */
        for (i = 0 ; i < B ; i++)
                p[i] = i;
        for (i = B ; i > 0 ; i -= 2) {
                k = p[i];
                p[i] = p[j = rand() % B];
                p[j] = k;
        }

/* Extend g and p arrays to allow for faster indexing */
        for (i = 0 ; i < B + 2 ; i++) {
                p[B + i] = p[i];
                for (j = 0 ; j < 3 ; j++)
                        g[B + i][j] = g[i][j];
        }
}

float turbulence(float point[3], float lofreq, float hifreq)
{
        float freq, t, p[3];

        p[0] = point[0] + 123.456;
        p[1] = point[1];
        p[2] = point[2];

        t = 0;
        for (freq = lofreq ; freq < hifreq ; freq *= 2.) {
                t += fabs(noise3(p)) / freq;
                p[0] *= 2.;
                p[1] *= 2.;
                p[2] *= 2.;
        }
        return t - 0.3; /* readjust to make mean value = 0.0 */
}

