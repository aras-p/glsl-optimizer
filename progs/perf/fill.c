/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Measure fill rates.
 *
 * Brian Paul
 * 21 Sep 2009
 */

#include "glmain.h"
#include "common.h"


int WinWidth = 1000, WinHeight = 1000;

static GLuint VBO, TexObj;


struct vertex
{
   GLfloat x, y, s, t, r, g, b, a;
};

#define VOFFSET(F) ((void *) offsetof(struct vertex, F))

static const struct vertex vertices[4] = {
   /*  x     y     s    t     r    g    b    a  */
   { -1.0, -1.0,  0.0, 0.0,  1.0, 0.0, 0.0, 0.5 },
   {  1.0, -1.0,  1.0, 0.0,  0.0, 1.0, 0.0, 0.5 },
   {  1.0,  1.0,  1.0, 1.0,  0.0, 0.0, 1.0, 0.5 },
   { -1.0,  1.0,  0.0, 1.0,  1.0, 1.0, 1.0, 0.5 }
};


static const char *VertexShader =
   "void main() \n"
   "{ \n"
   "   gl_Position = ftransform(); \n"
   "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
   "   gl_FrontColor = gl_Color; \n"
   "} \n";

/* simple fragment shader */
static const char *FragmentShader1 =
   "uniform sampler2D Tex; \n"
   "void main() \n"
   "{ \n"
   "   vec4 t = texture2D(Tex, gl_TexCoord[0].xy); \n"
   "   gl_FragColor = vec4(1.0) - t * gl_Color; \n"
   "} \n";

/**
 * A more complex fragment shader (but equivalent to first shader).
 * A good optimizer should catch some of these no-op operations, but
 * probably not all of them.
 */
static const char *FragmentShader2 =
   "uniform sampler2D Tex; \n"
   "void main() \n"
   "{ \n"
   "   // as above \n"
   "   vec4 t = texture2D(Tex, gl_TexCoord[0].xy); \n"
   "   t = vec4(1.0) - t * gl_Color; \n"

   "   vec4 u; \n"

   "   // no-op negate/swizzle \n"
   "   u = -t.wzyx; \n"
   "   t = -u.wzyx; \n"

   "   // no-op inverts \n"
   "   t = vec4(1.0) - t; \n"
   "   t = vec4(1.0) - t; \n"

   "   // no-op min/max \n"
   "   t = min(t, t); \n"
   "   t = max(t, t); \n"

   "   // no-op moves \n"
   "   u = t; \n"
   "   t = u; \n"
   "   u = t; \n"
   "   t = u; \n"

   "   // no-op add/mul \n"
   "   t = (t + t + t + t) * 0.25; \n"

   "   // no-op mul/sub \n"
   "   t = 3.0 * t - 2.0 * t; \n"

   "   // no-op negate/min/max \n"
   "   t = -min(-t, -t); \n"
   "   t = -max(-t, -t); \n"

   "   gl_FragColor = t; \n"
   "} \n";

static GLuint ShaderProg1, ShaderProg2;



/** Called from test harness/main */
void
PerfInit(void)
{
   GLint u;

   /* setup VBO w/ vertex data */
   glGenBuffersARB(1, &VBO);
   glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
   glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                   sizeof(vertices), vertices, GL_STATIC_DRAW_ARB);
   glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(x));
   glTexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(s));
   glColorPointer(4, GL_FLOAT, sizeof(struct vertex), VOFFSET(r));
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   /* setup texture */
   TexObj = PerfCheckerTexture(128, 128);

   /* setup shaders */
   ShaderProg1 = PerfShaderProgram(VertexShader, FragmentShader1);
   glUseProgram(ShaderProg1);
   u = glGetUniformLocation(ShaderProg1, "Tex");
   glUniform1i(u, 0);  /* texture unit 0 */

   ShaderProg2 = PerfShaderProgram(VertexShader, FragmentShader2);
   glUseProgram(ShaderProg2);
   u = glGetUniformLocation(ShaderProg2, "Tex");
   glUniform1i(u, 0);  /* texture unit 0 */

   glUseProgram(0);
}


static void
Ortho(void)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}



static void
DrawQuad(unsigned count)
{
   unsigned i;
   glClear(GL_COLOR_BUFFER_BIT);

   for (i = 0; i < count; i++) {
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      /* Avoid sending command buffers with huge numbers of fullscreen
       * quads.  Graphics schedulers don't always cope well with
       * this...
       */
      if (i % 128 == 0) {
         PerfSwapBuffers();
         glClear(GL_COLOR_BUFFER_BIT);
      }
   }

   glFinish();

   if (1)
      PerfSwapBuffers();
}

void
PerfNextRound(void)
{
}

/** Called from test harness/main */
void
PerfDraw(void)
{
   double rate;
   double pixelsPerDraw = WinWidth * WinHeight;

   Ortho();

   /* simple fill */
   rate = PerfMeasureRate(DrawQuad) * pixelsPerDraw;
   perf_printf("   Simple fill: %s pixels/second\n", 
               PerfHumanFloat(rate));

   /* blended fill */
   glEnable(GL_BLEND);
   rate = PerfMeasureRate(DrawQuad) * pixelsPerDraw;
   glDisable(GL_BLEND);
   perf_printf("   Blended fill: %s pixels/second\n", 
               PerfHumanFloat(rate));

   /* textured fill */
   glEnable(GL_TEXTURE_2D);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   rate = PerfMeasureRate(DrawQuad) * pixelsPerDraw;
   glDisable(GL_TEXTURE_2D);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   perf_printf("   Textured fill: %s pixels/second\n", 
               PerfHumanFloat(rate));

   /* shader1 fill */
   glUseProgram(ShaderProg1);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   rate = PerfMeasureRate(DrawQuad) * pixelsPerDraw;
   glUseProgram(0);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   perf_printf("   Shader1 fill: %s pixels/second\n", 
               PerfHumanFloat(rate));

   /* shader2 fill */
   glUseProgram(ShaderProg2);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   rate = PerfMeasureRate(DrawQuad) * pixelsPerDraw;
   glUseProgram(0);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   perf_printf("   Shader2 fill: %s pixels/second\n", 
               PerfHumanFloat(rate));

   exit(0);
}

