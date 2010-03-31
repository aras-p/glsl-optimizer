/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*- */
/*
  Copyright (c) 2010 Kristóf Ralovich

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "shaderutil.h"
#include <math.h>

static int Win;
static int WinWidth = 512, WinHeight = 512;
static int mouseGrabbed = 0;
static GLuint vertShader;
static GLuint fragShader;
static GLuint program;
static float rot[9] = {1,0,0,  0,1,0,   0,0,1};

static const char* vsSource =
  "varying vec2 rayDir;                                                \n"
  "                                                                    \n"
  "void main()                                                         \n"
  "{                                                                   \n"
  "  rayDir = gl_MultiTexCoord0.xy - vec2(0.5,0.5);                    \n"
  "  gl_Position = gl_ProjectionMatrix * gl_Vertex;                    \n"
  "}\n";

static const char* fsSource =
  "const float INF     = 9999.9;                                       \n"
  "const float EPSILON = 0.00001;                                      \n"
  "const vec3 lightPos = vec3(0.0, 8.0, 1.0);                          \n"
  "const vec4 backgroundColor = vec4(0.2,0.3,0.4,1);                   \n"
  "                                                                    \n"
  "varying vec2 rayDir;                                                \n"
  "                                                                    \n"
  "uniform mat3 rot;                                                   \n"
  "                                                                    \n"
  "struct Ray                                                          \n"
  "{                                                                   \n"
  "vec3 orig;                                                          \n"
  "vec3 dir;                                                           \n"
  "};                                                                  \n"
  "                                                                    \n"
  "struct Sphere                                                       \n"
  "{                                                                   \n"
  "  vec3 c;                                                           \n"
  "  float r;                                                          \n"
  "};                                                                  \n"
  "                                                                    \n"
  "struct Isec                                                         \n"
  "{                                                                   \n"
  "  float t;                                                          \n"
  "  int idx;                                                          \n"
  "  vec3 hit;                                                         \n"
  "  vec3 n;                                                           \n"
  "};                                                                  \n"
  "                                                                    \n"
#ifdef __APPLE__
  "Sphere spheres0 = Sphere( vec3(0.0,0.0,-1.0), 0.5 );                \n"
  "Sphere spheres1 = Sphere( vec3(-3.0,0.0,-1.0), 1.5 );               \n"
  "Sphere spheres2 = Sphere( vec3(0.0,3.0,-1.0), 0.5 );                \n"
  "Sphere spheres3 = Sphere( vec3(2.0,0.0,-1.0), 1.0 );                \n"
#else
  "const Sphere spheres0 = Sphere( vec3(0.0,0.0,-1.0), 0.5 );          \n"
  "const Sphere spheres1 = Sphere( vec3(-3.0,0.0,-1.0), 1.5 );         \n"
  "const Sphere spheres2 = Sphere( vec3(0.0,3.0,-1.0), 0.5 );          \n"
  "const Sphere spheres3 = Sphere( vec3(2.0,0.0,-1.0), 1.0 );          \n"
#endif
  "                                                                    \n"
  "// Mesa intel gen4 generates \"unsupported IR in fragment shader 13\" for\n"
  "// sqrt, let's work around.                                         \n"
  "float                                                               \n"
  "sqrt_hack(float f2)                                                 \n"
  "{                                                                   \n"
  "  vec3 v = vec3(f2,0.0,0.0);                                        \n"
  "  return length(v);                                                 \n"
  "}                                                                   \n"
  "                                                                    \n"
  "void                                                                \n"
  "intersect(const in Ray ray,                                         \n"
  "          const in Sphere sph,                                      \n"
  "          const in int idx,                                         \n"
  "          inout Isec isec)                                          \n"
  "{                                                                   \n"
  "  // Project both o and the sphere to the plane perpendicular to d  \n"
  "  // and containing c. Let x be the point where the ray intersects  \n"
  "  // the plane. If |x-c| < r, the ray intersects the sphere.        \n"
  "  vec3 o = ray.orig;                                                \n"
  "  vec3 d = ray.dir;                                                 \n"
  "  vec3 n = -d;                                                      \n"
  "  vec3 c = sph.c;                                                   \n"
  "  float r = sph.r;                                                  \n"
  "  float t = dot(c-o,n)/dot(n,d);                                    \n"
  "  vec3 x = o+d*t;                                                   \n"
  "  float e = length(x-c);                                            \n"
  "  if(e > r)                                                         \n"
  "  {                                                                 \n"
  "    // no intersection                                              \n"
  "    return;                                                         \n"
  "  }                                                                 \n"
  "                                                                    \n"
  "  // Apply Pythagorean theorem on the (intersection,x,c) triangle   \n"
  "  // to get the distance between c and the intersection.            \n"
  "#ifndef BUGGY_INTEL_GEN4_GLSL                                       \n"
  "  float f = sqrt(r*r - e*e);                                        \n"
  "#else                                                               \n"
  "  float f = sqrt_hack(r*r - e*e);                                   \n"
  "#endif                                                              \n"
  "  float dist = t - f;                                               \n"
  "  if(dist < 0.0)                                                    \n"
  "  {                                                                 \n"
  "    // inside the sphere                                            \n"
  "    return;                                                         \n"
  "  }                                                                 \n"
  "                                                                    \n"
  "  if(dist < EPSILON)                                                \n"
  "    return;                                                         \n"
  "                                                                    \n"
  "  if(dist > isec.t)                                                 \n"
  "    return;                                                         \n"
  "                                                                    \n"
  "  isec.t = dist;                                                    \n"
  "  isec.idx = idx;                                                   \n"
  "                                                                    \n"
  "  isec.hit  = ray.orig + ray.dir * isec.t;                          \n"
  "  isec.n = (isec.hit - c) / r;                                      \n"
  "}                                                                   \n"
  "                                                                    \n"
  "Isec                                                                \n"
  "intersect(const in Ray ray,                                         \n"
  "          const in float max_t /*= INF*/)                           \n"
  "{                                                                   \n"
  "  Isec nearest;                                                     \n"
  "  nearest.t = max_t;                                                \n"
  "  nearest.idx = -1;                                                 \n"
  "                                                                    \n"
  "  intersect(ray, spheres0, 0, nearest);                             \n"
  "  intersect(ray, spheres1, 1, nearest);                             \n"
  "  intersect(ray, spheres2, 2, nearest);                             \n"
  "  intersect(ray, spheres3, 3, nearest);                             \n"
  "                                                                    \n"
  "  return nearest;                                                   \n"
  "}                                                                   \n"
  "                                                                    \n"
  "vec4                                                                \n"
  "idx2color(const in int idx)                                         \n"
  "{                                                                   \n"
  "  vec4 diff;                                                        \n"
  "  if(idx == 0)                                                      \n"
  "    diff = vec4(1.0, 0.0, 0.0, 0.0);                                \n"
  "  else if(idx == 1)                                                 \n"
  "    diff = vec4(0.0, 1.0, 0.0, 0.0);                                \n"
  "  else if(idx == 2)                                                 \n"
  "    diff = vec4(0.0, 0.0, 1.0, 0.0);                                \n"
  "  else if(idx == 3)                                                 \n"
  "    diff = vec4(1.0, 1.0, 0.0, 0.0);                                \n"
  "  return diff;                                                      \n"
  "}                                                                   \n"
  "                                                                    \n"
  "vec4                                                                \n"
  "trace0(const in Ray ray)                                            \n"
  "{                                                                   \n"
  "  Isec isec = intersect(ray, INF);                                  \n"
  "                                                                    \n"
  "  if(isec.idx == -1)                                                \n"
  "  {                                                                 \n"
  "    return backgroundColor;                                         \n"
  "  }                                                                 \n"
  "                                                                    \n"
  "  vec4 diff = idx2color(isec.idx);                                  \n"
  "                                                                    \n"
  "  vec3 N = isec.n;                                                  \n"
  "  vec3 L = normalize(lightPos-isec.hit);                            \n"
  "  vec3 camera_dir = normalize(ray.orig - isec.hit);                 \n"
  "  return dot(N,L)*diff + pow(                                       \n"
  "    clamp(dot(reflect(-L,N),camera_dir),0.0,1.0),16.0);             \n"
  "}                                                                   \n"
  "                                                                    \n"
  "vec4                                                                \n"
  "trace1(const in Ray ray)                                            \n"
  "{                                                                   \n"
  "  Isec isec = intersect(ray, INF);                                  \n"
  "                                                                    \n"
  "  if(isec.idx == -1)                                                \n"
  "  {                                                                 \n"
  "    return backgroundColor;                                         \n"
  "  }                                                                 \n"
  "                                                                    \n"
  "  Ray reflRay = Ray(isec.hit, reflect(ray.dir, isec.n));            \n"
  "                                                                    \n"
  "  vec4 reflCol = trace0(reflRay);                                   \n"
  "                                                                    \n"
  "  vec4 diff = idx2color(isec.idx) + reflCol;                        \n"
  "                                                                    \n"
  "  vec3 N = isec.n;                                                  \n"
  "  vec3 L = normalize(lightPos-isec.hit);                            \n"
  "  vec3 camera_dir = normalize(ray.orig - isec.hit);                 \n"
  "  return dot(N,L)*diff + pow(                                       \n"
  "    clamp(dot(reflect(-L,N),camera_dir),0.0,1.0),16.0);             \n"
  "}                                                                   \n"
  "                                                                    \n"
  "void main()                                                         \n"
  "{                                                                   \n"
  "  const float z = -0.5;                                             \n"
  "  const vec3 cameraPos = vec3(0,0,3);                               \n"
  "  Ray r = Ray(cameraPos, normalize(vec3(rayDir, z) * rot));         \n"
  "  gl_FragColor = trace1(r);                                         \n"
  "}\n";

static
float
deg2rad(const float degree)
{
  return( degree * 0.017453292519943295769236907684886F);
}

static void
rotate_xy(float* mat3, const float degreesAroundX, const float degreesAroundY)
{
  const float rad1 = deg2rad(degreesAroundX);
  const float c1 = cosf(rad1);
  const float s1 = sinf(rad1);
  const float rad2 = deg2rad(degreesAroundY);
  const float c2 = cosf(rad2);
  const float s2 = sinf(rad2);
  mat3[0] = c2;    mat3[3] = 0.0F; mat3[6] = s2;
  mat3[1] = s1*s2; mat3[4] = c1;   mat3[7] = -s1*c2;
  mat3[2] = -c1*s2;mat3[5] = s1;   mat3[8] = c1*c2;
}

static void
identity(float* mat3)
{
  mat3[0] = 1.0F; mat3[3] = 0.0F; mat3[6] = 0.0F;
  mat3[1] = 0.0F; mat3[4] = 1.0F; mat3[7] = 0.0F;
  mat3[2] = 0.0F; mat3[5] = 0.0F; mat3[8] = 1.0F;
}

static void
Draw(void)
{
  GLint location = glGetUniformLocation(program, "rot");
  static const float m = -10.F;
  static const float p =  10.F;
  static const float d = -0.5F;

  glUseProgram(program);
  glUniformMatrix3fv(location, 1, 0, rot);

  glBegin(GL_QUADS);
  {
    glTexCoord2f(0.0F, 0.0F); glVertex3f(m, m, d);
    glTexCoord2f(1.0F, 0.0F); glVertex3f(p, m, d);
    glTexCoord2f(1.0F, 1.0F); glVertex3f(p, p, d);
    glTexCoord2f(0.0F, 1.0F); glVertex3f(m, p, d);
  }
  glEnd();
  glUseProgram(0);

  glutSwapBuffers();

  {
    static int frames = 0;
    static int t0 = 0;
    static int t1 = 0;
    float dt;
    frames++;
    t1 = glutGet(GLUT_ELAPSED_TIME);
    dt = (float)(t1-t0)/1000.0F;
    if(dt >= 5.0F)
    {
      float fps = (float)frames / dt;
      printf("%f FPS (%d frames in %f seconds)\n", fps, frames, dt);
      frames = 0;
      t0 = t1;
    }
  }
}


static void
Reshape(int width, int height)
{
  WinWidth = width;
  WinHeight = height;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-10, 10, -10, 10, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;
  switch (key) {
  case 27:
    glutDestroyWindow(Win);
    exit(0);
    break;
  }
  glutPostRedisplay();
}


static
void
drag(int x, int y)
{
  float scale = 1.5F;
  if(mouseGrabbed)
  {
    static GLfloat xRot = 0, yRot = 0;
    xRot = (float)(x - WinWidth/2) / scale;
    yRot = (float)(y - WinHeight/2) / scale;
    identity(rot);
    rotate_xy(rot, yRot, xRot);
    glutPostRedisplay();
  }
}


static
void
mouse(int button, int state, int x, int y)
{
  mouseGrabbed = (state == GLUT_DOWN);
}


static void
Init(void)
{
  glDisable(GL_DEPTH_TEST);

  if(!ShadersSupported())
  {
    fprintf(stderr, "Shaders are not supported!\n");
    exit(-1);
  }

  vertShader = CompileShaderText(GL_VERTEX_SHADER, vsSource);
  fragShader = CompileShaderText(GL_FRAGMENT_SHADER, fsSource);
  program = LinkShaders(vertShader, fragShader);
  glUseProgram(0);

  if(glGetError() != 0)
  {
    fprintf(stderr, "Shaders were not loaded!\n");
    exit(-1);
  }

  if(!glIsShader(vertShader))
  {
    fprintf(stderr, "Vertex shader failed!\n");
    exit(-1);
  }

  if(!glIsProgram(program))
  {
    fprintf(stderr, "Shader program failed!\n");
    exit(-1);
  }

  printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));
}


int
main(int argc, char *argv[])
{
  glutInitWindowSize(WinWidth, WinHeight);
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  Win = glutCreateWindow(argv[0]);
  glewInit();
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Key);
  glutDisplayFunc(Draw);
  glutMouseFunc(mouse);
  glutMotionFunc(drag);
  glutIdleFunc(Draw);
  Init();
  glutMainLoop();
  return 0;
}

