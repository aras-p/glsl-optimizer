/* $Id: context.c,v 1.40 2000/02/02 22:21:39 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "accum.h"
#include "alphabuf.h"
#include "clip.h"
#include "context.h"
#include "cva.h"
#include "depth.h"
#include "dlist.h"
#include "eval.h"
#include "enums.h"
#include "extensions.h"
#include "fog.h"
#include "get.h"
#include "glapi.h"
#include "glapinoop.h"
#include "glthread.h"
#include "hash.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "mmath.h"
#include "pb.h"
#include "pipeline.h"
#include "shade.h"
#include "simple_list.h"
#include "stencil.h"
#include "stages.h"
#include "state.h"
#include "translate.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "texture.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbcull.h"
#include "vbrender.h"
#include "vbxform.h"
#include "vertices.h"
#include "xform.h"
#endif



/**********************************************************************/
/*****                  Context and Thread management             *****/
/**********************************************************************/


#if !defined(THREADS)

struct immediate *_mesa_CurrentInput = NULL;

#endif




/**********************************************************************/
/*****                   Profiling functions                      *****/
/**********************************************************************/

#ifdef PROFILE

#include <sys/times.h>
#include <sys/param.h>


/*
 * Return system time in seconds.
 * NOTE:  this implementation may not be very portable!
 */
GLdouble gl_time( void )
{
   static GLdouble prev_time = 0.0;
   static GLdouble time;
   struct tms tm;
   clock_t clk;

   clk = times(&tm);

#ifdef CLK_TCK
   time = (double)clk / (double)CLK_TCK;
#else
   time = (double)clk / (double)HZ;
#endif

   if (time>prev_time) {
      prev_time = time;
      return time;
   }
   else {
      return prev_time;
   }
}

/*
 * Reset the timing/profiling counters
 */
static void init_timings( GLcontext *ctx )
{
   ctx->BeginEndCount = 0;
   ctx->BeginEndTime = 0.0;
   ctx->VertexCount = 0;
   ctx->VertexTime = 0.0;
   ctx->PointCount = 0;
   ctx->PointTime = 0.0;
   ctx->LineCount = 0;
   ctx->LineTime = 0.0;
   ctx->PolygonCount = 0;
   ctx->PolygonTime = 0.0;
   ctx->ClearCount = 0;
   ctx->ClearTime = 0.0;
   ctx->SwapCount = 0;
   ctx->SwapTime = 0.0;
}


/*
 * Print the accumulated timing/profiling data.
 */
static void print_timings( GLcontext *ctx )
{
   GLdouble beginendrate;
   GLdouble vertexrate;
   GLdouble pointrate;
   GLdouble linerate;
   GLdouble polygonrate;
   GLdouble overhead;
   GLdouble clearrate;
   GLdouble swaprate;
   GLdouble avgvertices;

   if (ctx->BeginEndTime>0.0) {
      beginendrate = ctx->BeginEndCount / ctx->BeginEndTime;
   }
   else {
      beginendrate = 0.0;
   }
   if (ctx->VertexTime>0.0) {
      vertexrate = ctx->VertexCount / ctx->VertexTime;
   }
   else {
      vertexrate = 0.0;
   }
   if (ctx->PointTime>0.0) {
      pointrate = ctx->PointCount / ctx->PointTime;
   }
   else {
      pointrate = 0.0;
   }
   if (ctx->LineTime>0.0) {
      linerate = ctx->LineCount / ctx->LineTime;
   }
   else {
      linerate = 0.0;
   }
   if (ctx->PolygonTime>0.0) {
      polygonrate = ctx->PolygonCount / ctx->PolygonTime;
   }
   else {
      polygonrate = 0.0;
   }
   if (ctx->ClearTime>0.0) {
      clearrate = ctx->ClearCount / ctx->ClearTime;
   }
   else {
      clearrate = 0.0;
   }
   if (ctx->SwapTime>0.0) {
      swaprate = ctx->SwapCount / ctx->SwapTime;
   }
   else {
      swaprate = 0.0;
   }

   if (ctx->BeginEndCount>0) {
      avgvertices = (GLdouble) ctx->VertexCount / (GLdouble) ctx->BeginEndCount;
   }
   else {
      avgvertices = 0.0;
   }

   overhead = ctx->BeginEndTime - ctx->VertexTime - ctx->PointTime
              - ctx->LineTime - ctx->PolygonTime;


   printf("                          Count   Time (s)    Rate (/s) \n");
   printf("--------------------------------------------------------\n");
   printf("glBegin/glEnd           %7d  %8.3f   %10.3f\n",
          ctx->BeginEndCount, ctx->BeginEndTime, beginendrate);
   printf("  vertexes transformed  %7d  %8.3f   %10.3f\n",
          ctx->VertexCount, ctx->VertexTime, vertexrate );
   printf("  points rasterized     %7d  %8.3f   %10.3f\n",
          ctx->PointCount, ctx->PointTime, pointrate );
   printf("  lines rasterized      %7d  %8.3f   %10.3f\n",
          ctx->LineCount, ctx->LineTime, linerate );
   printf("  polygons rasterized   %7d  %8.3f   %10.3f\n",
          ctx->PolygonCount, ctx->PolygonTime, polygonrate );
   printf("  overhead                       %8.3f\n", overhead );
   printf("glClear                 %7d  %8.3f   %10.3f\n",
          ctx->ClearCount, ctx->ClearTime, clearrate );
   printf("SwapBuffers             %7d  %8.3f   %10.3f\n",
          ctx->SwapCount, ctx->SwapTime, swaprate );
   printf("\n");

   printf("Average number of vertices per begin/end: %8.3f\n", avgvertices );
}
#endif





/**********************************************************************/
/***** GL Visual allocation/destruction                           *****/
/**********************************************************************/


/*
 * Allocate a new GLvisual object.
 * Input:  rgbFlag - GL_TRUE=RGB(A) mode, GL_FALSE=Color Index mode
 *         alphaFlag - alloc software alpha buffers?
 *         dbFlag - double buffering?
 *         stereoFlag - stereo buffer?
 *         depthFits - requested minimum bits per depth buffer value
 *         stencilFits - requested minimum bits per stencil buffer value
 *         accumFits - requested minimum bits per accum buffer component
 *         indexFits - number of bits per pixel if rgbFlag==GL_FALSE
 *         red/green/blue/alphaFits - number of bits per color component
 *                                     in frame buffer for RGB(A) mode.
 * Return:  pointer to new GLvisual or NULL if requested parameters can't
 *          be met.
 */
GLvisual *gl_create_visual( GLboolean rgbFlag,
                            GLboolean alphaFlag,
                            GLboolean dbFlag,
                            GLboolean stereoFlag,
                            GLint depthBits,
                            GLint stencilBits,
                            GLint accumBits,
                            GLint indexBits,
                            GLint redBits,
                            GLint greenBits,
                            GLint blueBits,
                            GLint alphaBits )
{
   GLvisual *vis;

   if (depthBits > (GLint) (8*sizeof(GLdepth))) {
      /* can't meet depth buffer requirements */
      return NULL;
   }
   if (stencilBits > (GLint) (8*sizeof(GLstencil))) {
      /* can't meet stencil buffer requirements */
      return NULL;
   }
   if (accumBits > (GLint) (8*sizeof(GLaccum))) {
      /* can't meet accum buffer requirements */
      return NULL;
   }

   vis = (GLvisual *) CALLOC( sizeof(GLvisual) );
   if (!vis) {
      return NULL;
   }

   vis->RGBAflag   = rgbFlag;
   vis->DBflag     = dbFlag;
   vis->StereoFlag = stereoFlag;
   vis->RedBits    = redBits;
   vis->GreenBits  = greenBits;
   vis->BlueBits   = blueBits;
   vis->AlphaBits  = alphaFlag ? 8*sizeof(GLubyte) : alphaBits;

   vis->IndexBits   = indexBits;
   vis->DepthBits   = (depthBits>0) ? 8*sizeof(GLdepth) : 0;
   vis->AccumBits   = (accumBits>0) ? 8*sizeof(GLaccum) : 0;
   vis->StencilBits = (stencilBits>0) ? 8*sizeof(GLstencil) : 0;

   vis->SoftwareAlpha = alphaFlag;

   return vis;
}



void gl_destroy_visual( GLvisual *vis )
{
   FREE( vis );
}



/**********************************************************************/
/***** GL Framebuffer allocation/destruction                      *****/
/**********************************************************************/


/*
 * Create a new framebuffer.  A GLframebuffer is a struct which
 * encapsulates the depth, stencil and accum buffers and related
 * parameters.
 * Input:  visual - a GLvisual pointer
 *         softwareDepth - create/use a software depth buffer?
 *         softwareStencil - create/use a software stencil buffer?
 *         softwareAccum - create/use a software accum buffer?
 *         softwareAlpha - create/use a software alpha buffer?

 * Return:  pointer to new GLframebuffer struct or NULL if error.
 */
GLframebuffer *gl_create_framebuffer( GLvisual *visual,
                                      GLboolean softwareDepth,
                                      GLboolean softwareStencil,
                                      GLboolean softwareAccum,
                                      GLboolean softwareAlpha )
{
   GLframebuffer *buffer;

   buffer = CALLOC_STRUCT(gl_frame_buffer);
   if (!buffer) {
      return NULL;
   }

   /* sanity checks */
   if (softwareDepth ) {
      assert(visual->DepthBits > 0);
   }
   if (softwareStencil) {
      assert(visual->StencilBits > 0);
   }
   if (softwareAccum) {
      assert(visual->RGBAflag);
      assert(visual->AccumBits > 0);
   }
   if (softwareAlpha) {
      assert(visual->RGBAflag);
      assert(visual->AlphaBits > 0);
   }

   buffer->Visual = visual;
   buffer->UseSoftwareDepthBuffer = softwareDepth;
   buffer->UseSoftwareStencilBuffer = softwareStencil;
   buffer->UseSoftwareAccumBuffer = softwareAccum;
   buffer->UseSoftwareAlphaBuffers = softwareAlpha;

   return buffer;
}



/*
 * Free a framebuffer struct and its buffers.
 */
void gl_destroy_framebuffer( GLframebuffer *buffer )
{
   if (buffer) {
      if (buffer->Depth) {
         FREE( buffer->Depth );
      }
      if (buffer->Accum) {
         FREE( buffer->Accum );
      }
      if (buffer->Stencil) {
         FREE( buffer->Stencil );
      }
      if (buffer->FrontLeftAlpha) {
         FREE( buffer->FrontLeftAlpha );
      }
      if (buffer->BackLeftAlpha) {
         FREE( buffer->BackLeftAlpha );
      }
      if (buffer->FrontRightAlpha) {
         FREE( buffer->FrontRightAlpha );
      }
      if (buffer->BackRightAlpha) {
         FREE( buffer->BackRightAlpha );
      }
      FREE(buffer);
   }
}



/**********************************************************************/
/*****       Context allocation, initialization, destroying       *****/
/**********************************************************************/


_glthread_DECLARE_STATIC_MUTEX(OneTimeLock);


/*
 * This function just calls all the various one-time-init functions in Mesa.
 */
static void one_time_init( void )
{
   static GLboolean alreadyCalled = GL_FALSE;
   _glthread_LOCK_MUTEX(OneTimeLock);
   if (!alreadyCalled) {
      /* do some implementation tests */
      assert( sizeof(GLbyte) == 1 );
      assert( sizeof(GLshort) >= 2 );
      assert( sizeof(GLint) >= 4 );
      assert( sizeof(GLubyte) == 1 );
      assert( sizeof(GLushort) >= 2 );
      assert( sizeof(GLuint) >= 4 );

      gl_init_clip();
      gl_init_eval();
      _mesa_init_fog();
      gl_init_math();
      gl_init_lists();
      gl_init_shade();
      gl_init_texture();
      gl_init_transformation();
      gl_init_translate();
      gl_init_vbrender();
      gl_init_vbxform();
      gl_init_vertices();

      if (getenv("MESA_DEBUG")) {
         _glapi_noop_enable_warnings(GL_TRUE);
      }
      else {
         _glapi_noop_enable_warnings(GL_FALSE);
      }

#if defined(DEBUG) && defined(__DATE__) && defined(__TIME__)
   fprintf(stderr, "Mesa DEBUG build %s %s\n", __DATE__, __TIME__);
#endif

      alreadyCalled = GL_TRUE;
   }
   _glthread_UNLOCK_MUTEX(OneTimeLock);
}



/*
 * Allocate and initialize a shared context state structure.
 */
static struct gl_shared_state *alloc_shared_state( void )
{
   GLuint d;
   struct gl_shared_state *ss;
   GLboolean outOfMemory;

   ss = CALLOC_STRUCT(gl_shared_state);
   if (!ss)
      return NULL;

   ss->DisplayList = _mesa_NewHashTable();

   ss->TexObjects = _mesa_NewHashTable();

   /* Default Texture objects */
   outOfMemory = GL_FALSE;
   for (d = 1 ; d <= 3 ; d++) {
      ss->DefaultD[d] = gl_alloc_texture_object(ss, 0, d);
      if (!ss->DefaultD[d]) {
         outOfMemory = GL_TRUE;
         break;
      }
      ss->DefaultD[d]->RefCount++; /* don't free if not in use */
   }

   if (!ss->DisplayList || !ss->TexObjects || outOfMemory) {
      /* Ran out of memory at some point.  Free everything and return NULL */
      if (ss->DisplayList)
         _mesa_DeleteHashTable(ss->DisplayList);
      if (ss->TexObjects)
         _mesa_DeleteHashTable(ss->TexObjects);
      if (ss->DefaultD[1])
         gl_free_texture_object(ss, ss->DefaultD[1]);
      if (ss->DefaultD[2])
         gl_free_texture_object(ss, ss->DefaultD[2]);
      if (ss->DefaultD[3])
         gl_free_texture_object(ss, ss->DefaultD[3]);
      FREE(ss);
      return NULL;
   }
   else {
      return ss;
   }
}


/*
 * Deallocate a shared state context and all children structures.
 */
static void free_shared_state( GLcontext *ctx, struct gl_shared_state *ss )
{
   /* Free display lists */
   while (1) {
      GLuint list = _mesa_HashFirstEntry(ss->DisplayList);
      if (list) {
         gl_destroy_list(ctx, list);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->DisplayList);

   /* Free texture objects */
   while (ss->TexObjectList)
   {
      if (ctx->Driver.DeleteTexture)
         (*ctx->Driver.DeleteTexture)( ctx, ss->TexObjectList );
      /* this function removes from linked list too! */
      gl_free_texture_object(ss, ss->TexObjectList);
   }
   _mesa_DeleteHashTable(ss->TexObjects);

   FREE(ss);
}



/*
 * Initialize the nth light.  Note that the defaults for light 0 are
 * different than the other lights.
 */
static void init_light( struct gl_light *l, GLuint n )
{
   make_empty_list( l );

   ASSIGN_4V( l->Ambient, 0.0, 0.0, 0.0, 1.0 );
   if (n==0) {
      ASSIGN_4V( l->Diffuse, 1.0, 1.0, 1.0, 1.0 );
      ASSIGN_4V( l->Specular, 1.0, 1.0, 1.0, 1.0 );
   }
   else {
      ASSIGN_4V( l->Diffuse, 0.0, 0.0, 0.0, 1.0 );
      ASSIGN_4V( l->Specular, 0.0, 0.0, 0.0, 1.0 );
   }
   ASSIGN_4V( l->EyePosition, 0.0, 0.0, 1.0, 0.0 );
   ASSIGN_3V( l->EyeDirection, 0.0, 0.0, -1.0 );
   l->SpotExponent = 0.0;
   gl_compute_spot_exp_table( l );
   l->SpotCutoff = 180.0;
   l->CosCutoff = 0.0;		/* KW: -ve values not admitted */
   l->ConstantAttenuation = 1.0;
   l->LinearAttenuation = 0.0;
   l->QuadraticAttenuation = 0.0;
   l->Enabled = GL_FALSE;
}



static void init_lightmodel( struct gl_lightmodel *lm )
{
   ASSIGN_4V( lm->Ambient, 0.2, 0.2, 0.2, 1.0 );
   lm->LocalViewer = GL_FALSE;
   lm->TwoSide = GL_FALSE;
   lm->ColorControl = GL_SINGLE_COLOR;
}


static void init_material( struct gl_material *m )
{
   ASSIGN_4V( m->Ambient,  0.2, 0.2, 0.2, 1.0 );
   ASSIGN_4V( m->Diffuse,  0.8, 0.8, 0.8, 1.0 );
   ASSIGN_4V( m->Specular, 0.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( m->Emission, 0.0, 0.0, 0.0, 1.0 );
   m->Shininess = 0.0;
   m->AmbientIndex = 0;
   m->DiffuseIndex = 1;
   m->SpecularIndex = 1;
}



static void init_texture_unit( GLcontext *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   texUnit->EnvMode = GL_MODULATE;
   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );
   texUnit->TexGenEnabled = 0;
   texUnit->GenModeS = GL_EYE_LINEAR;
   texUnit->GenModeT = GL_EYE_LINEAR;
   texUnit->GenModeR = GL_EYE_LINEAR;
   texUnit->GenModeQ = GL_EYE_LINEAR;
   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->ObjectPlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneQ, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneQ, 0.0, 0.0, 0.0, 0.0 );

   texUnit->CurrentD[1] = ctx->Shared->DefaultD[1];
   texUnit->CurrentD[2] = ctx->Shared->DefaultD[2];
   texUnit->CurrentD[3] = ctx->Shared->DefaultD[3];
}


static void init_fallback_arrays( GLcontext *ctx )
{
   struct gl_client_array *cl;
   GLuint i;

   cl = &ctx->Fallback.Normal;
   cl->Size = 3;
   cl->Type = GL_FLOAT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) ctx->Current.Normal;
   cl->Enabled = 1;

   cl = &ctx->Fallback.Color;
   cl->Size = 4;
   cl->Type = GL_UNSIGNED_BYTE;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) ctx->Current.ByteColor;
   cl->Enabled = 1;

   cl = &ctx->Fallback.Index;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_INT;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) &ctx->Current.Index;
   cl->Enabled = 1;

   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++) {
      cl = &ctx->Fallback.TexCoord[i];
      cl->Size = 4;
      cl->Type = GL_FLOAT;
      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Ptr = (void *) ctx->Current.Texcoord[i];
      cl->Enabled = 1;
   }

   cl = &ctx->Fallback.EdgeFlag;
   cl->Size = 1;
   cl->Type = GL_UNSIGNED_BYTE;
   cl->Stride = 0;
   cl->StrideB = 0;
   cl->Ptr = (void *) &ctx->Current.EdgeFlag;
   cl->Enabled = 1;
}


/* Initialize a 1-D evaluator map */
static void init_1d_map( struct gl_1d_map *map, int n, const float *initial )
{
   map->Order = 1;
   map->u1 = 0.0;
   map->u2 = 1.0;
   map->Points = (GLfloat *) MALLOC(n * sizeof(GLfloat));
   if (map->Points) {
      GLint i;
      for (i=0;i<n;i++)
         map->Points[i] = initial[i];
   }
}


/* Initialize a 2-D evaluator map */
static void init_2d_map( struct gl_2d_map *map, int n, const float *initial )
{
   map->Uorder = 1;
   map->Vorder = 1;
   map->u1 = 0.0;
   map->u2 = 1.0;
   map->v1 = 0.0;
   map->v2 = 1.0;
   map->Points = (GLfloat *) MALLOC(n * sizeof(GLfloat));
   if (map->Points) {
      GLint i;
      for (i=0;i<n;i++)
         map->Points[i] = initial[i];
   }
}


static void init_color_table( struct gl_color_table *p )
{
   p->Table[0] = 255;
   p->Table[1] = 255;
   p->Table[2] = 255;
   p->Table[3] = 255;
   p->Size = 1;
   p->IntFormat = GL_RGBA;
   p->Format = GL_RGBA;
}


/*
 * Initialize the attribute groups in a GLcontext.
 */
static void init_attrib_groups( GLcontext *ctx )
{
   GLuint i, j;

   assert(ctx);

   /* Constants, may be overriden by device driver */
   ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.MaxTextureSize = 1 << (MAX_TEXTURE_LEVELS - 1);
   ctx->Const.MaxTextureUnits = MAX_TEXTURE_UNITS;
   ctx->Const.MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;

   /* Modelview matrix */
   gl_matrix_ctr( &ctx->ModelView );
   gl_matrix_alloc_inv( &ctx->ModelView );

   ctx->ModelViewStackDepth = 0;
   for (i = 0 ; i < MAX_MODELVIEW_STACK_DEPTH ; i++) {
      gl_matrix_ctr( &ctx->ModelViewStack[i] );
      gl_matrix_alloc_inv( &ctx->ModelViewStack[i] );
   }

   /* Projection matrix - need inv for user clipping in clip space*/
   gl_matrix_ctr( &ctx->ProjectionMatrix );
   gl_matrix_alloc_inv( &ctx->ProjectionMatrix );

   gl_matrix_ctr( &ctx->ModelProjectMatrix );
   gl_matrix_ctr( &ctx->ModelProjectWinMatrix );
   ctx->ModelProjectWinMatrixUptodate = GL_FALSE;

   ctx->ProjectionStackDepth = 0;
   ctx->NearFarStack[0][0] = 1.0; /* These values seem weird by make */
   ctx->NearFarStack[0][1] = 0.0; /* sense mathematically. */

   for (i = 0 ; i < MAX_PROJECTION_STACK_DEPTH ; i++) {
      gl_matrix_ctr( &ctx->ProjectionStack[i] );
      gl_matrix_alloc_inv( &ctx->ProjectionStack[i] );
   }

   /* Texture matrix */
   for (i=0; i<MAX_TEXTURE_UNITS; i++) {
      gl_matrix_ctr( &ctx->TextureMatrix[i] );
      ctx->TextureStackDepth[i] = 0;
      for (j = 0 ; j < MAX_TEXTURE_STACK_DEPTH ; j++) {
         ctx->TextureStack[i][j].inv = 0;
      }
   }

   /* Accumulate buffer group */
   ASSIGN_4V( ctx->Accum.ClearColor, 0.0, 0.0, 0.0, 0.0 );

   /* Color buffer group */
   ctx->Color.IndexMask = 0xffffffff;
   ctx->Color.ColorMask[0] = 0xff;
   ctx->Color.ColorMask[1] = 0xff;
   ctx->Color.ColorMask[2] = 0xff;
   ctx->Color.ColorMask[3] = 0xff;
   ctx->Color.SWmasking = GL_FALSE;
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.DrawBuffer = GL_FRONT;
   ctx->Color.AlphaEnabled = GL_FALSE;
   ctx->Color.AlphaFunc = GL_ALWAYS;
   ctx->Color.AlphaRef = 0;
   ctx->Color.BlendEnabled = GL_FALSE;
   ctx->Color.BlendSrcRGB = GL_ONE;
   ctx->Color.BlendDstRGB = GL_ZERO;
   ctx->Color.BlendSrcA = GL_ONE;
   ctx->Color.BlendDstA = GL_ZERO;
   ctx->Color.BlendEquation = GL_FUNC_ADD_EXT;
   ctx->Color.BlendFunc = NULL;  /* this pointer set only when needed */
   ASSIGN_4V( ctx->Color.BlendColor, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color.SWLogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color.DitherFlag = GL_TRUE;
   ctx->Color.MultiDrawBuffer = GL_FALSE;

   /* Current group */
   ASSIGN_4V( ctx->Current.ByteColor, 255, 255, 255, 255);
   ctx->Current.Index = 1;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.Texcoord[i], 0.0, 0.0, 0.0, 1.0 );
   ASSIGN_4V( ctx->Current.RasterPos, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterDistance = 0.0;
   ASSIGN_4V( ctx->Current.RasterColor, 1.0, 1.0, 1.0, 1.0 );
   ctx->Current.RasterIndex = 1;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.RasterMultiTexCoord[i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterTexCoord = ctx->Current.RasterMultiTexCoord[0];
   ctx->Current.RasterPosValid = GL_TRUE;
   ctx->Current.EdgeFlag = GL_TRUE;
   ASSIGN_3V( ctx->Current.Normal, 0.0, 0.0, 1.0 );
   ctx->Current.Primitive = (GLenum) (GL_POLYGON + 1);

   ctx->Current.Flag = (VERT_NORM|VERT_INDEX|VERT_RGBA|VERT_EDGE|
                        VERT_TEX0_1|VERT_TEX1_1|VERT_MATERIAL);

   init_fallback_arrays( ctx );

   /* Depth buffer group */
   ctx->Depth.Test = GL_FALSE;
   ctx->Depth.Clear = 1.0;
   ctx->Depth.Func = GL_LESS;
   ctx->Depth.Mask = GL_TRUE;

   /* Evaluators group */
   ctx->Eval.Map1Color4 = GL_FALSE;
   ctx->Eval.Map1Index = GL_FALSE;
   ctx->Eval.Map1Normal = GL_FALSE;
   ctx->Eval.Map1TextureCoord1 = GL_FALSE;
   ctx->Eval.Map1TextureCoord2 = GL_FALSE;
   ctx->Eval.Map1TextureCoord3 = GL_FALSE;
   ctx->Eval.Map1TextureCoord4 = GL_FALSE;
   ctx->Eval.Map1Vertex3 = GL_FALSE;
   ctx->Eval.Map1Vertex4 = GL_FALSE;
   ctx->Eval.Map2Color4 = GL_FALSE;
   ctx->Eval.Map2Index = GL_FALSE;
   ctx->Eval.Map2Normal = GL_FALSE;
   ctx->Eval.Map2TextureCoord1 = GL_FALSE;
   ctx->Eval.Map2TextureCoord2 = GL_FALSE;
   ctx->Eval.Map2TextureCoord3 = GL_FALSE;
   ctx->Eval.Map2TextureCoord4 = GL_FALSE;
   ctx->Eval.Map2Vertex3 = GL_FALSE;
   ctx->Eval.Map2Vertex4 = GL_FALSE;
   ctx->Eval.AutoNormal = GL_FALSE;
   ctx->Eval.MapGrid1un = 1;
   ctx->Eval.MapGrid1u1 = 0.0;
   ctx->Eval.MapGrid1u2 = 1.0;
   ctx->Eval.MapGrid2un = 1;
   ctx->Eval.MapGrid2vn = 1;
   ctx->Eval.MapGrid2u1 = 0.0;
   ctx->Eval.MapGrid2u2 = 1.0;
   ctx->Eval.MapGrid2v1 = 0.0;
   ctx->Eval.MapGrid2v2 = 1.0;

   /* Evaluator data */
   {
      static GLfloat vertex[4] = { 0.0, 0.0, 0.0, 1.0 };
      static GLfloat normal[3] = { 0.0, 0.0, 1.0 };
      static GLfloat index[1] = { 1.0 };
      static GLfloat color[4] = { 1.0, 1.0, 1.0, 1.0 };
      static GLfloat texcoord[4] = { 0.0, 0.0, 0.0, 1.0 };

      init_1d_map( &ctx->EvalMap.Map1Vertex3, 3, vertex );
      init_1d_map( &ctx->EvalMap.Map1Vertex4, 4, vertex );
      init_1d_map( &ctx->EvalMap.Map1Index, 1, index );
      init_1d_map( &ctx->EvalMap.Map1Color4, 4, color );
      init_1d_map( &ctx->EvalMap.Map1Normal, 3, normal );
      init_1d_map( &ctx->EvalMap.Map1Texture1, 1, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture2, 2, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture3, 3, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture4, 4, texcoord );

      init_2d_map( &ctx->EvalMap.Map2Vertex3, 3, vertex );
      init_2d_map( &ctx->EvalMap.Map2Vertex4, 4, vertex );
      init_2d_map( &ctx->EvalMap.Map2Index, 1, index );
      init_2d_map( &ctx->EvalMap.Map2Color4, 4, color );
      init_2d_map( &ctx->EvalMap.Map2Normal, 3, normal );
      init_2d_map( &ctx->EvalMap.Map2Texture1, 1, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture2, 2, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture3, 3, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture4, 4, texcoord );
   }

   /* Fog group */
   ctx->Fog.Enabled = GL_FALSE;
   ctx->Fog.Mode = GL_EXP;
   ASSIGN_4V( ctx->Fog.Color, 0.0, 0.0, 0.0, 0.0 );
   ctx->Fog.Index = 0.0;
   ctx->Fog.Density = 1.0;
   ctx->Fog.Start = 0.0;
   ctx->Fog.End = 1.0;

   /* Hint group */
   ctx->Hint.PerspectiveCorrection = GL_DONT_CARE;
   ctx->Hint.PointSmooth = GL_DONT_CARE;
   ctx->Hint.LineSmooth = GL_DONT_CARE;
   ctx->Hint.PolygonSmooth = GL_DONT_CARE;
   ctx->Hint.Fog = GL_DONT_CARE;

   ctx->Hint.AllowDrawWin = GL_TRUE;
   ctx->Hint.AllowDrawSpn = GL_TRUE;
   ctx->Hint.AllowDrawMem = GL_TRUE;
   ctx->Hint.StrictLighting = GL_TRUE;

   /* Pipeline */
   gl_pipeline_init( ctx );
   gl_cva_init( ctx );

   /* Extensions */
   gl_extensions_ctr( ctx );

   ctx->AllowVertexCull = CLIP_CULLED_BIT;

   /* Lighting group */
   for (i=0;i<MAX_LIGHTS;i++) {
      init_light( &ctx->Light.Light[i], i );
   }
   make_empty_list( &ctx->Light.EnabledList );

   init_lightmodel( &ctx->Light.Model );
   init_material( &ctx->Light.Material[0] );
   init_material( &ctx->Light.Material[1] );
   ctx->Light.ShadeModel = GL_SMOOTH;
   ctx->Light.Enabled = GL_FALSE;
   ctx->Light.ColorMaterialFace = GL_FRONT_AND_BACK;
   ctx->Light.ColorMaterialMode = GL_AMBIENT_AND_DIFFUSE;
   ctx->Light.ColorMaterialBitmask
      = gl_material_bitmask( ctx,
                             GL_FRONT_AND_BACK,
                             GL_AMBIENT_AND_DIFFUSE, ~0, 0 );

   ctx->Light.ColorMaterialEnabled = GL_FALSE;

   /* Lighting miscellaneous */
   ctx->ShineTabList = MALLOC_STRUCT( gl_shine_tab );
   make_empty_list( ctx->ShineTabList );
   for (i = 0 ; i < 10 ; i++) {
      struct gl_shine_tab *s = MALLOC_STRUCT( gl_shine_tab );
      s->shininess = -1;
      s->refcount = 0;
      insert_at_tail( ctx->ShineTabList, s );
   }
   for (i = 0 ; i < 4 ; i++) {
      ctx->ShineTable[i] = ctx->ShineTabList->prev;
      ctx->ShineTable[i]->refcount++;
   }


   /* Line group */
   ctx->Line.SmoothFlag = GL_FALSE;
   ctx->Line.StippleFlag = GL_FALSE;
   ctx->Line.Width = 1.0;
   ctx->Line.StipplePattern = 0xffff;
   ctx->Line.StippleFactor = 1;

   /* Display List group */
   ctx->List.ListBase = 0;

   /* Pixel group */
   ctx->Pixel.RedBias = 0.0;
   ctx->Pixel.RedScale = 1.0;
   ctx->Pixel.GreenBias = 0.0;
   ctx->Pixel.GreenScale = 1.0;
   ctx->Pixel.BlueBias = 0.0;
   ctx->Pixel.BlueScale = 1.0;
   ctx->Pixel.AlphaBias = 0.0;
   ctx->Pixel.AlphaScale = 1.0;
   ctx->Pixel.ScaleOrBiasRGBA = GL_FALSE;
   ctx->Pixel.DepthBias = 0.0;
   ctx->Pixel.DepthScale = 1.0;
   ctx->Pixel.IndexOffset = 0;
   ctx->Pixel.IndexShift = 0;
   ctx->Pixel.ZoomX = 1.0;
   ctx->Pixel.ZoomY = 1.0;
   ctx->Pixel.MapColorFlag = GL_FALSE;
   ctx->Pixel.MapStencilFlag = GL_FALSE;
   ctx->Pixel.MapStoSsize = 1;
   ctx->Pixel.MapItoIsize = 1;
   ctx->Pixel.MapItoRsize = 1;
   ctx->Pixel.MapItoGsize = 1;
   ctx->Pixel.MapItoBsize = 1;
   ctx->Pixel.MapItoAsize = 1;
   ctx->Pixel.MapRtoRsize = 1;
   ctx->Pixel.MapGtoGsize = 1;
   ctx->Pixel.MapBtoBsize = 1;
   ctx->Pixel.MapAtoAsize = 1;
   ctx->Pixel.MapStoS[0] = 0;
   ctx->Pixel.MapItoI[0] = 0;
   ctx->Pixel.MapItoR[0] = 0.0;
   ctx->Pixel.MapItoG[0] = 0.0;
   ctx->Pixel.MapItoB[0] = 0.0;
   ctx->Pixel.MapItoA[0] = 0.0;
   ctx->Pixel.MapItoR8[0] = 0;
   ctx->Pixel.MapItoG8[0] = 0;
   ctx->Pixel.MapItoB8[0] = 0;
   ctx->Pixel.MapItoA8[0] = 0;
   ctx->Pixel.MapRtoR[0] = 0.0;
   ctx->Pixel.MapGtoG[0] = 0.0;
   ctx->Pixel.MapBtoB[0] = 0.0;
   ctx->Pixel.MapAtoA[0] = 0.0;

   /* Point group */
   ctx->Point.SmoothFlag = GL_FALSE;
   ctx->Point.Size = 1.0;
   ctx->Point.Params[0] = 1.0;
   ctx->Point.Params[1] = 0.0;
   ctx->Point.Params[2] = 0.0;
   ctx->Point.Attenuated = GL_FALSE;
   ctx->Point.MinSize = 0.0;
   ctx->Point.MaxSize = (GLfloat) MAX_POINT_SIZE;
   ctx->Point.Threshold = 1.0;

   /* Polygon group */
   ctx->Polygon.CullFlag = GL_FALSE;
   ctx->Polygon.CullFaceMode = GL_BACK;
   ctx->Polygon.FrontFace = GL_CCW;
   ctx->Polygon.FrontBit = 0;
   ctx->Polygon.FrontMode = GL_FILL;
   ctx->Polygon.BackMode = GL_FILL;
   ctx->Polygon.Unfilled = GL_FALSE;
   ctx->Polygon.SmoothFlag = GL_FALSE;
   ctx->Polygon.StippleFlag = GL_FALSE;
   ctx->Polygon.OffsetFactor = 0.0F;
   ctx->Polygon.OffsetUnits = 0.0F;
   ctx->Polygon.OffsetPoint = GL_FALSE;
   ctx->Polygon.OffsetLine = GL_FALSE;
   ctx->Polygon.OffsetFill = GL_FALSE;

   /* Polygon Stipple group */
   MEMSET( ctx->PolygonStipple, 0xff, 32*sizeof(GLuint) );

   /* Scissor group */
   ctx->Scissor.Enabled = GL_FALSE;
   ctx->Scissor.X = 0;
   ctx->Scissor.Y = 0;
   ctx->Scissor.Width = 0;
   ctx->Scissor.Height = 0;

   /* Stencil group */
   ctx->Stencil.Enabled = GL_FALSE;
   ctx->Stencil.Function = GL_ALWAYS;
   ctx->Stencil.FailFunc = GL_KEEP;
   ctx->Stencil.ZPassFunc = GL_KEEP;
   ctx->Stencil.ZFailFunc = GL_KEEP;
   ctx->Stencil.Ref = 0;
   ctx->Stencil.ValueMask = STENCIL_MAX;
   ctx->Stencil.Clear = 0;
   ctx->Stencil.WriteMask = STENCIL_MAX;

   /* Texture group */
   ctx->Texture.CurrentUnit = 0;      /* multitexture */
   ctx->Texture.CurrentTransformUnit = 0; /* multitexture */
   ctx->Texture.Enabled = 0;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      init_texture_unit( ctx, i );
   init_color_table(&ctx->Texture.Palette);

   /* Transformation group */
   ctx->Transform.MatrixMode = GL_MODELVIEW;
   ctx->Transform.Normalize = GL_FALSE;
   ctx->Transform.RescaleNormals = GL_FALSE;
   for (i=0;i<MAX_CLIP_PLANES;i++) {
      ctx->Transform.ClipEnabled[i] = GL_FALSE;
      ASSIGN_4V( ctx->Transform.EyeUserPlane[i], 0.0, 0.0, 0.0, 0.0 );
   }
   ctx->Transform.AnyClip = GL_FALSE;

   /* Viewport group */
   ctx->Viewport.X = 0;
   ctx->Viewport.Y = 0;
   ctx->Viewport.Width = 0;
   ctx->Viewport.Height = 0;
   ctx->Viewport.Near = 0.0;
   ctx->Viewport.Far = 1.0;
   gl_matrix_ctr(&ctx->Viewport.WindowMap);

#define Sz 10
#define Tz 14
   ctx->Viewport.WindowMap.m[Sz] = 0.5 * DEPTH_SCALE;
   ctx->Viewport.WindowMap.m[Tz] = 0.5 * DEPTH_SCALE;
#undef Sz
#undef Tz

   ctx->Viewport.WindowMap.flags = MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION;
   ctx->Viewport.WindowMap.type = MATRIX_3D_NO_ROT;

   /* Vertex arrays */
   ctx->Array.Vertex.Size = 4;
   ctx->Array.Vertex.Type = GL_FLOAT;
   ctx->Array.Vertex.Stride = 0;
   ctx->Array.Vertex.StrideB = 0;
   ctx->Array.Vertex.Ptr = NULL;
   ctx->Array.Vertex.Enabled = GL_FALSE;
   ctx->Array.Normal.Type = GL_FLOAT;
   ctx->Array.Normal.Stride = 0;
   ctx->Array.Normal.StrideB = 0;
   ctx->Array.Normal.Ptr = NULL;
   ctx->Array.Normal.Enabled = GL_FALSE;
   ctx->Array.Color.Size = 4;
   ctx->Array.Color.Type = GL_FLOAT;
   ctx->Array.Color.Stride = 0;
   ctx->Array.Color.StrideB = 0;
   ctx->Array.Color.Ptr = NULL;
   ctx->Array.Color.Enabled = GL_FALSE;
   ctx->Array.Index.Type = GL_FLOAT;
   ctx->Array.Index.Stride = 0;
   ctx->Array.Index.StrideB = 0;
   ctx->Array.Index.Ptr = NULL;
   ctx->Array.Index.Enabled = GL_FALSE;
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      ctx->Array.TexCoord[i].Size = 4;
      ctx->Array.TexCoord[i].Type = GL_FLOAT;
      ctx->Array.TexCoord[i].Stride = 0;
      ctx->Array.TexCoord[i].StrideB = 0;
      ctx->Array.TexCoord[i].Ptr = NULL;
      ctx->Array.TexCoord[i].Enabled = GL_FALSE;
   }
   ctx->Array.TexCoordInterleaveFactor = 1;
   ctx->Array.EdgeFlag.Stride = 0;
   ctx->Array.EdgeFlag.StrideB = 0;
   ctx->Array.EdgeFlag.Ptr = NULL;
   ctx->Array.EdgeFlag.Enabled = GL_FALSE;
   ctx->Array.ActiveTexture = 0;   /* GL_ARB_multitexture */

   /* Pixel transfer */
   ctx->Pack.Alignment = 4;
   ctx->Pack.RowLength = 0;
   ctx->Pack.ImageHeight = 0;
   ctx->Pack.SkipPixels = 0;
   ctx->Pack.SkipRows = 0;
   ctx->Pack.SkipImages = 0;
   ctx->Pack.SwapBytes = GL_FALSE;
   ctx->Pack.LsbFirst = GL_FALSE;
   ctx->Unpack.Alignment = 4;
   ctx->Unpack.RowLength = 0;
   ctx->Unpack.ImageHeight = 0;
   ctx->Unpack.SkipPixels = 0;
   ctx->Unpack.SkipRows = 0;
   ctx->Unpack.SkipImages = 0;
   ctx->Unpack.SwapBytes = GL_FALSE;
   ctx->Unpack.LsbFirst = GL_FALSE;

   /* Feedback */
   ctx->Feedback.Type = GL_2D;   /* TODO: verify */
   ctx->Feedback.Buffer = NULL;
   ctx->Feedback.BufferSize = 0;
   ctx->Feedback.Count = 0;

   /* Selection/picking */
   ctx->Select.Buffer = NULL;
   ctx->Select.BufferSize = 0;
   ctx->Select.BufferCount = 0;
   ctx->Select.Hits = 0;
   ctx->Select.NameStackDepth = 0;

   /* Optimized Accum buffer */
   ctx->IntegerAccumMode = GL_TRUE;
   ctx->IntegerAccumScaler = 0.0;

   /* Renderer and client attribute stacks */
   ctx->AttribStackDepth = 0;
   ctx->ClientAttribStackDepth = 0;

   /* Miscellaneous */
   ctx->NewState = NEW_ALL;
   ctx->RenderMode = GL_RENDER;
   ctx->StippleCounter = 0;
   ctx->NeedNormals = GL_FALSE;
   ctx->DoViewportMapping = GL_TRUE;

   ctx->NeedEyeCoords = GL_FALSE;
   ctx->NeedEyeNormals = GL_FALSE;
   ctx->vb_proj_matrix = &ctx->ModelProjectMatrix;

   /* Display list */
   ctx->CallDepth = 0;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   ctx->CurrentListPtr = NULL;
   ctx->CurrentBlock = NULL;
   ctx->CurrentListNum = 0;
   ctx->CurrentPos = 0;

   ctx->ErrorValue = (GLenum) GL_NO_ERROR;

   ctx->CatchSignals = GL_TRUE;

   /* For debug/development only */
   ctx->NoRaster = getenv("MESA_NO_RASTER") ? GL_TRUE : GL_FALSE;
   ctx->FirstTimeCurrent = GL_TRUE;

   /* Dither disable */
   ctx->NoDither = getenv("MESA_NO_DITHER") ? GL_TRUE : GL_FALSE;
   if (ctx->NoDither) {
      if (getenv("MESA_DEBUG")) {
         fprintf(stderr, "MESA_NO_DITHER set - dithering disabled\n");
      }
      ctx->Color.DitherFlag = GL_FALSE;
   }
}




/*
 * Allocate the proxy textures.  If we run out of memory part way through
 * the allocations clean up and return GL_FALSE.
 * Return:  GL_TRUE=success, GL_FALSE=failure
 */
static GLboolean alloc_proxy_textures( GLcontext *ctx )
{
   GLboolean out_of_memory;
   GLint i;

   ctx->Texture.Proxy1D = gl_alloc_texture_object(NULL, 0, 1);
   if (!ctx->Texture.Proxy1D) {
      return GL_FALSE;
   }

   ctx->Texture.Proxy2D = gl_alloc_texture_object(NULL, 0, 2);
   if (!ctx->Texture.Proxy2D) {
      gl_free_texture_object(NULL, ctx->Texture.Proxy1D);
      return GL_FALSE;
   }

   ctx->Texture.Proxy3D = gl_alloc_texture_object(NULL, 0, 3);
   if (!ctx->Texture.Proxy3D) {
      gl_free_texture_object(NULL, ctx->Texture.Proxy1D);
      gl_free_texture_object(NULL, ctx->Texture.Proxy2D);
      return GL_FALSE;
   }

   out_of_memory = GL_FALSE;
   for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
      ctx->Texture.Proxy1D->Image[i] = gl_alloc_texture_image();
      ctx->Texture.Proxy2D->Image[i] = gl_alloc_texture_image();
      ctx->Texture.Proxy3D->Image[i] = gl_alloc_texture_image();
      if (!ctx->Texture.Proxy1D->Image[i]
          || !ctx->Texture.Proxy2D->Image[i]
          || !ctx->Texture.Proxy3D->Image[i]) {
         out_of_memory = GL_TRUE;
      }
   }
   if (out_of_memory) {
      for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
         if (ctx->Texture.Proxy1D->Image[i]) {
            gl_free_texture_image(ctx->Texture.Proxy1D->Image[i]);
         }
         if (ctx->Texture.Proxy2D->Image[i]) {
            gl_free_texture_image(ctx->Texture.Proxy2D->Image[i]);
         }
         if (ctx->Texture.Proxy3D->Image[i]) {
            gl_free_texture_image(ctx->Texture.Proxy3D->Image[i]);
         }
      }
      gl_free_texture_object(NULL, ctx->Texture.Proxy1D);
      gl_free_texture_object(NULL, ctx->Texture.Proxy2D);
      gl_free_texture_object(NULL, ctx->Texture.Proxy3D);
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}



/*
 * Initialize a GLcontext struct.
 */
GLboolean gl_initialize_context_data( GLcontext *ctx,
                                      GLvisual *visual,
                                      GLcontext *share_list,
                                      void *driver_ctx,
                                      GLboolean direct )
{
   (void) direct;  /* not used */

   /* misc one-time initializations */
   one_time_init();

   ctx->DriverCtx = driver_ctx;
   ctx->Visual = visual;
   ctx->DrawBuffer = NULL;
   ctx->ReadBuffer = NULL;

   ctx->VB = gl_vb_create_for_immediate( ctx );
   if (!ctx->VB) {
      FREE( ctx );
      return GL_FALSE;
   }
   ctx->input = ctx->VB->IM;

   ctx->PB = gl_alloc_pb();
   if (!ctx->PB) {
      FREE( ctx->VB );
      FREE( ctx );
      return GL_FALSE;
   }

   if (share_list) {
      /* share the group of display lists of another context */
      ctx->Shared = share_list->Shared;
   }
   else {
      /* allocate new group of display lists */
      ctx->Shared = alloc_shared_state();
      if (!ctx->Shared) {
         FREE(ctx->VB);
         FREE(ctx->PB);
         FREE(ctx);
         return GL_FALSE;
      }
   }
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount++;
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   init_attrib_groups( ctx );

   gl_reset_vb( ctx->VB );
   gl_reset_input( ctx );

   if (visual->DBflag) {
      ctx->Color.DrawBuffer = GL_BACK;
      ctx->Color.DriverDrawBuffer = GL_BACK_LEFT;
      ctx->Color.DrawDestMask = BACK_LEFT_BIT;
      ctx->Pixel.ReadBuffer = GL_BACK;
      ctx->Pixel.DriverReadBuffer = GL_BACK_LEFT;
   }
   else {
      ctx->Color.DrawBuffer = GL_FRONT;
      ctx->Color.DriverDrawBuffer = GL_FRONT_LEFT;
      ctx->Color.DrawDestMask = FRONT_LEFT_BIT;
      ctx->Pixel.ReadBuffer = GL_FRONT;
      ctx->Pixel.DriverReadBuffer = GL_FRONT_LEFT;
   }

#ifdef PROFILE
   init_timings( ctx );
#endif

   if (!alloc_proxy_textures(ctx)) {
      free_shared_state(ctx, ctx->Shared);
      FREE(ctx->VB);
      FREE(ctx->PB);
      FREE(ctx);
      return GL_FALSE;
   }

   /* setup API dispatch tables */
   _mesa_init_exec_table( &ctx->Exec );
   _mesa_init_dlist_table( &ctx->Save );
   ctx->CurrentDispatch = &ctx->Exec;

   return GL_TRUE;
}



/*
 * Allocate and initialize a GLcontext structure.
 * Input:  visual - a GLvisual pointer
 *         sharelist - another context to share display lists with or NULL
 *         driver_ctx - pointer to device driver's context state struct
 * Return:  pointer to a new gl_context struct or NULL if error.
 */
GLcontext *gl_create_context( GLvisual *visual,
                              GLcontext *share_list,
                              void *driver_ctx,
                              GLboolean direct )
{
   GLcontext *ctx = (GLcontext *) CALLOC( sizeof(GLcontext) );
   if (!ctx) {
      return NULL;
   }

   if (gl_initialize_context_data(ctx, visual, share_list,
                                  driver_ctx, direct)) {
      return ctx;
   }
   else {
      FREE(ctx);
      return NULL;
   }
}



/*
 * Free the data associated with the given context.
 * But don't free() the GLcontext struct itself!
 */
void gl_free_context_data( GLcontext *ctx )
{
   GLuint i;
   struct gl_shine_tab *s, *tmps;

   /* if we're destroying the current context, unbind it first */
   if (ctx == gl_get_current_context()) {
      gl_make_current(NULL, NULL);
   }

#ifdef PROFILE
   if (getenv("MESA_PROFILE")) {
      print_timings( ctx );
   }
#endif

   gl_matrix_dtr( &ctx->ModelView );
   for (i = 0 ; i < MAX_MODELVIEW_STACK_DEPTH ; i++) {
      gl_matrix_dtr( &ctx->ModelViewStack[i] );
   }
   gl_matrix_dtr( &ctx->ProjectionMatrix );
   for (i = 0 ; i < MAX_PROJECTION_STACK_DEPTH ; i++) {
      gl_matrix_dtr( &ctx->ProjectionStack[i] );
   }

   FREE( ctx->PB );

   if(ctx->input != ctx->VB->IM)
      gl_immediate_free( ctx->input );

   gl_vb_free( ctx->VB );

   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount--;
   assert(ctx->Shared->RefCount >= 0);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   if (ctx->Shared->RefCount == 0) {
      /* free shared state */
      free_shared_state( ctx, ctx->Shared );
   }

   foreach_s( s, tmps, ctx->ShineTabList ) {
      FREE( s );
   }
   FREE( ctx->ShineTabList );

   /* Free proxy texture objects */
   gl_free_texture_object( NULL, ctx->Texture.Proxy1D );
   gl_free_texture_object( NULL, ctx->Texture.Proxy2D );
   gl_free_texture_object( NULL, ctx->Texture.Proxy3D );

   /* Free evaluator data */
   if (ctx->EvalMap.Map1Vertex3.Points)
      FREE( ctx->EvalMap.Map1Vertex3.Points );
   if (ctx->EvalMap.Map1Vertex4.Points)
      FREE( ctx->EvalMap.Map1Vertex4.Points );
   if (ctx->EvalMap.Map1Index.Points)
      FREE( ctx->EvalMap.Map1Index.Points );
   if (ctx->EvalMap.Map1Color4.Points)
      FREE( ctx->EvalMap.Map1Color4.Points );
   if (ctx->EvalMap.Map1Normal.Points)
      FREE( ctx->EvalMap.Map1Normal.Points );
   if (ctx->EvalMap.Map1Texture1.Points)
      FREE( ctx->EvalMap.Map1Texture1.Points );
   if (ctx->EvalMap.Map1Texture2.Points)
      FREE( ctx->EvalMap.Map1Texture2.Points );
   if (ctx->EvalMap.Map1Texture3.Points)
      FREE( ctx->EvalMap.Map1Texture3.Points );
   if (ctx->EvalMap.Map1Texture4.Points)
      FREE( ctx->EvalMap.Map1Texture4.Points );

   if (ctx->EvalMap.Map2Vertex3.Points)
      FREE( ctx->EvalMap.Map2Vertex3.Points );
   if (ctx->EvalMap.Map2Vertex4.Points)
      FREE( ctx->EvalMap.Map2Vertex4.Points );
   if (ctx->EvalMap.Map2Index.Points)
      FREE( ctx->EvalMap.Map2Index.Points );
   if (ctx->EvalMap.Map2Color4.Points)
      FREE( ctx->EvalMap.Map2Color4.Points );
   if (ctx->EvalMap.Map2Normal.Points)
      FREE( ctx->EvalMap.Map2Normal.Points );
   if (ctx->EvalMap.Map2Texture1.Points)
      FREE( ctx->EvalMap.Map2Texture1.Points );
   if (ctx->EvalMap.Map2Texture2.Points)
      FREE( ctx->EvalMap.Map2Texture2.Points );
   if (ctx->EvalMap.Map2Texture3.Points)
      FREE( ctx->EvalMap.Map2Texture3.Points );
   if (ctx->EvalMap.Map2Texture4.Points)
      FREE( ctx->EvalMap.Map2Texture4.Points );

   /* Free cache of immediate buffers. */
   while (ctx->nr_im_queued-- > 0) {
      struct immediate * next = ctx->freed_im_queue->next;
      FREE( ctx->freed_im_queue );
      ctx->freed_im_queue = next;
   }
   gl_extensions_dtr(ctx);
}



/*
 * Destroy a GLcontext structure.
 */
void gl_destroy_context( GLcontext *ctx )
{
   if (ctx) {
      gl_free_context_data(ctx);
      FREE( (void *) ctx );
   }
}



/*
 * Called by the driver after both the context and driver are fully
 * initialized.  Currently just reads the config file.
 */
void gl_context_initialize( GLcontext *ctx )
{
   gl_read_config_file( ctx );
}



/*
 * Copy attribute groups from one context to another.
 * Input:  src - source context
 *         dst - destination context
 *         mask - bitwise OR of GL_*_BIT flags
 */
void gl_copy_context( const GLcontext *src, GLcontext *dst, GLuint mask )
{
   if (mask & GL_ACCUM_BUFFER_BIT) {
      MEMCPY( &dst->Accum, &src->Accum, sizeof(struct gl_accum_attrib) );
   }
   if (mask & GL_COLOR_BUFFER_BIT) {
      MEMCPY( &dst->Color, &src->Color, sizeof(struct gl_colorbuffer_attrib) );
   }
   if (mask & GL_CURRENT_BIT) {
      MEMCPY( &dst->Current, &src->Current, sizeof(struct gl_current_attrib) );
   }
   if (mask & GL_DEPTH_BUFFER_BIT) {
      MEMCPY( &dst->Depth, &src->Depth, sizeof(struct gl_depthbuffer_attrib) );
   }
   if (mask & GL_ENABLE_BIT) {
      /* no op */
   }
   if (mask & GL_EVAL_BIT) {
      MEMCPY( &dst->Eval, &src->Eval, sizeof(struct gl_eval_attrib) );
   }
   if (mask & GL_FOG_BIT) {
      MEMCPY( &dst->Fog, &src->Fog, sizeof(struct gl_fog_attrib) );
   }
   if (mask & GL_HINT_BIT) {
      MEMCPY( &dst->Hint, &src->Hint, sizeof(struct gl_hint_attrib) );
   }
   if (mask & GL_LIGHTING_BIT) {
      MEMCPY( &dst->Light, &src->Light, sizeof(struct gl_light_attrib) );
      /*       gl_reinit_light_attrib( &dst->Light ); */
   }
   if (mask & GL_LINE_BIT) {
      MEMCPY( &dst->Line, &src->Line, sizeof(struct gl_line_attrib) );
   }
   if (mask & GL_LIST_BIT) {
      MEMCPY( &dst->List, &src->List, sizeof(struct gl_list_attrib) );
   }
   if (mask & GL_PIXEL_MODE_BIT) {
      MEMCPY( &dst->Pixel, &src->Pixel, sizeof(struct gl_pixel_attrib) );
   }
   if (mask & GL_POINT_BIT) {
      MEMCPY( &dst->Point, &src->Point, sizeof(struct gl_point_attrib) );
   }
   if (mask & GL_POLYGON_BIT) {
      MEMCPY( &dst->Polygon, &src->Polygon, sizeof(struct gl_polygon_attrib) );
   }
   if (mask & GL_POLYGON_STIPPLE_BIT) {
      /* Use loop instead of MEMCPY due to problem with Portland Group's
       * C compiler.  Reported by John Stone.
       */
      int i;
      for (i=0;i<32;i++) {
         dst->PolygonStipple[i] = src->PolygonStipple[i];
      }
   }
   if (mask & GL_SCISSOR_BIT) {
      MEMCPY( &dst->Scissor, &src->Scissor, sizeof(struct gl_scissor_attrib) );
   }
   if (mask & GL_STENCIL_BUFFER_BIT) {
      MEMCPY( &dst->Stencil, &src->Stencil, sizeof(struct gl_stencil_attrib) );
   }
   if (mask & GL_TEXTURE_BIT) {
      MEMCPY( &dst->Texture, &src->Texture, sizeof(struct gl_texture_attrib) );
   }
   if (mask & GL_TRANSFORM_BIT) {
      MEMCPY( &dst->Transform, &src->Transform, sizeof(struct gl_transform_attrib) );
   }
   if (mask & GL_VIEWPORT_BIT) {
      MEMCPY( &dst->Viewport, &src->Viewport, sizeof(struct gl_viewport_attrib) );
   }
}


/*
 * Set the current context, binding the given frame buffer to the context.
 */
void gl_make_current( GLcontext *newCtx, GLframebuffer *buffer )
{
   gl_make_current2( newCtx, buffer, buffer );
}


/*
 * Bind the given context to the given draw-buffer and read-buffer
 * and make it the current context for this thread.
 */
void gl_make_current2( GLcontext *newCtx, GLframebuffer *drawBuffer,
                       GLframebuffer *readBuffer )
{
#if 0
   GLcontext *oldCtx = gl_get_context();

   /* Flush the old context
    */
   if (oldCtx) {
      ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(oldCtx, "gl_make_current");

      /* unbind frame buffers from context */
      if (oldCtx->DrawBuffer) {
         oldCtx->DrawBuffer = NULL;
      }
      if (oldCtx->ReadBuffer) {
         oldCtx->ReadBuffer = NULL;
      }
   }
#endif

   /* We call this function periodically (just here for now) in
    * order to detect when multithreading has begun.
    */
   _glapi_check_multithread();

   _glapi_set_context((void *) newCtx);
   ASSERT(gl_get_current_context() == newCtx);
   if (newCtx) {
      SET_IMMEDIATE(newCtx, newCtx->input);
      _glapi_set_dispatch(newCtx->CurrentDispatch);
   }
   else {
      _glapi_set_dispatch(NULL);  /* none current */
   }

   if (MESA_VERBOSE) fprintf(stderr, "gl_make_current()\n");

   if (newCtx && drawBuffer && readBuffer) {
      /* TODO: check if newCtx and buffer's visual match??? */
      newCtx->DrawBuffer = drawBuffer;
      newCtx->ReadBuffer = readBuffer;
      newCtx->NewState = NEW_ALL;   /* just to be safe */
      gl_update_state( newCtx );
   }

   /* We can use this to help debug user's problems.  Tell the to set
    * the MESA_INFO env variable before running their app.  Then the
    * first time each context is made current we'll print some useful
    * information.
    */
   if (newCtx && newCtx->FirstTimeCurrent) {
      if (getenv("MESA_INFO")) {
         fprintf(stderr, "Mesa GL_VERSION = %s\n", (char *) _mesa_GetString(GL_VERSION));
         fprintf(stderr, "Mesa GL_RENDERER = %s\n", (char *) _mesa_GetString(GL_RENDERER));
         fprintf(stderr, "Mesa GL_VENDOR = %s\n", (char *) _mesa_GetString(GL_VENDOR));
         fprintf(stderr, "Mesa GL_EXTENSIONS = %s\n", (char *) _mesa_GetString(GL_EXTENSIONS));
#if defined(THREADS)
         fprintf(stderr, "Mesa thread-safe: YES\n");
#else
         fprintf(stderr, "Mesa thread-safe: NO\n");
#endif
#if defined(USE_X86_ASM)
         fprintf(stderr, "Mesa x86-optimized: YES\n");
#else
         fprintf(stderr, "Mesa x86-optimized: NO\n");
#endif
      }
      newCtx->FirstTimeCurrent = GL_FALSE;
   }
}



/*
 * Return current context handle for the calling thread.
 * This isn't the fastest way to get the current context.
 * If you need speed, see the GET_CURRENT_CONTEXT() macro in context.h
 */
GLcontext *gl_get_current_context( void )
{
   return (GLcontext *) _glapi_get_context();
}



/*
 * This should be called by device drivers just before they do a
 * swapbuffers.  Any pending rendering commands will be executed.
 */
void
_mesa_swapbuffers(GLcontext *ctx)
{
   FLUSH_VB( ctx, "swap buffers" );
}



/*
 * Return pointer to this context's current API dispatch table.
 * It'll either be the immediate-mode execute dispatcher or the
 * display list compile dispatcher.
 */
struct _glapi_table *
_mesa_get_dispatch(GLcontext *ctx)
{
   return ctx->CurrentDispatch;
}



/**********************************************************************/
/*****                Miscellaneous functions                     *****/
/**********************************************************************/


/*
 * This function is called when the Mesa user has stumbled into a code
 * path which may not be implemented fully or correctly.
 */
void gl_problem( const GLcontext *ctx, const char *s )
{
   fprintf( stderr, "Mesa implementation error: %s\n", s );
   fprintf( stderr, "Report to mesa-bugs@mesa3d.org\n" );
   (void) ctx;
}



/*
 * This is called to inform the user that he or she has tried to do
 * something illogical or if there's likely a bug in their program
 * (like enabled depth testing without a depth buffer).
 */
void gl_warning( const GLcontext *ctx, const char *s )
{
   GLboolean debug;
#ifdef DEBUG
   debug = GL_TRUE;
#else
   if (getenv("MESA_DEBUG")) {
      debug = GL_TRUE;
   }
   else {
      debug = GL_FALSE;
   }
#endif
   if (debug) {
      fprintf( stderr, "Mesa warning: %s\n", s );
   }
   (void) ctx;
}



/*
 * Compile an error into current display list.
 */
void gl_compile_error( GLcontext *ctx, GLenum error, const char *s )
{
   if (ctx->CompileFlag)
      gl_save_error( ctx, error, s );

   if (ctx->ExecuteFlag)
      gl_error( ctx, error, s );
}



/*
 * This is Mesa's error handler.  Normally, all that's done is the updating
 * of the current error value.  If Mesa is compiled with -DDEBUG or if the
 * environment variable "MESA_DEBUG" is defined then a real error message
 * is printed to stderr.
 * Input:  error - the error value
 *         s - a diagnostic string
 */
void gl_error( GLcontext *ctx, GLenum error, const char *s )
{
   GLboolean debug;

#ifdef DEBUG
   debug = GL_TRUE;
#else
   if (getenv("MESA_DEBUG")) {
      debug = GL_TRUE;
   }
   else {
      debug = GL_FALSE;
   }
#endif

   if (debug) {
      char errstr[1000];

      switch (error) {
	 case GL_NO_ERROR:
	    strcpy( errstr, "GL_NO_ERROR" );
	    break;
	 case GL_INVALID_VALUE:
	    strcpy( errstr, "GL_INVALID_VALUE" );
	    break;
	 case GL_INVALID_ENUM:
	    strcpy( errstr, "GL_INVALID_ENUM" );
	    break;
	 case GL_INVALID_OPERATION:
	    strcpy( errstr, "GL_INVALID_OPERATION" );
	    break;
	 case GL_STACK_OVERFLOW:
	    strcpy( errstr, "GL_STACK_OVERFLOW" );
	    break;
	 case GL_STACK_UNDERFLOW:
	    strcpy( errstr, "GL_STACK_UNDERFLOW" );
	    break;
	 case GL_OUT_OF_MEMORY:
	    strcpy( errstr, "GL_OUT_OF_MEMORY" );
	    break;
	 default:
	    strcpy( errstr, "unknown" );
	    break;
      }
      fprintf( stderr, "Mesa user error: %s in %s\n", errstr, s );
   }

   if (ctx->ErrorValue==GL_NO_ERROR) {
      ctx->ErrorValue = error;
   }

   /* Call device driver's error handler, if any.  This is used on the Mac. */
   if (ctx->Driver.Error) {
      (*ctx->Driver.Error)( ctx );
   }
}



void
_mesa_Finish( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glFinish");
   if (ctx->Driver.Finish) {
      (*ctx->Driver.Finish)( ctx );
   }
}



void
_mesa_Flush( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glFlush");
   if (ctx->Driver.Flush) {
      (*ctx->Driver.Flush)( ctx );
   }
}
