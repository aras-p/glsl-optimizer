/* $Id: dlist.c,v 1.7 1999/10/09 10:01:46 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


/* $XFree86: xc/lib/GL/mesa/src/dlist.c,v 1.3 1999/04/04 00:20:22 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "accum.h"
#include "api.h"
#include "alpha.h"
#include "attrib.h"
#include "bitmap.h"
#include "bbox.h"
#include "blend.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "copypix.h"
#include "depth.h"
#include "drawpix.h"
#include "enable.h"
#include "enums.h"
#include "eval.h"
#include "extensions.h"
#include "feedback.h"
#include "fog.h"
#include "get.h"
#include "glmisc.h"
#include "hash.h"
#include "image.h"
#include "light.h"
#include "lines.h"
#include "dlist.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "matrix.h"
#include "pipeline.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "readpix.h"
#include "rect.h"
#include "scissor.h"
#include "stencil.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbfill.h"
#include "vbxform.h"
#include "winpos.h"
#include "xform.h"
#ifdef XFree86Server
#undef MISC_H
#include "GL/xf86glx.h"
#endif
#endif



/*
Functions which aren't compiled but executed immediately:
	glIsList
	glGenLists
	glDeleteLists
	glEndList
	glFeedbackBuffer
	glSelectBuffer
	glRenderMode
	glReadPixels
	glPixelStore
	glFlush
	glFinish
	glIsEnabled
	glGet*

Functions which cause errors if called while compiling a display list:
	glNewList
*/



/*
 * Display list instructions are stored as sequences of "nodes".  Nodes
 * are allocated in blocks.  Each block has BLOCK_SIZE nodes.  Blocks
 * are linked together with a pointer.
 */


/* How many nodes to allocate at a time: 
 * - reduced now that we hold vertices etc. elsewhere.
 */
#define BLOCK_SIZE 64


/*
 * Display list opcodes.
 *
 * The fact that these identifiers are assigned consecutive
 * integer values starting at 0 is very important, see InstSize array usage)
 *
 * KW: Commented out opcodes now handled by vertex-cassettes.
 */
typedef enum {
	OPCODE_ACCUM,
	OPCODE_ALPHA_FUNC,
        OPCODE_BIND_TEXTURE,
	OPCODE_BITMAP,
	OPCODE_BLEND_COLOR,
	OPCODE_BLEND_EQUATION,
	OPCODE_BLEND_FUNC,
	OPCODE_BLEND_FUNC_SEPARATE,
        OPCODE_CALL_LIST,
        OPCODE_CALL_LIST_OFFSET,
	OPCODE_CLEAR,
	OPCODE_CLEAR_ACCUM,
	OPCODE_CLEAR_COLOR,
	OPCODE_CLEAR_DEPTH,
	OPCODE_CLEAR_INDEX,
	OPCODE_CLEAR_STENCIL,
        OPCODE_CLIP_PLANE,
	OPCODE_COLOR_MASK,
	OPCODE_COLOR_MATERIAL,
	OPCODE_COLOR_TABLE,
	OPCODE_COLOR_SUB_TABLE,
	OPCODE_COPY_PIXELS,
        OPCODE_COPY_TEX_IMAGE1D,
        OPCODE_COPY_TEX_IMAGE2D,
        OPCODE_COPY_TEX_IMAGE3D,
        OPCODE_COPY_TEX_SUB_IMAGE1D,
        OPCODE_COPY_TEX_SUB_IMAGE2D,
        OPCODE_COPY_TEX_SUB_IMAGE3D,
	OPCODE_CULL_FACE,
	OPCODE_DEPTH_FUNC,
	OPCODE_DEPTH_MASK,
	OPCODE_DEPTH_RANGE,
	OPCODE_DISABLE,
	OPCODE_DRAW_BUFFER,
	OPCODE_DRAW_PIXELS,
	OPCODE_ENABLE,
	OPCODE_EVALCOORD1,
	OPCODE_EVALCOORD2,
	OPCODE_EVALMESH1,
	OPCODE_EVALMESH2,
	OPCODE_EVALPOINT1,
	OPCODE_EVALPOINT2,
	OPCODE_FOG,
	OPCODE_FRONT_FACE,
	OPCODE_FRUSTUM,
	OPCODE_HINT,
	OPCODE_INDEX_MASK,
	OPCODE_INIT_NAMES,
	OPCODE_LIGHT,
	OPCODE_LIGHT_MODEL,
	OPCODE_LINE_STIPPLE,
	OPCODE_LINE_WIDTH,
	OPCODE_LIST_BASE,
	OPCODE_LOAD_IDENTITY,
	OPCODE_LOAD_MATRIX,
	OPCODE_LOAD_NAME,
	OPCODE_LOGIC_OP,
	OPCODE_MAP1,
	OPCODE_MAP2,
	OPCODE_MAPGRID1,
	OPCODE_MAPGRID2,
	OPCODE_MATRIX_MODE,
	OPCODE_MULT_MATRIX,
	OPCODE_ORTHO,
	OPCODE_PASSTHROUGH,
	OPCODE_PIXEL_MAP,
	OPCODE_PIXEL_TRANSFER,
	OPCODE_PIXEL_ZOOM,
	OPCODE_POINT_SIZE,
        OPCODE_POINT_PARAMETERS,
	OPCODE_POLYGON_MODE,
        OPCODE_POLYGON_STIPPLE,
	OPCODE_POLYGON_OFFSET,
	OPCODE_POP_ATTRIB,
	OPCODE_POP_MATRIX,
	OPCODE_POP_NAME,
	OPCODE_PRIORITIZE_TEXTURE,
	OPCODE_PUSH_ATTRIB,
	OPCODE_PUSH_MATRIX,
	OPCODE_PUSH_NAME,
	OPCODE_RASTER_POS,
	OPCODE_RECTF,
	OPCODE_READ_BUFFER,
        OPCODE_SCALE,
	OPCODE_SCISSOR,
	OPCODE_SELECT_TEXTURE_SGIS,
	OPCODE_SELECT_TEXTURE_COORD_SET,
	OPCODE_SHADE_MODEL,
	OPCODE_STENCIL_FUNC,
	OPCODE_STENCIL_MASK,
	OPCODE_STENCIL_OP,
        OPCODE_TEXENV,
        OPCODE_TEXGEN,
        OPCODE_TEXPARAMETER,
	OPCODE_TEX_IMAGE1D,
	OPCODE_TEX_IMAGE2D,
	OPCODE_TEX_IMAGE3D,
	OPCODE_TEX_SUB_IMAGE1D,
	OPCODE_TEX_SUB_IMAGE2D,
	OPCODE_TEX_SUB_IMAGE3D,
        OPCODE_TRANSLATE,
	OPCODE_VIEWPORT,
	OPCODE_WINDOW_POS,
        /* GL_ARB_multitexture */
        OPCODE_ACTIVE_TEXTURE,
        OPCODE_CLIENT_ACTIVE_TEXTURE,
	/* The following three are meta instructions */
	OPCODE_ERROR,	        /* raise compiled-in error */
	OPCODE_VERTEX_CASSETTE,	/* render prebuilt vertex buffer */
	OPCODE_CONTINUE,
	OPCODE_END_OF_LIST
} OpCode;


/*
 * Each instruction in the display list is stored as a sequence of
 * contiguous nodes in memory.
 * Each node is the union of a variety of datatypes.
 */
union node {
	OpCode		opcode;
	GLboolean	b;
	GLbitfield	bf;
	GLubyte		ub;
	GLshort		s;
	GLushort	us;
	GLint		i;
	GLuint		ui;
	GLenum		e;
	GLfloat		f;
	GLvoid		*data;
	void		*next;	/* If prev node's opcode==OPCODE_CONTINUE */
};



/* Number of nodes of storage needed for each instruction: */
static GLuint InstSize[ OPCODE_END_OF_LIST+1 ];

void mesa_print_display_list( GLuint list );


/**********************************************************************/
/*****                           Private                          *****/
/**********************************************************************/


/*
 * Allocate space for a display list instruction.
 * Input:  opcode - type of instruction
 *         argcount - number of arguments following the instruction
 * Return: pointer to first node in the instruction
 */
static Node *alloc_instruction( GLcontext *ctx, OpCode opcode, GLint argcount )
{
   Node *n, *newblock;
   GLuint count = InstSize[opcode];

   assert( (GLint) count == argcount+1 );

   if (ctx->CurrentPos + count + 2 > BLOCK_SIZE) {
      /* This block is full.  Allocate a new block and chain to it */
      n = ctx->CurrentBlock + ctx->CurrentPos;
      n[0].opcode = OPCODE_CONTINUE;
      newblock = (Node *) malloc( sizeof(Node) * BLOCK_SIZE );
      if (!newblock) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "Building display list" );
         return NULL;
      }
      n[1].next = (Node *) newblock;
      ctx->CurrentBlock = newblock;
      ctx->CurrentPos = 0;
   }

   n = ctx->CurrentBlock + ctx->CurrentPos;
   ctx->CurrentPos += count;

   n[0].opcode = opcode;

   return n;
}



/*
 * Make an empty display list.  This is used by glGenLists() to
 * reserver display list IDs.
 */
static Node *make_empty_list( void )
{
   Node *n = (Node *) malloc( sizeof(Node) );
   n[0].opcode = OPCODE_END_OF_LIST;
   return n;
}



/*
 * Destroy all nodes in a display list.
 * Input:  list - display list number
 */
void gl_destroy_list( GLcontext *ctx, GLuint list )
{
   Node *n, *block;
   GLboolean done;

   if (list==0)
      return;

   block = (Node *) HashLookup(ctx->Shared->DisplayList, list);
   n = block;

   done = block ? GL_FALSE : GL_TRUE;
   while (!done) {
      switch (n[0].opcode) {
	 /* special cases first */
         case OPCODE_VERTEX_CASSETTE: 
	    if ( ! -- ((struct immediate *) n[1].data)->ref_count )
	       gl_immediate_free( (struct immediate *) n[1].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_MAP1:
	    gl_free_control_points( ctx, n[1].e, (GLfloat *) n[6].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_MAP2:
	    gl_free_control_points( ctx, n[1].e, (GLfloat *) n[10].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_DRAW_PIXELS:
	    gl_free_image( (struct gl_image *) n[1].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_BITMAP:
	    gl_free_image( (struct gl_image *) n[7].data );
	    n += InstSize[n[0].opcode];
	    break;
         case OPCODE_COLOR_TABLE:
            gl_free_image( (struct gl_image *) n[3].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COLOR_SUB_TABLE:
            gl_free_image( (struct gl_image *) n[3].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_POLYGON_STIPPLE:
            free( n[1].data );
	    n += InstSize[n[0].opcode];
            break;
	 case OPCODE_TEX_IMAGE1D:
            gl_free_image( (struct gl_image *) n[8].data );
            n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_TEX_IMAGE2D:
            gl_free_image( (struct gl_image *) n[9].data );
            n += InstSize[n[0].opcode];
	    break;
         case OPCODE_TEX_SUB_IMAGE1D:
            {
               struct gl_image *image;
               image = (struct gl_image *) n[7].data;
               gl_free_image( image );
            }
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            {
               struct gl_image *image;
               image = (struct gl_image *) n[9].data;
               gl_free_image( image );
            }
            break;
	 case OPCODE_CONTINUE:
	    n = (Node *) n[1].next;
	    free( block );
	    block = n;
	    break;
	 case OPCODE_END_OF_LIST:
	    free( block );
	    done = GL_TRUE;
	    break;
	 default:
	    /* Most frequent case */
	    n += InstSize[n[0].opcode];
	    break;
      }
   }

   HashRemove(ctx->Shared->DisplayList, list);
}



/*
 * Translate the nth element of list from type to GLuint.
 */
static GLuint translate_id( GLsizei n, GLenum type, const GLvoid *list )
{
   GLbyte *bptr;
   GLubyte *ubptr;
   GLshort *sptr;
   GLushort *usptr;
   GLint *iptr;
   GLuint *uiptr;
   GLfloat *fptr;

   switch (type) {
      case GL_BYTE:
         bptr = (GLbyte *) list;
         return (GLuint) *(bptr+n);
      case GL_UNSIGNED_BYTE:
         ubptr = (GLubyte *) list;
         return (GLuint) *(ubptr+n);
      case GL_SHORT:
         sptr = (GLshort *) list;
         return (GLuint) *(sptr+n);
      case GL_UNSIGNED_SHORT:
         usptr = (GLushort *) list;
         return (GLuint) *(usptr+n);
      case GL_INT:
         iptr = (GLint *) list;
         return (GLuint) *(iptr+n);
      case GL_UNSIGNED_INT:
         uiptr = (GLuint *) list;
         return (GLuint) *(uiptr+n);
      case GL_FLOAT:
         fptr = (GLfloat *) list;
         return (GLuint) *(fptr+n);
      case GL_2_BYTES:
         ubptr = ((GLubyte *) list) + 2*n;
         return (GLuint) *ubptr * 256 + (GLuint) *(ubptr+1);
      case GL_3_BYTES:
         ubptr = ((GLubyte *) list) + 3*n;
         return (GLuint) *ubptr * 65536
              + (GLuint) *(ubptr+1) * 256
              + (GLuint) *(ubptr+2);
      case GL_4_BYTES:
         ubptr = ((GLubyte *) list) + 4*n;
         return (GLuint) *ubptr * 16777216
              + (GLuint) *(ubptr+1) * 65536
              + (GLuint) *(ubptr+2) * 256
              + (GLuint) *(ubptr+3);
      default:
         return 0;
   }
}




/**********************************************************************/
/*****                        Public                              *****/
/**********************************************************************/

void gl_init_lists( void )
{
   static int init_flag = 0;

   if (init_flag==0) {
      InstSize[OPCODE_ACCUM] = 3;
      InstSize[OPCODE_ALPHA_FUNC] = 3;
      InstSize[OPCODE_BIND_TEXTURE] = 3;
      InstSize[OPCODE_BITMAP] = 8;
      InstSize[OPCODE_BLEND_COLOR] = 5;
      InstSize[OPCODE_BLEND_EQUATION] = 2;
      InstSize[OPCODE_BLEND_FUNC] = 3;
      InstSize[OPCODE_BLEND_FUNC_SEPARATE] = 5;
      InstSize[OPCODE_CALL_LIST] = 2;
      InstSize[OPCODE_CALL_LIST_OFFSET] = 2;
      InstSize[OPCODE_CLEAR] = 2;
      InstSize[OPCODE_CLEAR_ACCUM] = 5;
      InstSize[OPCODE_CLEAR_COLOR] = 5;
      InstSize[OPCODE_CLEAR_DEPTH] = 2;
      InstSize[OPCODE_CLEAR_INDEX] = 2;
      InstSize[OPCODE_CLEAR_STENCIL] = 2;
      InstSize[OPCODE_CLIP_PLANE] = 6;
      InstSize[OPCODE_COLOR_MASK] = 5;
      InstSize[OPCODE_COLOR_MATERIAL] = 3;
      InstSize[OPCODE_COLOR_TABLE] = 4;
      InstSize[OPCODE_COLOR_SUB_TABLE] = 4;
      InstSize[OPCODE_COPY_PIXELS] = 6;
      InstSize[OPCODE_COPY_TEX_IMAGE1D] = 8;
      InstSize[OPCODE_COPY_TEX_IMAGE2D] = 9;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE1D] = 7;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE2D] = 9;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE3D] = 10;
      InstSize[OPCODE_CULL_FACE] = 2;
      InstSize[OPCODE_DEPTH_FUNC] = 2;
      InstSize[OPCODE_DEPTH_MASK] = 2;
      InstSize[OPCODE_DEPTH_RANGE] = 3;
      InstSize[OPCODE_DISABLE] = 2;
      InstSize[OPCODE_DRAW_BUFFER] = 2;
      InstSize[OPCODE_DRAW_PIXELS] = 2;
      InstSize[OPCODE_ENABLE] = 2;
      InstSize[OPCODE_EVALCOORD1] = 2;
      InstSize[OPCODE_EVALCOORD2] = 3;
      InstSize[OPCODE_EVALMESH1] = 4;
      InstSize[OPCODE_EVALMESH2] = 6;
      InstSize[OPCODE_EVALPOINT1] = 2;
      InstSize[OPCODE_EVALPOINT2] = 3;
      InstSize[OPCODE_FOG] = 6;
      InstSize[OPCODE_FRONT_FACE] = 2;
      InstSize[OPCODE_FRUSTUM] = 7;
      InstSize[OPCODE_HINT] = 3;
      InstSize[OPCODE_INDEX_MASK] = 2;
      InstSize[OPCODE_INIT_NAMES] = 1;
      InstSize[OPCODE_LIGHT] = 7;
      InstSize[OPCODE_LIGHT_MODEL] = 6;
      InstSize[OPCODE_LINE_STIPPLE] = 3;
      InstSize[OPCODE_LINE_WIDTH] = 2;
      InstSize[OPCODE_LIST_BASE] = 2;
      InstSize[OPCODE_LOAD_IDENTITY] = 1;
      InstSize[OPCODE_LOAD_MATRIX] = 17;
      InstSize[OPCODE_LOAD_NAME] = 2;
      InstSize[OPCODE_LOGIC_OP] = 2;
      InstSize[OPCODE_MAP1] = 7;
      InstSize[OPCODE_MAP2] = 11;
      InstSize[OPCODE_MAPGRID1] = 4;
      InstSize[OPCODE_MAPGRID2] = 7;
      InstSize[OPCODE_MATRIX_MODE] = 2;
      InstSize[OPCODE_MULT_MATRIX] = 17;
      InstSize[OPCODE_ORTHO] = 7;
      InstSize[OPCODE_PASSTHROUGH] = 2;
      InstSize[OPCODE_PIXEL_MAP] = 4;
      InstSize[OPCODE_PIXEL_TRANSFER] = 3;
      InstSize[OPCODE_PIXEL_ZOOM] = 3;
      InstSize[OPCODE_POINT_SIZE] = 2;
      InstSize[OPCODE_POINT_PARAMETERS] = 5;
      InstSize[OPCODE_POLYGON_MODE] = 3;
      InstSize[OPCODE_POLYGON_STIPPLE] = 2;
      InstSize[OPCODE_POLYGON_OFFSET] = 3;
      InstSize[OPCODE_POP_ATTRIB] = 1;
      InstSize[OPCODE_POP_MATRIX] = 1;
      InstSize[OPCODE_POP_NAME] = 1;
      InstSize[OPCODE_PRIORITIZE_TEXTURE] = 3;
      InstSize[OPCODE_PUSH_ATTRIB] = 2;
      InstSize[OPCODE_PUSH_MATRIX] = 1;
      InstSize[OPCODE_PUSH_NAME] = 2;
      InstSize[OPCODE_RASTER_POS] = 5;
      InstSize[OPCODE_RECTF] = 5;
      InstSize[OPCODE_READ_BUFFER] = 2;
      InstSize[OPCODE_SCALE] = 4;
      InstSize[OPCODE_SCISSOR] = 5;
      InstSize[OPCODE_STENCIL_FUNC] = 4;
      InstSize[OPCODE_STENCIL_MASK] = 2;
      InstSize[OPCODE_STENCIL_OP] = 4;
      InstSize[OPCODE_SHADE_MODEL] = 2;
      InstSize[OPCODE_TEXENV] = 7;
      InstSize[OPCODE_TEXGEN] = 7;
      InstSize[OPCODE_TEXPARAMETER] = 7;
      InstSize[OPCODE_TEX_IMAGE1D] = 9;
      InstSize[OPCODE_TEX_IMAGE2D] = 10;
      InstSize[OPCODE_TEX_IMAGE3D] = 11;
      InstSize[OPCODE_TEX_SUB_IMAGE1D] = 8;
      InstSize[OPCODE_TEX_SUB_IMAGE2D] = 10;
      InstSize[OPCODE_TEX_SUB_IMAGE3D] = 12;
      InstSize[OPCODE_TRANSLATE] = 4;
      InstSize[OPCODE_VIEWPORT] = 5;
      InstSize[OPCODE_WINDOW_POS] = 5;
      InstSize[OPCODE_CONTINUE] = 2;
      InstSize[OPCODE_ERROR] = 3;
      InstSize[OPCODE_VERTEX_CASSETTE] = 2;
      InstSize[OPCODE_END_OF_LIST] = 1;
      /* GL_ARB_multitexture */
      InstSize[OPCODE_ACTIVE_TEXTURE] = 2;
      InstSize[OPCODE_CLIENT_ACTIVE_TEXTURE] = 2;
   }
   init_flag = 1;
}


/*
 * Display List compilation functions
 */



static void save_Accum( GLcontext *ctx, GLenum op, GLfloat value )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ACCUM, 2 );
   if (n) {
      n[1].e = op;
      n[2].f = value;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Accum)( ctx, op, value );
   }
}


static void save_AlphaFunc( GLcontext *ctx, GLenum func, GLclampf ref )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ALPHA_FUNC, 2 );
   if (n) {
      n[1].e = func;
      n[2].f = (GLfloat) ref;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.AlphaFunc)( ctx, func, ref );
   }
}

static void save_BindTexture( GLcontext *ctx, GLenum target, GLuint texture )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BIND_TEXTURE, 2 );
   if (n) {
      n[1].e = target;
      n[2].ui = texture;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.BindTexture)( ctx, target, texture );
   }
}


static void save_Bitmap( GLcontext *ctx,
                         GLsizei width, GLsizei height,
                         GLfloat xorig, GLfloat yorig,
                         GLfloat xmove, GLfloat ymove,
                         const GLubyte *bitmap,
                         const struct gl_pixelstore_attrib *packing )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BITMAP, 7 );
   if (n) {
      struct gl_image *image = gl_unpack_bitmap( ctx, width, height,
                                                 bitmap, packing );
      if (image) {
         image->RefCount = 1;
      }
      n[1].i = (GLint) width;
      n[2].i = (GLint) height;
      n[3].f = xorig;
      n[4].f = yorig;
      n[5].f = xmove;
      n[6].f = ymove;
      n[7].data = (void *) image;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Bitmap)( ctx, width, height,
                           xorig, yorig, xmove, ymove, bitmap, packing );
   }
}


static void save_BlendEquation( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_EQUATION, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.BlendEquation)( ctx, mode );
   }
}


static void save_BlendFunc( GLcontext *ctx, GLenum sfactor, GLenum dfactor )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_FUNC, 2 );
   if (n) {
      n[1].e = sfactor;
      n[2].e = dfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.BlendFunc)( ctx, sfactor, dfactor );
   }
}


static void save_BlendFuncSeparate( GLcontext *ctx,
                                GLenum sfactorRGB, GLenum dfactorRGB,
                                GLenum sfactorA, GLenum dfactorA)
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_FUNC_SEPARATE, 4 );
   if (n) {
      n[1].e = sfactorRGB;
      n[2].e = dfactorRGB;
      n[3].e = sfactorA;
      n[4].e = dfactorA;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.BlendFuncSeparate)( ctx, sfactorRGB, dfactorRGB,
                                      sfactorA, dfactorA);
   }
}


static void save_BlendColor( GLcontext *ctx, GLfloat red, GLfloat green,
                         GLfloat blue, GLfloat alpha )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_COLOR, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.BlendColor)( ctx, red, green, blue, alpha );
   }
}


static void save_CallList( GLcontext *ctx, GLuint list )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CALL_LIST, 1 );
   if (n) {
      n[1].ui = list;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CallList)( ctx, list );
   }
}


static void save_CallLists( GLcontext *ctx,
                        GLsizei n, GLenum type, const GLvoid *lists )
{
   GLint i;
   FLUSH_VB(ctx, "dlist");

   for (i=0;i<n;i++) {
      GLuint list = translate_id( i, type, lists );
      Node *n = alloc_instruction( ctx, OPCODE_CALL_LIST_OFFSET, 1 );
      if (n) {
         n[1].ui = list;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CallLists)( ctx, n, type, lists );
   }
}


static void save_Clear( GLcontext *ctx, GLbitfield mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Clear)( ctx, mask );
   }
}


static void save_ClearAccum( GLcontext *ctx, GLfloat red, GLfloat green,
			 GLfloat blue, GLfloat alpha )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_ACCUM, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClearAccum)( ctx, red, green, blue, alpha );
   }
}


static void save_ClearColor( GLcontext *ctx, GLclampf red, GLclampf green,
			 GLclampf blue, GLclampf alpha )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_COLOR, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClearColor)( ctx, red, green, blue, alpha );
   }
}


static void save_ClearDepth( GLcontext *ctx, GLclampd depth )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_DEPTH, 1 );
   if (n) {
      n[1].f = (GLfloat) depth;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClearDepth)( ctx, depth );
   }
}


static void save_ClearIndex( GLcontext *ctx, GLfloat c )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_INDEX, 1 );
   if (n) {
      n[1].f = c;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClearIndex)( ctx, c );
   }
}


static void save_ClearStencil( GLcontext *ctx, GLint s )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_STENCIL, 1 );
   if (n) {
      n[1].i = s;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClearStencil)( ctx, s );
   }
}


static void save_ClipPlane( GLcontext *ctx, GLenum plane, const GLfloat *equ )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLIP_PLANE, 5 );
   if (n) {
      n[1].e = plane;
      n[2].f = equ[0];
      n[3].f = equ[1];
      n[4].f = equ[2];
      n[5].f = equ[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClipPlane)( ctx, plane, equ );
   }
}



static void save_ColorMask( GLcontext *ctx, GLboolean red, GLboolean green,
                        GLboolean blue, GLboolean alpha )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_MASK, 4 );
   if (n) {
      n[1].b = red;
      n[2].b = green;
      n[3].b = blue;
      n[4].b = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ColorMask)( ctx, red, green, blue, alpha );
   }
}


static void save_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_MATERIAL, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ColorMaterial)( ctx, face, mode );
   }
}


static void save_ColorTable( GLcontext *ctx, GLenum target, GLenum internalFormat,
                         struct gl_image *table )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_TABLE, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].data = (GLvoid *) table;
      if (table) {
         /* must retain this image */
         table->RefCount = 1;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ColorTable)( ctx, target, internalFormat, table );
   }
}


static void save_ColorSubTable( GLcontext *ctx, GLenum target,
                            GLsizei start, struct gl_image *data )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_SUB_TABLE, 3 );
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].data = (GLvoid *) data;
      if (data) {
         /* must retain this image */
         data->RefCount = 1;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ColorSubTable)( ctx, target, start, data );
   }
}



static void save_CopyPixels( GLcontext *ctx, GLint x, GLint y,
			 GLsizei width, GLsizei height, GLenum type )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_PIXELS, 5 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
      n[5].e = type;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyPixels)( ctx, x, y, width, height, type );
   }
}



static void save_CopyTexImage1D( GLcontext *ctx,
                             GLenum target, GLint level,
                             GLenum internalformat,
                             GLint x, GLint y, GLsizei width,
                             GLint border )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_TEX_IMAGE1D, 7 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].e = internalformat;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
      n[7].i = border;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyTexImage1D)( ctx, target, level, internalformat,
                            x, y, width, border );
   }
}


static void save_CopyTexImage2D( GLcontext *ctx,
                             GLenum target, GLint level,
                             GLenum internalformat,
                             GLint x, GLint y, GLsizei width,
                             GLsizei height, GLint border )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_TEX_IMAGE2D, 8 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].e = internalformat;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
      n[7].i = height;
      n[8].i = border;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyTexImage2D)( ctx, target, level, internalformat,
                            x, y, width, height, border );
   }
}



static void save_CopyTexSubImage1D( GLcontext *ctx,
                                GLenum target, GLint level,
                                GLint xoffset, GLint x, GLint y,
                                GLsizei width )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_TEX_SUB_IMAGE1D, 6 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyTexSubImage1D)( ctx, target, level, xoffset, x, y, width );
   }
}


static void save_CopyTexSubImage2D( GLcontext *ctx,
                                GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLint x, GLint y,
                                GLsizei width, GLint height )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_TEX_SUB_IMAGE2D, 8 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = x;
      n[6].i = y;
      n[7].i = width;
      n[8].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyTexSubImage2D)( ctx, target, level, xoffset, yoffset,
                               x, y, width, height );
   }
}


static void save_CopyTexSubImage3DEXT( GLcontext *ctx,
                                   GLenum target, GLint level,
                                   GLint xoffset, GLint yoffset, GLint zoffset,
                                   GLint x, GLint y,
                                   GLsizei width, GLint height )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_TEX_SUB_IMAGE3D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = zoffset;
      n[6].i = x;
      n[7].i = y;
      n[8].i = width;
      n[9].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CopyTexSubImage3DEXT)( ctx, target, level, xoffset, yoffset, zoffset,
                               x, y, width, height );
   }
}


static void save_CullFace( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CULL_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.CullFace)( ctx, mode );
   }
}


static void save_DepthFunc( GLcontext *ctx, GLenum func )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_FUNC, 1 );
   if (n) {
      n[1].e = func;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.DepthFunc)( ctx, func );
   }
}


static void save_DepthMask( GLcontext *ctx, GLboolean mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_MASK, 1 );
   if (n) {
      n[1].b = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.DepthMask)( ctx, mask );
   }
}


static void save_DepthRange( GLcontext *ctx, GLclampd nearval, GLclampd farval )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_RANGE, 2 );
   if (n) {
      n[1].f = (GLfloat) nearval;
      n[2].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.DepthRange)( ctx, nearval, farval );
   }
}


static void save_Disable( GLcontext *ctx, GLenum cap )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DISABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Disable)( ctx, cap );
   }
}


static void save_DrawBuffer( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DRAW_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.DrawBuffer)( ctx, mode );
   }
}


static void save_DrawPixels( GLcontext *ctx, struct gl_image *image )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DRAW_PIXELS, 1 );
   if (n) {
      n[1].data = (GLvoid *) image;
   }
   if (image) {
      image->RefCount = 1;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.DrawPixels)( ctx, image );
   }
}



static void save_Enable( GLcontext *ctx, GLenum cap )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ENABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Enable)( ctx, cap );
   }
}



static void save_EvalMesh1( GLcontext *ctx,
                        GLenum mode, GLint i1, GLint i2 )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_EVALMESH1, 3 );
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.EvalMesh1)( ctx, mode, i1, i2 );
   }
}


static void save_EvalMesh2( GLcontext *ctx, 
                        GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_EVALMESH2, 5 );
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
      n[4].i = j1;
      n[5].i = j2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.EvalMesh2)( ctx, mode, i1, i2, j1, j2 );
   }
}




static void save_Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_FOG, 5 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Fogfv)( ctx, pname, params );
   }
}


static void save_FrontFace( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_FRONT_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.FrontFace)( ctx, mode );
   }
}


static void save_Frustum( GLcontext *ctx, GLdouble left, GLdouble right,
                      GLdouble bottom, GLdouble top,
                      GLdouble nearval, GLdouble farval )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_FRUSTUM, 6 );
   if (n) {
      n[1].f = left;
      n[2].f = right;
      n[3].f = bottom;
      n[4].f = top;
      n[5].f = nearval;
      n[6].f = farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Frustum)( ctx, left, right, bottom, top, nearval, farval );
   }
}


static GLboolean save_Hint( GLcontext *ctx, GLenum target, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_HINT, 2 );
   if (n) {
      n[1].e = target;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      return (*ctx->Exec.Hint)( ctx, target, mode );
   }
   return GL_TRUE;		/* not queried */
}



static void save_IndexMask( GLcontext *ctx, GLuint mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_INDEX_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.IndexMask)( ctx, mask );
   }
}


static void save_InitNames( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_INIT_NAMES, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.InitNames)( ctx );
   }
}


static void save_Lightfv( GLcontext *ctx, GLenum light, GLenum pname,
                      const GLfloat *params, GLint numparams )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LIGHT, 6 );
   if (OPCODE_LIGHT) {
      GLint i;
      n[1].e = light;
      n[2].e = pname;
      for (i=0;i<numparams;i++) {
	 n[3+i].f = params[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Lightfv)( ctx, light, pname, params, numparams );
   }
}


static void save_LightModelfv( GLcontext *ctx,
                           GLenum pname, const GLfloat *params )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LIGHT_MODEL, 5 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LightModelfv)( ctx, pname, params );
   }
}


static void save_LineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LINE_STIPPLE, 2 );
   if (n) {
      n[1].i = factor;
      n[2].us = pattern;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LineStipple)( ctx, factor, pattern );
   }
}


static void save_LineWidth( GLcontext *ctx, GLfloat width )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LINE_WIDTH, 1 );
   if (n) {
      n[1].f = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LineWidth)( ctx, width );
   }
}


static void save_ListBase( GLcontext *ctx, GLuint base )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LIST_BASE, 1 );
   if (n) {
      n[1].ui = base;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ListBase)( ctx, base );
   }
}


static void save_LoadIdentity( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_LOAD_IDENTITY, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LoadIdentity)( ctx );
   }
}


static void save_LoadMatrixf( GLcontext *ctx, const GLfloat *m )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LOAD_MATRIX, 16 );
   if (n) {
      GLuint i;
      for (i=0;i<16;i++) {
	 n[1+i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LoadMatrixf)( ctx, m );
   }
}


static void save_LoadName( GLcontext *ctx, GLuint name )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LOAD_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LoadName)( ctx, name );
   }
}


static void save_LogicOp( GLcontext *ctx, GLenum opcode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LOGIC_OP, 1 );
   if (n) {
      n[1].e = opcode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.LogicOp)( ctx, opcode );
   }
}


static void save_Map1f( GLcontext *ctx,
                   GLenum target, GLfloat u1, GLfloat u2, GLint stride,
		   GLint order, const GLfloat *points, GLboolean retain )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP1, 6 );
   if (n) {
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = stride;
      n[5].i = order;
      n[6].data = (void *) points;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Map1f)( ctx, target, u1, u2, stride, order, points, GL_TRUE );
   }
   (void) retain;
}


static void save_Map2f( GLcontext *ctx, GLenum target,
                    GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                    GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                    const GLfloat *points, GLboolean retain )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP2, 10 );
   if (n) {
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].f = v1;
      n[5].f = v2;
      n[6].i = ustride;
      n[7].i = vstride;
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) points;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Map2f)( ctx, target,
                        u1, u2, ustride, uorder,
                        v1, v2, vstride, vorder, points, GL_TRUE );
   }
   (void) retain;
}


static void save_MapGrid1f( GLcontext *ctx, GLint un, GLfloat u1, GLfloat u2 )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAPGRID1, 3 );
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.MapGrid1f)( ctx, un, u1, u2 );
   }
}


static void save_MapGrid2f( GLcontext *ctx, 
                        GLint un, GLfloat u1, GLfloat u2,
		        GLint vn, GLfloat v1, GLfloat v2 )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAPGRID2, 6 );
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = vn;
      n[5].f = v1;
      n[6].f = v2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.MapGrid2f)( ctx, un, u1, u2, vn, v1, v2 );
   }
}


static void save_MatrixMode( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MATRIX_MODE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.MatrixMode)( ctx, mode );
   }
}


static void save_MultMatrixf( GLcontext *ctx, const GLfloat *m )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MULT_MATRIX, 16 );
   if (n) {
      GLuint i;
      for (i=0;i<16;i++) {
	 n[1+i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.MultMatrixf)( ctx, m );
   }
}


static void save_NewList( GLcontext *ctx, GLuint list, GLenum mode )
{
   /* It's an error to call this function while building a display list */
   gl_error( ctx, GL_INVALID_OPERATION, "glNewList" );
   (void) list;
   (void) mode;
}



static void save_Ortho( GLcontext *ctx, GLdouble left, GLdouble right,
                    GLdouble bottom, GLdouble top,
                    GLdouble nearval, GLdouble farval )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ORTHO, 6 );
   if (n) {
      n[1].f = left;
      n[2].f = right;
      n[3].f = bottom;
      n[4].f = top;
      n[5].f = nearval;
      n[6].f = farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Ortho)( ctx, left, right, bottom, top, nearval, farval );
   }
}


static void save_PixelMapfv( GLcontext *ctx,
                         GLenum map, GLint mapsize, const GLfloat *values )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_MAP, 3 );
   if (n) {
      n[1].e = map;
      n[2].i = mapsize;
      n[3].data  = (void *) malloc( mapsize * sizeof(GLfloat) );
      MEMCPY( n[3].data, (void *) values, mapsize * sizeof(GLfloat) );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PixelMapfv)( ctx, map, mapsize, values );
   }
}


static void save_PixelTransferf( GLcontext *ctx, GLenum pname, GLfloat param )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_TRANSFER, 2 );
   if (n) {
      n[1].e = pname;
      n[2].f = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PixelTransferf)( ctx, pname, param );
   }
}


static void save_PixelZoom( GLcontext *ctx, GLfloat xfactor, GLfloat yfactor )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_ZOOM, 2 );
   if (n) {
      n[1].f = xfactor;
      n[2].f = yfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PixelZoom)( ctx, xfactor, yfactor );
   }
}


static void save_PointParameterfvEXT( GLcontext *ctx, GLenum pname,
                                  const GLfloat *params)
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POINT_PARAMETERS, 4 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PointParameterfvEXT)( ctx, pname, params );
   }
}


static void save_PointSize( GLcontext *ctx, GLfloat size )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POINT_SIZE, 1 );
   if (n) {
      n[1].f = size;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PointSize)( ctx, size );
   }
}


static void save_PolygonMode( GLcontext *ctx, GLenum face, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_MODE, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PolygonMode)( ctx, face, mode );
   }
}


/*
 * Polygon stipple must have been upacked already!
 */
static void save_PolygonStipple( GLcontext *ctx, const GLuint *pattern )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_STIPPLE, 1 );
   if (n) {
      void *data;
      n[1].data = malloc( 32 * 4 );
      data = n[1].data;   /* This needed for Acorn compiler */
      MEMCPY( data, pattern, 32 * 4 );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PolygonStipple)( ctx, pattern );
   }
}


static void save_PolygonOffset( GLcontext *ctx, GLfloat factor, GLfloat units )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_OFFSET, 2 );
   if (n) {
      n[1].f = factor;
      n[2].f = units;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PolygonOffset)( ctx, factor, units );
   }
}


static void save_PopAttrib( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_ATTRIB, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PopAttrib)( ctx );
   }
}


static void save_PopMatrix( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PopMatrix)( ctx );
   }
}


static void save_PopName( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_NAME, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PopName)( ctx );
   }
}


static void save_PrioritizeTextures( GLcontext *ctx,
                                 GLsizei num, const GLuint *textures,
                                 const GLclampf *priorities )
{
   GLint i;
   FLUSH_VB(ctx, "dlist");

   for (i=0;i<num;i++) {
      Node *n;
      n = alloc_instruction( ctx,  OPCODE_PRIORITIZE_TEXTURE, 2 );
      if (n) {
         n[1].ui = textures[i];
         n[2].f = priorities[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PrioritizeTextures)( ctx, num, textures, priorities );
   }
}


static void save_PushAttrib( GLcontext *ctx, GLbitfield mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PUSH_ATTRIB, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PushAttrib)( ctx, mask );
   }
}


static void save_PushMatrix( GLcontext *ctx )
{
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_PUSH_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PushMatrix)( ctx );
   }
}


static void save_PushName( GLcontext *ctx, GLuint name )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PUSH_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PushName)( ctx, name );
   }
}


static void save_RasterPos4f( GLcontext *ctx,
                          GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_RASTER_POS, 4 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.RasterPos4f)( ctx, x, y, z, w );
   }
}


static void save_PassThrough( GLcontext *ctx, GLfloat token )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PASSTHROUGH, 1 );
   if (n) {
      n[1].f = token;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.PassThrough)( ctx, token );
   }
}


static void save_ReadBuffer( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_READ_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ReadBuffer)( ctx, mode );
   }
}


static void save_Rectf( GLcontext *ctx,
                    GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_RECTF, 4 );
   if (n) {
      n[1].f = x1;
      n[2].f = y1;
      n[3].f = x2;
      n[4].f = y2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Rectf)( ctx, x1, y1, x2, y2 );
   }
}


static void save_Rotatef( GLcontext *ctx, GLfloat angle,
                      GLfloat x, GLfloat y, GLfloat z )
{
   GLfloat m[16];
   gl_rotation_matrix( angle, x, y, z, m );
   save_MultMatrixf( ctx, m );  /* save and maybe execute */
}


static void save_Scalef( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_SCALE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Scalef)( ctx, x, y, z );
   }
}


static void save_Scissor( GLcontext *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_SCISSOR, 4 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = width;
      n[4].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Scissor)( ctx, x, y, width, height );
   }
}


static void save_ShadeModel( GLcontext *ctx, GLenum mode )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_SHADE_MODEL, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ShadeModel)( ctx, mode );
   }
}


static void save_StencilFunc( GLcontext *ctx, GLenum func, GLint ref, GLuint mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_FUNC, 3 );
   if (n) {
      n[1].e = func;
      n[2].i = ref;
      n[3].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.StencilFunc)( ctx, func, ref, mask );
   }
}


static void save_StencilMask( GLcontext *ctx, GLuint mask )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.StencilMask)( ctx, mask );
   }
}


static void save_StencilOp( GLcontext *ctx,
                        GLenum fail, GLenum zfail, GLenum zpass )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_OP, 3 );
   if (n) {
      n[1].e = fail;
      n[2].e = zfail;
      n[3].e = zpass;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.StencilOp)( ctx, fail, zfail, zpass );
   }
}




static void save_TexEnvfv( GLcontext *ctx,
                       GLenum target, GLenum pname, const GLfloat *params )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEXENV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexEnvfv)( ctx, target, pname, params );
   }
}


static void save_TexGenfv( GLcontext *ctx,
                       GLenum coord, GLenum pname, const GLfloat *params )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEXGEN, 6 );
   if (n) {
      n[1].e = coord;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexGenfv)( ctx, coord, pname, params );
   }
}


static void save_TexParameterfv( GLcontext *ctx, GLenum target,
                             GLenum pname, const GLfloat *params )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEXPARAMETER, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexParameterfv)( ctx, target, pname, params );
   }
}


static void save_TexImage1D( GLcontext *ctx, GLenum target,
                         GLint level, GLint components,
			 GLsizei width, GLint border,
                         GLenum format, GLenum type,
			 struct gl_image *teximage )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_IMAGE1D, 8 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = components;
      n[4].i = (GLint) width;
      n[5].i = border;
      n[6].e = format;
      n[7].e = type;
      n[8].data = teximage;
      if (teximage) {
         /* this prevents gl_TexImage2D() from freeing the image */
         teximage->RefCount = 1;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexImage1D)( ctx, target, level, components, width,
                        border, format, type, teximage );
   }
}


static void save_TexImage2D( GLcontext *ctx, GLenum target,
                         GLint level, GLint components,
			 GLsizei width, GLsizei height, GLint border,
                         GLenum format, GLenum type,
			 struct gl_image *teximage )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_IMAGE2D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = components;
      n[4].i = (GLint) width;
      n[5].i = (GLint) height;
      n[6].i = border;
      n[7].e = format;
      n[8].e = type;
      n[9].data = teximage;
      if (teximage) {
         /* this prevents gl_TexImage2D() from freeing the image */
         teximage->RefCount = 1;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexImage2D)( ctx, target, level, components, width,
                        height, border, format, type, teximage );
   }
}


static void save_TexImage3DEXT( GLcontext *ctx, GLenum target,
                            GLint level, GLint components,
                            GLsizei width, GLsizei height, GLsizei depth,
                            GLint border,
                            GLenum format, GLenum type,
                            struct gl_image *teximage )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_IMAGE3D, 10 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = components;
      n[4].i = (GLint) width;
      n[5].i = (GLint) height;
      n[6].i = (GLint) depth; 
      n[7].i = border;
      n[8].e = format;
      n[9].e = type;
      n[10].data = teximage;
      if (teximage) {
         /* this prevents gl_TexImage3D() from freeing the image */
         teximage->RefCount = 1;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexImage3DEXT)( ctx, target, level, components, width,
                           height, depth, border, format, type, teximage );
   }
}


static void save_TexSubImage1D( GLcontext *ctx,
                            GLenum target, GLint level, GLint xoffset,
                            GLsizei width, GLenum format, GLenum type,
                            struct gl_image *image )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_SUB_IMAGE1D, 7 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = (GLint) width;
      n[5].e = format;
      n[6].e = type;
      n[7].data = image;
      if (image)
         image->RefCount = 1;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexSubImage1D)( ctx, target, level, xoffset, width,
                           format, type, image );
   }
}


static void save_TexSubImage2D( GLcontext *ctx,
                            GLenum target, GLint level,
                            GLint xoffset, GLint yoffset,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            struct gl_image *image )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_SUB_IMAGE2D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = (GLint) width;
      n[6].i = (GLint) height;
      n[7].e = format;
      n[8].e = type;
      n[9].data = image;
      if (image)
         image->RefCount = 1;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexSubImage2D)( ctx, target, level, xoffset, yoffset,
                           width, height, format, type, image );
   }
}


static void save_TexSubImage3DEXT( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLint xoffset, GLint yoffset,GLint zoffset,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLenum format, GLenum type,
                               struct gl_image *image )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_TEX_SUB_IMAGE3D, 11 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = zoffset;
      n[6].i = (GLint) width;
      n[7].i = (GLint) height;
      n[8].i = (GLint) depth;
      n[9].e = format;
      n[10].e = type;
      n[11].data = image;
      if (image)
         image->RefCount = 1;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.TexSubImage3DEXT)( ctx, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type, image );
   }
}


static void save_Translatef( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx,  OPCODE_TRANSLATE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Translatef)( ctx, x, y, z );
   }
}



static void save_Viewport( GLcontext *ctx,
                       GLint x, GLint y, GLsizei width, GLsizei height )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx,  OPCODE_VIEWPORT, 4 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.Viewport)( ctx, x, y, width, height );
   }
}


static void save_WindowPos4fMESA( GLcontext *ctx,
                              GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx,  OPCODE_WINDOW_POS, 4 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.WindowPos4fMESA)( ctx, x, y, z, w );
   }
}






/* GL_ARB_multitexture */
static void save_ActiveTexture( GLcontext *ctx, GLenum target )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ACTIVE_TEXTURE, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ActiveTexture)( ctx, target );
   }
}


/* GL_ARB_multitexture */
static void save_ClientActiveTexture( GLcontext *ctx, GLenum target )
{
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLIENT_ACTIVE_TEXTURE, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec.ClientActiveTexture)( ctx, target );
   }
}





void gl_compile_cassette( GLcontext *ctx )
{
   Node *n = alloc_instruction( ctx, OPCODE_VERTEX_CASSETTE, 1 );
   struct immediate *new_im = gl_immediate_alloc(ctx);
   struct immediate *im = ctx->input;
   
   if (!n || !new_im) {
      if (n) free(n);
      if (new_im) gl_immediate_free(new_im);
      return;
   }

   /* Do some easy optimizations of the cassette.  
    */
   if (im->v.Obj.size < 4 && 
       im->Count > 15)
   {
      im->Bounds = (GLfloat (*)[3]) malloc(6 * sizeof(GLfloat));
      (gl_calc_bound_tab[im->v.Obj.size])( im->Bounds, &im->v.Obj );
   }


   n[1].data = (void *)im;   
   SET_IMMEDIATE( ctx, new_im );
}

/* KW: Compile commands  
 * 
 * Will appear in the list before the vertex buffer containing the
 * command that provoked the error.  I don't see this as a problem.  
 */
void gl_save_error( GLcontext *ctx, GLenum error, const char *s )
{
   Node *n;
   n = alloc_instruction( ctx, OPCODE_ERROR, 2 );
   if (n) {
      n[1].e = error;
      n[2].data = (void *) s;
   }
   /* execute already done */
}

/**********************************************************************/
/*                     Display list execution                         */
/**********************************************************************/


/*
 * Execute a display list.  Note that the ListBase offset must have already
 * been added before calling this function.  I.e. the list argument is
 * the absolute list number, not relative to ListBase.
 * Input:  list - display list number
 */
static void execute_list( GLcontext *ctx, GLuint list )
{
   Node *n;
   GLboolean done;
   OpCode opcode;

   if (!gl_IsList(ctx,list))
      return;

/*    mesa_print_display_list( list ); */

   ctx->CallDepth++;

   n = (Node *) HashLookup(ctx->Shared->DisplayList, list);

   done = GL_FALSE;
   while (!done) {
      opcode = n[0].opcode;

      switch (opcode) {
         case OPCODE_ERROR:
 	    gl_error( ctx, n[1].e, (const char *) n[2].data ); 
            break;
         case OPCODE_VERTEX_CASSETTE:
	    if (ctx->NewState)
	       gl_update_state(ctx);
	    if (ctx->CompileCVAFlag) {
	       ctx->CompileCVAFlag = 0;
	       ctx->CVA.elt.pipeline_valid = 0;
	    }
	    if (!ctx->CVA.elt.pipeline_valid)
	       gl_build_immediate_pipeline( ctx );

	    if ((MESA_VERBOSE & VERBOSE_DISPLAY_LIST) &&
		(MESA_VERBOSE & VERBOSE_IMMEDIATE))
	       gl_print_cassette( (struct immediate *) n[1].data, 0, ~0 );

	    gl_fixup_cassette( ctx, (struct immediate *) n[1].data ); 
	    gl_execute_cassette( ctx, (struct immediate *) n[1].data ); 
            break;
         case OPCODE_ACCUM:
	    gl_Accum( ctx, n[1].e, n[2].f );
	    break;
         case OPCODE_ALPHA_FUNC:
	    gl_AlphaFunc( ctx, n[1].e, n[2].f );
	    break;
         case OPCODE_BIND_TEXTURE:
            gl_BindTexture( ctx, n[1].e, n[2].ui );
            break;
	 case OPCODE_BITMAP:
            {
               static struct gl_pixelstore_attrib defaultPacking = {
                  1,            /* Alignment */
                  0,            /* RowLength */
                  0,            /* SkipPixels */
                  0,            /* SkipRows */
                  0,            /* ImageHeight */
                  0,            /* SkipImages */
                  GL_FALSE,     /* SwapBytes */
                  GL_FALSE      /* LsbFirst */
               };
               const struct gl_image *image = (struct gl_image *) n[7].data;
               const GLubyte *bitmap = image ? image->Data : NULL;
               gl_Bitmap( ctx, (GLsizei) n[1].i, (GLsizei) n[2].i,
                          n[3].f, n[4].f, n[5].f, n[6].f,
                          bitmap, &defaultPacking );
            }
	    break;
	 case OPCODE_BLEND_COLOR:
	    gl_BlendColor( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_BLEND_EQUATION:
	    gl_BlendEquation( ctx, n[1].e );
	    break;
	 case OPCODE_BLEND_FUNC:
	    gl_BlendFunc( ctx, n[1].e, n[2].e );
	    break;
	 case OPCODE_BLEND_FUNC_SEPARATE:
	    gl_BlendFuncSeparate( ctx, n[1].e, n[2].e, n[3].e, n[4].e );
	    break;
         case OPCODE_CALL_LIST:
	    /* Generated by glCallList(), don't add ListBase */
            if (ctx->CallDepth<MAX_LIST_NESTING) {
               execute_list( ctx, n[1].ui );
            }
            break;
         case OPCODE_CALL_LIST_OFFSET:
	    /* Generated by glCallLists() so we must add ListBase */
            if (ctx->CallDepth<MAX_LIST_NESTING) {
               execute_list( ctx, ctx->List.ListBase + n[1].ui );
            }
            break;
	 case OPCODE_CLEAR:
	    gl_Clear( ctx, n[1].bf );
	    break;
	 case OPCODE_CLEAR_COLOR:
	    gl_ClearColor( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_ACCUM:
	    gl_ClearAccum( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_DEPTH:
	    gl_ClearDepth( ctx, (GLclampd) n[1].f );
	    break;
	 case OPCODE_CLEAR_INDEX:
	    gl_ClearIndex( ctx, n[1].ui );
	    break;
	 case OPCODE_CLEAR_STENCIL:
	    gl_ClearStencil( ctx, n[1].i );
	    break;
         case OPCODE_CLIP_PLANE:
            {
               GLfloat equ[4];
               equ[0] = n[2].f;
               equ[1] = n[3].f;
               equ[2] = n[4].f;
               equ[3] = n[5].f;
               gl_ClipPlane( ctx, n[1].e, equ );
            }
            break;
	 case OPCODE_COLOR_MASK:
	    gl_ColorMask( ctx, n[1].b, n[2].b, n[3].b, n[4].b );
	    break;
	 case OPCODE_COLOR_MATERIAL:
	    gl_ColorMaterial( ctx, n[1].e, n[2].e );
	    break;
         case OPCODE_COLOR_TABLE:
            gl_ColorTable( ctx, n[1].e, n[2].e, (struct gl_image *) n[3].data);
            break;
         case OPCODE_COLOR_SUB_TABLE:
            gl_ColorSubTable( ctx, n[1].e, n[2].i,
                              (struct gl_image *) n[3].data);
            break;
	 case OPCODE_COPY_PIXELS:
	    gl_CopyPixels( ctx, n[1].i, n[2].i,
			   (GLsizei) n[3].i, (GLsizei) n[4].i, n[5].e );
	    break;
         case OPCODE_COPY_TEX_IMAGE1D:
	    gl_CopyTexImage1D( ctx, n[1].e, n[2].i, n[3].e, n[4].i,
                               n[5].i, n[6].i, n[7].i );
            break;
         case OPCODE_COPY_TEX_IMAGE2D:
	    gl_CopyTexImage2D( ctx, n[1].e, n[2].i, n[3].e, n[4].i,
                               n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE1D:
	    gl_CopyTexSubImage1D( ctx, n[1].e, n[2].i, n[3].i, n[4].i,
                                  n[5].i, n[6].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE2D:
	    gl_CopyTexSubImage2D( ctx, n[1].e, n[2].i, n[3].i, n[4].i,
                                  n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE3D:
            gl_CopyTexSubImage3DEXT( ctx, n[1].e, n[2].i, n[3].i, n[4].i,
                                     n[5].i, n[6].i, n[7].i, n[8].i , n[9].i);
            break;
	 case OPCODE_CULL_FACE:
	    gl_CullFace( ctx, n[1].e );
	    break;
	 case OPCODE_DEPTH_FUNC:
	    gl_DepthFunc( ctx, n[1].e );
	    break;
	 case OPCODE_DEPTH_MASK:
	    gl_DepthMask( ctx, n[1].b );
	    break;
	 case OPCODE_DEPTH_RANGE:
	    gl_DepthRange( ctx, (GLclampd) n[1].f, (GLclampd) n[2].f );
	    break;
	 case OPCODE_DISABLE:
	    gl_Disable( ctx, n[1].e );
	    break;
	 case OPCODE_DRAW_BUFFER:
	    gl_DrawBuffer( ctx, n[1].e );
	    break;
	 case OPCODE_DRAW_PIXELS:
	    gl_DrawPixels( ctx, (struct gl_image *) n[1].data );
	    break;
	 case OPCODE_ENABLE:
	    gl_Enable( ctx, n[1].e );
	    break;
	 case OPCODE_EVALMESH1:
	    gl_EvalMesh1( ctx, n[1].e, n[2].i, n[3].i );
	    break;
	 case OPCODE_EVALMESH2:
	    gl_EvalMesh2( ctx, n[1].e, n[2].i, n[3].i, n[4].i, n[5].i );
	    break;
	 case OPCODE_FOG:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       gl_Fogfv( ctx, n[1].e, p );
	    }
	    break;
	 case OPCODE_FRONT_FACE:
	    gl_FrontFace( ctx, n[1].e );
	    break;
         case OPCODE_FRUSTUM:
            gl_Frustum( ctx, n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_HINT:
	    gl_Hint( ctx, n[1].e, n[2].e );
	    break;
	 case OPCODE_INDEX_MASK:
	    gl_IndexMask( ctx, n[1].ui );
	    break;
	 case OPCODE_INIT_NAMES:
	    gl_InitNames( ctx );
	    break;
         case OPCODE_LIGHT:
	    {
	       GLfloat p[4];
	       p[0] = n[3].f;
	       p[1] = n[4].f;
	       p[2] = n[5].f;
	       p[3] = n[6].f;
	       gl_Lightfv( ctx, n[1].e, n[2].e, p, 4 );
	    }
	    break;
         case OPCODE_LIGHT_MODEL:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       gl_LightModelfv( ctx, n[1].e, p );
	    }
	    break;
	 case OPCODE_LINE_STIPPLE:
	    gl_LineStipple( ctx, n[1].i, n[2].us );
	    break;
	 case OPCODE_LINE_WIDTH:
	    gl_LineWidth( ctx, n[1].f );
	    break;
	 case OPCODE_LIST_BASE:
	    gl_ListBase( ctx, n[1].ui );
	    break;
	 case OPCODE_LOAD_IDENTITY:
            gl_LoadIdentity( ctx );
            break;
	 case OPCODE_LOAD_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       gl_LoadMatrixf( ctx, &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       gl_LoadMatrixf( ctx, m );
	    }
	    break;
	 case OPCODE_LOAD_NAME:
	    gl_LoadName( ctx, n[1].ui );
	    break;
	 case OPCODE_LOGIC_OP:
	    gl_LogicOp( ctx, n[1].e );
	    break;
	 case OPCODE_MAP1:
	    gl_Map1f( ctx, n[1].e, n[2].f, n[3].f,
                      n[4].i, n[5].i, (GLfloat *) n[6].data, GL_TRUE );
	    break;
	 case OPCODE_MAP2:
	    gl_Map2f( ctx, n[1].e,
                      n[2].f, n[3].f,  /* u1, u2 */
		      n[6].i, n[8].i,  /* ustride, uorder */
		      n[4].f, n[5].f,  /* v1, v2 */
		      n[7].i, n[9].i,  /* vstride, vorder */
		      (GLfloat *) n[10].data,
                      GL_TRUE);
	    break;
	 case OPCODE_MAPGRID1:
	    gl_MapGrid1f( ctx, n[1].i, n[2].f, n[3].f );
	    break;
	 case OPCODE_MAPGRID2:
	    gl_MapGrid2f( ctx, n[1].i, n[2].f, n[3].f, n[4].i, n[5].f, n[6].f);
	    break;
         case OPCODE_MATRIX_MODE:
            gl_MatrixMode( ctx, n[1].e );
            break;
	 case OPCODE_MULT_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       gl_MultMatrixf( ctx, &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       gl_MultMatrixf( ctx, m );
	    }
	    break;
         case OPCODE_ORTHO:
            gl_Ortho( ctx, n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_PASSTHROUGH:
	    gl_PassThrough( ctx, n[1].f );
	    break;
	 case OPCODE_PIXEL_MAP:
	    gl_PixelMapfv( ctx, n[1].e, n[2].i, (GLfloat *) n[3].data );
	    break;
	 case OPCODE_PIXEL_TRANSFER:
	    gl_PixelTransferf( ctx, n[1].e, n[2].f );
	    break;
	 case OPCODE_PIXEL_ZOOM:
	    gl_PixelZoom( ctx, n[1].f, n[2].f );
	    break;
	 case OPCODE_POINT_SIZE:
	    gl_PointSize( ctx, n[1].f );
	    break;
	 case OPCODE_POINT_PARAMETERS:
	    {
		GLfloat params[3];
		params[0] = n[2].f;
		params[1] = n[3].f;
		params[2] = n[4].f;
		gl_PointParameterfvEXT( ctx, n[1].e, params ); 
	    }
	    break;
	 case OPCODE_POLYGON_MODE:
	    gl_PolygonMode( ctx, n[1].e, n[2].e );
	    break;
	 case OPCODE_POLYGON_STIPPLE:
	    gl_PolygonStipple( ctx, (GLuint *) n[1].data );
	    break;
	 case OPCODE_POLYGON_OFFSET:
	    gl_PolygonOffset( ctx, n[1].f, n[2].f );
	    break;
	 case OPCODE_POP_ATTRIB:
	    gl_PopAttrib( ctx );
	    break;
	 case OPCODE_POP_MATRIX:
	    gl_PopMatrix( ctx );
	    break;
	 case OPCODE_POP_NAME:
	    gl_PopName( ctx );
	    break;
	 case OPCODE_PRIORITIZE_TEXTURE:
            gl_PrioritizeTextures( ctx, 1, &n[1].ui, &n[2].f );
	    break;
	 case OPCODE_PUSH_ATTRIB:
	    gl_PushAttrib( ctx, n[1].bf );
	    break;
	 case OPCODE_PUSH_MATRIX:
	    gl_PushMatrix( ctx );
	    break;
	 case OPCODE_PUSH_NAME:
	    gl_PushName( ctx, n[1].ui );
	    break;
	 case OPCODE_RASTER_POS:
            gl_RasterPos4f( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_READ_BUFFER:
	    gl_ReadBuffer( ctx, n[1].e );
	    break;
         case OPCODE_RECTF:
            gl_Rectf( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
            break;
         case OPCODE_SCALE:
            gl_Scalef( ctx, n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_SCISSOR:
	    gl_Scissor( ctx, n[1].i, n[2].i, n[3].i, n[4].i );
	    break;
	 case OPCODE_SHADE_MODEL:
	    gl_ShadeModel( ctx, n[1].e );
	    break;
	 case OPCODE_STENCIL_FUNC:
	    gl_StencilFunc( ctx, n[1].e, n[2].i, n[3].ui );
	    break;
	 case OPCODE_STENCIL_MASK:
	    gl_StencilMask( ctx, n[1].ui );
	    break;
	 case OPCODE_STENCIL_OP:
	    gl_StencilOp( ctx, n[1].e, n[2].e, n[3].e );
	    break;
         case OPCODE_TEXENV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               gl_TexEnvfv( ctx, n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXGEN:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               gl_TexGenfv( ctx, n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXPARAMETER:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               gl_TexParameterfv( ctx, n[1].e, n[2].e, params );
            }
            break;
	 case OPCODE_TEX_IMAGE1D:
	    gl_TexImage1D( ctx,
                           n[1].e, /* target */
                           n[2].i, /* level */
                           n[3].i, /* components */
                           n[4].i, /* width */
                           n[5].e, /* border */
                           n[6].e, /* format */
                           n[7].e, /* type */
                           (struct gl_image *) n[8].data );
	    break;
	 case OPCODE_TEX_IMAGE2D:
	    gl_TexImage2D( ctx,
                           n[1].e, /* target */
                           n[2].i, /* level */
                           n[3].i, /* components */
                           n[4].i, /* width */
                           n[5].i, /* height */
                           n[6].e, /* border */
                           n[7].e, /* format */
                           n[8].e, /* type */
                           (struct gl_image *) n[9].data );
	    break;
         case OPCODE_TEX_IMAGE3D:
            gl_TexImage3DEXT( ctx,
                              n[1].e, /* target */
                              n[2].i, /* level */
                              n[3].i, /* components */
                              n[4].i, /* width */
                              n[5].i, /* height */
                              n[6].i, /* depth  */
                              n[7].e, /* border */
                              n[8].e, /* format */
                              n[9].e, /* type */
                              (struct gl_image *) n[10].data );
            break;
         case OPCODE_TEX_SUB_IMAGE1D:
            gl_TexSubImage1D( ctx, n[1].e, n[2].i, n[3].i, n[4].i, n[5].e,
                              n[6].e, (struct gl_image *) n[7].data );
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            gl_TexSubImage2D( ctx, n[1].e, n[2].i, n[3].i, n[4].i, n[5].e,
                              n[6].i, n[7].e, n[8].e,
                              (struct gl_image *) n[9].data );
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            gl_TexSubImage3DEXT( ctx, n[1].e, n[2].i, n[3].i, n[4].i, n[5].i,
                                 n[6].i, n[7].i, n[8].i, n[9].e, n[10].e,
                                 (struct gl_image *) n[11].data );
            break;
         case OPCODE_TRANSLATE:
            gl_Translatef( ctx, n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_VIEWPORT:
	    gl_Viewport( ctx,
                         n[1].i, n[2].i, (GLsizei) n[3].i, (GLsizei) n[4].i );
	    break;
	 case OPCODE_WINDOW_POS:
            gl_WindowPos4fMESA( ctx, n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
         case OPCODE_ACTIVE_TEXTURE:  /* GL_ARB_multitexture */
            gl_ActiveTexture( ctx, n[1].e );
            break;
         case OPCODE_CLIENT_ACTIVE_TEXTURE:  /* GL_ARB_multitexture */
            gl_ClientActiveTexture( ctx, n[1].e );
            break;
	 case OPCODE_CONTINUE:
	    n = (Node *) n[1].next;
	    break;
	 case OPCODE_END_OF_LIST:
	    done = GL_TRUE;
	    break;
	 default:
            {
               char msg[1000];
               sprintf(msg, "Error in execute_list: opcode=%d", (int) opcode);
               gl_problem( ctx, msg );
            }
            done = GL_TRUE;
      }

      /* increment n to point to next compiled command */
      if (opcode!=OPCODE_CONTINUE) {
	 n += InstSize[opcode];
      }

   }
   ctx->CallDepth--;
}





/**********************************************************************/
/*                           GL functions                             */
/**********************************************************************/




/*
 * Test if a display list number is valid.
 */
GLboolean gl_IsList( GLcontext *ctx, GLuint list )
{
   if (list > 0 && HashLookup(ctx->Shared->DisplayList, list)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/*
 * Delete a sequence of consecutive display lists.
 */
void gl_DeleteLists( GLcontext *ctx, GLuint list, GLsizei range )
{
   GLuint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDeleteLists");
   if (range<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glDeleteLists" );
      return;
   }
   for (i=list;i<list+range;i++) {
      gl_destroy_list( ctx, i );
   }
}



/*
 * Return a display list number, n, such that lists n through n+range-1
 * are free.
 */
GLuint gl_GenLists( GLcontext *ctx, GLsizei range )
{
   GLuint base;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glGenLists", 0);
   if (range<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glGenLists" );
      return 0;
   }
   if (range==0) {
      return 0;
   }

   base = HashFindFreeKeyBlock(ctx->Shared->DisplayList, range);
   if (base) {
      /* reserve the list IDs by with empty/dummy lists */
      GLint i;
      for (i=0; i<range; i++) {
         HashInsert(ctx->Shared->DisplayList, base+i, make_empty_list());
      }
   }
   return base;
}



/*
 * Begin a new display list.
 */
void gl_NewList( GLcontext *ctx, GLuint list, GLenum mode )
{
   struct immediate *IM;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glNewList");

   if (MESA_VERBOSE&VERBOSE_API)
      fprintf(stderr, "glNewList %u %s\n", list, gl_lookup_enum_by_nr(mode));

   if (list==0) {
      gl_error( ctx, GL_INVALID_VALUE, "glNewList" );
      return;
   }

   if (mode!=GL_COMPILE && mode!=GL_COMPILE_AND_EXECUTE) {
      gl_error( ctx, GL_INVALID_ENUM, "glNewList" );
      return;
   }

   if (ctx->CurrentListPtr) {
      /* already compiling a display list */
      gl_error( ctx, GL_INVALID_OPERATION, "glNewList" );
      return;
   }

   /* Allocate new display list */
   ctx->CurrentListNum = list;
   ctx->CurrentBlock = (Node *) malloc( sizeof(Node) * BLOCK_SIZE );
   ctx->CurrentListPtr = ctx->CurrentBlock;
   ctx->CurrentPos = 0;

   IM = gl_immediate_alloc( ctx );
   SET_IMMEDIATE( ctx, IM );
   gl_reset_input( ctx );

   ctx->CompileFlag = GL_TRUE;
   ctx->CompileCVAFlag = GL_FALSE;
   ctx->ExecuteFlag = (mode == GL_COMPILE_AND_EXECUTE);
   ctx->API = ctx->Save;  /* Switch the API function pointers */
}



/*
 * End definition of current display list.
 */
void gl_EndList( GLcontext *ctx )
{
   if (MESA_VERBOSE&VERBOSE_API)
      fprintf(stderr, "glEndList\n");

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx, "glEndList" );

   /* Check that a list is under construction */
   if (!ctx->CurrentListPtr) {
      gl_error( ctx, GL_INVALID_OPERATION, "glEndList" );
      return;
   }

   (void) alloc_instruction( ctx, OPCODE_END_OF_LIST, 0 );

   /* Destroy old list, if any */
   gl_destroy_list(ctx, ctx->CurrentListNum);
   /* Install the list */
   HashInsert(ctx->Shared->DisplayList, ctx->CurrentListNum, ctx->CurrentListPtr);


   if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST)
      mesa_print_display_list(ctx->CurrentListNum);

   ctx->CurrentListNum = 0;
   ctx->CurrentListPtr = NULL;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   /* ctx->CompileCVAFlag = ...; */

   /* KW: Put back the old input pointer.
    */
   free( ctx->input );
   SET_IMMEDIATE( ctx, ctx->VB->IM );
   gl_reset_input( ctx );

   /* Haven't tracked down why this is needed.
    */
   ctx->NewState = ~0;

   ctx->API = ctx->Exec;   /* Switch the API function pointers */
}



void gl_CallList( GLcontext *ctx, GLuint list )
{
   /* VERY IMPORTANT:  Save the CompileFlag status, turn it off, */
   /* execute the display list, and restore the CompileFlag. */
   GLboolean save_compile_flag;

   if (MESA_VERBOSE&VERBOSE_API) {
      fprintf(stderr, "glCallList %u\n", list);
      mesa_print_display_list( list ); 
   }

   save_compile_flag = ctx->CompileFlag;   
   ctx->CompileFlag = GL_FALSE;
   
   FLUSH_VB( ctx, "call list" );
   execute_list( ctx, list );
   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag)
      ctx->API = ctx->Save;
}



/*
 * Execute glCallLists:  call multiple display lists.
 */
void gl_CallLists( GLcontext *ctx,
                   GLsizei n, GLenum type, const GLvoid *lists )
{
   GLuint list;
   GLint i;
   GLboolean save_compile_flag;

   /* Save the CompileFlag status, turn it off, execute display list,
    * and restore the CompileFlag.
    */
   save_compile_flag = ctx->CompileFlag;
   ctx->CompileFlag = GL_FALSE;

   FLUSH_VB( ctx, "call lists" );

   for (i=0;i<n;i++) {
      list = translate_id( i, type, lists );
      execute_list( ctx, ctx->List.ListBase + list );
   }

   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag)
           ctx->API = ctx->Save;


/*    RESET_IMMEDIATE( ctx ); */
}



/*
 * Set the offset added to list numbers in glCallLists.
 */
void gl_ListBase( GLcontext *ctx, GLuint base )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glListBase");
   ctx->List.ListBase = base;
}






/*
 * Assign all the pointers in 'table' to point to Mesa's display list
 * building functions.
 */
void gl_init_dlist_pointers( struct gl_api_table *table )
{
   table->Accum = save_Accum;
   table->AlphaFunc = save_AlphaFunc;
   table->AreTexturesResident = gl_AreTexturesResident;
   table->BindTexture = save_BindTexture;
   table->Bitmap = save_Bitmap;
   table->BlendColor = save_BlendColor;
   table->BlendEquation = save_BlendEquation;
   table->BlendFunc = save_BlendFunc;
   table->BlendFuncSeparate = save_BlendFuncSeparate;
   table->CallList = save_CallList;
   table->CallLists = save_CallLists;
   table->Clear = save_Clear;
   table->ClearAccum = save_ClearAccum;
   table->ClearColor = save_ClearColor;
   table->ClearDepth = save_ClearDepth;
   table->ClearIndex = save_ClearIndex;
   table->ClearStencil = save_ClearStencil;
   table->ClipPlane = save_ClipPlane;
   table->ColorMask = save_ColorMask;
   table->ColorMaterial = save_ColorMaterial;
   table->ColorTable = save_ColorTable;
   table->ColorSubTable = save_ColorSubTable;
   table->CopyPixels = save_CopyPixels;
   table->CopyTexImage1D = save_CopyTexImage1D;
   table->CopyTexImage2D = save_CopyTexImage2D;
   table->CopyTexSubImage1D = save_CopyTexSubImage1D;
   table->CopyTexSubImage2D = save_CopyTexSubImage2D;
   table->CopyTexSubImage3DEXT = save_CopyTexSubImage3DEXT;
   table->CullFace = save_CullFace;
   table->DeleteLists = gl_DeleteLists;   /* NOT SAVED */
   table->DeleteTextures = gl_DeleteTextures;  /* NOT SAVED */
   table->DepthFunc = save_DepthFunc;
   table->DepthMask = save_DepthMask;
   table->DepthRange = save_DepthRange;
   table->Disable = save_Disable;
   table->DisableClientState = gl_DisableClientState;  /* NOT SAVED */
   table->DrawBuffer = save_DrawBuffer;
   table->DrawPixels = save_DrawPixels;
   table->Enable = save_Enable;
   table->Error = gl_save_error;
   table->EnableClientState = gl_EnableClientState;   /* NOT SAVED */
   table->EndList = gl_EndList;   /* NOT SAVED */
   table->EvalMesh1 = save_EvalMesh1;
   table->EvalMesh2 = save_EvalMesh2;
   table->FeedbackBuffer = gl_FeedbackBuffer;   /* NOT SAVED */
   table->Finish = gl_Finish;   /* NOT SAVED */
   table->Flush = gl_Flush;   /* NOT SAVED */
   table->Fogfv = save_Fogfv;
   table->FrontFace = save_FrontFace;
   table->Frustum = save_Frustum;
   table->GenLists = gl_GenLists;   /* NOT SAVED */
   table->GenTextures = gl_GenTextures;   /* NOT SAVED */

   /* NONE OF THESE COMMANDS ARE COMPILED INTO DISPLAY LISTS */
   table->GetBooleanv = gl_GetBooleanv;
   table->GetClipPlane = gl_GetClipPlane;
   table->GetColorTable = gl_GetColorTable;
   table->GetColorTableParameteriv = gl_GetColorTableParameteriv;
   table->GetDoublev = gl_GetDoublev;
   table->GetError = gl_GetError;
   table->GetFloatv = gl_GetFloatv;
   table->GetIntegerv = gl_GetIntegerv;
   table->GetString = gl_GetString;
   table->GetLightfv = gl_GetLightfv;
   table->GetLightiv = gl_GetLightiv;
   table->GetMapdv = gl_GetMapdv;
   table->GetMapfv = gl_GetMapfv;
   table->GetMapiv = gl_GetMapiv;
   table->GetMaterialfv = gl_GetMaterialfv;
   table->GetMaterialiv = gl_GetMaterialiv;
   table->GetPixelMapfv = gl_GetPixelMapfv;
   table->GetPixelMapuiv = gl_GetPixelMapuiv;
   table->GetPixelMapusv = gl_GetPixelMapusv;
   table->GetPointerv = gl_GetPointerv;
   table->GetPolygonStipple = gl_GetPolygonStipple;
   table->GetTexEnvfv = gl_GetTexEnvfv;
   table->GetTexEnviv = gl_GetTexEnviv;
   table->GetTexGendv = gl_GetTexGendv;
   table->GetTexGenfv = gl_GetTexGenfv;
   table->GetTexGeniv = gl_GetTexGeniv;
   table->GetTexImage = gl_GetTexImage;
   table->GetTexLevelParameterfv = gl_GetTexLevelParameterfv;
   table->GetTexLevelParameteriv = gl_GetTexLevelParameteriv;
   table->GetTexParameterfv = gl_GetTexParameterfv;
   table->GetTexParameteriv = gl_GetTexParameteriv;

   table->Hint = save_Hint;
   table->IndexMask = save_IndexMask;
   table->InitNames = save_InitNames;
   table->IsEnabled = gl_IsEnabled;   /* NOT SAVED */
   table->IsTexture = gl_IsTexture;   /* NOT SAVED */
   table->IsList = gl_IsList;   /* NOT SAVED */
   table->LightModelfv = save_LightModelfv;
   table->Lightfv = save_Lightfv;
   table->LineStipple = save_LineStipple;
   table->LineWidth = save_LineWidth;
   table->ListBase = save_ListBase;
   table->LoadIdentity = save_LoadIdentity;
   table->LoadMatrixf = save_LoadMatrixf;
   table->LoadName = save_LoadName;
   table->LogicOp = save_LogicOp;
   table->Map1f = save_Map1f;
   table->Map2f = save_Map2f;
   table->MapGrid1f = save_MapGrid1f;
   table->MapGrid2f = save_MapGrid2f;
   table->MatrixMode = save_MatrixMode;
   table->MultMatrixf = save_MultMatrixf;
   table->NewList = save_NewList;
   table->Ortho = save_Ortho;
   table->PointParameterfvEXT = save_PointParameterfvEXT;
   table->PassThrough = save_PassThrough;
   table->PixelMapfv = save_PixelMapfv;
   table->PixelStorei = gl_PixelStorei;   /* NOT SAVED */
   table->PixelTransferf = save_PixelTransferf;
   table->PixelZoom = save_PixelZoom;
   table->PointSize = save_PointSize;
   table->PolygonMode = save_PolygonMode;
   table->PolygonOffset = save_PolygonOffset;
   table->PolygonStipple = save_PolygonStipple;
   table->PopAttrib = save_PopAttrib;
   table->PopClientAttrib = gl_PopClientAttrib;  /* NOT SAVED */
   table->PopMatrix = save_PopMatrix;
   table->PopName = save_PopName;
   table->PrioritizeTextures = save_PrioritizeTextures;
   table->PushAttrib = save_PushAttrib;
   table->PushClientAttrib = gl_PushClientAttrib;  /* NOT SAVED */
   table->PushMatrix = save_PushMatrix;
   table->PushName = save_PushName;
   table->RasterPos4f = save_RasterPos4f;
   table->ReadBuffer = save_ReadBuffer;
   table->ReadPixels = gl_ReadPixels;   /* NOT SAVED */
   table->Rectf = save_Rectf;
   table->RenderMode = gl_RenderMode;   /* NOT SAVED */
   table->Rotatef = save_Rotatef;
   table->Scalef = save_Scalef;
   table->Scissor = save_Scissor;
   table->SelectBuffer = gl_SelectBuffer;   /* NOT SAVED */
   table->ShadeModel = save_ShadeModel;
   table->StencilFunc = save_StencilFunc;
   table->StencilMask = save_StencilMask;
   table->StencilOp = save_StencilOp;
   table->TexEnvfv = save_TexEnvfv;
   table->TexGenfv = save_TexGenfv;
   table->TexImage1D = save_TexImage1D;
   table->TexImage2D = save_TexImage2D;
   table->TexImage3DEXT = save_TexImage3DEXT;
   table->TexSubImage1D = save_TexSubImage1D;
   table->TexSubImage2D = save_TexSubImage2D;
   table->TexSubImage3DEXT = save_TexSubImage3DEXT;
   table->TexParameterfv = save_TexParameterfv;
   table->Translatef = save_Translatef;
   table->Viewport = save_Viewport;

   /* GL_MESA_window_pos extension */
   table->WindowPos4fMESA = save_WindowPos4fMESA;

   /* GL_MESA_resize_buffers extension */
   table->ResizeBuffersMESA = gl_ResizeBuffersMESA;

   /* GL_ARB_multitexture */
   table->ActiveTexture = save_ActiveTexture;
   table->ClientActiveTexture = save_ClientActiveTexture;

   /* GL_EXT_get_proc_address */
   table->GetProcAddress = gl_GetProcAddress;  /* NOT SAVED */
}



/***
 *** Debugging code
 ***/
static const char *enum_string( GLenum k )
{
   return gl_lookup_enum_by_nr( k );
}


/*
 * Print the commands in a display list.  For debugging only.
 * TODO: many commands aren't handled yet.
 */
static void print_list( GLcontext *ctx, FILE *f, GLuint list )
{
   Node *n;
   GLboolean done;
   OpCode opcode;

   if (!glIsList(list)) {
      fprintf(f,"%u is not a display list ID\n",list);
      return;
   }

   n = (Node *) HashLookup(ctx->Shared->DisplayList, list);

   fprintf( f, "START-LIST %u, address %p\n", list, (void*)n );

   done = n ? GL_FALSE : GL_TRUE;
   while (!done) {
      opcode = n[0].opcode;

      switch (opcode) {
         case OPCODE_ACCUM:
            fprintf(f,"accum %s %g\n", enum_string(n[1].e), n[2].f );
	    break;
	 case OPCODE_BITMAP:
            fprintf(f,"Bitmap %d %d %g %g %g %g %p\n", n[1].i, n[2].i,
		       n[3].f, n[4].f, n[5].f, n[6].f, (void *) n[7].data );
	    break;
         case OPCODE_CALL_LIST:
            fprintf(f,"CallList %d\n", (int) n[1].ui );
            break;
         case OPCODE_CALL_LIST_OFFSET:
            fprintf(f,"CallList %d + offset %u = %u\n", (int) n[1].ui,
                    ctx->List.ListBase, ctx->List.ListBase + n[1].ui );
            break;
	 case OPCODE_DISABLE:
            fprintf(f,"Disable %s\n", enum_string(n[1].e));
	    break;
	 case OPCODE_ENABLE:
            fprintf(f,"Enable %s\n", enum_string(n[1].e));
	    break;
         case OPCODE_FRUSTUM:
            fprintf(f,"Frustum %g %g %g %g %g %g\n",
                    n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_LINE_STIPPLE:
	    fprintf(f,"LineStipple %d %x\n", n[1].i, (int) n[2].us );
	    break;
	 case OPCODE_LOAD_IDENTITY:
            fprintf(f,"LoadIdentity\n");
	    break;
	 case OPCODE_LOAD_MATRIX:
            fprintf(f,"LoadMatrix\n");
            fprintf(f,"  %8f %8f %8f %8f\n", n[1].f, n[5].f,  n[9].f, n[13].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[2].f, n[6].f, n[10].f, n[14].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[3].f, n[7].f, n[11].f, n[15].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[4].f, n[8].f, n[12].f, n[16].f);
	    break;
	 case OPCODE_MULT_MATRIX:
            fprintf(f,"MultMatrix (or Rotate)\n");
            fprintf(f,"  %8f %8f %8f %8f\n", n[1].f, n[5].f,  n[9].f, n[13].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[2].f, n[6].f, n[10].f, n[14].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[3].f, n[7].f, n[11].f, n[15].f);
            fprintf(f,"  %8f %8f %8f %8f\n", n[4].f, n[8].f, n[12].f, n[16].f);
	    break;
         case OPCODE_ORTHO:
            fprintf(f,"Ortho %g %g %g %g %g %g\n",
                    n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_POP_ATTRIB:
            fprintf(f,"PopAttrib\n");
	    break;
	 case OPCODE_POP_MATRIX:
            fprintf(f,"PopMatrix\n");
	    break;
	 case OPCODE_POP_NAME:
            fprintf(f,"PopName\n");
	    break;
	 case OPCODE_PUSH_ATTRIB:
            fprintf(f,"PushAttrib %x\n", n[1].bf );
	    break;
	 case OPCODE_PUSH_MATRIX:
            fprintf(f,"PushMatrix\n");
	    break;
	 case OPCODE_PUSH_NAME:
            fprintf(f,"PushName %d\n", (int) n[1].ui );
	    break;
	 case OPCODE_RASTER_POS:
            fprintf(f,"RasterPos %g %g %g %g\n", n[1].f, n[2].f,n[3].f,n[4].f);
	    break;
         case OPCODE_RECTF:
            fprintf( f, "Rectf %g %g %g %g\n", n[1].f, n[2].f, n[3].f, n[4].f);
            break;
         case OPCODE_SCALE:
            fprintf(f,"Scale %g %g %g\n", n[1].f, n[2].f, n[3].f );
            break;
         case OPCODE_TRANSLATE:
            fprintf(f,"Translate %g %g %g\n", n[1].f, n[2].f, n[3].f );
            break;

	 /*
	  * meta opcodes/commands
	  */
         case OPCODE_ERROR:
            fprintf(f,"Error: %s %s\n", enum_string(n[1].e), (const char *)n[2].data );
            break;
	 case OPCODE_VERTEX_CASSETTE:
            fprintf(f,"VERTEX-CASSETTE, id %u, %u elements\n", 
		    ((struct immediate *) n[1].data)->id,
		    ((struct immediate *) n[1].data)->Count - VB_START );
	    break;
	 case OPCODE_CONTINUE:
            fprintf(f,"DISPLAY-LIST-CONTINUE\n");
	    n = (Node *) n[1].next;
	    break;
	 case OPCODE_END_OF_LIST:
            fprintf(f,"END-LIST %u\n", list);
	    done = GL_TRUE;
	    break;
         default:
            if (opcode < 0 || opcode > OPCODE_END_OF_LIST) {
               fprintf(f,"ERROR IN DISPLAY LIST: opcode = %d, address = %p\n",
                       opcode, (void*) n);
               return;
            }
            else {
               fprintf(f,"command %d, %u operands\n",opcode,InstSize[opcode]);
            }
      }

      /* increment n to point to next compiled command */
      if (opcode!=OPCODE_CONTINUE) {
	 n += InstSize[opcode];
      }
   }
}








/*
 * Clients may call this function to help debug display list problems.
 * This function is _ONLY_FOR_DEBUGGING_PURPOSES_.  It may be removed,
 * changed, or break in the future without notice.
 */
void mesa_print_display_list( GLuint list )
{
   GET_CONTEXT;
   print_list( CC, stdout, list );
}
