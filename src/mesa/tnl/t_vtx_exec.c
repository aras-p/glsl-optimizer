/* $XFree86$ */
/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */
#include "api_noop.h"
#include "api_arrayelt.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "enums.h"
#include "glapi.h"
#include "colormac.h"
#include "light.h"
#include "state.h"
#include "vtxfmt.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_array_api.h"

static void _tnl_FlushVertices( GLcontext *, GLuint );


void tnl_copy_to_current( GLcontext *ctx ) 
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint flag = tnl->vertex_format;
   GLint i;

   assert(ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT);

   for (i = 0 ; i < 16 ; i++)
      if (flag & (1<<i))
	 COPY_4FV( ctx->Current.Attrib[i], tnl->attribptr[i] );

   if (flag & VERT_BIT_INDEX)
      ctx->Current.Index = tnl->indexptr[0];

   if (flag & VERT_BIT_EDGEFLAG)
      ctx->Current.EdgeFlag = tnl->edgeflagptr[0];

   if (flag & VERT_BIT_MATERIAL) {
      _mesa_update_material( ctx,
			  IM->Material[IM->LastMaterial],
			  IM->MaterialOrMask );

      tnl->Driver.NotifyMaterialChange( ctx );
   }


   ctx->Driver.NeedFlush &= ~FLUSH_UPDATE_CURRENT;
}

static GLboolean discreet_gl_prim[GL_POLYGON+1] = {
   1,				/* 0 points */
   1,				/* 1 lines */
   0,				/* 2 line_strip */
   0,				/* 3 line_loop */
   1,				/* 4 tris */
   0,				/* 5 tri_fan */
   0,				/* 6 tri_strip */
   1,				/* 7 quads */
   0,				/* 8 quadstrip */
   0,				/* 9 poly */
};

/* Optimize the primitive list:  ONLY FOR EXECUTE ATM
 */
static void optimize_prims( TNLcontext *tnl )
{
   int i, j;

   if (tnl->nrprims <= 1)
      return;

   for (j = 0, i = 1 ; i < tnl->nrprims; i++) {
      int pj = tnl->primlist[j].prim & 0xf;
      int pi = tnl->primlist[i].prim & 0xf;
      
      if (pj == pi && discreet_gl_prim[pj] &&
	  tnl->primlist[i].start == tnl->primlist[j].end) {
	 tnl->primlist[j].end = tnl->primlist[i].end;
      }
      else {
	 j++;
	 if (j != i) tnl->primlist[j] = tnl->primlist[i];
      }
   }

   tnl->nrprims = j+1;
}


/* Bind vertex buffer pointers, run pipeline:
 */
static void flush_prims( TNLcontext *tnl )
{
   int i,j;

   tnl->dma.current.ptr = tnl->dma.current.start += 
      (tnl->initial_counter - tnl->counter) * tnl->vertex_size * 4; 

   tnl->tcl.vertex_format = tnl->vertex_format;
   tnl->tcl.aos_components[0] = &tmp;
   tnl->tcl.nr_aos_components = 1;
   tnl->dma.flush = 0;

   tnl->Driver.RunPipeline( ... );
   
   tnl->nrprims = 0;
}


static void start_prim( TNLcontext *tnl, GLuint mode )
{
   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s %d\n", __FUNCTION__, 
	      tnl->initial_counter - tnl->counter);

   tnl->primlist[tnl->nrprims].start = tnl->initial_counter - tnl->counter;
   tnl->primlist[tnl->nrprims].prim = mode;
}

static void note_last_prim( TNLcontext *tnl, GLuint flags )
{
   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s %d\n", __FUNCTION__, 
	      tnl->initial_counter - tnl->counter);

   if (tnl->prim[0] != GL_POLYGON+1) {
      tnl->primlist[tnl->nrprims].prim |= flags;
      tnl->primlist[tnl->nrprims].end = tnl->initial_counter - tnl->counter;

      if (++tnl->nrprims == TNL_MAX_PRIMS)
	 flush_prims( tnl );
   }
}


static void copy_vertex( TNLcontext *tnl, GLuint n, GLfloat *dst )
{
   GLuint i;
   GLfloat *src = (GLfloat *)(tnl->dma.current.address + 
			      tnl->dma.current.ptr + 
			      (tnl->primlist[tnl->nrprims].start + n) * 
			      tnl->vertex_size * 4);

   if (MESA_VERBOSE & DEBUG_VFMT) 
      _mesa_debug(NULL, "copy_vertex %d\n", 
	      tnl->primlist[tnl->nrprims].start + n);

   for (i = 0 ; i < tnl->vertex_size; i++) {
      dst[i] = src[i];
   }
}

static GLuint copy_wrapped_verts( TNLcontext *tnl, GLfloat (*tmp)[15] )
{
   GLuint ovf, i;
   GLuint nr = (tnl->initial_counter - tnl->counter) - tnl->primlist[tnl->nrprims].start;

   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s %d verts\n", __FUNCTION__, nr);

   switch( tnl->prim[0] )
   {
   case GL_POINTS:
      return 0;
   case GL_LINES:
      ovf = nr&1;
      for (i = 0 ; i < ovf ; i++)
	 copy_vertex( tnl, nr-ovf+i, tmp[i] );
      return i;
   case GL_TRIANGLES:
      ovf = nr%3;
      for (i = 0 ; i < ovf ; i++)
	 copy_vertex( tnl, nr-ovf+i, tmp[i] );
      return i;
   case GL_QUADS:
      ovf = nr&3;
      for (i = 0 ; i < ovf ; i++)
	 copy_vertex( tnl, nr-ovf+i, tmp[i] );
      return i;
   case GL_LINE_STRIP:
      if (nr == 0) 
	 return 0;
      copy_vertex( tnl, nr-1, tmp[0] );
      return 1;
   case GL_LINE_LOOP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      if (nr == 0) 
	 return 0;
      else if (nr == 1) {
	 copy_vertex( tnl, 0, tmp[0] );
	 return 1;
      } else {
	 copy_vertex( tnl, 0, tmp[0] );
	 copy_vertex( tnl, nr-1, tmp[1] );
	 return 2;
      }
   case GL_TRIANGLE_STRIP:
      ovf = MIN2( nr-1, 2 );
      for (i = 0 ; i < ovf ; i++)
	 copy_vertex( tnl, nr-ovf+i, tmp[i] );
      return i;
   case GL_QUAD_STRIP:
      ovf = MIN2( nr-1, 2 );
      if (nr > 2) ovf += nr&1;
      for (i = 0 ; i < ovf ; i++)
	 copy_vertex( tnl, nr-ovf+i, tmp[i] );
      return i;
   default:
      assert(0);
      return 0;
   }
}



/* Extend for vertex-format changes on wrap:
 */
static void wrap_buffer( void )
{
   TNLcontext *tnl = tnl->tnl;
   GLfloat tmp[3][15];
   GLuint i, nrverts;

   if (MESA_VERBOSE & (DEBUG_VFMT|DEBUG_PRIMS))
      _mesa_debug(NULL, "%s %d\n", __FUNCTION__, 
	      tnl->initial_counter - tnl->counter);

   /* Don't deal with parity.  *** WONT WORK FOR COMPILE
    */
   if ((((tnl->initial_counter - tnl->counter) -  
	 tnl->primlist[tnl->nrprims].start) & 1)) {
      tnl->counter++;
      tnl->initial_counter++;
      return;
   }

   /* Copy vertices out of dma:
    */
   nrverts = copy_dma_verts( tnl, tmp );

   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%d vertices to copy\n", nrverts);
   

   /* Finish the prim at this point:
    */
   note_last_prim( tnl, 0 );
   flush_prims( tnl );

   /* Reset counter, dmaptr
    */
   tnl->dmaptr = (int *)(tnl->dma.current.ptr + tnl->dma.current.address);
   tnl->counter = (tnl->dma.current.end - tnl->dma.current.ptr) / 
      (tnl->vertex_size * 4);
   tnl->counter--;
   tnl->initial_counter = tnl->counter;
   tnl->notify = wrap_buffer;

   tnl->dma.flush = flush_prims;
   start_prim( tnl, tnl->prim[0] );


   /* Reemit saved vertices 
    * *** POSSIBLY IN NEW FORMAT
    *       --> Can't always extend at end of vertex?
    */
   for (i = 0 ; i < nrverts; i++) {
      if (MESA_VERBOSE & DEBUG_VERTS) {
	 int j;
	 _mesa_debug(NULL, "re-emit vertex %d to %p\n", i, tnl->dmaptr);
	 if (MESA_VERBOSE & DEBUG_VERBOSE)
	    for (j = 0 ; j < tnl->vertex_size; j++) 
	       _mesa_debug(NULL, "\t%08x/%f\n", *(int*)&tmp[i][j], tmp[i][j]);
      }

      memcpy( tnl->dmaptr, tmp[i], tnl->vertex_size * 4 );
      tnl->dmaptr += tnl->vertex_size;
      tnl->counter--;
   }
}



/* Always follow data, don't try to predict what's necessary.  
 */
static GLboolean check_vtx_fmt( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT) 
      ctx->Driver.FlushVertices( ctx, FLUSH_UPDATE_CURRENT );
   

   TNL_NEWPRIM(tnl);
   tnl->vertex_format = VERT_BIT_POS;
   tnl->prim = &ctx->Driver.CurrentExecPrimitive;


   /* Currently allow the full 4 components per attrib.  Can use the
    * mechanism from radeon driver color handling to reduce this (and
    * also to store ubyte colors where these are incoming).  This
    * won't work for compile mode.
    *
    * Only adding components when they are first received eliminates
    * the need for displaylist fixup, as there are no 'empty' slots
    * at the start of buffers.  
    */
   for (i = 0 ; i < 16 ; i++) {
      if (ind & (1<<i)) {
	 tnl->attribptr[i] = &tnl->vertex[tnl->vertex_size].f;
	 tnl->vertex_size += 4;
	 tnl->attribptr[i][0] = ctx->Current.Attrib[i][0];
	 tnl->attribptr[i][1] = ctx->Current.Attrib[i][1];
	 tnl->attribptr[i][2] = ctx->Current.Attrib[i][2];
	 tnl->attribptr[i][3] = ctx->Current.Attrib[i][3];
      }
      else
	 tnl->attribptr[i] = ctx->Current.Attrib[i];
   }

   /* Edgeflag, Index:
    */
   for (i = 16 ; i < 18 ; i++)
      ;

   /* Materials:
    */
   for (i = 18 ; i < 28 ; i++)
      ;

   /* Eval:
    */
   for (i = 28 ; i < 29 ; i++)
      ;
	   

   if (tnl->installed_vertex_format != tnl->vertex_format) {
      if (MESA_VERBOSE & DEBUG_VFMT)
	 _mesa_debug(NULL, "reinstall on vertex_format change\n");
      _mesa_install_exec_vtxfmt( ctx, &tnl->vtxfmt );
      tnl->installed_vertex_format = tnl->vertex_format;
   }

   return GL_TRUE;
}


void _tnl_InvalidateVtxfmt( GLcontext *ctx )
{
   tnl->recheck = GL_TRUE;
   tnl->fell_back = GL_FALSE;
}




static void _tnl_ValidateVtxfmt( GLcontext *ctx )
{
   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s\n", __FUNCTION__);

   if (ctx->Driver.NeedFlush)
      ctx->Driver.FlushVertices( ctx, ctx->Driver.NeedFlush );

   tnl->recheck = GL_FALSE;

   if (check_vtx_fmt( ctx )) {
      if (!tnl->installed) {
	 if (MESA_VERBOSE & DEBUG_VFMT)
	    _mesa_debug(NULL, "reinstall (new install)\n");

	 _mesa_install_exec_vtxfmt( ctx, &tnl->vtxfmt );
	 ctx->Driver.FlushVertices = _tnl_FlushVertices;
	 tnl->installed = GL_TRUE;
      }
      else
	 _mesa_debug(NULL, "%s: already installed", __FUNCTION__);
   } 
   else {
      if (MESA_VERBOSE & DEBUG_VFMT)
	 _mesa_debug(NULL, "%s: failed\n", __FUNCTION__);

      if (tnl->installed) {
	 if (tnl->tnl->dma.flush)
	    tnl->tnl->dma.flush( tnl->tnl );
	 _tnl_wakeup_exec( ctx );
	 tnl->installed = GL_FALSE;
      }
   }      
}





/* Begin/End
 */
static void _tnl_Begin( GLenum mode )
{
   GLcontext *ctx = tnl->context;
   TNLcontext *tnl = tnl->tnl;
   
   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s\n", __FUNCTION__);

   if (mode > GL_POLYGON) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glBegin" );
      return;
   }

   if (tnl->prim[0] != GL_POLYGON+1) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }
   
   if (ctx->NewState) 
      _mesa_update_state( ctx );

   if (tnl->recheck) 
      _tnl_ValidateVtxfmt( ctx );

   if (tnl->dma.flush && tnl->counter < 12) {
      if (MESA_VERBOSE & DEBUG_VFMT)
	 _mesa_debug(NULL, "%s: flush almost-empty buffers\n", __FUNCTION__);
      flush_prims( tnl );
   }

   if (!tnl->dma.flush) {
      if (tnl->dma.current.ptr + 12*tnl->vertex_size*4 > 
	  tnl->dma.current.end) {
	 TNL_NEWPRIM( tnl );
	 _tnl_RefillCurrentDmaRegion( tnl );
      }

      tnl->dmaptr = (int *)(tnl->dma.current.address + tnl->dma.current.ptr);
      tnl->counter = (tnl->dma.current.end - tnl->dma.current.ptr) / 
	 (tnl->vertex_size * 4);
      tnl->counter--;
      tnl->initial_counter = tnl->counter;
      tnl->notify = wrap_buffer;
      tnl->dma.flush = flush_prims;
      tnl->context->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   }
   
   
   tnl->prim[0] = mode;
   start_prim( tnl, mode | PRIM_BEGIN );
}





static void _tnl_End( void )
{
   TNLcontext *tnl = tnl->tnl;
   GLcontext *ctx = tnl->context;

   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s\n", __FUNCTION__);

   if (tnl->prim[0] == GL_POLYGON+1) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
      return;
   }
	  
   note_last_prim( tnl, PRIM_END );
   tnl->prim[0] = GL_POLYGON+1;
}


static void _tnl_FlushVertices( GLcontext *ctx, GLuint flags )
{
   if (MESA_VERBOSE & DEBUG_VFMT)
      _mesa_debug(NULL, "%s\n", __FUNCTION__);

   assert(tnl->installed);

   if (flags & FLUSH_UPDATE_CURRENT) {
      _tnl_copy_to_current( ctx );
      if (MESA_VERBOSE & DEBUG_VFMT)
	 _mesa_debug(NULL, "reinstall on update_current\n");
      _mesa_install_exec_vtxfmt( ctx, &tnl->vtxfmt );
      ctx->Driver.NeedFlush &= ~FLUSH_UPDATE_CURRENT;
   }

   if (flags & FLUSH_STORED_VERTICES) {
      TNLcontext *tnl = TNL_CONTEXT( ctx );
      assert (tnl->dma.flush == 0 ||
	      tnl->dma.flush == flush_prims);
      if (tnl->dma.flush == flush_prims)
	 flush_prims( TNL_CONTEXT( ctx ) );
      ctx->Driver.NeedFlush &= ~FLUSH_STORED_VERTICES;
   }
}



/* At this point, don't expect very many versions of each function to
 * be generated, so not concerned about freeing them?
 */


static void _tnl_InitVtxfmt( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(tnl->vtxfmt);

   MEMSET( vfmt, 0, sizeof(GLvertexformat) );

   /* Hook in chooser functions for codegen, etc:
    */
   _tnl_InitVtxfmtChoosers( vfmt );

   /* Handled fully in supported states, but no codegen:
    */
   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Rectf = _mesa_noop_Rectf;			/* generic helper */
   vfmt->Begin = _tnl_Begin;
   vfmt->End = _tnl_End;
   
   tnl->context = ctx;
   tnl->tnl = TNL_CONTEXT(ctx);
   tnl->prim = &ctx->Driver.CurrentExecPrimitive;
   tnl->primflags = 0;

   make_empty_list( &tnl->dfn_cache.Vertex2f );
   make_empty_list( &tnl->dfn_cache.Vertex2fv );
   make_empty_list( &tnl->dfn_cache.Vertex3f );
   make_empty_list( &tnl->dfn_cache.Vertex3fv );
   make_empty_list( &tnl->dfn_cache.Color4ub );
   make_empty_list( &tnl->dfn_cache.Color4ubv );
   make_empty_list( &tnl->dfn_cache.Color3ub );
   make_empty_list( &tnl->dfn_cache.Color3ubv );
   make_empty_list( &tnl->dfn_cache.Color4f );
   make_empty_list( &tnl->dfn_cache.Color4fv );
   make_empty_list( &tnl->dfn_cache.Color3f );
   make_empty_list( &tnl->dfn_cache.Color3fv );
   make_empty_list( &tnl->dfn_cache.SecondaryColor3fEXT );
   make_empty_list( &tnl->dfn_cache.SecondaryColor3fvEXT );
   make_empty_list( &tnl->dfn_cache.SecondaryColor3ubEXT );
   make_empty_list( &tnl->dfn_cache.SecondaryColor3ubvEXT );
   make_empty_list( &tnl->dfn_cache.Normal3f );
   make_empty_list( &tnl->dfn_cache.Normal3fv );
   make_empty_list( &tnl->dfn_cache.TexCoord2f );
   make_empty_list( &tnl->dfn_cache.TexCoord2fv );
   make_empty_list( &tnl->dfn_cache.TexCoord1f );
   make_empty_list( &tnl->dfn_cache.TexCoord1fv );
   make_empty_list( &tnl->dfn_cache.MultiTexCoord2fARB );
   make_empty_list( &tnl->dfn_cache.MultiTexCoord2fvARB );
   make_empty_list( &tnl->dfn_cache.MultiTexCoord1fARB );
   make_empty_list( &tnl->dfn_cache.MultiTexCoord1fvARB );

   _tnl_InitCodegen( &tnl->codegen );
}

static void free_funcs( struct dynfn *l )
{
   struct dynfn *f, *tmp;
   foreach_s (f, tmp, l) {
      remove_from_list( f );
      ALIGN_FREE( f->code );
      FREE( f );
   }
}


static void _tnl_DestroyVtxfmt( GLcontext *ctx )
{
   count_funcs();
   free_funcs( &tnl->dfn_cache.Vertex2f );
   free_funcs( &tnl->dfn_cache.Vertex2fv );
   free_funcs( &tnl->dfn_cache.Vertex3f );
   free_funcs( &tnl->dfn_cache.Vertex3fv );
   free_funcs( &tnl->dfn_cache.Color4ub );
   free_funcs( &tnl->dfn_cache.Color4ubv );
   free_funcs( &tnl->dfn_cache.Color3ub );
   free_funcs( &tnl->dfn_cache.Color3ubv );
   free_funcs( &tnl->dfn_cache.Color4f );
   free_funcs( &tnl->dfn_cache.Color4fv );
   free_funcs( &tnl->dfn_cache.Color3f );
   free_funcs( &tnl->dfn_cache.Color3fv );
   free_funcs( &tnl->dfn_cache.SecondaryColor3ubEXT );
   free_funcs( &tnl->dfn_cache.SecondaryColor3ubvEXT );
   free_funcs( &tnl->dfn_cache.SecondaryColor3fEXT );
   free_funcs( &tnl->dfn_cache.SecondaryColor3fvEXT );
   free_funcs( &tnl->dfn_cache.Normal3f );
   free_funcs( &tnl->dfn_cache.Normal3fv );
   free_funcs( &tnl->dfn_cache.TexCoord2f );
   free_funcs( &tnl->dfn_cache.TexCoord2fv );
   free_funcs( &tnl->dfn_cache.TexCoord1f );
   free_funcs( &tnl->dfn_cache.TexCoord1fv );
   free_funcs( &tnl->dfn_cache.MultiTexCoord2fARB );
   free_funcs( &tnl->dfn_cache.MultiTexCoord2fvARB );
   free_funcs( &tnl->dfn_cache.MultiTexCoord1fARB );
   free_funcs( &tnl->dfn_cache.MultiTexCoord1fvARB );
}

