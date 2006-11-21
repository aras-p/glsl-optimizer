/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <stdlib.h>

#include "glheader.h"
#include "context.h"
#include "state.h"
#include "api_validate.h"
#include "enums.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_attrib.h"
#include "brw_context.h"
#include "brw_aub.h"
#include "brw_state.h"
#include "brw_fallback.h"

#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"






static GLuint hw_prim[GL_POLYGON+1] = {
   _3DPRIM_POINTLIST,
   _3DPRIM_LINELIST,
   _3DPRIM_LINELOOP,
   _3DPRIM_LINESTRIP,
   _3DPRIM_TRILIST,
   _3DPRIM_TRISTRIP,
   _3DPRIM_TRIFAN,
   _3DPRIM_QUADLIST,
   _3DPRIM_QUADSTRIP,
   _3DPRIM_POLYGON
};


static const GLenum reduced_prim[GL_POLYGON+1] = {  
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};


/* When the primitive changes, set a state bit and re-validate.  Not
 * the nicest and would rather deal with this by having all the
 * programs be immune to the active primitive (ie. cope with all
 * possibilities).  That may not be realistic however.
 */
static GLuint brw_set_prim(struct brw_context *brw, GLenum prim)
{
   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim));
   
   /* Slight optimization to avoid the GS program when not needed:
    */
   if (prim == GL_QUAD_STRIP &&
       brw->attribs.Light->ShadeModel != GL_FLAT &&
       brw->attribs.Polygon->FrontMode == GL_FILL &&
       brw->attribs.Polygon->BackMode == GL_FILL)
      prim = GL_TRIANGLE_STRIP;

   if (prim != brw->primitive) {
      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim] != brw->intel.reduced_primitive) {
	 brw->intel.reduced_primitive = reduced_prim[prim];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }

      brw_validate_state(brw);
   }

   return hw_prim[prim];
}


static GLuint trim(GLenum prim, GLuint length)
{
   if (prim == GL_QUAD_STRIP)
      return length > 3 ? (length - length % 2) : 0;
   else if (prim == GL_QUADS)
      return length - length % 4;
   else 
      return length;
}


static void brw_emit_cliprect( struct brw_context *brw, 
			       const drm_clip_rect_t *rect )
{
   struct brw_drawrect bdr;

   bdr.header.opcode = CMD_DRAW_RECT;
   bdr.header.length = sizeof(bdr)/4 - 2;
   bdr.xmin = rect->x1;
   bdr.xmax = rect->x2 - 1;
   bdr.ymin = rect->y1;
   bdr.ymax = rect->y2 - 1;
   bdr.xorg = brw->intel.drawX;
   bdr.yorg = brw->intel.drawY;

   intel_batchbuffer_data( brw->intel.batch, &bdr, sizeof(bdr), 
			   INTEL_BATCH_NO_CLIPRECTS);
}


static void brw_emit_prim( struct brw_context *brw, 
			   const struct brw_draw_prim *prim )

{
   struct brw_3d_primitive prim_packet;

   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s %d %d\n", _mesa_lookup_enum_by_nr(prim->mode), 
		   prim->start, prim->count);

   prim_packet.header.opcode = CMD_3D_PRIM;
   prim_packet.header.length = sizeof(prim_packet)/4 - 2;
   prim_packet.header.pad = 0;
   prim_packet.header.topology = brw_set_prim(brw, prim->mode);
   prim_packet.header.indexed = prim->indexed;

   prim_packet.verts_per_instance = trim(prim->mode, prim->count);
   prim_packet.start_vert_location = prim->start;
   prim_packet.instance_count = 1;
   prim_packet.start_instance_location = 0;
   prim_packet.base_vert_location = 0;

   if (prim_packet.verts_per_instance) {
      intel_batchbuffer_data( brw->intel.batch, &prim_packet, sizeof(prim_packet), 
			      INTEL_BATCH_NO_CLIPRECTS);
   }
}



static void update_current_size( struct gl_client_array *array)
{
   const GLfloat *ptr = (const GLfloat *)array->Ptr;

   assert(array->StrideB == 0);
   assert(array->Type == GL_FLOAT || array->Type == GL_UNSIGNED_BYTE);

   if (ptr[3] != 1.0) 
      array->Size = 4;
   else if (ptr[2] != 0.0) 
      array->Size = 3;
   else if (ptr[1] != 0.0) 
      array->Size = 2;
   else
      array->Size = 1;
}



/* Fill in any gaps in passed arrays with pointers to current
 * attributes:
 */
static void brw_merge_inputs( struct brw_context *brw,
		       const struct gl_client_array *arrays[])
{
   struct gl_client_array *current_values = brw->vb.current_values;
   struct brw_vertex_element *inputs = brw->vb.inputs;
   struct brw_vertex_info old = brw->vb.info;
   GLuint i;

   memset(inputs, 0, sizeof(*inputs));
   memset(&brw->vb.info, 0, sizeof(brw->vb.info));

   for (i = 0; i < BRW_ATTRIB_MAX; i++) {
      if (arrays[i] && arrays[i]->Enabled)
      {
	 brw->vb.inputs[i].glarray = arrays[i];
	 brw->vb.info.varying |= (GLuint64EXT) 1 << i;
      }
      else 
      {
	 brw->vb.inputs[i].glarray = &current_values[i];
	 update_current_size(&current_values[i]);
      }

      brw->vb.info.sizes[i/16] |= (inputs[i].glarray->Size - 1) << ((i%16) * 2);
   }

   /* Raise statechanges if input sizes and varying have changed: 
    */
   if (memcmp(brw->vb.info.sizes, old.sizes, sizeof(old.sizes)) != 0)
      brw->state.dirty.brw |= BRW_NEW_INPUT_DIMENSIONS;

   if (brw->vb.info.varying != old.varying)
      brw->state.dirty.brw |= BRW_NEW_INPUT_VARYING;
}

static GLboolean check_fallbacks( struct brw_context *brw,
				  const struct brw_draw_prim *prim,
				  GLuint nr_prims )
{
   GLuint i;

   if (!brw->intel.strict_conformance)
      return GL_FALSE;

   if (brw->attribs.Polygon->SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_TRIANGLES) 
	    return GL_TRUE;
   }

   /* BRW hardware will do AA lines, but they are non-conformant it
    * seems.  TBD whether we keep this fallback:
    */
   if (brw->attribs.Line->SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_LINES) 
	    return GL_TRUE;
   }

   /* Stipple -- these fallbacks could be resolved with a little
    * bit of work?
    */
   if (brw->attribs.Line->StippleFlag) {
      for (i = 0; i < nr_prims; i++) {
	 /* GS doesn't get enough information to know when to reset
	  * the stipple counter?!?
	  */
	 if (prim[i].mode == GL_LINE_LOOP) 
	    return GL_TRUE;
	    
	 if (prim[i].mode == GL_POLYGON &&
	     (brw->attribs.Polygon->FrontMode == GL_LINE ||
	      brw->attribs.Polygon->BackMode == GL_LINE))
	    return GL_TRUE;
      }
   }


   if (brw->attribs.Point->SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (prim[i].mode == GL_POINTS) 
	    return GL_TRUE;
   }
      
   return GL_FALSE;
}


static GLboolean brw_try_draw_prims( GLcontext *ctx,
				     const struct gl_client_array *arrays[],
				     const struct brw_draw_prim *prim,
				     GLuint nr_prims,
				     const struct brw_draw_index_buffer *ib,
				     GLuint min_index,
				     GLuint max_index,
				     GLuint flags )
{
   struct intel_context *intel = intel_context(ctx);
   struct brw_context *brw = brw_context(ctx);
   GLboolean retval = GL_FALSE;
   GLuint i, j;

   if (ctx->NewState)
      _mesa_update_state( ctx );
      
   /* Bind all inputs, derive varying and size information:
    */
   brw_merge_inputs( brw, arrays );

   /* Have to validate state quite late.  Will rebuild tnl_program,
    * which depends on varying information.  
    * 
    * Note this is where brw->vs->prog_data.inputs_read is calculated,
    * so can't access it earlier.
    */

   LOCK_HARDWARE(intel);

   if (brw->intel.numClipRects == 0) {
      assert(intel->batch->ptr == intel->batch->map + intel->batch->offset);
      UNLOCK_HARDWARE(intel);
      return GL_TRUE;
   }

   {
      assert(intel->locked);
      


      /* Set the first primitive early, ahead of validate_state:
       */
      brw_set_prim(brw, prim[0].mode);

      /* XXX:  Need to separate validate and upload of state.  
       */
      brw_validate_state( brw );

      /* Various fallback checks:
       */
      if (brw->intel.Fallback) 
	 goto out;

      if (check_fallbacks( brw, prim, nr_prims ))
	 goto out;
	  
      /* Upload index, vertex data: 
       */
      if (ib)
	 brw_upload_indices( brw, ib );

      if (!brw_upload_vertices( brw, min_index, max_index)) {
	 goto out;
      }

      /* For single cliprect, state is already emitted: 
       */
      if (brw->intel.numClipRects == 1) {
	 for (i = 0; i < nr_prims; i++) {
	    brw_emit_prim(brw, &prim[i]);   
	 }
      }
      else {
	 /* Otherwise, explicitly do the cliprects at this point:
	  */
	 for (j = 0; j < brw->intel.numClipRects; j++) {
	    brw_emit_cliprect(brw, &brw->intel.pClipRects[j]);

	    /* Emit prims to batchbuffer: 
	     */
	    for (i = 0; i < nr_prims; i++) {
	       brw_emit_prim(brw, &prim[i]);   
	    }
	 }
      }
      
      intel->need_flush = GL_TRUE;
      retval = GL_TRUE;
   }

 out:

   /* Currently have to do this to synchronize with the map/unmap of
    * the vertex buffer in brw_exec_api.c.  Not sure if there is any
    * way around this, as not every flush is due to a buffer filling
    * up.
    */
   if (!intel_batchbuffer_flush( brw->intel.batch )) {
      DBG("%s intel_batchbuffer_flush failed\n", __FUNCTION__);
      retval = GL_FALSE;
   }

   if (retval && intel->thrashing) {
      bmSetFence(intel);
   }

   /* Free any old data so it doesn't clog up texture memory - we
    * won't be referencing it again.
    */
   while (brw->vb.upload.wrap != brw->vb.upload.buf) {
      ctx->Driver.BufferData(ctx,
			     GL_ARRAY_BUFFER_ARB,
			     BRW_UPLOAD_INIT_SIZE,
			     NULL,
			     GL_DYNAMIC_DRAW_ARB,
			     brw->vb.upload.vbo[brw->vb.upload.wrap]);
      brw->vb.upload.wrap++;
      brw->vb.upload.wrap %= BRW_NR_UPLOAD_BUFS;
   }

   UNLOCK_HARDWARE(intel);

   if (!retval)
      DBG("%s failed\n", __FUNCTION__);

   return retval;
}


GLboolean brw_draw_prims( GLcontext *ctx,
			  const struct gl_client_array *arrays[],
			  const struct brw_draw_prim *prim,
			  GLuint nr_prims,
			  const struct brw_draw_index_buffer *ib,
			  GLuint min_index,
			  GLuint max_index,
			  GLuint flags )
{
   struct intel_context *intel = intel_context(ctx);
   GLboolean retval;

   retval = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index, flags);

   
   if (!retval && bmError(intel)) {

      DBG("retrying\n");
      /* This looks like out-of-memory but potentially we have
       * situation where there is enough memory but it has become
       * fragmented.  Clear out all heaps and start from scratch by
       * faking a contended lock event:  (done elsewhere)
       */

      /* Then try a second time only to upload textures and draw the
       * primitives:
       */
      retval = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index, flags);
   }

   if (intel->aub_file && (INTEL_DEBUG & DEBUG_SYNC)) {
      intelFinish( &intel->ctx );
      intel->aub_wrap = 1;
   }


   return retval;
}


static void brw_invalidate_vbo_cb( struct intel_context *intel, void *ptr )
{
   /* nothing to do, we don't rely on the contents being preserved */
}


void brw_draw_init( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;
   
   brw->vb.upload.size = BRW_UPLOAD_INIT_SIZE;

   for (i = 0; i < BRW_NR_UPLOAD_BUFS; i++) {
      brw->vb.upload.vbo[i] = ctx->Driver.NewBufferObject(ctx, 1, GL_ARRAY_BUFFER_ARB);
      
      /* XXX: Set these to no-backing-store
       */
      bmBufferSetInvalidateCB(&brw->intel,
			      intel_bufferobj_buffer(intel_buffer_object(brw->vb.upload.vbo[i])),
			      brw_invalidate_vbo_cb,
			      &brw->intel,
			      GL_TRUE);

   }

   ctx->Driver.BufferData( ctx, 
			   GL_ARRAY_BUFFER_ARB, 
			   BRW_UPLOAD_INIT_SIZE,
			   NULL,
			   GL_DYNAMIC_DRAW_ARB,
			   brw->vb.upload.vbo[0] );
						     
   
   brw_init_current_values(ctx, brw->vb.current_values);
}

void brw_draw_destroy( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;
   
   for (i = 0; i < BRW_NR_UPLOAD_BUFS; i++)
      ctx->Driver.DeleteBuffer(ctx, brw->vb.upload.vbo[i]);
}
