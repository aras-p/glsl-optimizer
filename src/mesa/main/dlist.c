/* $Id: dlist.c,v 1.41 2000/05/24 15:04:45 brianp Exp $ */

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
#include "attrib.h"
#include "bitmap.h"
#include "bbox.h"
#include "blend.h"
#include "buffers.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "copypix.h"
#include "cva.h"
#include "depth.h"
#include "enable.h"
#include "enums.h"
#include "eval.h"
#include "extensions.h"
#include "feedback.h"
#include "get.h"
#include "glapi.h"
#include "hash.h"
#include "image.h"
#include "imaging.h"
#include "light.h"
#include "lines.h"
#include "dlist.h"
#include "macros.h"
#include "matrix.h"
#include "mem.h"
#include "pipeline.h"
#include "pixel.h"
#include "pixeltex.h"
#include "points.h"
#include "polygon.h"
#include "readpix.h"
#include "rect.h"
#include "state.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "types.h"
#include "varray.h"
#include "vb.h"
#include "vbfill.h"
#include "vbxform.h"
#include "xform.h"
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
	OPCODE_COLOR_TABLE_PARAMETER_FV,
	OPCODE_COLOR_TABLE_PARAMETER_IV,
	OPCODE_COLOR_SUB_TABLE,
        OPCODE_CONVOLUTION_FILTER_1D,
        OPCODE_CONVOLUTION_FILTER_2D,
        OPCODE_CONVOLUTION_PARAMETER_I,
        OPCODE_CONVOLUTION_PARAMETER_IV,
        OPCODE_CONVOLUTION_PARAMETER_F,
        OPCODE_CONVOLUTION_PARAMETER_FV,
        OPCODE_COPY_COLOR_SUB_TABLE,
        OPCODE_COPY_COLOR_TABLE,
	OPCODE_COPY_PIXELS,
        OPCODE_COPY_TEX_IMAGE1D,
        OPCODE_COPY_TEX_IMAGE2D,
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
	OPCODE_HINT_PGI,
	OPCODE_HISTOGRAM,
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
        OPCODE_MIN_MAX,
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
        OPCODE_RESET_HISTOGRAM,
        OPCODE_RESET_MIN_MAX,
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
        /* GL_SGIX/SGIS_pixel_texture */
        OPCODE_PIXEL_TEXGEN_SGIX,
        OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS,
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
      newblock = (Node *) MALLOC( sizeof(Node) * BLOCK_SIZE );
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
   Node *n = (Node *) MALLOC( sizeof(Node) );
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

   block = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);
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
            FREE(n[6].data);
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_MAP2:
            FREE(n[10].data);
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_DRAW_PIXELS:
	    FREE( n[5].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_BITMAP:
	    FREE( n[7].data );
	    n += InstSize[n[0].opcode];
	    break;
         case OPCODE_COLOR_TABLE:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COLOR_SUB_TABLE:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            FREE( n[7].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_POLYGON_STIPPLE:
            FREE( n[1].data );
	    n += InstSize[n[0].opcode];
            break;
	 case OPCODE_TEX_IMAGE1D:
            FREE(n[8].data);
            n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_TEX_IMAGE2D:
            FREE( n[9]. data );
            n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_TEX_IMAGE3D:
            FREE( n[10]. data );
            n += InstSize[n[0].opcode];
	    break;
         case OPCODE_TEX_SUB_IMAGE1D:
            FREE(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            FREE(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            FREE(n[11].data);
            n += InstSize[n[0].opcode];
            break;
	 case OPCODE_CONTINUE:
	    n = (Node *) n[1].next;
	    FREE( block );
	    block = n;
	    break;
	 case OPCODE_END_OF_LIST:
	    FREE( block );
	    done = GL_TRUE;
	    break;
	 default:
	    /* Most frequent case */
	    n += InstSize[n[0].opcode];
	    break;
      }
   }

   _mesa_HashRemove(ctx->Shared->DisplayList, list);
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
      InstSize[OPCODE_COLOR_TABLE] = 7;
      InstSize[OPCODE_COLOR_TABLE_PARAMETER_FV] = 7;
      InstSize[OPCODE_COLOR_TABLE_PARAMETER_IV] = 7;
      InstSize[OPCODE_COLOR_SUB_TABLE] = 7;
      InstSize[OPCODE_CONVOLUTION_FILTER_1D] = 7;
      InstSize[OPCODE_CONVOLUTION_FILTER_2D] = 8;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_I] = 4;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_IV] = 7;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_F] = 4;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_FV] = 7;
      InstSize[OPCODE_COPY_PIXELS] = 6;
      InstSize[OPCODE_COPY_COLOR_SUB_TABLE] = 6;
      InstSize[OPCODE_COPY_COLOR_TABLE] = 6;
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
      InstSize[OPCODE_DRAW_PIXELS] = 6;
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
      InstSize[OPCODE_HINT_PGI] = 3;
      InstSize[OPCODE_HISTOGRAM] = 5;
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
      InstSize[OPCODE_MIN_MAX] = 4;
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
      InstSize[OPCODE_RESET_HISTOGRAM] = 2;
      InstSize[OPCODE_RESET_MIN_MAX] = 2;
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
      InstSize[OPCODE_VERTEX_CASSETTE] = 9;
      InstSize[OPCODE_END_OF_LIST] = 1;
      /* GL_SGIX/SGIS_pixel_texture */
      InstSize[OPCODE_PIXEL_TEXGEN_SGIX] = 2;
      InstSize[OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS] = 3,
      /* GL_ARB_multitexture */
      InstSize[OPCODE_ACTIVE_TEXTURE] = 2;
      InstSize[OPCODE_CLIENT_ACTIVE_TEXTURE] = 2;
   }
   init_flag = 1;
}


/*
 * Display List compilation functions
 */



static void save_Accum( GLenum op, GLfloat value )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ACCUM, 2 );
   if (n) {
      n[1].e = op;
      n[2].f = value;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Accum)( op, value );
   }
}


static void save_AlphaFunc( GLenum func, GLclampf ref )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ALPHA_FUNC, 2 );
   if (n) {
      n[1].e = func;
      n[2].f = (GLfloat) ref;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->AlphaFunc)( func, ref );
   }
}


static void save_Begin( GLenum mode )
{
   _mesa_Begin(mode);  /* special case */
}


static void save_BindTexture( GLenum target, GLuint texture )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BIND_TEXTURE, 2 );
   if (n) {
      n[1].e = target;
      n[2].ui = texture;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BindTexture)( target, texture );
   }
}


static void save_Bitmap( GLsizei width, GLsizei height,
                         GLfloat xorig, GLfloat yorig,
                         GLfloat xmove, GLfloat ymove,
                         const GLubyte *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_bitmap( width, height, pixels, &ctx->Unpack );
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BITMAP, 7 );
   if (n) {
      n[1].i = (GLint) width;
      n[2].i = (GLint) height;
      n[3].f = xorig;
      n[4].f = yorig;
      n[5].f = xmove;
      n[6].f = ymove;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Bitmap)( width, height,
                           xorig, yorig, xmove, ymove, pixels );
   }
}


static void save_BlendEquation( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_EQUATION, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendEquation)( mode );
   }
}


static void save_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_BLEND_FUNC, 2 );
   if (n) {
      n[1].e = sfactor;
      n[2].e = dfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendFunc)( sfactor, dfactor );
   }
}


static void save_BlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB,
                                      GLenum sfactorA, GLenum dfactorA)
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->BlendFuncSeparateEXT)( sfactorRGB, dfactorRGB,
                                          sfactorA, dfactorA);
   }
}


static void save_BlendColor( GLfloat red, GLfloat green,
                             GLfloat blue, GLfloat alpha )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->BlendColor)( red, green, blue, alpha );
   }
}


static void save_CallList( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CALL_LIST, 1 );
   if (n) {
      n[1].ui = list;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CallList)( list );
   }
}


static void save_CallLists( GLsizei n, GLenum type, const GLvoid *lists )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CallLists)( n, type, lists );
   }
}


static void save_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Clear)( mask );
   }
}


static void save_ClearAccum( GLfloat red, GLfloat green,
                             GLfloat blue, GLfloat alpha )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->ClearAccum)( red, green, blue, alpha );
   }
}


static void save_ClearColor( GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->ClearColor)( red, green, blue, alpha );
   }
}


static void save_ClearDepth( GLclampd depth )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_DEPTH, 1 );
   if (n) {
      n[1].f = (GLfloat) depth;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearDepth)( depth );
   }
}


static void save_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_INDEX, 1 );
   if (n) {
      n[1].f = c;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearIndex)( c );
   }
}


static void save_ClearStencil( GLint s )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLEAR_STENCIL, 1 );
   if (n) {
      n[1].i = s;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearStencil)( s );
   }
}


static void save_ClipPlane( GLenum plane, const GLdouble *equ )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->ClipPlane)( plane, equ );
   }
}



static void save_ColorMask( GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->ColorMask)( red, green, blue, alpha );
   }
}


static void save_ColorMaterial( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_MATERIAL, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorMaterial)( face, mode );
   }
}


static void save_ColorTable( GLenum target, GLenum internalFormat,
                             GLsizei width, GLenum format, GLenum type,
                             const GLvoid *table )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D ||
       target == GL_PROXY_TEXTURE_2D ||
       target == GL_PROXY_TEXTURE_3D) {
      /* execute immediately */
      (*ctx->Exec->ColorTable)( target, internalFormat, width,
                                format, type, table );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type, table,
                                         &ctx->Unpack);
      Node *n;
      FLUSH_VB(ctx, "dlist");
      n = alloc_instruction( ctx, OPCODE_COLOR_TABLE, 6 );
      if (n) {
         n[1].e = target;
         n[2].e = internalFormat;
         n[3].i = width;
         n[4].e = format;
         n[5].e = type;
         n[6].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->ColorTable)( target, internalFormat, width,
                                   format, type, table );
      }
   }
}



static void
save_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glColorTableParameterfv");
   FLUSH_VB(ctx, "dlist");

   n = alloc_instruction( ctx, OPCODE_COLOR_TABLE_PARAMETER_FV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorTableParameterfv)( target, pname, params );
   }
}


static void
save_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glColorTableParameterfv");
   FLUSH_VB(ctx, "dlist");

   n = alloc_instruction( ctx, OPCODE_COLOR_TABLE_PARAMETER_IV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorTableParameteriv)( target, pname, params );
   }
}



static void save_ColorSubTable( GLenum target, GLsizei start, GLsizei count,
                                GLenum format, GLenum type,
                                const GLvoid *table)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(count, 1, 1, format, type, table,
                                      &ctx->Unpack);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COLOR_SUB_TABLE, 6 );
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = count;
      n[4].e = format;
      n[5].e = type;
      n[6].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorSubTable)(target, start, count, format, type, table);
   }
}


static void
save_CopyColorSubTable(GLenum target, GLsizei start,
                       GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_COLOR_SUB_TABLE, 6 );
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyColorSubTable)(target, start, x, y, width);
   }
}


static void
save_CopyColorTable(GLenum target, GLenum internalformat,
                    GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_COPY_COLOR_TABLE, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = internalformat;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyColorTable)(target, internalformat, x, y, width);
   }
}


static void
save_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width,
                         GLenum format, GLenum type, const GLvoid *filter)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type, filter,
                                      &ctx->Unpack);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_FILTER_1D, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].e = format;
      n[5].e = type;
      n[6].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionFilter1D)( target, internalFormat, width,
                                         format, type, filter );
   }
}


static void
save_ConvolutionFilter2D(GLenum target, GLenum internalFormat,
                         GLsizei width, GLsizei height, GLenum format,
                         GLenum type, const GLvoid *filter)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type, filter,
                                      &ctx->Unpack);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_FILTER_2D, 7 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].i = height;
      n[5].e = format;
      n[6].e = type;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionFilter2D)( target, internalFormat, width, height,
                                         format, type, filter );
   }
}


static void
save_ConvolutionParameteri(GLenum target, GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_PARAMETER_I, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameteri)( target, pname, param );
   }
}


static void
save_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_PARAMETER_IV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_CONVOLUTION_BORDER_COLOR ||
          pname == GL_CONVOLUTION_FILTER_SCALE ||
          pname == GL_CONVOLUTION_FILTER_BIAS) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
      else {
         n[4].i = n[5].i = n[6].i = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameteriv)( target, pname, params );
   }
}


static void
save_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_PARAMETER_F, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameterf)( target, pname, param );
   }
}


static void
save_ConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CONVOLUTION_PARAMETER_IV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_CONVOLUTION_BORDER_COLOR ||
          pname == GL_CONVOLUTION_FILTER_SCALE ||
          pname == GL_CONVOLUTION_FILTER_BIAS) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
      else {
         n[4].f = n[5].f = n[6].f = 0.0F;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameterfv)( target, pname, params );
   }
}


static void save_CopyPixels( GLint x, GLint y,
                             GLsizei width, GLsizei height, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyPixels)( x, y, width, height, type );
   }
}



static void
save_CopyTexImage1D( GLenum target, GLint level, GLenum internalformat,
                     GLint x, GLint y, GLsizei width, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyTexImage1D)( target, level, internalformat,
                                   x, y, width, border );
   }
}


static void
save_CopyTexImage2D( GLenum target, GLint level,
                     GLenum internalformat,
                     GLint x, GLint y, GLsizei width,
                     GLsizei height, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyTexImage2D)( target, level, internalformat,
                                   x, y, width, height, border );
   }
}



static void
save_CopyTexSubImage1D( GLenum target, GLint level,
                        GLint xoffset, GLint x, GLint y,
                        GLsizei width )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyTexSubImage1D)( target, level, xoffset, x, y, width );
   }
}


static void
save_CopyTexSubImage2D( GLenum target, GLint level,
                        GLint xoffset, GLint yoffset,
                        GLint x, GLint y,
                        GLsizei width, GLint height )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyTexSubImage2D)( target, level, xoffset, yoffset,
                               x, y, width, height );
   }
}


static void
save_CopyTexSubImage3D( GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint x, GLint y,
                        GLsizei width, GLint height )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->CopyTexSubImage3D)( target, level,
                                      xoffset, yoffset, zoffset,
                                      x, y, width, height );
   }
}


static void save_CullFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CULL_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CullFace)( mode );
   }
}


static void save_DepthFunc( GLenum func )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_FUNC, 1 );
   if (n) {
      n[1].e = func;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthFunc)( func );
   }
}


static void save_DepthMask( GLboolean mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_MASK, 1 );
   if (n) {
      n[1].b = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthMask)( mask );
   }
}


static void save_DepthRange( GLclampd nearval, GLclampd farval )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DEPTH_RANGE, 2 );
   if (n) {
      n[1].f = (GLfloat) nearval;
      n[2].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthRange)( nearval, farval );
   }
}


static void save_Disable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DISABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Disable)( cap );
   }
}


static void save_DrawBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DRAW_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DrawBuffer)( mode );
   }
}


static void save_DrawPixels( GLsizei width, GLsizei height,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                      pixels, &ctx->Unpack);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_DRAW_PIXELS, 5 );
   if (n) {
      n[1].i = width;
      n[2].i = height;
      n[3].e = format;
      n[4].e = type;
      n[5].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DrawPixels)( width, height, format, type, pixels );
   }
}



static void save_Enable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ENABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Enable)( cap );
   }
}



static void save_EvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_EVALMESH1, 3 );
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalMesh1)( mode, i1, i2 );
   }
}


static void save_EvalMesh2( 
                        GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->EvalMesh2)( mode, i1, i2, j1, j2 );
   }
}




static void save_Fogfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Fogfv)( pname, params );
   }
}


static void save_Fogf( GLenum pname, GLfloat param )
{
   save_Fogfv(pname, &param);
}


static void save_Fogiv(GLenum pname, const GLint *params )
{
   GLfloat p[4];
   switch (pname) {
      case GL_FOG_MODE:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_INDEX:
	 p[0] = (GLfloat) *params;
	 break;
      case GL_FOG_COLOR:
	 p[0] = INT_TO_FLOAT( params[0] );
	 p[1] = INT_TO_FLOAT( params[1] );
	 p[2] = INT_TO_FLOAT( params[2] );
	 p[3] = INT_TO_FLOAT( params[3] );
	 break;
      default:
         /* Error will be caught later in gl_Fogfv */
         ;
   }
   save_Fogfv(pname, p);
}


static void save_Fogi(GLenum pname, GLint param )
{
   save_Fogiv(pname, &param);
}


static void save_FrontFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_FRONT_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->FrontFace)( mode );
   }
}


static void save_Frustum( GLdouble left, GLdouble right,
                      GLdouble bottom, GLdouble top,
                      GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Frustum)( left, right, bottom, top, nearval, farval );
   }
}


static void save_Hint( GLenum target, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_HINT, 2 );
   if (n) {
      n[1].e = target;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Hint)( target, mode );
   }
}


/* GL_PGI_misc_hints*/
static void save_HintPGI( GLenum target, GLint mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_HINT_PGI, 2 );
   if (n) {
      n[1].e = target;
      n[2].i = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->HintPGI)( target, mode );
   }
}


static void
save_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_HISTOGRAM, 4 );
   if (n) {
      n[1].e = target;
      n[2].i = width;
      n[3].e = internalFormat;
      n[4].b = sink;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Histogram)( target, width, internalFormat, sink );
   }
}


static void save_IndexMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_INDEX_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->IndexMask)( mask );
   }
}


static void save_InitNames( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_INIT_NAMES, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->InitNames)();
   }
}


static void save_Lightfv( GLenum light, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LIGHT, 6 );
   if (OPCODE_LIGHT) {
      GLint i, nParams;
      n[1].e = light;
      n[2].e = pname;
      switch (pname) {
         case GL_AMBIENT:
            nParams = 4;
            break;
         case GL_DIFFUSE:
            nParams = 4;
            break;
         case GL_SPECULAR:
            nParams = 4;
            break;
         case GL_POSITION:
            nParams = 4;
            break;
         case GL_SPOT_DIRECTION:
            nParams = 3;
            break;
         case GL_SPOT_EXPONENT:
            nParams = 1;
            break;
         case GL_SPOT_CUTOFF:
            nParams = 1;
            break;
         case GL_CONSTANT_ATTENUATION:
            nParams = 1;
            break;
         case GL_LINEAR_ATTENUATION:
            nParams = 1;
            break;
         case GL_QUADRATIC_ATTENUATION:
            nParams = 1;
            break;
         default:
            nParams = 0;
      }
      for (i = 0; i < nParams; i++) {
	 n[3+i].f = params[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Lightfv)( light, pname, params );
   }
}


static void save_Lightf( GLenum light, GLenum pname, GLfloat params )
{
   save_Lightfv(light, pname, &params);
}


static void save_Lightiv( GLenum light, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_POSITION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         fparam[3] = (GLfloat) params[3];
         break;
      case GL_SPOT_DIRECTION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* error will be caught later in gl_Lightfv */
         ;
   }
   save_Lightfv( light, pname, fparam );
}


static void save_Lighti( GLenum light, GLenum pname, GLint param )
{
   save_Lightiv( light, pname, &param );
}


static void save_LightModelfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->LightModelfv)( pname, params );
   }
}


static void save_LightModelf( GLenum pname, GLfloat param )
{
   save_LightModelfv(pname, &param);
}


static void save_LightModeliv( GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* Error will be caught later in gl_LightModelfv */
         ;
   }
   save_LightModelfv(pname, fparam);
}


static void save_LightModeli( GLenum pname, GLint param )
{
   save_LightModeliv(pname, &param);
}


static void save_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LINE_STIPPLE, 2 );
   if (n) {
      n[1].i = factor;
      n[2].us = pattern;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LineStipple)( factor, pattern );
   }
}


static void save_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LINE_WIDTH, 1 );
   if (n) {
      n[1].f = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LineWidth)( width );
   }
}


static void save_ListBase( GLuint base )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LIST_BASE, 1 );
   if (n) {
      n[1].ui = base;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ListBase)( base );
   }
}


static void save_LoadIdentity( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_LOAD_IDENTITY, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadIdentity)();
   }
}


static void save_LoadMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->LoadMatrixf)( m );
   }
}


static void save_LoadMatrixd( const GLdouble *m )
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = m[i];
   }
   save_LoadMatrixf(f);
}


static void save_LoadName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LOAD_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadName)( name );
   }
}


static void save_LogicOp( GLenum opcode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_LOGIC_OP, 1 );
   if (n) {
      n[1].e = opcode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LogicOp)( opcode );
   }
}


static void save_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
                        GLint order, const GLdouble *points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP1, 6 );
   if (n) {
      GLfloat *pnts = gl_copy_map_points1d( target, stride, order, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = _mesa_evaluator_components(target);  /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map1d)( target, u1, u2, stride, order, points );
   }
}

static void save_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
                        GLint order, const GLfloat *points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP1, 6 );
   if (n) {
      GLfloat *pnts = gl_copy_map_points1f( target, stride, order, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = _mesa_evaluator_components(target);  /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map1f)( target, u1, u2, stride, order, points );
   }
}


static void save_Map2d( GLenum target,
                        GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
                        GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
                        const GLdouble *points )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP2, 10 );
   if (n) {
      GLfloat *pnts = gl_copy_map_points2d( target, ustride, uorder,
                                            vstride, vorder, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].f = v1;
      n[5].f = v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;  /*ustride*/
      n[7].i = _mesa_evaluator_components(target);           /*vstride*/
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map2d)( target,
                          u1, u2, ustride, uorder,
                          v1, v2, vstride, vorder, points );
   }
}


static void save_Map2f( GLenum target,
                        GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                        GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                        const GLfloat *points )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAP2, 10 );
   if (n) {
      GLfloat *pnts = gl_copy_map_points2f( target, ustride, uorder,
                                            vstride, vorder, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].f = v1;
      n[5].f = v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;  /*ustride*/
      n[7].i = _mesa_evaluator_components(target);           /*vstride*/
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map2f)( target, u1, u2, ustride, uorder,
                          v1, v2, vstride, vorder, points );
   }
}


static void save_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MAPGRID1, 3 );
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MapGrid1f)( un, u1, u2 );
   }
}


static void save_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
   save_MapGrid1f(un, u1, u2);
}


static void save_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                            GLint vn, GLfloat v1, GLfloat v2 )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->MapGrid2f)( un, u1, u2, vn, v1, v2 );
   }
}



static void save_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                            GLint vn, GLdouble v1, GLdouble v2 )
{
   save_MapGrid2f(un, u1, u2, vn, v1, v2);
}


static void save_MatrixMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MATRIX_MODE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MatrixMode)( mode );
   }
}


static void
save_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_MIN_MAX, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].b = sink;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Minmax)( target, internalFormat, sink );
   }
}


static void save_MultMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->MultMatrixf)( m );
   }
}


static void save_MultMatrixd( const GLdouble *m )
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = m[i];
   }
   save_MultMatrixf(f);
}


static void save_NewList( GLuint list, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   /* It's an error to call this function while building a display list */
   gl_error( ctx, GL_INVALID_OPERATION, "glNewList" );
   (void) list;
   (void) mode;
}



static void save_Ortho( GLdouble left, GLdouble right,
                    GLdouble bottom, GLdouble top,
                    GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Ortho)( left, right, bottom, top, nearval, farval );
   }
}


static void save_PixelMapfv( GLenum map, GLint mapsize, const GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_MAP, 3 );
   if (n) {
      n[1].e = map;
      n[2].i = mapsize;
      n[3].data  = (void *) MALLOC( mapsize * sizeof(GLfloat) );
      MEMCPY( n[3].data, (void *) values, mapsize * sizeof(GLfloat) );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelMapfv)( map, mapsize, values );
   }
}


static void save_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void save_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void save_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_TRANSFER, 2 );
   if (n) {
      n[1].e = pname;
      n[2].f = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTransferf)( pname, param );
   }
}


static void save_PixelTransferi( GLenum pname, GLint param )
{
   save_PixelTransferf( pname, (GLfloat) param );
}


static void save_PixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_ZOOM, 2 );
   if (n) {
      n[1].f = xfactor;
      n[2].f = yfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelZoom)( xfactor, yfactor );
   }
}


static void save_PointParameterfvEXT( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->PointParameterfvEXT)( pname, params );
   }
}


static void save_PointParameterfEXT( GLenum pname, GLfloat param )
{
   save_PointParameterfvEXT(pname, &param);
}


static void save_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POINT_SIZE, 1 );
   if (n) {
      n[1].f = size;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PointSize)( size );
   }
}


static void save_PolygonMode( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_MODE, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonMode)( face, mode );
   }
}


/*
 * Polygon stipple must have been upacked already!
 */
static void save_PolygonStipple( const GLubyte *pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_STIPPLE, 1 );
   if (n) {
      void *data;
      n[1].data = MALLOC( 32 * 4 );
      data = n[1].data;   /* This needed for Acorn compiler */
      MEMCPY( data, pattern, 32 * 4 );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonStipple)( (GLubyte*) pattern );
   }
}


static void save_PolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_POLYGON_OFFSET, 2 );
   if (n) {
      n[1].f = factor;
      n[2].f = units;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonOffset)( factor, units );
   }
}


static void save_PolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
   GET_CURRENT_CONTEXT(ctx);
   save_PolygonOffset(factor, ctx->Visual->DepthMaxF * bias);
}


static void save_PopAttrib( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_ATTRIB, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopAttrib)();
   }
}


static void save_PopMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopMatrix)();
   }
}


static void save_PopName( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_POP_NAME, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopName)();
   }
}


static void save_PrioritizeTextures( GLsizei num, const GLuint *textures,
                                     const GLclampf *priorities )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->PrioritizeTextures)( num, textures, priorities );
   }
}


static void save_PushAttrib( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PUSH_ATTRIB, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushAttrib)( mask );
   }
}


static void save_PushMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VB(ctx, "dlist");
   (void) alloc_instruction( ctx, OPCODE_PUSH_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushMatrix)();
   }
}


static void save_PushName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PUSH_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushName)( name );
   }
}


static void save_RasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->RasterPos4f)( x, y, z, w );
   }
}

static void save_RasterPos2d(GLdouble x, GLdouble y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void save_RasterPos2f(GLfloat x, GLfloat y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void save_RasterPos2i(GLint x, GLint y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void save_RasterPos2s(GLshort x, GLshort y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void save_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void save_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void save_RasterPos3i(GLint x, GLint y, GLint z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void save_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void save_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_RasterPos4f(x, y, z, w);
}

static void save_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   save_RasterPos4f(x, y, z, w);
}

static void save_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_RasterPos4f(x, y, z, w);
}

static void save_RasterPos2dv(const GLdouble *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void save_RasterPos2fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void save_RasterPos2iv(const GLint *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void save_RasterPos2sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void save_RasterPos3dv(const GLdouble *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void save_RasterPos3fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void save_RasterPos3iv(const GLint *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void save_RasterPos3sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void save_RasterPos4dv(const GLdouble *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}

static void save_RasterPos4fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}

static void save_RasterPos4iv(const GLint *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}

static void save_RasterPos4sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}


static void save_PassThrough( GLfloat token )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PASSTHROUGH, 1 );
   if (n) {
      n[1].f = token;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PassThrough)( token );
   }
}


static void save_ReadBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_READ_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ReadBuffer)( mode );
   }
}


static void save_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Rectf)( x1, y1, x2, y2 );
   }
}

static void save_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   save_Rectf(x1, y1, x2, y2);
}

static void save_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
   save_Rectf(v1[0], v1[1], v2[0], v2[1]);
}

static void save_Rectfv( const GLfloat *v1, const GLfloat *v2 )
{
   save_Rectf(v1[0], v1[1], v2[0], v2[1]);
}
 
static void save_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   save_Rectf(x1, y1, x2, y2);
}

static void save_Rectiv(const GLint *v1, const GLint *v2)
{
   save_Rectf(v1[0], v1[1], v2[0], v2[1]);
}

static void save_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   save_Rectf(x1, y1, x2, y2);
}

static void save_Rectsv(const GLshort *v1, const GLshort *v2)
{
   save_Rectf(v1[0], v1[1], v2[0], v2[1]);
}


static void
save_ResetHistogram(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_RESET_HISTOGRAM, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ResetHistogram)( target );
   }
}


static void
save_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_RESET_MIN_MAX, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ResetMinmax)( target );
   }
}


static void save_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GLfloat m[16];
   gl_rotation_matrix( angle, x, y, z, m );
   save_MultMatrixf( m );  /* save and maybe execute */
}


static void save_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   save_Rotatef(angle, x, y, z);
}


static void save_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_SCALE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Scalef)( x, y, z );
   }
}


static void save_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   save_Scalef(x, y, z);
}


static void save_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Scissor)( x, y, width, height );
   }
}


static void save_ShadeModel( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_SHADE_MODEL, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ShadeModel)( mode );
   }
}


static void save_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_FUNC, 3 );
   if (n) {
      n[1].e = func;
      n[2].i = ref;
      n[3].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilFunc)( func, ref, mask );
   }
}


static void save_StencilMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilMask)( mask );
   }
}


static void save_StencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_STENCIL_OP, 3 );
   if (n) {
      n[1].e = fail;
      n[2].e = zfail;
      n[3].e = zpass;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilOp)( fail, zfail, zpass );
   }
}


static void save_TexEnvfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->TexEnvfv)( target, pname, params );
   }
}


static void save_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   save_TexEnvfv( target, pname, &param );
}


static void save_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   save_TexEnvfv( target, pname, p );
}


static void save_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
   p[3] = INT_TO_FLOAT( param[3] );
   save_TexEnvfv( target, pname, p );
}


static void save_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->TexGenfv)( coord, pname, params );
   }
}


static void save_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   save_TexGenfv(coord, pname, p);
}


static void save_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   save_TexGenfv( coord, pname, &p );
}


static void save_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   save_TexGenfv( coord, pname, p );
}


static void save_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   save_TexGenfv(coord, pname, &param);
}


static void save_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   save_TexGeniv( coord, pname, &param );
}


static void save_TexParameterfv( GLenum target,
                             GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->TexParameterfv)( target, pname, params );
   }
}


static void save_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   save_TexParameterfv(target, pname, &param);
}


static void save_TexParameteri( GLenum target, GLenum pname, const GLint param )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   save_TexParameterfv(target, pname, fparam);
}


static void save_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) params[0];
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   save_TexParameterfv(target, pname, fparam);
}


static void save_TexImage1D( GLenum target,
                             GLint level, GLint components,
                             GLsizei width, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage1D)( target, level, components, width,
                               border, format, type, pixels );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type,
                                         pixels, &ctx->Unpack);
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
         n[8].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage1D)( target, level, components, width,
                                  border, format, type, pixels );
      }
   }
}


static void save_TexImage2D( GLenum target,
                             GLint level, GLint components,
                             GLsizei width, GLsizei height, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage2D)( target, level, components, width,
                               height, border, format, type, pixels );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                         pixels, &ctx->Unpack);
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
         n[9].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage2D)( target, level, components, width,
                                  height, border, format, type, pixels );
      }
   }
}


static void save_TexImage3D( GLenum target,
                             GLint level, GLint internalFormat,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage3D)( target, level, internalFormat, width,
                               height, depth, border, format, type, pixels );
   }
   else {
      Node *n;
      GLvoid *image = _mesa_unpack_image(width, height, depth, format, type,
                                         pixels, &ctx->Unpack);
      FLUSH_VB(ctx, "dlist");
      n = alloc_instruction( ctx, OPCODE_TEX_IMAGE3D, 10 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = (GLint) depth; 
         n[7].i = border;
         n[8].e = format;
         n[9].e = type;
         n[10].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage3D)( target, level, internalFormat, width,
                                height, depth, border, format, type, pixels );
      }
   }
}


static void save_TexSubImage1D( GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type,
                                      pixels, &ctx->Unpack);
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
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage1D)( target, level, xoffset, width,
                                  format, type, pixels );
   }
}


static void save_TexSubImage2D( GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                      pixels, &ctx->Unpack);
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
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage2D)( target, level, xoffset, yoffset,
                           width, height, format, type, pixels );
   }
}


static void save_TexSubImage3D( GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, height, depth, format, type,
                                      pixels, &ctx->Unpack);
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
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage3D)( target, level,
                                  xoffset, yoffset, zoffset,
                                  width, height, depth, format, type, pixels );
   }
}


static void save_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx,  OPCODE_TRANSLATE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Translatef)( x, y, z );
   }
}


static void save_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   save_Translatef(x, y, z);
}



static void save_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->Viewport)( x, y, width, height );
   }
}


static void save_WindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
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
      (*ctx->Exec->WindowPos4fMESA)( x, y, z, w );
   }
}

static void save_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void save_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void save_WindowPos2iMESA(GLint x, GLint y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void save_WindowPos2sMESA(GLshort x, GLshort y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void save_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void save_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void save_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void save_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void save_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_WindowPos4fMESA(x, y, z, w);
}

static void save_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   save_WindowPos4fMESA(x, y, z, w);
}

static void save_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_WindowPos4fMESA(x, y, z, w);
}

static void save_WindowPos2dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void save_WindowPos2fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void save_WindowPos2ivMESA(const GLint *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void save_WindowPos2svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void save_WindowPos3dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void save_WindowPos3fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void save_WindowPos3ivMESA(const GLint *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void save_WindowPos3svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void save_WindowPos4dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

static void save_WindowPos4fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

static void save_WindowPos4ivMESA(const GLint *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

static void save_WindowPos4svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}



/* GL_ARB_multitexture */
static void save_ActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_ACTIVE_TEXTURE, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ActiveTextureARB)( target );
   }
}


/* GL_ARB_multitexture */
static void save_ClientActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_CLIENT_ACTIVE_TEXTURE, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClientActiveTextureARB)( target );
   }
}



/* GL_ARB_transpose_matrix */

static void save_LoadTransposeMatrixdARB( const GLdouble m[16] )
{
   GLdouble tm[16];
   gl_matrix_transposed(tm, m);
   save_LoadMatrixd(tm);
}


static void save_LoadTransposeMatrixfARB( const GLfloat m[16] )
{
   GLfloat tm[16];
   gl_matrix_transposef(tm, m);
   save_LoadMatrixf(tm);
}


static void save_MultTransposeMatrixdARB( const GLdouble m[16] )
{
   GLdouble tm[16];
   gl_matrix_transposed(tm, m);
   save_MultMatrixd(tm);
}


static void save_MultTransposeMatrixfARB( const GLfloat m[16] )
{
   GLfloat tm[16];
   gl_matrix_transposef(tm, m);
   save_MultMatrixf(tm);
}


static void save_PixelTexGenSGIX(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_TEXGEN_SGIX, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTexGenSGIX)( mode );
   }
}


/* GL_ARB_texture_compression */
static void
save_CompressedTexImage1DARB(GLenum target, GLint level,
                             GLenum internalformat, GLsizei width,
                             GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
}


static void
save_CompressedTexImage2DARB(GLenum target, GLint level,
                             GLenum internalformat, GLsizei width,
                             GLsizei height, GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
}


static void
save_CompressedTexImage3DARB(GLenum target, GLint level,
                             GLenum internalformat, GLsizei width,
                             GLsizei height, GLsizei depth, GLint border,
                             GLsizei imageSize, const GLvoid *data)
{
}


static void
save_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format,
                                GLsizei imageSize, const GLvoid *data)
{
}


static void
save_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLsizei width, GLsizei height,
                                GLenum format, GLsizei imageSize,
                                const GLvoid *data)
{
}


static void
save_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLint zoffset, GLsizei width,
                                GLsizei height, GLsizei depth, GLenum format,
                                GLsizei imageSize, const GLvoid *data)
{
}


/* GL_SGIS_pixel_texture */

static void save_PixelTexGenParameteriSGIS(GLenum target, GLint value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   FLUSH_VB(ctx, "dlist");
   n = alloc_instruction( ctx, OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS, 2 );
   if (n) {
      n[1].e = target;
      n[2].i = value;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTexGenParameteriSGIS)( target, value );
   }
}


static void save_PixelTexGenParameterfSGIS(GLenum target, GLfloat value)
{
   save_PixelTexGenParameteriSGIS(target, (GLint) value);
}


static void save_PixelTexGenParameterivSGIS(GLenum target, const GLint *value)
{
   save_PixelTexGenParameteriSGIS(target, *value);
}


static void save_PixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value)
{
   save_PixelTexGenParameteriSGIS(target, (GLint) *value);
}

void gl_compile_cassette( GLcontext *ctx )
{
   Node *n = alloc_instruction( ctx, OPCODE_VERTEX_CASSETTE, 8 );
   struct immediate *im = ctx->input;   

   if (!n) 
      return;
   

   /* Do some easy optimizations of the cassette.  
    */
#if 0
   if (0 && im->v.Obj.size < 4 && im->Count > 15) {
      im->Bounds = (GLfloat (*)[3]) MALLOC(6 * sizeof(GLfloat));
      (gl_calc_bound_tab[im->v.Obj.size])( im->Bounds, &im->v.Obj );
   }
#endif

   n[1].data = (void *)im;   
   n[2].ui = im->Start;
   n[3].ui = im->Count;
   n[4].ui = im->BeginState;
   n[5].ui = im->OrFlag;
   n[6].ui = im->AndFlag;
   n[7].ui = im->LastData;
   n[8].ui = im->LastPrimitive;

   if (im->Count > VB_MAX - 4) {

      struct immediate *new_im = gl_immediate_alloc(ctx);      
      if (!new_im) return;
      SET_IMMEDIATE( ctx, new_im );
      gl_reset_input( ctx );

   } else {
      im->Count++;;
      im->Start = im->Count;	/* don't clear anything in reset_input */
      im->ref_count++;

      im->Primitive[im->Start] = ctx->Current.Primitive;
      im->LastPrimitive = im->Start;
      im->BeginState = VERT_BEGIN_0;
      im->OrFlag = 0;
      im->AndFlag = ~0;

      if (0)
	 fprintf(stderr, "in compile_cassette, BeginState is %x\n", 
	      im->BeginState);
   }   
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


static GLboolean
islist(GLcontext *ctx, GLuint list)
{
   if (list > 0 && _mesa_HashLookup(ctx->Shared->DisplayList, list)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
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

   if (!islist(ctx,list))
      return;

/*    mesa_print_display_list( list ); */

   ctx->CallDepth++;

   n = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);

   done = GL_FALSE;
   while (!done) {
      opcode = n[0].opcode;

      switch (opcode) {
         case OPCODE_ERROR:
 	    gl_error( ctx, n[1].e, (const char *) n[2].data ); 
            break;
         case OPCODE_VERTEX_CASSETTE: {
	    struct immediate *IM;

	    if (ctx->NewState)
	       gl_update_state(ctx);
	    if (ctx->CompileCVAFlag) {
	       ctx->CompileCVAFlag = 0;
	       ctx->CVA.elt.pipeline_valid = 0;
	    }
	    if (!ctx->CVA.elt.pipeline_valid)
	       gl_build_immediate_pipeline( ctx );

	    
	    IM = (struct immediate *) n[1].data;
	    IM->Start = n[2].ui;
	    IM->Count = n[3].ui;
	    IM->BeginState = n[4].ui;
	    IM->OrFlag = n[5].ui;
	    IM->AndFlag = n[6].ui;
	    IM->LastData = n[7].ui;
	    IM->LastPrimitive = n[8].ui;

	    if ((MESA_VERBOSE & VERBOSE_DISPLAY_LIST) &&
		(MESA_VERBOSE & VERBOSE_IMMEDIATE))
	       gl_print_cassette( (struct immediate *) n[1].data );

	    if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST) {
	       fprintf(stderr, "Run cassette %d, rows %d..%d, beginstate %x ",
		       IM->id,
		       IM->Start, IM->Count, IM->BeginState);
	       gl_print_vert_flags("orflag", IM->OrFlag);
	    }

	    gl_fixup_cassette( ctx, (struct immediate *) n[1].data ); 
	    gl_execute_cassette( ctx, (struct immediate *) n[1].data ); 
            break;
	 }
         case OPCODE_ACCUM:
	    (*ctx->Exec->Accum)( n[1].e, n[2].f );
	    break;
         case OPCODE_ALPHA_FUNC:
	    (*ctx->Exec->AlphaFunc)( n[1].e, n[2].f );
	    break;
         case OPCODE_BIND_TEXTURE:
            (*ctx->Exec->BindTexture)( n[1].e, n[2].ui );
            break;
	 case OPCODE_BITMAP:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->Bitmap)( (GLsizei) n[1].i, (GLsizei) n[2].i,
                 n[3].f, n[4].f, n[5].f, n[6].f, (const GLubyte *) n[7].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_BLEND_COLOR:
	    (*ctx->Exec->BlendColor)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_BLEND_EQUATION:
	    (*ctx->Exec->BlendEquation)( n[1].e );
	    break;
	 case OPCODE_BLEND_FUNC:
	    (*ctx->Exec->BlendFunc)( n[1].e, n[2].e );
	    break;
	 case OPCODE_BLEND_FUNC_SEPARATE:
	    (*ctx->Exec->BlendFuncSeparateEXT)(n[1].e, n[2].e, n[3].e, n[4].e);
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
	    (*ctx->Exec->Clear)( n[1].bf );
	    break;
	 case OPCODE_CLEAR_COLOR:
	    (*ctx->Exec->ClearColor)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_ACCUM:
	    (*ctx->Exec->ClearAccum)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_DEPTH:
	    (*ctx->Exec->ClearDepth)( (GLclampd) n[1].f );
	    break;
	 case OPCODE_CLEAR_INDEX:
	    (*ctx->Exec->ClearIndex)( n[1].ui );
	    break;
	 case OPCODE_CLEAR_STENCIL:
	    (*ctx->Exec->ClearStencil)( n[1].i );
	    break;
         case OPCODE_CLIP_PLANE:
            {
               GLdouble eq[4];
               eq[0] = n[2].f;
               eq[1] = n[3].f;
               eq[2] = n[4].f;
               eq[3] = n[5].f;
               (*ctx->Exec->ClipPlane)( n[1].e, eq );
            }
            break;
	 case OPCODE_COLOR_MASK:
	    (*ctx->Exec->ColorMask)( n[1].b, n[2].b, n[3].b, n[4].b );
	    break;
	 case OPCODE_COLOR_MATERIAL:
	    (*ctx->Exec->ColorMaterial)( n[1].e, n[2].e );
	    break;
         case OPCODE_COLOR_TABLE:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ColorTable)( n[1].e, n[2].e, n[3].i, n[4].e,
                                         n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->ColorTableParameterfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               (*ctx->Exec->ColorTableParameteriv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COLOR_SUB_TABLE:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ColorSubTable)( n[1].e, n[2].i, n[3].i,
                                            n[4].e, n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ConvolutionFilter1D)( n[1].e, n[2].i, n[3].i,
                                                  n[4].e, n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ConvolutionFilter2D)( n[1].e, n[2].i, n[3].i,
                                       n[4].i, n[5].e, n[6].e, n[7].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_I:
            (*ctx->Exec->ConvolutionParameteri)( n[1].e, n[2].e, n[3].i );
            break;
         case OPCODE_CONVOLUTION_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               (*ctx->Exec->ConvolutionParameteriv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_F:
            (*ctx->Exec->ConvolutionParameterf)( n[1].e, n[2].e, n[3].f );
            break;
         case OPCODE_CONVOLUTION_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->ConvolutionParameterfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COPY_COLOR_SUB_TABLE:
            (*ctx->Exec->CopyColorSubTable)( n[1].e, n[2].i,
                                             n[3].i, n[4].i, n[5].i );
            break;
         case OPCODE_COPY_COLOR_TABLE:
            (*ctx->Exec->CopyColorSubTable)( n[1].e, n[2].i,
                                             n[3].i, n[4].i, n[5].i );
            break;
	 case OPCODE_COPY_PIXELS:
	    (*ctx->Exec->CopyPixels)( n[1].i, n[2].i,
			   (GLsizei) n[3].i, (GLsizei) n[4].i, n[5].e );
	    break;
         case OPCODE_COPY_TEX_IMAGE1D:
	    (*ctx->Exec->CopyTexImage1D)( n[1].e, n[2].i, n[3].e, n[4].i,
                                         n[5].i, n[6].i, n[7].i );
            break;
         case OPCODE_COPY_TEX_IMAGE2D:
	    (*ctx->Exec->CopyTexImage2D)( n[1].e, n[2].i, n[3].e, n[4].i,
                                         n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE1D:
	    (*ctx->Exec->CopyTexSubImage1D)( n[1].e, n[2].i, n[3].i,
                                            n[4].i, n[5].i, n[6].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE2D:
	    (*ctx->Exec->CopyTexSubImage2D)( n[1].e, n[2].i, n[3].i,
                                     n[4].i, n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE3D:
            (*ctx->Exec->CopyTexSubImage3D)( n[1].e, n[2].i, n[3].i,
                            n[4].i, n[5].i, n[6].i, n[7].i, n[8].i , n[9].i);
            break;
	 case OPCODE_CULL_FACE:
	    (*ctx->Exec->CullFace)( n[1].e );
	    break;
	 case OPCODE_DEPTH_FUNC:
	    (*ctx->Exec->DepthFunc)( n[1].e );
	    break;
	 case OPCODE_DEPTH_MASK:
	    (*ctx->Exec->DepthMask)( n[1].b );
	    break;
	 case OPCODE_DEPTH_RANGE:
	    (*ctx->Exec->DepthRange)( (GLclampd) n[1].f, (GLclampd) n[2].f );
	    break;
	 case OPCODE_DISABLE:
	    (*ctx->Exec->Disable)( n[1].e );
	    break;
	 case OPCODE_DRAW_BUFFER:
	    (*ctx->Exec->DrawBuffer)( n[1].e );
	    break;
	 case OPCODE_DRAW_PIXELS:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->DrawPixels)( n[1].i, n[2].i, n[3].e, n[4].e,
                                        n[5].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_ENABLE:
	    (*ctx->Exec->Enable)( n[1].e );
	    break;
	 case OPCODE_EVALMESH1:
	    (*ctx->Exec->EvalMesh1)( n[1].e, n[2].i, n[3].i );
	    break;
	 case OPCODE_EVALMESH2:
	    (*ctx->Exec->EvalMesh2)( n[1].e, n[2].i, n[3].i, n[4].i, n[5].i );
	    break;
	 case OPCODE_FOG:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       (*ctx->Exec->Fogfv)( n[1].e, p );
	    }
	    break;
	 case OPCODE_FRONT_FACE:
	    (*ctx->Exec->FrontFace)( n[1].e );
	    break;
         case OPCODE_FRUSTUM:
            (*ctx->Exec->Frustum)( n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_HINT:
	    (*ctx->Exec->Hint)( n[1].e, n[2].e );
	    break;
	 case OPCODE_HINT_PGI:
	    (*ctx->Exec->HintPGI)( n[1].e, n[2].i );
	    break;
	 case OPCODE_HISTOGRAM:
	    (*ctx->Exec->Histogram)( n[1].e, n[2].i, n[3].e, n[4].b );
	    break;
	 case OPCODE_INDEX_MASK:
	    (*ctx->Exec->IndexMask)( n[1].ui );
	    break;
	 case OPCODE_INIT_NAMES:
	    (*ctx->Exec->InitNames)();
	    break;
         case OPCODE_LIGHT:
	    {
	       GLfloat p[4];
	       p[0] = n[3].f;
	       p[1] = n[4].f;
	       p[2] = n[5].f;
	       p[3] = n[6].f;
	       (*ctx->Exec->Lightfv)( n[1].e, n[2].e, p );
	    }
	    break;
         case OPCODE_LIGHT_MODEL:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       (*ctx->Exec->LightModelfv)( n[1].e, p );
	    }
	    break;
	 case OPCODE_LINE_STIPPLE:
	    (*ctx->Exec->LineStipple)( n[1].i, n[2].us );
	    break;
	 case OPCODE_LINE_WIDTH:
	    (*ctx->Exec->LineWidth)( n[1].f );
	    break;
	 case OPCODE_LIST_BASE:
	    (*ctx->Exec->ListBase)( n[1].ui );
	    break;
	 case OPCODE_LOAD_IDENTITY:
            (*ctx->Exec->LoadIdentity)();
            break;
	 case OPCODE_LOAD_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       (*ctx->Exec->LoadMatrixf)( &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       (*ctx->Exec->LoadMatrixf)( m );
	    }
	    break;
	 case OPCODE_LOAD_NAME:
	    (*ctx->Exec->LoadName)( n[1].ui );
	    break;
	 case OPCODE_LOGIC_OP:
	    (*ctx->Exec->LogicOp)( n[1].e );
	    break;
	 case OPCODE_MAP1:
            {
               GLenum target = n[1].e;
               GLint ustride = _mesa_evaluator_components(target);
               GLint uorder = n[5].i;
               GLfloat u1 = n[2].f;
               GLfloat u2 = n[3].f;
               (*ctx->Exec->Map1f)( target, u1, u2, ustride, uorder,
                                   (GLfloat *) n[6].data );
            }
	    break;
	 case OPCODE_MAP2:
            {
               GLenum target = n[1].e;
               GLfloat u1 = n[2].f;
               GLfloat u2 = n[3].f;
               GLfloat v1 = n[4].f;
               GLfloat v2 = n[5].f;
               GLint ustride = n[6].i;
               GLint vstride = n[7].i;
               GLint uorder = n[8].i;
               GLint vorder = n[9].i;
               (*ctx->Exec->Map2f)( target, u1, u2, ustride, uorder,
                                   v1, v2, vstride, vorder,
                                   (GLfloat *) n[10].data );
            }
	    break;
	 case OPCODE_MAPGRID1:
	    (*ctx->Exec->MapGrid1f)( n[1].i, n[2].f, n[3].f );
	    break;
	 case OPCODE_MAPGRID2:
	    (*ctx->Exec->MapGrid2f)( n[1].i, n[2].f, n[3].f, n[4].i, n[5].f, n[6].f);
	    break;
         case OPCODE_MATRIX_MODE:
            (*ctx->Exec->MatrixMode)( n[1].e );
            break;
         case OPCODE_MIN_MAX:
            (*ctx->Exec->Minmax)(n[1].e, n[2].e, n[3].b);
            break;
	 case OPCODE_MULT_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       (*ctx->Exec->MultMatrixf)( &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       (*ctx->Exec->MultMatrixf)( m );
	    }
	    break;
         case OPCODE_ORTHO:
            (*ctx->Exec->Ortho)( n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_PASSTHROUGH:
	    (*ctx->Exec->PassThrough)( n[1].f );
	    break;
	 case OPCODE_PIXEL_MAP:
	    (*ctx->Exec->PixelMapfv)( n[1].e, n[2].i, (GLfloat *) n[3].data );
	    break;
	 case OPCODE_PIXEL_TRANSFER:
	    (*ctx->Exec->PixelTransferf)( n[1].e, n[2].f );
	    break;
	 case OPCODE_PIXEL_ZOOM:
	    (*ctx->Exec->PixelZoom)( n[1].f, n[2].f );
	    break;
	 case OPCODE_POINT_SIZE:
	    (*ctx->Exec->PointSize)( n[1].f );
	    break;
	 case OPCODE_POINT_PARAMETERS:
	    {
		GLfloat params[3];
		params[0] = n[2].f;
		params[1] = n[3].f;
		params[2] = n[4].f;
		(*ctx->Exec->PointParameterfvEXT)( n[1].e, params ); 
	    }
	    break;
	 case OPCODE_POLYGON_MODE:
	    (*ctx->Exec->PolygonMode)( n[1].e, n[2].e );
	    break;
	 case OPCODE_POLYGON_STIPPLE:
	    (*ctx->Exec->PolygonStipple)( (GLubyte *) n[1].data );
	    break;
	 case OPCODE_POLYGON_OFFSET:
	    (*ctx->Exec->PolygonOffset)( n[1].f, n[2].f );
	    break;
	 case OPCODE_POP_ATTRIB:
	    (*ctx->Exec->PopAttrib)();
	    break;
	 case OPCODE_POP_MATRIX:
	    (*ctx->Exec->PopMatrix)();
	    break;
	 case OPCODE_POP_NAME:
	    (*ctx->Exec->PopName)();
	    break;
	 case OPCODE_PRIORITIZE_TEXTURE:
            (*ctx->Exec->PrioritizeTextures)( 1, &n[1].ui, &n[2].f );
	    break;
	 case OPCODE_PUSH_ATTRIB:
	    (*ctx->Exec->PushAttrib)( n[1].bf );
	    break;
	 case OPCODE_PUSH_MATRIX:
	    (*ctx->Exec->PushMatrix)();
	    break;
	 case OPCODE_PUSH_NAME:
	    (*ctx->Exec->PushName)( n[1].ui );
	    break;
	 case OPCODE_RASTER_POS:
            (*ctx->Exec->RasterPos4f)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_READ_BUFFER:
	    (*ctx->Exec->ReadBuffer)( n[1].e );
	    break;
         case OPCODE_RECTF:
            (*ctx->Exec->Rectf)( n[1].f, n[2].f, n[3].f, n[4].f );
            break;
         case OPCODE_RESET_HISTOGRAM:
            (*ctx->Exec->ResetHistogram)( n[1].e );
            break;
         case OPCODE_RESET_MIN_MAX:
            (*ctx->Exec->ResetMinmax)( n[1].e );
            break;
         case OPCODE_SCALE:
            (*ctx->Exec->Scalef)( n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_SCISSOR:
	    (*ctx->Exec->Scissor)( n[1].i, n[2].i, n[3].i, n[4].i );
	    break;
	 case OPCODE_SHADE_MODEL:
	    (*ctx->Exec->ShadeModel)( n[1].e );
	    break;
	 case OPCODE_STENCIL_FUNC:
	    (*ctx->Exec->StencilFunc)( n[1].e, n[2].i, n[3].ui );
	    break;
	 case OPCODE_STENCIL_MASK:
	    (*ctx->Exec->StencilMask)( n[1].ui );
	    break;
	 case OPCODE_STENCIL_OP:
	    (*ctx->Exec->StencilOp)( n[1].e, n[2].e, n[3].e );
	    break;
         case OPCODE_TEXENV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexEnvfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXGEN:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexGenfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXPARAMETER:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexParameterfv)( n[1].e, n[2].e, params );
            }
            break;
	 case OPCODE_TEX_IMAGE1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage1D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].e, /* border */
                                        n[6].e, /* format */
                                        n[7].e, /* type */
                                        n[8].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_TEX_IMAGE2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage2D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].i, /* height */
                                        n[6].e, /* border */
                                        n[7].e, /* format */
                                        n[8].e, /* type */
                                        n[9].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
         case OPCODE_TEX_IMAGE3D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage3D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].i, /* height */
                                        n[6].i, /* depth  */
                                        n[7].e, /* border */
                                        n[8].e, /* format */
                                        n[9].e, /* type */
                                        n[10].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage1D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].e,
                                           n[6].e, n[7].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage2D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].e,
                                           n[6].i, n[7].e, n[8].e, n[9].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage3D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].i, n[6].i, n[7].i,
                                           n[8].i, n[9].e, n[10].e,
                                           n[11].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TRANSLATE:
            (*ctx->Exec->Translatef)( n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_VIEWPORT:
	    (*ctx->Exec->Viewport)(n[1].i, n[2].i,
                                  (GLsizei) n[3].i, (GLsizei) n[4].i);
	    break;
	 case OPCODE_WINDOW_POS:
            (*ctx->Exec->WindowPos4fMESA)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
         case OPCODE_ACTIVE_TEXTURE:  /* GL_ARB_multitexture */
            (*ctx->Exec->ActiveTextureARB)( n[1].e );
            break;
         case OPCODE_CLIENT_ACTIVE_TEXTURE:  /* GL_ARB_multitexture */
            (*ctx->Exec->ClientActiveTextureARB)( n[1].e );
            break;
         case OPCODE_PIXEL_TEXGEN_SGIX:  /* GL_SGIX_pixel_texture */
            (*ctx->Exec->PixelTexGenSGIX)( n[1].e );
            break;
         case OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS:  /* GL_SGIS_pixel_texture */
            (*ctx->Exec->PixelTexGenParameteriSGIS)( n[1].e, n[2].i );
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
GLboolean
_mesa_IsList( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
   return islist(ctx, list);
}


/*
 * Delete a sequence of consecutive display lists.
 */
void
_mesa_DeleteLists( GLuint list, GLsizei range )
{
   GET_CURRENT_CONTEXT(ctx);
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
GLuint
_mesa_GenLists(GLsizei range )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint base;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glGenLists", 0);
   if (range<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glGenLists" );
      return 0;
   }
   if (range==0) {
      return 0;
   }

   /*
    * Make this an atomic operation
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   base = _mesa_HashFindFreeKeyBlock(ctx->Shared->DisplayList, range);
   if (base) {
      /* reserve the list IDs by with empty/dummy lists */
      GLint i;
      for (i=0; i<range; i++) {
         _mesa_HashInsert(ctx->Shared->DisplayList, base+i, make_empty_list());
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return base;
}



/*
 * Begin a new display list.
 */
void
_mesa_NewList( GLuint list, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
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
   ctx->CurrentBlock = (Node *) MALLOC( sizeof(Node) * BLOCK_SIZE );
   ctx->CurrentListPtr = ctx->CurrentBlock;
   ctx->CurrentPos = 0;

   IM = gl_immediate_alloc( ctx );
   SET_IMMEDIATE( ctx, IM );
   gl_reset_input( ctx );

   ctx->CompileFlag = GL_TRUE;
   ctx->CompileCVAFlag = GL_FALSE;
   ctx->ExecuteFlag = (mode == GL_COMPILE_AND_EXECUTE);

   ctx->CurrentDispatch = ctx->Save;
   _glapi_set_dispatch( ctx->CurrentDispatch );
}



/*
 * End definition of current display list.
 */
void
_mesa_EndList( void )
{
   GET_CURRENT_CONTEXT(ctx);
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
   _mesa_HashInsert(ctx->Shared->DisplayList, ctx->CurrentListNum, ctx->CurrentListPtr);


   if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST)
      mesa_print_display_list(ctx->CurrentListNum);

   ctx->CurrentListNum = 0;
   ctx->CurrentListPtr = NULL;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   /* ctx->CompileCVAFlag = ...; */

   /* KW: Put back the old input pointer.
    */
   if (--ctx->input->ref_count == 0)
      gl_immediate_free( ctx->input );

   SET_IMMEDIATE( ctx, ctx->VB->IM );
   gl_reset_input( ctx );

   /* Haven't tracked down why this is needed.
    */
   ctx->NewState = ~0;

   ctx->CurrentDispatch = ctx->Exec;
   _glapi_set_dispatch( ctx->CurrentDispatch );
}



void
_mesa_CallList( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
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
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch( ctx->CurrentDispatch );
   }
}



/*
 * Execute glCallLists:  call multiple display lists.
 */
void
_mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists )
{
   GET_CURRENT_CONTEXT(ctx);
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
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch( ctx->CurrentDispatch );
   }

/*    RESET_IMMEDIATE( ctx ); */
}



/*
 * Set the offset added to list numbers in glCallLists.
 */
void
_mesa_ListBase( GLuint base )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glListBase");
   ctx->List.ListBase = base;
}




/*
 * Assign all the pointers in <table> to point to Mesa's display list
 * building functions.
 */
void
_mesa_init_dlist_table( struct _glapi_table *table, GLuint tableSize )
{
   _mesa_init_no_op_table(table, tableSize);

   /* GL 1.0 */
   table->Accum = save_Accum;
   table->AlphaFunc = save_AlphaFunc;
   table->Begin = save_Begin;
   table->Bitmap = save_Bitmap;
   table->BlendFunc = save_BlendFunc;
   table->CallList = save_CallList;
   table->CallLists = save_CallLists;
   table->Clear = save_Clear;
   table->ClearAccum = save_ClearAccum;
   table->ClearColor = save_ClearColor;
   table->ClearDepth = save_ClearDepth;
   table->ClearIndex = save_ClearIndex;
   table->ClearStencil = save_ClearStencil;
   table->ClipPlane = save_ClipPlane;
   table->Color3b = _mesa_Color3b;
   table->Color3bv = _mesa_Color3bv;
   table->Color3d = _mesa_Color3d;
   table->Color3dv = _mesa_Color3dv;
   table->Color3f = _mesa_Color3f;
   table->Color3fv = _mesa_Color3fv;
   table->Color3i = _mesa_Color3i;
   table->Color3iv = _mesa_Color3iv;
   table->Color3s = _mesa_Color3s;
   table->Color3sv = _mesa_Color3sv;
   table->Color3ub = _mesa_Color3ub;
   table->Color3ubv = _mesa_Color3ubv;
   table->Color3ui = _mesa_Color3ui;
   table->Color3uiv = _mesa_Color3uiv;
   table->Color3us = _mesa_Color3us;
   table->Color3usv = _mesa_Color3usv;
   table->Color4b = _mesa_Color4b;
   table->Color4bv = _mesa_Color4bv;
   table->Color4d = _mesa_Color4d;
   table->Color4dv = _mesa_Color4dv;
   table->Color4f = _mesa_Color4f;
   table->Color4fv = _mesa_Color4fv;
   table->Color4i = _mesa_Color4i;
   table->Color4iv = _mesa_Color4iv;
   table->Color4s = _mesa_Color4s;
   table->Color4sv = _mesa_Color4sv;
   table->Color4ub = _mesa_Color4ub;
   table->Color4ubv = _mesa_Color4ubv;
   table->Color4ui = _mesa_Color4ui;
   table->Color4uiv = _mesa_Color4uiv;
   table->Color4us = _mesa_Color4us;
   table->Color4usv = _mesa_Color4usv;
   table->ColorMask = save_ColorMask;
   table->ColorMaterial = save_ColorMaterial;
   table->CopyPixels = save_CopyPixels;
   table->CullFace = save_CullFace;
   table->DeleteLists = _mesa_DeleteLists;
   table->DepthFunc = save_DepthFunc;
   table->DepthMask = save_DepthMask;
   table->DepthRange = save_DepthRange;
   table->Disable = save_Disable;
   table->DrawBuffer = save_DrawBuffer;
   table->DrawPixels = save_DrawPixels;
   table->EdgeFlag = _mesa_EdgeFlag;
   table->EdgeFlagv = _mesa_EdgeFlagv;
   table->Enable = save_Enable;
   table->End = _mesa_End;
   table->EndList = _mesa_EndList;
   table->EvalCoord1d = _mesa_EvalCoord1d;
   table->EvalCoord1dv = _mesa_EvalCoord1dv;
   table->EvalCoord1f = _mesa_EvalCoord1f;
   table->EvalCoord1fv = _mesa_EvalCoord1fv;
   table->EvalCoord2d = _mesa_EvalCoord2d;
   table->EvalCoord2dv = _mesa_EvalCoord2dv;
   table->EvalCoord2f = _mesa_EvalCoord2f;
   table->EvalCoord2fv = _mesa_EvalCoord2fv;
   table->EvalMesh1 = save_EvalMesh1;
   table->EvalMesh2 = save_EvalMesh2;
   table->EvalPoint1 = _mesa_EvalPoint1;
   table->EvalPoint2 = _mesa_EvalPoint2;
   table->FeedbackBuffer = _mesa_FeedbackBuffer;
   table->Finish = _mesa_Finish;
   table->Flush = _mesa_Flush;
   table->Fogf = save_Fogf;
   table->Fogfv = save_Fogfv;
   table->Fogi = save_Fogi;
   table->Fogiv = save_Fogiv;
   table->FrontFace = save_FrontFace;
   table->Frustum = save_Frustum;
   table->GenLists = _mesa_GenLists;
   table->GetBooleanv = _mesa_GetBooleanv;
   table->GetClipPlane = _mesa_GetClipPlane;
   table->GetDoublev = _mesa_GetDoublev;
   table->GetError = _mesa_GetError;
   table->GetFloatv = _mesa_GetFloatv;
   table->GetIntegerv = _mesa_GetIntegerv;
   table->GetLightfv = _mesa_GetLightfv;
   table->GetLightiv = _mesa_GetLightiv;
   table->GetMapdv = _mesa_GetMapdv;
   table->GetMapfv = _mesa_GetMapfv;
   table->GetMapiv = _mesa_GetMapiv;
   table->GetMaterialfv = _mesa_GetMaterialfv;
   table->GetMaterialiv = _mesa_GetMaterialiv;
   table->GetPixelMapfv = _mesa_GetPixelMapfv;
   table->GetPixelMapuiv = _mesa_GetPixelMapuiv;
   table->GetPixelMapusv = _mesa_GetPixelMapusv;
   table->GetPolygonStipple = _mesa_GetPolygonStipple;
   table->GetString = _mesa_GetString;
   table->GetTexEnvfv = _mesa_GetTexEnvfv;
   table->GetTexEnviv = _mesa_GetTexEnviv;
   table->GetTexGendv = _mesa_GetTexGendv;
   table->GetTexGenfv = _mesa_GetTexGenfv;
   table->GetTexGeniv = _mesa_GetTexGeniv;
   table->GetTexImage = _mesa_GetTexImage;
   table->GetTexLevelParameterfv = _mesa_GetTexLevelParameterfv;
   table->GetTexLevelParameteriv = _mesa_GetTexLevelParameteriv;
   table->GetTexParameterfv = _mesa_GetTexParameterfv;
   table->GetTexParameteriv = _mesa_GetTexParameteriv;
   table->Hint = save_Hint;
   table->IndexMask = save_IndexMask;
   table->Indexd = _mesa_Indexd;
   table->Indexdv = _mesa_Indexdv;
   table->Indexf = _mesa_Indexf;
   table->Indexfv = _mesa_Indexfv;
   table->Indexi = _mesa_Indexi;
   table->Indexiv = _mesa_Indexiv;
   table->Indexs = _mesa_Indexs;
   table->Indexsv = _mesa_Indexsv;
   table->InitNames = save_InitNames;
   table->IsEnabled = _mesa_IsEnabled;
   table->IsList = _mesa_IsList;
   table->LightModelf = save_LightModelf;
   table->LightModelfv = save_LightModelfv;
   table->LightModeli = save_LightModeli;
   table->LightModeliv = save_LightModeliv;
   table->Lightf = save_Lightf;
   table->Lightfv = save_Lightfv;
   table->Lighti = save_Lighti;
   table->Lightiv = save_Lightiv;
   table->LineStipple = save_LineStipple;
   table->LineWidth = save_LineWidth;
   table->ListBase = save_ListBase;
   table->LoadIdentity = save_LoadIdentity;
   table->LoadMatrixd = save_LoadMatrixd;
   table->LoadMatrixf = save_LoadMatrixf;
   table->LoadName = save_LoadName;
   table->LogicOp = save_LogicOp;
   table->Map1d = save_Map1d;
   table->Map1f = save_Map1f;
   table->Map2d = save_Map2d;
   table->Map2f = save_Map2f;
   table->MapGrid1d = save_MapGrid1d;
   table->MapGrid1f = save_MapGrid1f;
   table->MapGrid2d = save_MapGrid2d;
   table->MapGrid2f = save_MapGrid2f;
   table->Materialf = _mesa_Materialf;
   table->Materialfv = _mesa_Materialfv;
   table->Materiali = _mesa_Materiali;
   table->Materialiv = _mesa_Materialiv;
   table->MatrixMode = save_MatrixMode;
   table->MultMatrixd = save_MultMatrixd;
   table->MultMatrixf = save_MultMatrixf;
   table->NewList = save_NewList;
   table->Normal3b = _mesa_Normal3b;
   table->Normal3bv = _mesa_Normal3bv;
   table->Normal3d = _mesa_Normal3d;
   table->Normal3dv = _mesa_Normal3dv;
   table->Normal3f = _mesa_Normal3f;
   table->Normal3fv = _mesa_Normal3fv;
   table->Normal3i = _mesa_Normal3i;
   table->Normal3iv = _mesa_Normal3iv;
   table->Normal3s = _mesa_Normal3s;
   table->Normal3sv = _mesa_Normal3sv;
   table->Ortho = save_Ortho;
   table->PassThrough = save_PassThrough;
   table->PixelMapfv = save_PixelMapfv;
   table->PixelMapuiv = save_PixelMapuiv;
   table->PixelMapusv = save_PixelMapusv;
   table->PixelStoref = _mesa_PixelStoref;
   table->PixelStorei = _mesa_PixelStorei;
   table->PixelTransferf = save_PixelTransferf;
   table->PixelTransferi = save_PixelTransferi;
   table->PixelZoom = save_PixelZoom;
   table->PointSize = save_PointSize;
   table->PolygonMode = save_PolygonMode;
   table->PolygonOffset = save_PolygonOffset;
   table->PolygonStipple = save_PolygonStipple;
   table->PopAttrib = save_PopAttrib;
   table->PopMatrix = save_PopMatrix;
   table->PopName = save_PopName;
   table->PushAttrib = save_PushAttrib;
   table->PushMatrix = save_PushMatrix;
   table->PushName = save_PushName;
   table->RasterPos2d = save_RasterPos2d;
   table->RasterPos2dv = save_RasterPos2dv;
   table->RasterPos2f = save_RasterPos2f;
   table->RasterPos2fv = save_RasterPos2fv;
   table->RasterPos2i = save_RasterPos2i;
   table->RasterPos2iv = save_RasterPos2iv;
   table->RasterPos2s = save_RasterPos2s;
   table->RasterPos2sv = save_RasterPos2sv;
   table->RasterPos3d = save_RasterPos3d;
   table->RasterPos3dv = save_RasterPos3dv;
   table->RasterPos3f = save_RasterPos3f;
   table->RasterPos3fv = save_RasterPos3fv;
   table->RasterPos3i = save_RasterPos3i;
   table->RasterPos3iv = save_RasterPos3iv;
   table->RasterPos3s = save_RasterPos3s;
   table->RasterPos3sv = save_RasterPos3sv;
   table->RasterPos4d = save_RasterPos4d;
   table->RasterPos4dv = save_RasterPos4dv;
   table->RasterPos4f = save_RasterPos4f;
   table->RasterPos4fv = save_RasterPos4fv;
   table->RasterPos4i = save_RasterPos4i;
   table->RasterPos4iv = save_RasterPos4iv;
   table->RasterPos4s = save_RasterPos4s;
   table->RasterPos4sv = save_RasterPos4sv;
   table->ReadBuffer = save_ReadBuffer;
   table->ReadPixels = _mesa_ReadPixels;
   table->Rectd = save_Rectd;
   table->Rectdv = save_Rectdv;
   table->Rectf = save_Rectf;
   table->Rectfv = save_Rectfv;
   table->Recti = save_Recti;
   table->Rectiv = save_Rectiv;
   table->Rects = save_Rects;
   table->Rectsv = save_Rectsv;
   table->RenderMode = _mesa_RenderMode;
   table->Rotated = save_Rotated;
   table->Rotatef = save_Rotatef;
   table->Scaled = save_Scaled;
   table->Scalef = save_Scalef;
   table->Scissor = save_Scissor;
   table->SelectBuffer = _mesa_SelectBuffer;
   table->ShadeModel = save_ShadeModel;
   table->StencilFunc = save_StencilFunc;
   table->StencilMask = save_StencilMask;
   table->StencilOp = save_StencilOp;
   table->TexCoord1d = _mesa_TexCoord1d;
   table->TexCoord1dv = _mesa_TexCoord1dv;
   table->TexCoord1f = _mesa_TexCoord1f;
   table->TexCoord1fv = _mesa_TexCoord1fv;
   table->TexCoord1i = _mesa_TexCoord1i;
   table->TexCoord1iv = _mesa_TexCoord1iv;
   table->TexCoord1s = _mesa_TexCoord1s;
   table->TexCoord1sv = _mesa_TexCoord1sv;
   table->TexCoord2d = _mesa_TexCoord2d;
   table->TexCoord2dv = _mesa_TexCoord2dv;
   table->TexCoord2f = _mesa_TexCoord2f;
   table->TexCoord2fv = _mesa_TexCoord2fv;
   table->TexCoord2i = _mesa_TexCoord2i;
   table->TexCoord2iv = _mesa_TexCoord2iv;
   table->TexCoord2s = _mesa_TexCoord2s;
   table->TexCoord2sv = _mesa_TexCoord2sv;
   table->TexCoord3d = _mesa_TexCoord3d;
   table->TexCoord3dv = _mesa_TexCoord3dv;
   table->TexCoord3f = _mesa_TexCoord3f;
   table->TexCoord3fv = _mesa_TexCoord3fv;
   table->TexCoord3i = _mesa_TexCoord3i;
   table->TexCoord3iv = _mesa_TexCoord3iv;
   table->TexCoord3s = _mesa_TexCoord3s;
   table->TexCoord3sv = _mesa_TexCoord3sv;
   table->TexCoord4d = _mesa_TexCoord4d;
   table->TexCoord4dv = _mesa_TexCoord4dv;
   table->TexCoord4f = _mesa_TexCoord4f;
   table->TexCoord4fv = _mesa_TexCoord4fv;
   table->TexCoord4i = _mesa_TexCoord4i;
   table->TexCoord4iv = _mesa_TexCoord4iv;
   table->TexCoord4s = _mesa_TexCoord4s;
   table->TexCoord4sv = _mesa_TexCoord4sv;
   table->TexEnvf = save_TexEnvf;
   table->TexEnvfv = save_TexEnvfv;
   table->TexEnvi = save_TexEnvi;
   table->TexEnviv = save_TexEnviv;
   table->TexGend = save_TexGend;
   table->TexGendv = save_TexGendv;
   table->TexGenf = save_TexGenf;
   table->TexGenfv = save_TexGenfv;
   table->TexGeni = save_TexGeni;
   table->TexGeniv = save_TexGeniv;
   table->TexImage1D = save_TexImage1D;
   table->TexImage2D = save_TexImage2D;
   table->TexParameterf = save_TexParameterf;
   table->TexParameterfv = save_TexParameterfv;
   table->TexParameteri = save_TexParameteri;
   table->TexParameteriv = save_TexParameteriv;
   table->Translated = save_Translated;
   table->Translatef = save_Translatef;
   table->Vertex2d = _mesa_Vertex2d;
   table->Vertex2dv = _mesa_Vertex2dv;
   table->Vertex2f = _mesa_Vertex2f;
   table->Vertex2fv = _mesa_Vertex2fv;
   table->Vertex2i = _mesa_Vertex2i;
   table->Vertex2iv = _mesa_Vertex2iv;
   table->Vertex2s = _mesa_Vertex2s;
   table->Vertex2sv = _mesa_Vertex2sv;
   table->Vertex3d = _mesa_Vertex3d;
   table->Vertex3dv = _mesa_Vertex3dv;
   table->Vertex3f = _mesa_Vertex3f;
   table->Vertex3fv = _mesa_Vertex3fv;
   table->Vertex3i = _mesa_Vertex3i;
   table->Vertex3iv = _mesa_Vertex3iv;
   table->Vertex3s = _mesa_Vertex3s;
   table->Vertex3sv = _mesa_Vertex3sv;
   table->Vertex4d = _mesa_Vertex4d;
   table->Vertex4dv = _mesa_Vertex4dv;
   table->Vertex4f = _mesa_Vertex4f;
   table->Vertex4fv = _mesa_Vertex4fv;
   table->Vertex4i = _mesa_Vertex4i;
   table->Vertex4iv = _mesa_Vertex4iv;
   table->Vertex4s = _mesa_Vertex4s;
   table->Vertex4sv = _mesa_Vertex4sv;
   table->Viewport = save_Viewport;

   /* GL 1.1 */
   table->AreTexturesResident = _mesa_AreTexturesResident;
   table->ArrayElement = _mesa_ArrayElement;
   table->BindTexture = save_BindTexture;
   table->ColorPointer = _mesa_ColorPointer;
   table->CopyTexImage1D = save_CopyTexImage1D;
   table->CopyTexImage2D = save_CopyTexImage2D;
   table->CopyTexSubImage1D = save_CopyTexSubImage1D;
   table->CopyTexSubImage2D = save_CopyTexSubImage2D;
   table->DeleteTextures = _mesa_DeleteTextures;
   table->DisableClientState = _mesa_DisableClientState;
   table->DrawArrays = _mesa_DrawArrays;
   table->DrawElements = _mesa_DrawElements;
   table->EdgeFlagPointer = _mesa_EdgeFlagPointer;
   table->EnableClientState = _mesa_EnableClientState;
   table->GenTextures = _mesa_GenTextures;
   table->GetPointerv = _mesa_GetPointerv;
   table->IndexPointer = _mesa_IndexPointer;
   table->Indexub = _mesa_Indexub;
   table->Indexubv = _mesa_Indexubv;
   table->InterleavedArrays = _mesa_InterleavedArrays;
   table->IsTexture = _mesa_IsTexture;
   table->NormalPointer = _mesa_NormalPointer;
   table->PopClientAttrib = _mesa_PopClientAttrib;
   table->PrioritizeTextures = save_PrioritizeTextures;
   table->PushClientAttrib = _mesa_PushClientAttrib;
   table->TexCoordPointer = _mesa_TexCoordPointer;
   table->TexSubImage1D = save_TexSubImage1D;
   table->TexSubImage2D = save_TexSubImage2D;
   table->VertexPointer = _mesa_VertexPointer;

   /* GL 1.2 */
   table->CopyTexSubImage3D = save_CopyTexSubImage3D;
   table->DrawRangeElements = _mesa_DrawRangeElements;
   table->TexImage3D = save_TexImage3D;
   table->TexSubImage3D = save_TexSubImage3D;

   /* GL_ARB_imaging */
   /* Not all are supported */
   table->BlendColor = save_BlendColor;
   table->BlendEquation = save_BlendEquation;
   table->ColorSubTable = save_ColorSubTable;
   table->ColorTable = save_ColorTable;
   table->ColorTableParameterfv = save_ColorTableParameterfv;
   table->ColorTableParameteriv = save_ColorTableParameteriv;
   table->ConvolutionFilter1D = save_ConvolutionFilter1D;
   table->ConvolutionFilter2D = save_ConvolutionFilter2D;
   table->ConvolutionParameterf = save_ConvolutionParameterf;
   table->ConvolutionParameterfv = save_ConvolutionParameterfv;
   table->ConvolutionParameteri = save_ConvolutionParameteri;
   table->ConvolutionParameteriv = save_ConvolutionParameteriv;
   table->CopyColorSubTable = save_CopyColorSubTable;
   table->CopyColorTable = save_CopyColorTable;
   table->CopyConvolutionFilter1D = _mesa_CopyConvolutionFilter1D;
   table->CopyConvolutionFilter2D = _mesa_CopyConvolutionFilter2D;
   table->GetColorTable = _mesa_GetColorTable;
   table->GetColorTableParameterfv = _mesa_GetColorTableParameterfv;
   table->GetColorTableParameteriv = _mesa_GetColorTableParameteriv;
   table->GetConvolutionFilter = _mesa_GetConvolutionFilter;
   table->GetConvolutionParameterfv = _mesa_GetConvolutionParameterfv;
   table->GetConvolutionParameteriv = _mesa_GetConvolutionParameteriv;
   table->GetHistogram = _mesa_GetHistogram;
   table->GetHistogramParameterfv = _mesa_GetHistogramParameterfv;
   table->GetHistogramParameteriv = _mesa_GetHistogramParameteriv;
   table->GetMinmax = _mesa_GetMinmax;
   table->GetMinmaxParameterfv = _mesa_GetMinmaxParameterfv;
   table->GetMinmaxParameteriv = _mesa_GetMinmaxParameteriv;
   table->GetSeparableFilter = _mesa_GetSeparableFilter;
   table->Histogram = save_Histogram;
   table->Minmax = save_Minmax;
   table->ResetHistogram = save_ResetHistogram;
   table->ResetMinmax = save_ResetMinmax;
   table->SeparableFilter2D = _mesa_SeparableFilter2D;

   /* GL_EXT_texture3d */
#if 0
   table->CopyTexSubImage3DEXT = save_CopyTexSubImage3D;
   table->TexImage3DEXT = save_TexImage3DEXT;
   table->TexSubImage3DEXT = save_TexSubImage3D;
#endif

   /* GL_EXT_paletted_texture */
#if 0
   table->ColorTableEXT = save_ColorTable;
   table->ColorSubTableEXT = save_ColorSubTable;
#endif
   table->GetColorTableEXT = _mesa_GetColorTable;
   table->GetColorTableParameterfvEXT = _mesa_GetColorTableParameterfv;
   table->GetColorTableParameterivEXT = _mesa_GetColorTableParameteriv;

   /* 15. GL_SGIX_pixel_texture */
   table->PixelTexGenSGIX = save_PixelTexGenSGIX;

   /* 15. GL_SGIS_pixel_texture */
   table->PixelTexGenParameteriSGIS = save_PixelTexGenParameteriSGIS;
   table->PixelTexGenParameterfSGIS = save_PixelTexGenParameterfSGIS;
   table->PixelTexGenParameterivSGIS = save_PixelTexGenParameterivSGIS;
   table->PixelTexGenParameterfvSGIS = save_PixelTexGenParameterfvSGIS;
   table->GetPixelTexGenParameterivSGIS = _mesa_GetPixelTexGenParameterivSGIS;
   table->GetPixelTexGenParameterfvSGIS = _mesa_GetPixelTexGenParameterfvSGIS;

   /* GL_EXT_compiled_vertex_array */
   table->LockArraysEXT = _mesa_LockArraysEXT;
   table->UnlockArraysEXT = _mesa_UnlockArraysEXT;

   /* GL_EXT_point_parameters */
   table->PointParameterfEXT = save_PointParameterfEXT;
   table->PointParameterfvEXT = save_PointParameterfvEXT;

   /* GL_PGI_misc_hints */
   table->HintPGI = save_HintPGI;

   /* GL_EXT_polygon_offset */
   table->PolygonOffsetEXT = save_PolygonOffsetEXT;

   /* GL_EXT_blend_minmax */
#if 0
   table->BlendEquationEXT = save_BlendEquationEXT;
#endif

   /* GL_EXT_blend_color */
#if 0 
   table->BlendColorEXT = save_BlendColorEXT;
#endif

   /* GL_ARB_multitexture */
   table->ActiveTextureARB = save_ActiveTextureARB;
   table->ClientActiveTextureARB = save_ClientActiveTextureARB;
   table->MultiTexCoord1dARB = _mesa_MultiTexCoord1dARB;
   table->MultiTexCoord1dvARB = _mesa_MultiTexCoord1dvARB;
   table->MultiTexCoord1fARB = _mesa_MultiTexCoord1fARB;
   table->MultiTexCoord1fvARB = _mesa_MultiTexCoord1fvARB;
   table->MultiTexCoord1iARB = _mesa_MultiTexCoord1iARB;
   table->MultiTexCoord1ivARB = _mesa_MultiTexCoord1ivARB;
   table->MultiTexCoord1sARB = _mesa_MultiTexCoord1sARB;
   table->MultiTexCoord1svARB = _mesa_MultiTexCoord1svARB;
   table->MultiTexCoord2dARB = _mesa_MultiTexCoord2dARB;
   table->MultiTexCoord2dvARB = _mesa_MultiTexCoord2dvARB;
   table->MultiTexCoord2fARB = _mesa_MultiTexCoord2fARB;
   table->MultiTexCoord2fvARB = _mesa_MultiTexCoord2fvARB;
   table->MultiTexCoord2iARB = _mesa_MultiTexCoord2iARB;
   table->MultiTexCoord2ivARB = _mesa_MultiTexCoord2ivARB;
   table->MultiTexCoord2sARB = _mesa_MultiTexCoord2sARB;
   table->MultiTexCoord2svARB = _mesa_MultiTexCoord2svARB;
   table->MultiTexCoord3dARB = _mesa_MultiTexCoord3dARB;
   table->MultiTexCoord3dvARB = _mesa_MultiTexCoord3dvARB;
   table->MultiTexCoord3fARB = _mesa_MultiTexCoord3fARB;
   table->MultiTexCoord3fvARB = _mesa_MultiTexCoord3fvARB;
   table->MultiTexCoord3iARB = _mesa_MultiTexCoord3iARB;
   table->MultiTexCoord3ivARB = _mesa_MultiTexCoord3ivARB;
   table->MultiTexCoord3sARB = _mesa_MultiTexCoord3sARB;
   table->MultiTexCoord3svARB = _mesa_MultiTexCoord3svARB;
   table->MultiTexCoord4dARB = _mesa_MultiTexCoord4dARB;
   table->MultiTexCoord4dvARB = _mesa_MultiTexCoord4dvARB;
   table->MultiTexCoord4fARB = _mesa_MultiTexCoord4fARB;
   table->MultiTexCoord4fvARB = _mesa_MultiTexCoord4fvARB;
   table->MultiTexCoord4iARB = _mesa_MultiTexCoord4iARB;
   table->MultiTexCoord4ivARB = _mesa_MultiTexCoord4ivARB;
   table->MultiTexCoord4sARB = _mesa_MultiTexCoord4sARB;
   table->MultiTexCoord4svARB = _mesa_MultiTexCoord4svARB;

   /* GL_EXT_blend_func_separate */
   table->BlendFuncSeparateEXT = save_BlendFuncSeparateEXT;

   /* GL_MESA_window_pos */
   table->WindowPos2dMESA = save_WindowPos2dMESA;
   table->WindowPos2dvMESA = save_WindowPos2dvMESA;
   table->WindowPos2fMESA = save_WindowPos2fMESA;
   table->WindowPos2fvMESA = save_WindowPos2fvMESA;
   table->WindowPos2iMESA = save_WindowPos2iMESA;
   table->WindowPos2ivMESA = save_WindowPos2ivMESA;
   table->WindowPos2sMESA = save_WindowPos2sMESA;
   table->WindowPos2svMESA = save_WindowPos2svMESA;
   table->WindowPos3dMESA = save_WindowPos3dMESA;
   table->WindowPos3dvMESA = save_WindowPos3dvMESA;
   table->WindowPos3fMESA = save_WindowPos3fMESA;
   table->WindowPos3fvMESA = save_WindowPos3fvMESA;
   table->WindowPos3iMESA = save_WindowPos3iMESA;
   table->WindowPos3ivMESA = save_WindowPos3ivMESA;
   table->WindowPos3sMESA = save_WindowPos3sMESA;
   table->WindowPos3svMESA = save_WindowPos3svMESA;
   table->WindowPos4dMESA = save_WindowPos4dMESA;
   table->WindowPos4dvMESA = save_WindowPos4dvMESA;
   table->WindowPos4fMESA = save_WindowPos4fMESA;
   table->WindowPos4fvMESA = save_WindowPos4fvMESA;
   table->WindowPos4iMESA = save_WindowPos4iMESA;
   table->WindowPos4ivMESA = save_WindowPos4ivMESA;
   table->WindowPos4sMESA = save_WindowPos4sMESA;
   table->WindowPos4svMESA = save_WindowPos4svMESA;

   /* GL_MESA_resize_buffers */
   table->ResizeBuffersMESA = _mesa_ResizeBuffersMESA;

   /* GL_ARB_transpose_matrix */
   table->LoadTransposeMatrixdARB = save_LoadTransposeMatrixdARB;
   table->LoadTransposeMatrixfARB = save_LoadTransposeMatrixfARB;
   table->MultTransposeMatrixdARB = save_MultTransposeMatrixdARB;
   table->MultTransposeMatrixfARB = save_MultTransposeMatrixfARB;

   /* ARB 12. GL_ARB_texture_compression */
   table->CompressedTexImage3DARB = save_CompressedTexImage3DARB;
   table->CompressedTexImage2DARB = save_CompressedTexImage2DARB;
   table->CompressedTexImage1DARB = save_CompressedTexImage1DARB;
   table->CompressedTexSubImage3DARB = save_CompressedTexSubImage3DARB;
   table->CompressedTexSubImage2DARB = save_CompressedTexSubImage2DARB;
   table->CompressedTexSubImage1DARB = save_CompressedTexSubImage1DARB;
   table->GetCompressedTexImageARB = _mesa_GetCompressedTexImageARB;
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

   n = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);

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
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            fprintf(f,"ColorTableParameterfv %s %s %f %f %f %f\n",
                    enum_string(n[1].e), enum_string(n[2].e),
                    n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            fprintf(f,"ColorTableParameteriv %s %s %d %d %d %d\n",
                    enum_string(n[1].e), enum_string(n[2].e),
                    n[3].i, n[4].i, n[5].i, n[6].i);
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
         case OPCODE_BIND_TEXTURE:
	    fprintf(f,"BindTexture %s %d\n", gl_lookup_enum_by_nr(n[1].ui), 
		    n[2].ui);
	    break;
         case OPCODE_SHADE_MODEL:
	    fprintf(f,"ShadeModel %s\n", gl_lookup_enum_by_nr(n[1].ui)); 
	    break;

	 /*
	  * meta opcodes/commands
	  */
         case OPCODE_ERROR:
            fprintf(f,"Error: %s %s\n", enum_string(n[1].e), (const char *)n[2].data );
            break;
	 case OPCODE_VERTEX_CASSETTE:
            fprintf(f,"VERTEX-CASSETTE, id %u, rows %u..%u\n", 
		    ((struct immediate *) n[1].data)->id,
		    n[2].ui,
		    n[3].ui);
	    gl_print_cassette( (struct immediate *) n[1].data );
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
   GET_CURRENT_CONTEXT(ctx);
   print_list( ctx, stderr, list );
}
