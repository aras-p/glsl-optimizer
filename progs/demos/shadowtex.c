/*
 * Shadow demo using the GL_ARB_depth_texture, GL_ARB_shadow and
 * GL_ARB_shadow_ambient extensions.
 *
 * Brian Paul
 * 19 Feb 2001
 *
 * Added GL_EXT_shadow_funcs support on 23 March 2002
 * Added GL_EXT_packed_depth_stencil support on 15 March 2006.
 * Added GL_EXT_framebuffer_object support on 27 March 2006.
 * Removed old SGIX extension support on 5 April 2006.
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include "showbuffer.h"

#define DEG_TO_RAD (3.14159 / 180.0)

static GLint WindowWidth = 450, WindowHeight = 300;
static GLfloat Xrot = 15, Yrot = 0, Zrot = 0;

static GLfloat Red[4] = {1, 0, 0, 1};
static GLfloat Green[4] = {0, 1, 0, 1};
static GLfloat Blue[4] = {0, 0, 1, 1};
static GLfloat Yellow[4] = {1, 1, 0, 1};

static GLfloat LightDist = 10;
static GLfloat LightLatitude = 45.0;
static GLfloat LightLongitude = 45.0;
static GLfloat LightPos[4];
static GLfloat SpotDir[3];
static GLfloat SpotAngle = 40.0 * DEG_TO_RAD;
static GLfloat ShadowNear = 4.0, ShadowFar = 24.0;
static GLint ShadowTexWidth = 256, ShadowTexHeight = 256;

static GLboolean LinearFilter = GL_FALSE;

static GLfloat Bias = -0.06;

static GLboolean Anim = GL_TRUE;

static GLboolean NeedNewShadowMap = GL_FALSE;
static GLuint ShadowTexture, GrayTexture;
static GLuint ShadowFBO;

static GLboolean HaveFBO = GL_FALSE;
static GLboolean UseFBO = GL_FALSE;
static GLboolean HavePackedDepthStencil = GL_FALSE;
static GLboolean UsePackedDepthStencil = GL_FALSE;
static GLboolean HaveEXTshadowFuncs = GL_FALSE;
static GLboolean HaveShadowAmbient = GL_FALSE;

static GLint Operator = 0;
static const GLenum OperatorFunc[8] = {
   GL_LEQUAL, GL_LESS, GL_GEQUAL, GL_GREATER,
   GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS, GL_NEVER };
static const char *OperatorName[8] = {
   "GL_LEQUAL", "GL_LESS", "GL_GEQUAL", "GL_GREATER",
   "GL_EQUAL", "GL_NOTEQUAL", "GL_ALWAYS", "GL_NEVER" };


static GLuint DisplayMode;
#define SHOW_SHADOWS        0
#define SHOW_DEPTH_IMAGE    1
#define SHOW_DEPTH_MAPPING  2
#define SHOW_DISTANCE       3



static void
DrawScene(void)
{
   GLfloat k = 6;

   /* sphere */
   glPushMatrix();
   glTranslatef(1.6, 2.2, 2.7);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Green);
   glColor4fv(Green);
   glutSolidSphere(1.5, 15, 15);
   glPopMatrix();
   /* dodecahedron */
   glPushMatrix();
   glTranslatef(-2.0, 1.2, 2.1);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Red);
   glColor4fv(Red);
   glutSolidDodecahedron();
   glPopMatrix();
   /* icosahedron */
   glPushMatrix();
   glTranslatef(-0.6, 1.3, -0.5);
   glScalef(1.5, 1.5, 1.5);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Yellow);
   glColor4fv(Red);
   glutSolidIcosahedron();
   glPopMatrix();
   /* a plane */
   glPushMatrix();
   glTranslatef(0, -1.1, 0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Blue);
   glColor4fv(Blue);
   glNormal3f(0, 1, 0);
   glBegin(GL_POLYGON);
   glVertex3f(-k, 0, -k);
   glVertex3f( k, 0, -k);
   glVertex3f( k, 0,  k);
   glVertex3f(-k, 0,  k);
   glEnd();
   glPopMatrix();
}


/*
 * Load the GL_TEXTURE matrix with the projection from the light
 * source's point of view.
 */
static void
MakeShadowMatrix(const GLfloat lightPos[4], const GLfloat spotDir[3],
                 GLfloat spotAngle, GLfloat shadowNear, GLfloat shadowFar)
{
   GLfloat d;
   
   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glTranslatef(0.5, 0.5, 0.5 + Bias);
   glScalef(0.5, 0.5, 0.5);
   d = shadowNear * tan(spotAngle);
   glFrustum(-d, d, -d, d, shadowNear, shadowFar);
   gluLookAt(lightPos[0], lightPos[1], lightPos[2],
             lightPos[0] + spotDir[0],
             lightPos[1] + spotDir[1],
             lightPos[2] + spotDir[2],
             0, 1, 0);
   glMatrixMode(GL_MODELVIEW);
}


static void
EnableIdentityTexgen(void)
{
   /* texgen so that texcoord = vertex coord */
   static GLfloat sPlane[4] = { 1, 0, 0, 0 };
   static GLfloat tPlane[4] = { 0, 1, 0, 0 };
   static GLfloat rPlane[4] = { 0, 0, 1, 0 };
   static GLfloat qPlane[4] = { 0, 0, 0, 1 };

   glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
   glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
   glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
   glTexGenfv(GL_Q, GL_EYE_PLANE, qPlane);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

   glEnable(GL_TEXTURE_GEN_S);
   glEnable(GL_TEXTURE_GEN_T);
   glEnable(GL_TEXTURE_GEN_R);
   glEnable(GL_TEXTURE_GEN_Q);
}


/*
 * Setup 1-D texgen so that the distance from the light source, between
 * the near and far planes maps to s=0 and s=1.  When we draw the scene,
 * the grayness will indicate the fragment's distance from the light
 * source.
 */
static void
EnableDistanceTexgen(const GLfloat lightPos[4], const GLfloat lightDir[3],
                     GLfloat lightNear, GLfloat lightFar)
{
   GLfloat m, d;
   GLfloat sPlane[4];
   GLfloat nearPoint[3];

   m = sqrt(lightDir[0] * lightDir[0] +
            lightDir[1] * lightDir[1] +
            lightDir[2] * lightDir[2]);

   d = lightFar - lightNear;

   /* nearPoint = point on light direction vector which intersects the
    * near plane of the light frustum.
    */
   nearPoint[0] = lightPos[0] + lightDir[0] / m * lightNear;
   nearPoint[1] = lightPos[1] + lightDir[1] / m * lightNear;
   nearPoint[2] = lightPos[2] + lightDir[2] / m * lightNear;

   sPlane[0] = lightDir[0] / d / m;
   sPlane[1] = lightDir[1] / d / m;
   sPlane[2] = lightDir[2] / d / m;
   sPlane[3] = -(sPlane[0] * nearPoint[0]
               + sPlane[1] * nearPoint[1]
               + sPlane[2] * nearPoint[2]);

   glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glEnable(GL_TEXTURE_GEN_S);
}


static void
DisableTexgen(void)
{
   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glDisable(GL_TEXTURE_GEN_R);
   glDisable(GL_TEXTURE_GEN_Q);
}


static void
ComputeLightPos(GLfloat dist, GLfloat latitude, GLfloat longitude,
                GLfloat pos[4], GLfloat dir[3])
{

   pos[0] = dist * sin(longitude * DEG_TO_RAD);
   pos[1] = dist * sin(latitude * DEG_TO_RAD);
   pos[2] = dist * cos(latitude * DEG_TO_RAD) * cos(longitude * DEG_TO_RAD);
   pos[3] = 1;
   dir[0] = -pos[0];
   dir[1] = -pos[1];
   dir[2] = -pos[2];
}


/**
 * Render the shadow map / depth texture.
 * The result will be in the texture object named ShadowTexture.
 */
static void
RenderShadowMap(void)
{
   GLenum depthFormat; /* GL_DEPTH_COMPONENT or GL_DEPTH_STENCIL_EXT */
   GLenum depthType; /* GL_UNSIGNED_INT_24_8_EXT or GL_UNSIGNED_INT */
   float d;

   if (WindowWidth >= 1024 && WindowHeight >= 1024) {
      ShadowTexWidth = ShadowTexHeight = 1024;
   }
   else if (WindowWidth >= 512 && WindowHeight >= 512) {
      ShadowTexWidth = ShadowTexHeight = 512;
   }
   else if (WindowWidth >= 256 && WindowHeight >= 256) {
      ShadowTexWidth = ShadowTexHeight = 256;
   }
   else {
      ShadowTexWidth = ShadowTexHeight = 128;
   }
   printf("Rendering %d x %d depth texture\n", ShadowTexWidth, ShadowTexHeight);

   if (UsePackedDepthStencil) {
      depthFormat = GL_DEPTH_STENCIL_EXT;
      depthType = GL_UNSIGNED_INT_24_8_EXT;
   }
   else {
      depthFormat = GL_DEPTH_COMPONENT;
      depthType = GL_UNSIGNED_INT;
   }

   /* compute frustum to enclose spot light cone */
   d = ShadowNear * tan(SpotAngle);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-d, d, -d, d, ShadowNear, ShadowFar);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(LightPos[0], LightPos[1], LightPos[2], /* from */
             0, 0, 0, /* target */
             0, 1, 0); /* up */

   if (UseFBO) {
      glTexImage2D(GL_TEXTURE_2D, 0, depthFormat,
                   ShadowTexWidth, ShadowTexHeight, 0,
                   depthFormat, depthType, NULL);
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ShadowFBO);
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);
      assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)
             == GL_FRAMEBUFFER_COMPLETE_EXT);
   }

   assert(!glIsEnabled(GL_TEXTURE_1D));
   assert(!glIsEnabled(GL_TEXTURE_2D));

   glViewport(0, 0, ShadowTexWidth, ShadowTexHeight);
   glClear(GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   DrawScene();

   if (UseFBO) {
      /* all done! */
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   }
   else {
      /*
       * copy depth buffer into the texture map
       */
      if (DisplayMode == SHOW_DEPTH_MAPPING) {
         /* load depth image as gray-scale luminance texture */
         GLuint *depth = (GLuint *)
            malloc(ShadowTexWidth * ShadowTexHeight * sizeof(GLuint));
         assert(depth);
         glReadPixels(0, 0, ShadowTexWidth, ShadowTexHeight,
                      depthFormat, depthType, depth);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                      ShadowTexWidth, ShadowTexHeight, 0,
                      GL_LUMINANCE, GL_UNSIGNED_INT, depth);
         free(depth);
      }
      else {
         /* The normal shadow case - a real depth texture */
         glCopyTexImage2D(GL_TEXTURE_2D, 0, depthFormat,
                          0, 0, ShadowTexWidth, ShadowTexHeight, 0);
         if (UsePackedDepthStencil) {
            /* debug check */
            GLint intFormat;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                                     GL_TEXTURE_INTERNAL_FORMAT, &intFormat);
            assert(intFormat == GL_DEPTH_STENCIL_EXT);
         }
      }
   }
}


/**
 * Show the shadow map as a grayscale image.
 */
static void
ShowShadowMap(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, WindowWidth, 0, WindowHeight, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);

   glEnable(GL_TEXTURE_2D);

   DisableTexgen();

   /* interpret texture's depth values as luminance values */
#if defined(GL_ARB_shadow)
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
   glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
#endif
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);  glVertex2f(0, 0);
   glTexCoord2f(1, 0);  glVertex2f(ShadowTexWidth, 0);
   glTexCoord2f(1, 1);  glVertex2f(ShadowTexWidth, ShadowTexHeight);
   glTexCoord2f(0, 1);  glVertex2f(0, ShadowTexHeight);
   glEnd();

   glDisable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
}


/**
 * Redraw window image
 */
static void
Display(void)
{
   GLenum error;

   ComputeLightPos(LightDist, LightLatitude, LightLongitude,
                   LightPos, SpotDir);

   if (NeedNewShadowMap) {
      RenderShadowMap();
      NeedNewShadowMap = GL_FALSE;
   }

   glViewport(0, 0, WindowWidth, WindowHeight);
   if (DisplayMode == SHOW_DEPTH_IMAGE) {
      ShowShadowMap();
   }
   else {
      /* prepare to draw scene from camera's view */
      const GLfloat ar = (GLfloat) WindowWidth / (GLfloat) WindowHeight;

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-ar, ar, -1.0, 1.0, 4.0, 50.0);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0.0, 0.0, -22.0);
      glRotatef(Xrot, 1, 0, 0);
      glRotatef(Yrot, 0, 1, 0);
      glRotatef(Zrot, 0, 0, 1);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glLightfv(GL_LIGHT0, GL_POSITION, LightPos);

      if (LinearFilter) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      if (DisplayMode == SHOW_DEPTH_MAPPING) {
#if defined(GL_ARB_shadow)
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
#endif
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
         glEnable(GL_TEXTURE_2D);
         MakeShadowMatrix(LightPos, SpotDir, SpotAngle, ShadowNear, ShadowFar);
         EnableIdentityTexgen();
      }
      else if (DisplayMode == SHOW_DISTANCE) {
         glMatrixMode(GL_TEXTURE);
         glLoadIdentity();
         glMatrixMode(GL_MODELVIEW);
         EnableDistanceTexgen(LightPos, SpotDir, ShadowNear+Bias, ShadowFar);
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
         glEnable(GL_TEXTURE_1D);
         assert(!glIsEnabled(GL_TEXTURE_2D));
      }
      else {
         assert(DisplayMode == SHOW_SHADOWS);
#if defined(GL_ARB_shadow)
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB,
                         GL_COMPARE_R_TO_TEXTURE_ARB);
#endif
         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
         glEnable(GL_TEXTURE_2D);
         MakeShadowMatrix(LightPos, SpotDir, SpotAngle, ShadowNear, ShadowFar);
         EnableIdentityTexgen();
      }

      DrawScene();

      DisableTexgen();
      glDisable(GL_TEXTURE_1D);
      glDisable(GL_TEXTURE_2D);
   }

   glutSwapBuffers();

   error = glGetError();
   if (error) {
      printf("GL Error: %s\n", (char *) gluErrorString(error));
   }
}


static void
Reshape(int width, int height)
{
   WindowWidth = width;
   WindowHeight = height;
   NeedNewShadowMap = GL_TRUE;
}


static void
Idle(void)
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;
   Yrot += 75.0 * dt;
   /*LightLongitude -= 5.0;*/
   glutPostRedisplay();
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'b':
         Bias -= 0.01;
         printf("Bias %g\n", Bias);
         break;
      case 'B':
         Bias += 0.01;
         printf("Bias %g\n", Bias);
         break;
      case 'd':
         DisplayMode = SHOW_DISTANCE;
         break;
      case 'f':
         LinearFilter = !LinearFilter;
         printf("%s filtering\n", LinearFilter ? "Bilinear" : "Nearest");
         break;
      case 'i':
         DisplayMode = SHOW_DEPTH_IMAGE;
         break;
      case 'm':
         DisplayMode = SHOW_DEPTH_MAPPING;
         break;
      case 'n':
      case 's':
      case ' ':
         DisplayMode = SHOW_SHADOWS;
         break;
      case 'o':
         if (HaveEXTshadowFuncs) {
            Operator++;
            if (Operator >= 8)
               Operator = 0;
            printf("Operator: %s\n", OperatorName[Operator]);
#if defined(GL_ARB_shadow)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB,
                            OperatorFunc[Operator]);
#endif
         }
         break;
      case 'p':
         UsePackedDepthStencil = !UsePackedDepthStencil;
         if (UsePackedDepthStencil && !HavePackedDepthStencil) {
            printf("Sorry, GL_EXT_packed_depth_stencil not supported\n");
            UsePackedDepthStencil = GL_FALSE;
         }
         else {
            printf("Use GL_DEPTH_STENCIL_EXT: %d\n", UsePackedDepthStencil);
            /* Don't really need to regenerate shadow map texture, but do so
             * to exercise more code more often.
             */
            NeedNewShadowMap = GL_TRUE;
         }
         break;
      case 'z':
         Zrot -= step;
         break;
      case 'Z':
         Zrot += step;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   const int mod = glutGetModifiers();
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         if (mod)
            LightLatitude += step;
         else
            Xrot += step;
         break;
      case GLUT_KEY_DOWN:
         if (mod)
            LightLatitude -= step;
         else
            Xrot -= step;
         break;
      case GLUT_KEY_LEFT:
         if (mod)
            LightLongitude += step;
         else
            Yrot += step;
         break;
      case GLUT_KEY_RIGHT:
         if (mod)
            LightLongitude -= step;
         else
            Yrot -= step;
         break;
   }
   if (mod)
      NeedNewShadowMap = GL_TRUE;

   glutPostRedisplay();
}


static void
Init(void)
{
   static const GLfloat borderColor[4] = {1.0, 0.0, 0.0, 0.0};

#if defined(GL_ARB_depth_texture) && defined(GL_ARB_shadow)
   if (!glutExtensionSupported("GL_ARB_depth_texture") ||
       !glutExtensionSupported("GL_ARB_shadow")) {
#else
   if (1) {
#endif
      printf("Sorry, this demo requires the GL_ARB_depth_texture and GL_ARB_shadow extensions\n");
      exit(1);
   }
   printf("Using GL_ARB_depth_texture and GL_ARB_shadow\n");

#if defined(GL_ARB_shadow_ambient)
   HaveShadowAmbient = glutExtensionSupported("GL_ARB_shadow_ambient");
   if (HaveShadowAmbient) {
      printf("and GL_ARB_shadow_ambient\n");
   }
#endif

   HaveEXTshadowFuncs = glutExtensionSupported("GL_EXT_shadow_funcs");

   HavePackedDepthStencil = glutExtensionSupported("GL_EXT_packed_depth_stencil");
   UsePackedDepthStencil = HavePackedDepthStencil;

#if defined(GL_EXT_framebuffer_object)
   HaveFBO = glutExtensionSupported("GL_EXT_framebuffer_object");
   UseFBO = HaveFBO;
   if (UseFBO) {
      printf("Using GL_EXT_framebuffer_object\n");
   }
#endif

   /*
    * Set up the 2D shadow map texture
    */
   glGenTextures(1, &ShadowTexture);
   glBindTexture(GL_TEXTURE_2D, ShadowTexture);
   glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if defined(GL_ARB_shadow)
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB,
                   GL_COMPARE_R_TO_TEXTURE_ARB);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
#endif
   if (HaveShadowAmbient) {
#if defined(GL_ARB_shadow_ambient)
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 0.3);
#endif
   }

#if defined(GL_EXT_framebuffer_object)
   if (UseFBO) {
      glGenFramebuffersEXT(1, &ShadowFBO);
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ShadowFBO);
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                   GL_COLOR_ATTACHMENT0_EXT,
                                   GL_RENDERBUFFER_EXT, 0);
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                GL_TEXTURE_2D, ShadowTexture, 0);

      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   }
#endif

   /*
    * Setup 1-D grayscale texture image for SHOW_DISTANCE mode
    */
   glGenTextures(1, &GrayTexture);
   glBindTexture(GL_TEXTURE_1D, GrayTexture);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   {
      GLuint i;
      GLubyte image[256];
      for (i = 0; i < 256; i++)
         image[i] = i;
      glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE,
                   256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
   }

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
}


static void
PrintHelp(void)
{
   printf("Keys:\n");
   printf("  a = toggle animation\n");
   printf("  i = show depth texture image\n");
   printf("  m = show depth texture mapping\n");
   printf("  d = show fragment distance from light source\n");
   printf("  n = show normal, shadowed image\n");
   printf("  f = toggle nearest/bilinear texture filtering\n");
   printf("  b/B = decrease/increase shadow map Z bias\n");
   printf("  p = toggle use of packed depth/stencil\n");
   printf("  cursor keys = rotate scene\n");
   printf("  <shift> + cursor keys = rotate light source\n");
   if (HaveEXTshadowFuncs)
      printf("  o = cycle through comparison modes\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WindowWidth, WindowHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
   glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Display);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   PrintHelp();
   glutMainLoop();
   return 0;
}
