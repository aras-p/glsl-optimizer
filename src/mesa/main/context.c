/* $Id: context.c,v 1.170 2002/06/17 23:36:31 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
#include "buffers.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "dlist.h"
#include "eval.h"
#include "enums.h"
#include "extensions.h"
#include "fog.h"
#include "get.h"
#include "glthread.h"
#include "hash.h"
#include "light.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "simple_list.h"
#include "state.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#include "vpstate.h"
#include "vtxfmt.h"
#include "math/m_translate.h"
#include "math/m_matrix.h"
#include "math/m_xform.h"
#include "math/mathmod.h"
#endif

#if defined(MESA_TRACE)
#include "Trace/tr_context.h"
#include "Trace/tr_wrapper.h"
#endif

#ifdef USE_SPARC_ASM
#include "SPARC/sparc.h"
#endif

#ifndef MESA_VERBOSE
int MESA_VERBOSE = 0;
#endif

#ifndef MESA_DEBUG_FLAGS
int MESA_DEBUG_FLAGS = 0;
#endif


static void
free_shared_state( GLcontext *ctx, struct gl_shared_state *ss );


/**********************************************************************/
/*****       OpenGL SI-style interface (new in Mesa 3.5)          *****/
/**********************************************************************/

/* Called by window system/device driver (via gc->exports.destroyCurrent())
 * when the rendering context is to be destroyed.
 */
GLboolean
_mesa_destroyContext(__GLcontext *gc)
{
   if (gc) {
      _mesa_free_context_data(gc);
      (*gc->imports.free)(gc, gc);
   }
   return GL_TRUE;
}

/* Called by window system/device driver (via gc->exports.loseCurrent())
 * when the rendering context is made non-current.
 */
GLboolean
_mesa_loseCurrent(__GLcontext *gc)
{
   /* XXX unbind context from thread */
   return GL_TRUE;
}

/* Called by window system/device driver (via gc->exports.makeCurrent())
 * when the rendering context is made current.
 */
GLboolean
_mesa_makeCurrent(__GLcontext *gc)
{
   /* XXX bind context to thread */
   return GL_TRUE;
}

/* Called by window system/device driver - yadda, yadda, yadda.
 * See above comments.
 */
GLboolean
_mesa_shareContext(__GLcontext *gc, __GLcontext *gcShare)
{
   if (gc && gcShare && gc->Shared && gcShare->Shared) {
      gc->Shared->RefCount--;
      if (gc->Shared->RefCount == 0) {
         free_shared_state(gc, gc->Shared);
      }
      gc->Shared = gcShare->Shared;
      gc->Shared->RefCount++;
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

GLboolean
_mesa_copyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask)
{
   if (dst && src) {
      _mesa_copy_context( src, dst, mask );
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

GLboolean
_mesa_forceCurrent(__GLcontext *gc)
{
   return GL_TRUE;
}

GLboolean
_mesa_notifyResize(__GLcontext *gc)
{
   GLint x, y;
   GLuint width, height;
   __GLdrawablePrivate *d = gc->imports.getDrawablePrivate(gc);
   if (!d || !d->getDrawableSize)
      return GL_FALSE;
   d->getDrawableSize( d, &x, &y, &width, &height );
   /* update viewport, resize software buffers, etc. */
   return GL_TRUE;
}

void
_mesa_notifyDestroy(__GLcontext *gc)
{
}

/* Called by window system just before swapping buffers.
 * We have to finish any pending rendering.
 */
void
_mesa_notifySwapBuffers(__GLcontext *gc)
{
   FLUSH_VERTICES( gc, 0 );
}

struct __GLdispatchStateRec *
_mesa_dispatchExec(__GLcontext *gc)
{
   return NULL;
}

void
_mesa_beginDispatchOverride(__GLcontext *gc)
{
}

void
_mesa_endDispatchOverride(__GLcontext *gc)
{
}

/* Setup the exports.  The window system will call these functions
 * when it needs Mesa to do something.
 * NOTE: Device drivers should override these functions!  For example,
 * the Xlib driver should plug in the XMesa*-style functions into this
 * structure.  The XMesa-style functions should then call the _mesa_*
 * version of these functions.  This is an approximation to OO design
 * (inheritance and virtual functions).
 */
static void
_mesa_init_default_exports(__GLexports *exports)
{
    exports->destroyContext = _mesa_destroyContext;
    exports->loseCurrent = _mesa_loseCurrent;
    exports->makeCurrent = _mesa_makeCurrent;
    exports->shareContext = _mesa_shareContext;
    exports->copyContext = _mesa_copyContext;
    exports->forceCurrent = _mesa_forceCurrent;
    exports->notifyResize = _mesa_notifyResize;
    exports->notifyDestroy = _mesa_notifyDestroy;
    exports->notifySwapBuffers = _mesa_notifySwapBuffers;
    exports->dispatchExec = _mesa_dispatchExec;
    exports->beginDispatchOverride = _mesa_beginDispatchOverride;
    exports->endDispatchOverride = _mesa_endDispatchOverride;
}



/* exported OpenGL SI interface */
__GLcontext *
__glCoreCreateContext(__GLimports *imports, __GLcontextModes *modes)
{
    GLcontext *ctx;

    ctx = (GLcontext *) (*imports->calloc)(NULL, 1, sizeof(GLcontext));
    if (ctx == NULL) {
	return NULL;
    }
    ctx->Driver.CurrentExecPrimitive=0;  /* XXX why is this here??? */
    ctx->imports = *imports;
    _mesa_init_default_exports(&(ctx->exports));

    _mesa_initialize_visual(&ctx->Visual,
                            modes->rgbMode,
                            modes->doubleBufferMode,
                            modes->stereoMode,
                            modes->redBits,
                            modes->greenBits,
                            modes->blueBits,
                            modes->alphaBits,
                            modes->indexBits,
                            modes->depthBits,
                            modes->stencilBits,
                            modes->accumRedBits,
                            modes->accumGreenBits,
                            modes->accumBlueBits,
                            modes->accumAlphaBits,
                            0);

    _mesa_initialize_context(ctx, &ctx->Visual, NULL, imports);

    return ctx;
}


/* exported OpenGL SI interface */
void
__glCoreNopDispatch(void)
{
#if 0
   /* SI */
   __gl_dispatch = __glNopDispatchState;
#else
   /* Mesa */
   _glapi_set_dispatch(NULL);
#endif
}


/**********************************************************************/
/***** GL Visual allocation/destruction                           *****/
/**********************************************************************/


/*
 * Allocate a new GLvisual object.
 * Input:  rgbFlag - GL_TRUE=RGB(A) mode, GL_FALSE=Color Index mode
 *         dbFlag - double buffering?
 *         stereoFlag - stereo buffer?
 *         depthBits - requested bits per depth buffer value
 *                     Any value in [0, 32] is acceptable but the actual
 *                     depth type will be GLushort or GLuint as needed.
 *         stencilBits - requested minimum bits per stencil buffer value
 *         accumBits - requested minimum bits per accum buffer component
 *         indexBits - number of bits per pixel if rgbFlag==GL_FALSE
 *         red/green/blue/alphaBits - number of bits per color component
 *                                    in frame buffer for RGB(A) mode.
 *                                    We always use 8 in core Mesa though.
 * Return:  pointer to new GLvisual or NULL if requested parameters can't
 *          be met.
 */
GLvisual *
_mesa_create_visual( GLboolean rgbFlag,
                     GLboolean dbFlag,
                     GLboolean stereoFlag,
                     GLint redBits,
                     GLint greenBits,
                     GLint blueBits,
                     GLint alphaBits,
                     GLint indexBits,
                     GLint depthBits,
                     GLint stencilBits,
                     GLint accumRedBits,
                     GLint accumGreenBits,
                     GLint accumBlueBits,
                     GLint accumAlphaBits,
                     GLint numSamples )
{
   GLvisual *vis = (GLvisual *) CALLOC( sizeof(GLvisual) );
   if (vis) {
      if (!_mesa_initialize_visual(vis, rgbFlag, dbFlag, stereoFlag,
                                   redBits, greenBits, blueBits, alphaBits,
                                   indexBits, depthBits, stencilBits,
                                   accumRedBits, accumGreenBits,
                                   accumBlueBits, accumAlphaBits,
                                   numSamples)) {
         FREE(vis);
         return NULL;
      }
   }
   return vis;
}


/*
 * Initialize the fields of the given GLvisual.
 * Input:  see _mesa_create_visual() above.
 * Return: GL_TRUE = success
 *         GL_FALSE = failure.
 */
GLboolean
_mesa_initialize_visual( GLvisual *vis,
                         GLboolean rgbFlag,
                         GLboolean dbFlag,
                         GLboolean stereoFlag,
                         GLint redBits,
                         GLint greenBits,
                         GLint blueBits,
                         GLint alphaBits,
                         GLint indexBits,
                         GLint depthBits,
                         GLint stencilBits,
                         GLint accumRedBits,
                         GLint accumGreenBits,
                         GLint accumBlueBits,
                         GLint accumAlphaBits,
                         GLint numSamples )
{
   (void) numSamples;

   assert(vis);

   /* This is to catch bad values from device drivers not updated for
    * Mesa 3.3.  Some device drivers just passed 1.  That's a REALLY
    * bad value now (a 1-bit depth buffer!?!).
    */
   assert(depthBits == 0 || depthBits > 1);

   if (depthBits < 0 || depthBits > 32) {
      return GL_FALSE;
   }
   if (stencilBits < 0 || stencilBits > (GLint) (8 * sizeof(GLstencil))) {
      return GL_FALSE;
   }
   if (accumRedBits < 0 || accumRedBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumGreenBits < 0 || accumGreenBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumBlueBits < 0 || accumBlueBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }
   if (accumAlphaBits < 0 || accumAlphaBits > (GLint) (8 * sizeof(GLaccum))) {
      return GL_FALSE;
   }

   vis->rgbMode          = rgbFlag;
   vis->doubleBufferMode = dbFlag;
   vis->stereoMode       = stereoFlag;
   vis->redBits          = redBits;
   vis->greenBits        = greenBits;
   vis->blueBits         = blueBits;
   vis->alphaBits        = alphaBits;

   vis->indexBits      = indexBits;
   vis->depthBits      = depthBits;
   vis->accumRedBits   = (accumRedBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumGreenBits = (accumGreenBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumBlueBits  = (accumBlueBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->accumAlphaBits = (accumAlphaBits > 0) ? (8 * sizeof(GLaccum)) : 0;
   vis->stencilBits    = (stencilBits > 0) ? (8 * sizeof(GLstencil)) : 0;

   return GL_TRUE;
}


void
_mesa_destroy_visual( GLvisual *vis )
{
   FREE(vis);
}


/**********************************************************************/
/***** GL Framebuffer allocation/destruction                      *****/
/**********************************************************************/


/*
 * Create a new framebuffer.  A GLframebuffer is a struct which
 * encapsulates the depth, stencil and accum buffers and related
 * parameters.
 * Input:  visual - a GLvisual pointer (we copy the struct contents)
 *         softwareDepth - create/use a software depth buffer?
 *         softwareStencil - create/use a software stencil buffer?
 *         softwareAccum - create/use a software accum buffer?
 *         softwareAlpha - create/use a software alpha buffer?
 * Return:  pointer to new GLframebuffer struct or NULL if error.
 */
GLframebuffer *
_mesa_create_framebuffer( const GLvisual *visual,
                          GLboolean softwareDepth,
                          GLboolean softwareStencil,
                          GLboolean softwareAccum,
                          GLboolean softwareAlpha )
{
   GLframebuffer *buffer = CALLOC_STRUCT(gl_frame_buffer);
   assert(visual);
   if (buffer) {
      _mesa_initialize_framebuffer(buffer, visual,
                                   softwareDepth, softwareStencil,
                                   softwareAccum, softwareAlpha );
   }
   return buffer;
}


/*
 * Initialize a GLframebuffer object.
 * Input:  See _mesa_create_framebuffer() above.
 */
void
_mesa_initialize_framebuffer( GLframebuffer *buffer,
                              const GLvisual *visual,
                              GLboolean softwareDepth,
                              GLboolean softwareStencil,
                              GLboolean softwareAccum,
                              GLboolean softwareAlpha )
{
   assert(buffer);
   assert(visual);

   BZERO(buffer, sizeof(GLframebuffer));

   /* sanity checks */
   if (softwareDepth ) {
      assert(visual->depthBits > 0);
   }
   if (softwareStencil) {
      assert(visual->stencilBits > 0);
   }
   if (softwareAccum) {
      assert(visual->rgbMode);
      assert(visual->accumRedBits > 0);
      assert(visual->accumGreenBits > 0);
      assert(visual->accumBlueBits > 0);
   }
   if (softwareAlpha) {
      assert(visual->rgbMode);
      assert(visual->alphaBits > 0);
   }

   buffer->Visual = *visual;
   buffer->UseSoftwareDepthBuffer = softwareDepth;
   buffer->UseSoftwareStencilBuffer = softwareStencil;
   buffer->UseSoftwareAccumBuffer = softwareAccum;
   buffer->UseSoftwareAlphaBuffers = softwareAlpha;
}


/*
 * Free a framebuffer struct and its buffers.
 */
void
_mesa_destroy_framebuffer( GLframebuffer *buffer )
{
   if (buffer) {
      _mesa_free_framebuffer_data(buffer);
      FREE(buffer);
   }
}


/*
 * Free the data hanging off of <buffer>, but not <buffer> itself.
 */
void
_mesa_free_framebuffer_data( GLframebuffer *buffer )
{
   if (!buffer)
      return;

   if (buffer->DepthBuffer) {
      MESA_PBUFFER_FREE( buffer->DepthBuffer );
      buffer->DepthBuffer = NULL;
   }
   if (buffer->Accum) {
      MESA_PBUFFER_FREE( buffer->Accum );
      buffer->Accum = NULL;
   }
   if (buffer->Stencil) {
      MESA_PBUFFER_FREE( buffer->Stencil );
      buffer->Stencil = NULL;
   }
   if (buffer->FrontLeftAlpha) {
      MESA_PBUFFER_FREE( buffer->FrontLeftAlpha );
      buffer->FrontLeftAlpha = NULL;
   }
   if (buffer->BackLeftAlpha) {
      MESA_PBUFFER_FREE( buffer->BackLeftAlpha );
      buffer->BackLeftAlpha = NULL;
   }
   if (buffer->FrontRightAlpha) {
      MESA_PBUFFER_FREE( buffer->FrontRightAlpha );
      buffer->FrontRightAlpha = NULL;
   }
   if (buffer->BackRightAlpha) {
      MESA_PBUFFER_FREE( buffer->BackRightAlpha );
      buffer->BackRightAlpha = NULL;
   }
}



/**********************************************************************/
/*****       Context allocation, initialization, destroying       *****/
/**********************************************************************/


_glthread_DECLARE_STATIC_MUTEX(OneTimeLock);


/*
 * This function just calls all the various one-time-init functions in Mesa.
 */
static void
one_time_init( GLcontext *ctx )
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

      _mesa_init_lists();

      _math_init();
      _mesa_init_math();

#ifdef USE_SPARC_ASM
      _mesa_init_sparc_glapi_relocs();
#endif
      if (ctx->imports.getenv(ctx, "MESA_DEBUG")) {
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


static void
init_matrix_stack( struct matrix_stack *stack,
                   GLuint maxDepth, GLuint dirtyFlag )
{
   GLuint i;

   stack->Depth = 0;
   stack->MaxDepth = maxDepth;
   stack->DirtyFlag = dirtyFlag;
   /* The stack */
   stack->Stack = (GLmatrix *) CALLOC(maxDepth * sizeof(GLmatrix));
   for (i = 0; i < maxDepth; i++) {
      _math_matrix_ctr(&stack->Stack[i]);
      _math_matrix_alloc_inv(&stack->Stack[i]);
   }
   stack->Top = stack->Stack;
}


static void
free_matrix_stack( struct matrix_stack *stack )
{
   GLuint i;
   for (i = 0; i < stack->MaxDepth; i++) {
      _math_matrix_dtr(&stack->Stack[i]);
   }
   FREE(stack->Stack);
   stack->Stack = stack->Top = NULL;
}


/*
 * Allocate and initialize a shared context state structure.
 */
static struct gl_shared_state *
alloc_shared_state( void )
{
   struct gl_shared_state *ss;
   GLboolean outOfMemory;

   ss = CALLOC_STRUCT(gl_shared_state);
   if (!ss)
      return NULL;

   _glthread_INIT_MUTEX(ss->Mutex);

   ss->DisplayList = _mesa_NewHashTable();
   ss->TexObjects = _mesa_NewHashTable();
   ss->VertexPrograms = _mesa_NewHashTable();

   /* Default Texture objects */
   outOfMemory = GL_FALSE;

   ss->Default1D = _mesa_alloc_texture_object(ss, 0, GL_TEXTURE_1D);
   if (!ss->Default1D) {
      outOfMemory = GL_TRUE;
   }

   ss->Default2D = _mesa_alloc_texture_object(ss, 0, GL_TEXTURE_2D);
   if (!ss->Default2D) {
      outOfMemory = GL_TRUE;
   }

   ss->Default3D = _mesa_alloc_texture_object(ss, 0, GL_TEXTURE_3D);
   if (!ss->Default3D) {
      outOfMemory = GL_TRUE;
   }

   ss->DefaultCubeMap = _mesa_alloc_texture_object(ss, 0,
                                                   GL_TEXTURE_CUBE_MAP_ARB);
   if (!ss->DefaultCubeMap) {
      outOfMemory = GL_TRUE;
   }

   ss->DefaultRect = _mesa_alloc_texture_object(ss, 0,
                                                GL_TEXTURE_RECTANGLE_NV);
   if (!ss->DefaultRect) {
      outOfMemory = GL_TRUE;
   }

   if (!ss->DisplayList || !ss->TexObjects || !ss->VertexPrograms
       || outOfMemory) {
      /* Ran out of memory at some point.  Free everything and return NULL */
      if (ss->DisplayList)
         _mesa_DeleteHashTable(ss->DisplayList);
      if (ss->TexObjects)
         _mesa_DeleteHashTable(ss->TexObjects);
      if (ss->VertexPrograms)
         _mesa_DeleteHashTable(ss->VertexPrograms);
      if (ss->Default1D)
         _mesa_free_texture_object(ss, ss->Default1D);
      if (ss->Default2D)
         _mesa_free_texture_object(ss, ss->Default2D);
      if (ss->Default3D)
         _mesa_free_texture_object(ss, ss->Default3D);
      if (ss->DefaultCubeMap)
         _mesa_free_texture_object(ss, ss->DefaultCubeMap);
      if (ss->DefaultRect)
         _mesa_free_texture_object(ss, ss->DefaultRect);
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
static void
free_shared_state( GLcontext *ctx, struct gl_shared_state *ss )
{
   /* Free display lists */
   while (1) {
      GLuint list = _mesa_HashFirstEntry(ss->DisplayList);
      if (list) {
         _mesa_destroy_list(ctx, list);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->DisplayList);

   /* Free texture objects */
   while (ss->TexObjectList) {
      if (ctx->Driver.DeleteTexture)
         (*ctx->Driver.DeleteTexture)( ctx, ss->TexObjectList );
      /* this function removes from linked list too! */
      _mesa_free_texture_object(ss, ss->TexObjectList);
   }
   _mesa_DeleteHashTable(ss->TexObjects);

   /* Free vertex programs */
   while (1) {
      GLuint prog = _mesa_HashFirstEntry(ss->VertexPrograms);
      if (prog) {
         _mesa_delete_program(ctx, prog);
      }
      else {
         break;
      }
   }
   _mesa_DeleteHashTable(ss->VertexPrograms);

   FREE(ss);
}



/*
 * Initialize the nth light.  Note that the defaults for light 0 are
 * different than the other lights.
 */
static void
init_light( struct gl_light *l, GLuint n )
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
   _mesa_invalidate_spot_exp_table( l );
   l->SpotCutoff = 180.0;
   l->_CosCutoff = 0.0;		/* KW: -ve values not admitted */
   l->ConstantAttenuation = 1.0;
   l->LinearAttenuation = 0.0;
   l->QuadraticAttenuation = 0.0;
   l->Enabled = GL_FALSE;
}



static void
init_lightmodel( struct gl_lightmodel *lm )
{
   ASSIGN_4V( lm->Ambient, 0.2F, 0.2F, 0.2F, 1.0F );
   lm->LocalViewer = GL_FALSE;
   lm->TwoSide = GL_FALSE;
   lm->ColorControl = GL_SINGLE_COLOR;
}


static void
init_material( struct gl_material *m )
{
   ASSIGN_4V( m->Ambient,  0.2F, 0.2F, 0.2F, 1.0F );
   ASSIGN_4V( m->Diffuse,  0.8F, 0.8F, 0.8F, 1.0F );
   ASSIGN_4V( m->Specular, 0.0F, 0.0F, 0.0F, 1.0F );
   ASSIGN_4V( m->Emission, 0.0F, 0.0F, 0.0F, 1.0F );
   m->Shininess = 0.0;
   m->AmbientIndex = 0;
   m->DiffuseIndex = 1;
   m->SpecularIndex = 1;
}



static void
init_texture_unit( GLcontext *ctx, GLuint unit )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];

   texUnit->EnvMode = GL_MODULATE;
   texUnit->CombineModeRGB = GL_MODULATE;
   texUnit->CombineModeA = GL_MODULATE;
   texUnit->CombineSourceRGB[0] = GL_TEXTURE;
   texUnit->CombineSourceRGB[1] = GL_PREVIOUS_EXT;
   texUnit->CombineSourceRGB[2] = GL_CONSTANT_EXT;
   texUnit->CombineSourceA[0] = GL_TEXTURE;
   texUnit->CombineSourceA[1] = GL_PREVIOUS_EXT;
   texUnit->CombineSourceA[2] = GL_CONSTANT_EXT;
   texUnit->CombineOperandRGB[0] = GL_SRC_COLOR;
   texUnit->CombineOperandRGB[1] = GL_SRC_COLOR;
   texUnit->CombineOperandRGB[2] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[0] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[1] = GL_SRC_ALPHA;
   texUnit->CombineOperandA[2] = GL_SRC_ALPHA;
   texUnit->CombineScaleShiftRGB = 0;
   texUnit->CombineScaleShiftA = 0;

   ASSIGN_4V( texUnit->EnvColor, 0.0, 0.0, 0.0, 0.0 );
   texUnit->TexGenEnabled = 0;
   texUnit->GenModeS = GL_EYE_LINEAR;
   texUnit->GenModeT = GL_EYE_LINEAR;
   texUnit->GenModeR = GL_EYE_LINEAR;
   texUnit->GenModeQ = GL_EYE_LINEAR;
   texUnit->_GenBitS = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitT = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitR = TEXGEN_EYE_LINEAR;
   texUnit->_GenBitQ = TEXGEN_EYE_LINEAR;

   /* Yes, these plane coefficients are correct! */
   ASSIGN_4V( texUnit->ObjectPlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->ObjectPlaneQ, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneS, 1.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneT, 0.0, 1.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneR, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( texUnit->EyePlaneQ, 0.0, 0.0, 0.0, 0.0 );

   texUnit->Current1D = ctx->Shared->Default1D;
   texUnit->Current2D = ctx->Shared->Default2D;
   texUnit->Current3D = ctx->Shared->Default3D;
   texUnit->CurrentCubeMap = ctx->Shared->DefaultCubeMap;
   texUnit->CurrentRect = ctx->Shared->DefaultRect;
}




/* Initialize a 1-D evaluator map */
static void
init_1d_map( struct gl_1d_map *map, int n, const float *initial )
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
static void
init_2d_map( struct gl_2d_map *map, int n, const float *initial )
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


/*
 * Initialize the attribute groups in a GLcontext.
 */
static void
init_attrib_groups( GLcontext *ctx )
{
   GLuint i;

   assert(ctx);

   assert(MAX_TEXTURE_LEVELS >= MAX_3D_TEXTURE_LEVELS);
   assert(MAX_TEXTURE_LEVELS >= MAX_CUBE_TEXTURE_LEVELS);

   /* Constants, may be overriden by device drivers */
   ctx->Const.MaxTextureLevels = MAX_TEXTURE_LEVELS;
   ctx->Const.Max3DTextureLevels = MAX_3D_TEXTURE_LEVELS;
   ctx->Const.MaxCubeTextureLevels = MAX_CUBE_TEXTURE_LEVELS;
   ctx->Const.MaxTextureRectSize = MAX_TEXTURE_RECT_SIZE;
   ctx->Const.MaxTextureUnits = MAX_TEXTURE_UNITS;
   ctx->Const.MaxTextureMaxAnisotropy = MAX_TEXTURE_MAX_ANISOTROPY;
   ctx->Const.MaxTextureLodBias = MAX_TEXTURE_LOD_BIAS;
   ctx->Const.MaxArrayLockSize = MAX_ARRAY_LOCK_SIZE;
   ctx->Const.SubPixelBits = SUB_PIXEL_BITS;
   ctx->Const.MinPointSize = MIN_POINT_SIZE;
   ctx->Const.MaxPointSize = MAX_POINT_SIZE;
   ctx->Const.MinPointSizeAA = MIN_POINT_SIZE;
   ctx->Const.MaxPointSizeAA = MAX_POINT_SIZE;
   ctx->Const.PointSizeGranularity = (GLfloat) POINT_SIZE_GRANULARITY;
   ctx->Const.MinLineWidth = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidth = MAX_LINE_WIDTH;
   ctx->Const.MinLineWidthAA = MIN_LINE_WIDTH;
   ctx->Const.MaxLineWidthAA = MAX_LINE_WIDTH;
   ctx->Const.LineWidthGranularity = (GLfloat) LINE_WIDTH_GRANULARITY;
   ctx->Const.NumAuxBuffers = NUM_AUX_BUFFERS;
   ctx->Const.MaxColorTableSize = MAX_COLOR_TABLE_SIZE;
   ctx->Const.MaxConvolutionWidth = MAX_CONVOLUTION_WIDTH;
   ctx->Const.MaxConvolutionHeight = MAX_CONVOLUTION_HEIGHT;
   ctx->Const.NumCompressedTextureFormats = 0;
   ctx->Const.MaxClipPlanes = MAX_CLIP_PLANES;
   ctx->Const.MaxLights = MAX_LIGHTS;

   /* Initialize matrix stacks */
   init_matrix_stack(&ctx->ModelviewMatrixStack, MAX_MODELVIEW_STACK_DEPTH,
                     _NEW_MODELVIEW);
   init_matrix_stack(&ctx->ProjectionMatrixStack, MAX_PROJECTION_STACK_DEPTH,
                     _NEW_PROJECTION);
   init_matrix_stack(&ctx->ColorMatrixStack, MAX_COLOR_STACK_DEPTH,
                     _NEW_COLOR_MATRIX);
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      init_matrix_stack(&ctx->TextureMatrixStack[i], MAX_TEXTURE_STACK_DEPTH,
                        _NEW_TEXTURE_MATRIX);
   for (i = 0; i < MAX_PROGRAM_MATRICES; i++)
      init_matrix_stack(&ctx->ProgramMatrixStack[i], MAX_PROGRAM_STACK_DEPTH,
                        _NEW_TRACK_MATRIX);
   ctx->CurrentStack = &ctx->ModelviewMatrixStack;

   /* Init combined Modelview*Projection matrix */
   _math_matrix_ctr( &ctx->_ModelProjectMatrix );

   /* Accumulate buffer group */
   ASSIGN_4V( ctx->Accum.ClearColor, 0.0, 0.0, 0.0, 0.0 );

   /* Color buffer group */
   ctx->Color.IndexMask = 0xffffffff;
   ctx->Color.ColorMask[0] = 0xff;
   ctx->Color.ColorMask[1] = 0xff;
   ctx->Color.ColorMask[2] = 0xff;
   ctx->Color.ColorMask[3] = 0xff;
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor, 0, 0, 0, 0 );
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
   ASSIGN_4V( ctx->Color.BlendColor, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color.DitherFlag = GL_TRUE;
   ctx->Color.MultiDrawBuffer = GL_FALSE;

   /* Current group */
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_WEIGHT], 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_NORMAL], 0.0, 0.0, 1.0, 0.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR0], 1.0, 1.0, 1.0, 1.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_COLOR1], 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_FOG], 0.0, 0.0, 0.0, 0.0 );
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.Attrib[VERT_ATTRIB_TEX0 + i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.Index = 1;
   ctx->Current.EdgeFlag = GL_TRUE;
   
   ASSIGN_4V( ctx->Current.RasterPos, 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterDistance = 0.0;
   ASSIGN_4V( ctx->Current.RasterColor, 1.0, 1.0, 1.0, 1.0 );
   ctx->Current.RasterIndex = 1;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      ASSIGN_4V( ctx->Current.RasterTexCoords[i], 0.0, 0.0, 0.0, 1.0 );
   ctx->Current.RasterPosValid = GL_TRUE;


   /* Depth buffer group */
   ctx->Depth.Test = GL_FALSE;
   ctx->Depth.Clear = 1.0;
   ctx->Depth.Func = GL_LESS;
   ctx->Depth.Mask = GL_TRUE;
   ctx->Depth.OcclusionTest = GL_FALSE;

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
   MEMSET(ctx->Eval.Map1Attrib, 0, sizeof(ctx->Eval.Map1Attrib));
   ctx->Eval.Map2Color4 = GL_FALSE;
   ctx->Eval.Map2Index = GL_FALSE;
   ctx->Eval.Map2Normal = GL_FALSE;
   ctx->Eval.Map2TextureCoord1 = GL_FALSE;
   ctx->Eval.Map2TextureCoord2 = GL_FALSE;
   ctx->Eval.Map2TextureCoord3 = GL_FALSE;
   ctx->Eval.Map2TextureCoord4 = GL_FALSE;
   ctx->Eval.Map2Vertex3 = GL_FALSE;
   ctx->Eval.Map2Vertex4 = GL_FALSE;
   MEMSET(ctx->Eval.Map2Attrib, 0, sizeof(ctx->Eval.Map2Attrib));
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
      static GLfloat attrib[4] = { 0.0, 0.0, 0.0, 1.0 };

      init_1d_map( &ctx->EvalMap.Map1Vertex3, 3, vertex );
      init_1d_map( &ctx->EvalMap.Map1Vertex4, 4, vertex );
      init_1d_map( &ctx->EvalMap.Map1Index, 1, index );
      init_1d_map( &ctx->EvalMap.Map1Color4, 4, color );
      init_1d_map( &ctx->EvalMap.Map1Normal, 3, normal );
      init_1d_map( &ctx->EvalMap.Map1Texture1, 1, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture2, 2, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture3, 3, texcoord );
      init_1d_map( &ctx->EvalMap.Map1Texture4, 4, texcoord );
      for (i = 0; i < 16; i++)
         init_1d_map( ctx->EvalMap.Map1Attrib + i, 4, attrib );

      init_2d_map( &ctx->EvalMap.Map2Vertex3, 3, vertex );
      init_2d_map( &ctx->EvalMap.Map2Vertex4, 4, vertex );
      init_2d_map( &ctx->EvalMap.Map2Index, 1, index );
      init_2d_map( &ctx->EvalMap.Map2Color4, 4, color );
      init_2d_map( &ctx->EvalMap.Map2Normal, 3, normal );
      init_2d_map( &ctx->EvalMap.Map2Texture1, 1, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture2, 2, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture3, 3, texcoord );
      init_2d_map( &ctx->EvalMap.Map2Texture4, 4, texcoord );
      for (i = 0; i < 16; i++)
         init_2d_map( ctx->EvalMap.Map2Attrib + i, 4, attrib );
   }

   /* Fog group */
   ctx->Fog.Enabled = GL_FALSE;
   ctx->Fog.Mode = GL_EXP;
   ASSIGN_4V( ctx->Fog.Color, 0.0, 0.0, 0.0, 0.0 );
   ctx->Fog.Index = 0.0;
   ctx->Fog.Density = 1.0;
   ctx->Fog.Start = 0.0;
   ctx->Fog.End = 1.0;
   ctx->Fog.ColorSumEnabled = GL_FALSE;
   ctx->Fog.FogCoordinateSource = GL_FRAGMENT_DEPTH_EXT;

   /* Hint group */
   ctx->Hint.PerspectiveCorrection = GL_DONT_CARE;
   ctx->Hint.PointSmooth = GL_DONT_CARE;
   ctx->Hint.LineSmooth = GL_DONT_CARE;
   ctx->Hint.PolygonSmooth = GL_DONT_CARE;
   ctx->Hint.Fog = GL_DONT_CARE;
   ctx->Hint.ClipVolumeClipping = GL_DONT_CARE;
   ctx->Hint.TextureCompression = GL_DONT_CARE;
   ctx->Hint.GenerateMipmap = GL_DONT_CARE;

   /* Histogram group */
   ctx->Histogram.Width = 0;
   ctx->Histogram.Format = GL_RGBA;
   ctx->Histogram.Sink = GL_FALSE;
   ctx->Histogram.RedSize       = 0;
   ctx->Histogram.GreenSize     = 0;
   ctx->Histogram.BlueSize      = 0;
   ctx->Histogram.AlphaSize     = 0;
   ctx->Histogram.LuminanceSize = 0;
   for (i = 0; i < HISTOGRAM_TABLE_SIZE; i++) {
      ctx->Histogram.Count[i][0] = 0;
      ctx->Histogram.Count[i][1] = 0;
      ctx->Histogram.Count[i][2] = 0;
      ctx->Histogram.Count[i][3] = 0;
   }

   /* Min/Max group */
   ctx->MinMax.Format = GL_RGBA;
   ctx->MinMax.Sink = GL_FALSE;
   ctx->MinMax.Min[RCOMP] = 1000;    ctx->MinMax.Max[RCOMP] = -1000;
   ctx->MinMax.Min[GCOMP] = 1000;    ctx->MinMax.Max[GCOMP] = -1000;
   ctx->MinMax.Min[BCOMP] = 1000;    ctx->MinMax.Max[BCOMP] = -1000;
   ctx->MinMax.Min[ACOMP] = 1000;    ctx->MinMax.Max[ACOMP] = -1000;

   /* Extensions */
   _mesa_extensions_ctr( ctx );

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
   ctx->Light.ColorMaterialBitmask = _mesa_material_bitmask( ctx,
                                               GL_FRONT_AND_BACK,
                                               GL_AMBIENT_AND_DIFFUSE, ~0, 0 );

   ctx->Light.ColorMaterialEnabled = GL_FALSE;

   /* Lighting miscellaneous */
   ctx->_ShineTabList = MALLOC_STRUCT( gl_shine_tab );
   make_empty_list( ctx->_ShineTabList );
   for (i = 0 ; i < 10 ; i++) {
      struct gl_shine_tab *s = MALLOC_STRUCT( gl_shine_tab );
      s->shininess = -1;
      s->refcount = 0;
      insert_at_tail( ctx->_ShineTabList, s );
   }


   /* Line group */
   ctx->Line.SmoothFlag = GL_FALSE;
   ctx->Line.StippleFlag = GL_FALSE;
   ctx->Line.Width = 1.0;
   ctx->Line._Width = 1.0;
   ctx->Line.StipplePattern = 0xffff;
   ctx->Line.StippleFactor = 1;

   /* Display List group */
   ctx->List.ListBase = 0;

   /* Multisample */
   ctx->Multisample.Enabled = GL_FALSE;
   ctx->Multisample.SampleAlphaToCoverage = GL_FALSE;
   ctx->Multisample.SampleAlphaToOne = GL_FALSE;
   ctx->Multisample.SampleCoverage = GL_FALSE;
   ctx->Multisample.SampleCoverageValue = 1.0;
   ctx->Multisample.SampleCoverageInvert = GL_FALSE;

   /* Pixel group */
   ctx->Pixel.RedBias = 0.0;
   ctx->Pixel.RedScale = 1.0;
   ctx->Pixel.GreenBias = 0.0;
   ctx->Pixel.GreenScale = 1.0;
   ctx->Pixel.BlueBias = 0.0;
   ctx->Pixel.BlueScale = 1.0;
   ctx->Pixel.AlphaBias = 0.0;
   ctx->Pixel.AlphaScale = 1.0;
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
   ctx->Pixel.HistogramEnabled = GL_FALSE;
   ctx->Pixel.MinMaxEnabled = GL_FALSE;
   ctx->Pixel.PixelTextureEnabled = GL_FALSE;
   ctx->Pixel.FragmentRgbSource = GL_PIXEL_GROUP_COLOR_SGIS;
   ctx->Pixel.FragmentAlphaSource = GL_PIXEL_GROUP_COLOR_SGIS;
   ASSIGN_4V(ctx->Pixel.PostColorMatrixScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostColorMatrixBias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.ColorTableScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.ColorTableBias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.PCCTscale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PCCTbias, 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(ctx->Pixel.PCMCTscale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PCMCTbias, 0.0, 0.0, 0.0, 0.0);
   ctx->Pixel.ColorTableEnabled = GL_FALSE;
   ctx->Pixel.PostConvolutionColorTableEnabled = GL_FALSE;
   ctx->Pixel.PostColorMatrixColorTableEnabled = GL_FALSE;
   ctx->Pixel.Convolution1DEnabled = GL_FALSE;
   ctx->Pixel.Convolution2DEnabled = GL_FALSE;
   ctx->Pixel.Separable2DEnabled = GL_FALSE;
   for (i = 0; i < 3; i++) {
      ASSIGN_4V(ctx->Pixel.ConvolutionBorderColor[i], 0.0, 0.0, 0.0, 0.0);
      ctx->Pixel.ConvolutionBorderMode[i] = GL_REDUCE;
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterScale[i], 1.0, 1.0, 1.0, 1.0);
      ASSIGN_4V(ctx->Pixel.ConvolutionFilterBias[i], 0.0, 0.0, 0.0, 0.0);
   }
   for (i = 0; i < MAX_CONVOLUTION_WIDTH * MAX_CONVOLUTION_WIDTH * 4; i++) {
      ctx->Convolution1D.Filter[i] = 0.0;
      ctx->Convolution2D.Filter[i] = 0.0;
      ctx->Separable2D.Filter[i] = 0.0;
   }
   ASSIGN_4V(ctx->Pixel.PostConvolutionScale, 1.0, 1.0, 1.0, 1.0);
   ASSIGN_4V(ctx->Pixel.PostConvolutionBias, 0.0, 0.0, 0.0, 0.0);

   /* Point group */
   ctx->Point.SmoothFlag = GL_FALSE;
   ctx->Point.Size = 1.0;
   ctx->Point._Size = 1.0;
   ctx->Point.Params[0] = 1.0;
   ctx->Point.Params[1] = 0.0;
   ctx->Point.Params[2] = 0.0;
   ctx->Point._Attenuated = GL_FALSE;
   ctx->Point.MinSize = 0.0;
   ctx->Point.MaxSize = ctx->Const.MaxPointSize;
   ctx->Point.Threshold = 1.0;
   ctx->Point.PointSprite = GL_FALSE; /* GL_NV_point_sprite */
   ctx->Point.SpriteRMode = GL_ZERO; /* GL_NV_point_sprite */
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      ctx->Point.CoordReplace[i] = GL_FALSE; /* GL_NV_point_sprite */
   }

   /* Polygon group */
   ctx->Polygon.CullFlag = GL_FALSE;
   ctx->Polygon.CullFaceMode = GL_BACK;
   ctx->Polygon.FrontFace = GL_CCW;
   ctx->Polygon._FrontBit = 0;
   ctx->Polygon.FrontMode = GL_FILL;
   ctx->Polygon.BackMode = GL_FILL;
   ctx->Polygon.SmoothFlag = GL_FALSE;
   ctx->Polygon.StippleFlag = GL_FALSE;
   ctx->Polygon.OffsetFactor = 0.0F;
   ctx->Polygon.OffsetUnits = 0.0F;
   ctx->Polygon.OffsetMRD = 0.0F;
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
   ctx->Texture._ReallyEnabled = 0;   /* XXX obsolete */
   ctx->Texture._EnabledUnits = 0;
   for (i=0; i<MAX_TEXTURE_UNITS; i++)
      init_texture_unit( ctx, i );
   ctx->Texture.SharedPalette = GL_FALSE;
   _mesa_init_colortable(&ctx->Texture.Palette);

   /* Transformation group */
   ctx->Transform.MatrixMode = GL_MODELVIEW;
   ctx->Transform.Normalize = GL_FALSE;
   ctx->Transform.RescaleNormals = GL_FALSE;
   ctx->Transform.RasterPositionUnclipped = GL_FALSE;
   for (i=0;i<MAX_CLIP_PLANES;i++) {
      ASSIGN_4V( ctx->Transform.EyeUserPlane[i], 0.0, 0.0, 0.0, 0.0 );
   }
   ctx->Transform.ClipPlanesEnabled = 0;

   /* Viewport group */
   ctx->Viewport.X = 0;
   ctx->Viewport.Y = 0;
   ctx->Viewport.Width = 0;
   ctx->Viewport.Height = 0;
   ctx->Viewport.Near = 0.0;
   ctx->Viewport.Far = 1.0;
   _math_matrix_ctr(&ctx->Viewport._WindowMap);

#define Sz 10
#define Tz 14
   ctx->Viewport._WindowMap.m[Sz] = 0.5F * ctx->DepthMaxF;
   ctx->Viewport._WindowMap.m[Tz] = 0.5F * ctx->DepthMaxF;
#undef Sz
#undef Tz

   ctx->Viewport._WindowMap.flags = MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION;
   ctx->Viewport._WindowMap.type = MATRIX_3D_NO_ROT;

   /* Vertex arrays */
   ctx->Array.Vertex.Size = 4;
   ctx->Array.Vertex.Type = GL_FLOAT;
   ctx->Array.Vertex.Stride = 0;
   ctx->Array.Vertex.StrideB = 0;
   ctx->Array.Vertex.Ptr = NULL;
   ctx->Array.Vertex.Enabled = GL_FALSE;
   ctx->Array.Vertex.Flags = CA_CLIENT_DATA;
   ctx->Array.Normal.Type = GL_FLOAT;
   ctx->Array.Normal.Stride = 0;
   ctx->Array.Normal.StrideB = 0;
   ctx->Array.Normal.Ptr = NULL;
   ctx->Array.Normal.Enabled = GL_FALSE;
   ctx->Array.Normal.Flags = CA_CLIENT_DATA;
   ctx->Array.Color.Size = 4;
   ctx->Array.Color.Type = GL_FLOAT;
   ctx->Array.Color.Stride = 0;
   ctx->Array.Color.StrideB = 0;
   ctx->Array.Color.Ptr = NULL;
   ctx->Array.Color.Enabled = GL_FALSE;
   ctx->Array.Color.Flags = CA_CLIENT_DATA;
   ctx->Array.SecondaryColor.Size = 4;
   ctx->Array.SecondaryColor.Type = GL_FLOAT;
   ctx->Array.SecondaryColor.Stride = 0;
   ctx->Array.SecondaryColor.StrideB = 0;
   ctx->Array.SecondaryColor.Ptr = NULL;
   ctx->Array.SecondaryColor.Enabled = GL_FALSE;
   ctx->Array.SecondaryColor.Flags = CA_CLIENT_DATA;
   ctx->Array.FogCoord.Size = 1;
   ctx->Array.FogCoord.Type = GL_FLOAT;
   ctx->Array.FogCoord.Stride = 0;
   ctx->Array.FogCoord.StrideB = 0;
   ctx->Array.FogCoord.Ptr = NULL;
   ctx->Array.FogCoord.Enabled = GL_FALSE;
   ctx->Array.FogCoord.Flags = CA_CLIENT_DATA;
   ctx->Array.Index.Type = GL_FLOAT;
   ctx->Array.Index.Stride = 0;
   ctx->Array.Index.StrideB = 0;
   ctx->Array.Index.Ptr = NULL;
   ctx->Array.Index.Enabled = GL_FALSE;
   ctx->Array.Index.Flags = CA_CLIENT_DATA;
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      ctx->Array.TexCoord[i].Size = 4;
      ctx->Array.TexCoord[i].Type = GL_FLOAT;
      ctx->Array.TexCoord[i].Stride = 0;
      ctx->Array.TexCoord[i].StrideB = 0;
      ctx->Array.TexCoord[i].Ptr = NULL;
      ctx->Array.TexCoord[i].Enabled = GL_FALSE;
      ctx->Array.TexCoord[i].Flags = CA_CLIENT_DATA;
   }
   ctx->Array.TexCoordInterleaveFactor = 1;
   ctx->Array.EdgeFlag.Stride = 0;
   ctx->Array.EdgeFlag.StrideB = 0;
   ctx->Array.EdgeFlag.Ptr = NULL;
   ctx->Array.EdgeFlag.Enabled = GL_FALSE;
   ctx->Array.EdgeFlag.Flags = CA_CLIENT_DATA;
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

   /* Renderer and client attribute stacks */
   ctx->AttribStackDepth = 0;
   ctx->ClientAttribStackDepth = 0;

   /* Display list */
   ctx->CallDepth = 0;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   ctx->CurrentListPtr = NULL;
   ctx->CurrentBlock = NULL;
   ctx->CurrentListNum = 0;
   ctx->CurrentPos = 0;

   /* Color tables */
   _mesa_init_colortable(&ctx->ColorTable);
   _mesa_init_colortable(&ctx->ProxyColorTable);
   _mesa_init_colortable(&ctx->PostConvolutionColorTable);
   _mesa_init_colortable(&ctx->ProxyPostConvolutionColorTable);
   _mesa_init_colortable(&ctx->PostColorMatrixColorTable);
   _mesa_init_colortable(&ctx->ProxyPostColorMatrixColorTable);

   /* GL_NV_vertex_program */
   ctx->VertexProgram.Current = NULL;
   ctx->VertexProgram.CurrentID = 0;
   ctx->VertexProgram.Enabled = GL_FALSE;
   ctx->VertexProgram.PointSizeEnabled = GL_FALSE;
   ctx->VertexProgram.TwoSideEnabled = GL_FALSE;
   for (i = 0; i < VP_NUM_PROG_REGS / 4; i++) {
      ctx->VertexProgram.TrackMatrix[i] = GL_NONE;
      ctx->VertexProgram.TrackMatrixTransform[i] = GL_IDENTITY_NV;
   }

   /* Miscellaneous */
   ctx->NewState = _NEW_ALL;
   ctx->RenderMode = GL_RENDER;
   ctx->_ImageTransferState = 0;

   ctx->_NeedNormals = 0;
   ctx->_NeedEyeCoords = 0;
   ctx->_ModelViewInvScale = 1.0;

   ctx->ErrorValue = (GLenum) GL_NO_ERROR;

   ctx->CatchSignals = GL_TRUE;
   ctx->OcclusionResult = GL_FALSE;
   ctx->OcclusionResultSaved = GL_FALSE;

   /* For debug/development only */
   ctx->NoRaster = ctx->imports.getenv(ctx, "MESA_NO_RASTER") ? GL_TRUE : GL_FALSE;
   ctx->FirstTimeCurrent = GL_TRUE;

   /* Dither disable */
   ctx->NoDither = ctx->imports.getenv(ctx, "MESA_NO_DITHER") ? GL_TRUE : GL_FALSE;
   if (ctx->NoDither) {
      if (ctx->imports.getenv(ctx, "MESA_DEBUG")) {
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
static GLboolean
alloc_proxy_textures( GLcontext *ctx )
{
   GLboolean out_of_memory;
   GLint i;

   ctx->Texture.Proxy1D = _mesa_alloc_texture_object(NULL, 0, GL_TEXTURE_1D);
   if (!ctx->Texture.Proxy1D) {
      return GL_FALSE;
   }

   ctx->Texture.Proxy2D = _mesa_alloc_texture_object(NULL, 0, GL_TEXTURE_2D);
   if (!ctx->Texture.Proxy2D) {
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy1D);
      return GL_FALSE;
   }

   ctx->Texture.Proxy3D = _mesa_alloc_texture_object(NULL, 0, GL_TEXTURE_3D);
   if (!ctx->Texture.Proxy3D) {
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy1D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy2D);
      return GL_FALSE;
   }

   ctx->Texture.ProxyCubeMap = _mesa_alloc_texture_object(NULL, 0,
                                                     GL_TEXTURE_CUBE_MAP_ARB);
   if (!ctx->Texture.ProxyCubeMap) {
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy1D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy2D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy3D);
      return GL_FALSE;
   }

   ctx->Texture.ProxyRect = _mesa_alloc_texture_object(NULL, 0,
                                                      GL_TEXTURE_RECTANGLE_NV);
   if (!ctx->Texture.ProxyRect) {
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy1D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy2D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy3D);
      _mesa_free_texture_object(NULL, ctx->Texture.ProxyCubeMap);
      return GL_FALSE;
   }

   out_of_memory = GL_FALSE;
   for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
      ctx->Texture.Proxy1D->Image[i] = _mesa_alloc_texture_image();
      ctx->Texture.Proxy2D->Image[i] = _mesa_alloc_texture_image();
      ctx->Texture.Proxy3D->Image[i] = _mesa_alloc_texture_image();
      ctx->Texture.ProxyCubeMap->Image[i] = _mesa_alloc_texture_image();
      if (!ctx->Texture.Proxy1D->Image[i]
          || !ctx->Texture.Proxy2D->Image[i]
          || !ctx->Texture.Proxy3D->Image[i]
          || !ctx->Texture.ProxyCubeMap->Image[i]) {
         out_of_memory = GL_TRUE;
      }
   }
   ctx->Texture.ProxyRect->Image[0] = _mesa_alloc_texture_image();
   if (!ctx->Texture.ProxyRect->Image[0])
      out_of_memory = GL_TRUE;

   if (out_of_memory) {
      for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
         if (ctx->Texture.Proxy1D->Image[i]) {
            _mesa_free_texture_image(ctx->Texture.Proxy1D->Image[i]);
         }
         if (ctx->Texture.Proxy2D->Image[i]) {
            _mesa_free_texture_image(ctx->Texture.Proxy2D->Image[i]);
         }
         if (ctx->Texture.Proxy3D->Image[i]) {
            _mesa_free_texture_image(ctx->Texture.Proxy3D->Image[i]);
         }
         if (ctx->Texture.ProxyCubeMap->Image[i]) {
            _mesa_free_texture_image(ctx->Texture.ProxyCubeMap->Image[i]);
         }
      }
      if (ctx->Texture.ProxyRect->Image[0]) {
         _mesa_free_texture_image(ctx->Texture.ProxyRect->Image[0]);
      }
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy1D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy2D);
      _mesa_free_texture_object(NULL, ctx->Texture.Proxy3D);
      _mesa_free_texture_object(NULL, ctx->Texture.ProxyCubeMap);
      _mesa_free_texture_object(NULL, ctx->Texture.ProxyRect);
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}


static void add_debug_flags( const char *debug )
{
#ifdef MESA_DEBUG
   if (strstr(debug, "varray")) 
      MESA_VERBOSE |= VERBOSE_VARRAY;

   if (strstr(debug, "tex")) 
      MESA_VERBOSE |= VERBOSE_TEXTURE;

   if (strstr(debug, "imm")) 
      MESA_VERBOSE |= VERBOSE_IMMEDIATE;

   if (strstr(debug, "pipe")) 
      MESA_VERBOSE |= VERBOSE_PIPELINE;

   if (strstr(debug, "driver")) 
      MESA_VERBOSE |= VERBOSE_DRIVER;

   if (strstr(debug, "state")) 
      MESA_VERBOSE |= VERBOSE_STATE;

   if (strstr(debug, "api")) 
      MESA_VERBOSE |= VERBOSE_API;

   if (strstr(debug, "list")) 
      MESA_VERBOSE |= VERBOSE_DISPLAY_LIST;

   if (strstr(debug, "lighting")) 
      MESA_VERBOSE |= VERBOSE_LIGHTING;
   
   /* Debug flag:
    */
   if (strstr(debug, "flush")) 
      MESA_DEBUG_FLAGS |= DEBUG_ALWAYS_FLUSH;
#endif
}


/*
 * Initialize a GLcontext struct.  This includes allocating all the
 * other structs and arrays which hang off of the context by pointers.
 */
GLboolean
_mesa_initialize_context( GLcontext *ctx,
                          const GLvisual *visual,
                          GLcontext *share_list,
                          const __GLimports *imports )
{
   GLuint dispatchSize;

   ASSERT(imports);
   ASSERT(imports->other); /* other points to the device driver's context */

   /* assing imports */
   ctx->imports = *imports;

   /* initialize the exports (Mesa functions called by the window system) */
   _mesa_init_default_exports( &(ctx->exports) );

   /* misc one-time initializations */
   one_time_init(ctx);

#if 0
   /**
    ** OpenGL SI stuff
    **/
   if (!ctx->imports.malloc) {
      _mesa_init_default_imports(&ctx->imports, driver_ctx);
   }
   /* exports are setup by the device driver */
#endif

   ctx->DriverCtx = imports->other;
   ctx->Visual = *visual;
   ctx->DrawBuffer = NULL;
   ctx->ReadBuffer = NULL;

   if (share_list) {
      /* share state with another context */
      ctx->Shared = share_list->Shared;
   }
   else {
      /* allocate new, unshared state */
      ctx->Shared = alloc_shared_state();
      if (!ctx->Shared) {
         return GL_FALSE;
      }
   }
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount++;
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   /* Effectively bind the default textures to all texture units */
   ctx->Shared->Default1D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->Default2D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->Default3D->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->DefaultCubeMap->RefCount += MAX_TEXTURE_UNITS;
   ctx->Shared->DefaultRect->RefCount += MAX_TEXTURE_UNITS;

   init_attrib_groups( ctx );

   if (visual->doubleBufferMode) {
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

   if (!alloc_proxy_textures(ctx)) {
      free_shared_state(ctx, ctx->Shared);
      return GL_FALSE;
   }

   /* register the most recent extension functions with libGL */
   _glapi_add_entrypoint("glTbufferMask3DFX", 553);
   _glapi_add_entrypoint("glCompressedTexImage3DARB", 554);
   _glapi_add_entrypoint("glCompressedTexImage2DARB", 555);
   _glapi_add_entrypoint("glCompressedTexImage1DARB", 556);
   _glapi_add_entrypoint("glCompressedTexSubImage3DARB", 557);
   _glapi_add_entrypoint("glCompressedTexSubImage2DARB", 558);
   _glapi_add_entrypoint("glCompressedTexSubImage1DARB", 559);
   _glapi_add_entrypoint("glGetCompressedTexImageARB", 560);

   /* Find the larger of Mesa's dispatch table and libGL's dispatch table.
    * In practice, this'll be the same for stand-alone Mesa.  But for DRI
    * Mesa we do this to accomodate different versions of libGL and various
    * DRI drivers.
    */
   dispatchSize = MAX2(_glapi_get_dispatch_table_size(),
                       sizeof(struct _glapi_table) / sizeof(void *));

   /* setup API dispatch tables */
   ctx->Exec = (struct _glapi_table *) CALLOC(dispatchSize * sizeof(void*));
   ctx->Save = (struct _glapi_table *) CALLOC(dispatchSize * sizeof(void*));
   if (!ctx->Exec || !ctx->Save) {
      free_shared_state(ctx, ctx->Shared);
      if (ctx->Exec)
         FREE( ctx->Exec );
   }
   _mesa_init_exec_table(ctx->Exec, dispatchSize);
   _mesa_init_dlist_table(ctx->Save, dispatchSize);
   ctx->CurrentDispatch = ctx->Exec;

   ctx->ExecPrefersFloat = GL_FALSE;
   ctx->SavePrefersFloat = GL_FALSE;

   /* Neutral tnl module stuff */
   _mesa_init_exec_vtxfmt( ctx );
   ctx->TnlModule.Current = NULL;
   ctx->TnlModule.SwapCount = 0;

   /* Z buffer stuff */
   if (ctx->Visual.depthBits == 0) {
      /* Special case.  Even if we don't have a depth buffer we need
       * good values for DepthMax for Z vertex transformation purposes
       * and for per-fragment fog computation.
       */
      ctx->DepthMax = 1 << 16;
      ctx->DepthMaxF = (GLfloat) ctx->DepthMax;
   }
   else if (ctx->Visual.depthBits < 32) {
      ctx->DepthMax = (1 << ctx->Visual.depthBits) - 1;
      ctx->DepthMaxF = (GLfloat) ctx->DepthMax;
   }
   else {
      /* Special case since shift values greater than or equal to the
       * number of bits in the left hand expression's type are undefined.
       */
      ctx->DepthMax = 0xffffffff;
      ctx->DepthMaxF = (GLfloat) ctx->DepthMax;
   }
   ctx->MRD = 1.0;  /* Minimum resolvable depth value, for polygon offset */


#if defined(MESA_TRACE)
   ctx->TraceCtx = (trace_context_t *) CALLOC( sizeof(trace_context_t) );
#if 0
   /* Brian: do you want to have CreateContext fail here,
       or should we just trap in NewTrace (currently done)? */
   if (!(ctx->TraceCtx)) {
      free_shared_state(ctx, ctx->Shared);
      FREE( ctx->Exec );
      FREE( ctx->Save );
      return GL_FALSE;
   }
#endif
   trInitContext(ctx->TraceCtx);

   ctx->TraceDispatch = (struct _glapi_table *)
                        CALLOC(dispatchSize * sizeof(void*));
#if 0
   if (!(ctx->TraceCtx)) {
      free_shared_state(ctx, ctx->Shared);
      FREE( ctx->Exec );
      FREE( ctx->Save );
      FREE( ctx->TraceCtx );
      return GL_FALSE;
   }
#endif
   trInitDispatch(ctx->TraceDispatch);
#endif


   if (ctx->imports.getenv(ctx, "MESA_DEBUG"))
      add_debug_flags(ctx->imports.getenv(ctx, "MESA_DEBUG"));

   if (ctx->imports.getenv(ctx, "MESA_VERBOSE"))
      add_debug_flags(ctx->imports.getenv(ctx, "MESA_VERBOSE"));

   return GL_TRUE;
}



/*
 * Allocate and initialize a GLcontext structure.
 * Input:  visual - a GLvisual pointer (we copy the struct contents)
 *         sharelist - another context to share display lists with or NULL
 *         imports - points to a fully-initialized __GLimports object.
 * Return:  pointer to a new __GLcontextRec or NULL if error.
 */
GLcontext *
_mesa_create_context( const GLvisual *visual,
                      GLcontext *share_list,
                      const __GLimports *imports )
{
   GLcontext *ctx;

   ASSERT(visual);
   ASSERT(imports);
   ASSERT(imports->calloc);

   ctx = (GLcontext *) imports->calloc(NULL, 1, sizeof(GLcontext));
   if (!ctx)
      return NULL;

   ctx->Driver.CurrentExecPrimitive = 0;  /* XXX why is this here??? */

   if (_mesa_initialize_context(ctx, visual, share_list, imports)) {
      return ctx;
   }
   else {
      imports->free(NULL, ctx);
      return NULL;
   }
}



/*
 * Free the data associated with the given context.
 * But don't free() the GLcontext struct itself!
 */
void
_mesa_free_context_data( GLcontext *ctx )
{
   struct gl_shine_tab *s, *tmps;
   GLuint i;

   /* if we're destroying the current context, unbind it first */
   if (ctx == _mesa_get_current_context()) {
      _mesa_make_current(NULL, NULL);
   }

   /*
    * Free transformation matrix stacks
    */
   free_matrix_stack(&ctx->ModelviewMatrixStack);
   free_matrix_stack(&ctx->ProjectionMatrixStack);
   free_matrix_stack(&ctx->ColorMatrixStack);
   for (i = 0; i < MAX_TEXTURE_UNITS; i++)
      free_matrix_stack(&ctx->TextureMatrixStack[i]);
   for (i = 0; i < MAX_PROGRAM_MATRICES; i++)
      free_matrix_stack(&ctx->ProgramMatrixStack[i]);
   /* combined Modelview*Projection matrix */
   _math_matrix_dtr( &ctx->_ModelProjectMatrix );


   if (ctx->VertexProgram.Current) {
      ctx->VertexProgram.Current->RefCount--;
      if (ctx->VertexProgram.Current->RefCount <= 0)
         _mesa_delete_program(ctx, ctx->VertexProgram.CurrentID);
   }

   /* Shared context state (display lists, textures, etc) */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
   ctx->Shared->RefCount--;
   assert(ctx->Shared->RefCount >= 0);
   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   if (ctx->Shared->RefCount == 0) {
      /* free shared state */
      free_shared_state( ctx, ctx->Shared );
   }

   /* Free lighting shininess exponentiation table */
   foreach_s( s, tmps, ctx->_ShineTabList ) {
      FREE( s );
   }
   FREE( ctx->_ShineTabList );

   /* Free proxy texture objects */
   _mesa_free_texture_object( NULL, ctx->Texture.Proxy1D );
   _mesa_free_texture_object( NULL, ctx->Texture.Proxy2D );
   _mesa_free_texture_object( NULL, ctx->Texture.Proxy3D );
   _mesa_free_texture_object( NULL, ctx->Texture.ProxyCubeMap );
   _mesa_free_texture_object( NULL, ctx->Texture.ProxyRect );

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
   for (i = 0; i < 16; i++)
      FREE((ctx->EvalMap.Map1Attrib[i].Points));

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
   for (i = 0; i < 16; i++)
      FREE((ctx->EvalMap.Map2Attrib[i].Points));

   _mesa_free_colortable_data( &ctx->ColorTable );
   _mesa_free_colortable_data( &ctx->PostConvolutionColorTable );
   _mesa_free_colortable_data( &ctx->PostColorMatrixColorTable );
   _mesa_free_colortable_data( &ctx->Texture.Palette );

   _math_matrix_dtr(&ctx->Viewport._WindowMap);

   _mesa_extensions_dtr(ctx);

   FREE(ctx->Exec);
   FREE(ctx->Save);
}



/*
 * Destroy a GLcontext structure.
 */
void
_mesa_destroy_context( GLcontext *ctx )
{
   if (ctx) {
      _mesa_free_context_data(ctx);
      FREE( (void *) ctx );
   }
}



/*
 * Copy attribute groups from one context to another.
 * Input:  src - source context
 *         dst - destination context
 *         mask - bitwise OR of GL_*_BIT flags
 */
void
_mesa_copy_context( const GLcontext *src, GLcontext *dst, GLuint mask )
{
   if (mask & GL_ACCUM_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Accum = src->Accum;
   }
   if (mask & GL_COLOR_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Color = src->Color;
   }
   if (mask & GL_CURRENT_BIT) {
      /* OK to memcpy */
      dst->Current = src->Current;
   }
   if (mask & GL_DEPTH_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Depth = src->Depth;
   }
   if (mask & GL_ENABLE_BIT) {
      /* no op */
   }
   if (mask & GL_EVAL_BIT) {
      /* OK to memcpy */
      dst->Eval = src->Eval;
   }
   if (mask & GL_FOG_BIT) {
      /* OK to memcpy */
      dst->Fog = src->Fog;
   }
   if (mask & GL_HINT_BIT) {
      /* OK to memcpy */
      dst->Hint = src->Hint;
   }
   if (mask & GL_LIGHTING_BIT) {
      GLuint i;
      /* begin with memcpy */
      MEMCPY( &dst->Light, &src->Light, sizeof(struct gl_light) );
      /* fixup linked lists to prevent pointer insanity */
      make_empty_list( &(dst->Light.EnabledList) );
      for (i = 0; i < MAX_LIGHTS; i++) {
         if (dst->Light.Light[i].Enabled) {
            insert_at_tail(&(dst->Light.EnabledList), &(dst->Light.Light[i]));
         }
      }
   }
   if (mask & GL_LINE_BIT) {
      /* OK to memcpy */
      dst->Line = src->Line;
   }
   if (mask & GL_LIST_BIT) {
      /* OK to memcpy */
      dst->List = src->List;
   }
   if (mask & GL_PIXEL_MODE_BIT) {
      /* OK to memcpy */
      dst->Pixel = src->Pixel;
   }
   if (mask & GL_POINT_BIT) {
      /* OK to memcpy */
      dst->Point = src->Point;
   }
   if (mask & GL_POLYGON_BIT) {
      /* OK to memcpy */
      dst->Polygon = src->Polygon;
   }
   if (mask & GL_POLYGON_STIPPLE_BIT) {
      /* Use loop instead of MEMCPY due to problem with Portland Group's
       * C compiler.  Reported by John Stone.
       */
      GLuint i;
      for (i = 0; i < 32; i++) {
         dst->PolygonStipple[i] = src->PolygonStipple[i];
      }
   }
   if (mask & GL_SCISSOR_BIT) {
      /* OK to memcpy */
      dst->Scissor = src->Scissor;
   }
   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* OK to memcpy */
      dst->Stencil = src->Stencil;
   }
   if (mask & GL_TEXTURE_BIT) {
      /* Cannot memcpy because of pointers */
      _mesa_copy_texture_state(src, dst);
   }
   if (mask & GL_TRANSFORM_BIT) {
      /* OK to memcpy */
      dst->Transform = src->Transform;
   }
   if (mask & GL_VIEWPORT_BIT) {
      /* Cannot use memcpy, because of pointers in GLmatrix _WindowMap */
      dst->Viewport.X = src->Viewport.X;
      dst->Viewport.Y = src->Viewport.Y;
      dst->Viewport.Width = src->Viewport.Width;
      dst->Viewport.Height = src->Viewport.Height;
      dst->Viewport.Near = src->Viewport.Near;
      dst->Viewport.Far = src->Viewport.Far;
      _math_matrix_copy(&dst->Viewport._WindowMap, &src->Viewport._WindowMap);
   }

   /* XXX FIXME:  Call callbacks?
    */
   dst->NewState = _NEW_ALL;
}



static void print_info( void )
{
   fprintf(stderr, "Mesa GL_VERSION = %s\n",
	   (char *) _mesa_GetString(GL_VERSION));
   fprintf(stderr, "Mesa GL_RENDERER = %s\n",
	   (char *) _mesa_GetString(GL_RENDERER));
   fprintf(stderr, "Mesa GL_VENDOR = %s\n",
	   (char *) _mesa_GetString(GL_VENDOR));
   fprintf(stderr, "Mesa GL_EXTENSIONS = %s\n",
	   (char *) _mesa_GetString(GL_EXTENSIONS));
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
#if defined(USE_SPARC_ASM)
   fprintf(stderr, "Mesa sparc-optimized: YES\n");
#else
   fprintf(stderr, "Mesa sparc-optimized: NO\n");
#endif
}


/*
 * Set the current context, binding the given frame buffer to the context.
 */
void
_mesa_make_current( GLcontext *newCtx, GLframebuffer *buffer )
{
   _mesa_make_current2( newCtx, buffer, buffer );
}


/*
 * Bind the given context to the given draw-buffer and read-buffer
 * and make it the current context for this thread.
 */
void
_mesa_make_current2( GLcontext *newCtx, GLframebuffer *drawBuffer,
                     GLframebuffer *readBuffer )
{
   if (MESA_VERBOSE)
      _mesa_debug(newCtx, "_mesa_make_current2()\n");

   /* Check that the context's and framebuffer's visuals are compatible.
    * We could do a lot more checking here but this'll catch obvious
    * problems.
    */
   if (newCtx && drawBuffer && readBuffer) {
      if (newCtx->Visual.rgbMode != drawBuffer->Visual.rgbMode ||
          newCtx->Visual.redBits != drawBuffer->Visual.redBits ||
          newCtx->Visual.depthBits != drawBuffer->Visual.depthBits ||
          newCtx->Visual.stencilBits != drawBuffer->Visual.stencilBits ||
          newCtx->Visual.accumRedBits != drawBuffer->Visual.accumRedBits) {
         return; /* incompatible */
      }
   }

   /* We call this function periodically (just here for now) in
    * order to detect when multithreading has begun.
    */
   _glapi_check_multithread();

   _glapi_set_context((void *) newCtx);
   ASSERT(_mesa_get_current_context() == newCtx);


   if (!newCtx) {
      _glapi_set_dispatch(NULL);  /* none current */
   }
   else {
      _glapi_set_dispatch(newCtx->CurrentDispatch);

      if (drawBuffer && readBuffer) {
	 /* TODO: check if newCtx and buffer's visual match??? */
	 newCtx->DrawBuffer = drawBuffer;
	 newCtx->ReadBuffer = readBuffer;
	 newCtx->NewState |= _NEW_BUFFERS;
         /* _mesa_update_state( newCtx ); */
      }

      /* This is only for T&L - a bit out of place, or misnamed (BP) */
      if (newCtx->Driver.MakeCurrent)
	 newCtx->Driver.MakeCurrent( newCtx, drawBuffer, readBuffer );

      /* We can use this to help debug user's problems.  Tell them to set
       * the MESA_INFO env variable before running their app.  Then the
       * first time each context is made current we'll print some useful
       * information.
       */
      if (newCtx->FirstTimeCurrent) {
	 if (newCtx->imports.getenv(newCtx, "MESA_INFO")) {
	    print_info();
	 }
	 newCtx->FirstTimeCurrent = GL_FALSE;
      }
   }
}



/*
 * Return current context handle for the calling thread.
 * This isn't the fastest way to get the current context.
 * If you need speed, see the GET_CURRENT_CONTEXT() macro in context.h
 */
GLcontext *
_mesa_get_current_context( void )
{
   return (GLcontext *) _glapi_get_context();
}



/*
 * This should be called by device drivers just before they do a
 * swapbuffers.  Any pending rendering commands will be executed.
 * XXX we should really rename this function to _mesa_flush() or something.
 */
void
_mesa_swapbuffers(GLcontext *ctx)
{
   FLUSH_VERTICES( ctx, 0 );
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
void _mesa_problem( const GLcontext *ctx, const char *s )
{
   if (ctx) {
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Mesa implementation error: %s\n", s);
#ifdef XF86DRI
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Please report to the DRI bug database at dri.sourceforge.net\n");
#else
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Please report to the Mesa bug database at www.mesa3d.org\n" );
#endif
   }
   else {
      /* what can we do if we don't have a context???? */
      fprintf( stderr, "Mesa implementation error: %s\n", s );
#ifdef XF86DRI
      fprintf( stderr, "Please report to the DRI bug database at dri.sourceforge.net\n");
#else
      fprintf( stderr, "Please report to the Mesa bug database at www.mesa3d.org\n" );
#endif
   }
}



/*
 * This is called to inform the user that he or she has tried to do
 * something illogical or if there's likely a bug in their program
 * (like enabled depth testing without a depth buffer).
 */
void
_mesa_warning( const GLcontext *ctx, const char *s )
{
   (*ctx->imports.warning)((__GLcontext *) ctx, (char *) s);
}



/*
 * This is Mesa's error handler.  Normally, all that's done is the updating
 * of the current error value.  If Mesa is compiled with -DDEBUG or if the
 * environment variable "MESA_DEBUG" is defined then a real error message
 * is printed to stderr.
 * Input:  ctx - the GL context
 *         error - the error value
 *         where - usually the name of function where error was detected
 */
void
_mesa_error( GLcontext *ctx, GLenum error, const char *where )
{
   const char *debugEnv;
   GLboolean debug;

   if (ctx)
      debugEnv = ctx->imports.getenv(ctx, "MESA_DEBUG");
   else
      /* what can we do??? */
      debugEnv = "";

#ifdef DEBUG
   if (debugEnv && strstr(debugEnv, "silent"))
      debug = GL_FALSE;
   else
      debug = GL_TRUE;
#else
   if (debugEnv)
      debug = GL_TRUE;
   else
      debug = GL_FALSE;
#endif

   if (debug) {
      const char *errstr;
      switch (error) {
	 case GL_NO_ERROR:
	    errstr = "GL_NO_ERROR";
	    break;
	 case GL_INVALID_VALUE:
	    errstr = "GL_INVALID_VALUE";
	    break;
	 case GL_INVALID_ENUM:
	    errstr = "GL_INVALID_ENUM";
	    break;
	 case GL_INVALID_OPERATION:
	    errstr = "GL_INVALID_OPERATION";
	    break;
	 case GL_STACK_OVERFLOW:
	    errstr = "GL_STACK_OVERFLOW";
	    break;
	 case GL_STACK_UNDERFLOW:
	    errstr = "GL_STACK_UNDERFLOW";
	    break;
	 case GL_OUT_OF_MEMORY:
	    errstr = "GL_OUT_OF_MEMORY";
	    break;
         case GL_TABLE_TOO_LARGE:
            errstr = "GL_TABLE_TOO_LARGE";
            break;
	 default:
	    errstr = "unknown";
	    break;
      }
      fprintf(stderr, "Mesa user error: %s in %s\n", errstr, where);
   }

   if (!ctx)
      return;

   if (ctx->ErrorValue == GL_NO_ERROR) {
      ctx->ErrorValue = error;
   }

   /* Call device driver's error handler, if any.  This is used on the Mac. */
   if (ctx->Driver.Error) {
      (*ctx->Driver.Error)( ctx );
   }
}


/*
 * Call this to report debug information.  Uses stderr.
 */
void
_mesa_debug( const GLcontext *ctx, const char *fmtString, ... )
{
   va_list args;
   va_start( args, fmtString );  
   (void) ctx->imports.fprintf( (__GLcontext *) ctx, stderr, fmtString, args );
   va_end( args );
}


/*
 * A wrapper for printf.  Uses stdout.
 */
void
_mesa_printf( const GLcontext *ctx, const char *fmtString, ... )
{
   va_list args;
   va_start( args, fmtString );  
   (void) ctx->imports.fprintf( (__GLcontext *) ctx, stdout, fmtString, args );
   va_end( args );
}



void
_mesa_Finish( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (ctx->Driver.Finish) {
      (*ctx->Driver.Finish)( ctx );
   }
}



void
_mesa_Flush( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   if (ctx->Driver.Flush) {
      (*ctx->Driver.Flush)( ctx );
   }
}



const char *_mesa_prim_name[GL_POLYGON+4] = {
   "GL_POINTS",
   "GL_LINES",
   "GL_LINE_LOOP",
   "GL_LINE_STRIP",
   "GL_TRIANGLES",
   "GL_TRIANGLE_STRIP",
   "GL_TRIANGLE_FAN",
   "GL_QUADS",
   "GL_QUAD_STRIP",
   "GL_POLYGON",
   "outside begin/end",
   "inside unkown primitive",
   "unknown state"
};
